# DIY_hyrobot · 黄油机器人

黄油机器人的唯一维护仓库，包含 ROS2、ESP32-S3 驱动、YOLO 训练和硬件资源入口。

- [ROS2 功能包与详细教程](hy_ros/readme.md)：`hy_ros/`
- [ESP32-S3 驱动器](hy_driver/readme.md)：`hy_driver/`
- [YOLO 训练与模型导出](butter_yolov8n_train/readme.md)：`butter_yolov8n_train/`
- [SolidWorks 硬件与压缩包](hardware/README.md)：`hardware/`
- [资源下载与复现说明](hy_ros/docs/source/7_Resources.md)
- [本地独立开发、分支同步与备份](hy_ros/docs/source/8_Maintenance.md)

## 首次使用

```bash
git clone https://github.com/zhanghongchen1213/DIY_hyrobot.git
cd DIY_hyrobot
# 还原本项目少量测试脚本、测试图与 YOLO 模型（普通 Git 文件，无需下载压缩包）
python3 hy_ros/tools/fetch_source.py --group project
# 按需从公开上游下载，含 SHA-256 校验：
python3 hy_ros/tools/fetch_source.py --group asr qwen
# 只有 SolidWorks 采用本仓库 Release 压缩包；需安装 7-Zip。
python3 tools/download_resources.py --group Solidworks
```

`source/` 继续被 Git 忽略。相机源码、RKLLM SDK 和 Roboflow 数据集分别从上游获取，具体版本与命令见[资源来源说明](hy_ros/docs/source/7_Resources.md)。不重复打包第三方模型与数据集。普通源码、文档和图片均直接保存在仓库中。

## 文档网页

Windows 在仓库根目录执行：

```powershell
powershell -ExecutionPolicy Bypass -File .\hy_ros\docs\preview.ps1 -Port 8767
```

浏览器打开 `http://127.0.0.1:8767/`。Linux 可执行：

```bash
python3 -m venv .venv
. .venv/bin/activate
pip install -r hy_ros/docs/requirements.txt
python -m sphinx -b html hy_ros/docs/source hy_ros/docs/build/html
python -m http.server 8767 --bind 127.0.0.1 --directory hy_ros/docs/build/html
```

Read the Docs 构建配置位于仓库根目录 `.readthedocs.yaml`。旧站 `hy-ros.readthedocs.io` 的后台关联仓库需要改为本仓库；仓库代码无法代替该账号后台设置。在完成改绑前，以本仓库的文档和本地构建为准。

本仓库采用 subtree 的 squash 导入；三个原始本地仓库完整保留原分支与历史。日常在原目录 `git commit` 后执行 `git publish`，即可将该组件的当前分支合并并推送到总仓库。不要从组件目录直接向总仓库 `main` 推送根目录树。详见维护指南。

第三方依赖、模型和数据保留其原有许可证与来源；没有为整个仓库重新授予统一许可证。
