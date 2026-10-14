---
title: NanoCockpit
page_id: nanocockpit-index
---

[NanoCockpit](https://github.com/idsia-robotics/nanocockpit) is an application framework for
AI-based applications on the Crazyflie with an AI-deck. On the GAP8 it provides building blocks for
camera configuration and acquisition, CPX communication with the ESP32 over SPI, coroutines and
queues, forwarding the state estimate of the Crazyflie to the GAP8 over UART, and tracing.
Applications are built on top of these, for example to run a neural network on board or to stream
camera images and the state of the Crazyflie to a computer over Wi-Fi.

The framework, its examples and a viewer for the computer are found in the
[nanocockpit folder](https://github.com/bitcraze/aideck-gap8-examples/tree/master/nanocockpit) of
this repository:

* `lib/`: the framework
* `examples/`: example applications, described on the pages below
* `client/aideck_cpx_streamer/`: the viewer, which shows and saves the camera images and state

{% sub_page_menu %}

## NanoCockpit and the other examples

NanoCockpit uses its own firmware on the ESP32,
[aideck-esp-firmware-v2](https://www.bitcraze.io/documentation/repository/aideck-esp-firmware-v2/main/).
The GAP8 application and the ESP32 firmware have to match: the NanoCockpit examples need this
firmware, and the other examples in this repository need the original ESP32 firmware. See
[switching firmware](/docs/development/switching-firmware.md) for how to go from one to the other.

The NanoCockpit examples are flashed to the GAP8 with a JTAG programmer. Flashing them over the
air is not supported.

## Requirements

* A JTAG programmer, see the [JTAG programmer documentation](/docs/development/jtag-programmer.md)
* The ESP32 firmware for NanoCockpit, see the
  [aideck-esp-firmware-v2 documentation](https://www.bitcraze.io/documentation/repository/aideck-esp-firmware-v2/main/)
* [Docker](https://www.docker.com/) with the `bitcraze/aideck:3.8.1` image. NanoCockpit is
  developed and tested with GAP SDK 3.8.1, which is older than the version the other examples use.
* [uv](https://docs.astral.sh/uv/) for the viewer. The viewer is a normal Python package, so other
  tools work too, but the instructions use uv.

## More in upstream NanoCockpit

Some parts of NanoCockpit are not included here. They are found in the
[upstream repository](https://github.com/idsia-robotics/nanocockpit):

* the closed-loop follow-me demo, in which the Crazyflie follows a person using the on-board
  neural network
* a ROS 2 viewer
* Wireshark dissectors for the NanoCockpit protocols
* a notebook for analysing traces from the ESP32

## Credit

NanoCockpit was created by Elia Cereda, Alessandro Giusti and Daniele Palossi at IDSIA (USI-SUPSI)
and ETH Zürich. If you use it in an academic context, please cite their
[publication](https://ieeexplore.ieee.org/document/11494460).
