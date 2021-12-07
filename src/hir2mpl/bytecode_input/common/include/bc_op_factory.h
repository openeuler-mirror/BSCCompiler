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
#ifndef MPL_FE_BC_INPUT_BC_OP_FACTORY_H
#define MPL_FE_BC_INPUT_BC_OP_FACTORY_H
#include <map>
#include <functional>
#include "mempool_allocator.h"
#include "types_def.h"
#include "bc_instruction.h"

namespace maple {
namespace bc {
class BCOpFactory {
 public:
  template<typename T, typename U, U opcode, uint16 kind, bool wide, bool throwable>
  static BCInstruction *BCOpGenerator(MapleAllocator &allocator, uint32 pc) {
    MemPool *mp = allocator.GetMemPool();
    BCInstruction *op = mp->New<T>(allocator, pc, opcode);
    op->InitBCInStruction(kind, wide, throwable);
    return op;
  }
  using funcPtr = BCInstruction*(*)(MapleAllocator&, uint32);
};
}  // namespace bc
}  // namespace maple
#endif // MPL_FE_BC_INPUT_BC_OP_FACTORY_H