#
# streamer.py
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

import binascii
import ctypes
from enum import IntEnum
import numpy as np
import signal
import threading

from .cpx import CPXClient, CPXPacket, CPXHeader, CPXTarget, CPXFunction

UINT32_MAX = 2**32 - 1

class RepeatTimer(threading.Timer):
    def run(self) -> None:
        while not self.finished.wait(self.interval):  # type: ignore
            self.function(*self.args, **self.kwargs)  # type: ignore


class StreamerType(IntEnum):
    IMAGE       = 0x01
    INFERENCE   = 0xF0

class StateMessage(ctypes.LittleEndianStructure):
    _pack_ = 1
    _fields_ = [
        # STM32 timestamp [ticks]
        ("timestamp", ctypes.c_uint32),

        # position [mm]
        ("x", ctypes.c_int16),
        ("y", ctypes.c_int16),
        ("z", ctypes.c_int16),

        # velocity [mm/s]
        ("vx", ctypes.c_int16),
        ("vy", ctypes.c_int16),
        ("vz", ctypes.c_int16),

        # acceleration [mm/s^2]
        ("ax", ctypes.c_int16),
        ("ay", ctypes.c_int16),
        ("az", ctypes.c_int16),

        # compressed quaternion, see quatcompress.py (elements stored as xyzw)
        ("quat", ctypes.c_int32),
        
        # angular velocity [mrad/s]
        ("rateRoll", ctypes.c_int16),
        ("ratePitch", ctypes.c_int16),
        ("rateYaw", ctypes.c_int16),
    ]

class TOFMessage(ctypes.LittleEndianStructure):
    _pack_ = 1
    _fields_ = [
        ("resolution", ctypes.c_uint8),
        ("_padding", ctypes.c_uint8 * 3),
        
        ("data", ctypes.c_uint8 * 64),
    ]

class InferenceStampedMessage(ctypes.LittleEndianStructure):
    _pack_ = 1
    _fields_ = [
        ("stm32_timestamp", ctypes.c_uint32),
        ("x", ctypes.c_float),
        ("y", ctypes.c_float),
        ("z", ctypes.c_float),
        ("phi", ctypes.c_float),
    ]

class StreamerMetadata(ctypes.LittleEndianStructure):
    METADATA_VERSION = 10

    _pack_ = 1
    _fields_ = [
        # Metadata format version
        ("metadata_version", ctypes.c_uint8),

        # Frame resolution and pixel format
        ("frame_width", ctypes.c_uint16),
        ("frame_height", ctypes.c_uint16),
        ("frame_bpp", ctypes.c_uint8),
        ("frame_format", ctypes.c_uint8),

        # Sequential frame ID from the camera's hardware frame counter
        ("frame_id", ctypes.c_uint8),

        # GAP end-of-frame timestamp [usec]
        ("frame_timestamp", ctypes.c_uint32),

        # GAP state received timestamp [usec]
        ("state_timestamp", ctypes.c_uint32),

        # Latest state estimation received from STM32
        ("state", StateMessage),

        # GAP TOF received timestamp [usec]
        ("tof_timestamp", ctypes.c_uint32),

        # Latest TOF frame received from STM32
        ("tof", TOFMessage),

        # GAP timestamps used for RTT latency computation [usec]
        ("reply_frame_timestamp", ctypes.c_uint32),
        ("reply_recv_timestamp", ctypes.c_uint32),

        # Latest inference computed onboard by GAP
        ("inference", InferenceStampedMessage),
    ]

class StreamerStats(ctypes.LittleEndianStructure):
    _pack_ = 1
    _fields_ = [
        # GAP timestamps used for RTT latency computation [usec]
        ("reply_frame_timestamp", ctypes.c_uint32),
        ("reply_frame_id", ctypes.c_uint8),
    ]

class OffboardBuffer(ctypes.LittleEndianStructure):
    _pack_ = 1
    _fields_ = [
        ("stats", StreamerStats),
        ("inference_stamped", InferenceStampedMessage),
    ]

class StreamerCommand(IntEnum):
    BUFFER_BEGIN    = 0x10
    BUFFER_DATA     = 0x11

class StreamerBegin(ctypes.LittleEndianStructure):
    _pack_ = 1
    _fields_ = [
        ("type", ctypes.c_uint8),
        ("size", ctypes.c_uint32),
        ("checksum", ctypes.c_uint32),
        ("_padding", ctypes.c_uint8 * 2),
    ]

class StreamerData(ctypes.LittleEndianStructure):
    _pack_ = 1
    _fields_ = [
        ("_padding", ctypes.c_uint8 * 3),
    ]

class StreamerHeader(ctypes.LittleEndianStructure):
    _pack_ = 1
    _fields_ = [
        ("command", ctypes.c_uint8),
    ]

class StreamerClient:
    def __init__(self, log_fn=print, *args, **kwargs) -> None:    
        self.log_fn = log_fn
        self.deferred_crlf = False

        self.metadata_stats = []
        self.n_frames = 0

        signal.signal(signal.SIGINT, self.shutdown)
        signal.signal(signal.SIGTERM, self.shutdown)
        
        self.fps_timer = RepeatTimer(1.0, self.update_fps)
        self.fps_timer.start()

        self.cpx = CPXClient(*args, log_fn=self.log, **kwargs)
        self.cpx_header = CPXHeader(destination=CPXTarget.GAP, function=CPXFunction.STREAMER)

    def receive(self):
        expected_cmd = StreamerCommand.BUFFER_BEGIN
        rx_buffer = None
        buffer_type = None
        remaining_length = None
        expected_checksum = None

        for cpx_packet in self.cpx.receive():
            if cpx_packet.header.function != CPXFunction.STREAMER:
                self.log(f"Function 0x{cpx_packet.header.function:02x}, not a streamer packet ignoring")
                continue

            header = StreamerHeader.from_buffer_copy(cpx_packet.payload)
            packet_offset = ctypes.sizeof(header)
            packet_length = len(cpx_packet.payload)
            # self.log(f"Received packet 0x{header.command:02x} with length {packet_length}")
            # self.log(cpx_packet.payload.hex())

            if header.command != expected_cmd:
                # self.log(f"Received command 0x{header.command:02x} while expecting 0x{expected_cmd:02x}, resetting")
                expected_cmd = StreamerCommand.BUFFER_BEGIN
                rx_buffer = None
                buffer_type = None
                remaining_length = None
                expected_checksum = None
        
            if header.command != expected_cmd:
                # self.log(f"Command 0x{header.command:02x} is still not the expected 0x{expected_cmd:02x}, discarding")
                continue

            if expected_cmd == StreamerCommand.BUFFER_BEGIN:
                begin = StreamerBegin.from_buffer_copy(cpx_packet.payload, packet_offset)
                packet_offset += ctypes.sizeof(begin)

                expected_cmd = StreamerCommand.BUFFER_DATA
                rx_buffer = b''
                buffer_type = begin.type
                remaining_length = begin.size
                expected_checksum = begin.checksum
                # self.log(f"Received start of frame with size {remaining_length} bytes")
            elif expected_cmd == StreamerCommand.BUFFER_DATA:
                data = StreamerData.from_buffer_copy(cpx_packet.payload, packet_offset)
                packet_offset += ctypes.sizeof(data)

            if expected_cmd == StreamerCommand.BUFFER_DATA:
                # Process the payload of both BUFFER_BEGIN and BUFFER_DATA packets
                # (expected_cmd is changed to BUFFER_DATA above, when processing BUFFER_BEGIN)
                payload_length = min(packet_length - packet_offset, remaining_length)
                rx_chunk = cpx_packet.payload[packet_offset:packet_offset + payload_length]

                rx_buffer += rx_chunk
                remaining_length -= payload_length
                
                # self.log(packet_length, packet_offset, len(rx_buffer), remaining_length)

                if remaining_length == 0:
                    if expected_checksum != 0:
                        checksum = binascii.crc32(rx_buffer)
                        if checksum != expected_checksum:
                            print(f"Received buffer is corrupted (checksum {checksum}, expected {expected_checksum})")
                            expected_cmd = StreamerCommand.BUFFER_BEGIN
                            continue

                    yield self.process_buffer(buffer_type, rx_buffer)

                    expected_cmd = StreamerCommand.BUFFER_BEGIN
                    rx_buffer = None
                    buffer_type = None
                    remaining_length = None
                else:
                    expected_cmd = StreamerCommand.BUFFER_DATA
    
    def process_buffer(self, buffer_type, buffer):
        if buffer_type == StreamerType.IMAGE:
            frame, tof_frame, metadata = self.decode_frame(buffer)

            self.metadata_stats.append(metadata)

            return frame, tof_frame, metadata

    def decode_frame(self, buffer):
        metadata_size = ctypes.sizeof(StreamerMetadata)
        metadata = StreamerMetadata.from_buffer_copy(buffer)
        buffer = buffer[metadata_size:]

        assert metadata.metadata_version == StreamerMetadata.METADATA_VERSION, \
               f"Client supports StreamerMetadata v{StreamerMetadata.METADATA_VERSION} but received v{metadata.metadata_version}"

        frame = np \
            .frombuffer(buffer, dtype=f'<u{metadata.frame_bpp}') \
            .reshape((metadata.frame_height, metadata.frame_width))

        tof_resolution = metadata.tof.resolution
        
        tof_frame = None
        if tof_resolution > 0:
            tof_size = np.sqrt(tof_resolution).astype(int)
            tof_frame = np \
                .array(metadata.tof.data) \
                [:tof_resolution] \
                .reshape((tof_size, tof_size))

        return frame, tof_frame, metadata

    def _compute_checksum(buffer):
        checksum = binascii.crc32(buffer)

        if checksum == 0:
            # If the computed checksum is zero, it is transmitted as all ones. An all zero
            # transmitted checksum value means that the transmitter generated no checksum.
            checksum = UINT32_MAX

        return checksum

    def _send_buffer_begin(self, buffer_type, buffer_size, buffer_checksum, buffer_segment):
        command = StreamerHeader(command=StreamerCommand.BUFFER_BEGIN)
        buffer_begin = StreamerBegin(type=buffer_type, size=buffer_size, checksum=buffer_checksum)
        payload = bytes(command) + bytes(buffer_begin) + bytes(buffer_segment)
        self.cpx.send(self.cpx_header, payload)

    def _send_buffer_data(self, buffer_segment):
        command = StreamerHeader(command=StreamerCommand.BUFFER_DATA)
        buffer_data = StreamerData()
        payload = bytes(command) + bytes(buffer_data) + bytes(buffer_segment)
        self.cpx.send(self.cpx_header, payload)

    def _max_buffer_begin_length(self):
        return self.cpx.max_payload_length - ctypes.sizeof(StreamerHeader) - ctypes.sizeof(StreamerBegin)

    def _max_buffer_data_length(self):
        return self.cpx.max_payload_length - ctypes.sizeof(StreamerHeader) - ctypes.sizeof(StreamerData)

    def send_buffer(self, buffer_type, buffer):
        buffer = bytes(buffer)
        buffer_size = len(buffer)
        buffer_checksum = binascii.crc32(buffer)

        buffer_sent = 0
        while buffer_sent < buffer_size:
            if buffer_sent == 0:
                max_segment_length = self._max_buffer_begin_length()
            else:
                max_segment_length = self._max_buffer_data_length()

            segment = buffer[buffer_sent:buffer_sent + max_segment_length]
            
            if buffer_sent == 0:
                self._send_buffer_begin(buffer_type, buffer_size, buffer_checksum, segment)
            else:
                self._send_buffer_data(segment)

            buffer_sent += len(segment)

    def send_reply(self, metadata, network_output):
        reply = OffboardBuffer()

        reply.stats.reply_frame_timestamp = metadata.frame_timestamp
        reply.stats.reply_frame_id = metadata.frame_id
        
        if network_output is None:
            reply.inference_stamped.stm32_timestamp = 0
        else:
            reply.inference_stamped.stm32_timestamp = metadata.state.timestamp
            reply.inference_stamped.x   = network_output[0]
            reply.inference_stamped.y   = network_output[1]
            reply.inference_stamped.z   = network_output[2]
            reply.inference_stamped.phi = network_output[3]

        self.send_buffer(StreamerType.INFERENCE, reply)

    def log(self, *args, end='\n', **kwargs):
        if self.deferred_crlf and end != '':
            self.log_fn()
            self.deferred_crlf = False
        
        if end == '':
            self.deferred_crlf = True
        
        self.log_fn(*args, end=end, **kwargs)

    def update_fps(self):
        n_received = len(self.metadata_stats)

        if n_received == 0:
            return

        height, width = 0, 0
        rtt = 0.0
        fps = 0.0
        n_dropped = 0

        if n_received >= 1:
            last = self.metadata_stats[-1]
            
            height, width = last.frame_height, last.frame_width

            rtt_latencies = [frame.reply_recv_timestamp - frame.reply_frame_timestamp for frame in self.metadata_stats]
            rtt = np.mean(rtt_latencies) / 10**3

        if n_received >= 2:
            first = self.metadata_stats[0]
            last = self.metadata_stats[-1]

            frame_period = (last.frame_timestamp - first.frame_timestamp) / (n_received - 1) / 10**6
            fps = 1 / frame_period

            n_acquired = (last.frame_id - first.frame_id) % 256 + 1
            n_dropped = n_acquired - n_received

        self.metadata_stats = []
        self.n_frames += n_received

        self.log(f'\r{width} x {height}px, {fps:.1f}fps, RTT {rtt:.0f}ms, dropped {n_dropped}, total {self.n_frames}', end='')

    def shutdown(self, *args):
        self.fps_timer.cancel()
        self.cpx.shutdown()
