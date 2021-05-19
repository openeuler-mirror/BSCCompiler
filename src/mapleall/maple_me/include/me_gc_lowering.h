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
#ifndef MAPLE_ME_INCLUDE_ME_GC_LOWERING_H
#define MAPLE_ME_INCLUDE_ME_GC_LOWERING_H
#include "me_function.h"
#include "me_irmap.h"
#include "me_phase.h"
#include "mir_builder.h"
#include "me_cfg.h"

namespace maple {
class GCLowering {
 public:
  GCLowering(MeFunction &f, bool enabledDebug)
      : func(f),
        mirModule(f.GetMIRModule()),
        cfg(f.GetCfg()),
        irMap(*f.GetIRMap()),
        ssaTab(*f.GetMeSSATab()),
        enabledDebug(enabledDebug) {}

  ~GCLowering() = default;

  void Prepare();
  void GCLower();
  void Finish();

 private:
  void GCLower(BB&);
  void HandleAssignMeStmt(MeStmt&);
  void HandleVarAssignMeStmt(MeStmt&);
  void HandleIvarAssignMeStmt(MeStmt&);
  MeExpr *GetBase(IvarMeExpr &ivar);
  MIRIntrinsicID SelectWriteBarrier(MeStmt&);
  MIRIntrinsicID PrepareVolatileCall(MeStmt&, MIRIntrinsicID);
  void HandleWriteReferent(IassignMeStmt&);
  void CheckRefs();
  void ParseCheckFlag();
  void CheckFormals();
  void CheckRefsInAssignStmt(BB &bb);
  void CheckRefReturn(BB &bb);

  MeFunction &func;
  MIRModule &mirModule;
  MeCFG *cfg;
  IRMap &irMap;
  SSATab &ssaTab;
  bool isReferent = false;
  bool enabledDebug;
  bool checkRefFormal = false;
  bool checkRefAssign = false;
  bool checkRefReturn = false;
};

class MeDoGCLowering : public MeFuncPhase {
 public:
  explicit MeDoGCLowering(MePhaseID id) : MeFuncPhase(id) {}

  virtual ~MeDoGCLowering() = default;
  AnalysisResult *Run(MeFunction*, MeFuncResultMgr*, ModuleResultMgr*) override;
  std::string PhaseName() const override {
    return "gclowering";
  }
};
}  // namespace maple
#endif  // MAPLE_ME_INCLUDE_ME_GC_LOWERING_H
