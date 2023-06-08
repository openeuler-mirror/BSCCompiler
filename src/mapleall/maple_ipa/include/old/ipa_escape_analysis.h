/*
 * Copyright (c) [2019] Huawei Technologies Co.,Ltd.All rights reserved.
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
#ifndef MAPLEIPA_INCLUDE_IPAESCAPEANALYSIS_H
#define MAPLEIPA_INCLUDE_IPAESCAPEANALYSIS_H

#include <vector>
#include "class_hierarchy.h"
#include "call_graph.h"
#include "irmap.h"
#include "me_function.h"
#include "ea_connection_graph.h"
#include "intrinsics.h"

namespace maple {
class IPAEscapeAnalysis {
 public:
  static constexpr int kCalleeCandidateLimit = 600;
  static constexpr int kFuncInSCCLimit = 200000;
  static constexpr int kSCCConvergenceLimit = 20;
  static constexpr int kCalleeNodeLimit = 100000;
  static constexpr int kRCOperLB = 0;
  static constexpr bool kDebug = false;

  IPAEscapeAnalysis(KlassHierarchy *khTmp, IRMap *irMapTmp, MeFunction *funcTmp, MemPool *mp, CallGraph *pcgTmp)
      : kh(khTmp),
        irMap(irMapTmp),
        ssaTab(&irMap->GetSSATab()),
        mirModule(&irMapTmp->GetMIRModule()),
        func(funcTmp),
        eaCG(func->GetMirFunc()->GetEACG()),
        pcg(pcgTmp),
        allocator(mp),
        cgChangedInSCC(false),
        tempCount(0),
        retVar(nullptr) {}
  ~IPAEscapeAnalysis() = default;
  void ConstructConnGraph();
  void DoOptimization();

 private:
  TyIdx GetAggElemType(const MIRType &aggregate) const;
  bool IsSpecialEscapedObj(const MeExpr &alloc) const;
  EACGRefNode *GetOrCreateCGRefNodeForVar(VarMeExpr &var, bool createObjNode = false);
  EACGRefNode *GetOrCreateCGRefNodeForAddrof(AddrofMeExpr &var, bool createObjNode = false);
  EACGRefNode *GetOrCreateCGRefNodeForReg(RegMeExpr &reg, bool createObjNode = false);
  EACGRefNode *GetOrCreateCGRefNodeForVarOrReg(MeExpr &var, bool createObjNode = false);
  void GetArrayBaseNodeForReg(std::vector<EACGBaseNode*> &nodes, RegMeExpr &regVar, MeStmt &stmt);
  void GetOrCreateCGFieldNodeForIvar(std::vector<EACGBaseNode*> &fieldNodes, IvarMeExpr &ivar, MeStmt &stmt,
                                     bool createObjNode);
  void GetOrCreateCGFieldNodeForIAddrof(std::vector<EACGBaseNode*> &fieldNodes, OpMeExpr &expr, MeStmt &stmt,
                                        bool createObjNode);
  EACGObjectNode *GetOrCreateCGObjNode(MeExpr *expr, const MeStmt *stmt = nullptr, EAStatus easOfPhanObj = kNoEscape);
  void GetCGNodeForMeExpr(std::vector<EACGBaseNode*> &nodes, MeExpr &expr, MeStmt &stmt, bool createObjNode);
  void CollectDefStmtForReg(std::set<RegMeExpr*> &visited, std::set<AssignMeStmt*> &defStmts, RegMeExpr &regVar);
  void UpdateEscConnGraphWithStmt(MeStmt &stmt);
  void UpdateEscConnGraphWithPhi(const BB &bb);
  void HandleParaAtFuncEntry();
  void HandleParaAtCallSite(uint32 callInfo, CallMeStmt &call);
  void HandleSingleCallee(CallMeStmt &callMeStmt);
  bool HandleSpecialCallee(CallMeStmt *callMeStmt);
  void HandleMultiCallees(const CallMeStmt &callMeStmt);
  EAConnectionGraph *GetEAConnectionGraph(MIRFunction &function) const;
  void ProcessNoAndRetEscObj();
  void ProcessRetStmt();
  VarMeExpr *CreateEATempVarWithName(const std::string &name) const;
  VarMeExpr *CreateEATempVar();
  VarMeExpr *GetOrCreateEARetTempVar();
  VarMeExpr *CreateEATempVarMeExpr(OriginalSt &ost) const;
  OriginalSt *CreateEATempOstWithName(const std::string &name) const;
  OriginalSt *CreateEATempOst();
  OriginalSt *CreateEARetTempOst() const;
  VarMeExpr *GetOrCreateEARetTempVarMeExpr(OriginalSt &ost);
  void CountSuperclassCallAssignedOpration(MeStmt* stmt);
  void CountObjRCOperations();
  void DeleteRedundantRC();

  KlassHierarchy *kh;
  IRMap *irMap;
  SSATab *ssaTab;
  MIRModule *mirModule;
  MeFunction *func;
  EAConnectionGraph *eaCG;
  CallGraph *pcg;
  MapleAllocator allocator;
  bool cgChangedInSCC;
  uint32 tempCount;
  std::vector<VarMeExpr*> noAndRetEscObj;
  VarMeExpr *retVar;
  std::vector<OriginalSt*> noAndRetEscOst;
  std::vector<MeStmt*> gcStmts;
};
} // namespace maple
#endif
