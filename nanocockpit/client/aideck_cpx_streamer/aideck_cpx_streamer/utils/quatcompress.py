# MIT License
#
# Copyright (c) 2022 Elia Cereda
# Copyright (c) 2016 James A. Preiss
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

from typing import Sequence
import numpy as np

_1_2F = np.float32(1/2)
_SQRT1_2F = np.sqrt(_1_2F)

# assumes input quaternion is normalized. will fail if not.
def quatcompress(q: Sequence[np.float32]) -> np.uint32:
    q = np.asarray(q, dtype=np.float32)
    assert len(q) == 4

    # we send the values of the quaternion's smallest 3 elements.
    i_largest = np.abs(q).argmax()

    # since -q represents the same rotation as q,
    # transform the quaternion so the largest element is positive.
    # this avoids having to send its sign bit.
    negate = q[i_largest] < 0

    # 1/sqrt(2) is the largest possible value 
    # of the second-largest element in a unit quaternion.

    # do compression using sign bit and 9-bit precision per element.
    comp = np.uint32(i_largest)

    for i in range(4):
        if i != i_largest:
            negbit = np.uint32((q[i] < 0) ^ negate)
            mag = np.uint32(((1 << 9) - 1) * (np.abs(q[i]) / _SQRT1_2F) + _1_2F)
            comp = (comp << 10) | (negbit << 9) | mag

    return comp

def quatdecompress(comp: np.uint32) -> Sequence[np.float32]:
    comp = np.uint32(comp)
    q = np.empty(4, dtype=np.float32)

    mask = np.uint32((1 << 9) - 1)

    i_largest = comp >> 30
    sum_squares = np.float32(0)

    for i in reversed(range(4)):
        if i != i_largest:
            mag = comp & mask
            negbit = (comp >> 9) & 0x1
            comp = comp >> 10

            q[i] = _SQRT1_2F * np.float32(mag) / mask

            if negbit == 1:
                q[i] = -q[i]

            sum_squares += q[i] * q[i]

    q[i_largest] = np.sqrt(1 - sum_squares)

    return q
