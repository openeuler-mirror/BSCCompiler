/*
 * Copyright (c) [2023] Huawei Technologies Co.,Ltd.All rights reserved.
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
#ifndef MAPLE_IR_INCLUDE_MEM_REFERENCE_TABLE_H
#define MAPLE_IR_INCLUDE_MEM_REFERENCE_TABLE_H

#include "mempool_allocator.h"
#include "mir_module.h"
#include "orig_symbol.h"

namespace maple {

using MemDefUseSet = MapleUnorderedSet<OStIdx>;

class MemDefUse {
 public:
  explicit MemDefUse(MapleAllocator &allocator)
      : defSet(allocator.Adapter()),
        useSet(allocator.Adapter()) {}
  ~MemDefUse();

  MemDefUseSet &GetDefSet() {
    return defSet;
  }

  MemDefUseSet &GetUseSet() {
    return useSet;
  }

 private:
  MemDefUseSet defSet;
  MemDefUseSet useSet;
};

using MemDefUsePart = MapleUnorderedMap<BaseNode *, MemDefUse *>;
using OstTable = MapleVector<OriginalSt>;
class MemReferenceTable {
 public:
  MemReferenceTable(MapleAllocator &allocator, MIRFunction &func)
      : allocator(allocator),
        func(func),
        ostTable(allocator.Adapter()),
        memDefUsePart(allocator.Adapter()) {}
  ~MemReferenceTable() {}

  MIRFunction &GetFunction() {
    return func;
  }

  MemDefUsePart &GetMemDefUsePart() {
    return memDefUsePart;
  }

  OstTable &GetOstTable() {
    return ostTable;
  }

  MemDefUse *GetOrCreateMemDefUseFromBaseNode(BaseNode *node) {
    auto iter = memDefUsePart.find(node);
    if (iter != memDefUsePart.end()) {
      return iter->second;
    }
    auto *newDefUse = allocator.New<MemDefUse>(allocator);
    memDefUsePart[node] = newDefUse;
    return newDefUse;
  }

 private:
  MapleAllocator &allocator;
  MIRFunction &func;
  OstTable ostTable;
  MemDefUsePart memDefUsePart;
};
}  // namespace maple
#endif  // MAPLE_IR_INCLUDE_MEM_REFERENCE_TABLE_H
