#
# __init__.py
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

from typing import Iterator
from ..cpx import CPXPacket

class Transport:
    def __init__(self):
        self.ok = True
    
    @property
    def max_frame_length(self) -> int:
        raise NotImplementedError()

    def send(self, data: CPXPacket):
        raise NotImplementedError()

    def receive(self) -> Iterator[CPXPacket]:
        raise NotImplementedError()
    
    def shutdown(self):
        self.ok = False

from .tcp import TCPClientTransport
from .udp import UDPClientTransport
from .multi import MultiClientTransport

__all__ = [
    'TCPClientTransport',
    'UDPClientTransport',
    'MultiClientTransport',
]
