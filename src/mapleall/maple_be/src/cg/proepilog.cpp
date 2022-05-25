/*
 * Copyright (c) [2020] Huawei Technologies Co.,Ltd.All rights reserved.
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
#include "proepilog.h"
#if TARGAARCH64
#include "aarch64_proepilog.h"
#elif TARGRISCV64
#include "riscv64_proepilog.h"
#endif
#if TARGARM32
#include "arm32_proepilog.h"
#endif
#if TARGX86_64
#include "x64_proepilog.h"
#endif
#include "cgfunc.h"
#include "cg.h"

namespace maplebe {
using namespace maple;

Insn *GenProEpilog::InsertCFIDefCfaOffset(int32 &cfiOffset, Insn &insertAfter) {
  if (cgFunc.GenCfi() == false) {
    return &insertAfter;
  }
  CG *currCG = cgFunc.GetCG();
  ASSERT(currCG != nullptr, "get cg failed in InsertCFIDefCfaOffset");
  cfiOffset = AddtoOffsetFromCFA(cfiOffset);
  Insn &cfiInsn = currCG->BuildInstruction<cfi::CfiInsn>(cfi::OP_CFI_def_cfa_offset,
                                                         cgFunc.CreateCfiImmOperand(cfiOffset, k64BitSize));
  Insn *newIPoint = cgFunc.GetCurBB()->InsertInsnAfter(insertAfter, cfiInsn);
  cgFunc.SetDbgCallFrameOffset(cfiOffset);
  return newIPoint;
}

/* there are two stack protector:
 * 1. stack protector all: for all function
 * 2. stack protector strong: for some functon that
 *   <1> invoke alloca functon;
 *   <2> use stack address;
 *   <3> callee use return stack slot;
 *   <4> local symbol is vector type;
 * */
void GenProEpilog::NeedStackProtect() {
  ASSERT(stackProtect == false, "no stack protect default");
  CG *currCG = cgFunc.GetCG();
  if (currCG->IsStackProtectorAll()) {
    stackProtect = true;
    return;
  }

  if (!currCG->IsStackProtectorStrong()) {
    return;
  }

  if (cgFunc.HasAlloca()) {
    stackProtect = true;
    return;
  }

  /* check if function use stack address or callee function return stack slot */
  auto stackProtectInfo = cgFunc.GetStackProtectInfo();
  if ((stackProtectInfo & kAddrofStack) != 0 || (stackProtectInfo & kRetureStackSlot) != 0) {
    stackProtect = true;
    return;
  }

  /* check if local symbol is vector type */
  auto &mirFunction = cgFunc.GetFunction();
  uint32 symTabSize = mirFunction.GetSymTab()->GetSymbolTableSize();
  for (uint32 i = 0; i < symTabSize; ++i) {
    MIRSymbol *symbol = mirFunction.GetSymTab()->GetSymbolFromStIdx(i);
    if (symbol == nullptr || symbol->GetStorageClass() != kScAuto || symbol->IsDeleted()) {
      continue;
    }
    TyIdx tyIdx = symbol->GetTyIdx();
    MIRType *type = GlobalTables::GetTypeTable().GetTypeFromTyIdx(tyIdx);
    if (type->GetKind() == kTypeArray) {
      stackProtect = true;
      return;
    }

    if (type->IsStructType() && IncludeArray(*type)) {
      stackProtect = true;
      return;
    }
  }
}

bool GenProEpilog::IncludeArray(const MIRType &type) const {
  ASSERT(type.IsStructType(), "agg must be one of class/struct/union");
  auto &structType = static_cast<const MIRStructType&>(type);
  /* all elements of struct. */
  auto num = static_cast<uint8>(structType.GetFieldsSize());
  for (uint32 i = 0; i < num; ++i) {
    MIRType *elemType = structType.GetElemType(i);
    if (elemType->GetKind() == kTypeArray) {
      return true;
    }
    if (elemType->IsStructType() && IncludeArray(*elemType)) {
      return true;
    }
  }
  return false;
}

bool CgGenProEpiLog::PhaseRun(maplebe::CGFunc &f) {
  GenProEpilog *genPE = nullptr;
#if TARGAARCH64 || TARGRISCV64
  genPE = GetPhaseAllocator()->New<AArch64GenProEpilog>(f, *ApplyTempMemPool());
#endif
#if TARGARM32
  genPE = GetPhaseAllocator()->New<Arm32GenProEpilog>(f);
#endif
#if TARGX86_64
  genPE = GetPhaseAllocator()->New<X64GenProEpilog>(f);
#endif
  genPE->Run();
  return false;
}
}  /* namespace maplebe */
