io=uart
PMSIS_OS = freertos

APP = test
APP_SRCS += test.c com.c
APP_CFLAGS += -O3 -g

APP_CFLAGS += -DconfigUSE_TIMERS=1 -DINCLUDE_xTimerPendFunctionCall=1


RUNNER_CONFIG = $(CURDIR)/config.ini


include $(RULES_DIR)/pmsis_rules.mk
