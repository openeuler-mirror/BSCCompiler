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

set(inc_libmaple
  ${MAPLEALL_ROOT}
  ${MAPLEALL_ROOT}/maple_be/include
  ${MAPLEALL_ROOT}/maple_be/include/cg
  ${MAPLEALL_ROOT}/maple_be/include/be
  ${MAPLEALL_ROOT}/maple_be/include/ad
  ${MAPLEALL_ROOT}/maple_be/include/ad/target
  ${MAPLE_BUILD_OUTPUT}/common/target
  ${MAPLEALL_ROOT}/maple_be/include/cg/aarch64
  ${MAPLEALL_ROOT}/maple_be/include/be/aarch64
  ${MAPLEALL_ROOT}/maple_driver/defs
  ${MAPLEALL_ROOT}/maple_driver/include
  ${MAPLEALL_ROOT}/maple_util/include
  ${MAPLEALL_ROOT}/maple_pgo/include
  ${MAPLEALL_ROOT}/mpl2mpl/include
  ${MAPLEALL_ROOT}/maple_me/include
  ${MAPLEALL_ROOT}/maple_ipa/include
  ${MAPLEALL_ROOT}/maple_ipa/include/old
  ${MAPLEALL_ROOT}/maple_phase/include
  ${MAPLEALL_ROOT}/maple_ir/include
  ${MAPLEALL_ROOT}/mempool/include
  ${THIRD_PARTY_ROOT}/bounds_checking_function/include
)

set(src_libmaple
  src/as_compiler.cpp
  src/compiler.cpp
  src/compiler_factory.cpp
  src/dex2mpl_compiler.cpp
  src/driver_runner.cpp
  src/ipa_compiler.cpp
  src/jbc2mpl_compiler.cpp
  src/cpp2mpl_compiler.cpp
  src/clang_compiler.cpp
  src/ld_compiler.cpp
  src/maple.cpp
  src/maple_comb_compiler_wrapper.cpp
  src/maple_comb_compiler.cpp
  src/mpl_options.cpp
  src/mplcg_compiler.cpp
  src/hided_options.cpp
  src/parse_spec.cpp
)

#libmaple
add_library(libmaple STATIC ${src_libmaple})
set_target_properties(libmaple PROPERTIES
  COMPILE_FLAGS ""
  INCLUDE_DIRECTORIES "${inc_libmaple}"
  LINK_LIBRARIES ""
  RUNTIME_OUTPUT_DIRECTORY "${MAPLE_BUILD_OUTPUT}/lib/${HOST_ARCH}"
)

set(src_libmaple_driver
  src/triple.cpp
)

set(inc_libmaple_driver
  ${MAPLEALL_ROOT}/maple_driver/include
  ${MAPLEALL_ROOT}/maple_util/include
)

#libmaple_driver
add_library(libmaple_driver STATIC ${src_libmaple_driver})
set_target_properties(libmaple_driver PROPERTIES
  COMPILE_FLAGS ""
  INCLUDE_DIRECTORIES "${inc_libmaple_driver}"
  LINK_LIBRARIES ""
  ARCHIVE_OUTPUT_DIRECTORY "${MAPLE_BUILD_OUTPUT}/lib/${HOST_ARCH}"
)

set(src_libdriver_option
  src/driver_options.cpp
)

set(inc_libdriver_option
  ${MAPLEALL_ROOT}/maple_driver/include
  ${MAPLEALL_ROOT}/maple_util/include
)

#libdriver_option
add_library(libdriver_option STATIC ${src_libdriver_option})
set_target_properties(libdriver_option PROPERTIES
  COMPILE_FLAGS ""
  INCLUDE_DIRECTORIES "${inc_libdriver_option}"
  LINK_LIBRARIES ""
  ARCHIVE_OUTPUT_DIRECTORY "${MAPLE_BUILD_OUTPUT}/lib/${HOST_ARCH}"
)


