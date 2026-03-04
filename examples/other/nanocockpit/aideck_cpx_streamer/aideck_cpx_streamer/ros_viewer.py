#!/usr/bin/env python3
#
# ros_viewer.py
# Elia Cereda <elia.cereda@idsia.ch>
# Jerome Guzzi <jerome.guzzi@idsia.ch>
#
# Copyright (C) 2022-2025 IDSIA, USI-SUPSI
# 
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
# 
# This software is based on the following publication:
#    E. Cereda, A. Giusti, D. Palossi. "NanoCockpit: Performance-optimized 
#    Application Framework for AI-based Autonomous Nanorobotics"
# We kindly ask for a citation if you use in academic work.
#

# Workaround: import cv2 before cv_bridge to fix exception on arm64
# Source: https://github.com/ros-perception/vision_opencv/issues/339#issuecomment-831763376
import cv2

import rclpy
from rclpy.node import Node
from rclpy.time import Time
from collections import deque
from cv_bridge import CvBridge
from geometry_msgs.msg import AccelWithCovarianceStamped
from nav_msgs.msg import Odometry
from sensor_msgs.msg import Image
from threading import Thread

from .cpx import StreamerClient
from .utils import create_dataset_dir, FrameSaver, InferenceSaver
from .utils.quatcompress import quatdecompress
from .utils.ros import msg_to_array, array_to_msg
from .utils.thread_pool import ThreadPool

from aideck_cpx_msgs.msg import CPXPacket, ImageMetadata, Inference

class ROSViewer(Node):
    def __init__(self) -> None:
        super().__init__('ros_viewer')

        host = self.declare_parameter('host', rclpy.Parameter.Type.STRING).value
        port = self.declare_parameter('port', 5000).value
        save_dir = self.declare_parameter('save_dir', '').value

        self.client = StreamerClient(host=host, port=port, udp_send=False, log_fn=self.log)
        # self.client.cpx.add_callback(self.cpx_callback)

        self.cpx_pub = self.create_publisher(CPXPacket, "cpx", 100)

        self.bridge = CvBridge()
        self.image_pub = self.create_publisher(Image, "image_raw", 1)
        self.metadata_pub = self.create_publisher(ImageMetadata, "image_metadata", 1)
        self.odom_pub = self.create_publisher(Odometry, "image_odom", 1)
        self.accel_pub = self.create_publisher(AccelWithCovarianceStamped, "image_accel", 1)

        self.tof_pub = self.create_publisher(Image, "tof_raw", 1)

        self.inference_pub = self.create_publisher(Inference, "inference_onboard", 1)
        self.inference_sub = self.create_subscription(Inference, "inference", self.inference_callback, 1)

        self.metadata_queue = deque(maxlen=1000)

        self.pool = ThreadPool(n_workers=1)

        self.save = save_dir is not None
        if self.save:
            dataset_dir = create_dataset_dir(save_dir)
            self.frame_saver = FrameSaver(dataset_dir)
            self.inference_saver = InferenceSaver(dataset_dir)

        self.thread = Thread(target=self.main, daemon=True)
        self.thread.start()

    def main(self) -> None:
        self.pool.start()

        if self.save:
            self.frame_saver.open()
            self.inference_saver.open()

        for seq, (frame, tof_frame, metadata) in enumerate(self.client.receive()):
            now = self.get_clock().now().to_msg()

            self.push_metadata(now, metadata)

            msg = self.bridge.cv2_to_imgmsg(frame)
            msg.header.stamp = now
            self.image_pub.publish(msg)

            if tof_frame is not None:
                msg = self.bridge.cv2_to_imgmsg(tof_frame)
                msg.header.stamp = now
                self.tof_pub.publish(msg)
            
            msg = self.metadata_to_msg(metadata)
            msg.header.stamp = now
            self.metadata_pub.publish(msg)

            msg = self.state_to_odom(metadata.state)
            msg.header.stamp = now
            self.odom_pub.publish(msg)

            msg = self.state_to_accel(metadata.state)
            msg.header.stamp = now
            self.accel_pub.publish(msg)

            inference_timestamp = metadata.inference.stm32_timestamp
            if inference_timestamp != 0:
                self.pool.try_run(self.send_reply, metadata, None)

                stamp, _ = self.pop_metadata(stm32_timestamp=inference_timestamp)

                if stamp is not None:
                    msg = self.onboard_inference_to_msg(metadata.inference)
                    msg.header.stamp = stamp
                    self.inference_pub.publish(msg)

            if self.save:
                self.frame_saver.save(frame, tof_frame, metadata)
        
        if self.save:
            self.frame_saver.close()
            self.inference_saver.close()

    def log(self, message='', end='\n'):
        self.get_logger().info(f'{message}{end}')

    def state_to_odom(self, state):
        msg = Odometry()
        msg.header.frame_id = "cf/odom"
        msg.child_frame_id = "cf/base_link"
        
        msg.pose.pose.position.x = state.x / 1000
        msg.pose.pose.position.y = state.y / 1000
        msg.pose.pose.position.z = state.z / 1000
        
        q = quatdecompress(state.quat).astype(float)
        msg.pose.pose.orientation.x = q[0]
        msg.pose.pose.orientation.y = q[1]
        msg.pose.pose.orientation.z = q[2]
        msg.pose.pose.orientation.w = q[3]

        msg.twist.twist.linear.x = state.vx / 1000
        msg.twist.twist.linear.y = state.vy / 1000
        msg.twist.twist.linear.z = state.vz / 1000

        # TODO: double check reference frame
        msg.twist.twist.angular.x = state.rateRoll / 1000
        msg.twist.twist.angular.y = state.ratePitch / 1000
        msg.twist.twist.angular.z = state.rateYaw / 1000
        
        return msg
    
    def state_to_accel(self, state):
        msg = AccelWithCovarianceStamped()
        msg.header.frame_id = "cf/odom"

        msg.accel.accel.linear.x = state.ax / 1000
        msg.accel.accel.linear.y = state.ay / 1000
        msg.accel.accel.linear.z = state.az / 1000

        return msg
    
    def metadata_to_msg(self, metadata):
        msg = ImageMetadata()
        msg.metadata_version = metadata.metadata_version
        msg.frame_id = metadata.frame_id

        msg.frame_gap8_timestamp = metadata.frame_timestamp
        msg.state_gap8_timestamp = metadata.state_timestamp
        msg.state_stm32_timestamp = metadata.state.timestamp
        
        msg.reply_frame_gap8_timestamp = metadata.reply_frame_timestamp
        msg.reply_recv_gap8_timestamp = metadata.reply_recv_timestamp

        # TODO: publish the rest of the fields

        return msg

    def onboard_inference_to_msg(self, inference):
        msg = Inference()
        array_to_msg([inference.x, inference.y, inference.z, inference.phi], msg.output)
        msg.inference_time = 0.0

        return msg

    def cpx_callback(self, packet):
        msg = self.cpx_packet_to_msg(packet)   
        self.cpx_pub.publish(msg)

    def cpx_packet_to_msg(self, packet):
        msg = CPXPacket()

        msg.cpx.destination = packet.header.destination
        msg.cpx.source = packet.header.source
        msg.cpx.last_packet = packet.header.last_packet
        msg.cpx.reserved = packet.header.reserved
        msg.cpx.function = packet.header.function
        msg.cpx.version = packet.header.version

        msg.payload = packet.payload

        return msg

    def inference_callback(self, msg):
        _, metadata = self.pop_metadata(stamp=msg.header.stamp)
        if metadata is None:
            return

        output = msg_to_array(msg.output)

        success = self.pool.try_run(self.send_reply, metadata, output)

        if self.save:
            self.inference_saver.save(metadata, output, success, msg.inference_time)

    def push_metadata(self, stamp, metadata):
        stamp = Time.from_msg(stamp).nanoseconds
        self.metadata_queue.append((stamp, metadata))

    def pop_metadata(self, stamp=None, stm32_timestamp=None):
        assert (stamp is None) != (stm32_timestamp is None), "Exactly one of stamp or stm32_timestamp must be supplied"

        if stamp is not None:
            stamp = Time.from_msg(stamp).nanoseconds

        # If stamp or stm32_timestamp are zero, just return the most recent metadata
        if len(self.metadata_queue) > 0 and \
           (stamp == 0 or stm32_timestamp == 0):
            last_stamp, last_metadata = self.metadata_queue[-1]
            return last_stamp, last_metadata

        last_stamp = None
        last_metadata = None

        while len(self.metadata_queue) > 0 and \
              ((stamp is not None and self.metadata_queue[0][0] <= stamp) or \
               (stm32_timestamp is not None and self.metadata_queue[0][1].state.timestamp <= stm32_timestamp)):
            last_stamp, last_metadata = self.metadata_queue.popleft()

        if last_stamp == stamp or \
           (last_metadata is not None and last_metadata.state.timestamp == stm32_timestamp):
            return last_stamp, last_metadata
        else:
            return None, None

    def send_reply(self, metadata, output):
        # Send reply to the drone. Metadata contains the frame statistics used
        # to compute the round-trip time.
        try:
            self.client.send_reply(metadata, output)
        except OSError as e:
            # rospy.logwarn(f'Failed to send reply with error {e}')
            print(f'Failed to send reply with error {e}')


def main():
    rclpy.init()

    node = ROSViewer()
    rclpy.spin(node)

if __name__ == '__main__':
    main()
