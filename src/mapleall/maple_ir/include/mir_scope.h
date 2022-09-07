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
#include <tuple>
#include "mir_module.h"
#include "mir_type.h"
#include "src_position.h"

namespace maple {
// mapping src variable to mpl variables to display debug info
enum AliasTypeKind : uint8 {
  kATKType,
  kATKString,
  kATKEnum,
};

struct MIRAliasVars {
  GStrIdx mplStrIdx;      // maple varialbe name
  AliasTypeKind atk;
  unsigned index;
  bool isLocal;
  GStrIdx sigStrIdx;
  TypeAttrs attrs;
};

class MIRAlias {
 public:
  explicit MIRAlias(MIRModule *mod) : module(mod) {}
  ~MIRAlias() = default;

  bool IsEmpty() const {
    return aliasVarMap.size() == 0;
  }

  void SetAliasVarMap(GStrIdx idx, const MIRAliasVars &vars) {
    aliasVarMap[idx] = vars;
  }

  void AddAliasVarMap(GStrIdx idx, const MIRAliasVars &vars) {
    /* allow same idx, save last aliasVars */
    aliasVarMap[idx] = vars;
  }

  MapleMap<GStrIdx, MIRAliasVars> &GetAliasVarMap() {
    return aliasVarMap;
  }

  void Dump(int32 indent, bool isLocal = true) const;

 private:
  MIRModule *module;
  // source to maple variable alias
  MapleMap<GStrIdx, MIRAliasVars> aliasVarMap { module->GetMPAllocator().Adapter() };
};

class MIRTypeAliasTable {
 public:
  explicit MIRTypeAliasTable(MIRModule *mod) : module(mod) {}
  virtual ~MIRTypeAliasTable() = default;

  bool IsEmpty() const {
    return typeAliasMap.size() == 0;
  }

  const MapleMap<GStrIdx, TyIdx> &GetTypeAliasMap() const {
    return typeAliasMap;
  }

  TyIdx GetTyIdxFromMap(GStrIdx idx) const {
    auto it = typeAliasMap.find(idx);
    if (it == typeAliasMap.cend()) {
      return TyIdx(0);
    }
    return it->second;
  }

  void SetTypeAliasMap(GStrIdx gStrIdx, TyIdx tyIdx) {
    typeAliasMap[gStrIdx] = tyIdx;
  }

  void Dump(int32 indent) const;

 private:
  MIRModule *module;
  MapleMap<GStrIdx, TyIdx> typeAliasMap { module->GetMPAllocator().Adapter() };
};

class MIRScope {
 public:
  explicit MIRScope(MIRModule *mod, MIRFunction *f = nullptr);
  ~MIRScope() = default;

  bool IsEmpty() const {
    return (alias == nullptr || alias->IsEmpty()) &&
           (typeAlias == nullptr || typeAlias->IsEmpty()) &&
           subScopes.size() == 0;
  }

  bool IsSubScope(const MIRScope *scp) const;
  bool HasJoinScope(const MIRScope *scp1, const MIRScope *scp2) const;
  bool HasSameRange(const MIRScope *s1, const MIRScope *s2) const;

  unsigned GetId() const {
    return id;
  }

  void SetId(unsigned i) {
    id = i;
  }

  const SrcPosition &GetRangeLow() const {
    return range.first;
  }

  const SrcPosition &GetRangeHigh() const {
    return range.second;
  }

  void SetRange(SrcPosition low, SrcPosition high) {
    // The two positions that were changed by the #line directive may be not in the same file.
    ASSERT(!low.IsSameFile(high) || low.IsBfOrEq(high), "wrong order of low and high");
    range.first = low;
    range.second = high;
  }

  void SetAliasVarMap(GStrIdx idx, const MIRAliasVars &vars) {
    alias->SetAliasVarMap(idx, vars);
  }

  void AddAliasVarMap(GStrIdx idx, const MIRAliasVars &vars) {
    /* allow same idx, save last aliasVars */
    alias->AddAliasVarMap(idx, vars);
  }

  MapleMap<GStrIdx, MIRAliasVars> &GetAliasVarMap() {
    return alias->GetAliasVarMap();
  }

  MapleVector<MIRScope*> &GetSubScopes() {
    return subScopes;
  }

  void AddTuple(SrcPosition pos, SrcPosition posB, SrcPosition posE) {
    if (pos.LineNum() == 0 || posB.LineNum() == 0 || posE.LineNum() == 0) {
      return;
    }
    std::tuple<SrcPosition, SrcPosition, SrcPosition> srcPos(pos, posB, posE);
    blkSrcPos.push_back(srcPos);
  }

  SrcPosition GetScopeEndPos(const SrcPosition &pos);
  bool AddScope(MIRScope *scope);

  void SetTypeAlias(GStrIdx gStrIdx, TyIdx tyIdx) {
    if (typeAlias == nullptr) {
      typeAlias = module->GetMemPool()->New<MIRTypeAliasTable>(module);
    }
    typeAlias->SetTypeAliasMap(gStrIdx, tyIdx);
  }

  const MIRTypeAliasTable *GetTypeAliasTable() const {
    return typeAlias;
  }

  void Dump(int32 indent, bool isLocal = true) const;
  void Dump() const;

 private:
  MIRModule *module;
  MIRFunction *func;
  unsigned id;
  std::pair<SrcPosition, SrcPosition> range;
  MIRAlias *alias = nullptr;
  MIRTypeAliasTable *typeAlias = nullptr;
  // subscopes' range should be disjoint
  MapleVector<MIRScope*> subScopes { module->GetMPAllocator().Adapter() };
  MapleVector<std::tuple<SrcPosition, SrcPosition, SrcPosition>> blkSrcPos { module->GetMPAllocator().Adapter() };
};
}  // namespace maple
#endif  // MAPLE_IR_INCLUDE_MIR_SCOPE_H
