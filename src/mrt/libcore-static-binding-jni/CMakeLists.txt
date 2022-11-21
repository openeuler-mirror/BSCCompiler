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

set(sources_common
    src/java_lang_Class.cpp
    src/java_lang_reflect_Array.cpp
    src/java_lang_reflect_Field.cpp
    src/java_lang_reflect_Method.cpp
    src/java_lang_String.cpp
    src/java_lang_StringFactory.cpp
    src/java_lang_System.cpp
    src/libcore_util_CharsetUtils.cpp
    src/native_binding_utils.cpp
    src/java_lang_ref_Reference.cpp
    src/java_io_File.cpp
    src/com_android_internal_util_ArrayUtils.cpp
    src/java_lang_Object.cpp
    src/java_lang_Thread.cpp
)

set(include_common
    ${AOSP_ROOT}/libnativehelper/include_jni
    ${MAPLE_MRT_ROOT}/libcore-static-binding-jni/include
    ${MAPLE_MRT_ROOT}/maplert/include
    ${MAPLE_MRT_ROOT}/compiler-rt/include
    ${MAPLE_MRT_ROOT}/compiler-rt/public-headers
    ${MAPLE_MRT_ROOT}/libmrtbase/include
    ${MAPLE_MRT_ROOT}/libmrtbase/include/linux
    ${MAPLE_MRT_ROOT}/maplert/public-headers
    ${THIRD_PARTY_ROOT}/bounds_checking_function/include/
    ${MAPLE_MRT_ROOT}/dexinterface
    ${MAPLE_MRT_ROOT}/interpreter/zterp
    ${MAPLE_MRT_ROOT}
)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DPLATFORM_SDK_VERSION=27 -DJNIDEBUG=0 -DTRACE_LEVEL=3")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=hidden -std=c++11 -nostdlibinc -march=armv8-a")

#libcore-static-binding-jni
add_library(libcore-static-binding-jni STATIC ${sources_common})
set_target_properties(libcore-static-binding-jni PROPERTIES
    COMPILE_FLAGS ""
    INCLUDE_DIRECTORIES "${include_common}"
    LINK_LIBRARIES ""
    ARCHIVE_OUTPUT_DIRECTORY "${MAPLE_BUILD_OUTPUT}/ar/host-x86_64-${OPT}"
)
