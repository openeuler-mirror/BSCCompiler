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
#ifndef MAPLEME_INCLUDE_ME_CFGOPT
#define MAPLEME_INCLUDE_ME_CFGOPT
#include "me_function.h"
#include "me_phase.h"

namespace maple {
class MeCfgOpt {
 public:
  explicit MeCfgOpt(MeIRMap *irMap) : meIrMap(irMap) {}

  ~MeCfgOpt() = default;
  bool Run(MeCFG &cfg);

 private:
  bool PreCheck(const MeCFG &func) const;
  bool IsOk2Select(const MeExpr &expr0, const MeExpr &expr1) const;
  // collect expensive ops and if there is reference, return true
  static bool IsExpensiveOp(Opcode op);
  bool CollectExpensiveOps(const MeExpr &meExpr, std::set<int32> &exprSet) const {
    if (IsExpensiveOp(meExpr.GetOp())) {
      (void)exprSet.insert(meExpr.GetExprID());
    }
    PrimType primType = meExpr.GetPrimType();
    bool isRef = (primType == PTY_ref || primType == PTY_f64 || primType == PTY_f32);
    if (isRef) {
      return true;
    }
    for (size_t i = 0; i < meExpr.GetNumOpnds(); ++i) {
      if (!CollectExpensiveOps(*meExpr.GetOpnd(i), exprSet)) {
        isRef = false;
      }
    }
    return isRef;
  }

  bool HasFloatCmp(const MeExpr &meExpr) const;
  const MeStmt *GetCondBrStmtFromBB(const BB &bb) const;
  MeStmt *GetTheOnlyMeStmtFromBB(BB &bb) const;
  MeStmt *GetTheOnlyMeStmtWithGotoFromBB(BB &bb) const;
  MeIRMap *meIrMap;
};

class MeDoCfgOpt : public MeFuncPhase {
 public:
  explicit MeDoCfgOpt(MePhaseID id) : MeFuncPhase(id) {}

  virtual ~MeDoCfgOpt() = default;
  AnalysisResult *Run(MeFunction *func, MeFuncResultMgr *m, ModuleResultMgr *mrm) override;
  std::string PhaseName() const override {
    return "cfgopt";
  }

  bool FindLocalRefVarUses(const MeIRMap &irMap, const MeExpr &expr, const MeStmt &meStmt, const VarMeExpr &var) const;
  void EmitMapleIr(MeFunction &func, MeFuncResultMgr &m);
};
}  // namespace maple
#endif  // MAPLEME_INCLUDE_ME_CFGOPT
