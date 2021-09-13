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
#include "aarch64_global.h"
#include "aarch64_reaching.h"
#include "aarch64_cg.h"
#include "aarch64_live.h"

namespace maplebe {
using namespace maple;
#define GLOBAL_DUMP CG_DEBUG_FUNC(cgFunc)

void AArch64GlobalOpt::Run() {
  if (cgFunc.NumBBs() > kMaxBBNum || cgFunc.GetRD()->GetMaxInsnNO() > kMaxInsnNum) {
    return;
  }
  OptimizeManager optManager(cgFunc);
  optManager.Optimize<UxtwMovPattern>();
  optManager.Optimize<BackPropPattern>();
  optManager.Optimize<ForwardPropPattern>();
  optManager.Optimize<CselPattern>();
  optManager.Optimize<CmpCsetPattern>();
  optManager.Optimize<RedundantUxtPattern>();
  optManager.Optimize<LocalVarSaveInsnPattern>();
  optManager.Optimize<ExtendShiftOptPattern>();
}

/* if used Operand in insn is defined by zero in all define insn, return true */
bool OptimizePattern::OpndDefByZero(Insn &insn, int32 useIdx) const {
  ASSERT(insn.GetOperand(useIdx).IsRegister(), "the used Operand must be Register");
  /* Zero Register don't need be defined */
  if (insn.GetOperand(useIdx).IsZeroRegister()) {
    return true;
  }

  InsnSet defInsns = cgFunc.GetRD()->FindDefForRegOpnd(insn, useIdx);
  ASSERT(!defInsns.empty(), "operand must be defined before used");
  for (auto &defInsn : defInsns) {
    if (!InsnDefZero(*defInsn)) {
      return false;
    }
  }
  return true;
}

/* if used Operand in insn is defined by one in all define insn, return true */
bool OptimizePattern::OpndDefByOne(Insn &insn, int32 useIdx) const {
  ASSERT(insn.GetOperand(useIdx).IsRegister(), "the used Operand must be Register");
  /* Zero Register don't need be defined */
  if (insn.GetOperand(useIdx).IsZeroRegister()) {
    return false;
  }
  InsnSet defInsns = cgFunc.GetRD()->FindDefForRegOpnd(insn, useIdx);
  ASSERT(!defInsns.empty(), "operand must be defined before used");
  for (auto &defInsn : defInsns) {
    if (!InsnDefOne(*defInsn)) {
      return false;
    }
  }
  return true;
}

/* if used Operand in insn is defined by one valid bit in all define insn, return true */
bool OptimizePattern::OpndDefByOneOrZero(Insn &insn, int32 useIdx) const {
  if (insn.GetOperand(useIdx).IsZeroRegister()) {
    return true;
  }

  InsnSet defInsnSet = cgFunc.GetRD()->FindDefForRegOpnd(insn, useIdx);
  ASSERT(!defInsnSet.empty(), "Operand must be defined before used");

  for (auto &defInsn : defInsnSet) {
    if (!InsnDefOneOrZero(*defInsn)) {
      return false;
    }
  }
  return true;
}

/* if defined operand(must be first insn currently) in insn is const one, return true */
bool OptimizePattern::InsnDefOne(Insn &insn) {
  MOperator defMop = insn.GetMachineOpcode();
  switch (defMop) {
    case MOP_xmovri32:
    case MOP_xmovri64: {
      Operand &srcOpnd = insn.GetOperand(1);
      ASSERT(srcOpnd.IsIntImmediate(), "expects ImmOperand");
      ImmOperand &srcConst = static_cast<ImmOperand&>(srcOpnd);
      int64 srcConstValue = srcConst.GetValue();
      if (srcConstValue == 1) {
        return true;
      }
      return false;
    }
    default:
      return false;
  }
}

/* if defined operand(must be first insn currently) in insn is const zero, return true */
bool OptimizePattern::InsnDefZero(Insn &insn) {
  MOperator defMop = insn.GetMachineOpcode();
  switch (defMop) {
    case MOP_xmovri32:
    case MOP_xmovri64: {
      Operand &srcOpnd = insn.GetOperand(kInsnSecondOpnd);
      ASSERT(srcOpnd.IsIntImmediate(), "expects ImmOperand");
      ImmOperand &srcConst = static_cast<ImmOperand&>(srcOpnd);
      int64 srcConstValue = srcConst.GetValue();
      if (srcConstValue == 0) {
        return true;
      }
      return false;
    }
    case MOP_xmovrr:
    case MOP_wmovrr:
      return insn.GetOperand(kInsnSecondOpnd).IsZeroRegister();
    default:
      return false;
  }
}

/* if defined operand(must be first insn currently) in insn has only one valid bit, return true */
bool OptimizePattern::InsnDefOneOrZero(Insn &insn) {
  MOperator defMop = insn.GetMachineOpcode();
  switch (defMop) {
    case MOP_wcsetrc:
    case MOP_xcsetrc:
      return true;
    case MOP_xmovri32:
    case MOP_xmovri64: {
      Operand &defOpnd = insn.GetOperand(kInsnSecondOpnd);
      ASSERT(defOpnd.IsIntImmediate(), "expects ImmOperand");
      auto &defConst = static_cast<ImmOperand&>(defOpnd);
      int64 defConstValue = defConst.GetValue();
      if (defConstValue != 0 && defConstValue != 1) {
        return false;
      } else {
        return true;
      }
    }
    case MOP_xmovrr:
    case MOP_wmovrr: {
      return insn.GetOperand(kInsnSecondOpnd).IsZeroRegister();
    }
    case MOP_wlsrrri5:
    case MOP_xlsrrri6: {
      Operand &opnd2 = insn.GetOperand(kInsnThirdOpnd);
      ASSERT(opnd2.IsIntImmediate(), "expects ImmOperand");
      ImmOperand &opndImm = static_cast<ImmOperand&>(opnd2);
      int64 shiftBits = opndImm.GetValue();
      if (((defMop == MOP_wlsrrri5) && (shiftBits == k32BitSize - 1)) ||
          ((defMop == MOP_xlsrrri6) && (shiftBits == k64BitSize - 1))) {
        return true;
      } else {
        return false;
      }
    }
    default:
      return false;
  }
}

void ReplaceAsmListReg(Insn *insn, uint32 index, uint32 regNO, Operand *newOpnd) {
  MapleList<RegOperand*> *list = &static_cast<AArch64ListOperand&>(insn->GetOperand(index)).GetOperands();
  int32 size = list->size();
  for (int i = 0; i < size; ++i) {
    RegOperand *opnd = static_cast<RegOperand *>(*(list->begin()));
    list->pop_front();
    if (opnd->GetRegisterNumber() == regNO) {
      list->push_back(static_cast<RegOperand *>(newOpnd));
    } else {
      list->push_back(opnd);
    }
  }
}

void OptimizePattern::ReplaceAllUsedOpndWithNewOpnd(const InsnSet &useInsnSet, uint32 regNO,
                                                    Operand &newOpnd, bool updateInfo) const {
  for (auto useInsn : useInsnSet) {
    if (useInsn->GetMachineOpcode() == MOP_asm) {
      ReplaceAsmListReg(useInsn, kAsmInputListOpnd, regNO, &newOpnd);
    }
    const AArch64MD *md = &AArch64CG::kMd[static_cast<AArch64Insn*>(useInsn)->GetMachineOpcode()];
    uint32 opndNum = useInsn->GetOperandSize();
    for (uint32 i = 0; i < opndNum; ++i) {
      Operand &opnd = useInsn->GetOperand(i);
      AArch64OpndProp *regProp = static_cast<AArch64OpndProp*>(md->operand[i]);
      if (!regProp->IsRegUse() && !opnd.IsMemoryAccessOperand()) {
        continue;
      }

      if (opnd.IsRegister() && (static_cast<RegOperand&>(opnd).GetRegisterNumber() == regNO)) {
        useInsn->SetOperand(i, newOpnd);
        if (updateInfo) {
          cgFunc.GetRD()->InitGenUse(*useInsn->GetBB(), false);
        }
      } else if (opnd.IsMemoryAccessOperand()) {
        AArch64MemOperand &memOpnd = static_cast<AArch64MemOperand&>(opnd);
        RegOperand *base = memOpnd.GetBaseRegister();
        RegOperand *index = memOpnd.GetIndexRegister();
        MemOperand *newMem = nullptr;
        if (base != nullptr && (base->GetRegisterNumber() == regNO)) {
          newMem = static_cast<MemOperand*>(opnd.Clone(*cgFunc.GetMemoryPool()));
          CHECK_FATAL(newMem != nullptr, "null ptr check");
          newMem->SetBaseRegister(*static_cast<RegOperand*>(&newOpnd));
          useInsn->SetOperand(i, *newMem);
          if (updateInfo) {
            cgFunc.GetRD()->InitGenUse(*useInsn->GetBB(), false);
          }
        }
        if (index != nullptr && (index->GetRegisterNumber() == regNO)) {
          newMem = static_cast<MemOperand*>(opnd.Clone(*cgFunc.GetMemoryPool()));
          CHECK_FATAL(newMem != nullptr, "null ptr check");
          newMem->SetIndexRegister(*static_cast<RegOperand*>(&newOpnd));
          if (static_cast<RegOperand&>(newOpnd).GetValidBitsNum() != index->GetValidBitsNum()) {
            static_cast<AArch64MemOperand*>(newMem)->UpdateExtend(AArch64MemOperand::kSignExtend);
          }
          useInsn->SetOperand(i, *newMem);
          if (updateInfo) {
            cgFunc.GetRD()->InitGenUse(*useInsn->GetBB(), false);
          }
        }
      }
    }
  }
}

bool ForwardPropPattern::CheckCondition(Insn &insn) {
  if (!insn.IsMachineInstruction()) {
    return false;
  }
  if ((insn.GetMachineOpcode() != MOP_xmovrr) && (insn.GetMachineOpcode() != MOP_wmovrr) &&
      (insn.GetMachineOpcode() != MOP_xmovrr_uxtw)) {
    return false;
  }
  Operand &firstOpnd = insn.GetOperand(kInsnFirstOpnd);
  Operand &secondOpnd = insn.GetOperand(kInsnSecondOpnd);
  if (firstOpnd.GetSize() != secondOpnd.GetSize() && insn.GetMachineOpcode() != MOP_xmovrr_uxtw) {
    return false;
  }
  RegOperand &firstRegOpnd = static_cast<RegOperand&>(firstOpnd);
  RegOperand &secondRegOpnd = static_cast<RegOperand&>(secondOpnd);
  uint32 firstRegNO = firstRegOpnd.GetRegisterNumber();
  uint32 secondRegNO = secondRegOpnd.GetRegisterNumber();
  if (firstRegOpnd.IsZeroRegister() || !firstRegOpnd.IsVirtualRegister() || !secondRegOpnd.IsVirtualRegister()) {
    return false;
  }
  firstRegUseInsnSet = cgFunc.GetRD()->FindUseForRegOpnd(insn, firstRegNO, true);
  if (firstRegUseInsnSet.empty()) {
    return false;
  }
  InsnSet secondRegDefInsnSet = cgFunc.GetRD()->FindDefForRegOpnd(insn, secondRegNO, true);
  if (secondRegDefInsnSet.size() != 1 || RegOperand::IsSameReg(firstOpnd, secondOpnd)) {
    return false;
  }
  bool toDoOpt = true;
  for (auto useInsn : firstRegUseInsnSet) {
    if (!cgFunc.GetRD()->RegIsLiveBetweenInsn(secondRegNO, insn, *useInsn)) {
      toDoOpt = false;
      break;
    }
    InsnSet defInsnSet = cgFunc.GetRD()->FindDefForRegOpnd(*useInsn, firstRegNO, true);
    if (defInsnSet.size() > 1) {
      toDoOpt = false;
      break;
    }
  }
  return toDoOpt;
}

void ForwardPropPattern::Optimize(Insn &insn) {
  Operand &firstOpnd = insn.GetOperand(kInsnFirstOpnd);
  Operand &secondOpnd = insn.GetOperand(kInsnSecondOpnd);
  RegOperand &firstRegOpnd = static_cast<RegOperand&>(firstOpnd);
  uint32 firstRegNO = firstRegOpnd.GetRegisterNumber();

  for (auto *useInsn : firstRegUseInsnSet) {
    if (useInsn->GetMachineOpcode() == MOP_asm) {
      ReplaceAsmListReg(useInsn, kAsmInputListOpnd, firstRegNO, &secondOpnd);
      cgFunc.GetRD()->InitGenUse(*useInsn->GetBB(), false);
      continue;
    }
    const AArch64MD *md = &AArch64CG::kMd[static_cast<AArch64Insn*>(useInsn)->GetMachineOpcode()];
    uint32 opndNum = useInsn->GetOperandSize();
    for (uint32 i = 0; i < opndNum; ++i) {
      Operand &opnd = useInsn->GetOperand(i);
      const AArch64OpndProp *regProp = md->GetOperand(i);
      if (!regProp->IsRegUse() && !opnd.IsMemoryAccessOperand()) {
        continue;
      }

      if (opnd.IsRegister() && (static_cast<RegOperand&>(opnd).GetRegisterNumber() == firstRegNO)) {
        useInsn->SetOperand(i, secondOpnd);
        if (((useInsn->GetMachineOpcode() == MOP_xmovrr) || (useInsn->GetMachineOpcode() == MOP_wmovrr)) &&
            (static_cast<RegOperand&>(useInsn->GetOperand(kInsnSecondOpnd)).IsVirtualRegister()) &&
            (static_cast<RegOperand&>(useInsn->GetOperand(kInsnFirstOpnd)).IsVirtualRegister())) {
          (void)modifiedBB.insert(useInsn->GetBB());
        }
        cgFunc.GetRD()->InitGenUse(*useInsn->GetBB(), false);
      } else if (opnd.IsMemoryAccessOperand()) {
        AArch64MemOperand &memOpnd = static_cast<AArch64MemOperand&>(opnd);
        RegOperand *base = memOpnd.GetBaseRegister();
        RegOperand *index = memOpnd.GetIndexRegister();
        MemOperand *newMem = nullptr;
        if (base != nullptr && (base->GetRegisterNumber() == firstRegNO)) {
          newMem = static_cast<MemOperand*>(opnd.Clone(*cgFunc.GetMemoryPool()));
          CHECK_FATAL(newMem != nullptr, "null ptr check");
          newMem->SetBaseRegister(static_cast<RegOperand&>(secondOpnd));
          useInsn->SetOperand(i, *newMem);
          cgFunc.GetRD()->InitGenUse(*useInsn->GetBB(), false);
        }
        if ((index != nullptr) && (index->GetRegisterNumber() == firstRegNO)) {
          newMem = static_cast<MemOperand*>(opnd.Clone(*cgFunc.GetMemoryPool()));
          CHECK_FATAL(newMem != nullptr, "null ptr check");
          newMem->SetIndexRegister(static_cast<RegOperand&>(secondOpnd));
          if (static_cast<RegOperand&>(secondOpnd).GetValidBitsNum() != index->GetValidBitsNum()) {
            static_cast<AArch64MemOperand*>(newMem)->UpdateExtend(AArch64MemOperand::kSignExtend);
          }
          useInsn->SetOperand(i, *newMem);
          cgFunc.GetRD()->InitGenUse(*useInsn->GetBB(), false);
        }
      }
    }
  }
  insn.SetOperand(0, secondOpnd);
  cgFunc.GetRD()->UpdateInOut(*insn.GetBB(), true);
}

void ForwardPropPattern::RemoveMopUxtwToMov(Insn &insn) {
  RegOperand &secondOpnd = static_cast<RegOperand&>(insn.GetOperand(kInsnSecondOpnd));
  RegOperand &newOpnd = static_cast<RegOperand&>(insn.GetOperand(kInsnFirstOpnd));
  newOpnd.SetRegisterNumber(secondOpnd.GetRegisterNumber());

  uint32 firstRegNo = static_cast<RegOperand&>(insn.GetOperand(kInsnFirstOpnd)).GetRegisterNumber();
  InsnSet firstRegUseInsnSet = cgFunc.GetRD()->FindUseForRegOpnd(insn, firstRegNo, true);
  if (firstRegUseInsnSet.size() >= 1) {
    for (auto defInsn : firstRegUseInsnSet) {
      int optSize = defInsn->GetOperandSize();
      for (int i = 0; i < optSize; i++) {
        if (firstRegNo == static_cast<RegOperand&>(defInsn->GetOperand(i)).GetRegisterNumber()) {
          defInsn->SetOperand(i, newOpnd);
        }
      }
      cgFunc.GetRD()->InitGenUse(*defInsn->GetBB(), false);
    }
  }
  insn.GetBB()->RemoveInsn(insn);
}
void ForwardPropPattern::Init() {
  firstRegUseInsnSet.clear();
}

void ForwardPropPattern::Run() {
  bool secondTime = false;
  do {
    FOR_ALL_BB(bb, &cgFunc) {
      if (bb->IsUnreachable() || (secondTime && modifiedBB.find(bb) == modifiedBB.end())) {
        continue;
      }

      if (secondTime) {
        modifiedBB.erase(bb);
      }

      FOR_BB_INSNS(insn, bb) {
        Init();
        if (!CheckCondition(*insn)) {
          if (insn->GetMachineOpcode() == MOP_xmovrr_uxtw) {
            insn->SetMOperator(MOP_xuxtw64);
          }
          continue;
        }
        if (insn->GetMachineOpcode() == MOP_xmovrr_uxtw) {
          RemoveMopUxtwToMov(*insn);
          continue;
        }
        Optimize(*insn);
      }
    }
    secondTime = true;
  } while (!modifiedBB.empty());
}

bool BackPropPattern::CheckAndGetOpnd(Insn &insn) {
  if (!insn.IsMachineInstruction()) {
    return false;
  }
  if ((insn.GetMachineOpcode() != MOP_xmovrr) && (insn.GetMachineOpcode() != MOP_wmovrr)) {
    return false;
  }
  Operand &firstOpnd = insn.GetOperand(kInsnFirstOpnd);
  Operand &secondOpnd = insn.GetOperand(kInsnSecondOpnd);
  if (RegOperand::IsSameReg(firstOpnd, secondOpnd)) {
    return false;
  }
  if (firstOpnd.GetSize() != secondOpnd.GetSize()) {
    return false;
  }
  firstRegOpnd = &static_cast<RegOperand&>(firstOpnd);
  secondRegOpnd = &static_cast<RegOperand&>(secondOpnd);
  if (firstRegOpnd->IsZeroRegister() || !secondRegOpnd->IsVirtualRegister()) {
    return false;
  }
  firstRegNO = firstRegOpnd->GetRegisterNumber();
  secondRegNO = secondRegOpnd->GetRegisterNumber();
  return true;
}

bool BackPropPattern::DestOpndHasUseInsns(Insn &insn) {
  BB &bb = *insn.GetBB();
  InsnSet useInsnSetOfFirstOpnd;
  bool findRes = cgFunc.GetRD()->FindRegUseBetweenInsn(firstRegNO, insn.GetNext(),
                                                       bb.GetLastInsn(), useInsnSetOfFirstOpnd);
  if ((findRes && useInsnSetOfFirstOpnd.empty()) ||
      (!findRes && useInsnSetOfFirstOpnd.empty() && !bb.GetLiveOut()->TestBit(firstRegNO))) {
    return false;
  }
  return true;
}

bool BackPropPattern::DestOpndLiveOutToEHSuccs(Insn &insn) {
  BB &bb = *insn.GetBB();
  for (auto ehSucc : bb.GetEhSuccs()) {
    if (ehSucc->GetLiveIn()->TestBit(firstRegNO)) {
      return true;
    }
  }
  return false;
}

bool BackPropPattern::CheckSrcOpndDefAndUseInsns(Insn &insn) {
  BB &bb = *insn.GetBB();
  /* secondOpnd is defined in other BB */
  std::vector<Insn*> defInsnVec = cgFunc.GetRD()->FindRegDefBetweenInsn(secondRegNO, bb.GetFirstInsn(), insn.GetPrev());
  if (defInsnVec.size() != 1) {
    return false;
  }
  defInsnForSecondOpnd = defInsnVec.back();
  if (defInsnForSecondOpnd->GetMachineOpcode() == MOP_asm) {
    return false;
  }
  /* part defined */
  if ((defInsnForSecondOpnd->GetMachineOpcode() == MOP_xmovkri16) ||
      (defInsnForSecondOpnd->GetMachineOpcode() == MOP_wmovkri16)) {
    return false;
  }
  bool findFinish = cgFunc.GetRD()->FindRegUseBetweenInsn(secondRegNO, defInsnForSecondOpnd->GetNext(),
                                                          bb.GetLastInsn(), srcOpndUseInsnSet);
  if (!findFinish && bb.GetLiveOut()->TestBit(secondRegNO)) {
    return false;
  }
  return true;
}

bool BackPropPattern::CheckPredefineInsn(Insn &insn) {
  if (insn.GetPrev() == defInsnForSecondOpnd) {
    return true;
  }
  std::vector<Insn*> preDefInsnForFirstOpndVec;
  BB &bb = *insn.GetBB();
  if (cgFunc.GetRD()->CheckRegGen(bb, firstRegNO)) {
    preDefInsnForFirstOpndVec =
        cgFunc.GetRD()->FindRegDefBetweenInsn(firstRegNO, defInsnForSecondOpnd->GetNext(), insn.GetPrev());
  }
  if (!preDefInsnForFirstOpndVec.empty()) {
    return false;
  }
  /* there is no predefine insn in current bb */
  InsnSet useInsnSetForFirstOpnd;
  cgFunc.GetRD()->FindRegUseBetweenInsn(firstRegNO, defInsnForSecondOpnd->GetNext(), insn.GetPrev(),
                                        useInsnSetForFirstOpnd);
  if (!useInsnSetForFirstOpnd.empty()) {
    return false;
  }
  return true;
}

bool BackPropPattern::CheckRedefineInsn(Insn &insn) {
  for (auto useInsn : srcOpndUseInsnSet) {
    if ((useInsn->GetId() > insn.GetId()) && (insn.GetNext() != useInsn) &&
        !cgFunc.GetRD()->FindRegDefBetweenInsn(firstRegNO, insn.GetNext(), useInsn->GetPrev()).empty()) {
      return false;
    }
  }
  return true;
}

bool BackPropPattern::CheckCondition(Insn &insn) {
  if (!CheckAndGetOpnd(insn)) {
    return false;
  }
  /* Unless there is a reason that dest can not live out the current BB */
  if (cgFunc.HasAsm() && !DestOpndHasUseInsns(insn)) {
    return false;
  }
  /* first register must not be live out to eh_succs */
  if (DestOpndLiveOutToEHSuccs(insn)) {
    return false;
  }
  if (!CheckSrcOpndDefAndUseInsns(insn)) {
    return false;
  }
  /* check predefine insn */
  if (!CheckPredefineInsn(insn)) {
    return false;
  }
  /* check redefine insn */
  if (!CheckRedefineInsn(insn)) {
    return false;
  }
  return true;
}

void BackPropPattern::Optimize(Insn &insn) {
  Operand &firstOpnd = insn.GetOperand(kInsnFirstOpnd);
  ReplaceAllUsedOpndWithNewOpnd(srcOpndUseInsnSet, secondRegNO, firstOpnd, false);
  /* replace define insn */
  const AArch64MD *md = &AArch64CG::kMd[static_cast<AArch64Insn*>(defInsnForSecondOpnd)->GetMachineOpcode()];
  uint32 opndNum = defInsnForSecondOpnd->GetOperandSize();
  for (uint32 i = 0; i < opndNum; ++i) {
    Operand &opnd = defInsnForSecondOpnd->GetOperand(i);
    AArch64OpndProp *regProp = static_cast<AArch64OpndProp*>(md->operand[i]);
    if (!regProp->IsRegDef() && !opnd.IsMemoryAccessOperand()) {
      continue;
    }

    if (opnd.IsRegister() && (static_cast<RegOperand&>(opnd).GetRegisterNumber() == secondRegNO)) {
      defInsnForSecondOpnd->SetOperand(i, firstOpnd);
    } else if (opnd.IsMemoryAccessOperand()) {
      AArch64MemOperand &memOpnd = static_cast<AArch64MemOperand&>(opnd);
      RegOperand *base = memOpnd.GetBaseRegister();
      if (base != nullptr && memOpnd.GetAddrMode() == AArch64MemOperand::kAddrModeBOi &&
          (memOpnd.IsPostIndexed() || memOpnd.IsPreIndexed()) && base->GetRegisterNumber() == secondRegNO) {
        MemOperand *newMem = static_cast<MemOperand*>(opnd.Clone(*cgFunc.GetMemoryPool()));
        CHECK_FATAL(newMem != nullptr, "null ptr check");
        newMem->SetBaseRegister(static_cast<RegOperand&>(firstOpnd));
        defInsnForSecondOpnd->SetOperand(i, *newMem);
      }
    }
  }

  /* There is special implication when backward propagation is allowed for physical register R0.
   * This is a case that the calling func foo directly returns the result from the callee bar as follows:
   *
   * foo:
   * bl                                               bl // bar()
   * mov vreg, X0  //res = bar()        naive bkprop
   * ....          //X0 is not redefined    ====>        ....  //X0 may be reused as RA sees "X0 has not been used" after bl
   * mov X0, vreg                                              //In fact, X0 is implicitly used by foo. We need to tell RA that X0 is live
   * ret                                              ret
   *
   * To make RA simple, we tell RA to not use X0 by keeping "mov X0, X0". That is
   * foo:
   * bl //bar()
   * ....          // Perform backward prop X0 and force X0 cant be reused
   * mov X0, X0    // This can be easily remved later in peephole phase
   * ret
   */
  if (cgFunc.HasCall() &&
      !(cgFunc.GetFunction().IsReturnVoid()) &&
      (firstRegNO == R0) &&
      (static_cast<RegOperand&>(insn.GetOperand(kInsnSecondOpnd)).GetRegisterNumber() == R0))  {
    /* Keep this instruction: mov R0, R0 */
    return;
  } else {
    insn.GetBB()->RemoveInsn(insn);
  }
}

void BackPropPattern::Init() {
  firstRegOpnd = nullptr;
  secondRegOpnd = nullptr;
  firstRegNO = 0;
  secondRegNO = 0;
  srcOpndUseInsnSet.clear();
  defInsnForSecondOpnd = nullptr;
}

void BackPropPattern::Run() {
  bool secondTime = false;
  std::set<BB*, BBIdCmp> modifiedBB;
  do {
    FOR_ALL_BB(bb, &cgFunc) {
      if (bb->IsUnreachable() || (secondTime && modifiedBB.find(bb) == modifiedBB.end())) {
        continue;
      }

      if (secondTime) {
        modifiedBB.erase(bb);
      }

      FOR_BB_INSNS_REV(insn, bb) {
        Init();
        if (!CheckCondition(*insn)) {
          continue;
        }
        (void)modifiedBB.insert(bb);
        Optimize(*insn);
      }
      cgFunc.GetRD()->UpdateInOut(*bb);
    }
    secondTime = true;
  } while (!modifiedBB.empty());
}

bool CmpCsetPattern::CheckCondition(Insn &insn) {
  nextInsn = insn.GetNextMachineInsn();
  if (nextInsn == nullptr || !insn.IsMachineInstruction()) {
    return false;
  }

  MOperator firstMop = insn.GetMachineOpcode();
  MOperator secondMop = nextInsn->GetMachineOpcode();
  if (!(((firstMop == MOP_wcmpri) || (firstMop == MOP_xcmpri)) &&
        ((secondMop == MOP_wcsetrc) || (secondMop == MOP_xcsetrc)))) {
    return false;
  }

  /* get cmp_first operand */
  cmpFirstOpnd = &(insn.GetOperand(kInsnSecondOpnd));
  /* get cmp second Operand, ImmOperand must be 0 or 1 */
  cmpSecondOpnd = &(insn.GetOperand(kInsnThirdOpnd));
  ASSERT(cmpSecondOpnd->IsIntImmediate(), "expects ImmOperand");
  ImmOperand *cmpConstOpnd = static_cast<ImmOperand*>(cmpSecondOpnd);
  cmpConstVal = cmpConstOpnd->GetValue();
  /* get cset first Operand */
  csetFirstOpnd = &(nextInsn->GetOperand(kInsnFirstOpnd));
  if (((cmpConstVal != 0) && (cmpConstVal != 1)) || (cmpFirstOpnd->GetSize() != csetFirstOpnd->GetSize()) ||
      !OpndDefByOneOrZero(insn, 1)) {
    return false;
  }

  InsnSet useInsnSet = cgFunc.GetRD()->FindUseForRegOpnd(insn, 0, false);
  if (useInsnSet.size() > 1) {
    return false;
  }
  return true;
}

void CmpCsetPattern::Optimize(Insn &insn) {
  Insn *csetInsn = nextInsn;
  BB &bb = *insn.GetBB();
  nextInsn = nextInsn->GetNextMachineInsn();
  /* get condition Operand */
  CondOperand &cond = static_cast<CondOperand&>(csetInsn->GetOperand(kInsnSecondOpnd));
  if (((cmpConstVal == 0) && (cond.GetCode() == CC_NE)) || ((cmpConstVal == 1) && (cond.GetCode() == CC_EQ))) {
    if (RegOperand::IsSameReg(*cmpFirstOpnd, *csetFirstOpnd)) {
      bb.RemoveInsn(insn);
      bb.RemoveInsn(*csetInsn);
    } else {
      MOperator mopCode = (cmpFirstOpnd->GetSize() == k64BitSize) ? MOP_xmovrr : MOP_wmovrr;
      Insn &newInsn = cgFunc.GetCG()->BuildInstruction<AArch64Insn>(mopCode, *csetFirstOpnd, *cmpFirstOpnd);
      newInsn.SetId(insn.GetId());
      bb.ReplaceInsn(insn, newInsn);
      bb.RemoveInsn(*csetInsn);
    }
  } else if (((cmpConstVal == 1) && (cond.GetCode() == CC_NE)) ||
             ((cmpConstVal == 0) && (cond.GetCode() == CC_EQ))) {
    MOperator mopCode = (cmpFirstOpnd->GetSize() == k64BitSize) ? MOP_xeorrri13 : MOP_weorrri12;
    constexpr int64 eorImm = 1;
    auto &aarch64CGFunc = static_cast<AArch64CGFunc&>(cgFunc);
    ImmOperand &one = aarch64CGFunc.CreateImmOperand(eorImm, k8BitSize, false);
    Insn &newInsn = cgFunc.GetCG()->BuildInstruction<AArch64Insn>(mopCode, *csetFirstOpnd, *cmpFirstOpnd, one);
    newInsn.SetId(insn.GetId());
    bb.ReplaceInsn(insn, newInsn);
    bb.RemoveInsn(*csetInsn);
  }

  cgFunc.GetRD()->UpdateInOut(bb, true);
}

void CmpCsetPattern::Init() {
  cmpConstVal  = 0;
  cmpFirstOpnd = nullptr;
  cmpSecondOpnd = nullptr;
  csetFirstOpnd = nullptr;
}

void CmpCsetPattern::Run() {
  FOR_ALL_BB(bb, &cgFunc) {
    FOR_BB_INSNS(insn, bb) {
      Init();
      if (!CheckCondition(*insn)) {
        continue;
      }
      Optimize(*insn);
    }
  }
}

AArch64CC_t CselPattern::GetInverseCondCode(const CondOperand &cond) const {
  switch (cond.GetCode()) {
    case CC_NE:
      return CC_EQ;
    case CC_EQ:
      return CC_NE;
    case CC_LT:
      return CC_GE;
    case CC_GE:
      return CC_LT;
    case CC_GT:
      return CC_LE;
    case CC_LE:
      return CC_GT;
    default:
      return kCcLast;
  }
}

bool CselPattern::CheckCondition(Insn &insn) {
  MOperator mopCode = insn.GetMachineOpcode();
  if ((mopCode != MOP_xcselrrrc) && (mopCode != MOP_wcselrrrc)) {
    return false;
  }
  return true;
}

void CselPattern::Optimize(Insn &insn) {
  BB &bb = *insn.GetBB();
  Operand &opnd0 = insn.GetOperand(kInsnFirstOpnd);
  Operand &cond = insn.GetOperand(kInsnFourthOpnd);
  MOperator newMop = ((opnd0.GetSize()) == k64BitSize ? MOP_xcsetrc : MOP_wcsetrc);
  Operand &rflag = cgFunc.GetOrCreateRflag();
  if (OpndDefByOne(insn, kInsnSecondOpnd) && OpndDefByZero(insn, kInsnThirdOpnd)) {
    Insn &newInsn = cgFunc.GetCG()->BuildInstruction<AArch64Insn>(newMop, opnd0, cond, rflag);
    newInsn.SetId(insn.GetId());
    bb.ReplaceInsn(insn, newInsn);
    cgFunc.GetRD()->InitGenUse(bb, false);
  } else if (OpndDefByZero(insn, kInsnSecondOpnd) && OpndDefByOne(insn, kInsnThirdOpnd)) {
    CondOperand &originCond = static_cast<CondOperand&>(cond);
    AArch64CC_t inverseCondCode = GetInverseCondCode(originCond);
    if (inverseCondCode == kCcLast) {
      return;
    }
    auto &aarchCGFunc = static_cast<AArch64CGFunc&>(cgFunc);
    CondOperand &inverseCond = aarchCGFunc.GetCondOperand(inverseCondCode);
    Insn &newInsn = cgFunc.GetCG()->BuildInstruction<AArch64Insn>(newMop, opnd0, inverseCond, rflag);
    newInsn.SetId(insn.GetId());
    bb.ReplaceInsn(insn, newInsn);
    cgFunc.GetRD()->InitGenUse(bb, false);
  }
}

void CselPattern::Run() {
  FOR_ALL_BB(bb, &cgFunc) {
    FOR_BB_INSNS_SAFE(insn, bb, nextInsn) {
      if (!CheckCondition(*insn)) {
        continue;
      }
      Optimize(*insn);
    }
  }
}

uint32 RedundantUxtPattern::GetInsnValidBit(Insn &insn) {
  MOperator mOp = insn.GetMachineOpcode();
  uint32 nRet;
  switch (mOp) {
    case MOP_wcsetrc:
    case MOP_xcsetrc:
      nRet = 1;
      break;
    case MOP_wldrb:
    case MOP_wldarb:
    case MOP_wldxrb:
    case MOP_wldaxrb:
      nRet = k8BitSize;
      break;
    case MOP_wldrh:
    case MOP_wldarh:
    case MOP_wldxrh:
    case MOP_wldaxrh:
      nRet = k16BitSize;
      break;
    case MOP_wmovrr:
    case MOP_xmovri32:
    case MOP_wldrsb:
    case MOP_wldrsh:
    case MOP_wldli:
    case MOP_wldr:
    case MOP_wldp:
    case MOP_wldar:
    case MOP_wmovkri16:
    case MOP_wmovzri16:
    case MOP_wmovnri16:
    case MOP_wldxr:
    case MOP_wldaxr:
    case MOP_wldaxp:
    case MOP_wcsincrrrc:
    case MOP_wcselrrrc:
    case MOP_wcsinvrrrc:
      nRet = k32BitSize;
      break;
    default:
      nRet = k64BitSize;
      break;
  }
  return nRet;
}

uint32 RedundantUxtPattern::GetMaximumValidBit(Insn &insn, uint8 index, InsnSet &visitedInsn) const {
  InsnSet defInsnSet = cgFunc.GetRD()->FindDefForRegOpnd(insn, index);
  if (defInsnSet.empty()) {
    /* disable opt when there is no def point. */
    return k64BitSize;
  }

  uint32 validBit = 0;
  uint32 nMaxValidBit = 0;
  for (auto &defInsn : defInsnSet) {
    if (visitedInsn.find(defInsn) != visitedInsn.end()) {
      continue;
    }

    (void)visitedInsn.insert(defInsn);
    MOperator mOp = defInsn->GetMachineOpcode();
    if ((mOp == MOP_wmovrr) || (mOp == MOP_xmovrr)) {
      validBit = GetMaximumValidBit(*defInsn, 1, visitedInsn);
    } else {
      validBit = GetInsnValidBit(*defInsn);
    }

    nMaxValidBit = nMaxValidBit < validBit ? validBit : nMaxValidBit;
  }
  return nMaxValidBit;
}

bool RedundantUxtPattern::CheckCondition(Insn &insn) {
  BB &bb = *insn.GetBB();
  InsnSet visitedInsn1;
  InsnSet visitedInsn2;
  if (!((insn.GetMachineOpcode() == MOP_xuxth32 &&
         GetMaximumValidBit(insn, kInsnSecondOpnd, visitedInsn1) <= k16BitSize) ||
        (insn.GetMachineOpcode() == MOP_xuxtb32 &&
         GetMaximumValidBit(insn, kInsnSecondOpnd, visitedInsn2) <= k8BitSize))) {
    return false;
  }

  Operand &firstOpnd = insn.GetOperand(kInsnFirstOpnd);
  secondOpnd = &(insn.GetOperand(kInsnSecondOpnd));
  if (RegOperand::IsSameReg(firstOpnd, *secondOpnd)) {
    bb.RemoveInsn(insn);
    /* update in/out */
    cgFunc.GetRD()->UpdateInOut(bb, true);
    return false;
  }
  useInsnSet = cgFunc.GetRD()->FindUseForRegOpnd(insn, 0, false);
  RegOperand &firstRegOpnd = static_cast<RegOperand&>(firstOpnd);
  firstRegNO = firstRegOpnd.GetRegisterNumber();
  /* for uxth R1, V501, R1 is parameter register, this can't be optimized. */
  if (firstRegOpnd.IsPhysicalRegister()) {
    return false;
  }

  if (useInsnSet.empty()) {
    bb.RemoveInsn(insn);
    /* update in/out */
    cgFunc.GetRD()->UpdateInOut(bb, true);
    return false;
  }

  RegOperand *secondRegOpnd = static_cast<RegOperand*>(secondOpnd);
  ASSERT(secondRegOpnd != nullptr, "secondRegOpnd should not be nullptr");
  uint32 secondRegNO = secondRegOpnd->GetRegisterNumber();
  for (auto useInsn : useInsnSet) {
    InsnSet defInsnSet = cgFunc.GetRD()->FindDefForRegOpnd(*useInsn, firstRegNO, true);
    if ((defInsnSet.size() > 1) || !(cgFunc.GetRD()->RegIsLiveBetweenInsn(secondRegNO, insn, *useInsn))) {
      return false;
    }
  }
  return true;
}

void RedundantUxtPattern::Optimize(Insn &insn) {
  BB &bb = *insn.GetBB();
  ReplaceAllUsedOpndWithNewOpnd(useInsnSet, firstRegNO, *secondOpnd, true);
  bb.RemoveInsn(insn);
  cgFunc.GetRD()->UpdateInOut(bb, true);
}

void RedundantUxtPattern::Init() {
  useInsnSet.clear();
  secondOpnd = nullptr;
}

void RedundantUxtPattern::Run() {
  FOR_ALL_BB(bb, &cgFunc) {
    if (bb->IsUnreachable()) {
      continue;
    }
    FOR_BB_INSNS_SAFE(insn, bb, nextInsn) {
      Init();
      if (!CheckCondition(*insn)) {
        continue;
      }
      Optimize(*insn);
    }
  }
}

bool LocalVarSaveInsnPattern::CheckFirstInsn(Insn &firstInsn) {
  MOperator mOp = firstInsn.GetMachineOpcode();
  if (mOp != MOP_xmovrr && mOp != MOP_wmovrr) {
    return false;
  }
  firstInsnSrcOpnd = &(firstInsn.GetOperand(kInsnSecondOpnd));
  RegOperand *firstInsnSrcReg = static_cast<RegOperand*>(firstInsnSrcOpnd);
  if (firstInsnSrcReg->GetRegisterNumber() != R0) {
    return false;
  }
  firstInsnDestOpnd = &(firstInsn.GetOperand(kInsnFirstOpnd));
  RegOperand *firstInsnDestReg = static_cast<RegOperand*>(firstInsnDestOpnd);
  if (firstInsnDestReg->IsPhysicalRegister()) {
    return false;
  }
  return true;
}

bool LocalVarSaveInsnPattern::CheckSecondInsn() {
  MOperator mOp = secondInsn->GetMachineOpcode();
  if (mOp != MOP_wstr && mOp != MOP_xstr) {
    return false;
  }
  secondInsnSrcOpnd = &(secondInsn->GetOperand(kInsnFirstOpnd));
  if (!RegOperand::IsSameReg(*firstInsnDestOpnd, *secondInsnSrcOpnd)) {
    return false;
  }
  /* check memOperand is stack memOperand, and x0 is stored in localref var region */
  secondInsnDestOpnd = &(secondInsn->GetOperand(kInsnSecondOpnd));
  AArch64MemOperand *secondInsnDestMem = static_cast<AArch64MemOperand*>(secondInsnDestOpnd);
  RegOperand *baseReg = secondInsnDestMem->GetBaseRegister();
  RegOperand *indexReg = secondInsnDestMem->GetIndexRegister();
  if ((baseReg == nullptr) || !(cgFunc.IsFrameReg(*baseReg)) || (indexReg != nullptr)) {
    return false;
  }
  return true;
}

bool LocalVarSaveInsnPattern::CheckAndGetUseInsn(Insn &firstInsn) {
  InsnSet useInsnSet = cgFunc.GetRD()->FindUseForRegOpnd(firstInsn, kInsnFirstOpnd, false);
  if (useInsnSet.size() != 2) { /* secondInsn and another useInsn */
    return false;
  }

  /* useInsnSet includes secondInsn and another useInsn */
  for (auto tmpUseInsn : useInsnSet) {
    if (tmpUseInsn->GetId() != secondInsn->GetId()) {
      useInsn = tmpUseInsn;
      break;
    }
  }
  return true;
}

bool LocalVarSaveInsnPattern::CheckLiveRange(Insn &firstInsn) {
  uint32 maxInsnNO = cgFunc.GetRD()->GetMaxInsnNO();
  uint32 useInsnID = useInsn->GetId();
  uint32 defInsnID = firstInsn.GetId();
  uint32 distance = useInsnID > defInsnID ? useInsnID - defInsnID : defInsnID - useInsnID;
  float liveRangeProportion = static_cast<float>(distance) / maxInsnNO;
  /* 0.3 is a balance for real optimization effect */
  if (liveRangeProportion < 0.3) {
    return false;
  }
  return true;
}

bool LocalVarSaveInsnPattern::CheckCondition(Insn &firstInsn) {
  secondInsn = firstInsn.GetNext();
  if (secondInsn == nullptr) {
    return false;
  }
  /* check firstInsn is : mov vreg, R0; */
  if (!CheckFirstInsn(firstInsn)) {
    return false;
  }
  /* check the secondInsn is : str vreg, stackMem */
  if (!CheckSecondInsn()) {
    return false;
  }
  /* find the uses of the vreg */
  if (!CheckAndGetUseInsn(firstInsn)) {
    return false;
  }
  /* simulate live range using insn distance */
  if (!CheckLiveRange(firstInsn)) {
    return false;
  }
  RegOperand *firstInsnDestReg = static_cast<RegOperand*>(firstInsnDestOpnd);
  regno_t firstInsnDestRegNO = firstInsnDestReg->GetRegisterNumber();
  InsnSet defInsnSet = cgFunc.GetRD()->FindDefForRegOpnd(*useInsn, firstInsnDestRegNO, true);
  if (defInsnSet.size() != 1) {
    return false;
  }
  ASSERT((*(defInsnSet.begin()))->GetId() == firstInsn.GetId(), "useInsn has only one define Insn : firstInsn");
  /* check whether the stack mem is changed or not */
  AArch64MemOperand *secondInsnDestMem = static_cast<AArch64MemOperand*>(secondInsnDestOpnd);
  int64 memOffset = secondInsnDestMem->GetOffsetImmediate()->GetOffsetValue();
  InsnSet memDefInsnSet = cgFunc.GetRD()->FindDefForMemOpnd(*useInsn, memOffset, true);
  if (memDefInsnSet.size() != 1) {
    return false;
  }
  if ((*(memDefInsnSet.begin()))->GetId() != secondInsn->GetId()) {
    return false;
  }
   /* check whether has call between use and def */
  if (!cgFunc.GetRD()->HasCallBetweenDefUse(firstInsn, *useInsn)) {
    return false;
  }
  return true;
}

void LocalVarSaveInsnPattern::Optimize(Insn &insn) {
  /* insert ldr insn before useInsn */
  MOperator ldrOpCode = secondInsnSrcOpnd->GetSize() == k64BitSize ? MOP_xldr : MOP_wldr;
  Insn &ldrInsn = cgFunc.GetCG()->BuildInstruction<AArch64Insn>(ldrOpCode, *secondInsnSrcOpnd, *secondInsnDestOpnd);
  ldrInsn.SetId(useInsn->GetId() - 1);
  useInsn->GetBB()->InsertInsnBefore(*useInsn, ldrInsn);
  cgFunc.GetRD()->UpdateInOut(*useInsn->GetBB(), true);
  secondInsn->SetOperand(kInsnFirstOpnd, *firstInsnSrcOpnd);
  BB *saveInsnBB = insn.GetBB();
  saveInsnBB->RemoveInsn(insn);
  cgFunc.GetRD()->UpdateInOut(*saveInsnBB, true);
}

void LocalVarSaveInsnPattern::Init() {
  firstInsnSrcOpnd = nullptr;
  firstInsnDestOpnd = nullptr;
  secondInsnSrcOpnd = nullptr;
  secondInsnDestOpnd = nullptr;
  useInsn = nullptr;
  secondInsn = nullptr;
}

void LocalVarSaveInsnPattern::Run() {
  FOR_ALL_BB(bb, &cgFunc) {
    if (bb->IsCleanup()) {
      continue;
    }
    FOR_BB_INSNS(insn, bb) {
      if (!insn->IsCall()) {
        continue;
      }
      Insn *firstInsn = insn->GetNextMachineInsn();
      if (firstInsn == nullptr) {
        continue;
      }
      Init();
      if (!CheckCondition(*firstInsn)) {
        continue;
      }
      Optimize(*firstInsn);
    }
  }
}

bool ExtendShiftOptPattern::CheckCurrMop(Insn &insn) {
  MOperator op = insn.GetMachineOpcode();
  switch (op) {
    case MOP_xaddrrr:
    case MOP_waddrrr:
    case MOP_xsubrrr:
    case MOP_wsubrrr:
    case MOP_xeorrrr:
    case MOP_weorrrr:
    case MOP_xiorrrr:
    case MOP_wiorrrr:
    case MOP_wcmnrr:
    case MOP_xcmnrr:
    case MOP_wcmprr:
    case MOP_xcmprr:
      return true;
    case MOP_winegrr:
    case MOP_xinegrr:
      replaceIdx = kInsnSecondOpnd;
      return true;
    default:
      return false;
  }
}

void ExtendShiftOptPattern::SelectReplaceOp(Insn &use) {
  MOperator op = use.GetMachineOpcode();
  bool isExten = (extendOp != ExtendShiftOperand::kUndef);
  switch (op) {
    case MOP_xaddrrr: replaceOp = isExten ? MOP_xxwaddrrre : MOP_xaddrrrs;
      break;
    case MOP_waddrrr: replaceOp = isExten ? MOP_wwwaddrrre : MOP_waddrrrs;
      break;
    case MOP_xsubrrr: replaceOp = isExten ? MOP_xxwsubrrre : MOP_xsubrrrs;
      break;
    case MOP_wsubrrr: replaceOp = isExten ? MOP_wwwsubrrre : MOP_wsubrrrs;
      break;
    case MOP_xeorrrr: replaceOp = isExten ? MOP_undef : MOP_xeorrrrs;
      break;
    case MOP_weorrrr: replaceOp = isExten ? MOP_undef : MOP_weorrrrs;
      break;
    case MOP_xinegrr: replaceOp = isExten ? MOP_undef : MOP_xinegrrs;
      break;
    case MOP_winegrr: replaceOp = isExten ? MOP_undef : MOP_winegrrs;
      break;
    case MOP_xiorrrr: replaceOp = isExten ? MOP_undef : MOP_xiorrrrs;
      break;
    case MOP_wiorrrr: replaceOp = isExten ? MOP_undef : MOP_wiorrrrs;
      break;
    case MOP_xcmnrr: replaceOp = isExten ? MOP_xwcmnrre : MOP_xcmnrrs;
      break;
    case MOP_wcmnrr: replaceOp = isExten ? MOP_wwcmnrre : MOP_wcmnrrs;
      break;
    case MOP_xcmprr: replaceOp = isExten ? MOP_xwcmprre : MOP_xcmprrs;
      break;
    case MOP_wcmprr: replaceOp = isExten ? MOP_wwcmprre : MOP_wcmprrs;
      break;
    default:
      CHECK_FATAL(0, "check use op!");
  }
}

void ExtendShiftOptPattern::SelectExtenOp(const Insn &def) {
  MOperator op = def.GetMachineOpcode();
  switch (op) {
    case MOP_xsxtb32:
    case MOP_xsxtb64: extendOp = ExtendShiftOperand::kSXTB;
      break;
    case MOP_xsxth32:
    case MOP_xsxth64: extendOp = ExtendShiftOperand::kSXTH;
      break;
    case MOP_xsxtw64: extendOp = ExtendShiftOperand::kSXTW;
      break;
    case MOP_xuxtb32: extendOp = ExtendShiftOperand::kUXTB;
      break;
    case MOP_xuxth32: extendOp = ExtendShiftOperand::kUXTH;
      break;
    case MOP_xuxtw64: extendOp = ExtendShiftOperand::kUXTW;
      break;
    case MOP_wlslrri5:
    case MOP_xlslrri6: shiftOp = BitShiftOperand::kLSL;
      break;
    case MOP_xlsrrri6:
    case MOP_wlsrrri5: shiftOp = BitShiftOperand::kLSR;
      break;
    default: {
      extendOp = ExtendShiftOperand::kUndef;
      shiftOp = BitShiftOperand::kUndef;
    }
  }
}

/* first use must match SelectExtenOp */
bool ExtendShiftOptPattern::CheckDefUseInfo(const Insn &use, const Insn &def) {
  Operand &useOperand = use.GetOperand(replaceIdx);
  Operand &defOperand = def.GetOperand(0);
  Operand &defSrcOpnd = def.GetOperand(kInsnSecondOpnd);
  if (useOperand.GetSize() != defOperand.GetSize()) {
    return false;
  }
  AArch64RegOperand &regUse = static_cast<AArch64RegOperand&>(useOperand);
  AArch64RegOperand &regDef = static_cast<AArch64RegOperand&>(defOperand);
  /* case:
   * lsl w0, w0, #5
   * eor w0, w2, w0
   * --->
   * eor w0, w2, w0, lsl 5
   */
  if (defSrcOpnd.IsRegister()) {
    AArch64RegOperand &regDefSrc = static_cast<AArch64RegOperand&>(defSrcOpnd);
    if (regDef.GetRegisterNumber() == regDefSrc.GetRegisterNumber()) {
      return false;
    }
  }

  return regUse.GetRegisterNumber() == regDef.GetRegisterNumber();
}

void ExtendShiftOptPattern::ReplaceUseInsn(Insn &use, Insn &def, uint32 amount) {
  if (amount > k4ByteSize) {
    return;
  }
  AArch64CGFunc &a64CGFunc = static_cast<AArch64CGFunc&>(cgFunc);
  Operand &firstOpnd = use.GetOperand(kInsnFirstOpnd);
  Operand *secondOpnd = &use.GetOperand(kInsnSecondOpnd);
  /* thirdOpnd is extend or bitshift operand, depend on defInsn */
  Operand *shiftOpnd = nullptr;
  if (extendOp != ExtendShiftOperand::kUndef) {
    shiftOpnd = &a64CGFunc.CreateExtendShiftOperand(extendOp, amount, k64BitSize);
  } else if (shiftOp != BitShiftOperand::kUndef) {
    shiftOpnd = &a64CGFunc.CreateBitShiftOperand(shiftOp, amount, k64BitSize);
  } else {
    CHECK_FATAL(0, "extendOp and shiftOp are undef!");
  }

  SelectReplaceOp(use);
  if (replaceOp == MOP_undef) {
    return;
  }
  /* replace neg insn */
  Insn *replaceUseInsn = nullptr;
  if (replaceIdx == kInsnSecondOpnd) {
    secondOpnd = &def.GetOperand(kInsnSecondOpnd);
    replaceUseInsn = &cgFunc.GetCG()->BuildInstruction<AArch64Insn>(replaceOp, firstOpnd, *secondOpnd, *shiftOpnd);
  } else {
    Operand &thirdOpnd = def.GetOperand(kInsnSecondOpnd);
    replaceUseInsn = &cgFunc.GetCG()->BuildInstruction<AArch64Insn>(replaceOp, firstOpnd, *secondOpnd,
                                                                    thirdOpnd, *shiftOpnd);
  }
  use.GetBB()->ReplaceInsn(use, *replaceUseInsn);
  if (GLOBAL_DUMP) {
    LogInfo::MapleLogger() << "=======ReplaceInsn :\n";
    use.Dump();
    LogInfo::MapleLogger() << "=======NewInsn :\n";
    replaceUseInsn->Dump();
  }
}

/*
 * pattern1:
 * UXTB/UXTW X0, W1              <---- def x0
 * ....                          <---- (X0 not used)
 * AND/SUB/EOR X0, X1, X0        <---- use x0
 * ======>
 * AND/SUB/EOR X0, X1, W1 UXTB/UXTW
 *
 * pattern2:
 * UXTB/UXTW X1, W1
 * LSL X0, X1, #2
 * ....(X0 not used)
 * AND/SUB/EOR X0, X1, X0
 * ======>
 * AND/SUB/EOR X0, X1, W1 UXTB/UXTW #2
 *
 * pattern3:
 * LSL/LSR X0, X1, #8
 * ....(X0 not used)
 * AND/SUB/EOR X0, X1, X0
 * ======>
 * AND/SUB/EOR X0, X1, X1 LSL/LSR #8
 */
void ExtendShiftOptPattern::Optimize(Insn &insn) {
  if (shiftOp == BitShiftOperand::kLSL) {
    InsnSet preDef = cgFunc.GetRD()->FindDefForRegOpnd(*defInsn, kInsnSecondOpnd, false);
    if (preDef.size() == 1) {
      Insn *preDefInsn = *preDef.begin();
      CHECK_FATAL((preDefInsn != nullptr), "defInsn is null!");
      SelectExtenOp(*preDefInsn);
      /* preDefInsn must be uxt/sxt */
      if (extendOp != ExtendShiftOperand::kUndef) {
        AArch64ImmOperand &immOpnd = static_cast<AArch64ImmOperand &>(defInsn->GetOperand(kInsnThirdOpnd));
        /* do pattern2 */
        ReplaceUseInsn(insn, *preDefInsn, immOpnd.GetValue());
        return;
      }
    }
  }
  /* reset shiftOp and extendOp */
  SelectExtenOp(*defInsn);

  if (extendOp != ExtendShiftOperand::kUndef) {
    /* do pattern1 */
    ReplaceUseInsn(insn, *defInsn, 0);
  } else if (shiftOp != BitShiftOperand::kUndef) {
    /* do pattern3 */
    AArch64ImmOperand &immOpnd = static_cast<AArch64ImmOperand&>(defInsn->GetOperand(kInsnThirdOpnd));
    ReplaceUseInsn(insn, *defInsn, immOpnd.GetValue());
  } else {
    CHECK_FATAL(false, "extendOp and shiftOp is Both kUndef!");
  }
}

bool ExtendShiftOptPattern::CheckCondition(Insn &insn) {
  if (!CheckCurrMop(insn)) {
    return false;
  }
  if (insn.GetOperandSize() > k3ByteSize) {
    return false;
  }
  InsnSet regDefInsnSet = cgFunc.GetRD()->FindDefForRegOpnd(insn, replaceIdx, false);
  if (regDefInsnSet.empty()) {
    return false;
  }
  defInsn = *regDefInsnSet.begin();
  CHECK_FATAL((defInsn != nullptr), "defInsn is null!");

  SelectExtenOp(*defInsn);
  /* defInsn must be shift or extend */
  if ((extendOp == ExtendShiftOperand::kUndef) && (shiftOp == BitShiftOperand::kUndef)) {
    return false;
  }
  /* The optimization of cross-BB before RA prolongs the live range of vreg.
   * As a result, the calleesave register is preferentially used when RA allocates registers.
   * more calleesave register will increase stackmem size.
   * str x23, [x29,#48]
   * ...
   * ldr x23, [x29,#48]
   * will be added.
   */
  if (defInsn->GetBB() != insn.GetBB()) {
    return false;
  }
  return CheckDefUseInfo(insn, *defInsn);
}

void ExtendShiftOptPattern::Init() {
  replaceOp = MOP_undef;
  extendOp = ExtendShiftOperand::kUndef;
  shiftOp = BitShiftOperand::kUndef;
  defInsn = nullptr;
  replaceIdx = kInsnThirdOpnd;
}

void ExtendShiftOptPattern::Run() {
  if (!cgFunc.GetMirModule().IsCModule()) {
    return;
  }
  FOR_ALL_BB_REV(bb, &cgFunc) {
    FOR_BB_INSNS_REV(insn, bb) {
      if (!insn->IsMachineInstruction()) {
        continue;
      }
      Init();
      if (!CheckCondition(*insn)) {
        continue;
      }
      Optimize(*insn);
    }
  }
}

void UxtwMovPattern::Run() {
  if (!cgFunc.GetMirModule().IsCModule()) {
    return;
  }
  FOR_ALL_BB(bb, &cgFunc) {
    FOR_BB_INSNS(insn, bb) {
      if (!insn->IsMachineInstruction()) {
        continue;
      }
      if (!CheckCondition(*insn)) {
        continue;
      }
      Optimize(*insn);
    }
  }
}

/* Check for Implicit uxtw */
bool UxtwMovPattern::CheckHideUxtw(Insn &insn, regno_t regno) {
  const AArch64MD *md = &AArch64CG::kMd[static_cast<AArch64Insn&>(insn).GetMachineOpcode()];
  int optSize = insn.GetOperandSize();
  for (int i = 0; i < optSize; i++) {
    if (regno == static_cast<RegOperand&>(insn.GetOperand(i)).GetRegisterNumber()) {
      AArch64OpndProp *curOpndProp = md->GetOperand(i);
      if (curOpndProp->IsDef() && curOpndProp->GetSize() == k32BitSize) {
        return true;
      }
      break;
    }
  }
  return false;
}

bool UxtwMovPattern::CheckCondition(Insn &insn) {
  if (insn.GetMachineOpcode() == MOP_xuxtw64) {
    if (insn.GetOperand(kInsnFirstOpnd).GetSize() == k64BitSize &&
        insn.GetOperand(kInsnSecondOpnd).GetSize() == k32BitSize) {
      ASSERT(insn.GetOperand(kInsnSecondOpnd).IsRegister(), "is not Register");
      regno_t regno = static_cast<RegOperand&>(insn.GetOperand(kInsnSecondOpnd)).GetRegisterNumber();
      InsnSet preDef = cgFunc.GetRD()->FindDefForRegOpnd(insn, kInsnSecondOpnd, false);
      if (preDef.size() >= 1) {
        bool isHideUxtw = false;
        for (auto defInsn : preDef) {
          isHideUxtw = CheckHideUxtw(*defInsn, regno);
          if (!isHideUxtw) {
            return false;
          }
        }
        return true;
      }
    }
  }
  return false;
}

/* No initialization required */
void UxtwMovPattern::Init() {}

void UxtwMovPattern::Optimize(Insn &insn) {
  insn.SetMOperator(MOP_xmovrr_uxtw);
}
}  /* namespace maplebe */
