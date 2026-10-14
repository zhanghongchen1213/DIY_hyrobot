# 准备模型与软件资源

开始配置机器人前，先在 Ubuntu 主机准备测试文件和模型，再通过 NFS 共享给 RK3588S 端侧。按下面的目录放置文件，后续教程中的 `/mnt/nfs/hy_ros` 路径就能直接使用。

## 克隆项目并准备测试文件

在 Ubuntu 主机安装 Git、Python 3 和 curl，并将项目放到 NFS 共享目录下：

```bash
sudo apt update
sudo apt install -y git python3 curl
mkdir -p "$HOME/hy_linux/nfs"
cd "$HOME/hy_linux/nfs"
git clone https://github.com/zhanghongchen1213/DIY_hyrobot.git
cd DIY_hyrobot
python3 hy_ros/tools/fetch_source.py --group project
```

项目文件会准备到 `hy_ros/source/`，包括：

| 路径 | 用途 |
| --- | --- |
| `1.Lidar/wheeltec_udev.sh`、`1.Lidar/ldlidar_driver_python3.py` | 雷达串口配置与 Python 测试 |
| `0.RK3588S/3.butter_yolov8n/test.py`、`butter.jpg`、`yolov8.rknn` | 使用预训练模型测试黄油检测 |

直接运行预训练检测模型时，无需先训练 YOLO。雷达和相机的接线及测试步骤见[硬件连接与测试](development/hardware_testing.md)。

## 下载语音模型

以下下载命令均在 Ubuntu 主机的项目根目录执行。脚本按 `hy_ros/resources/source-manifest.json` 自动校验下载文件；遇到同名但内容不同的文件会停止，先将需要保留的文件移到其他位置，再重试。

### Sherpa-ONNX 流式语音识别

```bash
cd "$HOME/hy_linux/nfs/DIY_hyrobot"
python3 hy_ros/tools/fetch_source.py --group asr
```

该命令准备以下资源：

- Sherpa-ONNX `1.12.19`，Python 3.10 / aarch64 wheel。安装说明见[官方 RKNPU 安装入口](https://k2-fsa.github.io/sherpa/onnx/rknn/install.html)，[wheel 下载地址](https://huggingface.co/csukuangfj/sherpa-onnx-wheels/resolve/main/rknn/1.12.19/sherpa_onnx-1.12.19-cp310-cp310-manylinux_2_27_aarch64.whl)。
- [流式中英双语模型](https://github.com/k2-fsa/sherpa-onnx/releases/download/asr-models/sherpa-onnx-rk3588-streaming-zipformer-small-bilingual-zh-en-2023-02-16.tar.bz2)：`sherpa-onnx-rk3588-streaming-zipformer-small-bilingual-zh-en-2023-02-16`。
- RKNN Toolkit2 `v2.3.0` 的 aarch64 [`librknnrt.so`](https://raw.githubusercontent.com/airockchip/rknn-toolkit2/a8dd54d41e92c95b4f95780ed0534362b2c98b92/rknpu2/runtime/Linux/librknn_api/aarch64/librknnrt.so)，固定提交 `a8dd54d41e92c95b4f95780ed0534362b2c98b92`。

文件位于 `hy_ros/source/0.RK3588S/1.sherpa-onnx/`。模型仍是 `.tar.bz2` 压缩包，挂载 NFS 后，按[语音识别与合成](development/voice.md)复制到端侧、安装 wheel、解压模型并测试。主机下载步骤不会安装端侧 ARM wheel。

```{important}
[Sherpa 官方兼容说明](https://k2-fsa.github.io/sherpa/onnx/rknn/models.html)提示部分 `librknnrt` 2.3.x / 2.4.x 组合可能崩溃，并列出 2.2.0 为已知可用版本。若识别程序崩溃，按该说明使用仅对 ASR 进程生效的 `LD_LIBRARY_PATH` 测试兼容版本；更换全局运行库会同时影响 YOLO 与 RKLLM，需一并检查。
```

### Qwen3-VL-2B 模型

```bash
cd "$HOME/hy_linux/nfs/DIY_hyrobot"
python3 hy_ros/tools/fetch_source.py --group qwen
```

两个模型共约 3.23 GB，下载前确认主机有足够空间。文件会放到 `hy_ros/source/0.RK3588S/2.rknn-llm/`：

- `qwen3-vl-2b-instruct_w8a8_rk3588.rkllm`
- `qwen3-vl-2b_vision_rk3588.rknn`

下载来源为 [JiahaoLi 的公开模型镜像](https://huggingface.co/JiahaoLi/Qwen3-VL-RK3588/tree/025f0aa9b6ed2b22351acb2ebca30f4bd9ecdfe4)，固定提交 `025f0aa9b6ed2b22351acb2ebca30f4bd9ecdfe4`。官方入口见 [RKLLM 1.2.3 模型下载说明](https://github.com/airockchip/rknn-llm/blob/release-v1.2.3/README.md)，提取码为 `rkllm`。

模型安装、SDK 编译与运行测试见[语音识别与合成](development/voice.md)。百度 TTS 另需自己的百度账号及 `BAIDU_API_KEY`、`BAIDU_SECRET_KEY`、`BAIDU_VOICE_ID`，按同一章节配置；自定义音色录音可通过 `BAIDU_VOICE_SAMPLE` 指定。

## SDK、相机与可选训练资源

### RKLLM SDK

模型部署使用 [RKLLM `release-v1.2.3`](https://github.com/airockchip/rknn-llm/tree/f7df8e5de9b40dc7aaa66194cf14d4895aaf159a)，固定提交 `f7df8e5de9b40dc7aaa66194cf14d4895aaf159a`。完成 NFS 挂载后，按[语音章节](development/voice.md)在端侧获取和编译 SDK，使用项目提供的 `hy_ros/resources/rkllm/build-linux.sh` 配置编译器路径。模型与运行库应使用该章节的配套版本。

### 相机测试与视频处理

相机测试使用 [mjpg-streamer](https://github.com/jacksonliam/mjpg-streamer/tree/310b29f4a94c46652b20c4b7b6e5cf24e532af39)，固定提交 `310b29f4a94c46652b20c4b7b6e5cf24e532af39`。按[硬件连接与测试](development/hardware_testing.md)在端侧克隆、编译，并检查实际摄像头设备及分辨率。

ROS 视觉流水线另需 MPP、RGA 和 GStreamer 环境，按[视觉采集、推理与推流](development/vision.md)准备。

### 自行训练 YOLO（可选）

需要重新训练时，从 [Roboflow butter_robot v3](https://universe.roboflow.com/butter-robot/butter_robot/dataset/3) 下载 YOLOv8 导出包，并核对下载页的许可。解压到 `butter_yolov8n_train/Dataset/`，使其包含 `train/`、`valid/`、`test/` 的 images 与 labels 目录。

在训练电脑上进入 `butter_yolov8n_train`，使用该目录的 `dataset.yaml` 和 Notebook。数据准备、CUDA 环境及 ONNX → RKNN 转换见[目标检测模型训练与转换](development/model_training.md)。也可以自行采集图片并标注，图片准备本身不需要项目预训练模型。

### 端侧系统与驱动固件

端侧系统镜像使用 `Orangepi5pro_1.0.6_ubuntu_jammy_server_linux5.10.160`，烧录和网络配置见[端侧环境配置](development/device_setup.md)。ESP32-S3 工具链要求 ESP-IDF `>=5.1.5,<5.4.0`，编译和烧录见[ESP32 驱动固件说明](https://github.com/zhanghongchen1213/DIY_hyrobot/blob/main/hy_driver/readme.md)。IP、串口和硬件参数以实际设备为准。

## 配置 NFS 并检查端侧资源路径

### Ubuntu 主机导出共享目录

在共享目录下建立 `hy_ros` 链接，使端侧能通过 `/mnt/nfs/hy_ros` 访问项目。如果该位置已有 `hy_ros`，先确认它指向本项目，再跳过 `ln -s` 命令。

```bash
cd "$HOME/hy_linux/nfs"
ln -s DIY_hyrobot/hy_ros hy_ros
sudo apt install -y nfs-kernel-server
# 将网段替换为机器人所在局域网的实际网段。
printf '%s 192.168.22.0/24(rw,sync,no_subtree_check)\n' "$HOME/hy_linux/nfs" | sudo tee /etc/exports.d/diy_hyrobot.exports
sudo exportfs -ra
sudo systemctl restart nfs-kernel-server
```

### RK3588S 端侧挂载与检查

下例的主机 IP 为 `192.168.22.177`，共享目录为 `/home/k/hy_linux/nfs`。将这两处替换为 Ubuntu 主机的实际 IP 和绝对路径，端侧挂载点保持 `/mnt/nfs`：

```bash
sudo apt update
sudo apt install -y nfs-common
sudo mkdir -p /mnt/nfs
sudo mount -t nfs -o nolock 192.168.22.177:/home/k/hy_linux/nfs /mnt/nfs
ls /mnt/nfs/hy_ros/source
```

需要语音功能时，确认 `0.RK3588S/1.sherpa-onnx/` 和 `0.RK3588S/2.rknn-llm/` 中的文件可见；检测测试文件位于 `0.RK3588S/3.butter_yolov8n/`，雷达测试文件位于 `1.Lidar/`。随后按各功能章节将所需文件复制到端侧 `/opt` 目录。遇到读取权限问题，先检查主机目录的所有者和读权限。

需要同时准备三组文件或重新检查文件完整性时，在 Ubuntu 主机执行：

```bash
cd "$HOME/hy_linux/nfs/DIY_hyrobot"
python3 hy_ros/tools/fetch_source.py --group all
python3 hy_ros/tools/fetch_source.py --group all --verify-only
```

`all` 包含项目测试文件、ASR 和 Qwen 模型；SDK、相机工具与训练数据集按上面的对应章节准备。NFS 开机挂载设置见[端侧环境配置](development/device_setup.md)。

## SolidWorks 机械模型（可选）

自行装配、了解结构或修改机械件时，可下载 [SolidWorks 模型包](https://github.com/zhanghongchen1213/DIY_hyrobot/releases/tag/hardware-2026-10-13)。在 Ubuntu 主机执行：

```bash
cd "$HOME/hy_linux/nfs/DIY_hyrobot"
sudo apt install -y p7zip-full
python3 tools/download_resources.py --group Solidworks
python3 tools/download_resources.py --group Solidworks --verify-only
```

模型解压到 `Solidworks/`，总装文件为 `Solidworks/黄油机器人/黄油机器人总装.SLDASM`。Windows 可安装 7-Zip，并用 `python` 执行下载脚本；在 SolidWorks 中打开总装时保留零件的目录关系。
