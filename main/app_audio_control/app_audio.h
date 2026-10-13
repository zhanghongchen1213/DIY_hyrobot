/**
 * @file app_audio.h
 * @brief 音频控制应用模块
 *
 * 本模块提供音频播放控制功能，包括 MP3 音频播放、
 * HTTP 音频流处理和 RTOS 任务管理。
 *
 * @author ZHC
 * @date 2025
 * @version 1.0
 *
 * @note 使用前需要先初始化音频编解码器
 */

#ifndef __APP_AUDIO_H__
#define __APP_AUDIO_H__

/* ==================================================================================
 *                                    Includes
 * ================================================================================== */
#include <fcntl.h>
#include "all_include.h"
#include "esp_vfs.h"
#include "esp_http_client.h"
#include "freertos/ringbuf.h"
#include "audio_player.h"

/* ==================================================================================
 *                               Function Prototypes
 * ================================================================================== */

/**
 * @brief  初始化音频控制模块
 *
 * 初始化音频控制相关的 RTOS 任务和资源，包括：
 * - 创建音频播放任务
 * - 初始化音频缓冲区
 * - 配置音频播放参数
 *
 * @return void
 *
 * @note   必须在使用音频功能前调用此函数
 * @note   需要确保音频编解码器已初始化
 */
void rtos_audio_control_init(void);

#endif /* __APP_AUDIO_H__ */
