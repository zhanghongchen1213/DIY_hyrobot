---
name: "embedded-document-generation"
description: "专为嵌入式项目（STM32/RTOS/Linux驱动）设计。自动生成包含硬件架构、固件分层、任务调度及外设通信协议的开发文档。"
---

# 嵌入式开发文档专家 (Embedded Document Generator)

本技能专注于嵌入式系统的技术文档生成。它通过分析 `.ioc`, `Makefile`, `main.c`, `FreeRTOSConfig.h` 等特征文件，自动提取硬件配置、任务逻辑和底层驱动信息，生成符合嵌入式工程标准的文档。

## 目标

1.  **硬件感知 (Hardware Awareness)**：自动识别 MCU 型号、时钟配置、GPIO 分配及外设（UART/SPI/I2C/CAN）设置。
2.  **系统分析 (System Analysis)**：
    - **裸机**：分析中断服务程序 (ISR) 和主循环 (`while(1)`) 逻辑。
    - **RTOS**：分析任务 (`Task`)、队列 (`Queue`)、信号量 (`Semaphore`) 及优先级配置。
3.  **协议解析 (Protocol Parsing)**：提取自定义通信协议的帧结构和寄存器定义。
4.  **规范输出**：输出**全中文** Markdown 文档。

## 文档模板

```markdown
# [项目名称] 嵌入式开发文档

> 硬件平台：[如：STM32F407ZGT6]
> 操作系统：[如：FreeRTOS v10.3.1 / Bare Metal]
> 编译环境：[如：Keil MDK v5.3 / GCC Arm None Eabi]

## 1. 系统概述 (Overview)

### 1.1 项目背景
[简述设备功能、应用场景]

### 1.2 系统框图
- **输入**：[传感器列表]
- **处理**：[核心算法/逻辑]
- **输出**：[执行器/显示屏]

## 2. 硬件架构 (Hardware Architecture)

### 2.1 MCU 资源分配
| 外设 | 引脚 | 功能描述 | 配置参数 |
|-----|-----|---------|---------|
| USART1 | PA9/PA10 | 调试串口 | 115200, 8N1 |
| SPI2 | PB13/14/15 | 屏幕驱动 | 42MHz, Master |

### 2.2 关键电路说明
- **电源管理**：[描述 3.3V/5V 供电逻辑]
- **外部接口**：[描述物理接口定义]

## 3. 固件架构 (Firmware Architecture)

### 3.1 层次结构
- **驱动层 (BSP)**：[列出底层驱动文件，如 `bsp_led.c`, `bsp_can.c`]
- **中间件 (Middleware)**：[列出使用的库，如 FatFS, LwIP, USB_Device]
- **应用层 (App)**：[列出核心业务逻辑]

### 3.2 任务调度 (RTOS Tasks)
> *（仅 RTOS 项目适用）*

| 任务名称 | 优先级 | 堆栈大小 | 运行频率 | 功能描述 |
|---------|-------|---------|---------|---------|
| `StartTask` | High | 128 | Once | 系统初始化 |
| `SensorTask`| Normal| 512 | 100Hz | 数据采集与滤波 |
| `CommTask` | Low | 1024 | Event | 通信协议处理 |

### 3.3 中断管理 (Interrupts)
- **SysTick**：系统心跳
- **EXTI0**：[描述按键或外部触发功能]
- **DMA1_Stream3**：串口数据接收

## 4. 核心模块详解 (Core Modules)

### 4.1 [模块 A，如：电机控制]
- **文件位置**：`src/motor_ctrl.c`
- **控制算法**：[如：增量式 PID]
- **数据流**：编码器 -> 定时器捕获 -> PID 计算 -> PWM 输出

### 4.2 [模块 B，如：数据存储]
- **存储介质**：[如：W25Q64 Flash]
- **存储结构**：
    - 0x0000 - 0x1000: 系统参数
    - 0x1000 - 0xFFFF: 日志数据

## 5. 通信协议 (Communication Protocol)

### 5.1 物理层
- **接口**：RS485
- **波特率**：9600 bps

### 5.2 数据链路层
- **帧格式**：`[帧头 0xAA] [长度] [指令] [数据...] [CRC16]`

### 5.3 指令集
| 指令码 | 功能 | 数据定义 |
|-------|-----|---------|
| 0x01 | 读取状态 | 无 |
| 0x02 | 设置参数 | [参数ID 1B] [数值 4B] |

## 6. 编译与烧录 (Build & Flash)

### 6.1 编译步骤
```bash
make -j4
# 或者 Keil 点击 Build
```

### 6.2 烧录方法
- **工具**：ST-Link V2
- **连接**：SWDIO, SWCLK, GND, 3V3
- **操作**：使用 STM32CubeProgrammer 下载 `.hex` 文件

## 7. 版本记录 (Changelog)
- **v1.0.0**: 初始版本发布
```
