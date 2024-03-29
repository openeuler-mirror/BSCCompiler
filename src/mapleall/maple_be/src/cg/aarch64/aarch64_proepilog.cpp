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
#include "aarch64_cg.h"
#include "cg_option.h"
#include "cgfunc.h"
#include "cg_irbuilder.h"

#define PROEPILOG_DUMP CG_DEBUG_FUNC(cgFunc)

namespace maplebe {
using namespace maple;

namespace {

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
      { 0 }, /* undef */
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
      { 0 }, /* undef */
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

static inline bool AArch64NeedProEpilog(CGFunc &cgFunc) {
  if (cgFunc.GetMirModule().GetSrcLang() != kSrcLangC) {
    return true;
  } else if (cgFunc.GetFunction().GetAttr(FUNCATTR_varargs) || cgFunc.HasVLAOrAlloca()) {
    return true;
  }
  // note that tailcall insn is not a call
  FOR_ALL_BB(bb, &cgFunc) {
    FOR_BB_INSNS_REV(insn, bb) {
      if (insn->IsMachineInstruction() && (insn->IsCall() || insn->IsSpecialCall())) {
        return true;
      }
    }
  }
  auto &aarchCGFunc = static_cast<AArch64CGFunc&>(cgFunc);
  const MapleVector<AArch64reg> &regsToRestore = (aarchCGFunc.GetProEpilogSavedRegs().empty()) ?
      aarchCGFunc.GetCalleeSavedRegs() : aarchCGFunc.GetProEpilogSavedRegs();
  size_t calleeSavedRegSize = kOneRegister;
  CHECK_FATAL(regsToRestore.size() >= calleeSavedRegSize, "Forgot LR ?");
  if (regsToRestore.size() > calleeSavedRegSize || aarchCGFunc.HasStackLoadStore() ||
      static_cast<AArch64MemLayout *>(cgFunc.GetMemlayout())->GetSizeOfLocals() > 0 ||
      static_cast<AArch64MemLayout *>(cgFunc.GetMemlayout())->GetSizeOfCold() > 0 ||
      cgFunc.GetFunction().GetAttr(FUNCATTR_callersensitive)) {
    return true;
  }
  return false;
}

bool AArch64ProEpilogAnalysis::NeedProEpilog() {
  return AArch64NeedProEpilog(cgFunc);
}

bool AArch64GenProEpilog::NeedProEpilog() {
  return AArch64NeedProEpilog(cgFunc);
}

// find a idle register, default R30
AArch64reg AArch64GenProEpilog::GetStackGuardRegister(const BB &bb) const {
  if (Globals::GetInstance()->GetOptimLevel() == CGOptions::kLevel0) {
    return R30;
  }
  for (regno_t reg = R9; reg < R29; ++reg) {
    if (bb.GetLiveInRegNO().count(reg) == 0 && reg != R16) {
      if (!AArch64Abi::IsCalleeSavedReg(static_cast<AArch64reg>(reg))) {
        return static_cast<AArch64reg>(reg);
      }
    }
  }
  return R30;
}

// find two idle register, default R30 and R16
std::pair<AArch64reg, AArch64reg> AArch64GenProEpilog::GetStackGuardCheckRegister(const BB &bb) const {
  AArch64reg stGuardReg = R30;
  AArch64reg stCheckReg = R16;
  if (Globals::GetInstance()->GetOptimLevel() == CGOptions::kLevel0) {
    return {stGuardReg, stCheckReg};
  }
  for (regno_t reg = R9; reg < R29; ++reg) {
    if (bb.GetLiveOutRegNO().count(reg) == 0 && reg != R16) {
      if (AArch64Abi::IsCalleeSavedReg(static_cast<AArch64reg>(reg))) {
        continue;
      }
      if (stGuardReg == R30) {
        stGuardReg = static_cast<AArch64reg>(reg);
      } else {
        stCheckReg = static_cast<AArch64reg>(reg);
        break;
      }
    }
  }
  return {stGuardReg, stCheckReg};
}

// RealStackFrameSize - [GR,16] - [VR,16] - 8 (from fp to stack protect area)
// We allocate 16 byte for stack protect area
MemOperand *AArch64GenProEpilog::GetDownStack() {
  auto &aarchCGFunc = static_cast<AArch64CGFunc&>(cgFunc);
  uint64 vArea = 0;
  if (cgFunc.GetMirModule().IsCModule() && cgFunc.GetFunction().GetAttr(FUNCATTR_varargs)) {
    AArch64MemLayout *ml = static_cast<AArch64MemLayout *>(cgFunc.GetMemlayout());
    if (ml->GetSizeOfGRSaveArea() > 0) {
      vArea += RoundUp(ml->GetSizeOfGRSaveArea(), kAarch64StackPtrAlignment);
    }
    if (ml->GetSizeOfVRSaveArea() > 0) {
      vArea += RoundUp(ml->GetSizeOfVRSaveArea(), kAarch64StackPtrAlignment);
    }
  }

  int32 stkSize = static_cast<int32>(static_cast<AArch64MemLayout*>(cgFunc.GetMemlayout())->RealStackFrameSize());
  if (useFP) {
    stkSize -= static_cast<int32>(static_cast<AArch64MemLayout*>(cgFunc.GetMemlayout())->SizeOfArgsToStackPass());
  }
  int32 memSize = (stkSize - kOffset8MemPos) - static_cast<int32>(vArea);
  MemOperand *downStk = aarchCGFunc.CreateStackMemOpnd(stackBaseReg, memSize, GetPointerBitSize());
  if (downStk->GetMemVaryType() == kNotVary &&
      aarchCGFunc.IsImmediateOffsetOutOfRange(*downStk, k64BitSize)) {
    downStk = &aarchCGFunc.SplitOffsetWithAddInstruction(*downStk, k64BitSize, R16);
  }
  return downStk;
}

RegOperand &AArch64GenProEpilog::GenStackGuard(AArch64reg regNO) {
  auto &aarchCGFunc = static_cast<AArch64CGFunc&>(cgFunc);
  aarchCGFunc.GetDummyBB()->ClearInsns();

  cgFunc.SetCurBB(*aarchCGFunc.GetDummyBB());

  MIRSymbol *stkGuardSym = GlobalTables::GetGsymTable().GetSymbolFromStrIdx(
      GlobalTables::GetStrTable().GetStrIdxFromName(std::string("__stack_chk_guard")));
  StImmOperand &stOpnd = aarchCGFunc.CreateStImmOperand(*stkGuardSym, 0, 0);
  RegOperand &stAddrOpnd =
    aarchCGFunc.GetOrCreatePhysicalRegisterOperand(regNO, GetPointerBitSize(), kRegTyInt);
  aarchCGFunc.SelectAddrof(stAddrOpnd, stOpnd);

  MemOperand *guardMemOp = aarchCGFunc.CreateMemOperand(GetPointerBitSize(), stAddrOpnd,
                                                        aarchCGFunc.CreateImmOperand(0, k32BitSize, false));
  MOperator mOp = aarchCGFunc.PickLdInsn(k64BitSize, PTY_u64);
  Insn &insn = cgFunc.GetInsnBuilder()->BuildInsn(mOp, stAddrOpnd, *guardMemOp);
  insn.SetDoNotRemove(true);
  cgFunc.GetCurBB()->AppendInsn(insn);
  return stAddrOpnd;
}

void AArch64GenProEpilog::AddStackGuard(BB &bb) {
  if (!cgFunc.GetNeedStackProtect()) {
    return;
  }
  BB *formerCurBB = cgFunc.GetCurBB();
  auto &aarchCGFunc = static_cast<AArch64CGFunc&>(cgFunc);
  auto &stAddrOpnd = GenStackGuard(GetStackGuardRegister(bb));
  auto mOp = aarchCGFunc.PickStInsn(GetPointerBitSize(), PTY_u64);
  Insn &tmpInsn = cgFunc.GetInsnBuilder()->BuildInsn(mOp, stAddrOpnd, *GetDownStack());
  tmpInsn.SetDoNotRemove(true);
  cgFunc.GetCurBB()->AppendInsn(tmpInsn);

  bb.InsertAtBeginning(*aarchCGFunc.GetDummyBB());
  cgFunc.SetCurBB(*formerCurBB);
}

BB &AArch64GenProEpilog::GetOrGenStackGuardCheckFailBB(BB &bb) {
  if (stackChkFailBB != nullptr) {
    return *stackChkFailBB;
  }
  BB *formerCurBB = cgFunc.GetCurBB();
  auto &aarchCGFunc = static_cast<AArch64CGFunc&>(cgFunc);

  // create new check fail BB
  auto failLable = aarchCGFunc.CreateLabel();
  stackChkFailBB = aarchCGFunc.CreateNewBB(failLable, bb.IsUnreachable(), BB::kBBNoReturn, bb.GetFrequency());
  cgFunc.SetCurBB(*stackChkFailBB);
  MIRSymbol *failFunc = GlobalTables::GetGsymTable().GetSymbolFromStrIdx(
      GlobalTables::GetStrTable().GetStrIdxFromName(std::string("__stack_chk_fail")));
  ListOperand *srcOpnds = aarchCGFunc.CreateListOpnd(*cgFunc.GetFuncScopeAllocator());
  Insn &callInsn = aarchCGFunc.AppendCall(*failFunc, *srcOpnds);
  callInsn.SetDoNotRemove(true);
  ASSERT_NOT_NULL(cgFunc.GetLastBB());
  cgFunc.GetLastBB()->PrependBB(*stackChkFailBB);

  cgFunc.SetCurBB(*formerCurBB);
  return *stackChkFailBB;
}

void AArch64GenProEpilog::GenStackGuardCheckInsn(BB &bb) {
  if (!cgFunc.GetNeedStackProtect()) {
    return;
  }

  BB *formerCurBB = cgFunc.GetCurBB();
  auto &aarchCGFunc = static_cast<AArch64CGFunc&>(cgFunc);
  auto [stGuardReg, stCheckReg] = GetStackGuardCheckRegister(bb);
  auto &stAddrOpnd = GenStackGuard(stGuardReg);
  RegOperand &checkOp =
      aarchCGFunc.GetOrCreatePhysicalRegisterOperand(stCheckReg, GetPointerBitSize(), kRegTyInt);
  auto mOp = aarchCGFunc.PickLdInsn(GetPointerBitSize(), PTY_u64);
  Insn &newInsn = cgFunc.GetInsnBuilder()->BuildInsn(mOp, checkOp, *GetDownStack());
  newInsn.SetDoNotRemove(true);
  cgFunc.GetCurBB()->AppendInsn(newInsn);

  cgFunc.SelectBxor(stAddrOpnd, stAddrOpnd, checkOp, PTY_u64);
  auto &failBB = GetOrGenStackGuardCheckFailBB(bb);
  aarchCGFunc.SelectCondGoto(aarchCGFunc.GetOrCreateLabelOperand(failBB.GetLabIdx()), OP_brtrue, OP_ne,
                             stAddrOpnd, aarchCGFunc.CreateImmOperand(0, k64BitSize, false),
                             PTY_u64, false, BB::kUnknownProb);

  auto chkBB = cgFunc.CreateNewBB(bb.GetLabIdx(), bb.IsUnreachable(), BB::kBBIf, bb.GetFrequency());
  chkBB->AppendBBInsns(bb);
  bb.ClearInsns();
  auto *lastInsn = chkBB->GetLastMachineInsn();
  if (lastInsn != nullptr && (lastInsn->IsTailCall() || lastInsn->IsBranch())) {
    chkBB->RemoveInsn(*lastInsn);
    bb.AppendInsn(*lastInsn);
  }
  if (&bb == cgFunc.GetFirstBB()) {
    cgFunc.SetFirstBB(*chkBB);
  }
  chkBB->AppendBBInsns(*(cgFunc.GetCurBB()));
  bb.PrependBB(*chkBB);
  chkBB->PushBackSuccs(bb);
  auto &originPreds = bb.GetPreds();
  for (auto pred : originPreds) {
    pred->ReplaceSucc(bb, *chkBB);
    chkBB->PushBackPreds(*pred);
  }
  LabelIdx nextLable = aarchCGFunc.CreateLabel();
  bb.SetLabIdx(nextLable);
  cgFunc.SetLab2BBMap(nextLable, bb);
  bb.ClearPreds();
  bb.PushBackPreds(*chkBB);
  chkBB->PushBackSuccs(failBB);
  failBB.PushBackPreds(*chkBB);

  cgFunc.SetCurBB(*formerCurBB);
}

MemOperand *AArch64GenProEpilog::SplitStpLdpOffsetForCalleeSavedWithAddInstruction(CGFunc &cgFunc,
    const MemOperand &mo, uint32 bitLen, AArch64reg baseRegNum) {
  auto &aarchCGFunc = static_cast<AArch64CGFunc&>(cgFunc);
  CHECK_FATAL(mo.GetAddrMode() == MemOperand::kBOI, "mode should be kBOI");
  OfstOperand *ofstOp = mo.GetOffsetImmediate();
  auto offsetVal = static_cast<int32>(ofstOp->GetOffsetValue());
  CHECK_FATAL(offsetVal > 0, "offsetVal should be greater than 0");
  CHECK_FATAL((static_cast<uint32>(offsetVal) & 0x7) == 0, "(offsetVal & 0x7) should be equal to 0");
  /*
   * Offset adjustment due to FP/SP has already been done
   * in AArch64GenProEpilog::GeneratePushRegs() and AArch64GenProEpilog::GeneratePopRegs()
   */
  RegOperand &br = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(baseRegNum, bitLen, kRegTyInt);
  ImmOperand &immAddEnd = aarchCGFunc.CreateImmOperand(offsetVal, k64BitSize, true);
  RegOperand *origBaseReg = mo.GetBaseRegister();
  aarchCGFunc.SelectAdd(br, *origBaseReg, immAddEnd, PTY_i64);
  return &aarchCGFunc.CreateReplacementMemOperand(bitLen, br, 0);
}

void AArch64GenProEpilog::AppendInstructionPushPair(CGFunc &cgFunc,
    AArch64reg reg0, AArch64reg reg1, RegType rty, int32 offset) {
  auto &aarchCGFunc = static_cast<AArch64CGFunc&>(cgFunc);
  MOperator mOp = pushPopOps[kRegsPushOp][rty][kPushPopPair];
  Operand &o0 = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(reg0, GetPointerBitSize(), rty);
  Operand &o1 = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(reg1, GetPointerBitSize(), rty);
  Operand *o2 = &aarchCGFunc.CreateStkTopOpnd(static_cast<uint32>(offset), GetPointerBitSize());

  uint32 dataSize = GetPointerBitSize();
  CHECK_FATAL(offset >= 0, "offset must >= 0");
  if (offset > kStpLdpImm64UpperBound) {
    o2 = SplitStpLdpOffsetForCalleeSavedWithAddInstruction(cgFunc, *static_cast<MemOperand*>(o2), dataSize, R16);
  }
  Insn &pushInsn = cgFunc.GetInsnBuilder()->BuildInsn(mOp, o0, o1, *o2);
  // Identify that the instruction is not alias with any other memory instructions.
  auto *memDefUse = cgFunc.GetFuncScopeAllocator()->New<MemDefUse>(*cgFunc.GetFuncScopeAllocator());
  memDefUse->SetIndependent();
  pushInsn.SetReferenceOsts(memDefUse);
  std::string comment = "SAVE CALLEE REGISTER PAIR";
  pushInsn.SetComment(comment);
  AppendInstructionTo(pushInsn, cgFunc);
}

void AArch64GenProEpilog::AppendInstructionPushSingle(CGFunc &cgFunc,
    AArch64reg reg, RegType rty, int32 offset) {
  auto &aarchCGFunc = static_cast<AArch64CGFunc&>(cgFunc);
  MOperator mOp = pushPopOps[kRegsPushOp][rty][kPushPopSingle];
  Operand &o0 = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(reg, GetPointerBitSize(), rty);
  Operand *o1 = &aarchCGFunc.CreateStkTopOpnd(static_cast<uint32>(offset), GetPointerBitSize());

  MemOperand *aarchMemO1 = static_cast<MemOperand*>(o1);
  uint32 dataSize = GetPointerBitSize();
  if (aarchMemO1->GetMemVaryType() == kNotVary &&
      aarchCGFunc.IsImmediateOffsetOutOfRange(*aarchMemO1, dataSize)) {
    o1 = &aarchCGFunc.SplitOffsetWithAddInstruction(*aarchMemO1, dataSize, R16);
  }

  Insn &pushInsn = cgFunc.GetInsnBuilder()->BuildInsn(mOp, o0, *o1);
  // Identify that the instruction is not alias with any other memory instructions.
  auto *memDefUse = cgFunc.GetFuncScopeAllocator()->New<MemDefUse>(*cgFunc.GetFuncScopeAllocator());
  memDefUse->SetIndependent();
  pushInsn.SetReferenceOsts(memDefUse);
  std::string comment = "SAVE CALLEE REGISTER";
  pushInsn.SetComment(comment);
  AppendInstructionTo(pushInsn, cgFunc);
}

Insn &AArch64GenProEpilog::AppendInstructionForAllocateOrDeallocateCallFrame(int64 argsToStkPassSize,
                                                                             AArch64reg reg0, AArch64reg reg1,
                                                                             RegType rty, bool isAllocate) {
  auto &aarchCGFunc = static_cast<AArch64CGFunc&>(cgFunc);
  MOperator mOp = isAllocate ? pushPopOps[kRegsPushOp][rty][kPushPopPair] : pushPopOps[kRegsPopOp][rty][kPushPopPair];
  uint8 size;
  if (CGOptions::IsArm64ilp32()) {
    size = k8ByteSize;
  } else {
    size = GetPointerSize();
  }
  if (argsToStkPassSize <= kStrLdrImm64UpperBound - kOffset8MemPos) {
    mOp = isAllocate ? pushPopOps[kRegsPushOp][rty][kPushPopSingle] : pushPopOps[kRegsPopOp][rty][kPushPopSingle];
    MemOperand *o2 = aarchCGFunc.CreateStackMemOpnd(RSP, static_cast<int32>(argsToStkPassSize), size * kBitsPerByte);
    if (storeFP) {
      RegOperand &o0 = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(reg0, size * kBitsPerByte, rty);
      Insn &insn1 = cgFunc.GetInsnBuilder()->BuildInsn(mOp, o0, *o2);
      AppendInstructionTo(insn1, cgFunc);
    }
    RegOperand &o1 = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(reg1, size * kBitsPerByte, rty);
    o2 = aarchCGFunc.CreateStackMemOpnd(RSP, static_cast<int32>(argsToStkPassSize + size),
                                        size * kBitsPerByte);
    Insn &insn2 = cgFunc.GetInsnBuilder()->BuildInsn(mOp, o1, *o2);
    AppendInstructionTo(insn2, cgFunc);
    return insn2;
  } else {
    RegOperand &oo = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(R9, size * kBitsPerByte, kRegTyInt);
    ImmOperand &io1 = aarchCGFunc.CreateImmOperand(argsToStkPassSize, k64BitSize, true);
    aarchCGFunc.SelectCopyImm(oo, io1, PTY_i64);
    RegOperand &rsp = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(RSP, size * kBitsPerByte, kRegTyInt);
    MemOperand *mo = aarchCGFunc.CreateMemOperand(size * kBitsPerByte, rsp, oo);
    if (storeFP) {
      RegOperand &o0 = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(reg0, size * kBitsPerByte, rty);
      Insn &insn1 = cgFunc.GetInsnBuilder()->BuildInsn(isAllocate ? MOP_xstr : MOP_xldr, o0, *mo);
      AppendInstructionTo(insn1, cgFunc);
    }
    ImmOperand &io2 = aarchCGFunc.CreateImmOperand(size, k64BitSize, true);
    aarchCGFunc.SelectAdd(oo, oo, io2, PTY_i64);
    RegOperand &o1 = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(reg1, size * kBitsPerByte, rty);
    mo = aarchCGFunc.CreateMemOperand(size * kBitsPerByte, rsp, oo);
    Insn &insn2 = cgFunc.GetInsnBuilder()->BuildInsn(isAllocate ? MOP_xstr : MOP_xldr, o1, *mo);
    AppendInstructionTo(insn2, cgFunc);
    return insn2;
  }
}

Insn &AArch64GenProEpilog::CreateAndAppendInstructionForAllocateCallFrame(int64 argsToStkPassSize,
                                                                          AArch64reg reg0, AArch64reg reg1,
                                                                          RegType rty) {
  auto &aarchCGFunc = static_cast<AArch64CGFunc&>(cgFunc);
  CG *currCG = cgFunc.GetCG();
  MOperator mOp = (storeFP || argsToStkPassSize > kStrLdrPerPostUpperBound) ?
      pushPopOps[kRegsPushOp][rty][kPushPopPair] : pushPopOps[kRegsPushOp][rty][kPushPopSingle];
  Insn *allocInsn = nullptr;
  if (argsToStkPassSize > kStpLdpImm64UpperBound) {
    allocInsn = &AppendInstructionForAllocateOrDeallocateCallFrame(argsToStkPassSize, reg0, reg1, rty, true);
  } else {
    Operand &o0 = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(reg0, GetPointerBitSize(), rty);
    Operand &o1 = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(reg1, GetPointerBitSize(), rty);
    Operand *o2 = aarchCGFunc.CreateStackMemOpnd(RSP, static_cast<int32>(argsToStkPassSize),
                                                 GetPointerBitSize());
    allocInsn = (storeFP || argsToStkPassSize > kStrLdrPerPostUpperBound) ?
        &cgFunc.GetInsnBuilder()->BuildInsn(mOp, o0, o1, *o2) : &cgFunc.GetInsnBuilder()->BuildInsn(mOp, o1, *o2);
    AppendInstructionTo(*allocInsn, cgFunc);
  }
  if (currCG->InstrumentWithDebugTraceCall()) {
    aarchCGFunc.AppendCall(*currCG->GetDebugTraceEnterFunction());
  }
  return *allocInsn;
}

void AArch64GenProEpilog::AppendInstructionAllocateCallFrame(AArch64reg reg0, AArch64reg reg1, RegType rty) {
  auto &aarchCGFunc = static_cast<AArch64CGFunc&>(cgFunc);
  CG *currCG = cgFunc.GetCG();
  if (currCG->GenerateVerboseCG()) {
    auto &comment = cgFunc.GetOpndBuilder()->CreateComment("allocate activation frame");
    cgFunc.GetCurBB()->AppendInsn(cgFunc.GetInsnBuilder()->BuildCommentInsn(comment));
  }

  Insn *ipoint = nullptr;
  /*
   * stackFrameSize includes the size of args to stack-pass
   * if a function has neither VLA nor alloca.
   */
  int32 stackFrameSize = static_cast<int32>(
      static_cast<AArch64MemLayout*>(cgFunc.GetMemlayout())->RealStackFrameSize());
  int64 argsToStkPassSize = cgFunc.GetMemlayout()->SizeOfArgsToStackPass();
  /*
   * ldp/stp's imm should be within -512 and 504;
   * if stp's imm > 512, we fall back to the stp-sub version
   */
  bool useStpSub = false;
  int64 offset = 0;
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
  } else {
    if (stackFrameSize > kStpLdpImm64UpperBound) {
      useStpSub = true;
      offset = kOffset16MemPos;
      stackFrameSize -= offset;
    } else {
      offset = stackFrameSize;
    }
    MOperator mOp = (storeFP || offset > kStrLdrPerPostUpperBound) ?
        pushPopOps[kRegsPushOp][rty][kPushPopPair] : pushPopOps[kRegsPushOp][rty][kPushPopSingle];
    RegOperand &o0 = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(reg0, GetPointerBitSize(), rty);
    RegOperand &o1 = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(reg1, GetPointerBitSize(), rty);
    MemOperand &o2 = aarchCGFunc.CreateCallFrameOperand(static_cast<int32>(-offset), GetPointerBitSize());
    ipoint = (storeFP || offset > kStrLdrPerPostUpperBound) ?
        &cgFunc.GetInsnBuilder()->BuildInsn(mOp, o0, o1, o2) : &cgFunc.GetInsnBuilder()->BuildInsn(mOp, o1, o2);
    AppendInstructionTo(*ipoint, cgFunc);
    if (currCG->InstrumentWithDebugTraceCall()) {
      aarchCGFunc.AppendCall(*currCG->GetDebugTraceEnterFunction());
    }
  }
  ipoint->SetStackDef(true);

  if (!cgFunc.HasVLAOrAlloca() && argsToStkPassSize > 0) {
    CHECK_FATAL(!useStpSub, "Invalid assumption");
    ipoint = &CreateAndAppendInstructionForAllocateCallFrame(argsToStkPassSize, reg0, reg1, rty);
  }

  CHECK_FATAL(ipoint != nullptr, "ipoint should not be nullptr at this point");
  if (useStpSub) {
    Operand &spOpnd = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(RSP, k64BitSize, kRegTyInt);
    Operand &immOpnd = aarchCGFunc.CreateImmOperand(stackFrameSize, k32BitSize, true);
    aarchCGFunc.SelectSub(spOpnd, spOpnd, immOpnd, PTY_u64);
    ipoint = cgFunc.GetCurBB()->GetLastInsn();
    aarchCGFunc.SetUsedStpSubPairForCallFrameAllocation(true);
    ipoint->SetStackDef(true);
  }
}

void AArch64GenProEpilog::AppendInstructionAllocateCallFrameDebug(AArch64reg reg0, AArch64reg reg1, RegType rty) {
  auto &aarchCGFunc = static_cast<AArch64CGFunc&>(cgFunc);
  CG *currCG = cgFunc.GetCG();
  if (currCG->GenerateVerboseCG()) {
    auto &comment = cgFunc.GetOpndBuilder()->CreateComment("allocate activation frame for debugging");
    cgFunc.GetCurBB()->AppendInsn(cgFunc.GetInsnBuilder()->BuildCommentInsn(comment));
  }

  int32 stackFrameSize = static_cast<int32>(
      static_cast<AArch64MemLayout*>(cgFunc.GetMemlayout())->RealStackFrameSize());
  int64 argsToStkPassSize = cgFunc.GetMemlayout()->SizeOfArgsToStackPass();

  Insn *ipoint = nullptr;

  if (argsToStkPassSize > 0) {
    Operand &spOpnd = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(RSP, k64BitSize, kRegTyInt);
    Operand &immOpnd = aarchCGFunc.CreateImmOperand(stackFrameSize, k32BitSize, true);
    aarchCGFunc.SelectSub(spOpnd, spOpnd, immOpnd, PTY_u64);
    ipoint = cgFunc.GetCurBB()->GetLastInsn();
    ipoint->SetStackDef(true);
    ipoint = &CreateAndAppendInstructionForAllocateCallFrame(argsToStkPassSize, reg0, reg1, rty);
    CHECK_FATAL(ipoint != nullptr, "ipoint should not be nullptr at this point");
  } else {
    bool useStpSub = false;

    if (stackFrameSize > kStpLdpImm64UpperBound) {
      useStpSub = true;
      RegOperand &spOpnd = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(RSP, k64BitSize, kRegTyInt);
      ImmOperand &immOpnd = aarchCGFunc.CreateImmOperand(stackFrameSize, k32BitSize, true);
      aarchCGFunc.SelectSub(spOpnd, spOpnd, immOpnd, PTY_u64);
      ipoint = cgFunc.GetCurBB()->GetLastInsn();
      ipoint->SetStackDef(true);
    } else {
      MOperator mOp = (storeFP || stackFrameSize > kStrLdrPerPostUpperBound) ?
          pushPopOps[kRegsPushOp][rty][kPushPopPair] : pushPopOps[kRegsPushOp][rty][kPushPopSingle];
      RegOperand &o0 = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(reg0, GetPointerBitSize(), rty);
      RegOperand &o1 = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(reg1, GetPointerBitSize(), rty);
      MemOperand &o2 = aarchCGFunc.CreateCallFrameOperand(-stackFrameSize, GetPointerBitSize());
      ipoint = (storeFP || stackFrameSize > kStrLdrPerPostUpperBound) ?
          &cgFunc.GetInsnBuilder()->BuildInsn(mOp, o0, o1, o2) : &cgFunc.GetInsnBuilder()->BuildInsn(mOp, o1, o2);
      AppendInstructionTo(*ipoint, cgFunc);
      ipoint->SetStackDef(true);
    }

    if (useStpSub) {
      MOperator mOp = storeFP ? pushPopOps[kRegsPushOp][rty][kPushPopPair] :
                                pushPopOps[kRegsPushOp][rty][kPushPopSingle];
      RegOperand &o0 = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(reg0, GetPointerBitSize(), rty);
      RegOperand &o1 = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(reg1, GetPointerBitSize(), rty);
      MemOperand *o2 = aarchCGFunc.CreateStackMemOpnd(RSP, 0, GetPointerBitSize());
      ipoint = storeFP ? &cgFunc.GetInsnBuilder()->BuildInsn(mOp, o0, o1, *o2) :
                         &cgFunc.GetInsnBuilder()->BuildInsn(mOp, o1, *o2);
      AppendInstructionTo(*ipoint, cgFunc);
    }
    if (currCG->InstrumentWithDebugTraceCall()) {
      aarchCGFunc.AppendCall(*currCG->GetDebugTraceEnterFunction());
    }
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
  const MapleVector<AArch64reg> &regsToSave = (aarchCGFunc.GetProEpilogSavedRegs().empty()) ?
      aarchCGFunc.GetCalleeSavedRegs() : aarchCGFunc.GetProEpilogSavedRegs();

  CHECK_FATAL(!regsToSave.empty(), "FP/LR not added to callee-saved list?");

  AArch64reg intRegFirstHalf = kRinvalid;
  AArch64reg fpRegFirstHalf = kRinvalid;

  if (currCG->GenerateVerboseCG()) {
    auto &comment = cgFunc.GetOpndBuilder()->CreateComment("save callee-saved registers");
    cgFunc.GetCurBB()->AppendInsn(cgFunc.GetInsnBuilder()->BuildCommentInsn(comment));
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

  if (useFP) {
    if (currCG->GenerateVerboseCG()) {
      auto &comment = cgFunc.GetOpndBuilder()->CreateComment("copy SP to FP");
      cgFunc.GetCurBB()->AppendInsn(cgFunc.GetInsnBuilder()->BuildCommentInsn(comment));
    }
    Operand &spOpnd = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(RSP, k64BitSize, kRegTyInt);
    Operand &fpOpnd = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(stackBaseReg, k64BitSize, kRegTyInt);
    int64 argsToStkPassSize = cgFunc.GetMemlayout()->SizeOfArgsToStackPass();
    bool isLmbc = cgFunc.GetMirModule().GetFlavor() == MIRFlavor::kFlavorLmbc;
    if ((argsToStkPassSize > 0) || isLmbc) {
      Operand *immOpnd;
      if (isLmbc) {
        int32 size = static_cast<int32>(static_cast<AArch64MemLayout*>(cgFunc.GetMemlayout())->RealStackFrameSize());
        immOpnd = &aarchCGFunc.CreateImmOperand(size, k32BitSize, true);
      } else {
        immOpnd = &aarchCGFunc.CreateImmOperand(argsToStkPassSize, k32BitSize, true);
      }
      if (!isLmbc || cgFunc.SeenFP() || cgFunc.GetFunction().GetAttr(FUNCATTR_varargs)) {
        aarchCGFunc.SelectAdd(fpOpnd, spOpnd, *immOpnd, PTY_u64);
      }
      cgFunc.GetCurBB()->GetLastInsn()->SetFrameDef(true);
    } else {
      aarchCGFunc.SelectCopy(fpOpnd, PTY_u64, spOpnd, PTY_u64);
      cgFunc.GetCurBB()->GetLastInsn()->SetFrameDef(true);
    }
  }

  MapleVector<AArch64reg>::const_iterator it = regsToSave.begin();
  // skip the RFP & RLR
  if (*it == RFP) {
    ++it;
  }
  CHECK_FATAL(*it == RLR, "The second callee saved reg is expected to be RLR");
  ++it;

  // callee save offset
  // fp - callee save base = RealStackFrameSize - [GR,16] - [VR,16] - [cold,16] - [callee] - stack protect + 16(fplr)
  auto *memLayout = static_cast<AArch64MemLayout *>(cgFunc.GetMemlayout());
  int32 offset = 0;
  int32 tmp = 0;
  if (cgFunc.GetMirModule().GetFlavor() == MIRFlavor::kFlavorLmbc) {
    tmp = static_cast<int32>(memLayout->RealStackFrameSize() -
        /* FP/LR */
        (aarchCGFunc.SizeOfCalleeSaved() - (kDivide2 * kIntregBytelen)));
    offset = tmp - static_cast<int32>(memLayout->GetSizeOfLocals());
             /* SizeOfArgsToStackPass not deducted since
                AdjustmentStackPointer() is not called for lmbc */
  } else {
    tmp = static_cast<int32>(memLayout->RealStackFrameSize() -
        // FP/LR
        (aarchCGFunc.SizeOfCalleeSaved() - (kDivide2 * kIntregBytelen)));
    offset = tmp - static_cast<int32>(memLayout->SizeOfArgsToStackPass());
  }

  if (cgFunc.GetCG()->IsStackProtectorStrong() || cgFunc.GetCG()->IsStackProtectorAll()) {
    offset -= kAarch64StackPtrAlignmentInt;
  }

  if (cgFunc.GetMirModule().IsCModule() &&
      cgFunc.GetFunction().GetAttr(FUNCATTR_varargs) &&
      cgFunc.GetMirModule().GetFlavor() != MIRFlavor::kFlavorLmbc) {
    /* GR/VR save areas are above the callee save area */
    auto *ml = static_cast<AArch64MemLayout *>(cgFunc.GetMemlayout());
    auto saveareasize = static_cast<int32>(RoundUp(ml->GetSizeOfGRSaveArea(), GetPointerSize() * k2BitSize) +
        RoundUp(ml->GetSizeOfVRSaveArea(), GetPointerSize() * k2BitSize));
    offset -= saveareasize;
  }

  offset -= static_cast<int32>(RoundUp(memLayout->GetSizeOfSegCold(), k16BitSize));
  for (; it != regsToSave.end(); ++it) {
    AArch64reg reg = *it;
    // skip the RFP
    if (reg == RFP) {
      continue;
    }
    CHECK_FATAL(reg != RLR, "stray RLR in callee_saved_list?");
    RegType regType = AArch64isa::IsGPRegister(reg) ? kRegTyInt : kRegTyFloat;
    AArch64reg &firstHalf = AArch64isa::IsGPRegister(reg) ? intRegFirstHalf : fpRegFirstHalf;
    if (firstHalf == kRinvalid) {
      /* remember it */
      firstHalf = reg;
    } else {
      AppendInstructionPushPair(cgFunc, firstHalf, reg, regType, offset);
      GetNextOffsetCalleeSaved(offset);
      firstHalf = kRinvalid;
    }
  }

  if (intRegFirstHalf != kRinvalid) {
    AppendInstructionPushSingle(cgFunc, intRegFirstHalf, kRegTyInt, offset);
    GetNextOffsetCalleeSaved(offset);
  }

  if (fpRegFirstHalf != kRinvalid) {
    AppendInstructionPushSingle(cgFunc, fpRegFirstHalf, kRegTyFloat, offset);
    GetNextOffsetCalleeSaved(offset);
  }
}

void AArch64GenProEpilog::GeneratePushUnnamedVarargRegs() {
  auto &aarchCGFunc = static_cast<AArch64CGFunc&>(cgFunc);
  uint32 offset;
  if (cgFunc.GetMirModule().IsCModule() && cgFunc.GetFunction().GetAttr(FUNCATTR_varargs)) {
    AArch64MemLayout *memlayout = static_cast<AArch64MemLayout*>(cgFunc.GetMemlayout());
    uint8 size;
    if (CGOptions::IsArm64ilp32()) {
      size = k8ByteSize;
    } else {
      size = GetPointerSize();
    }
    uint32 dataSizeBits = size * kBitsPerByte;
    if (cgFunc.GetMirModule().GetFlavor() != MIRFlavor::kFlavorLmbc) {
      offset = static_cast<uint32>(memlayout->GetGRSaveAreaBaseLoc()); /* SP reference */
    } else {
      offset = static_cast<uint32>(memlayout->GetGRSaveAreaBaseLoc()) +
               memlayout->SizeOfArgsToStackPass();
    }
    if ((memlayout->GetSizeOfGRSaveArea() % kAarch64StackPtrAlignment) != 0) {
      offset += size;  /* End of area should be aligned. Hole between VR and GR area */
    }
    CHECK_FATAL(size != 0, "Divisor cannot be zero");
    uint32 startRegno = k8BitSize - (memlayout->GetSizeOfGRSaveArea() / size);
    ASSERT(startRegno <= k8BitSize, "Incorrect starting GR regno for GR Save Area");
    for (uint32 i = startRegno + static_cast<uint32>(R0); i < static_cast<uint32>(R8); i++) {
      uint32 tmpOffset = 0;
      if (CGOptions::IsBigEndian()) {
        if ((dataSizeBits >> 3) < 8) {
          tmpOffset += 8U - (dataSizeBits >> 3);
        }
      }
      Operand *stackLoc = &aarchCGFunc.CreateStkTopOpnd(offset + tmpOffset, dataSizeBits);
      RegOperand &reg =
          aarchCGFunc.GetOrCreatePhysicalRegisterOperand(static_cast<AArch64reg>(i), k64BitSize, kRegTyInt);
      Insn &inst = cgFunc.GetInsnBuilder()->BuildInsn(aarchCGFunc.PickStInsn(dataSizeBits, PTY_i64), reg, *stackLoc);
      cgFunc.GetCurBB()->AppendInsn(inst);
      offset += size;
    }
    if (!CGOptions::UseGeneralRegOnly()) {
      if (cgFunc.GetMirModule().GetFlavor() != MIRFlavor::kFlavorLmbc) {
        offset = static_cast<uint32>(memlayout->GetVRSaveAreaBaseLoc()); /* SP reference */
      } else {
        offset = static_cast<uint32>(memlayout->GetVRSaveAreaBaseLoc()) +
                 memlayout->SizeOfArgsToStackPass();
      }
      startRegno = k8BitSize - (memlayout->GetSizeOfVRSaveArea() / (size * k2BitSize));
      ASSERT(startRegno <= k8BitSize, "Incorrect starting GR regno for VR Save Area");
      dataSizeBits = k128BitSize;
      for (uint32 i = startRegno + static_cast<uint32>(V0); i < static_cast<uint32>(V8); i++) {
        uint32 tmpOffset = 0;
        if (CGOptions::IsBigEndian()) {
          if ((dataSizeBits >> 3) < 16) {
            tmpOffset += 16U - (dataSizeBits >> 3);
          }
        }
        Operand *stackLoc = &aarchCGFunc.CreateStkTopOpnd(offset + tmpOffset, dataSizeBits);
        RegOperand &reg =
            aarchCGFunc.GetOrCreatePhysicalRegisterOperand(static_cast<AArch64reg>(i),
                                                           dataSizeBits, kRegTyFloat);
        Insn &inst = cgFunc.GetInsnBuilder()->BuildInsn(aarchCGFunc.PickStInsn(dataSizeBits, PTY_f128), reg, *stackLoc);
        cgFunc.GetCurBB()->AppendInsn(inst);
        offset += size * k2BitSize;
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
  auto &wzr = cgFunc.GetZeroOpnd(k32BitSize);
  auto &refX16 = aarchCGFunc.CreateMemOpnd(reg, 0, k64BitSize);
  auto &soeInstr = cgFunc.GetInsnBuilder()->BuildInsn(MOP_wldr, wzr, refX16);
  if (currCG->GenerateVerboseCG()) {
    soeInstr.SetComment("soerror");
  }
  soeInstr.SetDoNotRemove(true);
  AppendInstructionTo(soeInstr, cgFunc);
}

void AArch64GenProEpilog::GenerateProlog(BB &bb) {
  if (!cgFunc.GetHasProEpilogue()) {
    return;
  }
  if (PROEPILOG_DUMP) {
    LogInfo::MapleLogger() << "generate prolog at BB " << bb.GetId() << "\n";
  }

  AddStackGuard(bb);

  auto &aarchCGFunc = static_cast<AArch64CGFunc&>(cgFunc);
  CG *currCG = cgFunc.GetCG();
  BB *formerCurBB = cgFunc.GetCurBB();
  aarchCGFunc.GetDummyBB()->ClearInsns();
  cgFunc.SetCurBB(*aarchCGFunc.GetDummyBB());

  // insert .loc for function
  if (currCG->GetCGOptions().WithLoc() && (!currCG->GetMIRModule()->IsCModule())) {
    MIRFunction *func = &cgFunc.GetFunction();
    MIRSymbol *fSym = GlobalTables::GetGsymTable().GetSymbolFromStidx(func->GetStIdx().Idx());
    if (currCG->GetCGOptions().WithSrc()) {
      uint32 tempmaxsize = static_cast<uint32>(currCG->GetMIRModule()->GetSrcFileInfo().size());
      uint32 endfilenum = currCG->GetMIRModule()->GetSrcFileInfo()[tempmaxsize - 1].second;
      if (fSym->GetSrcPosition().FileNum() != 0 && fSym->GetSrcPosition().FileNum() <= endfilenum) {
        int64_t lineNum = fSym->GetSrcPosition().LineNum();
        if (lineNum == 0) {
          if (cgFunc.GetFunction().GetAttr(FUNCATTR_native)) {
            lineNum = 0xffffe;
          } else {
            lineNum = 0xffffd;
          }
        }
        Insn &loc = cgFunc.BuildLocInsn(fSym->GetSrcPosition().FileNum(), lineNum, fSym->GetSrcPosition().Column());
        cgFunc.GetCurBB()->AppendInsn(loc);
      }
    } else {
      cgFunc.GetCurBB()->AppendInsn(cgFunc.BuildLocInsn(1, fSym->GetSrcPosition().MplLineNum(), 0));
    }
  }

  const MapleVector<AArch64reg> &regsToSave = (aarchCGFunc.GetProEpilogSavedRegs().empty()) ?
      aarchCGFunc.GetCalleeSavedRegs() : aarchCGFunc.GetProEpilogSavedRegs();
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
    int32 stackFrameSize = static_cast<int32>(
        static_cast<AArch64MemLayout*>(cgFunc.GetMemlayout())->RealStackFrameSize());
    if (stackFrameSize > 0) {
      if (currCG->GenerateVerboseCG()) {
        auto &comment = cgFunc.GetOpndBuilder()->CreateComment("allocate activation frame");
        cgFunc.GetCurBB()->AppendInsn(cgFunc.GetInsnBuilder()->BuildCommentInsn(comment));
      }
      Operand &immOpnd = aarchCGFunc.CreateImmOperand(stackFrameSize, k32BitSize, true);
      aarchCGFunc.SelectSub(spOpnd, spOpnd, immOpnd, PTY_u64);
      cgFunc.GetCurBB()->GetLastInsn()->SetStackDef(true);
    }
    if (currCG->GenerateVerboseCG()) {
      auto &comment = cgFunc.GetOpndBuilder()->CreateComment("copy SP to FP");
      cgFunc.GetCurBB()->AppendInsn(cgFunc.GetInsnBuilder()->BuildCommentInsn(comment));
    }
    if (useFP) {
      Operand &fpOpnd = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(stackBaseReg, k64BitSize, kRegTyInt);
      bool isLmbc = cgFunc.GetMirModule().GetFlavor() == MIRFlavor::kFlavorLmbc;
      int64 argsToStkPassSize = cgFunc.GetMemlayout()->SizeOfArgsToStackPass();
      if ((argsToStkPassSize > 0) || isLmbc) {
        Operand *immOpnd;
        if (isLmbc) {
          int32 size = static_cast<int32>(static_cast<AArch64MemLayout*>(cgFunc.GetMemlayout())->RealStackFrameSize());
          immOpnd = &aarchCGFunc.CreateImmOperand(size, k32BitSize, true);
        } else {
          immOpnd = &aarchCGFunc.CreateImmOperand(argsToStkPassSize, k32BitSize, true);
        }
        aarchCGFunc.SelectAdd(fpOpnd, spOpnd, *immOpnd, PTY_u64);
        cgFunc.GetCurBB()->GetLastInsn()->SetFrameDef(true);
      } else {
        aarchCGFunc.SelectCopy(fpOpnd, PTY_u64, spOpnd, PTY_u64);
        cgFunc.GetCurBB()->GetLastInsn()->SetFrameDef(true);
      }
    }
  }
  GeneratePushUnnamedVarargRegs();
  if (currCG->DoCheckSOE()) {
    AppendInstructionStackCheck(R16, kRegTyInt, kSoeChckOffset);
  }
  bb.InsertAtBeginning(*aarchCGFunc.GetDummyBB());
  cgFunc.SetCurBB(*formerCurBB);
}

void AArch64GenProEpilog::GenerateRet(BB &bb) {
  auto *lastInsn = bb.GetLastMachineInsn();
  if (lastInsn != nullptr && (lastInsn->IsTailCall() || lastInsn->IsBranch())) {
    return;
  }
  /* Insert the loc insn before ret insn
     so that the breakpoint can break at the end of the block's reverse parenthesis line. */
  SrcPosition pos = cgFunc.GetFunction().GetScope()->GetRangeHigh();
  if (cgFunc.GetCG()->GetCGOptions().WithDwarf() && cgFunc.GetWithSrc() &&
      cgFunc.GetMirModule().IsCModule() && pos.FileNum() != 0) {
    bb.AppendInsn(cgFunc.BuildLocInsn(pos.FileNum(), pos.LineNum(), pos.Column()));
  }
  bb.AppendInsn(cgFunc.GetInsnBuilder()->BuildInsn<AArch64CG>(MOP_xret));
}

/*
 * If exitBB made the TailcallOpt(replace blr/bl with br/b), return true, we don't create ret insn.
 * Otherwise, return false, create the ret insn.
 */
bool AArch64GenProEpilog::TestPredsOfRetBB(const BB &exitBB) {
  AArch64MemLayout *ml = static_cast<AArch64MemLayout*>(cgFunc.GetMemlayout());
  if (cgFunc.GetMirModule().IsCModule() &&
      (cgFunc.GetFunction().GetAttr(FUNCATTR_varargs) ||
       ml->GetSizeOfLocals() > 0 || cgFunc.HasVLAOrAlloca())) {
    return false;
  }
  const Insn *lastInsn = exitBB.GetLastInsn();
  while (lastInsn != nullptr && (!lastInsn->IsMachineInstruction() ||
                                 AArch64isa::IsPseudoInstruction(lastInsn->GetMachineOpcode()))) {
    lastInsn = lastInsn->GetPrev();
  }
  bool isTailCall = lastInsn == nullptr ? false : lastInsn->IsTailCall();
  return isTailCall;
}

void AArch64GenProEpilog::AppendInstructionPopSingle(CGFunc &cgFunc, AArch64reg reg, RegType rty, int32 offset) {
  auto &aarchCGFunc = static_cast<AArch64CGFunc&>(cgFunc);
  MOperator mOp = pushPopOps[kRegsPopOp][rty][kPushPopSingle];
  Operand &o0 = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(reg, GetPointerBitSize(), rty);
  Operand *o1 = &aarchCGFunc.CreateStkTopOpnd(static_cast<uint32>(offset), GetPointerBitSize());
  MemOperand *aarchMemO1 = static_cast<MemOperand*>(o1);
  uint32 dataSize = GetPointerBitSize();
  if (aarchMemO1->GetMemVaryType() == kNotVary && aarchCGFunc.IsImmediateOffsetOutOfRange(*aarchMemO1, dataSize)) {
    o1 = &aarchCGFunc.SplitOffsetWithAddInstruction(*aarchMemO1, dataSize, R16);
  }

  Insn &popInsn = cgFunc.GetInsnBuilder()->BuildInsn(mOp, o0, *o1);
  // Identify that the instruction is not alias with any other memory instructions.
  auto *memDefUse = cgFunc.GetFuncScopeAllocator()->New<MemDefUse>(*cgFunc.GetFuncScopeAllocator());
  memDefUse->SetIndependent();
  popInsn.SetReferenceOsts(memDefUse);
  popInsn.SetComment("RESTORE");
  cgFunc.GetCurBB()->AppendInsn(popInsn);
}

void AArch64GenProEpilog::AppendInstructionPopPair(CGFunc &cgFunc,
    AArch64reg reg0, AArch64reg reg1, RegType rty, int32 offset) {
  auto &aarchCGFunc = static_cast<AArch64CGFunc&>(cgFunc);
  MOperator mOp = pushPopOps[kRegsPopOp][rty][kPushPopPair];
  Operand &o0 = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(reg0, GetPointerBitSize(), rty);
  Operand &o1 = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(reg1, GetPointerBitSize(), rty);
  Operand *o2 = &aarchCGFunc.CreateStkTopOpnd(static_cast<uint32>(offset), GetPointerBitSize());

  uint32 dataSize = GetPointerBitSize();
  CHECK_FATAL(offset >= 0, "offset must >= 0");
  if (offset > kStpLdpImm64UpperBound) {
    o2 = SplitStpLdpOffsetForCalleeSavedWithAddInstruction(cgFunc, *static_cast<MemOperand*>(o2), dataSize, R16);
  }
  Insn &popInsn = cgFunc.GetInsnBuilder()->BuildInsn(mOp, o0, o1, *o2);
  // Identify that the instruction is not alias with any other memory instructions.
  auto *memDefUse = cgFunc.GetFuncScopeAllocator()->New<MemDefUse>(*cgFunc.GetFuncScopeAllocator());
  memDefUse->SetIndependent();
  popInsn.SetReferenceOsts(memDefUse);
  popInsn.SetComment("RESTORE RESTORE");
  cgFunc.GetCurBB()->AppendInsn(popInsn);
}


void AArch64GenProEpilog::AppendInstructionDeallocateCallFrame(AArch64reg reg0, AArch64reg reg1, RegType rty) {
  auto &aarchCGFunc = static_cast<AArch64CGFunc&>(cgFunc);
  MOperator mOp = pushPopOps[kRegsPopOp][rty][kPushPopPair];
  Operand &o0 = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(reg0, GetPointerBitSize(), rty);
  Operand &o1 = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(reg1, GetPointerBitSize(), rty);
  int32 stackFrameSize = static_cast<int32>(
      static_cast<AArch64MemLayout*>(cgFunc.GetMemlayout())->RealStackFrameSize());
  int64 argsToStkPassSize = cgFunc.GetMemlayout()->SizeOfArgsToStackPass();
  /*
   * ldp/stp's imm should be within -512 and 504;
   * if ldp's imm > 504, we fall back to the ldp-add version
   */
  bool useLdpAdd = false;
  int32 offset = 0;

  Operand *o2 = nullptr;
  if (!cgFunc.HasVLAOrAlloca() && argsToStkPassSize > 0) {
    o2 = aarchCGFunc.CreateStackMemOpnd(RSP, static_cast<int32>(argsToStkPassSize), GetPointerBitSize());
  } else {
    if (stackFrameSize > kStpLdpImm64UpperBound) {
      useLdpAdd = true;
      offset = kOffset16MemPos;
      stackFrameSize -= offset;
    } else {
      offset = stackFrameSize;
    }
    o2 = &aarchCGFunc.CreateCallFrameOperand(offset, GetPointerBitSize());
  }

  if (useLdpAdd) {
    Operand &spOpnd = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(RSP, k64BitSize, kRegTyInt);
    Operand &immOpnd = aarchCGFunc.CreateImmOperand(stackFrameSize, k32BitSize, true);
    aarchCGFunc.SelectAdd(spOpnd, spOpnd, immOpnd, PTY_u64);
  }

  if (!cgFunc.HasVLAOrAlloca() && argsToStkPassSize > 0) {
    CHECK_FATAL(!useLdpAdd, "Invalid assumption");
    if (argsToStkPassSize > kStpLdpImm64UpperBound) {
      (void)AppendInstructionForAllocateOrDeallocateCallFrame(argsToStkPassSize, reg0, reg1, rty, false);
    } else {
      Insn &deallocInsn = cgFunc.GetInsnBuilder()->BuildInsn(mOp, o0, o1, *o2);
      cgFunc.GetCurBB()->AppendInsn(deallocInsn);
    }
    Operand &spOpnd = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(RSP, k64BitSize, kRegTyInt);
    Operand &immOpnd = aarchCGFunc.CreateImmOperand(stackFrameSize, k32BitSize, true);
    aarchCGFunc.SelectAdd(spOpnd, spOpnd, immOpnd, PTY_u64);
  } else {
    Insn &deallocInsn = cgFunc.GetInsnBuilder()->BuildInsn(mOp, o0, o1, *o2);
    cgFunc.GetCurBB()->AppendInsn(deallocInsn);
  }
  cgFunc.GetCurBB()->GetLastInsn()->SetStackRevert(true);
}

void AArch64GenProEpilog::AppendInstructionDeallocateCallFrameDebug(AArch64reg reg0, AArch64reg reg1, RegType rty) {
  auto &aarchCGFunc = static_cast<AArch64CGFunc&>(cgFunc);
  MOperator mOp = pushPopOps[kRegsPopOp][rty][kPushPopPair];
  Operand &o0 = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(reg0, GetPointerBitSize(), rty);
  Operand &o1 = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(reg1, GetPointerBitSize(), rty);
  int32 stackFrameSize = static_cast<int32>(
      static_cast<AArch64MemLayout*>(cgFunc.GetMemlayout())->RealStackFrameSize());
  int32 argsToStkPassSize = static_cast<int32>(cgFunc.GetMemlayout()->SizeOfArgsToStackPass());
  /*
   * ldp/stp's imm should be within -512 and 504;
   * if ldp's imm > 504, we fall back to the ldp-add version
   */
  bool isLmbc = cgFunc.GetMirModule().GetFlavor() == MIRFlavor::kFlavorLmbc;
  if (cgFunc.HasVLAOrAlloca() || argsToStkPassSize == 0 || isLmbc) {
    int32 lmbcOffset = 0;
    if (!isLmbc) {
      stackFrameSize -= argsToStkPassSize;
    } else {
      lmbcOffset = argsToStkPassSize;
    }
    if (stackFrameSize > kStpLdpImm64UpperBound || isLmbc) {
      Operand *o2 = aarchCGFunc.CreateStackMemOpnd(RSP, (isLmbc ? lmbcOffset : 0), GetPointerBitSize());
      mOp = storeFP ? pushPopOps[kRegsPopOp][rty][kPushPopPair] : pushPopOps[kRegsPopOp][rty][kPushPopSingle];
      Insn &deallocInsn = storeFP ? cgFunc.GetInsnBuilder()->BuildInsn(mOp, o0, o1, *o2) :
                                    cgFunc.GetInsnBuilder()->BuildInsn(mOp, o1, *o2);
      cgFunc.GetCurBB()->AppendInsn(deallocInsn);
      Operand &spOpnd = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(RSP, k64BitSize, kRegTyInt);
      Operand &immOpnd = aarchCGFunc.CreateImmOperand(stackFrameSize, k32BitSize, true);
      aarchCGFunc.SelectAdd(spOpnd, spOpnd, immOpnd, PTY_u64);
    } else {
      MemOperand &o2 = aarchCGFunc.CreateCallFrameOperand(stackFrameSize, GetPointerBitSize());
      mOp = (storeFP || stackFrameSize > kStrLdrPerPostUpperBound) ?
          pushPopOps[kRegsPopOp][rty][kPushPopPair] : pushPopOps[kRegsPopOp][rty][kPushPopSingle];
      Insn &deallocInsn = (storeFP || stackFrameSize > kStrLdrPerPostUpperBound) ?
          cgFunc.GetInsnBuilder()->BuildInsn(mOp, o0, o1, o2) : cgFunc.GetInsnBuilder()->BuildInsn(mOp, o1, o2);
      cgFunc.GetCurBB()->AppendInsn(deallocInsn);
    }
  } else {
    Operand *o2 = aarchCGFunc.CreateStackMemOpnd(RSP, static_cast<int32>(argsToStkPassSize),
                                                 GetPointerBitSize());
    if (argsToStkPassSize > kStpLdpImm64UpperBound) {
      (void)AppendInstructionForAllocateOrDeallocateCallFrame(argsToStkPassSize, reg0, reg1, rty, false);
    } else {
      mOp = (storeFP || argsToStkPassSize > kStrLdrPerPostUpperBound) ?
          pushPopOps[kRegsPopOp][rty][kPushPopPair] : pushPopOps[kRegsPopOp][rty][kPushPopSingle];
      Insn &deallocInsn = (storeFP || argsToStkPassSize > kStrLdrPerPostUpperBound) ?
          cgFunc.GetInsnBuilder()->BuildInsn(mOp, o0, o1, *o2) : cgFunc.GetInsnBuilder()->BuildInsn(mOp, o1, *o2);
      cgFunc.GetCurBB()->AppendInsn(deallocInsn);
    }

    Operand &spOpnd = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(RSP, k64BitSize, kRegTyInt);
    Operand &immOpnd = aarchCGFunc.CreateImmOperand(stackFrameSize, k32BitSize, true);
    aarchCGFunc.SelectAdd(spOpnd, spOpnd, immOpnd, PTY_u64);
  }
  cgFunc.GetCurBB()->GetLastInsn()->SetStackRevert(true);
}

void AArch64GenProEpilog::GeneratePopRegs() {
  auto &aarchCGFunc = static_cast<AArch64CGFunc&>(cgFunc);
  CG *currCG = cgFunc.GetCG();

  const MapleVector<AArch64reg> &regsToRestore = (aarchCGFunc.GetProEpilogSavedRegs().empty()) ?
      aarchCGFunc.GetCalleeSavedRegs() : aarchCGFunc.GetProEpilogSavedRegs();

  CHECK_FATAL(!regsToRestore.empty(), "FP/LR not added to callee-saved list?");

  AArch64reg intRegFirstHalf = kRinvalid;
  AArch64reg fpRegFirstHalf = kRinvalid;

  if (currCG->GenerateVerboseCG()) {
    auto &comment = cgFunc.GetOpndBuilder()->CreateComment("restore callee-saved registers");
    cgFunc.GetCurBB()->AppendInsn(cgFunc.GetInsnBuilder()->BuildCommentInsn(comment));
  }

  MapleVector<AArch64reg>::const_iterator it = regsToRestore.begin();
  /*
   * Even if we don't use FP, since we push a pair of registers
   * in a single instruction (i.e., stp) and the stack needs be aligned
   * on a 16-byte boundary, push FP as well if the function has a call.
   * Make sure this is reflected when computing calleeSavedRegs.size()
   * skip the first two registers
   */
  // skip the RFP & RLR
  if (*it == RFP) {
    ++it;
  }
  CHECK_FATAL(*it == RLR, "The second callee saved reg is expected to be RLR");
  ++it;

  // callee save offset
  // fp - callee save base = RealStackFrameSize - [GR,16] - [VR,16] - [cold,16] - [callee] - stack protect + 16(fplr)
  AArch64MemLayout *memLayout = static_cast<AArch64MemLayout *>(cgFunc.GetMemlayout());
  int32 offset;
  int32 tmp;
  if (cgFunc.GetMirModule().GetFlavor() == MIRFlavor::kFlavorLmbc) {
    tmp = static_cast<int32>(memLayout->RealStackFrameSize() -
        /* FP/LR */
        (aarchCGFunc.SizeOfCalleeSaved() - (kDivide2 * kIntregBytelen)));
    offset = tmp - static_cast<int32>(memLayout->GetSizeOfLocals());
             /* SizeOfArgsToStackPass not deducted since
                AdjustmentStackPointer() is not called for lmbc */
  } else {
    tmp = static_cast<int32>(static_cast<AArch64MemLayout*>(cgFunc.GetMemlayout())->RealStackFrameSize() -
        /* for FP/LR */
        (aarchCGFunc.SizeOfCalleeSaved() - (kDivide2 * kIntregBytelen)));
    offset = tmp - static_cast<int32>(memLayout->SizeOfArgsToStackPass());
  }

  if (cgFunc.GetCG()->IsStackProtectorStrong() || cgFunc.GetCG()->IsStackProtectorAll()) {
    offset -= kAarch64StackPtrAlignmentInt;
  }

  if (cgFunc.GetMirModule().IsCModule() && cgFunc.GetFunction().GetAttr(FUNCATTR_varargs) &&
      cgFunc.GetMirModule().GetFlavor() != MIRFlavor::kFlavorLmbc) {
    /* GR/VR save areas are above the callee save area */
    AArch64MemLayout *ml = static_cast<AArch64MemLayout *>(cgFunc.GetMemlayout());
    auto saveareasize = static_cast<int32>(RoundUp(ml->GetSizeOfGRSaveArea(), GetPointerSize() * k2BitSize) +
        RoundUp(ml->GetSizeOfVRSaveArea(), GetPointerSize() * k2BitSize));
    offset -= saveareasize;
  }

  offset -= static_cast<int32>(RoundUp(memLayout->GetSizeOfSegCold(), k16BitSize));

  /*
   * We are using a cleared dummy block; so insertPoint cannot be ret;
   * see GenerateEpilog()
   */
  for (; it != regsToRestore.end(); ++it) {
    AArch64reg reg = *it;
    if (reg == RFP) {
      continue;
    }
    CHECK_FATAL(reg != RLR, "stray RLR in callee_saved_list?");
    RegType regType = AArch64isa::IsGPRegister(reg) ? kRegTyInt : kRegTyFloat;
    AArch64reg &firstHalf = AArch64isa::IsGPRegister(reg) ? intRegFirstHalf : fpRegFirstHalf;
    if (firstHalf == kRinvalid) {
      /* remember it */
      firstHalf = reg;
    } else {
      /* flush the pair */
      AppendInstructionPopPair(cgFunc, firstHalf, reg, regType, offset);
      GetNextOffsetCalleeSaved(offset);
      firstHalf = kRinvalid;
    }
  }

  if (intRegFirstHalf != kRinvalid) {
    AppendInstructionPopSingle(cgFunc, intRegFirstHalf, kRegTyInt, offset);
    GetNextOffsetCalleeSaved(offset);
  }

  if (fpRegFirstHalf != kRinvalid) {
    AppendInstructionPopSingle(cgFunc, fpRegFirstHalf, kRegTyFloat, offset);
    GetNextOffsetCalleeSaved(offset);
  }

  if (!currCG->GenerateDebugFriendlyCode()) {
    AppendInstructionDeallocateCallFrame(R29, RLR, kRegTyInt);
  } else {
    AppendInstructionDeallocateCallFrameDebug(R29, RLR, kRegTyInt);
  }
}

void AArch64GenProEpilog::AppendJump(const MIRSymbol &funcSymbol) {
  auto &aarchCGFunc = static_cast<AArch64CGFunc&>(cgFunc);
  Operand &targetOpnd = aarchCGFunc.GetOrCreateFuncNameOpnd(funcSymbol);
  cgFunc.GetCurBB()->AppendInsn(cgFunc.GetInsnBuilder()->BuildInsn(MOP_xuncond, targetOpnd));
}

void AArch64GenProEpilog::AppendBBtoEpilog(BB &epilogBB, BB &newBB) {
  if (epilogBB.GetPreds().empty() && &epilogBB != cgFunc.GetFirstBB() &&
      cgFunc.GetMirModule().IsCModule() && CGOptions::DoTailCallOpt()) {
    Insn &junk = cgFunc.GetInsnBuilder()->BuildInsn<AArch64CG>(MOP_pseudo_none);
    epilogBB.AppendInsn(junk);
    return;
  }
  FOR_BB_INSNS(insn, &newBB) {
    insn->SetDoNotRemove(true);
  }
  auto *lastInsn = epilogBB.GetLastMachineInsn();
  if (lastInsn != nullptr && (lastInsn->IsTailCall() || lastInsn->IsBranch())) {
    epilogBB.RemoveInsn(*lastInsn);
    epilogBB.AppendBBInsns(newBB);
    epilogBB.AppendInsn(*lastInsn);
  } else {
    epilogBB.AppendBBInsns(newBB);
  }
}

void AArch64GenProEpilog::GenerateEpilog(BB &bb) {
  if (!cgFunc.GetHasProEpilogue()) {
    return;
  }
  if (PROEPILOG_DUMP) {
    LogInfo::MapleLogger() << "generate epilog at BB " << bb.GetId() << "\n";
  }

  /* generate stack protected instruction */
  GenStackGuardCheckInsn(bb);

  auto &aarchCGFunc = static_cast<AArch64CGFunc&>(cgFunc);
  CG *currCG = cgFunc.GetCG();
  BB *formerCurBB = cgFunc.GetCurBB();
  aarchCGFunc.GetDummyBB()->ClearInsns();
  cgFunc.SetCurBB(*aarchCGFunc.GetDummyBB());
  Operand &spOpnd = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(RSP, k64BitSize, kRegTyInt);
  Operand &fpOpnd = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(stackBaseReg, k64BitSize, kRegTyInt);

  if (cgFunc.HasVLAOrAlloca() && cgFunc.GetMirModule().GetFlavor() != MIRFlavor::kFlavorLmbc) {
    aarchCGFunc.SelectCopy(spOpnd, PTY_u64, fpOpnd, PTY_u64);
  }

  const MapleVector<AArch64reg> &regsToSave = (aarchCGFunc.GetProEpilogSavedRegs().empty()) ?
      aarchCGFunc.GetCalleeSavedRegs() : aarchCGFunc.GetProEpilogSavedRegs();
  if (!regsToSave.empty()) {
    GeneratePopRegs();
  } else {
    auto stackFrameSize = static_cast<AArch64MemLayout*>(cgFunc.GetMemlayout())->RealStackFrameSize();
    if (stackFrameSize > 0) {
      if (currCG->GenerateVerboseCG()) {
        auto &comment = cgFunc.GetOpndBuilder()->CreateComment("pop up activation frame");
        cgFunc.GetCurBB()->AppendInsn(cgFunc.GetInsnBuilder()->BuildCommentInsn(comment));
      }

      if (cgFunc.HasVLAOrAlloca()) {
        auto size = static_cast<AArch64MemLayout*>(cgFunc.GetMemlayout())->GetSegArgsToStkPass().GetSize();
        stackFrameSize = stackFrameSize < size ? 0 : stackFrameSize - size;
      }

      if (stackFrameSize > 0) {
        Operand &immOpnd = aarchCGFunc.CreateImmOperand(stackFrameSize, k32BitSize, true);
        aarchCGFunc.SelectAdd(spOpnd, spOpnd, immOpnd, PTY_u64);
        aarchCGFunc.GetCurBB()->GetLastInsn()->SetStackRevert(true);
      }
    }
  }

  if (currCG->InstrumentWithDebugTraceCall()) {
    AppendJump(*(currCG->GetDebugTraceExitFunction()));
  }

  AppendBBtoEpilog(bb, *cgFunc.GetCurBB());
  if (cgFunc.GetCurBB()->GetHasCfi()) {
    bb.SetHasCfi();
  }

  cgFunc.SetCurBB(*formerCurBB);
}

void AArch64GenProEpilog::GenerateEpilogForCleanup(BB &bb) {
  auto &aarchCGFunc = static_cast<AArch64CGFunc&>(cgFunc);
  CHECK_FATAL(!cgFunc.GetExitBBsVec().empty(), "exit bb size is zero!");
  if (cgFunc.GetExitBB(0)->IsUnreachable()) {
    /* if exitbb is unreachable then exitbb can not be generated */
    GenerateEpilog(bb);
  } else if (aarchCGFunc.NeedCleanup()) {  /* bl to the exit epilogue */
    LabelOperand &targetOpnd = aarchCGFunc.GetOrCreateLabelOperand(cgFunc.GetExitBB(0)->GetLabIdx());
    bb.AppendInsn(cgFunc.GetInsnBuilder()->BuildInsn(MOP_xuncond, targetOpnd));
  }
}

void AArch64GenProEpilog::Run() {
  CHECK_FATAL(cgFunc.GetFunction().GetBody()->GetFirst()->GetOpCode() == OP_label,
              "The first statement should be a label");
  // update exitBB
  if (cgFunc.IsExitBBsVecEmpty()) {
    if (cgFunc.GetCleanupBB() != nullptr && cgFunc.GetCleanupBB()->GetPrev() != nullptr) {
      cgFunc.PushBackExitBBsVec(*cgFunc.GetCleanupBB()->GetPrev());
    } else if (!cgFunc.GetMirModule().IsCModule()) {
      cgFunc.PushBackExitBBsVec(*cgFunc.GetLastBB()->GetPrev());
    }
  }

  cgFunc.SetHasProEpilogue(NeedProEpilog());
  if (saveInfo == nullptr || saveInfo->prologBBs.empty()) {
    // not run proepilog analysis or analysis failed, insert proepilog at firstBB and exitBB
    GenerateProlog(*(cgFunc.GetFirstBB()));

    for (auto *exitBB : cgFunc.GetExitBBsVec()) {
      GenerateEpilog(*exitBB);
    }

    if (cgFunc.GetFunction().IsJava()) {
      GenerateEpilogForCleanup(*(cgFunc.GetCleanupBB()));
    }
  } else {
    CHECK_FATAL(cgFunc.GetMirModule().IsCModule(), "NIY, only support C");
    for (auto bbId : saveInfo->prologBBs) {
      GenerateProlog(*cgFunc.GetBBFromID(bbId));
    }
    for (auto bbId : saveInfo->epilogBBs) {
      GenerateEpilog(*cgFunc.GetBBFromID(bbId));
    }
  }

  // insert ret insn for exitBB
  for (auto *exitBB : cgFunc.GetExitBBsVec()) {
    if (cgFunc.GetHasProEpilogue() || (!exitBB->GetPreds().empty() && !TestPredsOfRetBB(*exitBB))) {
      GenerateRet(*exitBB);
    }
  }
}
}  /* namespace maplebe */
