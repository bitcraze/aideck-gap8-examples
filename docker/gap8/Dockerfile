FROM ubuntu:18.04


ENV DEBIAN_FRONTEND noninteractive 
ARG GAP_SDK_VERSION
RUN echo $GAP_SDK_VERSION

# Install needed packages
RUN apt-get update && apt-get install -y build-essential git libftdi-dev libftdi1 doxygen python3-pip libsdl2-dev curl cmake libusb-1.0-0-dev scons gtkwave libsndfile1-dev rsync autoconf automake texinfo libtool pkg-config libsdl2-ttf-dev 
RUN apt-get install -y wget

# Install gap8 openocd
RUN git clone https://github.com/GreenWaves-Technologies/gap8_openocd.git; cd gap8_openocd; ./bootstrap; ./configure --program-prefix=gap8- --prefix=/usr --datarootdir=/usr/share/gap8-openocd; make -j; make -j install

# Install toolchain
RUN git clone https://github.com/GreenWaves-Technologies/gap_riscv_toolchain_ubuntu_18.git; cd gap_riscv_toolchain_ubuntu_18 && ./install.sh "/usr/lib/gap_riscv_toolchain" && cd ..

# Install Gap SDK
RUN git clone https://github.com/GreenWaves-Technologies/gap_sdk.git; cd gap_sdk && git fetch --all --tags && git checkout tags/release-v${GAP_SDK_VERSION} && git submodule update --init --recursive

RUN pip3 install -U pip && cd gap_sdk && pip3 install -r requirements.txt; 

# These are needed since python2 is used by standard in 18.04
RUN apt-get install -y python-pip
RUN python -m pip install configparser
#RUN pip3 install requests

#RUN /bin/bash -c "echo 'alias python = python3' >> ~/.bashrc "


RUN /bin/bash -c "cd gap_sdk/; source configs/ai_deck.sh; ls; make all;"

RUN cd gap_sdk; pip3 install -r tools/nntool/requirements.txt

RUN /bin/bash -c "cd gap_sdk/; source configs/ai_deck.sh; ls; make nntool"


RUN apt-get install -y libopencv-dev python3-opencv

RUN /bin/bash -c "cd gap_sdk/; source configs/ai_deck.sh; ls; make gap_tools"

RUN mkdir -p /module/data/
WORKDIR /module/data/
