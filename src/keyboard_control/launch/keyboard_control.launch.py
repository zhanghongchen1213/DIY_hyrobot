from launch import LaunchDescription
from launch_ros.actions import Node

def generate_launch_description():
    return LaunchDescription([
        Node(
            package='keyboard_control',
            executable='keyboard_control_node',
            name='keyboard_control',
            # prefix='xterm -e', # 移除xterm依赖，直接在当前终端运行
            output='screen',
            emulate_tty=True,
            parameters=[
                {
                    'control_topic': '/uart/motor_control',
                    'check_rate': 50.0,
                    'timeout_sec': 0.2,
                }
            ]
        )
    ])
