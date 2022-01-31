# Image streaming example

**Note:** This example uses a new protocol that is still not finished, so this is
still very much experimental and will change in the future before replacing the old example.

This example will stream images from the AI-deck to a computer using WiFi. It can be set to
either stream raw data or JPEG compressed data.

## How to use the example

This example (and the ESP/Crazyflie firmware it needs) is still under development, so care needs to
be taken when running it.

### GAP8 firmware

Clone the GAP8 test firmware and select the correct branch:

```text
git clone -b cf-link https://github.com/bitcraze/AIdeck_examples.git
```

Go into the example and create a file for the WiFi SSID/key:
```text
cd AIdeck_examples/GAP8/test_functionalities/wifi_jpeg_streamer
touch wifi_credentials.h
```

Edit this file to contain the setup for your local WiFi (note that this file is in the ```.gitignore```):
```c
static const char ssid[] = "YourSSID";
static const char passwd[] = "YourWiFiKey";
```

Build and flash the application. **Note:** Due to a bug in the Makefiles in the SDK you will need to use
our toolbelt to build/flash the application, or fix the Makefiles as described in [this issue](https://github.com/GreenWaves-Technologies/gap_sdk/issues/266).

```text
docker pull bitcraze/aideck:4.8.0
docker run --rm -it -v $PWD:/module/data/ --device /dev/ttyUSB0 --privileged -P bitcraze/aideck:4.8.0 /bin/bash -c 'export GAPY_OPENOCD_CABLE=interface/ftdi/olimex-arm-usb-tiny-h.cfg; source /gap_sdk/configs/ai_deck.sh; cd /module/data/;  make clean all image flash'
```

### ESP32 firmware

Checkout the ESP32 firmware, build in flash it. Note that this firmware requires the ESP IDF 4.3.1.

```text
git clone https://github.com/bitcraze/aideck-esp-firmware.git
cd aideck-esp-firmware
idf.py app bootloader
docker run --rm -it -v $PWD:/module/ --device /dev/ttyUSB0 --privileged -P bitcraze/aideck-nina /bin/bash -c "/openocd-esp32/bin/openocd -f interface/ftdi/olimex-arm-usb-tiny-h.cfg -f board/esp-wroom-32.cfg -c 'program_esp32 build/bootloader/bootloader.bin 0x1000 verify' -c 'program_esp32 build/aideck_esp.bin 0x10000 verify reset exit'"
```

### Crazyflie firmware

If you're also connecting the Crazyflie you will need a new version of this firmware as well, otherwise the console
in the Crazyflie python client will fill up with garbage.

This code is in PR [904](https://github.com/bitcraze/crazyflie-firmware/pull/904) in the crazyflie-firmware repository.

Clone, build and flash it.

```text
git clone -b cf-link-aideck https://github.com/bitcraze/crazyflie-firmware.git
cd crazyflie-firmware
make DEBUG=1 all && make cload
```

### Viewer on the PC

Once everything is flashed, power cycle the hardware and wait for it to connect to the WiFi. The IP
can either be found by have a USB<>UART adapter and listening either to the GAP8 (UART1 TX on expansion connector),
the ESP32 (IO1 on the expansion header) or checking the assigned IP on your router interface.

The viewer needs openCV, it can be started by using the following command in the directory of the GAP8 example
code.

```text
python opencv-viewer.py -n <the-ip>
```

**Note:** The openCV version installed byt the GAP8 SDK is not compatible with this example (you will get and error
about along the lines of: ```The function is not implemented. Rebuild the library with Windows, GTK+ 2.x or Cocoa support.```), so if you're
not using a virtual environment for the SDK you will have to do it for the example. Then run the following command
in the GAP8 example firmware folder.

```text
virtualenv venv
venv/bin/activate
```
