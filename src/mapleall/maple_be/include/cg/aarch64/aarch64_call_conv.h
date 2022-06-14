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
#ifndef MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_CALL_CONV_H
#define MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_CALL_CONV_H

#include "types_def.h"
#include "becommon.h"
#include "call_conv.h"

namespace maplebe {
using namespace maple;

/*
 * We use the names used in ARM IHI 0055C_beta. $ 5.4.2.
 * nextGeneralRegNO (= _int_parm_num)  : Next General-purpose Register number
 * nextFloatRegNO (= _float_parm_num): Next SIMD and Floating-point Register Number
 * nextStackArgAdress (= _last_memOffset): Next Stacked Argument Address
 * for processing an incoming or outgoing parameter list
 */
class AArch64CallConvImpl {
 public:
  explicit AArch64CallConvImpl(BECommon &be) : beCommon(be) {}

  ~AArch64CallConvImpl() = default;

  /* Return size of aggregate structure copy on stack. */
  int32 LocateNextParm(MIRType &mirType, CCLocInfo &pLoc, bool isFirst = false, MIRFunction *func = nullptr);

  int32 LocateRetVal(MIRType &retType, CCLocInfo &ploc);

  void InitCCLocInfo(CCLocInfo &pLoc) const;

  /* for lmbc */
  uint32 FloatParamRegRequired(MIRStructType &structType, uint32 &fpSize);

  /*  return value related  */
  void InitReturnInfo(MIRType &retTy, CCLocInfo &pLoc);

  void SetupSecondRetReg(const MIRType &retTy2, CCLocInfo &pLoc) const;

  void SetupToReturnThroughMemory(CCLocInfo &pLoc) const {
    pLoc.regCount = 1;
    pLoc.reg0 = R8;
    pLoc.primTypeOfReg0 = PTY_u64;
  }

 private:
  BECommon &beCommon;
  uint64 paramNum = 0;  /* number of all types of parameters processed so far */
  int32 nextGeneralRegNO = 0;  /* number of integer parameters processed so far */
  uint32 nextFloatRegNO = 0;  /* number of float parameters processed so far */
  int32 nextStackArgAdress = 0;

  AArch64reg AllocateGPRegister() {
    ASSERT(nextGeneralRegNO >= 0, "nextGeneralRegNO can not be neg");
    return (nextGeneralRegNO < AArch64Abi::kNumIntParmRegs) ? AArch64Abi::intParmRegs[nextGeneralRegNO++] : kRinvalid;
  }

  void AllocateTwoGPRegisters(CCLocInfo &pLoc) {
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

  void AllocateNSIMDFPRegisters(CCLocInfo &ploc, uint32 num) {
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

  int32 ProcessPtyAggWhenLocateNextParm(MIRType &mirType, CCLocInfo &pLoc, uint64 &typeSize, int32 typeAlign);
};
}  /* namespace maplebe */

#endif  /* MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_CALL_CONV_H */
