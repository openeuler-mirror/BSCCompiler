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
using namespace x64;

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
        intParmRegs[nextGeneralParmRegNO++] : kRinvalid;
  }

  void AllocateTwoGPParmRegisters(CCLocInfo &pLoc) {
    if ((nextGeneralParmRegNO + 1) < kNumIntParmRegs) {
      pLoc.reg0 = intParmRegs[nextGeneralParmRegNO++];
      pLoc.reg1 = intParmRegs[nextGeneralParmRegNO++];
    } else {
      pLoc.reg0 = kRinvalid;
    }
  }

  X64reg AllocateGPReturnRegister() {
    return (nextGeneralReturnRegNO < kNumIntReturnRegs) ?
        intReturnRegs[nextGeneralReturnRegNO++] : kRinvalid;
  }

  void AllocateTwoGPReturnRegisters(CCLocInfo &pLoc) {
    if ((nextGeneralReturnRegNO + 1) < kNumIntReturnRegs) {
      pLoc.reg0 = intReturnRegs[nextGeneralReturnRegNO++];
      pLoc.reg1 = intReturnRegs[nextGeneralReturnRegNO++];
    } else {
      pLoc.reg0 = kRinvalid;
    }
  }

  BECommon &beCommon;
  uint64 paramNum = 0;  /* number of all types of parameters processed so far */
  int32 nextGeneralParmRegNO = 0;  /* number of integer parameters processed so far */
  int32 nextGeneralReturnRegNO = 0;  /* number of integer return processed so far */
  int32 nextStackArgAdress = 0;
};
}  /* namespace maplebe */

#endif  /* MAPLEBE_INCLUDE_CG_X64_X64_CALL_CONV_H */