---
title: Getting started
page_id: getting-started
---

## About
### AI-deck
The AI-deck enables low power on-board artificial intelligence capabilities for the Crazyflie using a GAP8 chip with RISC-V multi-core architecture, 512 Mbit HyperFlash and 64 Mbit HyperRAM. In addition the deck has a Himax HM01B0 grayscale camera with a NiNA W102 Wi-Fi module to stream your images to a desktop. These features fit the prerequites of a convolutional neural network, but the AI-deck is not limited to the application of CNN's. 

### For users starting with embedded applications and the Crazyflie
For this example a basic understanding on how to:
* Program in Python and C
* Use Linux systems
* Use Makefiles 
* Some basic knowledge about embedded programming

### Required hardware
* An JTAG debugger (we used a Olimex ARM-USB-TINY-H JTAG)
* ARM JTAG 20 to 10 pin adapter 
* CrazyFlie 2.1
* AI-Deck

## Setting up the GAP8 SDK
To get started, first set up the GAP8 SDK using the instruction on this repository:

https://github.com/GreenWaves-Technologies/gap_sdk.  


## Workflow using GAPFlow
To design a neural network and deploy it on the AI-deck, you should know the workflow of the GAP8 SDK for AI applications that is provided by GreenWaves Technologies. A neural network can be designed, trained, and evaluated using Tensorflow and Keras in Python. To let this code be able to run on the AI deck, an automated process is executed by the GAPFlow of the GAP8 SDK. 

<!-- ![GAPFlow](/illustrations/GAPFlow.png) --> [insert image about GAPFlow]
Courtesy of GreenWaves Technologies: https://greenwaves-technologies.com/

<!-- explaining about the model graph process and models -->

What you should provide in this workflow is:
* dataset with labels
* neural network model
* GAP application code
* optional: own autotiler operator

### Tensorflow and Keras use
Keras is a framework within Tensorflow. In addtion to providing a neural network framework, it also provides examples of common and simple neural networks and provides datasets along with it. Though when you want to make an application you might want to use other datasets that are relevant for your application. For this you have to supply your own dataset with labels, augmentation and transformations when required.

https://keras.io/datasets/

### NNTool use
<!-- In this example the NNTool state file can be found in example/model/nntool_script special attention to the following command/rule 

```aquant -f 8 <image folder>/*.<image extension> -T```

The NNTool makes use of post-training quantization and adjust the quantized weights using the images defined in the aforementioned rule in the state file. -->

The NNTool makes use of post-training quantization and adjust the quantized weights using a selection of images used for training the neural network.

<!-- explain a bit more in detail about quantization -->

https://github.com/GreenWaves-Technologies/gap_sdk/tree/master/tools/nntool


### Autotiler use
The Autotiler supports basic operators needed for a convolutional neural network. If you need a specialized operator for your neural network, then you can make your own autotiler operator by using this page.

https://greenwaves-technologies.com/manuals/BUILD/AUTOTILER/html/index.html

<!-- explain a bit more about how the autotiler works -->


