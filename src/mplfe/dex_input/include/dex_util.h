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
#ifndef MPL_FE_BC_INPUT_DEX_UTIL_H
#define MPL_FE_BC_INPUT_DEX_UTIL_H
#include <map>
#include "dex_op.h"
#include "bc_instruction.h"
#include "types_def.h"
namespace maple {
namespace bc {
static std::map<uint8, Opcode> dexOp2MIROp = {
    { kDexOpAddInt, OP_add },
    { kDexOpSubInt, OP_sub },
    { kDexOpMulInt, OP_mul },
    { kDexOpDivInt, OP_div },
    { kDexOpRemInt, OP_rem },
    { kDexOpAndInt, OP_band },
    { kDexOpOrInt, OP_bior },
    { kDexOpXorInt, OP_bxor },
    { kDexOpShlInt, OP_shl },
    { kDexOpShrInt, OP_ashr },
    { kDexOpUshrInt, OP_lshr },

    { kDexOpAddLong, OP_add },
    { kDexOpSubLong, OP_sub },
    { kDexOpMulLong, OP_mul },
    { kDexOpDivLong, OP_div },
    { kDexOpRemLong, OP_rem },
    { kDexOpAndLong, OP_band },
    { kDexOpOrLong, OP_bior },
    { kDexOpXorLong, OP_bxor },
    { kDexOpShlLong, OP_shl },
    { kDexOpShrLong, OP_ashr },
    { kDexOpUshrLong, OP_lshr },

    { kDexOpAddFloat, OP_add },
    { kDexOpSubFloat, OP_sub },
    { kDexOpMulFloat, OP_mul },
    { kDexOpDivFloat, OP_div },
    { kDexOpRemFloat, OP_rem },
    { kDexOpAddDouble, OP_add },
    { kDexOpSubDouble, OP_sub },
    { kDexOpMulDouble, OP_mul },
    { kDexOpDivDouble, OP_div },
    { kDexOpRemDouble, OP_rem },
};

static std::map<uint8, Opcode> dexOp2Addr2MIROp = {
    { kDexOpAddInt2Addr, OP_add },
    { kDexOpSubInt2Addr, OP_sub },
    { kDexOpMulInt2Addr, OP_mul },
    { kDexOpDivInt2Addr, OP_div },
    { kDexOpRemInt2Addr, OP_rem },
    { kDexOpAndInt2Addr, OP_band },
    { kDexOpOrInt2Addr, OP_bior },
    { kDexOpXorInt2Addr, OP_bxor },
    { kDexOpShlInt2Addr, OP_shl },
    { kDexOpShrInt2Addr, OP_ashr },
    { kDexOpUshrInt2Addr, OP_lshr },

    { kDexOpAddLong2Addr, OP_add },
    { kDexOpSubLong2Addr, OP_sub },
    { kDexOpMulLong2Addr, OP_mul },
    { kDexOpDivLong2Addr, OP_div },
    { kDexOpRemLong2Addr, OP_rem },
    { kDexOpAndLong2Addr, OP_band },
    { kDexOpOrLong2Addr, OP_bior },
    { kDexOpXorLong2Addr, OP_bxor },
    { kDexOpShlLong2Addr, OP_shl },
    { kDexOpShrLong2Addr, OP_ashr },
    { kDexOpUshrLong2Addr, OP_lshr },

    { kDexOpAddFloat2Addr, OP_add },
    { kDexOpSubFloat2Addr, OP_sub },
    { kDexOpMulFloat2Addr, OP_mul },
    { kDexOpDivFloat2Addr, OP_div },
    { kDexOpRemFloat2Addr, OP_rem },
    { kDexOpAddDouble2Addr, OP_add },
    { kDexOpSubDouble2Addr, OP_sub },
    { kDexOpMulDouble2Addr, OP_mul },
    { kDexOpDivDouble2Addr, OP_div },
    { kDexOpRemDouble2Addr, OP_rem },
};

static std::map<uint8, Opcode> dexOpCmp2MIROp = {
    { kDexOpCmpLong, OP_cmp },
    { kDexOpCmplFloat, OP_cmpl },
    { kDexOpCmpgFloat, OP_cmpg },
    { kDexOpCmplDouble, OP_cmpl },
    { kDexOpCmpgDouble, OP_cmpg },
};

static std::map<uint8, Opcode> dexOpLit2MIROp = {
    { kDexOpAddIntLit16, OP_add },
    { kDexOpRsubInt, OP_sub },
    { kDexOpMulIntLit16, OP_mul },
    { kDexOpDivIntLit16, OP_div },
    { kDexOpRemIntLit16, OP_rem },
    { kDexOpAndIntLit16, OP_band },
    { kDexOpOrIntLit16, OP_bior },
    { kDexOpXorIntLit16, OP_bxor },

    { kDexOpAddIntLit8, OP_add },
    { kDexOpRsubIntLit8, OP_sub },
    { kDexOpMulIntLit8, OP_mul },
    { kDexOpDivIntLit8, OP_div },
    { kDexOpRemIntLit8, OP_rem },
    { kDexOpAndIntLit8, OP_band },
    { kDexOpOrIntLit8, OP_bior },
    { kDexOpXorIntLit8, OP_bxor },
    { kDexOpShlIntLit8, OP_shl },
    { kDexOpShrIntLit8, OP_ashr },
    { kDexOpUshrIntLit8, OP_lshr },
};

static std::map<DexOpCode, Opcode> dexOpConditionOp2MIROp = {
    { kDexOpIfEq, OP_eq },
    { kDexOpIfNe, OP_ne },
    { kDexOpIfLt, OP_lt },
    { kDexOpIfGe, OP_ge },
    { kDexOpIfGt, OP_gt },
    { kDexOpIfLe, OP_le },

    { kDexOpIfEqZ, OP_eq },
    { kDexOpIfNeZ, OP_ne },
    { kDexOpIfLtZ, OP_lt },
    { kDexOpIfGeZ, OP_ge },
    { kDexOpIfGtZ, OP_gt },
    { kDexOpIfLeZ, OP_le },
};

static std::map<DexOpCode, Opcode> dexOpInvokeOp2MIROp = {
    { kDexOpInvokeVirtual, OP_virtualcallassigned },
    { kDexOpInvokeVirtualRange, OP_virtualcallassigned },
    { kDexOpInvokeSuper, OP_superclasscallassigned },
    { kDexOpInvokeSuperRange, OP_superclasscallassigned },
    { kDexOpInvokeDirect, OP_callassigned },
    { kDexOpInvokeDirectRange, OP_callassigned },
    { kDexOpInvokeStatic, OP_callassigned },
    { kDexOpInvokeStaticRange, OP_callassigned },
    { kDexOpInvokeInterface, OP_interfacecallassigned },
    { kDexOpInvokeInterfaceRange, OP_interfacecallassigned },
};

class DEXUtil {
 private:
  DEXUtil() = default;
  ~DEXUtil() = default;
};
}  // namespace bc
}  // namespace maple
#endif  // MPL_FE_BC_INPUT_DEX_UTIL_H