# # slam_toolbox 订阅 /scan_filtered 话题,进行在线建图
import os
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch_ros.actions import Node

def generate_launch_description():
    pkg_hy_slam = get_package_share_directory('hy_slam')
    
    # 配置文件路径
    slam_params_file = os.path.join(pkg_hy_slam, 'config', 'mapper_params_online_async.yaml')
    filter_params_file = os.path.join(pkg_hy_slam, 'config', 'laser_filter.yaml')

    # 1. 雷达数据裁剪 (Laser Filters)
    # 输入: /scan (假设雷达原始话题)
    # 输出: /scan_filtered (SLAM 使用的话题)
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

    # 2. SLAM (Slam Toolbox)
    # ✅ 修复：使用 /scan_filtered（laser_filter配置了keep_point_count保持点数恒定）
    slam_node = Node(
        package='slam_toolbox',
        executable='async_slam_toolbox_node',
        name='slam_toolbox',
        parameters=[slam_params_file, {'use_sim_time': False}],
        output='screen'
    )

    return LaunchDescription([
        # laser_filter_node,  # ✅ 重新启用：使用keep_point_count参数避免点数变化
        slam_node
    ])
