#include "all_include.h"

static const char *TAG = "MAIN";

static void system_nvs_flash_init(void);
static void system_nvs_flash_restart(void);
static void hardware_studio_init(void);
static esp_err_t bsp_spiffs_mount(void);
static void hardware_init(void);

void app_main(void)
{
    //*=======初始化NVS=======*//
    system_nvs_flash_init(); // 初始化NVS
    //*=======初始化Wi-Fi=======*//
    wifi_init_sta(); // 初始化Wi-Fi
    //*=======初始化硬件=======*//
    imu_init();             // IMU初始化
    hardware_studio_init(); // 初始化音频硬件
    safe_play_index(0);     // 播报开机音效
    hardware_init();        // 外设及硬件初始化
#if BLUE_CONTROL
    //*=======蓝牙初始化=======*//
    blue_init(); // 蓝牙初始化
#endif
    //*=======任务初始化=======*//
    rtos_audio_control_init(); // 音频播報任务初始化
    rtos_uart_init();          // 串口通信任务初始化
    app_pid_init();            // PID控制器初始化
#if UPDATE_ODOMETRY_DEBUG
    app_move_control_init(); // 运动控制初始化
#endif
#if BLUE_CONTROL
    rtos_blue_control_init(); // 蓝牙控制任务初始化
#endif
}

static void hardware_studio_init(void)
{
    bsp_spiffs_mount();                         // SPIFFS文件系统初始化
    bsp_codec_init();                           // 音频硬件初始化
    mp3_player_init();                          // MP3播放器初始化
    bsp_codec_volume_set(VOLUME_DEFAULT, NULL); // 设置默认音量
}

static void hardware_init(void)
{
    // 串口通信启动
    uart_comm_init_all();
#if SERVO_DEBUG
    // 舵机启动
    servo_init();
#endif
    // 电机及编码器初始化
    motor_init();
}
static void system_nvs_flash_init(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
#if NVS_CLEAR_VERSION
    // 烧录调试模式：每次启动时强制格式化NVS分区
    system_nvs_flash_restart();
#endif
}

static void system_nvs_flash_restart(void)
{
    // 1.格式化NVS分区
    ESP_LOGI(TAG, "正在格式化NVS分区...");
    esp_err_t ret = nvs_flash_erase();
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "格式化NVS分区失败: %s", esp_err_to_name(ret));
    }
    else
    {
        // 重新初始化NVS
        ret = nvs_flash_init();
        if (ret != ESP_OK)
        {
            ESP_LOGE(TAG, "重新初始化NVS失败: %s", esp_err_to_name(ret));
        }
        else
        {
            ESP_LOGI(TAG, "NVS分区格式化完成");
        }
    }
}

static esp_err_t bsp_spiffs_mount(void)
{
    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = "storage",
        .max_files = 5,
        .format_if_mount_failed = false,
    };

    esp_err_t ret_val = esp_vfs_spiffs_register(&conf);

    ESP_ERROR_CHECK(ret_val);

    size_t total = 0, used = 0;
    ret_val = esp_spiffs_info(conf.partition_label, &total, &used);
    if (ret_val != ESP_OK)
    {
        ESP_LOGE(TAG, "获取SPIFFS分区信息失败 (%s)", esp_err_to_name(ret_val));
    }
    else
    {
        ESP_LOGI(TAG, "分区大小: 总计 %d, 已用 %d", total, used);
    }

    return ret_val;
}
