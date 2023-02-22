#
# Copyright (c) [2022] Huawei Technologies Co.,Ltd.All rights reserved.
#
# OpenArkCompiler is licensed under Mulan PSL v2.
# You can use this software according to the terms and conditions of the Mulan PSL v2.
# You may obtain a copy of Mulan PSL v2 at:
#
#   http://license.coscl.org.cn/MulanPSL2
#
# THIS SOFTWARE IS PROVIDED ON AN AS IS BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR
# FIT FOR A PARTICULAR PURPOSE.
# See the Mulan PSL v2 for more details.
#

set(inc_dirs
  ${MAPLEALL_ROOT}
  ${MAPLEALL_ROOT}/maple_be/include
  ${MAPLEALL_ROOT}/maple_be/include/cg
  ${MAPLEALL_ROOT}/maple_be/include/be
  ${MAPLEALL_ROOT}/maple_be/include/ad
  ${MAPLEALL_ROOT}/maple_pgo/include/
  ${MAPLE_BUILD_OUTPUT}/common/target
  ${MAPLEALL_ROOT}/maple_driver/defs
  ${MAPLEALL_ROOT}/maple_driver/include
  ${MAPLEALL_ROOT}/maple_util/include
  ${MAPLEALL_ROOT}/mpl2mpl/include
  ${MAPLEALL_ROOT}/maple_me/include
  ${MAPLEALL_ROOT}/maple_ipa/include
  ${MAPLEALL_ROOT}/maple_ipa/include/old
  ${MAPLEALL_ROOT}/maple_phase/include
  ${MAPLEALL_ROOT}/maple_ir/include
  ${MAPLEALL_ROOT}/mempool/include
  ${THIRD_PARTY_ROOT}/bounds_checking_function/include
)

set(src_libmaplepgo
  src/instrument.cpp
  src/cfg_mst.cpp
  src/litepgo.cpp
)

#libmaplepgo
add_library(libmaplepgo STATIC ${src_libmaplepgo})

set_target_properties(libmaplepgo PROPERTIES
  COMPILE_FLAGS ""
  INCLUDE_DIRECTORIES "${inc_dirs}"
  LINK_LIBRARIES ""
  RUNTIME_OUTPUT_DIRECTORY "${MAPLE_BUILD_OUTPUT}/lib/${HOST_ARCH}"
)
