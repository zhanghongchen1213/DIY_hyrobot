# 开始前的准备

这里整理黄油机器人的硬件、软件与模型准备方法，以及各功能的部署步骤。可以按顺序从零复刻，也可以根据已有环境和需要选择章节。

## 硬件与工具

- 黄油机器人主控板、ESP32-S3 驱动板及配套供电。
- 相机、雷达、麦克风、扬声器和底盘等外设，按实际硬件连接。
- 系统 TF 卡、读卡器，以及刷写驱动板使用的 USB 数据线。
- 一台开发电脑，和机器人连接同一局域网。

硬件规格与框图见[项目介绍](1_product_introduction.md)，电路资料见[立创开源硬件入口](https://oshwhub.com/nhwjxzxz/huang-you-ji-qi-ren)，机械模型获取方式见[资源准备](7_Resources.md#solidworks-机械模型可选)。自行选配硬件时，设备接口、引脚和运行参数需要相应调整。

主控使用 RK3588S，驱动板使用 ESP32-S3。系统和驱动固件已经准备好的设备，可以先进行硬件测试；重新安装时从环境配置开始。

## 软件环境

| 用途 | 使用环境 | 对应章节 |
| --- | --- | --- |
| 开发电脑与文件共享 | Ubuntu 22.04、ROS 2 Humble、NFS | 主机环境配置 |
| 机器人主控 | 香橙派 Ubuntu 22.04 镜像、Python 3.10、ROS 2 Humble | 端侧环境配置 |
| 重建驱动固件 | ESP-IDF `>=5.1.5,<5.4.0`，项目使用 5.1.5 | 驱动固件准备 |
| 自行训练模型（可选） | Python 3.12、NVIDIA CUDA GPU | 模型训练与转换 |

对应操作：[主机环境配置](development/host_setup.md) · [端侧环境配置](development/device_setup.md) · [驱动固件准备](7_Resources.md#端侧系统与驱动固件) · [模型训练与转换](development/model_training.md)。

板端的 Python 3.10 与训练电脑的 Python 3.12 用途不同。ASR 的 ARM wheel 安装在机器人上，训练环境安装在训练电脑上。

## 先记录设备信息

| 需要填写的内容 | 获取方式 | 用途 |
| --- | --- | --- |
| 开发电脑 IP | 在电脑执行 `ip addr` | NFS 服务器地址 |
| 机器人 IP | 连接网络后执行 `ip addr` | SSH、Foxglove 和视频网页地址 |
| 驱动板 USB 串口 | 查看系统串口设备 | ESP32-S3 刷写与调试 |
| 相机设备 | 执行 `v4l2-ctl --list-devices` | 相机采集配置 |
| 麦克风设备 | 执行 `arecord -l` | 录音与 ASR 输入配置 |

教程里的 IP、设备号和用户名是示例，执行前替换为自己的值。`/dev/ttyS3` 是主控与驱动板通信的示例端口，与电脑刷写驱动板的 USB 串口用途不同。

## 准备程序与模型

按[资源准备](7_Resources.md)下载项目代码、外设测试文件、黄油预训练模型、ASR 与 Qwen 模型，并配置 NFS。使用提供的预训练模型即可进行视觉部署，无需先训练；数据集和机械模型属于可选资源。

ESP32-S3 固件需要重新编译或刷写时，进入下载项目的 `hy_driver/`，按该目录的 README 操作。需要舵机动作时检查固件的 `SERVO_DEBUG` 开关；当前项目配置未开启舵机初始化。

## 按顺序完成复刻

1. [主机环境配置](development/host_setup.md)：准备开发电脑、项目文件和 NFS 共享。
2. [端侧环境配置](development/device_setup.md)：安装系统、联网、SSH、挂载 NFS 和 ROS 2。
3. [硬件连接与测试](development/hardware_testing.md)：检查相机、雷达、麦克风和串口。
4. [语音识别与合成](development/voice.md)：部署模型、配置自己的百度音色账号。
5. [Foxglove 可视化](development/visualization.md)：连接机器人并查看话题与 TF。
6. [视觉检测与视频推流](development/vision.md)：运行黄油检测并查看视频。
7. [SLAM 建图与定位](development/slam.md)：准备底盘、里程计和雷达，保存地图并加载定位。

每一步确认输出符合该节说明，再继续下一步。需要重新训练检测类别时阅读[模型训练与转换](development/model_training.md)。[Nav2 导航与任务原型](development/navigation.md)属于扩展开发，其中检测、运动和抓取接口尚需集成，不能直接作为完整抓取流程运行。
