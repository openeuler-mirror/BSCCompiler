/*
 * Copyright (c) [2020-2021] Huawei Technologies Co.,Ltd.All rights reserved.
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
#ifndef MAPLEME_INCLUDE_MECHECKCAST_H
#define MAPLEME_INCLUDE_MECHECKCAST_H
#include "me_function.h"
#include "me_irmap_build.h"
#include "me_dominance.h"
#include "me_ssi.h"
#include "annotation_analysis.h"

namespace maple {
struct GenericNode {
  AnnotationType *aType = nullptr;
  GenericNode *next = nullptr;
  bool multiOutput = false;
};

enum ProveRes {
  kT,
  kF,
  kIgnore,
};

class CheckCast {
 public:
  CheckCast(MeFunction &func, KlassHierarchy &kh, MeSSI &ms)
      : func(&func), klassh(&kh), meSSI(&ms) {}
  ~CheckCast() = default;

  void DoCheckCastOpt();
  void FindRedundantChecks();
  void DeleteRedundants();
  void ReleaseMemory() {
    if (graphTempMem != nullptr) {
      delete graphTempMem;
      graphTempAllocator.SetMemPool(nullptr);
      graphTempMem = nullptr;
    }
  }
 private:
  void RemoveRedundantCheckCast(MeStmt &stmt, BB &bb) const;
  bool ProvedByAnnotationInfo(const IntrinsiccallMeStmt &callNode);
  void TryToResolveCall(MeStmt &meStmt);
  bool TryToResolveVar(VarMeExpr &var, MIRStructType *callStruct = nullptr, bool checkFirst = false);
  void TryToResolveDassign(MeStmt &meStmt);
  GenericNode *GetOrCreateGenericNode(AnnotationType *annoType);
  void BuildGenericGraph(AnnotationType *annoType);
  MIRType *TryToResolveType(GenericNode &retNode);
  bool TryToResolveStaticVar(const VarMeExpr &var);
  void TryToResolveIvar(IvarMeExpr &ivar, MIRStructType *callStruct = nullptr);
  void TryToResolveFuncArg(MeExpr &expr, AnnotationType &at);
  void TryToResolveFuncGeneric(MIRFunction &callee, const CallMeStmt &callMeStmt, size_t thisIdx);
  void AddClassInheritanceInfo(MIRType &mirType);
  bool NeedChangeVarType(MIRStructType *varStruct, MIRStructType *callStruct);
  bool ExactlyMatch(MIRStructType &varStruct, MIRStructType &callStruct) const;
  AnnotationType *CloneNewAnnotationType(AnnotationType *at, MIRStructType *callStruct);
  void AddNextNode(GenericNode &from, GenericNode &to) const;
  bool RetIsGenericRelative(MIRFunction &callee) const;
  void DumpGenericGraph();
  void DumpGenericNode(GenericNode &node, std::ostream &out) const;

  bool ProvedBySSI(const IntrinsiccallMeStmt &callNode);
  ProveRes TraverseBackProve(MeExpr &expr, MIRType &targetType, std::set<MePhiNode*> &visitedPhi);
  std::map<AnnotationType*, GenericNode*> created;
  std::set<GenericNode*> visited;
  MeFunction *func;
  KlassHierarchy *klassh;
  MeSSI *meSSI;
  MemPool *graphTempMem { new ThreadLocalMemPool(memPoolCtrler, "graph mempool tmp") };
  MapleAllocator graphTempAllocator { graphTempMem };
  MIRType *curCheckCastType { nullptr };
  std::vector<MeStmt*> redundantChecks;
};

MAPLE_FUNC_PHASE_DECLARE(MECheckCastOpt, MeFunction)
}  // namespace maple
#endif  // MAPLEME_INCLUDE_MECONDBASEDNPC_H
