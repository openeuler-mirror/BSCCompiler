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
    src/mrt_array.cpp
    src/mrt_class_init.cpp
    src/mrt_object.cpp
    src/mrt_reflection_class.cpp
    src/mrt_reflection_method.cpp
    src/mrt_reflection_field.cpp
    src/mrt_reflection_constructor.cpp
    src/modifier.cpp
    src/mrt_reflection_proxy.cpp
    src/mrt_reflection_executable.cpp
    src/mrt_annotation.cpp
    src/mrt_annotation_parser.cpp
    src/mrt_string.cpp
    src/mrt_util.cpp
    src/mrt_preload_class.cpp
    src/mrt_linker.cpp
    src/mrt_classloader.cpp
    src/mrt_reflection_reference.cpp
    src/literalstrname.cpp
    src/java2c_rule.cpp
    src/mrt_poisonstack.cpp
    src/mrt_methodhandle.cpp
    src/mrt_methodhandle_mpl.cpp
    src/mrt_handlecommon.cpp
    src/mrt_handleutil.cpp
    src/mrt_profile.cpp
    src/mrt_well_known.cpp
    src/mrt_primitive_util.cpp
    src/itab_util.cpp
    src/mobject.cpp
    src/marray.cpp
    src/mstring.cpp
    src/mclass.cpp
    src/mmethod.cpp
    src/mfield.cpp
    src/fieldmeta.cpp
    src/methodmeta.cpp
    src/deferredaccess.cpp
)

set(include_common
    ${MAPLE_MRT_ROOT}/maplert/include
    ${AOSP_ROOT}/libnativehelper/include_jni
    ${THIRD_PARTY_ROOT}/bounds_checking_function/include/
    ${MAPLE_MRT_ROOT}/compiler-rt/include
    ${MAPLE_MRT_ROOT}/compiler-rt/public-headers
    ${MAPLE_MRT_ROOT}/maplert/public-headers
    ${MAPLE_MRT_ROOT}/libmrtbase/include
    ${MAPLE_MRT_ROOT}/libmrtbase/include/linux
    ${MAPLE_MRT_ROOT}/interpreter/zterp
    ${MAPLE_MRT_ROOT}/dexinterface
    ${MAPLE_MRT_ROOT}/verifier/include
    ${MAPLE_MRT_ROOT}
)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DPLATFORM_SDK_VERSION=27 -DJNIDEBUG=0 -DTRACE_LEVEL=3")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=hidden -std=c++14 -nostdlibinc -march=armv8-a")

#libmaplert
add_library(libmaplert STATIC ${sources_common})
set_target_properties(libmaplert PROPERTIES
    COMPILE_FLAGS ""
    INCLUDE_DIRECTORIES "${include_common}"
    LINK_LIBRARIES ""
    ARCHIVE_OUTPUT_DIRECTORY "${MAPLE_BUILD_OUTPUT}/ar/host-x86_64-${OPT}"
)
