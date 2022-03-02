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
#include "x64_isa.h"

namespace maplebe {
using namespace maple;
namespace x64 {
bool IsAvailableReg(X64reg reg) {
  switch (reg) {
/* integer registers */
#define INT_REG(ID, PREF8, PREF8_16, PREF16, PREF32, PREF64, canBeAssigned, isCalleeSave, \
    isParam, isSpill, isExtraSpill) \
    case R##ID:                                                                           \
      return canBeAssigned;
#define INT_REG_ALIAS(ALIAS, ID)
#include "x64_int_regs.def"
#undef INT_REG
#undef INT_REG_ALIAS
/* fp-simd registers */
#define FP_SIMD_REG(ID, P8, P16, P32, P64, P128, canBeAssigned, isCalleeSave, \
    isParam, isSpill, isExtraSpill) \
    case V##ID:                                                                   \
      return canBeAssigned;
#undef FP_SIMD_REG
    default:
      return false;
  }
}

bool IsCallerSaveReg(X64reg regNO) {
  return (regNO == R0) || (regNO == R4) || (R2 <= regNO && regNO <= R3) ||
    (R6 <= regNO && regNO <= R7) || (R8 <= regNO && regNO <= R11) ||
    (V2 <= regNO && regNO <= V7) || (V16 <= regNO && regNO <= V23);
}

bool IsCalleeSavedReg(X64reg reg) {
  switch (reg) {
/* integer registers */
#define INT_REG(ID, PREF8, PREF8_16, PREF16, PREF32, PREF64, canBeAssigned, isCalleeSave, \
    isParam, isSpill, isExtraSpill) \
    case R##ID:                                                                           \
      return isCalleeSave;
#define INT_REG_ALIAS(ALIAS, ID)
#include "x64_int_regs.def"
#undef INT_REG
#undef INT_REG_ALIAS
/* fp-simd registers */
#define FP_SIMD_REG(ID, P8, P16, P32, P64, P128, canBeAssigned, isCalleeSave, \
    isParam, isSpill, isExtraSpill) \
    case V##ID:                                                                   \
      return isCalleeSave;
#undef FP_SIMD_REG
    default:
      return false;
  }
}

bool IsParamReg(X64reg reg) {
  switch (reg) {
/* integer registers */
#define INT_REG(ID, PREF8, PREF8_16, PREF16, PREF32, PREF64, canBeAssigned, isCalleeSave, \
    isParam, isSpill, isExtraSpill) \
    case R##ID:                                                                           \
      return isParam;
#define INT_REG_ALIAS(ALIAS, ID)
#include "x64_int_regs.def"
#undef INT_REG
#undef INT_REG_ALIAS
/* fp-simd registers */
#define FP_SIMD_REG(ID, P8, P16, P32, P64, P128, canBeAssigned, isCalleeSave, \
    isParam, isSpill, isExtraSpill) \
    case V##ID:                                                                   \
      return isParam;
#undef FP_SIMD_REG
    default:
      return false;
  }
}

bool IsSpillReg(X64reg reg) {
  switch (reg) {
/* integer registers */
#define INT_REG(ID, PREF8, PREF8_16, PREF16, PREF32, PREF64, canBeAssigned, isCalleeSave, \
    isParam, isSpill, isExtraSpill) \
    case R##ID:                                                                           \
      return isSpill;
#define INT_REG_ALIAS(ALIAS, ID)
#include "x64_int_regs.def"
#undef INT_REG
#undef INT_REG_ALIAS
/* fp-simd registers */
#define FP_SIMD_REG(ID, P8, P16, P32, P64, P128, canBeAssigned, isCalleeSave, \
    isParam, isSpill, isExtraSpill) \
    case V##ID:                                                                   \
      return isSpill;
#undef FP_SIMD_REG
    default:
      return false;
  }
}

bool IsExtraSpillReg(X64reg reg) {
  switch (reg) {
/* integer registers */
#define INT_REG(ID, PREF8, PREF8_16, PREF16, PREF32, PREF64, canBeAssigned, isCalleeSave, \
    isParam, isSpill, isExtraSpill) \
    case R##ID:                                                                           \
      return isExtraSpill;
#define INT_REG_ALIAS(ALIAS, ID)
#include "x64_int_regs.def"
#undef INT_REG
#undef INT_REG_ALIAS
/* fp-simd registers */
#define FP_SIMD_REG(ID, P8, P16, P32, P64, P128, canBeAssigned, isCalleeSave, \
    isParam, isSpill, isExtraSpill) \
    case V##ID:                                                                   \
      return isExtraSpill;
#undef FP_SIMD_REG
    default:
      return false;
  }
}

bool IsSpillRegInRA(X64reg regNO, bool has3RegOpnd) {
  /* if has 3 RegOpnd, previous reg used to spill. */
  if (has3RegOpnd) {
    return IsSpillReg(regNO) || IsExtraSpillReg(regNO);
  }
  return IsSpillReg(regNO);
}
}  /* namespace x64 */
}  /* namespace maplebe */
