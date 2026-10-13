from launch import LaunchDescription
from launch_ros.actions import Node

def generate_launch_description():
    return LaunchDescription([
        Node(
            package='rk_streamer',
            executable='rk_streamer_node',
            name='rk_streamer',
            output='screen',
            parameters=[
                {
                    'width': 1920,
                    'height': 1080,
                    'fps': 30,
                    'sub_topic': '/yolo/image_infer',
                    'pipeline': 'appsrc name=source ! video/x-raw,format=NV12 ! videoconvert ! mpph264enc ! h264parse config-interval=-1 ! rtspclientsink location=rtsp://127.0.0.1:8554/camera protocols=tcp'
                }
            ]
        )
    ])
