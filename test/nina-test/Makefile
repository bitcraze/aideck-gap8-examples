#
# This is a project Makefile. It is assumed the directory this Makefile resides in is a
# project subdirectory.
#

PROJECT_NAME := decktest

include $(IDF_PATH)/make/project.mk

openocd_flash:
	~/esp/openocd-esp32/bin/openocd -f interface/ftdi/olimex-arm-usb-tiny-h.cfg -f board/esp-wroom-32.cfg -c "program_esp32 build/partitions_singleapp.bin 0x8000 verify" -c "program_esp32 build/bootloader/bootloader.bin 0x1000 verify" -c "program_esp32 build/decktest.bin 0x10000 verify reset exit"

openocd:
	~/esp/openocd-esp32/bin/openocd  -f interface/ftdi/olimex-arm-usb-tiny-h.cfg -f board/esp-wroom-32.cfg

gdb:
	~/esp/xtensa-esp32-elf/bin/xtensa-esp32-elf-gdb -x gdbinit build/blink.elf
