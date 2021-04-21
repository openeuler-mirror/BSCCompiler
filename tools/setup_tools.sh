#!/bin/bash
#
# Copyright (C) [2020-2021] Futurewei Technologies, Inc. All rights reverved.
#
# Licensed under the Mulan Permissive Software License v2.
# You can use this software according to the terms and conditions of the MulanPSL - 2.0.
# You may obtain a copy of MulanPSL - 2.0 at:
#
#   https://opensource.org/licenses/MulanPSL-2.0
#
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR
# FIT FOR A PARTICULAR PURPOSE.
# See the MulanPSL - 2.0 for more details.
#

set -e

if [ -z "$MAPLE_ROOT" ]; then
  echo "Please \"source build/envsetup.sh\" to setup environment"
  exit 1
fi
echo MAPLE_ROOT: $MAPLE_ROOT

android_env=$1

TOOLS=$MAPLE_ROOT/tools

ANDROID_VERSION=android-10.0.0_r35
ANDROID_SRCDIR=$MAPLE_ROOT/../android/$ANDROID_VERSION

ANDROID_DIR=$MAPLE_ROOT/android

if [ "$OLD_OS" == "1" ]; then
  if [ ! -f $TOOLS/clang+llvm-10.0.0-x86_64-linux-gnu-ubuntu-18.04/bin/clang ]; then
    cd $TOOLS
    echo Start wget llvm-10.0.0 ...
    wget https://github.com/llvm/llvm-project/releases/download/llvmorg-10.0.0/clang+llvm-10.0.0-x86_64-linux-gnu-ubuntu-18.04.tar.xz
    echo unpacking clang+llvm ...
    tar xf clang+llvm-10.0.0-x86_64-linux-gnu-ubuntu-18.04.tar.xz
    echo Downloaded clang+llvm.
  fi
fi

if [ "$android_env" == "android" ]; then
  if [ ! -f $TOOLS/android-ndk-r21/ndk-build ]; then
    cd $TOOLS
    wget https://dl.google.com/android/repository/android-ndk-r21d-linux-x86_64.zip
    echo unpacking android ndk ...
    unzip android-ndk-r21d-linux-x86_64.zip > /dev/null
    mv android-ndk-r21d android-ndk-r21
    echo Downloaded android ndk.
  fi

  if [ ! -L $TOOLS/gcc ]; then
    cd $TOOLS
    ln -s ../android/prebuilts/gcc .
    echo Linked gcc.
  fi

  if [ ! -L $TOOLS/clang-r353983c ]; then
    cd $TOOLS
    ln -s ../android/prebuilts/clang/host/linux-x86/clang-r353983c .
    echo Linked clang.
  fi
fi

if [ ! -f $TOOLS/ninja/ninja ]; then
  cd $TOOLS
  echo Start wget ninja ...
  mkdir -p ./ninja
  cd ./ninja || exit 3
  wget https://github.com/ninja-build/ninja/releases/download/v1.10.0/ninja-linux.zip
  unzip ninja-linux.zip
  echo Downloaded ninja.
fi

if [ ! -f $TOOLS/gn/gn ]; then
  cd $TOOLS
  echo Start clone gn ...
  git clone https://gitee.com/xlnb/gn_binary.git gn
  chmod +x gn/gn
  echo Downloaded gn.
fi

if [ ! -f $TOOLS/gcc-linaro-7.5.0/bin/aarch64-linux-gnu-gcc ]; then
  cd $TOOLS
  echo Start wget gcc-linaro-7.5.0 ...
  wget https://releases.linaro.org/components/toolchain/binaries/latest-7/aarch64-linux-gnu/gcc-linaro-7.5.0-2019.12-i686_aarch64-linux-gnu.tar.xz
  echo unpacking gcc ...
  tar xf gcc-linaro-7.5.0-2019.12-i686_aarch64-linux-gnu.tar.xz
  mv gcc-linaro-7.5.0-2019.12-i686_aarch64-linux-gnu gcc-linaro-7.5.0
  echo Downloaded gcc aarch64 compiler.
fi

if [ ! -f $MAPLE_ROOT/third_party/d8/lib/d8.jar ]; then
  cd $TOOLS
  echo Start clone d8 ...
  git clone https://gitee.com/xlnb/r8-d81513.git
  mkdir -p $MAPLE_ROOT/third_party/d8/lib
  cp -f r8-d81513/d8/lib/d8.jar $MAPLE_ROOT/third_party/d8/lib
  echo Downloaded d8.jar.
fi

if [ ! -d $MAPLE_ROOT/third_party/icu ]; then
  cd $TOOLS
  echo Start clone ICU4C ...
  git clone https://gitee.com/xlnb/icu4c.git
  mkdir -p $MAPLE_ROOT/third_party/icu
  cp -r icu4c/lib/ $MAPLE_ROOT/third_party/icu/
  echo Downloaded icu4c libs
fi

# download prebuilt andriod
if [ ! -d $ANDROID_DIR/out/target/product/generic_arm64 ]; then
  cd $TOOLS
  echo Start clone AOSP CORE LIB ...
  git clone https://gitee.com/xlnb/aosp_core_bin.git
  cp -r aosp_core_bin/android $MAPLE_ROOT/
  cp -r aosp_core_bin/libjava-core $MAPLE_ROOT/
  echo Downloaded AOSP CORE LIB
fi

if [ ! -f $MAPLE_ROOT/third_party/libdex/prebuilts/aarch64-linux-gnu/libz.so.1.2.8 ]; then
  cd $TOOLS
  echo Start wget libz ...
  wget http://ports.ubuntu.com/pool/main/z/zlib/zlib1g_1.2.8.dfsg-2ubuntu4_arm64.deb
  mkdir -p libz_extract
  dpkg --extract zlib1g_1.2.8.dfsg-2ubuntu4_arm64.deb libz_extract
  ZLIBDIR=$MAPLE_ROOT/third_party/libdex/prebuilts/aarch64-linux-gnu
  mkdir -p $ZLIBDIR
  cp -f libz_extract/lib/aarch64-linux-gnu/libz.so.1.2.8 $ZLIBDIR
  echo Downloaded libz.
fi

# install qemu-user 2.5.0
if [ ! -f $TOOLS/qemu/package/usr/bin/qemu-aarch64 ]; then
  cd $TOOLS
  echo Start wget qemu-user ...
  rm -rf qemu
  git clone https://gitee.com/hu-_-wen/qemu.git
  cd qemu
  mkdir -p package
  dpkg-deb -R qemu-user_2.5+dfsg-5ubuntu10.48_amd64.deb package
  echo Installed qemu-aarch64
fi

if [ ! -f $TOOLS/open64_prebuilt/README.md ]; then
  cd $TOOLS
  git clone https://gitee.com/open64ark/open64_prebuilt.git
fi
if [ ! -f $TOOLS/open64_prebuilt/x86/riscv64/bin/clangfe ]; then
  cd $TOOLS/open64_prebuilt/x86
  git pull
  tar zxf open64ark-aarch64.tar.gz
  tar zxf open64ark-riscv.tar.gz
  mv riscv riscv64
  echo Downloaded open64_prebuilt.
fi

if [ ! -f $MAPLE_ROOT/third_party/dwarf_h/include/Dwarf.h ]; then
  cd $TOOLS
  rm -rf dwarf $MAPLE_ROOT/third_party/dwarf*
  git clone https://gitee.com/hu-_-wen/dwarf_h.git
  mv dwarf_h $MAPLE_ROOT/third_party/
  echo Downloaded dwarf header files.
fi

mkdir -p ${TOOL_BIN_PATH}
if [ "$OLD_OS" == "1" ]; then
  ln -s -f ${MAPLE_ROOT}/tools/clang+llvm-10.0.0-x86_64-linux-gnu-ubuntu-18.04/bin/clang++ ${TOOL_BIN_PATH}/clang++
  ln -s -f ${MAPLE_ROOT}/tools/clang+llvm-10.0.0-x86_64-linux-gnu-ubuntu-18.04/bin/clang ${TOOL_BIN_PATH}/clang
  ln -s -f ${MAPLE_ROOT}/tools/clang+llvm-10.0.0-x86_64-linux-gnu-ubuntu-18.04/bin/llvm-ar ${TOOL_BIN_PATH}/llvm-ar
  ln -s -f ${MAPLE_ROOT}/tools/clang+llvm-10.0.0-x86_64-linux-gnu-ubuntu-18.04/bin/llvm-ranlib ${TOOL_BIN_PATH}/llvm-ranlib
  ln -s -f /usr/bin/qemu-aarch64 ${TOOL_BIN_PATH}/qemu-aarch64
else
  ln -s -f /usr/bin/clang++ ${TOOL_BIN_PATH}/clang++
  ln -s -f /usr/bin/clang ${TOOL_BIN_PATH}/clang
  ln -s -f /usr/bin/llvm-ar ${TOOL_BIN_PATH}/llvm-ar
  ln -s -f /usr/bin/llvm-ranlib ${TOOL_BIN_PATH}/llvm-ranlib
  ln -s -f ${MAPLE_ROOT}/tools/qemu/package/usr/bin/qemu-aarch64 ${TOOL_BIN_PATH}/qemu-aarch64
fi
ln -s -f ${MAPLE_ROOT}/tools/open64_prebuilt/x86/aarch64/bin/clangfe ${TOOL_BIN_PATH}/clangfe

if [ ! -d $MAPLE_ROOT/../ThirdParty ]; then
  cd $MAPLE_ROOT/../
  git clone https://gitee.com/openarkcompiler/ThirdParty.git
  cd -
else
  cd $MAPLE_ROOT/../ThirdParty
  git pull origin master
  cd -
fi
