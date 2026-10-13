#include "gatts_server.h"
//*===全局变量===*//
static const char *TAG = "GATT";
QueueHandle_t received_data_queue;
bool notificationsEnabled = false; // 通知使能标志，true为使能，false为关闭
bool connected = false;            // 连接标志，true为连接，false为断开
extern QueueHandle_t audio_queue;  // 音频队列句柄

/// 声明静态函数
/**
 * @brief Profile A 的GATT事件处理函数
 * @param event GATT事件类型
 * @param gatts_if GATT接口
 * @param param 事件参数指针
 */
void gatts_profile_a_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);

/**
 * @brief Profile B 的GATT事件处理函数
 * @param event GATT事件类型
 * @param gatts_if GATT接口
 * @param param 事件参数指针
 */
void gatts_profile_b_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);

// 特征值初始数据
uint8_t char1_str[] = {0x11, 0x22, 0x33}; // 特征值初始数据

// Profile A 和 B 的特征属性
esp_gatt_char_prop_t a_property = 0; // Profile A 的特征属性
esp_gatt_char_prop_t b_property = 0; // Profile B 的特征属性

/**
 * @brief GATT特征值结构体
 * @note 用于存储特征值的最大长度、当前长度和值指针
 */
#define CHARACTERISTIC_MAX_LEN 400 // 假设数据包总大小是300字节
esp_attr_value_t gatts_demo_char1_val = {
    .attr_max_len = CHARACTERISTIC_MAX_LEN,
    .attr_len = sizeof(char1_str), // 当前特征值长度
    .attr_value = char1_str,       // 特征值数据指针
};

/**
 * @brief 广播配置完成标志
 * @note 用于标记广播配置是否完成
 */
uint8_t adv_config_done = 0;

#ifdef CONFIG_SET_RAW_ADV_DATA
uint8_t raw_adv_data[] = {
    0x02, 0x01, 0x06,       // Length 2, Data Type 1 (Flags), Data 1 (LE General Discoverable Mode, BR/EDR Not Supported)
    0x02, 0x0a, 0xeb,       // Length 2, Data Type 10 (TX power leve), Data 2 (-21)
    0x03, 0x03, 0xab, 0xcd, // Length 3, Data Type 3 (Complete 16-bit Service UUIDs), Data 3 (UUID)
};
uint8_t raw_scan_rsp_data[] = { // Length 15, Data Type 9 (Complete Local Name), Data 1 (ESP_GATTS_DEMO)
    0x0f, 0x09, 0x45, 0x53, 0x50, 0x5f, 0x47, 0x41, 0x54, 0x54, 0x53, 0x5f, 0x44,
    0x45, 0x4d, 0x4f};
#else

/**
 * @brief 广播服务UUID数组
 * @note 包含两个UUID：
 *       1. 16位UUID，值位于[12][13]位置
 *       2. 32位UUID，值位于[12][13][14][15]位置
 */
uint8_t adv_service_uuid128[32] = {
    /* LSB <--------------------------------------------------------------------------------> MSB */
    // 第一个UUID，16位
    0xfb,
    0x34,
    0x9b,
    0x5f,
    0x80,
    0x00,
    0x00,
    0x80,
    0x00,
    0x10,
    0x00,
    0x00,
    0xEE,
    0x00,
    0x00,
    0x00,
    // 第二个UUID，32位
    0xfb,
    0x34,
    0x9b,
    0x5f,
    0x80,
    0x00,
    0x00,
    0x80,
    0x00,
    0x10,
    0x00,
    0x00,
    0xFF,
    0x00,
    0x00,
    0x00,
};

/**
 * @brief 广播数据结构体
 * @note 广播数据长度必须小于31字节
 */
esp_ble_adv_data_t adv_data = {
    .set_scan_rsp = false,                                                // 是否为扫描响应数据
    .include_name = true,                                                 // 是否包含设备名称
    .include_txpower = false,                                             // 是否包含发射功率
    .min_interval = 0x0006,                                               // 最小连接间隔，单位1.25ms
    .max_interval = 0x0010,                                               // 最大连接间隔，单位1.25ms
    .appearance = 0x00,                                                   // 设备外观
    .manufacturer_len = 0,                                                // 制造商数据长度
    .p_manufacturer_data = NULL,                                          // 制造商数据指针
    .service_data_len = 0,                                                // 服务数据长度
    .p_service_data = NULL,                                               // 服务数据指针
    .service_uuid_len = sizeof(adv_service_uuid128),                      // 服务UUID长度
    .p_service_uuid = adv_service_uuid128,                                // 服务UUID指针
    .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT), // 广播标志
};

/**
 * @brief 扫描响应数据结构体
 */
esp_ble_adv_data_t scan_rsp_data = {
    .set_scan_rsp = true,                                                 // 设置为扫描响应数据
    .include_name = true,                                                 // 包含设备名称
    .include_txpower = true,                                              // 包含发射功率
    .appearance = 0x00,                                                   // 设备外观
    .manufacturer_len = 0,                                                // 制造商数据长度
    .p_manufacturer_data = NULL,                                          // 制造商数据指针
    .service_data_len = 0,                                                // 服务数据长度
    .p_service_data = NULL,                                               // 服务数据指针
    .service_uuid_len = sizeof(adv_service_uuid128),                      // 服务UUID长度
    .p_service_uuid = adv_service_uuid128,                                // 服务UUID指针
    .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT), // 广播标志
};

#endif /* CONFIG_SET_RAW_ADV_DATA */

/**
 * @brief 广播参数结构体
 */
esp_ble_adv_params_t adv_params = {
    .adv_int_min = 0x20,                                    // 最小广播间隔，单位0.625ms
    .adv_int_max = 0x40,                                    // 最大广播间隔，单位0.625ms
    .adv_type = ADV_TYPE_IND,                               // 广播类型：可连接的非定向广播
    .own_addr_type = BLE_ADDR_TYPE_PUBLIC,                  // 地址类型：公共地址
    .channel_map = ADV_CHNL_ALL,                            // 使用所有广播信道
    .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY, // 过滤策略
};

/**
 * @brief GATT Profile实例数组
 * @note 存储由ESP_GATTS_REG_EVT返回的gatts_if
 */
struct gatts_profile_inst gl_profile_tab[PROFILE_NUM] = {
    [PROFILE_A_APP_ID] = {
        .gatts_cb = gatts_profile_a_event_handler, // Profile A的事件处理函数
        .gatts_if = ESP_GATT_IF_NONE,              // 初始化为ESP_GATT_IF_NONE
    },
    [PROFILE_B_APP_ID] = {
        .gatts_cb = gatts_profile_b_event_handler, // Profile B的事件处理函数
        .gatts_if = ESP_GATT_IF_NONE,              // 初始化为ESP_GATT_IF_NONE
    },
};

// 初始化全局变量，防止未初始化指针导致的内存错误
prepare_type_env_t a_prepare_write_env = {
    .prepare_buf = NULL,
    .prepare_len = 0}; // Profile A 的准备写入环境

prepare_type_env_t b_prepare_write_env = {
    .prepare_buf = NULL,
    .prepare_len = 0}; // Profile B 的准备写入环境

/**
 * @brief 初始化准备写入环境
 * @param prepare_write_env 准备写入环境指针
 */
static void init_prepare_write_env(prepare_type_env_t *prepare_write_env);

/**
 * @brief 清理准备写入环境
 * @param prepare_write_env 准备写入环境指针
 */
static void cleanup_prepare_write_env(prepare_type_env_t *prepare_write_env);

/**
 * @brief 处理写入事件环境
 * @param gatts_if GATT接口
 * @param prepare_write_env 准备写入环境指针
 * @param param GATT事件参数指针
 */
void example_write_event_env(esp_gatt_if_t gatts_if, prepare_type_env_t *prepare_write_env, esp_ble_gatts_cb_param_t *param);

/**
 * @brief 处理执行写入事件环境
 * @param prepare_write_env 准备写入环境指针
 * @param param GATT事件参数指针
 */
void example_exec_write_event_env(prepare_type_env_t *prepare_write_env, esp_ble_gatts_cb_param_t *param);

/**
 * @brief 初始化准备写入环境
 * @param prepare_write_env 准备写入环境指针
 */
static void init_prepare_write_env(prepare_type_env_t *prepare_write_env)
{
    if (prepare_write_env == NULL)
    {
        ESP_LOGE(TAG, "Cannot initialize NULL prepare_write_env");
        return;
    }

    // 确保内存状态清零
    prepare_write_env->prepare_buf = NULL;
    prepare_write_env->prepare_len = 0;

    ESP_LOGD(TAG, "Initialized prepare_write_env: %p", prepare_write_env);
}

/**
 * @brief 清理准备写入环境
 * @param prepare_write_env 准备写入环境指针
 */
static void cleanup_prepare_write_env(prepare_type_env_t *prepare_write_env)
{
    if (prepare_write_env == NULL)
    {
        ESP_LOGE(TAG, "Cannot cleanup NULL prepare_write_env");
        return;
    }

    // 安全释放内存
    if (prepare_write_env->prepare_buf != NULL)
    {
        ESP_LOGD(TAG, "Cleaning up prepare buffer: %p, size: %d",
                 prepare_write_env->prepare_buf, prepare_write_env->prepare_len);
        free(prepare_write_env->prepare_buf);
        prepare_write_env->prepare_buf = NULL;
    }

    prepare_write_env->prepare_len = 0;
    ESP_LOGD(TAG, "Cleaned up prepare_write_env: %p", prepare_write_env);
}

/**
 * @brief GAP事件处理函数
 * @param event GAP事件类型
 * @param param GAP事件参数指针
 */
void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    switch (event)
    {
#ifdef CONFIG_SET_RAW_ADV_DATA
    // 原始广播数据设置完成事件
    case ESP_GAP_BLE_ADV_DATA_RAW_SET_COMPLETE_EVT:
        adv_config_done &= (~adv_config_flag); // 清除广播配置标志
        if (adv_config_done == 0)              // 如果所有配置都完成
        {
            esp_ble_gap_start_advertising(&adv_params); // 开始广播
        }
        break;
    // 原始扫描响应数据设置完成事件
    case ESP_GAP_BLE_SCAN_RSP_DATA_RAW_SET_COMPLETE_EVT:
        adv_config_done &= (~scan_rsp_config_flag); // 清除扫描响应配置标志
        if (adv_config_done == 0)                   // 如果所有配置都完成
        {
            esp_ble_gap_start_advertising(&adv_params); // 开始广播
        }
        break;
#else
    // 广播数据设置完成事件
    case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:
        adv_config_done &= (~adv_config_flag); // 清除广播配置标志
        if (adv_config_done == 0)              // 如果所有配置都完成
        {
            esp_ble_gap_start_advertising(&adv_params); // 开始广播
        }
        break;
    // 扫描响应数据设置完成事件
    case ESP_GAP_BLE_SCAN_RSP_DATA_SET_COMPLETE_EVT:
        adv_config_done &= (~scan_rsp_config_flag); // 清除扫描响应配置标志
        if (adv_config_done == 0)                   // 如果所有配置都完成
        {
            esp_ble_gap_start_advertising(&adv_params); // 开始广播
        }
        break;
#endif
    // 广播启动完成事件
    case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
        if (param->adv_start_cmpl.status != ESP_BT_STATUS_SUCCESS) // 如果广播启动失败
        {
            ESP_LOGE(TAG, "Advertising start failed\n"); // 记录错误日志
        }
        break;
    // 广播停止完成事件
    case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT:
        if (param->adv_stop_cmpl.status != ESP_BT_STATUS_SUCCESS) // 如果广播停止失败
        {
            ESP_LOGE(TAG, "Advertising stop failed\n"); // 记录错误日志
        }
        else
        {
            ESP_LOGI(TAG, "Stop adv successfully\n"); // 记录成功日志
        }
        break;
    // 连接参数更新事件
    case ESP_GAP_BLE_UPDATE_CONN_PARAMS_EVT:
        ESP_LOGI(TAG, "update connection params status = %d, min_int = %d, max_int = %d,conn_int = %d,latency = %d, timeout = %d",
                 param->update_conn_params.status,   // 更新状态
                 param->update_conn_params.min_int,  // 最小间隔
                 param->update_conn_params.max_int,  // 最大间隔
                 param->update_conn_params.conn_int, // 连接间隔
                 param->update_conn_params.latency,  // 从机延迟
                 param->update_conn_params.timeout); // 超时时间
        break;
    default:
        break;
    }
}

void example_write_event_env(esp_gatt_if_t gatts_if, prepare_type_env_t *prepare_write_env, esp_ble_gatts_cb_param_t *param)
{
    esp_gatt_status_t status = ESP_GATT_OK;

    // 参数安全检查
    if (prepare_write_env == NULL || param == NULL)
    {
        ESP_LOGE(TAG, "Invalid parameters in write event");
        return;
    }

    if (param->write.need_rsp)
    {
        if (param->write.is_prep)
        {
            // 检查写入数据长度是否合理
            if (param->write.len == 0 || param->write.len > GATTS_DEMO_CHAR_VAL_LEN_MAX)
            {
                ESP_LOGE(TAG, "Invalid write length: %d", param->write.len);
                status = ESP_GATT_INVALID_ATTR_LEN;
            }
            else if (prepare_write_env->prepare_buf == NULL)
            {
                // 首次分配内存
                prepare_write_env->prepare_buf = (uint8_t *)malloc(PREPARE_BUF_MAX_SIZE * sizeof(uint8_t));
                prepare_write_env->prepare_len = 0;
                if (prepare_write_env->prepare_buf == NULL)
                {
                    ESP_LOGE(TAG, "Gatt_server prep no mem, size: %d", PREPARE_BUF_MAX_SIZE);
                    status = ESP_GATT_NO_RESOURCES;
                }
                else
                {
                    // 初始化分配的内存
                    memset(prepare_write_env->prepare_buf, 0, PREPARE_BUF_MAX_SIZE);
                    ESP_LOGI(TAG, "Allocated prepare buffer: %d bytes", PREPARE_BUF_MAX_SIZE);
                }
            }
            else
            {
                // 检查偏移和长度是否合法
                if (param->write.offset > PREPARE_BUF_MAX_SIZE)
                {
                    ESP_LOGE(TAG, "Invalid offset: %d, max: %d", param->write.offset, PREPARE_BUF_MAX_SIZE);
                    status = ESP_GATT_INVALID_OFFSET;
                }
                else if ((param->write.offset + param->write.len) > PREPARE_BUF_MAX_SIZE)
                {
                    ESP_LOGE(TAG, "Write would exceed buffer: offset=%d, len=%d, max=%d",
                             param->write.offset, param->write.len, PREPARE_BUF_MAX_SIZE);
                    status = ESP_GATT_INVALID_ATTR_LEN;
                }
            }

            // 分配响应结构体
            esp_gatt_rsp_t *gatt_rsp = (esp_gatt_rsp_t *)malloc(sizeof(esp_gatt_rsp_t));
            if (gatt_rsp == NULL)
            {
                ESP_LOGE(TAG, "Failed to allocate GATT response");
                status = ESP_GATT_NO_RESOURCES;
            }
            else
            {
                // 初始化响应结构体
                memset(gatt_rsp, 0, sizeof(esp_gatt_rsp_t));
                gatt_rsp->attr_value.len = param->write.len;
                gatt_rsp->attr_value.handle = param->write.handle;
                gatt_rsp->attr_value.offset = param->write.offset;
                gatt_rsp->attr_value.auth_req = ESP_GATT_AUTH_REQ_NONE;

                // 安全复制数据
                if (param->write.len <= sizeof(gatt_rsp->attr_value.value))
                {
                    memcpy(gatt_rsp->attr_value.value, param->write.value, param->write.len);
                }
                else
                {
                    ESP_LOGE(TAG, "Write data too large for response: %d", param->write.len);
                    status = ESP_GATT_INVALID_ATTR_LEN;
                }

                esp_err_t response_err = esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, status, gatt_rsp);
                if (response_err != ESP_OK)
                {
                    ESP_LOGE(TAG, "Send response error: 0x%x", response_err);
                }

                // 安全释放响应结构体
                free(gatt_rsp);
                gatt_rsp = NULL;
            }

            // 只有在状态正常且缓冲区有效时才复制数据
            if (status == ESP_GATT_OK && prepare_write_env->prepare_buf != NULL)
            {
                memcpy(prepare_write_env->prepare_buf + param->write.offset,
                       param->write.value,
                       param->write.len);
                prepare_write_env->prepare_len += param->write.len;
                ESP_LOGD(TAG, "Copied %d bytes to prepare buffer, total: %d",
                         param->write.len, prepare_write_env->prepare_len);
            }
        }
        else
        {
            esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, status, NULL);
        }
    }
}

void example_exec_write_event_env(prepare_type_env_t *prepare_write_env, esp_ble_gatts_cb_param_t *param)
{
    // 参数安全检查
    if (prepare_write_env == NULL || param == NULL)
    {
        ESP_LOGE(TAG, "Invalid parameters in exec write event");
        return;
    }

    if (param->exec_write.exec_write_flag == ESP_GATT_PREP_WRITE_EXEC)
    {
        // 只有在缓冲区有效且长度合理时才打印
        if (prepare_write_env->prepare_buf != NULL &&
            prepare_write_env->prepare_len > 0 &&
            prepare_write_env->prepare_len <= PREPARE_BUF_MAX_SIZE)
        {
            ESP_LOGI(TAG, "Executing prepared write, length: %d", prepare_write_env->prepare_len);
            esp_log_buffer_hex(TAG, prepare_write_env->prepare_buf, prepare_write_env->prepare_len);
        }
        else
        {
            ESP_LOGW(TAG, "Invalid prepare buffer state: buf=%p, len=%d",
                     prepare_write_env->prepare_buf, prepare_write_env->prepare_len);
        }
    }
    else
    {
        ESP_LOGI(TAG, "ESP_GATT_PREP_WRITE_CANCEL");
    }

    // 安全释放内存
    if (prepare_write_env->prepare_buf != NULL)
    {
        ESP_LOGD(TAG, "Freeing prepare buffer: %p, size: %d",
                 prepare_write_env->prepare_buf, prepare_write_env->prepare_len);
        free(prepare_write_env->prepare_buf);
        prepare_write_env->prepare_buf = NULL;
    }

    // 重置长度
    prepare_write_env->prepare_len = 0;
}

void gatts_profile_a_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{
    switch (event)
    {
    case ESP_GATTS_REG_EVT:
        // 应用注册事件
        ESP_LOGI(TAG, "REGISTER_APP_EVT, status %d, app_id %d\n", param->reg.status, param->reg.app_id);
        gl_profile_tab[PROFILE_A_APP_ID].service_id.is_primary = true;
        gl_profile_tab[PROFILE_A_APP_ID].service_id.id.inst_id = 0x00;
        gl_profile_tab[PROFILE_A_APP_ID].service_id.id.uuid.len = ESP_UUID_LEN_16;
        gl_profile_tab[PROFILE_A_APP_ID].service_id.id.uuid.uuid.uuid16 = GATTS_SERVICE_UUID_TEST_A;

        // 设置设备名称
        esp_err_t set_dev_name_ret;
        // 使用默认名称
        set_dev_name_ret = esp_ble_gap_set_device_name(TEST_DEVICE_NAME);
        ESP_LOGI(TAG, "使用蓝牙名称: %s", TEST_DEVICE_NAME);

        if (set_dev_name_ret)
        {
            ESP_LOGE(TAG, "set device name failed, error code = %x", set_dev_name_ret);
        }
#ifdef CONFIG_SET_RAW_ADV_DATA
        // 配置原始广播数据
        esp_err_t raw_adv_ret = esp_ble_gap_config_adv_data_raw(raw_adv_data, sizeof(raw_adv_data));
        if (raw_adv_ret)
        {
            ESP_LOGE(TAG, "config raw adv data failed, error code = %x ", raw_adv_ret);
        }
        adv_config_done |= adv_config_flag;
        // 配置原始扫描响应数据
        esp_err_t raw_scan_ret = esp_ble_gap_config_scan_rsp_data_raw(raw_scan_rsp_data, sizeof(raw_scan_rsp_data));
        if (raw_scan_ret)
        {
            ESP_LOGE(TAG, "config raw scan rsp data failed, error code = %x", raw_scan_ret);
        }
        adv_config_done |= scan_rsp_config_flag;
#else
        // 配置广播数据
        esp_err_t ret = esp_ble_gap_config_adv_data(&adv_data);
        if (ret)
        {
            ESP_LOGE(TAG, "config adv data failed, error code = %x", ret);
        }
        adv_config_done |= adv_config_flag;
        // 配置扫描响应数据
        ret = esp_ble_gap_config_adv_data(&scan_rsp_data);
        if (ret)
        {
            ESP_LOGE(TAG, "config scan response data failed, error code = %x", ret);
        }
        adv_config_done |= scan_rsp_config_flag;

#endif
        // 创建GATT服务
        esp_ble_gatts_create_service(gatts_if, &gl_profile_tab[PROFILE_A_APP_ID].service_id, GATTS_NUM_HANDLE_TEST_A);
        break;
    case ESP_GATTS_READ_EVT:
    {
        // 处理读请求事件
        ESP_LOGI(TAG, "GATT_READ_EVT, conn_id %d, trans_id %" PRIu32 ", handle %d\n", param->read.conn_id, param->read.trans_id, param->read.handle);
        esp_gatt_rsp_t rsp;
        memset(&rsp, 0, sizeof(esp_gatt_rsp_t));
        rsp.attr_value.handle = param->read.handle;
        rsp.attr_value.len = 4;
        rsp.attr_value.value[0] = 0xde;
        rsp.attr_value.value[1] = 0xed;
        rsp.attr_value.value[2] = 0xbe;
        rsp.attr_value.value[3] = 0xef;
        // 发送读响应
        esp_ble_gatts_send_response(gatts_if, param->read.conn_id, param->read.trans_id,
                                    ESP_GATT_OK, &rsp);
        break;
    }
    case ESP_GATTS_WRITE_EVT:
    {
        // 处理写请求事件
        // ESP_LOGI(TAG, "GATT_WRITE_EVT, conn_id %d, trans_id %" PRIu32 ", handle %d", param->write.conn_id, param->write.trans_id, param->write.handle);
        if (!param->write.is_prep)
        {
            // 调试输出
            // ESP_LOGI(TAG, "GATT_WRITE_EVT, value len %d, value :", param->write.len);
            // esp_log_buffer_hex(TAG, param->write.value, param->write.len);
            if (gl_profile_tab[PROFILE_A_APP_ID].descr_handle == param->write.handle && param->write.len == 2)
            {
                uint16_t descr_value = param->write.value[1] << 8 | param->write.value[0];
                if (descr_value == 0x0001)
                {
#if 0
                    if (a_property & ESP_GATT_CHAR_PROP_BIT_NOTIFY)
                    {
                        // 启用通知功能
                        ESP_LOGI(TAG, "notify enable");
                        uint8_t notify_data[15];
                        for (int i = 0; i < sizeof(notify_data); ++i)
                        {
                            notify_data[i] = i % 0xff;
                        }
                        // 发送通知数据
                        esp_ble_gatts_send_indicate(gatts_if, param->write.conn_id, gl_profile_tab[PROFILE_A_APP_ID].char_handle,
                                                    sizeof(notify_data), notify_data, false);
                    }
#endif
                    notificationsEnabled = true; // 启用通知，设置标志
                    ESP_LOGI(TAG, "notify enable");
                }
                else if (descr_value == 0x0002)
                {
#if 0
                    if (a_property & ESP_GATT_CHAR_PROP_BIT_INDICATE)
                    {
                        // 启用指示功能
                        ESP_LOGI(TAG, "indicate enable");
                        uint8_t indicate_data[15];
                        for (int i = 0; i < sizeof(indicate_data); ++i)
                        {
                            indicate_data[i] = i % 0xff;
                        }
                        // 发送指示数据
                        esp_ble_gatts_send_indicate(gatts_if, param->write.conn_id, gl_profile_tab[PROFILE_A_APP_ID].char_handle,
                                                    sizeof(indicate_data), indicate_data, true);
                    }
#endif
                    // 启用指示功能
                    ESP_LOGI(TAG, "indicate enable");
                }
                else if (descr_value == 0x0000)
                {
                    notificationsEnabled = false; // 禁用通知，清除标志
                    // 禁用通知/指示功能
                    ESP_LOGI(TAG, "notify/indicate disable ");
                }
                else
                {
                    // 未知的描述符值
                    ESP_LOGE(TAG, "unknown descr value");
                    esp_log_buffer_hex(TAG, param->write.value, param->write.len);
                }
            }
            else if (param->write.handle == gl_profile_tab[PROFILE_A_APP_ID].char_handle)
            {
                // 直接写入特性值，入队数据供 blue_rec_task 处理
                received_data_t received_data;
                if (param->write.len <= RECEIVED_DATA_MAX_LEN)
                {
                    memcpy(received_data.data, param->write.value, param->write.len);
                    received_data.len = param->write.len;
                    if (xQueueSend(received_data_queue, &received_data, 0) != pdPASS)
                    {
                        ESP_LOGE(TAG, "队列已满，无法入队数据");
                    }
                }
                else
                {
                    ESP_LOGE(TAG, "接收数据过长");
                }
            }
        }
        example_write_event_env(gatts_if, &a_prepare_write_env, param); // 写入事件处理
        break;
    }
    case ESP_GATTS_EXEC_WRITE_EVT:
        // 执行写操作事件
        ESP_LOGI(TAG, "ESP_GATTS_EXEC_WRITE_EVT");
        esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, ESP_GATT_OK, NULL);
        // 处理准备写入的执行事件，入队累积数据
        if (param->exec_write.exec_write_flag == ESP_GATT_PREP_WRITE_EXEC)
        {
            received_data_t received_data;
            if (a_prepare_write_env.prepare_len <= RECEIVED_DATA_MAX_LEN)
            {
                memcpy(received_data.data, a_prepare_write_env.prepare_buf, a_prepare_write_env.prepare_len);
                received_data.len = a_prepare_write_env.prepare_len;
                if (xQueueSend(received_data_queue, &received_data, 0) != pdPASS)
                {
                    ESP_LOGE(TAG, "队列已满，无法入队数据");
                }
                else
                {
                    ESP_LOGI(TAG, "入队准备数据，长度 %d", a_prepare_write_env.prepare_len);
                }
            }
            else
            {
                ESP_LOGE(TAG, "准备数据过长");
            }
        }
        example_exec_write_event_env(&a_prepare_write_env, param);
        break;
    case ESP_GATTS_MTU_EVT:
        // MTU更新事件
        ESP_LOGI(TAG, "ESP_GATTS_MTU_EVT, MTU %d", param->mtu.mtu);
        break;
    case ESP_GATTS_UNREG_EVT:
        break;
    case ESP_GATTS_CREATE_EVT:
        // 服务创建事件
        ESP_LOGI(TAG, "CREATE_SERVICE_EVT, status %d,  service_handle %d\n", param->create.status, param->create.service_handle);
        gl_profile_tab[PROFILE_A_APP_ID].service_handle = param->create.service_handle;
        gl_profile_tab[PROFILE_A_APP_ID].char_uuid.len = ESP_UUID_LEN_16;
        gl_profile_tab[PROFILE_A_APP_ID].char_uuid.uuid.uuid16 = GATTS_CHAR_UUID_TEST_A;

        // 启动服务
        esp_ble_gatts_start_service(gl_profile_tab[PROFILE_A_APP_ID].service_handle);
        a_property = ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_WRITE | ESP_GATT_CHAR_PROP_BIT_NOTIFY;
        // 添加特征值
        esp_err_t add_char_ret = esp_ble_gatts_add_char(gl_profile_tab[PROFILE_A_APP_ID].service_handle, &gl_profile_tab[PROFILE_A_APP_ID].char_uuid,
                                                        ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
                                                        a_property,
                                                        &gatts_demo_char1_val, NULL);
        if (add_char_ret)
        {
            ESP_LOGE(TAG, "add char failed, error code =%x", add_char_ret);
        }
        break;
    case ESP_GATTS_ADD_INCL_SRVC_EVT:
        break;
    case ESP_GATTS_ADD_CHAR_EVT:
    {
        uint16_t length = 0;
        const uint8_t *prf_char;

        ESP_LOGI(TAG, "ADD_CHAR_EVT, status %d,  attr_handle %d, service_handle %d\n",
                 param->add_char.status, param->add_char.attr_handle, param->add_char.service_handle);
        gl_profile_tab[PROFILE_A_APP_ID].char_handle = param->add_char.attr_handle;
        gl_profile_tab[PROFILE_A_APP_ID].descr_uuid.len = ESP_UUID_LEN_16;
        gl_profile_tab[PROFILE_A_APP_ID].descr_uuid.uuid.uuid16 = ESP_GATT_UUID_CHAR_CLIENT_CONFIG;
        esp_err_t get_attr_ret = esp_ble_gatts_get_attr_value(param->add_char.attr_handle, &length, &prf_char);
        if (get_attr_ret == ESP_FAIL)
        {
            ESP_LOGE(TAG, "ILLEGAL HANDLE");
        }

        ESP_LOGI(TAG, "the gatts demo char length = %x\n", length);
        for (int i = 0; i < length; i++)
        {
            ESP_LOGI(TAG, "prf_char[%x] =%x\n", i, prf_char[i]);
        }
        esp_err_t add_descr_ret = esp_ble_gatts_add_char_descr(gl_profile_tab[PROFILE_A_APP_ID].service_handle, &gl_profile_tab[PROFILE_A_APP_ID].descr_uuid,
                                                               ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE, NULL, NULL);
        if (add_descr_ret)
        {
            ESP_LOGE(TAG, "add char descr failed, error code =%x", add_descr_ret);
        }
        break;
    }
    case ESP_GATTS_ADD_CHAR_DESCR_EVT:
        gl_profile_tab[PROFILE_A_APP_ID].descr_handle = param->add_char_descr.attr_handle;
        ESP_LOGI(TAG, "ADD_DESCR_EVT, status %d, attr_handle %d, service_handle %d\n",
                 param->add_char_descr.status, param->add_char_descr.attr_handle, param->add_char_descr.service_handle);
        break;
    case ESP_GATTS_DELETE_EVT:
        break;
    case ESP_GATTS_START_EVT:
        ESP_LOGI(TAG, "SERVICE_START_EVT, status %d, service_handle %d\n",
                 param->start.status, param->start.service_handle);
        break;
    case ESP_GATTS_STOP_EVT:
        break;
    case ESP_GATTS_CONNECT_EVT:
    {
        esp_ble_conn_update_params_t conn_params = {0};
        memcpy(conn_params.bda, param->connect.remote_bda, sizeof(esp_bd_addr_t));
        /* For the IOS system, please reference the apple official documents about the ble connection parameters restrictions. */
        conn_params.latency = 0;
        conn_params.max_int = 0x20; // max_int = 0x20*1.25ms = 40ms
        conn_params.min_int = 0x10; // min_int = 0x10*1.25ms = 20ms
        conn_params.timeout = 400;  // timeout = 400*10ms = 4000ms
        ESP_LOGI(TAG, "ESP_GATTS_CONNECT_EVT, conn_id %d, remote %02x:%02x:%02x:%02x:%02x:%02x:",
                 param->connect.conn_id,
                 param->connect.remote_bda[0], param->connect.remote_bda[1], param->connect.remote_bda[2],
                 param->connect.remote_bda[3], param->connect.remote_bda[4], param->connect.remote_bda[5]);
        gl_profile_tab[PROFILE_A_APP_ID].conn_id = param->connect.conn_id;
        connected = true; // 设置连接标志
        esp_ble_gap_update_conn_params(&conn_params);
        uint8_t audio_data = 1;
        xQueueSend(audio_queue, (void *)&audio_data, portMAX_DELAY);
        ESP_LOGW(TAG, "设备A蓝牙已链接");
        break;
    }
    case ESP_GATTS_DISCONNECT_EVT:
        ESP_LOGI(TAG, "ESP_GATTS_DISCONNECT_EVT, disconnect reason 0x%x", param->disconnect.reason);
        // 清理准备写入环境，防止内存泄漏
        cleanup_prepare_write_env(&a_prepare_write_env);
        cleanup_prepare_write_env(&b_prepare_write_env);
        ESP_LOGI(TAG, "Cleaned up prepare write environments on disconnect");

        esp_ble_gap_start_advertising(&adv_params);
        uint8_t audio_data = 2;
        xQueueSend(audio_queue, (void *)&audio_data, portMAX_DELAY);
        connected = false; // 清除连接标志
        ESP_LOGW(TAG, "设备A蓝牙已断开");
        break;
    case ESP_GATTS_CONF_EVT:
        ESP_LOGI(TAG, "ESP_GATTS_CONF_EVT, status %d attr_handle %d", param->conf.status, param->conf.handle);
        if (param->conf.status != ESP_GATT_OK)
        {
            esp_log_buffer_hex(TAG, param->conf.value, param->conf.len);
        }
        break;
    case ESP_GATTS_OPEN_EVT:
    case ESP_GATTS_CANCEL_OPEN_EVT:
    case ESP_GATTS_CLOSE_EVT:
    case ESP_GATTS_LISTEN_EVT:
    case ESP_GATTS_CONGEST_EVT:
    default:
        break;
    }
}

void gatts_profile_b_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{
    switch (event)
    {
    case ESP_GATTS_REG_EVT:
        // 应用注册事件
        ESP_LOGI(TAG, "REGISTER_APP_EVT, status %d, app_id %d\n", param->reg.status, param->reg.app_id);
        gl_profile_tab[PROFILE_B_APP_ID].service_id.is_primary = true;
        gl_profile_tab[PROFILE_B_APP_ID].service_id.id.inst_id = 0x00;
        gl_profile_tab[PROFILE_B_APP_ID].service_id.id.uuid.len = ESP_UUID_LEN_16;
        gl_profile_tab[PROFILE_B_APP_ID].service_id.id.uuid.uuid.uuid16 = GATTS_SERVICE_UUID_TEST_B;

        // 设置设备名称
        esp_err_t set_dev_name_ret;
        // 使用默认名称
        set_dev_name_ret = esp_ble_gap_set_device_name(TEST_DEVICE_NAME);
        ESP_LOGI(TAG, "使用蓝牙名称: %s", TEST_DEVICE_NAME);

        if (set_dev_name_ret)
        {
            ESP_LOGE(TAG, "set device name failed, error code = %x", set_dev_name_ret);
        }
#ifdef CONFIG_SET_RAW_ADV_DATA
        // 配置原始广播数据
        esp_err_t raw_adv_ret = esp_ble_gap_config_adv_data_raw(raw_adv_data, sizeof(raw_adv_data));
        if (raw_adv_ret)
        {
            ESP_LOGE(TAG, "config raw adv data failed, error code = %x ", raw_adv_ret);
        }
        adv_config_done |= adv_config_flag;
        // 配置原始扫描响应数据
        esp_err_t raw_scan_ret = esp_ble_gap_config_scan_rsp_data_raw(raw_scan_rsp_data, sizeof(raw_scan_rsp_data));
        if (raw_scan_ret)
        {
            ESP_LOGE(TAG, "config raw scan rsp data failed, error code = %x", raw_scan_ret);
        }
        adv_config_done |= scan_rsp_config_flag;
#else
        // 配置广播数据
        esp_err_t ret = esp_ble_gap_config_adv_data(&adv_data);
        if (ret)
        {
            ESP_LOGE(TAG, "config adv data failed, error code = %x", ret);
        }
        adv_config_done |= adv_config_flag;
        // 配置扫描响应数据
        ret = esp_ble_gap_config_adv_data(&scan_rsp_data);
        if (ret)
        {
            ESP_LOGE(TAG, "config scan response data failed, error code = %x", ret);
        }
        adv_config_done |= scan_rsp_config_flag;

#endif
        // 创建GATT服务
        esp_ble_gatts_create_service(gatts_if, &gl_profile_tab[PROFILE_B_APP_ID].service_id, GATTS_NUM_HANDLE_TEST_B);
        break;
    case ESP_GATTS_READ_EVT:
    {
        ESP_LOGI(TAG, "GATT_READ_EVT, conn_id %d, trans_id %" PRIu32 ", handle %d\n", param->read.conn_id, param->read.trans_id, param->read.handle);
        esp_gatt_rsp_t rsp;
        memset(&rsp, 0, sizeof(esp_gatt_rsp_t));
        rsp.attr_value.handle = param->read.handle;
        rsp.attr_value.len = 4;
        rsp.attr_value.value[0] = 0xde;
        rsp.attr_value.value[1] = 0xed;
        rsp.attr_value.value[2] = 0xbe;
        rsp.attr_value.value[3] = 0xef;
        esp_ble_gatts_send_response(gatts_if, param->read.conn_id, param->read.trans_id,
                                    ESP_GATT_OK, &rsp);
        break;
    }
    case ESP_GATTS_WRITE_EVT:
    {
        ESP_LOGI(TAG, "GATT_WRITE_EVT, conn_id %d, trans_id %" PRIu32 ", handle %d\n", param->write.conn_id, param->write.trans_id, param->write.handle);
        if (!param->write.is_prep)
        {
            ESP_LOGI(TAG, "GATT_WRITE_EVT, value len %d, value :", param->write.len);
            esp_log_buffer_hex(TAG, param->write.value, param->write.len);
            if (gl_profile_tab[PROFILE_B_APP_ID].descr_handle == param->write.handle && param->write.len == 2)
            {
                uint16_t descr_value = param->write.value[1] << 8 | param->write.value[0];
                if (descr_value == 0x0001)
                {
                    if (b_property & ESP_GATT_CHAR_PROP_BIT_NOTIFY)
                    {
                        ESP_LOGI(TAG, "notify enable");
                        uint8_t notify_data[15];
                        for (int i = 0; i < sizeof(notify_data); ++i)
                        {
                            notify_data[i] = i % 0xff;
                        }
                        // the size of notify_data[] need less than MTU size
                        esp_ble_gatts_send_indicate(gatts_if, param->write.conn_id, gl_profile_tab[PROFILE_B_APP_ID].char_handle,
                                                    sizeof(notify_data), notify_data, false);
                    }
                }
                else if (descr_value == 0x0002)
                {
                    if (b_property & ESP_GATT_CHAR_PROP_BIT_INDICATE)
                    {
                        ESP_LOGI(TAG, "indicate enable");
                        uint8_t indicate_data[15];
                        for (int i = 0; i < sizeof(indicate_data); ++i)
                        {
                            indicate_data[i] = i % 0xff;
                        }
                        // the size of indicate_data[] need less than MTU size
                        esp_ble_gatts_send_indicate(gatts_if, param->write.conn_id, gl_profile_tab[PROFILE_B_APP_ID].char_handle,
                                                    sizeof(indicate_data), indicate_data, true);
                    }
                }
                else if (descr_value == 0x0000)
                {
                    ESP_LOGI(TAG, "notify/indicate disable ");
                }
                else
                {
                    ESP_LOGE(TAG, "unknown value");
                }
            }
            else if (param->write.handle == gl_profile_tab[PROFILE_B_APP_ID].char_handle)
            {
                // 直接写入特性值，入队数据供 blue_rec_task 处理
                received_data_t received_data;
                if (param->write.len <= RECEIVED_DATA_MAX_LEN)
                {
                    memcpy(received_data.data, param->write.value, param->write.len);
                    received_data.len = param->write.len;
                    if (xQueueSend(received_data_queue, &received_data, 0) != pdPASS)
                    {
                        ESP_LOGE(TAG, "队列已满，无法入队数据");
                    }
                }
                else
                {
                    ESP_LOGE(TAG, "接收数据过长");
                }
            }
        }
        example_write_event_env(gatts_if, &b_prepare_write_env, param);
        break;
    }
    case ESP_GATTS_EXEC_WRITE_EVT:
        ESP_LOGI(TAG, "ESP_GATTS_EXEC_WRITE_EVT");
        esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, ESP_GATT_OK, NULL);
        // 处理准备写入的执行事件，入队累积数据
        if (param->exec_write.exec_write_flag == ESP_GATT_PREP_WRITE_EXEC)
        {
            received_data_t received_data;
            if (b_prepare_write_env.prepare_len <= RECEIVED_DATA_MAX_LEN)
            {
                memcpy(received_data.data, b_prepare_write_env.prepare_buf, b_prepare_write_env.prepare_len);
                received_data.len = b_prepare_write_env.prepare_len;
                if (xQueueSend(received_data_queue, &received_data, 0) != pdPASS)
                {
                    ESP_LOGE(TAG, "队列已满，无法入队数据");
                }
                else
                {
                    ESP_LOGI(TAG, "入队准备数据，长度 %d", b_prepare_write_env.prepare_len);
                }
            }
            else
            {
                ESP_LOGE(TAG, "准备数据过长");
            }
        }
        example_exec_write_event_env(&b_prepare_write_env, param);
        break;
    case ESP_GATTS_MTU_EVT:
        ESP_LOGI(TAG, "ESP_GATTS_MTU_EVT, MTU %d", param->mtu.mtu);
        break;
    case ESP_GATTS_UNREG_EVT:
        break;
    case ESP_GATTS_CREATE_EVT:
        ESP_LOGI(TAG, "CREATE_SERVICE_EVT, status %d,  service_handle %d\n", param->create.status, param->create.service_handle);
        gl_profile_tab[PROFILE_B_APP_ID].service_handle = param->create.service_handle;
        gl_profile_tab[PROFILE_B_APP_ID].char_uuid.len = ESP_UUID_LEN_16;
        gl_profile_tab[PROFILE_B_APP_ID].char_uuid.uuid.uuid16 = GATTS_CHAR_UUID_TEST_B;

        esp_ble_gatts_start_service(gl_profile_tab[PROFILE_B_APP_ID].service_handle);
        b_property = ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_WRITE | ESP_GATT_CHAR_PROP_BIT_NOTIFY;
        esp_err_t add_char_ret = esp_ble_gatts_add_char(gl_profile_tab[PROFILE_B_APP_ID].service_handle, &gl_profile_tab[PROFILE_B_APP_ID].char_uuid,
                                                        ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
                                                        b_property,
                                                        NULL, NULL);
        if (add_char_ret)
        {
            ESP_LOGE(TAG, "add char failed, error code =%x", add_char_ret);
        }
        break;
    case ESP_GATTS_ADD_INCL_SRVC_EVT:
        break;
    case ESP_GATTS_ADD_CHAR_EVT:
        ESP_LOGI(TAG, "ADD_CHAR_EVT, status %d,  attr_handle %d, service_handle %d\n",
                 param->add_char.status, param->add_char.attr_handle, param->add_char.service_handle);

        gl_profile_tab[PROFILE_B_APP_ID].char_handle = param->add_char.attr_handle;
        gl_profile_tab[PROFILE_B_APP_ID].descr_uuid.len = ESP_UUID_LEN_16;
        gl_profile_tab[PROFILE_B_APP_ID].descr_uuid.uuid.uuid16 = ESP_GATT_UUID_CHAR_CLIENT_CONFIG;
        esp_ble_gatts_add_char_descr(gl_profile_tab[PROFILE_B_APP_ID].service_handle, &gl_profile_tab[PROFILE_B_APP_ID].descr_uuid,
                                     ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
                                     NULL, NULL);
        break;
    case ESP_GATTS_ADD_CHAR_DESCR_EVT:
        gl_profile_tab[PROFILE_B_APP_ID].descr_handle = param->add_char_descr.attr_handle;
        ESP_LOGI(TAG, "ADD_DESCR_EVT, status %d, attr_handle %d, service_handle %d\n",
                 param->add_char_descr.status, param->add_char_descr.attr_handle, param->add_char_descr.service_handle);
        break;
    case ESP_GATTS_DELETE_EVT:
        break;
    case ESP_GATTS_START_EVT:
        ESP_LOGI(TAG, "SERVICE_START_EVT, status %d, service_handle %d\n",
                 param->start.status, param->start.service_handle);
        break;
    case ESP_GATTS_STOP_EVT:
        break;
    case ESP_GATTS_CONNECT_EVT:
        ESP_LOGI(TAG, "CONNECT_EVT, conn_id %d, remote %02x:%02x:%02x:%02x:%02x:%02x:",
                 param->connect.conn_id,
                 param->connect.remote_bda[0], param->connect.remote_bda[1], param->connect.remote_bda[2],
                 param->connect.remote_bda[3], param->connect.remote_bda[4], param->connect.remote_bda[5]);
        gl_profile_tab[PROFILE_B_APP_ID].conn_id = param->connect.conn_id;
        // 更新连接参数
        esp_ble_conn_update_params_t conn_params = {0};
        memcpy(conn_params.bda, param->connect.remote_bda, sizeof(esp_bd_addr_t));
        conn_params.latency = 0;
        conn_params.max_int = 0x20; // max_int = 0x20*1.25ms = 40ms
        conn_params.min_int = 0x10; // min_int = 0x10*1.25ms = 20ms
        conn_params.timeout = 400;  // timeout = 400*10ms = 4000ms
        // 启动连接参数更新
        esp_ble_gap_update_conn_params(&conn_params);
        // 设置连接标志
        connected = true;
        // 发送音频数据到音频队列
        // uint8_t audio_data = 1;
        // xQueueSend(audio_queue, (void *)&audio_data, portMAX_DELAY);
        ESP_LOGW(TAG, "设备B蓝牙已链接");
        break;
    case ESP_GATTS_CONF_EVT:
        ESP_LOGI(TAG, "ESP_GATTS_CONF_EVT status %d attr_handle %d", param->conf.status, param->conf.handle);
        if (param->conf.status != ESP_GATT_OK)
        {
            esp_log_buffer_hex(TAG, param->conf.value, param->conf.len);
        }
        break;
    case ESP_GATTS_DISCONNECT_EVT:
        ESP_LOGI(TAG, "ESP_GATTS_DISCONNECT_EVT, disconnect reason 0x%x", param->disconnect.reason);
        // 清理准备写入环境
        cleanup_prepare_write_env(&b_prepare_write_env);
        // 重新启动广播
        esp_ble_gap_start_advertising(&adv_params);
        // 设置连接标志
        connected = false;
        // 发送音频数据到音频队列
        // uint8_t audio_data = 2;
        // xQueueSend(audio_queue, (void *)&audio_data, portMAX_DELAY);
        ESP_LOGW(TAG, "设备B蓝牙已断开");
        break;
    case ESP_GATTS_OPEN_EVT:
    case ESP_GATTS_CANCEL_OPEN_EVT:
    case ESP_GATTS_CLOSE_EVT:
    case ESP_GATTS_LISTEN_EVT:
    case ESP_GATTS_CONGEST_EVT:
    default:
        break;
    }
}

/**
 * @brief GATT事件处理函数
 * @param event GATT事件类型
 * @param gatts_if GATT接口
 * @param param GATT事件参数指针
 */
void gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{
    /* 如果是注册事件，为每个profile存储gatts_if */
    if (event == ESP_GATTS_REG_EVT)
    {
        if (param->reg.status == ESP_GATT_OK)
        {
            // 注册成功，保存gatts_if到对应的profile
            gl_profile_tab[param->reg.app_id].gatts_if = gatts_if;
        }
        else
        {
            // 注册失败，记录错误日志
            ESP_LOGI(TAG, "Reg app failed, app_id %04x, status %d\n",
                     param->reg.app_id,
                     param->reg.status);
            return;
        }
    }

    /* 如果gatts_if等于profile A，调用profile A的回调处理函数，
     * 这里会调用每个profile的回调函数 */
    do
    {
        int idx;
        // 遍历所有profile
        for (idx = 0; idx < PROFILE_NUM; idx++)
        {
            // 如果gatts_if为ESP_GATT_IF_NONE（未指定特定gatts_if），
            // 或者匹配当前profile的gatts_if
            if (gatts_if == ESP_GATT_IF_NONE ||
                gatts_if == gl_profile_tab[idx].gatts_if)
            {
                // 如果当前profile有回调函数，则调用
                if (gl_profile_tab[idx].gatts_cb)
                {
                    gl_profile_tab[idx].gatts_cb(event, gatts_if, param);
                }
            }
        }
    } while (0);
}

void blue_init(void)
{
    esp_err_t ret;

    // 初始化准备写入环境，确保内存状态正确
    init_prepare_write_env(&a_prepare_write_env);
    init_prepare_write_env(&b_prepare_write_env);
    ESP_LOGI(TAG, "Initialized prepare write environments");

    // 创建接收数据队列，容量为3
    received_data_queue = xQueueCreate(3, sizeof(received_data_t));
    if (received_data_queue == NULL)
    {
        ESP_LOGE(TAG, "创建队列失败");
        return;
    }

    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ret = esp_bt_controller_init(&bt_cfg);
    if (ret)
    {
        ESP_LOGE(TAG, "%s initialize controller failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }

    ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
    if (ret)
    {
        ESP_LOGE(TAG, "%s enable controller failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }
    ret = esp_bluedroid_init();
    if (ret)
    {
        ESP_LOGE(TAG, "%s init bluetooth failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }
    ret = esp_bluedroid_enable();
    if (ret)
    {
        ESP_LOGE(TAG, "%s enable bluetooth failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }

    ret = esp_ble_gatts_register_callback(gatts_event_handler);
    if (ret)
    {
        ESP_LOGE(TAG, "gatts register error, error code = %x", ret);
        return;
    }
    ret = esp_ble_gap_register_callback(gap_event_handler);
    if (ret)
    {
        ESP_LOGE(TAG, "gap register error, error code = %x", ret);
        return;
    }
    ret = esp_ble_gatts_app_register(PROFILE_A_APP_ID);
    if (ret)
    {
        ESP_LOGE(TAG, "gatts app register error, error code = %x", ret);
        return;
    }
    ret = esp_ble_gatts_app_register(PROFILE_B_APP_ID);
    if (ret)
    {
        ESP_LOGE(TAG, "gatts app register error, error code = %x", ret);
        return;
    }
    esp_err_t local_mtu_ret = esp_ble_gatt_set_local_mtu(512);
    if (local_mtu_ret)
    {
        ESP_LOGE(TAG, "set local  MTU failed, error code = %x", local_mtu_ret);
    }
    return;
}
