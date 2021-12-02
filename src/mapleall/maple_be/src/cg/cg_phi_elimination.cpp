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
      regno_t tempMovDestRegNO = cgFunc->NewVReg(destReg.GetRegisterType(), destReg.GetSize() >> 3);
      RegOperand &tempMovDest = cgFunc->CreateVirtualRegisterOperand(tempMovDestRegNO);
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
        PlaceMovInPredBB(fBBId, CreateMoveCopyRematInfo(tempMovDest, *(phiOpndIt.second)));
      }
      Insn &movInsn = CreateMoveCopyRematInfo(destReg, tempMovDest);
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
    }
  }
}

void PhiEliminate::PlaceMovInPredBB(uint32 predBBId, Insn &movInsn) {
  BB *predBB = cgFunc->GetBBFromID(predBBId);
  ASSERT(movInsn.GetOperand(kInsnSecondOpnd).IsRegister(), "unexpect operand");
  auto &prevDef = static_cast<RegOperand&>(movInsn.GetOperand(kInsnSecondOpnd));
  VRegVersion *ssaVerion = GetSSAInfo()->FindSSAVersion(prevDef.GetRegisterNumber());
  ASSERT(ssaVerion != nullptr, "find ssaInfo failed");
  Insn *defInsn = ssaVerion->GetDefInsn();
  ASSERT(defInsn != nullptr, "get def insn failed");
  if (defInsn->GetBB()->GetId() == predBB->GetId()) {
    if (ssaVerion->GetDefType() == kDefByPhi) {
      Insn *posInsn = nullptr;
      FOR_BB_INSNS(insn, predBB) {
        if (!insn->IsMachineInstruction()) {
          continue;
        }
        ASSERT(insn != nullptr, "empty bb");
        if (!eliminatedBB.count(predBB->GetId())) {
          ASSERT(insn != nullptr, "unexpected null posInsn");
          predBB->InsertInsnBefore(*insn, movInsn);
          return;
        }
        if (insn->IsComment() || insn->IsMove()) {
          posInsn = insn;
          continue;
        }
        break;
      }
      if (posInsn == nullptr) { /* phi self depend */
        AppendMovAfterLastVregDef(*predBB, movInsn);
      } else {
        predBB->InsertInsnAfter(*posInsn, movInsn);
      }
    } else {
      ASSERT(ssaVerion->GetDefType() != kDefByNo, "Get DefType failed");
      predBB->InsertInsnAfter(*ssaVerion->GetDefInsn(), movInsn);
    }
  } else {
    if (predBB->GetKind() == BB::kBBFallthru) {
      predBB->AppendInsn(movInsn);
    } else {
      AppendMovAfterLastVregDef(*predBB, movInsn);
    }
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