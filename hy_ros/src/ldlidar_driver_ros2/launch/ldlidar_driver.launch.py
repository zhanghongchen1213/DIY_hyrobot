#!/usr/bin/env python3
from launch import LaunchDescription
from launch_ros.actions import Node

'''
parameters=[
        {'product_name': 'LDLiDAR'},
        {'topic_name': '/scan'},
        {'port_name': '/dev/ttyUSB0'},
        {'frame_id': 'lidar_frame'},
        {'laser_scan_dir': True},
        {'enable_angle_crop_func': False},
        {'angle_crop_min': 135.0},
        {'angle_crop_max': 225.0}
      ]
---
Parameter Description:
---
- Angle crop setting, Mask data within the set angle range:
  1. Enable angle crop fuction:
    1.1. enable angle crop,  example: {'enable_angle_crop_func': True}
    1.2. disable angle crop, example: {'enable_angle_crop_func': False}
  2. Angle cropping interval setting:
  - The distance and intensity data within the set angle range will be set to 0.
  - angle >= 'angle_crop_min' and angle <= 'angle_crop_max' which is [angle_crop_min, angle_crop_max], unit is degress.
    example:
      {'angle_crop_min': 135.0}
      {'angle_crop_max': 225.0}
      which is [135.0, 225.0], angle unit is degress.
'''

def generate_launch_description():
  return LaunchDescription([
    Node(
      # ldlidar publisher node
      package='ldlidar_driver_ros2',
      executable='ldlidar_driver_ros2_node',
      name='LDLiDAR',
      output='screen',
      parameters=[
        {'product_name': 'LDLiDAR'},
        {'topic_name': '/scan'},
        {'port_name': '/dev/ldlidar_serial'},
        {'frame_id': 'radar_Link'},
        {'laser_scan_dir': True},
        {'enable_angle_crop_func': False},
        {'angle_crop_min': 0.0},
        {'angle_crop_max': 0.0},
        {'angle_offset': -90.0} # 角度偏移量，在原安装位置需要偏移-90°，矫正方向
      ]
    )
  ])
