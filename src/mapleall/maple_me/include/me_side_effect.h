/*
 * Copyright (c) [2020-2022] Huawei Technologies Co.,Ltd.All rights reserved.
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
#ifndef MAPLE_ME_INCLUDE_MESIDEEFFECT_H
#define MAPLE_ME_INCLUDE_MESIDEEFFECT_H
#include "call_graph.h"
#include "mir_nodes.h"
#include "mir_builder.h"
#include "me_ir.h"
#include "me_dominance.h"
#include "me_function.h"

namespace maple {
class BB;
class VersionSt;
// Sideeffect summary
class FuncWithSideEffect {
 public:
  FuncWithSideEffect(bool pureArg, bool def, bool defArg, bool retGlobalArg,
                     bool exceptionArg, bool ret, bool privateDefArg, const std::string &name)
      : funcName(name), pure(pureArg), defArg(def), def(defArg), retGlobal(retGlobalArg),
        exception(exceptionArg), retArg(ret), privateDef(privateDefArg) {}

  std::string GetFuncName() const {
    return funcName;
  }

  virtual ~FuncWithSideEffect() = default;
  bool GetPure() const {
    return pure;
  }

  bool GetDefArg() const {
    return defArg;
  }

  bool GetDef() const {
    return def;
  }

  bool GetRetGlobal() const {
    return retGlobal;
  }

  bool GetException() const {
    return exception;
  }

  bool GetRetArg() const {
    return retArg;
  }

  bool GetPrivateDef() const {
    return privateDef;
  }

 private:
  std::string funcName;
  bool pure;
  bool defArg;
  bool def;
  bool retGlobal;
  bool exception;
  bool retArg;
  bool privateDef;
};

// SCC side effects mask position
enum SCCEffectPosition : uint8 {
  kHasThrow = 0x01,
  kHasRetGlobal = 0x02,
  kHasDef = 0x04,
  kHasDefArg = 0x08,
  kPure = 0x10,
  kNotPure = kPure,  // Same position as kPure, depending on usage.
  kHasRetArg = 0x20,
  kHasPrivateDef = 0x40
};

class IpaSideEffect {
 public:
  IpaSideEffect(MeFunction &mf, MemPool *memPool, CallGraph &callGraph, Dominance &dom);
  virtual ~IpaSideEffect() = default;
  void DoAnalysis();

 private:
  bool IsIgnoreMethod(const MIRFunction &func);
  void AnalyzeUseThrowEhExpr(MeExpr &expr);
  bool AnalyzeReturnAllocObj(MeExpr &expr, const std::vector<MeExpr*>&);
  void SideEffectAnalyzeBB(BB&, std::vector<bool>&);
  void GetEffectFromAllCallees(MIRFunction &baseFunc);
  bool AnalyzeReturnAllocObjVst(MeExpr&, const std::vector<MeExpr*>&);
  bool MatchPuidxAndSetSideEffects(PUIdx idx);
  void ReadSummary() const;
  void SetEffectsTrue();
  void CopySccSideEffectToAllFunctions(SCCNode<CGNode> &scc, uint8 seMask) const;
  void GetEffectFromCallee(MIRFunction &callee, const MIRFunction &caller);
  void DumpFuncInfo(const std::string &msg, const std::string &name);
  uint32 GetOrSetSCCNodeId(MIRFunction &mirfunc);
  bool IsCallingIntoSCC(uint32 sccID) const;
  void UpdateExternalFuncSideEffects(MIRFunction &func);
  bool AnalyzeDefExpr(VersionSt &baseVar, std::vector<VersionSt*> &varVector);
  bool MEAnalyzeDefExpr(MeExpr &baseExprMe, std::vector<MeExpr*> &varVector);
  bool UpdateSideEffectWithStmt(MeStmt &meStmt,
                                std::set<VarMeExpr*> &globalExprs, std::set<VarMeExpr*> &argExprs,
                                std::set<VarMeExpr*> &nextLevelGlobalExprs,
                                std::set<VarMeExpr*> &nextLevelArgExprs);
  void AnalyzeExpr(MeExpr &expr,
                   std::set<VarMeExpr*> &globalExprs, std::set<VarMeExpr*> &argExprs,
                   std::set<VarMeExpr*> &nextLevelGlobalExprs, std::set<VarMeExpr*> &nextLevelArgExprs);
  bool IsPureFromSummary(const MIRFunction &func) const;

  void SetHasDef() {
    hasDef = true;
  }

  bool isSeen;           // This module is compiled and the effects are valid
  bool notPure;          // Does module produce result purely based on the inputs
  bool hasDefArg;
  bool hasDef;           // Has a definition of a field of any pre-existing object
  bool hasRetGlobal;     // Returns freshly allocated object
  bool hasThrException;  // throws an exception
  bool hasRetArg;        // access field is use and private
  bool hasPrivateDef;    // access field is def and private
  bool isGlobal;
  bool isArg;
  MeFunction &meFunc;
  MapleAllocator alloc;
  CallGraph &callGraph;
  uint32 sccId;
  Dominance &dominance;
};

MAPLE_FUNC_PHASE_DECLARE(MESideEffect, MeFunction)
}  // namespace maple
#endif  // MAPLE_ME_INCLUDE_MESIDEEFFECT_H
