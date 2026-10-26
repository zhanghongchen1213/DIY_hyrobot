<p align="center">
  <img src="https://image-osb.obs.cn-east-3.myhuaweicloud.com/blog/2026-01-17/hy-robot/r01/cover/cover-video-d4f5311027e4.webp" alt="封面：打破次元壁！DIY黄油机器人。瑞克举起实物黄油机器人，莫蒂在旁，身后是绿色传送门。" width="100%">
</p>

<p align="center">
  <img src="https://img.shields.io/badge/ROS%202-Humble-111111?style=flat-square" alt="ROS 2 Humble">
  <img src="https://img.shields.io/badge/C%2B%2B-14-111111?style=flat-square" alt="C++ 14">
  <img src="https://img.shields.io/badge/ESP--IDF-5.1-111111?style=flat-square" alt="ESP-IDF 5.1">
</p>

<p align="center">
  <b>黄油机器人的 ROS 2 与 ESP32-S3 源码。</b><br>
  换上自己的串口、地图和模型路径，就能在 RK3588S 与 ESP32-S3 上复刻感知、运动和语音链路。
</p>

<p align="center">
  <a href="#功能">功能</a> ·
  <a href="#跑起来">跑起来</a> ·
  <a href="#目录">目录</a> ·
  <a href="#改成你的">改成你的</a> ·
  <a href="#遇到问题时">遇到问题时</a> ·
  <a href="#交流与贡献">交流与贡献</a>
</p>

## 这是什么

黄油机器人（Butter Robot）是一台给复刻和二次开发用的移动机器人。上位机是 RK3588S 上的 Ubuntu 22.04 与 ROS 2 Humble，下位机是 ESP32-S3，负责电机、编码器、IMU 和音频。

仓库里有四块可以单独使用的内容：`hy_ros/` 是 ROS 2 工作区，`hy_driver/` 是 ESP-IDF 固件，`butter_yolov8n_train/` 是单类别黄油检测的训练环境，`hardware/` 是 SolidWorks 压缩包的下载入口。视觉和本地模型依赖板端的 RKNN、RKLLM 与多媒体库。Nav2 抓取目前是实验原型：检测结果、底盘速度和舵机还没有接成闭环。

长文复刻教程已从本仓库移除。电路资料在[立创开源黄油机器人](https://oshwhub.com/nhwjxzxz/huang-you-ji-qi-ren)。

## 功能

| 能力 | 读者得到的结果 |
| --- | --- |
| **底盘通信** | `uart` 经 `/dev/ttyS3`、460800 波特率与 ESP32-S3 交换轮速、里程计、IMU 和电机控制 |
| **键盘与手柄** | 键盘向 `/uart/motor_control` 发布差速指令；手柄走 `/joy` |
| **里程计融合** | `ekf_odom` 融合轮式里程计与 IMU，发布 `odom` → `base_link` |
| **激光雷达** | `ldlidar_driver_ros2` 向 `/scan` 发布 `LaserScan` |
| **建图与定位** | `hy_slam` 用 slam_toolbox 做在线建图，并可加载地图做定位 |
| **导航原型** | `hy_nav2` 提供 Nav2 参数和任务状态机；自主搜索、抓取、返回尚未闭环 |
| **视觉** | 相机采集、YOLOv8 RKNN 标注和 RTSP 推流。推理节点发布标注图，不发布 `/detections` |
| **语音** | ASR 发布 `/audio/asr_text`，RKLLM 做本地文本生成，百度 TTS 在 8090 端口提供音频流 |
| **远程查看** | `all_launch` 启动 Foxglove Bridge，WebSocket 端口 8765 |
| **模型训练** | Notebook 训练 YOLOv8n 的 `butter` 类，并导出 ONNX；`.rknn` 需另行转换 |
| **机械模型** | 从本仓库 Release 下载 SolidWorks 总装 |

`all_launch` 默认拉起串口、EKF、机器人描述、Foxglove 和视觉容器。语音启动项在该文件里被注释，雷达、SLAM 和 Nav2 需要另开终端。

## 跑起来

按要做的事选择环境。

| 场景 | 门槛 |
| --- | --- |
| ROS 2 | Ubuntu 22.04、ROS 2 Humble；视觉与本地模型在 RK3588S 上运行。多个 C++ 包使用 C++14 |
| 固件 | ESP-IDF `>=5.1.5,<5.4.0`，`dependencies.lock` 锁定 5.1.5，目标芯片 `esp32s3` |
| 训练 | Python 3.12、uv、NVIDIA CUDA GPU。锁文件中的 PyTorch 为 2.5.1、CUDA 12.4 |
| 板端语音资源 | Sherpa-ONNX 1.12.19 的 Python 3.10 / aarch64 wheel |
| 机械模型 | Python 3.9+ 与 7-Zip |

先克隆仓库。各目录是普通 Git 文件，不是 submodule。

```bash
git clone https://github.com/zhanghongchen1213/DIY_hyrobot.git
cd DIY_hyrobot
python3 hy_ros/tools/fetch_source.py --group project
python3 hy_ros/tools/fetch_source.py --group project --verify-only
```

`--group` 还可取 `asr`、`qwen` 或 `all`。`hy_ros/source/`、训练用的 `Dataset/` 和根目录 `Solidworks/` 被 Git 忽略，需要用脚本或自行准备还原。相机源码、RKLLM SDK 与数据集的固定版本和校验记录见 [source-manifest.json](hy_ros/resources/source-manifest.json)。`rosdep` 不能补齐全部 RKNN / RKLLM 库。

### ROS 2

在已经准备好板端依赖的机器上：

```bash
cd hy_ros
source /opt/ros/humble/setup.bash
colcon build
source install/setup.bash
```

只编译一部分时，把包名换成下表中的包，例如 `colcon build --packages-up-to uart`。每个 launch 占一个终端，并先加载上面的环境。

| 用途 | 命令 |
| --- | --- |
| 串口、EKF、描述、Foxglove、视觉 | `ros2 launch all_launch all_launch.launch.py` |
| 底盘通信 | `ros2 launch uart uart.launch.py` |
| 里程计融合 | `ros2 launch ekf_odom ekf_odom.launch.py` |
| 机器人描述 | `ros2 launch butter_robot_description butter_robot_description.launch.py` |
| 雷达 | `ros2 launch ldlidar_driver_ros2 ldlidar_driver.launch.py` |
| 键盘 | `ros2 run keyboard_control keyboard_control_node --ros-args -p control_topic:=/uart/motor_control -p check_rate:=50.0 -p timeout_sec:=0.2` |
| ASR / 本地模型 / TTS | `ros2 launch audio_asr audio_asr.launch.py`、`ros2 launch audio_rkllm audio_rkllm.launch.py`、`ros2 launch audio_tts audio_tts.launch.py` |
| 建图 | `ros2 launch hy_slam slam.launch.py` |
| Nav2 | `ros2 launch hy_nav2 nav2_bringup.launch.py` |

键盘节点需要真实终端的标准输入。`all_launch` 已经包含串口、EKF、描述和视觉时，不要再单独启动这些节点。导航栈还依赖已安装的 Nav2、地图，以及另行启动的雷达；步骤见 [hy_nav2/README.md](hy_ros/src/hy_nav2/README.md)。

### 固件

先按 ESP-IDF 的方式安装工具链并激活终端，再执行：

```bash
cd hy_driver
idf.py set-target esp32s3
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor
```

`/dev/ttyUSB0` 换成实际串口，Windows 上例如 `COM3`。`set-target` 会重新生成工程配置。

### 训练

在有 NVIDIA GPU 的 Windows 或 Linux 上，从仓库根目录执行：

```bash
cd butter_yolov8n_train
uv sync --project butter_train_env --frozen --python 3.12
uv run --project butter_train_env --frozen python -m ipykernel install --user --name butter_train --display-name "Python (butter_train)"
```

用 `Python (butter_train)` 内核打开 `butter_yolov8n_train.ipynb`。数据集放到被 Git 忽略的 `Dataset/`，目录约定见 [训练说明](butter_yolov8n_train/readme.md)。查看已有曲线：

```bash
uv run --project butter_train_env --frozen tensorboard --logdir butter_train_results/yolov8n_butter --port 6006
```

Notebook 导出的是 ONNX，不直接生成 `.rknn`。随后在 RKNN-Toolkit2 环境转换，见 [训练说明](butter_yolov8n_train/readme.md)。

### 机械模型

下载机械模型：

```bash
python3 tools/download_resources.py --group Solidworks
python3 tools/download_resources.py --group Solidworks --verify-only
```

解压结果在 `Solidworks/`，总装文件是 `Solidworks/黄油机器人/黄油机器人总装.SLDASM`。压缩包来自 [hardware-2026-10-13](https://github.com/zhanghongchen1213/DIY_hyrobot/releases/tag/hardware-2026-10-13)。

## 目录

```text
DIY_hyrobot/
├── hy_ros/                         # ROS 2 工作区，在此执行 colcon
│   ├── src/                        # 15 个功能包
│   ├── docs/maintainer/            # 仓库维护说明
│   ├── images/                     # 外观、封面与硬件框图
│   └── tools/fetch_source.py
├── hy_driver/                      # ESP-IDF 工程，project(main_control)
├── butter_yolov8n_train/           # YOLOv8n 训练、uv 环境与历史结果
├── hardware/                       # SolidWorks Release 说明
├── tools/download_resources.py
└── resources/resources-manifest.json
```

| 包 | 作用 |
| --- | --- |
| `all_launch` | 聚合串口、EKF、描述、Foxglove 和视觉 |
| `uart` | 与 ESP32-S3 的串口协议 |
| `ekf_odom` | 轮式里程计与 IMU 融合 |
| `ldlidar_driver_ros2` | 乐动雷达驱动，发布 `/scan` |
| `hy_slam` | slam_toolbox 建图与定位 |
| `hy_nav2` | Nav2 参数和黄油任务原型 |
| `butter_robot_description` | URDF 与动态 TF |
| `rk_camera`、`rk_inference`、`rk_streamer` | 采集、RKNN 推理、RTSP 推流 |
| `audio_asr`、`audio_rkllm`、`audio_tts` | 识别、本地文本生成、语音合成 |
| `keyboard_control` | 键盘差速遥控 |
| `pid_debug_interfaces` | `MotorControl`、`MotorStatus` 消息 |

构建产物在 `hy_ros/build`、`hy_ros/install`、`hy_ros/log` 和 `hy_driver/build/`，这些目录已被忽略。训练曲线和权重在 `butter_yolov8n_train/butter_train_results/yolov8n_butter/`。

## 改成你的

| 要改的内容 | 文件 | 当前值 |
| --- | --- | --- |
| 底盘串口 | `hy_ros/src/uart/launch/uart.launch.py` | `port` 为 `/dev/ttyS3`，`baud` 为 `460800` |
| 雷达串口 | `hy_ros/src/ldlidar_driver_ros2/launch/ldlidar_driver.launch.py` | `port_name` 为 `/dev/ldlidar_serial`，`frame_id` 为 `radar_Link` |
| 相机 | `hy_ros/src/all_launch/launch/all_launch.launch.py` | `device_id` 为 `0`，分辨率 1920×1080、30 fps，图像话题 `/yolo/image_raw` |
| 检测模型 | 同上 | `model_path` 为 `/opt/rknn-toolkit2-lite/yolov8.rknn` |
| 推流地址 | 同上，`rk_streamer` 的 `pipeline` | `rtsp://127.0.0.1:8554/camera` |
| 语音识别 | `hy_ros/src/audio_asr/launch/audio_asr.launch.py` | 模型在 `/opt/sherpa-onnx/sherpa-onnx-rk3588-streaming-zipformer-small-bilingual-zh-en-2023-02-16`，`alsa_device` 为 `plughw:3,0` |
| 本地模型 | `hy_ros/src/audio_rkllm/launch/audio_rkllm.launch.py` | `llm_model_path` 为 `/opt/rknn-llm/qwen3-vl-2b-instruct_w8a8_rk3588.rkllm`，`enable_multimodal` 为 `False` |
| 语音合成 | 环境变量，由 `audio_tts.launch.py` 读取 | `BAIDU_API_KEY`、`BAIDU_SECRET_KEY`、`BAIDU_VOICE_ID`；`http_port` 为 `8090` |
| Wi-Fi | `hy_driver/components/BSP/Wifi/connect_wifi.c` | 改成自己的网络。不要把密码写进公开说明或提交 |
| 固件拉流地址 | `hy_driver/main/app_audio_control/app_audio.c` | 改成 RK3588S 上 TTS 服务的地址，端口与 `8090` 对齐 |
| 地图 | 安装后的 `hy_slam/map/my_map.yaml` | 仓库不包含这张地图，建图后按 [hy_nav2 说明](hy_ros/src/hy_nav2/README.md) 复制 yaml 和 pgm |
| 烧录串口 | `idf.py -p` 的参数 | 示例为 `/dev/ttyUSB0` |

改完 ROS 参数后，重新加载 `install/setup.bash` 再启动对应 launch。改完固件后，在已激活的 ESP-IDF 终端里重新 `idf.py build` 并烧录。舵机初始化由 `hy_driver/main/all_debug/all_debug.h` 的 `SERVO_DEBUG` 控制，当前为 `0`。

## 遇到问题时

| 现象 | 先检查 |
| --- | --- |
| 开发电脑上视觉包编不过 | 全量构建依赖 RK3588S 板端库。macOS 或 x86 主机适合改仓库说明、固件和训练工程 |
| `rosdep` 之后仍缺库 | RKNN、RKLLM 不在 `rosdep` 的覆盖范围内，版本与校验见 [source-manifest.json](hy_ros/resources/source-manifest.json) |
| 导航时底盘不动 | 看 `/cmd_vel` 是否有数据。当前 UART 不订阅该话题，需要自行接到 `MotorControl` |
| Nav2 启动时找不到地图 | `nav2_bringup.launch.py` 固定读取已安装的 `hy_slam/map/my_map.yaml`。先建图再复制 |
| 任务节点收不到黄油检测 | `rk_inference` 只发布标注图像，仓库里没有 `/detections` 的发布者 |
| 定位或规划异常 | 核对 [导航模块说明](hy_ros/src/hy_nav2/README.md) 中的地图路径、话题类型和尚未接通的接口 |
| ASR 进程崩溃 | 只用 `LD_LIBRARY_PATH` 给 ASR 进程指定兼容的 `librknnrt`，保持整机运行库不变 |
| 开了 `all_launch` 却没有语音、雷达或导航 | 该文件注释了 ASR、RKLLM 和 TTS，也不包含雷达、SLAM、Nav2 |

## 交流与贡献

部署和使用时遇到问题，可以到 [Issues](https://github.com/zhanghongchen1213/DIY_hyrobot/issues) 提问。用这套源码做出自己的成果后，也欢迎在 Issue 里留下地址。

发现 Bug 或有明确的改动建议，请 [提交 Issue](https://github.com/zhanghongchen1213/DIY_hyrobot/issues/new)。本仓库没有单独的贡献说明和安全政策，改代码前先用 Issue 说明改动范围。不要在公开 Issue 里贴密钥、令牌或环境变量文件。

**本文资料 · 接着动手**

**鸿尘客公众号**  
扫码关注鸿尘客公众号，回复 **“查看原文”**，获取本文开源资料与配套内容。

![鸿尘客公众号二维码](https://image-osb.obs.cn-east-3.myhuaweicloud.com/wemedia/wxlogo.webp)

**鸿尘客独立站**

点击下方获取本文**开源资料**与**配套内容**  
[开源资料：https://zhcmqtt.top/article/hy-robot](https://zhcmqtt.top/article/hy-robot)

![官网](https://image-osb.obs.cn-east-3.myhuaweicloud.com/wemedia/honchic_logo.webp)

## 许可

本仓库以 [MIT License](LICENSE) 授权，版权人是鸿尘客。

乐动雷达驱动 `ldlidar_driver_ros2` 保留上游 MIT 与 SHENZHEN LDROBOT CO., LTD. 的版权。ROS 2 生成的 linter 测试文件保留 Open Source Robotics Foundation 的 Apache-2.0 文件头。第三方依赖、模型与数据集保留各自许可。
