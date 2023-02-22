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

set(src_libmplipa
  src/old/ipa_escape_analysis.cpp
  src/old/do_ipa_escape_analysis.cpp
  src/old/ea_connection_graph.cpp
  src/old/ipa_option.cpp
  src/ipa_side_effect.cpp
  src/ipa_phase_manager.cpp
  src/prop_return_null.cpp
  src/prop_parameter_type.cpp
  src/ipa_collect.cpp
  src/ipa_clone.cpp
  src/region_identify.cpp
)

add_library(libmplipa STATIC ${src_libmplipa})
target_include_directories(libmplipa PRIVATE
  ${MAPLEALL_ROOT}/maple_ir/include
  ${MAPLEALL_ROOT}/mempool/include
  ${MAPLEALL_ROOT}/maple_util/include
  ${MAPLEALL_ROOT}/maple_pgo/include
  ${MAPLEALL_ROOT}/maple_me/include
  ${MAPLEALL_ROOT}/maple_ipa/include
  ${MAPLEALL_ROOT}/maple_ipa/include/old
  ${MAPLEALL_ROOT}/mpl2mpl/include
  ${MAPLEALL_ROOT}/maple_phase/include
  ${MAPLEALL_ROOT}/maple_driver/include
  ${THIRD_PARTY_ROOT}/bounds_checking_function/include
)

set_target_properties(libmplipa PROPERTIES 
    ARCHIVE_OUTPUT_DIRECTORY ${MAPLE_BUILD_OUTPUT}/lib/${HOST_ARCH}
)
