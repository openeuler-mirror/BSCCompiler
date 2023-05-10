/*
 * Copyright (c) [2019-2020] Huawei Technologies Co.,Ltd.All rights reserved.
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
#include "retype.h"
#include <algorithm>

namespace maple {
class AnnotationType;
void Retype::ReplaceRetypeExpr(const BaseNode &expr) {
  if (expr.NumOpnds() == 0) {
    return;
  }
  for (size_t i = 0; i < expr.NumOpnds(); ++i) {
    BaseNode *opnd = expr.Opnd(i);
    if (opnd->GetOpCode() == OP_retype) {
      opnd->SetOpnd(opnd->Opnd(0), i);
      continue;
    }
    ReplaceRetypeExpr(*opnd);
  }
}

void Retype::TransmitGenericInfo(MIRFunction &func, StmtNode &stmt) {
  if (stmt.GetOpCode() != OP_dassign) {
    return;
  }
  DassignNode &dassignNode = static_cast<DassignNode&>(stmt);
  BaseNode *rhs = static_cast<DassignNode&>(stmt).GetRHS();
  if (!rhs->IsLeaf()) {
    return;
  }
  GStrIdx lhsNameStrIdx = func.GetLocalOrGlobalSymbol(dassignNode.GetStIdx())->GetNameStrIdx();
  GStrIdx rhsNameStrIdx = func.GetLocalOrGlobalSymbol(static_cast<AddrofNode*>(rhs)->GetStIdx())->GetNameStrIdx();
  const MapleMap<GStrIdx, MIRAliasVars> &map = func.GetAliasVarMap();
  if (reg2varGenericInfo.find(lhsNameStrIdx) == reg2varGenericInfo.end() ||
      reg2varGenericInfo[lhsNameStrIdx].sigStrIdx == 0) {
    return;
  }
  if (reg2varGenericInfo.find(rhsNameStrIdx) != reg2varGenericInfo.end() &&
      reg2varGenericInfo[rhsNameStrIdx].sigStrIdx != 0) {
    return;
  }
  MIRAliasVars aliasVar = reg2varGenericInfo[lhsNameStrIdx];
  aliasVar.mplStrIdx = rhsNameStrIdx;
  if (map.find(rhsNameStrIdx) != map.end() && map.at(rhsNameStrIdx).mplStrIdx != rhsNameStrIdx) {
    CHECK_FATAL(false, "must be");
  }
  func.SetAliasVarMap(rhsNameStrIdx, aliasVar);
}

void Retype::RetypeStmt(MIRFunction &func) {
  if (func.IsEmpty()) {
    return;
  }
  for (auto &stmt : func.GetBody()->GetStmtNodes()) {
    if (stmt.GetOpCode() == OP_comment) {
      continue;
    }
    for (size_t i = 0; i < stmt.NumOpnds(); ++i) {
      BaseNode *opnd = stmt.Opnd(i);
      if (opnd->GetOpCode() == OP_retype) {
        stmt.SetOpnd(opnd->Opnd(0), i);
        TransmitGenericInfo(func, stmt);
        continue;
      } else {
        ReplaceRetypeExpr(*opnd);
      }
    }
  }
}

void Retype::DoRetype() {
  for (MIRFunction *func : std::as_const(mirModule->GetFunctionList())) {
    if (func->IsEmpty()) {
      continue;
    }
    for (auto pairIter = func->GetAliasVarMap().cbegin(); pairIter != func->GetAliasVarMap().cend(); ++pairIter) {
      GStrIdx regNameStrIdx = pairIter->second.mplStrIdx;
      reg2varGenericInfo[regNameStrIdx] = pairIter->second;
    }
    RetypeStmt(*func);
    reg2varGenericInfo.clear();
  }
}
}  // namespace maple
