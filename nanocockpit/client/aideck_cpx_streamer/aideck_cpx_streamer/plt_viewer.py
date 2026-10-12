#!/usr/bin/env python3
#
# plt_viewer.py
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

import argparse
import matplotlib.pyplot as plt
import numpy as np
import cv2

from .cpx import StreamerClient, StreamerMetadata
from .utils import create_dataset_dir, FrameSaver

class PltViewer:
    def __init__(self) -> None:
        # Args for setting IP/port of AI-deck. Default settings are for when AI-deck is in AP mode.
        parser = argparse.ArgumentParser(description='Connect to AI-deck streamer')
        parser.add_argument("-host", default="aideck.local", metavar="host", help="AI-deck host")
        parser.add_argument("-port", type=int, default='5000', metavar="port", help="AI-deck port")
        parser.add_argument("--no-udp-send", action='store_false', dest='udp_send', help="Do not send replies over UDP")
        parser.add_argument("-save", type=str, default=None, metavar="save", help="Save images to output directory")
        args = parser.parse_args()

        self.client = StreamerClient(host=args.host, port=args.port, udp_send=args.udp_send)

        save_dir = args.save
        self.frame_saver = None
        if save_dir is not None:
            dataset_dir = create_dataset_dir(save_dir)
            self.frame_saver = FrameSaver(dataset_dir)

        plt.ion()
        self.fig, self.ax = plt.subplots()
        self.fig.canvas.mpl_connect('close_event', self.on_close)

        self.ax.axis('off')

        self.im = None

        # The pose panel is only added once the drone sends an on-board inference
        self.pose_ax = None
        self.pose = None

    def main(self):
        try:
            if self.frame_saver:
                self.frame_saver.open()

            for frame, tof_frame, metadata in self.client.receive():
                # Send reply to the drone. By default, it sends the frame statistics used
                # to compute the round-trip time, but it can be used also to transmit user data
                # to the drone.
                self.client.send_reply(metadata, None)

                self.display(frame, metadata)
                
                if self.frame_saver:
                    self.frame_saver.save(frame, tof_frame, metadata)
        finally:
            if self.frame_saver:
                self.frame_saver.close()
    
    def display(self, frame: np.ndarray, metadata: StreamerMetadata):
        frame = cv2.cvtColor(frame, cv2.COLOR_GRAY2RGB) / 255
        # frame = cv2.cvtColor(frame, cv2.COLOR_BayerRG2RGB) / 255
        # frame = cv2.cvtColor(frame, cv2.COLOR_BayerRG2RGB) / 1023

        if self.im is not None and self.im.get_shape() != frame.shape:
            self.im.remove()
            self.im = None

        if self.im is None:
             self.im = self.ax.imshow(frame)
            #  self.im = self.ax.imshow(frame, cmap='gray', vmin=0, vmax=255)
            #  self.im = self.ax.imshow(frame, cmap='gray', vmin=0, vmax=1024)

        if metadata is not None:
            title = f"Frame #{metadata.frame_id}"

            # The state is only shown once the Crazyflie has sent one to the AI-deck
            if metadata.state_timestamp != 0:
                state_delay = (metadata.frame_timestamp - metadata.state_timestamp) / 1000.0
                state_x = (metadata.state.x) / 1000.0
                state_y = (metadata.state.y) / 1000.0
                state_z = (metadata.state.z) / 1000.0
                title += f"\nState: X {state_x:+.02f}m, Y {state_y:+.02f}m, Z {state_z:+.02f}m, delay {state_delay:+.0f}ms"
        else:
            title = ""
        self.ax.set_title(title)

        self.im.set_data(frame)

        if metadata is not None and self.has_inference(metadata.inference):
            self.display_pose(metadata.inference)

        self.fig.canvas.start_event_loop(0.01)

    @staticmethod
    def has_inference(inference) -> bool:
        # Examples without a network on board (e.g., the streamer) leave the inference zeroed
        return any(v != 0 for v in (inference.x, inference.y, inference.z, inference.phi))

    def display_pose(self, inference):
        # Top-down view of the person pose estimated on board by PULP-Frontnet, in the drone's
        # body frame: x forward (up in the plot), y left (left in the plot). The person faces
        # the direction phi + pi, i.e. phi = 0 means that they face the drone.
        arrow_length = 0.4

        if self.pose is None:
            # Make room for a second panel next to the camera frame
            width, height = self.fig.get_size_inches()
            self.fig.set_size_inches(2 * width, height, forward=True)
            grid = self.fig.add_gridspec(1, 2)
            self.ax.set_subplotspec(grid[0])
            self.pose_ax = ax = self.fig.add_subplot(grid[1])

            ax.set_aspect('equal')
            ax.set_xlim(2.0, -2.0)
            ax.set_ylim(-0.5, 5.0)
            ax.set_xlabel('Y [m]')
            ax.set_ylabel('X [m]')
            ax.grid(True)
            ax.plot(0.0, 0.0, marker='^', markersize=12, color='tab:blue', label='Drone')
            person, = ax.plot([], [], marker='o', markersize=10, color='tab:orange', linestyle='', label='Person')
            heading, = ax.plot([], [], color='tab:orange', linewidth=2)
            label = ax.text(0.03, 0.97, '', transform=ax.transAxes, va='top', family='monospace', fontsize='small')
            ax.legend(loc='upper right')
            self.pose = (person, heading, label)

        person, heading, label = self.pose
        facing = inference.phi + np.pi
        person.set_data([inference.y], [inference.x])
        heading.set_data(
            [inference.y, inference.y + arrow_length * np.sin(facing)],
            [inference.x, inference.x + arrow_length * np.cos(facing)],
        )
        label.set_text(
            f"X   {inference.x:+.2f}m\n"
            f"Y   {inference.y:+.2f}m\n"
            f"Z   {inference.z:+.2f}m\n"
            f"phi {np.degrees(inference.phi):+.0f}°"
        )

    def on_close(self, _event):
        self.client.shutdown()

def main():
    viewer = PltViewer()
    viewer.main()

if __name__ == "main":
    main()
