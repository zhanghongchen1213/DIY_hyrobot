#!/usr/bin/env python3
import asyncio
import aiohttp
import sys
import time
import os

async def save_stream_segmented(url, output_dir="debug_audio"):
    """
    连接 HTTP 音频流，并根据静默间隔自动分段保存为文件。
    方便调试 TTS 的输出顺序和内容。
    """
    if not os.path.exists(output_dir):
        os.makedirs(output_dir)
        
    print(f"正在连接音频流: {url}")
    print(f"音频将自动分段保存到目录: {output_dir}/")
    print("等待数据中... (超过 1.5 秒无数据将自动分段)")
    
    # 设置无限超时，防止长连接被切断
    timeout_settings = aiohttp.ClientTimeout(total=None, connect=None, sock_connect=None, sock_read=None)
    
    try:
        async with aiohttp.ClientSession(timeout=timeout_settings) as session:
            async with session.get(url) as response:
                if response.status != 200:
                    print(f"连接失败，状态码: {response.status}")
                    return

                print("连接成功！监听中...")
                
                current_file = None
                file_counter = 1
                is_writing = False
                
                # 获取 reader 对象
                reader = response.content
                
                while True:
                    try:
                        # 尝试在 1.5 秒内读取数据
                        # 如果 TTS 在合成中，数据流应该是连续的
                        # 如果一句话结束，通常会有停顿
                        chunk = await asyncio.wait_for(reader.read(4096), timeout=1.5)
                        
                        if not chunk:
                            # 读取返回空 bytes，说明连接关闭
                            print("\n服务器关闭了连接")
                            break
                            
                        # --- 收到数据 ---
                        if not is_writing:
                            # 新的一段开始：创建新文件
                            timestamp = time.strftime("%H%M%S")
                            filename = os.path.join(output_dir, f"{file_counter:03d}_{timestamp}.mp3")
                            current_file = open(filename, 'wb')
                            print(f"\n[开始记录] -> {filename} ", end="")
                            is_writing = True
                            file_counter += 1
                        
                        current_file.write(chunk)
                        print(".", end="", flush=True)
                        
                    except asyncio.TimeoutError:
                        # --- 超时（静默） ---
                        # 说明一段时间没收到数据，认为这一句（或这一段）结束了
                        if is_writing and current_file:
                            current_file.close()
                            current_file = None
                            is_writing = False
                            print(" [段落结束]")
                        
                        # 继续循环等待下一段数据，不退出连接
                        
    except aiohttp.ClientConnectionError:
        print(f"\n无法连接到 {url}。请确认 audio_tts 节点已启动且端口正确。")
    except Exception as e:
        print(f"\n发生错误: {e}")
    finally:
        if current_file:
            current_file.close()
            print(" [意外结束，文件已保存]")

if __name__ == "__main__":
    # 默认配置
    HOST = "192.168.22.219"
    PORT = 8090 # 对应 launch 文件中的 http_port
    URL = f"http://{HOST}:{PORT}/audio/stream"
    
    # 允许通过命令行参数覆盖 URL
    if len(sys.argv) > 1:
        URL = sys.argv[1]
    
    try:
        asyncio.run(save_stream_segmented(URL))
    except KeyboardInterrupt:
        print("\n用户停止录制")
