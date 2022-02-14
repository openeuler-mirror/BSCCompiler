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
#include "aarch64_utils.h"
#include "cg_option.h"

namespace maplebe {

AArch64MemOperand *GetOrCreateMemOperandForNewMOP(CGFunc &cgFunc,
                                                  const Insn &loadIns,
                                                  MOperator newLoadMop) {
  MemPool &memPool = *cgFunc.GetMemoryPool();
  auto *memOp = static_cast<AArch64MemOperand *>(loadIns.GetMemOpnd());
  MOperator loadMop = loadIns.GetMachineOpcode();

  ASSERT(loadIns.IsLoad() && AArch64CG::kMd[newLoadMop].IsLoad(),
         "ins and Mop must be load");

  AArch64MemOperand *newMemOp = memOp;

  uint32 memSize = AArch64CG::kMd[loadMop].GetOperandSize();
  uint32 newMemSize = AArch64CG::kMd[newLoadMop].GetOperandSize();

  if (newMemSize == memSize) {
    // if sizes are the same just return old memory operand
    return newMemOp;
  }

  newMemOp = static_cast<AArch64MemOperand *>(memOp->Clone(memPool));
  newMemOp->SetSize(newMemSize);

  if (!CGOptions::IsBigEndian()) {
    return newMemOp;
  }

  // for big-endian it's necessary to adjust offset if it's present
  if (memOp->GetAddrMode() != AArch64MemOperand::kAddrModeBOi ||
      newMemSize > memSize) {
    // currently, it's possible to adjust an offset only for immediate offset
    // operand if new size is less than the original one
    return nullptr;
  }

  auto *newOffOp = static_cast<AArch64OfstOperand *>(
      memOp->GetOffsetImmediate()->Clone(memPool));

  newOffOp->AdjustOffset((memSize - newMemSize) >> kLog2BitsPerByte);
  newMemOp->SetOffsetImmediate(*newOffOp);

  ASSERT(memOp->IsOffsetMisaligned(memSize) ||
             !newMemOp->IsOffsetMisaligned(newMemSize),
         "New offset value is misaligned!");

  return newMemOp;
}

} // namespace maplebe
