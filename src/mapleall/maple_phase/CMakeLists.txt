#
# Copyright (c) [2022] Huawei Technologies Co.,Ltd.All rights reserved.
#
# OpenArkCompiler is licensed under Mulan PSL v2.
# You can use this software according to the terms and conditions of the Mulan PSL v2.
# You may obtain a copy of Mulan PSL v2 at:
#
# http://license.coscl.org.cn/MulanPSL2
#
# THIS SOFTWARE IS PROVIDED ON AN AS IS BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR
# FIT FOR A PARTICULAR PURPOSE.
# See the Mulan PSL v2 for more details.
#

set(inc_libmplphase
    ${MAPLEALL_ROOT}/maple_me/include
    ${MAPLEALL_ROOT}/maple_ipa/include
    ${MAPLEALL_ROOT}/maple_ipa/include/old
    ${MAPLEALL_ROOT}/maple_util/include
    ${MAPLEALL_ROOT}/mempool/include
    ${MAPLEALL_ROOT}/maple_phase/include
    ${MAPLEALL_ROOT}/maple_ir/include
    ${MAPLEALL_ROOT}/mpl2mpl/include
    ${MAPLEALL_ROOT}/maple_be/include/cg
    ${MAPLEALL_ROOT}/maple_be/include/be
    ${MAPLEALL_ROOT}/maple_be/include/ad
    ${MAPLEALL_ROOT}/maple_be/include/ad/target
    ${THIRD_PARTY_ROOT}/bounds_checking_function/include
    ${THIRD_PARTY_ROOT}/llvm_modified/llvm/include/llvm/BinaryFormat
    ${MAPLEALL_ROOT}/maple_driver/include
)

set(src_libmplphase
    src/phase_impl.cpp
    src/maple_phase.cpp
    src/maple_phase_manager.cpp
    src/phase_driver.cpp
    src/maple_phase_support.cpp
)

#libmplphase
add_library(libmplphase STATIC ${src_libmplphase})
set_target_properties(libmplphase PROPERTIES
    COMPILE_FLAGS ""
    INCLUDE_DIRECTORIES "${inc_libmplphase}"
    LINK_LIBRARIES ""
    ARCHIVE_OUTPUT_DIRECTORY ${MAPLE_BUILD_OUTPUT}/lib/${HOST_ARCH}
)
