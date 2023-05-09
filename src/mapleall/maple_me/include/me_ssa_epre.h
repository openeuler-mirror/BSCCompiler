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
#ifndef MAPLE_ME_INCLUDE_MESSAEPRE_H
#define MAPLE_ME_INCLUDE_MESSAEPRE_H

#include "me_function.h"
#include "me_irmap.h"
#include "me_cfg.h"
#include "ssa_epre.h"
#include "class_hierarchy_phase.h"
#include "me_ssa_update.h"

namespace maple {
class MeSSAEPre : public SSAEPre {
 public:
  // a symbol is a candidate for ssaupdate if its ostidx key exists in the map;
  // the mapped set gives bbs where dassign's are inserted by ssa_epre for the symbol
  explicit MeSSAEPre(MeFunction &func, IRMap &map, Dominance &dom, Dominance &pdom, KlassHierarchy *kh,
                     MemPool &memPool, MemPool &mp2, uint32 limit, bool includeRef, bool epreLocalRefVar, bool lhsIvar)
      : SSAEPre(map, dom, pdom, memPool, mp2, kExprPre, limit, includeRef, lhsIvar),
        candsForSSAUpdate(std::less<OStIdx>()),
        func(&func),
        epreLocalRefVar(epreLocalRefVar),
        klassHierarchy(kh) {}

  ~MeSSAEPre() override = default;
  bool ScreenPhiBB(BBId) const override {
    return true;
  }

  std::map<OStIdx, std::unique_ptr<std::set<BBId>>> &GetCandsForSSAUpdate() {
    return candsForSSAUpdate;
  }

 protected:
  std::map<OStIdx, std::unique_ptr<std::set<BBId>>> candsForSSAUpdate;
  MeFunction *func;
  bool epreLocalRefVar;
  KlassHierarchy *klassHierarchy;

 private:
  void BuildWorkList() override;
  bool IsThreadObjField(const IvarMeExpr &expr) const override;
  BB *GetBB(BBId id) const override {
    return func->GetCfg()->GetBBFromID(id);
  }

  PUIdx GetPUIdx() const override {
    return func->GetMirFunc()->GetPuidx();
  }

  bool CfgHasDoWhile() const override {
    return func->GetCfg()->GetHasDoWhile();
  }

  bool EpreLocalRefVar() const override {
    return epreLocalRefVar;
  }

  void EnterCandsForSSAUpdate(OStIdx ostIdx, const BB &bb) override {
    MeSSAUpdate::InsertOstToSSACands(ostIdx, bb, &candsForSSAUpdate);
  }
};

MAPLE_FUNC_PHASE_DECLARE(MESSAEPre, MeFunction)
}  // namespace maple
#endif  // MAPLE_ME_INCLUDE_MESSAEPRE_H
