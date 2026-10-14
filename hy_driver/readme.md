# 黄油机器人 ESP32-S3 驱动固件

本目录负责底盘电机、编码器、IMU、串口、音频与条件编译外设，采用 ESP-IDF。ROS 2、视觉推理、语音模型和导航运行在 RK3588S 上，见[ROS 工作区](../hy_ros/readme.md)。项目外观、演示和硬件入口见[项目介绍](../hy_ros/docs/source/1_product_introduction.md)。

## 环境与构建

`main/idf_component.yml` 要求 ESP-IDF `>=5.1.5,<5.4.0`，`dependencies.lock` 当前锁定 5.1.5 和 ESP32-S3。先按 ESP-IDF 的安装方式配置工具链并激活其终端环境，再从仓库根目录执行：

```bash
cd hy_driver
idf.py set-target esp32s3
idf.py build
# 将 /dev/ttyUSB0 替换为实际串口；Windows 例如 COM3。
idf.py -p /dev/ttyUSB0 flash monitor
```

`set-target` 会重新生成工程配置，已有自定义配置应先保留。首次构建由组件管理器获取清单依赖。`main/CMakeLists.txt` 还会从 `spiffs/` 生成音频文件系统镜像；刷写时使用本工程完整产物。

## 模块与接口

| 模块 | 代码位置 | 当前职责 |
| --- | --- | --- |
| 电机与编码器 | `components/BSP/Motor/`、`main/app_move_control/` | TB6612 双电机、PCNT 编码器、左右轮 PID 与差速运动学 |
| IMU | `components/BSP/IMU/` | QMI8658 读取；航向积分与串口上报，ROS 层另行进行 EKF 融合 |
| 上位机通信 | `components/BSP/UART/`、`main/app_uart_control/` | UART1 460800，TX17 / RX18；packed 二进制结构 |
| 语音模块串口 | `components/BSP/UART/` | UART2 115200，TX42 / RX41；连接 CI-03T |
| 舵机 | `components/BSP/Servo/` | GPIO8 / 19 / 20，50 Hz PWM、0～60°逻辑角度；默认未初始化 |
| 音频 | `components/BSP/STUDIO/`、`main/app_audio_control/`、`spiffs/` | ES8311、I2S、提示音与网络音频流播放 |
| 网络与 BLE | `components/BSP/Wifi/`、`main/app_blue_control/` | Wi-Fi STA；BLE 为可选编译功能，默认关闭 |

启动入口为 `main/main.c`，依次准备 NVS、Wi-Fi、IMU、音频、外设与 FreeRTOS 应用任务。慢速和正常舵机档逐步移动，快速档直接到目标，不能把三档都描述为平滑插值。

上行协议含轮速、PID、里程计、陀螺仪、四元数、关节和时间戳等字段，未包含加速度字段。两端字段与长度受 `UPDATE_ODOMETRY_DEBUG` 影响，需与 ROS 端解析保持一致。UART 详细结构以 `main/app_uart_control/app_uart_control.h` 为准。

## 当前默认开关

配置在 `main/all_debug/all_debug.h`：

| 宏 | 当前值 | 影响 |
| --- | --- | --- |
| `UPDATE_ODOMETRY_DEBUG` | `1` | 启用运动学、里程计及对应协议字段 |
| `SERVO_DEBUG` | `0` | 舵机初始化关闭 |
| `BLUE_CONTROL` | `0` | BLE 初始化与任务关闭 |
| `PID_DEBUG` | `0` | 在线 PID 参数调试分支关闭；基础闭环 PID 仍运行 |
| `NVS_CLEAR_VERSION` | `1` | 每次启动清 NVS，不保留该配置下的跨重启数据 |

## 使用前修改

- Wi-Fi 配置目前位于 `components/BSP/Wifi/connect_wifi.c`；按自己的网络设置，不公开凭证。
- 网络音频服务器地址位于 `main/app_audio_control/app_audio.c`，应与 RK3588S TTS HTTP 服务的 IP、端口和路径一致。
- 电机引脚、轮径和轮距分别在 BSP 与运动学头文件中；实机更换部件后需校准。
- 需要舵机或 BLE 时先检查接线与对应开关，再构建；现有默认配置不能用于验证抓取动作。

## 验证与已知问题

上位机的 `/cmd_vel`、`/joint_commands` 尚未桥接到当前 UART，详见[导航接口说明](../hy_ros/docs/source/development/navigation.md)。

`app_uart_control.c` 的 `PID_DEBUG` 分支把左轮目标与参数同时写到左右轮；默认关闭该分支。开启在线调参前应先修正并在设备上验证，不能把此分支当成已验收功能。

资源来源见[复现说明](../hy_ros/docs/source/7_Resources.md)，日常代码维护见[维护指南](../hy_ros/docs/maintainer/repository.md)。本仓库没有统一许可证，第三方组件保留各自许可。
