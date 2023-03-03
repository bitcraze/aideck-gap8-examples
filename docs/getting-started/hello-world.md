---
title: Hello World Example
page_id: helloworld
---

This simple example will show how to make an app for the GAP88 chip, which sends a string that is printed out on the console on the [CFclient](https://www.bitcraze.io/documentation/repository/crazyflie-clients-python/master/) through the [CPX framework](/docs/getting_started/cpx.md).

> Make sure you have completed the [Getting started with the AI deck tutorial](https://www.bitcraze.io/documentation/tutorials/getting-started-with-aideck/) first

First build the hello world example:

```
$ docker run --rm -v ${PWD}:/module aideck-with-autotiler tools/build/make-example examples/other/hello_world_gap8 image
```

Then flash the example on the AIdeck

```
$ python -m cfloader flash examples/other/hello_world_gap8/BUILD/GAP8_V2/GCC_RISCV_FREERTOS/target.board.devices.flash.img deck-bcAI:gap8-fw -w radio://0/80/2M
```
> Note: Replace the 'radio://0/80/2M' with your crazyflie's URI

Connect to the Crazyflie with the CFclient and open up the console tab. You should see the following output:
```
CPX: GAP8: Hello world
CPX: GAP8: Hello world
```

