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
#ifndef MAPLE_ME_INCLUDE_MESTOREPRE_H
#define MAPLE_ME_INCLUDE_MESTOREPRE_H
#include "me_ssu_pre.h"
#include "me_alias_class.h"
#include "me_irmap_build.h"
#include "me_ssa_update.h"

namespace maple {
class MeStorePre : public MeSSUPre {
 public:
  MeStorePre(MeFunction &f, Dominance &dom, AliasClass &ac, MemPool &memPool, bool enabledDebug)
      : MeSSUPre(f, dom, memPool, kStorePre, enabledDebug), aliasClass(&ac), curTemp(nullptr),
        bbCurTempMap(spreAllocator.Adapter()),
        candsForSSAUpdate() {}

  virtual ~MeStorePre() = default;

  std::map<OStIdx, std::unique_ptr<std::set<BBId>>> &CandsForSSAUpdate() {
    return candsForSSAUpdate;
  }

 private:
  inline bool IsJavaLang() const {
    return mirModule->IsJavaModule();
  }
  // step 6 methods
  void CheckCreateCurTemp();
  RegMeExpr *EnsureRHSInCurTemp(BB &bb);
  void CodeMotion() override;
  // step 0 methods
  void CreateRealOcc(const OStIdx &ostIdx, MeStmt &meStmt);
  void CreateUseOcc(const OStIdx &ostIdx, BB &bb);
  void CreateSpreUseOccsThruAliasing(const OriginalSt &muOst, BB &bb);
  void FindAndCreateSpreUseOccs(const MeExpr &meExpr, BB &bb);
  void CreateSpreUseOccsForAll(BB &bb) const;
  void BuildWorkListBB(BB *bb) override;
  void PerCandInit() override {
    curTemp = nullptr;
    bbCurTempMap.clear();
  }

  void AddCandsForSSAUpdate(OStIdx ostIdx, const BB &bb) {
    MeSSAUpdate::InsertOstToSSACands(ostIdx, bb, &candsForSSAUpdate);
  }

  AliasClass *aliasClass;
  // step 6 code motion
  RegMeExpr *curTemp;                               // the preg for the RHS of inserted stores
  MapleUnorderedMap<BB*, RegMeExpr*> bbCurTempMap;  // map bb to curTemp version
  std::map<OStIdx, std::unique_ptr<std::set<BBId>>> candsForSSAUpdate;
};

MAPLE_FUNC_PHASE_DECLARE(MEStorePre, MeFunction)
}  // namespace maple
#endif  // MAPLE_ME_INCLUDE_MESTOREPRE_H
