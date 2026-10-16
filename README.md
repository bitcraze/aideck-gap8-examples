# AI-deck examples repository

Check out the [documentation](https://www.bitcraze.io/documentation/repository/aideck-gap8-examples/master/)
for starting guides.

The examples use one of two firmware combinations on the AI-deck:

* [NanoCockpit](nanocockpit/), in the `nanocockpit` folder, uses the
  [ESP32 firmware for NanoCockpit](https://github.com/bitcraze/aideck-esp-firmware-v2) and is
  flashed to the GAP8 with a JTAG programmer.
* The other examples, in the `examples` folder, use the original ESP32 firmware and the GAP8
  bootloader.

We recommend NanoCockpit. The other examples need the original firmware, which is only needed to
flash the GAP8 over the air without a JTAG programmer, or to send CPX packets between the GAP8 and
the Crazyflie.

Newer Crazyflie release packages include the ESP32 firmware for NanoCockpit. Upgrading the
Crazyflie with such a release therefore replaces the original ESP32 firmware, see
[switching firmware](docs/development/switching-firmware.md).

**Note:** The Autotiler of the GAP8 SDK can no longer be downloaded: GreenWaves Technologies, the
company behind the GAP8, went into liquidation in 2025, and its website is no longer online. The
classification and face detection examples need the Autotiler, so they can only be built if you
already have it. The [PULP-Frontnet](docs/examples/nanocockpit/pulp-frontnet.md) example in
NanoCockpit runs a neural network generated with [DORY](https://github.com/pulp-platform/dory)
instead. See our [announcement](https://github.com/orgs/bitcraze/discussions/1854) for more details.
