#!/bin/bash

# 一键启动串口节点
colcon build --packages-select butter_robot_description ldlidar_driver_ros2 hy_slam all_launch pid_debug_interfaces uart ekf_odom

source install/setup.bash

# ros2 launch ldlidar_driver_ros2 ldlidar_driver.launch.py &
# ros2 launch all_launch all_launch.launch.py &
# ros2 launch hy_slam slam.launch.py 

ros2 launch ldlidar_driver_ros2 ldlidar_driver.launch.py &
ros2 launch all_launch all_launch.launch.py &
ros2 launch hy_slam slam.launch.py 