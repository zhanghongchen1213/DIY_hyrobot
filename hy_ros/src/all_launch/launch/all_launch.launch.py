from launch import LaunchDescription
from launch.actions import IncludeLaunchDescription
from launch.launch_description_sources import PythonLaunchDescriptionSource
from ament_index_python.packages import get_package_share_directory
import os

from launch_ros.actions import Node
from launch_ros.actions import ComposableNodeContainer
from launch_ros.descriptions import ComposableNode

def generate_launch_description():
    # 1. uart 节点
    uart_share = get_package_share_directory('uart')
    uart_launch = os.path.join(uart_share, 'launch', 'uart.launch.py')
    
    # 2. ekf_odom 节点
    ekf_odom_share = get_package_share_directory('ekf_odom')
    ekf_odom_launch = os.path.join(ekf_odom_share, 'launch', 'ekf_odom.launch.py')
    
    # # 3. audio_asr 节点
    # audio_asr_share = get_package_share_directory('audio_asr')
    # audio_asr_launch = os.path.join(audio_asr_share, 'launch', 'audio_asr.launch.py')
    
    # # 4. audio_rkllm 节点
    # audio_rkllm_share = get_package_share_directory('audio_rkllm')
    # audio_rkllm_launch = os.path.join(audio_rkllm_share, 'launch', 'audio_rkllm.launch.py')
    
    # # 5. audio_tts 节点
    # audio_tts_share = get_package_share_directory('audio_tts')
    # audio_tts_launch = os.path.join(audio_tts_share, 'launch', 'audio_tts.launch.py')
    
    # 6. butter_robot_description 节点
    butter_robot_description_share = get_package_share_directory('butter_robot_description')
    butter_robot_description_launch = os.path.join(butter_robot_description_share, 'launch', 'butter_robot_description.launch.py')

    return LaunchDescription([
        # 1. uart 节点
        IncludeLaunchDescription(
            PythonLaunchDescriptionSource(uart_launch)
        ),
        
        # 2. ekf_odom 节点
        IncludeLaunchDescription(
            PythonLaunchDescriptionSource(ekf_odom_launch)
        ),
        
        # # 3. audio_asr 节点
        # IncludeLaunchDescription(
        #     PythonLaunchDescriptionSource(audio_asr_launch)
        # ),

        # # 4. audio_rkllm 节点
        # IncludeLaunchDescription(
        #     PythonLaunchDescriptionSource(audio_rkllm_launch)
        # ),
        
        # # 5. audio_tts 节点
        # IncludeLaunchDescription(
        #     PythonLaunchDescriptionSource(audio_tts_launch)
        # ),
        
        # 6. butter_robot_description 节点
        IncludeLaunchDescription(
            PythonLaunchDescriptionSource(butter_robot_description_launch)
        ),

        # 7. foxglove_bridge 节点 (端口: 8765)
        Node(
            package='foxglove_bridge',
            executable='foxglove_bridge',
            name='foxglove_bridge',
            parameters=[
                {'port': 8765},
                {'address': '0.0.0.0'},
                {'tls': False},
                {'send_buffer_limit': 10000000},
                {'use_compression': True},
                {'capabilities': ['clientPublish', 'parameters', 'parametersSubscribe', 'services', 'connectionGraph', 'assets']},
                {'asset_uri_allowlist': ['^package://(?:[-\\w]+/)*[-\\w]+\\.(?:dae|fbx|glb|gltf|jpeg|jpg|mtl|obj|png|stl|tif|tiff|urdf|webp|xacro)$']}
            ]
        ),
        
        # 8. 视觉管道 (rk_camera + rk_inference + rk_streamer)
        # 使用 ComposableNodeContainer 将它们运行在同一个进程容器中，启用零拷贝通信
        ComposableNodeContainer(
            name='butter_vision_container',
            namespace='',
            package='rclcpp_components',
            executable='component_container',
            composable_node_descriptions=[
                # 8.1 rk_camera 组件
                ComposableNode(
                    package='rk_camera',
                    plugin='RkCamera',
                    name='rk_camera',
                    parameters=[
                        {'device_id': 0},
                        {'width': 1920},
                        {'height': 1080},
                        {'framerate': 30},
                        {'rotation_180': True},
                        {'sub_control_topic': '/uart/enable_butter_yolo'},
                        {'pub_image_topic': '/yolo/image_raw'},
                        {'debug_fps': False}
                    ],
                    extra_arguments=[{'use_intra_process_comms': True}]
                ),
                # 8.2 rk_inference 组件
                ComposableNode(
                    package='rk_inference',
                    plugin='RkInference',
                    name='rk_inference',
                    parameters=[
                        {'sub_topic': '/yolo/image_raw'}, # 订阅 rk_camera
                        {'pub_topic': '/yolo/image_infer'}, # 发布推流图像
                        {'model_path': '/opt/rknn-toolkit2-lite/yolov8.rknn'},
                        {'conf_threshold': 0.5},
                        {'nms_threshold': 0.45}
                    ],
                    extra_arguments=[{'use_intra_process_comms': True}]
                ),
                # 8.3 rk_streamer 组件
                ComposableNode(
                    package='rk_streamer',
                    plugin='RkStreamer',
                    name='rk_streamer',
                    parameters=[
                        {'width': 1920},
                        {'height': 1080},
                        {'fps': 30},
                        {'sub_topic': '/yolo/image_infer'}, # 订阅 rk_inference 发布的话题
                        {'pipeline': 'appsrc name=source ! video/x-raw,format=NV12 ! videoconvert ! mpph264enc ! h264parse config-interval=-1 ! rtspclientsink location=rtsp://127.0.0.1:8554/camera protocols=tcp'}
                    ],
                    extra_arguments=[{'use_intra_process_comms': True}]
                )
            ],
            output='screen',
        )
        
        # 9. slam节点
        
    ])
