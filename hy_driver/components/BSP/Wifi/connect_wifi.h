/**
 * @file connect_wifi.h
 * @brief Wi-Fi 连接管理模块
 *
 * 本模块负责处理 ESP32 的 Wi-Fi Station 模式初始化及连接逻辑。
 * 包含事件处理、重连机制和连接状态同步。
 *
 * 功能特性：
 * - Wi-Fi Station 模式初始化
 * - 自动重连机制
 * - 连接状态事件处理
 * - 阻塞式等待连接成功
 *
 * @author ZHC
 * @date 2025
 * @version 1.0
 *
 * @note 使用前需要在 menuconfig 中配置 Wi-Fi SSID 和密码
 * @note 支持非阻塞式初始化和阻塞式等待连接
 */

#ifndef __CONNECT_WIFI_H__
#define __CONNECT_WIFI_H__

#ifdef __cplusplus
extern "C"
{
#endif

/* ==================================================================================
 *                                    Includes
 * ================================================================================== */
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "lwip/err.h"
#include "lwip/sys.h"

/* ==================================================================================
 *                               Function Prototypes
 * ================================================================================== */

/**
 * @brief  初始化 Wi-Fi Station 模式并启动后台连接
 *
 * 本函数执行以下操作：
 * 1. 初始化 TCP/IP 适配器
 * 2. 创建默认事件循环
 * 3. 配置 Wi-Fi 为 Station 模式
 * 4. 注册事件处理程序以处理连接、断开和 IP 获取事件
 * 5. 启动 Wi-Fi
 *
 * @return void
 *
 * @note   此函数为非阻塞，Wi-Fi 连接过程在后台进行
 * @note   如需等待连接成功，请调用 wifi_wait_connected()
 * @note   Wi-Fi 配置（SSID/密码）需在 menuconfig 中预先设置
 */
void wifi_init_sta(void);

/**
 * @brief  等待 Wi-Fi 连接成功
 *
 * 阻塞当前任务，直到 Wi-Fi 连接成功或超时。
 *
 * @param  wait_ms  等待超时时间（毫秒），使用 portMAX_DELAY 表示无限等待
 *
 * @return true   连接成功
 * @return false  连接超时或失败
 *
 * @note   此函数会阻塞当前任务，建议在独立任务中调用
 * @note   超时后不会停止 Wi-Fi，后台重连机制仍然有效
 */
bool wifi_wait_connected(uint32_t wait_ms);

#ifdef __cplusplus
}
#endif

#endif /* __CONNECT_WIFI_H__ */
