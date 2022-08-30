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
#ifndef MAPLE_ME_INCLUDE_MEANALYZERC_H
#define MAPLE_ME_INCLUDE_MEANALYZERC_H
#include "me_function.h"
#include "me_alias_class.h"
#include "me_irmap.h"

namespace maple {
class RCItem {
 public:
  RCItem(OriginalSt &o, MapleAllocator &alloc)
      : ost(o),
        noAlias(false),
        nonLocal(false),
        isFormal(false),
        needSomeRC(false),
        versionStack(alloc.Adapter()),
        occurBBs(std::less<BBId>(), alloc.Adapter()) {}

  virtual ~RCItem() = default;

  void Dump();
 private:
  friend class AnalyzeRC;
  OriginalSt &ost;
  bool noAlias;
  bool nonLocal;            // need to do placement optimization if (!nonLocal)
  bool isFormal;            // is an incoming formal parameter
  bool needSomeRC;          // true if any definition has rhs that needs RC
  MapleStack<MeExpr*> versionStack;
  MapleSet<BBId> occurBBs;  // set of BBs where the pointer occurs; only for local ref pointers
};

class AnalyzeRC {
 public:
  AnalyzeRC(MeFunction &f, Dominance &dom, AliasClass &ac, MemPool *memPool)
      : func(f),
        cfg(f.GetCfg()),
        irMap(*f.GetIRMap()),
        ssaTab(*f.GetMeSSATab()),
        dominance(dom),
        aliasClass(ac),
        analyzeRCMp(memPool),
        analyzeRCAllocator(memPool),
        rcItemsMap(std::less<OStIdx>(), analyzeRCAllocator.Adapter()),
        skipLocalRefVars(false) {}

  virtual ~AnalyzeRC() = default;
  void Run();

 private:
  void IdentifyRCStmts();
  void CreateCleanupIntrinsics();
  void RenameRefPtrs(BB *bb);
  void OptimizeRC();
  void RemoveUnneededCleanups();
  void RenameUses(MeStmt &meStmt);
  RCItem *FindOrCreateRCItem(OriginalSt &ost);
  OriginalSt *GetOriginalSt(const MeExpr &refLHS) const;
  VarMeExpr *GetZeroVersionVarMeExpr(const VarMeExpr &var);
  bool NeedIncref(const MeStmt &stmt) const;
  UnaryMeStmt *CreateIncrefZeroVersion(OriginalSt &ost);
  DassignMeStmt *CreateDassignInit(OriginalSt &ost, BB &bb);
  void TraverseStmt(BB &bb);
  bool NeedDecRef(const RCItem &rcItem, MeExpr &expr) const;
  bool NeedDecRef(IvarMeExpr &ivar) const;
  bool NeedDecRef(const VarMeExpr &var) const;

  friend class MeDoAnalyzeRC;
  MeFunction &func;
  MeCFG *cfg;
  IRMap &irMap;
  SSATab &ssaTab;
  Dominance &dominance;
  AliasClass &aliasClass;
  MemPool *analyzeRCMp;
  MapleAllocator analyzeRCAllocator;
  MapleMap<OStIdx, RCItem*> rcItemsMap;
  bool skipLocalRefVars;
};

MAPLE_FUNC_PHASE_DECLARE(MEAnalyzeRC, MeFunction)
}  // namespace maple
#endif  // MAPLE_ME_INCLUDE_MEANALYZERC_H
