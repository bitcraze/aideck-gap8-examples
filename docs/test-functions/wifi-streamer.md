---
title: WiFi Video Streamer
page_id: wifi-streamer
---


# Wifi video streamer

This example show how to set up the GAP8 to stream videos through the NINA (esp) wifi module. 

## Prerequisites
* Gap_SDK 3.2
* Nina flashed with the source code under NINA/firmware/main. See the readme

## Install the GAP sdk
Follow the instructions of https://github.com/GreenWaves-Technologies/gap_sdk. 

Next to the regular SDK (make all), make sure to also build GAPtools (make gap_tools) and OpenOCD (make openocd)

## Flash the NINA module


## Source GAP sdk for the AIdeck
Type the following command in the terminal in which you will be flashing your AIdeck with
~~~~~shell
source [GAP_SDK]/configs/ai_deck.sh
~~~~~

Depended on the programmer you are using, you might need to change the following line in the *.sh file:
~~~~~shell
export OPENOCD_CABLE=interface/ftdi/olimex-arm-usb-ocd-h.cfg
~~~~~

## Enter you credentials

In test.c, you would need to search for the following lines, and change it to your local wifi's ssid, password and computers IP address

~~~~~shell
nina_conf.ssid = "";
nina_conf.passwd = "";
nina_conf.ip_addr = "0.0.0.0";
~~~~~

## Build and flash the wifi example
This is how you can run the wifi example on L2 RAM (this is not an actual flash, as soon as the aideck resets, this is removed)
~~~~~shell
make clean
make all
make run
~~~~~
