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
#ifndef MAPLEBE_INCLUDE_CG_X64_X64_ABI_H
#define MAPLEBE_INCLUDE_CG_X64_X64_ABI_H

#include "x64_isa.h"
#include "types_def.h"
#include "becommon.h"

namespace maplebe {
using namespace maple;

namespace x64 {
constexpr int32 kNumIntParmRegs = 6;
constexpr int32 kNumIntReturnRegs = 2;
constexpr int32 kNumFloatParmRegs = 8;
constexpr int32 kNumFloatReturnRegs = 2;

constexpr uint32 kNormalUseOperandNum = 2;

constexpr X64reg kIntParmRegs[kNumIntParmRegs] = { R7, R6, R3, R2, R8, R9 };
constexpr X64reg kIntReturnRegs[kNumIntReturnRegs] = { R0, R3 };
constexpr X64reg kFloatParmRegs[kNumFloatParmRegs] = { V0, V1, V2, V3, V4, V5, V6, V7 };
constexpr X64reg kFloatReturnRegs[kNumFloatReturnRegs] = { V0, V1 };

/*
 * Refer to：
 * x64-bit Architecture.
 */
bool IsAvailableReg(X64reg reg);
bool IsCalleeSavedReg(X64reg reg);
bool IsCallerSaveReg(X64reg reg);
bool IsParamReg(X64reg reg);
bool IsSpillReg(X64reg reg);
bool IsExtraSpillReg(X64reg reg);
bool IsSpillRegInRA(X64reg regNO, bool has3RegOpnd);
PrimType IsVectorArrayType(MIRType *ty, uint32 &arraySize);
}  /* namespace x64 */

/*
 * X64-bit Architecture.
 * After the argument values have been computed, they are placed either in registers
 * or pushed on the stack. The way how values are passed is described in the
 * following sections.
 *   - INTEGER This class consists of integral types that fit into one of the general
       purpose registers.
     - SSE The class consists of types that fit into a vector register.
     - SSEUP The class consists of types that fit into a vector register and can be passed
       and returned in the upper bytes of it.
     - X87, X87UP These classes consists of types that will be returned via the x87 FPU.
     - COMPLEX_X87 This class consists of types that will be returned via the x87 FPU.
     - NO_CLASS This class is used as initializer in the algorithms. It will be used for
       padding and empty structures and unions.
     - MEMORY This class consists of types that will be passed and returned in memory via the stack.
 *
 */
}  /* namespace maplebe */

#endif  /* MAPLEBE_INCLUDE_CG_X64_X64_ABI_H */
