/**
 * @file app_move_control.h
 * @brief 运动控制与里程计模块
 *
 * 本模块提供差分驱动机器人的运动学计算和里程计功能，包括：
 * - 正运动学：左右轮速度 → 线速度/角速度
 * - 逆运动学：线速度/角速度 → 左右轮速度
 * - 里程计：累积位置、速度、姿态估计
 *
 * 机械参数：
 * - 轮子直径：0.060m（60mm）
 * - 轮距：0.125m（125mm）
 * - 空载转速：58 RPM ≈ 0.2 m/s
 *
 * @author ZHC
 * @date 2025
 * @version 1.0
 *
 * @note 使用前需要先初始化电机和编码器
 */

#ifndef __APP_MOVE_CONTROL_H__
#define __APP_MOVE_CONTROL_H__

/* ==================================================================================
 *                                    Includes
 * ================================================================================== */
#include "all_include.h"

/* ==================================================================================
 *                                     Macros
 * ================================================================================== */

#define WHEEL_DIAMETER_M 0.060f                         ///< 轮子直径（米），空载转速 58RPM，等效 0.2m/s
#define WHEEL_BASE_M 0.125f                             ///< 轮距（米）- 左右两轮中心距离
#define WHEEL_CIRCUMFERENCE_M (M_PI * WHEEL_DIAMETER_M) ///< 轮子周长（米）

/* ==================================================================================
 *                                   Data Types
 * ================================================================================== */

/**
 * @brief 四元数结构体
 *
 * 用于表示机器人的三维姿态
 */
typedef struct
{
    float w;  ///< 四元数标量部分
    float x;  ///< 四元数向量 X 分量
    float y;  ///< 四元数向量 Y 分量
    float z;  ///< 四元数向量 Z 分量
} quaternion_t;

/**
 * @brief 里程计数据结构体
 *
 * 存储机器人的位置、速度和姿态信息
 */
typedef struct
{
    float x;           ///< X 轴上的位置（米）
    float y;           ///< Y 轴上的位置（米）
    float angle;       ///< 航向角（弧度）
    float linear_vel;  ///< 线速度（米/秒）
    float angular_vel; ///< 角速度（弧度/秒）
    float gyro_x;      ///< 陀螺仪 X 轴角速度（弧度/秒）
    float gyro_y;      ///< 陀螺仪 Y 轴角速度（弧度/秒）
    float gyro_z;      ///< 陀螺仪 Z 轴角速度（弧度/秒）
    quaternion_t q;    ///< 姿态四元数
} odom_t;

/* ==================================================================================
 *                               Function Prototypes
 * ================================================================================== */

/**
 * @brief  初始化运动控制模块
 *
 * 初始化运动控制相关的任务和资源，包括：
 * - 创建里程计更新任务
 * - 初始化位置和姿态变量
 * - 配置运动学参数
 *
 * @return void
 *
 * @note   必须在电机和编码器初始化后调用
 */
void app_move_control_init(void);

/**
 * @brief  逆运动学解析
 *
 * 根据线速度和角速度计算左右轮的目标转速（RPM）。
 *
 * @param  linear_vel  线速度（米/秒）
 * @param  angular_vel 角速度（弧度/秒）
 * @param  rpm_left    输出左轮转速（RPM）
 * @param  rpm_right   输出右轮转速（RPM）
 *
 * @return void
 *
 * @note   差分驱动运动学公式：
 *         v_left = v - ω * L / 2
 *         v_right = v + ω * L / 2
 *         其中 v 为线速度，ω 为角速度，L 为轮距
 */
void inverse_kinematics(float linear_vel, float angular_vel, float *rpm_left, float *rpm_right);

/**
 * @brief  正运动学解析
 *
 * 根据左右轮的实际转速（RPM）计算机器人的线速度和角速度。
 *
 * @param  rpm_left    左轮转速（RPM）
 * @param  rpm_right   右轮转速（RPM）
 * @param  linear_vel  输出线速度（米/秒）
 * @param  angular_vel 输出角速度（弧度/秒）
 *
 * @return void
 *
 * @note   差分驱动运动学公式：
 *         v = (v_left + v_right) / 2
 *         ω = (v_right - v_left) / L
 *         其中 v_left、v_right 为左右轮线速度，L 为轮距
 */
void forward_kinematics(float rpm_left, float rpm_right, float *linear_vel, float *angular_vel);

/**
 * @brief  获取里程计信息
 *
 * 返回当前机器人的位置、速度和姿态数据。
 *
 * @return odom_t* 指向里程计数据结构的指针
 *
 * @note   返回的指针指向内部静态变量，不需要释放
 * @note   数据由里程计更新任务实时更新
 */
odom_t *get_odometry(void);

#endif /* __APP_MOVE_CONTROL_H__ */
