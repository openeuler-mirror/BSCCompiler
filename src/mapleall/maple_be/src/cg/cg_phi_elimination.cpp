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
#include "cg_phi_elimination.h"
#include "cg.h"
#include "cgbb.h"

namespace maplebe {
void PhiEliminate::TranslateTSSAToCSSA() {
  FOR_ALL_BB(bb, cgFunc) {
    eliminatedBB.emplace(bb->GetId());
    for (auto phiInsnIt : bb->GetPhiInsns()) {
      /* Method I create a temp move for phi-node */
      auto &destReg = static_cast<RegOperand&>(phiInsnIt.second->GetOperand(kInsnFirstOpnd));
      RegOperand &tempMovDest = cgFunc->GetOrCreateVirtualRegisterOperand(CreateTempRegForCSSA(destReg));
      auto &phiList = static_cast<PhiOperand&>(phiInsnIt.second->GetOperand(kInsnSecondOpnd));
      for (auto phiOpndIt : phiList.GetOperands()) {
        uint32 fBBId = phiOpndIt.first;
        ASSERT(fBBId != 0, "GetFromBBID = 0");
#if DEBUG
        bool find = false;
        for (auto predBB : bb->GetPreds()) {
          if (predBB->GetId() == fBBId) {
            find = true;
          }
        }
        CHECK_FATAL(find, "dont exited pred for phi-node");
#endif
        PlaceMovInPredBB(fBBId, CreateMov(tempMovDest, *(phiOpndIt.second)));
      }
      Insn &movInsn = CreateMov(destReg, tempMovDest);
      bb->ReplaceInsn(*phiInsnIt.second, movInsn);
    }
  }

  FOR_ALL_BB(bb, cgFunc) {
    FOR_BB_INSNS(insn, bb) {
      CHECK_FATAL(eliminatedBB.count(bb->GetId()), "still have phi");
      if (!insn->IsMachineInstruction()) {
        continue;
      }
      ReCreateRegOperand(*insn);
      bb->GetPhiInsns().clear();
    }
  }
  UpdateRematInfo();
}

void PhiEliminate::UpdateRematInfo() {
  if (CGOptions::GetRematLevel() > 0) {
    cgFunc->UpdateAllRegisterVregMapping(remateInfoAfterSSA);
  }
}

void PhiEliminate::PlaceMovInPredBB(uint32 predBBId, Insn &movInsn) {
  BB *predBB = cgFunc->GetBBFromID(predBBId);
  ASSERT(movInsn.GetOperand(kInsnSecondOpnd).IsRegister(), "unexpect operand");
  if (predBB->GetKind() == BB::kBBFallthru) {
    predBB->AppendInsn(movInsn);
  } else {
    AppendMovAfterLastVregDef(*predBB, movInsn);
  }
}

regno_t PhiEliminate::GetAndIncreaseTempRegNO() {
  while (GetSSAInfo()->GetAllSSAOperands().count(tempRegNO)) {
    tempRegNO++;
  }
  regno_t ori = tempRegNO;
  tempRegNO++;
  return ori;
}

RegOperand *PhiEliminate::MakeRoomForNoDefVreg(RegOperand &conflictReg) {
  regno_t conflictVregNO = conflictReg.GetRegisterNumber();
  auto rVregIt = replaceVreg.find(conflictVregNO);
  if (rVregIt != replaceVreg.end()) {
    return rVregIt->second;
  } else {
    RegOperand *regForRecreate = &CreateTempRegForCSSA(conflictReg);
    (void)replaceVreg.emplace(std::pair<regno_t, RegOperand*>(conflictVregNO, regForRecreate));
    return regForRecreate;
  }
}

void PhiEliminate::RecordRematInfo(regno_t vRegNO, PregIdx pIdx) {
  if (remateInfoAfterSSA.count(vRegNO)) {
    if (remateInfoAfterSSA[vRegNO] != pIdx) {
      remateInfoAfterSSA.erase(vRegNO);
    }
  } else {
    (void)remateInfoAfterSSA.emplace(std::pair<regno_t, PregIdx>(vRegNO, pIdx));
  }
}

bool CgPhiElimination::PhaseRun(maplebe::CGFunc &f) {
  CGSSAInfo *ssaInfo = GET_ANALYSIS(CgSSAConstruct, f);
  PhiEliminate *pe = f.GetCG()->CreatePhiElimintor(*GetPhaseMemPool(),f, *ssaInfo);
  pe->TranslateTSSAToCSSA();
  return false;
}
void CgPhiElimination::GetAnalysisDependence(maple::AnalysisDep &aDep) const {
  aDep.AddRequired<CgSSAConstruct>();
}
}
