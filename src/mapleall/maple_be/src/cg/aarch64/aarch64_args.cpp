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
  AArch64CGFunc *aarchCGFunc = static_cast<AArch64CGFunc*>(cgFunc);
  uint32 numFormal = static_cast<uint32>(aarchCGFunc->GetFunction().GetFormalCount());
  numFpRegs.resize(numFormal);
  fpSize.resize(numFormal);
  ParmLocator parmlocator(aarchCGFunc->GetBecommon());
  PLocInfo ploc;
  uint32 start = 0;
  if (numFormal) {
    MIRFunction *func = const_cast<MIRFunction *>(aarchCGFunc->GetBecommon().GetMIRModule().CurFunction());
    if (aarchCGFunc->GetBecommon().HasFuncReturnType(*func)) {
      TyIdx tyIdx = aarchCGFunc->GetBecommon().GetFuncReturnType(*func);
      if (aarchCGFunc->GetBecommon().GetTypeSize(tyIdx) <= k16ByteSize) {
        start = 1;
      }
    }
  }
  for (uint32 i = start; i < numFormal; ++i) {
    MIRType *ty = aarchCGFunc->GetFunction().GetNthParamType(i);
    parmlocator.LocateNextParm(*ty, ploc, i == 0);
    if (ploc.reg0 == kRinvalid) {
      continue;
    }
    aarchCGFunc->PushElemIntoFormalRegList(ploc.reg0);
    MIRSymbol *sym = aarchCGFunc->GetFunction().GetFormal(i);
    if (sym->IsPreg()) {
      continue;
    }
    argsList[i] = ploc.reg0;
    indexList.emplace_back(i);
    if (ploc.reg1 == kRinvalid) {
      continue;
    }
    if (ploc.numFpPureRegs) {
      uint32 index = static_cast<uint32>(indexList.size()) - 1;
      numFpRegs[index] = ploc.numFpPureRegs;
      fpSize[index] = ploc.fpSize;
      continue;
    }
    aarchCGFunc->PushElemIntoFormalRegList(ploc.reg1);
    pairReg[i] = ploc.reg1;
    if (ploc.reg2 == kRinvalid) {
      continue;
    }
    aarchCGFunc->PushElemIntoFormalRegList(ploc.reg2);
    if (ploc.reg3 == kRinvalid) {
      continue;
    }
    aarchCGFunc->PushElemIntoFormalRegList(ploc.reg3);
  }
}

ArgInfo AArch64MoveRegArgs::GetArgInfo(std::map<uint32, AArch64reg> &argsList, std::vector<uint32> &numFpRegs,
                                       std::vector<uint32> &fpSize, uint32 argIndex) const {
  AArch64CGFunc *aarchCGFunc = static_cast<AArch64CGFunc*>(cgFunc);
  ArgInfo argInfo;
  argInfo.reg = argsList[argIndex];
  argInfo.mirTy = aarchCGFunc->GetFunction().GetNthParamType(argIndex);
  argInfo.symSize = aarchCGFunc->GetBecommon().GetTypeSize(argInfo.mirTy->GetTypeIndex());
  argInfo.memPairSecondRegSize = 0;
  argInfo.doMemPairOpt = false;
  argInfo.CreateTwoStores  = false;
  if ((argInfo.symSize > k8ByteSize) && (argInfo.symSize <= k16ByteSize)) {
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
      argInfo.symSize = argInfo.stkSize = kSizeOfPtr;
    }
  } else if (argInfo.symSize > k16ByteSize) {
    /* For large struct passing, a pointer to the copy is used. */
    argInfo.symSize = argInfo.stkSize = kSizeOfPtr;
  } else if ((argInfo.mirTy->GetPrimType() == PTY_agg) && (argInfo.symSize < k4ByteSize)) {
    /* For small aggregate parameter, set to minimum of 4 bytes. */
    argInfo.symSize = argInfo.stkSize = k4ByteSize;
  } else if (numFpRegs[argIndex] > kOneRegister) {
    argInfo.symSize = argInfo.stkSize = fpSize[argIndex];
  } else {
    argInfo.stkSize = (argInfo.symSize < k4ByteSize) ? k4ByteSize : argInfo.symSize;
    if (argInfo.symSize > k4ByteSize) {
      argInfo.symSize = k8ByteSize;
    }
  }
  argInfo.regType = (argInfo.reg < V0) ? kRegTyInt : kRegTyFloat;
  argInfo.sym = aarchCGFunc->GetFunction().GetFormal(argIndex);
  CHECK_NULL_FATAL(argInfo.sym);
  argInfo.symLoc =
      static_cast<const AArch64SymbolAlloc*>(aarchCGFunc->GetMemlayout()->GetSymAllocInfo(argInfo.sym->GetStIndex()));
  CHECK_NULL_FATAL(argInfo.symLoc);
  if (argInfo.doMemPairOpt && (aarchCGFunc->GetBaseOffset(*(argInfo.symLoc)) & 0x7)) {
    /* Do not optimize for struct reg pair for unaligned access.
     * However, this symbol requires two parameter registers, separate stores must be generated.
     */
    argInfo.symSize = kSizeOfPtr;
    argInfo.doMemPairOpt = false;
    argInfo.CreateTwoStores = true;
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
  AArch64CGFunc *aarchCGFunc = static_cast<AArch64CGFunc*>(cgFunc);
  AArch64RegOperand *baseOpnd = static_cast<AArch64RegOperand*>(aarchCGFunc->GetBaseReg(*firstArgInfo.symLoc));
  RegOperand &regOpnd = aarchCGFunc->GetOrCreatePhysicalRegisterOperand(firstArgInfo.reg,
                                                                        firstArgInfo.stkSize * kBitsPerByte,
                                                                        firstArgInfo.regType);
  MOperator mOp = firstArgInfo.regType == kRegTyInt ? ((firstArgInfo.stkSize > k4ByteSize) ? MOP_xstp : MOP_wstp)
                                                    : ((firstArgInfo.stkSize > k4ByteSize) ? MOP_dstp : MOP_sstp);
  RegOperand &regOpnd2 = aarchCGFunc->GetOrCreatePhysicalRegisterOperand(secondArgInfo.reg,
                                                                         firstArgInfo.stkSize * kBitsPerByte,
                                                                         firstArgInfo.regType);

  int32 limit = (secondArgInfo.stkSize > k4ByteSize) ? kStpLdpImm64UpperBound : kStpLdpImm32UpperBound;
  int32 stOffset = aarchCGFunc->GetBaseOffset(*firstArgInfo.symLoc);
  MemOperand *memOpnd = nullptr;
  if (stOffset > limit || baseReg != nullptr) {
    if (baseReg == nullptr || lastSegment != firstArgInfo.symLoc->GetMemSegment()) {
      AArch64ImmOperand &immOpnd =
          aarchCGFunc->CreateImmOperand(stOffset - firstArgInfo.symLoc->GetOffset(), k64BitSize, false);
      baseReg = &aarchCGFunc->CreateRegisterOperandOfType(kRegTyInt, k8ByteSize);
      lastSegment = firstArgInfo.symLoc->GetMemSegment();
      aarchCGFunc->SelectAdd(*baseReg, *baseOpnd, immOpnd, PTY_a64);
    }
    AArch64OfstOperand &offsetOpnd = aarchCGFunc->CreateOfstOpnd(firstArgInfo.symLoc->GetOffset(), k32BitSize);
    if (firstArgInfo.symLoc->GetMemSegment()->GetMemSegmentKind() == kMsArgsStkPassed) {
      offsetOpnd.SetVary(kUnAdjustVary);
    }
    memOpnd = aarchCGFunc->GetMemoryPool()->New<AArch64MemOperand>(AArch64MemOperand::kAddrModeBOi,
                                                                   firstArgInfo.stkSize * kBitsPerByte,
                                                                   *baseReg, nullptr, &offsetOpnd, firstArgInfo.sym);
  } else {
    AArch64OfstOperand &offsetOpnd = aarchCGFunc->CreateOfstOpnd(stOffset, k32BitSize);
    if (firstArgInfo.symLoc->GetMemSegment()->GetMemSegmentKind() == kMsArgsStkPassed) {
      offsetOpnd.SetVary(kUnAdjustVary);
    }
    memOpnd = aarchCGFunc->GetMemoryPool()->New<AArch64MemOperand>(AArch64MemOperand::kAddrModeBOi,
                                                                   firstArgInfo.stkSize * kBitsPerByte,
                                                                   *baseOpnd, nullptr, &offsetOpnd, firstArgInfo.sym);
  }
  Insn &pushInsn = aarchCGFunc->GetCG()->BuildInstruction<AArch64Insn>(mOp, regOpnd, regOpnd2, *memOpnd);
  if (aarchCGFunc->GetCG()->GenerateVerboseCG()) {
    std::string argName = firstArgInfo.sym->GetName() + " " + secondArgInfo.sym->GetName();
    pushInsn.SetComment(std::string("store param: ").append(argName));
  }
  aarchCGFunc->GetCurBB()->AppendInsn(pushInsn);
}

void AArch64MoveRegArgs::GenOneInsn(ArgInfo &argInfo, AArch64RegOperand &baseOpnd, uint32 stBitSize, AArch64reg dest,
                                    int32 offset) {
  AArch64CGFunc *aarchCGFunc = static_cast<AArch64CGFunc*>(cgFunc);
  MOperator mOp = aarchCGFunc->PickStInsn(stBitSize, argInfo.mirTy->GetPrimType());
  RegOperand &regOpnd = aarchCGFunc->GetOrCreatePhysicalRegisterOperand(dest, stBitSize, argInfo.regType);

  AArch64OfstOperand &offsetOpnd = aarchCGFunc->CreateOfstOpnd(offset, k32BitSize);
  if (argInfo.symLoc->GetMemSegment()->GetMemSegmentKind() == kMsArgsStkPassed) {
    offsetOpnd.SetVary(kUnAdjustVary);
  }
  MemOperand *memOpnd = aarchCGFunc->GetMemoryPool()->New<AArch64MemOperand>(AArch64MemOperand::kAddrModeBOi,
                         stBitSize, baseOpnd, nullptr, &offsetOpnd, argInfo.sym);
  Insn &insn = aarchCGFunc->GetCG()->BuildInstruction<AArch64Insn>(mOp, regOpnd, *memOpnd);
  if (aarchCGFunc->GetCG()->GenerateVerboseCG()) {
    insn.SetComment(std::string("store param: ").append(argInfo.sym->GetName()));
  }
  aarchCGFunc->GetCurBB()->AppendInsn(insn);
}

void AArch64MoveRegArgs::GenerateStrInsn(ArgInfo &argInfo, AArch64reg reg2, uint32 numFpRegs, uint32 fpSize) {
  AArch64CGFunc *aarchCGFunc = static_cast<AArch64CGFunc*>(cgFunc);
  int32 stOffset = aarchCGFunc->GetBaseOffset(*argInfo.symLoc);
  AArch64RegOperand *baseOpnd = static_cast<AArch64RegOperand*>(aarchCGFunc->GetBaseReg(*argInfo.symLoc));
  RegOperand &regOpnd =
      aarchCGFunc->GetOrCreatePhysicalRegisterOperand(argInfo.reg, argInfo.stkSize * kBitsPerByte, argInfo.regType);
  MemOperand *memOpnd = nullptr;
  if (AArch64MemOperand::IsPIMMOffsetOutOfRange(stOffset, argInfo.symSize * kBitsPerByte) ||
      (baseReg != nullptr && (lastSegment == argInfo.symLoc->GetMemSegment()))) {
    if (baseReg == nullptr || lastSegment != argInfo.symLoc->GetMemSegment()) {
      AArch64ImmOperand &immOpnd = aarchCGFunc->CreateImmOperand(stOffset - argInfo.symLoc->GetOffset(), k64BitSize,
                                                                 false);
      baseReg = &aarchCGFunc->CreateRegisterOperandOfType(kRegTyInt, k8ByteSize);
      lastSegment = argInfo.symLoc->GetMemSegment();
      aarchCGFunc->SelectAdd(*baseReg, *baseOpnd, immOpnd, PTY_a64);
    }
    AArch64OfstOperand &offsetOpnd = aarchCGFunc->CreateOfstOpnd(argInfo.symLoc->GetOffset(), k32BitSize);
    if (argInfo.symLoc->GetMemSegment()->GetMemSegmentKind() == kMsArgsStkPassed) {
      offsetOpnd.SetVary(kUnAdjustVary);
    }
    memOpnd = aarchCGFunc->GetMemoryPool()->New<AArch64MemOperand>(AArch64MemOperand::kAddrModeBOi,
                                                                   argInfo.symSize * kBitsPerByte, *baseReg,
                                                                   nullptr, &offsetOpnd, argInfo.sym);
  } else {
    AArch64OfstOperand &offsetOpnd = aarchCGFunc->CreateOfstOpnd(stOffset, k32BitSize);
    if (argInfo.symLoc->GetMemSegment()->GetMemSegmentKind() == kMsArgsStkPassed) {
      offsetOpnd.SetVary(kUnAdjustVary);
    }
    memOpnd = aarchCGFunc->GetMemoryPool()->New<AArch64MemOperand>(AArch64MemOperand::kAddrModeBOi,
                                                                   argInfo.symSize * kBitsPerByte, *baseOpnd,
                                                                   nullptr, &offsetOpnd, argInfo.sym);
  }

  MOperator mOp = aarchCGFunc->PickStInsn(argInfo.symSize * kBitsPerByte, argInfo.mirTy->GetPrimType());
  Insn &insn = aarchCGFunc->GetCG()->BuildInstruction<AArch64Insn>(mOp, regOpnd, *memOpnd);
  if (aarchCGFunc->GetCG()->GenerateVerboseCG()) {
    insn.SetComment(std::string("store param: ").append(argInfo.sym->GetName()));
  }
  aarchCGFunc->GetCurBB()->AppendInsn(insn);

  if (argInfo.CreateTwoStores || argInfo.doMemPairOpt) {
    /* second half of the struct passing by registers. */
    uint32 part2BitSize = argInfo.memPairSecondRegSize * kBitsPerByte;
    GenOneInsn(argInfo, *baseOpnd, part2BitSize, reg2, (stOffset + kSizeOfPtr));
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
  AArch64CGFunc *aarchCGFunc = static_cast<AArch64CGFunc*>(cgFunc);
  BB *formerCurBB = aarchCGFunc->GetCurBB();
  aarchCGFunc->GetDummyBB()->ClearInsns();
  aarchCGFunc->SetCurBB(*aarchCGFunc->GetDummyBB());

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
          static_cast<AArch64SymbolAlloc *>(aarchCGFunc->GetMemlayout()->GetSymAllocInfo(
              secondArgInfo.sym->GetStIndex()));
      /* Make sure they are in same segment if want to use stp */
      if (IsInSameSegment(firstArgInfo, secondArgInfo)) {
        GenerateStpInsn(firstArgInfo, secondArgInfo);
        it = next;
        continue;
      }
    }
    GenerateStrInsn(firstArgInfo, pairReg[firstIndex], numFpRegs[firstIndex], fpSize[firstIndex]);
  }

  aarchCGFunc->GetFirstBB()->InsertAtBeginning(*aarchCGFunc->GetDummyBB());
  aarchCGFunc->SetCurBB(*formerCurBB);
}

void AArch64MoveRegArgs::MoveLocalRefVarToRefLocals(MIRSymbol &mirSym) {
  AArch64CGFunc *aarchCGFunc = static_cast<AArch64CGFunc*>(cgFunc);
  PrimType stype = mirSym.GetType()->GetPrimType();
  uint32 byteSize = GetPrimTypeSize(stype);
  uint32 bitSize = byteSize * kBitsPerByte;
  MemOperand &memOpnd = aarchCGFunc->GetOrCreateMemOpnd(mirSym, 0, bitSize, true);
  RegOperand *regOpnd = nullptr;
  if (mirSym.IsPreg()) {
    PregIdx pregIdx = aarchCGFunc->GetFunction().GetPregTab()->GetPregIdxFromPregno(mirSym.GetPreg()->GetPregNo());
    regOpnd = &aarchCGFunc->GetOrCreateVirtualRegisterOperand(aarchCGFunc->GetVirtualRegNOFromPseudoRegIdx(pregIdx));
  } else {
    regOpnd = &aarchCGFunc->GetOrCreateVirtualRegisterOperand(aarchCGFunc->NewVReg(kRegTyInt, k8ByteSize));
  }
  Insn &insn = aarchCGFunc->GetCG()->BuildInstruction<AArch64Insn>(
      aarchCGFunc->PickLdInsn(GetPrimTypeBitSize(stype), stype), *regOpnd, memOpnd);
  MemOperand &memOpnd1 = aarchCGFunc->GetOrCreateMemOpnd(mirSym, 0, bitSize, false);
  Insn &insn1 = aarchCGFunc->GetCG()->BuildInstruction<AArch64Insn>(
      aarchCGFunc->PickStInsn(GetPrimTypeBitSize(stype), stype), *regOpnd, memOpnd1);
  aarchCGFunc->GetCurBB()->InsertInsnBegin(insn1);
  aarchCGFunc->GetCurBB()->InsertInsnBegin(insn);
}


void AArch64MoveRegArgs::LoadStackArgsToVReg(MIRSymbol &mirSym) {
  AArch64CGFunc *aarchCGFunc = static_cast<AArch64CGFunc*>(cgFunc);
  PrimType stype = mirSym.GetType()->GetPrimType();
  uint32 byteSize = GetPrimTypeSize(stype);
  uint32 bitSize = byteSize * kBitsPerByte;
  MemOperand &memOpnd = aarchCGFunc->GetOrCreateMemOpnd(mirSym, 0, bitSize);
  PregIdx pregIdx = aarchCGFunc->GetFunction().GetPregTab()->GetPregIdxFromPregno(mirSym.GetPreg()->GetPregNo());
  RegOperand &dstRegOpnd = aarchCGFunc->GetOrCreateVirtualRegisterOperand(
      aarchCGFunc->GetVirtualRegNOFromPseudoRegIdx(pregIdx));
  Insn &insn = aarchCGFunc->GetCG()->BuildInstruction<AArch64Insn>(
      aarchCGFunc->PickLdInsn(GetPrimTypeBitSize(stype), stype), dstRegOpnd, memOpnd);

  if (aarchCGFunc->GetCG()->GenerateVerboseCG()) {
    std::string key = "param: %%";
    key += std::to_string(mirSym.GetPreg()->GetPregNo());
    ASSERT(mirSym.GetStorageClass() == kScFormal, "vreg parameters should be kScFormal type.");
    insn.SetComment(key);
  }

  aarchCGFunc->GetCurBB()->InsertInsnBegin(insn);
}

void AArch64MoveRegArgs::MoveArgsToVReg(const PLocInfo &ploc, MIRSymbol &mirSym) {
  AArch64CGFunc *aarchCGFunc = static_cast<AArch64CGFunc*>(cgFunc);
  RegType regType = (ploc.reg0 < V0) ? kRegTyInt : kRegTyFloat;
  PrimType stype = mirSym.GetType()->GetPrimType();
  uint32 byteSize = GetPrimTypeSize(stype);
  uint32 srcBitSize = ((byteSize < k4ByteSize) ? k4ByteSize : byteSize) * kBitsPerByte;
  PregIdx pregIdx = aarchCGFunc->GetFunction().GetPregTab()->GetPregIdxFromPregno(mirSym.GetPreg()->GetPregNo());
  RegOperand &dstRegOpnd =
      aarchCGFunc->GetOrCreateVirtualRegisterOperand(aarchCGFunc->GetVirtualRegNOFromPseudoRegIdx(pregIdx));
  dstRegOpnd.SetSize(srcBitSize);
  RegOperand &srcRegOpnd = aarchCGFunc->GetOrCreatePhysicalRegisterOperand(ploc.reg0, srcBitSize, regType);
  ASSERT(mirSym.GetStorageClass() == kScFormal, "should be args");
  MOperator mOp = aarchCGFunc->PickMovInsn(srcBitSize, regType);

  Insn &insn = aarchCGFunc->GetCG()->BuildInstruction<AArch64Insn>(mOp, dstRegOpnd, srcRegOpnd);
  if (aarchCGFunc->GetCG()->GenerateVerboseCG()) {
    std::string key = "param: %%";
    key += std::to_string(mirSym.GetPreg()->GetPregNo());
    insn.SetComment(key);
  }
  aarchCGFunc->GetCurBB()->InsertInsnBegin(insn);
}

void AArch64MoveRegArgs::MoveVRegisterArgs() {
  AArch64CGFunc *aarchCGFunc = static_cast<AArch64CGFunc*>(cgFunc);
  BB *formerCurBB = aarchCGFunc->GetCurBB();
  aarchCGFunc->GetDummyBB()->ClearInsns();
  aarchCGFunc->SetCurBB(*aarchCGFunc->GetDummyBB());
  ParmLocator parmlocator(aarchCGFunc->GetBecommon());
  PLocInfo ploc;

  uint32 formalCount = static_cast<uint32>(aarchCGFunc->GetFunction().GetFormalCount());
  uint32 start = 0;
  if (formalCount) {
    MIRFunction *func = const_cast<MIRFunction*>(aarchCGFunc->GetBecommon().GetMIRModule().CurFunction());
    if (aarchCGFunc->GetBecommon().HasFuncReturnType(*func)) {
      TyIdx idx = aarchCGFunc->GetBecommon().GetFuncReturnType(*func);
      if (aarchCGFunc->GetBecommon().GetTypeSize(idx) <= k16BitSize) {
        start = 1;
      }
    }
  }
  for (uint32 i = start; i < formalCount; ++i) {
    MIRType *ty = aarchCGFunc->GetFunction().GetNthParamType(i);
    parmlocator.LocateNextParm(*ty, ploc, i == 0);
    MIRSymbol *sym = aarchCGFunc->GetFunction().GetFormal(i);

    /* load locarefvar formals to store in the reflocals. */
    if (aarchCGFunc->GetFunction().GetNthParamAttr(i).GetAttr(ATTR_localrefvar) && ploc.reg0 == kRinvalid) {
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

  aarchCGFunc->GetFirstBB()->InsertAtBeginning(*aarchCGFunc->GetDummyBB());
  aarchCGFunc->SetCurBB(*formerCurBB);
}
}  /* namespace maplebe */
