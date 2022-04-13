# Copyright 2019 GreenWaves Technologies, SAS
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

RM = rm -f

APP = face_detection

NB_FRAMES ?= -1

FACE_DET_MODEL_SRC = FaceDetGenerator.c FaceDetModel.c
FACE_DET_MODEL_GEN = FaceDetKernels
FACE_DET_MODEL_GEN_C = $(addsuffix .c, $(FACE_DET_MODEL_GEN))
FACE_DET_MODEL_GEN_CLEAN = $(FACE_DET_MODEL_GEN_C) $(addsuffix .h, $(FACE_DET_MODEL_GEN))
FACE_DET_SRCS +=  main.c faceDet.c FaceDetBasicKernels.c ImageDraw.c $(FACE_DET_MODEL_GEN_C)

APP_SRCS += $(FACE_DET_SRCS)
APP_INC  += $(TILER_INC)
APP_CFLAGS += -O3 -g -D__PMSIS__  -DNB_FRAMES=$(NB_FRAMES)
# APP_CFLAGS += -DUSE_CAMERA
# APP_CFLAGS += -DUSE_STREAMER 

BOARD_NAME ?= ai_deck
PMSIS_OS ?= pulp_os
USE_PMSIS_BSP = 1
APP_LDFLAGS += -lgaptools -lgaplib 

export GAP_USE_OPENOCD=1
io=host

# This needs to be defined or else some other IO functionalities or cluster things will break...
APP_CFLAGS += -DHIMAX

PLPBRIDGE_FLAGS += -jtag

all:: model

# Build the code generator
GenTile:
	gcc -o GenTile -I$(TILER_INC) $(FACE_DET_MODEL_SRC) $(TILER_LIB) $(APP_CFLAGS)

# Run the code generator
$(FACE_DET_MODEL_GEN_C): GenTile
	./GenTile

model: $(FACE_DET_MODEL_GEN_C)

clean::
	$(RM) GenTile $(FACE_DET_MODEL_GEN_CLEAN)

.PHONY: model clean

include $(RULES_DIR)/pmsis_rules.mk
