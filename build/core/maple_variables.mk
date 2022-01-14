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
LIB_CORE_PATH := $(MAPLE_BUILD_OUTPUT)/libjava-core/host-x86_64-$(OPT)
LIB_CORE_JAR := $(LIB_CORE_PATH)/java-core.jar
LIB_CORE_MPLT := $(LIB_CORE_PATH)/java-core.mplt

GCC_LINARO_PATH := $(MAPLE_ROOT)/tools/gcc-linaro-7.5.0

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
APP_QEMU_SO := $(foreach APP, $(TARGETS), $(APP).VtableImpl.qemu.so)
APP_VTABLEIMPL_MPL := $(foreach APP, $(TARGETS), $(APP).VtableImpl.mpl)

MAPLE_OUT := $(MAPLE_BUILD_OUTPUT)
JAVA2JAR := $(MAPLE_OUT)/bin/java2jar
JBC2MPL_BIN := $(MAPLE_OUT)/bin/jbc2mpl
MAPLE_BIN := $(MAPLE_OUT)/bin/maple
MPLCG_BIN := $(MAPLE_OUT)/bin/mplcg
JAVA2D8 := $(MAPLE_OUT)/bin/java2d8
HIR2MPL_BIN := $(MAPLE_OUT)/bin/hir2mpl
JAVA2DEX := ${MAPLE_ROOT}/build/java2dex

D8 := $(MAPLE_ROOT)/build/d8
ADD_OBJS := $(MAPLE_ROOT)/src/mrt/maplert/src/mrt_module_init.c__
INIT_CXX_SRC := $(LIB_CORE_PATH)/mrt_module_init.cpp
INIT_CXX_O := $(LIB_CORE_PATH)/mrt_module_init.o

LDS := $(MAPLE_ROOT)/src/mrt/maplert/linker/maplelld.so.lds
DUPLICATE_DIR := $(MAPLE_ROOT)/src/mrt/codetricks/arch/arm64

QEMU_CLANG_CPP := $(TOOL_BIN_PATH)/clang++

QEMU_CLANG_FLAGS := -Wall -W -Werror -Wno-unused-command-line-argument -Wl,-z,now -fPIC -fstack-protector-strong \
    -fvisibility=hidden -std=c++14 -march=armv8-a

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

ifeq ($(OPT),O2)
    HIR2MPL_FLAGS := --rc
    MPLME_FLAGS := --O2 --quiet
    MPL2MPL_FLAGS := --O2 --quiet --regnativefunc --no-nativeopt --maplelinker
    MPLCG_FLAGS := --O2 --quiet --no-pie --verbose-asm --gen-c-macro-def --maplelinker --duplicate_asm_list=$(DUPLICATE_DIR)/duplicateFunc.s
    MPLCG_SO_FLAGS := --fpic
else ifeq ($(OPT),O0)
    HIR2MPL_FLAGS := --rc
    MPLME_FLAGS := --quiet
    MPL2MPL_FLAGS := --quiet --regnativefunc --maplelinker
    MPLCG_FLAGS := --quiet --no-pie --verbose-asm --gen-c-macro-def --maplelinker --duplicate_asm_list=$(DUPLICATE_DIR)/duplicateFunc.s
    MPLCG_SO_FLAGS := --fpic
else ifeq ($(OPT),GC_O2)
    HIR2MPL_FLAGS :=
    MPLME_FLAGS := --O2 --quiet --gconly
    MPL2MPL_FLAGS := --O2 --quiet --regnativefunc --no-nativeopt --maplelinker
    MPLCG_FLAGS := --O2 --quiet --no-pie --verbose-asm --gen-c-macro-def --maplelinker --duplicate_asm_list=$(DUPLICATE_DIR)/duplicateFunc.s --gconly
    MPLCG_SO_FLAGS := --fpic
else ifeq ($(OPT),GC_O0)
    HIR2MPL_FLAGS :=
    MPLME_FLAGS := --quiet --gconly
    MPL2MPL_FLAGS := --quiet --regnativefunc --maplelinker
    MPLCG_FLAGS := --quiet --no-pie --verbose-asm --gen-c-macro-def --maplelinker --duplicate_asm_list=$(DUPLICATE_DIR)/duplicateFunc.s --gconly
    MPLCG_SO_FLAGS := --fpic
endif
HIR2MPL_APP_FLAGS := -mplt=${LIB_CORE_PATH}/libcore-all.mplt
MPLCOMBO_FLAGS := --run=me:mpl2mpl:mplcg --option="$(MPLME_FLAGS):$(MPL2MPL_FLAGS):$(MPLCG_FLAGS) $(MPLCG_SO_FLAGS)"
JAVA2DEX_FLAGS := -p ${OUT_ROOT}/${MAPLE_BUILD_TYPE}/ops/third_party/JAVA_LIBRARIES/core-oj_intermediates/classes.jar:${OUT_ROOT}/${MAPLE_BUILD_TYPE}/ops/third_party/JAVA_LIBRARIES/core-libart_intermediates/classes.jar
