#!/bin/bash
# 一键启动键盘控制节点，该脚本需要在主机终端运行
sudo ./clear_environment.sh

colcon build --packages-select pid_debug_interfaces uart keyboard_control

source install/setup.bash

ros2 run keyboard_control keyboard_control_node


