---
title: Face detection example
page_id: face-detection
---

This is the face detection application based on the example as developed by Greenwaves technologies. It is a bit more tailor made towards the AI-deck and uses the wifi streamer to stream the output to your computer.

This was tested on **GAP_SDK version 4.8.0.2**, which at the moment of writing was the newest we had a docker container for.

## Building with Docker GAP-SDK

Make sure to follow the [getting started with the AI deck tutorial](https://www.bitcraze.io/documentation/tutorials/getting-started-with-aideck/) before continuing.

To clean, compile and flash the FaceDetection example you have to be in the aideck-gap8-examples directory and execute:

```
$ docker run --rm -v ${PWD}:/module aideck-with-autotiler tools/build/make-example examples/image_processing/FaceDetection clean model build image
```
Then flash the example with cfloader:
```
$ cfloader flash examples/image_processing/FaceDetection/BUILD/GAP8_V2/GCC_RISCV_FREERTOS/target.board.devices.flash.img deck-bcAI:gap8-fw -w [CRAZYFLIE URI]
```

> Replace `[CRAZYFLIE_URI]` with the URI of your crazyflie in the form radio://0/2M/80/E7E7E7E7E7


If you configured your Crazyflie firmware such that the AIdeck will act as access point (as described in the [wifi-streamer example](/docs/simple-examples/wifi-streamer.md)) you can now just connect to it. If you configured it to connect to an existing network you should make sure your computer is in the same network and you need to check the IP address of the AIdeck - for example connect to it through the cfclient and check the console prints.

Now you can run the image viewer:

    python3 opencv-viewer.py -n "AI_DECK_IP"

where "AI_DECK_IP" should be replaced with for example 192.168.4.1 (which is the default value and the value used if the AIdeck acts as access point, so in this case it can be omitted).

Now you should see something like this:

![image streamer](/docs/images/face_detection.png)

Note that the face detection does not work great under all conditions - try with a white background and dark hair...

In the makefile you can comment the following line if you would like to disable the streamer:

    APP_CFLAGS += -DUSE_STREAMER

Or - what is way more fun to play with - you can set the resolution of the streamed image with _STREAM\_W_ and _STREAM\_H_. Note that you cannot set values higher than 324x244 and that unproportional changes will result in distorted images. Note also that you need to adapt the resolution in the _opencv-viewer.py_ as well.
