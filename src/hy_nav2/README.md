# hy_nav2 功能包

Nav2 自主导航与任务调度功能包，实现"搜索黄油 → 导航抓取 → 返回起点"的自主任务。

## 功能说明

- **任务调度节点** (`butter_mission_node`): 状态机管理，协调导航和抓取任务
- **Nav2 导航栈**: AMCL定位 + 全局规划 + 局部避障
- **配置文件**: 适配黄油机器人的 Nav2 参数

## 编译

```bash
cd ~/hy_linux/nfs/hy_ros
colcon build --packages-select hy_nav2
source install/setup.bash
```

## 运行

### 1. 启动基础系统
```bash
ros2 launch ldlidar_driver_ros2 ldlidar_driver.launch.py &
ros2 launch uart uart_launch.py &
ros2 launch ekf_odom ekf_odom_launch.py &
ros2 launch hy_slam laser_filter_only.launch.py
```

### 2. 启动视觉检测
```bash
ros2 launch rk_camera camera_inference_streamer.launch.py
```

### 3. 启动 Nav2 导航栈
```bash
ros2 launch hy_nav2 nav2_bringup.launch.py
```

### 4. 启动任务调度
```bash
ros2 launch hy_nav2 butter_mission.launch.py
```

## 参数配置

在 `butter_mission.launch.py` 中可配置：
- `center_x`: 搜索中心点 X 坐标
- `center_y`: 搜索中心点 Y 坐标
- `search_angular_speed`: 搜索旋转速度

## 依赖

- ROS2 Humble
- navigation2
- nav2_bringup
- slam_toolbox
- hy_slam (地图文件)
