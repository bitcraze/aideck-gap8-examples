---
title: Using CPX
page_id: cpx
---

The Creazyflie Packet eXchange protocol is used to send data to/from the GAP8 from/to other target systems,
like the STM32, ESP32 or the host computer. For more information on the inner workings of CPX, have a look
at the [CPX documentation](/documentation/repository/crazyflie-firmware/master/functional-areas/cpx/).

## Using CPX

The CPX API available in the GAP8 can be viewed in the [cpx.h](https://github.com/bitcraze/aideck-gap8-examples/blob/master/lib/cpx/inc/cpx.h)
header file. From here it's possible to send/receive messages as well as print text to the Crazyflie
client console.
