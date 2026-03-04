# Nanocockpit GAP8 examples

## Requirements

Note that you need to flash the compatible NINA (ESP32) code for the nanocockpit to work, found here: TODO
Also note that the NINA firmware will break CPX towards GAP8 - meaning you need to have a JTAG debugger to update GAP8! Do not proceed if you do not have a debugger.

## Build instructions

The nanocockpit GAP8 code is composed of reusable components, under `lib/`, and a streamer example.

Configure your computer to use the GAP SDK Docker container as described [here](https://www.bitcraze.io/documentation/repository/aideck-gap8-examples/master/)(The nanocockpit framework was generally developed and tested with 3.8.1, however, for the nanocockpit streamer the newest release is also compatible).

Start a bash console inside your docker:
```sh
docker run --rm -it -v "${PWD}:/module/data/" -P -e "GAPY_OPENOCD_CABLE=interface/ftdi/olimex-arm-usb-tiny-h.cfg" --device-cgroup-rule="c 189:* rmw" -v /dev/bus:/dev/bus:ro -v /dev/serial:/dev/serial:ro bitcraze/aideck:3.8.1         
/bin/bash -c "
            source /gap_sdk/configs/ai_deck.sh; \
            cd /module/data/; \
            bash
        "
```

Select the example that you want to build, for example the nanocockpit WiFi-streamer (note that it is important that you start your bash console outside the nanocockpit folder, otherwise the lib files will not be found):
```sh
cd nanocockpit-streamer 
```

Compile and run the code over JTAG:

```shell
make all run
```

Flash the code:

```shell
make flash
```

## Matplotlib viewer

You can view the images with the plotter, found in aideck_cpx_streamer

Install the viewer as a Python package:

```shell
$ pip install aideck_cpx_streamer
```

<!-- The `-e` flag allows you to edit files in your package without having to re-install it every time. -->

Launch a minimal client that shows the received data in a GUI window:

```shell
$ plt_viewer
```

By default the client will attempt to connect to `aideck.local`, the default mDNS hostname used by the NINA code. You can connect to a different hostname using:

```shell
$ plt_viewer -host your-hostname.local
```

This client also supports saving the received data in a simple dataset format:

```shell
$ plt_viewer -save dataset/
```

The resulting `dataset/` directory will contain one PNG image for each received camera frame and a `metadata.csv` containing the onboard state estimation corresponding to each frame, plus some extra information.

## Expected results

You can expect a framerate of ~30fps without, and ~60fps with disabling bidirectional CPX. 