APP = test
FRAME_STREAMER_PATH = $(GAP_SDK_HOME)/tools/gap_tools/frame_streamer
FRAME_STREAMER_INC = $(GAP_SDK_HOME)/tools/gap_tools/include
JPEG_PATH = $(GAP_LIB_PATH)/jpeg
JPEG_INC = $(GAP_LIB_PATH)/include/gap_lib

APP_SRCS += test.c
APP_SRCS += $(FRAME_STREAMER_PATH)/frame_streamer.c
APP_SRCS += $(JPEG_PATH)/jpeg_encoder.c	
APP_SRCS +=	$(JPEG_PATH)/dct.c 
APP_SRCS +=	$(JPEG_PATH)/jpeg_constants.c
APP_SRCS +=	$(JPEG_PATH)/cluster.c

APP_INC	+= $(FRAME_STREAMER_INC)  
APP_INC	+= $(JPEG_INC)  

APP_CFLAGS += -O3 -g
APP_CFLAGS += -DPERF


RUNNER_CONFIG = $(CURDIR)/config.ini

override config_args += --config-opt=camera/image-stream=$(CURDIR)/imgTest0.pgm

streamer:
	./streamer

include $(RULES_DIR)/pmsis_rules.mk
