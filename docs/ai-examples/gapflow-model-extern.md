---
title: GAPflow Model Extern
page_id: gapflow-model-extern
---

# MNIST example simplified

The *GAP8/ai_examples/mnist_simple/* is a copy of Greenwaves' [MNIST example of GAPflow](https://github.com/GreenWaves-Technologies/gap_sdk/tree/master/examples/nntool/mnist). This example was tested and working in SDK `3.7`.

The GAP*flow* is expressed as Makefiles in the 'common' folder. It uses the NNTool and Autotiler tool consecutively as explained in the main folder.

This project includes a sample based on a simple MNIST graph defined in Keras. It goes from training right through to working code on GAP8 or the same code running on the PC for debugging purposes.

In train_model.mk the accuracy of a neural network can be increased by setting the epochs of the training.  

* It first trains the network using keras
* It then exports the network to H5 format
* It then converts the H5 file to a TFLITE file using TensorFlow's TFLITE converter
* It then generates an nntool state file by running an nntool script with commands to adjust the tensor and activation order, fuse certain operations together and automatically quantify the graph
* It then uses this state file to generate an AutoTiler graph model
* It then compiles and runs the model to produce GAP8 code
* Finally it builds and runs the produced code on GAP8
