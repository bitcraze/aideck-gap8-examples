---
title: Docker Nina
page_id: docker-nina
---


## Prerequisites

* Ubuntu 18.04 or 20.04
* Install Docker ([installation instructions](https://docs.docker.com/engine/install/ubuntu/))
* Configure Docker to work without Sudo (non-root) with [these instructions](https://docs.docker.com/engine/install/linux-postinstall/#manage-docker-as-a-non-root-user)

## Pull the docker image

There is a [prebuild docker image on docker hub](https://github.com/bitcraze/docker-aideck-nina) for the NINA -wifi module on the AIdeck.

Pull the latest image on your machine
```
docker pull bitcraze/aideck-nina
```

## Flash using docker

First check with `dmesg` (for ubuntu) on which port the programmer is on, and write it down (should look like /dev/ttyUSB0 or another number)

Then use the following line of code in the terminal (make sure to change the --device if your programmer is on another port) and build the firmware (with the menuconfig) and flash the Nina module.

> Working directory: AIdeck_examples/NINA/firmware

```
docker run --rm -it -v $PWD:/module/ --device /dev/ttyUSB0 --privileged -P bitcraze/aideck-nina /bin/bash -c "make clean; make menuconfig; make all; /openocd-esp32/bin/openocd -f interface/ftdi/olimex-arm-usb-tiny-h.cfg -f board/esp-wroom-32.cfg -c 'program_esp32 build/partitions_singleapp.bin 0x8000 verify' -c 'program_esp32 build/bootloader/bootloader.bin 0x1000 verify' -c 'program_esp32 build/ai-deck-jpeg-streamer-demo.bin 0x10000 verify reset exit'"
```

How to configure the Nina wifi credentials, see [the NINA flashing](/docs/test-functions/wifi-streamer.md) at *enter your credentials* instructions
