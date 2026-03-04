#
# frame_saver.py
# Elia Cereda <elia.cereda@idsia.ch>
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

from datetime import datetime
import numpy as np
import os
from PIL import Image
import cv2


def create_dataset_dir(base_dir):
    dataset_name = datetime.now().strftime('%Y-%m-%d-%H-%M-%S')
    dataset_dir = os.path.join(base_dir, dataset_name)

    os.makedirs(dataset_dir, exist_ok=True)
    
    return dataset_dir


class FrameSaver:
    def __init__(self, save_dir) -> None:
        self.save_dir = save_dir

        self.camera_raw_dir = os.path.join(save_dir, 'camera_raw')
        os.makedirs(self.camera_raw_dir, exist_ok=True)

        self.camera_rgb_dir = os.path.join(save_dir, 'camera_rgb')
        os.makedirs(self.camera_rgb_dir, exist_ok=True)

        self.tof_dir = os.path.join(save_dir, 'tof')
        os.makedirs(self.tof_dir, exist_ok=True)

    def open(self):
        self.metadata_csv = open(os.path.join(self.save_dir, "metadata.csv"), "w")

        self.metadata_csv.write("filename,metadata_version,")
        self.metadata_csv.write("frame_id,frame_gap8_timestamp,state_gap8_timestamp,state_stm32_timestamp,tof_timestamp,")
        self.metadata_csv.write("state_x,state_y,state_z,")
        self.metadata_csv.write("state_vx,state_vy,state_vz,")
        self.metadata_csv.write("state_ax,state_ay,state_az,")
        self.metadata_csv.write("state_quat,state_rateRoll,state_ratePitch,state_rateYaw\n")

        self.i = 0

    def save(self, frame, tof_frame, metadata):
        image = Image.fromarray(frame, mode="L")
        # image = Image.fromarray(frame << 6, mode="I;16") # 10-bit images, converted to 16-bit
        image_name = '%05d.png' % self.i
        image_path = os.path.join(self.camera_raw_dir, image_name)
        image.save(image_path)

        frame_rgb = cv2.cvtColor(frame, cv2.COLOR_BayerRG2RGB)
        image = Image.fromarray(frame_rgb, mode="RGB")
        image_name = '%05d.jpg' % self.i
        image_path = os.path.join(self.camera_rgb_dir, image_name)
        image.save(image_path)

        if tof_frame is not None:
            image = Image.fromarray(tof_frame, mode="L")
            image_name = '%05d.png' % self.i
            image_path = os.path.join(self.tof_dir, image_name)
            image.save(image_path)

        self.metadata_csv.write(f"{image_name},{metadata.metadata_version},")
        self.metadata_csv.write(f"{metadata.frame_id},{metadata.frame_timestamp},{metadata.state_timestamp},{metadata.state.timestamp},{metadata.tof_timestamp},")
        self.metadata_csv.write(f"{metadata.state.x},{metadata.state.y},{metadata.state.z},")
        self.metadata_csv.write(f"{metadata.state.vx},{metadata.state.vy},{metadata.state.vz},")
        self.metadata_csv.write(f"{metadata.state.ax},{metadata.state.ay},{metadata.state.az},")
        self.metadata_csv.write(f"{metadata.state.quat},{metadata.state.rateRoll},{metadata.state.ratePitch},{metadata.state.rateYaw}\n")

        self.i += 1

    def close(self):
        self.metadata_csv.close()

class InferenceSaver:
    def __init__(self, save_dir) -> None:
        self.save_dir = save_dir

    def open(self):
        self.inference_csv = open(os.path.join(self.save_dir, "inference.csv"), "w")

        self.inference_csv.write("frame_id,frame_gap8_timestamp,")
        self.inference_csv.write("output_x,output_y,output_z,output_phi,")
        self.inference_csv.write("send_success,inference_time\n")

        self.i = 0

    def save(self, metadata, network_output, send_success=False, inference_time=None):
        self.inference_csv.write(f"{metadata.frame_id},{metadata.frame_timestamp},")
        self.inference_csv.write(f"{network_output[0]},{network_output[1]},{network_output[2]},{network_output[3]},")
        self.inference_csv.write(f"{send_success},{inference_time}\n")

        self.i += 1

    def close(self):
        self.inference_csv.close()
