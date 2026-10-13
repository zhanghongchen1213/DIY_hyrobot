/**
 * @file imu.h
 * @brief QMI8658 六轴 IMU 传感器驱动
 *
 * 本模块实现 QMI8658 六轴惯性测量单元（IMU）的驱动程序，
 * 包括加速度计和陀螺仪的数据采集、姿态角计算等功能。
 *
 * 硬件配置：
 * - 传感器型号：QMI8658（六轴 IMU）
 * - 通信接口：I2C（400kHz 快速模式）
 * - I2C 地址：0x6A（7位地址）
 * - GPIO 引脚：SDA=GPIO1, SCL=GPIO2
 *
 * 传感器配置：
 * - 加速度计：±4g 量程，250Hz 采样率
 * - 陀螺仪：±512dps 量程，250Hz 采样率
 * - 低通滤波：已启用
 *
 * @author ZHC
 * @date 2025
 * @version 1.0
 */

#ifndef __IMU_H__
#define __IMU_H__

/* ==================================================================================
 *                                    Includes
 * ================================================================================== */
#include <stdio.h>
#include <math.h>
#include <string.h>
#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_dsp.h"
#include "driver/i2c.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/* ==================================================================================
 *                                     Macros
 * ================================================================================== */

/* I2C 总线配置 */
#define BSP_I2C_SDA GPIO_NUM_1   ///< I2C 数据线引脚（SDA）
#define BSP_I2C_SCL GPIO_NUM_2   ///< I2C 时钟线引脚（SCL）
#define BSP_I2C_NUM (0)          ///< 使用 I2C 端口 0
#define BSP_I2C_FREQ_HZ 400000   ///< I2C 时钟频率 400kHz（快速模式）

/* QMI8658 传感器配置 */
#define QMI8658_SENSOR_ADDR 0x6A ///< I2C 从机地址（7位地址模式）

/* 传感器量程和分辨率 */
#define ACC_SCALE 4.0f           ///< 加速度计量程：±4g
#define GYRO_SCALE 512.0f        ///< 陀螺仪量程：±512dps（度/秒）
#define ADC_RESOLUTION 32768.0f  ///< 16位ADC分辨率（2^15，有符号整数范围）

/* 通用位操作宏 */
#ifdef _BV
#undef _BV
#endif
#define _BV(b) (1UL << (uint32_t)(b))  ///< 位值宏（bit value）

#ifndef lowByte
#define lowByte(w) ((uint8_t)((w) & 0xff))  ///< 提取低字节
#endif

#ifndef highByte
#define highByte(w) ((uint8_t)((w) >> 8))   ///< 提取高字节
#endif

/* ==================================================================================
 *                                   Data Types
 * ================================================================================== */

/**
 * @brief QMI8658 寄存器地址枚举
 *
 * 定义 QMI8658 传感器的所有寄存器地址
 */
enum qmi8658_reg
{
    QMI8658_WHO_AM_I,            ///< 芯片 ID 寄存器（应读取 0x05）
    QMI8658_REVISION_ID,         ///< 版本 ID 寄存器
    QMI8658_CTRL1,               ///< 控制寄存器1：地址自动递增等
    QMI8658_CTRL2,               ///< 控制寄存器2：加速度计配置（量程、采样率）
    QMI8658_CTRL3,               ///< 控制寄存器3：陀螺仪配置（量程、采样率）
    QMI8658_CTRL4,               ///< 控制寄存器4：磁力计配置
    QMI8658_CTRL5,               ///< 控制寄存器5：低通滤波器配置
    QMI8658_CTRL6,               ///< 控制寄存器6：运动检测配置
    QMI8658_CTRL7,               ///< 控制寄存器7：传感器使能控制
    QMI8658_CTRL8,               ///< 控制寄存器8：其他功能配置
    QMI8658_CTRL9,               ///< 控制寄存器9：命令寄存器
    QMI8658_CATL1_L,             ///< 校准寄存器1低字节
    QMI8658_CATL1_H,             ///< 校准寄存器1高字节
    QMI8658_CATL2_L,             ///< 校准寄存器2低字节
    QMI8658_CATL2_H,             ///< 校准寄存器2高字节
    QMI8658_CATL3_L,             ///< 校准寄存器3低字节
    QMI8658_CATL3_H,             ///< 校准寄存器3高字节
    QMI8658_CATL4_L,             ///< 校准寄存器4低字节
    QMI8658_CATL4_H,             ///< 校准寄存器4高字节
    QMI8658_FIFO_WTM_TH,         ///< FIFO 水位阈值
    QMI8658_FIFO_CTRL,           ///< FIFO 控制寄存器
    QMI8658_FIFO_SMPL_CNT,       ///< FIFO 样本计数
    QMI8658_FIFO_STATUS,         ///< FIFO 状态寄存器
    QMI8658_FIFO_DATA,           ///< FIFO 数据寄存器
    QMI8658_STATUSINT = 45,      ///< 中断状态寄存器
    QMI8658_STATUS0,             ///< 状态寄存器0（数据就绪标志）
    QMI8658_STATUS1,             ///< 状态寄存器1
    QMI8658_TIMESTAMP_LOW,       ///< 时间戳低字节
    QMI8658_TIMESTAMP_MID,       ///< 时间戳中字节
    QMI8658_TIMESTAMP_HIGH,      ///< 时间戳高字节
    QMI8658_TEMP_L,              ///< 温度数据低字节
    QMI8658_TEMP_H,              ///< 温度数据高字节
    QMI8658_AX_L,                ///< 加速度 X 轴低字节
    QMI8658_AX_H,                ///< 加速度 X 轴高字节
    QMI8658_AY_L,                ///< 加速度 Y 轴低字节
    QMI8658_AY_H,                ///< 加速度 Y 轴高字节
    QMI8658_AZ_L,                ///< 加速度 Z 轴低字节
    QMI8658_AZ_H,                ///< 加速度 Z 轴高字节
    QMI8658_GX_L,                ///< 陀螺仪 X 轴低字节
    QMI8658_GX_H,                ///< 陀螺仪 X 轴高字节
    QMI8658_GY_L,                ///< 陀螺仪 Y 轴低字节
    QMI8658_GY_H,                ///< 陀螺仪 Y 轴高字节
    QMI8658_GZ_L,                ///< 陀螺仪 Z 轴低字节
    QMI8658_GZ_H,                ///< 陀螺仪 Z 轴高字节
    QMI8658_COD_STATUS = 70,     ///< 坐标系统状态
    QMI8658_dQW_L = 73,          ///< 四元数 W 分量低字节
    QMI8658_dQW_H,               ///< 四元数 W 分量高字节
    QMI8658_dQX_L,               ///< 四元数 X 分量低字节
    QMI8658_dQX_H,               ///< 四元数 X 分量高字节
    QMI8658_dQY_L,               ///< 四元数 Y 分量低字节
    QMI8658_dQY_H,               ///< 四元数 Y 分量高字节
    QMI8658_dQZ_L,               ///< 四元数 Z 分量低字节
    QMI8658_dQZ_H,               ///< 四元数 Z 分量高字节
    QMI8658_dVX_L,               ///< 速度 X 分量低字节
    QMI8658_dVX_H,               ///< 速度 X 分量高字节
    QMI8658_dVY_L,               ///< 速度 Y 分量低字节
    QMI8658_dVY_H,               ///< 速度 Y 分量高字节
    QMI8658_dVZ_L,               ///< 速度 Z 分量低字节
    QMI8658_dVZ_H,               ///< 速度 Z 分量高字节
    QMI8658_TAP_STATUS = 89,     ///< 敲击检测状态
    QMI8658_STEP_CNT_LOW,        ///< 计步器计数低字节
    QMI8658_STEP_CNT_MIDL,       ///< 计步器计数中字节
    QMI8658_STEP_CNT_HIGH,       ///< 计步器计数高字节
    QMI8658_RESET = 96           ///< 软复位寄存器（写入 0xb0 复位）
};

/**
 * @brief QMI8658 命令表枚举
 *
 * 定义可写入 CTRL9 命令寄存器的控制命令
 */
enum CommandTable
{
    CTRL_CMD_ACK = 0x00,                       ///< 命令确认
    CTRL_CMD_RST_FIFO = 0x04,                  ///< 复位 FIFO
    CTRL_CMD_REQ_FIFO = 0x05,                  ///< 请求 FIFO 数据
    CTRL_CMD_WRITE_WOM_SETTING = 0x08,         ///< 写入唤醒运动设置
    CTRL_CMD_ACCEL_HOST_DELTA_OFFSET = 0x09,   ///< 加速度计主机偏移校准
    CTRL_CMD_GYRO_HOST_DELTA_OFFSET = 0x0A,    ///< 陀螺仪主机偏移校准
    CTRL_CMD_CONFIGURE_TAP = 0x0C,             ///< 配置敲击检测
    CTRL_CMD_CONFIGURE_PEDOMETER = 0x0D,       ///< 配置计步器
    CTRL_CMD_CONFIGURE_MOTION = 0x0E,          ///< 配置运动检测
    CTRL_CMD_RESET_PEDOMETER = 0x0F,           ///< 复位计步器
    CTRL_CMD_COPY_USID = 0x10,                 ///< 复制用户 ID
    CTRL_CMD_SET_RPU = 0x11,                   ///< 设置 RPU
    CTRL_CMD_AHB_CLOCK_GATING = 0x12,          ///< AHB 时钟门控
    CTRL_CMD_ON_DEMAND_CALIBRATION = 0xA2,     ///< 按需校准
    CTRL_CMD_APPLY_GYRO_GAINS = 0xAA,          ///< 应用陀螺仪增益
};

/**
 * @brief IMU 传感器处理后的数据结构
 *
 * 包含转换为物理单位的加速度、角速度和姿态角数据
 */
typedef struct
{
    float acc_x;  ///< X 轴加速度（前后方向，单位：g）
    float acc_y;  ///< Y 轴加速度（左右方向，单位：g）
    float acc_z;  ///< Z 轴加速度（上下方向，单位：g）
    float gyr_x;  ///< X 轴角速度（绕前后轴旋转，单位：dps）
    float gyr_y;  ///< Y 轴角速度（绕左右轴旋转，单位：dps）
    float gyr_z;  ///< Z 轴角速度（绕上下轴旋转，单位：dps）
    float pitch;  ///< 俯仰角（绕 Y 轴旋转，前后倾斜，单位：度）
    float roll;   ///< 横滚角（绕 X 轴旋转，左右倾斜，单位：度）
    float yaw;    ///< 偏航角（绕 Z 轴旋转，航向角，单位：度）
} t_sQMI8658;

/**
 * @brief IMU 传感器原始数据结构
 *
 * 包含从寄存器读取的 16 位原始 ADC 值，未经单位转换
 */
typedef struct
{
    int16_t acc_x_raw;  ///< X 轴加速度原始值（-32768 ~ +32767）
    int16_t acc_y_raw;  ///< Y 轴加速度原始值（-32768 ~ +32767）
    int16_t acc_z_raw;  ///< Z 轴加速度原始值（-32768 ~ +32767）
    int16_t gyr_x_raw;  ///< X 轴陀螺仪原始值（-32768 ~ +32767）
    int16_t gyr_y_raw;  ///< Y 轴陀螺仪原始值（-32768 ~ +32767）
    int16_t gyr_z_raw;  ///< Z 轴陀螺仪原始值（-32768 ~ +32767）
} t_sQMI8658_raw;

/* ==================================================================================
 *                               Function Prototypes
 * ================================================================================== */

/**
 * @brief  初始化 IMU 传感器
 *
 * 配置 I2C 总线并初始化 QMI8658 传感器，包括：
 * - 配置 I2C 总线（400kHz，GPIO1/GPIO2）
 * - 验证芯片 ID
 * - 配置加速度计（±4g，250Hz）
 * - 配置陀螺仪（±512dps，250Hz）
 * - 启用低通滤波器
 *
 * @return void
 *
 * @note   此函数会阻塞直到传感器初始化成功
 */
void imu_init(void);

/**
 * @brief  读取并处理 IMU 传感器数据
 *
 * 从 QMI8658 读取原始传感器数据，并转换为物理单位和姿态角
 *
 * @param  p 指向 t_sQMI8658 结构体的指针，用于存储处理后的数据
 *
 * @return void
 *
 * @note   此函数会更新加速度、角速度和姿态角（pitch/roll/yaw）
 */
void imu_data_resolution(t_sQMI8658 *p);

#endif /* __IMU_H__ */
