# slam_toolbox 订阅 /scan_filtered 话题,与静态地图进行对比匹配，发布map->odom 的坐标变换，纠正里程计的累计误差
import os
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node

def generate_launch_description():
    pkg_hy_slam = get_package_share_directory('hy_slam')
    
    # 配置文件路径
    localization_params_file = os.path.join(pkg_hy_slam, 'config', 'mapper_params_localization.yaml')
    filter_params_file = os.path.join(pkg_hy_slam, 'config', 'laser_filter.yaml')

    # 声明地图文件参数
    map_file_name_arg = DeclareLaunchArgument(
        'map_file_name',
        default_value=os.path.join(pkg_hy_slam, 'map', 'my_map'), # 默认地图名
        description='Path to map file (without extension)'
    )

    # 1. 雷达数据裁剪 (Laser Filters)
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

    # 2. SLAM (Localization Mode)
    # 使用 /scan_filtered 进行定位
    localization_node = Node(
        package='slam_toolbox',
        executable='localization_slam_toolbox_node',
        name='slam_toolbox',
        parameters=[
            localization_params_file,
            {
                'use_sim_time': False,
                'map_file_name': LaunchConfiguration('map_file_name')
            }
        ],
        remappings=[
            ('/scan', '/scan_filtered')
        ],
        output='screen'
    )

    return LaunchDescription([
        map_file_name_arg,
        laser_filter_node,
        localization_node
    ])
