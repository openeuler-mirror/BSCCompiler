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

MemOperand *GetOrCreateMemOperandForNewMOP(CGFunc &cgFunc,
                                           const Insn &loadIns,
                                           MOperator newLoadMop) {
  MemPool &memPool = *cgFunc.GetMemoryPool();
  auto *memOp = static_cast<MemOperand *>(loadIns.GetMemOpnd());
  CHECK_FATAL(memOp != nullptr, "memOp should not be nullptr");
  MOperator loadMop = loadIns.GetMachineOpcode();

  ASSERT(loadIns.IsLoad() && AArch64CG::kMd[newLoadMop].IsLoad(),
         "ins and Mop must be load");

  MemOperand *newMemOp = memOp;

  uint32 memSize = AArch64CG::kMd[loadMop].GetOperandSize();
  uint32 newMemSize = AArch64CG::kMd[newLoadMop].GetOperandSize();
  if (newMemSize == memSize) {
    // if sizes are the same just return old memory operand
    return newMemOp;
  }
  newMemOp = memOp->Clone(memPool);
  newMemOp->SetSize(newMemSize);

  if (!CGOptions::IsBigEndian()) {
    return newMemOp;
  }

  // for big-endian it's necessary to adjust offset if it's present
  if (memOp->GetAddrMode() != MemOperand::kBOI ||
      newMemSize > memSize) {
    // currently, it's possible to adjust an offset only for immediate offset
    // operand if new size is less than the original one
    return nullptr;
  }

  auto *newOffOp = static_cast<OfstOperand *>(
      memOp->GetOffsetImmediate()->Clone(memPool));

  newOffOp->AdjustOffset(static_cast<int32>((memSize - newMemSize) >> kLog2BitsPerByte));
  newMemOp->SetOffsetOperand(*newOffOp);

  ASSERT(memOp->IsOffsetMisaligned(memSize) ||
         !newMemOp->IsOffsetMisaligned(newMemSize),
         "New offset value is misaligned!");

  return newMemOp;
}

} // namespace maplebe
