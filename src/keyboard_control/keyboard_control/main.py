import rclpy
from rclpy.node import Node
from pid_debug_interfaces.msg import MotorControl
import sys
import select
import termios
import tty
import threading
import time

class KeyboardControlNode(Node):
    def __init__(self):
        super().__init__(
            'keyboard_control',
            allow_undeclared_parameters=False,
            automatically_declare_parameters_from_overrides=True,
        )
        
        # 1. 声明参数
        self.control_topic = self._require_str('control_topic', '/uart/motor_control')
        self.check_rate = self._require_float('check_rate', 100.0) # Hz, 检查键盘频率
        self.timeout_sec = self._require_float('timeout_sec', 0.1) # 超过此时间无按键则归零

        # 2. 发布者
        self.pub_control = self.create_publisher(MotorControl, self.control_topic, 10)
        self._control_msg = MotorControl()  # 预分配消息对象
        
        # 3. 状态变量
        self.linear_vel = 0.0
        self.angular_vel = 0.0
        self.last_linear_vel = 0.0
        self.last_angular_vel = 0.0
        self.last_key_time = 0.0
        self.running = True
        self.first_run = True
        
        # 4. 启动键盘监听线程
        self.settings = termios.tcgetattr(sys.stdin)
        self.thread = threading.Thread(target=self._key_loop)
        self.thread.start()
        
        self.get_logger().info("""
键盘控制节点已启动。
控制键位：
  W: 前进    S: 后退
  A: 左转    D: 右转
  Q: 前左    E: 前右
  Z: 后左    C: 后右
松开即停。Ctrl+C退出。
""")

    def _require_str(self, name: str, default: str) -> str:
        # 如果参数不存在，使用默认值
        if not self.has_parameter(name):
            self.declare_parameter(name, default)
        
        p = self.get_parameter(name)
        v = p.value
        if isinstance(v, str) and len(v) > 0:
            return v
        return default

    def _require_float(self, name: str, default: float) -> float:
        if not self.has_parameter(name):
            self.declare_parameter(name, default)
        p = self.get_parameter(name)
        v = p.value
        if isinstance(v, (float, int)):
            return float(v)
        return default

    def _key_loop(self):
        try:
            tty.setcbreak(sys.stdin.fileno())
            while self.running and rclpy.ok():
                # 阻塞读取，超时时间极短，提高响应速度
                if select.select([sys.stdin], [], [], 0.005)[0]:
                    key = sys.stdin.read(1)
                    self._process_key(key)
                else:
                    self._check_timeout()
                
                # 持续发布当前状态
                self._publish_control()
                # 简单限频，但要保证足够快
                time.sleep(1.0 / self.check_rate)
                
        except Exception as e:
            self.get_logger().error(f"键盘监听异常: {e}")
        finally:
            termios.tcsetattr(sys.stdin, termios.TCSADRAIN, self.settings)

    def _process_key(self, key):
        self.last_key_time = time.time()
        
        if key == 'w':
            self.linear_vel = 0.15
            self.angular_vel = 0.0
        elif key == 's':
            self.linear_vel = -0.15
            self.angular_vel = 0.0
        elif key == 'a':
            self.linear_vel = 0.0
            self.angular_vel = 2.0
        elif key == 'd':
            self.linear_vel = 0.0
            self.angular_vel = -2.0
        elif key == 'q': # W + A (前左)
            self.linear_vel = 0.15
            self.angular_vel = 2.0
        elif key == 'e': # W + D (前右)
            self.linear_vel = 0.15
            self.angular_vel = -2.0
        elif key == 'z': # S + A (后左)
            self.linear_vel = -0.15
            self.angular_vel = 2.0
        elif key == 'c': # S + D (后右)
            self.linear_vel = -0.15
            self.angular_vel = -2.0
        elif key == '\x03': # Ctrl+C
            self.running = False
        else:
            # 其他按键，视为松开WASD? 或者保持不变？
            # 题目要求：用户按住W...松开立即为0
            # 这里的逻辑是：只要有 W 键进来，就设为 50。
            # 如果按了其他键，可能干扰，这里简单处理：非WASD视为无操作（会触发超时归零）
            pass

    def _check_timeout(self):
        # 如果距离上次有效按键超过阈值，归零
        if time.time() - self.last_key_time > self.timeout_sec:
            self.linear_vel = 0.0
            self.angular_vel = 0.0

    def _publish_control(self):
        # 仅当数值发生变化时才发布
        if (self.first_run or
            abs(self.linear_vel - self.last_linear_vel) > 1e-5 or
            abs(self.angular_vel - self.last_angular_vel) > 1e-5):

            # 使用预分配的消息对象
            msg = self._control_msg
            msg.linear_vel = float(self.linear_vel)
            msg.angular_vel = float(self.angular_vel)
            # 其他字段默认为0
            self.pub_control.publish(msg)
            
            self.last_linear_vel = self.linear_vel
            self.last_angular_vel = self.angular_vel
            self.first_run = False

    def destroy_node(self):
        self.running = False
        if self.thread.is_alive():
            self.thread.join()
        termios.tcsetattr(sys.stdin, termios.TCSADRAIN, self.settings)
        super().destroy_node()

def main(args=None):
    rclpy.init(args=args)
    node = KeyboardControlNode()
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        node.destroy_node()
        rclpy.shutdown()