---
title: Step By step guide to develop for GAP8
page_id: writing-aideck-gap8-code
---

## Introduction
The AIdeck enables WiFi communication with the Crazyflie
as well as using the power-efficient GAP8 to run neural networks on board. This tutorial will teach you about how to set up your own applications for your AIdeck.

## Folder Structure
In the [Getting started with the AI deck tutorial](https://www.bitcraze.io/documentation/tutorials/getting-started-with-aideck/) tutorial you already cloned the [AIdeck example repository](https://github.com/bitcraze/aideck-gap8-examples). As we use some library files from there it is important that you respect the exact paths (or do all required changes in paths yourself). For convinience the files we tell you to create already exist - so if you don't want to cheat but to follow this tutorial step by step, first delete the ```ai-deck-examples/examples/other/hello_world_gap8``` directory.

We'll use a classic example here: Hello World.
So we call our folder which contains the example ```hello_world_gap8```, and create the c file (called ```hello_world_gap8.c```) and the ```Makefile``` (for now all empty). We also say that this example belongs to the category ```other```, so we place it in ```ai-deck-examples/examples/other```
Now the folder structure looks like this (omitting all other examples):
```
ai-deck-examples   
│
└───examples
    │
    other
        |
        hello_world_gap8
            │   hello_world_gap8.c
            │   Makefile

```
## C code
Now we need to write the c code.
We start by including some dependencies:

```#include "pmsis.h"``` for the drivers

```#include "bsp/bsp.h"``` for some configuration parameters (pad configurations for connecting to memory, camera, etc.)

```#include "cpx.h"``` for using the CPX functions to send our hello world to the console

Then we have to write our main function:

```
int main(void)
{
  return pmsis_kickoff((void *)start_example);
}
```
We call pmsis_kickoff() to start the scheduler and an event kernel, and we give it a pointer to the function we want to execute.
This function is what we write next (insert it above the main function, such that it is found in the code of the main).

```
void start_example(void)
{
  pi_bsp_init();
  cpxInit();

  while (1)
  {
      cpxPrintToConsole(LOG_TO_CRTP, "Hello World\n");
      pi_time_wait_us(1000*1000);
  }

}
```
First we need to initialize the pads according to our configuration (the configuration is automatically chosen with sourcing the ai_deck.sh, which is automatically done in the docker) with pi_bsp_init().  
Then we need to initialize CPX (which initializes for example the SPI connection to the NINA WiFi), to be able to send CPX packets. You find more information in [the CPX documentation](https://www.bitcraze.io/documentation/repository/crazyflie-firmware/master/functional-areas/cpx/).  
Now we are ready for our while loop, in which we want to send "Hello World" to the console (called LOG_TO_CRTP). To not keep the busses overly busy we only want to send it every second, so we wait before we repeat.  

## Makefile
The Makefile is hierarchical - meaning we have hidden files which do most of the work and which we need to include with ```include $(RULES_DIR)/pmsis_rules.mk``` in the last line of the Makefile.

We start with defining where the io should go - possible are host or uart (this is actually not used in this example, but if you'd add a printf this would define where it goes).  
Then we define the operating system we want to use - we can use pulpos or freertos. As freertos is way more advanced (we are paying for this with some overhead, but i most cases it will be worth it) we chose this.
```
io=uart
PMSIS_OS = freertos
```
In the next step we need to set the name of our application (this defines the file names of the build output), include the sources (meaning our main c file as well as the two c files CPX needs) and include the header files directory (header files in the same directory as the Makefile should automatically found, but our CPX header files are in a library directory). Make sure all the relative paths are correct for your folder structure.

```
APP = hello_world_gap8
APP_SRCS += hello_world_gap8.c ../../../lib/cpx/src/com.c ../../../lib/cpx/src/cpx.c
APP_INC=../../../lib/cpx/inc
```
As a last step we want to set some compiler flags. Firstly, we want to compile optimized, so we add -O3. Then we add -g to embedd debug information.  
As we use timers for CPX we also need to add two additional defines to make sure all functions we need are included, those are the configUSE_TIMERS=1 and the INCLUDE_xTimerPendFunctionCall=1 defines.

```
APP_CFLAGS += -O3 -g
APP_CFLAGS += -DconfigUSE_TIMERS=1 -DINCLUDE_xTimerPendFunctionCall=1
```

## Compile and Flash
For this section we assume you are in the ```aideck-gap8-examples``` directory. 

Now we want to compile using docker:

```
docker run --rm -v ${PWD}:/module --privileged bitcraze/aideck tools/build/make-example SRC_DIR clean all
```
Where SRC_DIR is  the location of your code, here it is ```examples/other/hello_world_gap8```.

**Note:** You don't always need to "clean", if you don't modify the Makefile it should be fine to save some time by skipping this.

And finally we want to flash using the OTA updater:
```
cfloader flash [binary] deck-bcAI:gap8-fw -w CRAZYFLIE_URI
```
Where in this example [binary] has to be replaced with ```examples/other/hello_world_gap8/BUILD/GAP8_V2/GCC_RISCV_FREERTOS/target.board.devices.flash.img``` and the CRAZYFLIE_URI is something like ```radio://0/80/2M/E7E7E7E7E7```.

Now you can connect to your drone with the cfclient and should see a ```CPX: GAP8: Hello World``` print every second. 

**Note:** The LED will not blink as in most other examples, as we did not implement a task which does this.

## Further reading

* Check out [the CPX documentation](https://www.bitcraze.io/documentation/repository/crazyflie-firmware/master/functional-areas/cpx/) for more explanation of how to communicate with the AIdeck
* [The GAP8 repository examples](https://www.bitcraze.io/documentation/repository/aideck-gap8-examples/master/) to read about what examples we provide and to try them out
* [Greenwaves GAP github repository](https://github.com/GreenWaves-Technologies/gap_sdk) for the gap8 sdk and various examples.