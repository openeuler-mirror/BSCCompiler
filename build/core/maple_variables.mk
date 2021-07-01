#
# Copyright (c) [2020] Huawei Technologies Co.,Ltd.All rights reserved.
#
# OpenArkCompiler is licensed under Mulan PSL v2.
# You can use this software according to the terms and conditions of the Mulan PSL v2.
# You may obtain a copy of Mulan PSL v2 at:
#
#     http://license.coscl.org.cn/MulanPSL2
#
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR
# FIT FOR A PARTICULAR PURPOSE.
# See the Mulan PSL v2 for more details.
#
OPT := O2
DEBUG := 0
LIB_CORE_PATH := $(MAPLE_BUILD_OUTPUT)/libjava-core
LIB_CORE_JAR := $(LIB_CORE_PATH)/java-core.jar
LIB_CORE_MPLT := $(LIB_CORE_PATH)/java-core.mplt

ifndef OPS_ANDROID
  OPS_ANDROID := 0
endif

ANDROID_GCC_PATH := $(MAPLE_ROOT)/tools/gcc
ANDROID_CLANG_PATH := $(MAPLE_ROOT)/tools/clang-r353983c
GCC_LINARO_PATH := $(MAPLE_ROOT)/tools/gcc-linaro-7.5.0
NDK_PATH := $(MAPLE_ROOT)/tools/android-ndk-r21

TARGETS := $(APP)
APP_JAVA := $(foreach APP, $(TARGETS), $(APP).java)
APP_DEX := $(foreach APP, $(TARGETS), $(APP).dex)
APP_CLASS := $(foreach APP, $(TARGETS), $(APP).class)
APP_JAR := $(foreach APP, $(TARGETS), $(APP).jar)
APP_MPL := $(foreach APP, $(TARGETS), $(APP).mpl)
APP_MPLT:=$(foreach APP, $(TARGETS), $(APP).mplt)
APP_S := $(foreach APP, $(TARGETS), $(APP).VtableImpl.s)
APP_DEF := $(foreach APP, $(TARGETS), $(APP).VtableImpl.macros.def)
APP_O := $(foreach APP, $(TARGETS), $(APP).VtableImpl.o)
APP_SO := $(foreach APP, $(TARGETS), $(APP).so)
APP_VTABLEIMPL_MPL := $(foreach APP, $(TARGETS), $(APP).VtableImpl.mpl)

MAPLE_OUT := $(MAPLE_BUILD_OUTPUT)
JAVA2JAR := $(MAPLE_OUT)/bin/java2jar
JBC2MPL_BIN := $(MAPLE_OUT)/bin/jbc2mpl
MAPLE_BIN := $(MAPLE_OUT)/bin/maple
MPLCG_BIN := $(MAPLE_OUT)/bin/mplcg
JAVA2D8 := $(MAPLE_OUT)/bin/java2d8
DEX2MPL_BIN := $(MAPLE_OUT)/bin/dex2mpl
JAVA2DEX := ${MAPLE_ROOT}/build/java2dex

D8 := $(MAPLE_ROOT)/build/d8
ADD_OBJS := $(MAPLE_ROOT)/src/mrt/maplert/src/mrt_module_init.c__
INIT_CXX_SRC := $(LIB_CORE_PATH)/mrt_module_init.cpp
INIT_CXX_O := $(LIB_CORE_PATH)/mrt_module_init.o

LDS := $(MAPLE_ROOT)/src/mrt/maplert/linker/maplelld.so.lds
DUPLICATE_DIR := $(MAPLE_ROOT)/src/mrt/codetricks/arch/arm64

ifeq ($(OPS_ANDROID), 0)
    QEMU_CLANG_CPP := $(TOOL_BIN_PATH)/clang++
else
    QEMU_CLANG_CPP := $(ANDROID_CLANG_PATH)/bin/clang++
endif

QEMU_CLANG_FLAGS := -Wall -W -Werror -Wno-unused-command-line-argument -Wl,-z,now -fPIC -fstack-protector-strong \
    -fvisibility=hidden -std=c++14 -march=armv8-a

ifeq ($(OPS_ANDROID), 0)
    QEMU_CLANG_FLAGS += -nostdlibinc \
      --gcc-toolchain=$(GCC_LINARO_PATH) \
      --sysroot=$(GCC_LINARO_PATH)/aarch64-linux-gnu/libc \
      -isystem $(GCC_LINARO_PATH)/aarch64-linux-gnu/include/c++/7.5.0 \
      -isystem $(GCC_LINARO_PATH)/aarch64-linux-gnu/include/c++/7.5.0/aarch64-linux-gnu \
      -isystem $(GCC_LINARO_PATH)/aarch64-linux-gnu/include/c++/7.5.0/backward \
      -isystem $(GCC_LINARO_PATH)/lib/gcc/aarch64-linux-gnu/7.5.0/include \
      -isystem $(GCC_LINARO_PATH)/lib/gcc/aarch64-linux-gnu/7.5.0/include-fixed \
      -isystem $(GCC_LINARO_PATH)/aarch64-linux-gnu/include \
      -isystem $(GCC_LINARO_PATH)/aarch64-linux-gnu/libc/usr/include \
      -target aarch64-linux-gnu
else
    QEMU_CLANG_FLAGS := -Wall -fstack-protector-strong -fPIC -Werror -Wno-unused-command-line-argument \
      -fvisibility=hidden -std=c++14 -nostdlib -march=armv8-a -target aarch64-linux-android \
      -isystem $(MAPLE_ROOT)/android/bionic/libc \
      -isystem $(MAPLE_ROOT)/android/bionic/libc/include \
      -isystem $(MAPLE_ROOT)/android/bionic/libc/kernel/uapi \
      -isystem $(MAPLE_ROOT)/android/bionic/libc/kernel/uapi/asm-arm64 \
      -isystem $(MAPLE_ROOT)/android/bionic/libc/kernel/android/scsi \
      -isystem $(MAPLE_ROOT)/android/bionic/libc/kernel/android/uapi \
      -isystem $(ANDROID_CLANG_PATH)/lib64/clang/9.0.3/include
endif

ifeq ($(OPT),O2)
    DEX2MPL_FLAGS := -mplt=${OUT_ROOT}/${MAPLE_BUILD_TYPE}/libjava-core/libcore-all.mplt -litprofile=${MAPLE_ROOT}/src/mrt/codetricks/profile.pv/meta.list
    MPLME_FLAGS := --O2 --quiet
    MPL2MPL_FLAGS := --O2 --quiet --regnativefunc --no-nativeopt --maplelinker
    MPLCG_FLAGS := --O2 --quiet --no-pie --verbose-asm --gen-c-macro-def --maplelinker --duplicate_asm_list=$(DUPLICATE_DIR)/duplicateFunc.s
    MPLCG_SO_FLAGS := --fpic
else ifeq ($(OPT),O0)
    DEX2MPL_FLAGS := -mplt=${OUT_ROOT}/${MAPLE_BUILD_TYPE}/libjava-core/libcore-all.mplt -litprofile=${MAPLE_ROOT}/src/mrt/codetricks/profile.pv/meta.list
    MPLME_FLAGS := --quiet
    MPL2MPL_FLAGS := --quiet --regnativefunc --maplelinker
    MPLCG_FLAGS := --quiet --no-pie --verbose-asm --gen-c-macro-def --maplelinker --duplicate_asm_list=$(DUPLICATE_DIR)/duplicateFunc.s
    MPLCG_SO_FLAGS := --fpic
endif
MPLCOMBO_FLAGS := --run=me:mpl2mpl:mplcg --option="$(MPLME_FLAGS):$(MPL2MPL_FLAGS):$(MPLCG_FLAGS) $(MPLCG_SO_FLAGS)"
JAVA2DEX_FLAGS := -p ${OUT_ROOT}/${MAPLE_BUILD_TYPE}/ops/third_party/JAVA_LIBRARIES/core-oj_intermediates/classes.jar:${OUT_ROOT}/${MAPLE_BUILD_TYPE}/ops/third_party/JAVA_LIBRARIES/core-libart_intermediates/classes.jar
