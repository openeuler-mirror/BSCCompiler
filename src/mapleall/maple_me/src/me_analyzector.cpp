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
#include "dominance.h"
#include "me_analyzector.h"
#include "utils.h"

// AnalyzeCtor analyzes which fields are assigned inside of each constructor
namespace maple {

void AnalyzeCtor::ProcessFunc() {
  MIRFunction *curFunc = func->GetMirFunc();
  if (!curFunc->IsConstructor() || curFunc->IsEmpty() || curFunc->GetParamSize() == 0) {
    return;
  }
  hasSideEffect = false;
  auto cfg = func->GetCfg();
  if (curFunc->GetBody() != nullptr) {
    for (BB *bb : cfg->GetAllBBs()) {
      if (bb == nullptr)
        continue;
      for (auto &meStmt : bb->GetMeStmts()) {
        ProcessStmt(meStmt);
      }
    }
  }
  PUIdx puIdx = curFunc->GetPuidx();
  // if the function has calls with sideeffect, conservatively
  // we assume all fields are modified in ctor
  if (hasSideEffect) {
    MapleSet<FieldID> *fieldSubSet =
        func->GetMIRModule().GetMemPool()->New<MapleSet<FieldID>>(std::less<FieldID>(),
                                                                  func->GetMIRModule().GetMPAllocator().Adapter());
    fieldSubSet->insert(0);  // write to all
    func->GetMIRModule().SetPuIdxFieldSet(puIdx, fieldSubSet);
  } else if (!fieldSet.empty()) {
    MapleSet<FieldID> *fieldSubSet =
        func->GetMIRModule().GetMemPool()->New<MapleSet<FieldID>>(std::less<FieldID>(),
                                                                  func->GetMIRModule().GetMPAllocator().Adapter());
    std::copy(fieldSet.begin(), fieldSet.end(), std::inserter(*fieldSubSet, fieldSubSet->begin()));
    func->GetMIRModule().SetPuIdxFieldSet(puIdx, fieldSubSet);
  }
}

// collect field ids which are assigned inside the stmt and mark sideeffect
// flag for non-ctor calls
void AnalyzeCtor::ProcessStmt(MeStmt &stmt) {
  if (kOpcodeInfo.IsCall(stmt.GetOp())) {
    hasSideEffect = true;
    return;
  }
  switch (stmt.GetOp()) {
    case OP_iassign: {
      auto &iassign = static_cast<IassignMeStmt&>(stmt);
      auto &ivarMeExpr = static_cast<IvarMeExpr&>(*iassign.GetLHSVal());
      MIRType *baseType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(ivarMeExpr.GetTyIdx());
      ASSERT(baseType != nullptr, "null ptr check");
      auto *pointedType = static_cast<MIRPtrType*>(baseType)->GetPointedType();
      auto structType = safe_cast<MIRStructType>(pointedType);
      if (structType != nullptr) {
        MIRType *fieldType = structType->GetFieldType(ivarMeExpr.GetFieldID());
        if (fieldType->GetPrimType() != PTY_ref) {
          break;
        }
      }
      fieldSet.insert(ivarMeExpr.GetFieldID());
      break;
    }
    default:
      break;
  }
}

AnalysisResult *MeDoAnalyzeCtor::Run(MeFunction *func, MeFuncResultMgr *m, ModuleResultMgr *moduleResultMgr) {
  Dominance *dom = static_cast<Dominance*>(m->GetAnalysisResult(MeFuncPhase_DOMINANCE, func));
  auto *kh = static_cast<KlassHierarchy*>(moduleResultMgr->GetAnalysisResult(MoPhase_CHA, &func->GetMIRModule()));
  ASSERT_NOT_NULL(dom);
  ASSERT_NOT_NULL(m->GetAnalysisResult(MeFuncPhase_IRMAPBUILD, func));
  AnalyzeCtor analyzeCtor(*func, *dom, *kh);
  analyzeCtor.ProcessFunc();
  return nullptr;
}
}  // namespace maple
