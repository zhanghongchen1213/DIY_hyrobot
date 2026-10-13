/**
 * @file bsp_drivers.h
 * @brief BSP 驱动模块统一头文件
 *
 * 本文件汇总所有 BSP（Board Support Package）驱动模块的头文件，
 * 提供统一的包含接口，简化上层应用的头文件管理。
 *
 * 包含的驱动模块：
 * - servo_control: 三舵机控制系统（右臂、左臂、腰部）
 * - imu: QMI8658 六轴惯性测量单元（加速度计 + 陀螺仪）
 * - studio: ES8311 音频编解码器和 MP3 播放系统
 * - motor: TB6612FNG 双电机驱动和编码器系统
 * - connect_uart: 双 UART 通信系统（大脑模块 + 语音识别）
 * - connect_wifi: WiFi 连接管理系统
 *
 * @author ZHC
 * @date 2025
 * @version 1.0
 *
 * @note 使用时只需包含此文件即可访问所有 BSP 驱动功能
 */

#ifndef __BSP_DRIVERS_H__
#define __BSP_DRIVERS_H__

#include "servo_control.h"  ///< 舵机控制驱动
#include "imu.h"            ///< IMU 传感器驱动
#include "studio.h"         ///< 音频系统驱动
#include "motor.h"          ///< 电机和编码器驱动
#include "connect_uart.h"   ///< UART 通信驱动
#include "connect_wifi.h"   ///< WiFi 连接驱动

#endif /* __BSP_DRIVERS_H__ */