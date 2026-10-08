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
        self.fig, (self.ax, self.tof_ax) = plt.subplots(1, 2)
        self.fig.canvas.mpl_connect('close_event', self.on_close)
        
        self.ax.axis('off')
        self.tof_ax.axis('off')
        
        self.im = None
        self.tof = None

    def main(self):
        try:
            if self.frame_saver:
                self.frame_saver.open()

            for frame, tof_frame, metadata in self.client.receive():
                # Send reply to the drone. By default, it sends the frame statistics used
                # to compute the round-trip time, but it can be used also to transmit user data
                # to the drone.
                self.client.send_reply(metadata, None)

                self.display(frame, tof_frame, metadata)
                
                if self.frame_saver:
                    self.frame_saver.save(frame, tof_frame, metadata)
        finally:
            if self.frame_saver:
                self.frame_saver.close()
    
    def display(self, frame: np.ndarray, tof_frame: np.ndarray, metadata: StreamerMetadata):
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
        
        if self.tof is not None and (tof_frame is None or self.tof.get_shape() != tof_frame.shape):
            self.tof.remove()
            self.tof = None
        
        if self.tof is None and tof_frame is not None:
             self.tof = self.tof_ax.imshow(tof_frame, cmap='gray', vmin=0, vmax=255)
        
        if metadata is not None:
            state_delay = (metadata.frame_timestamp - metadata.state_timestamp) / 1000.0
            state_x = (metadata.state.x) / 1000.0
            state_y = (metadata.state.y) / 1000.0
            state_z = (metadata.state.z) / 1000.0
            title = f"Frame #{metadata.frame_id}\n" \
                    f"State: X {state_x:+.02f}m, Y {state_y:+.02f}m, Z {state_z:+.02f}m, delay {state_delay:+.0f}ms"
            
            if tof_frame is None:
                tof_title = "TOF Frame <na>"
            else:
                tof_shape = f"{tof_frame.shape[0]}x{tof_frame.shape[1]}"
                tof_delay = (metadata.frame_timestamp - metadata.tof_timestamp) / 1000.0
                tof_title = f"TOF Frame {tof_shape}\n" \
                            f"Delay {tof_delay:+.0f}ms"
        else:
            title = ""
            tof_title = ""
        self.ax.set_title(title)
        self.tof_ax.set_title(tof_title)
        
        self.im.set_data(frame)

        if tof_frame is not None:
            self.tof.set_data(tof_frame)

        self.fig.canvas.start_event_loop(0.01)

    def on_close(self, _event):
        self.client.shutdown()

def main():
    viewer = PltViewer()
    viewer.main()

if __name__ == "main":
    main()
