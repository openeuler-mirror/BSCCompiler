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
#include "aarch64_memlayout.h"
#include "aarch64_cgfunc.h"
#include "aarch64_rt.h"
#include "becommon.h"
#include "mir_nodes.h"

namespace maplebe {
using namespace maple;

/*
 *  Returns stack space required for a call
 *  which is used to pass arguments that cannot be
 *  passed through registers
 */
uint32 AArch64MemLayout::ComputeStackSpaceRequirementForCall(StmtNode &stmt,  int32 &aggCopySize, bool isIcall) {
  /* instantiate a parm locator */
  ParmLocator parmLocator(be);
  uint32 sizeOfArgsToStkPass = 0;
  size_t i = 0;
  /* An indirect call's first operand is the invocation target */
  if (isIcall) {
    ++i;
  }

  if (std::strcmp(stmt.GetOpName(), "call") == 0) {
    CallNode *callNode = static_cast<CallNode*>(&stmt);
    MIRFunction *fn = GlobalTables::GetFunctionTable().GetFunctionFromPuidx(callNode->GetPUIdx());
    MIRSymbol *symbol = be.GetMIRModule().CurFunction()->GetLocalOrGlobalSymbol(fn->GetStIdx(), false);
    if (symbol->GetName() == "MCC_CallFastNative" || symbol->GetName() == "MCC_CallFastNativeExt" ||
        symbol->GetName() == "MCC_CallSlowNative0" || symbol->GetName() == "MCC_CallSlowNative1" ||
        symbol->GetName() == "MCC_CallSlowNative2" || symbol->GetName() == "MCC_CallSlowNative3" ||
        symbol->GetName() == "MCC_CallSlowNative4" || symbol->GetName() == "MCC_CallSlowNative5" ||
        symbol->GetName() == "MCC_CallSlowNative6" || symbol->GetName() == "MCC_CallSlowNative7" ||
        symbol->GetName() == "MCC_CallSlowNative8" || symbol->GetName() == "MCC_CallSlowNativeExt") {
      ++i;
    }
  }

  aggCopySize = 0;
  for (uint32 anum = 0; i < stmt.NumOpnds(); ++i, ++anum) {
    BaseNode *opnd = stmt.Opnd(i);
    MIRType *ty = nullptr;
    if (opnd->GetPrimType() != PTY_agg) {
      ty = GlobalTables::GetTypeTable().GetTypeTable()[static_cast<uint32>(opnd->GetPrimType())];
    } else {
      Opcode opndOpcode = opnd->GetOpCode();
      ASSERT(opndOpcode == OP_dread || opndOpcode == OP_iread, "opndOpcode should be OP_dread or OP_iread");
      if (opndOpcode == OP_dread) {
        DreadNode *dread = static_cast<DreadNode*>(opnd);
        MIRSymbol *sym = be.GetMIRModule().CurFunction()->GetLocalOrGlobalSymbol(dread->GetStIdx());
        ty = GlobalTables::GetTypeTable().GetTypeFromTyIdx(sym->GetTyIdx());
        if (dread->GetFieldID() != 0) {
          ASSERT(ty->GetKind() == kTypeStruct || ty->GetKind() == kTypeClass ||
                 ty->GetKind() == kTypeUnion, "expect struct or class");
          if (ty->GetKind() == kTypeStruct || ty->GetKind() == kTypeUnion) {
            ty = static_cast<MIRStructType*>(ty)->GetFieldType(dread->GetFieldID());
          } else {
            ty = static_cast<MIRClassType*>(ty)->GetFieldType(dread->GetFieldID());
          }
        }
      } else {
        /* OP_iread */
        IreadNode *iread = static_cast<IreadNode*>(opnd);
        ty = GlobalTables::GetTypeTable().GetTypeFromTyIdx(iread->GetTyIdx());
        ASSERT(ty->GetKind() == kTypePointer, "expect pointer");
        ty = GlobalTables::GetTypeTable().GetTypeFromTyIdx(static_cast<MIRPtrType*>(ty)->GetPointedTyIdx());
        if (iread->GetFieldID() != 0) {
          ASSERT(ty->GetKind() == kTypeStruct || ty->GetKind() == kTypeClass ||
                 ty->GetKind() == kTypeUnion, "expect struct or class");
          if (ty->GetKind() == kTypeStruct || ty->GetKind() == kTypeUnion) {
            ty = static_cast<MIRStructType*>(ty)->GetFieldType(iread->GetFieldID());
          } else {
            ty = static_cast<MIRClassType*>(ty)->GetFieldType(iread->GetFieldID());
          }
        }
      }
    }
    PLocInfo ploc;
    aggCopySize += parmLocator.LocateNextParm(*ty, ploc);
    if (ploc.reg0 != 0) {
      continue;  /* passed in register, so no effect on actual area */
    }
    sizeOfArgsToStkPass = RoundUp(ploc.memOffset + ploc.memSize, kSizeOfPtr);
  }
  return sizeOfArgsToStkPass;
}

void AArch64MemLayout::SetSizeAlignForTypeIdx(uint32 typeIdx, uint32 &size, uint32 &align) const {
  if (be.GetTypeSize(typeIdx) > k16ByteSize) {
    /* size > 16 is passed on stack, the formal is just a pointer to the copy on stack. */
    align = kSizeOfPtr;
    size = kSizeOfPtr;
  } else {
    align = be.GetTypeAlign(typeIdx);
    size = be.GetTypeSize(typeIdx);
  }
}

void AArch64MemLayout::SetSegmentSize(AArch64SymbolAlloc &symbolAlloc, MemSegment &segment, uint32 typeIdx) {
  uint32 size;
  uint32 align;
  SetSizeAlignForTypeIdx(typeIdx, size, align);
  segment.SetSize(static_cast<int32>(RoundUp(static_cast<uint64>(segment.GetSize()), align)));
  symbolAlloc.SetOffset(segment.GetSize());
  segment.SetSize(segment.GetSize() + static_cast<int32>(size));
  segment.SetSize(static_cast<int32>(RoundUp(static_cast<uint64>(segment.GetSize()), kSizeOfPtr)));
}

void AArch64MemLayout::LayoutVarargParams() {
  uint32 nIntRegs = 0;
  uint32 nFpRegs = 0;
  ParmLocator parmlocator(be);
  PLocInfo ploc;
  MIRFunction *func = mirFunction;
  if (be.GetMIRModule().IsCModule() && func->GetAttr(FUNCATTR_varargs)) {
    for (uint32 i = 0; i < func->GetFormalCount(); i++) {
      if (i == 0) {
        if (be.HasFuncReturnType(*func)) {
          TyIdx tidx = be.GetFuncReturnType(*func);
          if (be.GetTypeSize(tidx.GetIdx()) <= k16ByteSize) {
            continue;
          }
        }
      }
      MIRType *ty = func->GetNthParamType(i);
      parmlocator.LocateNextParm(*ty, ploc);
      if (ploc.reg0 != kRinvalid) {
        if (ploc.reg0 >= R0 && ploc.reg0 <= R7) {
          nIntRegs++;
        } else if (ploc.reg0 >= V0 && ploc.reg0 <= V7) {
          nFpRegs++;
        }
      }
      if (ploc.reg1 != kRinvalid) {
        if (ploc.reg1 >= R0 && ploc.reg1 <= R7) {
          nIntRegs++;
        } else if (ploc.reg1 >= V0 && ploc.reg1 <= V7) {
          nFpRegs++;
        }
      }
      if (ploc.reg2 != kRinvalid) {
        if (ploc.reg2 >= R0 && ploc.reg2 <= R7) {
          nIntRegs++;
        } else if (ploc.reg2 >= V0 && ploc.reg2 <= V7) {
          nFpRegs++;
        }
      }
      if (ploc.reg3 != kRinvalid) {
        if (ploc.reg3 >= R0 && ploc.reg3 <= R7) {
          nIntRegs++;
        } else if (ploc.reg2 >= V0 && ploc.reg2 <= V7) {
          nFpRegs++;
        }
      }
    }
    SetSizeOfGRSaveArea((k8BitSize - nIntRegs) * kSizeOfPtr);
    SetSizeOfVRSaveArea((k8BitSize - nFpRegs) * kSizeOfPtr * k2ByteSize);
  }
}

void AArch64MemLayout::LayoutFormalParams() {
  ParmLocator parmLocator(be);
  PLocInfo ploc;
  for (size_t i = 0; i < mirFunction->GetFormalCount(); ++i) {
    MIRSymbol *sym = mirFunction->GetFormal(i);
    uint32 stIndex = sym->GetStIndex();
    AArch64SymbolAlloc *symLoc = memAllocator->GetMemPool()->New<AArch64SymbolAlloc>();
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
    bool noStackPara = false;
    MIRType *ty = mirFunction->GetNthParamType(i);
    uint32 ptyIdx = ty->GetTypeIndex();
    parmLocator.LocateNextParm(*ty, ploc, i == 0);
    if (ploc.reg0 != kRinvalid) {  /* register */
      symLoc->SetRegisters(ploc.reg0, ploc.reg1, ploc.reg2, ploc.reg3);
      if (mirFunction->GetNthParamAttr(i).GetAttr(ATTR_localrefvar)) {
        symLoc->SetMemSegment(segRefLocals);
        SetSegmentSize(*symLoc, segRefLocals, ptyIdx);
      } else if (!sym->IsPreg()) {
        uint32 size;
        uint32 align;
        SetSizeAlignForTypeIdx(ptyIdx, size, align);
        symLoc->SetMemSegment(GetSegArgsRegPassed());
        /* the type's alignment requirement may be smaller than a registser's byte size */
        if (ty->GetPrimType() == PTY_agg &&  be.GetTypeSize(ptyIdx) > k4ByteSize) {
          /* struct param aligned on 8 byte boundary unless it is small enough */
          align = kSizeOfPtr;
        }
        int32 tSize = 0;
        if ((IsPrimitiveVector(ty->GetPrimType()) && GetPrimTypeSize(ty->GetPrimType()) > k8ByteSize) ||
            AArch64Abi::IsVectorArrayType(ty, tSize) != PTY_void) {
          align = k16ByteSize;
        }
        segArgsRegPassed.SetSize(RoundUp(segArgsRegPassed.GetSize(), align));
        symLoc->SetOffset(segArgsRegPassed.GetSize());
        segArgsRegPassed.SetSize(segArgsRegPassed.GetSize() + size);
      }
      noStackPara = true;
    } else {  /* stack */
      uint32 size;
      uint32 align;
      SetSizeAlignForTypeIdx(ptyIdx, size, align);
      symLoc->SetMemSegment(GetSegArgsStkPassed());
      segArgsStkPassed.SetSize(RoundUp(segArgsStkPassed.GetSize(), align));
      symLoc->SetOffset(segArgsStkPassed.GetSize());
      segArgsStkPassed.SetSize(segArgsStkPassed.GetSize() + size);
      /* We need it as dictated by the AArch64 ABI $5.4.2 C12 */
      segArgsStkPassed.SetSize(RoundUp(segArgsStkPassed.GetSize(), kSizeOfPtr));
      if (mirFunction->GetNthParamAttr(i).GetAttr(ATTR_localrefvar)) {
        SetLocalRegLocInfo(sym->GetStIdx(), *symLoc);
        AArch64SymbolAlloc *symLoc1 = memAllocator->GetMemPool()->New<AArch64SymbolAlloc>();
        symLoc1->SetMemSegment(segRefLocals);
        SetSegmentSize(*symLoc1, segRefLocals, ptyIdx);
        SetSymAllocInfo(stIndex, *symLoc1);
      }
    }
  }
}

void AArch64MemLayout::LayoutLocalVariables(std::vector<MIRSymbol*> &tempVar, std::vector<MIRSymbol*> &returnDelays) {
  uint32 symTabSize = mirFunction->GetSymTab()->GetSymbolTableSize();
  for (uint32 i = 0; i < symTabSize; ++i) {
    MIRSymbol *sym = mirFunction->GetSymTab()->GetSymbolFromStIdx(i);
    if (sym == nullptr || sym->GetStorageClass() != kScAuto || sym->IsDeleted()) {
      continue;
    }
    uint32 stIndex = sym->GetStIndex();
    TyIdx tyIdx = sym->GetTyIdx();
    AArch64SymbolAlloc *symLoc = memAllocator->GetMemPool()->New<AArch64SymbolAlloc>();
    SetSymAllocInfo(stIndex, *symLoc);
    CHECK_FATAL(!symLoc->IsRegister(), "expect not register");

    if (sym->IsRefType()) {
      if (mirFunction->GetRetRefSym().find(sym) != mirFunction->GetRetRefSym().end()) {
        /* try to put ret_ref at the end of segRefLocals */
        returnDelays.emplace_back(sym);
        continue;
      }
      symLoc->SetMemSegment(segRefLocals);
      segRefLocals.SetSize(RoundUp(segRefLocals.GetSize(), be.GetTypeAlign(tyIdx)));
      symLoc->SetOffset(segRefLocals.GetSize());
      segRefLocals.SetSize(segRefLocals.GetSize() + be.GetTypeSize(tyIdx));
    } else {
      if (sym->GetName() == "__EARetTemp__" ||
          sym->GetName().substr(0, kEARetTempNameSize) == "__EATemp__") {
        tempVar.emplace_back(sym);
        continue;
      }
      symLoc->SetMemSegment(segLocals);
      MIRType *ty = GlobalTables::GetTypeTable().GetTypeFromTyIdx(tyIdx);
      uint32 align = be.GetTypeAlign(tyIdx);
      int32 tSize = 0;
      if ((IsPrimitiveVector(ty->GetPrimType()) && GetPrimTypeSize(ty->GetPrimType()) > k8ByteSize) ||
          AArch64Abi::IsVectorArrayType(ty, tSize) != PTY_void) {
        align = k16ByteSize;
      }
      if (ty->GetPrimType() == PTY_agg && align < k8BitSize) {
        segLocals.SetSize(RoundUp(segLocals.GetSize(), k8BitSize));
      } else {
        segLocals.SetSize(RoundUp(segLocals.GetSize(), align));
      }
      symLoc->SetOffset(segLocals.GetSize());
      segLocals.SetSize(segLocals.GetSize() + be.GetTypeSize(tyIdx));
    }
  }
}

void AArch64MemLayout::LayoutEAVariales(std::vector<MIRSymbol*> &tempVar) {
  for (auto sym : tempVar) {
    uint32 stIndex = sym->GetStIndex();
    TyIdx tyIdx = sym->GetTyIdx();
    AArch64SymbolAlloc *symLoc = memAllocator->GetMemPool()->New<AArch64SymbolAlloc>();
    SetSymAllocInfo(stIndex, *symLoc);
    ASSERT(!symLoc->IsRegister(), "expect not register");
    symLoc->SetMemSegment(segRefLocals);
    segRefLocals.SetSize(RoundUp(segRefLocals.GetSize(), be.GetTypeAlign(tyIdx)));
    symLoc->SetOffset(segRefLocals.GetSize());
    segRefLocals.SetSize(segRefLocals.GetSize() + be.GetTypeSize(tyIdx));
  }
}

void AArch64MemLayout::LayoutReturnRef(std::vector<MIRSymbol*> &returnDelays,
                                       int32 &structCopySize, int32 &maxParmStackSize) {
  for (auto sym : returnDelays) {
    uint32 stIndex = sym->GetStIndex();
    TyIdx tyIdx = sym->GetTyIdx();
    AArch64SymbolAlloc *symLoc = memAllocator->GetMemPool()->New<AArch64SymbolAlloc>();
    SetSymAllocInfo(stIndex, *symLoc);
    ASSERT(!symLoc->IsRegister(), "expect not register");

    ASSERT(sym->IsRefType(), "expect reftype ");
    symLoc->SetMemSegment(segRefLocals);
    segRefLocals.SetSize(RoundUp(segRefLocals.GetSize(), be.GetTypeAlign(tyIdx)));
    symLoc->SetOffset(segRefLocals.GetSize());
    segRefLocals.SetSize(segRefLocals.GetSize() + be.GetTypeSize(tyIdx));
  }
  segArgsToStkPass.SetSize(FindLargestActualArea(structCopySize));
  maxParmStackSize = segArgsToStkPass.GetSize();
  if (Globals::GetInstance()->GetOptimLevel() == 0) {
    AssignSpillLocationsToPseudoRegisters();
  } else {
    AArch64CGFunc *aarchCGFunc = static_cast<AArch64CGFunc*>(cgFunc);
    /* 8-VirtualRegNode occupy byte number */
    aarchCGFunc->SetCatchRegno(cgFunc->NewVReg(kRegTyInt, 8));
  }
  segRefLocals.SetSize(RoundUp(segRefLocals.GetSize(), kSizeOfPtr));
  segLocals.SetSize(RoundUp(segLocals.GetSize(), kSizeOfPtr));
}

void AArch64MemLayout::LayoutActualParams() {
  for (size_t i = 0; i < mirFunction->GetFormalCount(); ++i) {
    if (i == 0) {
      if (be.HasFuncReturnType(*mirFunction)) {
        continue;
      }
    }
    MIRSymbol *sym = mirFunction->GetFormal(i);
    if (sym->IsPreg()) {
      continue;
    }
    uint32 stIndex = sym->GetStIndex();
    AArch64SymbolAlloc *symLoc = static_cast<AArch64SymbolAlloc*>(GetSymAllocInfo(stIndex));
    if (symLoc->GetMemSegment() == &GetSegArgsRegPassed()) {  /* register */
      /*
       *  In O0, we store parameters passed via registers into memory.
       *  So, each of such parameter needs to get assigned storage in stack.
       *  If a function parameter is never accessed in the function body,
       *  and if we don't create its memory operand here, its offset gets
       *  computed when the instruction to store its value into stack
       *  is generated in the prologue when its memory operand is created.
       *  But, the parameter would see a different StackFrameSize than
       *  the parameters that are accessed in the body, because
       *  the size of the storage for FP/LR is added to the stack frame
       *  size in between.
       *  To make offset assignment easier, we create a memory operand
       *  for each of function parameters in advance.
       *  This has to be done after all of formal parameters and local
       *  variables get assigned their respecitve storage, i.e.
       *  CallFrameSize (discounting callee-saved and FP/LR) is known.
       */
      MIRType *ty = mirFunction->GetNthParamType(i);
      uint32 ptyIdx = ty->GetTypeIndex();
      static_cast<AArch64CGFunc*>(cgFunc)->GetOrCreateMemOpnd(*sym, 0, be.GetTypeAlign(ptyIdx) * kBitsPerByte);
    }
  }
}

void AArch64MemLayout::LayoutStackFrame(int32 &structCopySize, int32 &maxParmStackSize) {
  LayoutVarargParams();
  LayoutFormalParams();
  /*
   * We do need this as LDR/STR with immediate
   * requires imm be aligned at a 8/4-byte boundary,
   * and local varirables may need 8-byte alignment.
   */
  segArgsRegPassed.SetSize(RoundUp(segArgsRegPassed.GetSize(), kSizeOfPtr));
  /* we do need this as SP has to be aligned at a 16-bytes bounardy */
  segArgsStkPassed.SetSize(RoundUp(segArgsStkPassed.GetSize(), kSizeOfPtr + kSizeOfPtr));
  /* allocate the local variables in the stack */
  std::vector<MIRSymbol*> EATempVar;
  std::vector<MIRSymbol*> retDelays;
  LayoutLocalVariables(EATempVar, retDelays);
  LayoutEAVariales(EATempVar);

  /* handle ret_ref sym now */
  LayoutReturnRef(retDelays, structCopySize, maxParmStackSize);

  /*
   * for the actual arguments that cannot be pass through registers
   * need to allocate space for caller-save registers
   */
  LayoutActualParams();

  fixStackSize = RealStackFrameSize();
}

void AArch64MemLayout::AssignSpillLocationsToPseudoRegisters() {
  MIRPregTable *pregTab = cgFunc->GetFunction().GetPregTab();

  /* BUG: n_regs include index 0 which is not a valid preg index. */
  size_t nRegs = pregTab->Size();
  spillLocTable.resize(nRegs);
  for (size_t i = 1; i < nRegs; ++i) {
    PrimType pType = pregTab->PregFromPregIdx(i)->GetPrimType();
    AArch64SymbolAlloc *symLoc = memAllocator->GetMemPool()->New<AArch64SymbolAlloc>();
    symLoc->SetMemSegment(segLocals);
    segLocals.SetSize(RoundUp(segLocals.GetSize(), GetPrimTypeSize(pType)));
    symLoc->SetOffset(segLocals.GetSize());
    MIRType *mirTy = GlobalTables::GetTypeTable().GetTypeTable()[pType];
    segLocals.SetSize(segLocals.GetSize() + be.GetTypeSize(mirTy->GetTypeIndex()));
    spillLocTable[i] = symLoc;
  }

  if (!cgFunc->GetMirModule().IsJavaModule()) {
    return;
  }

  /*
   * Allocate additional stack space for "thrownval".
   * segLocals need 8 bit align
   */
  segLocals.SetSize(RoundUp(segLocals.GetSize(), kSizeOfPtr));
  AArch64CGFunc *aarchCGFunc = static_cast<AArch64CGFunc*>(cgFunc);
  RegOperand &baseOpnd = aarchCGFunc->GetOrCreateStackBaseRegOperand();
  int32 offset = segLocals.GetSize();

  AArch64OfstOperand *offsetOpnd =
      aarchCGFunc->GetMemoryPool()->New<AArch64OfstOperand>(offset + k16BitSize, k64BitSize);
  AArch64MemOperand *throwMem = aarchCGFunc->GetMemoryPool()->New<AArch64MemOperand>(
      AArch64MemOperand::kAddrModeBOi, k64BitSize, baseOpnd, static_cast<AArch64RegOperand*>(nullptr), offsetOpnd,
      nullptr);
  aarchCGFunc->SetCatchOpnd(*throwMem);
  segLocals.SetSize(segLocals.GetSize() + kSizeOfPtr);
}

SymbolAlloc *AArch64MemLayout::AssignLocationToSpillReg(regno_t vrNum) {
  AArch64SymbolAlloc *symLoc = memAllocator->GetMemPool()->New<AArch64SymbolAlloc>();
  symLoc->SetMemSegment(segSpillReg);
  uint32 regSize = cgFunc->GetVRegSize(vrNum);
  segSpillReg.SetSize(RoundUp(segSpillReg.GetSize(), regSize));
  symLoc->SetOffset(segSpillReg.GetSize());
  segSpillReg.SetSize(segSpillReg.GetSize() + regSize);
  SetSpillRegLocInfo(vrNum, *symLoc);
  return symLoc;
}

int32 AArch64MemLayout::StackFrameSize() {
  int32 total = segArgsRegPassed.GetSize() + static_cast<AArch64CGFunc*>(cgFunc)->SizeOfCalleeSaved() +
                GetSizeOfRefLocals() + locals().GetSize() + GetSizeOfSpillReg();

  if (GetSizeOfGRSaveArea() > 0) {
    total += RoundUp(GetSizeOfGRSaveArea(), kAarch64StackPtrAlignment);
  }
  if (GetSizeOfVRSaveArea() > 0) {
    total += RoundUp(GetSizeOfVRSaveArea(), kAarch64StackPtrAlignment);
  }

  /*
   * if the function does not have VLA nor alloca,
   * we allocate space for arguments to stack-pass
   * in the call frame; otherwise, it has to be allocated for each call and reclaimed afterward.
   */
  total += segArgsToStkPass.GetSize();
  return RoundUp(total, kAarch64StackPtrAlignment);
}

int32 AArch64MemLayout::RealStackFrameSize() {
  int32 size = StackFrameSize();
  if (cgFunc->GetCG()->AddStackGuard()) {
    size += kAarch64StackPtrAlignment;
  }
  return size;
}

int32 AArch64MemLayout::GetRefLocBaseLoc() const {
  AArch64CGFunc *aarchCGFunc = static_cast<AArch64CGFunc*>(cgFunc);
  int32 beforeSize = GetSizeOfLocals();
  if (aarchCGFunc->UsedStpSubPairForCallFrameAllocation()) {
    return beforeSize;
  }
  return beforeSize + kSizeOfFplr;
}

int32 AArch64MemLayout::GetGRSaveAreaBaseLoc() {
  int32 total = RealStackFrameSize() -
                RoundUp(GetSizeOfGRSaveArea(), kAarch64StackPtrAlignment);
  total -= SizeOfArgsToStackPass();
  return total;
}

int32 AArch64MemLayout::GetVRSaveAreaBaseLoc() {
  int32 total = RealStackFrameSize() -
                RoundUp(GetSizeOfGRSaveArea(), kAarch64StackPtrAlignment) -
                RoundUp(GetSizeOfVRSaveArea(), kAarch64StackPtrAlignment);
  total -= SizeOfArgsToStackPass();
  return total;
}
}  /* namespace maplebe */
