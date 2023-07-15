/*
 * Copyright (c) [2023] Huawei Technologies Co.,Ltd.All rights reserved.
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
#ifndef MAPLE_ME_INCLUDE_EXPR_UTIL_H
#define MAPLE_ME_INCLUDE_EXPR_UTIL_H
#include "bit_value.h"
#include "me_ir.h"

namespace maple {

const MIRIntConst *GetIntConst(const MeExpr &expr);
std::pair<const MIRIntConst *, uint8_t> GetIntConstOpndOfBinExpr(const MeExpr &expr);

MeExpr *SimplifyMultiUseByDemanded(IRMap &irMap, const MeExpr &expr);
MeExpr *SimplifyBinOpByDemanded(IRMap &irMap, const MeExpr &expr, const IntVal &demanded, BitValue &known,
                                const BitValue &lhsKnown, const BitValue &rhsKnown);
MeExpr *SimplifyMultiUseByDemanded(IRMap &irMap, const MeExpr &expr, const IntVal &demanded, BitValue &known,
                                   uint32 depth);
}  // namespace maple
#endif