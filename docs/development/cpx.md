---
title: Using CPX
page_id: cpx
redirects:
 - /docs/getting-started/cpx
---

The Crazyflie Packet eXchange protocol is used to send data to/from the GAP8 from/to other target systems,
like the STM32, ESP32 or the host computer. For more information on the inner workings of CPX, have a look
at the [CPX documentation](https://www.bitcraze.io/documentation/repository/crazyflie-firmware/master/functional-areas/cpx/).

## Using CPX

The CPX API available in the GAP8 can be viewed in the [cpx.h](https://github.com/bitcraze/aideck-gap8-examples/blob/master/lib/cpx/inc/cpx.h)
header file. From here it's possible to send/receive messages as well as print text to the Crazyflie
client console. This API is for the original ESP32 firmware.

## CPX in NanoCockpit

NanoCockpit has its own CPX implementation for the GAP8, in
[nanocockpit/lib/cpx](https://github.com/bitcraze/aideck-gap8-examples/tree/master/nanocockpit/lib/cpx),
which works with the ESP32 firmware for NanoCockpit. It only supports communication between the
GAP8 and the computer, through the ESP32: there is no route to the Crazyflie and no console in the
Crazyflie client. The [cpx component example](/docs/examples/nanocockpit/component-examples.md)
shows how to use it.
