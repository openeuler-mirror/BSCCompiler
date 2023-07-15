/*
 * Copyright (c) [2022] Huawei Technologies Co.,Ltd.All rights reserved.
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
#include "x64_cg.h"
#include "x64_isa.h"
#include "x64_MPISel.h"

namespace maplebe {
using namespace maple;

void X64MoveRegArgs::Run() {
  MoveVRegisterArgs();
  MoveRegisterArgs();
}

void X64MoveRegArgs::CollectRegisterArgs(std::map<uint32, X64reg> &argsList,
                                         std::vector<uint32> &indexList,
                                         std::map<uint32, X64reg> &pairReg,
                                         std::vector<uint32> &numFpRegs,
                                         std::vector<uint32> &fpSize) const {
  X64CGFunc *x64CGFunc = static_cast<X64CGFunc*>(cgFunc);
  uint32 numFormal = static_cast<uint32>(x64CGFunc->GetFunction().GetFormalCount());
  numFpRegs.resize(numFormal);
  fpSize.resize(numFormal);
  X64CallConvImpl parmlocator(x64CGFunc->GetBecommon());
  CCLocInfo ploc;
  uint32 start = 0;
  if (numFormal) {
    MIRFunction *func = const_cast<MIRFunction *>(x64CGFunc->GetBecommon().GetMIRModule().CurFunction());
    if (x64CGFunc->GetBecommon().HasFuncReturnType(*func)) {
      TyIdx tyIdx = x64CGFunc->GetBecommon().GetFuncReturnType(*func);
      if (GlobalTables::GetTypeTable().GetTypeFromTyIdx(tyIdx)->GetSize() <= k16ByteSize) {
        start = 1;
      }
    }
  }
  for (uint32 i = start; i < numFormal; ++i) {
    MIRType *ty = x64CGFunc->GetFunction().GetNthParamType(i);
    parmlocator.LocateNextParm(*ty, ploc, i == 0, &x64CGFunc->GetFunction());
    if (ploc.reg0 == kRinvalid) {
      continue;
    }
    X64reg reg0 = static_cast<X64reg>(ploc.reg0);
    MIRSymbol *sym = x64CGFunc->GetFunction().GetFormal(i);
    if (sym->IsPreg()) {
      continue;
    }
    argsList[i] = reg0;
    indexList.emplace_back(i);
    if (ploc.reg1 == kRinvalid) {
      continue;
    }
    pairReg[i] = static_cast<X64reg>(ploc.reg1);
  }
}

ArgInfo X64MoveRegArgs::GetArgInfo(std::map<uint32, X64reg> &argsList,
    uint32 argIndex, std::vector<uint32> &numFpRegs, std::vector<uint32> &fpSize) const {
  X64CGFunc *x64CGFunc = static_cast<X64CGFunc*>(cgFunc);
  ArgInfo argInfo;
  argInfo.reg = argsList[argIndex];
  argInfo.mirTy = x64CGFunc->GetFunction().GetNthParamType(argIndex);
  argInfo.symSize = argInfo.mirTy->GetSize();
  argInfo.memPairSecondRegSize = 0;
  argInfo.doMemPairOpt = false;
  argInfo.createTwoStores  = false;
  argInfo.isTwoRegParm = false;
  if ((argInfo.symSize > k8ByteSize) && (argInfo.symSize <= k16ByteSize)) {
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
      argInfo.symSize = argInfo.stkSize = GetPointerSize();
    }
  } else if (argInfo.symSize > k16ByteSize) {
    /* For large struct passing, a pointer to the copy is used. */
    argInfo.symSize = argInfo.stkSize = GetPointerSize();
  } else if ((argInfo.mirTy->GetPrimType() == PTY_agg) && (argInfo.symSize < k8ByteSize)) {
    argInfo.symSize = argInfo.stkSize = k8ByteSize;
  } else {
    argInfo.stkSize = (argInfo.symSize < k4ByteSize) ? k4ByteSize : argInfo.symSize;
    if (argInfo.symSize > k4ByteSize) {
      argInfo.symSize = k8ByteSize;
    }
  }

  if (GetVecLanes(argInfo.mirTy->GetPrimType()) > 0) {
    /* vector type */
    CHECK_FATAL(false, "NIY");
  }

  argInfo.regType = (argInfo.reg < V0) ? kRegTyInt : kRegTyFloat;
  argInfo.sym = x64CGFunc->GetFunction().GetFormal(argIndex);
  CHECK_NULL_FATAL(argInfo.sym);
  argInfo.symLoc = static_cast<const X64SymbolAlloc*>(x64CGFunc->GetMemlayout()->
      GetSymAllocInfo(argInfo.sym->GetStIndex()));
  CHECK_NULL_FATAL(argInfo.symLoc);
  return argInfo;
}

void X64MoveRegArgs::GenerateMovInsn(ArgInfo &argInfo, X64reg reg2) {
  /* reg2 is required when the struct size is between 8-16 bytes */
  X64CGFunc *x64CGFunc = static_cast<X64CGFunc*>(cgFunc);
  int32 stOffset = x64CGFunc->GetBaseOffset(*argInfo.symLoc);
  RegOperand *baseOpnd = static_cast<RegOperand*>(x64CGFunc->GetBaseReg(*argInfo.symLoc));
  uint32 opndSize = argInfo.symSize * kBitsPerByte;
  RegOperand &regOpnd = x64CGFunc->GetOpndBuilder()->CreatePReg(argInfo.reg,
      opndSize, argInfo.regType);
  MemOperand *memOpnd = &x64CGFunc->GetOpndBuilder()->CreateMem(*baseOpnd, stOffset, opndSize);

  MOperator mOp = x64::MOP_begin;
  if (opndSize == k64BitSize) {
    mOp = argInfo.regType == kRegTyInt ? x64::MOP_movq_r_m : x64::MOP_movfd_r_m;
  } else if (opndSize == k32BitSize) {
    mOp = argInfo.regType == kRegTyInt ? x64::MOP_movl_r_m : x64::MOP_movfs_r_m;
  } else if (opndSize == k16BitSize) {
    mOp = argInfo.regType == kRegTyInt ? x64::MOP_movw_r_m : x64::MOP_begin;
  } else if (opndSize == k8BitSize) {
    mOp = argInfo.regType == kRegTyInt ? x64::MOP_movb_r_m : x64::MOP_begin;
  } else {
    CHECK_FATAL(false, "NIY");
  }
  CHECK_FATAL(mOp != x64::MOP_begin, "NIY");
  Insn &insn = x64CGFunc->GetInsnBuilder()->BuildInsn(mOp, X64CG::kMd[mOp]);
  insn.AddOpndChain(regOpnd).AddOpndChain(*memOpnd);
  x64CGFunc->GetCurBB()->AppendInsn(insn);
  if (reg2 != kRinvalid) {
    RegOperand &regOpnd2 = x64CGFunc->GetOpndBuilder()->CreatePReg(reg2, opndSize, argInfo.regType);
    MemOperand *memOpnd2 = &x64CGFunc->GetOpndBuilder()->CreateMem(*baseOpnd, stOffset + 8, opndSize);
    Insn &insn2 = x64CGFunc->GetInsnBuilder()->BuildInsn(mOp, X64CG::kMd[mOp]);
    insn2.AddOpndChain(regOpnd2).AddOpndChain(*memOpnd2);
    x64CGFunc->GetCurBB()->AppendInsn(insn2);
  }
}

void X64MoveRegArgs::MoveRegisterArgs() {
  X64CGFunc *x64CGFunc = static_cast<X64CGFunc*>(cgFunc);
  BB *formerCurBB = x64CGFunc->GetCurBB();
  x64CGFunc->GetDummyBB()->ClearInsns();
  x64CGFunc->SetCurBB(*x64CGFunc->GetDummyBB());

  /* <[0], maplebe::R0>; <[1], maplebe::V0> */
  std::map<uint32, X64reg> movePara;
  /* [0], [1] */
  std::vector<uint32> moveParaIndex;
  std::map<uint32, X64reg> pairReg;
  std::vector<uint32> numFpRegs;
  std::vector<uint32> fpSize;
  CollectRegisterArgs(movePara, moveParaIndex, pairReg, numFpRegs, fpSize);

  for (auto indexItem = moveParaIndex.begin(); indexItem != moveParaIndex.end(); ++indexItem) {
    uint32 index = *indexItem;
    ArgInfo argInfo = GetArgInfo(movePara, index, numFpRegs, fpSize);
    GenerateMovInsn(argInfo, pairReg[index]);
  }

  x64CGFunc->GetFirstBB()->InsertAtBeginning(*x64CGFunc->GetDummyBB());
  x64CGFunc->SetCurBB(*formerCurBB);
}

void X64MoveRegArgs::LoadStackArgsToVReg(MIRSymbol &mirSym) {
  ASSERT(mirSym.GetStorageClass() == kScFormal, "NIY, vreg parameters should be kScFormal type.");
  X64CGFunc *x64CGFunc = static_cast<X64CGFunc*>(cgFunc);
  PrimType stype = mirSym.GetType()->GetPrimType();
  uint32 opndSize = GetPrimTypeBitSize(stype);
  auto symLoc = static_cast<const X64SymbolAlloc*>(x64CGFunc->GetMemlayout()->
      GetSymAllocInfo(mirSym.GetStIndex()));
  int32 stOffset = x64CGFunc->GetBaseOffset(*symLoc);
  RegOperand *baseOpnd = static_cast<RegOperand*>(x64CGFunc->GetBaseReg(*symLoc));
  MemOperand &memOpnd = x64CGFunc->GetOpndBuilder()->CreateMem(*baseOpnd, stOffset, opndSize);
  PregIdx pregIdx = x64CGFunc->GetFunction().GetPregTab()->GetPregIdxFromPregno(mirSym.GetPreg()->GetPregNo());
  RegOperand &dstRegOpnd = x64CGFunc->GetOpndBuilder()->CreateVReg(
      x64CGFunc->GetVirtualRegNOFromPseudoRegIdx(pregIdx), opndSize, cgFunc->GetRegTyFromPrimTy(stype));

  MOperator mOp;
  if (opndSize == k64BitSize) {
    mOp = x64::MOP_movq_m_r;
  } else if (opndSize == k32BitSize) {
    mOp = x64::MOP_movl_m_r;
  } else if (opndSize == k16BitSize) {
    mOp = x64::MOP_movw_m_r;
  } else if (opndSize == k8BitSize) {
    mOp = x64::MOP_movb_m_r;
  } else {
    CHECK_FATAL(false, "NIY");
  }
  Insn &insn = x64CGFunc->GetInsnBuilder()->BuildInsn(mOp, X64CG::kMd[mOp]);
  insn.AddOpndChain(memOpnd).AddOpndChain(dstRegOpnd);
  if (x64CGFunc->GetCG()->GenerateVerboseCG()) {
    std::string key = "param: %%";
    key += std::to_string(mirSym.GetPreg()->GetPregNo());
    insn.SetComment(key);
  }
  x64CGFunc->GetCurBB()->InsertInsnBegin(insn);
}

void X64MoveRegArgs::MoveArgsToVReg(const CCLocInfo &ploc, MIRSymbol &mirSym) {
  ASSERT(mirSym.GetStorageClass() == kScFormal, "NIY, vreg parameters should be kScFormal type.");
  X64CGFunc *x64CGFunc = static_cast<X64CGFunc*>(cgFunc);
  RegType regType = (ploc.reg0 < V0) ? kRegTyInt : kRegTyFloat;
  PrimType stype = mirSym.GetType()->GetPrimType();
  uint32 byteSize = GetPrimTypeSize(stype);
  uint32 srcBitSize = ((byteSize < k4ByteSize) ? k4ByteSize : byteSize) * kBitsPerByte;
  PregIdx pregIdx = x64CGFunc->GetFunction().GetPregTab()->GetPregIdxFromPregno(mirSym.GetPreg()->GetPregNo());
  RegOperand &dstRegOpnd = x64CGFunc->GetOpndBuilder()->CreateVReg(
      x64CGFunc->GetVirtualRegNOFromPseudoRegIdx(pregIdx), srcBitSize, regType);
  RegOperand &srcRegOpnd = x64CGFunc->GetOpndBuilder()->CreateVReg(ploc.reg0, srcBitSize, regType);

  MOperator mOp;
  if (srcBitSize == k64BitSize) {
    mOp = x64::MOP_movq_r_r;
  } else if (srcBitSize == k32BitSize) {
    mOp = x64::MOP_movl_r_r;
  } else if (srcBitSize == k16BitSize) {
    mOp = x64::MOP_movw_r_r;
  } else if (srcBitSize == k8BitSize) {
    mOp = x64::MOP_movb_r_r;
  } else {
    CHECK_FATAL(false, "NIY");
  }
  Insn &insn = x64CGFunc->GetInsnBuilder()->BuildInsn(mOp, X64CG::kMd[mOp]);
  insn.AddOpndChain(srcRegOpnd).AddOpndChain(dstRegOpnd);
  if (x64CGFunc->GetCG()->GenerateVerboseCG()) {
    std::string key = "param: %%";
    key += std::to_string(mirSym.GetPreg()->GetPregNo());
    insn.SetComment(key);
  }
  x64CGFunc->GetCurBB()->InsertInsnBegin(insn);
}

void X64MoveRegArgs::MoveVRegisterArgs() {
  X64CGFunc *x64CGFunc = static_cast<X64CGFunc*>(cgFunc);
  BB *formerCurBB = x64CGFunc->GetCurBB();
  x64CGFunc->GetDummyBB()->ClearInsns();
  x64CGFunc->SetCurBB(*x64CGFunc->GetDummyBB());
  X64CallConvImpl parmlocator(x64CGFunc->GetBecommon());
  CCLocInfo ploc;

  uint32 formalCount = static_cast<uint32>(x64CGFunc->GetFunction().GetFormalCount());
  uint32 start = 0;
  if (formalCount) {
    MIRFunction *func = const_cast<MIRFunction*>(x64CGFunc->GetBecommon().GetMIRModule().CurFunction());
    if (x64CGFunc->GetBecommon().HasFuncReturnType(*func)) {
      TyIdx idx = x64CGFunc->GetBecommon().GetFuncReturnType(*func);
      if (GlobalTables::GetTypeTable().GetTypeFromTyIdx(idx)->GetSize() <= k16BitSize) {
        start = 1;
      }
    }
  }
  for (uint32 i = start; i < formalCount; ++i) {
    MIRType *ty = x64CGFunc->GetFunction().GetNthParamType(i);
    parmlocator.LocateNextParm(*ty, ploc, i == 0, &x64CGFunc->GetFunction());
    MIRSymbol *sym = x64CGFunc->GetFunction().GetFormal(i);

    /* load locarefvar formals to store in the reflocals. */
    if (x64CGFunc->GetFunction().GetNthParamAttr(i).GetAttr(ATTR_localrefvar) && ploc.reg0 == kRinvalid) {
      CHECK_FATAL(false, "NIY");
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

  x64CGFunc->GetFirstBB()->InsertAtBeginning(*x64CGFunc->GetDummyBB());
  x64CGFunc->SetCurBB(*formerCurBB);
}
}  /* namespace maplebe */
