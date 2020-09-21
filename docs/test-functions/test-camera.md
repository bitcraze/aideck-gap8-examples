---
title: Test Himax Camera
page_id: test-camera
---

# Testing the Himax camera on the AIdeck


This concers the example in folder *GAP8/test_functionalities/test_camera/*. This was tested on **GAP_SDK version 3.7**.

In the makefile enable `APP_CFLAGS += -DASYNC_CAPTURE` if you want to test the asynchronous camera capture and remove it if you want to test the normal one. To save a color image enable `APP_CFLAGS += -DCOLOR_IMAGE`. And, to capture a `324x324` image enable `APP_CFLAGS += -DQVGA_MODE`. *Please note though that capturing an image in non-QVGA mode might not always work correctly.*

To test out the code, first source the AIdeck config (configs/ai_deck.sh) in your terminal and write 

    make clean all run io=host

for onram flashing with your programmer.

## Run in Docker
To build and execute in the docker we need to place the `demosaicking`-files in the `common`-folder inside the docker.
```
docker run --rm -it -v $PWD:/module/data/ --device /dev/ttyUSB0 --privileged -P gapsdk:3.7 /bin/bash -c 'export GAPY_OPENOCD_CABLE=interface/ftdi/olimex-arm-usb-tiny-h.cfg; source /gap_sdk/configs/ai_deck.sh; cd /module/data/;  make clean all run'
```