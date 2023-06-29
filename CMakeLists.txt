# This file is the master GN build configuration, all variables
# declare here will be implicitly global.
#
# Copyright (c) [2022] Huawei Technologies Co.,Ltd.All rights reserved.
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

# List all the input args

# project
cmake_minimum_required(VERSION 3.10)
project(maple)
set(CMAKE_STATIC_LIBRARY_PREFIX "")
set(CMAKE_SHARED_LIBRARY_PREFIX "")
set(EXECUTABLE_OUTPUT_PATH ${MAPLE_BUILD_OUTPUT}/bin)

set(COV_CHECK "1")
# env args
set(MAPLE_ROOT $ENV{MAPLE_ROOT})
set(ANDROID_ROOT ${MAPLE_ROOT}/android)
set(AOSP_ROOT ${MAPLE_ROOT}/third_party/aosp_modified)
set(MAPLE_BUILD_TYPE $ENV{MAPLE_BUILD_TYPE})
set(MAPLE_BUILD_OUTPUT $ENV{MAPLE_BUILD_OUTPUT})
set(TOOL_BIN_PATH $ENV{TOOL_BIN_PATH})
set(QEMU_PATH $ENV{TOOL_BIN_PATH})
set(OLD_OS $ENV{OLD_OS})
set(IS_AST2MPL_EXISTS $ENV{IS_AST2MPL_EXISTS})


# global args
set(DYNAMICLANG true)
set(RC_V2 true)
set(TEST_BENCHMARK false)
set(MEMORY_LEAK_CHECK false)
set(MARK_CYCLE_ROOTS false)
if(${OLD_OS} STREQUAL "1")
    set(DESIGNATOR "-Wno-gnu-designator")
else()
    set(DESIGNATOR "-Wno-c99-designator")
endif()
set(OPENSOURCE_DEPS ${MAPLE_ROOT}/src/mapleall/deplibs)
set(OPENSOURCE_OUTPUT ${MAPLE_BUILD_OUTPUT})
set(AST2MPL_ROOT ${MAPLE_ROOT}/src/ast2mpl)
set(MAPLEALL_ROOT ${MAPLE_ROOT}/src/mapleall)
set(HIR2MPL_ROOT ${MAPLE_ROOT}/src/hir2mpl)
set(MAPLE_MRT_ROOT ${MAPLE_ROOT}/src/mrt)
set(THIRD_PARTY_ROOT ${MAPLE_ROOT}/third_party)
set(LIB_HW_SECURE_C ${MAPLE_ROOT}/third_party/bounds_checking_function)


# dir
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${MAPLE_BUILD_OUTPUT}/ar/host-x86_64-${OPT})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${MAPLE_BUILD_OUTPUT}/lib)
set(PROJECT_BINARY_DIR ${MAPLE_BUILD_OUTPUT}/lib)


#--------------------------------- toolchain -------------------------------------
set(CLANG_PATH $ENV{ENHANCED_CLANG_PATH})
set(LLVMINC ${CLANG_PATH}/include)
set(LLVMLIBDIR ${CLANG_PATH}/lib)
set(CFE_SRC_DIR ${MAPLE_ROOT}/../third_party/clang-10.0.0.src)
set(CFE_SRC_PATH ${MAPLE_ROOT}/third_party/llvm-12.0.0.src/clang)
set(MAPLE_PARSER_PATH ${MAPLE_ROOT}/src/MapleFE)
set(GCC_LINARO_PATH ${MAPLE_ROOT}/tools/gcc-linaro-7.5.0)
set(NDK_PATH ${MAPLE_ROOT}/tools/android-ndk-r21)
set(target_toolchain "//build/toolchain:clang")

set(CMAKE_C_COMPILER ${TOOL_BIN_PATH}/clang)
set(CMAKE_CXX_COMPILER ${TOOL_BIN_PATH}/clang++)
set(CMAKE_AR ${TOOL_BIN_PATH}/llvm-ar)
set(CMAKE_RANLIB ${TOOL_BIN_PATH}/llvm-ranlib)

set(CMAKE_C_CROSS_COMPILER ${GCC_LINARO_PATH}/bin/aarch64-linux-gnu-gcc)
set(CMAKE_CXX_CROSS_COMPILER ${GCC_LINARO_PATH}/bin/aarch64-linux-gnu-g++)
set(CMAKE_AR_CROSS_COMPILER ${GCC_LINARO_PATH}/bin/aarch64-linux-gnu-ar)
set(CMAKE_RANLIB_CROSS_COMPILER ${GCC_LINARO_PATH}/bin/aarch64-linux-gnu-ranlib)

# toolchain:clang
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fuse-ld=lld ")
if(${CMAKE_BUILD_TYPE} STREQUAL "RELEASE" AND ${GPROF} STREQUAL 0)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -s")
endif()

# -----------------------CMAKE FLAGS---------------------------------
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -fno-common")
if(${CMAKE_BUILD_TYPE} STREQUAL "RELEASE")
    set(CMAKE_CXX_FLAGS
        "${CMAKE_CXX_FLAGS} -O2 -fno-strict-aliasing -D_FORTIFY_SOURCE=2")
    set(CMAKE_C_FLAGS
        "${CMAKE_C_FLAGS} -O2 -fno-strict-aliasing -D_FORTIFY_SOURCE=2")
elseif(${CMAKE_BUILD_TYPE} STREQUAL "DEBUG")
    add_definitions("-DDEBUG")
    set(CMAKE_CXX_FLAGS
        "${CMAKE_CXX_FLAGS} -O0 -g3 -ftrapv -fstack-check")
    set(CMAKE_C_FLAGS
        "${CMAKE_C_FLAGS} -O0 -g3 -ftrapv -fstack-check")
    if(${target_toolchain} STREQUAL "//build/toolchain:clang")
        set(CMAKE_CXX_FLAGS
            "${CMAKE_CXX_FLAGS} -fno-limit-debug-info")
        set(CMAKE_C_FLAGS
            "${CMAKE_C_FLAGS} -fno-limit-debug-info")
    endif()
else()
    set(CMAKE_CXX_FLAGS
        "${CMAKE_CXX_FLAGS} -O2 -fno-strict-aliasing -g")
    set(CMAKE_C_FLAGS
        "${CMAKE_C_FLAGS} -O2 -fno-strict-aliasing -g")
endif()

set(CMAKE_C_FLAGS
    "${CMAKE_C_FLAGS} -Wall -fstack-protector-strong -fPIC -fPIE -pipe -Werror -Wdate-time -Wfloat-equal ${DESIGNATOR}")
set(CMAKE_CXX_FLAGS
    "${CMAKE_CXX_FLAGS} -Wall -fstack-protector-strong -fPIC -fPIE -pipe -Wno-c99-designator -Wno-range-loop-construct -Werror -Wdate-time -Wfloat-equal ${DESIGNATOR} -DMIR_FEATURE_FULL=1")

if(HOST_ARCH STREQUAL 64)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m64")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m64")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fPIC -rdynamic -lpthread -ldl -Wl,-z,relro -Wl,-z,now -Wl,-z,noexecstack -pie -Wl,--start-group ")
else()
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m32")
endif()

if(DYNAMICLANG STREQUAL true)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DDYNAMICLANG")
endif()

if(RC_V2 STREQUAL true)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DRC_NO_MMAP")
endif()

if(TEST_BENCHMARK STREQUAL true)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DTEST_BENCHMARK")
endif()

if(MEMORY_LEAK_CHECK STREQUAL true)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DMEMORY_LEAK_CHECK")
endif()

if(MARK_CYCLE_ROOTS STREQUAL true)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DMARK_CYCLE_ROOTS")
endif()

if(MIR_JAVA STREQUAL 1)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DMIR_JAVA=1")
else()
    set(TARGET vm)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DMIR_JAVA=0")
endif()

if(${ASAN} STREQUAL 1)
    add_definitions(" -fsanitize=address ")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -ldl ${LLVMLIBDIR}/libclang_rt.asan-x86_64.a")
endif()

if(${COV} STREQUAL 1)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --coverage")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fprofile-arcs -ftest-coverage")
endif()

if(${GPROF} STREQUAL 1)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pg")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pg")
endif()

if(${TARGET} STREQUAL aarch64)
    set(CMAKE_CXX_FLAGS
         "${CMAKE_CXX_FLAGS} -DTARGAARCH64 -DMAPLE_ROOT=\"${MAPLE_ROOT}\"")
endif()

if(${TARGET} STREQUAL aarch64_ilp32)
    set(CMAKE_CXX_FLAGS
        "${CMAKE_CXX_FLAGS} -DTARGAARCH64 -DILP32 -DMAPLE_ROOT=\"${MAPLE_ROOT}\"")
endif()

if(${TARGET} STREQUAL x86_64)
    set(CMAKE_CXX_FLAGS
      "${CMAKE_CXX_FLAGS} -DTARGX86_64 -DMAPLE_ROOT=\"${MAPLE_ROOT}\"")
endif()

if(${TARGET} STREQUAL riscv64)
    set(CMAKE_CXX_FLAGS
        "${CMAKE_CXX_FLAGS} -DTARGRISCV64 -DMAPLE_ROOT=\"${MAPLE_ROOT}\"")
endif()

if(${TARGET} STREQUAL ark)
    set(CMAKE_CXX_FLAGS
      "${CMAKE_CXX_FLAGS} -DTARGARK -DMAPLE_ROOT=\"${MAPLE_ROOT}\"")
endif()


# -----------------------CMAKE FLAGS END---------------------------------


#subdirectory
add_subdirectory(src/hir2mpl)
add_subdirectory(src/hir2mpl/test)
add_subdirectory(src/mapleall)
add_subdirectory(third_party/bounds_checking_function)
add_subdirectory(src/mapleall/test)
add_subdirectory(src/mrt)
if (NOT ${ONLY_C} STREQUAL "1")
  add_subdirectory(third_party/aosp_modified/art/libdexfile)
  add_subdirectory(third_party/aosp_modified/system/core/libziparchive)
  add_subdirectory(third_party/aosp_modified/system/core/base)
endif()
