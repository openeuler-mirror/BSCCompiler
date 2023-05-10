/*
 * Copyright (c) [2022] Futurewei Technologies Co., Ltd. All rights reserved.
 *
 * OpenArkCompiler is licensed under the Mulan Permissive Software License v2.
 * You can use this software according to the terms and conditions of the MulanPSL - 2.0.
 * You may obtain a copy of MulanPSL - 2.0 at:
 *
 *   https://opensource.org/licenses/MulanPSL-2.0
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR
 * FIT FOR A PARTICULAR PURPOSE.
 * See the MulanPSL - 2.0 for more details.
 */

// For each function being compiled, lay out its local variables on its stack
// frame.  Allocate all the global variables within the global memory block
// which is addressed via offset from the global pointer GP during execution.
// Allocate this block pointed to by mirModule.globalBlkMap.

#include "lmbc_memlayout.h"
#include "mir_symbol.h"

namespace maple {

constexpr size_t kVarargSaveAreaSize = 192;

void LMBCMemLayout::LayoutStackFrame(void) {
  if (func->IsVarargs()) {
    seg_FPbased.size -= kVarargSaveAreaSize;
  }

  // allocate the local variables
  size_t symtabsize = func->GetSymTab()->GetSymbolTableSize();
  for (size_t i = 0; i < symtabsize; i++) {
    MIRSymbol *sym = func->GetSymTab()->GetSymbolFromStIdx(i);
    if (!sym) {
      continue;
    }
    if (sym->IsDeleted()) {
      continue;
    }
    if (sym->GetStorageClass() == kScPstatic && sym->LMBCAllocateOffSpecialReg()) {
      uint32 stindex = sym->GetStIndex();
      sym_alloc_table[stindex].memSegment = seg_GPbased;
      seg_GPbased->size = static_cast<int32>(maplebe::RoundUp(seg_GPbased->size, sym->GetType()->GetAlign()));
      sym_alloc_table[stindex].offset = seg_GPbased->size;
      seg_GPbased->size += static_cast<int32>(sym->GetType()->GetSize());
    }
    if (sym->GetStorageClass() != kScAuto) {
      continue;
    }
    uint32 stindex = sym->GetStIndex();
    sym_alloc_table[stindex].memSegment = &seg_FPbased;
    seg_FPbased.size -= static_cast<int32>(sym->GetType()->GetSize());
    seg_FPbased.size = static_cast<int32>(maplebe::RoundDown(seg_FPbased.size, sym->GetType()->GetAlign()));
    sym_alloc_table[stindex].offset = seg_FPbased.size;
  }
}

GlobalMemLayout::GlobalMemLayout(MIRModule *mod, MapleAllocator *mallocator)
    : seg_GPbased(MS_GPbased), sym_alloc_table(mallocator->Adapter()), mirModule(mod) {
  size_t symtabsize = GlobalTables::GetGsymTable().GetSymbolTableSize();
  sym_alloc_table.resize(symtabsize);
  MIRSymbol *sym = nullptr;
  // allocate the global variables ordered based on alignments
  for (uint32 curalign = 8; curalign != 0; curalign >>= 1) {
    for (size_t i = 0; i < symtabsize; i++) {
      sym = GlobalTables::GetGsymTable().GetSymbolFromStidx(i);
      if (!sym) {
        continue;
      }
      if (sym->GetStorageClass() != kScGlobal && sym->GetStorageClass() != kScFstatic) {
        continue;
      }
      if (!sym->LMBCAllocateOffSpecialReg()) {
        continue;
      }
      if (sym->GetType()->GetAlign() != curalign) {
        continue;
      }
      uint32 stindex = sym->GetStIndex();
      sym_alloc_table[stindex].memSegment = &seg_GPbased;
      seg_GPbased.size = static_cast<int32>(maplebe::RoundUp(seg_GPbased.size, sym->GetType()->GetAlign()));
      sym_alloc_table[stindex].offset = seg_GPbased.size;
      seg_GPbased.size += static_cast<int32>(sym->GetType()->GetSize());
    }
  }
  seg_GPbased.size = static_cast<int32>(maplebe::RoundUp(seg_GPbased.size, GetPrimTypeSize(PTY_ptr)));
  mirModule->SetGlobalMemSize(seg_GPbased.size);
}

}  // namespace maple
