# 复刻步骤与操作顺序

下面按部署顺序组织各功能章节。首次配置可先阅读[开始前的准备](project_status.md)和[资源准备](7_Resources.md)，已有部分环境时按需继续。每个章节都注明命令是在开发电脑还是机器人上执行。

```{toctree}
:maxdepth: 1

development/getting_started
development/host_setup
development/device_setup
development/hardware_testing
development/voice
development/visualization
development/vision
development/slam
```

## 扩展学习

- [模型训练与转换](development/model_training.md)：自行标注数据、训练模型并转换为 RKNN。
- [Nav2 导航与任务原型](development/navigation.md)：在已有地图上研究导航配置和任务接口集成。
- [参考资料](6_Reference_links.md)：查阅 ROS 2、模型工具链和可视化文档。

## 工作目录

`DIY_hyrobot/` 是下载后的项目目录，`hy_ros/` 是 ROS 工作区。按[资源准备中的 NFS 配置](7_Resources.md)建立共享后，开发电脑使用 `~/hy_linux/nfs/hy_ros`，机器人使用 `/mnt/nfs/hy_ros`。两侧路径指向同一份文件。

IP、串口、麦克风设备和模型路径需根据自己的设备设置。进入某个章节前，先确认命令是在开发电脑还是机器人上执行。
