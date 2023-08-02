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

#ifndef MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_UTILS_H
#define MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_UTILS_H

#include "aarch64_cg.h"
#include "aarch64_operand.h"
#include "aarch64_cgfunc.h"

namespace maplebe {

/**
 * Get or create new memory operand for load instruction loadIns for which
 * machine opcode will be replaced with newLoadMop.
 *
 * @param loadIns load instruction
 * @param newLoadMop new opcode for load instruction
 * @return memory operand for new load machine opcode
 *         or nullptr if memory operand can't be obtained
 */
MemOperand *GetOrCreateMemOperandForNewMOP(CGFunc &cgFunc,
    const Insn &loadIns, MOperator newLoadMop);
} // namespace maplebe

#endif // MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_UTILS_H
