#include "app_move_control.h"

static const char *TAG = "move_control";
static odom_t s_odom;
static TaskHandle_t s_odom_task_handle = NULL;

// 内部函数声明
static void update_odometry_internal(float rpm_left, float rpm_right, float dt_s);
static void odometry_update_task(void *arg);
static void yaw_to_quaternion(float yaw, quaternion_t *q);

/**
 * @brief 欧拉角转四元数 (仅处理Yaw)
 */
static void yaw_to_quaternion(float yaw, quaternion_t *q)
{
    // q = [cos(yaw/2), 0, 0, sin(yaw/2)]
    float half_yaw = yaw * 0.5f;
    q->w = cosf(half_yaw);
    q->x = 0.0f;
    q->y = 0.0f;
    q->z = sinf(half_yaw);
}

/**
 * @brief 内部里程计更新逻辑
 */
static void update_odometry_internal(float rpm_left, float rpm_right, float dt_s)
{
    if (dt_s <= 0.0f)
        return;

    // 1. 轮式里程计正运动学计算
    float wheel_v = 0.0f;
    float wheel_w = 0.0f;
    forward_kinematics(rpm_left, rpm_right, &wheel_v, &wheel_w);

    // 2. 获取IMU数据
    t_sQMI8658 imu_data;
    imu_data_resolution(&imu_data);

    // 将IMU数据转换并保存到 s_odom
    // IMU单位是 dps (度/秒)，转换为 rad/s
    float dps_to_rad = M_PI / 180.0f;
    s_odom.gyro_x = imu_data.gyr_x * dps_to_rad;
    s_odom.gyro_y = imu_data.gyr_y * dps_to_rad;
    s_odom.gyro_z = imu_data.gyr_z * dps_to_rad;

    // 3. 更新状态
    s_odom.linear_vel = wheel_v;
    s_odom.angular_vel = wheel_w;

    // 积分计算位置和角度
    // 航向角积分
    s_odom.angle += wheel_w * dt_s;

    // 归一化角度到 [-PI, PI]
    while (s_odom.angle > M_PI)
        s_odom.angle -= 2.0f * M_PI;
    while (s_odom.angle < -M_PI)
        s_odom.angle += 2.0f * M_PI;

    // 位置积分 (使用中点积分法或简单的欧拉积分)
    s_odom.x += wheel_v * cosf(s_odom.angle) * dt_s;
    s_odom.y += wheel_v * sinf(s_odom.angle) * dt_s;

    // --- IMU部分：独立积分用于计算四元数 ---
    // 定义一个静态变量来累积 IMU 的 Yaw 角
    static float s_imu_angle = 0.0f;
    s_imu_angle += s_odom.gyro_z * dt_s;

    // 归一化 IMU 角度
    while (s_imu_angle > M_PI)
        s_imu_angle -= 2.0f * M_PI;
    while (s_imu_angle < -M_PI)
        s_imu_angle += 2.0f * M_PI;

    // 4. 更新姿态四元数
    // 强制使用纯 IMU 积分得到的 Yaw 角
    yaw_to_quaternion(s_imu_angle, &s_odom.q);
}

/**
 * @brief 里程计更新任务
 */
static void odometry_update_task(void *arg)
{
    const TickType_t xPeriod = pdMS_TO_TICKS(10); // 10ms 周期
    TickType_t xLastWakeTime = xTaskGetTickCount();

    int64_t last_time_us = esp_timer_get_time();

    while (1)
    {
        // 绝对延时，确保周期准确
        vTaskDelayUntil(&xLastWakeTime, xPeriod);

        // 1. 计算精确的时间间隔 (dt)
        int64_t current_time_us = esp_timer_get_time();
        float dt_s = (float)(current_time_us - last_time_us) / 1000000.0f;
        last_time_us = current_time_us;

        // 2. 获取当前电机转速
        float rpm_left = 0.0f;
        float rpm_right = 0.0f;
        motor_get_rpm(MOTOR_A, &rpm_left);
        motor_get_rpm(MOTOR_B, &rpm_right);

        // 3. 执行里程计更新
        update_odometry_internal(rpm_left, rpm_right, dt_s);
    }
}

/**
 * @brief 初始化运动控制
 */
void app_move_control_init(void)
{
    memset(&s_odom, 0, sizeof(odom_t));
    // 初始化四元数 (w=1, x=0, y=0, z=0) 代表无旋转
    s_odom.q.w = 1.0f;
    s_odom.q.x = 0.0f;
    s_odom.q.y = 0.0f;
    s_odom.q.z = 0.0f;

    // 优先级设置为高于普通应用任务，但低于关键控制任务(如PID)
    xTaskCreatePinnedToCore(odometry_update_task, "odom_task", 4096, NULL, 5, &s_odom_task_handle, 1);
    ESP_LOGI(TAG, "运动控制及里程计任务已启动");
}

/**
 * @brief 逆运动学解析：线速度/角速度 -> 左右轮RPM
 * @note  公式:
 *        v_L = v - (w * L / 2)
 *        v_R = v + (w * L / 2)
 *        RPM = v_wheel / (PI * D) * 60
 */
void inverse_kinematics(float linear_vel, float angular_vel, float *rpm_left, float *rpm_right)
{
    // 计算左右轮线速度 (m/s)
    float vel_left = linear_vel - (angular_vel * WHEEL_BASE_M / 2.0f);
    float vel_right = linear_vel + (angular_vel * WHEEL_BASE_M / 2.0f);

    // 转换为RPM
    // RPM = (m/s) / (m/rev) * 60 (s/min)
    if (rpm_left)
    {
        *rpm_left = (vel_left / WHEEL_CIRCUMFERENCE_M) * 60.0f;
    }
    if (rpm_right)
    {
        *rpm_right = (vel_right / WHEEL_CIRCUMFERENCE_M) * 60.0f;
    }
}

/**
 * @brief 正运动学解析：左右轮RPM -> 线速度/角速度
 * @note  公式:
 *        v_L = RPM_L * (PI * D) / 60
 *        v_R = RPM_R * (PI * D) / 60
 *        v = (v_R + v_L) / 2
 *        w = (v_R - v_L) / L
 */
void forward_kinematics(float rpm_left, float rpm_right, float *linear_vel, float *angular_vel)
{
    // 计算左右轮线速度 (m/s)
    float vel_left = (rpm_left * WHEEL_CIRCUMFERENCE_M) / 60.0f;
    float vel_right = (rpm_right * WHEEL_CIRCUMFERENCE_M) / 60.0f;

    // 计算整车线速度和角速度
    if (linear_vel)
    {
        *linear_vel = (vel_right + vel_left) / 2.0f;
    }
    if (angular_vel)
    {
        *angular_vel = (vel_right - vel_left) / WHEEL_BASE_M;
    }
}

/**
 * @brief 获取里程计信息
 */
odom_t *get_odometry(void)
{
    return &s_odom;
}