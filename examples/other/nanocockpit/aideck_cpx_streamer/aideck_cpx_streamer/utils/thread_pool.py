#
# thread_pool.py
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


class ThreadPool:
    def __init__(self, n_workers, queue_size=1) -> None:
        self.pool = [Thread(target=self.worker_main, daemon=True) for _ in range(n_workers)]
        self.queue = queue.Queue(maxsize=queue_size)

    def start(self):
        for thread in self.pool:
            thread.start()

    def try_run(self, fn, *args, **kwargs):
        try:
            self.queue.put_nowait((fn, args, kwargs))
            return True
        except queue.Full:
            return False

    def worker_main(self):
        while True:
            fn, args, kwargs = self.queue.get()
            fn(*args, **kwargs)
