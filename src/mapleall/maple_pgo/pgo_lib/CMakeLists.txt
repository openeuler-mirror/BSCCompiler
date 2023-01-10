cmake_minimum_required(VERSION 3.13)
project(mplpgo_c C)

set(CMAKE_C_STANDARD 99)

set(MAPLE_ROOT $ENV{MAPLE_ROOT})
set(GCC_LINARO_PATH ${MAPLE_ROOT}/tools/gcc-linaro-7.5.0)
set(CMAKE_C_COMPILER ${GCC_LINARO_PATH}/bin/aarch64-linux-gnu-gcc)

add_library(mplpgo SHARED mplpgo.c mplpgo.h common_util.h)
