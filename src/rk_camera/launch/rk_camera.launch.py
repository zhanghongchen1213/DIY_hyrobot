from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node

def generate_launch_description():
    return LaunchDescription([
        # 声明启动参数
        DeclareLaunchArgument(
            'framerate',
            default_value='30',
            description='Camera framerate'
        ),
        DeclareLaunchArgument(
            'width',
            default_value='1920',
            description='Image width'
        ),
        DeclareLaunchArgument(
            'height',
            default_value='1080',
            description='Image height'
        ),
        
        Node(
            package='rk_camera',
            executable='rk_camera_node',
            name='rk_camera',
            output='screen',
            emulate_tty=True,
            parameters=[
                {
                    # 摄像头设备节点
                    'device_id': 0,
                    # 分辨率
                    'width': LaunchConfiguration('width'),   # 软编码，可在其他节点中调整
                    'height': LaunchConfiguration('height'), # 软编码，可在其他节点中调整
                    # 帧率
                    'framerate': LaunchConfiguration('framerate'), # 软编码，可在其他节点中调整
                    # 旋转
                    'rotation_180': True,
                    # 话题名称
                    'sub_control_topic': '/uart/enable_butter_yolo',
                    'pub_image_topic': '/yolo/image_raw',
                    # 调试开关
                    'debug_fps': False,
                }
            ]
        )
    ])
