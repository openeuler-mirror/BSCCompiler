/*
 * Copyright (c) [2021-2022] Huawei Technologies Co.,Ltd.All rights reserved.
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
#ifndef MAPLE_IPA_INCLUDE_COLLECT_IPA_INFO_H
#define MAPLE_IPA_INCLUDE_COLLECT_IPA_INFO_H
#include "mir_nodes.h"
#include "mir_builder.h"
#include "call_graph.h"
#include "me_ir.h"
#include "me_irmap.h"
#include "dominance.h"
#include "class_hierarchy.h"
#include "maple_phase.h"
#include "mir_module.h"
#include "stmt_identify.h"
#include "maple_phase_manager.h"
namespace maple {
union ParamValue {
  bool valueBool;
  int64_t valueInt;
  float valueFloat;
  double valueDouble;
};

enum ValueType {
  kBool,
  kInt,
  kFloat,
  kDouble,
};

class CollectIpaInfo {
 public:
  CollectIpaInfo(MIRModule &mod, MemPool &memPool)
      : module(mod),
        builder(*mod.GetMIRBuilder()),
        curFunc(nullptr),
        allocator(&memPool),
        stmtInfoToIntegerMap(allocator.Adapter()),
        integerString(allocator.Adapter()),
        stmtInfoVector(allocator.Adapter()) {}
  virtual ~CollectIpaInfo() = default;

  void RunOnScc(SCCNode<CGNode> &scc);
  void TraverseStmtInfo(size_t position);
  void UpdateCaleeParaAboutFloat(MeStmt &meStmt, float paramValue, uint32 index, CallerSummary &summary);
  void UpdateCaleeParaAboutDouble(MeStmt &meStmt, double paramValue, uint32 index, CallerSummary &summary);
  void UpdateCaleeParaAboutInt(MeStmt &meStmt, int64_t paramValue, uint32 index, CallerSummary &summary);
  bool IsConstKindValue(MeExpr *expr) const;
  bool CheckImpExprStmt(const MeStmt &meStmt) const;
  bool CollectBrImportantExpression(const MeStmt &meStmt, uint32 &index) const;
  void TransformStmtToIntegerSeries(MeStmt &meStmt);
  DefUsePositions &GetDefUsePositions(OriginalSt &ost, StmtInfoId position);
  void CollectDefUsePosition(ScalarMeExpr &scalar, StmtInfoId position,
      std::unordered_set<ScalarMeExpr*> &cycleCheck);
  void CollectJumpInfo(MeStmt &meStmt);
  void SetLabel(size_t currStmtInfoId, LabelIdx label);
  StmtInfoId GetRealFirstStmtInfoId(BB &bb);
  void TraverseMeExpr(MeExpr &meExpr, StmtInfoId stmtInfoId,
      std::unordered_set<ScalarMeExpr*> &cycleCheck);
  void TraverseMeStmt(MeStmt &meStmt);
  bool CollectSwitchImportantExpression(const MeStmt &meStmt, uint32 &index) const;
  bool CollectImportantExpression(const MeStmt &meStmt, uint32 &index) const;
  bool IsParameterOrUseParameter(const VarMeExpr *varExpr, uint32 &index) const;
  void ReplaceMeStmtWithStmtNode(StmtNode *stmt, StmtInfoId position);
  void Perform(MeFunction &func);
  void Dump();

  void PushInvalidKeyBack(uint invalidKey) {
    integerString.emplace_back(invalidKey);
    stmtInfoVector.emplace_back(StmtInfo(nullptr, -1u, allocator));
  }

  MapleVector<size_t> &GetIntegerString() {
    return integerString;
  }

  MapleVector<StmtInfo> &GetStmtInfo() {
    return stmtInfoVector;
  }

  uint GetCurrNewStmtIndex() {
    return ++currNewStmtIndex;
  }

  uint GetTotalStmtInfoCount() const {
    return currNewStmtIndex;
  }

  void SetDataMap(AnalysisDataManager *map) {
    dataMap = map;
  }

  MapleAllocator &GetAllocator() {
    return allocator;
  }
 private:
  MIRModule &module;
  MIRBuilder &builder;
  MIRFunction *curFunc;
  AnalysisDataManager *dataMap = nullptr;
  MapleAllocator allocator;
  MapleUnorderedMap<StmtInfo, StmtIndex, StmtInfoHash> stmtInfoToIntegerMap;
  MapleVector<StmtIndex> integerString;
  MapleVector<StmtInfo> stmtInfoVector;
  StmtIndex currNewStmtIndex = 0;
  StmtIndex prevInteger = kInvalidIndex;
  size_t continuousSequenceCount = 0;
};
MAPLE_SCC_PHASE_DECLARE_BEGIN(SCCCollectIpaInfo, maple::SCCNode<CGNode>)
OVERRIDE_DEPENDENCE
MAPLE_SCC_PHASE_DECLARE_END
}  // namespace maple
#endif  // MAPLE_IPA_INCLUDE_COLLECT_IPA_INFO_H
