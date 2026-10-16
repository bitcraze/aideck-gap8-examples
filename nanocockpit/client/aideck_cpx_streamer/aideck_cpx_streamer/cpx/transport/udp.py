#
# udp.py
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

import ctypes
import errno
import socket
from threading import Condition
from typing import Iterator

from . import Transport
from ..cpx import CPXPacket, CPXHeader

class UDPHeader(ctypes.LittleEndianStructure):
    _pack_ = 1
    _fields_ = [
        ("sequence", ctypes.c_uint16),
        ("cpx", CPXHeader),
    ]

    def __repr__(self) -> str:
        return f"<UDPHeader - sequence: {self.sequence}>"

# Maximum size of a CPX UDP packet (header + payload)
# Currently: maximum supported DMA transfer length by ESP32 (SPI_MAX_DMA_LEN - 4092 bytes)
CPX_UDP_MAX_FRAME_LENGTH = 4092
CPX_UDP_MTU = CPX_UDP_MAX_FRAME_LENGTH - ctypes.sizeof(UDPHeader)

class UDPClientTransport(Transport):
    def __init__(self, remote_host: str, remote_port: int = 5000, log_fn=print):
        super().__init__()

        self.remote_host = remote_host
        self.remote_port = remote_port

        self.socket = None
        self.next_tx_seq = None
        self.next_rx_seq = None
        self.cv = Condition()

        self.log = log_fn

    @property
    def max_frame_length(self) -> int:
        return CPX_UDP_MTU

    # MARK: Connection

    def is_connected(self):
        return self.socket != None

    def connect(self, *, local_host: str = '0.0.0.0', local_port):
        with self.cv:
            s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            s.bind((local_host, local_port))
            s.connect((self.remote_host, self.remote_port))
            
            # self.log(f"UDP socket bound to {local_host}:{local_port} and "
            #     f"connected to {self.remote_host}:{self.remote_port}")

            self.socket = s
            self.next_tx_seq = 0
            self.next_rx_seq = -1
            self.cv.notify_all()
            
    def disconnect(self):
        if not self.is_connected():
            return
        
        self.socket.close()
        self.socket = None
        self.next_tx_seq = None
        self.next_rx_seq = None

        # self.log(f"UDP socket disconnected")

    def shutdown(self):
        super().shutdown()
        self.disconnect()

    # MARK: Send

    def send(self, packet: CPXPacket):
        if not self.is_connected():
            return
        
        header = UDPHeader(sequence=self.next_tx_seq, cpx=packet.header)
        data = bytes(header) + bytes(packet.payload)

        # The traditional send() call does not guarantee to send the entirety of data
        # that it receives, leaving it up to the caller to check and possibly retry.
        # Use sendall() instead, since we want to ensure that CPX packets are either 
        # sent entirely or not at all.
        self.socket.sendall(data)

        self.next_tx_seq = (self.next_tx_seq + 1) % 65536

    # MARK: Receive

    def receive(self) -> Iterator[bytes]:
        self.ok = True
        
        while self.ok:
            with self.cv:
                self.cv.wait_for(self.is_connected)

                try:
                    buffer = self.socket.recv(CPX_UDP_MAX_FRAME_LENGTH)
                except OSError as e:
                        if e.errno == errno.EBADF:
                            # Connection closed by local host
                            continue
                        else:
                            self.log(e)
                            continue

                expected_length = ctypes.sizeof(UDPHeader)
                if len(buffer) < expected_length:
                    self.log(f"UDP header too short, got {len(buffer)}, need {expected_length}")
                    continue

                udp_header = UDPHeader.from_buffer_copy(buffer)
                buffer = buffer[expected_length:]

                expected_length = len(buffer)
                if expected_length > CPX_UDP_MTU:
                    self.log(f"Length ({expected_length}) in UDP header is over the supported maximum ({CPX_UDP_MTU}), discarding")
                    continue

                if udp_header.sequence < self.next_rx_seq:
                    self.log(f"UDP packet received out of order ({udp_header.sequence}), expected ({self.next_rx_seq}), discarding")
                    self.next_rx_seq = -1
                    continue

                self.next_rx_seq = (udp_header.sequence + 1) % 65536

                packet = CPXPacket(udp_header.cpx, buffer[:expected_length])
                yield packet
