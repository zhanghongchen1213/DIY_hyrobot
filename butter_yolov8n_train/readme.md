# 黄油目标检测模型训练与导出

本目录提供单类别 `butter` 的 YOLOv8n 训练 Notebook、uv 环境、基础权重与历史训练结果。最终 RK3588S 部署使用 `.rknn`，需要在 ONNX 导出后完成 RKNN-Toolkit2 转换；ROS 视觉代码位于[hy_ros](../hy_ros/readme.md)。

## 目录与运行环境

| 路径 | 用途 |
| --- | --- |
| `butter_yolov8n_train.ipynb` | GPU 检查、模型训练和 RKNN 兼容 ONNX 导出入口 |
| `butter_train_env/` | `pyproject.toml`、`uv.lock` 与内核环境说明 |
| `dataset.yaml` | 指向 `Dataset/train/images`、`valid/images`、`test/images`，类别为 butter |
| `yolov8n.pt` | Notebook 默认基础权重；另有 yolo11n.pt，但该 Notebook 未使用它 |
| `butter_train_results/yolov8n_butter/` | 已提交历史配置、指标、图表和 `weights/` 产物 |

当前环境使用 Python 3.12、PyTorch 2.5.1、torchvision 0.20.1、torchaudio 2.5.1，Windows/Linux 配置 CUDA 12.4 wheel 源。Ultralytics 8.2.82 来自 airockchip 分支，锁定提交 `4674fe6e003dfbc5f2250d3b39dd31faaf7a9877`。Notebook 使用 `cuda:0`，需要 NVIDIA CUDA GPU；macOS 不能直接照此设备参数训练。

## 数据准备

从 [Roboflow butter_robot v3](https://universe.roboflow.com/butter-robot/butter_robot/dataset/3) 获取 YOLOv8 导出数据，解压为：

```text
butter_yolov8n_train/
└── Dataset/
    ├── train/images/ 与 train/labels/
    ├── valid/images/ 与 valid/labels/
    └── test/images/ 与 test/labels/
```

`Dataset/` 被 Git 忽略，克隆仓库后需自行准备。原文记录 2,689 / 267 / 116 张训练、验证、测试图及 CC BY 4.0；图像数量和使用许可请以实际下载的导出包为准。

## 安装与训练

从仓库根目录开始，保持 Notebook 工作目录在 `butter_yolov8n_train`：

```bash
cd butter_yolov8n_train
uv sync --project butter_train_env --frozen --python 3.12
uv run --project butter_train_env --frozen python -m ipykernel install --user --name butter_train --display-name "Python (butter_train)"
```

在 VS Code、PyCharm 等 IDE 打开 `butter_yolov8n_train.ipynb`，选择 `Python (butter_train)` 内核，依次执行单元。环境中的 `main.py` 是 Hello 模板，不是训练程序。

Notebook 当前参数为 120 epochs、batch 64、640×640、device 0，结果写入 `butter_train_results/yolov8n_butter/`。历史 `args.yaml` 的优化器为 `auto`，不能直接写成固定 SGD。显存不足时可减小 batch；`exist_ok=True` 会复用同名输出目录，重训前应保留已有结果。

```bash
uv run --project butter_train_env --frozen tensorboard --logdir butter_train_results/yolov8n_butter --port 6006
```

## 导出与部署

Notebook 的 `model.export(format="rknn")` 在固定分支中生成权重目录下的 `best.onnx`（opset 12），不直接生成 `.rknn`。随后在 RKNN-Toolkit2 环境执行模型转换。

历史 `weights/best.pt` 与 `last.pt` 各 6,256,099 字节，`best.onnx` 为 12,065,989 字节；本目录没有已转换的 `.rknn`。可直接部署的项目模型由 `hy_ros/tools/fetch_source.py --group project` 还原，清单见 [source-manifest.json](../hy_ros/resources/source-manifest.json)。它不会自动被本次训练产物替换。

## 历史结果与检查范围

已提交的 120 轮 `results.csv` 最后一轮记录：Precision 0.89248、Recall 0.87582、mAP50 0.92558、mAP50-95 0.75695、train/dfl_loss 0.83165。重新训练的结果取决于数据、环境和参数。

视觉节点当前只发布标注图像。结构化检测、跟随和导航接口的限制见[ROS 工作区说明](../hy_ros/readme.md)。
