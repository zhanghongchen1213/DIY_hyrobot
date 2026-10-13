#include "imu.h"

static const char *TAG = "QMI8658A";
static uint8_t g_qmi8658_addr = QMI8658_SENSOR_ADDR; // 允许初始化过程中回退到备选地址
static esp_err_t qmi8658_register_read(uint8_t reg_addr, uint8_t *data, size_t len);
static esp_err_t qmi8658_register_write_byte(uint8_t reg_addr, uint8_t data);
static void qmi8658_Read_AccAndGry(t_sQMI8658_raw *p);
static void imu_read_data(t_sQMI8658 *p, t_sQMI8658_raw *p_raw);
static void qmi8658_init(void);

/************外部接口函数 ************/
void imu_data_resolution(t_sQMI8658 *p)
{
    t_sQMI8658_raw p_raw;
    memset(&p_raw, 0, sizeof(t_sQMI8658_raw));
    // 读取IMU数据
    imu_read_data(p, &p_raw);
}

/************ 内部函数 ************/

/**
 * @brief  读取 QMI8658 寄存器的值
 *
 * @param  reg_addr 寄存器地址
 * @param  data     数据缓冲区指针
 * @param  len      读取的字节数
 *
 * @retval ESP_OK   读取成功
 * @retval 其他     I2C 错误码
 *
 * @note   超时时间为 1000ms，使用全局地址 g_qmi8658_addr
 */
static esp_err_t qmi8658_register_read(uint8_t reg_addr, uint8_t *data, size_t len)
{
    // I2C 超时计算：1000ms / portTICK_PERIOD_MS 转换为系统时钟周期数
    return i2c_master_write_read_device(BSP_I2C_NUM, g_qmi8658_addr, &reg_addr, 1, data, len, 1000 / portTICK_PERIOD_MS);
}

/**
 * @brief  向 QMI8658 寄存器写入单字节数据
 *
 * @param  reg_addr 寄存器地址
 * @param  data     要写入的数据
 *
 * @retval ESP_OK   写入成功
 * @retval 其他     I2C 错误码
 *
 * @note   超时时间为 1000ms
 */
static esp_err_t qmi8658_register_write_byte(uint8_t reg_addr, uint8_t data)
{
    uint8_t write_buf[2] = {reg_addr, data}; // 组装写入缓冲区：[寄存器地址, 数据]

    return i2c_master_write_to_device(BSP_I2C_NUM, g_qmi8658_addr, write_buf, sizeof(write_buf), 1000 / portTICK_PERIOD_MS);
}

void imu_init(void)
{
    // I2C 总线配置
    i2c_config_t i2c_conf = {
        .mode = I2C_MODE_MASTER,              // 主机模式
        .sda_io_num = BSP_I2C_SDA,            // SDA 引脚：GPIO1
        .sda_pullup_en = GPIO_PULLUP_ENABLE,  // 使能 SDA 上拉电阻
        .scl_io_num = BSP_I2C_SCL,            // SCL 引脚：GPIO2
        .scl_pullup_en = GPIO_PULLUP_ENABLE,  // 使能 SCL 上拉电阻
        .master.clk_speed = BSP_I2C_FREQ_HZ}; // 时钟频率：400kHz（快速模式）

    // 应用 I2C 配置参数
    i2c_param_config(BSP_I2C_NUM, &i2c_conf);

    // 安装 I2C 驱动（中断驱动模式，无 DMA 缓冲区）
    i2c_driver_install(BSP_I2C_NUM, i2c_conf.mode, 0, 0, 0);

    // 初始化 QMI8658 传感器
    qmi8658_init();
    t_sQMI8658 imu_data;
    memset(&imu_data, 0, sizeof(t_sQMI8658));
    imu_data_resolution(&imu_data);
    ESP_LOGI(TAG, "pitch:%d, yaw:%d, roll:%d", imu_data.pitch, imu_data.yaw, imu_data.roll);
}

/**
 * @brief  初始化 QMI8658 传感器
 *
 * 配置传感器的工作模式、量程和采样率
 *
 * @note   此函数会阻塞直到芯片 ID 验证成功（ID = 0x05）
 * @warning 如果传感器未连接，此函数会无限等待
 */
static void qmi8658_init(void)
{
    // 步骤 1: 验证芯片 ID
    uint8_t id = 0;
    qmi8658_register_read(QMI8658_WHO_AM_I, &id, 1);
    ESP_LOGI(TAG, "QMI8658 ID: %d", id);

    // 等待芯片 ID 为 0x05（QMI8658 的正确 ID）
    // 警告：如果传感器未连接或损坏，此循环会无限等待
    while (id != 0x05)
    {
        vTaskDelay(1000 / portTICK_PERIOD_MS); // 延时 1 秒后重试
        qmi8658_register_read(QMI8658_WHO_AM_I, &id, 1);
        ESP_LOGI(TAG, "QMI8658 ID: %d", id);
    }
    ESP_LOGI(TAG, "QMI8658 OK!");

    // 步骤 2: 软复位传感器
    // 写入 0xb0 到 RESET 寄存器，复位所有寄存器到默认值
    // 必须在所有配置之前执行
    qmi8658_register_write_byte(QMI8658_RESET, 0xb0);
    vTaskDelay(10 / portTICK_PERIOD_MS); // 等待 10ms 让复位完成

    // 步骤 3: 配置控制寄存器
    // CTRL1 = 0x40: 使能地址自动递增（用于连续读取多个寄存器）
    qmi8658_register_write_byte(QMI8658_CTRL1, 0x40);

    // CTRL7 = 0x03: 使能加速度计和陀螺仪
    // 位[0] = 1: 使能加速度计
    // 位[1] = 1: 使能陀螺仪
    qmi8658_register_write_byte(QMI8658_CTRL7, 0x03);

    // CTRL2 = 0x95: 配置加速度计
    // 量程：±4g，采样率：250Hz
    qmi8658_register_write_byte(QMI8658_CTRL2, 0x95);

    // CTRL3 = 0xd5: 配置陀螺仪
    // 量程：±512dps，采样率：250Hz
    qmi8658_register_write_byte(QMI8658_CTRL3, 0xd5);

    // CTRL5 = 0x11: 使能低通滤波器
    // 降低高频噪声，提高数据稳定性
    qmi8658_register_write_byte(QMI8658_CTRL5, 0x11);
}

/**
 * @brief  读取加速度计和陀螺仪的原始数据
 *
 * @param  p 指向原始数据结构的指针
 *
 * @note   只有当数据就绪标志位置位时才读取数据
 */
static void qmi8658_Read_AccAndGry(t_sQMI8658_raw *p)
{
    uint8_t status, data_ready = 0;
    int16_t buf[6]; // 缓冲区：存储 6 个 int16_t 值（加速度 XYZ + 陀螺仪 XYZ）

    // 读取 STATUS0 寄存器，检查数据是否就绪
    qmi8658_register_read(QMI8658_STATUS0, &status, 1);

    // 检查位掩码 0x03（位[0]: 加速度计就绪，位[1]: 陀螺仪就绪）
    if (status & 0x03)
        data_ready = 1;

    if (data_ready == 1)
    {
        data_ready = 0;
        // 从 AX_L 寄存器开始连续读取 12 字节（6 个 int16_t）
        // 读取顺序：AX_L, AX_H, AY_L, AY_H, AZ_L, AZ_H, GX_L, GX_H, GY_L, GY_H, GZ_L, GZ_H
        qmi8658_register_read(QMI8658_AX_L, (uint8_t *)buf, 12);

        // 将缓冲区数据分配到结构体成员
        p->acc_x_raw = buf[0]; // 加速度 X 轴
        p->acc_y_raw = buf[1]; // 加速度 Y 轴
        p->acc_z_raw = buf[2]; // 加速度 Z 轴
        p->gyr_x_raw = buf[3]; // 陀螺仪 X 轴
        p->gyr_y_raw = buf[4]; // 陀螺仪 Y 轴
        p->gyr_z_raw = buf[5]; // 陀螺仪 Z 轴
    }
}

/**
 * @brief  读取传感器数据并计算姿态角
 *
 * @param  p      指向处理后数据结构的指针
 * @param  p_raw  指向原始数据结构的指针
 *
 * @note   使用加速度计数据计算姿态角（pitch/roll/yaw）
 * @note   使用 ESP-DSP 库优化的平方根函数提升性能
 */
static void imu_read_data(t_sQMI8658 *p, t_sQMI8658_raw *p_raw)
{
    float temp;

    // 读取加速度计和陀螺仪的原始数据
    qmi8658_Read_AccAndGry(p_raw);

    /* ========== 姿态角计算（基于加速度计） ========== */

    // 1. 计算俯仰角（Pitch）：绕 Y 轴旋转（前后倾斜）
    // 公式：pitch = atan(acc_x / √(acc_y² + acc_z²)) × (180/π)
    // 假设：重力主要在 Z 轴方向
    float y2_z2 = (float)p_raw->acc_y_raw * (float)p_raw->acc_y_raw +
                  (float)p_raw->acc_z_raw * (float)p_raw->acc_z_raw;
    float sqrt_y2_z2 = dsps_sqrtf_f32_ansi(y2_z2); // ESP-DSP 优化的平方根函数
    temp = (float)p_raw->acc_x_raw / sqrt_y2_z2;
    p->pitch = atan(temp) * 57.29578f; // 57.29578 = 180/π（弧度转角度）

    // 2. 计算横滚角（Roll）：绕 X 轴旋转（左右倾斜）
    // 公式：roll = atan(acc_y / √(acc_x² + acc_z²)) × (180/π)
    float x2_z2 = (float)p_raw->acc_x_raw * (float)p_raw->acc_x_raw +
                  (float)p_raw->acc_z_raw * (float)p_raw->acc_z_raw;
    float sqrt_x2_z2 = dsps_sqrtf_f32_ansi(x2_z2); // ESP-DSP 优化的平方根函数
    temp = (float)p_raw->acc_y_raw / sqrt_x2_z2;
    p->roll = atan(temp) * 57.29578f;

    // 3. 计算偏航角（Yaw）：绕 Z 轴旋转（航向角）
    // 公式：yaw = atan(√(acc_x² + acc_y²) / acc_z) × (180/π)
    // 注意：仅基于加速度计的偏航角精度有限，建议结合陀螺仪或磁力计
    float x2_y2 = (float)p_raw->acc_x_raw * (float)p_raw->acc_x_raw +
                  (float)p_raw->acc_y_raw * (float)p_raw->acc_y_raw;
    float sqrt_x2_y2 = dsps_sqrtf_f32_ansi(x2_y2); // ESP-DSP 优化的平方根函数
    temp = sqrt_x2_y2 / (float)p_raw->acc_z_raw;
    p->yaw = atan(temp) * 57.29578f;

    /* ========== 原始数据转换为物理量 ========== */

    // 4. 加速度转换：原始值 → g（重力加速度）
    // 公式：acc = acc_raw × (量程 / ADC分辨率)
    // 量程：±4g（CTRL2配置），分辨率：32768（16位有符号整数：-32768 ~ +32767）
    p->acc_x = p_raw->acc_x_raw * ACC_SCALE / ADC_RESOLUTION; // 单位：g
    p->acc_y = p_raw->acc_y_raw * ACC_SCALE / ADC_RESOLUTION;
    p->acc_z = p_raw->acc_z_raw * ACC_SCALE / ADC_RESOLUTION;

    // 5. 角速度转换：原始值 → dps（度/秒）
    // 公式：gyr = gyr_raw × (量程 / ADC分辨率)
    // 量程：±512dps（CTRL3配置），分辨率：32768（16位有符号整数）
    p->gyr_x = p_raw->gyr_x_raw * GYRO_SCALE / ADC_RESOLUTION; // 单位：dps
    p->gyr_y = p_raw->gyr_y_raw * GYRO_SCALE / ADC_RESOLUTION;
    p->gyr_z = p_raw->gyr_z_raw * GYRO_SCALE / ADC_RESOLUTION;
}