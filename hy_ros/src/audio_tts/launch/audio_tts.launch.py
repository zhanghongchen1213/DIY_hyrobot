import os
from launch import LaunchDescription
from launch_ros.actions import Node

def generate_launch_description():
    return LaunchDescription([
        Node(
            package='audio_tts',
            executable='audio_tts_node',
            name='audio_tts',
            output='screen',
            emulate_tty=True,
            parameters=[
                {
                    'sub_llm_topic': '/audio/llm_result',  
                    'pub_audio_status_topic': '/uart/audio_stream_flag',
                    'baidu_api_key': os.environ.get('BAIDU_API_KEY', ''),
                    'baidu_secret_key': os.environ.get('BAIDU_SECRET_KEY', ''),
                    'baidu_voice_id': os.environ.get('BAIDU_VOICE_ID', ''),
                    'http_port': 8090,
                    'spd': 10, # 语速 0-15，默认5，范围0-15，数值越大语速越快。人类语音参考10，机器人语音参考7
                    'pit': 5, # 音调 0-15，默认5，范围0-15，数值越大音调越高
                    'vol': 5, # 音量 0-15，默认5，范围0-15，数值越大音量越大
                    'aue': 3, # 音频格式 3=mp3, 6=wav
                    'lang': 'zh', # 语言 zh=中文，英文通用
                    'dialect': 'zh-CN-sichuan', # 方言，空为普通话；上海话：wuu-CN-shanghai；河南话：zh-CN-henan；四川话：zh-CN-sichuan；湖南话：zh-CN-hunan；贵州话：zh-CN-guizhou
                }
            ],
        )
    ])
