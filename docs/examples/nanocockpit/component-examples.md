---
title: NanoCockpit component examples
page_id: nanocockpit-component-examples
---

These small examples each show how to use one part of the NanoCockpit framework. They are a good
starting point for writing your own NanoCockpit application.

* `cpx`: shows the sending side of the low-level CPX API. Every half second, the GAP8 sends a small
  test packet addressed to the computer and prints that it did. Nothing on the computer side is
  included to receive the packets.
* `coroutine`: runs several tasks at the same time with the coroutines of `lib/coroutine.h`.
* `queue`: shows how to put elements in a queue and take them out again (`lib/queue.h`).
* `queue_async`: a producer task and a consumer task that exchange elements through an asynchronous
  queue.

The code is found in the
[examples folder](https://github.com/bitcraze/aideck-gap8-examples/tree/master/nanocockpit/examples).

## Prerequisites

* The [requirements for NanoCockpit](/docs/examples/nanocockpit/index.md#requirements)

## Building and running an example

The examples print what they do on the console of the GAP8, which is shown in the terminal when the
example is run over JTAG. Start a console in the container as described for the
[NanoCockpit streamer](/docs/examples/nanocockpit/streamer.md#building-and-flashing-the-example),
then build and run an example, for example `coroutine`:

```bash
cd nanocockpit/examples/coroutine
make clean build run
```

This loads the example into the memory of the GAP8 and starts it, without writing it to flash.
Keep the programmer connected while the example runs.

`queue` and `queue_async` stop by themselves when they are done. `cpx` and `coroutine` repeat what
they do until you stop them with Ctrl-C. The error messages printed after Ctrl-C are expected.
