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
#ifndef MAPLEBE_INCLUDE_CG_CG_PHASEMANAGER_H
#define MAPLEBE_INCLUDE_CG_CG_PHASEMANAGER_H
#include <string>
#include "mempool.h"
#include "mempool_allocator.h"
#include "mir_module.h"
#include "mir_lower.h"
#include "lower.h"
#include "constantfold.h"
#include "cgfunc.h"
#include "cg_phase.h"
#include "cg_option.h"
namespace maplebe {
using CgFuncOptTy = MapleFunctionPhase<CGFunc>;

/* =================== new phase manager ===================  */
class CgFuncPM : public FunctionPM {
 public:
  explicit CgFuncPM(MemPool *mp) : FunctionPM(mp, &id) {}
  PHASECONSTRUCTOR(CgFuncPM);
  std::string PhaseName() const override;
  ~CgFuncPM() override {
    cgOptions = nullptr;
    cg = nullptr;
    beCommon = nullptr;
  }
  bool PhaseRun(MIRModule &m) override;

  void SetCGOptions(CGOptions *curCGOptions) {
    cgOptions = curCGOptions;
  }

  CG *GetCG() {
    return cg;
  }
  BECommon *GetBECommon() {
    return beCommon;
  }
  void SweepUnusedStaticSymbol(MIRModule &m) const;
 private:
  bool FuncLevelRun(CGFunc &cgFunc, AnalysisDataManager &serialADM);
  void GenerateOutPutFile(MIRModule &m) const;
  void CreateCGAndBeCommon(MIRModule &m);
  void PrepareLower(MIRModule &m);
  void PostOutPut(MIRModule &m) const;
  void DoFuncCGLower(const MIRModule &m, MIRFunction &mirFunc) const;
  /* Tool functions */
  void DumpFuncCGIR(const CGFunc &f, const std::string &phaseName) const;
  /* For Emit */
  void InitProfile(MIRModule &m) const;
  void EmitGlobalInfo(MIRModule &m) const;
  void EmitDuplicatedAsmFunc(MIRModule &m) const;
  void EmitDebugInfo(const MIRModule &m) const;
  void EmitFastFuncs(const MIRModule &m) const;
  bool IsFramework(MIRModule &m) const;

  CG *cg = nullptr;
  BECommon *beCommon = nullptr;
  MIRLower *mirLower = nullptr;
  CGLowerer *cgLower = nullptr;
  /* module options */
  CGOptions *cgOptions = nullptr;
};
}  /* namespace maplebe */
#endif  /* MAPLEBE_INCLUDE_CG_CG_PHASEMANAGER_H */
