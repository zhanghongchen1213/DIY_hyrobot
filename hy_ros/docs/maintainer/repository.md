# 仓库维护与组件同步

维护仓库为 [DIY_hyrobot](https://github.com/zhanghongchen1213/DIY_hyrobot)。`hy_ros`、`hy_driver` 和 `butter_yolov8n_train` 是普通目录，不是 submodule。Git 历史包含 subtree squash 导入记录；日常可以直接在总仓库开发。

## 总仓库日常开发

从总仓库根目录开始：

```bash
git status --short
git switch -c codex/your-change
# 编辑本次涉及的文件，按模块完成相应检查。
git diff --check
git diff
git add 本次修改的文件
git commit -m "docs: update robot documentation"
```

是否推送、合并和发布按具体工作安排执行。普通克隆不需要旧电脑的组件路径、`git publish` 别名或私有备份。

## 独立组件仓库的可选同步

只有已经拥有独立组件 Git 仓库时，才使用 `tools/sync_component.py`。源路径必须是独立仓库根目录，不能把总仓库内的 `hy_ros/` 等普通目录当成源仓库。

脚本要求源仓库和总仓库工作区干净，且总仓库处于 `main`；使用当前或指定组件分支的已提交状态，执行 squash subtree 合并。先检查并提交两边修改，再运行：

```bash
git switch main
python3 tools/sync_component.py --component hy_ros --source /path/to/independent/hy_ros --branch main
python3 tools/sync_component.py --component hy_driver --source /path/to/independent/hy_driver --branch master
python3 tools/sync_component.py --component butter_yolov8n_train --source /path/to/independent/butter_yolov8n_train --branch main
```

组件名仅支持以上三项，路径和分支替换为实际值。省略 `--branch` 时使用源仓库当前分支，省略 `--push` 时只做本地合并。需要同步远端时可添加 `--push`：脚本先 fetch 并要求总仓库能 fast-forward，再合并组件，最后正常推送 `main`。

发生冲突时检查总仓库现场，按普通 Git 合并解决并提交，或执行 `git merge --abort`。不要将独立组件根目录直接推到总仓库 `main`，也不要在组件仓库直接拉取总仓库根目录树。

## 取回总仓库中的组件修改

确实需要回到独立组件仓库时，可在总仓库导出组件分支，再由独立仓库获取并合并：

```bash
# 总仓库内；分支已存在时改用新的名称。
git subtree split --prefix=hy_ros -b export-hy-ros
# 切换到独立组件仓库后执行。
git fetch /path/to/DIY_hyrobot export-hy-ros
git merge FETCH_HEAD
```

squash 导入可能需要原组件历史对象；新克隆不保证具有旧机器的全部对象。缺失时从对应独立仓库获取，先核对来源和 refs。不要为补齐本地历史把旧历史分支直接发布到总仓库远端。

## 资源更新

- 第三方模型与数据集保留上游来源，更新固定版本和 SHA-256 时同步修改 `hy_ros/resources/source-manifest.json`。
- 项目自有测试脚本、测试图和 YOLO 模型位于 `hy_ros/resources/project/`，`fetch_source.py` 将其还原到被忽略的 `hy_ros/source/`。
- SolidWorks 更新时先生成版本化归档与逐文件校验，完整解压验证后再更新 `resources/resources-manifest.json`。上传新 Release 和确认下载是独立步骤，保留已有版本资源。

## 历史迁移说明

旧文档记录了 2026-10-13 在 Windows `H:/Project/DIY/06_hy_robot/` 下保留三个独立组件仓库、创建 bundle 备份、配置 `git publish` 别名及调整组件远端的操作。这些是原机器的迁移记录，相关路径、Git 配置和备份没有随本仓库克隆交付。

当前克隆可确认总仓库 remote 与 subtree 导入记录，不能据此确认原电脑的备份仍存在、旧分支对象完整或旧 GitHub 仓库已归档。需要恢复旧历史或使用别名时，先在原机器核对配置与备份；默认使用本页的总仓库流程。
