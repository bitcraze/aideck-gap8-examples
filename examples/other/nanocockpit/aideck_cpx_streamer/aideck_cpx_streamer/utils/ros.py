#
# ros.py
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

import numpy as np
from std_msgs.msg import Float32MultiArray, MultiArrayDimension

def array_to_msg(array, msg=None, dtype=float):
    if msg is None:
        msg = Float32MultiArray()

    array = np.asarray(array, dtype=dtype)

    item_size = array.dtype.itemsize
    shape = map(
        lambda size, stride: MultiArrayDimension(size=size, stride=stride // item_size),
        array.shape, array.strides
    )

    msg.layout.data_offset = 0
    msg.layout.dim = list(shape)
    msg.data = list(array)

    return msg

def msg_to_array(msg):
    assert isinstance(msg, Float32MultiArray)
    dtype = np.float32

    array = np.array(msg.data, dtype=dtype)

    assert msg.layout.data_offset == 0
    dims = map(lambda dim: (dim.size, dim.stride * array.dtype.itemsize), msg.layout.dim)
    shape, strides = zip(*dims)

    array = np.lib.stride_tricks.as_strided(array, shape, strides)

    return array
