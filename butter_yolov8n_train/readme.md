# 全球首发！瑞克和莫蒂 专属定制黄油机器人！

> 本项目是 [DIY_hyrobot](https://github.com/zhanghongchen1213/DIY_hyrobot) 的 `butter_yolov8n_train/` 子目录。首次使用请先阅读 [资源获取与复现说明](https://github.com/zhanghongchen1213/DIY_hyrobot/blob/main/hy_ros/docs/source/7_Resources.md)；本地独立开发与同步见 [维护指南](https://github.com/zhanghongchen1213/DIY_hyrobot/blob/main/hy_ros/docs/source/8_Maintenance.md)。

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

## 🧠 YOLOv8n 目标检测训练

本仓库为黄油机器人提供完整的 **YOLOv8n 目标检测模型训练流程**，从数据准备到模型导出一站式完成，训练产出可直接部署至 RK3588S NPU 进行实时推理。

### 📦 数据集

|   项目   |                                              参数                                               |
| :------: | :---------------------------------------------------------------------------------------------: |
| 数据来源 | [Roboflow - butter_robot v3](https://universe.roboflow.com/butter-robot/butter_robot/dataset/3) |
| 检测类别 |                                       `butter`（单类别）                                        |
|  训练集  |                                        2,689 张标注图像                                         |
|  验证集  |                                         267 张标注图像                                          |
|  测试集  |                                         116 张标注图像                                          |
| 标注格式 |                                       YOLO 归一化坐标格式                                       |
| 开源协议 |                                            CC BY 4.0                                            |

### ⚡ 训练配置

|    项目    |          参数          |
| :--------: | :--------------------: |
|  基础模型  | YOLOv8n（Nano 轻量级） |
|  训练轮数  |       120 Epochs       |
|  批次大小  |           64           |
| 输入分辨率 |       640 × 640        |
|   优化器   | SGD（momentum=0.937）  |
| 初始学习率 |          0.01          |
|  训练设备  |     CUDA GPU 加速      |
|  混合精度  |  AMP 自动混合精度训练  |

### 🎯 数据增强策略

- **Mosaic 拼接增强**：4 张图像拼接，提升小目标检测能力
- **HSV 色彩空间增强**：色调 ±0.015、饱和度 ±0.7、明度 ±0.4
- **水平翻转**：50% 概率随机水平翻转
- **随机擦除**：40% 概率遮挡区域，增强鲁棒性
- **RandAugment**：自动数据增强策略
- **尺度缩放**：±0.5 倍随机缩放

### 📤 模型导出

|   导出格式    |       用途       |        文件大小         |
| :-----------: | :--------------: | :---------------------: |
| PyTorch (.pt) |    训练与验证    |         ~6.0 MB         |
| ONNX (.onnx)  |    跨平台推理    |        ~11.5 MB         |
|     RKNN      | RK3588S NPU 部署 | 通过 RKNN-Toolkit2 转换 |

### 🔬 训练产出

训练完成后自动生成以下评估文件：

- `results.csv` — 逐轮训练指标记录
- `results.png` — 训练损失与指标曲线
- `confusion_matrix.png` — 混淆矩阵
- `F1_curve.png` — F1 分数曲线
- `PR_curve.png` — 精确率-召回率曲线
- `P_curve.png` / `R_curve.png` — 精确率 / 召回率曲线
- `labels.jpg` — 数据集标签分布可视化

### 🤖 部署目标

训练完成的模型最终部署于黄油机器人的 **RK3588S NPU（6TOPS）** 上，实现：

- 实时黄油目标检测与定位
- 与 ROS2 视觉处理节点无缝集成
- 采集、推理、推流三组件零拷贝通信，低延迟运行
