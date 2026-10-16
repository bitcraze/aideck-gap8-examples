---
title: NanoCockpit streamer
page_id: nanocockpit-streamer
---

This example streams camera images from the GAP8 to a computer over Wi-Fi, together with the state
estimate of the Crazyflie if it is sent to the GAP8. The viewer shows the images and can save them,
with the state, as a dataset.

## Prerequisites

* The [requirements for NanoCockpit](/docs/examples/nanocockpit/index.md#requirements)

## Flashing the example

The streamer is released as a ready-made image with the default configuration,
`aideck_gap8_nanocockpit_streamer.bin` on the
[releases page](https://github.com/bitcraze/aideck-gap8-examples/releases). Flash it as described
in [flashing a ready-made image](/docs/development/flashing.md#flashing-a-ready-made-image).

To change the configuration, build and flash the example in `nanocockpit/examples/streamer` as
described in [building and flashing an example](/docs/development/flashing.md#building-and-flashing-an-example).

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

## Starting the viewer

The first time, uv downloads the viewer and the Python version it needs, which needs an internet
connection. The Wi-Fi network of the AI-deck has no internet, so install the viewer before you
connect to it. From the root of this repository, run

```bash
cd nanocockpit/client/aideck_cpx_streamer
uv sync
```

Then connect the computer to the Wi-Fi network of the AI-deck, `aideck-nanocockpit` by default,
and start the viewer in the same directory:

```bash
uv run plt_viewer
```

The viewer connects to `aideck.local` and shows the images, the frame rate it receives and the
number of dropped frames.

The images are sent uncompressed, with 8 bits per pixel, and the viewer shows them in grayscale.
With the color camera, the viewer does not reconstruct the colors.

The default hostname works with the default settings of the ESP32 firmware. Only if you changed
the hostname, or your computer cannot find `aideck.local`, give the hostname or IP address:

```bash
uv run plt_viewer -host [hostname or IP address]
```

To save the received images as a dataset, use

```bash
uv run plt_viewer -save [directory]
```

The viewer creates a subdirectory named after the date and time, with

* `camera_raw/`: one PNG image per frame, as received
* `camera_rgb/`: one JPEG image per frame, with the colors reconstructed as for the color camera
* `metadata.csv`: one line per frame, with the frame's timestamp and the state of the Crazyflie
  that belongs to it

The state is only filled in when the Crazyflie sends it, see the next section.

## Sending the state of the Crazyflie to the GAP8

The streamer attaches the latest state estimate it received from the Crazyflie to each frame. For
the Crazyflie to send it, flash the
[state streamer app](https://github.com/bitcraze/crazyflie-firmware/tree/master/examples/app_state_stream_aideck)
to the Crazyflie. It sends the state to the GAP8 100 times per second over UART1. The viewer then
shows the state with the image, and saves it in `metadata.csv`.

**Note:** The state streamer app cannot be used together with other decks that use UART1, for
example the Lighthouse deck, without changing the firmware. Check the
[deck pin allocation](https://www.bitcraze.io/documentation/system/platform/cf2-expansiondecks/#deck-pin-allocation)
to see which decks use it.
