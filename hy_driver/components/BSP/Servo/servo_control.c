/**
 * @file servo_control.c
 * @brief 三舵机统一角度控制系统实现
 *
 * 本文件实现基于 LEDC PWM 的舵机控制系统，采用统一角度系统设计。
 *
 * @author ZHC
 * @date 2025
 * @version 1.0
 */

#include "servo_control.h"

static const char *TAG = "SERVO";

/**
 * @brief 舵机当前统一角度缓存
 *
 * 存储三个舵机的当前统一角度值（0-60°），用于状态查询。
 * 索引对应关系：[0]=SERVO_A, [1]=SERVO_B, [2]=SERVO_C
 */
static int16_t s_servo_actual_angles[3] = {0, 0, 0};
void servo_init(void)
{
    servo_config_t servo_cfg = {
        .max_angle = 180,
        .min_width_us = 500,
        .max_width_us = 2500,
        .freq = 50,
        .timer_number = LEDC_TIMER_0,
        .channels = {
            .servo_pin = {
                SERVO_GPIO_A,
                SERVO_GPIO_B,
                SERVO_GPIO_C,
            },
            .ch = {
                SERVO_LEDC_A,
                SERVO_LEDC_B,
                SERVO_LEDC_C,
            },
        },
        .channel_number = 3,
    };

    iot_servo_init(LEDC_LOW_SPEED_MODE, &servo_cfg);
    servo_reset();
    servo_angles_t angles = servo_get_actual_angles();
    ESP_LOGI(TAG, "舵机初始化完成 - 实际角度A:%d°, B:%d°, C:%d°", angles.angle_a, angles.angle_b, angles.angle_c);
}

void servo_reset(void)
{
    // 使用统一角度系统设置所有舵机到零位（统一角度0°），使用快速模式
    servo_set_angle(SERVO_A, SERVO_UNIFIED_ZERO_ANGLE, SERVO_SPEED_NORMAL);
    servo_set_angle(SERVO_B, SERVO_UNIFIED_ZERO_ANGLE, SERVO_SPEED_NORMAL);
    servo_set_angle(SERVO_C, SERVO_UNIFIED_ZERO_ANGLE, SERVO_SPEED_NORMAL);
    ESP_LOGI(TAG, "舵机复位完成 - 所有舵机设置为统一角度%d°", SERVO_UNIFIED_ZERO_ANGLE);
}

/**
 * @brief  内部函数：直接设置舵机到指定统一角度
 *
 * 该函数执行实际的角度映射和PWM设置，不包含速度控制逻辑。
 *
 * @param  id             目标舵机标识
 * @param  unified_angle  统一角度值（0-60°）
 */
static void servo_set_angle_internal(servo_id_t id, int16_t unified_angle)
{
    int16_t actual_angle = 0;

    switch (id)
    {
    case SERVO_A:
        /* 右手舵机：正向映射（统一角度 = 实际角度） */
        actual_angle = SERVO_A_ZERO_POSITION + unified_angle;
        iot_servo_write_angle(LEDC_LOW_SPEED_MODE, SERVO_LEDC_A, actual_angle);
        s_servo_actual_angles[0] = unified_angle;
        ESP_LOGD(TAG, "舵机A：统一角度%d° -> 实际角度%d°", unified_angle, actual_angle);
        break;

    case SERVO_B:
        /* 左手舵机：反向映射（镜像对称，实现左右手对称动作） */
        actual_angle = SERVO_B_ZERO_POSITION - unified_angle;
        iot_servo_write_angle(LEDC_LOW_SPEED_MODE, SERVO_LEDC_B, actual_angle);
        s_servo_actual_angles[1] = unified_angle;
        ESP_LOGD(TAG, "舵机B：统一角度%d° -> 实际角度%d°", unified_angle, actual_angle);
        break;

    case SERVO_C:
        /* 腰部舵机：带偏移的正向映射（30° 偏移避免机械干涉） */
        actual_angle = SERVO_C_ZERO_POSITION + unified_angle;
        iot_servo_write_angle(LEDC_LOW_SPEED_MODE, SERVO_LEDC_C, actual_angle);
        s_servo_actual_angles[2] = unified_angle;
        ESP_LOGD(TAG, "舵机C：统一角度%d° -> 实际角度%d°", unified_angle, actual_angle);
        break;

    default:
        ESP_LOGW(TAG, "无效的舵机ID: %d", id);
        break;
    }
}

/**
 * @brief  设置指定舵机的角度（带速度控制）
 *
 * 角度映射公式说明：
 * =========================================================================
 * 统一角度系统通过线性映射将逻辑角度转换为各舵机的实际角度。
 *
 * 映射公式：
 * 实际角度 = 零位角度 ± 统一角度
 *
 * 各舵机映射关系：
 *
 * 1. SERVO_A（右手舵机）- 正向映射：
 *    公式：actual_angle = SERVO_A_ZERO_POSITION + unified_angle
 *    示例：actual_angle = 0 + unified_angle
 *    映射表：
 *      统一角度 0°  → 实际角度 0°
 *      统一角度 30° → 实际角度 30°
 *      统一角度 60° → 实际角度 60°
 *
 * 2. SERVO_B（左手舵机）- 反向映射（镜像对称）：
 *    公式：actual_angle = SERVO_B_ZERO_POSITION - unified_angle
 *    示例：actual_angle = 180 - unified_angle
 *    映射表：
 *      统一角度 0°  → 实际角度 180°
 *      统一角度 30° → 实际角度 150°
 *      统一角度 60° → 实际角度 120°
 *    说明：反向映射实现左右手对称动作
 *
 * 3. SERVO_C（腰部舵机）- 带偏移的正向映射：
 *    公式：actual_angle = SERVO_C_ZERO_POSITION + unified_angle
 *    示例：actual_angle = 30 + unified_angle
 *    映射表：
 *      统一角度 0°  → 实际角度 30°
 *      统一角度 30° → 实际角度 60°
 *      统一角度 60° → 实际角度 90°
 *    说明：30° 偏移避免机械结构干涉
 * =========================================================================
 *
 * @param  id             目标舵机标识
 * @param  unified_angle  统一角度值（0-60°）
 * @param  speed          运动速度等级
 */
void servo_set_angle(servo_id_t id, int16_t unified_angle, servo_speed_t speed)
{
    /* 参数范围检查 */
    if (unified_angle < SERVO_UNIFIED_ZERO_ANGLE || unified_angle > SERVO_UNIFIED_MAX_ANGLE)
    {
        ESP_LOGW(TAG, "统一角度%d超出范围[%d-%d]", unified_angle, SERVO_UNIFIED_ZERO_ANGLE, SERVO_UNIFIED_MAX_ANGLE);
        return;
    }

    /* 快速模式：直接跳转到目标角度 */
    if (speed == SERVO_SPEED_FAST)
    {
        servo_set_angle_internal(id, unified_angle);
        return;
    }

    /* 获取当前角度 */
    int16_t current_angle = s_servo_actual_angles[id];

    /* 如果已在目标位置，直接返回 */
    if (current_angle == unified_angle)
    {
        return;
    }

    /* 根据速度等级确定步进参数 */
    int16_t step = (speed == SERVO_SPEED_SLOW) ? SERVO_SPEED_SLOW_STEP : SERVO_SPEED_NORMAL_STEP;
    int16_t delay_ms = (speed == SERVO_SPEED_SLOW) ? SERVO_SPEED_SLOW_DELAY : SERVO_SPEED_NORMAL_DELAY;

    /* 确定运动方向 */
    int16_t direction = (unified_angle > current_angle) ? 1 : -1;

    /* 渐进运动到目标角度 */
    while (current_angle != unified_angle)
    {
        current_angle += direction * step;

        /* 防止越过目标位置 */
        if ((direction > 0 && current_angle > unified_angle) ||
            (direction < 0 && current_angle < unified_angle))
        {
            current_angle = unified_angle;
        }

        servo_set_angle_internal(id, current_angle);
        vTaskDelay(pdMS_TO_TICKS(delay_ms));
    }
}

servo_angles_t servo_get_actual_angles(void)
{
    servo_angles_t angles;
    angles.angle_a = s_servo_actual_angles[0];
    angles.angle_b = s_servo_actual_angles[1];
    angles.angle_c = s_servo_actual_angles[2];
    return angles;
}
