#include "app_blue_control.h"
#include "esp_spp_api.h"
#include "gatts_server.h"
#include "esp_log.h"
#include <string.h>
#include <inttypes.h>
#include <stdlib.h>
#include <math.h>
#include "esp_heap_caps.h"
static const char *TAG = "BLE";
//*============任务栈大小定义============*//
#define BLUE_REC_TASK_STACK_SIZE (8 * 1024)
#define BLUE_REC_STACK_DEPTH (BLUE_REC_TASK_STACK_SIZE / sizeof(StackType_t))

//*============全局变量结构体============*//
ControlCommand control_command;

//*============全局变量============*//
bool is_first_connect = true; // 首次连接标志位,true:首次连接,false:非首次连接续
uint32_t g_ble_send_interval_ms = BLE_SEND_INTERVAL_DEBUG_MS;

//*============任务句柄+Freertos资源============*//
TaskHandle_t blue_rec_task_handle = NULL;

//*============外部资源============*//
extern QueueHandle_t received_data_queue;
/********* 任务函数 *********/
static void blue_rec_task(void *pvParameters);

/********* 内部函数 *********/

/********* 外部函数 *********/
void rtos_blue_control_init(void)
{
    //*=====创建蓝牙数据接收任务=====*//
    BaseType_t rec_result = xTaskCreatePinnedToCore(
        blue_rec_task,
        "blue_rec_task",
        BLUE_REC_STACK_DEPTH, // 正确的栈深度单位
        NULL,
        6, // 稍高优先级，确保及时处理接收数据
        &blue_rec_task_handle,
        1 // 固定到核心1
    );

    if (rec_result != pdPASS)
    {
        ESP_LOGE(TAG, "蓝牙数据接收任务创建失败");
    }
    else
    {
        ESP_LOGI(TAG, "蓝牙接收任务创建成功");
    }
}

/**
 * @brief 蓝牙数据接收任务
 * @param pvParameters 任务参数（未使用）
 */
static void blue_rec_task(void *pvParameters)
{
    wxx_received_data_t received_data;
    ESP_LOGI(TAG, "蓝牙数据接收任务启动");

    while (1)
    {
        // 使用100ms超时等待队列中的数据，以便能够处理计数器
        if (xQueueReceive(received_data_queue, &received_data, pdMS_TO_TICKS(100)) == pdTRUE)
        {
            // 接收到数据时的处理
            // parse_received_data(&received_data);
        }
    }
}
