cmake_minimum_required(VERSION 3.13)
project(mplpgo_c C)

set(CMAKE_C_STANDARD 99)

set(MAPLE_ROOT $ENV{MAPLE_ROOT})
set(SYS_NAME $ENV{BiShengC_GET_OS_VERSION})
set(GCC_LINARO_PATH ${MAPLE_ROOT}/tools/gcc-linaro-7.5.0)
set(CMAKE_C_COMPILER ${GCC_LINARO_PATH}/bin/aarch64-linux-gnu-gcc)

add_library(mplpgo SHARED mplpgo.c mplpgo.h common_util.h)
add_library(mplpgo_static STATIC mplpgo.c mplpgo.h common_util.h)
 
SET_TARGET_PROPERTIES(mplpgo_static PROPERTIES OUTPUT_NAME mplpgo)
 
SET_TARGET_PROPERTIES(mplpgo_static PROPERTIES CLEAN_DIRECT_OUTPUT 1)
SET_TARGET_PROPERTIES(mplpgo PROPERTIES CLEAN_DIRECT_OUTPUT 1)

# set output directory
SET(LIBRARY_OUTPUT_PATH ${MAPLE_ROOT}/libpgo/lib_${SYS_NAME}) 
