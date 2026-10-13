import logging
import json
import time
import asyncio
import aiohttp
# import websockets
import base64

class BaiduTTSClient:
    def __init__(self, api_key, secret_key, voice_id=None):
        self.api_key = api_key
        self.secret_key = secret_key
        self.voice_id = voice_id
        self.access_token = None
        self.token_expires_at = 0
        self.logger = logging.getLogger('BaiduTTSClient')
        # 百度大模型声音复刻流式合成地址
        self.wss_url = "wss://aip.baidubce.com/ws/2.0/speech/publiccloudspeech/v1/voice/clone/tts"

    async def _refresh_token(self):
        """获取或刷新 Access Token"""
        if self.access_token and time.time() < self.token_expires_at:
            return

        url = "https://aip.baidubce.com/oauth/2.0/token"
        params = {
            "grant_type": "client_credentials",
            "client_id": self.api_key,
            "client_secret": self.secret_key
        }
        
        async with aiohttp.ClientSession() as session:
            async with session.post(url, params=params) as resp:
                data = await resp.json()
                if "access_token" in data:
                    self.access_token = data["access_token"]
                    self.token_expires_at = time.time() + data.get("expires_in", 2592000) - 60
                    self.logger.info("Access Token 已刷新")
                else:
                    self.logger.error(f"获取 Access Token 失败: {data}")
                    raise Exception("百度鉴权失败")

    async def synthesize(self, text, on_audio_chunk, spd=7, pit=5, vol=5, aue=3, lang="zh", dialect=""):
        """
        调用百度流式 TTS (复刻音色版)
        :param text: 要合成的文本
        :param on_audio_chunk: 回调函数，接收 bytes 音频数据
        :param spd: 语速 0-15
        :param pit: 音调 0-15
        :param vol: 音量 0-15
        :param aue: 音频格式 3=mp3, 6=wav
        :param lang: 语言 zh=中文
        :param dialect: 方言 (wuu-CN-shanghai, zh-CN-henan, zh-CN-sichuan, etc.)
        """
        if not text:
            return

        await self._refresh_token()
     # 拼接 URL 参数
        url = f"{self.wss_url}?access_token={self.access_token}&voice_id={self.voice_id}"
        
        try:
            # 解决 Python 3.10+ loop 参数移除问题
            # 改用 aiohttp 的 WebSocket 客户端，避免 websockets 库的老版本兼容性问题
            async with aiohttp.ClientSession() as session:
                async with session.ws_connect(url) as websocket:
                    # 1. 发送开始参数
                    start_payload = {
                        "type": "system.start",
                        "payload": {
                            "spd": spd,
                            "pit": pit,
                            "vol": vol,
                            "aue": aue,
                            "lang": lang,
                        },
                    }
                    
                    # 如果指定了方言，加入 payload
                    if dialect:
                        start_payload["payload"]["dialect"] = dialect
                    
                    await websocket.send_json(start_payload)
                    
                    # 2. 发送文本
                    text_payload = {
                        "type": "text",
                        "payload": {"text": text},
                    }
                    await websocket.send_json(text_payload)
                    self.logger.info(f"发送 TTS 请求文本: {text[:10]}...")

                    # 3. 发送结束信号
                    finish_payload = {"type": "system.finish"}
                    await websocket.send_json(finish_payload)

                    # 4. 接收循环
                    async for msg in websocket:
                        if msg.type == aiohttp.WSMsgType.TEXT:
                            msg_json = json.loads(msg.data)
                            # 检查系统消息
                            if msg_json.get("type") == "system.finish":
                                self.logger.info("TTS 合成结束")
                                break
                            if msg_json.get("type") == "system.error":
                                self.logger.error(f"TTS 错误: {msg_json}")
                                
                        elif msg.type == aiohttp.WSMsgType.BINARY:
                            # 二进制音频流
                            if on_audio_chunk:
                                on_audio_chunk(msg.data)
                        elif msg.type == aiohttp.WSMsgType.ERROR:
                            self.logger.error(f"WebSocket 连接异常关闭: {websocket.exception()}")
                            break
                            
        except Exception as e:
            self.logger.error(f"TTS WebSocket 异常: {e}")


    async def create_voice(self, voice_name, audio_base64, voice_desc=""):
        """
        创建百度大模型声音复刻音色
        :param voice_name: 音色名称
        :param audio_base64: Base64 编码的音频内容
        :param voice_desc: 音色描述
        :return: 成功返回 voice_id，失败抛出异常
        """
        await self._refresh_token()
        
        url = "https://aip.baidubce.com/rest/2.0/speech/publiccloudspeech/v1/voice/clone/create"
        params = {"access_token": self.access_token}
        
        payload = {
            "voice_name": voice_name,
            "voice_desc": voice_desc,
            "audio_file": audio_base64,
            # "text_id": "" # 可选
        }
        
        headers = {'Content-Type': 'application/json'}
        
        async with aiohttp.ClientSession() as session:
            async with session.post(url, params=params, json=payload, headers=headers) as resp:
                data = await resp.json()
                if resp.status == 200 and data.get("status") == 0:
                    self.logger.info(f"音色创建成功: {data}")
                    return data.get("data", {}).get("voice_id")
                else:
                    self.logger.error(f"音色创建失败: {data}")
                    raise Exception(f"Create Voice Failed: {data}")

    async def query_voice_list(self, page=1, per_page=10):
        """
        查询音色列表
        :param page: 页码，默认1
        :param per_page: 每页数量，默认10
        :return: 音色列表数据
        """
        await self._refresh_token()
        
        url = "https://aip.baidubce.com/rest/2.0/speech/publiccloudspeech/v1/voice/clone/list"
        params = {
            "access_token": self.access_token,
            "page": page,
            "per_page": per_page
        }
        
        async with aiohttp.ClientSession() as session:
            async with session.post(url, params=params) as resp:
                data = await resp.json()
                if resp.status == 200 and data.get("status") == 0:
                    self.logger.info(f"查询音色列表成功: {data}")
                    return data.get("data", {})
                else:
                    self.logger.error(f"查询音色列表失败: {data}")
                    raise Exception(f"Query Voice List Failed: {data}")

    async def query_voice_detail(self, voice_id):
        """
        查询音色详情 (通过列表过滤实现)
        :param voice_id: 音色ID
        :return: 音色详情字典或None
        """
        # 由于API文档未明确提供详情接口，此处通过遍历列表实现
        # 注意：如果音色在很后面的页码，可能需要遍历多次，这里仅简单实现查询第一页或假设用户知道在哪一页
        # 或者我们可以循环查询直到找到。为了效率，先查第一页。
        # 实际生产中应确认是否有直接详情接口。
        
        # 尝试查询前几页
        for p in range(1, 6):
            data = await self.query_voice_list(page=p, per_page=20)
            voice_list = data.get("items", [])
            if not voice_list:
                break
            
            for voice in voice_list:
                # 假设返回的数据中有 voice_id 字段
                if str(voice.get("voice_id")) == str(voice_id):
                    return voice
            
            # 如果当前页不满，说明没有更多数据了
            if len(voice_list) < 20:
                break
                
        self.logger.warning(f"未找到音色ID: {voice_id}")
        return None

    async def delete_voice(self, voice_id):
        """
        删除音色
        :param voice_id: 音色ID
        :return: 成功返回True
        """
        await self._refresh_token()
        
        # 推测的删除接口地址，根据 create/list 命名规范推断
        url = "https://aip.baidubce.com/rest/2.0/speech/publiccloudspeech/v1/voice/clone/delete"
        params = {
            "access_token": self.access_token,
            "voice_id": voice_id
        }
        
        async with aiohttp.ClientSession() as session:
            async with session.post(url, params=params) as resp:
                data = await resp.json()
                if resp.status == 200 and data.get("status") == 0:
                    self.logger.info(f"删除音色成功: {voice_id}")
                    return True
                else:
                    self.logger.error(f"删除音色失败: {data}")
                    raise Exception(f"Delete Voice Failed: {data}")


if __name__ == "__main__":
    async def test_main():
        import os
        
        # 配置日志输出到控制台
        logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(name)s - %(levelname)s - %(message)s')
        
        # ==========================================
        # 通过 BAIDU_API_KEY / BAIDU_SECRET_KEY / BAIDU_VOICE_ID 环境变量配置
        # ==========================================
        api_key = os.environ.get("BAIDU_API_KEY", "")
        secret_key = os.environ.get("BAIDU_SECRET_KEY", "")
        voice_id = os.environ.get("BAIDU_VOICE_ID", "")
        text = "What is my perpose? Oh, my god! I am not a programmed for friendship!"
        output_file = "test_output.mp3"
        
        # 创建音色参数
        voice_name = "机器人音色"
        voice_desc = "机器人音色复刻"
        base64_file_path = os.environ.get("BAIDU_VOICE_SAMPLE", os.path.join(os.path.dirname(__file__), "base64_robot.txt"))
        
        # 设置模式：
        run_create_voice = False  # True 为创建音色
        run_tts = True # True 为测试 TTS 合成
        run_list_voice = False # True 为查询音色列表
        run_query_detail = False # True 为查询音色详情
        run_delete_voice = False # True 为删除音色
        
        # ==========================================
        
        if not api_key or not secret_key:
            print("错误：请设置 BAIDU_API_KEY 和 BAIDU_SECRET_KEY 环境变量")
            return

        client = BaiduTTSClient(api_key, secret_key, voice_id)
        
        if run_create_voice:
            # ... 创建音色逻辑保持不变 ...
            print(f"正在读取 Base64 音频文件: {base64_file_path}")
            try:
                with open(base64_file_path, "r") as f:
                    audio_base64 = f.read().strip()
                
                print("正在请求创建音色...")
                voice_id = await client.create_voice(voice_name, audio_base64, voice_desc)
                print(f"\n[成功] 音色创建成功！Voice ID: {voice_id}")
                print(f"请保存此 Voice ID 用于后续 TTS 合成。")
                
            except Exception as e:
                print(f"\n[失败] 创建音色发生错误: {e}")
        
        if run_list_voice:
            print("\n正在查询音色列表...")
            try:
                data = await client.query_voice_list()
                print(f"音色列表: {json.dumps(data, ensure_ascii=False, indent=2)}")
            except Exception as e:
                print(f"查询列表失败: {e}")

        if run_query_detail and voice_id:
            print(f"\n正在查询音色详情: {voice_id}")
            try:
                detail = await client.query_voice_detail(voice_id)
                if detail:
                    print(f"音色详情: {json.dumps(detail, ensure_ascii=False, indent=2)}")
                else:
                    print("未找到该音色")
            except Exception as e:
                print(f"查询详情失败: {e}")

        if run_delete_voice and voice_id:
             print(f"\n正在删除音色: {voice_id}")
             try:
                 success = await client.delete_voice(voice_id)
                 if success:
                     print("删除成功")
             except Exception as e:
                 print(f"删除失败: {e}")

        if run_tts:
             # 测试 TTS 合成
             if not voice_id:
                 print("错误：TTS 测试需要提供 voice_id")
                 return

                 
             print(f"\n开始合成文本: {text}")
             print(f"目标输出文件: {output_file}")
             print("正在连接百度云 API...")
             
             try:
                 with open(output_file, "wb") as f:
                     def save_audio(chunk):
                         f.write(chunk)
                         print(".", end="", flush=True)
                         
                     await client.synthesize(text, save_audio)
                     
                 print(f"\n\n合成完成！音频已保存至: {os.path.abspath(output_file)}")
             except Exception as e:
                 print(f"\n合成失败: {e}")

    try:
        asyncio.run(test_main())
    except KeyboardInterrupt:
        pass
