/*
 * Copyright (c) [2023] Huawei Technologies Co.,Ltd.All rights reserved.
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
#include "me_simplifyexpr.h"
#include "me_phase_manager.h"

namespace maple {
void SimplifyExpr::DealWithOpnd(MeStmt &meStmt, MeExpr &meExpr, bool &doOpt) {
  for (uint8 i = 0; i < meExpr.GetNumOpnds(); ++i) {
    auto resOpnd = irMap.ReadContinuousMemory(*meExpr.GetOpnd(i));
    if (resOpnd != nullptr) {
      meExpr.SetOpnd(i, resOpnd);
      doOpt = true;
      continue;
    }
    DealWithOpnd(meStmt, *meExpr.GetOpnd(i), doOpt);
  }
}

bool SimplifyExpr::DealWithStmt(MeStmt &meStmt) {
  bool doOpt = false;
  for (size_t i = 0; i < meStmt.NumMeStmtOpnds(); ++i) {
    auto opnd = meStmt.GetOpnd(i);
    auto resOpnd = irMap.ReadContinuousMemory(*opnd);
    if (resOpnd != nullptr) {
      doOpt = true;
      meStmt.SetOpnd(i, resOpnd);
      continue;
    }
    DealWithOpnd(meStmt, *opnd, doOpt);
  }
  return doOpt;
}

void SimplifyExpr::TravelCands() {
  for (auto it = optStmtCands.begin(); it != optStmtCands.end();) {
    if (DealWithStmt(**it)) {
      ++it;
      continue;
    }
    it = optStmtCands.erase(it);
  }
}

void SimplifyExpr::Execute() {
  for (auto bIt = dom.GetReversePostOrder().begin(); bIt != dom.GetReversePostOrder().end(); ++bIt) {
    auto curBB = func.GetCfg()->GetBBFromID(BBId((*bIt)->GetID()));
    for (auto &meStmt : curBB->GetMeStmts()) {
      if (DealWithStmt(meStmt)) {
        optStmtCands.push_back(&meStmt);
      }
    }
  }
  while (!optStmtCands.empty()) {
    TravelCands();
  }
}

void MESimplifyExpr::GetAnalysisDependence(maple::AnalysisDep &aDep) const {
  aDep.AddRequired<MEIRMapBuild>();
  aDep.AddRequired<MEDominance>();
  aDep.SetPreservedAll();
}

bool MESimplifyExpr::PhaseRun(maple::MeFunction &f) {
  auto *irMap = GET_ANALYSIS(MEIRMapBuild, f);
  CHECK_FATAL(irMap != nullptr, "irMap phase has problem");
  auto *dom = EXEC_ANALYSIS(MEDominance, f)->GetDomResult();
  CHECK_FATAL(dom != nullptr, "dominance phase has problem");
  SimplifyExpr simplifyExpr(f, *irMap, *dom);
  simplifyExpr.Execute();
  return true;
}
}