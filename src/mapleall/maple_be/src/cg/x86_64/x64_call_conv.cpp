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

namespace {
constexpr int kMaxRegCount = 4;

/* 3.1 Machine Interface
   3.1.2 Data Representation
 */
enum X64ArgumentClass : uint8 {
  kX64NoClass,
  kX64IntegerClass,
  kX64FloatClass,
  kX64PointerClass,
  kX64VectorClass,
  kX64MemoryClass
};

int32 ProcessNonStructAndNonArrayWhenClassifyAggregate(const MIRType &mirType,
                                                       X64ArgumentClass classes[kMaxRegCount],
                                                       size_t classesLength) {
  CHECK_FATAL(classesLength > 0, "classLength must > 0");
  /* refering to Figure 3.1: Scalar Types */
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
      classes[0] = kX64IntegerClass;
      return 1;
    case PTY_f32:
    case PTY_f64:
    case PTY_c64:
    case PTY_c128:
      classes[0] = kX64FloatClass;
      return 1;
    default:
      CHECK_FATAL(false, "NYI");
  }

  /* should not reach to this point */
  return 0;
}
PrimType TraverseStructFieldsForFp(MIRType *ty, uint32 &numRegs) {
  if (ty->GetKind() == kTypeArray) {
    MIRArrayType *arrtype = static_cast<MIRArrayType *>(ty);
    MIRType *pty = GlobalTables::GetTypeTable().GetTypeFromTyIdx(arrtype->GetElemTyIdx());
    if (pty->GetKind() == kTypeArray || pty->GetKind() == kTypeStruct) {
      return TraverseStructFieldsForFp(pty, numRegs);
    }
    for (uint32 i = 0; i < arrtype->GetDim(); ++i) {
      numRegs += arrtype->GetSizeArrayItem(i);
    }
    return pty->GetPrimType();
  } else if (ty->GetKind() == kTypeStruct) {
    MIRStructType *sttype = static_cast<MIRStructType *>(ty);
    FieldVector fields = sttype->GetFields();
    PrimType oldtype = PTY_void;
    for (uint32 fcnt = 0; fcnt < fields.size(); ++fcnt) {
      TyIdx fieldtyidx = fields[fcnt].second.first;
      MIRType *fieldty = GlobalTables::GetTypeTable().GetTypeFromTyIdx(fieldtyidx);
      PrimType ptype = TraverseStructFieldsForFp(fieldty, numRegs);
      if (oldtype != PTY_void && oldtype != ptype) {
        return PTY_void;
      } else {
        oldtype = ptype;
      }
    }
    return oldtype;
  } else {
    numRegs++;
    return ty->GetPrimType();
  }
}

/* 3.2 Function Calling Sequence
   3.2.3 Parameter Passing
   - Definitions
   - Classification
     - Classification of aggregate (structures and arrays) and union types
     - Classification of basic types
   - Passing
   - Returning of Values
*/
/*
 * Classification of aggregate (structures and arrays) and union types:
 *   1. If the size of an object is larger than four eightbytes, or it contains unaligned fields, it has class MEMORY
 *   2. If a C++ object has either a non-trivial copy constructor or a non-trivial destructor 11, it is passed by
 *     invisible reference (the object is replaced in the parameter list by a pointer that has class INTEGER) 12.
 *   3. If the size of the aggregate exceeds a single eightbyte, each is classified separately.
 *     Each eightbyte gets initialized to class NO_CLASS.
 *   4. Each field of an object is classified recursively so that always two fields are considered.
 *     The resulting class is calculated according to the classes of the fields in the eightbyte:
 *       (a) If both classes are equal, this is the resulting class.
 *       (b) If one of the classes is NO_CLASS, the resulting class is the other class.
 *       (c) If one of the classes is MEMORY, the result is the MEMORY class.
 *       (d) If one of the classes is INTEGER, the result is the INTEGER.
 *       (e) If one of the classes is X87, X87UP, COMPLEX_X87 class, MEMORY
 *         is used as class.
 *       (f) Otherwise class SSE is used.
 *    5. Then a post merger cleanup is done:
 *       (a) If one of the classes is MEMORY, the whole argument is passed in
 *         memory.
 *       (b) If X87UP is not preceded by X87, the whole argument is passed in
 *         memory.
 *       (c) If the size of the aggregate exceeds two eightbytes and the first eightbyte
 *         isn’t SSE or any other eightbyte isn’t SSEUP, the whole argument is passed
 *         in memory.
 */
int32 ClassifyAggregate(const BECommon &be, MIRType &mirType, X64ArgumentClass classes[kMaxRegCount],
                        size_t classesLength, uint32 &fpSize);
uint32 ProcessStructWhenClassifyAggregate(const BECommon &be, MIRStructType &structType,
                                          X64ArgumentClass classes[kMaxRegCount],
                                          size_t classesLength, uint32 &fpSize) {
  CHECK_FATAL(classesLength > 0, "classLength must > 0");
  uint32 sizeOfTyInDwords = static_cast<uint32>(
      RoundUp(be.GetTypeSize(structType.GetTypeIndex()), k8ByteSize) >> k8BitShift);
  bool isF32 = false;
  bool isF64 = false;
  uint32 numRegs = 0;
  for (uint32 f = 0; f < structType.GetFieldsSize(); ++f) {
    TyIdx fieldTyIdx = structType.GetFieldsElemt(f).second.first;
    MIRType *fieldType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(fieldTyIdx);
    PrimType pType = TraverseStructFieldsForFp(fieldType, numRegs);
    if (pType == PTY_f32) {
      if (isF64) {
        isF64 = false;
        break;
      }
      isF32 = true;
    } else if (pType == PTY_f64) {
      if (isF32) {
        isF32 = false;
        break;
      }
      isF64 = true;
    } else if (IsPrimitiveVector(pType)) {
      isF64 = true;
      break;
    } else {
      isF32 = isF64 = false;
      break;
    }
  }
  if (isF32 || isF64) {
    for (uint32 i = 0; i < numRegs; ++i) {
      classes[i] = kX64FloatClass;
    }
    fpSize = isF32 ? k4ByteSize : k8ByteSize;
    return numRegs;
  }

  classes[0] = kX64IntegerClass;
  if (sizeOfTyInDwords == kDwordSizeTwo) {
    classes[1] = kX64IntegerClass;
  }
  return sizeOfTyInDwords;
}
int32 ClassifyAggregate(const BECommon &be, MIRType &mirType, X64ArgumentClass classes[kMaxRegCount],
                        size_t classesLength, uint32 &fpSize) {
  CHECK_FATAL(classesLength > 0, "invalid index");
  uint64 sizeOfTy = be.GetTypeSize(mirType.GetTypeIndex());
  if ((sizeOfTy > k16ByteSize) || (sizeOfTy == 0)) {
    return 0;
  }

  int32 sizeOfTyInDwords = RoundUp(sizeOfTy, k8ByteSize) >> k8BitShift;
  ASSERT(sizeOfTyInDwords > 0, "sizeOfTyInDwords should be sizeOfTyInDwords > 0");
  ASSERT(sizeOfTyInDwords <= kTwoRegister, "sizeOfTyInDwords should be <= 2");
  int32 i;
  for (i = 0; i < sizeOfTyInDwords; ++i) {
    classes[i] = kX64NoClass;
  }
  if ((mirType.GetKind() != kTypeStruct) && (mirType.GetKind() != kTypeArray) && (mirType.GetKind() != kTypeUnion)) {
    return ProcessNonStructAndNonArrayWhenClassifyAggregate(mirType, classes, classesLength);
  }
  if (mirType.GetKind() == kTypeStruct) {
    MIRStructType &structType = static_cast<MIRStructType&>(mirType);
    return static_cast<int32>(ProcessStructWhenClassifyAggregate(be, structType, classes, classesLength, fpSize));
  }
  /* post merger clean-up */
  for (i = 0; i < sizeOfTyInDwords; ++i) {
    if (classes[i] == kX64MemoryClass) {
      return 0;
    }
  }
  return sizeOfTyInDwords;
}
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

/*
 * Returning of Values:
 *   1. Classify the return type with the classification algorithm.
 *   2. If the type has class MEMORY, then the caller provides space for the return
 *     value and passes the address of this storage in %rdi as if it were the first
 *     argument to the function. In effect, this address becomes a “hidden” first argument.
 *     This storage must not overlap any data visible to the callee through
 *     other names than this argument.
 *     On return %rax will contain the address that has been passed in by the
 *     caller in %rdi.
 *   3. If the class is INTEGER, the next available register of the sequence %rax,
 *      %rdx is used.
 *   4. If the class is SSE, the next available vector register of the sequence %xmm0,
 *     %xmm1 is used.
 *   5. If the class is SSEUP, the eightbyte is returned in the next available eightbyte
 *     chunk of the last used vector register.
 *   6. If the class is X87, the value is returned on the X87 stack in %st0 as 80-bit
 *     x87 number.
 *   7. If the class is X87UP, the value is returned together with the previous X87
 *     value in %st0.
 *   8. If the class is COMPLEX_X87, the real part of the value is returned in
 *     %st0 and the imaginary part in %st1.
 */
int32 X64CallConvImpl::LocateRetVal(MIRType &retType, CCLocInfo &pLoc) {
  InitCCLocInfo(pLoc);
  uint32 retSize = beCommon.GetTypeSize(retType.GetTypeIndex().GetIdx());
  if (retSize == 0) {
    return 0;    /* size 0 ret val */
  }
  if (retSize <= k16ByteSize) {
    /* For return struct size less or equal to 16 bytes, the values */
    /* are returned in register pairs. */
    X64ArgumentClass classes[kMaxRegCount] = { kX64NoClass }; /* Max of four floats. */
    uint32 fpSize;
    /* Classify the return type with the classification algorithm. */
    uint32 numRegs = static_cast<uint32>(ClassifyAggregate(beCommon, retType, classes, sizeof(classes), fpSize));
    if (classes[0] == kX64FloatClass) {
      CHECK_FATAL(numRegs <= kMaxRegCount, "LocateNextParm: illegal number of regs");
      AllocateNSIMDFPRegisters(pLoc, numRegs);
      pLoc.numFpPureRegs = numRegs;
      pLoc.fpSize = fpSize;
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
}
void X64CallConvImpl::SetupSecondRetReg(const MIRType &retTy2, CCLocInfo &pLoc) {
  ASSERT(pLoc.reg1 == kRinvalid, "make sure reg1 equal kRinvalid");
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
      pLoc.reg1 = X64Abi::intReturnRegs[1];
      pLoc.primTypeOfReg1 = IsSignedInteger(pType) ? PTY_i64 : PTY_u64;  /* promote the type */
      break;
    default:
      CHECK_FATAL(false, "NYI");
  }
}
void X64CallConvImpl::InitReturnInfo(MIRType &retTy, CCLocInfo &ccLocInfo) {
  PrimType pType = retTy.GetPrimType();
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
      ccLocInfo.regCount = 1;
      ccLocInfo.reg0 = X64Abi::intReturnRegs[0];
      ccLocInfo.primTypeOfReg0 = IsSignedInteger(pType) ? PTY_i32 : PTY_u32;  /* promote the type */
      return;

    case PTY_ptr:
    case PTY_ref:
      CHECK_FATAL(false, "PTY_ptr should have been lowered");
      return;

    case PTY_a64:
    case PTY_u64:
    case PTY_i64:
    case PTY_i128:
    case PTY_u128:
      ccLocInfo.regCount = 1;
      ccLocInfo.reg0 = X64Abi::intReturnRegs[0];
      ccLocInfo.primTypeOfReg0 = IsSignedInteger(pType) ? PTY_i64 : PTY_u64;  /* promote the type */
      return;

    case PTY_f32:
    case PTY_f64:
    case PTY_c64:
    case PTY_v2i32:
    case PTY_v4i16:
    case PTY_v8i8:
    case PTY_v2u32:
    case PTY_v4u16:
    case PTY_v8u8:
    case PTY_v2f32:
    case PTY_c128:
    case PTY_v2i64:
    case PTY_v4i32:
    case PTY_v8i16:
    case PTY_v16i8:
    case PTY_v2u64:
    case PTY_v4u32:
    case PTY_v8u16:
    case PTY_v16u8:
    case PTY_v2f64:
    case PTY_v4f32:
      ccLocInfo.regCount = 1;
      ccLocInfo.reg0 = X64Abi::floatReturnRegs[0];
      ccLocInfo.primTypeOfReg0 = pType;
      return;

    case PTY_agg: {
      uint64 size = beCommon.GetTypeSize(retTy.GetTypeIndex());
      if ((size > k16ByteSize) || (size == 0)) {
        /*
         * The return value is returned via memory.
         * The address is in R7(rdi) and passed by the caller.
         */
        SetupToReturnThroughMemory(ccLocInfo);
        return;
      }
      uint32 fpSize;
      X64ArgumentClass classes[kMaxRegCount] = { kX64NoClass };
      ccLocInfo.regCount = static_cast<uint8>(ClassifyAggregate(beCommon, retTy, classes,
                                                      sizeof(classes) / sizeof(X64ArgumentClass), fpSize));
      if (classes[0] == kX64FloatClass) {
        switch (ccLocInfo.regCount) {
          case kTwoRegister:
            ccLocInfo.reg1 = X64Abi::floatReturnRegs[1];
            break;
          case kOneRegister:
            ccLocInfo.reg0 = X64Abi::floatReturnRegs[0];
            break;
          default:
            CHECK_FATAL(0, "X64CallConvImpl: unsupported");
        }
        if (fpSize == k4ByteSize) {
          ccLocInfo.primTypeOfReg0 = ccLocInfo.primTypeOfReg1 = PTY_f32;
        } else {
          ccLocInfo.primTypeOfReg0 = ccLocInfo.primTypeOfReg1 = PTY_f64;
        }
        return;
      } else if (ccLocInfo.regCount == 0) {
        SetupToReturnThroughMemory(ccLocInfo);
        return;
      } else {
        if (ccLocInfo.regCount == 1) {
          /* passing in registers */
          if (classes[0] == kX64FloatClass) {
            ccLocInfo.reg0 = X64Abi::floatReturnRegs[0];
            ccLocInfo.primTypeOfReg0 = PTY_f64;
          } else {
            ccLocInfo.reg0 = X64Abi::intReturnRegs[0];
            ccLocInfo.primTypeOfReg0 = PTY_i64;
          }
        } else {
          ASSERT(ccLocInfo.regCount == kMaxRegCount, "reg count from ClassifyAggregate() should be 0, 1, or 2");
          ASSERT(classes[0] == kX64IntegerClass, "error val :classes[0]");
          ASSERT(classes[1] == kX64IntegerClass, "error val :classes[1]");
          ccLocInfo.reg0 = X64Abi::intReturnRegs[0];
          ccLocInfo.primTypeOfReg0 = PTY_i64;
          ccLocInfo.reg1 = X64Abi::intReturnRegs[1];
          ccLocInfo.primTypeOfReg1 = PTY_i64;
        }
        return;
      }
    }
    default:
      CHECK_FATAL(false, "NYI");
  }
}

/*
 * The Classification of basic types:
 *  1. Arguments of types (signed and unsigned) _Bool, char, short, int,
 *  long, long long, and pointers are in the INTEGER class.
 *   2. Arguments of types float, double, _Decimal32, _Decimal64 and
 *  __m64 are in class SSE.
 *   3. Arguments of types __float128, _Decimal128 and __m128 are split
 *  into two halves. The least significant ones belong to class SSE, the most
 *  significant one to class SSEUP.
 *  4. Arguments of type __m256 are split into four eightbyte chunks. The least
 *  significant one belongs to class SSE and all the others to class SSEUP.
 *  5. The 64-bit mantissa of arguments of type long double belongs to class
 *  X87, the 16-bit exponent plus 6 bytes of padding belongs to class X87UP.
 *  6. Arguments of type __int128 offer the same operations as INTEGERs,
 *  yet they do not fit into one general purpose register but require two registers.
 *  7. Arguments of complex T where T is one of the types float or double
 *  8. A variable of type complex long double is classified as type COMPLEX_
 *  X87.
 */
/*
 * Passing:
 *   Once arguments are classified, the registers get assigned (in left-to-right
 *     order) for passing as follows:
 *   1. If the class is MEMORY, pass the argument on the stack.
 *   2. If the class is INTEGER, the next available register of the sequence %rdi,
 *     %rsi, %rdx, %rcx, %r8 and %r9 is used13.
 *   3. If the class is SSE, the next available vector register is used, the registers
 *     are taken in the order from %xmm0 to %xmm7.
 *   4. If the class is SSEUP, the eightbyte is passed in the next available eightbyte
 *     chunk of the last used vector register.
 *   5. If the class is X87, X87UP or COMPLEX_X87, it is passed in memory.
 */
int32 X64CallConvImpl::LocateNextParm(MIRType &mirType, CCLocInfo &pLoc, bool isFirst, MIRFunction *tFunc) {
  InitCCLocInfo(pLoc);

  bool is64x1vec = false;
  if (tFunc != nullptr && tFunc->GetParamSize() > 0) {
    is64x1vec = tFunc->GetNthParamAttr(paramNum).GetAttr(ATTR_oneelem_simd) != 0;
  }

  if (isFirst) {
    MIRFunction *func = tFunc != nullptr ? tFunc : const_cast<MIRFunction *>(beCommon.GetMIRModule().CurFunction());
    if (beCommon.HasFuncReturnType(*func)) {
      size_t size = beCommon.GetTypeSize(beCommon.GetFuncReturnType(*func));
      if (size == 0) {
        /* For return struct size 0 there is no return value. */
        return 0;
      } else if (size > k16ByteSize) {
        /* For return struct size > 16 bytes the pointer returns in R7(rdi). */
        pLoc.reg0 = R7;
        return kSizeOfPtr;
      }
      /* For return struct size less or equal to 16 bytes, the values
       * are returned in register pairs.
       * Check for pure float struct.
       */
      X64ArgumentClass classes[kMaxRegCount] = { kX64NoClass };
      uint32 fpSize;
      MIRType *retType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(beCommon.GetFuncReturnType(*func));
      uint32 numRegs = static_cast<uint32>(ClassifyAggregate(beCommon, *retType, classes, sizeof(classes), fpSize));
      if (classes[0] == kX64FloatClass) {
        CHECK_FATAL(numRegs <= kMaxRegCount, "LocateNextParm: illegal number of regs");
        AllocateNSIMDFPRegisters(pLoc, numRegs);
        pLoc.numFpPureRegs = numRegs;
        pLoc.fpSize = fpSize;
        return 0;
      } else {
        CHECK_FATAL(numRegs <= kTwoRegister, "LocateNextParm: illegal number of regs");
        if (numRegs == kOneRegister) {
          pLoc.reg0 = AllocateGPParmRegister();
        } else {
          AllocateTwoGPParmRegisters(pLoc);
        }
        return 0;
      }
    }
  }
  uint64 typeSize = beCommon.GetTypeSize(mirType.GetTypeIndex());
  if (typeSize == 0) {
    return 0;
  }
  int32 typeAlign = beCommon.GetTypeAlign(mirType.GetTypeIndex());
  /*
   * Refering to 3.2.3, The size of each argument gets rounded
   * up to eightbytes. Therefore the stack will always be eightbyte aligned.
   */
  ASSERT((nextStackArgAdress & (std::max(typeAlign, static_cast<int32>(k8ByteSize)) - 1)) == 0,
         "C.12 alignment requirement is violated");
  pLoc.memSize = static_cast<int32>(typeSize);
  ++paramNum;

  int32 aggCopySize = 0;
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
    case PTY_i128:
    case PTY_u128:
      typeSize = k8ByteSize;
      pLoc.reg0 = is64x1vec ? AllocateSIMDFPRegister() : AllocateGPParmRegister();
      ASSERT(nextGeneralParmRegNO <= X64Abi::kNumIntParmRegs, "RegNo should be pramRegNO");
      break;

    case PTY_f32:
    case PTY_f64:
    case PTY_c64:
    case PTY_v2i32:
    case PTY_v4i16:
    case PTY_v8i8:
    case PTY_v2u32:
    case PTY_v4u16:
    case PTY_v8u8:
    case PTY_v2f32:

      ASSERT(GetPrimTypeSize(PTY_f64) == k8ByteSize, "unexpected type size");
      typeSize = k8ByteSize;
      pLoc.reg0 = AllocateSIMDFPRegister();
      break;

    case PTY_c128:
    case PTY_v2i64:
    case PTY_v4i32:
    case PTY_v8i16:
    case PTY_v16i8:
    case PTY_v2u64:
    case PTY_v4u32:
    case PTY_v8u16:
    case PTY_v16u8:
    case PTY_v2f64:
    case PTY_v4f32:
      /* SIMD-FP registers have 128-bits. */
      pLoc.reg0 = AllocateSIMDFPRegister();
      ASSERT(nextGeneralParmRegNO <= X64Abi::kNumFloatParmRegs, "regNO should not be greater than kNumFloatParmRegs");
      ASSERT(typeSize == k16ByteSize, "unexpected type size");
      break;

    /* case PTY_agg */
    case PTY_agg: {
      aggCopySize = ProcessPtyAggWhenLocateNextParm(mirType, pLoc, typeSize, typeAlign);
      break;
    }

    default:
      CHECK_FATAL(false, "NYI");
  }

  if (pLoc.reg0 == kRinvalid) {
    /* being passed in memory */
    nextStackArgAdress = pLoc.memOffset + typeSize;
  }
  return aggCopySize;
}

int32 X64CallConvImpl::ProcessPtyAggWhenLocateNextParm(MIRType &mirType, CCLocInfo &pLoc, uint64 &typeSize,
    int32 typeAlign) {
  return 0;
}

}  /* namespace maplebe */
