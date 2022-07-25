/*
 * Copyright (c) [2019-2021] Huawei Technologies Co.,Ltd.All rights reserved.
 *
 * OpenArkCompiler is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *
 *     http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR
 * FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 */
#ifndef MPL2MPL_INCLUDE_MODULE_PHASE_MANAGER_H
#define MPL2MPL_INCLUDE_MODULE_PHASE_MANAGER_H
#include "maple_phase_manager.h"
#include "class_hierarchy_phase.h"
#include "class_init.h"
#include "bin_mpl_export.h"
#include "clone.h"
#include "ipa_clone.h"
#include "call_graph.h"
#include "verification.h"
#include "verify_mark.h"
#include "inline.h"
#include "method_replace.h"
#include "gen_profile.h"
#if MIR_JAVA
#include "native_stub_func.h"
#include "vtable_analysis.h"
#include "reflection_analysis.h"
#include "annotation_analysis.h"
#include "vtable_impl.h"
#include "java_intrn_lowering.h"
#include "simplify.h"
#include "java_eh_lower.h"
#include "muid_replacement.h"
#include "gen_check_cast.h"
#include "coderelayout.h"
#include "constantfold.h"
#include "preme.h"
#include "scalarreplacement.h"
#include "openProfile.h"
#include "update_mplt.h"
#endif  // ~MIR_JAVA
#include "option.h"
#include "me_option.h"

namespace maple {
class MEBETopLevelManager : public ModulePM {
 public:
  explicit MEBETopLevelManager(MemPool *mp) : ModulePM(mp, &id) {}
  ~MEBETopLevelManager() override {};
  PHASECONSTRUCTOR(MEBETopLevelManager)
  std::string PhaseName() const override;
  void DoPhasesPopulate(const MIRModule &m);
  void Run(MIRModule &mod);
  void InitFuncDescWithWhiteList(const MIRModule &mod);
  bool IsRunMpl2Mpl() const {
    return runMpl2mpl;
  }
  void SetRunMpl2Mpl(bool value) {
    runMpl2mpl = value;
  }
  bool IsRunMe() const {
    return runMe;
  }
  void SetRunMe(bool value) {
    runMe = value;
  }
 private:
  bool runMpl2mpl = false;
  bool runMe = false;
};
}  // namespace maple
#endif  // MPL2MPL_INCLUDE_MODULE_PHASE_MANAGER_H
