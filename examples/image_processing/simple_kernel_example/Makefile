APP = test
APP_SRCS += test.c $(GAP_LIB_PATH)/img_io/ImgIO.c img_proc.c
APP_INC  += . $(GAP_LIB_PATH)/include

APP_CFLAGS += -O3 -g


PMSIS_OS ?= pulp_os

# this flag enables asynchronous image capturing
# keep in mind that you should always have a buffer enqueued
# if recording an image stream to avoid missing data (meaning you need 2)
APP_CFLAGS += -DASYNC_CAPTURE
# configures camera in QVGA format (324x244)
# the extra 4 is from padding for the RGB camera (but also present on the grey one)
APP_CFLAGS += -DQVGA_MODE

# YOU CAN ONLY ACTIVATE ONE KERNEL AT A TIME

# apply the demosaicking algorithm (only useful if using RGB camera)
# runs sequential on fabric controller
# APP_CFLAGS += -DDEMOSAICKING_KERNEL_FC
# apply the demosaicking algorithm (only useful if using RGB camera)
# runs paralell on cluster
APP_CFLAGS += -DDEMOSAICKING_KERNEL_CLUSTER
# only if one of the demosaicking kernel above is active
# activate for a colored output
APP_CFLAGS += -DCOLOR_IMAGE
# apply an example kernel that inverts the image
# runs sequential on fabric controller
# APP_CFLAGS += -DINVERTING_KERNEL_FC
# apply an example kernel that inverts the image
# runs paralell on cluster
# APP_CFLAGS += -DINVERTING_KERNEL_CLUSTER

clean::
	rm -rf img_raw.ppm img_color.ppm img_gray.ppm img_inverted.ppm

include $(RULES_DIR)/pmsis_rules.mk
