APP = test
APP_SRCS += test.c $(GAP_LIB_PATH)/img_io/ImgIO.c
APP_INC  += . $(GAP_LIB_PATH)/include

APP_CFLAGS += -O3 -g


PMSIS_OS ?= pulp_os

APP_CFLAGS += -DASYNC_CAPTURE
APP_CFLAGS += -DQVGA_MODE
APP_CFLAGS += -DCOLOR_IMAGE


clean::
	rm -rf img_raw.ppm img_color.ppm img_gray.ppm

include $(RULES_DIR)/pmsis_rules.mk
