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
#include "method_replace.h"
#include <mir_symbol.h>

// Replace some method with new method call, signature and return value not changed
// def format (caller origCallee newCallee).
namespace maple {
void MethodReplace::InsertRecord(const std::string &caller, const std::string &origCallee,
                                 const std::string &newCallee) {
  auto callerIt = replaceMethodMap.find(caller);
  if (callerIt == replaceMethodMap.end()) {
    replaceMethodMap[caller] = { { origCallee, newCallee } };
    return;
  }
  std::unordered_map<std::string, std::string> &calleeMap = callerIt->second;
  calleeMap[origCallee] = newCallee;
}

void MethodReplace::Init() {
#define ORIFUNC(CALLER, ORIGINAL, NEWFUNC) InsertRecord(#CALLER, #ORIGINAL, #NEWFUNC)

#include "tobe_replaced_funcs.def"

#undef ORIFUNC
}

void MethodReplace::FindCalleeAndReplace(MIRFunction &callerFunc, StrStrMap &calleeMap) const {
  // iterate get callassignstmt to original callee, exclude virtual call interface call
  StmtNode *stmt = callerFunc.GetBody()->GetFirst();
  while (stmt != nullptr) {
    Opcode op = stmt->op;
    if (op != OP_callassigned) {
      stmt = stmt->GetNext();
      continue;
    }
    CallNode *callNode = static_cast<CallNode*>(stmt);
    MIRFunction *calleeFunc = GlobalTables::GetFunctionTable().GetFunctionFromPuidx(callNode->GetPUIdx());
    std::string calleeName = calleeFunc->GetName();
    auto it = calleeMap.find(calleeName);
    if (it == calleeMap.end()) {
      stmt = stmt->GetNext();
      continue;
    }
    std::string &newCalleeName = it->second;
    MIRFunction *newCalleeFunc = mBuilder.GetOrCreateFunction(newCalleeName, TyIdx(PTY_void));
    callNode->SetPUIdx(newCalleeFunc->GetPuidx());
    stmt = stmt->GetNext();
  }
}

// iterate callers in ReplaceMethod, find callee and replace
void MethodReplace::DoMethodReplace() {
  for (auto it = replaceMethodMap.begin(); it != replaceMethodMap.end(); ++it) {
    const std::string &callerName = it->first;
    MIRSymbol *symbol = GlobalTables::GetGsymTable().GetSymbolFromStrIdx(
        GlobalTables::GetStrTable().GetStrIdxFromName(callerName));
    if (symbol == nullptr || symbol->GetSKind() != kStFunc) {
      continue;
    }
    MIRFunction *callerFunc = symbol->GetValue().mirFunc;
    if (callerFunc == nullptr || callerFunc->GetBody() == nullptr) {
      continue;
    }
    mirModule->SetCurFunction(callerFunc);
    FindCalleeAndReplace(*callerFunc, it->second);
  }
}

bool M2MMethodReplace::PhaseRun(maple::MIRModule &m) {
  maple::MIRBuilder builder(&m);
  MethodReplace methodReplace(&m, GetPhaseMemPool(), builder);
  methodReplace.Init();
  methodReplace.DoMethodReplace();
  return true;
}

void M2MMethodReplace::GetAnalysisDependence(maple::AnalysisDep &aDep) const {
  aDep.SetPreservedAll();
}
}  // namespace maple

