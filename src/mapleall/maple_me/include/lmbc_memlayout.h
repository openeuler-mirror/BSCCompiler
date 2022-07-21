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
#ifndef MAPLEME_INCLUDE_LMBC_MEMLAYOUT_H
#define MAPLEME_INCLUDE_LMBC_MEMLAYOUT_H

#include "mir_module.h"
#include "mir_type.h"
#include "mir_const.h"
#include "mir_symbol.h"
#include "mir_function.h"
#include "becommon.h"

namespace maple {

typedef enum {
  MS_unknown,
  MS_upformal,  // for the incoming parameters that are passed on the caller's stack
  MS_formal,    // for the incoming parameters that are passed in registers
  MS_actual,    // for the outgoing parameters
  MS_local,     // for all local variables and temporaries
  MS_FPbased,   // addressed via offset from the frame pointer
  MS_GPbased,   // addressed via offset from the global pointer
  MS_largeStructActual, // for storing large struct actuals passed by value for ARM CPU
} MemSegmentKind;

class MemSegment;

// describes where a symbol is allocated
struct SymbolAlloc {
  MemSegment *mem_segment = nullptr;
  int32 offset = 0;
};  // class SymbolAlloc

// keeps track of the allocation of a memory segment
class MemSegment {
 public:
  explicit MemSegment(MemSegmentKind k) : kind(k), size(0) {}

  MemSegment(MemSegmentKind k, int32 sz) : kind(k), size(sz) {}

  ~MemSegment() {}

  MemSegmentKind kind;
  int32 size;             // size is negative if allocated offsets are negative
};  // class MemSegment

class LMBCMemLayout {
 public:
  LMBCMemLayout(MIRFunction *f, MemSegment *segGP, MapleAllocator *mallocator)
      : func(f),
        seg_GPbased(segGP),
        seg_FPbased(MS_FPbased),
        sym_alloc_table(mallocator->Adapter()) {
    sym_alloc_table.resize(f->GetSymTab()->GetSymbolTableSize());
  }

  ~LMBCMemLayout() {
    func = nullptr;
  }

  void LayoutStackFrame(void);
  int32 StackFrameSize(void) const {
    return -seg_FPbased.size;
  }

  MIRFunction *func;
  MemSegment *seg_GPbased;
  MemSegment seg_FPbased;
  MapleVector<SymbolAlloc> sym_alloc_table;  // index is StIdx
};

class GlobalMemLayout {
 public:
  GlobalMemLayout(MIRModule *mod, MapleAllocator *mallocator);
  ~GlobalMemLayout() {}

  MemSegment seg_GPbased;
  MapleVector<SymbolAlloc> sym_alloc_table;  // index is StIdx
  MIRModule *mirModule;
};

}  /* namespace maple */

#endif  /* MAPLEME_INCLUDE_LMBC_MEMLAYOUT_H */
