/**
 * @file all_include.h
 * @brief 全局头文件汇总
 *
 * 本文件汇总所有常用的系统头文件、RTOS 头文件和应用程序模块头文件，
 * 提供统一的包含接口，简化各源文件的头文件管理。
 *
 * @author ZHC
 * @date 2025
 * @version 1.0
 *
 * @note 使用时只需包含此文件即可访问所有常用功能
 * @warning 包含过多头文件可能增加编译时间，按需包含更佳
 */

#ifndef __ALL_INCLUDE_H__
#define __ALL_INCLUDE_H__

/* ==================================================================================
 *                                    Includes
 * ================================================================================== */

/* 项目配置头文件 */
#include "all_debug.h"     ///< 调试配置和版本定义
#include "bsp_drivers.h"   ///< 硬件驱动库

/* C 标准库 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <inttypes.h>
#include <math.h>
#include <dirent.h>
#include "errno.h"

/* ESP-IDF 系统库 */
#include "sdkconfig.h"
#include "sys/time.h"
#include "esp_system.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_psram.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_event.h"
#include "esp_timer.h"
#include "esp_sleep.h"
#include "esp_random.h"
#include "esp_srp.h"

/* 存储系统 */
#include "nvs.h"
#include "nvs_flash.h"
#include "esp_spiffs.h"
#include "esp_vfs_fat.h"

/* FreeRTOS */
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"

/* 应用程序模块 */
#include "app_audio.h"         ///< 音频控制模块
#include "app_blue_control.h"  ///< 蓝牙控制模块
#include "gatts_server.h"      ///< GATT 服务器模块
#include "app_pid_control.h"   ///< PID 控制模块
#include "app_move_control.h"  ///< 运动控制模块
#include "app_uart_control.h"  ///< UART 通信控制模块

#endif /* __ALL_INCLUDE_H__ */
