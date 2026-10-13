# 全球首发！瑞克和莫蒂 专属定制黄油机器人！

> 本项目是 [DIY_hyrobot](https://github.com/zhanghongchen1213/DIY_hyrobot) 的 `hy_ros/` 子目录。首次使用请先阅读 [资源获取与复现说明](https://github.com/zhanghongchen1213/DIY_hyrobot/blob/main/hy_ros/docs/source/7_Resources.md)；本地独立开发与同步见 [维护指南](https://github.com/zhanghongchen1213/DIY_hyrobot/blob/main/hy_ros/docs/source/8_Maintenance.md)。

## 🛒 购买链接

> 🐟 **闲鱼搜索**：`鸿尘客`
>
> 如有疑问可通过闲鱼私信联系卖家，支持定制化需求沟通。

# 📖 项目简介

**黄油机器人（Butter Robot）** 是一款基于 ROS2 Humble 的全功能自主移动机器人平台，采用 Rockchip RK3588S 作为主控芯片，搭配 ESP32S3 微控制器实现底层运动控制。项目涵盖感知、定位建图、运动控制、视觉处理和语音交互五大核心功能模块，是一个集成度较高的机器人学习与开发平台。

# 🔗 开源地址

| 平台                     | 链接                                                                                                                   |
| ------------------------ | ---------------------------------------------------------------------------------------------------------------------- |
| 个人博客                 | [https://zhcmqtt.top](https://zhcmqtt.top)                                                                             |
| CSDN博客                 | [https://blog.csdn.net](https://blog.csdn.net/weixin_46477159?spm=1000.2115.3001.5343)                                 |
| 立创硬件开源             | [https://x.jlc.com](https://oshwhub.com/nhwjxzxz/huang-you-ji-qi-ren)                                                  |
| GitHub——ROS算法开源      | [https://github.com/zhanghongchen1213/DIY_hyrobot/tree/main/hy_ros](https://github.com/zhanghongchen1213/DIY_hyrobot/tree/main/hy_ros)                             |
| GitHub——驱动器算法开源   | [https://github.com/zhanghongchen1213/DIY_hyrobot/tree/main/hy_driver](https://github.com/zhanghongchen1213/DIY_hyrobot/tree/main/hy_driver)                       |
| GitHub——yolo训练算法开源 | [https://github.com/zhanghongchen1213/DIY_hyrobot/tree/main/butter_yolov8n_train](https://github.com/zhanghongchen1213/DIY_hyrobot/tree/main/butter_yolov8n_train) |
| 开发教程开源             | [https://hy-ros.readthedocs.io](https://hy-ros.readthedocs.io/zh-cn/latest/1_product_introduction.html)                |

# 📷 产品外观

|                                                                                                  |                                                                                                  |
| :----------------------------------------------------------------------------------------------: | :----------------------------------------------------------------------------------------------: |
| [![黄油机器人实物照片 1](docs/source/_static/recovered/yl9J6U.png)](docs/source/_static/recovered/yl9J6U.png) | [![黄油机器人实物照片 2](docs/source/_static/recovered/yl9EAF.png)](docs/source/_static/recovered/yl9EAF.png) |
| [![黄油机器人实物照片 3](docs/source/_static/recovered/yl9BC0.png)](docs/source/_static/recovered/yl9BC0.png) | [![黄油机器人实物照片 4](docs/source/_static/recovered/yl9lw9.png)](docs/source/_static/recovered/yl9lw9.png) |
| [![黄油机器人实物照片 5](docs/source/_static/recovered/yl9KLc.png)](docs/source/_static/recovered/yl9KLc.png) | [![黄油机器人实物照片 6](docs/source/_static/recovered/yl9YdX.png)](docs/source/_static/recovered/yl9YdX.png) |
| [![黄油机器人实物照片 7](docs/source/_static/recovered/yl9GcO.png)](docs/source/_static/recovered/yl9GcO.png) | [![黄油机器人实物照片 8](docs/source/_static/recovered/yl9m6d.png)](docs/source/_static/recovered/yl9m6d.png) |

# 🏗️ 系统架构

系统采用分层模块化架构，主要分为以下五层：

- **感知层**：集成 LDLiDAR 2D 激光雷达（360° 环境扫描）、1080P 摄像头、6 轴 IMU 惯性测量单元和轮式编码器里程计，提供多源环境感知能力。
- **定位与建图层**：基于 slam_toolbox 实现在线异步 SLAM 建图，通过 robot_localization 扩展卡尔曼滤波（EKF）融合轮式里程计与 IMU 数据，并使用 laser_filters 过滤机器人自身遮挡。
- **控制层**：通过 UART 串口（460800 波特率）与 ESP32S3 通信，实现差速驱动底盘的 PID 速度控制和舵机控制，支持键盘、手柄、语音等多种输入方式。
- **视觉处理层**：利用 RK3588S 的 NPU（6TOPS 算力）硬件加速 YOLOv8 目标检测，三个视觉组件（采集、推理、推流）运行在同一 ComposableNodeContainer 中，通过零拷贝通信降低延迟。
- **交互层**：集成 sherpa-onnx ASR 语音识别、RKLLM 本地大语言模型推理和百度大模型音色复刻 TTS 语音合成，实现全流式语音交互；同时通过 Foxglove Bridge 提供 Web 端远程监控与数字孪生可视化。

# 🎬 视频演示

[![视频封面](docs/source/_static/recovered/ylsonn.jpg)](https://www.bilibili.com/video/BV1hQcFzrEXS/?spm_id_from=333.1007.top_right_bar_window_history.content.click)

> 📺 **视频作者**：B站 @鸿尘客
> 🔗 **视频链接**：[黄油机器人](https://www.bilibili.com/video/BV1hQcFzrEXS/?spm_id_from=333.1007.top_right_bar_window_history.content.click)

# ⚙️ 硬件规格

|     项目     |                      参数                      |
| :----------: | :--------------------------------------------: |
|  主芯片 SOC  |                Rockchip RK3588S                |
|     CPU      | 4 核 Cortex-A76(2.4GHZ)+4 核 Cortex-A5(1.8GHZ) |
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

# 📐 硬件框图

[![硬件框图](docs/source/_static/recovered/ylD0Gx.png)](docs/source/_static/recovered/ylD0Gx.png)

# 🚀 产品功能

> **五大核心能力**：语音交互 | 运动控制 | 视觉处理 | 定位导航 | 远程监控

---

## 🗣️ 语音交互 —— 全链路语音对话，开口即控制

|        模块        | 核心能力                                   | 技术方案                                                         |
| :----------------: | :----------------------------------------- | :--------------------------------------------------------------- |
|  **ASR 语音识别**  | 中英双语实时识别，增量输出无需等待整句     | sherpa-onnx + RKNN 加速 Zipformer，48kHz→16kHz 重采样，端点检测  |
| **LLM 大语言模型** | 本地多模态推理，语音触发拍照+图像理解      | Qwen3-VL-2B W8A8 量化，RK3588 NPU 加速，4096 上下文，流式输出    |
|  **TTS 语音合成**  | 自定义音色克隆，支持多种方言，边合成边播放 | 百度大模型音色复刻 API，句级分段合成，HTTP 流式推送（端口 8090） |

## 🎮 运动控制 —— 多模式操控，精准灵活

|       模块       | 核心能力                                     | 技术方案                                                           |
| :--------------: | :------------------------------------------- | :----------------------------------------------------------------- |
| **差速驱动底盘** | PID 闭环控制，实时调参，最大线速度 0.15 m/s  | UART 460800 波特率通信 ESP32S3，自定义二进制协议，左右电机独立控制 |
|   **舵机控制**   | 3 路舵机控制脊柱和双臂关节，URDF 实时联动    | `/joint_states` 话题发布关节状态，角度精确控制                     |
|   **键盘遥控**   | WASD 八方向移动，0.1 秒无操作自动停车        | 100Hz 轮询频率                                                     |
|   **手柄遥控**   | 摇杆模拟量控制速度，按键控制舵机（±5° 步进） | 线速度 + 角速度双轴控制                                            |
|   **语音遥控**   | 通过语音交互链路下发运动指令                 | ASR → LLM → 运动指令                                               |

## 👁️ 视觉处理 —— NPU 硬件加速，零拷贝低延迟

|      模块      | 核心能力                                    | 技术方案                                   |
| :------------: | :------------------------------------------ | :----------------------------------------- |
| **摄像头采集** | 1080P@30fps 硬件加速采集，支持 RGA 硬件旋转 | GStreamer + v4l2src，NV12 原生格式输出     |
|  **目标检测**  | 黄油/手掌检测 + 自主跟随                    | YOLOv8 + RKNN NPU 加速，640x640，NMS 0.45  |
|  **视频推流**  | RTSP 实时推流，零拷贝进程内通信             | MPP 硬件 H.264 编码，ComposableNode 零拷贝 |

## 🗺️ 定位导航 —— SLAM 建图 + Nav2 自主导航

|      模块      | 核心能力                                      | 技术方案                                         |
| :------------: | :-------------------------------------------- | :----------------------------------------------- |
| **里程计融合** | 50Hz EKF 融合轮式里程计与 IMU，输出高精度位姿 | robot_localization，2D 模式，odom → base_link TF |
| **SLAM 建图**  | 在线异步建图，回环检测，0.05 m/pixel 分辨率   | slam_toolbox + Ceres 求解器，激光量程 12.0 m     |
| **纯定位模式** | 加载已有地图重定位，过滤自身遮挡              | slam_toolbox 定位模式 + laser_filters            |
|  **自主导航**  | 全局路径规划 + 局部避障                       | Nav2 导航栈，DWA / TEB 规划算法                  |

## 🖥️ 远程监控 —— Web 端数字孪生，随时随地掌控

|        能力         | 说明                                                           |
| :-----------------: | :------------------------------------------------------------- |
| **Foxglove 可视化** | WebSocket（端口 8765）实时查看机器人状态、话题数据和 TF 坐标树 |
|   **RTSP 视频流**   | 远程查看摄像头画面及目标检测结果                               |
|    **远程控制**     | Web 端发布控制指令，实现数字孪生效果                           |
