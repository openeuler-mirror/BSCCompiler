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
        sym2reg_map(std::less<OStIdx>(), alloc.Adapter()),
        vstidx2reg_map(alloc.Adapter()),
        parm_used_vec(alloc.Adapter()),
        reg_formal_vec(alloc.Adapter()),
        ostDefedByChi(ssaTab->GetOriginalStTableSize(), false, alloc.Adapter()),
        ostDefedByDassign(ssaTab->GetOriginalStTableSize(), false, alloc.Adapter()),
        ostUsedByDread(ssaTab->GetOriginalStTableSize(), false, alloc.Adapter()),
        candsForSSAUpdate() {}

  virtual ~SSARename2Preg() = default;
  void RunSelf();
  void PromoteEmptyFunction();

 private:
  const MapleSet<unsigned int> *GetAliasSet(const OriginalSt *ost) {
    if (ost->GetIndex() >= aliasclass->GetAliasElemCount()) {
      return nullptr;
    }
    return aliasclass->FindAliasElem(*ost)->GetClassSet();
  }

  void Rename2PregStmt(MeStmt *);
  void Rename2PregExpr(MeStmt *, MeExpr *);
  void Rename2PregLeafRHS(MeStmt *, const VarMeExpr *);
  void Rename2PregLeafLHS(MeStmt *, const VarMeExpr *);
  void Rename2PregPhi(MePhiNode *, MapleMap<OStIdx, MePhiNode *> &);
  void UpdateRegPhi(MePhiNode *, MePhiNode *, const RegMeExpr *, const VarMeExpr *);
  void Rename2PregCallReturn(MapleVector<MustDefMeNode> &);
  RegMeExpr *RenameVar(const VarMeExpr *);
  void UpdateMirFunctionFormal();
  void SetupParmUsed(const VarMeExpr *);
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
  MapleMap<OStIdx, OriginalSt *> sym2reg_map;      // map var to reg in original symbol
  MapleUnorderedMap<int32, RegMeExpr *> vstidx2reg_map;  // maps the VarMeExpr's exprID to RegMeExpr
  MapleVector<bool> parm_used_vec;                       // if parameter is not used, it's false, otherwise true
  // if the parameter got promoted, the nth of func->mirFunc->_formal is the nth of reg_formal_vec, otherwise nullptr;
  MapleVector<RegMeExpr *> reg_formal_vec;
  MapleVector<bool> ostDefedByChi;
  MapleVector<bool> ostDefedByDassign;
  MapleVector<bool> ostUsedByDread;
  std::map<OStIdx, std::unique_ptr<std::set<BBId>>> candsForSSAUpdate;
 public:
  uint32 rename2pregCount = 0;
};

MAPLE_FUNC_PHASE_DECLARE(MESSARename2Preg, MeFunction)
}  // namespace maple
#endif  // MAPLE_ME_INCLUDE_MERENAME2PREG_H
