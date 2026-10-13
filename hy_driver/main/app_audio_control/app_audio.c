/**
 * @file app_audio.c
 * @brief 音频控制与 HTTPS 流媒体播放实现
 *
 * 本模块负责：
 * 1. 初始化音频播放系统
 * 2. 实现基于 VFS (虚拟文件系统) 的管道机制，将网络流转换为文件流
 * 3. 创建 HTTPS 客户端任务，从服务器拉取音频数据并写入管道
 * 4. 配合 audio_player 组件实现流式播放
 *
 * @author ZHC
 * @date 2025
 */

#include "app_audio.h"
#include "app_uart_control/app_uart_control.h"

static const char *TAG = "APP_AUDIO";
TaskHandle_t audio_player_task_handle;
TaskHandle_t https_stream_task_handle;

/* 函数声明 */
static void audio_player_task(void *pvParameters);
static void https_stream_task(void *pvParameters);

/** @brief 本地音频播放队列（用于播放本地提示音等） */
QueueHandle_t audio_queue = NULL;

/* ========================= VFS Pipe (虚拟管道) 定义 ========================= */

/** @brief VFS 挂载路径 */
#define PIPE_VFS_PATH "/dev/net"
/** @brief 虚拟音频流文件路径 */
#define PIPE_FILE_PATH "/dev/net/stream"
/** @brief 环形缓冲区大小 (20KB)，用于缓存网络音频数据 */
#define PIPE_RINGBUF_SIZE (20 * 1024)

/** @brief Python TTS 服务器地址 */
#define SERVER_URL "http://192.168.22.219:8090/audio"

/** @brief 环形缓冲区句柄 */
static RingbufHandle_t s_pipe_ringbuf = NULL;
/** @brief 互斥锁（预留，目前主要通过 RingBuffer 自身的线程安全特性） */
static SemaphoreHandle_t s_pipe_mutex = NULL;
/** @brief 流活动标志位，true 表示正在接收网络数据 */
static bool s_stream_active = false;

/* ========================= VFS 接口实现 ========================= */

/**
 * @brief 打开虚拟文件
 *
 * @param path 文件路径
 * @param flags 打开标志
 * @param mode 模式
 * @return int 返回虚拟文件描述符 (始终为 0，因为只有一个流)
 */
static int pipe_open(const char *path, int flags, int mode)
{
    ESP_LOGI(TAG, "VFS Pipe 打开: %s", path);
    // 这里可以重置 RingBuffer，但为了连续性通常不做清空
    return 0;
}

/**
 * @brief 从虚拟文件读取数据
 *
 * 这是流式播放的核心：
 * - 播放器调用 fread -> 映射到此函数
 * - 此函数从 RingBuffer 取数据
 * - 如果 Buffer 为空但网络流仍活跃，此函数会阻塞等待，
 *   从而欺骗播放器认为"磁盘"读取变慢了，而不是文件结束。
 *
 * @param fd 文件描述符
 * @param dst 目标缓冲区
 * @param size 请求读取的字节数
 * @return ssize_t 实际读取的字节数
 */
static ssize_t pipe_read(int fd, void *dst, size_t size)
{
    if (!s_pipe_ringbuf)
    {
        return -1;
    }

    size_t received_size = 0;

    // 1. 尝试从 RingBuffer 读取数据 (非阻塞或短超时)
    // xRingbufferReceiveUpTo 从 ByteBuf 类型缓冲区读取指定长度的数据
    uint8_t *data = (uint8_t *)xRingbufferReceiveUpTo(s_pipe_ringbuf, &received_size, pdMS_TO_TICKS(100), size);

    if (data)
    {
        // 成功读取到数据
        memcpy(dst, data, received_size);
        vRingbufferReturnItem(s_pipe_ringbuf, (void *)data); // 释放缓冲区空间
        return received_size;
    }
    else
    {
        // 2. 如果没有读取到数据
        if (!s_stream_active)
        {
            // 如果流已标记为结束且缓冲区无数据，返回 0 表示 EOF (文件结束)
            // audio_player 收到 0 后会停止播放
            return 0;
        }

        // 如果流还在活动 (网络连接正常) 但暂时没数据 (网速慢或 TTS 生成中)
        // 我们必须阻塞住播放器的读取线程，等待数据到来。
        // 否则返回 0 会导致播放器误判为文件结束而退出。

        // 循环等待策略
        while (s_stream_active)
        {
            // 继续尝试读取，带 100ms 超时
            data = (uint8_t *)xRingbufferReceiveUpTo(s_pipe_ringbuf, &received_size, pdMS_TO_TICKS(100), size);
            if (data)
            {
                memcpy(dst, data, received_size);
                vRingbufferReturnItem(s_pipe_ringbuf, (void *)data);
                return received_size;
            }
            // 如果超时仍无数据，检查网络任务状态
            // 只要 s_stream_active 为真，就继续等待，不返回 0
        }

        return 0; // 流结束
    }
}

/**
 * @brief 关闭虚拟文件
 */
static int pipe_close(int fd)
{
    ESP_LOGI(TAG, "VFS Pipe 关闭");
    return 0;
}

/** @brief VFS 驱动结构体定义 */
static esp_vfs_t s_pipe_vfs = {
    .flags = ESP_VFS_FLAG_DEFAULT,
    .open = pipe_open,
    .read = pipe_read,
    .close = pipe_close,
    // write 等其他接口留空，因为只读
};

/**
 * @brief 初始化音频控制模块
 *
 * 1. 创建本地音频队列
 * 2. 创建并注册 VFS RingBuffer 驱动
 * 3. 启动本地播放任务和 HTTPS 流任务
 */
void rtos_audio_control_init(void)
{
    // 1. 初始化本地音频播放队列 (用于 safe_play_index 等本地文件)
    audio_queue = xQueueCreate(5, sizeof(uint8_t));

    // 2. 初始化 RingBuffer 和 VFS
    // 创建字节流类型的 RingBuffer
    s_pipe_ringbuf = xRingbufferCreate(PIPE_RINGBUF_SIZE, RINGBUF_TYPE_BYTEBUF);
    if (!s_pipe_ringbuf)
    {
        ESP_LOGE(TAG, "RingBuffer 创建失败");
    }
    s_pipe_mutex = xSemaphoreCreateMutex();

    // 注册 VFS 到 /dev/net 路径
    ESP_ERROR_CHECK(esp_vfs_register(PIPE_VFS_PATH, &s_pipe_vfs, NULL));

    // 3. 创建任务
    // 本地音频播放任务 (优先级 3)
    BaseType_t result = pdPASS;
    result = xTaskCreatePinnedToCore(audio_player_task, "audio_player_task", 3 * 1024, NULL, 3, &audio_player_task_handle, 1);
    if (result != pdPASS)
    {
        ESP_LOGE(TAG, "语音播报任务创建失败");
    }

    // HTTPS 网络流任务 (优先级 5，核心 0，处理网络 I/O)
    result = xTaskCreatePinnedToCore(https_stream_task, "https_stream_task", 6 * 1024, NULL, 5, &https_stream_task_handle, 0);
    if (result != pdPASS)
    {
        ESP_LOGE(TAG, "HTTPS 流任务创建失败");
    }
}

/**
 * @brief 本地音频播放任务
 * 处理通过 audio_queue 发送的本地文件索引播放请求
 */
static void audio_player_task(void *pvParameters)
{
    while (1)
    {
        uint8_t audio_data;
        uint8_t latest_audio_data = 0;
        bool has_audio = false;

        // 循环读取队列中的所有音频数据，只保留最新的一个
        while (xQueueReceive(audio_queue, &audio_data, 0) == pdPASS)
        {
            latest_audio_data = audio_data;
            has_audio = true;
        }

        // 如果没有立即可用的音频数据，阻塞等待新数据
        if (!has_audio)
        {
            if (xQueueReceive(audio_queue, &latest_audio_data, pdMS_TO_TICKS(1000)) == pdPASS)
            {
                has_audio = true;
            }
        }

        // 播放最新的音频数据 (本地文件)
        if (has_audio)
        {
            safe_play_index(latest_audio_data);
        }
    }
}

/**
 * @brief HTTP 客户端事件回调
 */
static esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
    return ESP_OK;
}

/**
 * @brief HTTPS 流媒体下载任务
 *
 * 负责：
 * 1. 等待 Wi-Fi 连接
 * 2. 建立到 Python 服务器的长连接
 * 3. 循环读取音频数据并写入 VFS RingBuffer
 * 4. 触发 audio_player 播放虚拟文件
 */
static void https_stream_task(void *pvParameters)
{
    ESP_LOGI(TAG, "HTTPS 流任务启动, 等待 Wi-Fi 连接...");

    // 等待 Wi-Fi 连接成功 (阻塞直到连接完成)
    if (!wifi_wait_connected(portMAX_DELAY))
    {
        ESP_LOGE(TAG, "Wi-Fi 连接等待失败");
        vTaskDelete(NULL);
        return;
    }

    ESP_LOGI(TAG, "Wi-Fi 已连接，准备连接服务器: %s", SERVER_URL);

    // 配置 HTTP 客户端
    // 优化长连接配置：
    // 1. 增大 buffer_size 以应对网络抖动和大数据块
    // 2. 增加 timeout_ms 防止因服务器短暂静默导致的连接断开
    // 3. 显式启用 keep_alive
    esp_http_client_config_t config = {
        .url = SERVER_URL,
        .event_handler = _http_event_handler,
        .buffer_size = 1024 * 16,  // 增大到 16KB，提高吞吐量稳定性
        .timeout_ms = 40000,       // 设置较长超时，允许 LLM 思考
        .keep_alive_enable = true, // 开启 TCP Keep-Alive
        .is_async = true,          // [关键修改] 开启异步模式，允许非阻塞读取
        // .disable_auto_redirect = true, // 如果不需要重定向可开启
    };

    while (1)
    {
        // 初始化 HTTP 客户端
        esp_http_client_handle_t client = esp_http_client_init(&config);
        if (!client)
        {
            ESP_LOGE(TAG, "HTTP client 初始化失败");
            vTaskDelay(pdMS_TO_TICKS(5000));
            continue;
        }

    retry_connect:
        // 打开连接
        esp_err_t err = esp_http_client_open(client, 0);
        if (err == ESP_OK)
        {
            ESP_LOGI(TAG, "连接服务器成功");

            // 1. 标记流活动状态：告诉 VFS 驱动网络是通的，没有数据时应该阻塞等待而不是返回 EOF
            s_stream_active = true;

            // 2. 触发播放器打开虚拟文件
            // 打开 VFS 管道文件
            FILE *fp = fopen(PIPE_FILE_PATH, "r");
            if (fp)
            {
                // 启动播放器。播放器会创建一个新线程调用 fread 读取该文件。
                // fread 会调用我们的 pipe_read 函数。
                audio_player_play(fp);
            }
            else
            {
                ESP_LOGE(TAG, "无法打开 VFS 文件: %s", PIPE_FILE_PATH);
            }

            // 获取 HTTP 响应头长度
            // 注意：对于流式传输 (Chunked)，Content-Length 通常为 -1 (未知)
            int content_length = -1;
            int64_t connect_start_time = esp_timer_get_time();

            // 在异步模式下，循环等待 Headers
            while (1)
            {
                content_length = esp_http_client_fetch_headers(client);
                if (content_length >= 0)
                {
                    break; // 成功获取 Headers
                }

                if (content_length == -1 && (errno == EAGAIN || errno == EWOULDBLOCK))
                {
                    // 检查连接超时 (30秒)
                    if (esp_timer_get_time() - connect_start_time > 30000000)
                    {
                        ESP_LOGE(TAG, "等待 Headers 超时");
                        break;
                    }
                    vTaskDelay(pdMS_TO_TICKS(10));
                    continue;
                }

                ESP_LOGE(TAG, "获取 Headers 失败");
                break;
            }

            if (content_length < 0)
            {
                // Headers 获取失败，断开重连
                goto retry_connect;
            }

            ESP_LOGI(TAG, "HTTP 响应状态码: %d, Content-Length: %d",
                     esp_http_client_get_status_code(client), content_length);

            if (content_length == 0)
            {
                ESP_LOGW(TAG, "警告: Content-Length 为 0，服务器可能未发送数据或连接已结束");
            }

            // 分配更大的临时接收缓冲区 (4KB) 以提高读取效率
            int recv_buf_size = 4096;
            char *buffer = malloc(recv_buf_size);
            if (buffer)
            {
                int64_t last_data_time = esp_timer_get_time();
                bool has_received_data = false;

                while (1)
                {
                    // 3. 从网络读取数据 (非阻塞)
                    int read_len = esp_http_client_read(client, buffer, recv_buf_size);

                    if (read_len > 0)
                    {
                        has_received_data = true;
                        last_data_time = esp_timer_get_time();

                        // 4. 将数据写入 RingBuffer
                        // VFS 驱动的 pipe_read 会从另一端读取这些数据
                        // 如果 RingBuffer 满了，这里会阻塞等待 (pdMS_TO_TICKS(1000))
                        if (xRingbufferSend(s_pipe_ringbuf, buffer, read_len, pdMS_TO_TICKS(1000)) != pdTRUE)
                        {
                            ESP_LOGW(TAG, "RingBuffer 已满，丢弃数据 (播放器处理过慢?)");
                        }
                    }
                    else if (read_len == 0)
                    {
                        // 0 表示连接被对方关闭 (EOF)
                        if (esp_http_client_is_complete_data_received(client))
                        {
                            ESP_LOGI(TAG, "服务器流传输正常结束 (EOF)");
                            extern uint8_t s_audio_stream_flag;
                            s_audio_stream_flag = 4;
                        }
                        else
                        {
                            ESP_LOGW(TAG, "连接被意外关闭");
                        }
                        break;
                    }
                    else
                    {
                        // read_len < 0，检查是否为 EAGAIN
                        if (errno == EAGAIN || errno == EWOULDBLOCK)
                        {
                            int64_t now = esp_timer_get_time();
                            int64_t elapsed_us = now - last_data_time;

                            // 双重超时逻辑：
                            // 1. 如果还未收到任何数据 (LLM 思考中)，允许等待 30秒
                            // 2. 如果已开始接收数据 (正在播放)，静默超过 2秒 则认为流结束
                            int64_t timeout_us = has_received_data ? 2000000 : 30000000; // 2s vs 30s

                            if (elapsed_us > timeout_us)
                            {
                                if (has_received_data)
                                {
                                    ESP_LOGI(TAG, "语音流自然结束 (静默 > 2s)");
                                }
                                else
                                {
                                    ESP_LOGE(TAG, "LLM 响应超时 (> 30s)");
                                }
                                break;
                            }

                            // 暂时无数据，稍后重试
                            vTaskDelay(pdMS_TO_TICKS(10));
                            continue;
                        }

                        ESP_LOGE(TAG, "HTTP 读取错误: errno=%d", errno);
                        break;
                    }
                }
                free(buffer);
            }
            else
            {
                ESP_LOGE(TAG, "内存分配失败");
            }
        }
        else
        {
            ESP_LOGE(TAG, "连接服务器失败: %s", esp_err_to_name(err));
        }

        // 5. 清理与重连逻辑
        s_stream_active = false; // 标记流结束，通知 VFS pipe_read 返回 EOF
        esp_http_client_close(client);
        esp_http_client_cleanup(client);

        ESP_LOGI(TAG, "连接断开，5秒后重试...");
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}
