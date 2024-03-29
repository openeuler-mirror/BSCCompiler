/*
 * Copyright (c) [2022] Huawei Technologies Co., Ltd. All rights reserved.
 *
 * OpenArkCompiler is licensed under the Mulan Permissive Software License v2.
 * You can use this software according to the terms and conditions of the MulanPSL - 2.0.
 * You may obtain a copy of MulanPSL - 2.0 at:
 *
 *   https://opensource.org/licenses/MulanPSL-2.0
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR
 * FIT FOR A PARTICULAR PURPOSE.
 * See the MulanPSL - 2.0 for more details.
 */

#include "tailcall.h"
#include "cgfunc.h"

namespace maplebe {
using namespace maple;

/*
 *  Remove redundant mov and mark optimizable bl/blr insn in the BB.
 *  Return value: true to call this modified block again.
 */
bool TailCallOpt::OptimizeTailBB(BB &bb, MapleSet<Insn*, InsnIdCmp> &callInsns) const {
  Insn *lastInsn = bb.GetLastInsn();
  if (bb.NumInsn() == 1 && lastInsn->IsMachineInstruction() &&
      !AArch64isa::IsPseudoInstruction(lastInsn->GetMachineOpcode()) && !InsnIsCallCand(*bb.GetLastInsn())) {
    return false;
  }
  FOR_BB_INSNS_REV_SAFE(insn, &bb, prevInsn) {
    if (!insn->IsMachineInstruction() || AArch64isa::IsPseudoInstruction(insn->GetMachineOpcode())) {
      continue;
    }
    if (InsnIsLoadPair(*insn)) {
      if (bb.GetKind() == BB::kBBReturn) {
        RegOperand &reg = static_cast<RegOperand &>(insn->GetOperand(0));
        if (OpndIsCalleeSaveReg(reg)) {
          continue;  /* inserted restore from calleeregs-placement, ignore */
        }
      }
      return false;
    } else if (InsnIsMove(*insn)) {
      CHECK_FATAL(insn->GetOperand(0).IsRegister(), "operand0 is not register");
      CHECK_FATAL(insn->GetOperand(1).IsRegister(), "operand1 is not register");
      auto &reg1 = static_cast<RegOperand&>(insn->GetOperand(0));
      auto &reg2 = static_cast<RegOperand&>(insn->GetOperand(1));
      if (!OpndIsR0Reg(reg1) || !OpndIsR0Reg(reg2)) {
        return false;
      }
      bb.RemoveInsn(*insn);
      continue;
    } else if (InsnIsIndirectCall(*insn) && insn->GetMayTailCall()) {
      if (insn->GetOperand(0).IsRegister()) {
        RegOperand &reg = static_cast<RegOperand&>(insn->GetOperand(0));
        if (OpndIsCalleeSaveReg(reg)) {
          return false;  /* can't tailcall, register will be overwritten by restore */
        }
      }
      (void)callInsns.insert(insn);
      return false;
    } else if (InsnIsCall(*insn) && insn->GetMayTailCall()) {
      (void)callInsns.insert(insn);
      return false;
    } else if (InsnIsUncondJump(*insn)) {
      continue;
    } else {
      return false;
    }
  }
  return true;
}

/* Recursively invoke this function for all predecessors of exitBB */
void TailCallOpt::TailCallBBOpt(BB &bb, MapleSet<Insn*, InsnIdCmp> &callInsns, BB &exitBB) {
  /* callsite also in the return block as in "if () return; else foo();"
     call in the exit block */
  if (!bb.IsEmpty() && !OptimizeTailBB(bb, callInsns)) {
    return;
  }

  for (auto tmpBB : bb.GetPreds()) {
    if (tmpBB->GetId() == bb.GetId() || tmpBB->GetSuccs().size() != 1 || !tmpBB->GetEhSuccs().empty() ||
        (tmpBB->GetKind() != BB::kBBFallthru && tmpBB->GetKind() != BB::kBBGoto)) {
      continue;
    }

    if (OptimizeTailBB(*tmpBB, callInsns)) {
      TailCallBBOpt(*tmpBB, callInsns, exitBB);
    }
  }
}

/*
 *  If a function without callee-saved register, and end with a function call,
 *  then transfer bl/blr to b/br.
 *  Return value: true if function do not need Prologue/Epilogue. false otherwise.
 */
bool TailCallOpt::DoTailCallOpt() {
  /* Count how many call insns in the whole function. */
  uint32 nCount = 0;
  bool hasGetStackClass = false;

  FOR_ALL_BB(bb, &cgFunc) {
    FOR_BB_INSNS(insn, bb) {
      if (insn->IsCall()) {
        /*
         * lib call "savectx, vfork, getcontext" which might cause fault
         * not in whitelist yet
         */
        if (InsnIsCall(*insn) && IsFuncNeedFrame(*insn)) {
          hasGetStackClass = true;
        }

        ++nCount;
      }
    }
  }
  if ((nCount > 0 && cgFunc.GetFunction().GetAttr(FUNCATTR_interface)) || hasGetStackClass) {
    return false;
  }

  if (nCount == 0) {
    // no bl instr in any bb
    return true;
  }

  size_t exitBBSize = cgFunc.GetExitBBsVec().size();
  /* For now to reduce complexity */

  BB *exitBB = nullptr;
  if (exitBBSize == 0) {
    if (cgFunc.GetCleanupBB() != nullptr && cgFunc.GetCleanupBB()->GetPrev() != nullptr) {
      exitBB = cgFunc.GetCleanupBB()->GetPrev();
    } else {
      exitBB = cgFunc.GetLastBB();
    }
  } else {
    exitBB = cgFunc.GetExitBBsVec().front();
  }
  uint32 i = 1;
  size_t optCount = 0;
  do {
    MapleSet<Insn*, InsnIdCmp> callInsns(tmpAlloc.Adapter());
    TailCallBBOpt(*exitBB, callInsns, *exitBB);
    if (callInsns.size() != 0) {
      optCount += callInsns.size();
      exitBB2CallSitesMap.emplace(exitBB, callInsns);
    }
    if (i < exitBBSize) {
      exitBB = cgFunc.GetExitBBsVec()[i];
      ++i;
    } else {
      break;
    }
  } while (true);

  /* unequal means regular calls exist in function */
  return nCount == optCount;
}

void TailCallOpt::ConvertToTailCalls(MapleSet<Insn*, InsnIdCmp> &callInsnsMap) {
  BB *exitBB = GetCurTailcallExitBB();

  FOR_BB_INSNS(insn, exitBB) {
    if (InsnIsAddWithRsp(*insn)) {
      return;
    }
  }

  /* Replace all of the call insns. */
  for (Insn *callInsn : callInsnsMap) {
    ReplaceInsnMopWithTailCall(*callInsn);
    BB *bb = callInsn->GetBB();
    if (bb->GetKind() == BB::kBBGoto) {
      bb->SetKind(BB::kBBFallthru);
      if (InsnIsUncondJump(*bb->GetLastInsn())) {
        bb->RemoveInsn(*bb->GetLastInsn());
      }
    }
    ASSERT(bb->GetSuccs().size() <= 1, "expect no succ or single succ");
    for (auto sBB: bb->GetSuccs()) {
      bb->RemoveSuccs(*sBB);
      sBB->RemovePreds(*bb);
      bb->SetKind(BB::kBBReturn);
      cgFunc.PushBackExitBBsVec(*bb);
      cgFunc.GetCommonExitBB()->PushBackPreds(*bb);
        /* if next bb is exit BB */
      if (sBB->GetKind() == BB::kBBReturn && sBB->GetPreds().empty() &&
          !CGCFG::InSwitchTable(sBB->GetLabIdx(), cgFunc)) {
        auto it = std::find(cgFunc.GetExitBBsVec().begin(), cgFunc.GetExitBBsVec().end(), sBB);
        CHECK_FATAL(it != cgFunc.GetExitBBsVec().end(), "find unuse exit failed");
        (void)cgFunc.EraseExitBBsVec(it);
        cgFunc.GetTheCFG()->RemoveBB(*sBB);
      }
    }
  }
}

void TailCallOpt::TideExitBB() {
  cgFunc.GetTheCFG()->UnreachCodeAnalysis();
  std::vector<BB*> realRets;
  for (auto *exitBB : cgFunc.GetExitBBsVec()) {
    if (!exitBB->GetPreds().empty() || exitBB == cgFunc.GetFirstBB()) {
      (void)realRets.emplace_back(exitBB);
    }
  }
  cgFunc.ClearExitBBsVec();
  for (auto *cand: realRets) {
    cgFunc.PushBackExitBBsVec(*cand);
  }
}

void TailCallOpt::Run() {
  if (CGOptions::DoTailCallOpt()) {
    (void)DoTailCallOpt(); // return value == "no call instr/only or 1 tailcall"
  }
  if (cgFunc.GetMirModule().IsCModule() && !exitBB2CallSitesMap.empty()) {
    cgFunc.GetTheCFG()->InitInsnVisitor(cgFunc);
    for (auto pair : exitBB2CallSitesMap) {
      BB *curExitBB = pair.first;
      MapleSet<Insn*, InsnIdCmp>& callInsnsMap = pair.second;
      SetCurTailcallExitBB(curExitBB);
      ConvertToTailCalls(callInsnsMap);
    }
    TideExitBB();
  }
}

bool CgTailCallOpt::PhaseRun(maplebe::CGFunc &f) {
  TailCallOpt *tailCallOpt = f.GetCG()->CreateCGTailCallOpt(*GetPhaseMemPool(), f);
  tailCallOpt->Run();
  return false;
}

MAPLE_TRANSFORM_PHASE_REGISTER_CANSKIP(CgTailCallOpt, tailcallopt)
}  /* namespace maplebe */
