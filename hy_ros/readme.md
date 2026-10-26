# 黄油机器人 ROS 2 工作区

> 本项目是 [DIY_hyrobot](https://github.com/zhanghongchen1213/DIY_hyrobot) 的 `hy_ros/` 子目录。模型与测试文件用 `tools/fetch_source.py` 按 [source-manifest.json](resources/source-manifest.json) 还原。

## 🛒 购买链接

> 🐟 **闲鱼搜索**：`鸿尘客`
>
> 如有疑问可通过闲鱼私信联系卖家，支持定制化需求沟通。

## 📖 项目简介

**黄油机器人（Butter Robot）** 是一款基于 ROS2 Humble 的移动机器人学习与开发平台，采用 Rockchip RK3588S 作为主控芯片，搭配 ESP32S3 微控制器实现底层运动控制。项目涵盖感知、定位建图、运动控制、视觉处理和语音交互五大核心功能模块，是一个集成度较高的机器人学习与开发平台。

## 🔗 开源地址

| 平台                     | 链接                                                                                                                   |
| ------------------------ | ---------------------------------------------------------------------------------------------------------------------- |
| 个人博客                 | [https://zhcmqtt.top](https://zhcmqtt.top)                                                                             |
| CSDN博客                 | [https://blog.csdn.net](https://blog.csdn.net/weixin_46477159?spm=1000.2115.3001.5343)                                 |
| 立创硬件开源             | [https://x.jlc.com](https://oshwhub.com/nhwjxzxz/huang-you-ji-qi-ren)                                                  |
| GitHub——ROS算法开源      | [https://github.com/zhanghongchen1213/DIY_hyrobot/tree/main/hy_ros](https://github.com/zhanghongchen1213/DIY_hyrobot/tree/main/hy_ros)                             |
| GitHub——驱动器算法开源   | [https://github.com/zhanghongchen1213/DIY_hyrobot/tree/main/hy_driver](https://github.com/zhanghongchen1213/DIY_hyrobot/tree/main/hy_driver)                       |
| GitHub——yolo训练算法开源 | [https://github.com/zhanghongchen1213/DIY_hyrobot/tree/main/butter_yolov8n_train](https://github.com/zhanghongchen1213/DIY_hyrobot/tree/main/butter_yolov8n_train) |

## 📷 产品外观

|                                                                                                  |                                                                                                  |
| :----------------------------------------------------------------------------------------------: | :----------------------------------------------------------------------------------------------: |
| [![黄油机器人实物照片 1](images/yl9J6U.png)](images/yl9J6U.png) | [![黄油机器人实物照片 2](images/yl9EAF.png)](images/yl9EAF.png) |
| [![黄油机器人实物照片 3](images/yl9BC0.png)](images/yl9BC0.png) | [![黄油机器人实物照片 4](images/yl9lw9.png)](images/yl9lw9.png) |
| [![黄油机器人实物照片 5](images/yl9KLc.png)](images/yl9KLc.png) | [![黄油机器人实物照片 6](images/yl9YdX.png)](images/yl9YdX.png) |
| [![黄油机器人实物照片 7](images/yl9GcO.png)](images/yl9GcO.png) | [![黄油机器人实物照片 8](images/yl9m6d.png)](images/yl9m6d.png) |

## 🏗️ 系统架构

系统采用分层模块化架构，主要分为以下五层：

- **感知层**：集成 LDLiDAR 2D 激光雷达（360° 环境扫描）、1080P 摄像头、6 轴 IMU 惯性测量单元和轮式编码器里程计，提供多源环境感知能力。
- **定位与建图层**：基于 slam_toolbox 实现在线异步 SLAM 建图，通过 robot_localization 扩展卡尔曼滤波（EKF）融合轮式里程计与 IMU 数据，定位配置使用 laser_filters 过滤机器人自身遮挡；当前建图输入仍为原始雷达。
- **控制层**：通过 UART 串口（460800 波特率）与 ESP32S3 通信，实现差速驱动底盘的 PID 速度控制和舵机控制，支持键盘与手柄输入；大模型到运动指令的接口尚未接通。
- **视觉处理层**：利用 RK3588S 的 NPU（6TOPS 算力）硬件加速 YOLOv8 目标检测，三个视觉组件（采集、推理、推流）运行在同一 ComposableNodeContainer 中，启用进程内通信减少节点间图像复制。
- **交互层**：集成 sherpa-onnx ASR 语音识别、RKLLM 本地文本生成和百度大模型音色复刻 TTS 语音合成，实现全流式语音交互；同时通过 Foxglove Bridge 提供 Web 端远程监控与数字孪生可视化。

## 🎬 视频演示

[![视频封面](images/ylsonn.jpg)](https://www.bilibili.com/video/BV1hQcFzrEXS/?spm_id_from=333.1007.top_right_bar_window_history.content.click)

> 📺 **视频作者**：B站 @鸿尘客
> 🔗 **视频链接**：[黄油机器人](https://www.bilibili.com/video/BV1hQcFzrEXS/?spm_id_from=333.1007.top_right_bar_window_history.content.click)

## ⚙️ 硬件规格

下表沿用项目原硬件配置记录；电池、存储和外设规格以实际装配为准。

|     项目     |                      参数                      |
| :----------: | :--------------------------------------------: |
|  主芯片 SOC  |                Rockchip RK3588S                |
|     CPU      | 4 核 Cortex-A76(2.4GHz)+4 核 Cortex-A55(1.8GHz) |
|     NPU      |                     6TOPS                      |
|     VPU      |          H.264/H.265/JPEG 硬件编解码           |
|     RAM      |                   LPDDR5 8GB                   |
|      TF      |                      32GB                      |
|  Wi-Fi+蓝牙  |                 Wi-Fi 5+BT 5.0                 |
|  驱动器 MCU  |                    ESP32S3                     |
|   电池容量   |                  3000mAh 12V                   |
| 摄像头分辨率 |                1920x1080 30FPS                 |
|   操作系统   |                  Ubuntu22.04                   |
|   ROS 版本   |                  ROS2 Humble                   |

## 📐 硬件框图

[![硬件框图](images/ylD0Gx.png)](images/ylD0Gx.png)

## 🚀 产品功能

> **五大核心能力**：语音交互 | 运动控制 | 视觉处理 | 定位导航 | 远程监控

---

### 🗣️ 语音交互 —— 识别、文本生成与播报

|        模块        | 核心能力                                   | 技术方案                                                         |
| :----------------: | :----------------------------------------- | :--------------------------------------------------------------- |
|  **ASR 语音识别**  | 中英双语实时识别，增量输出无需等待整句     | sherpa-onnx + RKNN 加速 Zipformer，48kHz→16kHz 重采样，端点检测  |
| **LLM 大语言模型** | 本地文本流式生成；可触发拍照，图像理解未接通      | Qwen3-VL-2B 文本推理路径，RKLLM，4096 上下文，流式输出    |
|  **TTS 语音合成**  | 自定义音色克隆，支持多种方言，边合成边播放 | 百度大模型音色复刻 API，句级分段合成，HTTP 流式推送（端口 8090） |

### 🎮 运动控制 —— 多模式操控，精准灵活

|       模块       | 核心能力                                     | 技术方案                                                           |
| :--------------: | :------------------------------------------- | :----------------------------------------------------------------- |
| **差速驱动底盘** | PID 闭环控制；键盘线速度配置 0.15 m/s，在线调参默认关闭  | UART 460800 波特率通信 ESP32S3，自定义二进制协议，左右电机独立控制 |
|   **舵机控制**   | 3 路舵机接口与关节状态映射；固件舵机初始化默认关闭    | `/joint_states` 话题发布关节状态，角度精确控制                     |
|   **键盘遥控**   | WASD 八方向移动，launch 配置 0.2 秒无输入停车        | launch 配置 50 Hz 轮询                                                     |
|   **手柄遥控**   | 摇杆模拟量控制速度，按键控制舵机（±5° 步进） | 线速度 + 角速度双轴控制                                            |
|   **语音控制接口**   | ASR / LLM 文本链路已有；运动指令转换待实现                 | 尚无 LLM 到运动控制的桥接                                               |

### 👁️ 视觉处理 —— NPU 检测与视频推流

|      模块      | 核心能力                                    | 技术方案                                   |
| :------------: | :------------------------------------------ | :----------------------------------------- |
| **摄像头采集** | 1080P@30fps 硬件加速采集，支持 RGA 硬件旋转 | GStreamer + v4l2src，NV12 原生格式输出     |
|  **目标检测**  | 单类别黄油检测与图像标注；手掌检测、跟随未实现                    | YOLOv8 + RKNN NPU 加速，640x640，NMS 0.45  |
|  **视频推流**  | RTSP 实时推流，同进程图像传输优化             | MPP 硬件 H.264 编码，ComposableNode 进程内通信 |

### 🗺️ 定位导航 —— SLAM 建图与 Nav2 配置

|      模块      | 核心能力                                      | 技术方案                                         |
| :------------: | :-------------------------------------------- | :----------------------------------------------- |
| **里程计融合** | 50 Hz 配置的 EKF 融合轮式里程计与 IMU，输出二维位姿 | robot_localization，2D 模式，odom → base_link TF |
| **SLAM 建图**  | 在线异步建图，回环检测，0.05 m/pixel 分辨率   | slam_toolbox + Ceres 求解器，激光量程 12.0 m     |
| **纯定位模式** | 加载已有地图重定位，过滤自身遮挡              | slam_toolbox 定位模式 + laser_filters            |
|  **自主导航**  | Nav2 规划与避障配置；完整任务闭环待集成                       | Nav2 原型，NavFn / DWB；控制与检测接口待集成                  |

### 🖥️ 远程监控 —— Web 端数字孪生，随时随地掌控

|        能力         | 说明                                                           |
| :-----------------: | :------------------------------------------------------------- |
| **Foxglove 可视化** | WebSocket（端口 8765）实时查看机器人状态、话题数据和 TF 坐标树 |
|   **RTSP 视频流**   | 远程查看摄像头画面及目标检测结果                               |
|    **远程控制**     | Web 端发布控制指令，实现数字孪生效果                           |

## 安装与启动

目标环境为 Ubuntu 22.04、ROS 2 Humble，视觉与本地模型运行在 RK3588S。模型、多媒体库、音频依赖和串口按板端环境准备；`rosdep` 不能自动补齐全部 RKNN/RKLLM 外部库。目录下的 `package.json` 只有 Bun 依赖，不是 ROS 构建入口。

```bash
# 从仓库根目录开始，在已准备好依赖的板端工作区执行。
cd hy_ros
source /opt/ros/humble/setup.bash
colcon build
source install/setup.bash
```

全量构建依赖板端库，普通 macOS 或 x86 主机不能据此直接运行所有包。只调试部分模块时可使用 `colcon build --packages-up-to 包名`。

| 用途 | 启动命令 |
| --- | --- |
| 底盘通信 | `ros2 launch uart uart.launch.py` |
| 里程计融合 | `ros2 launch ekf_odom ekf_odom.launch.py` |
| 机器人描述与 TF | `ros2 launch butter_robot_description butter_robot_description.launch.py` |
| 雷达 | `ros2 launch ldlidar_driver_ros2 ldlidar_driver.launch.py` |
| 键盘 | `ros2 run keyboard_control keyboard_control_node --ros-args -p control_topic:=/uart/motor_control -p check_rate:=50.0 -p timeout_sec:=0.2` |
| ASR / LLM / TTS | `ros2 launch audio_asr audio_asr.launch.py` / `ros2 launch audio_rkllm audio_rkllm.launch.py` / `ros2 launch audio_tts audio_tts.launch.py` |
| 视觉及基础容器 | `ros2 launch all_launch all_launch.launch.py` |
| SLAM | `ros2 launch hy_slam slam.launch.py` |
| Nav2 原型 | 先核对[导航模块说明](src/hy_nav2/README.md)，再运行其启动文件 |

键盘节点需要可交互的真实终端，优先使用表中的 `ros2 run`；原 launch 文件可查阅配置，不能仅凭 `emulate_tty` 保证标准输入可用。

每个独立 launch 占一个终端，先加载 ROS 与工作区环境。`all_launch` 包含 UART、EKF、URDF、Foxglove 和三个视觉组件；使用时避免重复启动这些节点。其语音启动项被注释，不含雷达、SLAM 或 Nav2。

## 接口限制与维护

视觉节点当前只发布标注图像；RKLLM ROS 节点尚未集成视觉编码器；导航任务的检测、速度、舵机与位姿接口尚未全部接通。接口缺口见[导航模块说明](src/hy_nav2/README.md)。语音模型路径和百度音色环境变量写在 `audio_asr`、`audio_rkllm`、`audio_tts` 的 launch 文件里。

资源清单见 [source-manifest.json](resources/source-manifest.json)，代码同步见[维护说明](docs/maintainer/repository.md)。本仓库以 MIT 协议授权，版权人是鸿尘客，全文见仓库根目录 `LICENSE`。乐动雷达驱动保留上游许可。
