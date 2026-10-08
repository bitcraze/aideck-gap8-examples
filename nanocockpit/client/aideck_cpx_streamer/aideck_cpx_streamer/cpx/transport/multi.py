#
# multi.py
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

import queue
from threading import Thread
from typing import Iterator

from . import Transport, TCPClientTransport, UDPClientTransport
from ..cpx import CPXPacket

class MultiClientTransport(Transport):
    def __init__(self, remote_host: str, remote_port: int = 5000, udp_send: bool = True, rx_queue: int = 2, log_fn=print):
            super().__init__()

            self.log = log_fn

            self.tcp_transport = TCPClientTransport(remote_host, remote_port, log_fn=self.log)
            self.udp_transport = UDPClientTransport(remote_host, remote_port, log_fn=self.log)
            assert self.tcp_transport.max_frame_length == self.udp_transport.max_frame_length, \
               f"TCP ({self.tcp_transport.max_frame_length}) and UDP ({self.udp_transport.max_frame_length}) maximum frame lengths must match"

            self.udp_send = udp_send
            if self.udp_send:
                self.log("Will send replies over UDP")

            self.rx_queue = queue.Queue(maxsize=rx_queue)

    @property
    def max_frame_length(self) -> int:
        return self.tcp_transport.max_frame_length

    # MARK: Send

    def send(self, data: CPXPacket):
        if self.udp_send:
            self.udp_transport.send(data)
        else:
            self.tcp_transport.send(data)
    
    # MARK: Receive

    def receive(self) -> Iterator[CPXPacket]:
        tcp_thread = Thread(target=self.tcp_receive, daemon=True)
        tcp_thread.start()

        udp_thread = Thread(target=self.udp_receive, daemon=True)
        udp_thread.start()

        while self.ok:
            packet = self.rx_queue.get()

            # Fake packet used to break out of waiting on shutdown
            if packet is None:
                continue

            yield packet

    def tcp_receive(self):
        for reset, packet in self.tcp_transport.receive():
            if reset:
                local_port = self.tcp_transport.local_port
                self.udp_transport.disconnect()
                self.udp_transport.connect(local_port=local_port)
                continue

            self.rx_queue.put(packet)
        
        # Fake packet used to break out of waiting on shutdown
        self.rx_queue.put(None)

    def udp_receive(self):
        for packet in self.udp_transport.receive():
            self.rx_queue.put(packet)

        # Fake packet used to break out of waiting on shutdown
        self.rx_queue.put(None)

    def shutdown(self):
        super().shutdown()
        self.tcp_transport.shutdown()
        self.udp_transport.shutdown()
