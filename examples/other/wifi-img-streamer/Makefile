io=uart
PMSIS_OS = freertos

ifeq ($(SETUP_WIFI_AP), 1)
APP_CFLAGS += -DSETUP_WIFI_AP
endif

CAMERA_RESOLUTION ?= qvga
CAPTURE_MODE ?= start-stop
OUTPUT_PROFILING_DATA ?= 0
SENSOR_FRAME_RATE ?= default
STREAM_ENCODING ?= raw

ifeq ($(CAMERA_RESOLUTION),qvga)
APP_CFLAGS += -DCAMERA_FORMAT=PI_CAMERA_QVGA
APP_CFLAGS += -DCAMERA_WIDTH=324 -DCAMERA_HEIGHT=244
APP_CFLAGS += -DCAMERA_QVGA_WINDOW_ENABLE=1
else ifeq ($(CAMERA_RESOLUTION),qqvga)
APP_CFLAGS += -DCAMERA_FORMAT=PI_CAMERA_QQVGA
APP_CFLAGS += -DCAMERA_WIDTH=162 -DCAMERA_HEIGHT=122
else
$(error CAMERA_RESOLUTION must be qvga or qqvga)
endif

ifeq ($(CAPTURE_MODE),start-stop)
APP_CFLAGS += -DCAPTURE_MODE=0
else ifeq ($(CAPTURE_MODE),pipelined)
APP_CFLAGS += -DCAPTURE_MODE=2
else
$(error CAPTURE_MODE must be start-stop or pipelined)
endif

ifeq ($(SENSOR_FRAME_RATE),default)
else ifeq ($(SENSOR_FRAME_RATE),60)
APP_CFLAGS += -DSENSOR_FRAME_RATE_HZ=60
APP_CFLAGS += -DHIMAX_FRAME_LENGTH_LINES=266
APP_CFLAGS += -DHIMAX_MAX_INTEGRATION_LINES=264
APP_CFLAGS += -DHIMAX_QVGA_WINDOW_ENABLE=1
else
$(error SENSOR_FRAME_RATE must be default or 60)
endif

ifeq ($(STREAM_ENCODING),raw)
APP_CFLAGS += -DSTREAM_ENCODING_MODE=0
else ifeq ($(STREAM_ENCODING),jpeg)
APP_CFLAGS += -DSTREAM_ENCODING_MODE=1
else
$(error STREAM_ENCODING must be raw or jpeg)
endif

ifeq ($(filter $(OUTPUT_PROFILING_DATA),0 1),)
$(error OUTPUT_PROFILING_DATA must be 0 or 1)
endif

APP = wifi-img-streamer
APP_SRCS += wifi-img-streamer.c camera_pipeline.c cpx_stream.c himax_timing.c
APP_SRCS += ../../../lib/cpx/src/com.c ../../../lib/cpx/src/cpx.c
APP_CFLAGS += -O3 -g
APP_CFLAGS += -DOUTPUT_PROFILING_DATA=$(OUTPUT_PROFILING_DATA)
APP_INC=../../../lib/cpx/inc
APP_CFLAGS += -DconfigUSE_TIMERS=1 -DINCLUDE_xTimerPendFunctionCall=1

CONFIG_GAP_LIB_JPEG = 1

RUNNER_CONFIG = $(CURDIR)/config.ini


include $(RULES_DIR)/pmsis_rules.mk
