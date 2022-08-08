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


set(src_libmempool
  src/maple_string.cpp
  src/mempool.cpp
)

add_library(mempool STATIC ${src_libmempool})
target_include_directories(mempool PRIVATE
    ${MAPLEALL_ROOT}/maple_ir/include
    ${MAPLEALL_ROOT}/maple_util/include
    ${MAPLEALL_ROOT}/maple_driver/include
    ${THIRD_PARTY_ROOT}/bounds_checking_function/include
    include 
)

set_target_properties(mempool PROPERTIES 
    LIBRARY_OUTPUT_DIRECTORY ${MAPLE_BUILD_OUTPUT}/lib/64
    ARCHIVE_OUTPUT_DIRECTORY ${MAPLE_BUILD_OUTPUT}/lib/64
    RUNTIME_OUTPUT_DIRECTORY ${MAPLE_BUILD_OUTPUT}/lib/64
)

