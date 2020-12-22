#!/bin/bash
#
# Copyright (C) [2020] Futurewei Technologies, Inc. All rights reverved.
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

TOOLS=$(pwd)
export MAPLE_ROOT=$TOOLS/..
echo MAPLE_ROOT: $MAPLE_ROOT

ANDROID_VERSION=android-10.0.0_r35
ANDROID_SRCDIR=$MAPLE_ROOT/../android/$ANDROID_VERSION

ANDROID_DIR=$MAPLE_ROOT/android

if [ ! -f $TOOLS/ninja/ninja ]; then
  cd $TOOLS
  mkdir -p ./ninja
  cd ./ninja || exit 3
  wget https://github.com/ninja-build/ninja/releases/download/v1.10.0/ninja-linux.zip
  unzip ninja-linux.zip
  echo Downloaded ninja.
fi

if [ ! -f $TOOLS/gn/gn ]; then
  cd $TOOLS
  git clone https://gitee.com/xlnb/gn_binary.git gn
  chmod +x gn/gn
  echo Downloaded gn.
fi

if [ ! -f $TOOLS/clang+llvm-8.0.0-x86_64-linux-gnu-ubuntu-16.04/bin/clang ]; then
  cd $TOOLS
  wget https://releases.llvm.org/8.0.0/clang+llvm-8.0.0-x86_64-linux-gnu-ubuntu-16.04.tar.xz
  echo unpacking clang+llvm ...
  tar xf clang+llvm-8.0.0-x86_64-linux-gnu-ubuntu-16.04.tar.xz
  echo Downloaded clang+llvm.
fi

if [ ! -f $TOOLS/gcc-linaro-7.5.0/bin/aarch64-linux-gnu-gcc ]; then
  cd $TOOLS
  wget https://releases.linaro.org/components/toolchain/binaries/latest-7/aarch64-linux-gnu/gcc-linaro-7.5.0-2019.12-i686_aarch64-linux-gnu.tar.xz
  echo unpacking gcc ...
  tar xf gcc-linaro-7.5.0-2019.12-i686_aarch64-linux-gnu.tar.xz
  mv gcc-linaro-7.5.0-2019.12-i686_aarch64-linux-gnu gcc-linaro-7.5.0
  echo Downloaded gcc aarch64 compiler.
fi

if [ ! -f $TOOLS/android-ndk-r21/ndk-build ]; then
  cd $TOOLS
  wget https://dl.google.com/android/repository/android-ndk-r21d-linux-x86_64.zip
  echo unpacking android ndk ...
  unzip android-ndk-r21d-linux-x86_64.zip > /dev/null
  mv android-ndk-r21d android-ndk-r21
  echo Downloaded android ndk.
fi

if [ ! -f $MAPLE_ROOT/third_party/d8/lib/d8.jar ]; then
  cd $TOOLS
  git clone https://gitee.com/xlnb/r8-d81513.git
  mkdir -p $MAPLE_ROOT/third_party/d8/lib
  cp -f r8-d81513/d8/lib/d8.jar $MAPLE_ROOT/third_party/d8/lib
  echo Downloaded AOSP.
fi

# download and build andriod source
if [ ! -d $ANDROID_DIR/out/target/product/generic_arm64/obj ]; then
  mkdir -p $ANDROID_SRCDIR
  cd $ANDROID_SRCDIR
  repo init -u https://android.googlesource.com/platform/manifest -b $ANDROID_VERSION
  repo sync
  . ./build/envsetup.sh
  lunch 2
  make -j32
  make core-all
  ln -s $ANDROID_SRCDIR $ANDROID_DIR
  echo Downloaded and built AOSP.
fi

# create java-core.jar
if [ ! -f $MAPLE_ROOT/libjava-core/java-core.jar ]; then
  mkdir -p $MAPLE_ROOT/libjava-core
  JLIBDIR=$ANDROID_DIR/out/target/product/generic_arm64/obj/JAVA_LIBRARIES
  cp -f $JLIBDIR/core-all_intermediates/javalib.jar $MAPLE_ROOT/libjava-core/java-core.jar
  cd $MAPLE_ROOT/libjava-core
  $MAPLE_ROOT/build/d8 --min-api 39 --output . java-core.jar
  mv classes.dex libcore-all.dex
  echo Copied jar files.
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

if [ ! -f $MAPLE_ROOT/third_party/icu/lib/aarch64-linux-gnu/libicuuc.so ]; then
  cd $TOOLS
  wget https://github.com/unicode-org/icu/archive/release-56-1.tar.gz
  tar zxf release-56-1.tar.gz
  DIR=$TOOLS/icu-release-56-1/icu4c
  export CROSS_BUILD_DIR=$DIR/linux
  export ANDROID_NDK=$MAPLE_ROOT/tools/android-ndk-r21
  export ANDROID_TOOLCHAIN=$DIR/toolchain
  export PATH=$ANDROID_TOOLCHAIN/bin:$PATH
  mkdir -p $DIR/linux $DIR/aarch64
  (cd $DIR/linux; ../source/runConfigureICU Linux --prefix=$DIR/prebuilts; make -j16)
  (cd $DIR; $ANDROID_NDK/build/tools/make-standalone-toolchain.sh --platform=android-21 \
     --install-dir=$ANDROID_TOOLCHAIN --toolchain=aarch64-linux-android-clang3.6)
  (cd $DIR/aarch64; ../source/configure --prefix=$DIR/prebuilt --host=aarch64-linux-android \
     CFLAGS='-Os' CXXFLAGS='--std=c++11' --enable-shared=yes -with-cross-build=$CROSS_BUILD_DIR \
     CC=aarch64-linux-android-clang CXX=aarch64-linux-android-clang++ AR=aarch64-linux-android-ar \
     RINLIB=aarch64-linux-android-ranlib --with-data-packaging=archive; make -j16)
  AARCH64DIR=$MAPLE_ROOT/third_party/icu/lib/aarch64-linux-gnu
  mkdir -p $AARCH64DIR
  cd $AARCH64DIR
  cp -f $DIR/aarch64/lib/libicuuc.so libicuuc.so.56
  cp -f $DIR/aarch64/stubdata/libicudata.so libicudata.so.56
  ln -s libicuuc.so.56 libicuuc.so
  ln -s libicudata.so.56 libicudata.so
  echo Downloaded icu.
fi

if [ ! -f $MAPLE_ROOT/third_party/libdex/prebuilts/aarch64-linux-gnu/libz.so.1.2.8 ]; then
  cd $TOOLS
  wget http://ports.ubuntu.com/pool/main/z/zlib/zlib1g_1.2.8.dfsg-2ubuntu4_arm64.deb
  mkdir -p libz_extract
  dpkg --extract zlib1g_1.2.8.dfsg-2ubuntu4_arm64.deb libz_extract
  ZLIBDIR=$MAPLE_ROOT/third_party/libdex/prebuilts/aarch64-linux-gnu
  mkdir -p $ZLIBDIR
  cp -f libz_extract/lib/aarch64-linux-gnu/libz.so.1.2.8 $ZLIBDIR
  echo Downloaded libz.
fi

