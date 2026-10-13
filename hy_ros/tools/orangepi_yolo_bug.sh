#!/bin/bash

# 一键启动串口节点
colcon build --packages-select rk_camera rk_inference rk_streamer

source install/setup.bash

# 同时启动 rk_camera 和 rk_inference 两个 launch 文件
ros2 launch rk_camera rk_camera.launch.py & ros2 launch rk_inference rk_inference.launch.py & ros2 launch rk_streamer rk_streamer.launch.py
