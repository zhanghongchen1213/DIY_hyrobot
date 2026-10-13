#!/usr/bin/env python3
from launch import LaunchDescription
from launch.actions import IncludeLaunchDescription
from launch.launch_description_sources import PythonLaunchDescriptionSource
from ament_index_python.packages import get_package_share_directory
import os

def generate_launch_description():
    # 获取功能包路径
    nav2_bringup_dir = get_package_share_directory('nav2_bringup')
    hy_slam_dir = get_package_share_directory('hy_slam')
    hy_nav2_dir = get_package_share_directory('hy_nav2')

    # 地图文件路径
    map_yaml_file = os.path.join(hy_slam_dir, 'map', 'my_map.yaml')

    # Nav2参数文件
    nav2_params_file = os.path.join(hy_nav2_dir, 'config', 'nav2_params.yaml')

    return LaunchDescription([
        # 启动Nav2导航栈
        IncludeLaunchDescription(
            PythonLaunchDescriptionSource(
                os.path.join(nav2_bringup_dir, 'launch', 'bringup_launch.py')
            ),
            launch_arguments={
                'map': map_yaml_file,
                'params_file': nav2_params_file,
                'use_sim_time': 'false',
            }.items(),
        ),
    ])
