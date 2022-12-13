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
#include "abi.h"
#include "x64_abi.h"

namespace maplebe {
using namespace maple;
using namespace x64;

constexpr const uint32 kMaxStructParamByReg = 4;

class X64CallConvImpl {
 public:
  explicit X64CallConvImpl(BECommon &be) : beCommon(be) {}

  ~X64CallConvImpl() = default;

  void InitCCLocInfo(CCLocInfo &pLoc) const;

  /* Passing  value related */
  int32 LocateNextParm(MIRType &mirType, CCLocInfo &pLoc, bool isFirst = false, MIRFunction *func = nullptr);

  /*  return value related  */
  int32 LocateRetVal(MIRType &retType, CCLocInfo &ploc);

 private:
  X64reg AllocateGPParmRegister() {
    return (nextGeneralParmRegNO < kNumIntParmRegs) ?
        kIntParmRegs[nextGeneralParmRegNO++] : kRinvalid;
  }

  void AllocateTwoGPParmRegisters(CCLocInfo &pLoc) {
    if ((nextGeneralParmRegNO + 1) < kNumIntParmRegs) {
      pLoc.reg0 = kIntParmRegs[nextGeneralParmRegNO++];
      pLoc.reg1 = kIntParmRegs[nextGeneralParmRegNO++];
    } else {
      pLoc.reg0 = kRinvalid;
    }
  }

  X64reg AllocateSIMDFPRegister() {
    return (nextFloatRegNO < kNumFloatParmRegs) ?
        kFloatParmRegs[nextFloatRegNO++] : kRinvalid;
  }

  X64reg AllocateGPReturnRegister() {
    return (nextGeneralReturnRegNO < kNumIntReturnRegs) ?
        kIntReturnRegs[nextGeneralReturnRegNO++] : kRinvalid;
  }

  void AllocateTwoGPReturnRegisters(CCLocInfo &pLoc) {
    if ((nextGeneralReturnRegNO + 1) < kNumIntReturnRegs) {
      pLoc.reg0 = kIntReturnRegs[nextGeneralReturnRegNO++];
      pLoc.reg1 = kIntReturnRegs[nextGeneralReturnRegNO++];
    } else {
      pLoc.reg0 = kRinvalid;
    }
  }

  X64reg AllocateSIMDFPReturnRegister() {
    return (nextFloatRetRegNO < kNumFloatReturnRegs) ?
        kFloatReturnRegs[nextFloatRetRegNO++] : kRinvalid;
  }

  BECommon &beCommon;
uint64 paramNum = 0;  /* number of all types of parameters processed so far */
  int32 nextGeneralParmRegNO = 0;  /* number of integer parameters processed so far */
  int32 nextGeneralReturnRegNO = 0;  /* number of integer return processed so far */
  int32 nextStackArgAdress = 0;
  uint32 nextFloatRegNO = 0;
  uint32 nextFloatRetRegNO = 0;
};
}  /* namespace maplebe */

#endif  /* MAPLEBE_INCLUDE_CG_X64_X64_CALL_CONV_H */
