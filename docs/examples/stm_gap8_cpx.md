---
title: STM-GAP8 CPX communication Example
page_id: stm-gap8-cpx
---

This simple example will show how to make an app for the GAP88 chip, which communicates with the STM (main CPU) on the
Crazyflie using CPX.
CPX packets are sent from the STM to the GAP8, containing a counter that is increased for each packet. The same number is
sent back to the STM in a new CPX packet.

This example is intended to be used together with the STM example "app_stm_gap8_cpx" in the examples folder of the
[crazyflie-firmware repository](https://github.com/bitcraze/crazyflie-firmware)


> Make sure you have completed the [Getting started with the AI deck tutorial](https://www.bitcraze.io/documentation/tutorials/getting-started-with-aideck/) first

First build the stm-gap8-cpx example:

```
$ docker run --rm -v ${PWD}:/module bitcraze/aideck tools/build/make-example examples/other/stm_gap8_cpx all
```

Then flash the example on the AIdeck

```
$ python -m cfloader flash examples/other/stm_gap8_cpx/BUILD/GAP8_V2/GCC_RISCV_FREERTOS/target.board.devices.flash.img deck-bcAI:gap8-fw -w radio://0/80/2M
```
> Note: Replace the 'radio://0/80/2M' with your crazyflie's URI

Build and flash the app-stm-gap8-cpx in the Crazyflie, see the [instruction in the crazyflie-firmware repository](https://github.com/bitcraze/crazyflie-firmware/tree/master/examples/app_stm_gap8_cpx).

Connect to the Crazyflie with the CFclient and open up the console tab. You should see the following output:
```
APP: Hello! I am the stm_gap8_cpx app
...
CPX: GAP8: Starting counter bouncer
...
APP: Sent packet to GAP8 (0)
APP: Got packet from GAP8 (0)
APP: Sent packet to GAP8 (1)
APP: Got packet from GAP8 (1)
APP: Sent packet to GAP8 (2)
APP: Got packet from GAP8 (2)
APP: Sent packet to GAP8 (3)
APP: Got packet from GAP8 (3)
```
