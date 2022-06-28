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
#ifndef MAPLE_IR_INCLUDE_MIR_SCOPE_H
#define MAPLE_IR_INCLUDE_MIR_SCOPE_H
#include "mir_module.h"
#include "mir_type.h"
#include "src_position.h"

namespace maple {
// mapping src variable to mpl variables to display debug info
struct MIRAliasVars {
  GStrIdx mplStrIdx;  // maple varialbe name
  TyIdx tyIdx;
  bool isLocal;
  GStrIdx sigStrIdx;
};

class MIRScope {
 public:
  MIRScope(MIRModule *mod);
  ~MIRScope() = default;

  bool NeedEmitAliasInfo() const {
    return aliasVarMap.size() != 0 || subScopes.size() != 0;
  }

  bool IsSubScope(const MIRScope *scp) const;
  bool HasJoinScope(const MIRScope *scp1, const MIRScope *scp2) const;
  bool HasSameRange(const MIRScope *s1, const MIRScope *s2) const;

  unsigned GetId() const {
    return id;
  }

  const SrcPosition &GetRangeLow() const {
    return range.first;
  }

  const SrcPosition &GetRangeHigh() const {
    return range.second;
  }

  void SetRange(SrcPosition low, SrcPosition high) {
    ASSERT(low.IsBfOrEq(high), "wrong order of low and high");
    range.first = low;
    range.second = high;
  }

  void SetAliasVarMap(GStrIdx idx, const MIRAliasVars &vars) {
    ASSERT(aliasVarMap.find(idx) == aliasVarMap.end(), "alias already exist");
    aliasVarMap[idx] = vars;
  }

  void AddAliasVarMap(GStrIdx idx, const MIRAliasVars &vars) {
    /* allow same idx, save last aliasVars */
    aliasVarMap[idx] = vars;
  }

  MapleMap<GStrIdx, MIRAliasVars> &GetAliasVarMap() {
    return aliasVarMap;
  }

  MapleVector<MIRScope*> &GetSubScopes() {
    return subScopes;
  }

  void IncLevel();
  bool AddScope(MIRScope *scope);
  void Dump(int32 indent) const;
  void Dump() const;

 private:
  MIRModule *module;
  unsigned id;
  std::pair<SrcPosition, SrcPosition> range;
  // source to maple variable alias
  MapleMap<GStrIdx, MIRAliasVars> aliasVarMap { module->GetMPAllocator().Adapter() };
  // subscopes' range should be disjoint
  MapleVector<MIRScope*> subScopes { module->GetMPAllocator().Adapter() };
};
}  // namespace maple
#endif  // MAPLE_IR_INCLUDE_MIR_SCOPE_H
