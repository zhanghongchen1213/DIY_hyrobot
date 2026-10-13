import rclpy
from rclpy.node import Node
from sensor_msgs.msg import JointState, Joy, Imu
from nav_msgs.msg import Odometry
from std_msgs.msg import UInt16, UInt8
from pid_debug_interfaces.msg import MotorControl, MotorStatus
import serial
import threading
import time
import ctypes
import queue
import math

# 预计算常量
DEG_TO_RAD = math.pi / 180.0

# ==========================================
# 数据包结构定义 (与 C 代码保持一致)
# ==========================================

class UartUplinkPacket(ctypes.LittleEndianStructure):
    """
    @brief 上行数据包结构（设备上报给上位机）
    """
    _pack_ = 1
    _fields_ = [
        ("start_flag", ctypes.c_uint8),      # 0xAA
        ("chat_gpt_count", ctypes.c_uint16),
        
        # --- 电机与PID状态 (调试用) ---
        ("left_target_speed", ctypes.c_float),
        ("right_target_speed", ctypes.c_float),
        ("left_actual_speed", ctypes.c_float),
        ("right_actual_speed", ctypes.c_float),
        ("left_kp", ctypes.c_float),
        ("left_ki", ctypes.c_float),
        ("left_kd", ctypes.c_float),
        ("right_kp", ctypes.c_float),
        ("right_ki", ctypes.c_float),
        ("right_kd", ctypes.c_float),

        # --- 里程计与姿态 (用于SLAM/Nav/RVIZ) ---
        ("position_x", ctypes.c_float),  # m
        ("position_y", ctypes.c_float),  # m
        ("theta_wheel", ctypes.c_float), # rad
        ("linear_vel_x", ctypes.c_float), # m/s
        ("angular_vel_wheel", ctypes.c_float), # rad/s

        # --- IMU 传感器部分 ---
        ("gyro_x", ctypes.c_float),
        ("gyro_y", ctypes.c_float),
        ("gyro_z", ctypes.c_float),

        # 姿态四元数 (用于数字孪生)
        ("q_w", ctypes.c_float),
        ("q_x", ctypes.c_float),
        ("q_y", ctypes.c_float),
        ("q_z", ctypes.c_float),

        # --- 关节状态 (用于Joint State) ---
        ("servo_a_angle", ctypes.c_int16), 
        ("servo_b_angle", ctypes.c_int16),
        ("servo_c_angle", ctypes.c_int16),

        ("timestamp", ctypes.c_uint32),
        ("end_flag", ctypes.c_uint8),        # 0x55
    ]
    
    START_FLAG = 0xAA
    END_FLAG = 0x55


class UartDownlinkPacket(ctypes.LittleEndianStructure):
    """
    @brief 下行数据包结构（上位机下发给设备）
    用于下发状态标志位
    """
    _pack_ = 1
    _fields_ = [
        ("start_flag", ctypes.c_uint8),      # 0xAA
        
        ("audio_stream_flag", ctypes.c_uint8), # 音频状态: 0=无, 1=ASR结束, 2=TTS结束, 3=HTTPS下发完成
        ("left_target_speed", ctypes.c_float),  # 左电机目标速度
        ("right_target_speed", ctypes.c_float), # 右电机目标速度
        ("left_kp", ctypes.c_float),            # 左电机PID比例系数
        ("left_ki", ctypes.c_float),            # 左电机PID积分系数
        ("left_kd", ctypes.c_float),            # 左电机PID微分系数
        ("right_kp", ctypes.c_float),           # 右电机PID比例系数
        ("right_ki", ctypes.c_float),           # 右电机PID积分系数
        ("right_kd", ctypes.c_float),           # 右电机PID微分系数

        ("linear_vel", ctypes.c_float),         # 线速度 (m/s)
        ("angular_vel", ctypes.c_float),        # 角速度 (rad/s)
        ("servo_a_angle", ctypes.c_float),      # 舵机A角度 (rad)
        ("servo_b_angle", ctypes.c_float),      # 舵机B角度 (rad)
        ("servo_c_angle", ctypes.c_float),      # 舵机C角度 (rad)

        ("timestamp", ctypes.c_uint32),        # 时间戳 (ms)
        ("end_flag", ctypes.c_uint8),        # 0x66
    ]
    
    START_FLAG = 0xAA
    END_FLAG = 0x66


class UartNode(Node):
    """
    串口通讯节点（UART）
    事件驱动模式：收到 audio_stream_flag 话题更新时立即下发串口指令。
    """

    def __init__(self):
        super().__init__(
            'uart',
            allow_undeclared_parameters=False,
            automatically_declare_parameters_from_overrides=True,
        )
        
        # 1. 参数读取
        self.port = self._require_str('port')
        self.baud = self._require_int('baud')
        self.chat_topic = self._require_str('pub_chat_topic')
        self.motor_status_topic = self._require_str('pub_motor_status_topic')
        self.wheel_odom_topic = self._require_str('pub_wheel_odom_topic')
        self.imu_data_topic = self._require_str('pub_imu_data_topic')
        self.audio_status_topic = self._require_str('sub_audio_status_topic')
        self.motor_control_topic = self._require_str('sub_motor_control_topic')
        self.joy_topic = self._require_str('sub_joy_topic')

        # 2. 通信资源初始化
        self._ser = None
        self._serial_lock = threading.Lock()
        self._buf = bytearray()
        self._stop = threading.Event()
        self.send_queue = queue.Queue()
        
        # Joy Control State
        self.joy_servo_a = 0.0
        self.joy_servo_b = 0.0
        self.joy_servo_c = 0.0
        self.last_buttons = []
        self._buttons_len = 0  # 缓存按钮数组长度
        
        # 3. ROS 接口
        # [Pub] 上行
        self.pub_chat = self.create_publisher(UInt16, self.chat_topic, 10)
        self.pub_motor_status = self.create_publisher(MotorStatus, self.motor_status_topic, 10)
        self.pub_joint_state = self.create_publisher(JointState, 'joint_states', 10)

        self.pub_odom = self.create_publisher(Odometry, self.wheel_odom_topic, 10)
        self.pub_imu = self.create_publisher(Imu, self.imu_data_topic, 10)

        # 预分配消息对象 (避免回调中频繁创建)
        self._downlink_packet = UartDownlinkPacket()
        self._joint_msg = JointState()
        self._joint_msg.name = ['right_arm_joint', 'left_arm_joint', 'spine_joint']
        self._joint_msg.position = [0.0, 0.0, 0.0]
        self._odom_msg = Odometry()
        self._odom_msg.header.frame_id = "odom"
        self._odom_msg.child_frame_id = "base_link"
        self._imu_msg = Imu()
        self._imu_msg.header.frame_id = "base_link"
        self._motor_status_msg = MotorStatus()
        self._chat_msg = UInt16()  # 预分配 chat 消息

        # [Sub] 下行
        self.sub_audio_status = self.create_subscription(
            UInt8, self.audio_status_topic, self._on_audio_status, 10)
        self.sub_motor_control = self.create_subscription(
            MotorControl, self.motor_control_topic, self._on_motor_control, 10)
        self.sub_joy = self.create_subscription(
            Joy, self.joy_topic, self._on_joy, 10)
        
        # 4. 启动线程
        self._open()
        self._read_thread = threading.Thread(target=self._run_read, daemon=True)
        self._read_thread.start()
        self._write_thread = threading.Thread(target=self._run_write, daemon=True)
        self._write_thread.start()
        
        self.get_logger().info(f'UART 节点启动。事件驱动模式。端口: {self.port}')

    def _on_audio_status(self, msg: UInt8):
        """
        ROS 回调：收到音频状态更新，立即组装下行包发送。
        """
        flag = msg.data
        # 复用预分配的下行包对象
        packet = UartDownlinkPacket()
        packet.start_flag = UartDownlinkPacket.START_FLAG
        packet.audio_stream_flag = flag
        packet.timestamp = int(time.time() * 1000) & 0xFFFFFFFF
        packet.end_flag = UartDownlinkPacket.END_FLAG

        self.send_queue.put(packet)

    def _on_joy(self, msg: Joy):
        """
        ROS 回调：收到手柄数据，解析并控制
        """
        # 1. 解析 Axes (前4个)
        linear = 0.0
        angular = 0.0
        if len(msg.axes) >= 4:
            linear = msg.axes[1] * 0.11  # Max 0.11 m/s
            angular = msg.axes[3] * 1.76 # Max 1.76 rad/s

        # 2. 解析 Buttons (前6个) - 优化：避免每次创建新列表
        buttons = msg.buttons
        btn_len = len(buttons)
        if self._buttons_len != btn_len:
            self.last_buttons = [0] * btn_len
            self._buttons_len = btn_len

        # 内联检查按钮按下（避免函数调用开销）
        last = self.last_buttons
        if btn_len > 0 and buttons[0] == 1 and last[0] == 0:
            self.joy_servo_a += 5.0
        if btn_len > 1 and buttons[1] == 1 and last[1] == 0:
            self.joy_servo_a -= 5.0
        if btn_len > 2 and buttons[2] == 1 and last[2] == 0:
            self.joy_servo_b += 5.0
        if btn_len > 3 and buttons[3] == 1 and last[3] == 0:
            self.joy_servo_b -= 5.0
        if btn_len > 4 and buttons[4] == 1 and last[4] == 0:
            self.joy_servo_c += 5.0
        if btn_len > 5 and buttons[5] == 1 and last[5] == 0:
            self.joy_servo_c -= 5.0

        # 原地更新而非创建新列表
        for i in range(btn_len):
            last[i] = buttons[i]

        # 3. 发送下行包
        packet = UartDownlinkPacket()
        packet.start_flag = UartDownlinkPacket.START_FLAG
        packet.audio_stream_flag = 0
        
        packet.linear_vel = float(linear)
        packet.angular_vel = float(angular)
        
        packet.servo_a_angle = float(self.joy_servo_a)
        packet.servo_b_angle = float(self.joy_servo_b)
        packet.servo_c_angle = float(self.joy_servo_c)
        
        packet.timestamp = int(time.time() * 1000) & 0xFFFFFFFF
        packet.end_flag = UartDownlinkPacket.END_FLAG
        
        # DEBUG: 打印调试信息
        self.get_logger().debug(
            f"[Joy Debug] Linear={packet.linear_vel:.2f}, Angular={packet.angular_vel:.2f}, "
            f"Servo A={packet.servo_a_angle:.2f}, Servo B={packet.servo_b_angle:.2f}, Servo C={packet.servo_c_angle:.2f}"
        )
        
        # 暂时注释掉串口下发，等待调试确认
        self.send_queue.put(packet)

    def _on_motor_control(self, msg: MotorControl):
        """
        ROS 回调：收到电机控制指令，组装下行包发送。
        """
        packet = UartDownlinkPacket()
        packet.start_flag = UartDownlinkPacket.START_FLAG
        packet.audio_stream_flag = 0 # 提供默认值即可
        packet.left_target_speed = msg.left_target_speed
        packet.right_target_speed = msg.right_target_speed
        packet.left_kp = msg.left_kp
        packet.left_ki = msg.left_ki
        packet.left_kd = msg.left_kd
        packet.right_kp = msg.right_kp
        packet.right_ki = msg.right_ki
        packet.right_kd = msg.right_kd
        
        packet.linear_vel = msg.linear_vel
        packet.angular_vel = msg.angular_vel
        packet.servo_a_angle = msg.servo_a_angle
        packet.servo_b_angle = msg.servo_b_angle
        packet.servo_c_angle = msg.servo_c_angle

        packet.timestamp = int(time.time() * 1000) & 0xFFFFFFFF
        packet.end_flag = UartDownlinkPacket.END_FLAG
        
        self.send_queue.put(packet)
        
        # 打印发送的电机控制指令
        self.get_logger().info(f"发送电机控制指令: "
                               f"左目标速度={packet.left_target_speed:.2f}, "
                               f"右目标速度={packet.right_target_speed:.2f}, "
                               f"左PID={packet.left_kp:.2f},{packet.left_ki:.2f},{packet.left_kd:.2f}, "
                               f"右PID={packet.right_kp:.2f},{packet.right_ki:.2f},{packet.right_kd:.2f}, "
                               f"线速度={packet.linear_vel:.2f}, 角速度={packet.angular_vel:.2f}, "
                               f"舵机={packet.servo_a_angle:.2f},{packet.servo_b_angle:.2f},{packet.servo_c_angle:.2f}")

    def _open(self):
        with self._serial_lock:
            try:
                self._ser = serial.Serial(
                    self.port,
                    self.baud,
                    bytesize=serial.EIGHTBITS,
                    parity=serial.PARITY_NONE,
                    stopbits=serial.STOPBITS_ONE,
                    timeout=0.01, # 读超时
                    write_timeout=1,
                )
                self.get_logger().info(f'已打开串口 {self.port}，波特率 {self.baud}')
            except Exception as e:
                self.get_logger().error(f'打开串口失败：{e}')
                self._ser = None

    def _run_write(self):
        """写线程：从队列获取数据包并写入串口"""
        while not self._stop.is_set():
            try:
                # 阻塞等待，超时方便响应停止信号
                packet = self.send_queue.get(timeout=0.5)
                
                if self._ser and self._ser.is_open:
                    data = bytes(packet)
                    with self._serial_lock:
                        self._ser.write(data)
                    self.get_logger().debug(f"发送下行数据: {[hex(b) for b in data]}")
                else:
                    self.get_logger().warning("串口未打开，丢弃发送数据")
                
                self.send_queue.task_done()
            except queue.Empty:
                continue
            except Exception as e:
                self.get_logger().error(f"写串口异常: {e}")

    def _run_read(self):
        """读线程：循环读取并解析"""
        while not self._stop.is_set():
            if self._ser is None or not self._ser.is_open:
                time.sleep(1.0)
                self._open()
                continue
                
            try:
                data = b''
                if self._ser.in_waiting:
                    with self._serial_lock:
                        data = self._ser.read(self._ser.in_waiting)
                
                if data:
                    self._buf.extend(data)
                    self._parse()
                else:
                    time.sleep(0.005)
            except Exception as e:
                self.get_logger().error(f'读串口异常: {e}')
                time.sleep(1.0)

    def _parse(self):
        """解析上行数据包 (UplinkPacket)"""
        b = self._buf
        packet_size = ctypes.sizeof(UartUplinkPacket)
        
        while len(b) >= packet_size:
            # 1. 寻找起始标志
            try:
                idx = b.index(UartUplinkPacket.START_FLAG)
            except ValueError:
                b.clear()
                break
                
            if idx > 0:
                del b[:idx] # 丢弃头部噪声
                continue # 重新检查长度
            
            # 此时 b[0] == START_FLAG
            if len(b) < packet_size:
                break # 数据不够完整包
            
            # 2. 检查结束标志
            if b[packet_size - 1] != UartUplinkPacket.END_FLAG:
                del b[0]
                continue
                
            # 3. 解析数据
            try:
                # 直接从 buffer 解析，避免额外拷贝
                packet = UartUplinkPacket.from_buffer_copy(b[:packet_size])

                # 获取当前时间戳，用于同步
                current_time = self.get_clock().now().to_msg()

                # 发布话题 - 使用预分配对象
                self._chat_msg.data = packet.chat_gpt_count
                self.pub_chat.publish(self._chat_msg)

                # 1. 发布 MotorStatus (电机状态，用于PID调试) - 使用预分配对象
                status_msg = self._motor_status_msg
                status_msg.left_target_speed = packet.left_target_speed
                status_msg.right_target_speed = packet.right_target_speed
                status_msg.left_actual_speed = packet.left_actual_speed
                status_msg.right_actual_speed = packet.right_actual_speed
                status_msg.left_kp = packet.left_kp
                status_msg.left_ki = packet.left_ki
                status_msg.left_kd = packet.left_kd
                status_msg.right_kp = packet.right_kp
                status_msg.right_ki = packet.right_ki
                status_msg.right_kd = packet.right_kd
                self.pub_motor_status.publish(status_msg)

                # 2. 发布 JointState (关节状态) - 使用预分配对象
                joint_msg = self._joint_msg
                joint_msg.header.stamp = current_time
                # 使用预计算常量 DEG_TO_RAD
                joint_msg.position[0] = float(packet.servo_a_angle) * DEG_TO_RAD
                joint_msg.position[1] = float(packet.servo_b_angle) * DEG_TO_RAD
                joint_msg.position[2] = float(packet.servo_c_angle) * DEG_TO_RAD
                self.pub_joint_state.publish(joint_msg)

                # 3. 发布 Odometry - 使用预分配对象
                odom_msg = self._odom_msg
                odom_msg.header.stamp = current_time

                # 位置 (m)
                odom_msg.pose.pose.position.x = float(packet.position_x)
                odom_msg.pose.pose.position.y = float(packet.position_y)
                odom_msg.pose.pose.position.z = 0.0

                # 姿态 (Theta -> Quaternion)
                half_theta = float(packet.theta_wheel) * 0.5
                odom_msg.pose.pose.orientation.z = math.sin(half_theta)
                odom_msg.pose.pose.orientation.w = math.cos(half_theta)

                # 速度
                odom_msg.twist.twist.linear.x = float(packet.linear_vel_x)
                odom_msg.twist.twist.angular.z = float(packet.angular_vel_wheel)

                self.pub_odom.publish(odom_msg)

                # 4. 发布 IMU - 使用预分配对象
                imu_msg = self._imu_msg
                imu_msg.header.stamp = current_time

                # 角速度 (rad/s)
                imu_msg.angular_velocity.x = float(packet.gyro_x)
                imu_msg.angular_velocity.y = float(packet.gyro_y)
                imu_msg.angular_velocity.z = -float(packet.gyro_z)  # 取反适配 ROS 坐标系

                # 姿态 (四元数) - 取反 z 适配 ROS ENU 坐标系
                imu_msg.orientation.w = float(packet.q_w)
                imu_msg.orientation.x = float(packet.q_x)
                imu_msg.orientation.y = float(packet.q_y)
                imu_msg.orientation.z = -float(packet.q_z)

                self.get_logger().debug(f"解析上行: gyro_x={packet.gyro_x:.2f}, "
                                       f"gyro_y={packet.gyro_y:.2f}, "
                                       f"gyro_z={packet.gyro_z:.2f}, "
                                       f"q_w={packet.q_w:.2f}, "
                                       f"q_x={packet.q_x:.2f}, "
                                       f"q_y={packet.q_y:.2f}, "
                                       f"q_z={packet.q_z:.2f}, "
                                       f"时间ms={packet.timestamp:.2f}")

                self.pub_imu.publish(imu_msg)

                self.get_logger().debug(
                    f"解析上行: Count={packet.chat_gpt_count}, TS={packet.timestamp}"
                )
                self.get_logger().debug(f"解析上行: 左目标速度={packet.left_target_speed:.2f}, "
                                       f"右目标速度={packet.right_target_speed:.2f}, "
                                       f"左实际速度={packet.left_actual_speed:.2f}, "
                                       f"右实际速度={packet.right_actual_speed:.2f}, "
                                       f"左PID={packet.left_kp:.2f},{packet.left_ki:.2f},{packet.left_kd:.2f}, "
                                       f"右PID={packet.right_kp:.2f},{packet.right_ki:.2f},{packet.right_kd:.2f}")
    
                
                # 4. 移除已解析的数据
                del b[:packet_size]
                
            except Exception as e:
                self.get_logger().error(f"解析包内容异常: {e}")
                del b[0] # 防死循环

    def destroy_node(self):
        self._stop.set()
        if self._ser:
            self._ser.close()
        return super().destroy_node()

    def _require_str(self, name: str) -> str:
        p = self.get_parameter(name)
        v = p.value
        if isinstance(v, str) and len(v) > 0:
            return v
        raise RuntimeError(f"缺少必需字符串参数: {name}")

    def _require_int(self, name: str) -> int:
        p = self.get_parameter(name)
        v = p.value
        if isinstance(v, int):
            return v
        raise RuntimeError(f"缺少必需整数参数: {name}")


def main(args=None):
    rclpy.init(args=args)
    node = UartNode()
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        node.destroy_node()
        rclpy.shutdown()
