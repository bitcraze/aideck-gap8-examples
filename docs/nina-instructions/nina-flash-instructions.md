---
title: NINA flash instructions
page_id: nina-flash-instructions
---


# Building NINA 

If you would like to use docker to setup the NINA environment, go to the [docker nina instructions](../nina-instructions/docker-nina.md)

Follow instructions here for installing dependencies:

https://docs.espressif.com/projects/esp-idf/en/latest/get-started/index.html

***Make sure you install IDF version v3.3.1***

The env var IDF_PATH must be define and the esp toolchain must be in the path.

Also the openocd as provided by esp should be installed:

https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/jtag-debugging/building-openocd-linux.html?highlight=openocd

To flash with openocd

	~/esp/openocd-esp32/bin/openocd -f interface/ftdi/olimex-arm-usb-tiny-h.cfg -f board/esp-wroom-32.cfg -c "program_esp32 build/partitions_singleapp.bin 0x8000 verify" -c "program_esp32 build/bootloader/bootloader.bin 0x1000 verify" -c "program_esp32 build/ai-deck-jpeg-streamer-demo.bin 0x10000 verify reset exit"

Check if the interface file matches your programmer

## Debug messages console

If the crazyflie firmware is compiled with the DEBUG_NINA_PRINT define, you will be able to see the debug messages from the NINA in the CFclient console. This way you can check if there is anything wrong with connecting with your WiFi.

*Make sure that the ai-deck driver is force compiled!
