APP_DIR = $(CURDIR)/app

APP_CFLAGS += -I$(APP_DIR)
APP_CFLAGS += -Wno-error=multichar -Wno-error=int-conversion -Wno-error=implicit-function-declaration
APP_CFLAGS += -Wno-error=incompatible-pointer-types -Wno-error=discarded-qualifiers -Wno-error=attributes

NETWORK_NAME = frontnet-160x32-bgaug

NETWORK_DIR = $(APP_DIR)/networks/$(NETWORK_NAME)
include $(NETWORK_DIR)/network.mk
