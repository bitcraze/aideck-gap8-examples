---
title: Home
page_id: home
---

This repository contains examples that can be run on the GAP8 as well as some supporting
documentation for developing for the GAP8. The examples are a good starting point for
creating your own applications.

Start with the
[Getting started with the AI deck tutorial](https://www.bitcraze.io/documentation/tutorials/getting-started-with-aideck/).

The examples use one of two firmware combinations on the AI-deck:

* The [NanoCockpit examples](/docs/examples/nanocockpit/index.md) use the
  [ESP32 firmware for NanoCockpit](https://www.bitcraze.io/documentation/repository/aideck-esp-firmware-v2/main/),
  and are flashed to the GAP8 with a JTAG programmer.
* The other examples use the original ESP32 firmware and the GAP8 bootloader, and are flashed to
  the GAP8 over the air.

See [switching firmware](/docs/development/switching-firmware.md) for how to go from one to the
other.

Newer Crazyflie release packages include the ESP32 firmware for NanoCockpit. Upgrading the
Crazyflie with such a release therefore replaces the original ESP32 firmware, see
[switching firmware](/docs/development/switching-firmware.md).
