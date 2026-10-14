# Nav2 配置与黄油任务原型

本包提供 Nav2 参数、导航启动文件与六状态任务节点。当前是实验性原型，尚未完成检测、底盘控制、抓取和导航完成反馈的闭环集成。

## 文件与依赖

- `config/nav2_params.yaml`：AMCL、NavFn、DWB 与代价地图参数。
- `launch/nav2_bringup.launch.py`：包含 Nav2 的 `bringup_launch.py`，固定读取已安装 `hy_slam/map/my_map.yaml`。
- `launch/butter_mission.launch.py`：只启动任务节点，不包含导航栈。
- `hy_nav2/butter_mission_node.py`：搜索中心、旋转搜索、目标导航、抓取和返回状态骨架。

目标环境为 Ubuntu 22.04 / ROS 2 Humble。需要 `navigation2`、`nav2_bringup`、`hy_slam` 与地图，以及雷达、UART、EKF、机器人 TF；视觉运行还依赖 RK3588S 的模型与库。

## 编译与地图

在 ROS 工作区执行：

```bash
source /opt/ros/humble/setup.bash
colcon build --packages-up-to hy_nav2 hy_slam
source install/setup.bash
```

`hy_slam/scripts/save_map.sh` 实际保存到 `src/hy_slam/map/`，其 `setup.py` 没有安装地图文件。建图后需复制 `my_map.yaml` 和 `my_map.pgm` 到 Nav2 固定读取的目录：

```bash
map_directory="$(ros2 pkg prefix hy_slam)/share/hy_slam/map"
mkdir -p "$map_directory"
cp src/hy_slam/map/my_map.yaml src/hy_slam/map/my_map.pgm "$map_directory/"
```

地图生成与序列化定位方法见[SLAM 教程](../../docs/source/development/slam.md)。清理或重建安装空间后需重新检查地图。

## 启动入口

以下各组在独立终端执行，每个终端先加载 ROS 与工作区环境：

```bash
# 基础系统；每个命令占一个终端。
ros2 launch ldlidar_driver_ros2 ldlidar_driver.launch.py
ros2 launch uart uart.launch.py
ros2 launch ekf_odom ekf_odom.launch.py
ros2 launch butter_robot_description butter_robot_description.launch.py
ros2 launch hy_slam laser_filter_only.launch.py
```

```bash
# 独立视觉调试，不具备同进程容器传输条件。
ros2 launch rk_camera rk_camera.launch.py
ros2 launch rk_inference rk_inference.launch.py
ros2 launch rk_streamer rk_streamer.launch.py
```

也可使用 `ros2 launch all_launch all_launch.launch.py`，同时启动视觉容器、UART、EKF、URDF 和 Foxglove，此时不重复启动这些节点；雷达与滤波需另行启动。

```bash
# 地图和接口准备后，启动导航栈。
ros2 launch hy_nav2 nav2_bringup.launch.py
# 下列节点只用于接口完成后的任务调试。
ros2 launch hy_nav2 butter_mission.launch.py
```

## 任务接口与未完成项

| 接口 | 当前任务节点用法 | 集成缺口 |
| --- | --- | --- |
| `/detections` | `std_msgs/msg/String`，解析 `butter,x,y,distance` | 视觉节点没有发布者；x/y 直接当地图坐标，尚无坐标变换 |
| `/amcl_pose` | 订阅 `PoseStamped` | 标准 AMCL 输出 `PoseWithCovarianceStamped`，类型不匹配 |
| `/cmd_vel` | 发布 `Twist`，搜索与 Nav2 速度输出 | UART 不订阅，需要到 `MotorControl` 的桥接 |
| `/joint_commands` | 发布 `String` 的 `grasp` | 没有接收执行端，固件舵机初始化默认关闭 |
| 里程计 | Nav2 配置 `/odom` | EKF 默认输出 `/odometry/filtered`，需要统一 |
| `/navigate_to_pose` | 发送导航目标 | 状态切换没有等待最终成功或失败结果 |

节点没有实现距离小于 0.3 m 才抓取、抓取等待或执行回执。不能仅根据启动成功判定“导航抓取返回”可用。

`butter_mission.launch.py` 当前可配置 `center_x=0.0`、`center_y=0.0` 和 `search_angular_speed=0.5`。更多参数、话题与 TF 检查见[导航教程](../../docs/source/development/navigation.md)。
