FROM ubuntu:16.04 AS build-env
MAINTAINER https://www.openarkcompiler.cn

# Setting up the build environment
RUN sed -i 's/archive.ubuntu.com/mirrors.163.com/g' /etc/apt/sources.list && \
    dpkg --add-architecture i386 && \
    apt-get -y update && \
    apt-get -y dist-upgrade && \
    apt-get -y install openjdk-8-jdk git-core build-essential zlib1g-dev libc6-dev-i386 g++-multilib gcc-multilib linux-libc-dev:i386 && \
    apt-get -y install gcc-5-aarch64-linux-gnu g++-5-aarch64-linux-gnu unzip tar curl && \
    apt-get -y install python3-paramiko python-paramiko python-requests && \
    mkdir -p /tools/ninja /tools/gn

# 在国内请反注释下行, 因为容器也是个单独的系统，所以别用127.0.0.1
#ENV http_proxy=http://192.168.3.81:1081 \ 
#    https_proxy=http://192.168.3.81:1081

RUN cd /tools && \
    curl -C - -LO http://releases.llvm.org/8.0.0/clang+llvm-8.0.0-x86_64-linux-gnu-ubuntu-16.04.tar.xz && \
    curl -LO https://github.com/ninja-build/ninja/releases/download/v1.9.0/ninja-linux.zip && \
    curl -LO http://tools.harmonyos.com/mirrors/gn/1523/linux/gn.1523.tar && \
    tar Jvxf /tools/clang+llvm-8.0.0-x86_64-linux-gnu-ubuntu-16.04.tar.xz -C /tools/ && \
    unzip /tools/ninja-linux.zip -d /tools/ninja/ && \
    tar xvf /tools/gn.1523.tar && \
    chmod a+x /tools/gn/gn && \
    rm /tools/clang+llvm-8.0.0-x86_64-linux-gnu-ubuntu-16.04.tar.xz /tools/ninja-linux.zip && \
    rm -rf /var/cache/apt/archives

# copy source
COPY . /OpenArkCompiler
WORKDIR /OpenArkCompiler

# create symbolic link
RUN mkdir -p /OpenArkCompiler/tools /OpenArkCompiler/tools/gn && \
    ln -s /tools/ninja /OpenArkCompiler/tools/ninja_1.9.0 && \
    ln -s /tools/gn/gn /OpenArkCompiler/tools/gn/gn && \
    ln -s /tools/clang+llvm-8.0.0-x86_64-linux-gnu-ubuntu-16.04 /OpenArkCompiler/tools/clang+llvm-8.0.0-x86_64-linux-gnu-ubuntu-16.04

# compile
RUN ["/bin/bash", "-c", "source build/envsetup.sh && make && ls -al "]

# build final docker image
FROM ubuntu:16.04
RUN sed -i 's/archive.ubuntu.com/mirrors.163.com/g' /etc/apt/sources.list && \
    apt-get -y update && \
    apt-get install -y openjdk-8-jdk curl vim && \
    rm -rf /var/cache/apt/archives
COPY --from=build-env /OpenArkCompiler/output /OpenArkCompiler
VOLUME /OpenArkCompiler
ENV PATH=/OpenArkCompiler/bin:$PATH
CMD maple -h
