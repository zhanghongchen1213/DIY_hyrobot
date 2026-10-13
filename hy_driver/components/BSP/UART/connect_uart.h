/**
 * @file connect_uart.h
 * @brief 双 UART 通信系统驱动
 *
 * 本模块实现 ESP32-S3 平台上的双 UART 通信系统，支持两路独立的串口通信：
 * - UART1：高速通信（460800 波特率），用于连接外部大脑/AI 模块
 * - UART2：标准通信（115200 波特率），用于连接 CI_03T 语音识别模块
 *
 * 硬件配置：
 * - UART1（大脑模块）：
 *   - 波特率：460800 bps（高速数据传输）
 *   - GPIO：TX=17, RX=18
 *   - 缓冲区：RX=8KB, TX=8KB（大缓冲区支持高速数据流）
 *   - 用途：与外部 AI 处理单元进行双向数据交换
 *
 * - UART2（语音识别模块）：
 *   - 波特率：115200 bps（标准串口速率）
 *   - GPIO：TX=42, RX=41
 *   - 缓冲区：RX=256B, TX=0B（仅接收，不发送）
 *   - 用途：接收 CI_03T 语音识别模块的命令数据
 *
 * 缓冲区设计说明：
 * =========================================================================
 * UART1 使用 8KB 大缓冲区的原因：
 * - 高速数据传输：460800 bps ≈ 57.6 KB/s 理论速率
 * - 防止数据丢失：提供足够的缓冲空间应对突发数据
 * - 双向通信：支持与外部 AI 模块的复杂数据交互
 * - 实时性要求：大缓冲区减少轮询频率，降低 CPU 负载
 *
 * UART2 使用 256B 小缓冲区的原因：
 * - 数据量小：语音识别命令通常 < 100 字节
 * - 单向接收：仅接收模式，TX 缓冲区设为 0（禁用发送）
 * - 节省内存：语音命令频率低，无需大缓冲区
 * - 简化设计：CI_03T 模块仅输出识别结果，不需要复杂交互
 * =========================================================================
 *
 * @author ZHC
 * @date 2025
 * @version 1.0
 *
 * @note 使用前必须先调用 uart_comm_init_all() 初始化两路 UART
 * @note UART 事件通过 FreeRTOS 队列机制处理，队列大小为 20
 * @warning 禁止在中断上下文中调用 UART 读写函数
 */

#ifndef __CONNECT_UART_H__
#define __CONNECT_UART_H__

#ifdef __cplusplus
extern "C"
{
#endif

/* ==================================================================================
 *                                    Includes
 * ================================================================================== */
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "esp_err.h"
#include "driver/uart.h"
#include "driver/gpio.h"

/* ==================================================================================
 *                                     Macros
 * ================================================================================== */

/* UART1（大脑模块）配置参数 */
#define BRAIN_UART_NUM UART_NUM_1             ///< 使用 UART1 端口
#define BRAIN_UART_BAUD_RATE 460800           ///< 波特率 460800 bps（高速数据传输）
#define BRAIN_UART_TXD_PIN GPIO_NUM_17        ///< TXD 引脚 GPIO17
#define BRAIN_UART_RXD_PIN GPIO_NUM_18        ///< RXD 引脚 GPIO18
#define BRAIN_UART_RTS_PIN UART_PIN_NO_CHANGE ///< 不使用 RTS 硬件流控
#define BRAIN_UART_CTS_PIN UART_PIN_NO_CHANGE ///< 不使用 CTS 硬件流控
#define BRAIN_UART_RX_BUF_SIZE (8192)         ///< 接收缓冲区大小 8KB
#define BRAIN_UART_TX_BUF_SIZE (8192)         ///< 发送缓冲区大小 8KB

/* UART2（语音识别模块）配置参数 */
#define CI03T_UART_NUM UART_NUM_2             ///< 使用 UART2 端口
#define CI03T_UART_BAUD_RATE 115200           ///< 波特率 115200 bps（标准串口速率）
#define CI03T_UART_TXD_PIN GPIO_NUM_42        ///< TXD 引脚 GPIO42
#define CI03T_UART_RXD_PIN GPIO_NUM_41        ///< RXD 引脚 GPIO41
#define CI03T_UART_RTS_PIN UART_PIN_NO_CHANGE ///< 不使用 RTS 硬件流控
#define CI03T_UART_CTS_PIN UART_PIN_NO_CHANGE ///< 不使用 CTS 硬件流控
#define CI03T_UART_RX_BUF_SIZE (256)          ///< 接收缓冲区大小 256 字节
#define CI03T_UART_TX_BUF_SIZE (0)            ///< 发送缓冲区大小 0 字节（禁用 TX 缓冲）

/* ==================================================================================
 *                               Function Prototypes
 * ================================================================================== */

/**
 * @brief  初始化所有 UART 通信接口
 *
 * 本函数初始化 UART1 和 UART2 两路串口通信，配置参数如下：
 *
 * UART1 配置（大脑模块）：
 * - 波特率：460800 bps
 * - 数据位：8 位
 * - 停止位：1 位
 * - 校验位：无
 * - 流控：无
 * - 缓冲区：RX=8KB, TX=8KB
 * - 事件队列：20 个事件
 *
 * UART2 配置（语音识别模块）：
 * - 波特率：115200 bps
 * - 数据位：8 位
 * - 停止位：1 位
 * - 校验位：无
 * - 流控：无
 * - 缓冲区：RX=256B, TX=0B（禁用发送）
 * - 事件队列：20 个事件
 *
 * @return ESP_OK   初始化成功
 * @return ESP_FAIL 初始化失败
 *
 * @note   必须在使用 UART 通信前调用此函数
 * @note   初始化失败时会输出错误日志
 */
esp_err_t uart_comm_init_all(void);

/**
 * @brief  获取指定 UART 端口的事件队列句柄
 *
 * 本函数返回指定 UART 端口的 FreeRTOS 事件队列句柄，用于接收 UART 事件通知。
 *
 * 支持的 UART 事件类型：
 * - UART_DATA: 接收到数据
 * - UART_FIFO_OVF: FIFO 溢出
 * - UART_BUFFER_FULL: 缓冲区满
 * - UART_BREAK: 检测到 BREAK 信号
 * - UART_PARITY_ERR: 校验错误
 * - UART_FRAME_ERR: 帧错误
 *
 * @param  port  UART 端口号（UART_NUM_1 或 UART_NUM_2）
 *
 * @return QueueHandle_t 事件队列句柄，失败时返回 NULL
 *
 * @note   必须在 uart_comm_init_all() 之后调用
 * @note   队列大小为 20，可容纳 20 个待处理事件
 */
QueueHandle_t uart_get_event_queue(uart_port_t port);

#ifdef __cplusplus
}
#endif

#endif /* __CONNECT_UART_H__ */
