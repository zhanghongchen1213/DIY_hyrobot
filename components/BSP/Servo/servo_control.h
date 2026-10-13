/**
 * @file servo_control.h
 * @brief 三舵机统一角度控制系统
 *
 * 本模块实现基于 LEDC PWM 的舵机控制系统，采用统一角度系统设计，
 * 简化多舵机协调控制，支持三个舵机的独立控制和零位校准。
 *
 * 硬件配置：
 * - 舵机 A（右手）：GPIO8，LEDC_CHANNEL_0
 * - 舵机 B（左手）：GPIO19，LEDC_CHANNEL_1
 * - 舵机 C（腰部）：GPIO20，LEDC_CHANNEL_2
 * - PWM 频率：50Hz（标准舵机频率）
 * - 脉宽范围：500-2500μs（对应 0-180°）
 *
 * 统一角度系统设计：
 * - 统一角度范围：0-60°（所有舵机使用相同的逻辑角度）
 * - 零位定义：统一角度 0° 对应机器人的标准零位姿态
 * - 角度映射：每个舵机根据安装方向和零位偏移进行独立映射
 *
 * @author ZHC
 * @date 2025
 * @version 1.0
 */

#ifndef __SERVO_CONTROL_H__
#define __SERVO_CONTROL_H__

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "iot_servo.h"

/* ========== GPIO 引脚定义 ========== */

#define SERVO_GPIO_A 8  ///< 右手舵机控制引脚（GPIO8）
#define SERVO_GPIO_B 19 ///< 左手舵机控制引脚（GPIO19）
#define SERVO_GPIO_C 20 ///< 腰部舵机控制引脚（GPIO20）

/* ========== LEDC 通道定义 ========== */

#define SERVO_LEDC_A LEDC_CHANNEL_0 ///< 舵机 A 的 LEDC 通道
#define SERVO_LEDC_B LEDC_CHANNEL_1 ///< 舵机 B 的 LEDC 通道
#define SERVO_LEDC_C LEDC_CHANNEL_2 ///< 舵机 C 的 LEDC 通道

/* ========== 统一角度系统定义 ========== */

/**
 * 统一角度系统说明：
 *
 * 为了简化多舵机协调控制，本系统采用统一角度系统设计。
 * 所有舵机使用相同的逻辑角度范围（0-60°），内部自动映射到各舵机的实际角度。
 *
 * 优势：
 * - 简化上层控制逻辑，无需关心各舵机的安装方向和零位差异
 * - 便于实现对称动作（如左右手同步运动）
 * - 统一的角度范围便于参数调试和动作设计
 */
#define SERVO_UNIFIED_ZERO_ANGLE 0  ///< 统一角度系统的零位角度（机器人标准姿态）
#define SERVO_UNIFIED_MAX_ANGLE 60  ///< 统一角度系统的最大角度（运动范围限制）

/* ========== 舵机速度控制参数 ========== */

#define SERVO_SPEED_SLOW_STEP    1    ///< 缓慢模式：每步移动1度
#define SERVO_SPEED_SLOW_DELAY   30   ///< 缓慢模式：每步延时30ms
#define SERVO_SPEED_NORMAL_STEP  2    ///< 正常模式：每步移动2度
#define SERVO_SPEED_NORMAL_DELAY 15   ///< 正常模式：每步延时15ms

/* ========== 各舵机零位实际角度修正量 ========== */

/**
 * 零位修正说明：
 *
 * 由于舵机安装方向和机械结构差异，各舵机在统一角度 0° 时的实际角度不同：
 *
 * - SERVO_A（右手）：正向安装，零位 = 0°
 * - SERVO_B（左手）：反向安装，零位 = 180°（镜像对称）
 * - SERVO_C（腰部）：带偏移安装，零位 = 30°（避免机械干涉）
 */
#define SERVO_A_ZERO_POSITION 0   ///< 舵机 A 零位时的实际角度（正向安装）
#define SERVO_B_ZERO_POSITION 180 ///< 舵机 B 零位时的实际角度（反向安装，镜像对称）
#define SERVO_C_ZERO_POSITION 30  ///< 舵机 C 零位时的实际角度（带偏移，避免机械干涉）

/* ========== 枚举类型定义 ========== */

/**
 * @brief 舵机标识枚举
 *
 * 用于指定操作的目标舵机
 */
typedef enum
{
    SERVO_A = 0, ///< 舵机 A（右手）
    SERVO_B,     ///< 舵机 B（左手）
    SERVO_C,     ///< 舵机 C（腰部）
} servo_id_t;

/**
 * @brief 舵机速度等级枚举
 *
 * 控制舵机从当前位置运动到目标位置的速度
 */
typedef enum
{
    SERVO_SPEED_SLOW = 0, ///< 缓慢：平稳运动，适合精细动作
    SERVO_SPEED_NORMAL,   ///< 正常：中等速度
    SERVO_SPEED_FAST,     ///< 快速：直接跳转到目标位置（默认）
} servo_speed_t;

/* ========== 数据结构定义 ========== */

/**
 * @brief 舵机角度状态结构体
 *
 * 存储三个舵机的当前统一角度值
 */
typedef struct
{
    int16_t angle_a; ///< 舵机 A 的统一角度（0-60°）
    int16_t angle_b; ///< 舵机 B 的统一角度（0-60°）
    int16_t angle_c; ///< 舵机 C 的统一角度（0-60°）
} servo_angles_t;

/* ========== 公共 API 函数声明 ========== */

/**
 * @brief  初始化舵机控制系统
 *
 * 配置 LEDC PWM 外设，初始化三个舵机通道，并将所有舵机复位到零位。
 *
 * 初始化参数：
 * - PWM 频率：50Hz（标准舵机频率）
 * - 脉宽范围：500-2500μs（对应 0-180°）
 * - 最大角度：180°
 * - 定时器：LEDC_TIMER_0
 * - 速度模式：LEDC_LOW_SPEED_MODE
 *
 * @note   使用舵机功能前必须先调用此函数
 * @note   初始化后所有舵机自动复位到统一角度 0°
 */
void servo_init(void);

/**
 * @brief  复位所有舵机到零位
 *
 * 将所有舵机设置为统一角度 0°（机器人标准姿态）。
 *
 * @note   零位姿态由各舵机的零位修正量决定
 */
void servo_reset(void);

/**
 * @brief  设置指定舵机的角度
 *
 * 使用统一角度系统设置舵机角度，内部自动映射到实际角度。
 * 支持三种速度模式控制舵机运动过程。
 *
 * @param  id     目标舵机标识（SERVO_A、SERVO_B 或 SERVO_C）
 * @param  angle  统一角度值（0-60°）
 * @param  speed  运动速度等级（SERVO_SPEED_SLOW、SERVO_SPEED_NORMAL 或 SERVO_SPEED_FAST）
 *
 * @note   超出范围的角度值将被忽略并输出警告日志
 * @note   SERVO_SPEED_FAST 模式下直接跳转到目标角度
 * @note   SERVO_SPEED_SLOW/NORMAL 模式下渐进运动到目标角度
 */
void servo_set_angle(servo_id_t id, int16_t angle, servo_speed_t speed);

/**
 * @brief  获取所有舵机的当前角度
 *
 * 返回三个舵机的当前统一角度值。
 *
 * @return servo_angles_t 包含三个舵机角度的结构体
 *
 * @note   返回的是统一角度值，不是实际角度值
 */
servo_angles_t servo_get_actual_angles(void);

#endif
