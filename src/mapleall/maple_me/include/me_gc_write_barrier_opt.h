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
#ifndef MAPLE_ME_INCLUDE_ME_GC_WRITE_BARRIER_OPT_H
#define MAPLE_ME_INCLUDE_ME_GC_WRITE_BARRIER_OPT_H
#include "me_function.h"
#include "me_irmap.h"

namespace maple {
class GCWriteBarrierOpt {
 public:
  GCWriteBarrierOpt(MeFunction &f, Dominance &dom, bool enabledDebug)
      : func(f),
        mirModule(f.GetMIRModule()),
        irMap(*f.GetIRMap()),
        ssaTab(*f.GetMeSSATab()),
        dominance(dom),
        callBBs(0, false),
        visited(0, false),
        enabledDebug(enabledDebug) {}

  ~GCWriteBarrierOpt() = default;

  void Prepare();
  void GCLower(BB&, std::map<OStIdx, std::vector<MeStmt*>>&);
  void Finish();

 private:
  bool IsWriteBarrier(const MeStmt&);
  void ResetMeStmt(IntrinsiccallMeStmt&);
  OStIdx GetOStIdx(MeExpr&);
  bool IsCall(const MeStmt&);
  bool HasYieldPoint(const MeStmt&, const MeStmt&);
  bool HasCallAfterStmt(const MeStmt&);
  bool HasCallBeforeStmt(const MeStmt&);
  bool HasCallBetweenStmt(const MeStmt&, const MeStmt&);
  bool IsBackEdgeDest(const BB&);
  bool HasCallInBB(const BB&);

  MeFunction &func;
  MIRModule &mirModule;
  IRMap &irMap;
  SSATab &ssaTab;
  Dominance &dominance;
  std::vector<bool> callBBs;
  std::vector<bool> visited;
  bool enabledDebug;
};

class MeDoGCWriteBarrierOpt : public MeFuncPhase {
 public:
  explicit MeDoGCWriteBarrierOpt(MePhaseID id) : MeFuncPhase(id) {}

  virtual ~MeDoGCWriteBarrierOpt() = default;
  AnalysisResult *Run(MeFunction*, MeFuncResultMgr*, ModuleResultMgr*) override;
  std::string PhaseName() const override {
    return "GCWriteBarrierOpt";
  }
};
}  // namespace maple
#endif  // MAPLE_ME_INCLUDE_ME_GC_WRITE_BARRIER_OPT_H
