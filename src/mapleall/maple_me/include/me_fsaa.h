/*
 * Copyright (c) [2021] Huawei Technologies Co.,Ltd.All rights reserved.
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
#ifndef MAPLE_ME_INCLUDE_ME_FSAA_H
#define MAPLE_ME_INCLUDE_ME_FSAA_H
#include "me_option.h"
#include "me_function.h"
#include "me_dominance.h"
#include "me_ssa_tab.h"
#include "maple_phase.h"

namespace maple {
class FSAA {
 public:
  FSAA(MeFunction *f, Dominance *dm)
      : func(f), mirModule(&f->GetMIRModule()), ssaTab(f->GetMeSSATab()), dom(dm) {}
  ~FSAA() {}

  BB *FindUniquePointerValueDefBB(VersionSt *vst);
  void ProcessBB(BB *bb);

  bool needUpdateSSA = false;
 private:
  void RemoveMayDefIfSameAsRHS(const IassignNode *stmt);
  void RemoveMayDefByIreadRHS(const IreadSSANode *rhs, TypeOfMayDefList &mayDefNodes);
  void RemoveMayDefByDreadRHS(const AddrofSSANode *rhs, TypeOfMayDefList &mayDefNodes);
  void EraseMayDefItem(TypeOfMayDefList &mayDefNodes, MapleMap<OStIdx, MayDefNode>::iterator &it, bool canBeErased);

  MeFunction *func;
  MIRModule *mirModule;
  SSATab *ssaTab;
  Dominance *dom;

  std::string PhaseName() const {
    return "fsaa";
  }
};

MAPLE_FUNC_PHASE_DECLARE(MEFSAA, MeFunction)
}  // namespace maple
#endif  // MAPLE_ME_INCLUDE_ME_FSAA_H
