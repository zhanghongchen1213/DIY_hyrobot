#!/usr/bin/env python3
"""
黄油机器人任务调度节点
实现：搜索黄油 → 导航抓取 → 返回起点
"""
import rclpy
from rclpy.node import Node
from rclpy.action import ActionClient
from enum import Enum
import math

from geometry_msgs.msg import PoseStamped, Twist
from nav2_msgs.action import NavigateToPose
from std_msgs.msg import String


class MissionState(Enum):
    """任务状态枚举"""
    IDLE = 0
    GO_TO_CENTER = 1
    SEARCHING = 2
    NAVIGATE_TO_BUTTER = 3
    GRASPING = 4
    RETURN_HOME = 5


class ButterMissionNode(Node):
    def __init__(self):
        super().__init__('butter_mission_node')

        # 参数声明
        self.declare_parameter('center_x', 0.0)
        self.declare_parameter('center_y', 0.0)
        self.declare_parameter('search_angular_speed', 0.5)

        self.center_x = self.get_parameter('center_x').value
        self.center_y = self.get_parameter('center_y').value
        self.search_speed = self.get_parameter('search_angular_speed').value

        # 状态变量
        self.state = MissionState.IDLE
        self.home_pose = None
        self.butter_detected = False
        self.butter_x = 0.0
        self.butter_y = 0.0

        # 订阅器
        self.detection_sub = self.create_subscription(
            String, '/detections', self.detection_callback, 10)
        self.pose_sub = self.create_subscription(
            PoseStamped, '/amcl_pose', self.pose_callback, 10)

        # 发布器
        self.cmd_vel_pub = self.create_publisher(Twist, '/cmd_vel', 10)
        self.joint_cmd_pub = self.create_publisher(String, '/joint_commands', 10)

        # Action客户端
        self.nav_client = ActionClient(self, NavigateToPose, '/navigate_to_pose')

        # 定时器
        self.timer = self.create_timer(0.1, self.state_machine_callback)

        self.get_logger().info('黄油任务调度节点已启动')
        self.get_logger().info(f'搜索中心点: ({self.center_x}, {self.center_y})')

    def detection_callback(self, msg):
        """处理目标检测结果"""
        # 假设消息格式: "butter,x,y,distance"
        if 'butter' in msg.data:
            parts = msg.data.split(',')
            if len(parts) >= 4:
                self.butter_detected = True
                self.butter_x = float(parts[1])
                self.butter_y = float(parts[2])
                self.get_logger().info(f'检测到黄油: ({self.butter_x}, {self.butter_y})')

    def pose_callback(self, msg):
        """记录当前位姿"""
        if self.state == MissionState.IDLE and self.home_pose is None:
            self.home_pose = msg
            self.get_logger().info('起点位姿已记录')
            self.state = MissionState.GO_TO_CENTER

    def state_machine_callback(self):
        """状态机主循环"""
        if self.state == MissionState.GO_TO_CENTER:
            self.go_to_center()
        elif self.state == MissionState.SEARCHING:
            self.search_butter()
        elif self.state == MissionState.NAVIGATE_TO_BUTTER:
            self.navigate_to_butter()
        elif self.state == MissionState.GRASPING:
            self.grasp_butter()
        elif self.state == MissionState.RETURN_HOME:
            self.return_home()

    def go_to_center(self):
        """前往搜索中心点"""
        goal_msg = NavigateToPose.Goal()
        goal_msg.pose.header.frame_id = 'map'
        goal_msg.pose.header.stamp = self.get_clock().now().to_msg()
        goal_msg.pose.pose.position.x = self.center_x
        goal_msg.pose.pose.position.y = self.center_y
        goal_msg.pose.pose.orientation.w = 1.0

        self.nav_client.wait_for_server()
        self.send_goal_future = self.nav_client.send_goal_async(
            goal_msg, feedback_callback=self.nav_feedback_callback)
        self.send_goal_future.add_done_callback(self.nav_goal_response_callback)

        self.state = MissionState.SEARCHING
        self.get_logger().info('前往搜索中心点')

    def search_butter(self):
        """360度旋转搜索黄油"""
        if self.butter_detected:
            # 停止旋转
            twist = Twist()
            self.cmd_vel_pub.publish(twist)
            self.state = MissionState.NAVIGATE_TO_BUTTER
            self.get_logger().info('检测到黄油，停止搜索')
        else:
            # 持续旋转
            twist = Twist()
            twist.angular.z = self.search_speed
            self.cmd_vel_pub.publish(twist)

    def navigate_to_butter(self):
        """导航到黄油位置"""
        goal_msg = NavigateToPose.Goal()
        goal_msg.pose.header.frame_id = 'map'
        goal_msg.pose.header.stamp = self.get_clock().now().to_msg()
        goal_msg.pose.pose.position.x = self.butter_x
        goal_msg.pose.pose.position.y = self.butter_y
        goal_msg.pose.pose.orientation.w = 1.0

        self.nav_client.send_goal_async(goal_msg)
        self.state = MissionState.GRASPING
        self.get_logger().info(f'导航到黄油: ({self.butter_x}, {self.butter_y})')

    def grasp_butter(self):
        """执行抓取动作"""
        # 发送舵机控制指令
        cmd = String()
        cmd.data = 'grasp'
        self.joint_cmd_pub.publish(cmd)

        self.get_logger().info('执行抓取动作')
        self.state = MissionState.RETURN_HOME

    def return_home(self):
        """返回起点"""
        if self.home_pose:
            goal_msg = NavigateToPose.Goal()
            goal_msg.pose = self.home_pose

            self.nav_client.send_goal_async(goal_msg)
            self.get_logger().info('返回起点')
            self.state = MissionState.IDLE

    def nav_feedback_callback(self, feedback_msg):
        """导航反馈回调"""
        pass

    def nav_goal_response_callback(self, future):
        """导航目标响应回调"""
        goal_handle = future.result()
        if not goal_handle.accepted:
            self.get_logger().warn('导航目标被拒绝')
            return

        self.get_logger().info('导航目标已接受')


def main(args=None):
    rclpy.init(args=args)
    node = ButterMissionNode()

    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        node.destroy_node()
        rclpy.shutdown()


if __name__ == '__main__':
    main()
