/*
 * Copyright (c) [2022] Futurewei Technologies, Inc. All rights reverved.
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
#include "mir_function.h"
#include "printing.h"

namespace maple {

// scp is a sub scope
// (low (scp.low, scp.high] high]
bool MIRScope::IsSubScope(MIRScope *scp) {
  // special case for function level scope which might not have range specified
  if (level == 0) {
    return true;
  }
  SrcPosition l  = GetRangeLow();
  SrcPosition h  = GetRangeHigh();
  SrcPosition l1 = scp->GetRangeLow();
  SrcPosition h1 = scp->GetRangeHigh();
  return l.IsBfOrEq(l1) && h1.IsBfOrEq(h);
}

// s1 and s2 has join
// (s1.low (s2.low s1.high] s2.high]
// (s2.low (s1.low s2.high] s1.high]
bool MIRScope::HasJoinScope(MIRScope *scp1, MIRScope *scp2) {
  SrcPosition l1 = scp1->GetRangeLow();
  SrcPosition h1 = scp1->GetRangeHigh();
  SrcPosition l2 = scp2->GetRangeLow();
  SrcPosition h2 = scp1->GetRangeHigh();
  return (l1.IsBfOrEq(l2) && l2.IsBfOrEq(h1)) ||
         (l2.IsBfOrEq(l1) && l1.IsBfOrEq(h2));
}

void MIRScope::IncLevel() {
  level++;
  for (auto s : subScopes) {
    s->IncLevel();
  }
}

bool MIRScope::AddScope(MIRScope *scope) {
  // check first if it is valid with parent scope and sibling sub scopes
  ASSERT(IsSubScope(scope), "is not a subscope");
  for (auto s : subScopes) {
    if (HasJoinScope(s, scope)) {
      ASSERT(false, "has join range with another subscope");
    }
  }
  if (this != module->CurFunction()->GetScope()) {
    // skip level incremental if this is function-scope, of level 0,
    // as scope is aready starting from 1
    scope->IncLevel();
  }
  subScopes.push_back(scope);
  return true;
}

void MIRScope::Dump(int32 indent) const {
  unsigned ind = (level != 0);
  if (level != 0) {
    SrcPosition low = range.first;
    SrcPosition high = range.second;
    PrintIndentation(indent);
    // LogInfo::MapleLogger() << level << " ";
    LogInfo::MapleLogger() << "SCOPE <(" <<
      low.FileNum() << ", " <<
      low.LineNum() << ", " <<
      low.Column() << "), (" <<
      high.FileNum() << ", " <<
      high.LineNum() << ", " <<
      high.Column() << ")> {\n";
  }

  for (auto it : aliasVarMap) {
    PrintIndentation(indent + ind);
    LogInfo::MapleLogger() << "ALIAS %" << GlobalTables::GetStrTable().GetStringFromStrIdx(it.first) << " %"
      << GlobalTables::GetStrTable().GetStringFromStrIdx(it.second.memPoolStrIdx) << " ";
    GlobalTables::GetTypeTable().GetTypeFromTyIdx(it.second.tyIdx)->Dump(0);
    if (it.second.sigStrIdx) {
      LogInfo::MapleLogger() << " \"" << GlobalTables::GetStrTable().GetStringFromStrIdx(it.second.sigStrIdx) << "\"";
    }
    LogInfo::MapleLogger() << '\n';
  }

  for (auto it : subScopes) {
    it->Dump(indent + ind);
  }

  if (level != 0) {
    PrintIndentation(indent);
    LogInfo::MapleLogger() << "}\n";
  }
}

void MIRScope::D() const {
  Dump(0);
}
}  // namespace maple
