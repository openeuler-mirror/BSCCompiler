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
#ifndef MAPLE_ME_INCLUDE_ME_SUBSUM_RC_H
#define MAPLE_ME_INCLUDE_ME_SUBSUM_RC_H
#include "me_ssu_pre.h"

namespace maple {
class DassignStmtPtrComparator {
 public:
  bool operator()(const DassignMeStmt *stmtA, const DassignMeStmt *stmtB) const {
    ASSERT_NOT_NULL(stmtA);
    ASSERT_NOT_NULL(stmtB);
    ASSERT_NOT_NULL(stmtA->GetLHS());
    ASSERT_NOT_NULL(stmtB->GetLHS());
    return static_cast<VarMeExpr*>(stmtA->GetLHS())->GetVstIdx() <
           static_cast<VarMeExpr*>(stmtB->GetLHS())->GetVstIdx();
  }
};

class SubsumRC : public MeSSUPre {
 public:
  SubsumRC(MeFunction &f, Dominance &dom, MemPool &mp, bool enabledDebug)
      : MeSSUPre(f, dom, mp, kSubsumePre, enabledDebug),
        candMap(spreAllocator.Adapter()),
        bbVisited(f.GetCfg()->GetAllBBs().size(), false, spreAllocator.Adapter()),
        verstCantSubsum(f.GetIRMap()->GetVerst2MeExprTable().size(), false, spreAllocator.Adapter()) {}

  virtual ~SubsumRC() = default;
  void RunSSUPre();

 protected:
  MapleMap<DassignMeStmt*, SpreWorkCand*, DassignStmtPtrComparator> candMap;
  MapleVector<bool> bbVisited;
  MapleVector<bool> verstCantSubsum;

 private:
  void SetCantSubsum();
  // step 6 methods
  void ReplaceExpr(BB &bb, const MeExpr &var, MeExpr &reg);
  void SubsumeRC(MeStmt &stmt);
  // step 0 methods
  void CreateRealOcc(VarMeExpr &varX, DassignMeStmt &meStmt, MeStmt &decRef);
  void BuildRealOccs(MeStmt &stmt, BB &bb);
  void BuildWorkListBB(BB *bb) override;
  void PerCandInit() override {}
  void CodeMotion() override {}
  bool IsRhsPostDominateLhs(const SOcc &rhsOcc, const VarMeExpr &lhs) const;
};

MAPLE_FUNC_PHASE_DECLARE(MESubsumRC, MeFunction)
}  // namespace maple
#endif  // MAPLE_ME_INCLUDE_ME_SUBSUM_RC_H
