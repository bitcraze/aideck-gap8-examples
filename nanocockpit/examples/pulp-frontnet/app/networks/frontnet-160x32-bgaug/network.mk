# network.mk
# Elia Cereda <elia.cereda@idsia.ch>
# Alessio Burrello <alessio.burrello@unibo.it>
#
# Copyright (C) 2022-2025 IDSIA, USI-SUPSI
#               2019-2020 University of Bologna
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

CORE ?= 8
FLASH_TYPE ?= HYPERFLASH
RAM_TYPE ?= HYPERRAM

APP_SRCS += $(wildcard $(NETWORK_DIR)/src/*.c)
APP_CFLAGS += -I$(NETWORK_DIR)/inc
APP_LDFLAGS += -lm

APP_CFLAGS += -DNUM_CORES=$(CORE)

# -O2 with -fno-indirect-inlining is just as fast as -O3 and reduces code size considerably
# by not inlining of small functions in the management code
APP_CFLAGS  += -O2 -fno-indirect-inlining -flto
APP_LDFLAGS += -Wl,--print-memory-usage -flto

APP_CFLAGS += -DGAP_SDK=1

ifeq '$(FLASH_TYPE)' 'MRAM'
    READFS_FLASH = target/chip/soc/mram
endif

APP_CFLAGS += -DFLASH_TYPE=$(FLASH_TYPE) -DUSE_$(FLASH_TYPE) -DUSE_$(RAM_TYPE)
APP_CFLAGS += -DALWAYS_BLOCK_DMA_TRANSFERS

FLASH_FILES += $(NETWORK_DIR)/hex/layer0_BNReluConvolution_weights.hex
FLASH_FILES += $(NETWORK_DIR)/hex/layer2_BNReluConvolution_weights.hex
FLASH_FILES += $(NETWORK_DIR)/hex/layer3_BNReluConvolution_weights.hex
FLASH_FILES += $(NETWORK_DIR)/hex/layer4_BNReluConvolution_weights.hex
FLASH_FILES += $(NETWORK_DIR)/hex/layer5_BNReluConvolution_weights.hex
FLASH_FILES += $(NETWORK_DIR)/hex/layer6_BNReluConvolution_weights.hex
FLASH_FILES += $(NETWORK_DIR)/hex/layer7_BNReluConvolution_weights.hex
FLASH_FILES += $(NETWORK_DIR)/hex/layer8_FullyConnected_weights.hex
FLASH_FILES += $(NETWORK_DIR)/hex/inputs.hex

READFS_FILES += $(FLASH_FILES)
APP_CFLAGS += -DFS_READ_FS
#PLPBRIDGE_FLAGS += -f
