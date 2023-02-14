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
#include "aarch64_args.h"
#include <fstream>
#include "aarch64_cgfunc.h"
#include "aarch64_cg.h"

namespace maplebe {
using namespace maple;

void AArch64MoveRegArgs::Run() {
  MoveVRegisterArgs();
  MoveRegisterArgs();
}

void AArch64MoveRegArgs::CollectRegisterArgs(std::map<uint32, AArch64reg> &argsList,
                                             std::vector<uint32> &indexList,
                                             std::map<uint32, AArch64reg> &pairReg,
                                             std::vector<uint32> &numFpRegs,
                                             std::vector<uint32> &fpSize) const {
  uint32 numFormal = static_cast<uint32>(aarFunc->GetFunction().GetFormalCount());
  numFpRegs.resize(numFormal);
  fpSize.resize(numFormal);
  AArch64CallConvImpl parmlocator(aarFunc->GetBecommon());
  CCLocInfo ploc;
  uint32 start = 0;
  if (numFormal > 0) {
    MIRFunction *func = aarFunc->GetBecommon().GetMIRModule().CurFunction();
    if (func->IsReturnStruct() && func->IsFirstArgReturn()) {
      TyIdx tyIdx = func->GetFuncRetStructTyIdx();
      if (aarFunc->GetBecommon().GetTypeSize(tyIdx) <= k16ByteSize) {
        start = 1;
      }
    }
  }
  for (uint32 i = start; i < numFormal; ++i) {
    MIRType *ty = aarFunc->GetFunction().GetNthParamType(i);
    parmlocator.LocateNextParm(*ty, ploc, i == 0, &aarFunc->GetFunction());
    if (ploc.reg0 == kRinvalid) {
      continue;
    }
    AArch64reg reg0 = static_cast<AArch64reg>(ploc.reg0);
    MIRSymbol *sym = aarFunc->GetFunction().GetFormal(i);
    if (sym->IsPreg()) {
      continue;
    }
    argsList[i] = reg0;
    indexList.emplace_back(i);
    if (ploc.reg1 == kRinvalid) {
      continue;
    }
    if (ploc.numFpPureRegs > 0) {
      uint32 index = i;
      numFpRegs[index] = ploc.numFpPureRegs;
      fpSize[index] = ploc.fpSize;
      continue;
    }
    pairReg[i] = static_cast<AArch64reg>(ploc.reg1);
  }
}

ArgInfo AArch64MoveRegArgs::GetArgInfo(std::map<uint32, AArch64reg> &argsList, std::vector<uint32> &numFpRegs,
                                       std::vector<uint32> &fpSize, uint32 argIndex) const {
  ArgInfo argInfo;
  argInfo.reg = argsList[argIndex];
  argInfo.mirTy = aarFunc->GetFunction().GetNthParamType(argIndex);
  argInfo.symSize = aarFunc->GetBecommon().GetTypeSize(argInfo.mirTy->GetTypeIndex());
  argInfo.memPairSecondRegSize = 0;
  argInfo.doMemPairOpt = false;
  argInfo.createTwoStores  = false;
  argInfo.isTwoRegParm = false;

  if (GetVecLanes(argInfo.mirTy->GetPrimType()) > 0) {
    /* vector type */
    argInfo.stkSize = argInfo.symSize;
  } else if ((argInfo.symSize > k8ByteSize) && (argInfo.symSize <= k16ByteSize)) {
    argInfo.isTwoRegParm = true;
    if (numFpRegs[argIndex] > kOneRegister) {
      argInfo.symSize = argInfo.stkSize = fpSize[argIndex];
    } else {
      if (argInfo.symSize > k12ByteSize) {
        argInfo.memPairSecondRegSize = k8ByteSize;
      } else {
        /* Round to 4 the stack space required for storing the struct */
        argInfo.memPairSecondRegSize = k4ByteSize;
      }
      argInfo.doMemPairOpt = true;
      if (CGOptions::IsArm64ilp32()) {
        argInfo.symSize = argInfo.stkSize = k8ByteSize;
      } else {
        argInfo.symSize = argInfo.stkSize = GetPointerSize();
      }
    }
  } else if (argInfo.symSize > k16ByteSize) {
    /* For large struct passing, a pointer to the copy is used. */
    if (CGOptions::IsArm64ilp32()) {
      argInfo.symSize = argInfo.stkSize = k8ByteSize;
    } else {
      argInfo.symSize = argInfo.stkSize = GetPointerSize();
    }
  } else if ((argInfo.mirTy->GetPrimType() == PTY_agg) && (argInfo.symSize < k8ByteSize)) {
    /*
     * For small aggregate parameter, set to minimum of 8 bytes.
     * B.5:If the argument type is a Composite Type then the size of the argument is rounded up to the
     * nearest multiple of 8 bytes.
     */
    argInfo.symSize = argInfo.stkSize = k8ByteSize;
  } else if (numFpRegs[argIndex] > kOneRegister) {
    argInfo.isTwoRegParm = true;
    argInfo.symSize = argInfo.stkSize = fpSize[argIndex];
  } else {
    argInfo.stkSize = (argInfo.symSize < k4ByteSize) ? k4ByteSize : argInfo.symSize;
    if (argInfo.symSize > k4ByteSize) {
      argInfo.symSize = k8ByteSize;
    }
  }
  argInfo.regType = (argInfo.reg < V0) ? kRegTyInt : kRegTyFloat;
  argInfo.sym = aarFunc->GetFunction().GetFormal(argIndex);
  CHECK_NULL_FATAL(argInfo.sym);
  argInfo.symLoc =
      static_cast<const AArch64SymbolAlloc*>(aarFunc->GetMemlayout()->GetSymAllocInfo(argInfo.sym->GetStIndex()));
  CHECK_NULL_FATAL(argInfo.symLoc);
  if (argInfo.doMemPairOpt && ((static_cast<uint32>(aarFunc->GetBaseOffset(*(argInfo.symLoc))) & 0x7) != 0)) {
    /* Do not optimize for struct reg pair for unaligned access.
     * However, this symbol requires two parameter registers, separate stores must be generated.
     */
    argInfo.symSize = GetPointerSize();
    argInfo.doMemPairOpt = false;
    argInfo.createTwoStores = true;
  }
  return argInfo;
}

bool AArch64MoveRegArgs::IsInSameSegment(const ArgInfo &firstArgInfo, const ArgInfo &secondArgInfo) const {
  if (firstArgInfo.symLoc->GetMemSegment() != secondArgInfo.symLoc->GetMemSegment()) {
    return false;
  }
  if (firstArgInfo.symSize != secondArgInfo.symSize) {
    return false;
  }
  if (firstArgInfo.symSize != k4ByteSize && firstArgInfo.symSize != k8ByteSize) {
    return false;
  }
  if (firstArgInfo.regType != secondArgInfo.regType) {
    return false;
  }
  return firstArgInfo.symLoc->GetOffset() + firstArgInfo.stkSize == secondArgInfo.symLoc->GetOffset();
}

void AArch64MoveRegArgs::GenerateStpInsn(const ArgInfo &firstArgInfo, const ArgInfo &secondArgInfo) {
  RegOperand *baseOpnd = static_cast<RegOperand*>(aarFunc->GetBaseReg(*firstArgInfo.symLoc));
  RegOperand &regOpnd = aarFunc->GetOrCreatePhysicalRegisterOperand(firstArgInfo.reg,
                                                                        firstArgInfo.stkSize * kBitsPerByte,
                                                                        firstArgInfo.regType);
  MOperator mOp = firstArgInfo.regType == kRegTyInt ? ((firstArgInfo.stkSize > k4ByteSize) ? MOP_xstp : MOP_wstp)
                                                    : ((firstArgInfo.stkSize > k4ByteSize) ? MOP_dstp : MOP_sstp);
  RegOperand *regOpnd2 = &aarFunc->GetOrCreatePhysicalRegisterOperand(secondArgInfo.reg,
                                                                          firstArgInfo.stkSize * kBitsPerByte,
                                                                          firstArgInfo.regType);
  if (firstArgInfo.doMemPairOpt && firstArgInfo.isTwoRegParm) {
    AArch64reg regFp2 = static_cast<AArch64reg>(firstArgInfo.reg + kOneRegister);
    regOpnd2 = &aarFunc->GetOrCreatePhysicalRegisterOperand(regFp2,
                                                                firstArgInfo.stkSize * kBitsPerByte,
                                                                firstArgInfo.regType);
  }

  int32 limit = (secondArgInfo.stkSize > k4ByteSize) ? kStpLdpImm64UpperBound : kStpLdpImm32UpperBound;
  int32 stOffset = aarFunc->GetBaseOffset(*firstArgInfo.symLoc);
  MemOperand *memOpnd = nullptr;
  if (stOffset > limit || baseReg != nullptr) {
    if (baseReg == nullptr || lastSegment != firstArgInfo.symLoc->GetMemSegment()) {
      ImmOperand &immOpnd =
          aarFunc->CreateImmOperand(stOffset - firstArgInfo.symLoc->GetOffset(), k64BitSize, false);
      baseReg = &aarFunc->CreateRegisterOperandOfType(kRegTyInt, k8ByteSize);
      lastSegment = firstArgInfo.symLoc->GetMemSegment();
      aarFunc->SelectAdd(*baseReg, *baseOpnd, immOpnd, GetLoweredPtrType());
    }
    uint64 offVal = static_cast<uint64>(firstArgInfo.symLoc->GetOffset());
    OfstOperand &offsetOpnd = aarFunc->CreateOfstOpnd(offVal, k32BitSize);
    if (firstArgInfo.symLoc->GetMemSegment()->GetMemSegmentKind() == kMsArgsStkPassed) {
      offsetOpnd.SetVary(kUnAdjustVary);
    }
    memOpnd = aarFunc->CreateMemOperand(firstArgInfo.stkSize * kBitsPerByte, *baseReg, offsetOpnd);
  } else {
    OfstOperand &offsetOpnd = aarFunc->CreateOfstOpnd(static_cast<uint64>(static_cast<int64>(stOffset)),
                                                          k32BitSize);
    if (firstArgInfo.symLoc->GetMemSegment()->GetMemSegmentKind() == kMsArgsStkPassed) {
      offsetOpnd.SetVary(kUnAdjustVary);
    }
    memOpnd = aarFunc->CreateMemOperand(firstArgInfo.stkSize * kBitsPerByte, *baseOpnd, offsetOpnd);
  }
  Insn &pushInsn = aarFunc->GetInsnBuilder()->BuildInsn(mOp, regOpnd, *regOpnd2, *memOpnd);
  if (aarFunc->GetCG()->GenerateVerboseCG()) {
    std::string argName = firstArgInfo.sym->GetName() + " " + secondArgInfo.sym->GetName();
    pushInsn.SetComment(std::string("store param: ").append(argName));
  }
  aarFunc->GetCurBB()->AppendInsn(pushInsn);
}

void AArch64MoveRegArgs::GenOneInsn(const ArgInfo &argInfo, RegOperand &baseOpnd, uint32 stBitSize, AArch64reg dest,
                                    int32 offset) const {
  MOperator mOp = aarFunc->PickStInsn(stBitSize, argInfo.mirTy->GetPrimType());
  RegOperand &regOpnd = aarFunc->GetOrCreatePhysicalRegisterOperand(dest, stBitSize, argInfo.regType);

  OfstOperand &offsetOpnd = aarFunc->CreateOfstOpnd(static_cast<uint64>(static_cast<int64>(offset)), k32BitSize);
  if (argInfo.symLoc->GetMemSegment()->GetMemSegmentKind() == kMsArgsStkPassed) {
    offsetOpnd.SetVary(kUnAdjustVary);
  }
  MemOperand *memOpnd = aarFunc->CreateMemOperand(stBitSize, baseOpnd, offsetOpnd);
  Insn &insn = aarFunc->GetInsnBuilder()->BuildInsn(mOp, regOpnd, *memOpnd);
  if (aarFunc->GetCG()->GenerateVerboseCG()) {
    insn.SetComment(std::string("store param: ").append(argInfo.sym->GetName()));
  }
  aarFunc->GetCurBB()->AppendInsn(insn);
}

void AArch64MoveRegArgs::GenerateStrInsn(const ArgInfo &argInfo, AArch64reg reg2, uint32 numFpRegs, uint32 fpSize) {
  int32 stOffset = aarFunc->GetBaseOffset(*argInfo.symLoc);
  auto *baseOpnd = static_cast<RegOperand*>(aarFunc->GetBaseReg(*argInfo.symLoc));
  RegOperand &regOpnd =
      aarFunc->GetOrCreatePhysicalRegisterOperand(argInfo.reg, argInfo.stkSize * kBitsPerByte, argInfo.regType);
  MemOperand *memOpnd = nullptr;
  if (MemOperand::IsPIMMOffsetOutOfRange(stOffset, argInfo.symSize * kBitsPerByte) ||
      (baseReg != nullptr && (lastSegment == argInfo.symLoc->GetMemSegment()))) {
    if (baseReg == nullptr || lastSegment != argInfo.symLoc->GetMemSegment()) {
      ImmOperand &immOpnd = aarFunc->CreateImmOperand(stOffset - argInfo.symLoc->GetOffset(), k64BitSize,
                                                          false);
      baseReg = &aarFunc->CreateRegisterOperandOfType(kRegTyInt, k8ByteSize);
      lastSegment = argInfo.symLoc->GetMemSegment();
      aarFunc->SelectAdd(*baseReg, *baseOpnd, immOpnd, PTY_a64);
    }
    OfstOperand &offsetOpnd = aarFunc->CreateOfstOpnd(static_cast<uint64>(argInfo.symLoc->GetOffset()), k32BitSize);
    if (argInfo.symLoc->GetMemSegment()->GetMemSegmentKind() == kMsArgsStkPassed) {
      offsetOpnd.SetVary(kUnAdjustVary);
    }
    memOpnd = aarFunc->CreateMemOperand(argInfo.symSize * kBitsPerByte, *baseReg, offsetOpnd);
  } else {
    OfstOperand &offsetOpnd = aarFunc->CreateOfstOpnd(static_cast<uint64>(static_cast<int64>(stOffset)),
        k32BitSize);
    if (argInfo.symLoc->GetMemSegment()->GetMemSegmentKind() == kMsArgsStkPassed) {
      offsetOpnd.SetVary(kUnAdjustVary);
    }
    memOpnd = aarFunc->CreateMemOperand(argInfo.symSize * kBitsPerByte, *baseOpnd, offsetOpnd);
  }

  MOperator mOp = aarFunc->PickStInsn(argInfo.symSize * kBitsPerByte, argInfo.mirTy->GetPrimType());
  Insn &insn = aarFunc->GetInsnBuilder()->BuildInsn(mOp, regOpnd, *memOpnd);
  if (aarFunc->GetCG()->GenerateVerboseCG()) {
    insn.SetComment(std::string("store param: ").append(argInfo.sym->GetName()));
  }
  aarFunc->GetCurBB()->AppendInsn(insn);

  if (argInfo.createTwoStores || argInfo.doMemPairOpt) {
    /* second half of the struct passing by registers. */
    uint32 part2BitSize = argInfo.memPairSecondRegSize * kBitsPerByte;
    GenOneInsn(argInfo, *baseOpnd, part2BitSize, reg2, (stOffset + GetPointerSize()));
  } else if (numFpRegs > kOneRegister) {
    uint32 fpSizeBits = fpSize * kBitsPerByte;
    AArch64reg regFp2 = static_cast<AArch64reg>(argInfo.reg + kOneRegister);
    GenOneInsn(argInfo, *baseOpnd, fpSizeBits, regFp2, (stOffset + static_cast<int>(fpSize)));
    if (numFpRegs > kTwoRegister) {
      AArch64reg regFp3 = static_cast<AArch64reg>(argInfo.reg + kTwoRegister);
      GenOneInsn(argInfo, *baseOpnd, fpSizeBits, regFp3, (stOffset + static_cast<int>(fpSize * k4BitShift)));
    }
    if (numFpRegs > kThreeRegister) {
      AArch64reg regFp3 = static_cast<AArch64reg>(argInfo.reg + kThreeRegister);
      GenOneInsn(argInfo, *baseOpnd, fpSizeBits, regFp3, (stOffset + static_cast<int>(fpSize * k8BitShift)));
    }
  }
}

void AArch64MoveRegArgs::MoveRegisterArgs() {
  BB *formerCurBB = aarFunc->GetCurBB();
  aarFunc->GetDummyBB()->ClearInsns();
  aarFunc->SetCurBB(*aarFunc->GetDummyBB());

  std::map<uint32, AArch64reg> movePara;
  std::vector<uint32> moveParaIndex;
  std::map<uint32, AArch64reg> pairReg;
  std::vector<uint32> numFpRegs;
  std::vector<uint32> fpSize;
  CollectRegisterArgs(movePara, moveParaIndex, pairReg, numFpRegs, fpSize);

  std::vector<uint32>::iterator it;
  std::vector<uint32>::iterator next;
  for (it = moveParaIndex.begin(); it != moveParaIndex.end(); ++it) {
    uint32 firstIndex = *it;
    ArgInfo firstArgInfo = GetArgInfo(movePara, numFpRegs, fpSize, firstIndex);
    next = it;
    ++next;
    if ((next != moveParaIndex.end()) || (firstArgInfo.doMemPairOpt)) {
      uint32 secondIndex = (firstArgInfo.doMemPairOpt) ? firstIndex : *next;
      ArgInfo secondArgInfo = GetArgInfo(movePara, numFpRegs, fpSize, secondIndex);
      secondArgInfo.reg = (firstArgInfo.doMemPairOpt) ? pairReg[firstIndex] : movePara[secondIndex];
      secondArgInfo.symSize = (firstArgInfo.doMemPairOpt) ? firstArgInfo.memPairSecondRegSize : secondArgInfo.symSize;
      secondArgInfo.symLoc = (firstArgInfo.doMemPairOpt) ? secondArgInfo.symLoc :
          static_cast<AArch64SymbolAlloc *>(aarFunc->GetMemlayout()->GetSymAllocInfo(
              secondArgInfo.sym->GetStIndex()));
      /* Make sure they are in same segment if want to use stp */
      if (((firstArgInfo.isTwoRegParm && secondArgInfo.isTwoRegParm) ||
          (!firstArgInfo.isTwoRegParm && !secondArgInfo.isTwoRegParm)) &&
          (firstArgInfo.doMemPairOpt || IsInSameSegment(firstArgInfo, secondArgInfo))) {
        GenerateStpInsn(firstArgInfo, secondArgInfo);
        if (!firstArgInfo.doMemPairOpt) {
          it = next;
        }
        continue;
      }
    }
    GenerateStrInsn(firstArgInfo, pairReg[firstIndex], numFpRegs[firstIndex], fpSize[firstIndex]);
  }

  if (cgFunc->GetCG()->IsLmbc() && (cgFunc->GetSpSaveReg() != 0)) {
    /* lmbc uses vreg act as SP when alloca is present due to usage of FP for - offset */
    aarFunc->GetFirstBB()->InsertAtEnd(*aarFunc->GetDummyBB());
  } else {
    /* Java requires insertion at begining as it has fast unwind and other features */
    aarFunc->GetFirstBB()->InsertAtBeginning(*aarFunc->GetDummyBB());
  }
  aarFunc->SetCurBB(*formerCurBB);
}

void AArch64MoveRegArgs::MoveLocalRefVarToRefLocals(MIRSymbol &mirSym) const {
  PrimType stype = mirSym.GetType()->GetPrimType();
  uint32 byteSize = GetPrimTypeSize(stype);
  uint32 bitSize = byteSize * kBitsPerByte;
  MemOperand &memOpnd = aarFunc->GetOrCreateMemOpnd(mirSym, 0, bitSize, true);
  RegOperand *regOpnd = nullptr;
  if (mirSym.IsPreg()) {
    PregIdx pregIdx = aarFunc->GetFunction().GetPregTab()->GetPregIdxFromPregno(mirSym.GetPreg()->GetPregNo());
    regOpnd = &aarFunc->GetOrCreateVirtualRegisterOperand(aarFunc->GetVirtualRegNOFromPseudoRegIdx(pregIdx));
  } else {
    regOpnd = &aarFunc->GetOrCreateVirtualRegisterOperand(aarFunc->NewVReg(kRegTyInt, k8ByteSize));
  }
  Insn &insn = aarFunc->GetInsnBuilder()->BuildInsn(
      aarFunc->PickLdInsn(GetPrimTypeBitSize(stype), stype), *regOpnd, memOpnd);
  MemOperand &memOpnd1 = aarFunc->GetOrCreateMemOpnd(mirSym, 0, bitSize, false);
  Insn &insn1 = aarFunc->GetInsnBuilder()->BuildInsn(
      aarFunc->PickStInsn(GetPrimTypeBitSize(stype), stype), *regOpnd, memOpnd1);
  aarFunc->GetCurBB()->InsertInsnBegin(insn1);
  aarFunc->GetCurBB()->InsertInsnBegin(insn);
}

void AArch64MoveRegArgs::LoadStackArgsToVReg(MIRSymbol &mirSym) const {
  PrimType stype = mirSym.GetType()->GetPrimType();
  uint32 byteSize = GetPrimTypeSize(stype);
  uint32 bitSize = byteSize * kBitsPerByte;
  MemOperand &memOpnd = aarFunc->GetOrCreateMemOpnd(mirSym, 0, bitSize);
  PregIdx pregIdx = aarFunc->GetFunction().GetPregTab()->GetPregIdxFromPregno(mirSym.GetPreg()->GetPregNo());
  RegOperand &dstRegOpnd = aarFunc->GetOrCreateVirtualRegisterOperand(
      aarFunc->GetVirtualRegNOFromPseudoRegIdx(pregIdx));
  Insn &insn = aarFunc->GetInsnBuilder()->BuildInsn(
      aarFunc->PickLdInsn(GetPrimTypeBitSize(stype), stype), dstRegOpnd, memOpnd);

  if (aarFunc->GetCG()->GenerateVerboseCG()) {
    std::string str = "param: %%";
    str += std::to_string(mirSym.GetPreg()->GetPregNo());
    ASSERT(mirSym.GetStorageClass() == kScFormal, "vreg parameters should be kScFormal type.");
    insn.SetComment(str);
  }

  aarFunc->GetCurBB()->InsertInsnBegin(insn);
}

void AArch64MoveRegArgs::MoveArgsToVReg(const CCLocInfo &ploc, MIRSymbol &mirSym) const {
  RegType regType = (ploc.reg0 < V0) ? kRegTyInt : kRegTyFloat;
  PrimType sType = mirSym.GetType()->GetPrimType();
  uint32 byteSize = GetPrimTypeSize(sType);
  uint32 srcBitSize = ((byteSize < k4ByteSize) ? k4ByteSize : byteSize) * kBitsPerByte;
  PregIdx pregIdx = aarFunc->GetFunction().GetPregTab()->GetPregIdxFromPregno(mirSym.GetPreg()->GetPregNo());
  RegOperand &dstRegOpnd =
      aarFunc->GetOrCreateVirtualRegisterOperand(aarFunc->GetVirtualRegNOFromPseudoRegIdx(pregIdx));
  dstRegOpnd.SetSize(srcBitSize);
  RegOperand &srcRegOpnd = aarFunc->GetOrCreatePhysicalRegisterOperand(
      static_cast<AArch64reg>(ploc.reg0), srcBitSize, regType);
  ASSERT(mirSym.GetStorageClass() == kScFormal, "should be args");
  MOperator mOp = aarFunc->PickMovBetweenRegs(sType, sType);
  if (mOp == MOP_vmovvv || mOp == MOP_vmovuu) {
    auto &vInsn = aarFunc->GetInsnBuilder()->BuildVectorInsn(mOp, AArch64CG::kMd[mOp]);
    (void)vInsn.AddOpndChain(dstRegOpnd).AddOpndChain(srcRegOpnd);
    auto *vecSpec1 = aarFunc->GetMemoryPool()->New<VectorRegSpec>(srcBitSize >> k3ByteSize, k8BitSize);
    auto *vecSpec2 = aarFunc->GetMemoryPool()->New<VectorRegSpec>(srcBitSize >> k3ByteSize, k8BitSize);
    (void)vInsn.PushRegSpecEntry(vecSpec1).PushRegSpecEntry(vecSpec2);
    aarFunc->GetCurBB()->InsertInsnBegin(vInsn);
    return;
  }
  Insn &insn = CreateMoveArgsToVRegInsn(mOp, dstRegOpnd, srcRegOpnd, sType);
  if (aarFunc->GetCG()->GenerateVerboseCG()) {
    std::string str = "param: %%";
    str += std::to_string(mirSym.GetPreg()->GetPregNo());
    insn.SetComment(str);
  }
  aarFunc->GetCurBB()->InsertInsnBegin(insn);
}

Insn &AArch64MoveRegArgs::CreateMoveArgsToVRegInsn(MOperator mOp, RegOperand &destOpnd, RegOperand &srcOpnd,
                                                   PrimType primType) const {
  if ((mOp == MOP_wmovrr || mOp == MOP_xmovrr) && CGOptions::CalleeEnsureParam()) {
    /* callee ensure the validbit of parameters of function */
    switch (primType) {
      case PTY_u1: {
        ImmOperand &lsbOpnd = aarFunc->CreateImmOperand(maplebe::k0BitSize, srcOpnd.GetSize(), false);
        ImmOperand &widthOpnd = aarFunc->CreateImmOperand(maplebe::k1BitSize, srcOpnd.GetSize(), false);
        bool is64Bit = (srcOpnd.GetSize() == maplebe::k64BitSize);
        return aarFunc->GetInsnBuilder()->BuildInsn(is64Bit ? MOP_xubfxrri6i6 : MOP_wubfxrri5i5, destOpnd, srcOpnd,
                                                    lsbOpnd, widthOpnd);
      }
      case PTY_u8:
      case PTY_i8:
        return aarFunc->GetInsnBuilder()->BuildInsn(MOP_xuxtb32, destOpnd, srcOpnd);
      case PTY_u16:
      case PTY_i16:
        return aarFunc->GetInsnBuilder()->BuildInsn(MOP_xuxth32, destOpnd, srcOpnd);
      case PTY_u32:
      case PTY_i32: {
        destOpnd.SetValidBitsNum(maplebe::k64BitSize);
        return aarFunc->GetInsnBuilder()->BuildInsn(MOP_xuxtw64, destOpnd, srcOpnd);
      }
      default:
        return aarFunc->GetInsnBuilder()->BuildInsn(mOp, destOpnd, srcOpnd);
    }
  } else {
    return aarFunc->GetInsnBuilder()->BuildInsn(mOp, destOpnd, srcOpnd);
  }
}

void AArch64MoveRegArgs::MoveVRegisterArgs() const {
  BB *formerCurBB = aarFunc->GetCurBB();
  aarFunc->GetDummyBB()->ClearInsns();
  aarFunc->SetCurBB(*aarFunc->GetDummyBB());
  AArch64CallConvImpl parmlocator(aarFunc->GetBecommon());
  CCLocInfo ploc;

  auto formalCount = static_cast<uint32>(aarFunc->GetFunction().GetFormalCount());
  uint32 start = 0;
  if (formalCount > 0) {
    MIRFunction *func = aarFunc->GetBecommon().GetMIRModule().CurFunction();
    if (func->IsReturnStruct() && func->IsFirstArgReturn()) {
      TyIdx tyIdx = func->GetFuncRetStructTyIdx();
      if (aarFunc->GetBecommon().GetTypeSize(tyIdx) <= k16BitSize) {
        start = 1;
      }
    }
  }
  for (uint32 i = start; i < formalCount; ++i) {
    MIRType *ty = aarFunc->GetFunction().GetNthParamType(i);
    parmlocator.LocateNextParm(*ty, ploc, i == 0, &aarFunc->GetFunction());
    MIRSymbol *sym = aarFunc->GetFunction().GetFormal(i);

    /* load locarefvar formals to store in the reflocals. */
    if (aarFunc->GetFunction().GetNthParamAttr(i).GetAttr(ATTR_localrefvar) && ploc.reg0 == kRinvalid) {
      MoveLocalRefVarToRefLocals(*sym);
    }

    if (!sym->IsPreg()) {
      continue;
    }

    if (ploc.reg0 == kRinvalid) {
      /* load stack parameters to the vreg. */
      LoadStackArgsToVReg(*sym);
    } else {
      MoveArgsToVReg(ploc, *sym);
    }
  }

  if (cgFunc->GetCG()->IsLmbc() && (cgFunc->GetSpSaveReg() != 0)) {
    /* lmbc uses vreg act as SP when alloca is present due to usage of FP for - offset */
    aarFunc->GetFirstBB()->InsertAtEnd(*aarFunc->GetDummyBB());
  } else {
    /* Java requires insertion at begining as it has fast unwind and other features */
    aarFunc->GetFirstBB()->InsertAtBeginning(*aarFunc->GetDummyBB());
  }
  aarFunc->SetCurBB(*formerCurBB);
}
}  /* namespace maplebe */
