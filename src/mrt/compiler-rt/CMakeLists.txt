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
    src/allocator.cpp
    src/chelper.cpp
    src/chosen.cpp
    src/cinterface.cpp
    src/collector/arena.cpp
    src/collector/collector.cpp
    src/collector/collector_rc.cpp
    src/collector/collector_ms.cpp
    src/collector/collector_naiverc.cpp
    src/collector/collector_naiverc_ms.cpp
    src/collector/collector_tracing.cpp
    src/collector/conn_comp.cpp
    src/collector/cp_generator.cpp
    src/collector/cycle_collector.cpp
    src/collector/gc_reason.cpp
    src/collector/mpl_thread_pool.cpp
    src/collector/mrt_bitmap.cpp
    src/collector/native_gc.cpp
    src/collector/rp_base.cpp
    src/collector/rc_reference_processor.cpp
    src/collector/gc_reference_processor.cpp
    src/collector/stats.cpp
    src/exception/eh_personality.cpp
    src/exception/exception_handling.cpp
    src/gc_log.cpp
    src/libs.cpp
    src/mm_config.cpp
    src/errno_utils.cpp
    src/mm_utils.cpp
    src/exception/mpl_exception.cpp
    src/exception/mrt_exception.cpp
    src/mutator_list.cpp
    src/namemangler.cpp
    src/mangle_for_jni.cpp
    src/file_layout.cpp
    src/muid.cpp
    src/panic.cpp
    src/profile.cpp
    src/heap_stats.cpp
    src/allocator/ros_allocator.cpp
    src/allocator/space.cpp
    src/allocator/mem_map.cpp
    src/allocator/bp_allocator.cpp
    src/allocator/page_map.cpp
    src/exception/stack_unwinder.cpp
    src/yieldpoint.cpp
    src/tracer.cpp
    src/linker/linker.cpp
    src/linker/linker_api.cpp
    src/linker/linker_info.cpp
    src/linker/linker_model.cpp
    src/linker/linker_utils.cpp
    src/linker/linker_cache.cpp
    src/linker/linker_gctib.cpp
    src/linker/linker_lazy_binding.cpp
    src/linker/linker_method_builder.cpp
    src/linker/linker_hotfix.cpp
    src/linker/linker_debug.cpp
    src/loader/file_adapter.cpp
    src/loader/hash_pool.cpp
    src/loader/loader_utils.cpp
    src/loader/object_locator.cpp
    src/loader/object_loader.cpp
    src/arch/arm64/cc_native_method_stub_arm64.S
    src/arch/arm64/fastFuncs.S
    src/arch/arm64/i2r_stub_arm64.S
    src/arch/arm64/interp_native_method_stub_arm64.S
    src/arch/arm64/memset.S
    src/arch/arm64/prepare_args_for_exception_catcher_arm64.S
    src/arch/arm64/proxy_stub_arm64.S
    src/arch/arm64/r2c_stub_arm64.S
    src/arch/arm64/signal_handler_arm64.cpp
    src/arch/arm64/signal_handler_stub_arm64.S
    src/arch/arm64/yieldpoint_arm64.S
    src/libs_fast.cpp
)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=hidden -pthread -std=gnu++14 -fasynchronous-unwind-tables -nostdlibinc -DMAPLE_EH_UTEST=0")

#libmplcompiler-rt
add_library(libmplcompiler-rt STATIC ${sources_common})
target_include_directories(libmplcompiler-rt PRIVATE
    include
    public-headers
    ${AOSP_ROOT}/libnativehelper/include_jni
    ${THIRD_PARTY_ROOT}/bounds_checking_function/include/
    ${MAPLE_MRT_ROOT}/maplert/include
    ${MAPLE_MRT_ROOT}/maplert/public-headers/
    ${MAPLE_MRT_ROOT}/libmrtbase/include/
    ${MAPLE_MRT_ROOT}/libmrtbase/include/linux
    ${MAPLE_MRT_ROOT}/dexinterface
    ${MAPLE_MRT_ROOT}/interpreter/zterp
    ${MAPLE_MRT_ROOT}
)
set(include_common
    ${MAPLE_MRT_ROOT}/compiler-rt/include
    ${MAPLE_MRT_ROOT}/compiler-rt/public-headers
    ${AOSP_ROOT}/libnativehelper/include_jni
    ${THIRD_PARTY_ROOT}/bounds_checking_function/include/
    ${MAPLE_MRT_ROOT}/maplert/include
    ${MAPLE_MRT_ROOT}/maplert/public-headers/
    ${MAPLE_MRT_ROOT}/libmrtbase/include/
    ${MAPLE_MRT_ROOT}/libmrtbase/include/linux
    ${MAPLE_MRT_ROOT}/dexinterface
    ${MAPLE_MRT_ROOT}/interpreter/zterp
    ${MAPLE_MRT_ROOT}
)
set_target_properties(libmplcompiler-rt PROPERTIES
    COMPILE_FLAGS ""
    INCLUDE_DIRECTORIES "${include_common}"
    LINK_LIBRARIES ""
    ARCHIVE_OUTPUT_DIRECTORY "${MAPLE_BUILD_OUTPUT}/ar/host-x86_64-${OPT}"
)