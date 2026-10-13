/**
 * @file motor.h
 * @brief TB6612FNG 双电机驱动与编码器控制模块
 *
 * 本模块实现基于 TB6612FNG 双 H 桥驱动芯片的电机控制系统，
 * 包括 PWM 速度控制、方向控制、PCNT 编码器读取和 RPM 计算等功能。
 *
 * 硬件配置：
 * - 电机驱动芯片：TB6612FNG（双 H 桥，最大 1.2A 连续电流）
 * - 电机型号：N20 直流减速电机（标称 34 RPM）
 * - 编码器：霍尔编码器（7 PPR 基础脉冲，2654.1 线/转输出）
 * - PWM 控制：MCPWM 外设（1kHz 频率，0-100% 占空比）
 * - 编码器接口：PCNT 外设（4 倍频正交解码）
 *
 * 电机接口：
 * - 电机 A（左轮）：PWMA=GPIO9, AIN1=GPIO11, AIN2=GPIO10
 * - 电机 B（右轮）：PWMB=GPIO14, BIN1=GPIO12, BIN2=GPIO13
 *
 * 编码器接口：
 * - 电机 A 编码器：A 相=GPIO3, B 相=GPIO46
 * - 电机 B 编码器：A 相=GPIO21, B 相=GPIO47
 *
 * @author ZHC
 * @date 2025
 * @version 1.0
 */

#ifndef __MOTOR_H__
#define __MOTOR_H__

#include "esp_err.h"
#include <stdio.h>
#include "esp_err.h"
#include "driver/i2c.h"
#include "driver/mcpwm_prelude.h"
#include "driver/pulse_cnt.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "math.h"
#include "string.h"

/* ========== TB6612FNG 电机驱动引脚定义 ========== */

// 电机 A（左轮）控制引脚
#define PWMA 9  ///< 电机 A 的 PWM 速度控制引脚（GPIO9）
#define AIN1 11 ///< 电机 A 的方向控制引脚 1（GPIO11）
#define AIN2 10 ///< 电机 A 的方向控制引脚 2（GPIO10）

// 电机 B（右轮）控制引脚
#define PWMB 14 ///< 电机 B 的 PWM 速度控制引脚（GPIO14）
#define BIN1 12 ///< 电机 B 的方向控制引脚 1（GPIO12）
#define BIN2 13 ///< 电机 B 的方向控制引脚 2（GPIO13）

/* ========== 编码器引脚定义 ========== */

#define ENCODER_A_PHASE_A_PIN 3  ///< 电机 A 编码器 A 相引脚（GPIO3）
#define ENCODER_A_PHASE_B_PIN 46 ///< 电机 A 编码器 B 相引脚（GPIO46）
#define ENCODER_B_PHASE_A_PIN 21 ///< 电机 B 编码器 A 相引脚（GPIO21）
#define ENCODER_B_PHASE_B_PIN 47 ///< 电机 B 编码器 B 相引脚（GPIO47）

/* ========== 编码器参数定义 ========== */

/**
 * N20 电机编码器官方参数：
 * - 电机轴基础脉冲：7 PPR（每转 7 个脉冲）
 * - 减速比：约 379:1（2654.1 / 7 = 379.157）
 * - 输出轴编码线数：2654.1 线/转（官方标准值）
 * - 4 倍频模式下：每转 10616.4 个计数（2654.1 × 4）
 */
#define ENCODER_PPR 7              ///< 编码器每转脉冲数（电机轴基础脉冲，7 PPR）
#define ENCODER_OUTPUT_PPR 2654.1f ///< 减速后输出轴编码线数（官方标准：2654.1 线/转）
#define ENCODER_SAMPLE_TIME_MS 1   ///< 编码器采样时间间隔（毫秒，用于 RPM 计算防抖）

/* ========== PCNT 计数器限制值定义 ========== */

/**
 * PCNT 硬件计数器范围：-32768 ~ +32767（16 位有符号整数）
 * 当计数值达到边界时触发溢出中断，软件累积溢出次数以扩展计数范围
 */
#define PCNT_UNIT_MAX_COUNT 32767  ///< PCNT 单元最大计数值（正向溢出阈值）
#define PCNT_UNIT_MIN_COUNT -32768 ///< PCNT 单元最小计数值（反向溢出阈值）

/* ========== 枚举类型定义 ========== */

/**
 * @brief 电机标识枚举
 *
 * 用于指定操作的目标电机
 */
typedef enum
{
    MOTOR_A = 0,   ///< 电机 A（左轮，左侧接口）
    MOTOR_B = 1,   ///< 电机 B（右轮，下方接口）
    MOTOR_BOTH = 2 ///< 双电机同时控制
} motor_id_t;

/**
 * @brief 电机方向枚举
 *
 * 定义电机的旋转方向，通过 AIN1/AIN2 或 BIN1/BIN2 引脚组合控制
 */
typedef enum
{
    MOTOR_DIRECTION_FORWARD = 0, ///< 正转（前进方向）
    MOTOR_DIRECTION_REVERSE = 1  ///< 反转（后退方向）
} motor_direction_t;

/**
 * @brief 电机停止方式枚举
 *
 * TB6612FNG 支持两种停止模式：
 * - 滑行停止：方向引脚全为低电平，电机自由滑行（快速衰减）
 * - 刹车停止：方向引脚全为高电平，电机短路制动（慢速衰减）
 */
typedef enum
{
    MOTOR_STOP_COAST = 0, ///< 滑行停止（快速衰减，惯性滑行）
    MOTOR_STOP_BRAKE = 1  ///< 刹车停止（慢速衰减，短路制动）
} motor_stop_mode_t;

/**
 * @brief 编码器倍频模式枚举
 *
 * 正交编码器支持不同的倍频模式：
 * - 1 倍频：仅 A 相上升沿计数
 * - 2 倍频：A 相上升沿和下降沿计数
 * - 4 倍频：A/B 相所有边沿计数（本模块默认使用）
 */
typedef enum
{
    ENCODER_MODE_X1 = 0, ///< 1 倍频模式（最低分辨率）
    ENCODER_MODE_X2 = 1, ///< 2 倍频模式（中等分辨率）
    ENCODER_MODE_X4 = 2  ///< 4 倍频模式（最高分辨率，默认）
} encoder_mode_t;

/* ========== 数据结构定义 ========== */

/**
 * @brief 编码器状态结构体
 *
 * 存储单个编码器的运行状态和配置信息
 */
typedef struct
{
    pcnt_unit_handle_t pcnt_unit; ///< PCNT 脉冲计数器单元句柄
    int32_t pulse_count;          ///< 累积脉冲计数值（包含溢出累积）
    float speed_rpm;              ///< 输出轴转速（转/分钟）
    encoder_mode_t mode;          ///< 编码器倍频模式（默认 X4）
    bool is_initialized;          ///< 初始化状态标志
} encoder_t;

/**
 * @brief TB6612FNG 电机驱动器控制结构体
 *
 * 包含 TB6612FNG 双 H 桥驱动芯片的所有控制资源和状态信息。
 *
 * TB6612FNG 方向控制真值表：
 * | IN1 | IN2 | 电机状态        |
 * |-----|-----|----------------|
 * |  0  |  0  | 滑行停止（快衰）|
 * |  0  |  1  | 反转（CCW）     |
 * |  1  |  0  | 正转（CW）      |
 * |  1  |  1  | 刹车停止（慢衰）|
 */
typedef struct
{
    // GPIO 引脚配置
    gpio_num_t ain1_pin;                 ///< 电机 A 方向控制引脚 1（GPIO11）
    gpio_num_t ain2_pin;                 ///< 电机 A 方向控制引脚 2（GPIO10）
    gpio_num_t pwma_pin;                 ///< 电机 A 速度控制 PWM 引脚（GPIO9）
    gpio_num_t bin1_pin;                 ///< 电机 B 方向控制引脚 1（GPIO12）
    gpio_num_t bin2_pin;                 ///< 电机 B 方向控制引脚 2（GPIO13）
    gpio_num_t pwmb_pin;                 ///< 电机 B 速度控制 PWM 引脚（GPIO14）

    // MCPWM 外设资源句柄
    mcpwm_timer_handle_t timer_handle;   ///< MCPWM 定时器句柄（1kHz 时基）
    mcpwm_oper_handle_t operator_handle; ///< MCPWM 操作器句柄（连接定时器和生成器）
    mcpwm_cmpr_handle_t cmpr_a_handle;   ///< 电机 A 比较器句柄（占空比控制）
    mcpwm_cmpr_handle_t cmpr_b_handle;   ///< 电机 B 比较器句柄（占空比控制）
    mcpwm_gen_handle_t gen_a_handle;     ///< 电机 A 生成器句柄（PWM 波形输出）
    mcpwm_gen_handle_t gen_b_handle;     ///< 电机 B 生成器句柄（PWM 波形输出）

    // 状态标志
    bool is_initialized;                 ///< 驱动器初始化状态标志
    bool motor_a_enabled;                ///< 电机 A 使能状态标志
    bool motor_b_enabled;                ///< 电机 B 使能状态标志
} tb6612fng_motor_t;

/**
 * @brief 双电机系统控制结构体
 *
 * 整合 TB6612FNG 驱动器和双编码器，提供完整的电机控制和反馈系统
 */
typedef struct
{
    tb6612fng_motor_t tb6612fng; ///< TB6612FNG 电机驱动器实例
    encoder_t encoder_a;         ///< 电机 A 编码器实例
    encoder_t encoder_b;         ///< 电机 B 编码器实例
    bool is_initialized;         ///< 系统初始化状态标志
} dual_motor_t;

/**
 * @brief 车体运动状态结构体
 *
 * 存储双电机的实时运动状态数据，用于里程计和运动控制
 */
typedef struct
{
    float motor_a_rpm;     ///< 电机 A 输出轴实际转速（转/分钟，正值=正转，负值=反转）
    float motor_b_rpm;     ///< 电机 B 输出轴实际转速（转/分钟，正值=正转，负值=反转）
    uint32_t timestamp_ms; ///< 数据采集时间戳（毫秒）
    bool data_valid;       ///< 数据有效性标志（true=数据可用，false=数据无效）
} vehicle_motion_state_t;

/* ========== 公共 API 函数声明 ========== */

/**
 * @brief  初始化双电机系统
 *
 * 初始化 TB6612FNG 驱动器和双编码器，包括：
 * - 配置 GPIO 方向控制引脚
 * - 配置 MCPWM 外设（1kHz PWM 频率）
 * - 配置 PCNT 编码器（4 倍频正交解码）
 * - 设置毛刺滤波器（1us）
 * - 注册溢出中断回调
 *
 * @retval ESP_OK              初始化成功
 * @retval ESP_ERR_INVALID_ARG 参数错误
 * @retval 其他                硬件初始化失败
 *
 * @note   此函数必须在使用其他电机 API 之前调用
 * @note   初始化后电机处于禁用状态，需调用 motor_enable() 激活
 */
esp_err_t motor_init(void);

/**
 * @brief  控制双电机停止
 *
 * 根据指定的停止模式停止电机运动：
 * - 滑行停止：方向引脚全为低电平，电机自由滑行（快速衰减）
 * - 刹车停止：方向引脚全为高电平，电机短路制动（慢速衰减）
 *
 * @param  motor_a_stop_mode 电机 A 停止方式（MOTOR_STOP_COAST 或 MOTOR_STOP_BRAKE）
 * @param  motor_b_stop_mode 电机 B 停止方式（MOTOR_STOP_COAST 或 MOTOR_STOP_BRAKE）
 *
 * @retval ESP_OK                 停止成功
 * @retval ESP_ERR_INVALID_STATE  电机系统未初始化
 *
 * @note   停止后 PWM 占空比设置为 0
 */
esp_err_t motor_stop(motor_stop_mode_t motor_a_stop_mode, motor_stop_mode_t motor_b_stop_mode);

/**
 * @brief  激活双电机使能状态
 *
 * 使能双电机，允许电机接收速度和方向控制命令
 *
 * @retval ESP_OK                 使能成功
 * @retval ESP_ERR_INVALID_STATE  电机系统未初始化
 *
 * @note   初始化后必须调用此函数才能控制电机运动
 */
esp_err_t motor_enable(void);

/**
 * @brief  解除双电机使能状态（急停功能）
 *
 * 禁用双电机并执行滑行停止，用于紧急停止场景
 *
 * @retval ESP_OK                 禁用成功
 * @retval ESP_ERR_INVALID_STATE  电机系统未初始化
 *
 * @note   禁用后需重新调用 motor_enable() 才能恢复控制
 */
esp_err_t motor_disable(void);

/**
 * @brief  设置指定电机的转速和方向
 *
 * 通过设置 PWM 占空比和方向引脚电平控制电机运动
 *
 * @param  motor_id  目标电机标识（MOTOR_A、MOTOR_B 或 MOTOR_BOTH）
 * @param  speed     速度值（0-100，对应 0%-100% PWM 占空比）
 * @param  direction 运动方向（MOTOR_DIRECTION_FORWARD 或 MOTOR_DIRECTION_REVERSE）
 *
 * @retval ESP_OK                 设置成功
 * @retval ESP_ERR_INVALID_STATE  电机系统未初始化或电机未使能
 * @retval ESP_ERR_INVALID_ARG    速度值超出范围（>100）或电机 ID 无效
 *
 * @note   速度值 0 表示停止，100 表示最大速度
 * @note   实际转速取决于电机负载和电源电压
 */
esp_err_t motor_set_speed_and_dir(motor_id_t motor_id, uint32_t speed, motor_direction_t direction);

/**
 * @brief  获取电机的实际转速
 *
 * 通过编码器计数差分计算电机输出轴的实际转速（RPM）
 *
 * 计算公式：
 * 1. 电机轴转速(RPS) = (计数差 / 时间差) / (基础PPR × 倍频系数)
 * 2. 输出轴转速(RPS) = 电机轴转速(RPS) / 减速比
 * 3. 输出轴转速(RPM) = 输出轴转速(RPS) × 60
 *
 * @param  motor_id 目标电机标识（MOTOR_A 或 MOTOR_B）
 * @param  rpm      输出转速值指针（转/分钟，正值=正转，负值=反转）
 *
 * @retval ESP_OK                 获取成功
 * @retval ESP_ERR_INVALID_STATE  编码器未初始化
 * @retval ESP_ERR_INVALID_ARG    参数为空或电机 ID 无效
 *
 * @note   首次调用返回 0，需要至少两次调用才能计算速度
 * @note   采样间隔小于 ENCODER_SAMPLE_TIME_MS 时返回上次计算值
 */
esp_err_t motor_get_rpm(motor_id_t motor_id, float *rpm);

#endif