import rclpy
from rclpy.node import Node
from std_msgs.msg import String, UInt8
import asyncio
import threading
import logging
import re

from audio_tts.https_server import HTTPSServer
from audio_tts.tts_api import BaiduTTSClient

class AudioTTSNode(Node):
    def __init__(self, loop):
        super().__init__('audio_tts', allow_undeclared_parameters=True, automatically_declare_parameters_from_overrides=True)
        self.loop = loop
        
        # 参数读取与校验
        # 按照 audio_asr 的规范，使用严格的参数获取方法
        self.llm_topic = self._require_str('sub_llm_topic')
        self.api_key = self._require_str('baidu_api_key')
        self.secret_key = self._require_str('baidu_secret_key')
        self.voice_id = self._require_str('baidu_voice_id')
        self.http_port = self._require_int('http_port')

        # 可选参数
        self.spd = self.get_parameter('spd').value if self.has_parameter('spd') else 5
        self.pit = self.get_parameter('pit').value if self.has_parameter('pit') else 5
        self.vol = self.get_parameter('vol').value if self.has_parameter('vol') else 5
        self.aue = self.get_parameter('aue').value if self.has_parameter('aue') else 3
        self.lang = self.get_parameter('lang').value if self.has_parameter('lang') else 'zh'
        self.dialect = self.get_parameter('dialect').value if self.has_parameter('dialect') else ''

        # 初始化组件
        self.server = HTTPSServer(loop=self.loop, port=self.http_port)
        # 注册播放完成回调
        self.server.on_play_finished = self._on_https_play_finished
        
        self.tts_client = BaiduTTSClient(self.api_key, self.secret_key, self.voice_id)

        # 音频状态发布话题
        self.audio_status_topic = self._require_str('pub_audio_status_topic')
        self.pub_status = self.create_publisher(UInt8, self.audio_status_topic, 10)
        
        # 创建文本处理队列和 worker
        self.text_queue = asyncio.Queue()
        self.processing_task = self.loop.create_task(self._process_text_queue())
        
        # 启动 HTTP Server
        asyncio.run_coroutine_threadsafe(self.server.start(), self.loop)
        
        # 订阅 LLM 结果
        self.subscription = self.create_subscription(
            String,
            self.llm_topic,
            self.listener_callback,
            10)
        
        self.text_buffer = ""
        # 简单的分句标点正则表达式
        self.punctuations = re.compile(r'[。，？！,?!.\n]+')
        
        self.get_logger().info(f'Audio TTS 节点已启动。监听话题: {self.llm_topic}, HTTP 服务端口: {self.http_port}')

    def _on_https_play_finished(self):
        """当 HTTPS 播放结束时调用的回调"""
        self.get_logger().info("HTTPS 播放结束，发送状态 3")
        self.pub_status.publish(UInt8(data=3))

    def _require_str(self, name: str) -> str:
        """
        获取必选字符串参数，若缺失或类型错误则报错退出。
        """
        p = self.get_parameter(name)
        v = p.value
        if isinstance(v, str) and len(v) > 0:
            self.get_logger().debug(f"参数 '{name}' 值为 {v}")
            return v
        if v is None or (isinstance(v, str) and len(v) == 0):
            self.get_logger().fatal(f"缺少必需参数 '{name}'（字符串）")
            raise RuntimeError(f"缺少必需参数 '{name}'（字符串）")
        self.get_logger().fatal(f"参数 '{name}' 类型不匹配，期望字符串，实际为 {type(v).__name__}")
        raise RuntimeError(f"参数 '{name}' 类型不匹配")

    def _require_int(self, name: str) -> int:
        """
        获取必选整数参数，若缺失或类型错误则报错退出。
        """
        p = self.get_parameter(name)
        v = p.value
        if isinstance(v, int):
            self.get_logger().debug(f"参数 '{name}' 值为 {v}")
            return v
        if v is None:
            self.get_logger().fatal(f"缺少必需参数 '{name}'（整数）")
            raise RuntimeError(f"缺少必需参数 '{name}'（整数）")
        self.get_logger().fatal(f"参数 '{name}' 类型不匹配，期望整数，实际为 {type(v).__name__}")
        raise RuntimeError(f"参数 '{name}' 类型不匹配")

    def _require_float(self, name: str) -> float:
        """
        获取必选浮点数参数，若缺失或类型错误则报错退出。
        """
        p = self.get_parameter(name)
        v = p.value
        if isinstance(v, float):
            self.get_logger().debug(f"参数 '{name}' 值为 {v}")
            return v
        if v is None:
            self.get_logger().fatal(f"缺少必需参数 '{name}'（浮点数）")
            raise RuntimeError(f"缺少必需参数 '{name}'（浮点数）")
        self.get_logger().fatal(f"参数 '{name}' 类型不匹配，期望浮点数，实际为 {type(v).__name__}")
        raise RuntimeError(f"参数 '{name}' 类型不匹配")

    async def _process_text_queue(self):
        """
        后台 worker：顺序处理文本队列中的合成请求。
        确保句子按照顺序合成，避免并发导致的音频乱序。
        """
        self.get_logger().info("文本合成 Worker 已启动")
        while True:
            try:
                text = await self.text_queue.get()
                
                # 检查是否为结束标记
                if text is None:
                    self.get_logger().info("TTS 合成全部完成，发送状态 2")
                    self.pub_status.publish(UInt8(data=2))
                    # 告知 Server 音频流已结束
                    self.server.push_audio(None)
                    self.text_queue.task_done()
                    continue
                
                self.get_logger().info(f'Worker 开始合成: {text[:10]}...')
                
                # 显式使用 loop 调用 async 方法
                # 注意：synthesize 是 async def，应该 await
                await self.tts_client.synthesize(
                    text, 
                    self.server.push_audio, # 调用 HTTPServer 的 push_audio 方法
                    spd=self.spd,
                    pit=self.pit,
                    vol=self.vol,
                    aue=self.aue,
                    lang=self.lang,
                    dialect=self.dialect
                )
                
                self.text_queue.task_done()
            except Exception as e:
                # 捕获异常，防止 Worker 退出
                self.get_logger().error(f"Worker 合成异常: {e}")
                # 确保任务标记为完成，防止 join 阻塞
                try:
                    self.text_queue.task_done()
                except ValueError:
                    pass

    def listener_callback(self, msg):
        """
        监听 LLM 输出的回调函数。
        收到文本后，进行分句并调用 TTS。
        """
        text = msg.data
        if not text:
            return
            
        self.get_logger().debug(f'收到 LLM 文本: {text}')
        
        # 处理结束信号
        if text == "END":
            if self.text_buffer:
                self.get_logger().info(f'合成缓冲区剩余文本 (END 信号): {self.text_buffer}')
                # 将剩余文本放入队列
                self.loop.call_soon_threadsafe(self.text_queue.put_nowait, self.text_buffer)
                self.text_buffer = ""
            
            # 发送结束标记给 Worker，表示本轮对话结束
            self.loop.call_soon_threadsafe(self.text_queue.put_nowait, None)
            return

        self.text_buffer += text
        
        # 检查标点符号进行分句
        # 注意：这里简单的处理流式文本，实际可能需要更复杂的缓冲策略
        while True:
            match = self.punctuations.search(self.text_buffer)
            if match:
                end_pos = match.end()
                sentence = self.text_buffer[:end_pos]
                self.text_buffer = self.text_buffer[end_pos:]
                
                # 将分好的句子放入队列
                self.get_logger().debug(f'分句加入队列: {sentence}')
                self.loop.call_soon_threadsafe(self.text_queue.put_nowait, sentence)
            else:
                break

def main(args=None):
    rclpy.init(args=args)
    
    # 创建新的事件循环并在独立线程中运行
    # 这是为了让 asyncio (aiohttp/websockets) 和 rclpy.spin 并行工作
    loop = asyncio.new_event_loop()
    asyncio.set_event_loop(loop)
    
    def run_loop(loop):
        asyncio.set_event_loop(loop)
        loop.run_forever()
        
    loop_thread = threading.Thread(target=run_loop, args=(loop,), daemon=True)
    loop_thread.start()
    
    try:
        node = AudioTTSNode(loop)
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    except Exception as e:
        # 如果是参数错误等，这里会捕获
        logging.error(f"Node execution failed: {e}")
    finally:
        # 清理资源
        if 'node' in locals():
            # 停止 Server
            if hasattr(node, 'server'):
                asyncio.run_coroutine_threadsafe(node.server.stop(), loop)
            node.destroy_node()
            
        rclpy.shutdown()
        loop.call_soon_threadsafe(loop.stop)
        loop_thread.join(timeout=1.0)

if __name__ == '__main__':
    main()
