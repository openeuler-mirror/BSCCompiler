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
#include "mir_scope.h"
#include "mir_function.h"
#include "printing.h"

namespace maple {

static unsigned scopeId = 1;

MIRScope::MIRScope(MIRModule *mod,  MIRFunction *f)
  : module(mod), func(f), id(scopeId++), isLocal(true) {
  alias = module->GetMemPool()->New<MIRAlias>(module);
  typeAlias = module->GetMemPool()->New<MIRTypeAlias>(module);
}

void MIRAlias::Dump(int32 indent, bool isLocal) const {
  bool first = true;
  for (auto it : aliasVarMap) {
    if (first) {
      first = false;
      LogInfo::MapleLogger() << '\n';
    } else {
      LogInfo::MapleLogger() << ",\n";
    }
    PrintIndentation(indent);
    LogInfo::MapleLogger() << "ALIAS "
                           << (isLocal ? "%" : "$")
                           << GlobalTables::GetStrTable().GetStringFromStrIdx(it.first)
                           << ((it.second.isLocal) ? " %" : " $")
                           << GlobalTables::GetStrTable().GetStringFromStrIdx(it.second.mplStrIdx) << " ";
    switch (it.second.atk) {
      case kATKType: {
        TyIdx idx(it.second.index);
        GlobalTables::GetTypeTable().GetTypeFromTyIdx(TyIdx(idx))->Dump(0);
        break;
      }
      case kATKString: {
        GStrIdx idx(it.second.index);
        LogInfo::MapleLogger() << "\"" << GlobalTables::GetStrTable().GetStringFromStrIdx(idx)
                               << "\"";
        break;
      }
      case kATKEnum: {
        MIREnum *mirEnum = GlobalTables::GetEnumTable().enumTable[it.second.index];
        CHECK_NULL_FATAL(mirEnum);
        LogInfo::MapleLogger() << "$" << GlobalTables::GetStrTable().GetStringFromStrIdx(mirEnum->GetNameIdx());
        break;
      }
      default :
        break;
    }
    it.second.attrs.DumpAttributes();
    if (it.second.sigStrIdx) {
      LogInfo::MapleLogger() << " \"" << GlobalTables::GetStrTable().GetStringFromStrIdx(it.second.sigStrIdx) << "\"";
    }
  }
}

void MIRTypeAlias::Dump(int32 indent) const {
  bool first = true;
  for (auto it = typeAliasMap.cbegin(); it != typeAliasMap.cend(); ++it) {
    if (first) {
      first = false;
      LogInfo::MapleLogger() << '\n';
    } else {
      LogInfo::MapleLogger() << ",\n";
    }
    PrintIndentation(indent);
    const std::string name = GlobalTables::GetStrTable().GetStringFromStrIdx(it->first);
    MIRType *type = GlobalTables::GetTypeTable().GetTypeFromTyIdx(it->second);
    CHECK_NULL_FATAL(type);
    LogInfo::MapleLogger() << "TYPEALIAS $" << name << " ";
    type->Dump(1);
  }
}

// scp is a sub scope
// (low (scp.low, scp.high] high]
bool MIRScope::IsSubScope(const MIRScope *scp) const {
  auto &l = GetRangeLow();
  auto &l1 = scp->GetRangeLow();
  // allow included file
  if (l.FileNum() != l1.FileNum()) {
    return true;
  }
  auto &h = GetRangeHigh();
  auto &h1 = scp->GetRangeHigh();
  return l.IsBfOrEq(l1) && h1.IsBfOrEq(h);
}

// s1 and s2 has join
// (s1.low (s2.low s1.high] s2.high]
// (s2.low (s1.low s2.high] s1.high]
bool MIRScope::HasJoinScope(const MIRScope *scp1, const MIRScope *scp2) const {
  auto &l1 = scp1->GetRangeLow();
  auto &h1 = scp1->GetRangeHigh();
  auto &l2 = scp2->GetRangeLow();
  auto &h2 = scp2->GetRangeHigh();
  return (l1.IsBfOrEq(l2) && l2.IsBfOrEq(h1)) || (l2.IsBfOrEq(l1) && l1.IsBfOrEq(h2));
}

// scope range of s1 and s2 may be completly same when macro calling macro expand
bool MIRScope::HasSameRange(const MIRScope *scp1, const MIRScope *scp2) const {
  auto &l1 = scp1->GetRangeLow();
  auto &h1 = scp1->GetRangeHigh();
  auto &l2 = scp2->GetRangeLow();
  auto &h2 = scp2->GetRangeHigh();
  return l1.IsEq(l2) && h1.IsEq(h2);
}

SrcPosition MIRScope::GetScopeEndPos(const SrcPosition &pos) {
  SrcPosition low  = GetRangeLow();
  SrcPosition high = GetRangeHigh();
  if (pos.IsEq(low)) {
    return high;
  }
  for (auto it : func->GetScope()->blkSrcPos) {
    SrcPosition p  = std::get<0>(it);
    SrcPosition pB = std::get<1>(it);
    SrcPosition pE = std::get<2>(it);
    if (pos.IsEq(p)) {
      // pB < low < p < pE < high
      if (pB.IsBfOrEq(low) &&
          low.IsBfOrEq(p) &&
          pE.IsBfOrEq(high)) {
        return high;
      }
    }
  }

  SrcPosition result = SrcPosition();
  for (auto *scope : subScopes) {
    result = scope->GetScopeEndPos(pos);
    if (result.IsValid()) {
      return result;
    }
  }
  return result;
}

bool MIRScope::AddScope(MIRScope *scope) {
  // check first if it is valid with parent scope and sibling sub scopes
  CHECK_FATAL(IsSubScope(scope), "<%s %s> is not a subscope of scope <%s %s>",
              scope->GetRangeLow().DumpLocWithColToString().c_str(),
              scope->GetRangeHigh().DumpLocWithColToString().c_str(),
              GetRangeLow().DumpLocWithColToString().c_str(),
              GetRangeHigh().DumpLocWithColToString().c_str());
  for (auto *s : subScopes) {
    if (!HasSameRange(s, scope) && HasJoinScope(s, scope)) {
      CHECK_FATAL(false, "<%s %s> has join range with another subscope <%s %s>",
                  scope->GetRangeLow().DumpLocWithColToString().c_str(),
                  scope->GetRangeHigh().DumpLocWithColToString().c_str(),
                  s->GetRangeLow().DumpLocWithColToString().c_str(),
                  s->GetRangeHigh().DumpLocWithColToString().c_str());
    }
  }
  subScopes.push_back(scope);
  return true;
}

void MIRScope::Dump(int32 indent) const {
  LogInfo::MapleLogger() << '\n';
  SrcPosition low = range.first;
  SrcPosition high = range.second;
  PrintIndentation(indent);
  LogInfo::MapleLogger() << "SCOPE " <<
    id << " <(" <<
    low.FileNum() << ", " <<
    low.LineNum() << ", " <<
    low.Column() << "), (" <<
    high.FileNum() << ", " <<
    high.LineNum() << ", " <<
    high.Column() << ")> {";

  typeAlias->Dump(indent + 1);
  if (!typeAlias->IsEmpty() && !alias->IsEmpty()) {
    LogInfo::MapleLogger() << ",";
  }
  alias->Dump(indent + 1, isLocal);
  if (subScopes.size() == 0) {
    LogInfo::MapleLogger() << "\n";
  }
  for (auto it : subScopes) {
    if (!it->IsEmpty()) {
      it->Dump(indent + 1);
    }
  }

  PrintIndentation(indent);
  LogInfo::MapleLogger() << "}\n";
}

void MIRScope::Dump() const {
  Dump(0);
}
}  // namespace maple
