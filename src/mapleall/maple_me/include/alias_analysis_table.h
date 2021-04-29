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
#ifndef MAPLE_ME_INCLUDE_ALIAS_ANALYSIS_TABLE_H
#define MAPLE_ME_INCLUDE_ALIAS_ANALYSIS_TABLE_H
#include "orig_symbol.h"
#include "ssa_tab.h"
#include "class_hierarchy.h"

namespace maple {
class AliasAnalysisTable {
 public:
  AliasAnalysisTable(SSATab &ssaTable, const MapleAllocator &allocator, KlassHierarchy &kh)
      : ssaTab(ssaTable),
        alloc(allocator),
        prevLevelNode(alloc.Adapter()),
        nextLevelNodes(alloc.Adapter()),
        memPool(ssaTab.GetMempool()),
        klassHierarchy(kh) {}

  ~AliasAnalysisTable() = default;

  OriginalSt *GetPrevLevelNode(const OriginalSt &ost);
  MapleVector<OriginalSt*> *GetNextLevelNodes(const OriginalSt &ost);
  OriginalSt *FindOrCreateAddrofSymbolOriginalSt(OriginalSt &ost);
  OriginalSt *FindOrCreateExtraLevOriginalSt(OriginalSt &ost, TyIdx ptyidx, FieldID fld);

 private:
  OriginalSt *FindOrCreateExtraLevSymOrRegOriginalSt(OriginalSt &ost, TyIdx tyIdx, FieldID fld);
  OriginalSt *FindExtraLevOriginalSt(const MapleVector<OriginalSt*> &nextLevelOsts, FieldID fld);
  SSATab &ssaTab;
  MapleAllocator alloc;
 public:
  MapleMap<OStIdx, OriginalSt*> prevLevelNode;                 // index is the OStIdx
 private:
  MapleMap<OStIdx, MapleVector<OriginalSt*>*> nextLevelNodes;  // index is the OStIdx
  MemPool *memPool;
  KlassHierarchy &klassHierarchy;
};
}  // namespace maple
#endif  // MAPLE_ME_INCLUDE_ALIAS_ANALYSIS_TABLE_H
