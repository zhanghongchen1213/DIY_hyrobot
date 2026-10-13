/**
 * @file connect_uart.c
 * @brief 双 UART 通信模块实现文件
 *
 * 本文件实现 ESP32-S3 平台上的双 UART 通信系统，提供统一的初始化接口。
 *
 * @author ZHC
 * @date 2025
 * @version 1.0
 */

#include "connect_uart.h"
#include "esp_err.h"
#include "driver/uart.h"
#include "driver/gpio.h"

/**
 * @brief UART 事件队列句柄数组
 *
 * 存储所有 UART 端口的事件队列句柄，用于接收 UART 硬件事件通知。
 * 索引对应 UART 端口号（UART_NUM_0、UART_NUM_1、UART_NUM_2）。
 */
static QueueHandle_t s_uart_event_queue[UART_NUM_MAX] = {0};

/* ========== 内部函数声明 ========== */
static esp_err_t uart_apply_config_ex(uart_port_t port, int baud, gpio_num_t tx, gpio_num_t rx, int rts, int cts, int rx_buf, int tx_buf, int queue_len, int rx_tout);
static esp_err_t uart_comm_init_brain(void);
static esp_err_t uart_comm_init_ci03t(void);

/* ========== 内部函数实现 ========== */

/**
 * @brief  通用 UART 配置和初始化函数
 *
 * 本函数封装了 UART 初始化的完整流程，包括参数配置、引脚设置、驱动安装和超时配置。
 *
 * 缓冲区配置说明：
 * =========================================================================
 * ESP32-S3 UART 驱动使用 DMA 缓冲区机制：
 *
 * 1. RX 缓冲区（rx_buf）：
 *    - 用于存储接收到的数据
 *    - 大小应根据数据速率和处理频率设计
 *    - 计算公式：rx_buf ≥ 波特率(bps) / 10 × 处理间隔(s)
 *    - 示例：460800 bps，100ms 处理间隔 → 至少 4608 字节
 *
 * 2. TX 缓冲区（tx_buf）：
 *    - 用于缓存待发送的数据
 *    - 设为 0 时禁用发送缓冲（阻塞发送模式）
 *    - 非零时启用异步发送（提高发送效率）
 *
 * 3. 事件队列（queue_len）：
 *    - 存储 UART 硬件事件（数据到达、错误等）
 *    - 队列长度应大于预期的突发事件数
 *    - 设为 0 时禁用事件队列
 *
 * 4. 接收超时（rx_tout）：
 *    - 单位：符号时间（1 符号 = 1 起始位 + 8 数据位 + 1 停止位 = 10 位）
 *    - 作用：当接收缓冲区有数据但未满时，超时后触发 UART_DATA 事件
 *    - 示例：rx_tout=10 表示接收 10 个字符的时间后触发事件
 * =========================================================================
 *
 * @param  port       UART 端口号
 * @param  baud       波特率（bps）
 * @param  tx         TX 引脚号
 * @param  rx         RX 引脚号
 * @param  rts        RTS 引脚号（流控，不使用时设为 UART_PIN_NO_CHANGE）
 * @param  cts        CTS 引脚号（流控，不使用时设为 UART_PIN_NO_CHANGE）
 * @param  rx_buf     RX 缓冲区大小（字节）
 * @param  tx_buf     TX 缓冲区大小（字节，0 表示禁用）
 * @param  queue_len  事件队列长度（0 表示禁用）
 * @param  rx_tout    接收超时（符号时间）
 *
 * @return
 *     - ESP_OK: 配置成功
 *     - ESP_FAIL: 配置失败
 */
static esp_err_t uart_apply_config_ex(uart_port_t port, int baud, gpio_num_t tx, gpio_num_t rx, int rts, int cts, int rx_buf, int tx_buf, int queue_len, int rx_tout)
{
    uart_config_t cfg = {
        .baud_rate = baud,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    esp_err_t ret = uart_param_config(port, &cfg);
    if (ret != ESP_OK)
        return ret;
    ret = uart_set_pin(port, tx, rx, rts, cts);
    if (ret != ESP_OK)
        return ret;
    QueueHandle_t *queue_param = NULL;
    if (queue_len > 0)
    {
        queue_param = &s_uart_event_queue[port];
    }
    ret = uart_driver_install(port, rx_buf, tx_buf, queue_len, queue_param, 0);
    if (ret != ESP_OK)
        return ret;
    uart_set_rx_timeout(port, rx_tout);
    return ESP_OK;
}

/**
 * @brief  初始化 UART1（大脑模块通信）
 *
 * 配置参数说明：
 * - 波特率：460800 bps（高速通信）
 * - 缓冲区：RX=8KB, TX=8KB（支持高速双向数据流）
 * - 事件队列：32 个事件（应对高频数据传输）
 * - 接收超时：2 个符号时间（约 43μs，快速响应）
 *
 * @return
 *     - ESP_OK: 初始化成功
 *     - ESP_FAIL: 初始化失败
 */
static esp_err_t uart_comm_init_brain(void)
{
    return uart_apply_config_ex(BRAIN_UART_NUM, BRAIN_UART_BAUD_RATE, BRAIN_UART_TXD_PIN, BRAIN_UART_RXD_PIN, BRAIN_UART_RTS_PIN, BRAIN_UART_CTS_PIN, BRAIN_UART_RX_BUF_SIZE, BRAIN_UART_TX_BUF_SIZE, 32, 2);
}

/**
 * @brief  初始化 UART2（CI_03T 语音识别模块）
 *
 * 配置参数说明：
 * - 波特率：115200 bps（标准串口速率）
 * - 缓冲区：RX=256B, TX=0B（仅接收模式，节省内存）
 * - 事件队列：4 个事件（低频命令接收）
 * - 接收超时：10 个符号时间（约 870μs，适合命令接收）
 *
 * @return
 *     - ESP_OK: 初始化成功
 *     - ESP_FAIL: 初始化失败
 */
static esp_err_t uart_comm_init_ci03t(void)
{
    return uart_apply_config_ex(CI03T_UART_NUM, CI03T_UART_BAUD_RATE, CI03T_UART_TXD_PIN, CI03T_UART_RXD_PIN, CI03T_UART_RTS_PIN, CI03T_UART_CTS_PIN, CI03T_UART_RX_BUF_SIZE, CI03T_UART_TX_BUF_SIZE, 4, 10);
}

/* ========== 公共函数实现 ========== */

esp_err_t uart_comm_init_all(void)
{
    // 初始化 UART1（大脑模块）
    esp_err_t r1 = uart_comm_init_brain();

    // 初始化 UART2（语音识别模块）
    esp_err_t r2 = uart_comm_init_ci03t();

    // 返回初始化结果：两路都成功才返回 ESP_OK，否则返回第一个失败的错误码
    return (r1 == ESP_OK && r2 == ESP_OK) ? ESP_OK : (r1 != ESP_OK ? r1 : r2);
}

QueueHandle_t uart_get_event_queue(uart_port_t port)
{
    // 返回指定 UART 端口的事件队列句柄
    return s_uart_event_queue[port];
}
