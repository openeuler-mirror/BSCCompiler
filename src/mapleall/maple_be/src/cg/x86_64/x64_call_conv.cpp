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
#include "abi.h"
#include "x64_call_conv.h"
namespace maplebe {
using namespace maple;
using namespace x64;
constexpr int kMaxRegCount = 4;

int32 ClassifyAggregate(MIRType &mirType, uint64 sizeOfTy, ArgumentClass classes[kMaxRegCount]) {
  /*
   * 1. If the size of an object is larger than four eightbytes, or it contains unaligned
   * fields, it has class MEMORY;
   * 2. for the processors that do not support the __m256 type, if the size of an object
   * is larger than two eightbytes and the first eightbyte is not SSE or any other eightbyte
   * is not SSEUP, it still has class MEMORY.
   * This in turn ensures that for rocessors that do support the __m256 type, if the size of
   * an object is four eightbytes and the first eightbyte is SSE and all other eightbytes are
   * SSEUP, it can be passed in a register.
   *(Currently, assume that m256 is not supported)
   */
  if (sizeOfTy > k2EightBytesSize) {
    classes[0] = kMemoryClass;
  } else if (sizeOfTy > k1EightBytesSize) {
    classes[0] = kIntegerClass;
    classes[1] = kIntegerClass;
  } else {
    classes[0] = kIntegerClass;
  }
  return static_cast<int32>(sizeOfTy);
}

int32 Classification(const BECommon &be, MIRType &mirType, ArgumentClass classes[kMaxRegCount]) {
  switch (mirType.GetPrimType()) {
    /*
     * Arguments of types void, (signed and unsigned) _Bool, char, short, int,
     * long, long long, and pointers are in the INTEGER class.
     */
    case PTY_void:
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
      classes[0] = kIntegerClass;
      return k8ByteSize;
    /*
     * Arguments of type __int128 offer the same operations as INTEGERs,
     * yet they do not fit into one general purpose register but require
     * two registers.
     */
    case PTY_i128:
    case PTY_u128:
      classes[0] = kIntegerClass;
      classes[1] = kIntegerClass;
      return k16ByteSize;
    case PTY_f32:
    case PTY_f64:
      classes[0] = kFloatClass;
      return k8ByteSize;
    case PTY_agg: {
      /*
       * The size of each argument gets rounded up to eightbytes,
       * Therefore the stack will always be eightbyte aligned.
       */
      uint64 sizeOfTy = RoundUp(mirType.GetSize(), k8ByteSize);
      if (sizeOfTy == 0) {
        return 0;
      }
      /* If the size of an object is larger than four eightbytes, it has class MEMORY */
      if ((sizeOfTy > k4EightBytesSize)) {
          classes[0] = kMemoryClass;
          return static_cast<int32>(sizeOfTy);
      }
      return ClassifyAggregate(mirType, sizeOfTy, classes);
    }
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
}

int32 X64CallConvImpl::LocateNextParm(MIRType &mirType, CCLocInfo &pLoc, bool isFirst, MIRFunction *tFunc) {
  InitCCLocInfo(pLoc);
  ArgumentClass classes[kMaxRegCount] = { kNoClass };  /* Max of four Regs. */
  int32 alignedTySize = Classification(beCommon, mirType, classes);
  if (alignedTySize == 0) {
    return 0;
  }
  pLoc.memSize = alignedTySize;
  ++paramNum;
  if (classes[0] == kIntegerClass) {
    if (alignedTySize == k8ByteSize) {
      pLoc.reg0 = AllocateGPParmRegister();
      ASSERT(nextGeneralParmRegNO <= kNumIntParmRegs, "RegNo should be pramRegNO");
    } else if (alignedTySize == k16ByteSize) {
      AllocateTwoGPParmRegisters(pLoc);
      ASSERT(nextGeneralParmRegNO <= kNumIntParmRegs, "RegNo should be pramRegNO");
    }
  } else if (classes[0] == kFloatClass) {
    if (alignedTySize == k8ByteSize) {
      pLoc.reg0 = AllocateSIMDFPRegister();
      ASSERT(nextGeneralParmRegNO <= kNumFloatParmRegs, "RegNo should be pramRegNO");
    } else {
      CHECK_FATAL(false, "niy");
    }
  }
  if (pLoc.reg0 == kRinvalid || classes[0] == kMemoryClass) {
    /* being passed in memory */
    nextStackArgAdress = pLoc.memOffset + alignedTySize;
  }
  return 0;
}

int32 X64CallConvImpl::LocateRetVal(MIRType &retType, CCLocInfo &pLoc) {
  InitCCLocInfo(pLoc);
  ArgumentClass classes[kMaxRegCount] = { kNoClass };  /* Max of four Regs. */
  int32 alignedTySize = Classification(beCommon, retType, classes);
  if (alignedTySize == 0) {
    return 0;    /* size 0 ret val */
  }
  if (classes[0] == kIntegerClass) {
    /* If the class is INTEGER, the next available register of the sequence %rax, */
    /* %rdx is used. */
    CHECK_FATAL(alignedTySize <= k16ByteSize, "LocateRetVal: illegal number of regs");
    if (alignedTySize == k8ByteSize) {
      pLoc.regCount = kOneRegister;
      pLoc.reg0 = AllocateGPReturnRegister();
      ASSERT(nextGeneralReturnRegNO <= kNumIntReturnRegs, "RegNo should be pramRegNO");
    } else if (alignedTySize == k16ByteSize) {
      pLoc.regCount = kTwoRegister;
      AllocateTwoGPReturnRegisters(pLoc);
      ASSERT(nextGeneralReturnRegNO <= kNumIntReturnRegs, "RegNo should be pramRegNO");
    }
    if (nextGeneralReturnRegNO == kOneRegister) {
      pLoc.primTypeOfReg0 = retType.GetPrimType() == PTY_agg ? PTY_u64 : retType.GetPrimType();
    } else if (nextGeneralReturnRegNO == kTwoRegister) {
      pLoc.primTypeOfReg0 = retType.GetPrimType() == PTY_agg ? PTY_u64 : retType.GetPrimType();
      pLoc.primTypeOfReg1 = retType.GetPrimType() == PTY_agg ? PTY_u64 : retType.GetPrimType();
    }
    return 0;
  } else if (classes[0] == kFloatClass) {
    /* If the class is SSE, the next available vector register of the sequence %xmm0, */
    /* %xmm1 is used. */
    CHECK_FATAL(alignedTySize <= k16ByteSize, "LocateRetVal: illegal number of regs");
    if (alignedTySize == k8ByteSize) {
      pLoc.regCount = 1;
      pLoc.reg0 = AllocateSIMDFPReturnRegister();
      ASSERT(nextFloatRetRegNO <= kNumFloatReturnRegs, "RegNo should be pramRegNO");
    } else if (alignedTySize == k16ByteSize) {
      CHECK_FATAL(false, "niy");
    }
    if (nextFloatRetRegNO == kOneRegister) {
      pLoc.primTypeOfReg0 = retType.GetPrimType() == PTY_agg ? PTY_f64 : retType.GetPrimType();
    } else if (nextFloatRetRegNO == kTwoRegister) {
      CHECK_FATAL(false, "niy");
    }
    return 0;
  }
  if (pLoc.reg0 == kRinvalid || classes[0] == kMemoryClass) {
    /*
     * the caller provides space for the return value and passes
     * the address of this storage in %rdi as if it were the first
     * argument to the function. In effect, this address becomes a
     * “hidden” first argument.
     * On return %rax will contain the address that has been passed
     * in by the caller in %rdi.
     * Currently, this scenario is not fully supported.
     */
    pLoc.reg0 = AllocateGPReturnRegister();
    return 0;
  }
  CHECK_FATAL(false, "NYI");
  return 0;
}
}  /* namespace maplebe */
