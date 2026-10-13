import os
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node

def generate_launch_description():
    """
    生成黄油机器人的描述启动文件。
    主要功能：
    1. 读取 URDF 文件内容
    2. 启动 robot_state_publisher 节点发布 /robot_description 和 TF
    3. 启动 dynamic_tf_publisher 发布带当前时间戳的radar_Link TF
    """

    # 1. 声明启动参数
    # use_sim_time: 是否使用仿真时间（在 Gazebo 中仿真时需要设为 true）
    use_sim_time = LaunchConfiguration('use_sim_time', default='false')

    # 2. 获取 URDF 文件路径
    # 通过 ament_index 获取 share 目录下的安装路径
    urdf_file_name = 'butter_robot.urdf'
    urdf_path = os.path.join(
        get_package_share_directory('butter_robot_description'),
        'urdf',
        urdf_file_name)

    # 3. 读取 URDF 文件内容
    # robot_state_publisher 需要通过参数直接传入文件内容字符串，而不是路径
    with open(urdf_path, 'r') as infp:
        robot_desc = infp.read()

    return LaunchDescription([
        # 声明 use_sim_time 参数
        DeclareLaunchArgument(
            'use_sim_time',
            default_value='false',
            description='Use simulation (Gazebo) clock if true'),

        # 启动 robot_state_publisher 节点
        Node(
            package='robot_state_publisher',
            executable='robot_state_publisher',
            name='robot_state_publisher',
            output='screen',
            # 传入参数：
            # - use_sim_time: 同步仿真时间
            # - robot_description: 机器人的 XML 描述内容
            parameters=[{'use_sim_time': use_sim_time, 'robot_description': robot_desc}],
            # 参数列表（通常不需要显式传入文件路径，上面的 robot_description 参数已足够）
            arguments=[urdf_path]),

        # 🔧 修复：使用自定义Python节点发布带当前时间戳的TF到/tf话题
        # 解决slam_toolbox无法使用时间戳为0的/tf_static的问题
        # 该节点以10Hz频率发布base_link → radar_Link的TF转换
        Node(
            package='butter_robot_description',
            executable='dynamic_tf_publisher.py',
            name='dynamic_tf_publisher',
            output='screen'
        ),
    ])
