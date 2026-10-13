import logging
import asyncio
from aiohttp import web
import threading

class HTTPSServer:
    """
    异步 HTTPS 音频流服务器。
    
    功能：
    1. 启动一个轻量级 HTTP 服务器。
    2. 提供 /audio 接口，供客户端（如播放器）连接。
    3. 接收来自 TTS 模块的音频数据块，并通过 HTTP 流式下发给客户端。
    
    流程解析：
    1. [外部输入] main.py 中的 Worker 调用 push_audio() 将 TTS 生成的音频数据放入 audio_queue。
    2. [客户端连接] 外部播放器访问 http://<ip>:<port>/audio。
    3. [流式响应] handle_stream() 被触发，建立 HTTP 长连接。
    4. [数据转发] handle_stream() 循环从 audio_queue 取出数据，实时写入 HTTP 响应流。
    """
    def __init__(self, loop, host='0.0.0.0', port=8080):
        self.loop = loop
        self.host = host
        self.port = port
        self.app = web.Application()
        # 注册路由：当访问 /audio 时，调用 handle_stream 处理
        self.app.router.add_get('/audio', self.handle_stream)
        self.runner = None
        self.site = None
        # 音频数据队列：用于连接 TTS 生成端（生产者）和 HTTP 下载端（消费者）
        # 必须在 loop 中初始化，或者在使用前初始化
        self.audio_queue = None
        self.logger = logging.getLogger('HTTPSServer')
        self.running = False
        # 外部回调：播放结束时调用
        self.on_play_finished = None

    async def start(self):
        """
        启动 HTTPS 服务器。
        使用 aiohttp 提供的异步启动方式，非阻塞。
        """
        # 在 loop 线程中初始化 Queue，确保事件循环一致性
        if self.audio_queue is None:
            self.audio_queue = asyncio.Queue()
            
        self.runner = web.AppRunner(self.app)
        await self.runner.setup()
        self.site = web.TCPSite(self.runner, self.host, self.port)
        await self.site.start()
        self.running = True
        self.logger.info(f"HTTPS 服务器已启动: http://{self.host}:{self.port}/audio")

    async def stop(self):
        """停止服务器并释放资源"""
        self.running = False
        if self.site:
            await self.site.stop()
        if self.runner:
            await self.runner.cleanup()
        self.logger.info("HTTPS 服务器已停止")

    def push_audio(self, audio_data):
        """
        [生产者接口] 将音频数据推送到队列中。
        
        参数:
            audio_data: TTS 生成的二进制音频片段。传入 None 表示本轮对话结束。
            
        说明:
            此方法由 ROS 节点的主线程或 Worker 线程调用。
            使用 call_soon_threadsafe 确保跨线程操作 asyncio.Queue 的安全性。
        """
        def _put_data():
            if self.audio_queue is not None:
                self.audio_queue.put_nowait(audio_data)
        
        try:
            # 确保在服务器的事件循环中执行 put 操作
            if self.loop.is_running():
                self.loop.call_soon_threadsafe(_put_data)
        except RuntimeError:
            pass

    async def handle_stream(self, request):
        """
        [消费者接口] 处理音频流请求。
        
        流程:
        1. 客户端发起 HTTP GET 请求。
        2. 服务器返回 200 OK 和流式响应头 (Transfer-Encoding: chunked)。
        3. 进入循环：
           - 等待 audio_queue 中出现新的音频数据 (await get())。
           - 将数据写入 HTTP 响应流 (await response.write())。
           - 如果队列中收到 None，表示流结束，断开连接。
        
        注意：
        - 这是一个异步生成器模式，实现了数据的实时透传。
        - 只要 TTS 在不断产生数据，客户端就能不断听到声音，无需等待全部合成完成。
        """
        self.logger.info("客户端连接到音频流")
        
        # 再次确保队列已初始化（防御性编程）
        if self.audio_queue is None:
             self.audio_queue = asyncio.Queue()

        # 创建流式响应对象
        response = web.StreamResponse(
            status=200,
            reason='OK',
            headers={
                'Content-Type': 'audio/mp3',  # 根据 TTS 格式可能是 audio/wav
                'Cache-Control': 'no-cache',  # 禁止缓存，确保实时性
                'Connection': 'keep-alive',
            }
        )
        # 发送响应头，建立连接
        await response.prepare(request)

        try:
            while self.running:
                # 异步等待音频数据：这里会挂起，直到 TTS 推送新数据
                data = await self.audio_queue.get()
                
                if data is None: 
                    # 收到结束信号，说明本轮对话的所有音频已发完
                    # 触发播放结束回调
                    if self.on_play_finished and self.loop.is_running():
                        self.loop.call_soon(self.on_play_finished)
                    self.audio_queue.task_done()
                    # 继续保持连接还是断开？通常播放器读完会断开。
                    # 或者我们可以主动断开来告知播放器结束。
                    break
                
                # 将二进制音频数据写入 HTTP 流
                await response.write(data)
                self.audio_queue.task_done()
                
        except Exception as e:
            # 客户端断开连接（如播放器停止播放）会触发异常
            self.logger.error(f"流连接关闭: {e}")
        finally:
            self.logger.info("客户端已断开")
        
        return response
