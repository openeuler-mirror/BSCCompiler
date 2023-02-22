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

set(inc_dir
  ${MAPLEALL_ROOT}/maple_ir/include
  ${MAPLEALL_ROOT}/mempool/include
  ${THIRD_PARTY_ROOT}/bounds_checking_function/include
  ${MAPLEALL_ROOT}/maple_me/include
  ${MAPLEALL_ROOT}/maple_ipa/include
  ${MAPLEALL_ROOT}/maple_ipa/include/old
  ${MAPLEALL_ROOT}/mpl2mpl/include
  ${MAPLEALL_ROOT}/maple_util/include
  ${MAPLEALL_ROOT}/maple_pgo/include
  ${MAPLEALL_ROOT}/maple_driver/include
  ${MAPLEALL_ROOT}/maple_phase/include
  ${MAPLEALL_ROOT}/maple_be/include/be
  ${MAPLEALL_ROOT}/maple_be/include/cg
  ${MAPLEALL_ROOT}/maple_be/include/ad
  ${MAPLEALL_ROOT}/maple_be/include/ad/target
  ${MAPLE_BUILD_OUTPUT}/common/target
  ${MAPLEALL_ROOT}/maple_san/include
)

set(src_libmplme
  src/dse.cpp
  src/hdse.cpp
  src/meexpr_use_info.cpp
  src/vst_use_info.cpp
  src/me_sink.cpp
  src/prop.cpp
  src/me_alias_class.cpp
  src/me_critical_edge.cpp
  src/me_bb_layout.cpp
  src/me_bypath_eh.cpp
  src/me_profile_gen.cpp
  src/me_profile_use.cpp
  src/me_cfg.cpp
  src/me_dse.cpp
  src/me_hdse.cpp
  src/me_dominance.cpp
  src/me_emit.cpp
  src/me_ssa_devirtual.cpp
  src/ssa_devirtual.cpp
  src/me_function.cpp
  src/me_loop_analysis.cpp
  src/me_irmap.cpp
  src/me_irmap_build.cpp
  src/me_analyzector.cpp
  src/me_loop_canon.cpp
  src/me_loop_inversion.cpp
  src/me_scalar_analysis.cpp
  src/me_loop_unrolling.cpp
  src/me_option.cpp
  src/me_options.cpp
  src/me_phase_manager.cpp
  src/me_prop.cpp
  src/copy_prop.cpp
  src/me_ssa_prop.cpp
  src/me_placement_rc.cpp
  src/me_analyze_rc.cpp
  src/me_delegate_rc.cpp
  src/me_cond_based_opt.cpp
  src/me_rc_lowering.cpp
  src/me_lower_globals.cpp
  src/me_may2dassign.cpp
  src/preg_renamer.cpp
  src/me_rename2preg.cpp
  src/me_ssa.cpp
  src/me_toplevel_ssa.cpp
  src/me_ssa_tab.cpp
  src/me_ssa_update.cpp
  src/me_stmt_fre.cpp
  src/me_stmt_pre.cpp
  src/me_store_pre.cpp
  src/me_stack_protect.cpp
  src/me_ssa_epre.cpp
  src/me_gvn.cpp
  src/me_ssa_lpre.cpp
  src/me_ssu_pre.cpp
  src/ssa_epre.cpp
  src/ssa_pre.cpp
  src/ssa_epre_for_sr.cpp
  src/ssa_epre_for_lftr.cpp
  src/ssa_pre_for_hoist.cpp
  src/occur.cpp
  src/me_inequality_graph.cpp
  src/me_abco.cpp
  src/me_ssi.cpp
  src/me_cfg_opt.cpp
  src/me_predict.cpp
  src/me_check_cast.cpp
  src/me_side_effect.cpp
  src/me_bb_analyze.cpp
  src/meconstprop.cpp
  src/sync_select.cpp
  src/me_gc_lowering.cpp
  src/me_gc_write_barrier_opt.cpp
  src/me_subsum_rc.cpp
  src/me_func_opt.cpp
  src/me_verify.cpp
  src/cast_opt.cpp
  src/me_merge_stmts.cpp
  src/me_fsaa.cpp
  src/pme_mir_lower.cpp
  src/lfo_inject_iv.cpp
  src/pme_emit.cpp
  src/lfo_iv_canon.cpp
  src/lfo_dep_test.cpp
  src/lfo_loop_vec.cpp
  src/me_value_range_prop.cpp
  src/me_jump_threading.cpp
  src/me_obj_size.cpp
  src/cfg_opt.cpp
  src/optimizeCFG.cpp
  src/me_ivopts.cpp
  src/seqvec.cpp
  src/me_autovec.cpp
  src/me_safety_warning.cpp
  src/lfo_unroll.cpp
  src/code_factoring.cpp
  src/me_slp.cpp
  src/lmbc_memlayout.cpp
  src/lmbc_lower.cpp
  src/mc_ssa_pre.cpp
)

set(src_libmplmewpo
  src/alias_class.cpp
  src/demand_driven_alias_analysis.cpp
  src/type_based_alias_analysis.cpp
  src/bb.cpp
  src/func_emit.cpp
  src/irmap.cpp
  src/irmap_build.cpp
  src/irmap_emit.cpp
  src/me_ir.cpp
  src/orig_symbol.cpp
  src/ssa.cpp
  src/ssa_mir_nodes.cpp
  src/ssa_tab.cpp
  src/ver_symbol.cpp
)

#libmplme
add_library(libmplme STATIC ${src_libmplme})
set_target_properties(libmplme PROPERTIES
  COMPILE_FLAGS ""
  INCLUDE_DIRECTORIES "${inc_dir}"
  LINK_LIBRARIES ""
  ARCHIVE_OUTPUT_DIRECTORY ${MAPLE_BUILD_OUTPUT}/lib/${HOST_ARCH}
)

#libmplmewpo
add_library(libmplmewpo STATIC ${src_libmplmewpo})
set_target_properties(libmplmewpo PROPERTIES
  COMPILE_FLAGS ""
  INCLUDE_DIRECTORIES "${inc_dir}"
  LINK_LIBRARIES ""
  ARCHIVE_OUTPUT_DIRECTORY ${MAPLE_BUILD_OUTPUT}/lib/${HOST_ARCH}
)
