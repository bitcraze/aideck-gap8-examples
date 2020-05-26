---
title: MNIST Example
page_id: mnist-example
---

The example shows a simple convolutional neural network based on the MNIST example of the GAP8 SDK. MNIST is a CNN that classifies handwritten digits ranging from 0 to 9. Added to this is a simple application for the Crazyflie where it takes the output of the MNIST network and uses that to switch states. If the neural network identifies a 1, the GAP8 code will send a UART byte to the Crazyflie which in turn gives a command to unlock. If the neural network identifies a 9 it will unlock. If the Crazyflie is unlocked then it will turn right or left when the neural network identifies a 4 or 5, respectively.

This example can be easily modified to a different classification task by using a similar and simple dataset. 

In addition, to get quickly started on your own neural network you can make use of the principles of Transfer Learning.