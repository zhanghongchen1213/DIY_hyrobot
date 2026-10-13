# 资源来源与教程复现检查

## 结论：source 不必整体上传

原教程已经提供 Sherpa-ONNX 在线安装、ASR 模型下载、Roboflow 数据集以及 RKNN 模型转换工具的链接。这些第三方资源可直接从上游获取，不需要在本仓库重复打包发布。

原 `hy_ros/source` 被 `.gitignore` 排除；里面还存在只写成 `cp /mnt/nfs/hy_ros/source/...` 的资源。这是作者的本地 NFS 路径，不是公开下载地址。本次逐项补齐获取路径和版本：普通代码和少量本项目文件直接存 Git，第三方资源从上游获取，只有 SolidWorks 做压缩发布。

## 一次克隆与项目自有资源

```bash
git clone https://github.com/zhanghongchen1213/DIY_hyrobot.git
cd DIY_hyrobot
python3 hy_ros/tools/fetch_source.py --group project
```

这一步无需网络下载二进制包：从本仓库 `hy_ros/resources/project/` 还原雷达测试脚本、udev 规则及 YOLO 的 `test.py`、`butter.jpg`、`yolov8.rknn`，共约 6.07 MB。它们是普通 Git 文件，没有压缩上传。YOLO 模型也可按照原教程训练、转换生成；提供现成模型用于直接验证端侧部署。

`hy_ros/source/` 继续被 Git 忽略。`hy_ros/resources/source-manifest.json` 保存所需文件的路径、尺寸和 SHA-256。下载工具保留已有的不同内容文件，不会覆盖自行训练的模型。

## Sherpa ASR 与 RKNN 运行库

```bash
python3 hy_ros/tools/fetch_source.py --group asr
```

- Sherpa wheel：沿用[官方 RKNPU 安装入口](https://k2-fsa.github.io/sherpa/onnx/rknn/install.html)，固定 `1.12.19 / cp310 / aarch64`。上游 wheel 的 SHA-256 与本地一致。
- 流式中英双语 ASR 模型：沿用教程中的 [Sherpa 官方 Release 下载](https://github.com/k2-fsa/sherpa-onnx/releases/download/asr-models/sherpa-onnx-rk3588-streaming-zipformer-small-bilingual-zh-en-2023-02-16.tar.bz2)，官方同名归档现为 49,676,089 字节，已实际下载并验证 GitHub 公布的 SHA-256。它与作者旧包不同，详见下方版本说明。
- `librknnrt.so`：来自 [RKNN Toolkit2 v2.3.0](https://github.com/airockchip/rknn-toolkit2/tree/a8dd54d41e92c95b4f95780ed0534362b2c98b92)，已用 Git blob 哈希验证与本地文件逐字节一致。

**ASR 归档版本说明：** 当前官方包的 SHA-256 为 `738aeb2ba4e3fe1e2d5d42a421c3e90e889e93c555a83f3a3d740fd0c6827d72`。本地旧包是 49,718,022 字节，SHA-256 为 `ff8d3c9b217d0a7834047ed93c4b7e2e5f785e29141d7bc425f49f653405fde3`。完整解压比对发现 9 个文件中 7 个相同，`encoder.rknn` 和 `joiner.rknn` 内容有差异；两版嵌入的转换器版本都为 2.1.0。不能把新版包称为作者旧包的逐字节备份，也未在机器人上验证新版识别效果。脚本为新下载校验当前官方包，同时接受并保留已有的作者旧包。

[Sherpa 官方模型说明](https://k2-fsa.github.io/sherpa/onnx/rknn/models.html)提示部分 `librknnrt` 2.3.x/2.4.x 组合可能崩溃，而 2.2.0 是其已知可用版本。这里保留作者实际使用的 2.3.0 校验来源；若 ASR 崩溃，可按官方说明用进程独立的 `LD_LIBRARY_PATH` 测试 2.2.0，避免直接替换全局库影响 YOLO/RKLLM。运行库组合与语音识别仍需端侧测试。

这些文件恢复到原教程的 `hy_ros/source/0.RK3588S/1.sherpa-onnx/`，然后在 RK3588 的 Python 3.10 环境按原安装步骤使用。脚本只下载，不在 Windows 上安装 ARM wheel。

## Qwen3-VL 与 RKLLM SDK

```bash
python3 hy_ros/tools/fetch_source.py --group qwen
```

两个模型共 3,225,511,369 字节，使用 [JiahaoLi 的公开模型镜像](https://huggingface.co/JiahaoLi/Qwen3-VL-RK3588/tree/025f0aa9b6ed2b22351acb2ebca30f4bd9ecdfe4)。这不是瑞芯微官方账号；本次已比对镜像公布的 SHA-256 与本地原件，两个文件均一致，并固定了镜像提交版本。官方获取入口也见 [RKLLM 1.2.3 的模型下载说明](https://github.com/airockchip/rknn-llm/blob/release-v1.2.3/README.md)，提取码 `rkllm`。官方网盘入口存在，但本次未在网盘内完成整包下载验证。

- `qwen3-vl-2b-instruct_w8a8_rk3588.rkllm`：`d5474340221fc495c70e1ec2c7dafc4ebf88292ce466db7e771e3a20b99cf21f`
- `qwen3-vl-2b_vision_rk3588.rknn`：`3d707ef5dbf0e420ac48e57b5bf0ed6c0fd1d5d048c29d81e1b5a8d051ab7ea8`

本地 `rknn-llm.zip` 的运行库标识为 **1.2.3**，不是原注释中的 1.2.1。归档内 848 个文件与官方 `release-v1.2.3` 一致；唯一修改是 `build-linux.sh` 的编译器路径。现在教程直接克隆固定版本，并从 `hy_ros/resources/rkllm/build-linux.sh` 复制这一个小脚本，不重新托管 168 MB SDK ZIP。不要随意换成上游最新版运行库。

## 相机、数据集与采集照片

- `2.Camera/master.zip`：其中 220 个文件与 [mjpg-streamer 固定提交](https://github.com/jacksonliam/mjpg-streamer/tree/310b29f4a94c46652b20c4b7b6e5cf24e532af39) 完全一致。开发教程已改为从该上游克隆后编译，无需原 ZIP。
- YOLO 标注数据集：原教程已有 [Roboflow 数据集](https://universe.roboflow.com/butter-robot/butter_robot/dataset/3)。下载版本 3 的 YOLOv8 导出包，解压到 `butter_yolov8n_train/Dataset/`，使其包含 `train/`、`valid/`、`test/`。本次自动访问返回 HTTP 403，未能重新导出验证；请在浏览器登录后确认版本 3 可下载。该数据集入口来自原教程，不代表已验证其匿名下载可用。现有本地导出注明 CC BY 4.0，保留其来源说明。数据集不重复上传。
- `source/butter_img/`：原始采集照片不是运行预训练模型所必需，训练可使用上述标注集，也可重新采集并标注。不要求下载作者全部照片。原文引用的 `tools/capture_tool.py` 不在原仓库，本次改为明确说明自行采集流程，不冒充原作者已有工具。
- 训练笔记本已移除原电脑绝对路径。从 `butter_yolov8n_train` 目录启动 Jupyter，使用该目录的 `dataset.yaml`。

## SolidWorks：唯一压缩发布的本地目录

```bash
# Ubuntu 解压工具；Windows 安装 7-Zip，使用 python 代替 python3
sudo apt install p7zip-full
python3 tools/download_resources.py --group Solidworks
```

125 个原始文件由本仓库的 [hardware-2026-10-13 Release](https://github.com/zhanghongchen1213/DIY_hyrobot/releases/tag/hardware-2026-10-13) 提供。7z / LZMA2 Ultra 固实压缩保留中文文件名和目录结构，解压到根目录 `Solidworks/`。归档及全部文件有 SHA-256 校验；`resources/resources-manifest.json` 只包含硬件包，没有 source 或数据集压缩包。

## 校验和按需下载

```bash
# 项目资源 + ASR + Qwen；不包括相机/SDK Git 仓库和 Roboflow 数据集
python3 hy_ros/tools/fetch_source.py --group all
python3 hy_ros/tools/fetch_source.py --group all --verify-only
python3 tools/download_resources.py --group Solidworks --verify-only
```

`fetch_source.py` 只还原部署所需文件，不保证还原整个作者 source 目录的每个 ZIP 和采集照片。相机、SDK 与数据集分别按上文和开发教程获取；文件路径、固定版本和校验值都已给出。

## NFS 路径衔接

原教程默认主机导出 `~/hy_linux/nfs`，端侧挂载 `/mnt/nfs`。建议在 Ubuntu 主机执行：

```bash
mkdir -p "$HOME/hy_linux/nfs"
cd "$HOME/hy_linux/nfs"
git clone https://github.com/zhanghongchen1213/DIY_hyrobot.git
cd DIY_hyrobot
python3 hy_ros/tools/fetch_source.py --group all
cd ..
# 此处需要尚未存在 hy_ros 路径；已有旧工作区请先保留它，并自行选择目录。
ln -s DIY_hyrobot/hy_ros hy_ros
sudo apt install nfs-kernel-server
# 将示例网段替换为机器人所在的可信局域网网段。
printf '%s 192.168.22.0/24(rw,sync,no_subtree_check)\n' "$HOME/hy_linux/nfs" | sudo tee /etc/exports.d/diy_hyrobot.exports
sudo exportfs -ra
sudo systemctl restart nfs-kernel-server
```

端侧按开发教程挂载主机的 `~/hy_linux/nfs` 到 `/mnt/nfs`。随后 `/mnt/nfs/hy_ros/source` 应可见，原有复制命令可以直接使用。NFS 默认 root squash；遇到权限问题先检查目录所有者与读权限。

## 其他复现条件与修正

- NFS：原文的 `nfs_config.sh` 不在仓库，已改为明确的系统命令。
- ROS：修正过时的 `uart_launch.py`、`ekf_odom_launch.py`、`camera_inference_streamer.launch.py`，使用现有三个视觉组件的独立启动文件。
- TTS：API Key / Secret Key / Voice ID 由使用者自己的百度账号提供，通过 `BAIDU_API_KEY`、`BAIDU_SECRET_KEY`、`BAIDU_VOICE_ID` 环境变量读取。仓库已有 base64 音色示例，也可用 `BAIDU_VOICE_SAMPLE` 指定自己的录音。作者历史中出现过的密钥应在百度后台轮换，不能视作公共服务。
- ESP32：从 `hy_driver` 目录构建；`main/idf_component.yml` 要求 ESP-IDF >=5.1.5 且 <5.4.0。执行 `idf.py set-target esp32s3`、`idf.py build`、`idf.py -p 串口 flash monitor`。依赖由组件管理器下载，构建缓存不提交。
- 系统镜像：`Orangepi5pro_1.0.6_ubuntu_jammy_server_linux5.10.160` 不在本地 source 中，仍需从香橙派官方渠道获取。IP、串口、驱动与硬件参数需按实际设备设置。

本次验证范围是文件来源比对、资源路径、压缩解压一致性、Git 同步和文档 HTML。未连接机器人，也未完成 RK3588 推理、ROS 导航、ESP32 烧录、GPU 训练或 SolidWorks 装配实机验证。第三方项目链接保留上游来源；本项目自己的 GitHub 入口统一为 DIY_hyrobot。
