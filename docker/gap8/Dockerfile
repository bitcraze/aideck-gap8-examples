FROM ubuntu:18.04

ENV DEBIAN_FRONTEND noninteractive 

RUN apt-get update && apt-get install -y build-essential git libftdi-dev libftdi1 doxygen python3-pip libsdl2-dev curl cmake libusb-1.0-0-dev scons gtkwave libsndfile1-dev rsync autoconf automake texinfo libtool pkg-config libsdl2-ttf-dev libopencv-dev

RUN  curl -s https://packagecloud.io/install/repositories/github/git-lfs/script.deb.sh | bash && apt-get install git-lfs && git lfs install


RUN git clone https://github.com/GreenWaves-Technologies/gap_riscv_toolchain_ubuntu_18.git

RUN cd gap_riscv_toolchain_ubuntu_18 && ./install.sh "/usr/lib/gap_riscv_toolchain" && cd ..

RUN git clone https://github.com/GreenWaves-Technologies/gap_sdk.git

RUN cd gap_sdk && git checkout SDK_3.6.1_I2S && git submodule update --init --recursive

RUN pip3 install -U pip && cd gap_sdk && pip3 install -r requirements.txt && pip3 install -r tools/nntool/requirements.txt


RUN /bin/bash -c "cd gap_sdk/; source configs/ai_deck.sh; ls; make all;"

RUN /bin/bash -c "cd gap_sdk/; source configs/ai_deck.sh; ls; make gap_tools"

RUN apt install wget

RUN /bin/bash -c "cd gap_sdk/; source configs/ai_deck.sh; ls; make nntool"

RUN pip3 install requests

RUN /bin/bash -c "cd gap_sdk/; source configs/ai_deck.sh; ls; make openocd"
RUN /bin/bash -c "cd gap_sdk/; source configs/ai_deck.sh; ls; make pulp-os"
RUN mkdir data

WORKDIR /module
