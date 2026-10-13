#!/bin/bash
# 一键启动所有节点
colcon build

source install/setup.bash

ros2 launch all_launch all_launch.launch.py