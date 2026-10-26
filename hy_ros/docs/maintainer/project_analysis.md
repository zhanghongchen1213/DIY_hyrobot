# 当前实现与复现边界

本页按仓库源码核对各模块职责，帮助区分现有实现、历史演示与需要继续接通的接口。文档修订不代表机器人已经通过实机验收。

## 仓库模块

| 目录 | 当前交付 | 运行条件 |
| --- | --- | --- |
| `hy_ros/src/` | ROS 2 串口、里程计融合、视觉、语音、建图及导航原型 | Ubuntu 22.04、ROS 2 Humble；视觉与本地模型依赖 RK3588S 库和设备 |
| `hy_driver/` | ESP32-S3 电机 PID、编码器、IMU、串口、音频和条件编译外设 | ESP-IDF `>=5.1.5,<5.4.0`；当前依赖锁定版本为 5.1.5 |
| `butter_yolov8n_train/` | 单类别 `butter` 训练 Notebook、环境锁文件与历史结果 | Python 3.12、CUDA GPU；数据集另行获取 |
| `hardware/`、`resources/` | SolidWorks Release 入口、文件与归档校验清单 | 7-Zip；装配引用需在 SolidWorks 中确认 |
| `tools/` | 硬件资源下载、独立组件 subtree 同步 | Python 3.9+、Git；同步只适用于已有独立组件仓库 |

## ROS 接口与启动边界

| 模块 | 当前接口或入口 | 阅读时需注意 |
| --- | --- | --- |
| UART | `uart.launch.py`；`/uart/motor_control`、`/joy`、`/uart/audio_stream_flag` 输入 | 当前没有订阅 Nav2 的 `/cmd_vel` 和任务节点的 `/joint_commands` |
| EKF | `ekf_odom.launch.py`；融合轮式里程计与 IMU，配置频率 50 Hz | TF 和传感器参数需按实机核对 |
| 视觉 | `rk_camera` → `rk_inference` → `rk_streamer` | 当前模型为单类别黄油检测；推理节点发布标注图像，没有 `/detections` 结构化结果 |
| 统一启动 | `ros2 launch all_launch all_launch.launch.py` | 启动 UART、EKF、URDF、Foxglove 和视觉容器；语音启动项被注释，不包含雷达、SLAM 或 Nav2 |
| ASR / RKLLM / TTS | 各自的 `audio_*.launch.py` | 本地 RKLLM 节点当前使用文本推理；视觉编码器未集成，拍照触发不等于图像理解 |
| SLAM | `slam.launch.py` | 当前建图订阅 `/scan`，其中滤波节点未启用；定位启动文件则使用 `/scan_filtered` |
| Nav2 | `nav2_bringup.launch.py`、`butter_mission.launch.py` | 前者启动 Nav2，后者只启动任务节点，两者需要分别运行 |

同一视觉容器配置了进程内通信，用于减少节点间图像复制；图像采集和处理仍有内存拷贝。独立启动三个视觉节点不具备同进程容器的传输条件，不能将整条硬件流水线称为完全零拷贝。

## 导航任务尚未接通的接口

`hy_nav2/hy_nav2/butter_mission_node.py` 定义了搜索、接近、抓取和返回的状态机，但当前有以下断点：

1. 节点需要 `/detections` 的 `std_msgs/msg/String`，按 `butter,x,y,distance` 逗号分隔；现有 `rk_inference/src/rk_inference.cpp` 发布图像，尚无该结果接口。任务节点直接把 x/y 作为地图坐标，没有完成相机到地图的坐标转换。
2. 节点订阅 `/amcl_pose` 使用 `geometry_msgs/msg/PoseStamped`；标准 AMCL 输出为 `PoseWithCovarianceStamped`，消息类型不匹配。
3. 导航与搜索发布 `/cmd_vel`，抓取发布 `/joint_commands`；当前 UART 节点没有对应订阅或转换桥接。
4. `nav2_bringup.launch.py` 固定读取已安装 `hy_slam` 下的 `map/my_map.yaml`；仓库未包含地图，`hy_slam/setup.py` 也未安装源目录里的地图文件。
5. 当前 ESP32 固件的 `SERVO_DEBUG=0`，舵机初始化未启用，不能仅靠发布抓取指令验证机械动作。
6. EKF 默认发布 `/odometry/filtered`，Nav2 配置读取 `/odom`；需要统一话题或显式重映射。
7. 任务节点发送导航目标后立即切换状态，没有等待导航成功、检测接近距离或等待抓取完成。

因此“搜索黄油 → 导航抓取 → 返回起点”应作为待集成的任务原型阅读。地图复制和接口缺口见 [hy_nav2 说明](../../src/hy_nav2/README.md)。

## 固件默认配置

配置文件为 `hy_driver/main/all_debug/all_debug.h`：

| 宏 | 当前值 | 行为 |
| --- | --- | --- |
| `UPDATE_ODOMETRY_DEBUG` | `1` | 启用相关运动学与里程计功能；串口结构随此宏变化 |
| `BLUE_CONTROL` | `0` | BLE 初始化与任务关闭 |
| `SERVO_DEBUG` | `0` | 舵机初始化关闭 |
| `PID_DEBUG` | `0` | 在线 PID 调试分支关闭；该分支的左右轮参数赋值仍需修正 |
| `NVS_CLEAR_VERSION` | `1` | 每次启动清除 NVS；不能依此配置验证凭证跨重启持久保存 |

两端串口协议应保持相同字段与长度。修改条件编译开关后需同时检查 ROS 端解析和实际硬件行为。

## 模型、资源与历史记录

- 训练 Notebook 使用 `butter` 单类别、120 轮、批次 64、640×640 和 `cuda:0`。环境锁定 PyTorch 2.5.1 / CUDA 12.4 wheel 与 airockchip Ultralytics 8.2.82 固定提交；原电脑驱动版本不能代替环境要求。
- `format="rknn"` 在固定的 Ultralytics 分支中导出 RKNN 兼容 ONNX，之后仍需 RKNN-Toolkit2 转换为 `.rknn`。
- `Dataset/`、`hy_ros/source/` 与根目录 `Solidworks/` 被 Git 忽略，分别按资源清单准备。已有训练指标和演示截图是历史产出，不是本次重训或运行结果。
- 固定版本和校验记录在 [source-manifest.json](../../resources/source-manifest.json)。重新下载时以清单、上游许可与实际校验结果为准。
- [维护说明](repository.md)区分当前总仓库流程与旧电脑的独立组件工作区。`git publish` 别名、私有备份和旧远端归档状态不会随克隆自动获得。

## 其他源码核对发现

- `all_launch/package.xml` 未完整声明启用中的视觉、Foxglove 与组件容器依赖；相机包清单也未完整覆盖 CMake 使用的图像库。仅执行 rosdep 不能保证准备好所有依赖，按各包构建配置逐项核对。
- 相机源码读取参数 `image_topic`；启动文件中的 `pub_image_topic` 与 `sub_control_topic` 未被该源码读取。修改这些启动字段不能据此判定话题已更名或摄像头已受控制，应以实际 ROS 参数为准。
- 固件 `main/app_uart_control/app_uart_control.c` 的 `PID_DEBUG` 分支把左轮目标和 PID 参数写给左右两轮；当前默认关闭，开启调参前需修正并验证。

## 本次文档检查范围

核对范围包括源码入口、启动与构建配置和各模块 README。项目 README 已修正职责、命令与版本；两份 BMAD 工具资料保留原文，雷达上游 README 仅修正 ROS 2 构建说明。

说明之间的链接可在本机核对。RK3588 推理、ROS 话题与 TF、ESP32 编译烧录、CUDA 重训及 SolidWorks 装配需要对应平台和设备，不能从静态分析得出运行通过的结论。
