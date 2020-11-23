---
title: Face detection example
page_id: face-detection
---

This is the face detection application based on the example as developed by Greenwaves technologies. It is a bit more tailor made towards the AI-deck and uses the wifi streamer to stream the output to your computer. 

This was tested on **GAP_SDK version 3.7**. Make sure that next to `make SDK`, you also do `make gap_tools`.

> Working directory: AIdeck_examples/GAP8/image_processing_examples/FaceDetection

To make the face detection application

    make clean
    make all PMSIS_OS=pulpos

To try out the code on RAM with help of the debugger:

    make run

To flash the code fully on the ai deck:

    make image
    make flash
    

In the makefile you can uncomment the following lines if you would like to use the himax camera (works in SDK 3.7) or the streamer (only works until 3.5) :

    # APP_CFLAGS += -DUSE_CAMERA
    # APP_CFLAGS += -DUSE_STREAMER 

After that, you can also use `viewer.py` to see the image stream. The rectangle generated around your face is implemented by the firmware.
