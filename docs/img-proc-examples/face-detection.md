---
title: Face detection example
page_id: face-detection
---

This is the face detection application based on the example as developed by Greenwaves technologies. It is a bit more tailor made towards the AIdeck and uses the wifi streamer to stream the output to your computer. 

This was tested on **GAP_SDK version 3.5**. Make sure that next to `make SDK`, you also do `make gap_tools`.

Go to the folder: *GAP8/image_processing_examples/FaceDetection*

To make the facedetection application

    make clean
    make all PMSIS_OS=pulpos

To try out the code on RAM with help of the debugger:

    make run

To flash the code fully on the ai deck:

    make image
    make flash

