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

if(${CMAKE_BUILD_TYPE} STREQUAL "RELEASE")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 -fno-strict-aliasing -D_FORTIFY_SOURCE=2")
elseif(${CMAKE_BUILD_TYPE} STREQUAL "DEBUG")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 -g3 -ftrapv -fstack-check")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 -fno-strict-aliasing -g")
endif()

if(${GCONLY} STREQUAL "1")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DGCONLY=1")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DGCONLY=0")
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror -fPIC -fstack-protector-strong -fno-common -pipe -Wextra -Weffc++ -Wdate-time -Wfloat-equal -Wshadow -Wformat=2")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -isystem ${GCC_LINARO_PATH}/bin/../lib/gcc/aarch64-linux-gnu/7.5.0/../../../../aarch64-linux-gnu/include/c++/7.5.0")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -isystem ${GCC_LINARO_PATH}/bin/../lib/gcc/aarch64-linux-gnu/7.5.0/../../../../aarch64-linux-gnu/include/c++/7.5.0/aarch64-linux-gnu")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -isystem ${GCC_LINARO_PATH}/bin/../lib/gcc/aarch64-linux-gnu/7.5.0/../../../../aarch64-linux-gnu/include/c++/7.5.0/backward")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -isystem ${GCC_LINARO_PATH}/bin/../lib/gcc/aarch64-linux-gnu/7.5.0/include")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -isystem ${GCC_LINARO_PATH}/bin/../lib/gcc/aarch64-linux-gnu/7.5.0/include-fixed")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -isystem ${GCC_LINARO_PATH}/bin/../lib/gcc/aarch64-linux-gnu/7.5.0/../../../../aarch64-linux-gnu/include")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -isystem ${GCC_LINARO_PATH}/bin/../aarch64-linux-gnu/libc/usr/include")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -target aarch64-linux-gnu")

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-z,relro -Wl,-z,now -Wl,-z,noexecstack")

add_subdirectory(compiler-rt)
add_subdirectory(libcore-static-binding-jni)
add_subdirectory(maplert)

set(deps
    libmplcompiler-rt
    libcore-static-binding-jni
    libmaplert
    libhuawei_secure_c
)

#maple-rt
add_library(maple-rt INTERFACE)
set_target_properties(maple-rt PROPERTIES
    COMPILE_FLAGS ""
    INCLUDE_DIRECTORIES ""
    LINK_LIBRARIES "${deps}"
    RUNTIME_OUTPUT_DIRECTORY "${MAPLE_BUILD_OUTPUT}/bin"
)