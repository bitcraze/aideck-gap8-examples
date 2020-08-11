FROM ubuntu:18.04

ENV DEBIAN_FRONTEND noninteractive 

RUN apt-get update && apt-get install -y gcc git wget make libncurses-dev flex bison gperf python python-pip python-setuptools python-serial python-cryptography python-future python-pyparsing curl

RUN curl -O https://dl.espressif.com/dl/xtensa-esp32-elf-linux64-1.22.0-80-g6c4433a-5.2.0.tar.gz

RUN ls && tar -xzf xtensa-esp32-elf-linux64-1.22.0-80-g6c4433a-5.2.0.tar.gz

ENV PATH="/xtensa-esp32-elf/bin:$PATH"

RUN git clone -b v3.3.1 --recursive https://github.com/espressif/esp-idf.git

ENV IDF_PATH '/esp-idf'

RUN pip install -r $IDF_PATH/requirements.txt

RUN pip install virtualenv==16.7.10

RUN cd esp-idf && ./install.sh

RUN /bin/bash -c "cd esp-idf && . ./export.sh"

RUN curl -OL https://github.com/espressif/openocd-esp32/releases/download/v0.10.0-esp32-20190313/openocd-esp32-linux64-0.10.0-esp32-20190313.tar.gz

RUN tar -xzf openocd-esp32-linux64-0.10.0-esp32-20190313.tar.gz

RUN apt-get -y install make libtool pkg-config autoconf automake texinfo libusb-1.0

WORKDIR /module
