# Nav2 导航与任务原型

先完成[开始前的准备](../project_status.md)、[SLAM 建图](slam.md)与地图保存。本节先启动地图、定位和规划组件，再介绍自动任务需要接通的接口。配套任务节点尚不能直接完成“搜索黄油 → 导航抓取 → 返回起点”。

```{important}
启动自动任务前，需要补齐以下接口：视觉发布 `/detections` 的目标坐标；任务节点的 `/amcl_pose` 类型与 AMCL 的 `PoseWithCovarianceStamped` 对齐；将 `/cmd_vel` 转成 UART 的 `MotorControl`；为 `/joint_commands` 的 `grasp` 指令提供执行端并启用舵机。任务状态还需等待导航到达与抓取完成。未接通这些接口时，先进行地图、定位与规划检查，不启动自动抓取任务。
```

## 端侧安装依赖

在端侧（RK3588S）执行以下命令安装 Nav2 导航相关功能包：

```bash
sudo apt update
sudo apt install -y \
    ros-humble-navigation2 \
    ros-humble-nav2-bringup \
    ros-humble-nav2-simple-commander
```

```{important}
本章节依赖[SLAM 建图与定位](slam.md)的功能包（`slam-toolbox`、`robot-localization`、`laser-filters`），若未安装请先完成 SLAM 配置。同时需要[视觉检测系统](vision.md)、底盘通信功能包（`uart`）和里程计融合功能包（`ekf_odom`）正常运行。
```

### 前置条件检查

在开始 Nav2 部署前，请确认以下条件已满足：

- ✅ 已完成 SLAM 建图，将 `src/hy_slam/map/` 中的地图复制到本节所需的安装目录
- ✅ 已部署视觉检测系统（YOLOv8 + RKNN NPU）
- ✅ 底盘串口通信正常（UART 与 ESP32S3）
- ✅ 里程计融合节点正常运行（EKF）
- ✅ 雷达数据正常发布（`/scan` 话题）

## 系统架构设计

### 整体架构

导航系统采用分层设计，从上到下依次为：

```text
┌─────────────────────────────────────────────────────────┐
│  任务调度层 (Mission Layer)                              │
│  - 状态机管理 (搜索/导航/抓取/返回)                       │
│  - 目标点规划与发布                                       │
│  - 视觉检测结果订阅                                       │
└─────────────────────────────────────────────────────────┘
                         ↓
┌─────────────────────────────────────────────────────────┐
│  导航规划层 (Nav2 Stack)                                 │
│  - 全局路径规划 (NavFn)                                  │
│  - 局部轨迹规划 (DWB)                                    │
│  - 行为协调 (Behavior Tree)                              │
└─────────────────────────────────────────────────────────┘
                         ↓
┌─────────────────────────────────────────────────────────┐
│  定位感知层 (Localization & Perception)                  │
│  - AMCL 粒子滤波定位                                     │
│  - 代价地图构建 (静态层+障碍层+膨胀层)                    │
│  - 雷达数据过滤 (laser_filters)                          │
└─────────────────────────────────────────────────────────┘
                         ↓
┌─────────────────────────────────────────────────────────┐
│  执行控制层 (Control Layer)                              │
│  - 速度指令发布 (/cmd_vel)                               │
│  - 底盘运动控制 (UART → ESP32S3)                         │
│  - 舵机控制 (抓取动作)                                    │
└─────────────────────────────────────────────────────────┘
```

### 关键技术点

- **TF 坐标变换链**：`map` → `odom` → `base_link` → `radar_Link`
- **视觉接口**：现有视觉容器使用进程内图像传输；任务节点需要的检测坐标接口尚未接通
- **代价地图融合**：静态地图 + 实时雷达障碍 + 安全膨胀区域

## 任务原型与当前状态切换

任务节点定义六个状态，当前行为如下：

| 状态 | 当前代码行为 | 尚缺的完成条件 |
| --- | --- | --- |
| `IDLE` | 首次收到位姿时保存起点 | `/amcl_pose` 消息类型需与 AMCL 对齐 |
| `GO_TO_CENTER` | 发送搜索中心导航目标，立即切到搜索 | 等待导航成功或处理失败 |
| `SEARCHING` | 持续发布角速度，发现目标时停止搜索 | 结构化检测结果、搜索范围与超时 |
| `NAVIGATE_TO_BUTTER` | 把消息中的 x/y 直接作为 map 坐标发送，立即切到抓取 | 相机到地图转换、导航到达与距离检查 |
| `GRASPING` | 发布字符串 `grasp`，立即切到返回 | 舵机桥接、动作完成反馈与等待 |
| `RETURN_HOME` | 发送起点导航目标后切回空闲 | 等待返回成功、失败处理与状态复位 |

现有代码没有实现“距离小于 0.3 m 才抓取”或“等待 3 秒完成动作”。`/cmd_vel` 与 `/joint_commands` 也没有接到当前 UART 节点。这些是后续集成需要补齐的条件。

## Nav2 核心组件

### 定位模块 (AMCL)

**功能**：基于粒子滤波的蒙特卡洛定位算法，融合雷达数据与里程计实现机器人在地图中的定位。

**关键参数**：

- `max_particles: 2000`：粒子数量，影响定位精度和计算量
- `laser_model_type: likelihood_field`：激光模型类型
- `update_min_d: 0.25`：移动 0.25m 触发一次更新
- `update_min_a: 0.2`：旋转 0.2rad 触发一次更新

**输出**：发布 `map` → `odom` 的 TF 变换

### 全局规划器 (NavFn Planner)

**功能**：基于 Dijkstra 算法在静态地图上规划从起点到终点的最优路径。

**特点**：

- 考虑静态障碍物和膨胀区域
- 生成平滑的全局路径
- `expected_planner_frequency: 20.0` 是期望规划频率配置，不能作为实测更新频率保证

### 局部规划器 (DWB Controller)

**功能**：动态窗口法（Dynamic Window Approach）局部轨迹规划，实时避障。

**工作原理**：

1. 在速度空间采样多条候选轨迹（vx, vy, vθ）
2. 对每条轨迹评分（路径跟随、目标接近、障碍避让）
3. 选择得分最高的轨迹执行

**关键参数**：

- `max_vel_x: 0.15`：最大线速度 (m/s)
- `max_vel_theta: 1.0`：最大角速度 (rad/s)
- `sim_time: 1.5`：轨迹预测时间 (s)

### 代价地图 (Costmap 2D)

**功能**：构建机器人周围环境的代价地图，用于路径规划和避障。

**三层结构**：

1. **静态层 (Static Layer)**：加载 SLAM 构建的静态地图
2. **障碍层 (Obstacle Layer)**：实时雷达检测的动态障碍物
3. **膨胀层 (Inflation Layer)**：障碍物周围膨胀安全区域

**全局 vs 局部代价地图**：

- 全局代价地图：覆盖整个地图，用于全局路径规划
- 局部代价地图：3m×3m 滚动窗口，用于局部避障

## 功能包结构

### hy_nav2 功能包目录结构

```text
hy_nav2/
├── config/
│   └── nav2_params.yaml          # Nav2 核心参数配置
├── launch/
│   ├── nav2_bringup.launch.py    # Nav2 导航栈启动
│   └── butter_mission.launch.py  # 仅任务节点启动
├── hy_nav2/
│   ├── __init__.py
│   └── butter_mission_node.py    # 任务调度节点
├── package.xml
└── setup.py
```

### 核心文件说明

**nav2_params.yaml**

- 包含 AMCL、控制器、规划器、代价地图等所有 Nav2 组件的参数
- 需根据机器人实际尺寸调整 `robot_radius: 0.15`
- 速度限制需与底盘实际能力匹配

**butter_mission_node.py**

- 实现状态机逻辑
- 订阅 `/detections` 话题获取目标检测结果
- 发布 `/cmd_vel` 搜索速度指令；到 UART 的控制桥接尚未实现
- 调用 Nav2 的 `NavigateToPose` Action 发送目标；当前不等待最终结果

**nav2_bringup.launch.py**

- 启动地图服务器（加载 SLAM 地图）
- 启动 AMCL 定位节点
- 启动 Nav2 导航栈（规划器、控制器、行为服务器）

## 配置与后续集成

### 配置文件编写

**步骤 1：创建 Nav2 参数文件**

在 `config/nav2_params.yaml` 中配置：

- AMCL 定位参数（粒子数、更新阈值）
- 控制器参数（速度限制、加速度限制）
- 规划器参数（路径规划算法）
- 代价地图参数（分辨率、膨胀半径）

```{tip}
可参考 Nav2 官方示例：`/opt/ros/humble/share/nav2_bringup/params/nav2_params.yaml`
```

**步骤 2：适配机器人参数**

根据黄油机器人实际情况调整：

- `robot_radius: 0.15`（机器人半径，单位：米）
- `max_vel_x: 0.15`（最大线速度，与底盘能力匹配）
- `inflation_radius: 0.35`（障碍物膨胀半径，确保安全距离）

### 任务调度节点开发

**当前实现与待接通项**：

1. **状态机管理**：使用 Python Enum 定义状态，在定时器回调中切换状态
2. **Action Client**：调用 Nav2 的 `NavigateToPose` Action 发送导航目标
3. **视觉融合**：订阅 `/detections` 话题，解析黄油位置信息
4. **坐标接口待实现**：现有代码直接读取 x/y 并作为地图坐标，未完成坐标转换

**关键接口**：

- 订阅：`/detections` (目标检测结果)、`/amcl_pose` (当前位姿)
- 发布：`/cmd_vel` (速度控制)、`/joint_commands` (舵机控制)
- Action：`/navigate_to_pose` (导航目标)

### 启动文件编写

**nav2_bringup.launch.py 职责**：

- 启动 Map Server（加载 `hy_slam` 保存的地图）
- 启动 AMCL 定位节点
- 启动 Nav2 导航栈（通过 `navigation_launch.py`）
- 启动 Lifecycle Manager 管理节点生命周期

**butter_mission.launch.py 职责**：

- 不包含 `nav2_bringup.launch.py`，导航栈需另行启动
- 启动任务调度节点 `butter_mission_node`
- 传递参数：中心点坐标、搜索速度等

## 编译与运行

### 编译功能包

```bash
cd /mnt/nfs/hy_ros
colcon build --packages-up-to hy_nav2 hy_slam
source install/setup.bash
```

### 启动系统

**地图准备**：`nav2_bringup.launch.py` 固定读取已安装 `hy_slam` 下的 `map/my_map.yaml`。在同一工作区先完成建图保存，再复制地图：

```bash
cd /mnt/nfs/hy_ros
source install/setup.bash
map_directory="$(ros2 pkg prefix hy_slam)/share/hy_slam/map"
mkdir -p "$map_directory"
cp src/hy_slam/map/my_map.yaml src/hy_slam/map/my_map.pgm "$map_directory/"
```

地图 YAML 中的 `image` 应指向可访问的 PGM，复制后确认地图加载目录中两个文件齐全。EKF 输出 `/odometry/filtered`，打开 `src/hy_nav2/config/nav2_params.yaml`，将 `bt_navigator.ros__parameters.odom_topic` 从 `/odom` 改为 `/odometry/filtered`，重新编译并加载工作区。以下命令在独立终端启动，每个终端先加载相同 ROS 和工作区环境。

```bash
# 终端 1：启动基础系统（雷达+底盘+里程计融合）
ros2 launch ldlidar_driver_ros2 ldlidar_driver.launch.py &
ros2 launch uart uart.launch.py &
ros2 launch ekf_odom ekf_odom.launch.py &
ros2 launch butter_robot_description butter_robot_description.launch.py &
ros2 launch hy_slam laser_filter_only.launch.py

# 终端 2：启动视觉检测（摄像头+YOLOv8+推流）
ros2 launch rk_camera rk_camera.launch.py &
ros2 launch rk_inference rk_inference.launch.py &
ros2 launch rk_streamer rk_streamer.launch.py

# 终端 3：启动 Nav2 导航栈
ros2 launch hy_nav2 nav2_bringup.launch.py

# 终端 4：仅在接口断点补齐后调试任务节点
ros2 launch hy_nav2 butter_mission.launch.py
```

```{tip}
可使用 `tmux` 或 `screen` 管理多个终端，或参考 `all_launch` 功能包编写统一的 launch 文件整合所有节点。
```

## 调试与验证

用 `ros2 topic info -v` 检查话题类型和订阅者。自动任务接口完成后，还要确认 `/cmd_vel` 能到达底盘、检测结果能到达任务节点，再进行运动测试。

### 检查系统状态

**验证 TF 树完整性**

```bash
# 生成 TF 树图
ros2 run tf2_tools view_frames

# 检查关键坐标变换
ros2 run tf2_ros tf2_echo map base_link
```

应包含完整链路：`map` → `odom` → `base_link` → `radar_Link`

**检查话题通信**

```bash
# 查看导航相关话题
ros2 topic list | grep -E 'cmd_vel|odom|scan|map|amcl'

# 查看目标检测结果
ros2 topic echo /detections

# 查看机器人位姿
ros2 topic echo /amcl_pose
```

**检查 Action 服务**

```bash
# 查看可用 Action
ros2 action list

# 查看导航 Action 状态
ros2 action info /navigate_to_pose
```

### 常见问题排查

**问题 1：机器人不移动**

- 检查 `/cmd_vel` 话题是否有数据：`ros2 topic hz /cmd_vel`
- 检查 TF 树是否完整（特别是 `map` → `odom` 变换）
- 确认底盘串口通信正常

**问题 2：定位漂移**

- 增加 AMCL 粒子数：`max_particles: 2000 → 3000`
- 检查 EKF 输出：`ros2 topic echo /odometry/filtered`，并核对与 Nav2 `/odom` 配置的对齐
- 确认地图与实际环境一致

**问题 3：路径规划失败**

- 检查代价地图：`ros2 topic echo /global_costmap/costmap --once`
- 减小膨胀半径：`inflation_radius: 0.35 → 0.25`
- 确认目标点在可达区域内

**问题 4：搜索时未检测到黄油**

- 检查视觉检测节点是否运行：`ros2 node list | grep inference`
- 查看检测结果：`ros2 topic echo /detections`
- 调整搜索旋转速度：`search_angular_speed: 0.5 → 0.3`

### Foxglove 可视化

启动 Foxglove Bridge 进行实时监控：

```bash
ros2 run foxglove_bridge foxglove_bridge --ros-args -p port:=8765
```

在浏览器打开 [Foxglove Studio](https://foxglove.dev/studio)，连接到 `ws://<机器人IP>:8765`

**推荐查看的面板**：

- `/map`：静态地图
- `/global_costmap/costmap`：全局代价地图
- `/local_costmap/costmap`：局部代价地图（3m×3m 滚动窗口）
- `/plan`：全局路径
- `/local_plan`：局部轨迹
- `/amcl_pose`：定位位姿
- TF 树：坐标变换关系

## 参数调优建议

### 速度参数

根据实际测试调整机器人运动速度：

- `max_vel_x: 0.15`：最大线速度，过大可能导致失控
- `max_vel_theta: 1.0`：最大角速度，影响转弯灵活性
- `acc_lim_x: 0.5`：线加速度限制，避免急加速
- `acc_lim_theta: 1.5`：角加速度限制

### 目标容差

调整到达目标的精度要求：

- `xy_goal_tolerance: 0.15`：位置容差（米），过小可能无法到达
- `yaw_goal_tolerance: 0.25`：角度容差（弧度）

### 代价地图参数

- `robot_radius: 0.15`：机器人半径，需实测确定
- `inflation_radius: 0.35`：膨胀半径，确保安全距离
- 如需调整障碍物观测距离，根据雷达有效距离和实际场地设置，再检查代价地图覆盖范围。

## 完成后检查

- 地图服务器能加载 `my_map.yaml`，Foxglove 能显示 `/map` 和代价地图。
- 雷达、滤波、里程计和 TF 连续输出；设置机器人初始位姿后，AMCL 能发布定位位姿。
- `ros2 action info /navigate_to_pose` 能找到导航 Action 服务。
- 接入自动任务前，逐项完成本章开头的检测、消息类型、速度、舵机和动作反馈要求。

需要行走或抓取测试时，先完成控制接口适配，再从单个导航目标开始检查。
