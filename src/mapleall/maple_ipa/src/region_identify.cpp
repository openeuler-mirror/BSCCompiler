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
#include "region_identify.h"
#include <sys/types.h>
#include <vector>
#include "ipa_collect.h"
#include "opcodes.h"
#include "stmt_identify.h"

namespace {
  constexpr size_t kRegionInputLimit = 8;
  constexpr size_t kRegionOutputLimit = 1;
  constexpr size_t kRegionMinInvalidStmtNumber = 4;
}

namespace maple {
const bool RegionCandidate::HasDefinitionOutofRegion(DefUsePositions &defUse) const {
  for (auto defPos : defUse.definePositions) {
    if (defPos > endId || defPos < startId) {
      return true;
    }
  }
  return false;
}

void RegionCandidate::CollectRegionInputAndOutput(StmtInfo &stmtInfo, CollectIpaInfo &ipaInfo) {
  for (auto defUsePositions : stmtInfo.GetRegDefUse()) {
    auto &regIdx = defUsePositions.first;
    auto src = SymbolRegPair(StIdx(0), defUsePositions.first);
    for (auto usePos : defUsePositions.second.usePositions) {
      if (usePos >= startId && usePos <= endId) {
        continue;
      }
      (void)regionOutputs.insert(src);
      auto &useStmtInfo = ipaInfo.GetStmtInfo()[usePos];
      if (HasDefinitionOutofRegion(useStmtInfo.GetRegDefUse()[regIdx])) {
        (void)regionInputs.insert(src);
      }
    }
    if (HasDefinitionOutofRegion(defUsePositions.second)) {
      (void)regionInputs.insert(src);
    }
  }
  for (auto defUsePositions : stmtInfo.GetSymbolDefUse()) {
    auto &stIdx = defUsePositions.first;
    auto src = SymbolRegPair(stIdx, PregIdx(0));
    for (auto usePos : defUsePositions.second.usePositions) {
      if (usePos >= startId && usePos <= endId) {
        continue;
      }
      (void)regionOutputs.insert(src);
      auto &useStmtInfo = ipaInfo.GetStmtInfo()[usePos];
      if (HasDefinitionOutofRegion(useStmtInfo.GetSymbolDefUse()[stIdx])) {
        (void)regionInputs.insert(src);
      }
    }
    if (HasDefinitionOutofRegion(defUsePositions.second)) {
      (void)regionInputs.insert(src);
    }
  }
}

bool RegionCandidate::HasJumpOutOfRegion(StmtInfo &stmtInfo, bool isStart) {
  for (auto location : stmtInfo.GetLocationsJumpTo()) {
    if (location == endId + 1) {
      (void)stmtJumpToEnd.insert(stmtInfo.GetStmtNode());
    }
    if (location > endId + 1 || location < startId) {
      return true;
    }
  }
  if (isStart) {
    return false;
  }
  for (auto location : stmtInfo.GetLocationsJumpFrom()) {
    if (location < startId || location > endId) {
      return true;
    }
  }
  return false;
}

bool RegionCandidate::IsLegal(CollectIpaInfo &ipaInfo) {
  std::unordered_set<Opcode> stmtTypes;
  uint32 validStmts = 0;
  bool hasCall = false;
  for (auto i = startId; i <= endId; ++i) {
    auto &stmtInfo = ipaInfo.GetStmtInfo()[i];
    auto currOp = stmtInfo.GetHashCandidateAt(0);
    if (!(currOp == OP_comment) && !(currOp == OP_label) && !(currOp == OP_eval)) {
      if (currOp == OP_dassign || currOp == OP_iassign || currOp == OP_regassign) {
        (void)stmtTypes.insert(OP_dassign);
      } else {
        (void)stmtTypes.insert(static_cast<Opcode>(currOp));
      }
      validStmts++;
    }
    if (HasJumpOutOfRegion(stmtInfo, i == startId)) {
      return false;
    }
    if (!stmtInfo.IsValid()) {
      return false;
    }
    if (stmtInfo.IsCall()) {
      hasCall = true;
    }
    CollectRegionInputAndOutput(stmtInfo, ipaInfo);
    if (regionInputs.size() > kRegionInputLimit || regionOutputs.size() > kRegionOutputLimit) {
      return false;
    }
  }
  return stmtTypes.size() > 1 && (validStmts > kRegionMinInvalidStmtNumber && hasCall);
}

void RegionIdentify::RegionInit() {
  std::vector<size_t> integerString;
  auto &rawString = ipaInfo->GetIntegerString();
  integerString.assign(rawString.begin(), rawString.end());
  if (integerString.empty()) {
    return;
  }
  (void)integerString.emplace_back(0);
  (void)ipaInfo->GetStmtInfo().emplace_back(StmtInfo(nullptr, kInvalidPuIdx));
  SuffixArray sa(integerString, integerString.size(), ipaInfo->GetCurrNewStmtIndex());
  sa.Run(true);
  CreateRegionCandidates(sa);
}

void RegionIdentify::CreateRegionCandidates(SuffixArray &sa) {
  for (auto *subStrings : sa.GetRepeatedSubStrings()) {
    std::vector<RegionCandidate> candidates;
    for (auto occurrence : subStrings->GetOccurrences()) {
      auto startPosition = occurrence.first;
      auto endPosition = occurrence.second;
      auto *startStmtInfo = GetNearestNonnullStmtInfo(startPosition, true);
      auto *endStmtInfo = GetNearestNonnullStmtInfo(endPosition, false);
      auto *function = GlobalTables::GetFunctionTable().GetFunctionFromPuidx(startStmtInfo->GetPuIdx());
      auto currRegion = RegionCandidate(startPosition, endPosition, startStmtInfo, endStmtInfo, function);
      if (currRegion.IsLegal(*ipaInfo)) {
        (void)candidates.emplace_back(currRegion);
      }
    }
    delete subStrings;
    subStrings = nullptr;
    if (!(candidates.size() > 1)) {
      continue;
    }
    CreateRegionGroups(candidates);
  }
}

void RegionIdentify::CreateRegionGroups(std::vector<RegionCandidate> &regions) {
  for (size_t i = 0; i < regions.size(); ++i) {
    auto &currRegion = regions[i];
    if (currRegion.GetGroupId() != kInvalidIndex) {
      continue;
    }
    auto currGroup = RegionGroup();
    GroupId newGroupId = regionGroups.size();
    currGroup.SetGroupId(newGroupId);
    currRegion.SetGroupId(newGroupId);
    (void)currGroup.GetGroups().emplace_back(currRegion);
    for (size_t j = i + 1; j < regions.size(); ++j) {
      auto &nextRegion = regions[j];
      if (CheckOverlapAmongGroupRegions(currGroup, nextRegion)) {
        continue;
      }
      if (HasSameStructure(currRegion, nextRegion)) {
        nextRegion.SetGroupId(newGroupId);
        (void)currGroup.GetGroups().emplace_back(nextRegion);
      }
      ClearSrcMappings();
    }
    if (currGroup.GetGroups().size() < kGroupSizeLimit) {
      continue;
    }
    (void)regionGroups.emplace_back(currGroup);
  }
}

void RegionIdentify::ClearSrcMappings() {
  symMap.clear();
  leftRegMap.clear();
  rightRegMap.clear();
  leftConstMap.clear();
  rightConstMap.clear();
}

StmtInfo *RegionIdentify::GetNearestNonnullStmtInfo(StmtInfoId index, bool forward) {
  auto &stmtInfoVector = ipaInfo->GetStmtInfo();
  while (stmtInfoVector[index].GetStmtNode() == nullptr) {
    index = forward ? index + 1 : index - 1;
  }
  return &stmtInfoVector[index];
}

bool RegionIdentify::CheckOverlapAmongGroupRegions(RegionGroup &group, RegionCandidate &region) {
  for (auto &groupedRegion : group.GetGroups()) {
    if (region.IsOverlapWith(groupedRegion)) {
      return true;
    }
  }
  return false;
}

bool RegionIdentify::CheckCompatibilifyBetweenSrcs(BaseNode &lhs, BaseNode &rhs) {
  switch (lhs.GetOpCode()) {
    case OP_dassign: {
      auto leftStIdx = static_cast<DassignNode&>(lhs).GetStIdx();
      auto rightStIdx = static_cast<DassignNode&>(rhs).GetStIdx();
      if (!CompareSymbolStructure(leftStIdx, rightStIdx)) {
        return false;
      }
      break;
    }
    case OP_regassign: {
      auto leftRegIdx = static_cast<RegassignNode&>(lhs).GetRegIdx();
      auto rightRegIdx = static_cast<RegassignNode&>(rhs).GetRegIdx();
      if (!CompareRegStructure(leftRegIdx, rightRegIdx)) {
        return false;
      }
      break;
    }
    case OP_addrof:
    case OP_dread: {
      auto leftStIdx = static_cast<DreadNode&>(lhs).GetStIdx();
      auto rightStIdx = static_cast<DreadNode&>(rhs).GetStIdx();
      return CompareSymbolStructure(leftStIdx, rightStIdx);
    }
    case OP_regread: {
      auto leftRegIdx = static_cast<RegreadNode&>(lhs).GetRegIdx();
      auto rightRegIdx = static_cast<RegreadNode&>(rhs).GetRegIdx();
      return CompareRegStructure(leftRegIdx, rightRegIdx);
    }
    case OP_constval: {
      auto *leftVal = static_cast<ConstvalNode&>(lhs).GetConstVal();
      auto *rightVal = static_cast<ConstvalNode&>(rhs).GetConstVal();
      return CompareConstStructure(leftVal, rightVal);
    }
    default: {
      break;
    }
  }
  return true;
}

bool RegionIdentify::CheckCompatibilifyAmongRegionComponents(BaseNode &lhs, BaseNode &rhs) {
  if (lhs.GetOpCode() != rhs.GetOpCode()) {
    return false;
  }

  if (lhs.GetOpCode() == OP_block) {
    auto *leftStmt = static_cast<BlockNode&>(lhs).GetFirst();
    auto *rightStmt = static_cast<BlockNode&>(rhs).GetFirst();
    while (leftStmt != nullptr) {
      if (!CheckCompatibilifyAmongRegionComponents(*leftStmt, *rightStmt)) {
        return false;
      }
      leftStmt = leftStmt->GetNext();
      rightStmt = rightStmt->GetNext();
    }
    return true;
  }

  if (!CheckCompatibilifyBetweenSrcs(lhs, rhs)) {
    return false;
  }

  for (size_t i = 0; i < lhs.GetNumOpnds(); ++i) {
    if (!CheckCompatibilifyAmongRegionComponents(*lhs.Opnd(i), *rhs.Opnd(i))) {
      return false;
    }
  }
  return true;
}

bool RegionIdentify::CompareConstStructure(const MIRConst *leftConst, const MIRConst *rightConst) {
  if (leftConstMap.find(leftConst) != leftConstMap.end()) {
    if (leftConstMap[leftConst] != rightConst) {
      return false;
    }
  } else {
    leftConstMap[leftConst] = rightConst;
  }
  if (rightConstMap.find(rightConst) != rightConstMap.end()) {
    if (rightConstMap[rightConst] != leftConst) {
      return false;
    }
  } else {
    rightConstMap[rightConst] = leftConst;
  }
  return true;
}

bool RegionIdentify::CompareRegStructure(const PregIdx leftIdx, const PregIdx rightIdx) {
  if (leftRegMap.find(leftIdx) != leftRegMap.end()) {
    if (leftRegMap[leftIdx] != rightIdx) {
      return false;
    }
  } else {
    leftRegMap[leftIdx] = rightIdx;
  }
  if (rightRegMap.find(rightIdx) != rightRegMap.end()) {
    if (rightRegMap[rightIdx] != leftIdx) {
      return false;
    }
  } else {
    rightRegMap[rightIdx] = leftIdx;
  }
  return true;
}

bool RegionIdentify::CompareSymbolStructure(const StIdx leftIdx, const StIdx rightIdx) {
  if (leftIdx.IsGlobal() != rightIdx.IsGlobal()) {
    return false;
  }
  if (leftIdx.IsGlobal() && rightIdx.IsGlobal()) {
    return leftIdx == rightIdx;
  }
  if (symMap.find(leftIdx) != symMap.end()) {
    if (symMap[leftIdx] != rightIdx) {
      return false;
    }
  } else {
    symMap[leftIdx] = rightIdx;
  }
  if (symMap.find(rightIdx) != symMap.end()) {
    if (symMap[rightIdx] != leftIdx) {
      return false;
    }
  } else {
    symMap[rightIdx] = leftIdx;
  }
  return true;
}

bool RegionIdentify::HasSameStructure(RegionCandidate &lhs, RegionCandidate &rhs) {
  if (lhs.GetRegionOutPuts().size() != rhs.GetRegionOutPuts().size()) {
    return false;
  }
  auto *leftStmt = lhs.GetStart()->GetStmtNode();
  auto *rightStmt = rhs.GetStart()->GetStmtNode();
  while (leftStmt && rightStmt && leftStmt->GetStmtInfoId() <= lhs.GetEndId()) {
    if (!CheckCompatibilifyAmongRegionComponents(*leftStmt, *rightStmt)) {
      return false;
    }
    leftStmt = leftStmt->GetNext();
    rightStmt = rightStmt->GetNext();
  }
  return true;
}
}
