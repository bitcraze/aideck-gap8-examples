APP = test
APP_SRCS += test.c ImgIO.c
APP_CFLAGS += -O3 -g

APP_CFLAGS += -DASYNC_CAPTURE

clean::
	rm -rf img_out.ppm

include $(RULES_DIR)/pmsis_rules.mk
