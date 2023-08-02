/*
 * Copyright (c) [2022] Huawei Technologies Co.,Ltd.All rights reserved.
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

#include "vst_use_info.h"
#include "ssa.h"
#include "me_cfg.h"

namespace maple {
void VstUseInfo::CollectUseInfoInExpr(BaseNode *expr, StmtNode *stmt) {
  if (expr == nullptr) {
    return;
  }
  if (expr->IsSSANode()) {
    VersionSt *vst = static_cast<SSANode*>(expr)->GetSSAVar();
    AddUseSiteOfVst(vst, stmt);
  }
  // collect from opnds
  for (size_t i = 0; i < expr->NumOpnds(); ++i) {
    auto *opnd = expr->Opnd(i);
    CollectUseInfoInExpr(opnd, stmt);
  }
}

void VstUseInfo::CollectUseInfoInStmt(StmtNode *stmt) {
  // collect from muList
  if (useInfoState > kVstUseInfoTopLevelVst && kOpcodeInfo.HasSSAUse(stmt->GetOpCode())) {
    TypeOfMayUseList &mayUseList = ssaTab->GetStmtsSSAPart().GetMayUseNodesOf(*stmt);
    for (auto it = mayUseList.begin(); it != mayUseList.end(); ++it) {
      MayUseNode &mayUse = it->second;
      VersionSt *vst = mayUse.GetOpnd();
      AddUseSiteOfVst(vst, &mayUse);
    }
  }
  // collect from opnds
  for (size_t i = 0; i < stmt->NumOpnds(); ++i) {
    auto *opnd = stmt->Opnd(i);
    CollectUseInfoInExpr(opnd, stmt);
  }
}

void VstUseInfo::CollectUseInfoInBB(BB *bb) {
  if (bb == nullptr) {
    return;
  }
  // collect from philist
  const auto &phiList = bb->GetPhiList();
  for (const auto &ost2phi : phiList) {
    PhiNode phi = ost2phi.second;
    for (size_t i = 0; i < phi.GetPhiOpnds().size(); ++i) {
      AddUseSiteOfVst(phi.GetPhiOpnd(i), &phi);
    }
  }
  // collect from stmts
  for (StmtNode &stmt : bb->GetStmtNodes()) {
    CollectUseInfoInStmt(&stmt);
  }
}

void VstUseInfo::CollectUseInfoInFunc(MeFunction *f, Dominance *dom, VstUnseInfoState state) {
  if (!IsUseInfoInvalid() && state <= useInfoState) { // use info has been collected before
    return;
  }
  useInfoState = state; // update state
  if (useSites == nullptr) {
    useSites = allocator.New<MapleVector<VstUseSiteList*>>(allocator.Adapter());
  }
  ssaTab = f->GetMeSSATab();
  useSites->resize(ssaTab->GetVersionStTable().GetVersionStVectorSize());

  for (auto *bb : dom->GetReversePostOrder()) {
    CollectUseInfoInBB(f->GetCfg()->GetBBFromID(BBId(bb->GetID())));
  }
}
} // namespace maple