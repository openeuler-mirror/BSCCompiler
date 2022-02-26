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
#ifndef MAPLEBE_INCLUDE_CG_X64_X64_CALL_CONV_H
#define MAPLEBE_INCLUDE_CG_X64_X64_CALL_CONV_H

#include "types_def.h"
#include "becommon.h"
#include "call_conv.h"
#include "x64_abi.h"

namespace maplebe {
using namespace maple;
class X64CallConvImpl {
 public:
  explicit X64CallConvImpl(BECommon &be) : beCommon(be) {}

  ~X64CallConvImpl() = default;

  /* Return size of aggregate structure copy on stack. */
  int32 LocateNextParm(MIRType &mirType, CCLocInfo &pLoc, bool isFirst = false, MIRFunction *func = nullptr);

  int32 LocateRetVal(MIRType &retType, CCLocInfo &ploc);

  void InitCCLocInfo(CCLocInfo &pLoc) const;

  /*  return value related  */
  void InitReturnInfo(MIRType &retTy, CCLocInfo &pLoc);

  void SetupSecondRetReg(const MIRType &retTy2, CCLocInfo &pLoc);

  void SetupToReturnThroughMemory(CCLocInfo &pLoc) {
    pLoc.regCount = 1;
    pLoc.reg0 = R7;  /* storage in %rdi provided by caller */
    pLoc.primTypeOfReg0 = PTY_u64;
  }

 private:
  X64reg AllocateGPParmRegister() {
    return (nextGeneralParmRegNO < X64Abi::kNumIntParmRegs) ?
      X64Abi::intParmRegs[nextGeneralParmRegNO++] : kRinvalid;
  }

  void AllocateTwoGPParmRegisters(CCLocInfo &pLoc) {
    if ((nextGeneralParmRegNO + 1) < X64Abi::kNumIntParmRegs) {
      pLoc.reg0 = X64Abi::intParmRegs[nextGeneralParmRegNO++];
      pLoc.reg1 = X64Abi::intParmRegs[nextGeneralParmRegNO++];
    } else {
      pLoc.reg0 = kRinvalid;
    }
  }

  X64reg AllocateGPReturnRegister() {
    return (nextGeneralReturnRegNO < X64Abi::kNumIntReturnRegs) ?
      X64Abi::intReturnRegs[nextGeneralReturnRegNO++] : kRinvalid;
  }

  void AllocateTwoGPReturnRegisters(CCLocInfo &pLoc) {
    if ((nextGeneralReturnRegNO + 1) < X64Abi::kNumIntReturnRegs) {
      pLoc.reg0 = X64Abi::intReturnRegs[nextGeneralReturnRegNO++];
      pLoc.reg1 = X64Abi::intReturnRegs[nextGeneralReturnRegNO++];
    } else {
      pLoc.reg0 = kRinvalid;
    }
  }

  X64reg AllocateSIMDFPRegister() {
    return (nextFloatRegNO < X64Abi::kNumFloatParmRegs) ? X64Abi::floatParmRegs[nextFloatRegNO++] : kRinvalid;
  }

  void AllocateNSIMDFPRegisters(CCLocInfo &ploc, uint32 num) {
    if ((nextFloatRegNO + num - 1) < X64Abi::kNumFloatParmRegs) {
      switch (num) {
        case kOneRegister:
          ploc.reg0 = X64Abi::floatParmRegs[nextFloatRegNO++];
          break;
        case kTwoRegister:
          ploc.reg0 = X64Abi::floatParmRegs[nextFloatRegNO++];
          ploc.reg1 = X64Abi::floatParmRegs[nextFloatRegNO++];
          break;
        case kThreeRegister:
          ploc.reg0 = X64Abi::floatParmRegs[nextFloatRegNO++];
          ploc.reg1 = X64Abi::floatParmRegs[nextFloatRegNO++];
          ploc.reg2 = X64Abi::floatParmRegs[nextFloatRegNO++];
          break;
        case kFourRegister:
          ploc.reg0 = X64Abi::floatParmRegs[nextFloatRegNO++];
          ploc.reg1 = X64Abi::floatParmRegs[nextFloatRegNO++];
          ploc.reg2 = X64Abi::floatParmRegs[nextFloatRegNO++];
          ploc.reg3 = X64Abi::floatParmRegs[nextFloatRegNO++];
          break;
        default:
          CHECK_FATAL(0, "AllocateNSIMDFPRegisters: unsupported");
      }
    } else {
      ploc.reg0 = kRinvalid;
    }
  }

  void RoundNGRNUpToNextEven() {
    nextGeneralParmRegNO = static_cast<int32>((nextGeneralParmRegNO + 1) & ~static_cast<int32>(1));
  }

  int32 ProcessPtyAggWhenLocateNextParm(MIRType &mirType, CCLocInfo &pLoc, uint64 &typeSize, int32 typeAlign);

  BECommon &beCommon;
  uint64 paramNum = 0;  /* number of all types of parameters processed so far */
  int32 nextGeneralParmRegNO = 0;  /* number of integer parameters processed so far */
  int32 nextGeneralReturnRegNO = 0;  /* number of integer return processed so far */
  uint32 nextFloatRegNO = 0;  /* number of float parameters processed so far */
  int32 nextStackArgAdress = 0;
};
}  /* namespace maplebe */

#endif  /* MAPLEBE_INCLUDE_CG_X64_X64_CALL_CONV_H */
