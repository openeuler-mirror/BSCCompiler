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

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror")

set(include_directories
    ${MAPLE_ROOT}/tools/gtest_lib/include
    ${MAPLEALL_ROOT}/maple_util/include
    ${MAPLEALL_ROOT}/maple_ir/include/
    ${MAPLEALL_ROOT}/mempool/include/
    ${THIRD_PARTY_ROOT}/bounds_checking_function/include
)

set(src_mapleallUT
    cl_ut_test.cpp
    int_val_test.cpp
    stacktrace_test.cpp
    maple_string_test.cpp
)

set(deps
    libdriver_option
    libmplphase
    libmplutil
    libmplir
    libmempool
    libmpl2mpl
    libcommandline
    libHWSecureC
    ${MAPLE_ROOT}/tools/gtest_lib/lib/libgmock.a
    ${MAPLE_ROOT}/tools/gtest_lib/lib/libgmock_main.a
    ${MAPLE_ROOT}/tools/gtest_lib/lib/libgtest.a
    ${MAPLE_ROOT}/tools/gtest_lib/lib/libgtest_main.a
    pthread
    dl
)

#mapleallUT
add_executable(mapleallUT "${src_mapleallUT}")
set_target_properties(mapleallUT PROPERTIES
    COMPILE_FLAGS ""
    INCLUDE_DIRECTORIES "${include_directories}"
    LINK_LIBRARIES "${deps}"
    RUNTIME_OUTPUT_DIRECTORY "${MAPLE_BUILD_OUTPUT}/bin"
)
