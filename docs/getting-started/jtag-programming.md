---
title: JTAG programming
page_id: jtag-programming
---

## JTAG adapter/debugger
The GAP8 SDK has recently added support for OpenOCD and debuggers with a FTDI interface should be usable. We have tested the Olimex ARM-USB-TINY-H with success and also the JLINK. The same applies to the ESP32 and a rule of thumb is that if the JTAG debugger is compatible with the ESP32, it also works for the GAP8. The STLink v2 will not work as it can only debug Cortex cores.

Tested programmers at Bitcraze AB:
- Olimex ARM-USB-TINY-H

> It's a good choice to follow our hardware selection, you could find detailed information about Olimex Debugger in [USER’S MANUAL](https://www.olimex.com/Products/ARM/JTAG/_resources/ARM-USB-TINY_and_TINY_H_manual.pdf), it's helpful for you to learn how to use debugger with OpenOCD. The Olimex debugger needs a USB type B cable which might not be that common anymore, just a heads up.

## JTAG connectors
There are two Cortex-M 10pin (2×5, 1.27mm pitch) JTAG interfaces on the AI-deck so that both the GAP8 and the ESP32 can be programmed and debugged easily. They are edge mounted on the PCB to save height. The GAP8 JTAG is located on the left side and the ESP32 JTAG to the right when viewing the board from the top and camera front. Note that pin-1 is located to the left, marked with a 1 on the bottom side of the board.

Use a 2×5 pin 1.27mm (0.05“) flat cable together with our [debug adapter](https://www.bitcraze.io/debug-adapter-kit) or e.g. an adapter such as the [Olimex ARM-JTAG-20-10](https://www.olimex.com/Products/ARM/JTAG/ARM-JTAG-20-10), is needed to connect the debugger and the AI-deck. 

![JTAG cable connected to GAP8](ai-deck-jtag-connecting.png)

## Programming/Debugging
For details follow each MCUs guide. The [getting started guide](/docs/getting-started/getting-started.md) contains basic instructions as well.

ESP32 JTAG Debugging
https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/jtag-debugging/index.html

GAP8 JTAG debugging
https://github.com/GreenWaves-Technologies/gap_sdk#openocd
