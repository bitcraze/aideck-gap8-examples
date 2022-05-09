# Copyright 2021 Bitcraze AB
# Copyright 2020 Google LLC
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# https://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
"""
Demo for fine-tuning MobileNetv2 on a custom dataset, and generating an
(un)quantized TensorFlow lite model for inference on the AI-deck. 

Based on TensorFlow "retrain classification" demo.
https://github.com/google-coral/tutorials/blob/52b60653698a10e7c83c5761cf6a2acc3db57d22/retrain_classification_ptq_tf2.ipynb
"""

import argparse
import os

import numpy as np
import tensorflow as tf
import PIL.Image
import scipy

def parse_args():
    args = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )

    args.add_argument("--epochs", dest="epochs", type=int, default=20)
    args.add_argument(
        "--finetune_epochs", dest="finetune_epochs", type=int, default=20
    )
    args.add_argument(
        "--dataset_path",
        metavar="dataset_path",
        help="path to dataset",
        default="training_data",
    )
    args.add_argument("--batch_size", dest="batch_size", type=int, default=8)
    args.add_argument(
        "--image_width", dest="image_width", type=int, default=324
    )
    args.add_argument(
        "--image_height", dest="image_height", type=int, default=244
    )
    args.add_argument(
        "--image_channels", dest="image_channels", type=int, default=1
    )

    return args.parse_args()


if __name__ == "__main__":
    args = parse_args()
    ROOT_PATH = (
        f"{os.path.abspath(os.curdir)}/GAP8/ai_examples/classification/"
    )
    DATASET_PATH = f"{ROOT_PATH}{args.dataset_path}"
    if not os.path.exists(DATASET_PATH):
        ROOT_PATH = "./"
        DATASET_PATH = args.dataset_path
    if not os.path.exists(DATASET_PATH):
        raise ValueError(f"Dataset path '{DATASET_PATH}' does not exist.")
    print(DATASET_PATH + "/*/*/*")

    train_datagen = tf.keras.preprocessing.image.ImageDataGenerator(
        rotation_range=10,
        shear_range=0.2,
        zoom_range=0.1,
        horizontal_flip=True,
        brightness_range=[0.5, 1.5],
    )
    train_generator = train_datagen.flow_from_directory(
        f"{DATASET_PATH}/train",
        target_size=(args.image_width, args.image_height),
        batch_size=args.batch_size,
        class_mode="categorical",
        color_mode="grayscale",
    )

    val_datagen = tf.keras.preprocessing.image.ImageDataGenerator()
    val_generator = val_datagen.flow_from_directory(
        f"{DATASET_PATH}/validation",
        target_size=(args.image_width, args.image_height),
        batch_size=args.batch_size,
        class_mode="categorical",
        color_mode="grayscale",
    )

    FIRST_LAYER_STRIDE = 2

    # Create the base model from the pre-trained MobileNet V2
    base_model = tf.keras.applications.MobileNetV2(
        input_shape=(
            96,
            96,
            3,
        ),
        include_top=False,
        weights="imagenet",
        alpha=0.35,
    )
    base_model.trainable = False

    # Add a custom head, which will predict the classes
    model = tf.keras.Sequential(
        [
            tf.keras.Input(shape=(args.image_width, args.image_height, 1)),
            tf.keras.layers.SeparableConvolution2D(
                filters=3,
                kernel_size=1,
                # activation="relu",
                activation=None,
                strides=FIRST_LAYER_STRIDE,
            ),
            tf.keras.layers.experimental.preprocessing.Resizing(
                96, 96, interpolation="bilinear"
            ),
            base_model,
            tf.keras.layers.SeparableConvolution2D(
                filters=32, kernel_size=3, activation="relu"
            ),
            tf.keras.layers.Dropout(0.2),
            tf.keras.layers.GlobalAveragePooling2D(),
            tf.keras.layers.Dense(units=2, activation="softmax"),
        ]
    )

    model.compile(
        optimizer=tf.keras.optimizers.Adam(1e-5),
        loss="categorical_crossentropy",
        metrics=["accuracy"],
    )

    model.summary()

    print(
        "Number of trainable weights = {}".format(len(model.trainable_weights))
    )

    # Train the custom head
    history = model.fit(
        train_generator,
        steps_per_epoch=len(train_generator),
        epochs=args.epochs,
        validation_data=val_generator,
        validation_steps=len(val_generator),
    )

    # Fine-tune the model
    print("Number of layers in the base model: ", len(base_model.layers))

    base_model.trainable = True
    fine_tune_at = 100

    # Freeze all the layers before the `fine_tune_at` layer
    for layer in base_model.layers[:fine_tune_at]:
        layer.trainable = False

    model.compile(
        optimizer=tf.keras.optimizers.Adam(5e-5),
        loss="categorical_crossentropy",
        metrics=["accuracy"],
    )

    model.summary()

    print(
        "Number of trainable weights = {}".format(len(model.trainable_weights))
    )

    history_fine = model.fit(
        train_generator,
        steps_per_epoch=len(train_generator),
        epochs=args.finetune_epochs,
        validation_data=val_generator,
        validation_steps=len(val_generator),
    )

    # Convert to TensorFlow lite
    converter = tf.lite.TFLiteConverter.from_keras_model(model)
    tflite_model = converter.convert()

    with open(f"{ROOT_PATH}/model/classification.tflite", "wb") as f:
        f.write(tflite_model)
    # Convert to quantized TensorFlow Lite
    def representative_data_gen():
        dataset_list = tf.data.Dataset.list_files(DATASET_PATH + "/*/*/*")
        for i in range(100):
            image = next(iter(dataset_list))
            image = tf.io.read_file(image)
            image = tf.io.decode_jpeg(image, channels=1)
            image = tf.image.resize(
                image, [args.image_width, args.image_height]
            )
            image = tf.cast(image, tf.float32)
            image = tf.expand_dims(image, 0)
            yield [image]

    converter = tf.lite.TFLiteConverter.from_keras_model(model)
    converter.optimizations = [tf.lite.Optimize.DEFAULT]
    converter.representative_dataset = representative_data_gen
    converter.target_spec.supported_ops = [tf.lite.OpsSet.TFLITE_BUILTINS_INT8]
    converter.target_spec.supported_types = [tf.int8]
    converter.inference_input_type = tf.uint8
    converter.inference_output_type = tf.uint8
    tflite_model = converter.convert()

    with open(
        f"{ROOT_PATH}/model/classification_q.tflite", "wb"
    ) as f:
        f.write(tflite_model)

    batch_images, batch_labels = next(val_generator)

    logits = model(batch_images)
    prediction = np.argmax(logits, axis=1)
    truth = np.argmax(batch_labels, axis=1)

    keras_accuracy = tf.keras.metrics.Accuracy()
    keras_accuracy(prediction, truth)

    print("Raw model accuracy: {:.3%}".format(keras_accuracy.result()))

    def set_input_tensor(interpreter, input):
        input_details = interpreter.get_input_details()[0]
        tensor_index = input_details["index"]
        input_tensor = interpreter.tensor(tensor_index)()[0]
        input_tensor[:, :] = input

    def classify_image(interpreter, input):
        set_input_tensor(interpreter, input)
        interpreter.invoke()
        output_details = interpreter.get_output_details()[0]
        output = interpreter.get_tensor(output_details["index"])
        # Outputs from the TFLite model are uint8, so we dequantize the results:
        scale, zero_point = output_details["quantization"]
        output = scale * (output - zero_point)
        top_1 = np.argmax(output)
        return top_1

    interpreter = tf.lite.Interpreter(
        f"{ROOT_PATH}/model/classification_q.tflite"
    )
    interpreter.allocate_tensors()

    # Collect all inference predictions in a list
    batch_prediction = []
    batch_truth = np.argmax(batch_labels, axis=1)

    for i in range(len(batch_images)):
        prediction = classify_image(interpreter, batch_images[i])
        batch_prediction.append(prediction)

    # Compare all predictions to the ground truth
    tflite_accuracy = tf.keras.metrics.Accuracy()
    tflite_accuracy(batch_prediction, batch_truth)
    print("Quant TF Lite accuracy: {:.3%}".format(tflite_accuracy.result()))
