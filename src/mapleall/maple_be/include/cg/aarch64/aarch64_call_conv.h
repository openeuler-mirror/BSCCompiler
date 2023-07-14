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
#include "aarch64_isa.h"
#include "aarch64_abi.h"

namespace maplebe {
using namespace maple;

// We use the names used in Procedure Call Standard for the Arm 64-bit
// Architecture (AArch64) 2022Q3.  $6.8.2
// nextGeneralRegNO (= _int_parm_num)  : Next General-purpose Register number
// nextFloatRegNO (= _float_parm_num): Next SIMD and Floating-point Register Number
// nextStackArgAdress (= _last_memOffset): Next Stacked Argument Address
// for processing an incoming or outgoing parameter list
class AArch64CallConvImpl {
 public:
  explicit AArch64CallConvImpl(BECommon &be) : beCommon(be) {}

  ~AArch64CallConvImpl() = default;

  // Return size of aggregate structure copy on stack.
  uint64 LocateNextParm(const MIRType &mirType, CCLocInfo &ploc, bool isFirst = false,
                        MIRFuncType *tFunc = nullptr);

  void LocateRetVal(const MIRType &retType, CCLocInfo &ploc) const;

  void InitCCLocInfo(CCLocInfo &ploc) const;

  // for lmbc
  uint32 FloatParamRegRequired(const MIRStructType &structType, uint32 &fpSize);

  void SetupSecondRetReg(const MIRType &retTy2, CCLocInfo &ploc) const;

  void SetupToReturnThroughMemory(CCLocInfo &ploc) const {
    ploc.regCount = 1;
    ploc.reg0 = R8;
    ploc.primTypeOfReg0 = GetExactPtrPrimType();
  }

 private:
  BECommon &beCommon;
  uint32 nextGeneralRegNO = 0;  //number of integer parameters processed so far
  uint32 nextFloatRegNO = 0;    // number of float parameters processed so far
  uint64 nextStackArgAdress = 0;

  AArch64reg AllocateGPRegister() {
    return (nextGeneralRegNO < AArch64Abi::kNumIntParmRegs) ? AArch64Abi::kIntParmRegs[nextGeneralRegNO++] : kRinvalid;
  }

  void AllocateGPRegister(const MIRType &mirType, CCLocInfo &ploc, uint64 size, uint64 align);

  AArch64reg AllocateSIMDFPRegister() {
    return (nextFloatRegNO < AArch64Abi::kNumFloatParmRegs) ? AArch64Abi::kFloatParmRegs[nextFloatRegNO++] : kRinvalid;
  }

  uint64 AllocateRegisterForAgg(const MIRType &mirType, CCLocInfo &ploc, uint64 size, uint64 &align);
};
}  /* namespace maplebe */

#endif  /* MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_CALL_CONV_H */
