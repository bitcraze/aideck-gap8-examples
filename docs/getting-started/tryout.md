---
title: First time try out
page_id: tryout
---
# Try out your AI-deck for the first time

When you first receive your AIdeck, it should be flashed with a wifi streamer example of the camera image stream. Once the AI-deck is powered up by the Crazyflie, it will automatically create an hotspot called 'Bitcraze AI-deck Example'.

*If you do not find the hotspot 'Bitcraze AI-deck Example', it might be that the example has not been flashed on your AIdeck, depended on the source where you bought your copy from, then you might need to flash the wifi example on yourselve (go [here](/docs/test-functions/wifi-streamer.md)).*

On your crazyflie, make sure that it is updated to the latest firmware. Also make sure that you force the AIdeck driver in your config.mk, as it will reset the AI-deck properly upon startup.
 
`#CFLAGS += -DDECK_FORCE=bcAIDeck`

In this repo in the folder named 'NINA' you will find a file called *viewer.py*. If you run this with python (preferably version 3), you will be able to see the camera image stream on your computer. This will confirm for you that your AIdeck is working.

After trying out the WiFi demo and if you have a JTAG-ready programmer at your disposal, set up your development program with the [getting-started guide](getting-started.md), which also contains links to the GAP-SDK documentation from Greenwave technologies try out the face detector example we demo-ed in this blog post.
