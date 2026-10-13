from launch import LaunchDescription
from launch_ros.actions import Node

def generate_launch_description():
    return LaunchDescription([
        Node(
            package='rk_inference',
            executable='rk_inference_node',
            name='rk_inference',
            output='screen',
            emulate_tty=True,
            parameters=[
                {
                    'sub_topic': '/yolo/image_raw',
                    'pub_topic': '/yolo/image_infer',
                    'model_path': '/opt/rknn-toolkit2-lite/yolov8.rknn',
                    'conf_threshold': 0.5,
                    'nms_threshold': 0.45,
                }
            ]
        )
    ])
