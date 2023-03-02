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
#ifndef MPL2MPL_INCLUDE_OUTLINE_H
#define MPL2MPL_INCLUDE_OUTLINE_H
#include "mir_builder.h"
#include "ipa_collect.h"
#include "region_identify.h"

namespace maple {
class OutlineCandidate {
 public:
  explicit OutlineCandidate(RegionCandidate *candidate) : regionCandidate(candidate) {}
  virtual ~OutlineCandidate() {
      regionCandidate = nullptr;
  }

  size_t InsertIntoParameterList(BaseNode &expr);

  RegionCandidate *GetRegionCandidate() {
    return regionCandidate;
  }

  std::vector<BaseNode*> &GetParameters() {
    return parameters;
  }

  std::vector<BaseNode*> &GetRegionValueNumberToSrcExprMap() {
    return valueNumberToConstexpr;
  }

  std::unordered_map<BaseNode*, size_t> &GetExprToParameterIndexMap() {
    return exprToParameterIndex;
  }

  std::unordered_map<StIdx, size_t> &GetStIdxToParameterIndexMap() {
    return stIdxToParameterIndex;
  }

  std::unordered_map<BaseNode*, StmtNode*> &GetReplacedStmtMap() {
    return replacedStmtMap;
  }

  void Dump() {
    regionCandidate->GetFunction()->SetCurrentFunctionToThis();
    for (auto *parameter : parameters) {
      LogInfo::MapleLogger() << "input: ";
      parameter->Dump();
    }
    regionCandidate->Dump();
  }

  BaseNode *GetReturnExpr() {
    return returnValue;
  }

  void CreateReturnExpr(const SymbolRegPair &outputPair) {
    if (returnValue != nullptr) {
      return;
    }
    auto *function = regionCandidate->GetFunction();
    function->GetModule()->SetCurFunction(function);
    auto *builder = function->GetModule()->GetMIRBuilder();
    if (outputPair.second != 0) {
      auto pregIdx = outputPair.second;
      returnValue = builder->CreateExprRegread(function->GetPregItem(pregIdx)->GetPrimType(), pregIdx);
    } else {
      returnValue = builder->CreateExprDread(*function->GetSymbolTabItem(outputPair.first.Idx()));
    }
  }

  void SetCleared() {
    cleared = true;
  }

  bool IsCleared() const {
    return cleared;
  }
 private:
  bool cleared = false;
  RegionCandidate *regionCandidate;
  BaseNode *returnValue = nullptr;
  std::vector<BaseNode*> valueNumberToConstexpr;
  std::vector<BaseNode*> parameters;
  std::unordered_map<BaseNode*, size_t> exprToParameterIndex;
  std::unordered_map<StIdx, size_t> stIdxToParameterIndex;
  std::unordered_map<BaseNode*, StmtNode*> replacedStmtMap;
};

class OutlineGroup {
 public:
  explicit OutlineGroup(RegionGroup &group) : groupId(group.GetGroupId()) {
    for (auto &region : group.GetGroups()) {
      regionGroup.emplace_back(&region);
    }
  }
  virtual ~OutlineGroup() = default;

  void PrepareParameterLists() {
    CollectOutlineInfo();
    PrepareExtraParameterLists();
  }

  void ReplaceOutlineRegions() {
    ReplaceOutlineCandidateWithCall();
    CompleteOutlineFunction();
    EraseOutlineRegion();
  }

  std::vector<OutlineCandidate> &GetOutlineRegions() {
    return regionGroup;
  }

  void SetOutlineFunction(MIRFunction *func) {
    outlineFunc = func;
  }

  MIRFunction *GetOutlineFunction() {
    return outlineFunc;
  }

  size_t GetParameterSize() {
    return regionGroup.front().GetParameters().size();
  }

  void Dump() {
    LogInfo::MapleLogger() << "region group: " << groupId << "\n";
    LogInfo::MapleLogger() << "region number: " << regionGroup.size() << "\n";
    for (auto &region : regionGroup) {
      region.Dump();
    }
  }

 private:
  void CollectOutlineInfo();
  void PrepareExtraParameterLists();
  void ReplaceOutlineCandidateWithCall();
  void CompleteOutlineFunction();
  void EraseOutlineRegion();

  std::vector<OutlineCandidate> regionGroup;
  std::vector<BaseNode*> parameterList;
  std::vector<uint32> extraParameterValueNumber;
  MIRFunction *outlineFunc = nullptr;
  GroupId groupId;
};

class OutLine {
 public:
  OutLine(CollectIpaInfo *ipaInfo, MIRModule *module, MemPool *memPool) :
    ipaInfo(ipaInfo), module(module), memPool(memPool) {}
  virtual ~OutLine() {
      ipaInfo = nullptr;
  }
  void Run();
 private:
  MIRFunction *CreateNewOutlineFunction(PrimType returnType);
  void PruneCandidateRegionsOverlapWithOutlinedRegion(RegionGroup &group);
  void DoOutline(OutlineGroup &outlineGroup);
  CollectIpaInfo *ipaInfo;
  MIRModule *module;
  MemPool *memPool;
  uint32 newFuncIndex = 0;
  std::vector<RegionCandidate*> outlinedRegion;
};
MAPLE_MODULE_PHASE_DECLARE(M2MOutline)
}  // namespace maple
#endif  // MPL2MPL_INCLUDE_OUTLINE_H
