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
#include "aarch64_proepilog.h"
#include "cg_option.h"
#include "cgfunc.h"

namespace maplebe {
using namespace maple;

namespace {
const std::set<std::string> kFrameWhiteListFunc {
#include "framewhitelist.def"
};

bool IsFuncNeedFrame(const std::string &funcName) {
  return kFrameWhiteListFunc.find(funcName) != kFrameWhiteListFunc.end();
}
constexpr int32 kSoeChckOffset = 8192;

enum RegsPushPop : uint8 {
  kRegsPushOp,
  kRegsPopOp
};

enum PushPopType : uint8 {
  kPushPopSingle = 0,
  kPushPopPair = 1
};

MOperator pushPopOps[kRegsPopOp + 1][kRegTyFloat + 1][kPushPopPair + 1] = {
  { /* push */
    { 0 /* undef */ },
    { /* kRegTyInt */
      MOP_xstr, /* single */
      MOP_xstp, /* pair   */
    },
    { /* kRegTyFloat */
      MOP_dstr, /* single */
      MOP_dstp, /* pair   */
    },
  },
  { /* pop */
    { 0 /* undef */ },
    { /* kRegTyInt */
      MOP_xldr, /* single */
      MOP_xldp, /* pair   */
    },
    { /* kRegTyFloat */
      MOP_dldr, /* single */
      MOP_dldp, /* pair   */
    },
  }
};

inline void AppendInstructionTo(Insn &insn, CGFunc &func) {
  func.GetCurBB()->AppendInsn(insn);
}
}

bool AArch64GenProEpilog::HasLoop() {
  FOR_ALL_BB(bb, &cgFunc) {
    if (bb->IsBackEdgeDest()) {
      return true;
    }
    FOR_BB_INSNS_REV(insn, bb) {
      if (!insn->IsMachineInstruction()) {
        continue;
      }
      if (insn->HasLoop()) {
        return true;
      }
    }
  }
  return false;
}

/*
 *  Remove redundant mov and mark optimizable bl/blr insn in the BB.
 *  Return value: true if is empty bb, otherwise false.
 */
bool AArch64GenProEpilog::OptimizeTailBB(BB &bb, std::set<Insn*> &callInsns) {
  FOR_BB_INSNS_REV_SAFE(insn, &bb, prev_insn) {
    if (!insn->IsMachineInstruction()) {
      continue;
    }
    MOperator insnMop = insn->GetMachineOpcode();
    switch (insnMop) {
      case MOP_wmovrr:
      case MOP_xmovrr: {
        CHECK_FATAL(insn->GetOperand(0).IsRegister(), "operand0 is not register");
        CHECK_FATAL(insn->GetOperand(1).IsRegister(), "operand1 is not register");
        auto &reg1 = static_cast<AArch64RegOperand&>(insn->GetOperand(0));
        auto &reg2 = static_cast<AArch64RegOperand&>(insn->GetOperand(1));

        if (reg1.GetRegisterNumber() != R0 || reg2.GetRegisterNumber() != R0) {
          return false;
        }

        bb.RemoveInsn(*insn);
        break;
      }
      case MOP_xbl:
      case MOP_xblr: {
        (void)callInsns.insert(insn);
        return false;
      }
      default:
        return false;
    }
  }

  return true;
}

/* Recursively invoke this function until exitBB's precursor not exist. */
void AArch64GenProEpilog::TailCallBBOpt(const BB &exitBB, std::set<Insn*> &callInsns) {
  for (auto tmpBB : exitBB.GetPreds()) {
    if (tmpBB->GetSuccs().size() != 1 || !tmpBB->GetEhSuccs().empty() || tmpBB->GetKind() != BB::kBBFallthru) {
      continue;
    }

    if (OptimizeTailBB(*tmpBB, callInsns)) {
      TailCallBBOpt(*tmpBB, callInsns);
    }
  }
}

/*
 *  If a function without callee-saved register, and end with a function call,
 *  then transfer bl/blr to b/br.
 *  Return value: true if function do not need Prologue/Epilogue. false otherwise.
 */
bool AArch64GenProEpilog::TailCallOpt() {
  /* Count how many call insns in the whole function. */
  uint32 nCount = 0;
  bool hasGetStackClass = false;

  FOR_ALL_BB(bb, &cgFunc) {
    FOR_BB_INSNS(insn, bb) {
      if (insn->IsCall()) {
        if (insn->GetMachineOpcode() == MOP_xbl) {
          auto &target = static_cast<FuncNameOperand&>(insn->GetOperand(0));
          if (IsFuncNeedFrame(target.GetName())) {
            hasGetStackClass = true;
          }
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
  if (exitBBSize > 1) {
    return false;
  }

  BB *exitBB = nullptr;
  if (exitBBSize == 0) {
    if (cgFunc.GetLastBB()->GetPrev()->GetFirstStmt() == cgFunc.GetCleanupLabel() &&
        cgFunc.GetLastBB()->GetPrev()->GetPrev() != nullptr) {
      exitBB = cgFunc.GetLastBB()->GetPrev()->GetPrev();
    } else {
      exitBB = cgFunc.GetLastBB()->GetPrev();
    }
  } else {
    exitBB = cgFunc.GetExitBBsVec().front();
  }

  CHECK_FATAL(exitBB->GetFirstMachineInsn() == nullptr, "exit bb should be empty.");

  std::set<Insn*> callInsns;
  TailCallBBOpt(*exitBB, callInsns);

  if (nCount != callInsns.size()) {
    return false;
  }
  /* Replace all of the call insns. */
  for (auto callInsn : callInsns) {
    MOperator insnMop = callInsn->GetMachineOpcode();
    switch (insnMop) {
      case MOP_xbl: {
        callInsn->SetMOP(MOP_tail_call_opt_xbl);
        break;
      }
      case MOP_xblr: {
        callInsn->SetMOP(MOP_tail_call_opt_xblr);
        break;
      }
      default:
        ASSERT(false, "Internal error.");
        break;
    }
  }
  return true;
}

bool AArch64GenProEpilog::NeedProEpilog() {
  if (cgFunc.GetMirModule().GetSrcLang() != kSrcLangC) {
    return true;
  } else if (static_cast<AArch64MemLayout *>(cgFunc.GetMemlayout())->GetSizeOfLocals() > 0 ||
             cgFunc.GetFunction().GetAttr(FUNCATTR_varargs) || cgFunc.HasVLAOrAlloca()) {
    return true;
  }
  auto &aarchCGFunc = static_cast<AArch64CGFunc&>(cgFunc);
  const MapleVector<AArch64reg> &regsToRestore = aarchCGFunc.GetCalleeSavedRegs();
  size_t calleeSavedRegSize = kTwoRegister;
  CHECK_FATAL(regsToRestore.size() >= calleeSavedRegSize, "Forgot FP and LR ?");
  if (regsToRestore.size() > calleeSavedRegSize || aarchCGFunc.HasStackLoadStore() ||
      cgFunc.GetFunction().GetAttr(FUNCATTR_callersensitive)) {
    return true;
  }
  if (cgFunc.GetCG()->DoPrologueEpilogue()) {
    return !TailCallOpt();
  } else {
    FOR_ALL_BB(bb, &cgFunc) {
      FOR_BB_INSNS_REV(insn, bb) {
        if (insn->IsCall()) {
          return true;
        }
      }
    }
  }
  return false;
}

void AArch64GenProEpilog::GenStackGuard(BB &bb) {
  auto &aarchCGFunc = static_cast<AArch64CGFunc&>(cgFunc);
  CG *currCG = cgFunc.GetCG();
  if (currCG->AddStackGuard()) {
    BB *formerCurBB = cgFunc.GetCurBB();
    aarchCGFunc.GetDummyBB()->ClearInsns();
    aarchCGFunc.GetDummyBB()->SetIsProEpilog(true);
    cgFunc.SetCurBB(*aarchCGFunc.GetDummyBB());

    MIRSymbol *stkGuardSym = GlobalTables::GetGsymTable().GetSymbolFromStrIdx(
        GlobalTables::GetStrTable().GetStrIdxFromName(std::string("__stack_chk_guard")));
    StImmOperand &stOpnd = aarchCGFunc.CreateStImmOperand(*stkGuardSym, 0, 0);
    AArch64RegOperand &stAddrOpnd =
        aarchCGFunc.GetOrCreatePhysicalRegisterOperand(R9, kSizeOfPtr * kBitsPerByte, kRegTyInt);
    aarchCGFunc.SelectAddrof(stAddrOpnd, stOpnd);

    AArch64MemOperand *guardMemOp =
        aarchCGFunc.GetMemoryPool()->New<AArch64MemOperand>(AArch64MemOperand::kAddrModeBOi, kSizeOfPtr * kBitsPerByte,
            stAddrOpnd, nullptr, &aarchCGFunc.GetOrCreateOfstOpnd(0, k32BitSize), stkGuardSym);
    MOperator mOp = aarchCGFunc.PickLdInsn(k64BitSize, PTY_u64);
    Insn &insn = currCG->BuildInstruction<AArch64Insn>(mOp, stAddrOpnd, *guardMemOp);
    insn.SetDoNotRemove(true);
    cgFunc.GetCurBB()->AppendInsn(insn);

    int vArea = 0;
    if (cgFunc.GetMirModule().IsCModule() && cgFunc.GetFunction().GetAttr(FUNCATTR_varargs)) {
      AArch64MemLayout *ml = static_cast<AArch64MemLayout *>(cgFunc.GetMemlayout());
      if (ml->GetSizeOfGRSaveArea() > 0) {
        vArea += RoundUp(ml->GetSizeOfGRSaveArea(), kAarch64StackPtrAlignment);
      }
      if (ml->GetSizeOfVRSaveArea() > 0) {
        vArea += RoundUp(ml->GetSizeOfVRSaveArea(), kAarch64StackPtrAlignment);
      }
    }

    int32 stkSize = static_cast<AArch64MemLayout*>(cgFunc.GetMemlayout())->RealStackFrameSize();
    if (useFP) {
      stkSize -= static_cast<AArch64MemLayout*>(cgFunc.GetMemlayout())->SizeOfArgsToStackPass();
    }
    AArch64MemOperand *downStk =
        aarchCGFunc.GetMemoryPool()->New<AArch64MemOperand>(stackBaseReg, stkSize - kOffset8MemPos - vArea,
                                                            kSizeOfPtr * kBitsPerByte);
    if (downStk->GetMemVaryType() == kNotVary &&
        aarchCGFunc.IsImmediateOffsetOutOfRange(*downStk, k64BitSize)) {
      downStk = &aarchCGFunc.SplitOffsetWithAddInstruction(*downStk, k64BitSize, R10);
    }
    mOp = aarchCGFunc.PickStInsn(kSizeOfPtr * kBitsPerByte, PTY_u64);
    Insn &tmpInsn = currCG->BuildInstruction<AArch64Insn>(mOp, stAddrOpnd, *downStk);
    tmpInsn.SetDoNotRemove(true);
    cgFunc.GetCurBB()->AppendInsn(tmpInsn);

    bb.InsertAtBeginning(*aarchCGFunc.GetDummyBB());
    aarchCGFunc.GetDummyBB()->SetIsProEpilog(false);
    cgFunc.SetCurBB(*formerCurBB);
  }
}

BB &AArch64GenProEpilog::GenStackGuardCheckInsn(BB &bb) {
  CG *currCG = cgFunc.GetCG();
  if (!currCG->AddStackGuard()) {
    return bb;
  }

  BB *formerCurBB = cgFunc.GetCurBB();
  cgFunc.GetDummyBB()->ClearInsns();
  cgFunc.SetCurBB(*(cgFunc.GetDummyBB()));
  auto &aarchCGFunc = static_cast<AArch64CGFunc&>(cgFunc);

  const MIRSymbol *stkGuardSym = GlobalTables::GetGsymTable().GetSymbolFromStrIdx(
      GlobalTables::GetStrTable().GetStrIdxFromName(std::string("__stack_chk_guard")));
  StImmOperand &stOpnd = aarchCGFunc.CreateStImmOperand(*stkGuardSym, 0, 0);
  AArch64RegOperand &stAddrOpnd = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(R9, kSizeOfPtr * kBitsPerByte,
                                                                                 kRegTyInt);
  aarchCGFunc.SelectAddrof(stAddrOpnd, stOpnd);

  AArch64MemOperand *guardMemOp =
      cgFunc.GetMemoryPool()->New<AArch64MemOperand>(AArch64MemOperand::kAddrModeBOi,
                                                     kSizeOfPtr * kBitsPerByte, stAddrOpnd, nullptr,
                                                     &aarchCGFunc.GetOrCreateOfstOpnd(0, k32BitSize),
                                                     stkGuardSym);
  MOperator mOp = aarchCGFunc.PickLdInsn(k64BitSize, PTY_u64);
  Insn &insn = currCG->BuildInstruction<AArch64Insn>(mOp, stAddrOpnd, *guardMemOp);
  insn.SetDoNotRemove(true);
  cgFunc.GetCurBB()->AppendInsn(insn);

  int vArea = 0;
  if (cgFunc.GetMirModule().IsCModule() && cgFunc.GetFunction().GetAttr(FUNCATTR_varargs)) {
    AArch64MemLayout *ml = static_cast<AArch64MemLayout *>(cgFunc.GetMemlayout());
    if (ml->GetSizeOfGRSaveArea() > 0) {
      vArea += RoundUp(ml->GetSizeOfGRSaveArea(), kAarch64StackPtrAlignment);
    }
    if (ml->GetSizeOfVRSaveArea() > 0) {
      vArea += RoundUp(ml->GetSizeOfVRSaveArea(), kAarch64StackPtrAlignment);
    }
  }

  AArch64RegOperand &checkOp =
      aarchCGFunc.GetOrCreatePhysicalRegisterOperand(R10, kSizeOfPtr * kBitsPerByte, kRegTyInt);
  int32 stkSize = static_cast<AArch64MemLayout*>(cgFunc.GetMemlayout())->RealStackFrameSize();
  if (useFP) {
    stkSize -= static_cast<AArch64MemLayout*>(cgFunc.GetMemlayout())->SizeOfArgsToStackPass();
  }
  AArch64MemOperand *downStk =
      aarchCGFunc.GetMemoryPool()->New<AArch64MemOperand>(stackBaseReg, stkSize - kOffset8MemPos - vArea,
                                                          kSizeOfPtr * kBitsPerByte);
  if (downStk->GetMemVaryType() == kNotVary && aarchCGFunc.IsImmediateOffsetOutOfRange(*downStk, k64BitSize)) {
    downStk = &aarchCGFunc.SplitOffsetWithAddInstruction(*static_cast<AArch64MemOperand*>(downStk), k64BitSize, R10);
  }
  mOp = aarchCGFunc.PickLdInsn(kSizeOfPtr * kBitsPerByte, PTY_u64);
  Insn &newInsn = currCG->BuildInstruction<AArch64Insn>(mOp, checkOp, *downStk);
  newInsn.SetDoNotRemove(true);
  cgFunc.GetCurBB()->AppendInsn(newInsn);

  cgFunc.SelectBxor(stAddrOpnd, stAddrOpnd, checkOp, PTY_u64);
  LabelIdx failLable = aarchCGFunc.CreateLabel();
  aarchCGFunc.SelectCondGoto(aarchCGFunc.GetOrCreateLabelOperand(failLable), OP_brtrue, OP_eq,
                             stAddrOpnd, aarchCGFunc.CreateImmOperand(0, k64BitSize, false), PTY_u64, false);

  MIRSymbol *failFunc = GlobalTables::GetGsymTable().GetSymbolFromStrIdx(
      GlobalTables::GetStrTable().GetStrIdxFromName(std::string("__stack_chk_fail")));
  AArch64ListOperand *srcOpnds =
      cgFunc.GetMemoryPool()->New<AArch64ListOperand>(*cgFunc.GetFuncScopeAllocator());
  Insn &callInsn = aarchCGFunc.AppendCall(*failFunc, *srcOpnds);
  callInsn.SetDoNotRemove(true);

  bb.AppendBBInsns(*(cgFunc.GetCurBB()));

  BB *newBB = cgFunc.CreateNewBB(failLable, bb.IsUnreachable(), bb.GetKind(), bb.GetFrequency());
  bb.AppendBB(*newBB);
  if (cgFunc.GetLastBB() == &bb) {
    cgFunc.SetLastBB(*newBB);
  }
  bb.SetKind(BB::kBBFallthru);
  bb.PushBackSuccs(*newBB);
  newBB->PushBackPreds(bb);

  cgFunc.SetCurBB(*formerCurBB);
  return *newBB;
}

/*
 * The following functions are for pattern matching for fast path
 * where function has early return of spedified pattern load/test/return
 */
void AArch64GenProEpilog::ReplaceMachedOperand(Insn &orig, Insn &target, const RegOperand &matchedOpnd,
                                               bool isFirstDst) {
  auto &aarchCGFunc = static_cast<AArch64CGFunc&>(cgFunc);
  for (uint32 i = 0; i < target.GetOpndNum(); ++i) {
    Operand *src = target.GetOpnd(i);
    CHECK_FATAL(src != nullptr, "src is null in ReplaceMachedOperand");
    if (src->IsRegister()) {
      RegOperand *reg = static_cast<RegOperand*>(src);
      if (reg != &matchedOpnd) {
        continue;
      }
      if (isFirstDst) {
        Operand *origSrc = orig.GetOpnd(0);
        RegOperand *origPhys = static_cast<RegOperand*>(origSrc);
        CHECK_FATAL(origPhys != nullptr, "pointer is null");
        regno_t regNO = origPhys->GetRegisterNumber();
        RegOperand &phys = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(static_cast<AArch64reg>(regNO),
                                                                          matchedOpnd.GetSize(), kRegTyInt);
        target.SetOpnd(i, phys);
      } else {
        /* Replace the operand with the src of inst */
        RegOperand &phys = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(static_cast<AArch64reg>(R16),
                                                                          reg->GetSize(), kRegTyInt);
        target.SetOpnd(i, phys);
      }
      return;
    }
    if (src->IsMemoryAccessOperand()) {
      MemOperand *memOpnd = static_cast<MemOperand*>(src);
      Operand *base = memOpnd->GetBaseRegister();
      Operand *offset = memOpnd->GetIndexRegister();
      if (base != nullptr && base->IsRegister()) {
        RegOperand *reg = static_cast<RegOperand*>(base);
        if (reg != &matchedOpnd) {
          continue;
        }
        if (isFirstDst) {
          Operand *origSrc = orig.GetOpnd(0);
          RegOperand *origPhys = static_cast<RegOperand*>(origSrc);
          CHECK_FATAL(origPhys != nullptr, "orig_phys cast failed");
          regno_t regNO = origPhys->GetRegisterNumber();
          RegOperand &phys = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(static_cast<AArch64reg>(regNO),
                                                                            matchedOpnd.GetSize(), kRegTyInt);
          memOpnd->SetBaseRegister(phys);
        } else {
          /* Replace the operand with the src of inst */
          RegOperand &phys =
              aarchCGFunc.GetOrCreatePhysicalRegisterOperand(static_cast<AArch64reg>(R16), base->GetSize(), kRegTyInt);
          memOpnd->SetBaseRegister(phys);
        }
        return;
      }
      if (offset != nullptr && offset->IsRegister()) {
        RegOperand *reg = static_cast<RegOperand*>(offset);
        if (reg != &matchedOpnd) {
          continue;
        }
        if (isFirstDst) {
          Operand *origSrc = orig.GetOpnd(0);
          RegOperand *origPhys = static_cast<RegOperand*>(origSrc);
          CHECK_FATAL(origPhys != nullptr, "orig_phys is nullptr in ReplaceMachedOperand");
          regno_t regNO = origPhys->GetRegisterNumber();
          RegOperand &phys = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(static_cast<AArch64reg>(regNO),
                                                                            matchedOpnd.GetSize(), kRegTyInt);
          memOpnd->SetIndexRegister(phys);
        } else {
          /* Replace the operand with the src of inst */
          RegOperand &phys = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(static_cast<AArch64reg>(R16),
                                                                            offset->GetSize(), kRegTyInt);
          memOpnd->SetIndexRegister(phys);
        }
        return;
      }
    }
  }
  if (!isFirstDst) {
    RegOperand &phys = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(static_cast<AArch64reg>(R16),
                                                                      matchedOpnd.GetSize(), kRegTyInt);
    orig.SetResult(0, phys);
  }
}

bool AArch64GenProEpilog::BackwardFindDependency(BB &ifbb, RegOperand &targetOpnd, Insn *&load,
                                                 Insn *&mov, Insn *&depMov, std::list<Insn*> &list) {
  load = nullptr;
  mov = nullptr;
  depMov = nullptr;
  BB *pred = &ifbb;
  /*
   * Pattern match,  (*) instruction are moved down below branch.
   *   mov reg1, R0                         mov  reg1, R0                        *
   *   ld  Rx  , [reg1, const]              ld   R16 , [R0, const]
   *   mov reg2, Rx                   =>    mov  reg2, R16    <- this may exist  *
   *                                        mov  Rx  , R16    <- replicate       *
   *   cbr       Rx, label                  cbr        R16, label
   *
   *   R16 is used because it is used as spill register.
   *   Need to modify if different register allcoation mechanism is used.
   */
  do {
    FOR_BB_INSNS_REV(insn, pred) {
      if (insn == ifbb.GetLastInsn()) {
        continue;
      }
      if (insn->IsImmaterialInsn()) {
        continue;
      }
      if (!insn->IsMachineInstruction()) {
        continue;
      }

      bool found = false;  /* allowing for only one src to be register */
      for (uint32 r = 0; r < insn->GetResultNum(); ++r) {
        Operand *dst = insn->GetResult(r);
        CHECK_FATAL(dst != nullptr, "pointer is null");
        if (!dst->IsRegister()) {
          continue;
        }
        RegOperand *regOpnd = static_cast<RegOperand*>(dst);
        if (regOpnd != &targetOpnd) {
          continue;
        }
        if (load != nullptr) {
          if (mov != nullptr) {
            return false;
          }
          MOperator opCode = insn->GetMachineOpcode();
          if (opCode != MOP_xmovrr) {
            return false;
          }
          Operand *mvSrc = insn->GetOpnd(0);
          RegOperand *mvRegSrc = static_cast<RegOperand*>(mvSrc);
          CHECK_FATAL(mvRegSrc != nullptr, "mv_regsrc cast failed");
          regno_t mvReg = mvRegSrc->GetRegisterNumber();
          /* make it very specific for now */
          if (mvReg != R0) {
            return false;
          }
          Operand *mvDst = insn->GetResult(0);
          RegOperand *mvRegDst = static_cast<RegOperand*>(mvDst);
          CHECK_FATAL(mvRegDst != nullptr, "mv_regdst cast failed");
          mvReg = mvRegDst->GetRegisterNumber();
          if (mvReg != R20) {
            return false;
          }
          mov = insn;
        }
        /* Found def, continue dep chain with src */
        for (uint32 s = 0; s < insn->GetOpndNum(); ++s) {
          Operand *src = insn->GetOpnd(s);
          CHECK_FATAL(src != nullptr, "src is nullptr in BackwardFindDependency");
          if (src->IsRegister()) {
            if (found) {
              return false;
            }
            RegOperand *preg = static_cast<RegOperand*>(src);
            targetOpnd = *preg;
            if (!preg->IsPhysicalRegister() || insn->GetMachineOpcode() != MOP_xmovrr) {
              return false;
            }
            /*
             * Skipping the start of the dependency chain because
             * the the parameter reg will be propagated leaving
             * the mov instruction alone to be relocated down
             * to the cold path.
             */
            found = false;
          } else if (src->IsMemoryAccessOperand()) {
            MemOperand *memOpnd = static_cast<MemOperand*>(src);
            Operand *base = memOpnd->GetBaseRegister();
            Operand *offset = memOpnd->GetIndexRegister();
            if (base != nullptr && base->IsRegister()) {
              if (found) {
                return false;
              }
              load = insn;
              targetOpnd = *(static_cast<RegOperand*>(base));
              found = true;
              Operand *ldDst = insn->GetResult(0);
              RegOperand *ldRdst = static_cast<RegOperand*>(ldDst);
              CHECK_FATAL(ldRdst != nullptr, "ld_rdst is nullptr in BackwardFindDependency");
              if (ldRdst->GetRegisterNumber() != R1) {
                return false;  /* hard code for now. */
              }
              /* Make sure instruction depending on load is mov and cond br */
              for (Insn *ni = insn->GetNext(); ni != nullptr; ni = ni->GetNext()) {
                if (ni->GetMachineOpcode() == MOP_xmovrr || ni->GetMachineOpcode() == MOP_wmovrr) {
                  Operand *dep = ni->GetOpnd(0);
                  RegOperand *rdep = static_cast<RegOperand*>(dep);
                  if (rdep == ldRdst) {
                    if (depMov != nullptr) {
                      return false;
                    }
                    depMov = ni;
                  }
                }
              }
            }
            if (offset != nullptr && offset->IsRegister()) {
              return false;
            }
          }
        }
      }
      if (!found) {
        list.push_back(insn);
      }
    }
    if (pred->GetPreds().empty()) {
      break;
    }
    pred = pred->GetPreds().front();
  } while (pred != nullptr);

  return true;
}

void AArch64GenProEpilog::ForwardPropagateAndRename(Insn &mov, Insn &load, const BB &terminateBB) {
  /*
   * This is specialized function to work with IsolateFastPath().
   * The control flow and instruction pattern must be recognized.
   */
  Insn *insn = &mov;
  bool isFirstDst = true;
  /* process mov and load two instructions */
  for (int32 i = 0; i < 2; ++i) {
    /* Finish the bb the mov is in */
    for (Insn *target = insn->GetNext(); target != nullptr; target = target->GetNext()) {
      if (target->IsImmaterialInsn()) {
        continue;
      }
      if (!target->IsMachineInstruction()) {
        continue;
      }
      Operand *dst = insn->GetResult(0);
      RegOperand *rdst = static_cast<RegOperand*>(dst);
      CHECK_FATAL(rdst != nullptr, "rdst is nullptr in ForwardPropagateAndRename");
      ReplaceMachedOperand(*insn, *target, *rdst, isFirstDst);
    }
    CHECK_FATAL(!insn->GetBB()->GetSuccs().empty(), "null succs check!");
    BB *bb = insn->GetBB()->GetSuccs().front();
    while (1) {
      FOR_BB_INSNS(target, bb) {
        if (!target->IsMachineInstruction()) {
          continue;
        }
        Operand *dst = insn->GetResult(0);
        RegOperand *rdst = static_cast<RegOperand*>(dst);
        CHECK_FATAL(rdst != nullptr, "rdst is nullptr in ForwardPropagateAndRename");
        ReplaceMachedOperand(*insn, *target, *rdst, isFirstDst);
      }
      if (bb == &terminateBB) {
        break;
      }
      CHECK_FATAL(!bb->GetSuccs().empty(), "null succs check!");
      bb = bb->GetSuccs().front();
    }
    insn = &load;
    isFirstDst = false;
  }
}

BB *AArch64GenProEpilog::IsolateFastPath(BB &bb) {
  /*
   * Detect "if (cond) return" fast path, and move extra instructions
   * to the slow path.
   * Must match the following block structure.  BB1 can be a series of
   * single-pred/single-succ blocks.
   *     BB1 ops1 cmp-br to BB3        BB1 cmp-br to BB3
   *     BB2 ops2 br to retBB    ==>   BB2 ret
   *     BB3 slow path                 BB3 ops1 ops2
   * BB3 will be used to generate prolog stuff.
   */
  if (bb.GetPrev() != nullptr) {
    return nullptr;
  }
  BB *ifBB = nullptr;
  BB *returnBB = nullptr;
  BB *coldBB = nullptr;
  auto &aarchCGFunc = static_cast<AArch64CGFunc&>(cgFunc);
  CG *currCG = cgFunc.GetCG();
  {
    BB &curBB = bb;
    /* Look for straight line code */
    while (1) {
      if (curBB.GetEhSuccs().empty()) {
        return nullptr;
      }
      if (curBB.GetSuccs().size() == 1) {
        if (curBB.HasCall()) {
          return nullptr;
        }
        BB *succ = curBB.GetSuccs().front();
        if (succ->GetPreds().size() != 1 || !succ->GetEhPreds().empty()) {
          return nullptr;
        }
        curBB = *succ;
      } else if (curBB.GetKind() == BB::kBBIf) {
        ifBB = &curBB;
        break;
      } else {
        return nullptr;
      }
    }
  }
  /* targets of if bb can only be reached by if bb */
  {
    CHECK_FATAL(!ifBB->GetSuccs().empty(), "null succs check!");
    BB *first = ifBB->GetSuccs().front();
    BB *second = ifBB->GetSuccs().back();
    if (first->GetPreds().size() != 1 || !first->GetEhPreds().empty()) {
      return nullptr;
    }
    if (second->GetPreds().size() != 1 || !second->GetEhPreds().empty()) {
      return nullptr;
    }

    /* One target of the if bb jumps to a return bb */
    CHECK_FATAL(!first->GetSuccs().empty(), "null succs check!");
    if (first->GetKind() != BB::kBBGoto || first->GetSuccs().front()->GetKind() != BB::kBBReturn) {
      return nullptr;
    }
    if (second->GetSuccs().empty()) {
      return nullptr;
    }
    returnBB = first;
    coldBB = second;
  }
  /*
   * The control flow matches at this point.
   * Make sure the hot bb contains atmost a
   * 'mov x0, value' and 'b'.
   */
  {
    CHECK_FATAL(returnBB != nullptr, "null ptr check");
    const int32 twoInsnInReturnBB = 2;
    if (returnBB->NumInsn() > twoInsnInReturnBB) {
      return nullptr;
    }
    Insn *first = returnBB->GetFirstInsn();
    while (first->IsImmaterialInsn()) {
      first = first->GetNext();
    }
    if (first == returnBB->GetLastInsn()) {
      if (!first->IsBranch()) {
        return nullptr;
      }
    } else {
      MOperator opCode = first->GetMachineOpcode();
      /* only allow mov constant */
      if (opCode != MOP_xmovri64 && opCode != MOP_xmovri32) {
        return nullptr;
      }
      Insn *last = returnBB->GetLastInsn();
      if (!last->IsBranch()) {
        return nullptr;
      }
    }
  }

  /*
   * Resolve any register usage issues.
   * 1) Any use of parameter registes must be renamed
   * 2) Any usage of callee saved register that needs saving in prolog
   *    must be able to move down into the cold path.
   */

  /* Find the branch's src register for backward propagation. */
  Insn *condBr = ifBB->GetLastInsn();
  auto &targetOpnd = static_cast<RegOperand&>(condBr->GetOperand(0));
  if (targetOpnd.GetRegisterType() != kRegTyInt) {
    return nullptr;
  }

  /* Search backward looking for dependencies for the cond branch */
  std::list<Insn*> insnList;  /* instructions to be moved to coldbb */
  Insn *ld = nullptr;
  Insn *mv = nullptr;
  Insn *depMv = nullptr;
  /*
   * The mv is the 1st move using the parameter register leading to the branch
   * The ld is the load using the parameter register indirectly for the branch
   * The depMv is the move which preserves the result of the load but might
   *    destroy a parameter register which will be moved below the branch.
   */
  if (!BackwardFindDependency(*ifBB, targetOpnd, ld, mv, depMv, insnList)) {
    return nullptr;
  }
  if (ld == nullptr || mv == nullptr) {
    return nullptr;
  }
  /*
   * depMv can be live out, so duplicate it
   * and set dest to output of ld and src R16
   */
  if (depMv != nullptr) {
    CHECK_FATAL(depMv->GetMachineOpcode(), "return check");
    Insn &newMv = currCG->BuildInstruction<AArch64Insn>(
        depMv->GetMachineOpcode(), ld->GetOperand(0),
        aarchCGFunc.GetOrCreatePhysicalRegisterOperand(static_cast<AArch64reg>(R16),
                                                       depMv->GetOperand(1).GetSize(), kRegTyInt));
    insnList.push_front(&newMv);
    /* temporary put it some where */
    CHECK_FATAL(coldBB != nullptr, "null ptr check");
    static_cast<void>(coldBB->InsertInsnBegin(newMv));
  } else {
    uint32 regSize = ld->GetOperand(0).GetSize();
    Insn &newMv = currCG->BuildInstruction<AArch64Insn>(
        (regSize <= k32BitSize) ? MOP_xmovri32 : MOP_xmovri64, ld->GetOperand(0),
        aarchCGFunc.GetOrCreatePhysicalRegisterOperand(static_cast<AArch64reg>(R16), regSize, kRegTyInt));
    insnList.push_front(&newMv);
    /* temporary put it some where */
    CHECK_FATAL(coldBB != nullptr, "null ptr check");
    static_cast<void>(coldBB->InsertInsnBegin(newMv));
  }

  ForwardPropagateAndRename(*mv, *ld, *returnBB);

  for (auto *in : insnList) {
    in->GetBB()->RemoveInsn(*in);
    CHECK_FATAL(coldBB != nullptr, "null ptr check");
    static_cast<void>(coldBB->InsertInsnBegin(*in));
  }

  /* All instructions are in the right place, replace branch to ret bb to just ret. */
  CHECK_FATAL(returnBB != nullptr, "null ptr check");
  returnBB->RemoveInsn(*returnBB->GetLastInsn());
  returnBB->AppendInsn(currCG->BuildInstruction<AArch64Insn>(MOP_xret));
  /* bb is now a retbb and has no succ. */
  returnBB->SetKind(BB::kBBReturn);
  BB *tgtBB = returnBB->GetSuccs().front();
  auto predIt = std::find(tgtBB->GetPredsBegin(), tgtBB->GetPredsEnd(), returnBB);
  tgtBB->ErasePreds(predIt);
  returnBB->ClearSuccs();

  return coldBB;
}

AArch64MemOperand *AArch64GenProEpilog::SplitStpLdpOffsetForCalleeSavedWithAddInstruction(const AArch64MemOperand &mo,
                                                                                          uint32 bitLen,
                                                                                          AArch64reg baseRegNum) {
  auto &aarchCGFunc = static_cast<AArch64CGFunc&>(cgFunc);
  CHECK_FATAL(mo.GetAddrMode() == AArch64MemOperand::kAddrModeBOi, "mode should be kAddrModeBOi");
  AArch64OfstOperand *ofstOp = mo.GetOffsetImmediate();
  int32 offsetVal = ofstOp->GetOffsetValue();
  CHECK_FATAL(offsetVal > 0, "offsetVal should be greater than 0");
  CHECK_FATAL((static_cast<uint32>(offsetVal) & 0x7) == 0, "(offsetVal & 0x7) should be equal to 0");
  /*
   * Offset adjustment due to FP/SP has already been done
   * in AArch64GenProEpilog::GeneratePushRegs() and AArch64GenProEpilog::GeneratePopRegs()
   */
  AArch64RegOperand &br = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(baseRegNum, bitLen, kRegTyInt);
  if (aarchCGFunc.GetSplitBaseOffset() == 0) {
    aarchCGFunc.SetSplitBaseOffset(offsetVal);  /* remember the offset; don't forget to clear it */
    ImmOperand &immAddEnd = aarchCGFunc.CreateImmOperand(offsetVal, k64BitSize, true);
    RegOperand *origBaseReg = mo.GetBaseRegister();
    aarchCGFunc.SelectAdd(br, *origBaseReg, immAddEnd, PTY_i64);
  }
  offsetVal = offsetVal - aarchCGFunc.GetSplitBaseOffset();
  return &aarchCGFunc.CreateReplacementMemOperand(bitLen, br, offsetVal);
}

void AArch64GenProEpilog::AppendInstructionPushPair(AArch64reg reg0, AArch64reg reg1, RegType rty, int32 offset) {
  auto &aarchCGFunc = static_cast<AArch64CGFunc&>(cgFunc);
  CG *currCG = cgFunc.GetCG();
  MOperator mOp = pushPopOps[kRegsPushOp][rty][kPushPopPair];
  Operand &o0 = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(reg0, kSizeOfPtr * kBitsPerByte, rty);
  Operand &o1 = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(reg1, kSizeOfPtr * kBitsPerByte, rty);
  Operand *o2 = &aarchCGFunc.CreateStkTopOpnd(offset, kSizeOfPtr * kBitsPerByte);

  uint32 dataSize = kSizeOfPtr * kBitsPerByte;
  CHECK_FATAL(offset >= 0, "offset must >= 0");
  if (offset > kStpLdpImm64UpperBound) {
    o2 = SplitStpLdpOffsetForCalleeSavedWithAddInstruction(*static_cast<AArch64MemOperand*>(o2), dataSize, R16);
  }
  Insn &pushInsn = currCG->BuildInstruction<AArch64Insn>(mOp, o0, o1, *o2);
  std::string comment = "SAVE CALLEE REGISTER PAIR";
  pushInsn.SetComment(comment);
  AppendInstructionTo(pushInsn, cgFunc);

  /* Append CFi code */
  if (cgFunc.GenCfi() && !CGOptions::IsNoCalleeCFI()) {
    int32 stackFrameSize = static_cast<AArch64MemLayout*>(cgFunc.GetMemlayout())->RealStackFrameSize();
    stackFrameSize -= cgFunc.GetMemlayout()->SizeOfArgsToStackPass();
    int32 cfiOffset = stackFrameSize - offset;
    BB *curBB = cgFunc.GetCurBB();
    Insn *newInsn = curBB->InsertInsnAfter(pushInsn, aarchCGFunc.CreateCfiOffsetInsn(reg0, -cfiOffset, k64BitSize));
    curBB->InsertInsnAfter(*newInsn, aarchCGFunc.CreateCfiOffsetInsn(reg1, -cfiOffset + kOffset8MemPos, k64BitSize));
  }
}

void AArch64GenProEpilog::AppendInstructionPushSingle(AArch64reg reg, RegType rty, int32 offset) {
  auto &aarchCGFunc = static_cast<AArch64CGFunc&>(cgFunc);
  CG *currCG = cgFunc.GetCG();
  MOperator mOp = pushPopOps[kRegsPushOp][rty][kPushPopSingle];
  Operand &o0 = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(reg, kSizeOfPtr * kBitsPerByte, rty);
  Operand *o1 = &aarchCGFunc.CreateStkTopOpnd(offset, kSizeOfPtr * kBitsPerByte);

  AArch64MemOperand *aarchMemO1 = static_cast<AArch64MemOperand*>(o1);
  uint32 dataSize = kSizeOfPtr * kBitsPerByte;
  if (aarchMemO1->GetMemVaryType() == kNotVary &&
      aarchCGFunc.IsImmediateOffsetOutOfRange(*aarchMemO1, dataSize)) {
    o1 = &aarchCGFunc.SplitOffsetWithAddInstruction(*aarchMemO1, dataSize, R9);
  }

  Insn &pushInsn = currCG->BuildInstruction<AArch64Insn>(mOp, o0, *o1);
  std::string comment = "SAVE CALLEE REGISTER";
  pushInsn.SetComment(comment);
  AppendInstructionTo(pushInsn, cgFunc);

  /* Append CFI code */
  if (cgFunc.GenCfi() && !CGOptions::IsNoCalleeCFI()) {
    int32 stackFrameSize = static_cast<AArch64MemLayout*>(cgFunc.GetMemlayout())->RealStackFrameSize();
    stackFrameSize -= cgFunc.GetMemlayout()->SizeOfArgsToStackPass();
    int32 cfiOffset = stackFrameSize - offset;
    cgFunc.GetCurBB()->InsertInsnAfter(pushInsn,
                                       aarchCGFunc.CreateCfiOffsetInsn(reg, -cfiOffset, k64BitSize));
  }
}

Insn &AArch64GenProEpilog::AppendInstructionForAllocateOrDeallocateCallFrame(int64 argsToStkPassSize,
                                                                             AArch64reg reg0, AArch64reg reg1,
                                                                             RegType rty, bool isAllocate) {
  auto &aarchCGFunc = static_cast<AArch64CGFunc&>(cgFunc);
  CG *currCG = cgFunc.GetCG();
  MOperator mOp = isAllocate ? pushPopOps[kRegsPushOp][rty][kPushPopPair] : pushPopOps[kRegsPopOp][rty][kPushPopPair];
  if (argsToStkPassSize <= kStrLdrImm64UpperBound - kOffset8MemPos) {
    mOp = isAllocate ? pushPopOps[kRegsPushOp][rty][kPushPopSingle] : pushPopOps[kRegsPopOp][rty][kPushPopSingle];
    AArch64RegOperand &o0 = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(reg0, kSizeOfPtr * kBitsPerByte, rty);
    AArch64MemOperand *o2 = aarchCGFunc.GetMemoryPool()->New<AArch64MemOperand>(RSP, argsToStkPassSize,
                                                                                kSizeOfPtr * kBitsPerByte);
    Insn &insn1 = currCG->BuildInstruction<AArch64Insn>(mOp, o0, *o2);
    AppendInstructionTo(insn1, cgFunc);
    AArch64RegOperand &o1 = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(reg1, kSizeOfPtr * kBitsPerByte, rty);
    o2 = aarchCGFunc.GetMemoryPool()->New<AArch64MemOperand>(RSP, argsToStkPassSize + kSizeOfPtr,
                                                             kSizeOfPtr * kBitsPerByte);
    Insn &insn2 = currCG->BuildInstruction<AArch64Insn>(mOp, o1, *o2);
    AppendInstructionTo(insn2, cgFunc);
    return insn2;
  } else {
    AArch64RegOperand &oo = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(R9, kSizeOfPtr * kBitsPerByte, kRegTyInt);
    AArch64ImmOperand &io1 = aarchCGFunc.CreateImmOperand(argsToStkPassSize, k64BitSize, true);
    aarchCGFunc.SelectCopyImm(oo, io1, PTY_i64);
    AArch64RegOperand &o0 = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(reg0, kSizeOfPtr * kBitsPerByte, rty);
    AArch64RegOperand &rsp = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(RSP, kSizeOfPtr * kBitsPerByte, kRegTyInt);
    AArch64MemOperand *mo = aarchCGFunc.GetMemoryPool()->New<AArch64MemOperand>(
        AArch64MemOperand::kAddrModeBOrX, kSizeOfPtr * kBitsPerByte, rsp, oo, 0);
    Insn &insn1 = currCG->BuildInstruction<AArch64Insn>(isAllocate ? MOP_xstr : MOP_xldr, o0, *mo);
    AppendInstructionTo(insn1, cgFunc);
    AArch64ImmOperand &io2 = aarchCGFunc.CreateImmOperand(kSizeOfPtr, k64BitSize, true);
    aarchCGFunc.SelectAdd(oo, oo, io2, PTY_i64);
    AArch64RegOperand &o1 = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(reg1, kSizeOfPtr * kBitsPerByte, rty);
    mo = aarchCGFunc.GetMemoryPool()->New<AArch64MemOperand>(AArch64MemOperand::kAddrModeBOrX,
                                                             kSizeOfPtr * kBitsPerByte, rsp, oo, 0);
    Insn &insn2 = currCG->BuildInstruction<AArch64Insn>(isAllocate ? MOP_xstr : MOP_xldr, o1, *mo);
    AppendInstructionTo(insn2, cgFunc);
    return insn2;
  }
}

Insn &AArch64GenProEpilog::CreateAndAppendInstructionForAllocateCallFrame(int64 argsToStkPassSize,
                                                                          AArch64reg reg0, AArch64reg reg1,
                                                                          RegType rty) {
  auto &aarchCGFunc = static_cast<AArch64CGFunc&>(cgFunc);
  CG *currCG = cgFunc.GetCG();
  MOperator mOp = pushPopOps[kRegsPushOp][rty][kPushPopPair];
  Insn *allocInsn = nullptr;
  if (argsToStkPassSize > kStpLdpImm64UpperBound) {
    allocInsn = &AppendInstructionForAllocateOrDeallocateCallFrame(argsToStkPassSize, reg0, reg1, rty, true);
  } else {
    Operand &o0 = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(reg0, kSizeOfPtr * kBitsPerByte, rty);
    Operand &o1 = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(reg1, kSizeOfPtr * kBitsPerByte, rty);
    Operand *o2 = aarchCGFunc.GetMemoryPool()->New<AArch64MemOperand>(RSP, argsToStkPassSize,
                                                                      kSizeOfPtr * kBitsPerByte);
    allocInsn = &currCG->BuildInstruction<AArch64Insn>(mOp, o0, o1, *o2);
    AppendInstructionTo(*allocInsn, cgFunc);
  }
  if (currCG->NeedInsertInstrumentationFunction()) {
    aarchCGFunc.AppendCall(*currCG->GetInstrumentationFunction());
  } else if (currCG->InstrumentWithDebugTraceCall()) {
    aarchCGFunc.AppendCall(*currCG->GetDebugTraceEnterFunction());
  } else if (currCG->InstrumentWithProfile()) {
    aarchCGFunc.AppendCall(*currCG->GetProfileFunction());
  }
  return *allocInsn;
}

void AArch64GenProEpilog::AppendInstructionAllocateCallFrame(AArch64reg reg0, AArch64reg reg1, RegType rty) {
  auto &aarchCGFunc = static_cast<AArch64CGFunc&>(cgFunc);
  CG *currCG = cgFunc.GetCG();
  if (currCG->GenerateVerboseCG()) {
    cgFunc.GetCurBB()->AppendInsn(aarchCGFunc.CreateCommentInsn("allocate activation frame"));
  }

  Insn *ipoint = nullptr;
  /*
   * stackFrameSize includes the size of args to stack-pass
   * if a function has neither VLA nor alloca.
   */
  int32 stackFrameSize = static_cast<AArch64MemLayout*>(cgFunc.GetMemlayout())->RealStackFrameSize();
  int64 argsToStkPassSize = cgFunc.GetMemlayout()->SizeOfArgsToStackPass();
  /*
   * ldp/stp's imm should be within -512 and 504;
   * if stp's imm > 512, we fall back to the stp-sub version
   */
  bool useStpSub = false;
  int64 offset = 0;
  int32 cfiOffset = 0;
  if (!cgFunc.HasVLAOrAlloca() && argsToStkPassSize > 0) {
    /*
     * stack_frame_size == size of formal parameters + callee-saved (including FP/RL)
     *                     + size of local vars
     *                     + size of actuals
     * (when passing more than 8 args, its caller's responsibility to
     *  allocate space for it. size of actuals represent largest such size in the function.
     */
    Operand &spOpnd = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(RSP, k64BitSize, kRegTyInt);
    Operand &immOpnd = aarchCGFunc.CreateImmOperand(stackFrameSize, k32BitSize, true);
    aarchCGFunc.SelectSub(spOpnd, spOpnd, immOpnd, PTY_u64);
    ipoint = cgFunc.GetCurBB()->GetLastInsn();
    cfiOffset = stackFrameSize;
  } else {
    if (stackFrameSize > kStpLdpImm64UpperBound) {
      useStpSub = true;
      offset = kOffset16MemPos;
      stackFrameSize -= offset;
    } else {
      offset = stackFrameSize;
    }
    MOperator mOp = pushPopOps[kRegsPushOp][rty][kPushPopPair];
    AArch64RegOperand &o0 = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(reg0, kSizeOfPtr * kBitsPerByte, rty);
    AArch64RegOperand &o1 = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(reg1, kSizeOfPtr * kBitsPerByte, rty);
    AArch64MemOperand &o2 = aarchCGFunc.CreateCallFrameOperand(-offset, kSizeOfPtr * kBitsPerByte);
    ipoint = &currCG->BuildInstruction<AArch64Insn>(mOp, o0, o1, o2);
    AppendInstructionTo(*ipoint, cgFunc);
    cfiOffset = offset;
    if (currCG->NeedInsertInstrumentationFunction()) {
      aarchCGFunc.AppendCall(*currCG->GetInstrumentationFunction());
    } else if (currCG->InstrumentWithDebugTraceCall()) {
      aarchCGFunc.AppendCall(*currCG->GetDebugTraceEnterFunction());
    } else if (currCG->InstrumentWithProfile()) {
      aarchCGFunc.AppendCall(*currCG->GetProfileFunction());
    }
  }

  ipoint = InsertCFIDefCfaOffset(cfiOffset, *ipoint);

  if (!cgFunc.HasVLAOrAlloca() && argsToStkPassSize > 0) {
    CHECK_FATAL(!useStpSub, "Invalid assumption");
    ipoint = &CreateAndAppendInstructionForAllocateCallFrame(argsToStkPassSize, reg0, reg1, rty);
  }

  if (useStpSub) {
    Operand &spOpnd = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(RSP, k64BitSize, kRegTyInt);
    Operand &immOpnd = aarchCGFunc.CreateImmOperand(stackFrameSize, k32BitSize, true);
    aarchCGFunc.SelectSub(spOpnd, spOpnd, immOpnd, PTY_u64);
    ipoint = cgFunc.GetCurBB()->GetLastInsn();
    aarchCGFunc.SetUsedStpSubPairForCallFrameAllocation(true);
  }

  CHECK_FATAL(ipoint != nullptr, "ipoint should not be nullptr at this point");
  int32 cfiOffsetSecond = 0;
  if (useStpSub) {
    cfiOffsetSecond = stackFrameSize;
    ipoint = InsertCFIDefCfaOffset(cfiOffsetSecond, *ipoint);
  }
  cfiOffsetSecond = GetOffsetFromCFA();
  if (!cgFunc.HasVLAOrAlloca()) {
    cfiOffsetSecond -= argsToStkPassSize;
  }
  if (cgFunc.GenCfi()) {
    BB *curBB = cgFunc.GetCurBB();
    if (useFP) {
      ipoint = curBB->InsertInsnAfter(
          *ipoint, aarchCGFunc.CreateCfiOffsetInsn(stackBaseReg, -cfiOffsetSecond, k64BitSize));
    }
    curBB->InsertInsnAfter(*ipoint,
                           aarchCGFunc.CreateCfiOffsetInsn(RLR, -cfiOffsetSecond + kOffset8MemPos, k64BitSize));
  }
}

void AArch64GenProEpilog::AppendInstructionAllocateCallFrameDebug(AArch64reg reg0, AArch64reg reg1, RegType rty) {
  auto &aarchCGFunc = static_cast<AArch64CGFunc&>(cgFunc);
  CG *currCG = cgFunc.GetCG();
  if (currCG->GenerateVerboseCG()) {
    cgFunc.GetCurBB()->AppendInsn(aarchCGFunc.CreateCommentInsn("allocate activation frame for debugging"));
  }

  int32 stackFrameSize = static_cast<AArch64MemLayout*>(cgFunc.GetMemlayout())->RealStackFrameSize();
  int64 argsToStkPassSize = cgFunc.GetMemlayout()->SizeOfArgsToStackPass();

  Insn *ipoint = nullptr;
  int32 cfiOffset = 0;

  if (argsToStkPassSize > 0) {
    Operand &spOpnd = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(RSP, k64BitSize, kRegTyInt);
    Operand &immOpnd = aarchCGFunc.CreateImmOperand(stackFrameSize, k32BitSize, true);
    aarchCGFunc.SelectSub(spOpnd, spOpnd, immOpnd, PTY_u64);
    ipoint = cgFunc.GetCurBB()->GetLastInsn();
    cfiOffset = stackFrameSize;
    (void)InsertCFIDefCfaOffset(cfiOffset, *ipoint);
    ipoint = &CreateAndAppendInstructionForAllocateCallFrame(argsToStkPassSize, reg0, reg1, rty);
    CHECK_FATAL(ipoint != nullptr, "ipoint should not be nullptr at this point");
    cfiOffset = GetOffsetFromCFA();
    cfiOffset -= argsToStkPassSize;
  } else {
    bool useStpSub = false;

    if (stackFrameSize > kStpLdpImm64UpperBound) {
      useStpSub = true;
      AArch64RegOperand &spOpnd = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(RSP, k64BitSize, kRegTyInt);
      ImmOperand &immOpnd = aarchCGFunc.CreateImmOperand(stackFrameSize, k32BitSize, true);
      aarchCGFunc.SelectSub(spOpnd, spOpnd, immOpnd, PTY_u64);
      ipoint = cgFunc.GetCurBB()->GetLastInsn();
      cfiOffset = stackFrameSize;
      ipoint = InsertCFIDefCfaOffset(cfiOffset, *ipoint);
    } else {
      MOperator mOp = pushPopOps[kRegsPushOp][rty][kPushPopPair];
      AArch64RegOperand &o0 = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(reg0, kSizeOfPtr * kBitsPerByte, rty);
      AArch64RegOperand &o1 = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(reg1, kSizeOfPtr * kBitsPerByte, rty);
      AArch64MemOperand &o2 = aarchCGFunc.CreateCallFrameOperand(-stackFrameSize, kSizeOfPtr * kBitsPerByte);
      ipoint = &currCG->BuildInstruction<AArch64Insn>(mOp, o0, o1, o2);
      AppendInstructionTo(*ipoint, cgFunc);
      cfiOffset = stackFrameSize;
      ipoint = InsertCFIDefCfaOffset(cfiOffset, *ipoint);
    }

    if (useStpSub) {
      MOperator mOp = pushPopOps[kRegsPushOp][rty][kPushPopPair];
      AArch64RegOperand &o0 = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(reg0, kSizeOfPtr * kBitsPerByte, rty);
      AArch64RegOperand &o1 = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(reg1, kSizeOfPtr * kBitsPerByte, rty);
      AArch64MemOperand *o2 = aarchCGFunc.GetMemoryPool()->New<AArch64MemOperand>(RSP, 0, kSizeOfPtr * kBitsPerByte);
      ipoint = &currCG->BuildInstruction<AArch64Insn>(mOp, o0, o1, *o2);
      AppendInstructionTo(*ipoint, cgFunc);
    }

    if (currCG->NeedInsertInstrumentationFunction()) {
      aarchCGFunc.AppendCall(*currCG->GetInstrumentationFunction());
    } else if (currCG->InstrumentWithDebugTraceCall()) {
      aarchCGFunc.AppendCall(*currCG->GetDebugTraceEnterFunction());
    } else if (currCG->InstrumentWithProfile()) {
      aarchCGFunc.AppendCall(*currCG->GetProfileFunction());
    }

    CHECK_FATAL(ipoint != nullptr, "ipoint should not be nullptr at this point");
    cfiOffset = GetOffsetFromCFA();
  }
  if (cgFunc.GenCfi()) {
    BB *curBB = cgFunc.GetCurBB();
    if (useFP) {
      ipoint = curBB->InsertInsnAfter(*ipoint, aarchCGFunc.CreateCfiOffsetInsn(stackBaseReg, -cfiOffset, k64BitSize));
    }
    curBB->InsertInsnAfter(*ipoint, aarchCGFunc.CreateCfiOffsetInsn(RLR, -cfiOffset + kOffset8MemPos, k64BitSize));
  }
}

/*
 *  From AArch64 Reference Manual
 *  C1.3.3 Load/Store Addressing Mode
 *  ...
 *  When stack alignment checking is enabled by system software and
 *  the base register is the SP, the current stack pointer must be
 *  initially quadword aligned, that is aligned to 16 bytes. Misalignment
 *  generates a Stack Alignment fault.  The offset does not have to
 *  be a multiple of 16 bytes unless the specific Load/Store instruction
 *  requires this. SP cannot be used as a register offset.
 */
void AArch64GenProEpilog::GeneratePushRegs() {
  auto &aarchCGFunc = static_cast<AArch64CGFunc&>(cgFunc);
  CG *currCG = cgFunc.GetCG();
  const MapleVector<AArch64reg> &regsToSave = aarchCGFunc.GetCalleeSavedRegs();

  CHECK_FATAL(!regsToSave.empty(), "FP/LR not added to callee-saved list?");

  AArch64reg intRegFirstHalf = kRinvalid;
  AArch64reg fpRegFirstHalf = kRinvalid;

  if (currCG->GenerateVerboseCG()) {
    cgFunc.GetCurBB()->AppendInsn(aarchCGFunc.CreateCommentInsn("save callee-saved registers"));
  }

  /*
   * Even if we don't use RFP, since we push a pair of registers in one instruction
   * and the stack needs be aligned on a 16-byte boundary, push RFP as well if function has a call
   * Make sure this is reflected when computing callee_saved_regs.size()
   */
  if (!currCG->GenerateDebugFriendlyCode()) {
    AppendInstructionAllocateCallFrame(R29, RLR, kRegTyInt);
  } else {
    AppendInstructionAllocateCallFrameDebug(R29, RLR, kRegTyInt);
  }

  if (currCG->GenerateVerboseCG()) {
    cgFunc.GetCurBB()->AppendInsn(aarchCGFunc.CreateCommentInsn("copy SP to FP"));
  }
  if (useFP) {
    Operand &spOpnd = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(RSP, k64BitSize, kRegTyInt);
    Operand &fpOpnd = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(stackBaseReg, k64BitSize, kRegTyInt);
    int64 argsToStkPassSize = cgFunc.GetMemlayout()->SizeOfArgsToStackPass();
    if (argsToStkPassSize > 0) {
      Operand &immOpnd = aarchCGFunc.CreateImmOperand(argsToStkPassSize, k32BitSize, true);
      aarchCGFunc.SelectAdd(fpOpnd, spOpnd, immOpnd, PTY_u64);
      cgFunc.GetCurBB()->GetLastInsn()->SetFrameDef(true);
      if (cgFunc.GenCfi()) {
        cgFunc.GetCurBB()->AppendInsn(aarchCGFunc.CreateCfiDefCfaInsn(stackBaseReg,
            static_cast<AArch64MemLayout*>(
            cgFunc.GetMemlayout())->RealStackFrameSize() - argsToStkPassSize, k64BitSize));
      }
    } else {
      aarchCGFunc.SelectCopy(fpOpnd, PTY_u64, spOpnd, PTY_u64);
      cgFunc.GetCurBB()->GetLastInsn()->SetFrameDef(true);
      if (cgFunc.GenCfi()) {
        cgFunc.GetCurBB()->AppendInsn(
            currCG->BuildInstruction<cfi::CfiInsn>(cfi::OP_CFI_def_cfa_register,
            aarchCGFunc.CreateCfiRegOperand(stackBaseReg, k64BitSize)));
      }
    }
  }

  MapleVector<AArch64reg>::const_iterator it = regsToSave.begin();
  /* skip the first two registers */
  CHECK_FATAL(*it == RFP, "The first callee saved reg is expected to be RFP");
  ++it;
  CHECK_FATAL(*it == RLR, "The second callee saved reg is expected to be RLR");
  ++it;

  int32 offset = static_cast<AArch64MemLayout*>(cgFunc.GetMemlayout())->RealStackFrameSize() -
                 (aarchCGFunc.SizeOfCalleeSaved() - (kDivide2 * kIntregBytelen) /* for FP/LR */) -
                 cgFunc.GetMemlayout()->SizeOfArgsToStackPass();

  if (cgFunc.GetMirModule().IsCModule() && cgFunc.GetFunction().GetAttr(FUNCATTR_varargs)) {
    /* GR/VR save areas are above the callee save area */
    AArch64MemLayout *ml = static_cast<AArch64MemLayout *>(cgFunc.GetMemlayout());
    int saveareasize = RoundUp(ml->GetSizeOfGRSaveArea(), kSizeOfPtr * k2BitSize) +
                       RoundUp(ml->GetSizeOfVRSaveArea(), kSizeOfPtr * k2BitSize);
    offset -= saveareasize;
  }

  for (; it != regsToSave.end(); ++it) {
    AArch64reg reg = *it;
    CHECK_FATAL(reg != RFP, "stray RFP in callee_saved_list?");
    CHECK_FATAL(reg != RLR, "stray RLR in callee_saved_list?");

    RegType regType = AArch64isa::IsGPRegister(reg) ? kRegTyInt : kRegTyFloat;
    AArch64reg &firstHalf = AArch64isa::IsGPRegister(reg) ? intRegFirstHalf : fpRegFirstHalf;
    if (firstHalf == kRinvalid) {
      /* remember it */
      firstHalf = reg;
    } else {
      AppendInstructionPushPair(firstHalf, reg, regType, offset);
      GetNextOffsetCalleeSaved(offset);
      firstHalf = kRinvalid;
    }
  }

  if (intRegFirstHalf != kRinvalid) {
    AppendInstructionPushSingle(intRegFirstHalf, kRegTyInt, offset);
    GetNextOffsetCalleeSaved(offset);
  }

  if (fpRegFirstHalf != kRinvalid) {
    AppendInstructionPushSingle(fpRegFirstHalf, kRegTyFloat, offset);
    GetNextOffsetCalleeSaved(offset);
  }

  /*
   * in case we split stp/ldp instructions,
   * so that we generate a load-into-base-register instruction
   * for pop pairs as well.
   */
  aarchCGFunc.SetSplitBaseOffset(0);
}

void AArch64GenProEpilog::GeneratePushUnnamedVarargRegs() {
  auto &aarchCGFunc = static_cast<AArch64CGFunc&>(cgFunc);
  CG *currCG = cgFunc.GetCG();
  if (cgFunc.GetMirModule().IsCModule() && cgFunc.GetFunction().GetAttr(FUNCATTR_varargs)) {
    AArch64MemLayout *memlayout = static_cast<AArch64MemLayout*>(cgFunc.GetMemlayout());
    uint32 dataSizeBits = kSizeOfPtr * kBitsPerByte;
    int32 offset = memlayout->GetGRSaveAreaBaseLoc();
    if (memlayout->GetSizeOfGRSaveArea() % kAarch64StackPtrAlignment) {
      offset += kSizeOfPtr;  /* End of area should be aligned. Hole between VR and GR area */
    }
    int32 start_regno = k8BitSize - (memlayout->GetSizeOfGRSaveArea() / kSizeOfPtr);
    ASSERT(start_regno <= k8BitSize, "Incorrect starting GR regno for GR Save Area");
    for (uint32 i = start_regno + static_cast<uint32>(R0); i < static_cast<uint32>(R8); i++) {
      Operand &stackloc = aarchCGFunc.CreateStkTopOpnd(offset, dataSizeBits);
      RegOperand &reg =
          aarchCGFunc.GetOrCreatePhysicalRegisterOperand(static_cast<AArch64reg>(i), k64BitSize, kRegTyInt);
      Insn &inst =
          currCG->BuildInstruction<AArch64Insn>(aarchCGFunc.PickStInsn(dataSizeBits, PTY_i64), reg, stackloc);
      cgFunc.GetCurBB()->AppendInsn(inst);
      offset += kSizeOfPtr;
    }
    if (!CGOptions::UseGeneralRegOnly()) {
      offset = memlayout->GetVRSaveAreaBaseLoc();
      start_regno = k8BitSize - (memlayout->GetSizeOfVRSaveArea() / (kSizeOfPtr * k2BitSize));
      ASSERT(start_regno <= k8BitSize, "Incorrect starting GR regno for VR Save Area");
      for (uint32 i = start_regno + static_cast<uint32>(V0); i < static_cast<uint32>(V8); i++) {
        Operand &stackloc = aarchCGFunc.CreateStkTopOpnd(offset, dataSizeBits);
        RegOperand &reg =
            aarchCGFunc.GetOrCreatePhysicalRegisterOperand(static_cast<AArch64reg>(i), k64BitSize, kRegTyInt);
        Insn &inst =
            currCG->BuildInstruction<AArch64Insn>(aarchCGFunc.PickStInsn(dataSizeBits, PTY_i64), reg, stackloc);
        cgFunc.GetCurBB()->AppendInsn(inst);
        offset += (kSizeOfPtr * k2BitSize);
      }
    }
  }
}

void AArch64GenProEpilog::AppendInstructionStackCheck(AArch64reg reg, RegType rty, int32 offset) {
  auto &aarchCGFunc = static_cast<AArch64CGFunc&>(cgFunc);
  CG *currCG = cgFunc.GetCG();
  /* sub x16, sp, #0x2000 */
  auto &x16Opnd = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(reg, k64BitSize, rty);
  auto &spOpnd = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(RSP, k64BitSize, rty);
  auto &imm1 = aarchCGFunc.CreateImmOperand(offset, k64BitSize, true);
  aarchCGFunc.SelectSub(x16Opnd, spOpnd, imm1, PTY_u64);

  /* ldr wzr, [x16] */
  auto &wzr = AArch64RegOperand::Get32bitZeroRegister();
  auto &refX16 = aarchCGFunc.CreateMemOpnd(reg, 0, k64BitSize);
  auto &soeInstr = currCG->BuildInstruction<AArch64Insn>(MOP_wldr, wzr, refX16);
  if (currCG->GenerateVerboseCG()) {
    soeInstr.SetComment("soerror");
  }
  soeInstr.SetDoNotRemove(true);
  AppendInstructionTo(soeInstr, cgFunc);
}

void AArch64GenProEpilog::GenerateProlog(BB &bb) {
  auto &aarchCGFunc = static_cast<AArch64CGFunc&>(cgFunc);
  CG *currCG = cgFunc.GetCG();
  BB *formerCurBB = cgFunc.GetCurBB();
  aarchCGFunc.GetDummyBB()->ClearInsns();
  aarchCGFunc.GetDummyBB()->SetIsProEpilog(true);
  cgFunc.SetCurBB(*aarchCGFunc.GetDummyBB());
  if (!cgFunc.GetHasProEpilogue()) {
    return;
  }

  // insert .loc for function
  if (currCG->GetCGOptions().WithLoc()) {
    MIRFunction *func = &cgFunc.GetFunction();
    MIRSymbol *fSym = GlobalTables::GetGsymTable().GetSymbolFromStidx(func->GetStIdx().Idx());
    if (currCG->GetCGOptions().WithSrc()) {
      uint32 tempmaxsize = currCG->GetMIRModule()->GetSrcFileInfo().size();
      uint32 endfilenum = currCG->GetMIRModule()->GetSrcFileInfo()[tempmaxsize - 1].second;
      if (fSym->GetSrcPosition().FileNum() != 0 && fSym->GetSrcPosition().FileNum() <= endfilenum) {
        Operand *o0 = cgFunc.CreateDbgImmOperand(fSym->GetSrcPosition().FileNum());
        int64_t lineNum = fSym->GetSrcPosition().LineNum();
        if (lineNum == 0) {
          if (cgFunc.GetFunction().GetAttr(FUNCATTR_native)) {
            lineNum = 0xffffe;
          } else {
            lineNum = 0xffffd;
          }
        }
        Operand *o1 = cgFunc.CreateDbgImmOperand(lineNum);
        Insn &loc = currCG->BuildInstruction<mpldbg::DbgInsn>(mpldbg::OP_DBG_loc, *o0, *o1);
        cgFunc.GetCurBB()->AppendInsn(loc);
      }
    } else {
      Operand *o0 = cgFunc.CreateDbgImmOperand(1);
      // line number might not be available.
      // CG_ASSERT(func->srcPosition.MplLinenum(), "return check");
      Operand *o1 = cgFunc.CreateDbgImmOperand(fSym->GetSrcPosition().MplLineNum());
      Insn &loc = currCG->BuildInstruction<mpldbg::DbgInsn>(mpldbg::OP_DBG_loc, *o0, *o1);
      cgFunc.GetCurBB()->AppendInsn(loc);
    }
  }

  const MapleVector<AArch64reg> &regsToSave = aarchCGFunc.GetCalleeSavedRegs();
  if (!regsToSave.empty()) {
    /*
     * Among other things, push the FP & LR pair.
     * FP/LR are added to the callee-saved list in AllocateRegisters()
     * We add them to the callee-saved list regardless of UseFP() being true/false.
     * Activation Frame is allocated as part of pushing FP/LR pair
     */
    GeneratePushRegs();
  } else {
    Operand &spOpnd = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(RSP, k64BitSize, kRegTyInt);
    int32 stackFrameSize = static_cast<AArch64MemLayout*>(cgFunc.GetMemlayout())->RealStackFrameSize();
    if (stackFrameSize > 0) {
      if (currCG->GenerateVerboseCG()) {
        cgFunc.GetCurBB()->AppendInsn(aarchCGFunc.CreateCommentInsn("allocate activation frame"));
      }
      Operand &immOpnd = aarchCGFunc.CreateImmOperand(stackFrameSize, k32BitSize, true);
      aarchCGFunc.SelectSub(spOpnd, spOpnd, immOpnd, PTY_u64);

      int32 offset = stackFrameSize;
      (void)InsertCFIDefCfaOffset(offset, *(cgFunc.GetCurBB()->GetLastInsn()));
    }
    if (currCG->GenerateVerboseCG()) {
      cgFunc.GetCurBB()->AppendInsn(aarchCGFunc.CreateCommentInsn("copy SP to FP"));
    }
    if (useFP) {
      Operand &fpOpnd = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(stackBaseReg, k64BitSize, kRegTyInt);
      int64 argsToStkPassSize = cgFunc.GetMemlayout()->SizeOfArgsToStackPass();
      if (argsToStkPassSize > 0) {
        Operand &immOpnd = aarchCGFunc.CreateImmOperand(argsToStkPassSize, k32BitSize, true);
        aarchCGFunc.SelectAdd(fpOpnd, spOpnd, immOpnd, PTY_u64);
        cgFunc.GetCurBB()->GetLastInsn()->SetFrameDef(true);
        if (cgFunc.GenCfi()) {
          cgFunc.GetCurBB()->AppendInsn(aarchCGFunc.CreateCfiDefCfaInsn(
              stackBaseReg,
              static_cast<AArch64MemLayout*>(cgFunc.GetMemlayout())->RealStackFrameSize() - argsToStkPassSize,
              k64BitSize));
        }
      } else {
        aarchCGFunc.SelectCopy(fpOpnd, PTY_u64, spOpnd, PTY_u64);
        cgFunc.GetCurBB()->GetLastInsn()->SetFrameDef(true);
        if (cgFunc.GenCfi()) {
          cgFunc.GetCurBB()->AppendInsn(
              currCG->BuildInstruction<cfi::CfiInsn>(cfi::OP_CFI_def_cfa_register,
                                                     aarchCGFunc.CreateCfiRegOperand(stackBaseReg, k64BitSize)));
        }
      }
    }
  }
  GeneratePushUnnamedVarargRegs();
  if (currCG->DoCheckSOE()) {
    AppendInstructionStackCheck(R16, kRegTyInt, kSoeChckOffset);
  }
  bb.InsertAtBeginning(*aarchCGFunc.GetDummyBB());
  cgFunc.SetCurBB(*formerCurBB);
  aarchCGFunc.GetDummyBB()->SetIsProEpilog(false);
}

void AArch64GenProEpilog::GenerateRet(BB &bb) {
  CG *currCG = cgFunc.GetCG();
  bb.AppendInsn(currCG->BuildInstruction<AArch64Insn>(MOP_xret));
}

/*
 * If all the preds of exitBB made the TailcallOpt(replace blr/bl with br/b), return true, we don't create ret insn.
 * Otherwise, return false, create the ret insn.
 */
bool AArch64GenProEpilog::TestPredsOfRetBB(const BB &exitBB) {
  AArch64MemLayout *ml = static_cast<AArch64MemLayout*>(cgFunc.GetMemlayout());
  if (cgFunc.GetMirModule().IsCModule() &&
      (cgFunc.GetFunction().GetAttr(FUNCATTR_varargs) ||
       ml->GetSizeOfLocals() > 0 || cgFunc.HasVLAOrAlloca())) {
    return false;
  }
  for (auto tmpBB : exitBB.GetPreds()) {
    Insn *firstInsn = tmpBB->GetFirstInsn();
    if ((firstInsn == nullptr || tmpBB->IsCommentBB()) && (!tmpBB->GetPreds().empty())) {
      if (!TestPredsOfRetBB(*tmpBB)) {
        return false;
      }
    } else {
      Insn *lastInsn = tmpBB->GetLastInsn();
      if (lastInsn == nullptr) {
        return false;
      }
      MOperator insnMop = lastInsn->GetMachineOpcode();
      if (insnMop != MOP_tail_call_opt_xbl && insnMop != MOP_tail_call_opt_xblr) {
        return false;
      }
    }
  }
  return true;
}

void AArch64GenProEpilog::AppendInstructionPopSingle(AArch64reg reg, RegType rty, int32 offset) {
  auto &aarchCGFunc = static_cast<AArch64CGFunc&>(cgFunc);
  CG *currCG = cgFunc.GetCG();
  MOperator mOp = pushPopOps[kRegsPopOp][rty][kPushPopSingle];
  Operand &o0 = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(reg, kSizeOfPtr * kBitsPerByte, rty);
  Operand *o1 = &aarchCGFunc.CreateStkTopOpnd(offset, kSizeOfPtr * kBitsPerByte);
  AArch64MemOperand *aarchMemO1 = static_cast<AArch64MemOperand*>(o1);
  uint32 dataSize = kSizeOfPtr * kBitsPerByte;
  if (aarchMemO1->GetMemVaryType() == kNotVary && aarchCGFunc.IsImmediateOffsetOutOfRange(*aarchMemO1, dataSize)) {
    o1 = &aarchCGFunc.SplitOffsetWithAddInstruction(*aarchMemO1, dataSize, R9);
  }

  Insn &popInsn = currCG->BuildInstruction<AArch64Insn>(mOp, o0, *o1);
  popInsn.SetComment("RESTORE");
  cgFunc.GetCurBB()->AppendInsn(popInsn);

  /* Append CFI code. */
  if (cgFunc.GenCfi() && !CGOptions::IsNoCalleeCFI()) {
    cgFunc.GetCurBB()->AppendInsn(aarchCGFunc.CreateCfiRestoreInsn(reg, k64BitSize));
  }
}

void AArch64GenProEpilog::AppendInstructionPopPair(AArch64reg reg0, AArch64reg reg1, RegType rty, int32 offset) {
  auto &aarchCGFunc = static_cast<AArch64CGFunc&>(cgFunc);
  CG *currCG = cgFunc.GetCG();
  MOperator mOp = pushPopOps[kRegsPopOp][rty][kPushPopPair];
  Operand &o0 = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(reg0, kSizeOfPtr * kBitsPerByte, rty);
  Operand &o1 = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(reg1, kSizeOfPtr * kBitsPerByte, rty);
  Operand *o2 = &aarchCGFunc.CreateStkTopOpnd(offset, kSizeOfPtr * kBitsPerByte);

  uint32 dataSize = kSizeOfPtr * kBitsPerByte;
  CHECK_FATAL(offset >= 0, "offset must >= 0");
  if (offset > kStpLdpImm64UpperBound) {
    o2 = SplitStpLdpOffsetForCalleeSavedWithAddInstruction(*static_cast<AArch64MemOperand*>(o2), dataSize, R16);
  }
  Insn &popInsn = currCG->BuildInstruction<AArch64Insn>(mOp, o0, o1, *o2);
  popInsn.SetComment("RESTORE RESTORE");
  cgFunc.GetCurBB()->AppendInsn(popInsn);

  /* Append CFI code */
  if (cgFunc.GenCfi() && !CGOptions::IsNoCalleeCFI()) {
    cgFunc.GetCurBB()->AppendInsn(aarchCGFunc.CreateCfiRestoreInsn(reg0, k64BitSize));
    cgFunc.GetCurBB()->AppendInsn(aarchCGFunc.CreateCfiRestoreInsn(reg1, k64BitSize));
  }
}


void AArch64GenProEpilog::AppendInstructionDeallocateCallFrame(AArch64reg reg0, AArch64reg reg1, RegType rty) {
  auto &aarchCGFunc = static_cast<AArch64CGFunc&>(cgFunc);
  CG *currCG = cgFunc.GetCG();
  MOperator mOp = pushPopOps[kRegsPopOp][rty][kPushPopPair];
  Operand &o0 = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(reg0, kSizeOfPtr * kBitsPerByte, rty);
  Operand &o1 = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(reg1, kSizeOfPtr * kBitsPerByte, rty);
  int32 stackFrameSize = static_cast<AArch64MemLayout*>(cgFunc.GetMemlayout())->RealStackFrameSize();
  int64 argsToStkPassSize = cgFunc.GetMemlayout()->SizeOfArgsToStackPass();
  /*
   * ldp/stp's imm should be within -512 and 504;
   * if ldp's imm > 504, we fall back to the ldp-add version
   */
  bool useLdpAdd = false;
  int64 offset = 0;

  Operand *o2 = nullptr;
  if (!cgFunc.HasVLAOrAlloca() && argsToStkPassSize > 0) {
    o2 = aarchCGFunc.GetMemoryPool()->New<AArch64MemOperand>(RSP, argsToStkPassSize, kSizeOfPtr * kBitsPerByte);
  } else {
    if (stackFrameSize > kStpLdpImm64UpperBound) {
      useLdpAdd = true;
      offset = kOffset16MemPos;
      stackFrameSize -= offset;
    } else {
      offset = stackFrameSize;
    }
    o2 = &aarchCGFunc.CreateCallFrameOperand(offset, kSizeOfPtr * kBitsPerByte);
  }

  if (useLdpAdd) {
    Operand &spOpnd = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(RSP, k64BitSize, kRegTyInt);
    Operand &immOpnd = aarchCGFunc.CreateImmOperand(stackFrameSize, k32BitSize, true);
    aarchCGFunc.SelectAdd(spOpnd, spOpnd, immOpnd, PTY_u64);
    if (cgFunc.GenCfi()) {
      int64 cfiOffset = GetOffsetFromCFA();
      BB *curBB = cgFunc.GetCurBB();
      curBB->InsertInsnAfter(*(curBB->GetLastInsn()),
                             aarchCGFunc.CreateCfiDefCfaInsn(RSP, cfiOffset - stackFrameSize, k64BitSize));
    }
  }

  if (!cgFunc.HasVLAOrAlloca() && argsToStkPassSize > 0) {
    CHECK_FATAL(!useLdpAdd, "Invalid assumption");
    if (argsToStkPassSize > kStpLdpImm64UpperBound) {
      (void)AppendInstructionForAllocateOrDeallocateCallFrame(argsToStkPassSize, reg0, reg1, rty, false);
    } else {
      Insn &deallocInsn = currCG->BuildInstruction<AArch64Insn>(mOp, o0, o1, *o2);
      cgFunc.GetCurBB()->AppendInsn(deallocInsn);
    }
    Operand &spOpnd = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(RSP, k64BitSize, kRegTyInt);
    Operand &immOpnd = aarchCGFunc.CreateImmOperand(stackFrameSize, k32BitSize, true);
    aarchCGFunc.SelectAdd(spOpnd, spOpnd, immOpnd, PTY_u64);
  } else {
    Insn &deallocInsn = currCG->BuildInstruction<AArch64Insn>(mOp, o0, o1, *o2);
    cgFunc.GetCurBB()->AppendInsn(deallocInsn);
  }

  if (cgFunc.GenCfi()) {
    /* Append CFI restore */
    if (useFP) {
      cgFunc.GetCurBB()->AppendInsn(aarchCGFunc.CreateCfiRestoreInsn(stackBaseReg, k64BitSize));
    }
    cgFunc.GetCurBB()->AppendInsn(aarchCGFunc.CreateCfiRestoreInsn(RLR, k64BitSize));
  }
}

void AArch64GenProEpilog::AppendInstructionDeallocateCallFrameDebug(AArch64reg reg0, AArch64reg reg1, RegType rty) {
  auto &aarchCGFunc = static_cast<AArch64CGFunc&>(cgFunc);
  CG *currCG = cgFunc.GetCG();
  MOperator mOp = pushPopOps[kRegsPopOp][rty][kPushPopPair];
  Operand &o0 = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(reg0, kSizeOfPtr * kBitsPerByte, rty);
  Operand &o1 = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(reg1, kSizeOfPtr * kBitsPerByte, rty);
  int32 stackFrameSize = static_cast<AArch64MemLayout*>(cgFunc.GetMemlayout())->RealStackFrameSize();
  int64 argsToStkPassSize = cgFunc.GetMemlayout()->SizeOfArgsToStackPass();
  /*
   * ldp/stp's imm should be within -512 and 504;
   * if ldp's imm > 504, we fall back to the ldp-add version
   */
  if (cgFunc.HasVLAOrAlloca() || argsToStkPassSize == 0) {
    stackFrameSize -= argsToStkPassSize;
    if (stackFrameSize > kStpLdpImm64UpperBound) {
      Operand *o2;
      o2 = aarchCGFunc.GetMemoryPool()->New<AArch64MemOperand>(RSP, 0, kSizeOfPtr * kBitsPerByte);
      Insn &deallocInsn = currCG->BuildInstruction<AArch64Insn>(mOp, o0, o1, *o2);
      cgFunc.GetCurBB()->AppendInsn(deallocInsn);
      if (cgFunc.GenCfi()) {
        /* Append CFI restore */
        if (useFP) {
          cgFunc.GetCurBB()->AppendInsn(aarchCGFunc.CreateCfiRestoreInsn(stackBaseReg, k64BitSize));
        }
        cgFunc.GetCurBB()->AppendInsn(aarchCGFunc.CreateCfiRestoreInsn(RLR, k64BitSize));
      }
      Operand &spOpnd = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(RSP, k64BitSize, kRegTyInt);
      Operand &immOpnd = aarchCGFunc.CreateImmOperand(stackFrameSize, k32BitSize, true);
      aarchCGFunc.SelectAdd(spOpnd, spOpnd, immOpnd, PTY_u64);
    } else {
      AArch64MemOperand &o2 = aarchCGFunc.CreateCallFrameOperand(stackFrameSize, kSizeOfPtr * kBitsPerByte);
      Insn &deallocInsn = currCG->BuildInstruction<AArch64Insn>(mOp, o0, o1, o2);
      cgFunc.GetCurBB()->AppendInsn(deallocInsn);
      if (cgFunc.GenCfi()) {
        if (useFP) {
          cgFunc.GetCurBB()->AppendInsn(aarchCGFunc.CreateCfiRestoreInsn(stackBaseReg, k64BitSize));
        }
        cgFunc.GetCurBB()->AppendInsn(aarchCGFunc.CreateCfiRestoreInsn(RLR, k64BitSize));
      }
    }
  } else {
    Operand *o2;
    o2 = aarchCGFunc.GetMemoryPool()->New<AArch64MemOperand>(RSP, argsToStkPassSize, kSizeOfPtr * kBitsPerByte);
    if (argsToStkPassSize > kStpLdpImm64UpperBound) {
      (void)AppendInstructionForAllocateOrDeallocateCallFrame(argsToStkPassSize, reg0, reg1, rty, false);
    } else {
      Insn &deallocInsn = currCG->BuildInstruction<AArch64Insn>(mOp, o0, o1, *o2);
      cgFunc.GetCurBB()->AppendInsn(deallocInsn);
    }

    if (cgFunc.GenCfi()) {
      if (useFP) {
        cgFunc.GetCurBB()->AppendInsn(aarchCGFunc.CreateCfiRestoreInsn(stackBaseReg, k64BitSize));
      }
      cgFunc.GetCurBB()->AppendInsn(aarchCGFunc.CreateCfiRestoreInsn(RLR, k64BitSize));
    }
    Operand &spOpnd = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(RSP, k64BitSize, kRegTyInt);
    Operand &immOpnd = aarchCGFunc.CreateImmOperand(stackFrameSize, k32BitSize, true);
    aarchCGFunc.SelectAdd(spOpnd, spOpnd, immOpnd, PTY_u64);
  }
}

void AArch64GenProEpilog::GeneratePopRegs() {
  auto &aarchCGFunc = static_cast<AArch64CGFunc&>(cgFunc);
  CG *currCG = cgFunc.GetCG();
  const MapleVector<AArch64reg> &regsToRestore = aarchCGFunc.GetCalleeSavedRegs();

  CHECK_FATAL(!regsToRestore.empty(), "FP/LR not added to callee-saved list?");

  AArch64reg intRegFirstHalf = kRinvalid;
  AArch64reg fpRegFirstHalf = kRinvalid;

  if (currCG->GenerateVerboseCG()) {
    cgFunc.GetCurBB()->AppendInsn(aarchCGFunc.CreateCommentInsn("restore callee-saved registers"));
  }

  MapleVector<AArch64reg>::const_iterator it = regsToRestore.begin();
  /*
   * Even if we don't use FP, since we push a pair of registers
   * in a single instruction (i.e., stp) and the stack needs be aligned
   * on a 16-byte boundary, push FP as well if the function has a call.
   * Make sure this is reflected when computing calleeSavedRegs.size()
   * skip the first two registers
   */
  CHECK_FATAL(*it == RFP, "The first callee saved reg is expected to be RFP");
  ++it;
  CHECK_FATAL(*it == RLR, "The second callee saved reg is expected to be RLR");
  ++it;

  int32 offset = static_cast<AArch64MemLayout*>(cgFunc.GetMemlayout())->RealStackFrameSize() -
                 (aarchCGFunc.SizeOfCalleeSaved() - (kDivide2 * kIntregBytelen) /* for FP/LR */) -
                 cgFunc.GetMemlayout()->SizeOfArgsToStackPass();

  if (cgFunc.GetMirModule().IsCModule() && cgFunc.GetFunction().GetAttr(FUNCATTR_varargs)) {
    /* GR/VR save areas are above the callee save area */
    AArch64MemLayout *ml = static_cast<AArch64MemLayout *>(cgFunc.GetMemlayout());
    int saveareasize = RoundUp(ml->GetSizeOfGRSaveArea(), kSizeOfPtr * k2BitSize) +
                       RoundUp(ml->GetSizeOfVRSaveArea(), kSizeOfPtr * k2BitSize);
    offset -= saveareasize;
  }

  /*
   * We are using a cleared dummy block; so insertPoint cannot be ret;
   * see GenerateEpilog()
   */
  for (; it != regsToRestore.end(); ++it) {
    AArch64reg reg = *it;
    CHECK_FATAL(reg != RFP, "stray RFP in callee_saved_list?");
    CHECK_FATAL(reg != RLR, "stray RLR in callee_saved_list?");

    RegType regType = AArch64isa::IsGPRegister(reg) ? kRegTyInt : kRegTyFloat;
    AArch64reg &firstHalf = AArch64isa::IsGPRegister(reg) ? intRegFirstHalf : fpRegFirstHalf;
    if (firstHalf == kRinvalid) {
      /* remember it */
      firstHalf = reg;
    } else {
      /* flush the pair */
      AppendInstructionPopPair(firstHalf, reg, regType, offset);
      GetNextOffsetCalleeSaved(offset);
      firstHalf = kRinvalid;
    }
  }

  if (intRegFirstHalf != kRinvalid) {
    AppendInstructionPopSingle(intRegFirstHalf, kRegTyInt, offset);
    GetNextOffsetCalleeSaved(offset);
  }

  if (fpRegFirstHalf != kRinvalid) {
    AppendInstructionPopSingle(fpRegFirstHalf, kRegTyFloat, offset);
    GetNextOffsetCalleeSaved(offset);
  }

  if (!currCG->GenerateDebugFriendlyCode()) {
    AppendInstructionDeallocateCallFrame(R29, RLR, kRegTyInt);
  } else {
    AppendInstructionDeallocateCallFrameDebug(R29, RLR, kRegTyInt);
  }

  if (cgFunc.GenCfi()) {
    cgFunc.GetCurBB()->AppendInsn(aarchCGFunc.CreateCfiDefCfaInsn(RSP, 0, k64BitSize));
  }
  /*
   * in case we split stp/ldp instructions,
   * so that we generate a load-into-base-register instruction
   * for the next function, maybe? (seems not necessary, but...)
   */
  aarchCGFunc.SetSplitBaseOffset(0);
}

void AArch64GenProEpilog::AppendJump(const MIRSymbol &funcSymbol) {
  auto &aarchCGFunc = static_cast<AArch64CGFunc&>(cgFunc);
  CG *currCG = cgFunc.GetCG();
  Operand &targetOpnd = aarchCGFunc.CreateFuncLabelOperand(funcSymbol);
  cgFunc.GetCurBB()->AppendInsn(currCG->BuildInstruction<AArch64Insn>(MOP_xuncond, targetOpnd));
}

void AArch64GenProEpilog::GenerateEpilog(BB &bb) {
  if (!cgFunc.GetHasProEpilogue()) {
    if (bb.GetPreds().empty() || !TestPredsOfRetBB(bb)) {
      GenerateRet(bb);
    }
    return;
  }

  /* generate stack protected instruction */
  BB &epilogBB = GenStackGuardCheckInsn(bb);

  auto &aarchCGFunc = static_cast<AArch64CGFunc&>(cgFunc);
  CG *currCG = cgFunc.GetCG();
  BB *formerCurBB = cgFunc.GetCurBB();
  aarchCGFunc.GetDummyBB()->ClearInsns();
  aarchCGFunc.GetDummyBB()->SetIsProEpilog(true);
  cgFunc.SetCurBB(*aarchCGFunc.GetDummyBB());

  Operand &spOpnd = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(RSP, k64BitSize, kRegTyInt);
  Operand &fpOpnd = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(stackBaseReg, k64BitSize, kRegTyInt);

  if (cgFunc.HasVLAOrAlloca()) {
    aarchCGFunc.SelectCopy(spOpnd, PTY_u64, fpOpnd, PTY_u64);
  }

  /* Hack: exit bb should always be reachable, since we need its existance for ".cfi_remember_state" */
  if (&epilogBB != cgFunc.GetLastBB() && epilogBB.GetNext() != nullptr) {
    BB *nextBB = epilogBB.GetNext();
    do {
      if (nextBB == cgFunc.GetLastBB() || !nextBB->IsEmpty()) {
        break;
      }
      nextBB = nextBB->GetNext();
    } while (nextBB != nullptr);
    if (nextBB != nullptr && !nextBB->IsEmpty() && cgFunc.GenCfi()) {
      cgFunc.GetCurBB()->AppendInsn(currCG->BuildInstruction<cfi::CfiInsn>(cfi::OP_CFI_remember_state));
      cgFunc.GetCurBB()->SetHasCfi();
      nextBB->InsertInsnBefore(*nextBB->GetFirstInsn(),
                               currCG->BuildInstruction<cfi::CfiInsn>(cfi::OP_CFI_restore_state));
      nextBB->SetHasCfi();
    }
  }

  const MapleVector<AArch64reg> &regsToSave = aarchCGFunc.GetCalleeSavedRegs();
  if (!regsToSave.empty()) {
    GeneratePopRegs();
  } else {
    int32 stackFrameSize = static_cast<AArch64MemLayout*>(cgFunc.GetMemlayout())->RealStackFrameSize();
    if (stackFrameSize > 0) {
      if (currCG->GenerateVerboseCG()) {
        cgFunc.GetCurBB()->AppendInsn(aarchCGFunc.CreateCommentInsn("pop up activation frame"));
      }

      if (cgFunc.HasVLAOrAlloca()) {
        stackFrameSize -= static_cast<AArch64MemLayout*>(cgFunc.GetMemlayout())->GetSegArgsStkPass().GetSize();
      }

      if (stackFrameSize > 0) {
        Operand &immOpnd = aarchCGFunc.CreateImmOperand(stackFrameSize, k32BitSize, true);
        aarchCGFunc.SelectAdd(spOpnd, spOpnd, immOpnd, PTY_u64);
        if (cgFunc.GenCfi()) {
          cgFunc.GetCurBB()->AppendInsn(aarchCGFunc.CreateCfiDefCfaInsn(RSP, 0, k64BitSize));
        }
      }
    }
  }

  if (currCG->InstrumentWithDebugTraceCall()) {
    AppendJump(*(currCG->GetDebugTraceExitFunction()));
  }

  GenerateRet(*(cgFunc.GetCurBB()));
  epilogBB.AppendBBInsns(*cgFunc.GetCurBB());
  if (cgFunc.GetCurBB()->GetHasCfi()) {
    epilogBB.SetHasCfi();
  }

  cgFunc.SetCurBB(*formerCurBB);
  aarchCGFunc.GetDummyBB()->SetIsProEpilog(false);
}

void AArch64GenProEpilog::GenerateEpilogForCleanup(BB &bb) {
  auto &aarchCGFunc = static_cast<AArch64CGFunc&>(cgFunc);
  CG *currCG = cgFunc.GetCG();
  CHECK_FATAL(!cgFunc.GetExitBBsVec().empty(), "exit bb size is zero!");
  if (cgFunc.GetExitBB(0)->IsUnreachable()) {
    /* if exitbb is unreachable then exitbb can not be generated */
    GenerateEpilog(bb);
  } else if (aarchCGFunc.NeedCleanup()) {  /* bl to the exit epilogue */
    LabelOperand &targetOpnd = aarchCGFunc.GetOrCreateLabelOperand(cgFunc.GetExitBB(0)->GetLabIdx());
    bb.AppendInsn(currCG->BuildInstruction<AArch64Insn>(MOP_xuncond, targetOpnd));
  }
}

void AArch64GenProEpilog::Run() {
  CHECK_FATAL(cgFunc.GetFunction().GetBody()->GetFirst()->GetOpCode() == OP_label,
              "The first statement should be a label");
  cgFunc.SetHasProEpilogue(NeedProEpilog());
  if (cgFunc.GetHasProEpilogue()) {
    GenStackGuard(*(cgFunc.GetFirstBB()));
  }
  BB *proLog = nullptr;
  if (Globals::GetInstance()->GetOptimLevel() == CGOptions::kLevel2) {
    /* There are some O2 dependent assumptions made */
    proLog = IsolateFastPath(*(cgFunc.GetFirstBB()));
  }

  if (cgFunc.IsExitBBsVecEmpty()) {
    if (cgFunc.GetLastBB()->GetPrev()->GetFirstStmt() == cgFunc.GetCleanupLabel() &&
        cgFunc.GetLastBB()->GetPrev()->GetPrev()) {
      cgFunc.PushBackExitBBsVec(*cgFunc.GetLastBB()->GetPrev()->GetPrev());
    } else {
      cgFunc.PushBackExitBBsVec(*cgFunc.GetLastBB()->GetPrev());
    }
  }

  if (proLog != nullptr) {
    GenerateProlog(*proLog);
    proLog->SetFastPath(true);
    cgFunc.GetFirstBB()->SetFastPath(true);
  } else {
    GenerateProlog(*(cgFunc.GetFirstBB()));
  }

  for (auto *exitBB : cgFunc.GetExitBBsVec()) {
    GenerateEpilog(*exitBB);
  }

  if (cgFunc.GetFunction().IsJava()) {
    GenerateEpilogForCleanup(*(cgFunc.GetCleanupBB()));
  }
}
}  /* namespace maplebe */
