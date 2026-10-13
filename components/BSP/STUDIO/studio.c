/**
 * @file studio.c
 * @brief ESP32-S3音频编解码器驱动实现文件
 *
 * 本文件实现ESP32-S3平台上音频编解码器(ES8311)的驱动功能，
 * 包括设备初始化、音频参数配置、数据传输等核心功能。
 *
 * @author ZHC
 * @date 2025
 * @version 1.0
 *
 * @note 实现特性:
 *       - ES8311 DAC音频输出驱动
 *       - 动态音频参数配置
 *       - 音量和静音控制
 *
 * @warning 所有公共函数调用前需确保bsp_codec_init()已成功执行
 */

#include "studio.h"

/* ========================= 全局变量区 ========================= */

/** @defgroup GLOBAL_HANDLES 设备句柄全局变量
 * @{
 */
static const char *TAG = "STUDIO";
static esp_codec_dev_handle_t play_dev_handle = NULL;          /**< 音频播放设备句柄(ES8311) */
static i2s_chan_handle_t i2s_tx_chan = NULL;                   /**< I2S发送通道句柄 */
static const audio_codec_data_if_t *i2s_data_if_output = NULL; /**< DAC输出数据接口(ES8311) */
static file_iterator_instance_t *file_iterator = NULL;         /**< 文件迭代器实例 */
static audio_player_config_t player_config = {0};              /**< 音频播放器配置 */
static SemaphoreHandle_t flash_mutex = NULL;
uint8_t g_sys_volume = VOLUME_DEFAULT;
/** @} */

/* ========================= 内部函数声明区 ========================= */
static esp_codec_dev_handle_t bsp_audio_codec_speaker_init(void);
static esp_err_t bsp_audio_output_init(void); /**< ES8311 DAC输出接口初始化 */
static void pa_en_init(void);
static int compare_files_numeric(const void *a, const void *b);
static void sort_file_iterator_list(file_iterator_instance_t *iterator);
static esp_err_t _audio_player_mute_fn(AUDIO_PLAYER_MUTE_SETTING setting);
static esp_err_t _audio_player_write_fn(void *audio_buffer, size_t len, size_t *bytes_written, uint32_t timeout_ms);
static esp_err_t _audio_player_std_clock(uint32_t rate, uint32_t bits_cfg, i2s_slot_mode_t ch);
static void _audio_player_callback(audio_player_cb_ctx_t *ctx);
/* ========================= 内部函数实现区 ========================= */
/**
 * @brief 音频静音控制函数
 * @param setting 静音设置参数，AUDIO_PLAYER_MUTE表示静音，AUDIO_PLAYER_UNMUTE表示取消静音
 * @return esp_err_t 返回操作结果，ESP_OK表示成功
 * @note 该函数用于控制音频编解码器的静音状态，优化后减少不必要的音量设置操作
 */
static esp_err_t _audio_player_mute_fn(AUDIO_PLAYER_MUTE_SETTING setting)
{
    esp_err_t ret = ESP_OK;
    static bool volume_initialized = false;
    static uint8_t last_volume = 0;

    // 设置编解码器静音状态
    bsp_codec_mute_set(setting == AUDIO_PLAYER_MUTE ? true : false);

    // 只在第一次取消静音或音量发生变化时设置音量，避免频繁I2C操作
    if (setting == AUDIO_PLAYER_UNMUTE && (!volume_initialized || last_volume != g_sys_volume))
    {
        // 使用全局变量g_sys_volume设置音量
        bsp_codec_volume_set(g_sys_volume, NULL);
        last_volume = g_sys_volume;
        volume_initialized = true;
    }

    return ret;
}

/**
 * @brief 音频数据写入函数
 * @param audio_buffer 音频数据缓冲区指针
 * @param len 要写入的数据长度
 * @param bytes_written 实际写入的字节数
 * @param timeout_ms 写入超时时间（毫秒）
 * @return esp_err_t 返回操作结果，ESP_OK表示成功
 * @note 该函数用于将音频数据通过I2S接口写入音频设备，在播放过程中会不断被调用
 * 播放一帧，进入一次，在播放的时会不断进入
 */
static esp_err_t _audio_player_write_fn(void *audio_buffer, size_t len, size_t *bytes_written, uint32_t timeout_ms)
{
    esp_err_t ret = ESP_OK;

    // 调用I2S接口写入音频数据
    ret = bsp_i2s_write(audio_buffer, len, bytes_written, timeout_ms);

    return ret;
}

/**
 * @brief 设置音频采样率函数
 * @param rate 采样率值，单位Hz
 * @param bits_cfg 位宽配置，表示音频数据的位宽
 * @param ch 声道模式，表示I2S的声道配置
 * @return esp_err_t 返回操作结果，ESP_OK表示成功
 * @note 该函数在音频播放时调用一次，用于配置音频编解码器的采样率、位宽和声道模式，据 mp3 文件信息，设置采样率，位宽，通道数。每换一首歌曲，都会调用一次
 */
static esp_err_t _audio_player_std_clock(uint32_t rate, uint32_t bits_cfg, i2s_slot_mode_t ch)
{
    esp_err_t ret = ESP_OK;

    // 调用编解码器接口设置采样率、位宽和声道模式
    ret = bsp_codec_set_output_fs(rate, bits_cfg, ch);

    return ret;
}

/**
 * @brief 音频播放器回调函数
 * @param ctx 回调上下文，包含音频事件信息
 * @note 该函数在播放器每次状态变化时被调用，用于处理不同播放事件
 */
static void _audio_player_callback(audio_player_cb_ctx_t *ctx)
{
    // 打印当前音频事件
    ESP_LOGI(TAG, "ctx->audio_event = %d", ctx->audio_event);

    // 根据音频事件类型进行不同处理
    switch (ctx->audio_event)
    {
    case AUDIO_PLAYER_CALLBACK_EVENT_IDLE: // 播放完成事件
    {
        ESP_LOGI(TAG, "AUDIO_PLAYER_REQUEST_IDLE");
        break;
    }
    case AUDIO_PLAYER_CALLBACK_EVENT_PLAYING: // 开始播放事件
        ESP_LOGI(TAG, "AUDIO_PLAYER_REQUEST_PLAY");
        gpio_set_level(PA_EN_GPIO_PIN, 1); // 开启音频功放
        break;
    case AUDIO_PLAYER_CALLBACK_EVENT_PAUSE: // 暂停播放事件
        ESP_LOGI(TAG, "AUDIO_PLAYER_REQUEST_PAUSE");
        gpio_set_level(PA_EN_GPIO_PIN, 0); // 关闭音频功放
        break;
    default: // 其他未处理事件
        break;
    }
}

static int compare_files_numeric(const void *a, const void *b)
{
    const char *file1 = *(const char **)a;
    const char *file2 = *(const char **)b;
    int num1 = atoi(file1);
    int num2 = atoi(file2);
    return num1 - num2;
}

// 自定义文件排序函数
static void sort_file_iterator_list(file_iterator_instance_t *iterator)
{
    if (iterator == NULL || iterator->list == NULL || iterator->count == 0)
    {
        return;
    }

    // 使用qsort对文件列表进行排序
    qsort(iterator->list, iterator->count, sizeof(char *), compare_files_numeric);
}
static void pa_en_init(void)
{
    gpio_config_t gpio_init_struct = {0};

    gpio_init_struct.intr_type = GPIO_INTR_DISABLE;         /* 失能引脚中断 */
    gpio_init_struct.mode = GPIO_MODE_INPUT_OUTPUT;         /* 输入输出模式 */
    gpio_init_struct.pull_up_en = GPIO_PULLUP_ENABLE;       /* 使能上拉 */
    gpio_init_struct.pull_down_en = GPIO_PULLDOWN_DISABLE;  /* 失能下拉 */
    gpio_init_struct.pin_bit_mask = 1ull << PA_EN_GPIO_PIN; /* 设置的引脚的位掩码 */
    gpio_config(&gpio_init_struct);                         /* 配置GPIO */
}

/**
 * @brief  初始化 ES8311 DAC 音频输出接口
 *
 * I2S 时钟配置原理说明：
 * =========================================================================
 * I2S（Inter-IC Sound）是一种数字音频传输协议，需要多个时钟信号：
 *
 * 1. MCLK（主时钟，Master Clock）：
 *    - 频率：通常为采样率的 256 倍或 384 倍
 *    - 作用：为编解码器提供基准时钟
 *    - 计算：MCLK = 采样率 × 256（或 384）
 *    - 示例：16kHz × 256 = 4.096MHz
 *
 * 2. BCLK（位时钟，Bit Clock / SCLK）：
 *    - 频率：采样率 × 通道数 × 位宽
 *    - 作用：同步每个音频数据位的传输
 *    - 计算：BCLK = 采样率 × 通道数 × 位宽
 *    - 示例：16kHz × 2（立体声）× 32位 = 1.024MHz
 *
 * 3. LRCK（左右声道时钟，Word Select / WS）：
 *    - 频率：等于采样率
 *    - 作用：区分左右声道数据
 *    - 计算：LRCK = 采样率
 *    - 示例：16kHz
 *
 * 时钟关系：MCLK > BCLK > LRCK
 * =========================================================================
 *
 * @return
 *     - ESP_OK: 初始化成功
 *     - ESP_FAIL: 初始化失败
 *
 * @note   该函数专门为 ES8311 DAC 设计，使用输出引脚组
 * @note   默认配置：16kHz 采样率，32 位位宽，立体声
 */
static esp_err_t bsp_audio_output_init(void)
{
    esp_err_t ret = ESP_FAIL;

    /* 检查输出数据接口是否已初始化 */
    if (i2s_tx_chan && i2s_data_if_output)
    {
        /* 输出接口已初始化 */
        return ESP_OK;
    }

    /* =====================================================================
     * 步骤 1: 配置 I2S 输出通道
     * ===================================================================== */
    /**
     * I2S 通道配置说明：
     * - BSP_I2S_NUM_OUT: 使用 I2S_NUM_1 外设
     * - I2S_ROLE_MASTER: ESP32-S3 作为主设备，生成所有时钟信号
     * - auto_clear: 自动清除 DMA 缓冲区，防止播放旧数据
     */
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(BSP_I2S_NUM_OUT, I2S_ROLE_MASTER);
    chan_cfg.auto_clear = true; /* 自动清除DMA缓冲区中的遗留数据 */
    ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, &i2s_tx_chan, NULL));

    /* =====================================================================
     * 步骤 2: 配置 I2S 标准模式参数
     * ===================================================================== */
    /**
     * I2S 标准模式配置（Philips I2S 格式）：
     *
     * 时钟配置（clk_cfg）：
     * - 采样率：16000Hz（默认值，可动态修改）
     * - MCLK 倍数：256（MCLK = 16kHz × 256 = 4.096MHz）
     * - BCLK 计算：16kHz × 2（通道）× 32（位宽）= 1.024MHz
     *
     * 时隙配置（slot_cfg）：
     * - 位宽：32 位（I2S_DATA_BIT_WIDTH_32BIT）
     * - 通道模式：立体声（I2S_SLOT_MODE_STEREO）
     * - Philips 格式：标准 I2S 协议，数据在 LRCK 变化后一个 BCLK 周期传输
     *
     * GPIO 配置（gpio_cfg）：
     * - MCLK：GPIO16（主时钟输出）
     * - BCLK：GPIO7（位时钟输出）
     * - WS/LRCK：GPIO15（左右声道时钟输出）
     * - DOUT：GPIO6（音频数据输出）
     */
    const i2s_std_config_t std_cfg_output = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(16000),                              /* 采样率16kHz */
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(32, I2S_SLOT_MODE_STEREO), /* 32位立体声 */
        .gpio_cfg = {
            .mclk = GPIO_I2S_MCLK_OUT, /* 主时钟（GPIO16） */
            .bclk = GPIO_I2S_SCLK_OUT, /* 位时钟（GPIO7） */
            .ws = GPIO_I2S_LRCK_OUT,   /* 字选择时钟（GPIO15） */
            .dout = GPIO_I2S_DOUT,     /* 数据输出引脚（GPIO6） */
            .din = GPIO_NUM_NC,        /* 输出通道不需要输入引脚 */
        },
    };

    /* 初始化I2S发送通道 */
    if (i2s_tx_chan != NULL)
    {
        ESP_GOTO_ON_ERROR(i2s_channel_init_std_mode(i2s_tx_chan, &std_cfg_output), err, "I2S", "I2S输出通道初始化失败");
        ESP_GOTO_ON_ERROR(i2s_channel_enable(i2s_tx_chan), err, "I2S", "I2S输出通道使能失败");
    }

    /* 创建I2S输出数据接口 */
    audio_codec_i2s_cfg_t i2s_cfg_output = {
        .port = BSP_I2S_NUM_OUT,
        .rx_handle = NULL, /* 输出接口不需要接收通道 */
        .tx_handle = i2s_tx_chan,
    };
    i2s_data_if_output = audio_codec_new_i2s_data(&i2s_cfg_output);
    if (i2s_data_if_output == NULL)
    {
        goto err;
    }

    return ESP_OK;

err:
    /* 错误处理：清理已分配的资源 */
    if (i2s_tx_chan)
    {
        i2s_del_channel(i2s_tx_chan);
        i2s_tx_chan = NULL;
    }
    return ret;
}

/**
 * @brief  初始化 ES8311 音频编解码器（DAC 输出模式）
 *
 * ES8311 初始化流程说明：
 * =========================================================================
 * ES8311 是一款低功耗、高性能的音频编解码器芯片，支持 DAC 和 ADC 功能。
 * 本函数配置 ES8311 工作在 DAC 输出模式，用于音频播放。
 *
 * 初始化流程：
 * 1. 初始化 I2S 数据接口（如果尚未初始化）
 * 2. 创建 GPIO 控制接口（用于功放控制等）
 * 3. 创建 I2C 控制接口（用于寄存器配置）
 * 4. 配置硬件增益参数（功放电压、DAC 电压）
 * 5. 创建 ES8311 编解码器设备
 * 6. 创建并返回编解码器设备句柄
 *
 * 硬件连接：
 * - I2C 接口：用于配置 ES8311 内部寄存器
 * - I2S 接口：用于传输音频数据流
 * - GPIO 接口：用于控制功放使能等
 *
 * @return esp_codec_dev_handle_t 编解码器设备句柄，失败时触发断言
 *
 * @note   此函数会自动初始化所有必需的接口（I2S、I2C、GPIO）
 * @note   ES8311 配置为从模式，ESP32-S3 作为 I2S 主机
 * @note   使用 MCLK（主时钟）以获得更好的音频质量
 * =========================================================================
 */
static esp_codec_dev_handle_t bsp_audio_codec_speaker_init(void)
{
    /* =====================================================================
     * 步骤 1: 初始化 I2S 数据接口
     * ===================================================================== */
    /* 检查I2S输出数据接口是否已初始化，如未初始化则调用音频输出初始化函数 */
    if (i2s_data_if_output == NULL)
    {
        /* 配置ES8311 DAC的I2S输出接口 */
        ESP_ERROR_CHECK(bsp_audio_output_init());
    }
    assert(i2s_data_if_output); /* 确保I2S输出数据接口有效 */

    /* =====================================================================
     * 步骤 2: 创建 GPIO 控制接口
     * ===================================================================== */
    /* 创建GPIO接口，用于控制编解码器的GPIO引脚（如功放使能引脚） */
    const audio_codec_gpio_if_t *gpio_if = audio_codec_new_gpio();

    /* =====================================================================
     * 步骤 3: 创建 I2C 控制接口
     * ===================================================================== */
    /**
     * I2C 控制接口用于配置 ES8311 内部寄存器，包括：
     * - 工作模式选择（DAC/ADC/CODEC）
     * - 采样率配置
     * - 音量控制
     * - 静音控制
     * - 增益设置等
     */
    /* 配置I2C控制接口参数 */
    audio_codec_i2c_cfg_t i2c_cfg = {
        .port = BSP_I2C_NUM,               /* I2C端口号（I2C_NUM_0） */
        .addr = ES8311_CODEC_DEFAULT_ADDR, /* ES8311芯片I2C地址（0x18） */
    };
    /* 创建I2C控制接口 */
    const audio_codec_ctrl_if_t *i2c_ctrl_if = audio_codec_new_i2c_ctrl(&i2c_cfg);
    assert(i2c_ctrl_if); /* 确保I2C控制接口创建成功 */

    /* =====================================================================
     * 步骤 4: 配置硬件增益参数
     * ===================================================================== */
    /**
     * 硬件增益配置说明：
     * - pa_voltage: 功放电源电压（5.0V），影响最大输出功率
     * - codec_dac_voltage: DAC 输出电压（3.3V），影响信号幅度
     *
     * 这些参数用于计算最佳增益设置，确保音频信号不失真且功率充足
     */
    /* 配置硬件增益参数 */
    esp_codec_dev_hw_gain_t gain = {
        .pa_voltage = 5.0,        /* 功放电压5.0V */
        .codec_dac_voltage = 3.3, /* 编解码器DAC电压3.3V */
    };

    /* =====================================================================
     * 步骤 5: 创建 ES8311 编解码器设备
     * ===================================================================== */
    /**
     * ES8311 配置参数说明：
     *
     * 工作模式：
     * - ESP_CODEC_DEV_WORK_MODE_DAC: 仅 DAC 输出模式（音频播放）
     * - ESP_CODEC_DEV_WORK_MODE_ADC: 仅 ADC 输入模式（音频录制）
     * - ESP_CODEC_DEV_WORK_MODE_BOTH: DAC+ADC 模式（全双工）
     *
     * 主从模式：
     * - master_mode = false: ES8311 作为从设备，ESP32-S3 提供时钟
     * - master_mode = true: ES8311 作为主设备，自己生成时钟
     *
     * MCLK（主时钟）：
     * - use_mclk = true: 使用主时钟，音质更好，支持更多采样率
     * - use_mclk = false: 不使用主时钟，功耗更低但采样率受限
     */
    /* 配置ES8311编解码器参数 */
    es8311_codec_cfg_t es8311_cfg = {
        .ctrl_if = i2c_ctrl_if,                    /* I2C控制接口 */
        .gpio_if = gpio_if,                        /* GPIO接口 */
        .codec_mode = ESP_CODEC_DEV_WORK_MODE_DAC, /* 工作模式：DAC输出 */
        .pa_pin = GPIO_PWR_CTRL,                   /* 功放控制引脚 */
        .pa_reverted = false,                      /* 功放控制逻辑不反转 */
        .master_mode = false,                      /* 从模式(ESP32为主) */
        .use_mclk = true,                          /* 使用主时钟MCLK */
        .digital_mic = false,                      /* 不使用数字麦克风 */
        .invert_mclk = false,                      /* MCLK不反转 */
        .invert_sclk = false,                      /* SCLK不反转 */
        .hw_gain = gain,                           /* 硬件增益配置 */
    };
    /* 创建ES8311编解码器设备 */
    const audio_codec_if_t *es8311_dev = es8311_codec_new(&es8311_cfg);
    assert(es8311_dev); /* 确保ES8311设备创建成功 */

    /* =====================================================================
     * 步骤 6: 创建编解码器设备句柄
     * ===================================================================== */
    /**
     * 编解码器设备句柄整合了：
     * - 编解码器接口（ES8311 驱动）
     * - 数据接口（I2S 数据传输）
     * - 设备类型（输出设备）
     *
     * 通过此句柄可以进行：
     * - 音频数据写入
     * - 音量控制
     * - 静音控制
     * - 采样率配置等操作
     */
    /* 配置编解码器设备参数 */
    esp_codec_dev_cfg_t codec_dev_cfg = {
        .dev_type = ESP_CODEC_DEV_TYPE_OUT, /* 设备类型：输出设备 */
        .codec_if = es8311_dev,             /* 编解码器接口 */
        .data_if = i2s_data_if_output,      /* I2S输出数据接口 */
    };
    /* 创建并返回编解码器设备句柄 */
    return esp_codec_dev_new(&codec_dev_cfg);
}

/* ========================= 公共函数实现区 ========================= */

esp_err_t bsp_codec_init(void)
{
    /* 初始化音频播放设备(ES8311) */
    play_dev_handle = bsp_audio_codec_speaker_init();
    assert((play_dev_handle) && "音频播放设备初始化失败");

    /* 设置默认音频参数：16kHz采样率，32位位宽，立体声 */
    bsp_codec_set_fs(CODEC_DEFAULT_SAMPLE_RATE, CODEC_DEFAULT_BIT_WIDTH, CODEC_DEFAULT_CHANNEL);

    /* 初始化功放使能引脚 */
    pa_en_init();

    return ESP_OK;
}

/**
 * @brief  写入音频数据到播放设备
 *
 * 音频数据流路径说明：
 * =========================================================================
 * 完整的音频数据流路径（从 MP3 文件到扬声器）：
 *
 * 1. MP3 文件（SPIFFS 文件系统）
 *    ↓
 * 2. MP3 解码器（audio_player 组件）
 *    - 读取 MP3 文件数据
 *    - 解码为 PCM 音频数据（原始音频采样）
 *    ↓
 * 3. 音频播放器回调函数（_audio_player_write_fn）
 *    - 调用本函数 bsp_i2s_write()
 *    ↓
 * 4. 编解码器设备层（esp_codec_dev_write）
 *    - 音量控制
 *    - 静音控制
 *    - 数据格式转换
 *    ↓
 * 5. I2S 数据接口层（audio_codec_i2s_data）
 *    - 将 PCM 数据写入 I2S DMA 缓冲区
 *    ↓
 * 6. I2S 硬件外设（ESP32-S3 I2S_NUM_1）
 *    - DMA 自动传输数据
 *    - 生成 I2S 时钟信号（MCLK、BCLK、LRCK）
 *    - 串行输出音频数据（DOUT）
 *    ↓
 * 7. ES8311 音频编解码器芯片
 *    - 接收 I2S 数字音频数据
 *    - DAC 转换为模拟音频信号
 *    ↓
 * 8. 功放（PA）
 *    - 放大模拟音频信号
 *    ↓
 * 9. 扬声器
 *    - 输出声音
 *
 * 数据格式转换：
 * - 输入：PCM 音频数据（解码后的原始音频）
 * - 格式：16/24/32 位，单声道/立体声
 * - 输出：I2S 串行数据流
 * =========================================================================
 *
 * @param[in]  audio_buffer  音频数据缓冲区指针（PCM 格式）
 * @param[in]  len           要写入的数据长度（字节）
 * @param[out] bytes_written 实际写入的字节数
 * @param[in]  timeout_ms    超时时间（毫秒，当前未使用）
 *
 * @return
 *     - ESP_OK: 写入成功
 *     - ESP_ERR_INVALID_ARG: 参数无效
 *     - ESP_ERR_INVALID_STATE: 播放设备未初始化
 *     - ESP_FAIL: 写入失败
 *
 * @note   此函数由音频播放器回调函数调用，不应直接调用
 * @note   音频数据格式需与当前设置的采样率、位宽、通道数匹配
 */
esp_err_t bsp_i2s_write(void *audio_buffer, size_t len, size_t *bytes_written, uint32_t timeout_ms)
{
    esp_err_t ret = ESP_OK;

    /* 参数有效性检查 */
    if (audio_buffer == NULL || bytes_written == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    /* 检查播放设备是否已初始化 */
    if (play_dev_handle == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }

    /* 将音频数据写入播放设备（经过编解码器设备层处理后写入 I2S） */
    ret = esp_codec_dev_write(play_dev_handle, audio_buffer, len);

    /* 设置实际写入的字节数 */
    *bytes_written = (ret == ESP_OK) ? len : 0;

    return ret;
}

esp_err_t bsp_codec_set_output_fs(uint32_t rate, uint32_t bits_cfg, i2s_slot_mode_t ch)
{
    esp_err_t ret = ESP_OK;

    /* 配置音频采样参数结构体 */
    esp_codec_dev_sample_info_t fs = {
        .sample_rate = rate,         /* 采样率 */
        .channel = ch,               /* 通道模式 */
        .bits_per_sample = bits_cfg, /* 位宽 */
    };

    /* 关闭当前播放设备以重新配置参数 */
    if (play_dev_handle)
    {
        ret = esp_codec_dev_close(play_dev_handle);
        /* 使用新参数重新打开播放设备 */
        ret |= esp_codec_dev_open(play_dev_handle, &fs);
    }
    else
    {
        ret = ESP_ERR_INVALID_STATE; /* 播放设备未初始化 */
    }

    return ret;
}

esp_err_t bsp_codec_set_fs(uint32_t rate, uint32_t bits_cfg, i2s_slot_mode_t ch)
{
    esp_err_t ret = ESP_OK;

    /* 配置输出设备 */
    ret = bsp_codec_set_output_fs(rate, bits_cfg, ch);

    return ret;
}

// 设置喇叭音量
esp_err_t bsp_codec_volume_set(int volume, int *volume_set)
{
    esp_err_t ret = ESP_OK;
    float v = volume;
    ret = esp_codec_dev_set_out_vol(play_dev_handle, (int)v);
    return ret;
}

esp_err_t bsp_codec_mute_set(bool enable)
{
    esp_err_t ret = ESP_OK;
    ret = esp_codec_dev_set_out_mute(play_dev_handle, enable);
    return ret;
}

/**
 * @brief  初始化 MP3 播放器
 *
 * 音频播放器回调函数机制说明：
 * =========================================================================
 * 音频播放器使用回调函数机制实现解耦和灵活性，主要包括三类回调：
 *
 * 1. 数据写入回调（write_fn）：
 *    - 函数：_audio_player_write_fn()
 *    - 触发时机：每次需要输出音频数据时
 *    - 作用：将解码后的 PCM 数据写入 I2S 接口
 *    - 调用频率：高频（每帧音频数据）
 *
 * 2. 静音控制回调（mute_fn）：
 *    - 函数：_audio_player_mute_fn()
 *    - 触发时机：播放开始/暂停/停止时
 *    - 作用：控制音频输出的静音状态和音量
 *    - 调用频率：低频（状态变化时）
 *
 * 3. 时钟配置回调（clk_set_fn）：
 *    - 函数：_audio_player_std_clock()
 *    - 触发时机：播放新文件时（采样率可能不同）
 *    - 作用：根据音频文件参数动态配置 I2S 时钟
 *    - 调用频率：低频（切换文件时）
 *
 * 4. 状态事件回调（callback_register）：
 *    - 函数：_audio_player_callback()
 *    - 触发时机：播放器状态变化时
 *    - 作用：处理播放事件（开始、暂停、完成等）
 *    - 调用频率：低频（状态变化时）
 *
 * 回调函数执行流程：
 * ┌─────────────────────────────────────────────────────────────┐
 * │ 音频播放器（audio_player）                                   │
 * │                                                             │
 * │  [MP3 解码] → [PCM 数据] → write_fn() → I2S 输出           │
 * │       ↓                                                     │
 * │  [状态变化] → callback() → 功放控制                         │
 * │       ↓                                                     │
 * │  [新文件] → clk_set_fn() → 重新配置采样率                   │
 * │       ↓                                                     │
 * │  [播放/暂停] → mute_fn() → 静音控制                         │
 * └─────────────────────────────────────────────────────────────┘
 *
 * 优势：
 * - 解耦：播放器不依赖具体的硬件实现
 * - 灵活：可以轻松更换不同的音频输出设备
 * - 可测试：可以用模拟回调函数进行单元测试
 * =========================================================================
 *
 * @note   此函数会创建音频播放器任务（优先级 6，运行在核心 1）
 * @note   播放器任务会自动管理 MP3 解码和音频输出
 */
void mp3_player_init(void)
{
    // 初始化文件迭代器，用于遍历指定挂载点下的音乐文件
    file_iterator = file_iterator_new("/spiffs");
    assert(file_iterator != NULL); // 确保文件迭代器初始化成功

    // 对文件列表进行数字排序
    sort_file_iterator_list(file_iterator);

    // 配置音频播放器参数
    player_config.mute_fn = _audio_player_mute_fn;      // 设置静音回调函数
    player_config.write_fn = _audio_player_write_fn;    // 设置音频数据写入回调函数
    player_config.clk_set_fn = _audio_player_std_clock; // 设置采样率配置回调函数
    player_config.priority = 6;                         // 设置任务优先级
    player_config.coreID = 1;                           // 设置运行核心

    // 创建音频播放器实例，并检查是否成功
    ESP_ERROR_CHECK(audio_player_new(player_config));

    // 注册音频播放状态回调函数，用于处理播放事件
    ESP_ERROR_CHECK(audio_player_callback_register(_audio_player_callback, NULL));

    // 初始化互斥锁
    if (flash_mutex == NULL)
        flash_mutex = xSemaphoreCreateMutex();
}

void safe_play_index(int index)
{
    // 记录日志，显示当前播放的索引号
    ESP_LOGI(TAG, "safe_play_index(%d)", index);

    // 定义文件路径缓冲区和文件指针
    char filename[128];
    FILE *fp = NULL;

    // 尝试获取互斥锁进行文件操作，等待时间500毫秒
    if (xSemaphoreTake(flash_mutex, pdMS_TO_TICKS(500)) == pdTRUE)
    {
        // 通过索引号获取完整文件路径
        int retval = file_iterator_get_full_path_from_index(file_iterator, index, filename, sizeof(filename));
        if (retval != 0)
        {
            // 以二进制只读模式打开文件
            fp = fopen(filename, "rb");
            if (fp)
            {
                ESP_LOGI(TAG, "Playing '%s'", filename);
            }
            else
            {
                ESP_LOGE(TAG, "unable to open index %d, filename '%s'", index, filename);
            }
        }
        else
        {
            ESP_LOGE(TAG, "unable to retrieve filename for index %d", index);
        }

        // 立即释放互斥锁，不在播放过程中持有
        xSemaphoreGive(flash_mutex);

        // 如果文件打开成功，开始播放（此时已释放锁）
        if (fp)
        {
            audio_player_play(fp);
        }
    }
    else
    {
        // 获取互斥锁失败，记录错误日志
        ESP_LOGE(TAG, "Failed to get flash mutex for playback");
    }
}
