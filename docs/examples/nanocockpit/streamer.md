---
title: NanoCockpit streamer
page_id: nanocockpit-streamer
---

This example streams camera images from the GAP8 to a computer over Wi-Fi, together with the state
estimate of the Crazyflie if it is sent to the GAP8. The viewer shows the images and can save them,
with the state, as a dataset.

## Prerequisites

* The [requirements for NanoCockpit](/docs/examples/nanocockpit/index.md#requirements)

For flashing, mount the AI-deck on the Crazyflie and connect the JTAG programmer to the GAP8 JTAG
connector. We recommend disconnecting the programmer when flashing is done.

## Flashing the released binary

The streamer is released as a ready-made binary with the default configuration. Download
`aideck_gap8_nanocockpit_streamer.bin` from the
[releases page](https://github.com/bitcraze/aideck-gap8-examples/releases) and run the following
command in the directory where you saved it. The command is for the Olimex ARM-USB-TINY-H:

```bash
docker run --rm -it -v "${PWD}:/module" -w /module \
    --device-cgroup-rule="c 189:* rmw" \
    -v /dev/bus:/dev/bus:ro -v /dev/serial:/dev/serial:ro \
    bitcraze/aideck:3.8.1 \
    bash -c 'source /gap_sdk/configs/ai_deck.sh; f=aideck_gap8_nanocockpit_streamer.bin; gap8-openocd -c "gdb_port disabled; telnet_port disabled; tcl_port disabled" -c "script interface/ftdi/olimex-arm-usb-tiny-h.cfg; script target/gap8revb.tcl; script tcl/flash_image.tcl; script tcl/jtag_boot.tcl; gap_flash_raw_hyper $f $(stat -c %s $f) /gap_sdk/tools/gap8-openocd-tools; exit;"'
```

Disconnect the programmer and power cycle the Crazyflie when the command is done.

**Note:** Flashing over JTAG overwrites the GAP8 bootloader, see
[switching firmware](/docs/development/switching-firmware.md) if you later want to use the other
examples.

## Configuring the example

The settings of the example are found in
[config.h](https://github.com/bitcraze/aideck-gap8-examples/blob/master/nanocockpit/examples/streamer/config.h).
The most important ones are

* **Resolution and frame rate:** `HIMAX_FORMAT` and `HIMAX_FRAME_RATE`. The default is the HALF
  format at 60 frames per second. The HALF format sees the full field of view of the sensor at half
  the resolution. The images are cropped slightly, so they are 160x160 pixels in HALF format and
  320x320 pixels in FULL format. Which frame rates the sensor can reach in each format is listed in
  the [table of supported frame rates](https://github.com/idsia-robotics/nanocockpit/blob/main/src/gap/docs/himax_supported_framerates.md)
  in upstream NanoCockpit.
* **Exposure:** auto-exposure is disabled (`HIMAX_AE`), and images are captured with a fixed
  exposure time `HIMAX_INTEGRATION_MS` and analog gain `HIMAX_AGAIN`. The defaults suit indoor
  light. Outdoors, the image will be overexposed: lower the exposure time or the gain. The exposure
  time must be shorter than the time between two frames.
* **Bidirectional SPI:** `CPX_SPI_BIDIRECTIONAL` is disabled, so the GAP8 only sends to the ESP32.
  Enable it if the GAP8 must also receive data from the computer, or to see round-trip times in the
  viewer. It lowers the SPI clock from 30 MHz to 7.2 MHz because of a limitation of the AI-deck
  hardware, which lowers the frame rate that can be reached.

## Building and flashing the example

From the root of this repository, start a console in the container:

```bash
docker run --rm -it -v "${PWD}:/module/data/" -P \
    -e "GAPY_OPENOCD_CABLE=interface/ftdi/olimex-arm-usb-tiny-h.cfg" \
    --device-cgroup-rule="c 189:* rmw" \
    -v /dev/bus:/dev/bus:ro -v /dev/serial:/dev/serial:ro \
    bitcraze/aideck:3.8.1 \
    /bin/bash -c "source /gap_sdk/configs/ai_deck.sh; cd /module/data/; bash"
```

In the container, build and flash the example:

```bash
cd nanocockpit/examples/streamer
make clean build image flash
```

Disconnect the programmer and power cycle the Crazyflie when flashing is done.

**Note:** USB in Docker is only supported on Linux.

## Starting the viewer

Connect the computer to the Wi-Fi network of the AI-deck, `aideck-nanocockpit` by default. Then,
from the root of this repository, run

```bash
cd nanocockpit/client/aideck_cpx_streamer
uv run plt_viewer
```

The first time, uv installs the viewer and the Python version it needs. The viewer connects to
`aideck.local` and shows the images, the frame rate it receives and the number of dropped frames.

The images are sent uncompressed, with 8 bits per pixel, and the viewer shows them in grayscale.
With the color camera, the colors are not reconstructed.

The default hostname works with the default settings of the ESP32 firmware. Only if you changed
the hostname, or your computer cannot find `aideck.local`, give the hostname or IP address:

```bash
uv run plt_viewer -host [hostname or IP address]
```

To save the received images as a dataset, use

```bash
uv run plt_viewer -save [directory]
```

The directory gets one PNG image per frame and a `metadata.csv` file, with the state of the
Crazyflie that belongs to each frame.

## Sending the state of the Crazyflie to the GAP8

The streamer attaches the latest state estimate it received from the Crazyflie to each frame. For
the Crazyflie to send it, flash the
[state streamer app](https://github.com/bitcraze/crazyflie-firmware/tree/master/examples/app_state_stream_aideck)
to the Crazyflie. It sends the state to the GAP8 100 times per second over UART1. The viewer then
shows the state with the image, and saves it in `metadata.csv`.

**Note:** The state streamer app cannot be used together with other decks that use UART1. Check
the [deck pin allocation](https://www.bitcraze.io/documentation/system/platform/cf2-expansiondecks/#deck-pin-allocation)
to see which decks use it.
