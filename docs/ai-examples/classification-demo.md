---
title: Classification Demo
page_id: classification-demo
---

# Image classification on the AI-deck

In this example, a binary classification CNN (object or background) is trained and executed on the AI-deck. The models included in the example are trained on data from a very particular domain (a table in the Bitcraze arena, with a particular Christmas package) with limited data augmentation, resulting in poor generalization and low robustness. For good results on your own domain, the example can be trained on a custom dataset captured by the AI-deck camera, and fitted to detect multiple custom classes of your choosing. The training can currently only be done on a native installation. Execution on / flashing the AI-deck can be done with a native installation or with the GAP8 docker.

<img src="/docs/images/classification.gif" width="50%" height="50%" alt="packet classifcation"/>

The used CNN is a pre-trained MobileNetV2 (alpha = 0.35, image input size=96x96x3) with a custom classification head and prepended convolutional layers to accept the grayscale/Bayer camera stream of the AI-deck and resize this to MobileNet's expected input shape. The example was tested on an AI-deck v1.1 with a Bayer color camera equipped.

CNN fine-tuning demo based on TensorFlow ["retrain classification" demo](https://github.com/google-coral/tutorials/blob/52b60653698a10e7c83c5761cf6a2acc3db57d22/retrain_classification_ptq_tf2.ipynb).

For more information on good deep learning practices, we recommend reviewing [Deep Learning by Goodfellow, Bengio and Courville](https://www.deeplearningbook.org/), available online for free.
## File structure
`root(classification)` contains everything required to (re)train a custom MobileNetV2, and export to a TensorFlow Lite model. It also contains the project to deploy the generated TensorFlow Lite model on the AI-deck.

`model/` contains the generated TensorFlow Lite models, and `nntool` scripts.

`training_data/` should contain all training/validation images, the TensorFlow training pipeline expects a file structure as included, with separate folders for each class in both a train and validation directory.

`samples/` should contains sample images used by `nntool` to quantize the imported models (optional, only required if MODEL_PREQUANTIZED = false in Makefile). These can be taken from the dataset.

---
## Prerequisites
### Build Environments

#### Tensor Flow
* A seperate Python environment with Python requirements installed. We recommend using a virtual environment manager such as Miniconda or Penv. Install requirements using pip: `pip install -r requirements.txt`

### GAP8
There are two approaches for this:
* Install gap_sdk natively on your machine (>= 4.8.0)
* Build inside Docker container with gap_sdk >= 4.8.0

#### For docker container

```
docker run --rm -it -v $PWD:/module/data/ bitcraze/aideck:4.8.0 /bin/bash`
```

Do update the numpy version in the container to 1.21.5 (ignore the warning), and then source gap8_sdk aideck source file

```
pip3 install numpy==1.21.5
source /gap_sdk/configs/ai_deck.sh
```

---
## Generate a custom dataset

Collect images from the AI-deck using the WiFi streamer. Place them in the training_data folder, according to the instructions inside. The captured data must be split into a train and validation set by hand (a good starting point is a 75% train - 25 % validation split). The existing classes can be renamed as desired. For more than two classes, increase the number of units in the final (dense) layer of the model.

This is the folder structure you should follow:
Put here the training and validation images like this:

```
/train/class_1/*.jpeg
/train/class_2/*.jpeg
/validation/class_1/*.jpeg
/validation/class_2/*.jpeg
```
---
## Fine-tune a pre-trained image classification CNN
From `GAP8/ai_examples/classification/` run `python train_classifier.py [--args]`.

For possible arguments, review the `parse_args()` function in `main.py`.

Automatically generates quantized and non-quantized TensorFlow Lite models.

---
## Execute the image classification CNN on the AI-deck

From a terminal in `classifier`, execute:

```
make model build image
``` 

Then use the `...flash.img` and `...flash.readfs.img` file to flash the binary on the aideck. 