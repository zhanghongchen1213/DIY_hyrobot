/**
 * @file gatts_server.h
 * @brief BLE GATT 服务器模块
 *
 * 本模块实现基于 ESP32 的 BLE GATT 服务器，提供双 Profile 架构：
 * - Profile A：主要数据通信服务
 * - Profile B：辅助控制服务
 *
 * 硬件配置：
 * - 蓝牙协议：BLE 4.2+
 * - 设备名称：HY_ESP32S3
 * - 支持的连接数：1
 *
 * GATT 服务配置：
 * - Profile A 服务 UUID：0x00FF
 * - Profile A 特征值 UUID：0xFF01
 * - Profile B 服务 UUID：0x00EE
 * - Profile B 特征值 UUID：0xEE01
 *
 * @author ZHC
 * @date 2025
 * @version 1.0
 *
 * @note 使用前需要先初始化 NVS 和蓝牙控制器
 */

#ifndef __GATTS_SERVER_H__
#define __GATTS_SERVER_H__

/* ==================================================================================
 *                                    Includes
 * ================================================================================== */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_bt.h"
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_bt_defs.h"
#include "esp_bt_main.h"
#include "esp_gatt_common_api.h"
#include "sdkconfig.h"

/* ==================================================================================
 *                                     Macros
 * ================================================================================== */

/* Profile A 的 GATT 服务相关定义 */
#define GATTS_SERVICE_UUID_TEST_A 0x00FF  ///< Profile A 的服务 UUID
#define GATTS_CHAR_UUID_TEST_A 0xFF01     ///< Profile A 的特征值 UUID
#define GATTS_DESCR_UUID_TEST_A 0x3333    ///< Profile A 的描述符 UUID
#define GATTS_NUM_HANDLE_TEST_A 4         ///< Profile A 的句柄数量

/* Profile B 的 GATT 服务相关定义 */
#define GATTS_SERVICE_UUID_TEST_B 0x00EE  ///< Profile B 的服务 UUID
#define GATTS_CHAR_UUID_TEST_B 0xEE01     ///< Profile B 的特征值 UUID
#define GATTS_DESCR_UUID_TEST_B 0x2222    ///< Profile B 的描述符 UUID
#define GATTS_NUM_HANDLE_TEST_B 4         ///< Profile B 的句柄数量

/* 设备名称和厂商数据长度定义 */
#define TEST_DEVICE_NAME "HY_ESP32S3"      ///< BLE 设备名称
#define TEST_MANUFACTURER_DATA_LEN 17      ///< 厂商数据长度

/* GATT 特征值最大长度 */
#define GATTS_DEMO_CHAR_VAL_LEN_MAX 0x190  ///< 特征值最大长度为 400 字节

/* 准备写入缓冲区最大大小 */
#define PREPARE_BUF_MAX_SIZE 1024  ///< 准备写入缓冲区最大 1024 字节

/* 广播和扫描响应配置标志位 */
#define adv_config_flag (1 << 0)       ///< 广播数据配置标志位
#define scan_rsp_config_flag (1 << 1)  ///< 扫描响应数据配置标志位

/* Profile 配置 */
#define PROFILE_NUM 2       ///< Profile 数量
#define PROFILE_A_APP_ID 0  ///< Profile A 的应用 ID
#define PROFILE_B_APP_ID 1  ///< Profile B 的应用 ID

/* 事件标志 */
#define DISCONNECTED_EVENT (1 << 0)  ///< 断开连接事件标志

/* 接收数据缓冲区大小 */
#define RECEIVED_DATA_MAX_LEN GATTS_DEMO_CHAR_VAL_LEN_MAX  ///< 接收数据最大长度

/* ==================================================================================
 *                                   Data Types
 * ================================================================================== */

/**
 * @brief 准备写入环境结构体
 *
 * 用于存储准备写入操作的数据缓冲区和长度
 */
typedef struct
{
    uint8_t *prepare_buf;  ///< 准备写入的数据缓冲区指针
    int prepare_len;       ///< 准备写入的数据长度
} prepare_type_env_t;

/**
 * @brief 接收数据结构体
 *
 * 用于存储从 GATT 特征值接收的数据
 */
typedef struct
{
    uint8_t data[RECEIVED_DATA_MAX_LEN];  ///< 接收的数据缓冲区
    size_t len;                           ///< 实际接收的数据长度
} received_data_t;

/**
 * @brief GATT Profile 实例结构体
 *
 * 每个 GATT-based profile 对应一个 app_id 和一个 gatts_if
 */
struct gatts_profile_inst
{
    esp_gatts_cb_t gatts_cb;       ///< GATT 服务器回调函数
    uint16_t gatts_if;             ///< GATT 接口
    uint16_t app_id;               ///< 应用 ID
    uint16_t conn_id;              ///< 连接 ID
    uint16_t service_handle;       ///< 服务句柄
    esp_gatt_srvc_id_t service_id; ///< 服务 ID
    uint16_t char_handle;          ///< 特征值句柄
    esp_bt_uuid_t char_uuid;       ///< 特征值 UUID
    esp_gatt_perm_t perm;          ///< 权限
    esp_gatt_char_prop_t property; ///< 属性
    uint16_t descr_handle;         ///< 描述符句柄
    esp_bt_uuid_t descr_uuid;      ///< 描述符 UUID
};

/* ==================================================================================
 *                              Global Variables
 * ================================================================================== */

extern struct gatts_profile_inst gl_profile_tab[PROFILE_NUM];  ///< Profile 实例数组
extern EventGroupHandle_t connected_event_group;               ///< 连接事件组句柄

/* ==================================================================================
 *                               Function Prototypes
 * ================================================================================== */

/**
 * @brief  初始化 BLE GATT 服务器
 *
 * 初始化蓝牙控制器和 GATT 服务器，包括：
 * - 初始化 NVS
 * - 初始化蓝牙控制器
 * - 注册 GAP 和 GATTS 回调
 * - 创建 GATT 服务和特征值
 * - 启动广播
 *
 * @return void
 *
 * @note   使用蓝牙功能前必须先调用此函数
 * @note   初始化后自动开始广播，设备名称为 HY_ESP32S3
 */
void blue_init(void);

#endif /* __GATTS_SERVER_H__ */
