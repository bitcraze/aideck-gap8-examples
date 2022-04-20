---
title: Flashing
page_id: flashing
---

# Flashing the GAP8

There's two different ways to flash firmware on the GAP8, via JTAG or via cfloader (Crazyflie PA).

## cfloader

If you want over-the-air firmware updates you can use the cfloader by running the following command
(replacing [binary] and [radio-address] with proper values). Note, this requires the GAP8 bootloader
to be flashed on the GAP8.

```bash
$ cfloader flash [binary] deck-bcAI:gap8-fw -w [radio-address]
```

## JTAG

You can flash the example with an Olimex ARM-USB-TINY-H JTAG using the following command (replacing example-directory with the example you want to flash):

```bash
docker run --rm -v ${PWD}:/module --device /dev/ttyUSB0 --privileged -P bitcraze/aideck tools/build/make-example [example-directory] flash

```

**Note:** This will overwrite the booloader!