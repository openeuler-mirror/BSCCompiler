/*
 * Copyright (c) [2022] Huawei Technologies Co.,Ltd.All rights reserved.
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
#include "cg_validbit_opt.h"
#include "mempool.h"
#include "aarch64_validbit_opt.h"
#include "aarch64_reg_coalesce.h"

namespace maplebe {


InsnSet ValidBitPattern::GetAllUseInsn(const RegOperand &defReg) const {
  InsnSet allUseInsn;
  if ((ssaInfo != nullptr) && defReg.IsSSAForm()) {
    VRegVersion *defVersion = ssaInfo->FindSSAVersion(defReg.GetRegisterNumber());
    CHECK_FATAL(defVersion != nullptr, "useVRegVersion must not be null based on ssa");
    for (auto insnInfo : defVersion->GetAllUseInsns()) {
      Insn *currInsn = insnInfo.second->GetInsn();
      allUseInsn.emplace(currInsn);
    }
  }
  return allUseInsn;
}

void ValidBitPattern::DumpAfterPattern(std::vector<Insn*> &prevInsns, const Insn *replacedInsn, const Insn *newInsn) {
  LogInfo::MapleLogger() << ">>>>>>> In " << GetPatternName() << " : <<<<<<<\n";
  if (!prevInsns.empty()) {
    if ((replacedInsn == nullptr) && (newInsn == nullptr)) {
      LogInfo::MapleLogger() << "======= RemoveInsns : {\n";
    } else {
      LogInfo::MapleLogger() << "======= PrevInsns : {\n";
    }
    for (auto *prevInsn : prevInsns) {
      if (prevInsn != nullptr) {
        LogInfo::MapleLogger() << "[primal form] ";
        prevInsn->Dump();
        if (ssaInfo != nullptr) {
          LogInfo::MapleLogger() << "[ssa form] ";
          ssaInfo->DumpInsnInSSAForm(*prevInsn);
        }
      }
    }
    LogInfo::MapleLogger() << "}\n";
  }
  if (replacedInsn != nullptr) {
    LogInfo::MapleLogger() << "======= OldInsn :\n";
    LogInfo::MapleLogger() << "[primal form] ";
    replacedInsn->Dump();
    if (ssaInfo != nullptr) {
      LogInfo::MapleLogger() << "[ssa form] ";
      ssaInfo->DumpInsnInSSAForm(*replacedInsn);
    }
  }
  if (newInsn != nullptr) {
    LogInfo::MapleLogger() << "======= NewInsn :\n";
    LogInfo::MapleLogger() << "[primal form] ";
    newInsn->Dump();
    if (ssaInfo != nullptr) {
      LogInfo::MapleLogger() << "[ssa form] ";
      ssaInfo->DumpInsnInSSAForm(*newInsn);
    }
  }
}

void ValidBitOpt::RectifyValidBitNum() {
  FOR_ALL_BB(bb, cgFunc) {
    FOR_BB_INSNS(insn, bb) {
      if (!insn->IsMachineInstruction()) {
        continue;
      }
      SetValidBits(*insn);
    }
  }
  bool iterate;
  /* Use reverse postorder to converge with minimal iterations */
  do {
    iterate = false;
    MapleVector<uint32> reversePostOrder = ssaInfo->GetReversePostOrder();
    for (uint32 bbId : reversePostOrder) {
      BB *bb = cgFunc->GetBBFromID(bbId);
      FOR_BB_INSNS(insn, bb) {
        if (!insn->IsPhi()) {
          continue;
        }
        bool change = SetPhiValidBits(*insn);
        if (change) {
          /* if vb changes once, iterate. */
          iterate = true;
        }
      }
    }
  } while (iterate);
}

void ValidBitOpt::RecoverValidBitNum() {
  FOR_ALL_BB(bb, cgFunc) {
    FOR_BB_INSNS(insn, bb) {
      if (!insn->IsMachineInstruction() && !insn->IsPhi()) {
        continue;
      }
      uint32 opndNum = insn->GetOperandSize();
      for (uint32 i = 0; i < opndNum; ++i) {
        Operand &opnd = insn->GetOperand(i);
        if (!opnd.IsRegister()) {
          continue;
        }
        auto &regOpnd = static_cast<RegOperand&>(opnd);
        if (insn->OpndIsDef(i)) {
          regOpnd.SetValidBitsNum(regOpnd.GetSize());
        }
      }
    }
  }
}

void ValidBitOpt::Run() {
  /*
   * Set validbit of regOpnd before optimization
   */
  RectifyValidBitNum();
  DoOpt();
  cgDce->DoDce();
  /*
   * Recover validbit of regOpnd after optimization
   */
  RecoverValidBitNum();
}

bool CgValidBitOpt::PhaseRun(maplebe::CGFunc &f) {
  CGSSAInfo *ssaInfo = GET_ANALYSIS(CgSSAConstruct, f);
  CHECK_FATAL(ssaInfo != nullptr, "Get ssaInfo failed");
  LiveIntervalAnalysis *ll = GET_ANALYSIS(CGliveIntervalAnalysis, f);
  CHECK_FATAL(ll != nullptr, "Get ll failed");
  auto *vbOpt = f.GetCG()->CreateValidBitOpt(*GetPhaseMemPool(), f, *ssaInfo, *ll);
  CHECK_FATAL(vbOpt != nullptr, "vbOpt instance create failed");
  vbOpt->Run();
  ll->ClearBFS();
  return true;
}

void CgValidBitOpt::GetAnalysisDependence(AnalysisDep &aDep) const {
  aDep.AddRequired<CgSSAConstruct>();
  aDep.AddRequired<CGliveIntervalAnalysis>();
  aDep.AddPreserved<CgSSAConstruct>();
}
MAPLE_TRANSFORM_PHASE_REGISTER_CANSKIP(CgValidBitOpt, cgvalidbitopt)
} /* namespace maplebe */

