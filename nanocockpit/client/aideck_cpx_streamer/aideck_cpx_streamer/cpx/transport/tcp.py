#
# tcp.py
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
import time
from typing import Iterator

from . import Transport
from ..cpx import CPXPacket, CPXHeader

# From Python docs: for best match with hardware and network realities, RECEIVE_SIZE should be
# a relatively small power of 2, for example, 4096.
TCP_RECEIVE_SIZE = 1024

# Define the TCP send buffer size as a function of the sent message size, bounding the maximum queue size
from ..streamer import OffboardBuffer
TCP_SEND_BUFFER = int(ctypes.sizeof(OffboardBuffer) * 10)

class TCPHeader(ctypes.LittleEndianStructure):
    _pack_ = 1
    _fields_ = [
        ("length", ctypes.c_uint16),
        ("cpx", CPXHeader),
    ]

    def __repr__(self) -> str:
        return f"<TCPHeader - length: {self.length}>"

# Maximum size of a CPX TCP packet (header + payload)
# Currently: maximum supported DMA transfer length by ESP32 (SPI_MAX_DMA_LEN - 4092 bytes)
CPX_TCP_MAX_FRAME_LENGTH = 4092
CPX_TCP_MTU = CPX_TCP_MAX_FRAME_LENGTH - ctypes.sizeof(TCPHeader)

class struct_linger(ctypes.Structure):
    _fields_ = [
        ("l_onoff", ctypes.c_int),
        ("l_linger", ctypes.c_int),
    ]

class TCPClientTransport(Transport):
    def __init__(self, host: str, port: int = 5000, log_fn=print):
        super().__init__()

        self.host = host
        self.port = port

        self.socket = None

        self.log = log_fn

    @property
    def max_frame_length(self) -> int:
        return CPX_TCP_MTU

    # MARK: Connection

    def is_connected(self):
        return self.socket != None

    def connect(self):
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

        # s.settimeout(5)

        # Enable TCP Keep-alive with a 5s timeout
        s.setsockopt(socket.SOL_SOCKET, socket.SO_KEEPALIVE, 1)
        self.setsockopt_tcp_keepidle(s, 1)
        s.setsockopt(socket.SOL_TCP, socket.TCP_KEEPINTVL, 1)
        s.setsockopt(socket.SOL_TCP, socket.TCP_KEEPCNT, 5)

        # Wait up to 5s upon close() for remaining data to be transmitted
        s.setsockopt(socket.SOL_SOCKET, socket.SO_LINGER, struct_linger(1, 5))

        ## Optimize the socket for latency
        # Ensures CPX packets are not delayed while waiting for more data to fill a TCP segment
        s.setsockopt(socket.SOL_TCP, socket.TCP_NODELAY, 1)
        # Explicitly define the TCP send buffer size to avoid queuing up CPX packets 
        s.setsockopt(socket.SOL_SOCKET, socket.SO_SNDBUF, TCP_SEND_BUFFER)

        # Connect to remote host
        s.connect((self.host, self.port))
        
        self.socket = s
        return s

    def setsockopt_tcp_keepidle(self, s, tcp_keepidle):
        if hasattr(socket, 'TCP_KEEPIDLE'):
            s.setsockopt(socket.SOL_TCP, socket.TCP_KEEPIDLE, tcp_keepidle)
        else:
            if hasattr(socket, 'TCP_KEEPALIVE'):
                TCP_KEEPALIVE = socket.TCP_KEEPALIVE
            else:
                TCP_KEEPALIVE = 0x10
            s.setsockopt(socket.SOL_TCP, TCP_KEEPALIVE, tcp_keepidle)

    def disconnect(self):
        if not self.is_connected():
            return

        self.socket.close()
        self.socket = None

        # self.log(f"TCP socket disconnected")

    @property
    def local_port(self):
        if not self.is_connected():
            return None
        
        local_host, local_port = self.socket.getsockname()

        return local_port

    def shutdown(self):
        super().shutdown()
        self.disconnect()

    # MARK: Send

    def send(self, packet: CPXPacket):
        if not self.is_connected():
            return

        header = TCPHeader(length=len(packet.payload), cpx=packet.header)
        data = bytes(header) + bytes(packet.payload)

        try:
            # The traditional send() call does not guarantee to send the entirety of data
            # that it receives, leaving it up to the caller to check and possibly retry.
            # Use sendall() instead, since we want to ensure that CPX packets are either 
            # sent entirely or not at all.
            self.socket.sendall(data)

        except socket.timeout:
            self.log("TCP send timed out")
            self.disconnect()
        except ConnectionResetError:
            self.log("TCP connection reset")
            self.disconnect()

    # MARK: Receive

    def _receive_stream(self) -> Iterator[tuple[bool, bytes]]:
        self.ok = True
        
        while self.ok:
            self.log()
            self.log(f"Connecting to {self.host}:{self.port}...")

            try:
                s = self.connect()
            except (socket.timeout, TimeoutError):
                self.log("Connection timed out, retrying")
                continue
            except ConnectionRefusedError:
                self.log("Connection refused, retrying")
                continue
            except OSError as e:
                self.log(f"{e}. Retrying in 5 seconds")
                time.sleep(5)
                continue

            self.log("Socket connected, ready to get data")
            yield True, b''

            with s:
                while self.ok and self.is_connected():
                    try:
                        chunk = s.recv(TCP_RECEIVE_SIZE)
                    except (socket.timeout, TimeoutError):
                        self.log("Receive timed out")
                        break
                    except ConnectionResetError:
                        self.log("Connection reset, retrying")
                        break
                    except OSError as e:
                        if e.errno == errno.EBADF:
                            # Connection closed by local host
                            break
                        else:
                            self.log(e)
                            break

                    if len(chunk) == 0:
                        self.log("Connection closed by remote host")
                        break

                    yield False, chunk

    WAIT_HEADER = 0
    WAIT_PAYLOAD = 1

    def receive(self) -> Iterator[tuple[bool, CPXPacket]]:
        for reset, chunk in self._receive_stream():
            if reset:
                buffer = b''
                state = self.WAIT_HEADER
                tcp_header = None
                yield True, None

            buffer += chunk

            if state == self.WAIT_HEADER:
                expected_length = ctypes.sizeof(TCPHeader)
                if len(buffer) < expected_length:
                    # self.log(f"Waiting TCP header, got {len(buffer)}, need {expected_length}")
                    continue

                tcp_header = TCPHeader.from_buffer_copy(buffer)
                buffer = buffer[expected_length:]
                state = self.WAIT_PAYLOAD

            elif state == self.WAIT_PAYLOAD:
                expected_length = tcp_header.length
                if expected_length > CPX_TCP_MTU:
                    self.log(f"Length ({expected_length}) in TCP header is over the supported maximum ({CPX_TCP_MTU}), resetting")
                    break

                if len(buffer) < expected_length:
                    # self.log(f"Waiting TCP payload, got {len(buffer)}, need {expected_length}")
                    continue

                packet = CPXPacket(tcp_header.cpx, buffer[:expected_length])
                yield False, packet

                buffer = buffer[expected_length:]
                state = self.WAIT_HEADER
                tcp_header = None
