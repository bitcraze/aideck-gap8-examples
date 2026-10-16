#
# cpx.py
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
from enum import IntEnum
from typing import Callable

CPX_VERSION = 0

class CPXTarget(IntEnum):
    STM32     = 0x01       # The STM in the Crazyflie
    ESP32     = 0x02       # The ESP on the AI-deck
    WIFI_HOST = 0x03       # A remote computer connected via Wi-Fi
    GAP       = 0x04       # The GAP on the AI-deck

class CPXFunction(IntEnum):
    SYSTEM     = 0x01
    CONSOLE    = 0x02
    CRTP       = 0x03
    WIFI_CTRL  = 0x04
    APP        = 0x05

    STREAMER   = 0x06

    TEST       = 0x0E
    BOOTLOADER = 0x0F

    LAST       = 0x10

class CPXHeader(ctypes.LittleEndianStructure):
    _pack_ = 1
    _fields_ = [
        ("destination", ctypes.c_uint8, 3),
        ("source", ctypes.c_uint8, 3),
        ("last_packet", ctypes.c_uint8, 1),
        ("reserved", ctypes.c_uint8, 1),
        ("function", ctypes.c_uint8, 6),
        ("version", ctypes.c_uint8, 2),
    ]

    def __init__(self, destination: CPXTarget, function: CPXFunction, source: CPXTarget = CPXTarget.WIFI_HOST, last_packet: bool = True):
        super().__init__(
            destination=destination, source=source,
            last_packet=last_packet, reserved=False,
            function=function,
            version=CPX_VERSION
        )

    def __repr__(self) -> str:
        return f"<CPXHeader - destination: {self.destination}, source: {self.source}, last_packet: {self.last_packet}, reserved: {self.reserved}, function: {self.function}, version: {self.version}>"

class CPXPacket:
    def __init__(self, header: CPXHeader, payload: bytes):
        self.header = header
        self.payload = bytes(payload)

class CPXClient:
    def __init__(self, host: str = None, port: int = 5000, udp_send: bool = True, log_fn=print) -> None:
        self.log = log_fn

        if host is not None:
            from .transport import MultiClientTransport
            self.transport = MultiClientTransport(host, port, log_fn=self.log, udp_send=udp_send)
        else:
            raise ValueError("Transport not specified")
        
        self.callback_fn = None

    @property
    def max_payload_length(self):
        return self.transport.max_frame_length - ctypes.sizeof(CPXHeader)

    def send(self, header: CPXHeader, payload: bytes):
        packet = CPXPacket(header, payload)

        if self.callback_fn is not None:
            self.callback_fn(packet)
        
        self.transport.send(packet)

    def receive(self):
        for packet in self.transport.receive():
            cpx_version = packet.header.version
            if cpx_version != CPX_VERSION:
                raise ValueError(
                    f"Received packet with unsupported CPX version {cpx_version}, expected {CPX_VERSION}"
                )
            
            if self.callback_fn is not None:
                self.callback_fn(packet)

            yield packet

    def shutdown(self):
        self.transport.shutdown()
        self.log("CPX client shutting down")

    def add_callback(self, callback: Callable[[CPXPacket], None]):
        """Register a callback that is called everytime a CPX packet is sent or received"""
        self.callback_fn = callback
