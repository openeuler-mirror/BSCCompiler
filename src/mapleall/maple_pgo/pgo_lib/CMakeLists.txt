cmake_minimum_required(VERSION 3.13)
project(mplpgo_c C)

set(CMAKE_C_STANDARD 99)

set(MAPLE_ROOT $ENV{MAPLE_ROOT})
set(SYS_NAME $ENV{BISHENGC_GET_OS_VERSION})
set(GCC_LINARO_PATH ${MAPLE_ROOT}/tools/gcc-linaro-7.5.0)
set(ENV_PGO $ENV{TEST_ENV})
add_compile_options(-fpic -O2 -fstack-protector-strong -D_FORTIFY_SOURCE=2)

if (SYS_NAME STREQUAL "SUSE")
  if (EXISTS /opt/buildtools/bisheng_embedded_cpu-301.0.0/hcc_arm64le/bin/aarch64-target-linux-gnu-gcc)
    if (ENV_PGO STREQUAL "TEST")
        set(CMAKE_C_COMPILER /opt/buildtools/bisheng_embedded_cpu-301.0.0/hcc_arm64le/bin/aarch64-target-linux-gnu-gcc)
    endif()
  else ()
    set(CMAKE_C_COMPILER aarch64-hpe-gcc)
endif()
elseif (SYS_NAME STREQUAL "Ubuntu")
  set(CMAKE_C_COMPILER ${GCC_LINARO_PATH}/bin/aarch64-linux-gnu-gcc)
else ()
  message("error: unsupported target OS!")
endif()

add_library(mplpgo SHARED mplpgo.c mplpgo.h common_util.h)
add_library(mplpgo_static STATIC mplpgo.c mplpgo.h common_util.h)
 
SET_TARGET_PROPERTIES(mplpgo_static PROPERTIES OUTPUT_NAME mplpgo)
 
SET_TARGET_PROPERTIES(mplpgo_static PROPERTIES CLEAN_DIRECT_OUTPUT 1)
SET_TARGET_PROPERTIES(mplpgo PROPERTIES CLEAN_DIRECT_OUTPUT 1)
SET_TARGET_PROPERTIES(mplpgo_static PROPERTIES LINK_OPTIONS "-Wl,-s,-z,relro,-z,now")
SET_TARGET_PROPERTIES(mplpgo PROPERTIES LINK_OPTIONS "-Wl,-s,-z,relro,-z,now")

# set output directory
SET(LIBRARY_OUTPUT_PATH ${MAPLE_ROOT}/libpgo/lib_${SYS_NAME})