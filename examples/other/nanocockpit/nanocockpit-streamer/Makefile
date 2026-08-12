#
# Makefile
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

APP = streamer

library_path = ../lib

APP_INC  = $(CURDIR) $(CURDIR)/$(library_path)
APP_CFLAGS  += -Werror

# -O2 with -fno-indirect-inlining is just as fast as -O3 and reduces code size considerably
# by not inlining of small functions in the management code
APP_CFLAGS  += -g -O2 -fno-indirect-inlining -flto
APP_LDFLAGS += -g -Wl,--print-memory-usage -flto

APP_SRCS += main.c
APP_SRCS += $(library_path)/camera.c $(library_path)/camera/himax.c $(library_path)/cluster.c $(library_path)/crc32.c $(library_path)/debug.c $(library_path)/rng.c $(library_path)/soc.c $(library_path)/streamer.c $(library_path)/time.c $(library_path)/trace.c $(library_path)/queue.c
APP_SRCS += $(library_path)/cpx/cpx.c $(library_path)/cpx/cpx_spi.c
APP_SRCS += $(library_path)/uart.c $(library_path)/uart_protocol.c

include $(RULES_DIR)/pmsis_rules.mk
