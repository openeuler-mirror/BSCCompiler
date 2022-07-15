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
#ifndef MAPLE_MPL2MPL_INCLUDE_PROFILEGEN_H
#define MAPLE_MPL2MPL_INCLUDE_PROFILEGEN_H
#include <algorithm>
#include "module_phase_manager.h"
#include "me_pgo_instrument.h"
#include "bb.h"
#include "me_irmap.h"
#include "maple_phase_manager.h"


namespace maple {
static constexpr const uint32_t kMplModProfMergeFuncs = 9;  // gcov reserves 9 slots
static constexpr const uint32_t kGCCVersion = 0x4137352A;   // gcc v7.5 under tools
// static constexpr const uint32_t kGCCVersion = 0x4139342A; // GCC V9.3/9.4
// anything other than merge_add will be supported supported in future
static constexpr const uint32_t kMplFuncProfCtrInfoNum = 1;

class ProfileGenPM : public SccPM {
 public:
  explicit ProfileGenPM(MemPool *memPool) : SccPM(memPool, &id) {}
  bool PhaseRun(MIRModule &m) override;
  PHASECONSTRUCTOR(ProfileGenPM);
  ~ProfileGenPM() override {}
  std::string PhaseName() const override;
 private:
  void GetAnalysisDependence(AnalysisDep &aDep) const override;
};

class ProfileGen {
 public:
  explicit ProfileGen(MIRModule &module) : mod(module) {
    for (MIRFunction *f : mod.GetFunctionList()) {
      if (!f->IsEmpty()) { validFuncs.push_back(f); }
    }
  };

  std::string flatenName(const std::string &name) const {
    std::string filteredName = name;
    std::replace(filteredName.begin(), filteredName.end(), '.', '_');
    std::replace(filteredName.begin(), filteredName.end(), '-', '_');
    std::replace(filteredName.begin(), filteredName.end(), '/', '_');
    return filteredName;
  }

  void CreateModProfDesc();
  void CreateFuncProfDesc();
  void CreateFuncProfDescTbl();
  void FixupDesc();
  void CreateInitProc();
  void CreateExitProc();
  void GenFuncCtrTbl();
  void Run();
  std::vector<MIRFunction *> getValidFuncs() { return validFuncs; }

 private:
    MIRModule &mod;
    MIRSymbol *modProfDesc = nullptr;
    // Keep order of funcs visited
    MIRSymbol *funcProfDescTbl = nullptr;
    std::vector<MIRSymbol *> funcProfDescs;
    std::vector<MIRFunction *> validFuncs;
};
}  // namespace maple
#endif  // MAPLE_MPL2MPL_INCLUDE_PROFILEGEN_H
