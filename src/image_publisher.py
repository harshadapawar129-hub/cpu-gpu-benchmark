#!/usr/bin/env python3
import rclpy
from rclpy.node import Node
from sensor_msgs.msg import Image
from cv_bridge import CvBridge
import numpy as np


class ImagePublisher(Node):

    def __init__(self):
        super().__init__('benchmark_image_publisher')

        self.publisher = self.create_publisher(
            Image, '/image', 10
        )

        self.bridge = CvBridge()

        self.width = 3840
        self.height = 2160

        # Generate a repeatable synthetic BGR image.
        rng = np.random.default_rng(42)

        self.image = rng.integers(
            0, 256,
            (self.height, self.width, 3),
            dtype=np.uint8
        )

        self.timer = self.create_timer(
            0.1, self.publish_image
        )

        self.get_logger().info(
            'Publishing 1280x720 images on /image'
        )

    def publish_image(self):

        msg = self.bridge.cv2_to_imgmsg(
            self.image, encoding='bgr8'
        )

        self.publisher.publish(msg)


def main(args=None):

    rclpy.init(args=args)

    node = ImagePublisher()

    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass

    node.destroy_node()
    rclpy.shutdown()


if __name__ == '__main__':
    main()
