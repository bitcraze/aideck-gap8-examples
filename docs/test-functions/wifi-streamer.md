---
title: WiFi Video Streamer
page_id: wifi-streamer
---


# Wifi video streamer

This example streams JPEG images from the GAP8 connected camera to a socket connected on WiFI. This concerns the example in *AIdeck_examples/GAP8/test_functionalities/wifi_jpeg_streamer/*.

This was tested on **GAP_SDK version 3.8.1**.


## Prerequisites
* GAP8 flashed with the source in ```/GAP8/test_functionalities/wifi_jpeg_streamer```
* Nina flashed with the source in ```/NINA/firmware```

## GAP8
Make sure you have followed [these instructions](../getting-started/getting-started.md) to set up your development environment for the GAP8.

### Build and flash the wifi example
This is how you can run the wifi example on L2 RAM (this is not an actual flash, as soon as the AI-deck resets, this is removed)
~~~~~shell
make clean all run io=host
~~~~~

this flashes the program directly on the AI-deck

~~~~~shell
make clean all
make image flash io=host
~~~~~

## NINA
### Flash the NINA module
See [the NINA flashing](/docs/nina-instructions/nina-flash-instructions.md) instructions

### Enter your credentials

By default, the Nina will act as an access-point where you can connect your host. If you would
like the NINA to connect to an access-point instead do the following:

```shell
cd NINA/firmware
make menuconfig
```

Enter the menu "AI deck example Configuration", use as AP and enter the credentials. Now rebuild it and flash.

```shell
make
```

## Host

In order to view the output from the camera do the following:

```shell
cd NINA
python3 viewer.py
```

The viewer supports the following options. By default, it connects to the IP
of the AI-deck when in AP mode.

```
usage: viewer.py [-h] [-n ip] [-p port]

Connect to AI-deck JPEG streamer example

optional arguments:
  -h, --help  show this help message and exit
  -n ip       AI-deck IP
  -p port     AI-deck port
```
