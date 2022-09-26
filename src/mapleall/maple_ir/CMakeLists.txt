#
# Copyright (c) [2022] Huawei Technologies Co.,Ltd.All rights reserved.
#
# OpenArkCompiler is licensed under Mulan PSL v2.
# You can use this software according to the terms and conditions of the Mulan PSL v2.
# You may obtain a copy of Mulan PSL v2 at:
#
#     http://license.coscl.org.cn/MulanPSL2
#
# THIS SOFTWARE IS PROVIDED ON AN AS IS BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR
# FIT FOR A PARTICULAR PURPOSE.
# See the Mulan PSL v2 for more details.
#

set(inc_libmplir
  ${MAPLEALL_ROOT}/maple_ir/include
  ${MAPLEALL_ROOT}/mpl2mpl/include
  ${MAPLEALL_ROOT}/maple_util/include
  ${MAPLEALL_ROOT}/maple_driver/include
  ${MAPLEALL_ROOT}/mempool/include
  ${THIRD_PARTY_ROOT}/bounds_checking_function/include
  ${MAPLEALL_ROOT}/maple_ipa/include
  ${MAPLEALL_ROOT}/maple_ipa/include/old
  ${MAPLEALL_ROOT}/maple_me/include
  ${MAPLEALL_ROOT}/maple_phase/include
  ${THIRD_PARTY_ROOT}/llvm_modified/llvm/include/llvm/BinaryFormat
)

set(src_libmplir
  src/global_tables.cpp
  src/intrinsics.cpp
  src/lexer.cpp
  src/mir_symbol_builder.cpp
  src/mir_builder.cpp
  src/mir_const.cpp
  src/mir_scope.cpp
  src/mir_function.cpp
  src/mir_lower.cpp
  src/mir_module.cpp
  src/verification.cpp
  src/verify_annotation.cpp
  src/verify_mark.cpp
  src/mir_nodes.cpp
  src/mir_symbol.cpp
  src/mir_type.cpp
  src/mir_enum.cpp
  src/opcode_info.cpp
  src/option.cpp
  src/mpl2mpl_options.cpp
  src/parser.cpp
  src/mir_parser.cpp
  src/mir_pragma.cpp
  src/printing.cpp
  src/bin_func_import.cpp
  src/bin_func_export.cpp
  src/bin_mpl_import.cpp
  src/bin_mpl_export.cpp
  src/debug_info.cpp
  src/debug_info_util.cpp
  ${MAPLEALL_ROOT}/maple_ipa/src/old/ea_connection_graph.cpp
)

set(src_irbuild "src/driver.cpp")

set(src_mpldbg "src/mpl_dbg.cpp")

set(deps_libmplir
    libdriver_option
    libmplphase
    libmplutil
    libcommandline
)

set(deps_irbuild
    libmplir
    libmplphase
    libmplutil
    libcommandline
    libmempool
    libmpl2mpl
    libHWSecureC
    libdriver_option
)

set(CMAKE_CXX_FLAGS
    "${CMAKE_CXX_FLAGS} -DSTORE_BACK_VTABLE_REF_AFTER_LOAD=1")

#libmplir
add_library(libmplir STATIC ${src_libmplir})
set_target_properties(libmplir PROPERTIES
    COMPILE_FLAGS ""
    INCLUDE_DIRECTORIES "${inc_libmplir}"
    LINK_LIBRARIES "${deps_libmplir}"
    ARCHIVE_OUTPUT_DIRECTORY ${MAPLE_BUILD_OUTPUT}/lib/${HOST_ARCH}
)

#irbuild
add_executable(irbuild "${src_irbuild}")
set_target_properties(irbuild PROPERTIES
    COMPILE_FLAGS ""
    INCLUDE_DIRECTORIES "${inc_libmplir}"
    LINK_LIBRARIES "${deps_irbuild}"
    RUNTIME_OUTPUT_DIRECTORY "${MAPLE_BUILD_OUTPUT}/bin"
)

#mpldbg
add_executable(mpldbg "${src_mpldbg}")
set_target_properties(mpldbg PROPERTIES
    COMPILE_FLAGS ""
    INCLUDE_DIRECTORIES "${inc_libmplir}"
    LINK_LIBRARIES "${deps_irbuild}"
    RUNTIME_OUTPUT_DIRECTORY "${MAPLE_BUILD_OUTPUT}/bin"
)

