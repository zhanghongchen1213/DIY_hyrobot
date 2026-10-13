# 单仓库维护与本地分支同步

唯一维护仓库：[DIY_hyrobot](https://github.com/zhanghongchen1213/DIY_hyrobot)。三个代码组件是普通目录，不是 Git submodule；克隆一次即可获得全部代码。

## 本地布局和历史

本次迁移保留以下独立开发目录以及各自 `.git`：

```text
H:/Project/DIY/06_hy_robot/hy_ros                    main
H:/Project/DIY/06_hy_robot/hy_driver                 master、base_drive
H:/Project/DIY/06_hy_robot/butter_yolov8n_train       main
H:/Project/DIY/06_hy_robot/Solidworks                硬件原件
H:/Project/DIY/DIY_hyrobot                          总仓库 main
```

本地完整历史通过 `git bundle --all` 备份在 `hy_ros/_bmad/monorepo-migration-2026-10-13/`；同目录保存迁移前 refs、Git 配置和资源清单。总仓库使用 `git subtree --squash` 导入组件快照，因此旧代码历史（包括曾存在的硬编码密钥和失效二进制资源）不会成为新云端仓库的祖先提交。本地分支和旧提交仍然存在，可以正常查看、切换与开发。

## 推荐日常流程

在原组件目录完成修改并提交，例如：

```powershell
cd H:\Project\DIY\06_hy_robot\hy_ros
git add <本次修改的文件>
git commit -m "feat: improve robot behavior"
git publish
```

本次在这三个本地仓库配置了 `git publish` 别名。它从当前分支读取已经提交的状态，先更新总仓库 main，再通过 subtree 合并到同名子目录，最后正常推送 main。未提交文件不会自动打包，也不会强制覆盖远端。如果任何工作区有未提交内容、总仓库不在 main、远端分叉或发生内容冲突，命令会停止并保留现场。

别名不是 Git 自带命令，也不会跟随克隆传递。其他电脑或指定分支时，在总仓库执行：

```powershell
python tools/sync_component.py --component hy_ros --source H:/Project/DIY/06_hy_robot/hy_ros --branch main --push
python tools/sync_component.py --component hy_driver --source H:/Project/DIY/06_hy_robot/hy_driver --branch master --push
# 选择 base_drive 表示合并这个分支的状态，需先确认这正是计划发布的版本：
python tools/sync_component.py --component hy_driver --source H:/Project/DIY/06_hy_robot/hy_driver --branch base_drive --push
```

省略 `--branch` 使用组件当前分支；省略 `--push` 只做本地合并，检查后可在总仓库 `git push origin main`。脚本使用正常合并处理双方修改，不会简单覆盖整个目录；冲突需要在总仓库解决并提交，或执行 `git merge --abort`。解决后再次执行同步命令即可推送。

三个组件的 `origin` 指向新总仓库，但刻意取消旧的 upstream 并设置 `push.default=nothing`。组件根目录树与总仓库根目录树不同，直接 `git push origin main` 不是目录合并操作；应使用 `git publish`。不要直接 `git pull origin main` 到组件目录，那会混入总仓库目录层级。

## 将总仓库里的组件修改取回独立工作区

正常开发优先从独立组件仓库向总仓库发布。如果直接在总仓库修改了组件，可在总仓库导出组件分支，再在独立仓库合并（不用强推）：

```powershell
cd H:/Project/DIY/DIY_hyrobot
git subtree split --prefix=hy_ros -b export-hy-ros
cd H:/Project/DIY/06_hy_robot/hy_ros
git fetch H:/Project/DIY/DIY_hyrobot export-hy-ros
git merge FETCH_HEAD
```

若导出分支已经存在，先确认没有未保存工作，再使用新的分支名。变更冲突按正常 Git 合并处理。Squash 导入记录了组件的原始 commit，拆分时仍需要对应的本地历史；本次总仓库中保留了这些本地 Git 对象，云端的新克隆可先从原组件仓库 fetch 补齐。不要把旧历史分支直接推到新云端仓库。

## 资源更新

第三方模型与数据集继续从上游获取，更新时核对版本和 SHA-256，修改 `hy_ros/resources/source-manifest.json` 及教程。项目自己的小脚本/测试图/YOLO 模型位于 `hy_ros/resources/project/`，直接提交 Git，`fetch_source.py` 会还原到被忽略的 source 目录。

只有 SolidWorks 更新时创建新的版本化硬件 Release：重新无损压缩、生成逐文件/归档 SHA-256、完整解压验证，更新 `resources/resources-manifest.json` 与文档。保留旧 Release，使已有 Git 提交仍能获取对应硬件资源。

旧的三个 GitHub 仓库保留为只读归档，以保留旧链接与历史记录；所有后续开发与发布以 DIY_hyrobot 为准。没有删除旧仓库。

## 文档站迁移

根目录 `.readthedocs.yaml` 已使用 `hy_ros/docs/source/conf.py` 与 `hy_ros/docs/requirements.txt`。需要在 Read the Docs 项目后台把仓库关联改成 `https://github.com/zhanghongchen1213/DIY_hyrobot` 并触发 main 构建；只有 GitHub 的权限不能自动完成这个账号设置。本地 HTML 可直接用 `hy_ros/docs/preview.ps1` 构建和启动。
