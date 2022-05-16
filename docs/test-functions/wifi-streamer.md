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
$ docker run --rm -v ${PWD}:/module aideck-with-container tools/build/make-example examples/other/wifi-img-streamer image
$ cfloader flash examples/other/wifi-img-streamer/BUILD/GAP8_V2/GCC_RISCV_FREERTOS/target.board.devices.flash.img deck-bcAI:gap8-fw -w radio://0/80/2M
```

**Note**: if you get `Unable to find image 'aideck-with-autotiler:latest' locally`, make sure that you have done [this step of the getting started guide](https://www.bitcraze.io/documentation/tutorials/getting-started-with-aideck/#setup-the-autotiler-in-docker) properly. Or replace it with the autotiler-less docker image bitcraze/aideck


## Starting the viewer

In order to start the viewer, you will need to the [python package of opencv](https://pypi.org/project/opencv-python/?msclkid=d7172048cae011ecb4ebefc85fe0fc45). Since this has a conflict with the [opencv-python-headless](https://pypi.org/project/opencv-python-headless/) that is a requirement for the [CFclient](https://www.bitcraze.io/documentation/repository/crazyflie-clients-python/master/), it is important to make a python virtual environment like [venv](https://docs.python.org/3/library/venv.html) or an alternative. 

Install opencv python in that python environment as:

```shell
$ pip install opencv-python
```

In order to view the images from the GAP8 connect to the WiFi access point on the deck and
run the following:

```shell
$ cd aideck-gap8-examples/examples/other/wifi-img-streamer
$ python opencv-viewer.py
```

Then you should be able to see the raw/color or jpeg image in the viewer like this:

![opencv viewer](/docs/images/viewer.png)

**Note**: If you like to save the images, you can run the opencv-viewer.py with the `--save` flag.

**Note**: If you enabled your aideck to connect to a wifi point, you would need to check the console tab of the CFclient to retrieve the ipaddress of the aideck. Then you will need to use the `-n` with the ip address with the viewer script.

