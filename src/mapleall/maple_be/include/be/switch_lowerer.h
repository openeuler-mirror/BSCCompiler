/*
 * Copyright (c) [2020] Huawei Technologies Co.,Ltd.All rights reserved.
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
#ifndef MAPLEBE_INCLUDE_BE_SWITCH_LOWERER_H
#define MAPLEBE_INCLUDE_BE_SWITCH_LOWERER_H
#include "mir_nodes.h"
#include "mir_module.h"
#include "lower.h"

namespace maplebe {
using namespace maple;
class BELowerer;

class SwitchLowerer {
 public:
  SwitchLowerer(maple::MIRModule &mod, maple::SwitchNode &stmt,
                CGLowerer *lower, maple::MapleAllocator &allocator)
      : mirModule(mod),
        stmt(&stmt),
        cgLowerer(lower),
        switchItems(allocator.Adapter()),
        ownAllocator(&allocator) {}

  SwitchLowerer(maple::MIRModule &mod, maple::SwitchNode &stmt,
                maple::MapleAllocator &allocator)
      : mirModule(mod),
        stmt(&stmt),
        switchItems(allocator.Adapter()),
        ownAllocator(&allocator) {}

  ~SwitchLowerer() = default;

  maple::BlockNode *LowerSwitch(LabelIdx newLabelIdx = 0);

 private:
  using Cluster = std::pair<maple::int32, maple::int32>;
  using SwitchItem = std::pair<maple::int32, maple::int32>;

  maple::MIRModule &mirModule;
  maple::SwitchNode *stmt;
  CGLowerer *cgLowerer;
  /*
   * the original switch table is sorted and then each dense (in terms of the
   * case tags) region is condensed into 1 switch item; in the switchItems
   * table, each item either corresponds to an original entry in the original
   * switch table (pair's second is 0), or to a dense region (pair's second
   * gives the upper limit of the dense range)
   */
  maple::MapleVector<SwitchItem> switchItems;  /* uint32 is index in switchTable */
  maple::MapleAllocator *ownAllocator;
  const maple::int32 kClusterSwitchCutoff = 5;
  const float kClusterSwitchDensityHigh = 0.4;
  const float kClusterSwitchDensityLow = 0.2;
  const maple::int32 kMaxRangeGotoTableSize = 127;
  bool jumpToDefaultBlockGenerated = false;

  void FindClusters(MapleVector<Cluster> &clusters) const;
  void InitSwitchItems(MapleVector<Cluster> &clusters);
  maple::RangeGotoNode *BuildRangeGotoNode(int32 startIdx, int32 endIdx, LabelIdx newLabelIdx);
  maple::CompareNode *BuildCmpNode(Opcode opCode, uint32 idx);
  maple::GotoNode *BuildGotoNode(int32 idx);
  maple::CondGotoNode *BuildCondGotoNode(int32 idx, Opcode opCode, BaseNode &cond);
  maple::BlockNode *BuildCodeForSwitchItems(int32 start, int32 end, bool lowBlockNodeChecked,
                                            bool highBlockNodeChecked, LabelIdx newLabelIdx = 0);
};
}  /* namespace maplebe */

#endif  /* MAPLEBE_INCLUDE_BE_SWITCH_LOWERER_H */
