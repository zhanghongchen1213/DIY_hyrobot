#!/usr/bin/env python3
from launch import LaunchDescription
from launch_ros.actions import Node

def generate_launch_description():
    return LaunchDescription([
        Node(
            package='hy_nav2',
            executable='butter_mission_node',
            name='butter_mission_node',
            output='screen',
            parameters=[{
                'center_x': 0.0,
                'center_y': 0.0,
                'search_angular_speed': 0.5,
            }]
        ),
    ])
