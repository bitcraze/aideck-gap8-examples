---
title: Docker Gap8
page_id: docker-gap8
---

# Docker Gap8

Prebuild the docker image while in the same directory as the docker file.

*Note: Depending on your operating system and docker installation you might need to perform the following commands with `sudo`. For more information see the [docker documentation](https://docs.docker.com/engine/install/linux-postinstall/#manage-docker-as-a-non-root-user)*.

```
export GAP_SDK_VERSION=3.7
```
If you want to permanently use that version, we recommend that you put it inside your `.bashrc`. For example by appending it
```
echo 'export GAP_SDK_VERSION=3.7' >> ~/.bashrc 
```

_Note: if you would you would like to also build docker images for SDK version 3.4, 3.5 or 3.6, just replace the previous version number. The build process will checkout the right version of the sdk_

```
docker build --tag gapsdk:${GAP_SDK_VERSION} --build-arg GAP_SDK_VERSION .
```

Open up the container to install the auto tiler
```
docker run --rm -it gapsdk:${GAP_SDK_VERSION} /bin/bash
```

Then in the container write:
```
cd /gap_sdk
source configs/ai_deck.sh
make autotiler
```
This will install the autotiler, which requires you to registrer your email and get a special URL token to download and install the autotiler.

In a seperate terminal on your local machine, commit the changes to the image by first looking up the container ID status:
```
docker ps
```

Copy and past the container ID and replace the <container id> on the line here below, then run it in the seperate terminal (also adapt the sdk version if you did before)
```
export GAP_SDK_VERSION=3.7
docker commit <container id> gapsdk:${GAP_SDK_VERSION}
```

This will save the install of the autotiler on your image.

### Running an Example
On your host navigate to the `make`-file you want to execute. For example

```
cd <path/to/AIdeck_examples/Repository>/GAP8/image_processing_examples/image_manipulations
```

The following docker commands will build and run the program on RAM (this will disappear after restart of the aideck). Make sure to replace the /dev/ttyUSB0 with the actual path of your debugger (which can be found by dmesg on Ubuntu). Also make sure that the right .cfg file is selected that fits your debuggger.

```
docker run --rm -it -v $PWD:/module/data/ --device /dev/ttyUSB0 --privileged -P gapsdk:${GAP_SDK_VERSION} /bin/bash -c 'export GAPY_OPENOCD_CABLE=interface/ftdi/olimex-arm-usb-tiny-h.cfg; source /gap_sdk/configs/ai_deck.sh; cd /module/data/;  make clean all run'
```

The following docker commands will build and flash the program on the GAP8 (this will NOT disappear after restart of the aideck).

```
docker run --rm -it -v $PWD:/module/data/ --device /dev/ttyUSB0 --privileged -P gapsdk:${GAP_SDK_VERSION} /bin/bash -c 'export GAPY_OPENOCD_CABLE=interface/ftdi/olimex-arm-usb-tiny-h.cfg; source /gap_sdk/configs/ai_deck.sh; cd /module/data/;  make clean all image flash'
```
