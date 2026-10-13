/**
 * @file app_blue_control.h
 * @brief 蓝牙控制应用模块
 *
 * 本模块提供蓝牙双向控制接口，包括：
 * - BLE GATT 服务器通信
 * - 运动控制命令接收
 * - 实时数据上传
 *
 * 通信协议：
 * - 下行：接收线速度、角速度控制命令
 * - 上行：发送机器人状态数据
 * - 发送频率：30ms（调试模式）
 *
 * @author ZHC
 * @date 2025
 * @version 1.0
 *
 * @warning 需要确保控制命令格式与客户端一致
 */

#ifndef __APP_BLUE_CONTROL_H__
#define __APP_BLUE_CONTROL_H__

/* ==================================================================================
 *                                    Includes
 * ================================================================================== */
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "driver/uart.h"
#include "gatts_server.h"

/* ==================================================================================
 *                                     Macros
 * ================================================================================== */

#define BLE_SEND_INTERVAL_DEBUG_MS 30  ///< 蓝牙数据发送间隔（调试模式：30ms，用于实时监控）

/* ==================================================================================
 *                                   Data Types
 * ================================================================================== */

/**
 * @brief 运动控制命令结构体
 *
 * 用于存储从蓝牙客户端接收的运动控制指令
 */
typedef struct
{
    float linear_vel;   ///< 线速度 (m/s)
    float angular_vel;  ///< 角速度 (rad/s)
    uint32_t timestamp; ///< 命令时间戳 (ms)
} ControlCommand;

/**
 * @brief 蓝牙接收数据结构体
 *
 * 用于存储从 BLE GATT 特征值接收的原始字节数据
 */
typedef struct
{
    uint8_t data[36];  ///< 接收的字节数据缓冲区（最大36字节）
    uint16_t len;      ///< 实际接收的数据长度
} wxx_received_data_t;

/* ==================================================================================
 *                               Function Prototypes
 * ================================================================================== */

/**
 * @brief  初始化蓝牙控制模块
 *
 * 初始化蓝牙控制相关的 RTOS 任务和资源，包括：
 * - 创建蓝牙通信任务
 * - 初始化数据接收队列
 * - 配置数据发送定时器
 *
 * @return void
 *
 * @note   必须在 BLE GATT 服务器初始化后调用
 * @note   启动后会自动创建蓝牙数据收发任务
 */
void rtos_blue_control_init(void);

#endif /* __APP_BLUE_CONTROL_H__ */
