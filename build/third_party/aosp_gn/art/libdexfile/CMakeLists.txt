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
set(include_directories
  ${AOSP_ROOT}/system/core/base/include
  ${AOSP_ROOT}/system/core/libziparchive/include
  ${AOSP_ROOT}/system/core/liblog/include
  ${AOSP_ROOT}/system/core/libutils/include
  ${AOSP_ROOT}/art/libartpalette/include
  ${AOSP_ROOT}/art/libartbase
  ${AOSP_ROOT}/art/libdexfile
  ${AOSP_ROOT}/include
  ${AOSP_ROOT}/libnativehelper/include_jni
)

set(sources_files
  dex/art_dex_file_loader.cc
  dex/compact_dex_file.cc
  dex/compact_offset_table.cc
  dex/descriptors_names.cc
  dex/dex_file.cc
  dex/dex_file_exception_helpers.cc
  dex/dex_file_layout.cc
  dex/dex_file_loader.cc
  dex/dex_file_tracking_registrar.cc
  dex/dex_file_verifier.cc
  dex/dex_instruction.cc
  dex/modifiers.cc
  dex/primitive.cc
  dex/signature.cc
  dex/standard_dex_file.cc
  dex/type_lookup_table.cc
  dex/utf.cc
)

# libdexfile
add_library(libdexfile STATIC ${sources_files})
set_target_properties(libdexfile PROPERTIES
  COMPILE_FLAGS "${CMAKE_CXX_FLAGS} -Wno-defaulted-function-deleted -std=gnu++17 -fPIC"
  INCLUDE_DIRECTORIES "${include_directories}"
  LINK_LIBRARIES ""
  RUNTIME_OUTPUT_DIRECTORY "${MAPLE_BUILD_OUTPUT}/lib/${HOST_ARCH}"
)
