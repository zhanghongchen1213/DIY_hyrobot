①编译方法：本目录是 ROS 2 ament/colcon 功能包。
在 ROS 工作区加载 ROS 2 环境后，单独编译功能包：
colcon build --packages-select ldlidar_driver_ros2

②设备别名：端口设备重命名
launch中启动的LD06/STL-19P/LD19雷达默认设备名为：/dev/ldlidar_serial，本仓库规则脚本是“scripts/Ldlidar_udev.sh”，
如果您使用的ttl电平转换芯片为CP2102，设备号需要改为0001；

具体修改方法请分别查看对应的驱动资料。

③运行方法
启动：按实际雷达与串口核对 launch 配置；源码说明面向 LD06，其他型号需单独验证。
source install/setup.bash
ros2 launch ldlidar_driver_ros2 ldlidar_driver.launch.py

④rviz可视化查看点云：
开启rviz2，本仓库 launch 的 frame_id 为 radar_Link，RViz 基坐标选择 radar_Link 或已连通的固定坐标系，选择“laserscan”的可视化类型，话题选择“scan”。

⑤雷达角度分割
启用雷达角度分割：{'enable_angle_crop_func': True}
禁用雷达角度分割：{'enable_angle_crop_func': False}

## 环境说明

本项目使用 Ubuntu 22.04 / ROS 2 Humble；本次仅核对源码与启动配置，没有连接雷达进行运行验证。上游驱动来源和许可保留在本目录 LICENSE。
