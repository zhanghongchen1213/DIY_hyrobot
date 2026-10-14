# 文档维护与本地预览

`source/index.rst` 是 Sphinx 总入口，`source/development/` 按部署任务组织教程，`source/_static/recovered/` 保存现有图片。原 `3_Development_instructions.md` 页面保留为教程入口。

## Linux 与 macOS

在仓库根目录执行：

```bash
python3 -m venv hy_ros/docs/.venv
. hy_ros/docs/.venv/bin/activate
python -m pip install -r hy_ros/docs/requirements.txt
python -m sphinx -b html -n -W --keep-going hy_ros/docs/source hy_ros/docs/build/html
python -m http.server 8767 --bind 127.0.0.1 --directory hy_ros/docs/build/html
```

浏览器打开 `http://127.0.0.1:8767/`。`-W` 将构建警告视为失败，修改章节、图片和链接后应运行此检查。

如需保存后自动重建，在已激活的环境中执行：

```bash
python -m pip install sphinx-autobuild
bash hy_ros/docs/auto_build.sh --host 127.0.0.1 --port 8767
```

`sphinx-autobuild` 是可选的本地预览工具；普通构建与 Read the Docs 不需要安装它。

## Windows

在仓库根目录执行：

```powershell
powershell -ExecutionPolicy Bypass -File .\hy_ros\docs\preview.ps1 -Port 8767
```

脚本创建文档专用虚拟环境、安装依赖、构建 HTML 并启动本地服务器。只构建时添加 `-BuildOnly`。严格检查可使用其环境执行：

```powershell
.\hy_ros\docs\.venv\Scripts\python.exe -m sphinx -b html -n -W --keep-going .\hy_ros\docs\source .\hy_ros\docs\build\html
```

## 章节和发布

- 每页保留一个一级标题，按任务划分二级、三级标题；不手写重复章节编号。
- 新页面加入教程入口或 `index.rst` 的 toctree；删除空页时同步移除引用。
- 子目录教程引用图片使用 `../_static/`，引用资源页使用 `../7_Resources.md`。
- 不把历史截图、静态核对或 HTML 构建写成设备验收；实现限制集中记录在[当前实现与复现边界](maintainer/project_analysis.md)。

总仓库 Read the Docs 配置为根目录 `.readthedocs.yaml`，其构建路径指向本目录。组件内部的 `.readthedocs.yaml` 属于旧独立仓库上下文。线上仓库关联与构建状态需在 Read the Docs 后台另行核对。

## 教程内容范围

`source/` 发布项目介绍、硬件资料、资源准备、操作步骤与参考资料，不限定读者来源或已有设备。源码分析和仓库同步说明保存在 `maintainer/`，不参与 Sphinx 构建；不要将迁移记录、审计过程和后台管理步骤放入教程。
