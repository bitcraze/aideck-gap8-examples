io=uart
PMSIS_OS = freertos

ifeq ($(SETUP_WIFI_AP), 1)
APP_CFLAGS += -DSETUP_WIFI_AP
endif

APP = wifi-img-streamer
APP_SRCS += wifi-img-streamer.c ../../../lib/cpx/src/com.c ../../../lib/cpx/src/cpx.c
APP_CFLAGS += -O3 -g
APP_INC=../../../lib/cpx/inc
APP_CFLAGS += -DconfigUSE_TIMERS=1 -DINCLUDE_xTimerPendFunctionCall=1

CONFIG_GAP_LIB_JPEG = 1

RUNNER_CONFIG = $(CURDIR)/config.ini


include $(RULES_DIR)/pmsis_rules.mk
