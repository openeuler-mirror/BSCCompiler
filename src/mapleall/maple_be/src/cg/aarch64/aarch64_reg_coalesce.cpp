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
#include "aarch64_reg_coalesce.h"
#include "cg.h"
#include "cg_option.h"
#include "aarch64_isa.h"
#include "aarch64_insn.h"
#include "aarch64_cgfunc.h"
#include "aarch64_cg.h"

/*
 * This phase implements if-conversion optimization,
 * which tries to convert conditional branches into cset/csel instructions
 */
namespace maplebe {

#define REGCOAL_DUMP CG_DEBUG_FUNC(*cgFunc)

bool AArch64RegisterCoalesce::IsSimpleMov(Insn &insn) {
  if (insn.GetMachineOpcode() == MOP_xmovrr || insn.GetMachineOpcode() == MOP_wmovrr ||
      insn.GetMachineOpcode() == MOP_xvmovs || insn.GetMachineOpcode() == MOP_xvmovd) {
    return true;
  }
  return false;
}

bool AArch64RegisterCoalesce::IsUnconcernedReg(const RegOperand &regOpnd) const {
  RegType regType = regOpnd.GetRegisterType();
  if (regType == kRegTyCc || regType == kRegTyVary) {
    return true;
  }
  if (regOpnd.IsConstReg()) {
    return true;
  }
  if (!regOpnd.IsVirtualRegister()) {
    return true;
  }
  return false;
}

LiveInterval *AArch64RegisterCoalesce::GetOrCreateLiveInterval(regno_t regNO) {
  LiveInterval *lr = GetLiveInterval(regNO);
  if (lr == nullptr) {
    lr = memPool->New<LiveInterval>(alloc);
    vregIntervals[regNO] = lr;
    lr->SetRegNO(regNO);
  }
  return lr;
}

void AArch64RegisterCoalesce::UpdateCallInfo(uint32 bbId, uint32 currPoint) {
  for (auto vregNO : vregLive) {
    LiveInterval *lr = GetLiveInterval(vregNO);
    lr->IncNumCall();
  }
}

void AArch64RegisterCoalesce::SetupLiveIntervalByOp(Operand &op, Insn &insn, bool isDef) {
  if (!op.IsRegister()) {
    return;
  }
  auto &regOpnd = static_cast<RegOperand&>(op);
  uint32 regNO = regOpnd.GetRegisterNumber();
  if (IsUnconcernedReg(regOpnd)) {
    return;
  }
  LiveInterval *lr = GetOrCreateLiveInterval(regNO);
  uint32 point = isDef ? insn.GetId() : (insn.GetId() - 1);
  lr->AddRange(insn.GetBB()->GetId(), point, vregLive.find(regNO) != vregLive.end());
  if (lr->GetRegType() == kRegTyUndef) {
    lr->SetRegType(regOpnd.GetRegisterType());
  }
  if (candidates.find(regNO) != candidates.end()) {
    lr->AddRefPoint(&insn, isDef);
  }
  if (isDef) {
    vregLive.erase(regNO);
  } else {
    vregLive.insert(regNO);
  }
}

void AArch64RegisterCoalesce::ComputeLiveIntervalsForEachDefOperand(Insn &insn) {
  const AArch64MD *md = &AArch64CG::kMd[static_cast<AArch64Insn&>(insn).GetMachineOpcode()];
  uint32 opndNum = insn.GetOperandSize();
  for (uint32 i = 0; i < opndNum; ++i) {
    if (insn.GetMachineOpcode() == MOP_asm && (i == kAsmOutputListOpnd || i == kAsmClobberListOpnd)) {
      for (auto opnd : static_cast<ListOperand &>(insn.GetOperand(i)).GetOperands()) {
        SetupLiveIntervalByOp(*static_cast<RegOperand *>(opnd), insn, true);
      }
      continue;
    }
    Operand &opnd = insn.GetOperand(i);
    if (opnd.IsMemoryAccessOperand()) {
      auto &memOpnd = static_cast<AArch64MemOperand&>(opnd);
      if (!memOpnd.IsIntactIndexed()) {
        SetupLiveIntervalByOp(opnd, insn, true);
      }
    }
    if (!md->GetOperand(i)->IsRegDef()) {
      continue;
    }
    SetupLiveIntervalByOp(opnd, insn, true);
  }
}

void AArch64RegisterCoalesce::ComputeLiveIntervalsForEachUseOperand(Insn &insn) {
  const AArch64MD *md = &AArch64CG::kMd[static_cast<AArch64Insn&>(insn).GetMachineOpcode()];
  uint32 opndNum = insn.GetOperandSize();
  for (uint32 i = 0; i < opndNum; ++i) {
    if (insn.GetMachineOpcode() == MOP_asm && i == kAsmInputListOpnd) {
      for (auto opnd : static_cast<ListOperand &>(insn.GetOperand(i)).GetOperands()) {
        SetupLiveIntervalByOp(*static_cast<RegOperand *>(opnd), insn, false);
      }
      continue;
    }
    if (md->GetOperand(i)->IsRegDef() && !md->GetOperand(i)->IsRegUse()) {
      continue;
    }
    Operand &opnd = insn.GetOperand(i);
    if (opnd.IsList()) {
      auto &listOpnd = static_cast<ListOperand&>(opnd);
      for (auto op : listOpnd.GetOperands()) {
        SetupLiveIntervalByOp(*op, insn, false);
      }
    } else if (opnd.IsMemoryAccessOperand()) {
      auto &memOpnd = static_cast<AArch64MemOperand&>(opnd);
      Operand *base = memOpnd.GetBaseRegister();
      Operand *offset = memOpnd.GetIndexRegister();
      if (base != nullptr) {
        SetupLiveIntervalByOp(*base, insn, false);
      }
      if (offset != nullptr) {
        SetupLiveIntervalByOp(*offset, insn, false);
      }
    } else {
      SetupLiveIntervalByOp(opnd, insn, false);
    }
  }
}

/* handle live range for bb->live_out */
void AArch64RegisterCoalesce::SetupLiveIntervalInLiveOut(regno_t liveOut, BB &bb, uint32 currPoint) {
  --currPoint;

  if (liveOut >= kAllRegNum) {
    (void)vregLive.insert(liveOut);
    LiveInterval *lr = GetOrCreateLiveInterval(liveOut);
    lr->AddRange(bb.GetId(), currPoint, false);
    return;
  }
}

void AArch64RegisterCoalesce::CollectCandidate() {
  for (size_t bbIdx = bfs->sortedBBs.size(); bbIdx > 0; --bbIdx) {
    BB *bb = bfs->sortedBBs[bbIdx - 1];

    FOR_BB_INSNS_SAFE(insn, bb, ninsn) {
      if (!insn->IsMachineInstruction()) {
        continue;
      }
      if (IsSimpleMov(*insn)) {
        RegOperand &regDest = static_cast<RegOperand&>(insn->GetOperand(kInsnFirstOpnd));
        RegOperand &regSrc = static_cast<RegOperand&>(insn->GetOperand(kInsnSecondOpnd));
        if (regDest.GetRegisterNumber() == regSrc.GetRegisterNumber()) {
          continue;
        }
        if (regDest.IsVirtualRegister() && !(static_cast<AArch64CGFunc*>(cgFunc)->IsRegRematCand(regDest))) {
          candidates.insert(regDest.GetRegisterNumber());
        }
        if (regSrc.IsVirtualRegister() && !(static_cast<AArch64CGFunc*>(cgFunc)->IsRegRematCand(regSrc))) {
          candidates.insert(regSrc.GetRegisterNumber());
        }
      }
    }
  }
}

void AArch64RegisterCoalesce::ComputeLiveIntervals() {
  /* colloct refpoints and build interfere only for cands. */
  CollectCandidate();

  uint32 currPoint = cgFunc->GetTotalNumberOfInstructions() + bfs->sortedBBs.size();
  /* distinguish use/def */
  CHECK_FATAL(currPoint < (INT_MAX >> 2), "integer overflow check");
  currPoint = currPoint << 2;
  for (size_t bbIdx = bfs->sortedBBs.size(); bbIdx > 0; --bbIdx) {
    BB *bb = bfs->sortedBBs[bbIdx - 1];

    vregLive.clear();
    for (auto liveOut : bb->GetLiveOutRegNO()) {
      SetupLiveIntervalInLiveOut(liveOut, *bb, currPoint);
    }
    --currPoint;

    if (bb->GetLastInsn() != nullptr && bb->GetLastInsn()->IsCall()) {
      UpdateCallInfo(bb->GetId(), currPoint);
    }

    FOR_BB_INSNS_REV_SAFE(insn, bb, ninsn) {
      insn->SetId(currPoint);
      if (!insn->IsMachineInstruction()) {
        --currPoint;
        if (ninsn != nullptr && ninsn->IsCall()) {
          UpdateCallInfo(bb->GetId(), currPoint);
        }
        continue;
      }

      ComputeLiveIntervalsForEachDefOperand(*insn);
      ComputeLiveIntervalsForEachUseOperand(*insn);

      if (ninsn != nullptr && ninsn->IsCall()) {
        UpdateCallInfo(bb->GetId(), currPoint - 2);
      }

      /* distinguish use/def */
      currPoint -= 2;
    }
    for (auto lin : bb->GetLiveInRegNO()) {
      if (lin >= kAllRegNum) {
        LiveInterval *li = GetLiveInterval(lin);
        if (li != nullptr) {
          li->AddRange(bb->GetId(), currPoint, currPoint);
        }
      }
    }
    /* move one more step for each BB */
    --currPoint;
  }

  if (REGCOAL_DUMP) {
    LogInfo::MapleLogger() << "After ComputeLiveIntervals\n";
    Dump();
  }
}

void AArch64RegisterCoalesce::CheckInterference(LiveInterval &li1, LiveInterval &li2) {
  auto ranges1 = li1.GetRanges();
  auto ranges2 = li2.GetRanges();
  bool conflict = false;
  for (auto range : ranges1) {
    auto bbid = range.first;
    auto posVec1 = range.second;
    auto it = ranges2.find(bbid);
    if (it == ranges2.end()) {
      continue;
    } else {
      /* check overlap */
      auto posVec2 = it->second;
      for (auto pos1 : posVec1) {
        for (auto pos2 : posVec2) {
          if (!((pos1.first < pos2.first && pos1.second < pos2.first) ||
              (pos2.first < pos1.second && pos2.second < pos1.first))) {
            conflict = true;
            break;
          }
        }
      }
    }
  }
  if (conflict) {
    li1.AddConflict(li2.GetRegNO());
    li2.AddConflict(li1.GetRegNO());
  }
  return;
}

/* replace regDest with regSrc. */
void AArch64RegisterCoalesce::CoalesceRegPair(RegOperand &regDest, RegOperand &regSrc) {
  LiveInterval *lrDest = GetLiveInterval(regDest.GetRegisterNumber());
  LiveInterval *lrSrc = GetLiveInterval(regSrc.GetRegisterNumber());
  /* replace dest with src */

  /* replace all refPoints */
  for (auto insn : lrDest->GetDefPoint()) {
    cgFunc->ReplaceOpndInInsn(regDest, regSrc, *insn);
  }
  for (auto insn : lrDest->GetUsePoint()) {
    cgFunc->ReplaceOpndInInsn(regDest, regSrc, *insn);
  }

  /* merge destlr to srclr */
  lrSrc->MergeRanges(*lrDest);

  /* update conflicts */
  lrSrc->MergeConflict(*lrDest);
  for (auto reg : lrDest->GetConflict()) {
    LiveInterval *conf = GetLiveInterval(reg);
    if (conf) {
      conf->AddConflict(lrSrc->GetRegNO());
    }
  }

  /* merge refpoints */
  lrSrc->MergeRefPoints(*lrDest);

  vregIntervals.erase(lrDest->GetRegNO());
}

void AArch64RegisterCoalesce::CollectMoveForEachBB(BB &bb, std::vector<Insn*> &movInsns) {
  AArch64CGFunc *a64CGFunc = static_cast<AArch64CGFunc*>(cgFunc);
  FOR_BB_INSNS_SAFE(insn, &bb, ninsn) {
    if (!insn->IsMachineInstruction()) {
      continue;
    }
    if (IsSimpleMov(*insn)) {
      RegOperand &regDest = static_cast<RegOperand &>(insn->GetOperand(kInsnFirstOpnd));
      RegOperand &regSrc = static_cast<RegOperand &>(insn->GetOperand(kInsnSecondOpnd));
      if (!regSrc.IsVirtualRegister() || !regDest.IsVirtualRegister()) {
        continue;
      }
      if (regSrc.GetRegisterNumber() == regDest.GetRegisterNumber()) {
        continue;
      }
      if (a64CGFunc->IsRegRematCand(regDest)) {
        continue;
      }
      if (a64CGFunc->IsRegRematCand(regSrc)) {
        continue;
      }
      movInsns.emplace_back(insn);
    }
  }
}

void AArch64RegisterCoalesce::CoalesceMoves(std::vector<Insn*> &movInsns) {
  AArch64CGFunc *a64CGFunc = static_cast<AArch64CGFunc*>(cgFunc);
  bool changed = false;
  do {
    changed = false;
    for (auto insn : movInsns) {
      RegOperand &regDest = static_cast<RegOperand &>(insn->GetOperand(kInsnFirstOpnd));
      RegOperand &regSrc = static_cast<RegOperand &>(insn->GetOperand(kInsnSecondOpnd));
      if (regSrc.GetRegisterNumber() == regDest.GetRegisterNumber()) {
        continue;
      }
      if (a64CGFunc->IsRegRematCand(regDest)) {
        continue;
      }
      if (a64CGFunc->IsRegRematCand(regSrc)) {
        continue;
      }
      LiveInterval *li1 = GetLiveInterval(regDest.GetRegisterNumber());
      LiveInterval *li2 = GetLiveInterval(regSrc.GetRegisterNumber());
      CheckInterference(*li1, *li2);
      if (!li1->IsConflictWith(regSrc.GetRegisterNumber()) ||
          (li1->GetDefPoint().size() == 1 && li2->GetDefPoint().size() == 1)) {
        if (REGCOAL_DUMP) {
          LogInfo::MapleLogger() << "try to coalesce: " << regDest.GetRegisterNumber() << " <- "
                                 << regSrc.GetRegisterNumber() << std::endl;
        }
        CoalesceRegPair(regDest, regSrc);
        changed = true;
      }
    }
  } while (changed);
}

void AArch64RegisterCoalesce::CoalesceRegisters() {
  std::vector<Insn*> movInsns;
  AArch64CGFunc *a64CGFunc = static_cast<AArch64CGFunc*>(cgFunc);
  if (REGCOAL_DUMP) {
    cgFunc->DumpCFGToDot("regcoal-");
    LogInfo::MapleLogger() << "handle function: " << a64CGFunc->GetFunction().GetName() << std::endl;
  }
  for (size_t bbIdx = bfs->sortedBBs.size(); bbIdx > 0; --bbIdx) {
    BB *bb = bfs->sortedBBs[bbIdx - 1];

    if (bb->GetCritical() == false) {
      continue;
    }
    CollectMoveForEachBB(*bb, movInsns);
  }
  for (size_t bbIdx = bfs->sortedBBs.size(); bbIdx > 0; --bbIdx) {
    BB *bb = bfs->sortedBBs[bbIdx - 1];

    if (bb->GetCritical() == true) {
      continue;
    }
    CollectMoveForEachBB(*bb, movInsns);
  }
  CoalesceMoves(movInsns);

  /* clean up dead mov */
  a64CGFunc->CleanupDeadMov();
}

}  /* namespace maplebe */
