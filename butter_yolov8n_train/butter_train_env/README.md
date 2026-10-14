# YOLO 训练环境

本目录由 uv 管理 Python 3.12 训练依赖。`pyproject.toml` 声明依赖，`uv.lock` 固定 PyTorch 2.5.1、CUDA 12.4 wheel 源及 airockchip Ultralytics 8.2.82 的提交版本。`main.py` 是初始化模板，训练入口为上级目录的 Notebook。

从仓库根目录开始：

```bash
cd butter_yolov8n_train
uv sync --project butter_train_env --frozen --python 3.12
uv run --project butter_train_env --frozen python -m ipykernel install --user --name butter_train --display-name "Python (butter_train)"
```

在 IDE 中打开 `butter_yolov8n_train.ipynb`，选择 `Python (butter_train)`。工作目录应保持为 `butter_yolov8n_train`；`--project` 选择环境，不会把 Notebook 相对路径改到本环境目录。

当前 Notebook 固定 CUDA 设备，需要 NVIDIA GPU；数据集、输出目录和转换步骤见[训练说明](../readme.md)。依赖安装成功不能代替训练或 RK3588S 推理验证。
