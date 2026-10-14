# DIY_hyrobot · 黄油机器人

黄油机器人的总仓库，包含 ROS 2 上位机、ESP32-S3 驱动、YOLO 训练和硬件资源入口。各目录是普通 Git 文件，不是 submodule，克隆一次即可获得项目代码和文档。

## 项目目录

| 目录 | 内容与入口 |
| --- | --- |
| `hy_ros/` | [ROS 工作区](hy_ros/readme.md)：串口、EKF、视觉、语音、SLAM 和导航原型 |
| `hy_driver/` | [ESP32-S3 固件](hy_driver/readme.md)：电机、编码器、IMU、UART 与音频 |
| `butter_yolov8n_train/` | [训练与导出](butter_yolov8n_train/readme.md)：单类别黄油模型、uv 环境和历史结果 |
| `hardware/` | [机械资源](hardware/README.md)：SolidWorks Release、校验和总装入口 |
| `hy_ros/docs/` | [文档构建](hy_ros/docs/README.md)、[开发教程](hy_ros/docs/source/3_Development_instructions.md) |
| `tools/`、`resources/` | 硬件下载、独立组件同步工具及资源清单 |

项目图片、硬件规格和演示见[项目介绍](hy_ros/docs/source/1_product_introduction.md)。复刻从[开始前的准备](hy_ros/docs/source/project_status.md)进入：Nav2 抓取任务仍是实验原型，本地 RKLLM 当前为文本推理，手掌检测与自主跟随未实现。

## 首次使用

```bash
git clone https://github.com/zhanghongchen1213/DIY_hyrobot.git
cd DIY_hyrobot
# 还原本项目测试脚本、测试图与 YOLO 模型，均来自普通 Git 文件。
python3 hy_ros/tools/fetch_source.py --group project
# 按需下载 ASR 或 Qwen 模型，按清单进行 SHA-256 校验。
python3 hy_ros/tools/fetch_source.py --group asr qwen
# SolidWorks 使用本仓库 Release 压缩包，需要 Python 3.9+ 和 7-Zip。
python3 tools/download_resources.py --group Solidworks
```

`hy_ros/source/`、训练 `Dataset/` 和根目录 `Solidworks/` 被 Git 忽略。相机源码、RKLLM SDK 与数据集分别从上游获取，固定版本和命令见[资源来源说明](hy_ros/docs/source/7_Resources.md)。ASR/Qwen 下载并不包含完整训练数据集。

## 运行环境与入口

- ROS：Ubuntu 22.04 / ROS 2 Humble，板端视觉与本地模型依赖 RK3588S 驱动、RKNN/RKLLM、RGA 和多媒体库。按教程准备依赖后，在 `hy_ros/` 执行 `colcon build`；各包启动方式见模块 README。
- 固件：ESP-IDF `>=5.1.5,<5.4.0`，当前锁定 5.1.5。在 `hy_driver/` 执行 `idf.py set-target esp32s3`、`idf.py build`，再按实际串口刷写。
- 训练：Python 3.12 和 NVIDIA CUDA GPU。从 `butter_yolov8n_train/` 使用已提交的 uv 锁文件安装依赖，再在 IDE 执行 Notebook；数据集需另行获取。

文档预览可在开发电脑运行；ROS、固件和模型运行需要上面对应的平台与设备。

## 文档网页

Linux 或 macOS 在仓库根目录执行：

```bash
python3 -m venv hy_ros/docs/.venv
. hy_ros/docs/.venv/bin/activate
python -m pip install -r hy_ros/docs/requirements.txt
python -m sphinx -b html -n -W --keep-going hy_ros/docs/source hy_ros/docs/build/html
python -m http.server 8767 --bind 127.0.0.1 --directory hy_ros/docs/build/html
```

Windows 可执行：

```powershell
powershell -ExecutionPolicy Bypass -File .\hy_ros\docs\preview.ps1 -Port 8767
```

浏览器打开 `http://127.0.0.1:8767/`。章节按主机、端侧、硬件测试、语音、可视化、训练、视觉部署、SLAM 与 Nav2 分页，旧开发教程地址保留为总入口。

总仓库 Read the Docs 配置位于根目录 `.readthedocs.yaml`。旧站 `hy-ros.readthedocs.io` 的仓库关联和最新构建需在后台核对；本地构建通过不等于线上发布成功。

## 维护与许可

日常直接在总仓库开发、提交和审查变更，详见[维护指南](hy_ros/docs/maintainer/repository.md)。只有拥有独立组件仓库时才使用 `tools/sync_component.py`。

仓库根目录没有统一许可证。ROS 各包、第三方固件依赖、模型与数据保留各自许可和来源，不为整个仓库重新授予统一授权。
