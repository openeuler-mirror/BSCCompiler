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
#include "aarch64_cgfunc.h"
#include "becommon.h"
#include "aarch64_call_conv.h"

namespace maplebe {
using namespace maple;

// external interface to look for pure float struct
uint32 AArch64CallConvImpl::FloatParamRegRequired(MIRStructType &structType, uint32 &fpSize) const {
  PrimType baseType = PTY_begin;
  size_t elemNum = 0;
  if (!IsHomogeneousAggregates(structType, baseType, elemNum)) {
    return 0;
  }
  fpSize = GetPrimTypeSize(baseType);
  return static_cast<uint32>(elemNum);
}

static void AllocateHomogeneousAggregatesRegister(CCLocInfo &ploc, const AArch64reg *regList,
                                                  uint32 maxRegNum, PrimType baseType,
                                                  uint32 allocNum, uint32 begin = 0) {
  CHECK_FATAL(allocNum + begin - 1 < maxRegNum, "NIY, out of range.");
  if (allocNum >= kOneRegister) {
    ploc.reg0 = regList[begin++];
    ploc.primTypeOfReg0 = baseType;
  }
  if (allocNum >= kTwoRegister) {
    ploc.reg1 = regList[begin++];
    ploc.primTypeOfReg1 = baseType;
  }
  if (allocNum >= kThreeRegister) {
    ploc.reg2 = regList[begin++];
    ploc.primTypeOfReg2 = baseType;
  }
  if (allocNum >= kFourRegister) {
    ploc.reg3 = regList[begin++];
    ploc.primTypeOfReg3 = baseType;
  }
  ploc.regCount = allocNum;
}

void AArch64CallConvImpl::InitCCLocInfo(CCLocInfo &ploc) const {
  ploc.Clear();
  ploc.memOffset = nextStackArgAdress;
}

// instantiated with the type of the function return value, it describes how
// the return value is to be passed back to the caller
//
// Refer to Procedure Call Standard for the Arm 64-bit
// Architecture (AArch64) 2022Q3.  $6.9
//  "If the type, T, of the result of a function is such that
//     void func(T arg)
//   would require that arg be passed as a value in a register (or set of registers)
//   according to the rules in Parameter passing, then the result is returned in the
//   same registers as would be used for such an argument."
void AArch64CallConvImpl::LocateRetVal(const MIRType &retType, CCLocInfo &ploc) const {
  InitCCLocInfo(ploc);
  size_t retSize = retType.GetSize();
  if (retSize == 0) {
    return;    // size 0 ret val
  }

  PrimType primType = retType.GetPrimType();
  if (IsPrimitiveFloat(primType) || IsPrimitiveVector(primType)) {
    // float or vector, return in v0
    ploc.reg0 = AArch64Abi::kFloatReturnRegs[0];
    ploc.primTypeOfReg0 = primType;
    ploc.regCount = 1;
    return;
  }
  if (IsPrimitiveInteger(primType) && GetPrimTypeBitSize(primType) <= k64BitSize) {
    // interger and size <= 64-bit, return in x0
    ploc.reg0 = AArch64Abi::kIntReturnRegs[0];
    ploc.primTypeOfReg0 = primType;
    ploc.regCount = 1;
    return;
  }
  PrimType baseType = PTY_begin;
  size_t elemNum = 0;
  if (IsHomogeneousAggregates(retType, baseType, elemNum)) {
    // homogeneous aggregates, return in v0-v3
    AllocateHomogeneousAggregatesRegister(ploc, AArch64Abi::kFloatReturnRegs,
        AArch64Abi::kNumFloatParmRegs, baseType, static_cast<uint32>(elemNum));
    return;
  }
  if (retSize <= k16ByteSize) {
    // agg size <= 16-byte or int128, return in x0-x1
    ploc.reg0 = AArch64Abi::kIntReturnRegs[0];
    ploc.primTypeOfReg0 = PTY_u64;
    if (retSize > k8ByteSize) {
      ploc.reg1 = AArch64Abi::kIntReturnRegs[1];
      ploc.primTypeOfReg1 = PTY_u64;
    }
    ploc.regCount = retSize <= k8ByteSize ? kOneRegister : kTwoRegister;
    return;
  }
}

uint64 AArch64CallConvImpl::AllocateRegisterForAgg(const MIRType &mirType, CCLocInfo &ploc,
                                                   uint64 size, uint64 align) {
  uint64 aggCopySize = 0;
  PrimType baseType = PTY_begin;
  size_t elemNum = 0;
  if (IsHomogeneousAggregates(mirType, baseType, elemNum)) {
    if ((nextFloatRegNO + elemNum - 1) < AArch64Abi::kNumFloatParmRegs) {
      // C.2  If the argument is an HFA or an HVA and there are sufficient unallocated SIMD and
      //      Floating-point registers (NSRN + number of members <= 8), then the argument is
      //      allocated to SIMD and Floating-point registers (with one register per member of
      //      the HFA or HVA). The NSRN is incremented by the number of registers used.
      //      The argument has now been allocated
      AllocateHomogeneousAggregatesRegister(ploc, AArch64Abi::kFloatReturnRegs,
          AArch64Abi::kNumFloatParmRegs, baseType, elemNum, nextFloatRegNO);
      nextFloatRegNO += elemNum;
    } else {
      // C.3  If the argument is an HFA or an HVA then the NSRN is set to 8 and the size of the
      //      argument is rounded up to the nearest multiple of 8 bytes.
      nextFloatRegNO = AArch64Abi::kNumFloatParmRegs;
      ploc.reg0 = kRinvalid;
    }
  } else if (size <= k16ByteSize) {
    // small struct, passed by general purpose register
    AllocateGPRegister(mirType, ploc, size, align);
  } else {
    // large struct, a pointer to the copy is used
    ploc.reg0 = AllocateGPRegister();
    ploc.primTypeOfReg0 = PTY_a64;
    ploc.memSize = k8ByteSize;
    aggCopySize = RoundUp(size, k8ByteSize);
  }
  return aggCopySize;
}

// allocate general purpose register
void AArch64CallConvImpl::AllocateGPRegister(const MIRType &mirType, CCLocInfo &ploc,
                                             uint64 size, uint64 align) {
  if (IsPrimitiveInteger(mirType.GetPrimType()) && size <= k8ByteSize) {
    // C.9  If the argument is an Integral or Pointer Type, the size of the argument is less
    //      than or equal to 8 bytes and the NGRN is less than 8, the argument is copied to
    //      the least significant bits in x[NGRN]. The NGRN is incremented by one.
    //      The argument has now been allocated.
    ploc.reg0 = AllocateGPRegister();
    ploc.primTypeOfReg0 = mirType.GetPrimType();
    return;
  }
  if (align == k16ByteSize) {
    // C.10  If the argument has an alignment of 16 then the NGRN is rounded up to the next
    //       even number.
    nextGeneralRegNO = (nextGeneralRegNO + 1U) & ~1U;
  }
  if (mirType.GetPrimType() == PTY_i128 || mirType.GetPrimType() == PTY_u128) {
    // C.11  If the argument is an Integral Type, the size of the argument is equal to 16
    //       and the NGRN is less than 7, the argument is copied to x[NGRN] and x[NGRN+1].
    //       x[NGRN] shall contain the lower addressed double-word of the memory
    //       representation of the argument. The NGRN is incremented by two.
    //       The argument has now been allocated.
    if (nextGeneralRegNO < AArch64Abi::kNumIntParmRegs - 1) {
      ASSERT(size == k16ByteSize, "NIY, size must be 16-byte.");
      ploc.reg0 = AllocateGPRegister();
      ploc.primTypeOfReg0 = PTY_u64;
      ploc.reg1 = AllocateGPRegister();
      ploc.primTypeOfReg1 = PTY_u64;
      return;
    }
  } else if (size <= k16ByteSize) {
    // C.12  If the argument is a Composite Type and the size in double-words of the argument
    //       is not more than 8 minus NGRN, then the argument is copied into consecutive
    //       general-purpose registers, starting at x[NGRN]. The argument is passed as though
    //       it had been loaded into the registers from a double-word-aligned address with
    //       an appropriate sequence of LDR instructions loading consecutive registers from
    //       memory (the contents of any unused parts of the registers are unspecified by this
    //       standard). The NGRN is incremented by the number of registers used.
    //       The argument has now been allocated.
    ASSERT(mirType.GetPrimType() == PTY_agg, "NIY, primType must be PTY_agg.");
    auto regNum = (size <= k8ByteSize) ? kOneRegister : kTwoRegister;
    if (nextGeneralRegNO + regNum - 1 < AArch64Abi::kNumIntParmRegs) {
      ploc.reg0 = AllocateGPRegister();
      ploc.primTypeOfReg0 = (size <= k4ByteSize && !CGOptions::IsBigEndian()) ? PTY_u32 : PTY_u64;
      if (regNum == kTwoRegister) {
        ploc.reg1 = AllocateGPRegister();
        ploc.primTypeOfReg1 =
            (size <= k12ByteSize && !CGOptions::IsBigEndian()) ? PTY_u32 : PTY_u64;
      }
      return;
    }
  }

  // C.13  The NGRN is set to 8.
  ploc.reg0 = kRinvalid;
  nextGeneralRegNO = AArch64Abi::kNumIntParmRegs;
}

static void SetupCCLocInfoRegCount(CCLocInfo &ploc) {
  if (ploc.reg0 == kRinvalid) {
    return;
  }
  ploc.regCount = kOneRegister;
  if (ploc.reg1 == kRinvalid) {
    return;
  }
  ploc.regCount++;
  if (ploc.reg2 == kRinvalid) {
    return;
  }
  ploc.regCount++;
  if (ploc.reg3 == kRinvalid) {
    return;
  }
  ploc.regCount++;
}

// Refer to Procedure Call Standard for the Arm 64-bit
// Architecture (AArch64) 2022Q3.  $6.8.2
//
// LocateNextParm should be called with each parameter in the parameter list
// starting from the beginning, one call per parameter in sequence; it returns
// the information on how each parameter is passed in ploc
//
// *** CAUTION OF USE: ***
// If LocateNextParm is called for function formals, third argument isFirst is true.
// LocateNextParm is then checked against a function parameter list.  All other calls
// of LocateNextParm are against caller's argument list must not have isFirst set,
// or it will be checking the caller's enclosing function.
uint64 AArch64CallConvImpl::LocateNextParm(MIRType &mirType, CCLocInfo &ploc, bool isFirst, MIRFuncType *tFunc) {
  InitCCLocInfo(ploc);

  uint64 typeSize = mirType.GetSize();
  if (typeSize == 0) {
    return 0;
  }

  if (isFirst) {
    auto *func = (tFunc != nullptr) ? tFunc :
                                      beCommon.GetMIRModule().CurFunction()->GetMIRFuncType();
    if (func->FirstArgReturn()) {
      // For return struct in memory, the pointer returns in x8.
      SetupToReturnThroughMemory(ploc);
      return GetPointerSize();
    }
  }

  uint64 typeAlign = mirType.GetAlign();

  ploc.memSize = static_cast<int32>(typeSize);

  uint64 aggCopySize = 0;
  if (IsPrimitiveFloat(mirType.GetPrimType()) || IsPrimitiveVector(mirType.GetPrimType())) {
    // float or vector, passed by float or SIMD register
    ploc.reg0 = AllocateSIMDFPRegister();
    ploc.primTypeOfReg0 = mirType.GetPrimType();
  } else if (IsPrimitiveInteger(mirType.GetPrimType())) {
    // integer, passed by general purpose register
    AllocateGPRegister(mirType, ploc, typeSize, typeAlign);
  } else {
    CHECK_FATAL(mirType.GetPrimType() == PTY_agg, "NIY");
    aggCopySize = AllocateRegisterForAgg(mirType, ploc, typeSize, typeAlign);
  }

  SetupCCLocInfoRegCount(ploc);
  if (ploc.reg0 == kRinvalid) {
    // being passed in memory
    typeAlign = (typeAlign <= k8ByteSize) ? k8ByteSize : typeAlign;
    nextStackArgAdress = RoundUp(nextStackArgAdress, typeAlign);
    ploc.memOffset = static_cast<int32>(nextStackArgAdress);
    // large struct, passed with pointer
    nextStackArgAdress += (aggCopySize != 0 ? k8ByteSize : typeSize);
  }
  return aggCopySize;
}

void AArch64CallConvImpl::SetupSecondRetReg(const MIRType &retTy2, CCLocInfo &ploc) const {
  ASSERT(ploc.reg1 == kRinvalid, "make sure reg1 equal kRinvalid");
  PrimType pType = retTy2.GetPrimType();
  switch (pType) {
    case PTY_void:
      break;
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
      ploc.reg1 = AArch64Abi::kIntReturnRegs[1];
      ploc.primTypeOfReg1 = IsSignedInteger(pType) ? PTY_i64 : PTY_u64;  // promote the type
      break;
    default:
      CHECK_FATAL(false, "NYI");
  }
}
}  /* namespace maplebe */
