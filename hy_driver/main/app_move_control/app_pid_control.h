/**
 * @file app_pid_control.h
 * @brief PID 闭环速度控制模块
 *
 * 本模块实现双电机的 PID 闭环速度控制，包括：
 * - PID 参数配置（Kp、Ki、Kd）
 * - 目标速度设置
 * - 实时速度反馈
 * - 输出限幅和积分抗饱和
 *
 * PID 控制特性：
 * - 控制周期：可配置（通常 10-50ms）
 * - 输出范围：0-100%（PWM 占空比）
 * - 支持死区控制
 * - 支持积分限幅
 *
 * @author ZHC
 * @date 2025
 * @version 1.0
 *
 * @note 使用前需要先初始化电机和编码器
 */

#ifndef __APP_PID_CONTROL_H__
#define __APP_PID_CONTROL_H__

/* ==================================================================================
 *                                    Includes
 * ================================================================================== */
#include "all_include.h"

/* ==================================================================================
 *                                   Data Types
 * ================================================================================== */

/**
 * @brief PID 控制参数结构体
 *
 * 存储 PID 控制器的增益参数和限幅参数
 */
typedef struct
{
    float kp;           ///< 比例系数（Proportional Gain）
    float ki;           ///< 积分系数（Integral Gain）
    float kd;           ///< 微分系数（Derivative Gain）
    float output_max;   ///< 最大输出限幅（0-100%）
    float integral_max; ///< 积分限幅，防止积分饱和
    float dead_zone;    ///< 死区，代表误差允许的最小范围（RPM）
} pid_params_t;

/**
 * @brief PID 运行时状态结构体
 *
 * 存储 PID 控制器的实时状态变量
 */
typedef struct
{
    float target_speed; ///< 目标速度（RPM，额定转速 34RPM）
    float actual_speed; ///< 实际测量速度（过滤后，单位：RPM）
    float error_last;   ///< 上次误差（RPM）
    float integral_sum; ///< 积分累计（RPM）
    float output;       ///< PID 计算输出（PWM 占空比 0-100%）
} pid_state_t;

/**
 * @brief PID 控制块结构体
 *
 * 整合单个电机的 PID 参数和状态
 */
typedef struct
{
    motor_id_t motor_id; ///< 电机 ID（MOTOR_A 或 MOTOR_B）
    pid_params_t params; ///< PID 参数（KP/KI/KD，输出限幅，积分限幅，死区）
    pid_state_t state;   ///< PID 状态（目标速度，实际速度，上次误差，积分累计，输出）
} pid_ctrl_block_t;

/* ==================================================================================
 *                               Function Prototypes
 * ================================================================================== */

/**
 * @brief  初始化 PID 控制系统
 *
 * 初始化 PID 控制相关的任务和资源，包括：
 * - 创建 PID 控制定时器
 * - 初始化 PID 参数
 * - 创建速度控制任务
 *
 * @return void
 *
 * @note   必须在电机和编码器初始化后调用
 * @note   创建定时器和控制任务
 */
void app_pid_init(void);

/**
 * @brief  设置目标速度
 *
 * 设置指定电机的目标转速（RPM）。
 *
 * @param  motor_id  电机 ID（MOTOR_A、MOTOR_B 或 MOTOR_BOTH）
 * @param  speed     目标速度（RPM）
 *
 * @return void
 *
 * @note   速度为正值表示正转，负值表示反转
 * @note   速度为 0 时电机停止
 */
void app_pid_set_speed(motor_id_t motor_id, float speed);

/**
 * @brief  更新 PID 参数
 *
 * 动态更新指定电机的 PID 控制参数。
 *
 * @param  motor_id  电机 ID（MOTOR_A 或 MOTOR_B）
 * @param  kp        比例系数
 * @param  ki        积分系数
 * @param  kd        微分系数
 *
 * @return void
 *
 * @note   参数更新后立即生效
 * @note   建议在电机静止时调整参数
 */
void app_pid_set_params(motor_id_t motor_id, float kp, float ki, float kd);

/**
 * @brief  获取 PID 运行状态
 *
 * 返回指定电机的 PID 控制块，包含参数和实时状态。
 *
 * @param  motor_id  电机 ID（MOTOR_A 或 MOTOR_B）
 *
 * @return const pid_ctrl_block_t* 指向控制块的常量指针
 *
 * @note   返回的指针指向内部数据，不应修改
 * @note   用于调试和监控 PID 运行状态
 */
const pid_ctrl_block_t *app_pid_get_status(motor_id_t motor_id);

#endif /* __APP_PID_CONTROL_H__ */
