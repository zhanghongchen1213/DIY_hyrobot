#!/usr/bin/env python3
"""
动态TF发布器
用于发布带当前时间戳的静态TF到/tf话题（而不是/tf_static）
解决slam_toolbox和其他节点无法使用时间戳为0的静态TF的问题
"""

import rclpy
from rclpy.node import Node
from geometry_msgs.msg import TransformStamped
from tf2_ros import TransformBroadcaster


class DynamicTFPublisher(Node):
    def __init__(self):
        super().__init__('dynamic_tf_publisher')

        # 创建TF广播器
        self.tf_broadcaster = TransformBroadcaster(self)

        # 创建定时器，以10Hz频率发布TF
        self.timer = self.create_timer(0.1, self.publish_tf)

        self.get_logger().info('Dynamic TF Publisher started: base_link -> radar_Link, spine_Link -> camera_Link')

    def publish_tf(self):
        """发布多个静态TF转换（使用当前时间戳）"""
        transforms = []

        # 1. base_link -> radar_Link (来自URDF radar_joint)
        t1 = TransformStamped()
        t1.header.stamp = self.get_clock().now().to_msg()
        t1.header.frame_id = 'base_link'
        t1.child_frame_id = 'radar_Link'
        t1.transform.translation.x = 0.029032
        t1.transform.translation.y = 0.0
        t1.transform.translation.z = 0.0532
        t1.transform.rotation.x = 0.0
        t1.transform.rotation.y = 0.0
        t1.transform.rotation.z = 0.0
        t1.transform.rotation.w = 1.0
        transforms.append(t1)

        # 2. spine_Link -> camera_Link (来自URDF camera_joint)
        t2 = TransformStamped()
        t2.header.stamp = self.get_clock().now().to_msg()
        t2.header.frame_id = 'spine_Link'
        t2.child_frame_id = 'camera_Link'
        t2.transform.translation.x = 0.069858
        t2.transform.translation.y = 0.024995
        t2.transform.translation.z = 0.11868
        t2.transform.rotation.x = 0.0
        t2.transform.rotation.y = 0.0
        t2.transform.rotation.z = 0.0
        t2.transform.rotation.w = 1.0
        transforms.append(t2)

        # 批量发布所有TF
        self.tf_broadcaster.sendTransform(transforms)


def main(args=None):
    rclpy.init(args=args)
    node = DynamicTFPublisher()

    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        node.destroy_node()
        rclpy.shutdown()


if __name__ == '__main__':
    main()
