---
title: GAP8
page_id: gap8
redirects:
 - /docs/getting-started/gap8
---

The GAP8 is a 9-core IoT application processor produced by Greenwaves. It is based on RISC-V and the [PULP](https://pulp-platform.org) (Parallel Ultra-Low-Power Processing Platform) open-source platform.
You find more information about the processor itself on the [Greenwaves homepage](https://greenwaves-technologies.com/gap8_mcu_ai/) and in the [manual](https://greenwaves-technologies.com/manuals/BUILD/HOME/html/index.html), but to give you a quick start with the AIdeck we also explain some of the features of GAP8 and how it is connected on the AIdeck here.

## Cores ##

You have one separate core, the *fabric controller* (FC), which you can imagine as the system's "boss" - the core with access to the peripherals, its own L1 memory and its own instruction cache. The FC can then offload tasks to the *cluster* (CL). The CL is very powerful when working together - it has 8 cores which share L1 memory and an instruction cache. The shared resources mean great acceleration when all cores are executing the same code on different data - but keep in mind that trying to use them individually will be highly ineffective, as you will get a lot of instruction cache misses. Great examples are machine learning as the [classification example](/docs/examples/classification-demo.md).

## Memory ##

Internally, GAP8 only has non-persistent memory. The memory is divided into three parts:
- the FC L1 memory (16kB)
- the CL L1 memory (64kB)
- the shared L2 memory (512kB)

Externally additional memory can be connected. In our case (the AIdeck), this is a combined flash/RAM chip with a HyperBus interface. As this is our third level of memory, we call it L3. We have
- 64Mb of L3 RAM
- of L3 flash

## Peripherals ##

GAP8 features a rich peripheral set; most important for us are:
- CPI to connect to the camera
- SPI to connect to the ESP32 in the NINA WiFi module
- UART and GPIOs to connect to the STM32 on the Crazyflie

The data transfers from peripherals can be handled by a so-called *micro DMA*, which relieves the FC of some load.

## SDK ##

> ⚠️ Important Notice: The GreenWaves Technologies website is down, preventing fetching and compiling the autotiler. This means that deploying neural networks through [gap_sdk](https://github.com/GreenWaves-Technologies/gap_sdk) or our Docker image is not possible unless you already have the file. However, you can still deploy neural networks using [DORY](https://github.com/pulp-platform/dory) as an alternative.
>
> For more details, updates and workarounds, see our announcement [here](https://github.com/orgs/bitcraze/discussions/1854).

The GAP8 SDK allows you to compile and execute applications on the GAP8 IoT Application Processor.
Besides tools for building/programming/flashing, it also provides tools to help deploy neural networks and support two different operating systems.

Due to some changes made in the GAP8 SDK it's not easy to use a local installation of the official
SDK yet, so use our dockerized version instead. If you would like to understand the changes we've made,
have a look at the [aideck-docker repository](https://github.com/bitcraze/docker-aideck) and the
[GreenWaves SDK](https://github.com/GreenWaves-Technologies/gap_sdk).

To see a list of what SDK versions we have docker containers for, have a look at [our Docker hub](https://hub.docker.com/r/bitcraze/aideck/tags),
note that we only aim to keep the examples working with the latest tag.

Also, note that to be able to use the autotiler in the GAP8 SDK (Facedetection and Classifcation examples), you will have to set it up and accept the license manually.

Setting up docker and the autotiler

```
$ docker run --rm -it --name myAiDeckContainer bitcraze/aideck
$ cd /gap_sdk
$ source configs/ai_deck.sh
$ make autotiler
```

Follow the instructions of the autotiler pull script. Press enter immediately at  `Enter URL from email: `, fill in your information, wait for the email with the URL. Once you receive the email, fill in the URL at the current `Enter URL from email: `, read the licence and accept if it is all fine with you.

In a second **separate** terminal on your local machine, commit the changes to a new image by running:
```
$ docker commit myAiDeckContainer aideck-with-autotiler
```

This will save the state of the container with the installed autotiler to a new image called `aideck-with-autotiler` that you will use later.

You can now go back to the first terminal and close the container.

```
$ exit
```

Remember that this needs to be done every time you pull a new image of the bitcraze/aideck docker image.




For additional information on the SDK, look at the [GreenWaves GAP SDK documentation](https://greenwaves-technologies.com/manuals/BUILD/HOME/html/index.html).
