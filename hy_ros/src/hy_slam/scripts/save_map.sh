#!/bin/bash
# 功能：保存 slam_toolbox 构建的地图
# 用法：./save_map.sh [地图名]

MAP_NAME=${1:-my_map}

# 获取当前脚本所在目录的绝对路径
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
# 设置保存目录为脚本上一级目录下的 map 文件夹 (即 src/hy_slam/map)
SAVE_DIR="$SCRIPT_DIR/../map"

# 确保保存目录存在
mkdir -p $SAVE_DIR

# 调用 ROS2 服务保存地图
# slam_toolbox 提供了 /slam_toolbox/save_map 服务
# 这将保存 .pgm (地图图像) 和 .yaml (元数据) 文件
ros2 service call /slam_toolbox/save_map slam_toolbox/srv/SaveMap "{name: {data: '$SAVE_DIR/$MAP_NAME'}}"

# (可选) 同时保存序列化地图 (.posegraph)，用于后续加载继续建图
# ros2 service call /slam_toolbox/serialize_map slam_toolbox/srv/SerializePoseGraph "{filename: '$SAVE_DIR/${MAP_NAME}_serialized'}"

echo "Map saved to: $SAVE_DIR/$MAP_NAME"
