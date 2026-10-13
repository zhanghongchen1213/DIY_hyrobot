import os
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch_ros.actions import Node

def generate_launch_description():
    pkg_hy_slam = get_package_share_directory('hy_slam')
    filter_params_file = os.path.join(pkg_hy_slam, 'config', 'laser_filter.yaml')

    # 1. 雷达数据裁剪 (Laser Filters)
    # 仅启动滤波器，用于 Nav2/AMCL 场景
    laser_filter_node = Node(
        package='laser_filters',
        executable='scan_to_scan_filter_chain',
        name='laser_filter',
        parameters=[filter_params_file],
        remappings=[
            ('scan', '/scan'),
            ('scan_filtered', '/scan_filtered')
        ],
        output='screen'
    )

    return LaunchDescription([
        laser_filter_node
    ])
