---
title: WiFi Video Streamer
page_id: wifi-streamer
---


# Wifi video streamer

This example streams JPEG images from the GAP8 connected camera to a socket connected on WiFI. This concerns the example in *GAP8/test_functionalities/wifi_jpeg_streamer/*


## Prerequisites
* GAP8 flashed with the source in ```/GAP8/test_functionalities/wifi_jpeg_streamer```
* Nina flashed with the source in ```/NINA/firmware```

## GAP8
### Install the GAP sdk
Follow the instructions of https://github.com/GreenWaves-Technologies/gap_sdk. 

Next to the regular SDK (make all), make sure to also build GAPtools (make gap_tools) and OpenOCD (make openocd)

### Source GAP sdk for the AIdeck
Type the following command in the terminal in which you will be flashing your AIdeck with
~~~~~shell
source [GAP_SDK]/configs/ai_deck.sh
~~~~~

Depended on the programmer you are using, you might need to change the following line in the *.sh file:
~~~~~shell
export OPENOCD_CABLE=interface/ftdi/olimex-arm-usb-ocd-h.cfg
~~~~~

### Build and flash the wifi example
This is how you can run the wifi example on L2 RAM (this is not an actual flash, as soon as the aideck resets, this is removed)
~~~~~shell
make clean all run io=host
~~~~~

this flashes the program directly on the aideck

~~~~~shell
make clean all
make image flash io=host
~~~~~

## NINA
### Flash the NINA module
See [the nina flashing](/docs/nina-instructions/nina-flash-instructions.md) instructions

### Enter you credentials

By default the Nina will act as a access-point where you can connect your host. If you would
like the Nina to connect to an access-point instead do the following:

```shell
cd NINA/firmware
make menuconfig
```

Enter the menu "AI deck example Configuration", use as AP and enter the credentials. Now requild it and flash.

```shell
make
```

## Host

In order to view the output from the camera do the following:

```shell
cd NINA
python3 viewer.py
```

The viewer supports the following options. By default it connects to the IP
of the AI-deck when in AP mode.

```
usage: viewer.py [-h] [-n ip] [-p port]

Connect to AI-deck JPEG streamer example

optional arguments:
  -h, --help  show this help message and exit
  -n ip       AI-deck IP
  -p port     AI-deck port
```
