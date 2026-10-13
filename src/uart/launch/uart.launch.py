from launch import LaunchDescription
from launch_ros.actions import Node


def generate_launch_description():
    return LaunchDescription([
        Node(
            package='uart',  # 您的包名
            executable='uart_node',  # 在 setup.py 中 entry_points 定义的可执行文件名
            name='uart',  # ROS 网络中节点的自定义名称
            output='screen',  # 将节点输出打印到屏幕
            emulate_tty=True,  # 对于Python节点，这通常能帮助立即看到 print 输出
            parameters=[  # ROS2要求为list；每个dict是一组参数覆盖
                {
                    'port': '/dev/ttyS3',
                    'baud': 460800,
                    'pub_chat_topic': '/uart/chat_gpt_count',
                    'pub_motor_status_topic': '/uart/motor_status',
                    'pub_wheel_odom_topic': '/uart/wheel_odom',
                    'pub_imu_data_topic': '/uart/imu_data',
                    'sub_audio_status_topic': '/uart/audio_stream_flag', # 订阅音频状态
                    'sub_motor_control_topic': '/uart/motor_control',  # 订阅键盘控制
                    'sub_joy_topic': '/joy',  # 订阅游戏手柄控制
                }
            ]
        )
    ])
