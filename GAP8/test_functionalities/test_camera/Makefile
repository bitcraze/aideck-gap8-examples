APP = test
APP_SRCS += test.c 
APP_CFLAGS += -O3 -g 

APP_LDFLAGS += -lgaptools -lgaplib 

PMSIS_OS ?= pulp_os

APP_CFLAGS += -DASYNC_CAPTURE
APP_CFLAGS += -DQVGA_MODE
APP_CFLAGS += -DCOLOR_IMAGE


clean::
	rm -rf img_raw.ppm img_color.ppm img_gray.ppm

include $(RULES_DIR)/pmsis_rules.mk
