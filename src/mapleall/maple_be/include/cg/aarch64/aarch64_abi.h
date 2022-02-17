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
}  /* namespace maplebe */

#endif  /* MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_ABI_H */
