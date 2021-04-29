/*
 * Copyright (c) [2020] Huawei Technologies Co.,Ltd.All rights reserved.
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
#include "me_phase.h"
#include "me_function.h"

namespace maple {
class SSARename2Preg {
 public:
  SSARename2Preg(MemPool *mp, MeFunction *f, MeIRMap *hmap, AliasClass *alias)
      : alloc(mp),
        func(f),
        meirmap(hmap),
        ssaTab(f->GetMeSSATab()),
        mirModule(&f->GetMIRModule()),
        aliasclass(alias),
        sym2reg_map(std::less<OStIdx>(), alloc.Adapter()),
        vstidx2reg_map(alloc.Adapter()),
        parm_used_vec(alloc.Adapter()),
        reg_formal_vec(alloc.Adapter()) {}

  void RunSelf();
  void PromoteEmptyFunction();

 private:
  AliasElem *GetAliasElem(const OriginalSt *ost) {
    if (ost->GetIndex() >= aliasclass->GetAliasElemCount()) {
      return nullptr;
    }
    return aliasclass->FindAliasElem(*ost);
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
  std::string PhaseName() const {
    return "rename2preg";
  }

  MapleAllocator alloc;
  MeFunction *func;
  MeIRMap *meirmap;
  SSATab *ssaTab;
  MIRModule *mirModule;
  AliasClass *aliasclass;
  MapleMap<OStIdx, OriginalSt *> sym2reg_map;      // map var to reg in original symbol
  MapleUnorderedMap<int32, RegMeExpr *> vstidx2reg_map;  // maps the VarMeExpr's exprID to RegMeExpr
  MapleVector<bool> parm_used_vec;                       // if parameter is not used, it's false, otherwise true
  // if the parameter got promoted, the nth of func->mirFunc->_formal is the nth of reg_formal_vec, otherwise nullptr;
  MapleVector<RegMeExpr *> reg_formal_vec;
 public:
  uint32 rename2pregCount = 0;
};

class MeDoSSARename2Preg : public MeFuncPhase {
 public:
  explicit MeDoSSARename2Preg(MePhaseID id) : MeFuncPhase(id) {}

  virtual ~MeDoSSARename2Preg() = default;
  AnalysisResult *Run(MeFunction *func, MeFuncResultMgr *funcRst, ModuleResultMgr*) override;
  std::string PhaseName() const override {
    return "rename2preg";
  }
};
}  // namespace maple
#endif  // MAPLE_ME_INCLUDE_MERENAME2PREG_H
