# Image classification on the AI-deck

In this example, a binary classification CNN (object or background) is trained and executed on the AI-deck. The models included in the example are trained on data from a very particular domain (a table in the Bitcraze arena, with a particular Christmas package) with limited data augmentation, resulting in poor generalization and low robustness. For good results on your own domain, the example can be trained on a custom dataset captured by the AI-deck camera, and fitted to detect multiple custom classes of your choosing. The training can currently only be done on a native installation. Execution on / flashing the AI-deck can be done with a native installation or with the GAP8 docker.

<img src="classification.gif" width="50%" height="50%"/>

The used CNN is a pre-trained MobileNetV2 (alpha = 0.35, image input size=96x96x3) with a custom classification head and prepended convolutional layers to accept the grayscale/Bayer camera stream of the AI-deck and resize this to MobileNet's expected input shape. The example was tested on an AI-deck v1.1 with a Bayer color camera equipped.

CNN fine-tuning demo based on TensorFlow ["retrain classification" demo](https://github.com/google-coral/tutorials/blob/52b60653698a10e7c83c5761cf6a2acc3db57d22/retrain_classification_ptq_tf2.ipynb).

For more information on good deep learning practices, we recommend reviewing [Deep Learning by Goodfellow, Bengio and Courville](https://www.deeplearningbook.org/), available online for free.

---
## Table of contents
- [File structure](#file-structure)
- [Prerequisites](#prerequisites)
- [Generate a custom dataset](#generate-a-custom-dataset)
- [Fine-tune a pre-trained image classification CNN](#fine-tune-a-pre-trained-image-classification-cnn)
- [Execute the image classification CNN on the AI-deck](#execute-the-image-classification-cnn-on-the-ai-deck)

---
## File structure
`training` contains everything required to (re)train a custom MobileNetV2, and export to a TensorFlow Lite model.

`training/data` should contain all training/validation images, the TensorFlow training pipeline expects a file structure as included, with separate folders for each class in both a train and validation directory.

`gap8_code` contains the project to deploy the generated TensorFlow Lite model on the AI-deck.

`gap8_code/model` contains the generated TensorFlow Lite models, and `nntool` scripts.

`gap8_code/samples` should contains sample images used by `nntool` to quantize the imported models (optional, only required if MODEL_PREQUANTIZED = false in Makefile). These can be taken from the dataset.

---
## Prerequisites
### Native
* gap_sdk >= 4.8.0
* A Python environment with Python requirements installed. We recommend using a virtual environment manager such as Miniconda. Install requirements using pip: `pip install -r requirements.txt`

### Docker
* Docker with gap_sdk >= 4.8.0

---
## Generate a custom dataset

Collect images from the AI-deck using the WiFi streamer. Place them in the data folder, adhering to the folder structure. The captured data must be split into a train and validation set by hand (a good starting point is a 75% train - 25 % validation split). The existing classes can be renamed as desired. For more than two classes, increase the number of units in the final (dense) layer of the model.

---
## Fine-tune a pre-trained image classification CNN
From `GAP8/ai_examples/classification/` run `python main.py [--args]`.

For possible arguments, review the `parse_args()` function in `main.py`.

Automatically generates (overwrites) quantized and non-quantized TensorFlow Lite models.

---
## Execute the image classification CNN on the AI-deck

From a terminal in `gap8_code`, execute:

### Native
```
make clean all run
``` 

### Docker
```
docker run --rm -it -v $PWD:/module/data/ --device /dev/ttyUSB0 --privileged -P bitcraze/aideck /bin/bash -c 'export GAPY_OPENOCD_CABLE=interface/ftdi/olimex-arm-usb-tiny-h.cfg; source /gap_sdk/configs/ai_deck.sh; cd /module/data/;  make clean all run'
```