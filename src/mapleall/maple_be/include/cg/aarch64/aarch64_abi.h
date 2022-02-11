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
#ifndef MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_ABI_H
#define MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_ABI_H

#include "aarch64_isa.h"
#include "types_def.h"
#include "becommon.h"

namespace maplebe {
using namespace maple;

namespace AArch64Abi {
constexpr int32 kNumIntParmRegs = 8;
constexpr int32 kNumFloatParmRegs = 8;
constexpr int32 kYieldPointReservedReg = 19;
constexpr uint32 kNormalUseOperandNum = 3;
constexpr uint32 kMaxInstrForCondBr = 260000; // approximately less than (2^18);

constexpr AArch64reg intReturnRegs[kNumIntParmRegs] = { R0, R1, R2, R3, R4, R5, R6, R7 };
constexpr AArch64reg floatReturnRegs[kNumFloatParmRegs] = { V0, V1, V2, V3, V4, V5, V6, V7 };
constexpr AArch64reg intParmRegs[kNumIntParmRegs] = { R0, R1, R2, R3, R4, R5, R6, R7 };
constexpr AArch64reg floatParmRegs[kNumFloatParmRegs] = { V0, V1, V2, V3, V4, V5, V6, V7 };

/*
 * Refer to ARM IHI 0055C_beta: Procedure Call Standard  for
 * ARM 64-bit Architecture. Section 5.5
 */
bool IsAvailableReg(AArch64reg reg);
bool IsCalleeSavedReg(AArch64reg reg);
bool IsCallerSaveReg(AArch64reg reg);
bool IsParamReg(AArch64reg reg);
bool IsSpillReg(AArch64reg reg);
bool IsExtraSpillReg(AArch64reg reg);
bool IsSpillRegInRA(AArch64reg regNO, bool has3RegOpnd);
PrimType IsVectorArrayType(MIRType *ty, uint32 &arraySize);
}  /* namespace AArch64Abi */

/*
 * Refer to ARM IHI 0055C_beta: Procedure Call Standard for
 * ARM 64-bit Architecture. Table 1.
 */
enum AArch64ArgumentClass : uint8 {
  kAArch64NoClass,
  kAArch64IntegerClass,
  kAArch64FloatClass,
  kAArch64ShortVectorClass,
  kAArch64PointerClass,
  kAArch64CompositeTypeHFAClass,  /* Homegeneous Floating-point Aggregates */
  kAArch64CompositeTypeHVAClass,  /* Homegeneous Short-Vector Aggregates */
  kAArch64MemoryClass
};

/* for specifying how a parameter is passed */
struct PLocInfo {
  AArch64reg reg0;  /* 0 means parameter is stored on the stack */
  AArch64reg reg1;
  AArch64reg reg2;  /* can have up to 4 single precision fp registers */
  AArch64reg reg3;  /* for small structure return. */
  int32 memOffset;
  int32 memSize;
  uint32 fpSize;
  uint32 numFpPureRegs;
};

/*
 * We use the names used in ARM IHI 0055C_beta. $ 5.4.2.
 * nextGeneralRegNO (= _int_parm_num)  : Next General-purpose Register number
 * nextFloatRegNO (= _float_parm_num): Next SIMD and Floating-point Register Number
 * nextStackArgAdress (= _last_memOffset): Next Stacked Argument Address
 * for processing an incoming or outgoing parameter list
 */
class ParmLocator {
 public:
  /* IHI 0055C_beta $ 5.4.2 Stage A (nextStackArgAdress is set to 0, meaning "relative to the current SP") */
  explicit ParmLocator(BECommon &be) : beCommon(be) {}

  ~ParmLocator() = default;

  /* Return size of aggregate structure copy on stack. */
  int32 LocateNextParm(MIRType &mirType, PLocInfo &pLoc, bool isFirst = false, MIRFunction *func = nullptr);
  int32 LocateRetVal(MIRType &retType, PLocInfo &ploc);
  void InitPLocInfo(PLocInfo &pLoc) const;

 private:
  BECommon &beCommon;
  uint64 paramNum           = 0;  /* number of all types of parameters processed so far */
  int32 nextGeneralRegNO   = 0;  /* number of integer parameters processed so far */
  uint32 nextFloatRegNO    = 0;  /* number of float parameters processed so far */
  int32 nextStackArgAdress = 0;

  AArch64reg AllocateGPRegister() {
    return (nextGeneralRegNO < AArch64Abi::kNumIntParmRegs) ? AArch64Abi::intParmRegs[nextGeneralRegNO++] : kRinvalid;
  }

  void AllocateTwoGPRegisters(PLocInfo &pLoc) {
    if ((nextGeneralRegNO + 1) < AArch64Abi::kNumIntParmRegs) {
      pLoc.reg0 = AArch64Abi::intParmRegs[nextGeneralRegNO++];
      pLoc.reg1 = AArch64Abi::intParmRegs[nextGeneralRegNO++];
    } else {
      pLoc.reg0 = kRinvalid;
    }
  }

  AArch64reg AllocateSIMDFPRegister() {
    return (nextFloatRegNO < AArch64Abi::kNumFloatParmRegs) ? AArch64Abi::floatParmRegs[nextFloatRegNO++] : kRinvalid;
  }

  void AllocateNSIMDFPRegisters(PLocInfo &ploc, uint32 num) {
    if ((nextFloatRegNO + num - 1) < AArch64Abi::kNumFloatParmRegs) {
      switch (num) {
        case kOneRegister:
          ploc.reg0 = AArch64Abi::floatParmRegs[nextFloatRegNO++];
          break;
        case kTwoRegister:
          ploc.reg0 = AArch64Abi::floatParmRegs[nextFloatRegNO++];
          ploc.reg1 = AArch64Abi::floatParmRegs[nextFloatRegNO++];
          break;
        case kThreeRegister:
          ploc.reg0 = AArch64Abi::floatParmRegs[nextFloatRegNO++];
          ploc.reg1 = AArch64Abi::floatParmRegs[nextFloatRegNO++];
          ploc.reg2 = AArch64Abi::floatParmRegs[nextFloatRegNO++];
          break;
        case kFourRegister:
          ploc.reg0 = AArch64Abi::floatParmRegs[nextFloatRegNO++];
          ploc.reg1 = AArch64Abi::floatParmRegs[nextFloatRegNO++];
          ploc.reg2 = AArch64Abi::floatParmRegs[nextFloatRegNO++];
          ploc.reg3 = AArch64Abi::floatParmRegs[nextFloatRegNO++];
          break;
        default:
          CHECK_FATAL(0, "AllocateNSIMDFPRegisters: unsupported");
      }
    } else {
      ploc.reg0 = kRinvalid;
    }
  }

  void RoundNGRNUpToNextEven() {
    nextGeneralRegNO = static_cast<int32>((nextGeneralRegNO + 1) & ~static_cast<int32>(1));
  }

  int32 ProcessPtyAggWhenLocateNextParm(MIRType &mirType, PLocInfo &pLoc, uint64 &typeSize, int32 typeAlign);
};

/* given the type of the return value, determines the return mechanism */
class ReturnMechanism {
 public:
  ReturnMechanism(MIRType &retType, const BECommon &be);

  ~ReturnMechanism() = default;

  uint8 GetRegCount() const {
    return regCount;
  }

  PrimType GetPrimTypeOfReg0() const {
    return primTypeOfReg0;
  }

  AArch64reg GetReg0() const {
    return reg0;
  }

  AArch64reg GetReg1() const {
    return reg1;
  }

  AArch64reg GetReg2() const {
    return reg2;
  }

  AArch64reg GetReg3() const {
    return reg3;
  }

  void SetupToReturnThroughMemory() {
    regCount = 1;
    reg0 = R8;
    primTypeOfReg0 = PTY_u64;
  }

  void SetupSecondRetReg(const MIRType &retTy2);
 private:
  uint8 regCount;             /* number of registers <= 2 storing the return value */
  AArch64reg reg0;            /* first register storing the return value */
  AArch64reg reg1;            /* second register storing the return value */
  AArch64reg reg2;            /* third register storing the return value */
  AArch64reg reg3;            /* fourth register storing the return value */
  PrimType primTypeOfReg0;    /* the primitive type stored in reg0 */
  PrimType primTypeOfReg1;    /* the primitive type stored in reg1 */
};
}  /* namespace maplebe */

#endif  /* MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_ABI_H */
