# Sample MNIST Build using GAPFlow

This project includes a sample based on a simple model graph defined in Keras.

It goes from training right through to working code on GAP8 or the same code running on the PC for
debugging purposes.

* It first trains the network using keras
* It then exports the network to H5 format
* It then converts the H5 file to a TFLITE file using TensorFlow's TFLITE converter
* It then generates an nntool state file by running an nntool script with commands to adjust the tensor and activation order, fuse certain operations together and automatically quantify the graph
* It then uses this state file to generate an AutoTiler graph model
* It then compiles and runs the model to produce GAP8 code
* Finally it builds and runs the produced code on GAP8

While this example uses Makefiles the same steps could be achieved with any build system.

The process can be run to quantize the model in 16 or 8 bits weights and activations.

## Build and run without Docker
*This example uses the SDK Version `3.7`.*

To build and run on GAP8:
```
make all run
```
To build and run on GVSOC
```
make all run platform=gvsoc
```
The input image is specified in the Makefile and loaded with the functions defined in ${GAP_SDK_HOME}/libs/gap_lib/img_io/ImgIO.c

To clean the generated model and code but not the trained network type
```
make clean
```
To clean the trained keras save file type
```
make clean_train
```
To build and run the network compiled on the pc
```
make -f emul.mk all
```
This will produce an executable, model_emul, that can be used to evaluate files
```
e.g. ./model_emul images/5558_6.pgm 
```
This mode allows the application to be run with PC tools like valgrind which is very interesting for debugging.
The cluster only has one core in this mode.

The build defaults to 8 bit quantization. 16 bit quantization can be selected by preceeding the build lines above with QUANT_BITS=16.
```
e.g. QUANT_BITS=16 make -f emul.mk all
```

## Build and run within the docker
*This example uses the SDK Version `3.7`.*

Navigate to top-level folder of the example
```
cd <path/to/ai-deck/repository>/GAP8/ai_examples/gapflow_model_extern/
```
Build and execute in Docker similar as the other examples
```
docker run --rm -it -v $PWD:/module/data/ --device /dev/ttyUSB0 --privileged -P gapsdk:3.7 /bin/bash -c 'export GAPY_OPENOCD_CABLE=interface/ftdi/olimex-arm-usb-tiny-h.cfg; source /gap_sdk/configs/ai_deck.sh; cd /module/data/;  make clean clean_train clean_model clean_images all run'
```
