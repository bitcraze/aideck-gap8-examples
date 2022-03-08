# Image streaming example

This example will stream images from the AI-deck to a computer using WiFi. It can be set to
either stream raw data or JPEG compressed data. If the raw data is Bayer encoded the viewer
will colorize the image.

## How to use the example

### GAP8 firmware

This example can be build to either set up the WiFi as an access point, or to let
another party handle the WiFi (like the Crazyflie). The parameter that controls this is
```SETUP_WIFI_AP``` and it is enabled by setting it to ```1```.

```text
docker pull bitcraze/aideck:4.8.0
docker run --rm -it -v $PWD:/module/data/ --device /dev/ttyUSB0 --privileged -P bitcraze/aideck:4.8.0 /bin/bash -c 'export GAPY_OPENOCD_CABLE=interface/ftdi/olimex-arm-usb-tiny-h.cfg; source /gap_sdk/configs/ai_deck.sh; cd /module/data/;  make clean all image flash'
```

### ESP32/Crazyflie firmware

For the example to work you will need a recent version of the Crazyflie and ESP firmware.

### Viewer on the PC

Once everything is flashed, power cycle the hardware and wait for it to connect to the WiFi. If
you're connecting the AI-deck to an access point you can see the assigned IP in the Crazyflie client
console. If you're using the example as AP, then the IP of the AI-deck will be ```192.168.4.1```.

The viewer needs openCV, it can be started by using the following command in the directory of
the GAP8 example code.

```text
python opencv-viewer.py -n <the-ip>
```

**Note:** The openCV version installed by the GAP8 SDK is not compatible with this example (you will get and error
about along the lines of: ```The function is not implemented. Rebuild the library with Windows, GTK+ 2.x or Cocoa support.```), so if you're
not using a virtual environment for the SDK you will have to do it for the example. Then run the following command
in the GAP8 example firmware folder.

```text
virtualenv venv
venv/bin/activate
```
