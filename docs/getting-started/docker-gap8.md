---
title: Docker Gap8
page_id: docker-gap8
---



## Prerequisites

* Ubuntu 18.04 or 20.04
* Install Docker ([installation instructions](https://docs.docker.com/engine/install/ubuntu/))
* Configure Docker to work without Sudo (non-root) with [these instructions](https://docs.docker.com/engine/install/linux-postinstall/#manage-docker-as-a-non-root-user).

## Pull the docker image

There is a [prebuild docker image on docker hub](https://github.com/bitcraze/docker-aideck) for the GAP8 chip on the AIdeck.

Pull the latest image on your machine
```
docker pull bitcraze/aideck
```

## Configure with Autotiler

The autotiler is necessary for the Facedetection and Mnist example. As the autotiler is licensed by [Greenwaves-technologies](https://greenwaves-technologies.com/), this needs to be implemented seperately.

Open up the container to install the autotiler

```
docker run --rm -it bitcraze/aideck /bin/bash
```

Then in the container write:
```
cd /gap_sdk
source configs/ai_deck.sh
make autotiler
```
This will install the autotiler, which requires you to register your email and get a special URL token to download and install the autotiler.

In a second **separate** terminal on your local machine, commit the changes to the image by first looking up the container ID status:
```
docker ps
```

Copy and past the container ID and replace the <container id> on the line here below, then run it in the separate terminal (also adapt the SDK version if you did before)
```
docker commit <container id> bitcraze/aideck
```

This will save the install of the autotiler on your image. You can close the container in the other terminal with 'exit'. Remember that this needs to be done everytime you pull a new image of the bitcraze/aideck docker image

## Running an Example
On your host navigate to the Makefile you want to execute. For example

```
cd <path/to/AIdeck_examples/Repository>/GAP8/image_processing_examples/image_manipulations
```

The following docker commands will build and run the program on RAM (this will disappear after restarting of the AI-deck). Make sure to replace the /dev/ttyUSB0 with the actual path of your debugger (which can be found by `dmesg` on Ubuntu). Also make sure that the right .cfg file is selected that fits your debugger.

```
docker run --rm -it -v $PWD:/module/data/ --device /dev/ttyUSB0 --privileged -P bitcraze/aideck /bin/bash -c 'export GAPY_OPENOCD_CABLE=interface/ftdi/olimex-arm-usb-tiny-h.cfg; source /gap_sdk/configs/ai_deck.sh; cd /module/data/;  make clean all run'
```

The following docker commands will build and flash the program on the GAP8 (this will NOT disappear after restart of the AI-deck).

```
docker run --rm -it -v $PWD:/module/data/ --device /dev/ttyUSB0 --privileged -P bitcraze/aideck /bin/bash -c 'export GAPY_OPENOCD_CABLE=interface/ftdi/olimex-arm-usb-tiny-h.cfg; source /gap_sdk/configs/ai_deck.sh; cd /module/data/;  make clean all image flash'
```

# Versions

Currently this is only available for version **3.8.1** of the [GAP sdk](https://github.com/GreenWaves-Technologies/gap_sdk). If you would like to try out earlier versions, you will need to build the docker image yourselve locally. The docker image for the gap8 can be found in the [docker-aideck repository](https://github.com/bitcraze/docker-aideck). 