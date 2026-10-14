# AI-deck examples repository

> ⚠️ Important Notice: The GreenWaves Technologies website is down, preventing fetching and compiling the autotiler. This means that deploying neural networks through [gap_sdk](https://github.com/GreenWaves-Technologies/gap_sdk) or our Docker image is not possible unless you already have the file. However, you can still deploy neural networks using [DORY](https://github.com/pulp-platform/dory) as an alternative.
>
> For more details, updates and workarounds, see our announcement [here](https://github.com/orgs/bitcraze/discussions/1854).

Check out the [documentation](https://www.bitcraze.io/documentation/repository/aideck-gap8-examples/master/)
for starting guides.

The examples use one of two firmware combinations on the AI-deck:

* [NanoCockpit](nanocockpit/), in the `nanocockpit` folder, uses the
  [ESP32 firmware for NanoCockpit](https://github.com/bitcraze/aideck-esp-firmware-v2) and is
  flashed to the GAP8 with a JTAG programmer.
* The other examples, in the `examples` folder, use the original ESP32 firmware and the GAP8
  bootloader.

Newer Crazyflie release packages include the ESP32 firmware for NanoCockpit. Upgrading the
Crazyflie with such a release therefore replaces the original ESP32 firmware, see
[switching firmware](docs/development/switching-firmware.md).
