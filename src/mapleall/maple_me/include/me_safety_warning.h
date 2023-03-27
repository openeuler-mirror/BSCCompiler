/*
 * Copyright (c) [2021] Huawei Technologies Co.,Ltd.All rights reserved.
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
#ifndef MAPLE_ME_INCLUDE_ME_SAFETY_WARNING_H
#define MAPLE_ME_INCLUDE_ME_SAFETY_WARNING_H
#include "me_function.h"
#include "opcodes.h"
namespace maple {
std::string GetNthStr(size_t index);
// param: assert stmt, mir module, caller function
// return True --- delete assert stmt.
using SafetyWarningHandler = std::function<bool(const MeStmt&, const MIRModule&, const MeFunction&)>;
using SafetyWarningHandlers = std::map<Opcode, SafetyWarningHandler>;
class MESafetyWarning : public MapleFunctionPhase<MeFunction> {
 public:
  explicit MESafetyWarning(MemPool *mp) : MapleFunctionPhase<MeFunction>(&id, mp) {
    realNpeHandleMap =
        (MeOption::npeCheckMode == SafetyCheckMode::kDynamicCheckSilent) ? &npeSilentHandleMap : &npeHandleMap;
    realBoundaryHandleMap = (MeOption::boundaryCheckMode == SafetyCheckMode::kDynamicCheckSilent)
                                ? &boundarySilentHandleMap
                                : &boundaryHandleMap;
  }

  ~MESafetyWarning() override = default;
  static unsigned int id;
  static MaplePhase *CreatePhase(MemPool *createMP) {
    return createMP->New<MESafetyWarning>(createMP);
  }

  bool PhaseRun(MeFunction &f) override;
  std::string PhaseName() const override;

 private:
  void GetAnalysisDependence(maple::AnalysisDep &aDep) const override;
  bool IsStaticModeForOp(Opcode op) const;
  SafetyWarningHandler *FindHandler(Opcode op) const;

  SafetyWarningHandlers *realNpeHandleMap;
  SafetyWarningHandlers *realBoundaryHandleMap;
  static SafetyWarningHandlers npeHandleMap;
  static SafetyWarningHandlers boundaryHandleMap;
  static SafetyWarningHandlers npeSilentHandleMap;
  static SafetyWarningHandlers boundarySilentHandleMap;
};
}  // namespace maple
#endif /* MAPLE_ME_INCLUDE_ME_SAFETY_WARNING_H */
