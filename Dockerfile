FROM ubuntu:18.04 AS build-env
MAINTAINER https://www.openarkcompiler.cn

# Setting up the build environment
RUN apt-get -y update
RUN apt-get -y install clang llvm lld libelf-dev libssl-dev python \
    qemu openjdk-8-jre-headless openjdk-8-jdk-headless git build-essential \
    zlib1g-dev libc6-dev-i386 gcc-7-aarch64-linux-gnu g++-7-aarch64-linux-gnu \
    lsb-core wget zip curl

# copy source
COPY . /OpenArkCompiler
WORKDIR /OpenArkCompiler

# use the latest cmake
WORKDIR /OpenArkCompiler/tools
# change the version as you need
RUN wget https://github.com/Kitware/CMake/releases/download/v3.24.3/cmake-3.24.3-linux-x86_64.tar.gz && \
    tar -xf cmake-3.24.3-linux-x86_64.tar.gz && ln -s cmake-3.24.3-linux-x86_64 cmake
ENV PATH="/OpenArkCompiler/tools/cmake/bin:/$PATH"

# build env
# this will take a long time.
WORKDIR /OpenArkCompiler
RUN ["/bin/bash", "-c", "source build/envsetup.sh arm release && make setup"]

# compile
ENV PATH="/OpenArkCompiler/tools/cmake/bin:/OpenArkCompiler/tools/ninja:/$PATH"
RUN ["/bin/bash", "-c", "source build/envsetup.sh arm release && make setup && make"]

