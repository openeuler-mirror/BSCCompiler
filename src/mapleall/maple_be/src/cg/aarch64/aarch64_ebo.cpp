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
#include "aarch64_ebo.h"
#include "aarch64_cg.h"
#include "mpl_logging.h"
#include "aarch64_utils.h"

namespace maplebe {
using namespace maple;
#define EBO_DUMP CG_DEBUG_FUNC(*cgFunc)

enum AArch64Ebo::ExtOpTable : uint8 {
  AND,
  SXTB,
  SXTH,
  SXTW,
  ZXTB,
  ZXTH,
  ZXTW,
  ExtTableSize
};

namespace {

using PairMOperator = MOperator[2];

constexpr uint8 insPairsNum = 5;

PairMOperator extInsnPairTable[ExtTableSize][insPairsNum] = {
  /* {origMop, newMop} */
  {{MOP_wldrb, MOP_wldrb},  {MOP_undef, MOP_undef},   {MOP_undef, MOP_undef},   {MOP_undef, MOP_undef},
   {MOP_undef, MOP_undef}},   /* AND */
  {{MOP_wldrb, MOP_wldrsb}, {MOP_wldr, MOP_wldrsb},   {MOP_undef, MOP_undef},   {MOP_undef, MOP_undef},
   {MOP_undef, MOP_undef}},   /* SXTB */
  {{MOP_wldrh, MOP_wldrsh}, {MOP_wldrb, MOP_wldrb},   {MOP_wldrsb, MOP_wldrsb}, {MOP_wldrsh, MOP_wldrsh},
   {MOP_undef, MOP_undef}},   /* SXTH */
  {{MOP_wldrh, MOP_wldrh},  {MOP_wldrsh, MOP_wldrsh}, {MOP_wldrb, MOP_wldrb},   {MOP_wldrsb, MOP_wldrsb},
   {MOP_wldr, MOP_xldrsw}}, /* SXTW */
  {{MOP_wldrb, MOP_wldrb},  {MOP_wldrsb, MOP_wldrb},  {MOP_undef, MOP_undef},   {MOP_undef, MOP_undef},
   {MOP_undef, MOP_undef}},   /* ZXTB */
  {{MOP_wldrh, MOP_wldrh},  {MOP_wldrb, MOP_wldrb},   {MOP_wldr, MOP_wldrh},   {MOP_undef, MOP_undef},
   {MOP_undef, MOP_undef}},   /* ZXTH */
  {{MOP_wldr, MOP_wldr},    {MOP_wldrh, MOP_wldrh},   {MOP_wldrb, MOP_wldrb},   {MOP_undef, MOP_undef},
   {MOP_undef, MOP_undef}}    /* ZXTW */
};

} // anonymous namespace

MOperator AArch64Ebo::ExtLoadSwitchBitSize(MOperator lowMop) const {
  switch (lowMop) {
    case MOP_wldrsb :
      return MOP_xldrsb;
    case MOP_wldrsh :
      return MOP_xldrsh;
    default:
      break;
  }
  return lowMop;
}

bool AArch64Ebo::IsFmov(const Insn &insn) const {
  return ((MOP_xvmovsr <= insn.GetMachineOpcode()) && (insn.GetMachineOpcode() <= MOP_xvmovrd));
}

bool AArch64Ebo::IsAdd(const Insn &insn) const {
  return ((MOP_xaddrrr <= insn.GetMachineOpcode()) && (insn.GetMachineOpcode() <= MOP_ssub));
}

bool AArch64Ebo::IsZeroRegister(const Operand &opnd) const {
  if (!opnd.IsRegister()) {
    return false;
  }
  const AArch64RegOperand *regOpnd = static_cast<const AArch64RegOperand*>(&opnd);
  return regOpnd->IsZeroRegister();
}

bool AArch64Ebo::IsClinitCheck(const Insn &insn) const {
  MOperator mOp = insn.GetMachineOpcode();
  return ((mOp == MOP_clinit) || (mOp == MOP_clinit_tail));
}

/* retrun true if insn is globalneeded */
bool AArch64Ebo::IsGlobalNeeded(Insn &insn) const {
  /* Calls may have side effects. */
  if (insn.IsCall()) {
    return true;
  }

  /* Intrinsic call should not be removed. */
  if (insn.IsSpecialIntrinsic()) {
    return true;
  }

  /* Clinit should not be removed. */
  if (insn.IsFixedInsn()) {
    return true;
  }

  /* Yieldpoints should not be removed by optimizer. */
  if (cgFunc->GetCG()->GenYieldPoint() && insn.IsYieldPoint()) {
    return true;
  }

  Operand *opnd = insn.GetResult(0);
  if ((opnd != nullptr) && (opnd->IsConstReg() || (opnd->IsRegister() && static_cast<RegOperand*>(opnd)->IsSPOrFP()))) {
    return true;
  }
  return false;
}

/* in aarch64,resOp will not be def and use in the same time */
bool AArch64Ebo::ResIsNotDefAndUse(Insn &insn) const {
  (void)insn;
  return true;
}

/* Return true if opnd live out of bb. */
bool AArch64Ebo::LiveOutOfBB(const Operand &opnd, const BB &bb) const {
  CHECK_FATAL(opnd.IsRegister(), "expect register here.");
  /* when optimize_level < 2, there is need to anlyze live range. */
  if (live == nullptr) {
    return false;
  }
  bool isLiveOut = false;
  if (bb.GetLiveOut()->TestBit(static_cast<RegOperand const*>(&opnd)->GetRegisterNumber())) {
    isLiveOut = true;
  }
  return isLiveOut;
}

bool AArch64Ebo::IsLastAndBranch(BB &bb, Insn &insn) const {
  return (bb.GetLastInsn() == &insn) && insn.IsBranch();
}

bool AArch64Ebo::IsSameRedefine(BB &bb, Insn &insn, OpndInfo &opndInfo) const {
  MOperator mOp = insn.GetMachineOpcode();
  if (!(mOp == MOP_xmovri32 || mOp == MOP_xmovri64 || mOp == MOP_wsfmovri || mOp == MOP_xdfmovri)) {
    return false;
  }
  OpndInfo *sameInfo = opndInfo.same;
  if (sameInfo == nullptr || sameInfo->insn == nullptr || sameInfo->bb != &bb ||
      sameInfo->insn->GetMachineOpcode() != mOp) {
    return false;
  }
  Insn *prevInsn = sameInfo->insn;
  if (!prevInsn->GetOperand(kInsnSecondOpnd).IsImmediate()) {
    return false;
  }
  auto &sameOpnd = static_cast<AArch64ImmOperand&>(prevInsn->GetOperand(kInsnSecondOpnd));
  auto &opnd = static_cast<AArch64ImmOperand&>(insn.GetOperand(kInsnSecondOpnd));
  if (sameOpnd.GetValue() == opnd.GetValue()) {
    sameInfo->refCount += opndInfo.refCount;
    return true;
  }
  return false;
}

const RegOperand &AArch64Ebo::GetRegOperand(const Operand &opnd) const {
  CHECK_FATAL(opnd.IsRegister(), "aarch64 shoud not have regShiftOp! opnd is not register!");
  const auto &res = static_cast<const RegOperand&>(opnd);
  return res;
}

/* Create infomation for local_opnd from its def insn current_insn. */
OpndInfo *AArch64Ebo::OperandInfoDef(BB &currentBB, Insn &currentInsn, Operand &localOpnd) {
  int32 hashVal = localOpnd.IsRegister() ? -1 : ComputeOpndHash(localOpnd);
  OpndInfo *opndInfoPrev = GetOpndInfo(localOpnd, hashVal);
  OpndInfo *opndInfo = GetNewOpndInfo(currentBB, &currentInsn, localOpnd, hashVal);
  if (localOpnd.IsMemoryAccessOperand()) {
    MemOpndInfo *memInfo = static_cast<MemOpndInfo*>(opndInfo);
    MemOperand *mem = static_cast<MemOperand*>(&localOpnd);
    Operand *base = mem->GetBaseRegister();
    Operand *offset = mem->GetOffset();
    if (base != nullptr && base->IsRegister()) {
      memInfo->SetBaseInfo(*OperandInfoUse(currentBB, *base));
    }
    if (offset != nullptr && offset->IsRegister()) {
      memInfo->SetOffsetInfo(*OperandInfoUse(currentBB, *offset));
    }
  }
  opndInfo->same = opndInfoPrev;
  if ((opndInfoPrev != nullptr)) {
    opndInfoPrev->redefined = true;
    if (opndInfoPrev->bb == &currentBB) {
      opndInfoPrev->redefinedInBB = true;
      opndInfoPrev->redefinedInsn = &currentInsn;
    }
    UpdateOpndInfo(localOpnd, *opndInfoPrev, opndInfo, hashVal);
  } else {
    SetOpndInfo(localOpnd, opndInfo, hashVal);
  }
  return opndInfo;
}

void AArch64Ebo::DefineClinitSpecialRegisters(InsnInfo &insnInfo) {
  Insn *insn = insnInfo.insn;
  CHECK_FATAL(insn != nullptr, "nullptr of currInsnInfo");
  RegOperand &phyOpnd1 = a64CGFunc->GetOrCreatePhysicalRegisterOperand(R16, k64BitSize, kRegTyInt);
  OpndInfo *opndInfo = OperandInfoDef(*insn->GetBB(), *insn, phyOpnd1);
  opndInfo->insnInfo = &insnInfo;

  RegOperand &phyOpnd2 = a64CGFunc->GetOrCreatePhysicalRegisterOperand(R17, k64BitSize, kRegTyInt);
  opndInfo = OperandInfoDef(*insn->GetBB(), *insn, phyOpnd2);
  opndInfo->insnInfo = &insnInfo;
}

void AArch64Ebo::BuildCallerSaveRegisters() {
  callerSaveRegTable.clear();
  RegOperand &phyOpndR0 = a64CGFunc->GetOrCreatePhysicalRegisterOperand(R0, k64BitSize, kRegTyInt);
  RegOperand &phyOpndV0 = a64CGFunc->GetOrCreatePhysicalRegisterOperand(V0, k64BitSize, kRegTyFloat);
  callerSaveRegTable.emplace_back(&phyOpndR0);
  callerSaveRegTable.emplace_back(&phyOpndV0);
  for (uint32 i = R1; i <= R18; i++) {
    RegOperand &phyOpnd =
        a64CGFunc->GetOrCreatePhysicalRegisterOperand(static_cast<AArch64reg>(i), k64BitSize, kRegTyInt);
    callerSaveRegTable.emplace_back(&phyOpnd);
  }
  for (uint32 i = V1; i <= V7; i++) {
    RegOperand &phyOpnd =
        a64CGFunc->GetOrCreatePhysicalRegisterOperand(static_cast<AArch64reg>(i), k64BitSize, kRegTyFloat);
    callerSaveRegTable.emplace_back(&phyOpnd);
  }
  for (uint32 i = V16; i <= V31; i++) {
    RegOperand &phyOpnd =
        a64CGFunc->GetOrCreatePhysicalRegisterOperand(static_cast<AArch64reg>(i), k64BitSize, kRegTyFloat);
    callerSaveRegTable.emplace_back(&phyOpnd);
  }
  CHECK_FATAL(callerSaveRegTable.size() < kMaxCallerSaveReg,
      "number of elements in callerSaveRegTable must less then 45!");
}

void AArch64Ebo::DefineAsmRegisters(InsnInfo &insnInfo) {
  Insn *insn = insnInfo.insn;
  ASSERT(insn->GetMachineOpcode() == MOP_asm, "insn should be a call insn.");
  ListOperand &outList = const_cast<ListOperand&>(
      static_cast<const ListOperand&>(insn->GetOperand(kAsmOutputListOpnd)));
  for (auto opnd : outList.GetOperands()) {
    OpndInfo *opndInfo = OperandInfoDef(*insn->GetBB(), *insn, *opnd);
    opndInfo->insnInfo = &insnInfo;
  }
  ListOperand &clobberList = const_cast<ListOperand&>(
      static_cast<const ListOperand&>(insn->GetOperand(kAsmClobberListOpnd)));
  for (auto opnd : clobberList.GetOperands()) {
    OpndInfo *opndInfo = OperandInfoDef(*insn->GetBB(), *insn, *opnd);
    opndInfo->insnInfo = &insnInfo;
  }
  ListOperand &inList = const_cast<ListOperand&>(static_cast<const ListOperand&>(insn->GetOperand(kAsmInputListOpnd)));
  for (auto opnd : inList.GetOperands()) {
    OperandInfoUse(*(insn->GetBB()), *opnd);
  }
}

void AArch64Ebo::DefineCallerSaveRegisters(InsnInfo &insnInfo) {
  Insn *insn = insnInfo.insn;
  if (insn->IsAsmInsn()) {
    DefineAsmRegisters(insnInfo);
    return;
  }
  ASSERT(insn->IsCall() || insn->IsTailCall(), "insn should be a call insn.");
  if (CGOptions::DoIPARA()) {
    auto *targetOpnd = insn->GetCallTargetOperand();
    CHECK_FATAL(targetOpnd != nullptr, "target is null in AArch64Insn::IsCallToFunctionThatNeverReturns");
    if (targetOpnd->IsFuncNameOpnd()) {
      FuncNameOperand *target = static_cast<FuncNameOperand*>(targetOpnd);
      const MIRSymbol *funcSt = target->GetFunctionSymbol();
      ASSERT(funcSt->GetSKind() == kStFunc, "funcst must be a function name symbol");
      MIRFunction *func = funcSt->GetFunction();
      if (func != nullptr && func->IsReferedRegsValid()) {
        for (auto preg : func->GetReferedRegs()) {
          if (AArch64Abi::IsCalleeSavedReg(static_cast<AArch64reg>(preg))) {
            continue;
          }
          RegOperand *opnd = &a64CGFunc->GetOrCreatePhysicalRegisterOperand((AArch64reg)preg, k64BitSize,
              AArch64isa::IsFPSIMDRegister((AArch64reg)preg) ? kRegTyFloat : kRegTyInt);
          OpndInfo *opndInfo = OperandInfoDef(*insn->GetBB(), *insn, *opnd);
          opndInfo->insnInfo = &insnInfo;
        }
        return;
      }
    }
  }
  for (auto opnd : callerSaveRegTable) {
    OpndInfo *opndInfo = OperandInfoDef(*insn->GetBB(), *insn, *opnd);
    opndInfo->insnInfo = &insnInfo;
  }
}

void AArch64Ebo::DefineReturnUseRegister(Insn &insn) {
  /* Define scalar callee save register and FP, LR. */
  for (uint32 i = R19; i <= R30; i++) {
    RegOperand &phyOpnd =
        a64CGFunc->GetOrCreatePhysicalRegisterOperand(static_cast<AArch64reg>(i), k64BitSize, kRegTyInt);
    OperandInfoUse(*insn.GetBB(), phyOpnd);
  }

  /* Define SP */
  RegOperand &phyOpndSP =
      a64CGFunc->GetOrCreatePhysicalRegisterOperand(static_cast<AArch64reg>(RSP), k64BitSize, kRegTyInt);
  OperandInfoUse(*insn.GetBB(), phyOpndSP);

  /* Define FP callee save registers. */
  for (uint32 i = V8; i <= V15; i++) {
    RegOperand &phyOpnd =
        a64CGFunc->GetOrCreatePhysicalRegisterOperand(static_cast<AArch64reg>(i), k64BitSize, kRegTyFloat);
    OperandInfoUse(*insn.GetBB(), phyOpnd);
  }
}

void AArch64Ebo::DefineCallUseSpecialRegister(Insn &insn) {
  if (insn.GetMachineOpcode() == MOP_asm) {
    return;
  }
  AArch64reg fpRegNO = RFP;
  if (!beforeRegAlloc && cgFunc->UseFP()) {
    fpRegNO = R29;
  }
  /* Define FP, LR. */
  RegOperand &phyOpndFP =
      a64CGFunc->GetOrCreatePhysicalRegisterOperand(fpRegNO, k64BitSize, kRegTyInt);
  OperandInfoUse(*insn.GetBB(), phyOpndFP);
  RegOperand &phyOpndLR =
      a64CGFunc->GetOrCreatePhysicalRegisterOperand(static_cast<AArch64reg>(RLR), k64BitSize, kRegTyInt);
  OperandInfoUse(*insn.GetBB(), phyOpndLR);

  /* Define SP */
  RegOperand &phyOpndSP =
      a64CGFunc->GetOrCreatePhysicalRegisterOperand(static_cast<AArch64reg>(RSP), k64BitSize, kRegTyInt);
  OperandInfoUse(*insn.GetBB(), phyOpndSP);
}

/* return true if op1 == op2 */
bool AArch64Ebo::OperandEqSpecial(const Operand &op1, const Operand &op2) const {
  switch (op1.GetKind()) {
    case Operand::kOpdRegister: {
      const AArch64RegOperand &reg1 = static_cast<const AArch64RegOperand&>(op1);
      const AArch64RegOperand &reg2 = static_cast<const AArch64RegOperand&>(op2);
      return reg1 == reg2;
    }
    case Operand::kOpdImmediate: {
      const ImmOperand &imm1 = static_cast<const ImmOperand&>(op1);
      const ImmOperand &imm2 = static_cast<const ImmOperand&>(op2);
      return imm1 == imm2;
    }
    case Operand::kOpdOffset: {
      const AArch64OfstOperand &ofst1 = static_cast<const AArch64OfstOperand&>(op1);
      const AArch64OfstOperand &ofst2 = static_cast<const AArch64OfstOperand&>(op2);
      return ofst1 == ofst2;
    }
    case Operand::kOpdStImmediate: {
      const StImmOperand &stImm1 = static_cast<const StImmOperand&>(op1);
      const StImmOperand &stImm2 = static_cast<const StImmOperand&>(op2);
      return stImm1 == stImm2;
    }
    case Operand::kOpdMem: {
      const AArch64MemOperand &mem1 = static_cast<const AArch64MemOperand&>(op1);
      const AArch64MemOperand &mem2 = static_cast<const AArch64MemOperand&>(op2);
      if (mem1.GetAddrMode() == mem2.GetAddrMode()) {
        ASSERT(mem1.GetBaseRegister() != nullptr, "nullptr check");
        ASSERT(mem2.GetBaseRegister() != nullptr, "nullptr check");
      }
      return ((mem1.GetAddrMode() == mem2.GetAddrMode()) &&
              OperandEqual(*(mem1.GetBaseRegister()), *(mem2.GetBaseRegister())) &&
              OperandEqual(*(mem1.GetIndexRegister()), *(mem2.GetIndexRegister())) &&
              OperandEqual(*(mem1.GetOffsetOperand()), *(mem2.GetOffsetOperand())) &&
              (mem1.GetSymbol() == mem2.GetSymbol()) && (mem1.GetSize() == mem2.GetSize()));
    }
    default: {
      return false;
    }
  }
}

int32 AArch64Ebo::GetOffsetVal(const MemOperand &mem) const {
  const AArch64MemOperand &memOpnd = static_cast<const AArch64MemOperand&>(mem);
  AArch64OfstOperand *offset = memOpnd.GetOffsetImmediate();
  int32 val = 0;
  if (offset != nullptr) {
    val += static_cast<int>(offset->GetOffsetValue());

    if (offset->IsSymOffset() || offset->IsSymAndImmOffset()) {
      val += offset->GetSymbol()->GetStIdx().Idx();
    }
  }
  return val;
}

/*
 * move vreg1, #1
 * move vreg2, vreg1
 * ===>
 * move vreg1, #1
 * move vreg2, #1
 * return true if do simplify successfully.
 */
bool AArch64Ebo::DoConstProp(Insn &insn, uint32 idx, Operand &opnd) {
  AArch64ImmOperand *src = static_cast<AArch64ImmOperand*>(&opnd);
  const AArch64MD *md = &AArch64CG::kMd[(insn.GetMachineOpcode())];
  /* avoid the invalid case "cmp wzr, #0"/"add w1, wzr, #100" */
  Operand &destOpnd = insn.GetOperand(idx);
  if (src->IsZero() && destOpnd.IsRegister() &&
      (static_cast<RegOperand&>(destOpnd).GetRegisterType() == kRegTyInt) &&
      (insn.IsStore() || insn.IsMove() || md->IsCondDef())) {
    insn.SetOperand(idx, *GetZeroOpnd(src->GetSize()));
    return true;
  }
  MOperator mopCode = insn.GetMachineOpcode();
  switch (mopCode) {
    case MOP_xmovrr:
    case MOP_wmovrr: {
      ASSERT(idx == kInsnSecondOpnd, "src const for move must be the second operand.");
      uint32 targetSize = insn.GetOperand(idx).GetSize();
      if (src->GetSize() != targetSize) {
        src = static_cast<AArch64ImmOperand*>(src->Clone(*cgFunc->GetMemoryPool()));
        CHECK_FATAL(src != nullptr, "pointer result is null");
        src->SetSize(targetSize);
      }
      if (src->IsSingleInstructionMovable() && (insn.GetOperand(kInsnFirstOpnd).GetSize() == targetSize)) {
        if (EBO_DUMP) {
          LogInfo::MapleLogger() << " Do constprop:Prop constval " << src->GetValue() << "into insn:\n";
          insn.Dump();
        }
        insn.SetOperand(kInsnSecondOpnd, *src);
        MOperator mOp = (mopCode == MOP_wmovrr) ? MOP_xmovri32 : MOP_xmovri64;
        insn.SetMOP(mOp);
        if (EBO_DUMP) {
          LogInfo::MapleLogger() << " after constprop the insn is:\n";
          insn.Dump();
        }
        return true;
      }
      break;
    }
    case MOP_xaddrrr:
    case MOP_waddrrr:
    case MOP_xsubrrr:
    case MOP_wsubrrr: {
      if ((idx != kInsnThirdOpnd) || !src->IsInBitSize(kMaxImmVal24Bits, 0) ||
          !(src->IsInBitSize(kMaxImmVal12Bits, 0) || src->IsInBitSize(kMaxImmVal12Bits, kMaxImmVal12Bits))) {
        return false;
      }
      Operand &result = insn.GetOperand(0);
      bool is64Bits = (result.GetSize() == k64BitSize);
      if (EBO_DUMP) {
        LogInfo::MapleLogger() << " Do constprop:Prop constval " << src->GetValue() << "into insn:\n";
        insn.Dump();
      }
      if (src->IsZero()) {
        MOperator mOp = is64Bits ? MOP_xmovrr : MOP_wmovrr;
        insn.SetMOP(mOp);
        insn.PopBackOperand();
        if (EBO_DUMP) {
          LogInfo::MapleLogger() << " after constprop the insn is:\n";
          insn.Dump();
        }
        return true;
      }
      insn.SetOperand(kInsnThirdOpnd, *src);
      if ((mopCode == MOP_xaddrrr) || (mopCode == MOP_waddrrr)) {
        is64Bits ? insn.SetMOP(MOP_xaddrri12) : insn.SetMOP(MOP_waddrri12);
      } else if ((mopCode == MOP_xsubrrr) || (mopCode == MOP_wsubrrr)) {
        is64Bits ? insn.SetMOP(MOP_xsubrri12) : insn.SetMOP(MOP_wsubrri12);
      }
      if (EBO_DUMP) {
        LogInfo::MapleLogger() << " after constprop the insn is:\n";
        insn.Dump();
      }
      return true;
    }
    default:
      break;
  }
  return false;
}

/* optimize csel to cset */
bool AArch64Ebo::Csel2Cset(Insn &insn, const MapleVector<Operand*> &opnds) {
  MOperator opCode = insn.GetMachineOpcode();

  if (insn.GetOpndNum() == 0) {
    return false;
  }

  Operand *res = insn.GetResult(0);

  /* csel ->cset */
  if ((opCode == MOP_wcselrrrc) || (opCode == MOP_xcselrrrc)) {
    ASSERT(res != nullptr, "expect a register");
    ASSERT(res->IsRegister(), "expect a register");
    /* only do integers */
    RegOperand *reg = static_cast<RegOperand*>(res);
    if ((res == nullptr) || (!reg->IsOfIntClass())) {
      return false;
    }
    Operand *op0 = opnds.at(kInsnSecondOpnd);
    Operand *op1 = opnds.at(kInsnThirdOpnd);
    AArch64ImmOperand *imm0 = nullptr;
    AArch64ImmOperand *imm1 = nullptr;
    if (op0->IsImmediate()) {
      imm0 = static_cast<AArch64ImmOperand*>(op0);
    }
    if (op1->IsImmediate()) {
      imm1 = static_cast<AArch64ImmOperand*>(op1);
    }

    bool reverse = (imm1 != nullptr) && imm1->IsOne() &&
                   (((imm0 != nullptr) && imm0->IsZero()) || op0->IsZeroRegister());
    if (((imm0 != nullptr) && imm0->IsOne() && (((imm1 != nullptr) && imm1->IsZero()) || op1->IsZeroRegister())) ||
        reverse) {
      if (EBO_DUMP) {
        LogInfo::MapleLogger() << "change csel insn :\n";
        insn.Dump();
      }
      AArch64CGFunc *aarFunc = static_cast<AArch64CGFunc *>(cgFunc);
      Operand *result = insn.GetResult(0);
      Operand &condOperand = insn.GetOperand(kInsnFourthOpnd);
      Operand &rflag = aarFunc->GetOrCreateRflag();
      if (!reverse) {
        Insn &newInsn = cgFunc->GetCG()->BuildInstruction<AArch64Insn>(
            (opCode == MOP_xcselrrrc) ? MOP_xcsetrc : MOP_wcsetrc, *result, condOperand, rflag);
        insn.GetBB()->ReplaceInsn(insn, newInsn);
        if (EBO_DUMP) {
          LogInfo::MapleLogger() << "to cset insn ====>\n";
          newInsn.Dump();
        }
      } else {
        auto &cond = static_cast<CondOperand&>(condOperand);
        if (!CheckCondCode(cond)) {
          return false;
        }
        CondOperand &reverseCond = a64CGFunc->GetCondOperand(GetReverseCond(cond));
        Insn &newInsn = cgFunc->GetCG()->BuildInstruction<AArch64Insn>(
            (opCode == MOP_xcselrrrc) ? MOP_xcsetrc : MOP_wcsetrc, *result, reverseCond, rflag);
        insn.GetBB()->ReplaceInsn(insn, newInsn);
        if (EBO_DUMP) {
          LogInfo::MapleLogger() << "to cset insn ====>\n";
          newInsn.Dump();
        }
      }
      return true;
    }
  }
  return false;
}

/* Look at an expression that has a constant operand and attempt to simplify the computations. */
bool AArch64Ebo::SimplifyConstOperand(Insn &insn, const MapleVector<Operand*> &opnds,
                                      const MapleVector<OpndInfo*> &opndInfo) {
  BB *bb = insn.GetBB();
  bool result = false;
  if (insn.GetOpndNum() < 1) {
    return false;
  }
  ASSERT(opnds.size() > 1, "opnds size must greater than 1");
  Operand *op0 = opnds[kInsnSecondOpnd];
  Operand *op1 = opnds[kInsnThirdOpnd];
  Operand *res = insn.GetResult(0);
  CHECK_FATAL(res != nullptr, "null ptr check");
  const AArch64MD *md = &AArch64CG::kMd[static_cast<AArch64Insn*>(&insn)->GetMachineOpcode()];
  uint32 opndSize = md->GetOperandSize();
  bool op0IsConstant = op0->IsConstant() && !op1->IsConstant();
  bool op1IsConstant = !op0->IsConstant() && op1->IsConstant();
  bool bothConstant = op0->IsConstant() && op1->IsConstant();
  AArch64ImmOperand *immOpnd = nullptr;
  Operand *op = nullptr;
  int32 idx0 = kInsnSecondOpnd;
  if (op0IsConstant) {
    // cannot convert zero reg (r30) to a immOperand
    immOpnd = op0->IsZeroRegister() ? &a64CGFunc->CreateImmOperand(0, op0->GetSize(), false)
                                    : static_cast<AArch64ImmOperand*>(op0);
    op = op1;
    if (op->IsMemoryAccessOperand()) {
      op = &(insn.GetOperand(kInsnThirdOpnd));
    }
    idx0 = kInsnThirdOpnd;
  } else if (op1IsConstant) {
    // cannot convert zero reg (r30) to a immOperand
    immOpnd = op1->IsZeroRegister() ? &a64CGFunc->CreateImmOperand(0, op1->GetSize(), false)
                                    : static_cast<AArch64ImmOperand*>(op1);
    op = op0;
    if (op->IsMemoryAccessOperand()) {
      op = &(insn.GetOperand(kInsnSecondOpnd));
    }
  } else if (bothConstant) {
    AArch64ImmOperand *immOpnd0 = op0->IsZeroRegister() ? &a64CGFunc->CreateImmOperand(0, op0->GetSize(), false)
                                                        : static_cast<AArch64ImmOperand*>(op0);
    AArch64ImmOperand *immOpnd1 = op1->IsZeroRegister() ? &a64CGFunc->CreateImmOperand(0, op1->GetSize(), false)
                                                        : static_cast<AArch64ImmOperand*>(op1);
    return SimplifyBothConst(*insn.GetBB(), insn, *immOpnd0, *immOpnd1, opndSize);
  }
  CHECK_FATAL(immOpnd != nullptr, "constant operand required!");
  CHECK_FATAL(op != nullptr, "constant operand required!");
  /* For orr insn and one of the opnd is zero
   * orr resOp, imm1, #0  |  orr resOp, #0, imm1
   * =======>
   * mov resOp, imm1 */
  if (((insn.GetMachineOpcode() == MOP_wiorrri12) || (insn.GetMachineOpcode() == MOP_xiorrri13)) && immOpnd->IsZero()) {
    MOperator mOp = opndSize == k64BitSize ? MOP_xmovrr : MOP_wmovrr;
    Insn &newInsn = cgFunc->GetCG()->BuildInstruction<AArch64Insn>(mOp, *res, *op);
    bb->ReplaceInsn(insn, newInsn);
    return true;
  }
  /* For the imm is 0. Then replace the insn by a move insn. */
  if (((MOP_xaddrrr <= insn.GetMachineOpcode()) && (insn.GetMachineOpcode() <= MOP_sadd) && immOpnd->IsZero()) ||
      (op1IsConstant && (MOP_xsubrrr <= insn.GetMachineOpcode()) && (insn.GetMachineOpcode() <= MOP_ssub) &&
       immOpnd->IsZero())) {
    Insn &newInsn = cgFunc->GetCG()->BuildInstruction<AArch64Insn>(opndSize == k64BitSize ? MOP_xmovrr : MOP_wmovrr,
                                                                   *res, *op);
    bb->ReplaceInsn(insn, newInsn);
    return true;
  }

  if ((insn.GetMachineOpcode() == MOP_xaddrrr) || (insn.GetMachineOpcode() == MOP_waddrrr)) {
    if (immOpnd->IsInBitSize(kMaxImmVal24Bits, 0)) {
      /*
       * ADD Wd|WSP, Wn|WSP, #imm{, shift} ; 32-bit general registers
       * ADD Xd|SP,  Xn|SP,  #imm{, shift} ; 64-bit general registers
       * imm : 0 ~ 4095, shift: none, LSL #0, or LSL #12
       * aarch64 assembly takes up to 24-bits, if the lower 12 bits is all 0
       */
      if (immOpnd->IsInBitSize(kMaxImmVal12Bits, 0) || immOpnd->IsInBitSize(kMaxImmVal12Bits, kMaxImmVal12Bits)) {
        MOperator mOp = opndSize == k64BitSize ? MOP_xaddrri12 : MOP_waddrri12;
        Insn &newInsn = cgFunc->GetCG()->BuildInstruction<AArch64Insn>(mOp, *res, *op, *immOpnd);
        bb->ReplaceInsn(insn, newInsn);
        result = true;
      }
    }
  }
  /* Look for the sequence which can be simpified. */
  if (result || (insn.GetMachineOpcode() == MOP_xaddrri12) || (insn.GetMachineOpcode() == MOP_waddrri12)) {
    Insn *prev = opndInfo[idx0]->insn;
    if ((prev != nullptr) && ((prev->GetMachineOpcode() == MOP_xaddrri12) ||
                              (prev->GetMachineOpcode() == MOP_waddrri12))) {
      OpndInfo *prevInfo0 = opndInfo[idx0]->insnInfo->origOpnd[kInsnSecondOpnd];
      /* if prevop0 has been redefined. skip this optimiztation. */
      if (prevInfo0->redefined) {
        return result;
      }
      Operand &prevOpnd0 = prev->GetOperand(kInsnSecondOpnd);
      AArch64ImmOperand &imm0 = static_cast<AArch64ImmOperand&>(prev->GetOperand(kInsnThirdOpnd));
      int64_t val = imm0.GetValue() + immOpnd->GetValue();
      AArch64ImmOperand &imm1 = a64CGFunc->CreateImmOperand(val, opndSize, imm0.IsSignedValue());
      if (imm1.IsInBitSize(kMaxImmVal24Bits, 0) && (imm1.IsInBitSize(kMaxImmVal12Bits, 0) ||
                                                    imm1.IsInBitSize(kMaxImmVal12Bits, kMaxImmVal12Bits))) {
        MOperator mOp = (opndSize == k64BitSize ? MOP_xaddrri12 : MOP_waddrri12);
        bb->ReplaceInsn(insn, cgFunc->GetCG()->BuildInstruction<AArch64Insn>(mOp, *res, prevOpnd0, imm1));
        result = true;
      }
    }
  }
  return result;
}

AArch64CC_t AArch64Ebo::GetReverseCond(const CondOperand &cond) const {
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
      CHECK_FATAL(0, "Not support yet.");
  }
  return kCcLast;
}

/* return true if cond == CC_LE */
bool AArch64Ebo::CheckCondCode(const CondOperand &cond) const {
  switch (cond.GetCode()) {
    case CC_NE:
    case CC_EQ:
    case CC_LT:
    case CC_GE:
    case CC_GT:
    case CC_LE:
      return true;
    default:
      return false;
  }
}

bool AArch64Ebo::SimplifyBothConst(BB &bb, Insn &insn, const AArch64ImmOperand &immOperand0,
                                   const AArch64ImmOperand &immOperand1, uint32 opndSize) {
  MOperator mOp = insn.GetMachineOpcode();
  int64 val = 0;
  /* do not support negative const simplify yet */
  if (immOperand0.GetValue() < 0 || immOperand1.GetValue() < 0) {
    return false;
  }
  uint64 opndValue0 = static_cast<uint64>(immOperand0.GetValue());
  uint64 opndValue1 = static_cast<uint64>(immOperand1.GetValue());
  switch (mOp) {
    case MOP_weorrri12:
    case MOP_weorrrr:
    case MOP_xeorrri13:
    case MOP_xeorrrr:
      val = static_cast<int64>(opndValue0 ^ opndValue1);
      break;
    case MOP_wandrri12:
    case MOP_waddrri24:
    case MOP_wandrrr:
    case MOP_xandrri13:
    case MOP_xandrrr:
      val = static_cast<int64>(opndValue0 & opndValue1);
      break;
    case MOP_wiorrri12:
    case MOP_wiorrrr:
    case MOP_xiorrri13:
    case MOP_xiorrrr:
      val = static_cast<int64>(opndValue0 | opndValue1);
      break;
    default:
      return false;
  }
  Operand *res = insn.GetResult(0);
  AArch64ImmOperand *immOperand = &a64CGFunc->CreateImmOperand(val, opndSize, false);
  if (!immOperand->IsSingleInstructionMovable()) {
    ASSERT(res->IsRegister(), " expect a register operand");
    static_cast<AArch64CGFunc*>(cgFunc)->SplitMovImmOpndInstruction(val, *(static_cast<RegOperand*>(res)), &insn);
    bb.RemoveInsn(insn);
  } else {
    MOperator newmOp = opndSize == k64BitSize ? MOP_xmovri64 : MOP_xmovri32;
    Insn &newInsn = cgFunc->GetCG()->BuildInstruction<AArch64Insn>(newmOp, *res, *immOperand);
    bb.ReplaceInsn(insn, newInsn);
  }
  return true;
}

bool AArch64Ebo::OperandLiveAfterInsn(const RegOperand &regOpnd, Insn &insn) {
  for (Insn *nextInsn = insn.GetNext(); nextInsn != nullptr; nextInsn = nextInsn->GetNext()) {
    if (!nextInsn->IsMachineInstruction()) {
      continue;
    }
    int32 lastOpndId = static_cast<int32>(nextInsn->GetOperandSize() - 1);
    for (int32 i = lastOpndId; i >= 0; --i) {
      Operand &opnd = nextInsn->GetOperand(static_cast<uint32>(i));
      if (opnd.IsMemoryAccessOperand()) {
        auto &mem = static_cast<MemOperand&>(opnd);
        Operand *base = mem.GetBaseRegister();
        Operand *offset = mem.GetOffset();

        if (base != nullptr && base->IsRegister()) {
          auto *tmpRegOpnd = static_cast<RegOperand*>(base);
          if (tmpRegOpnd->GetRegisterNumber() == regOpnd.GetRegisterNumber()) {
            return true;
          }
        }
        if (offset != nullptr && offset->IsRegister()) {
          auto *tmpRegOpnd = static_cast<RegOperand*>(offset);
          if (tmpRegOpnd->GetRegisterNumber() == regOpnd.GetRegisterNumber()) {
            return true;
          }
        }
      }

      if (!opnd.IsRegister()) {
        continue;
      }
      auto &tmpRegOpnd = static_cast<RegOperand&>(opnd);
      if (tmpRegOpnd.GetRegisterNumber() != regOpnd.GetRegisterNumber()) {
        continue;
      }
#if TARGAARCH64 || TARGRISCV64
      const AArch64MD *md = &AArch64CG::kMd[static_cast<AArch64Insn*>(nextInsn)->GetMachineOpcode()];
      auto *regProp = static_cast<AArch64OpndProp*>(md->operand[static_cast<uint32>(i)]);
#endif
      bool isUse = regProp->IsUse();
      /* if noUse Redefined, no need to check live-out. */
      return isUse;
    }
  }
  return LiveOutOfBB(regOpnd, *insn.GetBB());
}

bool AArch64Ebo::ValidPatternForCombineExtAndLoad(OpndInfo *prevOpndInfo, Insn *insn, MOperator newMop,
                                                  MOperator oldMop, const RegOperand& opnd) {
  if (newMop == oldMop) {
    return true;
  }
  if (prevOpndInfo == nullptr || prevOpndInfo->refCount > 1) {
    return false;
  }
  if (OperandLiveAfterInsn(opnd, *insn)) {
    return false;
  }
  Insn *prevInsn = prevOpndInfo->insn;
  AArch64MemOperand *memOpnd = static_cast<AArch64MemOperand*>(prevInsn->GetMemOpnd());
  ASSERT(!prevInsn->IsStorePair(), "do not do this opt for str pair");
  ASSERT(!prevInsn->IsLoadPair(), "do not do this opt for ldr pair");
  if (memOpnd->GetAddrMode() == AArch64MemOperand::kAddrModeBOi &&
      !a64CGFunc->IsOperandImmValid(newMop, prevInsn->GetMemOpnd(), kInsnSecondOpnd)) {
    return false;
  }
  uint32 shiftAmount = memOpnd->ShiftAmount();
  if (shiftAmount == 0) {
    return true;
  }
  const AArch64MD *md = &AArch64CG::kMd[newMop];
  uint32 memSize = md->GetOperandSize() / k8BitSize;
  uint32 validShiftAmount = memSize == 8 ? 3 : memSize == 4 ? 2 : memSize == 2 ? 1 : 0;
  if (shiftAmount != validShiftAmount) {
    return false;
  }
  return true;
}

bool AArch64Ebo::CombineExtensionAndLoad(Insn *insn, const MapleVector<OpndInfo*> &origInfos,
                                         ExtOpTable idx, bool is64bits) {
  if (!beforeRegAlloc) {
    return false;
  }

  OpndInfo *opndInfo = origInfos[kInsnSecondOpnd];

  if (opndInfo == nullptr) {
    return false;
  }

  Insn *prevInsn = opndInfo->insn;

  if (prevInsn == nullptr) {
    return false;
  }

  MOperator prevMop = prevInsn->GetMachineOpcode();
  ASSERT(prevMop != MOP_undef, "Invalid opcode of instruction!");

  PairMOperator *begin = &extInsnPairTable[idx][0];
  PairMOperator *end = &extInsnPairTable[idx][insPairsNum];

  auto pairIt = std::find_if(begin, end, [prevMop](PairMOperator insPair) {
    return prevMop == insPair[0];
  });

  if (pairIt == end) {
    return false;
  }

  auto &res = static_cast<RegOperand &>(prevInsn->GetOperand(kInsnFirstOpnd));
  OpndInfo *prevOpndInfo = GetOpndInfo(res, -1);

  MOperator newPreMop = (*pairIt)[1];
  ASSERT(newPreMop != MOP_undef, "Invalid opcode of instruction!");

  if (!ValidPatternForCombineExtAndLoad(prevOpndInfo, insn, newPreMop, prevMop,
                                        res)) {
    return false;
  }

  auto *newMemOp =
      GetOrCreateMemOperandForNewMOP(*cgFunc, *prevInsn, newPreMop);

  if (newMemOp == nullptr) {
    return false;
  }

  prevInsn->SetMemOpnd(newMemOp);

  if (is64bits && idx <= SXTW && idx >= SXTB) {
    newPreMop = ExtLoadSwitchBitSize(newPreMop);
    auto &prevDstOpnd = static_cast<RegOperand&>(prevInsn->GetOperand(kInsnFirstOpnd));
    prevDstOpnd.SetSize(k64BitSize);
    prevDstOpnd.SetValidBitsNum(k64BitSize);
  }

  prevInsn->SetMOP(newPreMop);

  MOperator movOp = is64bits ? MOP_xmovrr : MOP_wmovrr;

  if (insn->GetMachineOpcode() == MOP_wandrri12 ||
      insn->GetMachineOpcode() == MOP_xandrri13) {
    Insn &newInsn = cgFunc->GetCG()->BuildInstruction<AArch64Insn>(
        movOp, insn->GetOperand(kInsnFirstOpnd),
        insn->GetOperand(kInsnSecondOpnd));
    insn->GetBB()->ReplaceInsn(*insn, newInsn);
  } else {
    insn->SetMOP(movOp);
  }

  return true;
}

bool AArch64Ebo::CombineMultiplyAdd(Insn *insn, const Insn *prevInsn, InsnInfo *insnInfo, Operand *addOpnd,
                                    bool is64bits, bool isFp) {
  /* don't use register if it was redefined. */
  OpndInfo *opndInfo1 = insnInfo->origOpnd[kInsnSecondOpnd];
  OpndInfo *opndInfo2 = insnInfo->origOpnd[kInsnThirdOpnd];
  if (((opndInfo1 != nullptr) && opndInfo1->redefined) || ((opndInfo2 != nullptr) && opndInfo2->redefined)) {
    return false;
  }
  Operand &res = insn->GetOperand(kInsnFirstOpnd);
  Operand &opnd1 = prevInsn->GetOperand(kInsnSecondOpnd);
  Operand &opnd2 = prevInsn->GetOperand(kInsnThirdOpnd);
  MOperator mOp = isFp ? (is64bits ? MOP_dmadd : MOP_smadd) : (is64bits ? MOP_xmaddrrrr : MOP_wmaddrrrr);
  insn->GetBB()->ReplaceInsn(*insn, cgFunc->GetCG()->BuildInstruction<AArch64Insn>(mOp, res, opnd1, opnd2, *addOpnd));
  return true;
}

bool AArch64Ebo::CheckCanDoMadd(Insn *insn, OpndInfo *opndInfo, int32 pos, bool is64bits, bool isFp) {
  if ((opndInfo == nullptr) || (opndInfo->insn == nullptr)) {
    return false;
  }
  if (!cgFunc->GetMirModule().IsCModule()) {
    return false;
  }
  Insn *insn1 = opndInfo->insn;
  InsnInfo *insnInfo = opndInfo->insnInfo;
  if (insnInfo == nullptr) {
    return false;
  }
  Operand &addOpnd = insn->GetOperand(static_cast<uint32>(pos));
  MOperator opc1 = insn1->GetMachineOpcode();
  if ((isFp && ((opc1 == MOP_xvmuld) || (opc1 == MOP_xvmuls))) ||
      (!isFp && ((opc1 == MOP_xmulrrr) || (opc1 == MOP_wmulrrr)))) {
    return CombineMultiplyAdd(insn, insn1, insnInfo, &addOpnd, is64bits, isFp);
  }
  return false;
}

bool AArch64Ebo::CombineMultiplySub(Insn *insn, OpndInfo *opndInfo, bool is64bits, bool isFp) {
  if ((opndInfo == nullptr) || (opndInfo->insn == nullptr)) {
    return false;
  }
  if (!cgFunc->GetMirModule().IsCModule()) {
    return false;
  }
  Insn *insn1 = opndInfo->insn;
  InsnInfo *insnInfo = opndInfo->insnInfo;
  if (insnInfo == nullptr) {
    return false;
  }
  Operand &subOpnd = insn->GetOperand(kInsnSecondOpnd);
  MOperator opc1 = insn1->GetMachineOpcode();
  if ((isFp && ((opc1 == MOP_xvmuld) || (opc1 == MOP_xvmuls))) ||
      (!isFp && ((opc1 == MOP_xmulrrr) || (opc1 == MOP_wmulrrr)))) {
    /* don't use register if it was redefined. */
    OpndInfo *opndInfo1 = insnInfo->origOpnd[kInsnSecondOpnd];
    OpndInfo *opndInfo2 = insnInfo->origOpnd[kInsnThirdOpnd];
    if (((opndInfo1 != nullptr) && opndInfo1->redefined) || ((opndInfo2 != nullptr) && opndInfo2->redefined)) {
      return false;
    }
    Operand &res = insn->GetOperand(kInsnFirstOpnd);
    Operand &opnd1 = insn1->GetOperand(kInsnSecondOpnd);
    Operand &opnd2 = insn1->GetOperand(kInsnThirdOpnd);
    MOperator mOp = isFp ? (is64bits ? MOP_dmsub : MOP_smsub) : (is64bits ? MOP_xmsubrrrr : MOP_wmsubrrrr);
    insn->GetBB()->ReplaceInsn(*insn, cgFunc->GetCG()->BuildInstruction<AArch64Insn>(mOp, res, opnd1, opnd2, subOpnd));
    return true;
  }
  return false;
}

bool AArch64Ebo::CombineMultiplyNeg(Insn *insn, OpndInfo *opndInfo, bool is64bits, bool isFp) {
  if ((opndInfo == nullptr) || (opndInfo->insn == nullptr)) {
    return false;
  }
  if (!cgFunc->GetMirModule().IsCModule()) {
    return false;
  }
  Insn *insn1 = opndInfo->insn;
  InsnInfo *insnInfo = opndInfo->insnInfo;
  CHECK_NULL_FATAL(insnInfo);
  MOperator opc1 = insn1->GetMachineOpcode();
  if ((isFp && ((opc1 == MOP_xvmuld) || (opc1 == MOP_xvmuls))) ||
      (!isFp && ((opc1 == MOP_xmulrrr) || (opc1 == MOP_wmulrrr)))) {
    /* don't use register if it was redefined. */
    OpndInfo *opndInfo1 = insnInfo->origOpnd[kInsnSecondOpnd];
    OpndInfo *opndInfo2 = insnInfo->origOpnd[kInsnThirdOpnd];
    if (((opndInfo1 != nullptr) && opndInfo1->redefined) || ((opndInfo2 != nullptr) && opndInfo2->redefined)) {
      return false;
    }
    Operand &res = insn->GetOperand(kInsnFirstOpnd);
    Operand &opnd1 = insn1->GetOperand(kInsnSecondOpnd);
    Operand &opnd2 = insn1->GetOperand(kInsnThirdOpnd);
    MOperator mOp = isFp ? (is64bits ? MOP_dnmul : MOP_snmul) : (is64bits ? MOP_xmnegrrr : MOP_wmnegrrr);
    insn->GetBB()->ReplaceInsn(*insn, cgFunc->GetCG()->BuildInstruction<AArch64Insn>(mOp, res, opnd1, opnd2));
    return true;
  }
  return false;
}

bool AArch64Ebo::CombineLsrAnd(Insn &insn, const OpndInfo &opndInfo, bool is64bits, bool isFp) {
  if (opndInfo.insn == nullptr) {
    return false;
  }
  if (!cgFunc->GetMirModule().IsCModule()) {
    return false;
  }
  AArch64CGFunc *aarchFunc = static_cast<AArch64CGFunc*>(cgFunc);
  Insn *prevInsn = opndInfo.insn;
  InsnInfo *insnInfo = opndInfo.insnInfo;
  if (insnInfo == nullptr) {
    return false;
  }
  CHECK_NULL_FATAL(insnInfo);
  MOperator opc1 = prevInsn->GetMachineOpcode();
  if (!isFp && ((opc1 == MOP_xlsrrri6) || (opc1 == MOP_wlsrrri5))) {
    /* don't use register if it was redefined. */
    OpndInfo *opndInfo1 = insnInfo->origOpnd[kInsnSecondOpnd];
    if ((opndInfo1 != nullptr) && opndInfo1->redefined) {
      return false;
    }
    Operand &res = insn.GetOperand(kInsnFirstOpnd);
    Operand &opnd1 = prevInsn->GetOperand(kInsnSecondOpnd);
    int64 immVal1 = static_cast<AArch64ImmOperand&>(prevInsn->GetOperand(kInsnThirdOpnd)).GetValue();
    Operand &immOpnd1 = is64bits ? aarchFunc->CreateImmOperand(immVal1, kMaxImmVal6Bits, false)
                                 : aarchFunc->CreateImmOperand(immVal1, kMaxImmVal5Bits, false);
    int64 immVal2 = static_cast<AArch64ImmOperand&>(insn.GetOperand(kInsnThirdOpnd)).GetValue();
    int64 immV2 = __builtin_ffsll(immVal2 + 1) - 1;
    if (immVal1 + immV2 < k1BitSize || (is64bits && immVal1 + immV2 > k64BitSize) ||
        (!is64bits && immVal1 + immV2 > k32BitSize)) {
        return false;
    }
    Operand &immOpnd2 = is64bits ? aarchFunc->CreateImmOperand(immV2, kMaxImmVal6Bits, false)
                                 : aarchFunc->CreateImmOperand(immV2, kMaxImmVal5Bits, false);
    MOperator mOp = (is64bits ? MOP_xubfxrri6i6 : MOP_wubfxrri5i5);
    insn.GetBB()->ReplaceInsn(insn, cgFunc->GetCG()->BuildInstruction<AArch64Insn>(mOp, res, opnd1,
                                                                                   immOpnd1, immOpnd2));
    return true;
  }
  return false;
}

/* Do some special pattern */
bool AArch64Ebo::SpecialSequence(Insn &insn, const MapleVector<OpndInfo*> &origInfos) {
  MOperator opCode = insn.GetMachineOpcode();
  AArch64CGFunc *aarchFunc = static_cast<AArch64CGFunc*>(cgFunc);
  switch (opCode) {
    /*
     * mov R503, R0
     * mov R0, R503
     *  ==> mov R0, R0
     */
    case MOP_wmovrr:
    case MOP_xmovrr: {
      OpndInfo *opndInfo = origInfos[kInsnSecondOpnd];
      if (opndInfo == nullptr) {
        return false;
      }
      Insn *prevInsn = opndInfo->insn;
      if ((prevInsn != nullptr) && (prevInsn->GetMachineOpcode() == opCode) &&
          (prevInsn == insn.GetPreviousMachineInsn()) &&
          !RegistersIdentical(prevInsn->GetOperand(kInsnFirstOpnd), prevInsn->GetOperand(kInsnSecondOpnd)) &&
          !RegistersIdentical(insn.GetOperand(kInsnFirstOpnd), insn.GetOperand(kInsnSecondOpnd))) {
        Operand *reg1 = insn.GetResult(0);
        Operand &reg2 = prevInsn->GetOperand(kInsnSecondOpnd);
        Insn &newInsn = cgFunc->GetCG()->BuildInstruction<AArch64Insn>(insn.GetMachineOpcode(), *reg1, reg2);
        insn.GetBB()->ReplaceInsn(insn, newInsn);
        return true;
      }
      break;
    }
    /*
     * Extension elimination.  Look for load extension pair.  There are two cases.
     * 1) extension size == load size -> change the load type or eliminate the extension
     * 2) extension size >  load size -> possibly eliminating the extension
     *
     * Example of 1)
     *  ldrb x1, []      or  ldrb x1, []      or   ldrsb x1, []      or   ldrsb x1, []
     *  sxtb x1, x1          zxtb x1, x1           sxtb  x1, x1           zxtb  x1, x1
     * ===> ldrsb x1, []     ===> ldrb x1, []      ===> ldrsb x1, []      ===> ldrb x1, []
     *      mov   x1, x1          mov  x1, x1           mov   x1, x1           mov  x1, x1
     *
     * Example of 2)
     *  ldrb x1, []      or  ldrb x1, []   or   ldrsb x1, []     or   ldrsb x1, []
     *  sxth x1, x1          zxth x1, x1        sxth  x1, x1          zxth  x1, x1
     * ===> ldrb x1, []     ===> ldrb x1, []   ===> ldrsb x1, []     ===> no change
     *      mov x1, x1           mov  x1, x1        mov   x1, x1
     */
    case MOP_wandrri12: {
      bool doAndOpt = false;
      if (static_cast<ImmOperand&>(insn.GetOperand(kInsnThirdOpnd)).GetValue() == 0xff) {
        doAndOpt = CombineExtensionAndLoad(&insn, origInfos, AND, false);
      }
      if (doAndOpt) {
        return doAndOpt;
      }
      /*
      *  lsr     d0, d1, #6
      *  and     d0, d0, #1
      * ===> ubfx d0, d1, #6, #1
      */
      int64 immValue = static_cast<ImmOperand&>(insn.GetOperand(kInsnThirdOpnd)).GetValue();
      if (!beforeRegAlloc && immValue != 0 &&
          (static_cast<uint64>(immValue) & (static_cast<uint64>(immValue) + 1)) == 0) {
        /* immValue is (1 << n - 1) */
        OpndInfo *opndInfo = origInfos.at(kInsnSecondOpnd);
        return CombineLsrAnd(insn, *opndInfo, false, false);
      }
      break;
    }
    case MOP_xandrri13: {
      bool doAndOpt = false;
      if (static_cast<ImmOperand&>(insn.GetOperand(kInsnThirdOpnd)).GetValue() == 0xff) {
        doAndOpt = CombineExtensionAndLoad(&insn, origInfos, AND, true);
      }
      if (doAndOpt) {
        return doAndOpt;
      }
      /*
      *  lsr     d0, d1, #6
      *  and     d0, d0, #1
      * ===> ubfx d0, d1, #6, #1
      */
      int64 immValue = static_cast<ImmOperand&>(insn.GetOperand(kInsnThirdOpnd)).GetValue();
      if (!beforeRegAlloc && immValue != 0 &&
          (static_cast<uint64>(immValue) & (static_cast<uint64>(immValue) + 1)) == 0) {
        /* immValue is (1 << n - 1) */
        OpndInfo *opndInfo = origInfos.at(kInsnSecondOpnd);
        return CombineLsrAnd(insn, *opndInfo, true, false);
      }
      break;
    }
    case MOP_xsxtb32:
      return CombineExtensionAndLoad(&insn, origInfos, SXTB, false);
    case MOP_xsxtb64:
      return CombineExtensionAndLoad(&insn, origInfos, SXTB, true);
    case MOP_xsxth32:
      return CombineExtensionAndLoad(&insn, origInfos, SXTH, false);
    case MOP_xsxth64:
      return CombineExtensionAndLoad(&insn, origInfos, SXTH, true);
    case MOP_xsxtw64:
      return CombineExtensionAndLoad(&insn, origInfos, SXTW, true);
    case MOP_xuxtb32:
      return CombineExtensionAndLoad(&insn, origInfos, ZXTB, false);
    case MOP_xuxth32:
      return CombineExtensionAndLoad(&insn, origInfos, ZXTH, false);
    case MOP_xuxtw64:
      return CombineExtensionAndLoad(&insn, origInfos, ZXTW, true);
    /*
     *  lsl     x1, x1, #3
     *  add     x0, x0, x1
     * ===> add x0, x0, x1, 3
     *
     *  mul     x1, x1, x2
     *  add     x0, x0, x1     or   add   x0, x1, x0
     * ===> madd x0, x1, x2, x0
     */
    case MOP_xaddrrr:
    case MOP_waddrrr: {
      if (insn.GetResult(0) == nullptr) {
        return false;
      }
      bool is64bits = (insn.GetResult(0)->GetSize() == k64BitSize);
      OpndInfo *opndInfo = origInfos.at(kInsnThirdOpnd);
      if ((opndInfo != nullptr) && (opndInfo->insn != nullptr)) {
        Insn *insn1 = opndInfo->insn;
        InsnInfo *insnInfo1 = opndInfo->insnInfo;
        if (insnInfo1 == nullptr) {
          return false;
        }
        Operand &op0 = insn.GetOperand(kInsnSecondOpnd);
        MOperator opc1 = insn1->GetMachineOpcode();
        if ((opc1 == MOP_xlslrri6) || (opc1 == MOP_wlslrri5)) {
          /* don't use register if it was redefined. */
          if (cgFunc->GetMirModule().IsCModule()) {
            /* global opt will do this pattern when is CMoudle */
            return false;
          }
          OpndInfo *opndInfo1 = insnInfo1->origOpnd[kInsnSecondOpnd];
          if ((opndInfo1 != nullptr) && opndInfo1->redefined) {
            return false;
          }
          Operand &res = insn.GetOperand(kInsnFirstOpnd);
          Operand &opnd1 = insn1->GetOperand(kInsnSecondOpnd);
          auto &immOpnd = static_cast<AArch64ImmOperand&>(insn1->GetOperand(kInsnThirdOpnd));
          uint32 xLslrriBitLen = 6;
          uint32 wLslrriBitLen = 5;
          Operand &shiftOpnd = aarchFunc->CreateBitShiftOperand(BitShiftOperand::kLSL,
              static_cast<uint32>(immOpnd.GetValue()),static_cast<int32>((
              opCode == MOP_xlslrri6) ? xLslrriBitLen : wLslrriBitLen));
          MOperator mOp = (is64bits ? MOP_xaddrrrs : MOP_waddrrrs);
          insn.GetBB()->ReplaceInsn(insn, cgFunc->GetCG()->BuildInstruction<AArch64Insn>(mOp, res, op0,
                                                                                         opnd1, shiftOpnd));
          return true;
        } else if ((opc1 == MOP_xmulrrr) || (opc1 == MOP_wmulrrr)) {
          return CombineMultiplyAdd(&insn, insn1, insnInfo1, &op0, is64bits, false);
        }
      }
      opndInfo = origInfos.at(kInsnSecondOpnd);
      return CheckCanDoMadd(&insn, opndInfo, kInsnThirdOpnd, is64bits, false);
      break;
    }
    /*
     *  fmul     d1, d1, d2
     *  fadd     d0, d0, d1     or   add   d0, d1, d0
     * ===> fmadd d0, d1, d2, d0
     */
    case MOP_dadd:
    case MOP_sadd: {
      if (!CGOptions::IsFastMath()) {
        return false;
      }
      bool is64bits = (insn.GetResult(0)->GetSize() == k64BitSize);
      OpndInfo *opndInfo = origInfos.at(kInsnSecondOpnd);
      if (CheckCanDoMadd(&insn, opndInfo, kInsnThirdOpnd, is64bits, true)) {
        return true;
      }
      opndInfo = origInfos.at(kInsnThirdOpnd);
      if (CheckCanDoMadd(&insn, opndInfo, kInsnSecondOpnd, is64bits, true)) {
        return true;
      }
      break;
    }
    /*
     *  mul     x1, x1, x2
     *  sub     x0, x0, x1
     * ===> msub x0, x1, x2, x0
     */
    case MOP_xsubrrr:
    case MOP_wsubrrr: {
      bool is64bits = (insn.GetResult(0)->GetSize() == k64BitSize);
      OpndInfo *opndInfo = origInfos.at(kInsnThirdOpnd);
      if (CombineMultiplySub(&insn, opndInfo, is64bits, false)) {
        return true;
      }
      break;
    }
    /*
     *  fmul     d1, d1, d2
     *  fsub     d0, d0, d1
     * ===> fmsub d0, d1, d2, d0
     */
    case MOP_dsub:
    case MOP_ssub: {
      if (!CGOptions::IsFastMath()) {
        return false;
      }
      bool is64bits = (insn.GetResult(0)->GetSize() == k64BitSize);
      OpndInfo *opndInfo = origInfos.at(kInsnThirdOpnd);
      if (CombineMultiplySub(&insn, opndInfo, is64bits, true)) {
        return true;
      }
      break;
    }
    /*
     *  mul     x1, x1, x2
     *  neg     x0, x1
     * ===> mneg x0, x1, x2
     */
    case MOP_xinegrr:
    case MOP_winegrr: {
      bool is64bits = (insn.GetResult(0)->GetSize() == k64BitSize);
      OpndInfo *opndInfo = origInfos.at(kInsnSecondOpnd);
      if (CombineMultiplyNeg(&insn, opndInfo, is64bits, false)) {
        return true;
      }
      break;
    }
    /*
     *  fmul     d1, d1, d2
     *  fneg     d0, d1
     * ===> fnmul d0, d1, d2
     */
    case MOP_wfnegrr:
    case MOP_xfnegrr: {
      if (!CGOptions::IsFastMath()) {
        return false;
      }
      bool is64bits = (insn.GetResult(0)->GetSize() == k64BitSize);
      OpndInfo *opndInfo = origInfos.at(kInsnSecondOpnd);
      if (CombineMultiplyNeg(&insn, opndInfo, is64bits, true)) {
        return true;
      }
      break;
    }
    case MOP_wstr:
    case MOP_xstr:
    case MOP_wldr:
    case MOP_xldr: {
      /*
       * add x2, x1, imm
       * ldr x3, [x2]
       * -> ldr x3, [x1, imm]
       * ---------------------
       * add x2, x1, imm
       * str x3, [x2]
       * -> str x3, [x1, imm]
       */
      if (cgFunc->GetMirModule().IsCModule()) {
        /* strldr opt will do this pattern when is CMoudle */
        return false;
      }
      CHECK_NULL_FATAL(insn.GetResult(0));
      OpndInfo *opndInfo = origInfos[kInsnSecondOpnd];
      if (insn.IsLoad() && opndInfo == nullptr) {
        return false;
      }
      const AArch64MD *md = &AArch64CG::kMd[static_cast<AArch64Insn*>(&insn)->GetMachineOpcode()];
      bool is64bits = md->Is64Bit();
      uint32 size = md->GetOperandSize();
      OpndInfo *baseInfo = nullptr;
      MemOperand *memOpnd = nullptr;
      if (insn.IsLoad()) {
        MemOpndInfo *memInfo = static_cast<MemOpndInfo*>(opndInfo);
        baseInfo = memInfo->GetBaseInfo();
        memOpnd = static_cast<MemOperand*>(memInfo->opnd);
      } else {
        Operand *res = insn.GetResult(0);
        ASSERT(res->IsMemoryAccessOperand(), "res must be MemoryAccessOperand");
        memOpnd = static_cast<MemOperand*>(res);
        Operand *base = memOpnd->GetBaseRegister();
        ASSERT(base->IsRegister(), "base must be Register");
        baseInfo = GetOpndInfo(*base, -1);
      }

      if (static_cast<AArch64MemOperand*>(memOpnd)->GetAddrMode() != AArch64MemOperand::kAddrModeBOi) {
        return false;
      }

      if ((baseInfo != nullptr) && (baseInfo->insn != nullptr)) {
        Insn *insn1 = baseInfo->insn;
        if (insn1->GetBB() != insn.GetBB()) {
          return false;
        }
        InsnInfo *insnInfo1 = baseInfo->insnInfo;
        CHECK_NULL_FATAL(insnInfo1);
        MOperator opc1 = insn1->GetMachineOpcode();
        if ((opc1 == MOP_xaddrri12) || (opc1 == MOP_waddrri12)) {
          if (memOpnd->GetOffset() == nullptr) {
            return false;
          }
          AArch64ImmOperand *imm0 = static_cast<AArch64ImmOperand*>(memOpnd->GetOffset());
          if (imm0 == nullptr) {
            return false;
          }
          int64 imm0Val = imm0->GetValue();
          Operand &res = insn.GetOperand(kInsnFirstOpnd);
          RegOperand *op1 = &static_cast<RegOperand&>(insn1->GetOperand(kInsnSecondOpnd));
          AArch64ImmOperand &imm1 = static_cast<AArch64ImmOperand&>(insn1->GetOperand(kInsnThirdOpnd));
          int64 immVal;
          /* don't use register if it was redefined. */
          OpndInfo *opndInfo1 = insnInfo1->origOpnd[kInsnSecondOpnd];
          if ((opndInfo1 != nullptr) && opndInfo1->redefined) {
            /*
             * add x2, x1, imm0, LSL imm1
             * add x2, x2, imm2
             * ldr x3, [x2]
             * -> ldr x3, [x1, imm]
             * ----------------------------
             * add x2, x1, imm0, LSL imm1
             * add x2, x2, imm2
             * str x3, [x2]
             * -> str x3, [x1, imm]
             */
            Insn *insn2 = opndInfo1->insn;
            if (insn2 == nullptr) {
              return false;
            }
            MOperator opCode2 = insn2->GetMachineOpcode();
            if ((opCode2 != MOP_xaddrri24) && (opCode2 != MOP_waddrri24)) {
              return false;
            }
            auto &res2 = static_cast<RegOperand&>(insn2->GetOperand(kInsnFirstOpnd));
            auto &base2 = static_cast<RegOperand&>(insn2->GetOperand(kInsnSecondOpnd));
            auto &immOpnd2 = static_cast<AArch64ImmOperand&>(insn2->GetOperand(kInsnThirdOpnd));
            auto &res1 = static_cast<RegOperand&>(insn1->GetOperand(kInsnFirstOpnd));
            OpndInfo *base2DefOpndInfo = GetOpndInfo(base2, -1);
            if (base2DefOpndInfo == nullptr) {
              return false;
            }
            bool isNotSkipCall = beforeRegAlloc ||
                (base2DefOpndInfo->insn != nullptr && !base2DefOpndInfo->insn->IsCall());
            if (RegistersIdentical(res1, *op1) && RegistersIdentical(res1, res2) &&
                (!base2DefOpndInfo->redefined) && isNotSkipCall) {
              if (beforeRegAlloc) {
                immVal = imm0Val + imm1.GetValue() +
                         static_cast<int64>(static_cast<uint64>(immOpnd2.GetValue()) << kMaxImmVal12Bits);
              } else if (RegistersIdentical(res2, base2)) {
                if (RegistersIdentical(insn1->GetOperand(0), insn1->GetOperand(1))) {
                  return false;
                } else {
                  immVal = imm0Val + imm1.GetValue() +
                           static_cast<int64>(static_cast<uint64>(immOpnd2.GetValue()) << kMaxImmVal12Bits);
                }
              } else {
                immVal = imm0Val + imm1.GetValue() +
                         static_cast<int64>(static_cast<uint64>(immOpnd2.GetValue()) << kMaxImmVal12Bits);
              }
              op1 = &base2;
            } else {
              return false;
            }
          } else {
            if (beforeRegAlloc) {
              immVal = imm0Val + imm1.GetValue();
            } else if (RegistersIdentical(insn1->GetOperand(0), insn1->GetOperand(1))) {
              immVal = imm0Val;
            } else {
              immVal = imm0Val + imm1.GetValue();
            }
          }

          /* multiple of 4 and 8 */
          const int multiOfFour = 4;
          const int multiOfEight = 8;
          is64bits = is64bits &&
                     (!static_cast<AArch64Insn&>(insn).CheckRefField(static_cast<size_t>(kInsnFirstOpnd), false));
          if ((!is64bits && (immVal < kStrLdrImm32UpperBound) && (immVal % multiOfFour == 0)) ||
              (is64bits && (immVal < kStrLdrImm64UpperBound) && (immVal % multiOfEight == 0))) {
            /* Reserved physicalReg beforeRA */
            if (beforeRegAlloc && op1->IsPhysicalRegister()) {
              return false;
            }
            MemOperand &mo = aarchFunc->CreateMemOpnd(*op1, immVal, size);
            Insn &ldrInsn = cgFunc->GetCG()->BuildInstruction<AArch64Insn>(opCode, res, mo);
            insn.GetBB()->ReplaceInsn(insn, ldrInsn);
            return true;
          }
        }
      }
      break;
    }  /* end case MOP_xldr */
    case MOP_xcsetrc:
    case MOP_wcsetrc: {
      /* i.   cmp     x0, x1
       *      cset    w0, EQ     ===> cmp x0, x1
       *      cmp     w0, #0          cset w0, EQ
       *      cset    w0, NE
       *
       * ii.  cmp     x0, x1
       *      cset    w0, EQ     ===> cmp x0, x1
       *      cmp     w0, #0          cset w0, NE
       *      cset    w0, EQ
       *
       *  a.< -1 : 0x20ff25e0 > < 0 > cmp(226) (opnd0: vreg:C105 class: [CC]) (opnd1: vreg:R104 class: [I]) (opnd2:
       *  vreg:R106 class: [I])
       *  b.< -1 : 0x20ff60a0 > < 0 > cset(72) (opnd0: vreg:R101 class: [I]) (opnd1: CC: EQ)
       *  c.< -1*  : 0x20ff3870 > < 0 > cmp(223) (opnd0: vreg:C105 class: [CC]) (opnd1: vreg:R101 class: [I]) (opnd2:
       *  imm:0)
       *  d.< *  -1 : 0x20ff3908 > < 0 > cset(72) (opnd0: vreg:R107 class: [I]) (opnd1: CC: NE)
       *  d1.< -1 : 0x20ff3908 > < 0 > *  cset(72) (opnd0: vreg:R107 class: [I]) (opnd1: CC: EQ) i, d
       *  ===> mov R107 R101 ii, a,b,c,d1 ===> a,b,cset Rxx
       *  NE, c, mov R107 Rxx
       */
      auto &cond = static_cast<CondOperand&>(insn.GetOperand(kInsnSecondOpnd));
      if ((cond.GetCode() != CC_NE) && (cond.GetCode() != CC_EQ)) {
        return false;
      }
      bool reverse = (cond.GetCode() == CC_EQ);
      OpndInfo *condInfo = origInfos[kInsnSecondOpnd];
      if ((condInfo != nullptr) && condInfo->insn) {
        Insn *cmp1 = condInfo->insn;
        if ((cmp1->GetMachineOpcode() == MOP_xcmpri) || (cmp1->GetMachineOpcode() == MOP_wcmpri)) {
          InsnInfo *cmpInfo1 = condInfo->insnInfo;
          CHECK_FATAL(cmpInfo1 != nullptr, "pointor cmpInfo1 is null");
          OpndInfo *info0 = cmpInfo1->origOpnd[kInsnSecondOpnd];
          /* if R101 was not redefined. */
          if ((info0 != nullptr) && (info0->insnInfo != nullptr) && (info0->insn != nullptr) &&
              (reverse || !info0->redefined) && cmp1->GetOperand(kInsnThirdOpnd).IsImmediate()) {
            Insn *csetInsn = info0->insn;
            MOperator opc1 = csetInsn->GetMachineOpcode();
            if (((opc1 == MOP_xcsetrc) || (opc1 == MOP_wcsetrc)) &&
                static_cast<ImmOperand&>(cmp1->GetOperand(kInsnThirdOpnd)).IsZero()) {
              CondOperand &cond1 = static_cast<CondOperand&>(csetInsn->GetOperand(kInsnSecondOpnd));
              if (!CheckCondCode(cond1)) {
                return false;
              }
              if (EBO_DUMP) {
                LogInfo::MapleLogger() << "< === do specical condition optimization, replace insn  ===> \n";
                insn.Dump();
              }
              Operand *result = insn.GetResult(0);
              CHECK_FATAL(result != nullptr, "pointor result is null");
              uint32 size = result->GetSize();
              if (reverse) {
                /* After regalloction, we can't create a new register. */
                if (!beforeRegAlloc) {
                  return false;
                }
                AArch64CGFunc *aarFunc = static_cast<AArch64CGFunc*>(cgFunc);
                Operand &r = aarFunc->CreateRegisterOperandOfType(static_cast<RegOperand*>(result)->GetRegisterType(),
                                                                  size / kBitsPerByte);
                /* after generate a new vreg, check if the size of DataInfo is big enough */
                EnlargeSpaceForLA(*csetInsn);
                CondOperand &cond2 = aarFunc->GetCondOperand(GetReverseCond(cond1));
                Operand &rflag = aarFunc->GetOrCreateRflag();
                Insn &newCset = cgFunc->GetCG()->BuildInstruction<AArch64Insn>(
                    result->GetSize() == k64BitSize ? MOP_xcsetrc : MOP_wcsetrc, r, cond2, rflag);
                /* new_cset use the same cond as cset_insn. */
                IncRef(*info0->insnInfo->origOpnd[kInsnSecondOpnd]);
                csetInsn->GetBB()->InsertInsnAfter(*csetInsn, newCset);
                MOperator mOp = (result->GetSize() == k64BitSize ? MOP_xmovrr : MOP_wmovrr);
                Insn &newInsn = cgFunc->GetCG()->BuildInstruction<AArch64Insn>(mOp, *result, r);
                insn.GetBB()->ReplaceInsn(insn, newInsn);
                if (EBO_DUMP) {
                  LogInfo::MapleLogger() << "< === with new insn ===> \n";
                  newInsn.Dump();
                }
              } else {
                Operand *result1 = csetInsn->GetResult(0);
                MOperator mOp = ((result->GetSize() == k64BitSize) ? MOP_xmovrr : MOP_wmovrr);
                Insn &newInsn = cgFunc->GetCG()->BuildInstruction<AArch64Insn>(mOp, *result, *result1);
                insn.GetBB()->ReplaceInsn(insn, newInsn);
                if (EBO_DUMP) {
                  LogInfo::MapleLogger() << "< === with new insn ===> \n";
                  newInsn.Dump();
                }
              }
              return true;
            }
          }
        }
      }
    }  /* end case MOP_wcsetrc */
    [[clang::fallthrough]];
    default:
      break;
  }
  return false;
}

/*
 *  *iii. mov w16, v10.s[1]   //  FMOV from simd 105   ---> replace_insn
 *      mov w1, w16     ----->insn
 *      ==>
 *      mov w1, v10.s[1]
 */
bool AArch64Ebo::IsMovToSIMDVmov(Insn &insn, const Insn &replaceInsn) const {
  if (insn.GetMachineOpcode() == MOP_wmovrr && replaceInsn.GetMachineOpcode() == MOP_xvmovrv) {
    insn.SetMOP(replaceInsn.GetMachineOpcode());
    return true;
  }
  return false;
}

bool AArch64Ebo::IsPseudoRet(Insn &insn) const {
  MOperator mop = insn.GetMachineOpcode();
  if (mop == MOP_pseudo_ret_int || mop == MOP_pseudo_ret_float) {
    return true;
  }
  return false;
}

bool AArch64Ebo::ChangeLdrMop(Insn &insn, const Operand &opnd) const {
  ASSERT(insn.IsLoad(), "expect insn is load in ChangeLdrMop");
  ASSERT(opnd.IsRegister(), "expect opnd is a register in ChangeLdrMop");

  const RegOperand *regOpnd = static_cast<const RegOperand*>(&opnd);
  if (static_cast<RegOperand&>(insn.GetOperand(kInsnFirstOpnd)).GetRegisterType() != regOpnd->GetRegisterType()) {
    return false;
  }

  if (static_cast<MemOperand&>(insn.GetOperand(kInsnSecondOpnd)).GetIndexRegister()) {
    return false;
  }

  bool bRet = true;
  if (regOpnd->GetRegisterType() == kRegTyFloat) {
    switch (insn.GetMachineOpcode()) {
      case MOP_wldrb:
        insn.SetMOP(MOP_bldr);
        break;
      case MOP_wldrh:
        insn.SetMOP(MOP_hldr);
        break;
      case MOP_wldr:
        insn.SetMOP(MOP_sldr);
        break;
      case MOP_xldr:
        insn.SetMOP(MOP_dldr);
        break;
      case MOP_wldli:
        insn.SetMOP(MOP_sldli);
        break;
      case MOP_xldli:
        insn.SetMOP(MOP_dldli);
        break;
      case MOP_wldrsb:
      case MOP_wldrsh:
      default:
        bRet = false;
        break;
    }
  } else if (regOpnd->GetRegisterType() == kRegTyInt) {
    switch (insn.GetMachineOpcode()) {
      case MOP_bldr:
        insn.SetMOP(MOP_wldrb);
        break;
      case MOP_hldr:
        insn.SetMOP(MOP_wldrh);
        break;
      case MOP_sldr:
        insn.SetMOP(MOP_wldr);
        break;
      case MOP_dldr:
        insn.SetMOP(MOP_xldr);
        break;
      case MOP_sldli:
        insn.SetMOP(MOP_wldli);
        break;
      case MOP_dldli:
        insn.SetMOP(MOP_xldli);
        break;
      default:
        bRet = false;
        break;
    }
  } else {
    ASSERT(false, "Internal error.");
  }
  return bRet;
}
}  /* namespace maplebe */
