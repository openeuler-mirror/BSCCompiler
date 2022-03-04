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
#include "x64_cgfunc.h"
#include "becommon.h"
#include "x64_call_conv.h"

namespace maplebe {
using namespace maple;
using namespace x64;

/* X86-64 Machine Interface: Data Representation */
enum X64ArgumentClass : uint8 {
  kX64NoClass,
  kX64IntegerClass,
  kX64FloatClass,
  kX64PointerClass,
  kX64VectorClass,
  kX64MemoryClass
};

/* X86-64 Function Calling Sequence */
uint32 ClassifyBasicType(const MIRType &mirType, X64ArgumentClass &classes) {
  switch (mirType.GetPrimType()) {
    case PTY_u1:
    case PTY_u8:
    case PTY_i8:
    case PTY_u16:
    case PTY_i16:
    case PTY_a32:
    case PTY_u32:
    case PTY_i32:
    case PTY_a64:
    case PTY_ptr:
    case PTY_ref:
    case PTY_u64:
    case PTY_i64:
      classes = kX64IntegerClass;
      return kOneRegister;
    case PTY_i128:
    case PTY_u128:
      classes = kX64IntegerClass;
      return kTwoRegister;
    default:
      CHECK_FATAL(false, "NYI");
  }
  return 0;
}

void X64CallConvImpl::InitCCLocInfo(CCLocInfo &pLoc) const {
  pLoc.reg0 = kRinvalid;
  pLoc.reg1 = kRinvalid;
  pLoc.reg2 = kRinvalid;
  pLoc.reg3 = kRinvalid;
  pLoc.memOffset = nextStackArgAdress;
  pLoc.fpSize = 0;
  pLoc.numFpPureRegs = 0;
}

int32 X64CallConvImpl::LocateNextParm(MIRType &mirType, CCLocInfo &pLoc, bool isFirst, MIRFunction *tFunc) {
  InitCCLocInfo(pLoc);

  uint64 typeSize = beCommon.GetTypeSize(mirType.GetTypeIndex());
  if (typeSize == 0) {
    return 0;
  }
  pLoc.memSize = static_cast<int32>(typeSize);
  ++paramNum;

  switch (mirType.GetPrimType()) {
    case PTY_u1:
    case PTY_u8:
    case PTY_i8:
    case PTY_u16:
    case PTY_i16:
    case PTY_a32:
    case PTY_u32:
    case PTY_i32:
    case PTY_ptr:
    case PTY_ref:
    case PTY_a64:
    case PTY_u64:
    case PTY_i64:
      typeSize = k8ByteSize;
      pLoc.reg0 = AllocateGPParmRegister();
      ASSERT(nextGeneralParmRegNO <= kNumIntParmRegs, "RegNo should be pramRegNO");
      break;
    case PTY_i128:
    case PTY_u128:
      typeSize = k16ByteSize;
      AllocateTwoGPParmRegisters(pLoc);
      ASSERT(nextGeneralParmRegNO <= kNumIntParmRegs, "RegNo should be pramRegNO");
      break;
    default:
      CHECK_FATAL(false, "NYI");
  }

  if (pLoc.reg0 == kRinvalid) {
    /* being passed in memory */
    nextStackArgAdress = pLoc.memOffset + typeSize;
  }
  return 0;
}

int32 X64CallConvImpl::LocateRetVal(MIRType &retType, CCLocInfo &pLoc) {
  InitCCLocInfo(pLoc);

  uint32 retSize = beCommon.GetTypeSize(retType.GetTypeIndex().GetIdx());
  if (retSize == 0) {
    return 0;    /* size 0 ret val */
  }

  if (retSize <= k16ByteSize) {
    X64ArgumentClass argClass;
    uint32 numRegs = ClassifyBasicType(retType, argClass);
    if (argClass == kX64FloatClass) {
      return 0;
    } else {
      /* If the class is INTEGER, the next available register of the sequence %rax, */
      /* %rdx is used. */
      CHECK_FATAL(numRegs <= kTwoRegister, "LocateNextParm: illegal number of regs");
      if (numRegs == kOneRegister) {
        pLoc.reg0 = AllocateGPReturnRegister();
      } else {
        AllocateTwoGPReturnRegisters(pLoc);
      }
      return 0;
    }
  } else {
    /* For return struct size > 16 bytes the pointer returns in R7(rdi). */
    pLoc.reg0 = R7;
    return kSizeOfPtr;
  }

  /*
   * Refering to X86-64 Abi, The size of each argument gets rounded
   * up to eightbytes.
   * Therefore the stack will always be eightbyte aligned.
   */
}
}  /* namespace maplebe */
