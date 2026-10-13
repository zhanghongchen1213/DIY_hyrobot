from launch import LaunchDescription
from launch_ros.actions import Node


def generate_launch_description():
    # 为长路径设置模型目录常量，减少单行长度以通过 flake8 E501
    model_dir = (
        '/opt/sherpa-onnx/'
        'sherpa-onnx-rk3588-streaming-zipformer-small-bilingual-zh-en-2023-02-16'
    )

    return LaunchDescription([
        Node(
            package='audio_asr',  # 所属包名，需与 setup.py/entry_points 保持一致
            executable='audio_asr_node',  # 控制台脚本名，对应 setup.py 的 entry_points
            name='audio_asr',  # ROS 网络中的节点名称
            output='screen',  # 将节点日志输出到控制台
            emulate_tty=True,  # 提升 Python 节点的实时输出表现
            parameters=[  # ROS2 要求为 list；内部为单个 dict 容器
                {
                    # 控制开关话题（Bool），收到 True 开始识别，关闭后等待端点再退出
                    'sub_chat_topic': '/uart/chat_gpt_count',  # 控制开关话题（Bool），收到 True 开始识别，关闭后等待端点再退出
                    # 识别文本发布的话题名（String）
                    'pub_asr_text_topic': '/audio/asr_text',
                     #音频状态发布话题，用于通知底层
                    'pub_audio_status_topic': '/uart/audio_stream_flag',
                    # 推理后端提供者：CPU/GPU/RKNN 等；RK 平台通常使用 rknn
                    'provider': 'rknn',
                    # sherpa-onnx 模型三件套（RKNN 格式）：encoder/decoder/joiner
                    'encoder': model_dir + '/encoder.rknn',
                    'decoder': model_dir + '/decoder.rknn',
                    'joiner': model_dir + '/joiner.rknn',
                    # 与模型配套的词表文件
                    'tokens': model_dir + '/tokens.txt',
                    # ALSA 输入设备（录音源），例如麦克风卡槽
                    'alsa_device': 'plughw:3,0',
                    # CLI 模式下的静默超时秒数，用于平滑终止子进程
                    'silence_timeout_sec': 2.0,
                    # 模式选择：True 走 CLI（sherpa-onnx-alsa）；False 走 Python API
                    'use_cli': False,
                    
                    # 端点检测规则1：基础静默超时（秒）。
                    # 含义：只要静默时间超过此值，即视为一句话结束。
                    'rule1_min_trailing_silence': 1.5,
                    
                    # 端点检测规则2：长语音静默超时（秒）。
                    # 含义：当语音长度超过 rule3 时，使用此较短的静默阈值来快速断句。
                    'rule2_min_trailing_silence': 1.0,
                    
                    # 端点检测规则3：触发规则2的最短语音长度（秒）。
                    # 含义：只有当前语音长度超过此值，才应用 rule2。
                    'rule3_min_utterance_length': 5.0,
                    'target_device_idx': 3,
                }
            ],
        )
    ])
