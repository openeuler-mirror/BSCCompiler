/*
 * Copyright (c) [2019-2021] Huawei Technologies Co.,Ltd.All rights reserved.
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
#ifndef MPL2MPL_INCLUDE_SCALARREPLACEMENT_H
#define MPL2MPL_INCLUDE_SCALARREPLACEMENT_H
#include "phase_impl.h"
namespace maple {
using StmtVec = std::vector<StmtNode*>;
using FldRefMap = std::unordered_map<FieldID, StmtVec>;
class ScalarReplacement : public FuncOptimizeImpl {
 public:
  ScalarReplacement(MIRModule &mod, KlassHierarchy *kh, bool trace) : FuncOptimizeImpl(mod, kh, trace) {}

  ~ScalarReplacement() override {
    curSym = nullptr;
    newScalarSym = nullptr;
  }

  FuncOptimizeImpl *Clone() override {
    return new ScalarReplacement(*this);
  }

  void ProcessFunc(MIRFunction *func) override;

 private:
  MIRSymbol *curSym = nullptr;
  MIRSymbol *newScalarSym = nullptr;
  FieldID curFieldid = 0;
  std::vector<IntrinsiccallNode*> localRefCleanup;
  std::unordered_map<MIRSymbol*, FldRefMap> localVarMap;
  template <typename Func>
  BaseNode *IterateExpr(StmtNode *stmt, BaseNode *expr, Func const &applyFunc);
  template <typename Func>
  void IterateStmt(StmtNode *stmt, Func const &applyFunc);
  BaseNode *MarkDassignDread(StmtNode *stmt, BaseNode *opnd);
  void CollectCandidates();
  void DumpCandidates() const;
  bool IsMemsetLocalvar(StmtNode *stmt) const;
  bool IsSetClass(StmtNode *stmt) const;
  bool IsCCWriteRefField(StmtNode *stmt) const;
  bool CanBeReplaced(const StmtVec *refs) const;
  BaseNode *ReplaceDassignDread(StmtNode *stmt, BaseNode *opnd);
  void AppendLocalRefCleanup(const MIRSymbol *sym);
  void ReplaceWithScalar(const StmtVec *refs);
  void FixRCCalls(const StmtVec *refs);
  void ReplaceLocalVars();
};

MAPLE_MODULE_PHASE_DECLARE(M2MScalarReplacement)
}  // namespace maple
#endif  // MPL2MPL_INCLUDE_SCALARREPLACEMENT_H
