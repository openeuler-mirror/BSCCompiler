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

namespace maple {
// mapping src variable to mpl variables to display debug info
struct MIRAliasVars {
  GStrIdx memPoolStrIdx;
  TyIdx tyIdx;
  GStrIdx sigStrIdx;
};

class MIRScope {
 public:
  MIRModule *module;
  unsigned level = 0;
  std::pair<SrcPosition, SrcPosition> range;
  // source to maple variable alias
  MapleMap<GStrIdx, MIRAliasVars> aliasVarMap{module->GetMPAllocator().Adapter()};
  MapleVector<MIRScope *> subScopes{module->GetMPAllocator().Adapter()};

  MIRScope(MIRModule *mod) : module(mod), level(0) {}
  MIRScope(MIRModule *mod, unsigned l) : module(mod), level(l) {}
  ~MIRScope() {}

  bool NeedEmitAliasInfo() const {
    return aliasVarMap.size() != 0 || subScopes.size() != 0;
  }

  bool IsSubScope(MIRScope *s);
  bool HasJoinScope(MIRScope *s1, MIRScope *s2);

  SrcPosition GetRangeLow() {
    return range.first;
  }

  SrcPosition GetRangeHigh() {
    return range.second;
  }

  void SetRange(SrcPosition low, SrcPosition high) {
    ASSERT(low.IsBfOrEq(high), "wrong order of low and high");
    range.first = low;
    range.second = high;
  }

  void SetAliasVarMap(GStrIdx idx, const MIRAliasVars &vars) {
    aliasVarMap[idx] = vars;
  }

  void IncLevel();
  bool AddScope(MIRScope *scope);
  void Dump(int32 indent) const;
  void D() const;
};

}  // namespace maple
#endif  // MAPLE_IR_INCLUDE_MIR_SCOPE_H
