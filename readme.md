# 全球首发！瑞克和莫蒂 专属定制黄油机器人！

> 本项目是 [DIY_hyrobot](https://github.com/zhanghongchen1213/DIY_hyrobot) 的 `hy_driver/` 子目录。首次使用请先阅读 [资源获取与复现说明](https://github.com/zhanghongchen1213/DIY_hyrobot/blob/main/hy_ros/docs/source/7_Resources.md)；本地独立开发与同步见 [维护指南](https://github.com/zhanghongchen1213/DIY_hyrobot/blob/main/hy_ros/docs/source/8_Maintenance.md)。

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
| [![yl9J6U.md.png](../hy_ros/docs/source/_static/recovered/yl9J6U.png)](../hy_ros/docs/source/_static/recovered/yl9J6U.png) | [![yl9EAF.md.png](../hy_ros/docs/source/_static/recovered/yl9EAF.png)](../hy_ros/docs/source/_static/recovered/yl9EAF.png) |
| [![yl9BC0.md.png](../hy_ros/docs/source/_static/recovered/yl9BC0.png)](../hy_ros/docs/source/_static/recovered/yl9BC0.png) | [![yl9lw9.md.png](../hy_ros/docs/source/_static/recovered/yl9lw9.png)](../hy_ros/docs/source/_static/recovered/yl9lw9.png) |
| [![yl9KLc.md.png](../hy_ros/docs/source/_static/recovered/yl9KLc.png)](../hy_ros/docs/source/_static/recovered/yl9KLc.png) | [![yl9YdX.md.png](../hy_ros/docs/source/_static/recovered/yl9YdX.png)](../hy_ros/docs/source/_static/recovered/yl9YdX.png) |
| [![yl9GcO.md.png](../hy_ros/docs/source/_static/recovered/yl9GcO.png)](../hy_ros/docs/source/_static/recovered/yl9GcO.png) | [![yl9m6d.md.png](../hy_ros/docs/source/_static/recovered/yl9m6d.png)](../hy_ros/docs/source/_static/recovered/yl9m6d.png) |

# 🏗️ 系统架构

系统采用分层模块化架构，主要分为以下五层：

- **感知层**：集成 LDLiDAR 2D 激光雷达（360° 环境扫描）、1080P 摄像头、6 轴 IMU 惯性测量单元和轮式编码器里程计，提供多源环境感知能力。
- **定位与建图层**：基于 slam_toolbox 实现在线异步 SLAM 建图，通过 robot_localization 扩展卡尔曼滤波（EKF）融合轮式里程计与 IMU 数据，并使用 laser_filters 过滤机器人自身遮挡。
- **控制层**：通过 UART 串口（460800 波特率）与 ESP32S3 通信，实现差速驱动底盘的 PID 速度控制和舵机控制，支持键盘、手柄、语音等多种输入方式。
- **视觉处理层**：利用 RK3588S 的 NPU（6TOPS 算力）硬件加速 YOLOv8 目标检测，三个视觉组件（采集、推理、推流）运行在同一 ComposableNodeContainer 中，通过零拷贝通信降低延迟。
- **交互层**：集成 sherpa-onnx ASR 语音识别、RKLLM 本地大语言模型推理和百度大模型音色复刻 TTS 语音合成，实现全流式语音交互；同时通过 Foxglove Bridge 提供 Web 端远程监控与数字孪生可视化。

# 🎬 视频演示

[![点击观看视频](../hy_ros/docs/source/_static/recovered/ylsonn.jpg)](https://www.bilibili.com/video/BV1hQcFzrEXS/?vd_source=d74d3e2c07a341457b5e708db2594eab)

> 📺 **视频作者**：B站 @鸿尘客
> 🔗 **视频链接**：[黄油机器人](https://www.bilibili.com/video/BV1hQcFzrEXS/?vd_source=d74d3e2c07a341457b5e708db2594eab)

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

[![ylD0Gx.md.png](../hy_ros/docs/source/_static/recovered/ylD0Gx.png)](../hy_ros/docs/source/_static/recovered/ylD0Gx.png)

# 🚀 产品功能

## 🧠 自主导航与建图

- **SLAM 实时建图**：基于 slam_toolbox 实现在线异步 SLAM，搭配 LDLiDAR 2D 激光雷达（360° 扫描），实时构建环境地图
- **多源融合定位**：通过 robot_localization 扩展卡尔曼滤波（EKF），融合轮式里程计与 QMI8658 六轴 IMU 数据，提供高精度位姿估计
- **自主路径规划**：基于 Nav2 导航框架，支持全局路径规划与局部避障，实现自主点到点导航
- **激光滤波**：使用 laser_filters 过滤机器人自身遮挡区域，提升建图与导航可靠性

## 🏎️ 差速驱动底盘

- **双电机驱动**：采用 TB6612FNG 双 H 桥驱动 N20 减速电机（34 RPM 额定），支持前进、后退、原地转向
- **PID 闭环控制**：左右轮独立 PID 速度控制，支持实时参数在线调节（Kp/Ki/Kd）
- **高精度编码器**：霍尔编码器 + PCNT 硬件四倍频解码（2654.1 PPR），实时反馈转速
- **差分运动学**：完整的正/逆运动学解算，线速度/角速度与左右轮转速自动转换
- **里程计输出**：实时累积位置（x, y）、航向角（θ）、线速度与角速度，四元数姿态表示

## 🦾 三自由度舵机关节

- **三舵机控制**：右臂（GPIO8）、左臂（GPIO19）、腰部（GPIO20），LEDC PWM 50Hz 驱动
- **统一角度系统**：0°~60° 逻辑角度范围，每个舵机独立零位校准
- **速度可控**：支持慢速/正常/快速三档运动模式，平滑插值过渡避免抖动

## 🎤 语音交互系统

- **语音唤醒与识别**：集成 CI-03T 语音识别模块（UART2，115200 波特率），支持唤醒词触发
- **ASR 语音识别**：上位机端运行 sherpa-onnx 语音识别引擎，实现语音转文字
- **大模型对话**：支持 RKLLM 本地大语言模型推理 + ChatGPT 云端对话双模式
- **TTS 语音合成**：百度大模型音色复刻 TTS，全流式语音合成输出
- **全流式交互**：语音识别 → 大模型推理 → 语音合成全链路流式处理，低延迟对话体验

## 🔊 音频播放系统

- **硬件音频通路**：ES8311 音频编解码器 + I2S 数字音频接口 + 功放输出
- **MP3 播放**：基于 esp-audio-player 库，支持 SPIFFS 文件系统中的 MP3 文件播放
- **音量控制**：支持 0~100 级音量调节与静音功能
- **开机提示音**：系统启动自动播放欢迎音效

## 👁️ 视觉识别系统

- **1080P 摄像头**：1920×1080 @ 30FPS 高清图像采集
- **YOLOv8 目标检测**：利用 RK3588S NPU（6TOPS 算力）硬件加速推理
- **零拷贝架构**：视觉采集、推理、推流三组件运行在同一 ComposableNodeContainer，通过零拷贝通信降低延迟
- **实时视频推流**：支持远程实时画面查看

## 📡 多通道通信

- **高速串口（UART1）**：460800 波特率连接 RK3588S 主控，结构化数据包协议（起始标志 0xAA，上行结束 0x55 / 下行结束 0x66）
- **语音串口（UART2）**：115200 波特率连接 CI-03T 语音识别模块
- **Wi-Fi 连接**：ESP32-S3 STA 模式，支持自动重连与 NVS 凭证存储
- **蓝牙 BLE**：GATT Server 模式，支持低功耗蓝牙通信（条件编译可选）
- **实时数据上报**：电机状态、PID 参数、里程计、IMU 六轴数据、舵机角度、时间戳同步上传

## 🖥️ 远程监控与数字孪生

- **Foxglove Bridge**：通过 Web 端远程监控机器人状态
- **数字孪生可视化**：实时同步机器人位姿、关节角度、传感器数据至可视化界面
- **多输入控制**：支持键盘、手柄、语音等多种遥控方式
