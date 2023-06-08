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
#ifndef MAPLE_ME_INCLUDE_MERENAME2PREG_H
#define MAPLE_ME_INCLUDE_MERENAME2PREG_H
#include "me_function.h"
#include "maple_phase.h"
#include "me_alias_class.h"

namespace maple {
class SSARename2Preg {
 public:
  SSARename2Preg(MemPool *mp, MeFunction *f, MeIRMap *hmap, Dominance *domTree, AliasClass *alias)
      : alloc(mp),
        func(f),
        meirmap(hmap),
        dom(domTree),
        ssaTab(f->GetMeSSATab()),
        mirModule(&f->GetMIRModule()),
        aliasclass(alias),
        sym2regMap(std::less<OStIdx>(), alloc.Adapter()),
        vstidx2regMap(alloc.Adapter()),
        parmUsedVec(alloc.Adapter()),
        regFormalVec(alloc.Adapter()),
        ostDefedByChi(ssaTab->GetOriginalStTableSize(), false, alloc.Adapter()),
        ostDefedByDassign(ssaTab->GetOriginalStTableSize(), false, alloc.Adapter()),
        ostUsedByDread(ssaTab->GetOriginalStTableSize(), false, alloc.Adapter()),
        candsForSSAUpdate() {}

  virtual ~SSARename2Preg() = default;
  void RunSelf();
  void PromoteEmptyFunction();
  uint32 rename2pregCount = 0;

 private:
  const AliasClass::AliasSet *GetAliasSet(const OriginalSt *ost) const {
    return aliasclass->GetAliasSet(ost->GetIndex());
  }

  void Rename2PregStmt(MeStmt *stmt);
  void Rename2PregExpr(MeStmt *mestmt, MeExpr *meexpr);
  void Rename2PregLeafRHS(MeStmt *mestmt, const VarMeExpr *varmeexpr);
  void Rename2PregLeafLHS(MeStmt &mestmt, const VarMeExpr &varmeexpr);
  RegMeExpr *CreatePregForVar(const VarMeExpr &varMeExpr);
  RegMeExpr *FindOrCreatePregForVarPhiOpnd(const VarMeExpr *varMeExpr);
  bool Rename2PregPhi(MePhiNode &mevarphinode, MapleMap<OStIdx, MePhiNode *> &regPhiList);
  void UpdateRegPhi(MePhiNode &mevarphinode, MePhiNode &regphinode, const VarMeExpr *lhs);
  void Rename2PregCallReturn(MapleVector<MustDefMeNode> &mustdeflist);
  bool VarMeExprIsRenameCandidate(const VarMeExpr &varMeExpr) const;
  RegMeExpr *RenameVar(const VarMeExpr *varMeExpr);
  void UpdateMirFunctionFormal();
  void SetupParmUsed(const VarMeExpr *varmeexpr);
  void Init();
  void CollectUsedOst(const MeExpr *meExpr);
  void CollectDefUseInfoOfOst();
  std::string PhaseName() const {
    return "rename2preg";
  }

  MapleAllocator alloc;
  MeFunction *func;
  MeIRMap *meirmap;
  Dominance *dom;
  SSATab *ssaTab;
  MIRModule *mirModule;
  AliasClass *aliasclass;
  MapleMap<OStIdx, OriginalSt *> sym2regMap;      // map var to reg in original symbol
  MapleUnorderedMap<int32, RegMeExpr *> vstidx2regMap;  // maps the VarMeExpr's exprID to RegMeExpr
  MapleVector<bool> parmUsedVec;                       // if parameter is not used, it's false, otherwise true
  // if the parameter got promoted, the nth of func->mirFunc->_formal is the nth of regFormalVec, otherwise nullptr;
  MapleVector<RegMeExpr *> regFormalVec;
  MapleVector<bool> ostDefedByChi;
  MapleVector<bool> ostDefedByDassign;
  MapleVector<bool> ostUsedByDread;
  std::map<OStIdx, std::unique_ptr<std::set<BBId>>> candsForSSAUpdate;
};

MAPLE_FUNC_PHASE_DECLARE(MESSARename2Preg, MeFunction)
}  // namespace maple
#endif  // MAPLE_ME_INCLUDE_MERENAME2PREG_H
