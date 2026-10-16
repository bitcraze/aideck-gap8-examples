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
3. **Flash an example** to the GAP8 over the air, see [flashing](/docs/development/flashing.md#over-the-air),
   and set up the Wi-Fi. The released `aideck_gap8_wifi_img_streamer_with_ap.bin` from the
   [releases page](https://github.com/bitcraze/aideck-gap8-examples/releases) sets up its own
   access point, named `WiFi streaming example`. For the other examples, set up the Wi-Fi through
   the Crazyflie firmware, as described in the
   [WiFi video streamer](/docs/examples/wifi-streamer.md#setup-the-wifi-via-the-crazyflie-firmware)
   example.

Flashing the GAP8 bootloader before the ESP32 firmware makes it easy to check that the ESP32
firmware works: the original ESP32 firmware does not run properly while a NanoCockpit application
runs on the GAP8. When it runs, the green LED of the ESP32 blinks and the console of the Crazyflie
shows a line like

```
ESP32: I (483) SYS: Initialized
```

## From the original firmware to NanoCockpit

1. **Flash the ESP32 firmware for NanoCockpit**, as described in its
   [documentation](https://www.bitcraze.io/documentation/repository/aideck-esp-firmware-v2/main/building-and-flashing/).
   When it runs, the AI-deck creates the Wi-Fi network `aideck-nanocockpit`, and the green LED of
   the ESP32 blinks.
2. **Flash a NanoCockpit example** to the GAP8 with a JTAG programmer, for example the
   [NanoCockpit streamer](/docs/examples/nanocockpit/streamer.md). This overwrites the GAP8
   bootloader.

## After upgrading the Crazyflie

Newer Crazyflie release packages include the ESP32 firmware for NanoCockpit. Upgrading the
Crazyflie with such a release replaces the original ESP32 firmware, but does not change the GAP8:
the GAP8 bootloader and the example on it stay.

* To keep using the other examples, flash the original ESP32 firmware back, as in step 2 of
  [From NanoCockpit to the original firmware](#from-nanocockpit-to-the-original-firmware). No JTAG
  programmer is needed, because the GAP8 bootloader is still there.
* To use NanoCockpit, the ESP32 part is already done: continue with step 2 of
  [From the original firmware to NanoCockpit](#from-the-original-firmware-to-nanocockpit).

To upgrade the Crazyflie without flashing the ESP32, choose which targets to flash, and leave out
`bcAI:esp-fw`. For example, `cfcli bootload flash --release --targets stm32-fw,nrf51-fw` only
flashes the Crazyflie itself. Without a list, `--targets` lets you choose interactively.
