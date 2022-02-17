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
#ifndef MAPLEBE_INCLUDE_CG_CALL_CONV_H
#define MAPLEBE_INCLUDE_CG_CALL_CONV_H

#include "types_def.h"
#include "becommon.h"

namespace maplebe {
using namespace maple;

/* for specifying how a parameter is passed */
struct CCLocInfo {
  regno_t reg0 = 0;  /* 0 means parameter is stored on the stack */
  regno_t reg1 = 0;
  regno_t reg2 = 0;  /* can have up to 4 single precision fp registers */
  regno_t reg3 = 0;  /* for small structure return. */
  int32 memOffset = 0;
  int32 memSize = 0;
  uint32 fpSize = 0;
  uint32 numFpPureRegs = 0;
  uint8 regCount = 0;             /* number of registers <= 2 storing the return value */
  PrimType primTypeOfReg0;    /* the primitive type stored in reg0 */
  PrimType primTypeOfReg1;    /* the primitive type stored in reg1 */

  uint8 GetRegCount() const {
    return regCount;
  }

  PrimType GetPrimTypeOfReg0() const {
    return primTypeOfReg0;
  }

  regno_t GetReg0() const {
    return reg0;
  }

  regno_t GetReg1() const {
    return reg1;
  }

  regno_t GetReg2() const {
    return reg2;
  }

  regno_t GetReg3() const {
    return reg3;
  }
};
}  /* namespace maplebe */

#endif  /* MAPLEBE_INCLUDE_CG_CALL_CONV_H */
