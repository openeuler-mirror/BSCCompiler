/*
 * Copyright (c) [2020-2021] Huawei Technologies Co.,Ltd.All rights reserved.
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
#include "me_may2dassign.h"

// this phase converts all maydassign back to dassign
namespace maple {
void May2Dassign::DoIt() {
  MeCFG *cfg = func.GetCfg();
  auto eIt = cfg->valid_end();
  for (auto bIt = cfg->valid_begin(); bIt != eIt; ++bIt) {
    auto *bb = *bIt;
    for (auto &stmt : bb->GetMeStmts()) {
      if (stmt.GetOp() != OP_maydassign) {
        continue;
      }
      auto &mass = static_cast<MaydassignMeStmt&>(stmt);
      // chiList for Maydassign has only 1 element
      CHECK_FATAL(!mass.GetChiList()->empty(), "chiList is empty in DoIt");
      VarMeExpr *theLhs = static_cast<VarMeExpr *>(mass.GetChiList()->cbegin()->second->GetLHS());
      ASSERT(mass.GetMayDassignSym() == theLhs->GetOst(),
             "MeDoMay2Dassign: cannot find maydassign lhs");
      auto *dass = static_cast<DassignMeStmt*>(irMap->CreateAssignMeStmt(*theLhs, *mass.GetRHS(), *bb));
      dass->SetNeedDecref(mass.NeedDecref());
      dass->SetNeedIncref(mass.NeedIncref());
      dass->SetWasMayDassign(true);
      dass->SetChiList(*mass.GetChiList());
      dass->GetChiList()->clear();
      bb->ReplaceMeStmt(&mass, dass);
    }
  }
}

void MEMay2Dassign::GetAnalysisDependence(maple::AnalysisDep &aDep) const {
  aDep.AddRequired<MEIRMapBuild>();
  aDep.SetPreservedAll();
}

bool MEMay2Dassign::PhaseRun(maple::MeFunction &f) {
  May2Dassign may2Dassign(f);
  may2Dassign.DoIt();
  return true;
}
}  // namespace maple
