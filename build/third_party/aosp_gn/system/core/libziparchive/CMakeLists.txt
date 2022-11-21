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
  ${AOSP_ROOT}/system/core/include
  ${AOSP_ROOT}/system/core/base/include
  ${AOSP_ROOT}/system/core/libziparchive/include
  ${AOSP_ROOT}/system/core/libziparchive
  ${AOSP_ROOT}/system/core/liblog/include
  ${AOSP_ROOT}/system/core/libutils/include
  ${AOSP_ROOT}/include
)

set(sources_files
  zip_archive.cc
  zip_archive_stream_entry.cc
  zip_writer.cc
)

# libziparchive
add_library(libziparchive STATIC ${sources_files})
set_target_properties(libziparchive PROPERTIES
  COMPILE_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++17 -fPIC"
  INCLUDE_DIRECTORIES "${include_directories}"
  LINK_LIBRARIES ""
  RUNTIME_OUTPUT_DIRECTORY "${MAPLE_BUILD_OUTPUT}/lib/${HOST_ARCH}"
)
