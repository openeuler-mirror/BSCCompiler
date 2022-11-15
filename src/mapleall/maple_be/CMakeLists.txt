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

set(deps_maple
  libcglowerer
  libdriver_option
  libmaple_driver
  libmplphase
  libcommandline
  libmplutil
  libmempool
  libmaplepgo
  libHWSecureC
  libcg
  libmplad
  libmplbe
  libmplipa
  libmplir
  libmplme
  libmplmewpo
  libmpl2mpl
  libmaple
)

set(inc_dirs
    ${MAPLEALL_ROOT}/maple_be/include/cg
    ${MAPLEALL_ROOT}/maple_be/include/ad
    ${MAPLE_BUILD_OUTPUT}/common/target
    ${MAPLEALL_ROOT}/maple_be/include/ad/target
    ${MAPLEALL_ROOT}/maple_be/include/be
    ${MAPLEALL_ROOT}/maple_driver/include
    ${MAPLEALL_ROOT}/maple_pgo/include
    ${MAPLEALL_ROOT}/maple_util/include
    ${MAPLEALL_ROOT}/maple_ir/include
    ${MAPLEALL_ROOT}/maple_me/include
    ${MAPLEALL_ROOT}/mpl2mpl/include
    ${MAPLEALL_ROOT}/mempool/include
    ${MAPLEALL_ROOT}/maple_ipa/include
    ${MAPLEALL_ROOT}/maple_ipa/include/old
    ${MAPLEALL_ROOT}/maple_phase/include
    ${THIRD_PARTY_ROOT}/bounds_checking_function/include
    ${THIRD_PARTY_ROOT}/llvm_modified/llvm/include/llvm/BinaryFormat
)

set(src_maple "")
set(deps_libmplbe "libcglowerer")

if(${TARGET} STREQUAL "aarch64" OR ${TARGET} STREQUAL "aarch64_ilp32")
    list(APPEND inc_dirs
         ${MAPLEALL_ROOT}/maple_be/include/cg/aarch64
         ${MAPLEALL_ROOT}/maple_be/include/be/aarch64
    )
    list(APPEND src_maple
        src/cg/aarch64/aarch64_abi.cpp
        src/cg/aarch64/aarch64_call_conv.cpp
        src/cg/aarch64/mpl_atomic.cpp
        src/cg/aarch64/aarch64_cfi_generator.cpp
        src/cg/aarch64/aarch64_cgfunc.cpp
        src/cg/aarch64/aarch64_dependence.cpp
        src/cg/aarch64/aarch64_data_dep_base.cpp
        src/cg/aarch64/aarch64_ebo.cpp
        src/cg/aarch64/aarch64_emitter.cpp
        src/cg/aarch64/aarch64_fixshortbranch.cpp
        src/cg/aarch64/aarch64_global.cpp
        src/cg/aarch64/aarch64_proepilog.cpp
        src/cg/aarch64/aarch64_operand.cpp
        src/cg/aarch64/aarch64_reg_info.cpp
        src/cg/aarch64/aarch64_ssa.cpp
        src/cg/aarch64/aarch64_prop.cpp
        src/cg/aarch64/aarch64_dce.cpp
        src/cg/aarch64/aarch64_phi_elimination.cpp
        src/cg/aarch64/aarch64_reg_coalesce.cpp
        src/cg/aarch64/aarch64_ico.cpp
        src/cg/aarch64/aarch64_insn.cpp
        src/cg/aarch64/aarch64_isa.cpp
        src/cg/aarch64/aarch64_memlayout.cpp
        src/cg/aarch64/aarch64_args.cpp
        src/cg/aarch64/aarch64_live.cpp
        src/cg/aarch64/aarch64_yieldpoint.cpp
        src/cg/aarch64/aarch64_offset_adjust.cpp
        src/cg/aarch64/aarch64_optimize_common.cpp
        src/cg/aarch64/aarch64_peep.cpp
        src/cg/aarch64/aarch64_reaching.cpp
        src/cg/aarch64/aarch64_schedule.cpp
        src/cg/aarch64/aarch64_strldr.cpp
        src/cg/aarch64/aarch64_ra_opt.cpp
        src/cg/aarch64/aarch64_tailcall.cpp
        src/cg/aarch64/aarch64_alignment.cpp
        src/cg/aarch64/aarch64_regsaves.cpp
        src/cg/aarch64/aarch64_utils.cpp
        src/cg/aarch64/aarch64_cg.cpp
        src/cg/aarch64/aarch64_validbit_opt.cpp
        src/cg/aarch64/aarch64_rce.cpp
        src/cg/aarch64/aarch64_cfgo.cpp
        src/cg/aarch64/aarch64_pgo_gen.cpp
        src/cg/aarch64/aarch64_isolate_fastpath.cpp
        src/cg/aarch64/aarch64_rematerialize.cpp
        src/cg/cfi_generator.cpp
        src/cg/cfgo.cpp
        src/cg/local_opt.cpp
        src/cg/ebo.cpp
        src/cg/ra_opt.cpp
        src/cg/tailcall.cpp
        src/cg/cg_pgo_gen.cpp
        src/cg/cg_pgo_use.cpp
        src/cg/cg_ssa.cpp
        src/cg/cg_prop.cpp
        src/cg/cg_dce.cpp
        src/cg/cg_phi_elimination.cpp
        src/cg/reg_coalesce.cpp
        src/cg/global.cpp
        src/cg/ico.cpp
        src/cg/peep.cpp
        src/cg/pressure.cpp
        src/cg/reaching.cpp
        src/cg/schedule.cpp
        src/cg/strldr.cpp
        src/cg/cg_dominance.cpp
        src/cg/cg_pre.cpp
        src/cg/cg_occur.cpp
        src/cg/cg_ssu_pre.cpp
        src/cg/cg_ssa_pre.cpp
        src/cg/regsaves.cpp
        src/cg/cg_critical_edge.cpp
        src/cg/alignment.cpp
        src/cg/cg_validbit_opt.cpp
        src/cg/cg_rce.cpp
        src/cg/control_dep_analysis.cpp
        src/cg/data_dep_base.cpp
        src/cg/data_dep_analysis.cpp
        src/cg/global_schedule.cpp
    )
endif()

if(${TARGET} STREQUAL "x86_64")
    list(APPEND inc_dirs
        ${MAPLEALL_ROOT}/maple_be/include/cg/x86_64
        ${MAPLEALL_ROOT}/maple_be/include/be/x86_64
    )
    list(APPEND src_maple
        src/cg/x86_64/x64_cg.cpp
        src/cg/x86_64/x64_MPIsel.cpp
        src/cg/x86_64/x64_cgfunc.cpp
        src/cg/x86_64/x64_memlayout.cpp
        src/cg/x86_64/x64_emitter.cpp
        src/cg/x86_64/x64_abi.cpp
        src/cg/x86_64/x64_call_conv.cpp
        src/cg/x86_64/x64_standardize.cpp
        src/cg/x86_64/x64_live.cpp
        src/cg/x86_64/x64_reg_info.cpp
        src/cg/x86_64/x64_proepilog.cpp
        src/cg/x86_64/x64_args.cpp
        src/cg/x86_64/x64_peep.cpp
        src/cg/x86_64/x64_reaching.cpp
        src/cg/x86_64/x64_local_opt.cpp
        src/cg/x86_64/x64_cfgo.cpp
        src/cg/x86_64/x64_isa.cpp
        src/cg/x86_64/x64_optimize_common.cpp
        src/cg/x86_64/x64_rematerialize.cpp
        src/cg/peep.cpp
        src/cg/alignment.cpp
        src/cg/reaching.cpp
        src/cg/local_opt.cpp
        src/cg/cfgo.cpp
        src/cg/cg_dominance.cpp
        src/cg/cg_pre.cpp
        src/cg/cg_occur.cpp
    )
endif()

if(${TARGET} STREQUAL "riscv64")
    list(APPEND inc_dirs
        ${MAPLEALL_ROOT}/maple_be/include/cg/riscv64
        ${MAPLEALL_ROOT}/maple_be/include/be/riscv64
    )
    list(APPEND src_maple
        src/cg/riscv64/mpl_atomic.cpp
        src/cg/riscv64/riscv64_abi.cpp
        src/cg/riscv64/riscv64_args.cpp
        src/cg/riscv64/riscv64_cg.cpp
        src/cg/riscv64/riscv64_cgfunc.cpp
        src/cg/riscv64/riscv64_color_ra.cpp
        src/cg/riscv64/riscv64_dependence.cpp
        src/cg/riscv64/riscv64_ebo.cpp
        src/cg/riscv64/riscv64_emitter.cpp
        src/cg/riscv64/riscv64_fixshortbranch.cpp
        src/cg/riscv64/riscv64_global.cpp
        src/cg/riscv64/riscv64_ico.cpp
        src/cg/riscv64/riscv64_immediate.cpp
        src/cg/riscv64/riscv64_insn.cpp
        src/cg/riscv64/riscv64_isa.cpp
        src/cg/riscv64/riscv64_live.cpp
        src/cg/riscv64/riscv64_lsra.cpp
        src/cg/riscv64/riscv64_memlayout.cpp
        src/cg/riscv64/riscv64_offset_adjust.cpp
        src/cg/riscv64/riscv64_operand.cpp
        src/cg/riscv64/riscv64_optimize_common.cpp
        src/cg/riscv64/riscv64_peep.cpp
        src/cg/riscv64/riscv64_proepilog.cpp
        src/cg/riscv64/riscv64_reaching.cpp
        src/cg/riscv64/riscv64_reg_alloc.cpp
        src/cg/riscv64/riscv64_schedule.cpp
        src/cg/riscv64/riscv64_strldr.cpp
        src/cg/riscv64/riscv64_yieldpoint.cpp
        src/cg/riscv64/riscv64_ra_opt.cpp
    )
endif()

if(${TARGET} STREQUAL "ark")
    list(APPEND inc_dirs
        ${MAPLEALL_ROOT}/maple_be/include/cg/ark
        ${MAPLEALL_ROOT}/maple_be/include/be/ark
    )
    list(APPEND src_maple
        src/cg/ark/foo.cpp
    )
endif()

set(src_libmplad "src/ad/mad.cpp")
set(src_libcglowerer
    src/be/bbt.cpp
    src/be/trycatchblockslower.cpp
    src/be/lower.cpp
)
set(src_libmplbe
    src/be/becommon.cpp
    src/be/switch_lowerer.cpp
    src/be/rt.cpp
)
set(src_libcg
    src/cg/args.cpp
    src/cg/cg_irbuilder.cpp
    src/cg/cfi.cpp
    src/cg/cgbb.cpp
    src/cg/operand.cpp
    src/cg/cgfunc.cpp
    src/cg/cg_cfg.cpp
    src/cg/cg_option.cpp
    src/cg/cg_options.cpp
    src/cg/dbg.cpp
    src/cg/optimize_common.cpp
    src/cg/eh_func.cpp
    src/cg/emit.cpp
    src/cg/live.cpp
    src/cg/loop.cpp
    src/cg/isel.cpp
    src/cg/standardize.cpp
    src/cg/memlayout.cpp
    src/cg/yieldpoint.cpp
    src/cg/label_creation.cpp
    src/cg/offset_adjust.cpp
    src/cg/reg_alloc.cpp
    src/cg/reg_alloc_basic.cpp
    src/cg/reg_alloc_lsra.cpp
    src/cg/reg_alloc_color_ra.cpp
    src/cg/proepilog.cpp
    src/cg/isolate_fastpath.cpp
    src/cg/rematerialize.cpp
    src/cg/cg.cpp
    src/cg/isa.cpp
    src/cg/insn.cpp
    src/cg/cg_phasemanager.cpp
)

string(REPLACE "-DRC_NO_MMAP" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
string(REPLACE "-DMIR_JAVA=1" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

# libmplad
add_library(libmplad STATIC ${src_libmplad})
target_include_directories(libmplad PRIVATE ${inc_dirs})
set_target_properties(libmplad PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${MAPLE_BUILD_OUTPUT}/lib/${HOST_ARCH}
    COMPILE_FLAGS "${CMAKE_CXX_FLAGS} -DRC_NO_MMAP"
)

# maplegen
set(src_maplegen
    ${MAPLEALL_ROOT}/maple_be/mdgen/src/mdgenerator.cpp
    ${MAPLEALL_ROOT}/maple_be/mdgen/src/mdlexer.cpp
    ${MAPLEALL_ROOT}/maple_be/mdgen/src/mdmain.cpp
    ${MAPLEALL_ROOT}/maple_be/mdgen/src/mdparser.cpp
    ${MAPLEALL_ROOT}/maple_be/mdgen/src/mdrecord.cpp
)
set(inc_maplegen
    ${MAPLEALL_ROOT}/maple_be/mdgen/include
    ${MAPLEALL_ROOT}/maple_util/include
    ${MAPLEALL_ROOT}/maple_ir/include
    ${MAPLEALL_ROOT}/mpl2mpl/include
    ${MAPLEALL_ROOT}/mempool/include
    ${THIRD_PARTY_ROOT}/bounds_checking_function/include
)
set(deps_maplegen
    libmplutil
    libmempool
    libHWSecureC
)
add_executable(maplegen "${src_maplegen}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -ldl")
set_target_properties(maplegen PROPERTIES
    COMPILE_FLAGS ""
    INCLUDE_DIRECTORIES "${inc_maplegen}"
    LINK_LIBRARIES "${deps_maplegen}"
    RUNTIME_OUTPUT_DIRECTORY "${MAPLE_BUILD_OUTPUT}/bin"
)

#libcg
add_library(libcg STATIC ${src_libcg})
set_target_properties(libcg PROPERTIES
    COMPILE_FLAGS ""
    INCLUDE_DIRECTORIES "${inc_dirs}"
    LINK_LIBRARIES ""
    ARCHIVE_OUTPUT_DIRECTORY ${MAPLE_BUILD_OUTPUT}/lib/${HOST_ARCH}
)

#libmplbe
add_library(libmplbe STATIC ${src_libmplbe})
set_target_properties(libmplbe PROPERTIES
    COMPILE_FLAGS ""
    INCLUDE_DIRECTORIES "${inc_dirs}"
    LINK_LIBRARIES "${deps_libmplbe}"
    ARCHIVE_OUTPUT_DIRECTORY ${MAPLE_BUILD_OUTPUT}/lib/${HOST_ARCH}
)

# libcglowerer
add_library(libcglowerer STATIC ${src_libcglowerer})
set_target_properties(libcglowerer PROPERTIES
    INCLUDE_DIRECTORIES "${inc_dirs}"
    ARCHIVE_OUTPUT_DIRECTORY ${MAPLE_BUILD_OUTPUT}/lib/${HOST_ARCH}
)


#maple
add_executable(maple "${src_maple}")
set_target_properties(maple PROPERTIES
  COMPILE_FLAGS ""
  INCLUDE_DIRECTORIES "${inc_dirs}"
  LINK_LIBRARIES "${deps_maple}"
  RUNTIME_OUTPUT_DIRECTORY "${MAPLE_BUILD_OUTPUT}/bin"
)

message("deps_maplegen: ${deps_maplegen}")





