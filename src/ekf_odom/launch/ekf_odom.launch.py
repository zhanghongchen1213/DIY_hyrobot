from launch import LaunchDescription
from launch_ros.actions import Node
import os
from ament_index_python.packages import get_package_share_directory

def generate_launch_description():
    # 获取包路径
    package_name = 'ekf_odom'
    pkg_share = get_package_share_directory(package_name)
    
    # 配置文件路径
    config_file = os.path.join(pkg_share, 'config', 'ekf.yaml')

    return LaunchDescription([
        Node(
            package='robot_localization',
            executable='ekf_node',
            name='ekf_filter_node',
            output='screen',
            emulate_tty=True,
            parameters=[config_file],
            # 话题重映射：如果 config 中使用默认名称，可以在这里映射，
            # 或者直接在 config 中指定话题名称。通常推荐在 config 中指定。
            # remappings=[('odometry/filtered', 'odometry/filtered')] 
        )
    ])
