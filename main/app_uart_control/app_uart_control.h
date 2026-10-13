/**
 * @file app_uart_control.h
 * @brief UART 串口通信控制模块
 *
 * 本模块负责处理与上位机的双向 UART 通信协议，包括：
 * - 上行数据包：上报里程计、电机状态、IMU 数据、关节状态等
 * - 下行数据包：接收运动控制指令、PID 参数调整、舵机控制等
 * - 语音识别命令：支持 CI-03T 语音模块指令
 *
 * 通信协议：
 * - 上行包：起始标志 0xAA，结束标志 0x55
 * - 下行包：起始标志 0xAA，结束标志 0x66
 * - 数据格式：结构体打包传输（__attribute__((packed))）
 *
 * @author ZHC
 * @date 2025
 * @version 1.0
 *
 * @note 使用前需要先初始化 UART 硬件
 * @note 支持条件编译（UPDATE_ODOMETRY_DEBUG）以控制数据包大小
 */

#ifndef __APP_UART_CONTROL_H__
#define __APP_UART_CONTROL_H__

/* ==================================================================================
 *                                    Includes
 * ================================================================================== */
#include "all_include.h"

/* ==================================================================================
 *                                     Macros
 * ================================================================================== */

#define SEND_PACKET_START_FLAG 0xAA    ///< 上行数据包起始标志
#define SEND_PACKET_END_FLAG 0x55      ///< 上行数据包结束标志

#define RECEIVE_PACKET_START_FLAG 0xAA ///< 下行数据包起始标志
#define RECEIVE_PACKET_END_FLAG 0x66   ///< 下行数据包结束标志

/* ==================================================================================
 *                                   Data Types
 * ================================================================================== */

/**
 * @brief CI-03T 语音识别命令枚举
 *
 * 定义语音模块支持的基本控制指令
 */
typedef enum
{
    WAKE_UP = 0x00, ///< 唤醒指令
    CHAT_GPT,       ///< ChatGPT 对话指令
    WAKE_EXIT       ///< 退出唤醒指令
} CI_03T_CMD;

/**
 * @brief 上行数据包结构体（设备 → 上位机）
 *
 * 用于上报机器人实时状态数据，包括：
 * - 电机目标速度与实际速度
 * - PID 控制参数（Kp、Ki、Kd）
 * - 里程计位置与速度（仅调试模式）
 * - IMU 传感器数据（陀螺仪、姿态四元数）
 * - 舵机关节角度
 *
 * @note 使用 __attribute__((packed)) 确保结构体紧凑排列，无内存对齐
 * @note UPDATE_ODOMETRY_DEBUG 宏控制是否包含里程计和 IMU 数据
 */
typedef struct
{
    uint8_t start_flag; ///< 起始标志 0xAA

    uint16_t chat_gpt_count; ///< 用户触发 ChatGPT 的次数统计

    // --- 电机与 PID 状态（调试用）---
    float left_target_speed;  ///< 左电机目标速度（RPM）
    float right_target_speed; ///< 右电机目标速度（RPM）
    float left_actual_speed;  ///< 左电机实际速度（RPM）
    float right_actual_speed; ///< 右电机实际速度（RPM）
    float left_kp;            ///< 左电机 PID 比例系数
    float left_ki;            ///< 左电机 PID 积分系数
    float left_kd;            ///< 左电机 PID 微分系数
    float right_kp;           ///< 右电机 PID 比例系数
    float right_ki;           ///< 右电机 PID 积分系数
    float right_kd;           ///< 右电机 PID 微分系数

#if UPDATE_ODOMETRY_DEBUG
    // --- 里程计与姿态（用于 SLAM/Nav/RVIZ）---
    float position_x;        ///< 编码器积分得到的位置 X（米）
    float position_y;        ///< 编码器积分得到的位置 Y（米）
    float theta_wheel;       ///< 编码器积分得到的航向角 θ（弧度）
    float linear_vel_x;      ///< 编码器计算的线速度（米/秒）
    float angular_vel_wheel; ///< 编码器计算的角速度（弧度/秒）

    // --- IMU 传感器部分（纯 IMU 数据）---
    float gyro_x; ///< IMU 陀螺仪 X 轴角速度（弧度/秒）
    float gyro_y; ///< IMU 陀螺仪 Y 轴角速度（弧度/秒）
    float gyro_z; ///< IMU 陀螺仪 Z 轴角速度（弧度/秒，最重要！）

    // --- 姿态四元数（用于数字孪生）---
    float q_w; ///< IMU 积分得到的四元数 W 分量（标量）
    float q_x; ///< IMU 积分得到的四元数 X 分量
    float q_y; ///< IMU 积分得到的四元数 Y 分量
    float q_z; ///< IMU 积分得到的四元数 Z 分量

    // --- 关节状态（用于 Joint State）---
    int16_t servo_a_angle; ///< 舵机 A 当前角度（度）
    int16_t servo_b_angle; ///< 舵机 B 当前角度（度）
    int16_t servo_c_angle; ///< 舵机 C 当前角度（度）
#endif

    uint32_t timestamp; ///< 时间戳（毫秒）
    uint8_t end_flag;   ///< 结束标志 0x55
} __attribute__((packed)) uart_uplink_packet_t;

/**
 * @brief 下行数据包结构体（上位机 → 设备）
 *
 * 用于接收上位机下发的控制指令，包括：
 * - 音频流控制标志
 * - 电机目标速度设置
 * - PID 参数动态调整
 * - 运动控制指令（线速度、角速度）
 * - 舵机角度控制
 *
 * @note 使用 __attribute__((packed)) 确保结构体紧凑排列，无内存对齐
 * @note UPDATE_ODOMETRY_DEBUG 宏控制是否包含运动学和舵机控制字段
 */
typedef struct
{
    uint8_t start_flag; ///< 起始标志 0xAA

    uint8_t audio_stream_flag; ///< 音频流标志位（0=未触发，1=语音识别结束，2=TTS 合成结束，3=端侧 HTTPS 下发完成，4=端侧 HTTPS 接收完成）

    // --- 电机目标速度 ---
    float left_target_speed;  ///< 左电机目标速度（RPM）
    float right_target_speed; ///< 右电机目标速度（RPM）

    // --- PID 参数调整 ---
    float left_kp;  ///< 左电机 PID 比例系数
    float left_ki;  ///< 左电机 PID 积分系数
    float left_kd;  ///< 左电机 PID 微分系数
    float right_kp; ///< 右电机 PID 比例系数
    float right_ki; ///< 右电机 PID 积分系数
    float right_kd; ///< 右电机 PID 微分系数

#if UPDATE_ODOMETRY_DEBUG
    // --- 运动控制指令 ---
    float linear_vel;  ///< 线速度（米/秒）
    float angular_vel; ///< 角速度（弧度/秒）

    // --- 舵机角度控制 ---
    float servo_a_angle; ///< 舵机 A 目标角度（度）
    float servo_b_angle; ///< 舵机 B 目标角度（度）
    float servo_c_angle; ///< 舵机 C 目标角度（度）
#endif

    uint32_t timestamp; ///< 时间戳（毫秒）
    uint8_t end_flag;   ///< 结束标志 0x66
} __attribute__((packed)) uart_downlink_packet_t;

/* ==================================================================================
 *                               Function Prototypes
 * ================================================================================== */

/**
 * @brief  初始化 UART 通信控制模块
 *
 * 初始化 UART 通信相关的 RTOS 任务和资源，包括：
 * - 配置 UART 硬件参数（波特率、数据位、停止位）
 * - 创建数据收发任务
 * - 初始化数据包解析缓冲区
 *
 * @return void
 *
 * @note   必须在 UART 驱动初始化后调用
 * @note   启动后会自动创建 UART 收发任务
 */
void rtos_uart_init(void);

#endif /* __APP_UART_CONTROL_H__ */
