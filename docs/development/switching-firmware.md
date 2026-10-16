---
title: Switching between NanoCockpit and the original firmware
page_id: switching-firmware
---

The examples in this repository use one of two firmware combinations on the AI-deck:

* **NanoCockpit**: the [ESP32 firmware for NanoCockpit](https://www.bitcraze.io/documentation/repository/aideck-esp-firmware-v2/main/),
  and a [NanoCockpit example](/docs/examples/nanocockpit/index.md) on the GAP8, flashed with a JTAG
  programmer.
* **Original**: the [original ESP32 firmware](https://github.com/bitcraze/aideck-esp-firmware), the
  GAP8 bootloader, and one of the other examples on the GAP8, flashed over the air through the
  Crazyflie.

The GAP8 application and the ESP32 firmware have to match. This page describes in which order to
flash things to go from one combination to the other.

## From NanoCockpit to the original firmware

1. **Flash the GAP8 bootloader** with a [JTAG programmer](/docs/development/jtag-programmer.md), as
   described in the [aideck-gap8-bootloader](https://github.com/bitcraze/aideck-gap8-bootloader#building)
   repository. Flashing a NanoCockpit example over JTAG overwrote it, and it is needed to flash the
   GAP8 over the air.
2. **Flash the original ESP32 firmware** over the air, with `aideck_esp.bin` from the
   [releases page](https://github.com/bitcraze/aideck-esp-firmware/releases), as described in the
   [aideck-esp-firmware](https://github.com/bitcraze/aideck-esp-firmware#flash-using-the-cfloader)
   repository. With cfcli, the command is `cfcli bootload flash --bin bcAI:esp-fw=aideck_esp.bin`.
3. **Flash an example** to the GAP8 over the air, see [flashing](/docs/development/flashing.md#over-the-air), and
   set up the Wi-Fi through the Crazyflie firmware, as described in the
   [WiFi video streamer](/docs/examples/wifi-streamer.md#setup-the-wifi-via-the-crazyflie-firmware)
   example.

Flashing the GAP8 bootloader before the ESP32 firmware makes it easy to check that the ESP32
firmware works: the original ESP32 firmware does not run properly while a NanoCockpit application
runs on the GAP8. When it runs, the LED of the ESP32 blinks and the console of the Crazyflie shows
a line like

```
ESP32: I (483) SYS: Initialized
```

Newer Crazyflie release packages include the ESP32 firmware for NanoCockpit. If you upgrade the
Crazyflie with such a release, it replaces the original ESP32 firmware again. To upgrade the
Crazyflie without flashing the ESP32, select the targets to flash with
`cfcli bootload flash --release --targets`.

## From the original firmware to NanoCockpit

1. **Flash the ESP32 firmware for NanoCockpit**, as described in its
   [documentation](https://www.bitcraze.io/documentation/repository/aideck-esp-firmware-v2/main/building-and-flashing/).
2. **Flash a NanoCockpit example** to the GAP8 with a JTAG programmer, for example the
   [NanoCockpit streamer](/docs/examples/nanocockpit/streamer.md). This overwrites the GAP8
   bootloader.
