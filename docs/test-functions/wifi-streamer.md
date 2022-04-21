---
title: WiFi Video Streamer
page_id: wifi-streamer
---

This example streams JPEG or raw images from the GAP8 to the host where they are displayed and
colorized.

## Prerequisites

* Completed the [Getting started with the AI deck tutorial](https://www.bitcraze.io/documentation/tutorials/getting-started-with-aideck/)
* opencv-python installed on host

## Configuration

To select which mode (RAW or JPEG) change the code below in the ```wifi-img-streamer.c``` file.

```c
typedef enum
{
  RAW_ENCODING = 0,
  JPEG_ENCODING = 1
} __attribute__((packed)) StreamerMode_t;

static StreamerMode_t streamerMode = RAW_ENCODING;
```

## Building and flashing the example

To build and flash the example run the following:

```shell
$ cd aideck-gap8-examples
$ docker run --rm -v ${PWD}:/module bitcraze/aideck tools/build/make-example examples/other/wifi-img-streamer image
$ cfloader flash examples/other/wifi-img-streamer/BUILD/GAP8_V2/GCC_RISCV_FREERTOS/target.board.devices.flash.img deck-bcAI:gap8-fw -w radio://0/80/2M
```

## Starting the viewer

In order to view the images from the GAP8 connect to the WiFi access point on the deck and
run the following:

```shell
$ cd aideck-gap8-examples/examples/other/wifi-img-streamer
$ python opencv-viewer.py -n 192.168.4.1
```
