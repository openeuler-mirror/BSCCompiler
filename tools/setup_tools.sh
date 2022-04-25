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

#USR_EMAIL=`git config user.email`

if [ "$android_env" == "android" ]; then
  if [ ! -f $TOOLS/android-ndk-r21/ndk-build ]; then
    cd $TOOLS
    wget https://dl.google.com/android/repository/android-ndk-r21d-linux-x86_64.zip --no-check-certificate
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
  wget https://github.com/ninja-build/ninja/releases/download/v1.10.0/ninja-linux.zip --no-check-certificate
  unzip ninja-linux.zip
  echo Downloaded ninja.
fi

if [ ! -f $TOOLS/gn/gn ]; then
  cd $TOOLS
  echo Start clone gn ...
  git clone --depth 1 https://gitee.com/xlnb/gn_binary.git gn
  chmod +x gn/gn
  echo Downloaded gn.
fi

if [ ! -f $TOOLS/gcc-linaro-7.5.0/bin/aarch64-linux-gnu-gcc ]; then
  cd $TOOLS
  echo Start wget gcc-linaro-7.5.0 ...
  wget https://releases.linaro.org/components/toolchain/binaries/latest-7/aarch64-linux-gnu/gcc-linaro-7.5.0-2019.12-i686_aarch64-linux-gnu.tar.xz --no-check-certificate
  echo unpacking gcc ...
  tar xf gcc-linaro-7.5.0-2019.12-i686_aarch64-linux-gnu.tar.xz
  mv gcc-linaro-7.5.0-2019.12-i686_aarch64-linux-gnu gcc-linaro-7.5.0
  echo Downloaded gcc aarch64 compiler.
fi

if [ ! -d $TOOLS/sysroot-glibc-linaro-2.25 ]; then
  cd $TOOLS
  echo Start wget sysroot-glibc-linaro-2.25 ...
  wget https://releases.linaro.org/components/toolchain/binaries/latest-7/aarch64-linux-gnu/sysroot-glibc-linaro-2.25-2019.12-aarch64-linux-gnu.tar.xz --no-check-certificate
  echo unpacking sysroot ...
  tar xf sysroot-glibc-linaro-2.25-2019.12-aarch64-linux-gnu.tar.xz
  mv sysroot-glibc-linaro-2.25-2019.12-aarch64-linux-gnu sysroot-glibc-linaro-2.25
  echo Downloaded aarch64 sysroot.
fi

if [ ! -f $MAPLE_ROOT/third_party/d8/lib/d8.jar ]; then
  cd $TOOLS
  echo Start clone d8 ...
  git clone --depth 1 https://gitee.com/xlnb/r8-d81513.git
  mkdir -p $MAPLE_ROOT/third_party/d8/lib
  cp -f r8-d81513/d8/lib/d8.jar $MAPLE_ROOT/third_party/d8/lib
  echo Downloaded d8.jar.
fi

if [ ! -d $MAPLE_ROOT/third_party/icu ]; then
  cd $TOOLS
  echo Start clone ICU4C ...
  git clone --depth 1 https://gitee.com/xlnb/icu4c.git
  mkdir -p $MAPLE_ROOT/third_party/icu
  cp -r icu4c/lib/ $MAPLE_ROOT/third_party/icu/
  echo Downloaded icu4c libs
fi

# download prebuilt andriod
if [ ! -d $ANDROID_DIR/out/target/product/generic_arm64 ]; then
  cd $TOOLS
  echo Start clone AOSP CORE LIB ...
  git clone --depth 1 https://gitee.com/xlnb/aosp_core_bin.git
  cp -r aosp_core_bin/android $MAPLE_ROOT/
  cp -r aosp_core_bin/libjava-core $MAPLE_ROOT/
  echo Downloaded AOSP CORE LIB
fi

if [ ! -f $MAPLE_ROOT/third_party/libdex/prebuilts/aarch64-linux-gnu/libz.so.1.2.8 ]; then
  cd $TOOLS
  echo Start wget libz ...
  wget http://ports.ubuntu.com/pool/main/z/zlib/zlib1g_1.2.8.dfsg-2ubuntu4_arm64.deb --no-check-certificate
  mkdir -p libz_extract
  dpkg --extract zlib1g_1.2.8.dfsg-2ubuntu4_arm64.deb libz_extract
  ZLIBDIR=$MAPLE_ROOT/third_party/libdex/prebuilts/aarch64-linux-gnu
  mkdir -p $ZLIBDIR
  cp -f libz_extract/lib/aarch64-linux-gnu/libz.so.1.2.8 $ZLIBDIR
  echo Downloaded libz.
fi

# install a stable qemu-user based on different OS

QEMU_VERSION=`$TOOLS/qemu/usr/bin/qemu-aarch64 -version | awk 'NR == 1' | awk '{print $3}' | sed -e "s/^[^0-9]*//" -e "s/\..*//"`

if [ "$QEMU_VERSION" = "3" ] || [ "$QEMU_VERSION" = "4" ]; then
  echo "QEMU version is too new and QEMU will be reversed to a stable version."
  rm -rf $TOOLS/qemu
fi

if [ ! -f $TOOLS/qemu/usr/bin/qemu-aarch64 ]; then
  cd $TOOLS
  echo Start wget qemu-user ...
  rm -rf qemu
  mkdir -p qemu
  if [ "$OLD_OS" == "1" ];then
    wget http://archive.ubuntu.com/ubuntu/pool/universe/q/qemu/qemu-user_2.11+dfsg-1ubuntu7.39_amd64.deb --no-check-certificate
    dpkg-deb -R qemu-user_2.11+dfsg-1ubuntu7.39_amd64.deb qemu
  else
    # we will use QEMU 2.11 for now, and will upgrade it to a new version after further investigations
    wget http://archive.ubuntu.com/ubuntu/pool/universe/q/qemu/qemu-user_2.11+dfsg-1ubuntu7.39_amd64.deb --no-check-certificate
    dpkg-deb -R qemu-user_2.11+dfsg-1ubuntu7.39_amd64.deb qemu
  fi
  echo Installed qemu-aarch64
fi

# clang2mpl
if [ ! -d $TOOLS/clang2mpl ]; then
  cd $TOOLS
  git clone --depth 1 https://gitee.com/openarkcompiler-incubator/clang2mpl.git
fi
# routinly updated to be compatible with maple
cd $TOOLS/clang2mpl
git clean -df
git checkout .
git checkout master
git pull

if [ ! -d $MAPLE_ROOT/../ThirdParty ]; then
  cd $MAPLE_ROOT/../
  git clone --depth 1 https://gitee.com/openarkcompiler/ThirdParty.git
  cd -
else
  cd $MAPLE_ROOT/../ThirdParty
  git pull origin master
  cd -
fi

mkdir -p ${TOOL_BIN_PATH}
ln -s -f ${MAPLE_ROOT}/../ThirdParty/clang+llvm-12.0.0-x86_64-linux-gnu-ubuntu-18.04 ${MAPLE_ROOT}/tools
ln -s -f ${MAPLE_ROOT}/../ThirdParty/llvm-12.0.0.src ${MAPLE_ROOT}/third_party/llvm-12.0.0.src
ln -s -f ${MAPLE_ROOT}/tools/clang+llvm-12.0.0-x86_64-linux-gnu-ubuntu-18.04/bin/clang++ ${TOOL_BIN_PATH}/clang++
ln -s -f ${MAPLE_ROOT}/tools/clang+llvm-12.0.0-x86_64-linux-gnu-ubuntu-18.04/bin/clang ${TOOL_BIN_PATH}/clang
ln -s -f ${MAPLE_ROOT}/tools/clang+llvm-12.0.0-x86_64-linux-gnu-ubuntu-18.04/bin/llvm-ar ${TOOL_BIN_PATH}/llvm-ar
ln -s -f ${MAPLE_ROOT}/tools/clang+llvm-12.0.0-x86_64-linux-gnu-ubuntu-18.04/bin/llvm-ranlib ${TOOL_BIN_PATH}/llvm-ranlib
ln -s -f ${MAPLE_ROOT}/tools/clang+llvm-12.0.0-x86_64-linux-gnu-ubuntu-18.04/bin/FileCheck ${TOOL_BIN_PATH}/FileCheck
ln -s -f ${MAPLE_ROOT}/tools/qemu/usr/bin/qemu-aarch64 ${TOOL_BIN_PATH}/qemu-aarch64
ln -s -f ${MAPLE_ROOT}/build/java2dex ${TOOL_BIN_PATH}/java2dex
ln -s -f ${MAPLE_ROOT}/tools/r8-d81513/d8/lib ${MAPLE_ROOT}/tools/lib
ln -s -f ${MAPLE_ROOT}/tools/gcc-linaro-7.5.0/bin/aarch64-linux-gnu-gcc ${TOOL_BIN_PATH}/aarch64-linux-gnu-gcc
ln -s -f ${MAPLE_ROOT}/tools/gcc-linaro-7.5.0/bin/aarch64-linux-gnu-as ${TOOL_BIN_PATH}/aarch64-linux-gnu-as
ln -s -f ${MAPLE_ROOT}/tools/gcc-linaro-7.5.0/bin/aarch64-linux-gnu-ld ${TOOL_BIN_PATH}/aarch64-linux-gnu-ld

# prepare scripts for tests
mkdir -p ${MAPLE_ROOT}/output/script
cp ${MAPLE_ROOT}/testsuite/driver/script/check.py ${MAPLE_ROOT}/output/script/
cp ${MAPLE_ROOT}/testsuite/driver/script/kernel.py ${MAPLE_ROOT}/output/script/
