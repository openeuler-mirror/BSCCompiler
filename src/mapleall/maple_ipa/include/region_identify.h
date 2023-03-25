/*
 * Copyright (c) [2022] Huawei Technologies Co.,Ltd.All rights reserved.
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
#ifndef MAPLE_IPA_INCLUDE_REGION_IDENTIFY_H
#define MAPLE_IPA_INCLUDE_REGION_IDENTIFY_H

#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include "cfg_primitive_types.h"
#include "ipa_collect.h"
#include "mir_nodes.h"
#include "opcodes.h"
#include "ptr_list_ref.h"
#include "stmt_identify.h"
#include "suffix_array.h"

namespace maple {
constexpr size_t kGroupSizeLimit = 2;
using GroupId = size_t;
using StmtIterator = PtrListRefIterator<StmtNode>;
using SymbolRegPair = std::pair<StIdx, PregIdx>;
using RegionReturnVector = std::vector<SymbolRegPair>;

class RegionCandidate {
 public:
  RegionCandidate(StmtInfoId startId, StmtInfoId endId, StmtInfo *start, StmtInfo *end, MIRFunction* function)
      : startId(startId), endId(endId), start(start), end(end), function(function), length(endId - startId + 1) {}
  virtual ~RegionCandidate() = default;
  void CollectRegionInputAndOutput(StmtInfo &stmtInfo, CollectIpaInfo &ipaInfo);
  const bool HasDefinitionOutofRegion(DefUsePositions &defUse) const;
  bool HasJumpOutOfRegion(StmtInfo &stmtInfo, bool isStart);
  bool IsLegal(CollectIpaInfo &ipaInfo);

  static SymbolRegPair GetSymbolRegPair(BaseNode &node) {
    switch (node.GetOpCode()) {
      case OP_regread: {
        return SymbolRegPair(StIdx(0), static_cast<RegreadNode&>(node).GetRegIdx());
      }
      case OP_regassign: {
        return SymbolRegPair(StIdx(0), static_cast<RegassignNode&>(node).GetRegIdx());
      }
      case OP_addrof:
      case OP_dread: {
        return SymbolRegPair(static_cast<DreadNode&>(node).GetStIdx(), PregIdx(0));
      }
      case OP_dassign: {
        return SymbolRegPair(static_cast<DassignNode&>(node).GetStIdx(), PregIdx(0));
      }
      case OP_asm:
      case OP_callassigned:
      case OP_icallassigned:
      case OP_icallprotoassigned:
      case OP_intrinsiccallassigned: {
        auto *callReturnVector = node.GetCallReturnVector();
        auto &pair = callReturnVector->front();
        return callReturnVector->empty() ? SymbolRegPair(0, 0) : SymbolRegPair(pair.first, pair.second.GetPregIdx());
      }
      default: {
        CHECK_FATAL(false, "unexpected op code");
        return SymbolRegPair(StIdx(0), PregIdx(0));
      }
    }
  }

  StmtInfo *GetStart() {
    return start;
  }

  StmtInfo *GetEnd() {
    return end;
  }

  MIRFunction *GetFunction() {
    return function;
  }

  void Dump() {
    LogInfo::MapleLogger() << "range: " << startId << "->" << endId << "\n";
    function->SetCurrentFunctionToThis();
    for (auto &pair : regionOutputs) {
      if (pair.second != 0) {
        LogInfo::MapleLogger() << "output: reg %" << pair.second << "\n";
      } else {
        auto symTableItem = function->GetSymbolTabItem(pair.first.Idx());
        ASSERT(symTableItem != nullptr, "nullptr check");
        LogInfo::MapleLogger() << "output: var %" << symTableItem->GetName() << "\n";
      }
    }
    auto *currStmt = start->GetStmtNode();
    auto endPosition = end->GetStmtNode()->GetStmtInfoId();
    while (currStmt != nullptr && (currStmt->GetOpCode() == OP_label || currStmt->GetStmtInfoId() <= endPosition)) {
      currStmt->Dump();
      currStmt = currStmt->GetNext();
    }
  }

  void SetGroupId(GroupId id) {
    groupId = id;
  }

  GroupId GetGroupId() {
    return groupId;
  }

  StmtInfoId GetStartId() {
    return startId;
  }

  StmtInfoId GetEndId() {
    return endId;
  }

  size_t GetLength() {
    return length;
  }

  std::set<SymbolRegPair> &GetRegionInPuts() {
    return regionInputs;
  }

  std::set<SymbolRegPair> &GetRegionOutPuts() {
    return regionOutputs;
  }

  std::unordered_set<BaseNode*> &GetStmtJumpToEnd() {
    return stmtJumpToEnd;
  }

  PrimType GetOutputPrimType() {
    if (regionOutputs.empty()) {
      return PTY_void;
    }
    auto stIdx = regionOutputs.begin()->first;
    auto pregIdx = regionOutputs.begin()->second;
    if (stIdx.Idx() != 0) {
      auto symTableItem = function->GetSymbolTabItem(stIdx.Idx());
      ASSERT(symTableItem != nullptr, "nullptr check");
      return symTableItem->GetType()->GetPrimType();
    } else {
      return function->GetPregItem(pregIdx)->GetPrimType();
    }
  }

  bool IsOverlapWith(RegionCandidate &rhs) {
    return (startId >= rhs.GetStartId() && startId <= rhs.GetEndId()) ||
        (rhs.GetStartId() >= startId && rhs.GetStartId() <= endId);
  }

  bool IsRegionInput(BaseNode &node) {
    auto pair = GetSymbolRegPair(node);
    return regionInputs.find(pair) != regionInputs.end();
  }

  template<typename Functor>
  void TraverseRegion(Functor processor) {
    auto &stmtList = start->GetCurrBlock()->GetStmtNodes();
    auto begin = StmtNodes::iterator(start->GetStmtNode());
    for (auto it = begin; it != stmtList.end() && it->GetStmtInfoId() <= endId ; ++it) {
      processor(it);
    }
  }

 private:
  StmtInfoId startId;
  StmtInfoId endId;
  StmtInfo* start;
  StmtInfo* end;
  MIRFunction *function;
  size_t length;
  GroupId groupId = kInvalidIndex;
  std::set<SymbolRegPair> regionOutputs;
  std::set<SymbolRegPair> regionInputs;
  std::unordered_set<BaseNode*> stmtJumpToEnd;
};

class RegionGroup {
 public:
  RegionGroup() = default;

  virtual ~RegionGroup() = default;

  void Dump() {
    LogInfo::MapleLogger() << "region group: " << groupId << "\n";
    LogInfo::MapleLogger() << "region number: " << groups.size() << "\n";
    if (groups.size() < kGroupSizeLimit) {
      return;
    }
    for (auto &region : groups) {
      LogInfo::MapleLogger() << "region candidates: " << region.GetStart()->GetStmtNode()->GetStmtInfoId() << "->"
          << region.GetEnd()->GetStmtNode()->GetStmtInfoId() << "\n";
      LogInfo::MapleLogger() << "origin range: " << region.GetStartId() << "->"
          << region.GetEndId() << "\n";
      region.Dump();
    }
  }

  std::vector<RegionCandidate> &GetGroups() {
    return groups;
  }

  uint32 GetGroupId() {
    return groupId;
  }

  void SetGroupId(GroupId id) {
    groupId = id;
  }

  int64 GetCost() {
    return cost;
  }

  void SetCost(int64 newCost) {
    cost = newCost;
  }

 private:
  std::vector<RegionCandidate> groups;
  uint32 groupId = 0;
  int64 cost = 0;
};

class RegionIdentify {
 public:
  explicit RegionIdentify(CollectIpaInfo *ipaInfo) : ipaInfo(ipaInfo) {}
  virtual ~RegionIdentify() = default;
  void RegionInit();
  std::vector<RegionGroup> &GetRegionGroups() {
    return regionGroups;
  }

 private:
  void CreateRegionCandidates(SuffixArray &sa);
  void CreateRegionGroups(std::vector<RegionCandidate> &regions);
  void ClearSrcMappings();
  bool CheckOverlapAmongGroupRegions(RegionGroup &group, RegionCandidate &region);
  bool IsRegionLegal(uint startPosition, uint endPosition);
  bool CheckCompatibilifyAmongRegionComponents(BaseNode &lhs, BaseNode& rhs);
  bool CheckCompatibilifyBetweenSrcs(BaseNode &lhs, BaseNode& rhs);
  bool CompareSymbolStructure(const StIdx leftIdx, const StIdx rightIdx);
  bool HasSameStructure(RegionCandidate &lhs, RegionCandidate &rhs);
  StmtInfo *GetNearestNonnullStmtInfo(StmtInfoId index, bool forward);

  CollectIpaInfo *ipaInfo;
  std::vector<RegionGroup> regionGroups;
  std::unordered_map<StIdx, StIdx> symMap;
  std::unordered_map<PregIdx, PregIdx> leftRegMap;
  std::unordered_map<PregIdx, PregIdx> rightRegMap;
  std::unordered_map<uint32, uint32> leftStrMap;
  std::unordered_map<uint32, uint32> rightStrMap;
  std::unordered_map<const MIRConst *, const MIRConst *> leftConstMap;
  std::unordered_map<const MIRConst *, const MIRConst *> rightConstMap;
};
}
#endif  // MAPLE_IPA_INCLUDE_REGION_IDENTIFY_H
