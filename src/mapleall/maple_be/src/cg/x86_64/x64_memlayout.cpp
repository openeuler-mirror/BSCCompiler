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
#include "x64_memlayout.h"
#include "x64_cgfunc.h"
#include "becommon.h"
#include "mir_nodes.h"

namespace maplebe {
using namespace maple;
void X64MemLayout::SetSizeAlignForTypeIdx(uint32 typeIdx, uint32 &size, uint32 &align) const {
  if (be.GetTypeSize(typeIdx) > k16ByteSize) {
    /* size > 16 is passed on stack, the formal is just a pointer to the copy on stack. */
    align = kSizeOfPtr;
    size = kSizeOfPtr;
  } else {
    align = be.GetTypeAlign(typeIdx);
    size = static_cast<uint32>(be.GetTypeSize(typeIdx));
  }
}

void X64MemLayout::LayoutFormalParams() {
  for (size_t i = 0; i < mirFunction->GetFormalCount(); ++i) {
    MIRSymbol *sym = mirFunction->GetFormal(i);
    uint32 stIndex = sym->GetStIndex();
    X64SymbolAlloc *symLoc = memAllocator->GetMemPool()->New<X64SymbolAlloc>();
    SetSymAllocInfo(stIndex, *symLoc);
    if (i == 0) {
     if (be.HasFuncReturnType(*mirFunction)) {
       symLoc->SetMemSegment(GetSegArgsRegPassed());
       symLoc->SetOffset(GetSegArgsRegPassed().GetSize());
       TyIdx tidx = be.GetFuncReturnType(*mirFunction);
       if (be.GetTypeSize(tidx.GetIdx()) > k16ByteSize) {
         segArgsRegPassed.SetSize(segArgsRegPassed.GetSize() + kSizeOfPtr);
       }
       continue;
     }
    }

    MIRType *ty = mirFunction->GetNthParamType(i);
    uint32 ptyIdx = ty->GetTypeIndex();
    if (!sym->IsPreg()) {
      uint32 size;
      uint32 align;
      SetSizeAlignForTypeIdx(ptyIdx, size, align);
      symLoc->SetMemSegment(GetSegArgsRegPassed());
      segArgsRegPassed.SetSize(static_cast<uint32>(RoundUp(segArgsRegPassed.GetSize(), align)));
      symLoc->SetOffset(segArgsRegPassed.GetSize());
      segArgsRegPassed.SetSize(segArgsRegPassed.GetSize() + size);
    }
  }
}

void X64MemLayout::LayoutLocalVariables() {
  uint32 symTabSize = mirFunction->GetSymTab()->GetSymbolTableSize();
  for (uint32 i = 0; i < symTabSize; ++i) {
    MIRSymbol *sym = mirFunction->GetSymTab()->GetSymbolFromStIdx(i);
    if (sym == nullptr || sym->GetStorageClass() != kScAuto || sym->IsDeleted()) {
      continue;
    }
    uint32 stIndex = sym->GetStIndex();
    TyIdx tyIdx = sym->GetTyIdx();
    X64SymbolAlloc *symLoc = memAllocator->GetMemPool()->New<X64SymbolAlloc>();
    SetSymAllocInfo(stIndex, *symLoc);
    CHECK_FATAL(!symLoc->IsRegister(), "expect not register");

    symLoc->SetMemSegment(segLocals);
    uint32 align = be.GetTypeAlign(tyIdx);
    segLocals.SetSize(static_cast<uint32>(RoundUp(segLocals.GetSize(), align)));
    symLoc->SetOffset(segLocals.GetSize());
    segLocals.SetSize(segLocals.GetSize() + be.GetTypeSize(tyIdx));
  }
}

void X64MemLayout::LayoutStackFrame(int32 &structCopySize, int32 &maxParmStackSize) {
  LayoutFormalParams();

  // Need to be aligned ?
  segArgsRegPassed.SetSize(RoundUp(segArgsRegPassed.GetSize(), kSizeOfPtr));
  segArgsStkPassed.SetSize(RoundUp(segArgsStkPassed.GetSize(), kSizeOfPtr + kSizeOfPtr));

  /* allocate the local variables in the stack */
  LayoutLocalVariables();

  // TODO:Need to adapt to the cc interface.
  structCopySize = 0;
  // TODO:Scenes with more than 6 parameters are not yet enabled.
  maxParmStackSize = 0;

  cgFunc->SetUseFP(cgFunc->UseFP());
}

uint64 X64MemLayout::StackFrameSize() const {
  uint64 total = locals().GetSize();
  constexpr int kX64StackPtrAlignment = 16;

  return RoundUp(total, kX64StackPtrAlignment);
}
}  /* namespace maplebe */
