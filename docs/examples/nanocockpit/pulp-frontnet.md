---
title: NanoCockpit PULP-Frontnet
page_id: nanocockpit-pulp-frontnet
---

This example runs the PULP-Frontnet neural network on the GAP8. For each camera frame, the network
estimates the pose of a person in front of the Crazyflie: the position (x, y, z) relative to the
Crazyflie, and the direction the person is facing. The example streams the camera images to the
computer with the latest estimate, and also sends the estimate to the Crazyflie over UART1.

The network was generated with [DORY](https://github.com/pulp-platform/dory). The generated code and
the weights are included in the example, so DORY is not needed to build it.

## Prerequisites

* The [requirements for NanoCockpit](/docs/examples/nanocockpit/index.md#requirements)

There is no released binary of this example, it has to be built.

## Configuring the example

The settings are found in
[config.h](https://github.com/bitcraze/aideck-gap8-examples/blob/master/nanocockpit/examples/pulp-frontnet/config.h).
The camera settings are explained on the
[NanoCockpit streamer](/docs/examples/nanocockpit/streamer.md#configuring-the-example) page. This
example is configured for the HALF format at 30 frames per second by default.

## Building and flashing the example

Build and flash the example in `nanocockpit/examples/pulp-frontnet` as described in
[building and flashing an example](/docs/development/flashing.md#building-and-flashing-an-example).

## Viewing the results

Start the viewer as described for the
[NanoCockpit streamer](/docs/examples/nanocockpit/streamer.md#starting-the-viewer). When it
receives an estimate, the viewer shows a second panel with the estimated position and direction of
the person, seen from above.

The network was trained on images of a person moving around in a room in front of the drone, and
it works less well in environments that differ from its training data. It does not work well with
a person sitting close to the camera, for example at a desk; standing further away, with more of
the body in view, works better.

## Using the estimate on the Crazyflie

The estimate is sent to the Crazyflie over UART1, but the Crazyflie firmware does not use it. The
closed-loop follow-me demo in [upstream NanoCockpit](https://github.com/idsia-robotics/nanocockpit)
shows how the Crazyflie can follow the person based on the estimate.
