---
title: First time try out
page_id: tryout
---
# Try out your AI-deck for the first time

## Check whether the wifi module has been flashed
When you first receive your AI-deck, it should be flashed with a wifi streamer example of the camera image stream. Once the AI-deck is powered up by the Crazyflie, it will automatically create a hotspot called 'Bitcraze AI-deck Example'.

> If you do not find the hotspot 'Bitcraze AI-deck Example', it might be that the example has not been flashed on your AI-deck, depending on the source where you bought your copy from, then you might need to flash the wifi example on yourself (go [here](/docs/test-functions/wifi-streamer.md)).

## Update firmware and force the AI-deck driver
On your Crazyflie, make sure that it is updated to the latest firmware. Also make sure that you force the AI-deck driver in your config.mk as explained [here](https://www.bitcraze.io/documentation/repository/crazyflie-firmware/master/development/howto/#enabling-the-driver
), as it will reset the AI-deck properly upon startup.

```
#CFLAGS += -DDECK_FORCE=bcAIDeck
```

## See the image stream
In this repo in the folder named 'NINA' you will find a file called *[viewer.py](https://github.com/bitcraze/AIdeck_examples/blob/master/NINA/viewer.py)*. If you run this with python (preferably version 3), you will be able to see the camera image stream on your computer. This will confirm for you that your AI-deck is working.

## Explore more
After trying out the WiFi demo and if you have a JTAG-ready programmer at your disposal, set up your development program with the [getting-started guide](getting-started.md), which also contains links to the GAP-SDK documentation from Greenwave technologies. You can also check AI-deck category on the Bitcraze blog.

You can find some discussions about the AI-deck in [Bitcraze forum](https://forum.bitcraze.io/viewforum.php?f=21) and [Issues](https://github.com/bitcraze/AIdeck_examples/issues).
