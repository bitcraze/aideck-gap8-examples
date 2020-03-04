# Sample MNIST Build using GAPFlow

This example is from the GAP8 SDK and is adjusted so that it run can use functionalities of the AI-deck and Crazyflie. The original example of the GAP8 SDK can be found here https://github.com/GreenWaves-Technologies/gap_sdk/tree/master/examples/nntool/mnist.

The GAP*flow* is expressed as Makefiles in the 'common' folder. It uses the NNTool and Autotiler tool consecutively as explained in the main folder.

This project includes a sample based on a simple MNIST graph defined in Keras. It goes from training right through to working code on GAP8 or the same code running on the PC for debugging purposes.

<!-- In model_decl.mk the accuracy of a neural network can be increased by setting the epochs of the training.   -->

* It first trains the network using keras
* It then exports the network to H5 format
* It then converts the H5 file to a TFLITE file using TensorFlow's TFLITE converter
* It then generates an nntool state file by running an nntool script with commands to adjust the tensor and activation order, fuse certain operations together and automatically quantify the graph
* It then uses this state file to generate an AutoTiler graph model
* It then compiles and runs the model to produce GAP8 code
* Finally it builds and runs the produced code on GAP8

### Makefile use
While this example uses Makefiles the same steps could be achieved with any build system.

The process can be run to quantize the model in 8 or 16 bits weights and activations.

To build and run on GAP8:

make all run

To build and run on GVSOC

make all run platform=gvsoc

The image loaded is included in a header file. This can be modified in the Makefile. There are also make options to load the file via the bridge. This mode is not supported for GVSOC.

To clean the generated model and code but not the trained network type

make clean

To clean the trained keras save file type

make clean_train

To build and run the network compiled on the pc

make -f emul.mk all

This will produce an executable, mnist_emul, that can be used to evaluate files

e.g. ./mnist_emul images/5558_6.pgm 

This mode allows the application to be run with PC tools like valgrind which is very interesting for debugging.
The cluster only has one core in this mode.

The build defaults to 8 bit quantization. 16 bit quantization can be selected by preceeding the build lines above with MODEL_BITS=16.

e.g. MODEL_BITS=16 make -f emul.mk all
