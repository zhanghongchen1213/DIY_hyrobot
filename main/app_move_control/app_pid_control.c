/**
 * @file app_pid_control.c
 * @brief 双电机速度环PID控制系统实现
 * @author ZHC
 * @date 2025
 * @version 1.0
 */

#include "app_pid_control.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "math.h"
#include "motor.h"

static const char *TAG = "pid_control";

// 默认PID参数
#define DEFAULT_KP 8.5f            ///< 默认PID比例系数
#define DEFAULT_KI 1.0f            ///< 默认PID积分系数
#define DEFAULT_KD 0.5f            ///< 默认PID微分系数
#define DEFAULT_OUTPUT_MAX 100.0f  ///< 默认输出最大限幅
#define DEFAULT_INTEGRAL_MAX 85.0f ///< 默认积分最大限幅
#define DEFAULT_DEAD_ZONE 1.5f     ///< 默认死区
#define FILTER_COEFF 0.6f          ///< 低通滤波系数
#define MAX_SPEED 35.0f            ///< 最大速度 (RPM)，等价最大线速度为 0.11m/s，最大角速度为 1.76rad/s

// 任务句柄
static TaskHandle_t s_pid_task_handle = NULL;

// PID控制器实例
static pid_ctrl_block_t s_pid_ctrls[2];

// 内部函数声明
static void pid_control_task(void *arg);
static void pid_calculate(pid_ctrl_block_t *ctrl);

/**
 * @brief 初始化PID控制系统
 */
void app_pid_init(void)
{
    // 初始化控制器数据
    for (int i = 0; i < 2; i++)
    {
        s_pid_ctrls[i].motor_id = (i == 0) ? MOTOR_A : MOTOR_B;
        s_pid_ctrls[i].params.kp = DEFAULT_KP;
        s_pid_ctrls[i].params.ki = DEFAULT_KI;
        s_pid_ctrls[i].params.kd = DEFAULT_KD;
        s_pid_ctrls[i].params.output_max = DEFAULT_OUTPUT_MAX;
        s_pid_ctrls[i].params.integral_max = DEFAULT_INTEGRAL_MAX;
        s_pid_ctrls[i].params.dead_zone = DEFAULT_DEAD_ZONE;

        s_pid_ctrls[i].state.target_speed = 0.0f;
        s_pid_ctrls[i].state.actual_speed = 0.0f;
        s_pid_ctrls[i].state.error_last = 0.0f;
        s_pid_ctrls[i].state.integral_sum = 0.0f;
        s_pid_ctrls[i].state.output = 0.0f;
    }

    // 使能电机
    motor_enable();

    // 创建控制任务 (高优先级)
    xTaskCreatePinnedToCore(pid_control_task, "pid_task", 4096, NULL, configMAX_PRIORITIES - 2, &s_pid_task_handle, 1);
}

/**
 * @brief 设置目标速度
 */
void app_pid_set_speed(motor_id_t motor_id, float speed)
{
    /*限制速度范围在[-MAX_SPEED, MAX_SPEED]*/
    float local_speed = speed;
    if (local_speed > MAX_SPEED)
        local_speed = MAX_SPEED;
    if (local_speed < -MAX_SPEED)
        local_speed = -MAX_SPEED;

    if (motor_id == MOTOR_A || motor_id == MOTOR_BOTH)
    {
        s_pid_ctrls[0].state.target_speed = local_speed;
    }
    if (motor_id == MOTOR_B || motor_id == MOTOR_BOTH)
    {
        s_pid_ctrls[1].state.target_speed = local_speed;
    }
}

/**
 * @brief 更新PID参数
 */
void app_pid_set_params(motor_id_t motor_id, float kp, float ki, float kd)
{
    if (motor_id == MOTOR_A || motor_id == MOTOR_BOTH)
    {
        s_pid_ctrls[0].params.kp = kp;
        s_pid_ctrls[0].params.ki = ki;
        s_pid_ctrls[0].params.kd = kd;
    }
    if (motor_id == MOTOR_B || motor_id == MOTOR_BOTH)
    {
        s_pid_ctrls[1].params.kp = kp;
        s_pid_ctrls[1].params.ki = ki;
        s_pid_ctrls[1].params.kd = kd;
    }
}

/**
 * @brief 获取PID运行状态
 */
const pid_ctrl_block_t *app_pid_get_status(motor_id_t motor_id)
{
    if (motor_id == MOTOR_A)
        return &s_pid_ctrls[0];
    else if (motor_id == MOTOR_B)
        return &s_pid_ctrls[1];
    return NULL;
}

// ---------------- 内部实现 ----------------

/**
 * @brief PID控制任务主体
 */
static void pid_control_task(void *arg)
{
    while (1)
    {
        // 依次处理两个电机
        for (int i = 0; i < 2; i++)
        {
            pid_ctrl_block_t *ctrl = &s_pid_ctrls[i];

            // 1. 获取实际速度 (motor_get_rpm最小采样间隔为1ms)
            float raw_rpm = 0.0f;
            motor_get_rpm(ctrl->motor_id, &raw_rpm);

            // 2. 获取实际测量速度, 并应用低通滤波
            ctrl->state.actual_speed = ctrl->state.actual_speed * (1.0f - FILTER_COEFF) + raw_rpm * FILTER_COEFF;

            // 4. PID计算
            pid_calculate(ctrl);

            // 5. 输出控制
            // 注意：PID输出已被限幅在 params.output_max (默认100) 范围内
            // 这里将 PID输出(Control Value) 解释为 PWM占空比
            float pid_output_val = ctrl->state.output;
            motor_direction_t dir = MOTOR_DIRECTION_FORWARD;
            uint32_t speed_pwm_duty = 0;

            if (pid_output_val >= 0)
            {
                dir = MOTOR_DIRECTION_FORWARD;
                speed_pwm_duty = (uint32_t)pid_output_val;
            }
            else
            {
                dir = MOTOR_DIRECTION_REVERSE;
                speed_pwm_duty = (uint32_t)(-pid_output_val);
            }
            // 执行电机控制 (输入为 PWM 0-100)
            motor_set_speed_and_dir(ctrl->motor_id, speed_pwm_duty, dir);
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

/**
 * @brief 位置式PID计算核心
 * @note 关于单位换算的重要说明：
 *       PID控制器的输入误差(error)单位是 RPM (转/分钟)
 *       PID控制器的输出(output)单位是 PWM占空比 (0-100%)
 */
static void pid_calculate(pid_ctrl_block_t *ctrl)
{
    float error = ctrl->state.target_speed - ctrl->state.actual_speed;

    // 死区处理
    if (fabs(error) < ctrl->params.dead_zone)
    {
        error = 0.0f;
    }

    // 比例项 (P_out单位为PWM%)
    float p_out = ctrl->params.kp * error;

    // 积分项 (带抗饱和)
    ctrl->state.integral_sum += ctrl->params.ki * error;
    if (ctrl->state.integral_sum > ctrl->params.integral_max)
        ctrl->state.integral_sum = ctrl->params.integral_max;
    else if (ctrl->state.integral_sum < -ctrl->params.integral_max)
        ctrl->state.integral_sum = -ctrl->params.integral_max;

    float i_out = ctrl->state.integral_sum;

    // 微分项
    float d_out = ctrl->params.kd * (error - ctrl->state.error_last);

    // 总输出 (单位: PWM占空比%, 范围理论上可能超过100，需限幅)
    float total_out = p_out + i_out + d_out;

    // 输出限幅 (适配 motor_set_speed_and_dir 的输入范围 0-100)
    if (total_out > ctrl->params.output_max)
        total_out = ctrl->params.output_max;
    else if (total_out < -ctrl->params.output_max)
        total_out = -ctrl->params.output_max;

    ctrl->state.output = total_out;
    ctrl->state.error_last = error;
}
