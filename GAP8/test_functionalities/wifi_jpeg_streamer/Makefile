APP = test
APP_SRCS += test.c
APP_CFLAGS += -O3 -g

APP_LDFLAGS += -lgaptools -lgaplib 

RUNNER_CONFIG = $(CURDIR)/config.ini

streamer:
	./streamer

include $(RULES_DIR)/pmsis_rules.mk
