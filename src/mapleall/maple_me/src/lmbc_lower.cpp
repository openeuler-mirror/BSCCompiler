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

#include "lmbc_lower.h"

namespace maple {

using namespace std;

inline PrimType UnsignedPrimType(int32 bytesize) {
  if (bytesize == 4) {
    return PTY_u32;
  }
  if (bytesize == 2) {
    return PTY_u16;
  }
  if (bytesize == 1) {
    return PTY_u8;
  }
  return PTY_u32;
}

PregIdx LMBCLowerer::GetSpecialRegFromSt(const MIRSymbol *sym) {
  MIRStorageClass storageClass = sym->GetStorageClass();
  PregIdx specreg = 0;
  if (storageClass == kScAuto || storageClass == kScFormal) {
    CHECK(sym->GetStIndex() < memlayout->sym_alloc_table.size(),
          "index out of range in LMBCLowerer::GetSpecialRegFromSt");
    SymbolAlloc symalloc = memlayout->sym_alloc_table[sym->GetStIndex()];
    if (symalloc.mem_segment->kind == MS_upformal || symalloc.mem_segment->kind == MS_formal ||
        symalloc.mem_segment->kind == MS_FPbased) {
      specreg = -kSregFp;
    } else if (symalloc.mem_segment->kind == MS_actual || symalloc.mem_segment->kind == MS_SPbased) {
      specreg = -kSregSp;
    } else {
      CHECK_FATAL(false, "LMBCLowerer::LowerDread: bad memory layout for local variable");
    }
  } else if (storageClass == kScGlobal || storageClass == kScFstatic || storageClass == kScExtern || storageClass == kScPstatic) {
    specreg = -kSregGp;
  } else {
    CHECK_FATAL(false, "LMBCLowerer::LowerDread: NYI");
  }
  return specreg;
}

BaseNode *LMBCLowerer::ReadregNodeForSymbol(MIRSymbol *sym) {
  return mirBuilder->CreateExprRegread(LOWERED_PTR_TYPE, GetSpecialRegFromSt(sym));
}

BaseNode *LMBCLowerer::LowerAddrof(AddrofNode *expr) {
  MIRSymbol *symbol = func->GetLocalOrGlobalSymbol(expr->GetStIdx());
  if (symbol->GetStorageClass() == kScText) {
    return expr;
  }
  int32 offset = 0;
  if (expr->GetFieldID() != 0) {
    MIRStructType *structty = dynamic_cast<MIRStructType *>(symbol->GetType());
    CHECK_FATAL(structty, "LMBCLowerer::LowerAddrof: non-zero fieldID for non-structure");
    offset = becommon->GetFieldOffset(*structty, expr->GetFieldID()).first;
  }
  //BaseNode *rrn = ReadregNodeForSymbol(symbol);
  PrimType symty = (expr->GetPrimType() == PTY_simplestr || expr->GetPrimType() == PTY_simpleobj) ? expr->GetPrimType() : LOWERED_PTR_TYPE;
  BaseNode *rrn = mirBuilder->CreateExprRegread(symty, GetSpecialRegFromSt(symbol));
  offset += symbol->IsLocal() ? memlayout->sym_alloc_table[symbol->GetStIndex()].offset
                              : globmemlayout->sym_alloc_table[symbol->GetStIndex()].offset;
  return (offset == 0) ? rrn
                       : mirBuilder->CreateExprBinary(OP_add, *GlobalTables::GetTypeTable().GetTypeFromTyIdx((TyIdx)expr->GetPrimType()), rrn,
                                                                 mirBuilder->GetConstInt(offset));
}

BaseNode *LMBCLowerer::LowerDread(AddrofNode *expr) {
  MIRSymbol *symbol = func->GetLocalOrGlobalSymbol(expr->GetStIdx());
  PrimType symty = symbol->GetType()->GetPrimType();
  int32 offset = 0;
  if (expr->GetFieldID() != 0) {
    MIRStructType *structty = dynamic_cast<MIRStructType *>(symbol->GetType());
    CHECK_FATAL(structty, "LMBCLowerer::LowerDread: non-zero fieldID for non-structure");
    FieldPair thepair = structty->TraverseToField(expr->GetFieldID());
    symty = GlobalTables::GetTypeTable().GetTypeFromTyIdx(thepair.second.first)->GetPrimType();
    offset = becommon->GetFieldOffset(*structty, expr->GetFieldID()).first;
  }
  // allow dread class reference
  PregIdx spcreg = GetSpecialRegFromSt(symbol);
  if (spcreg == -kSregFp) {
    CHECK_FATAL(symbol->IsLocal(), "load from fp non local?");
    IreadFPoffNode *ireadoff = mirBuilder->CreateExprIreadFPoff(
      symty, memlayout->sym_alloc_table[symbol->GetStIndex()].offset + offset);
    return ireadoff;
  } else {
    BaseNode *rrn = mirBuilder->CreateExprRegread(LOWERED_PTR_TYPE, spcreg);
    SymbolAlloc &symalloc = symbol->IsLocal() ? memlayout->sym_alloc_table[symbol->GetStIndex()]
                                              : globmemlayout->sym_alloc_table[symbol->GetStIndex()];
    IreadoffNode *ireadoff = mirBuilder->CreateExprIreadoff(symty, symalloc.offset + offset, rrn);
    return ireadoff;
  }
}

static MIRType *GetPointedToType(const MIRPtrType *pointerty) {
  MIRType *atype = GlobalTables::GetTypeTable().GetTypeFromTyIdx(pointerty->GetPointedTyIdx());
  if (atype->GetKind() == kTypeArray) {
    MIRArrayType *arraytype = static_cast<MIRArrayType *>(atype);
    return GlobalTables::GetTypeTable().GetTypeFromTyIdx(arraytype->GetElemTyIdx());
  }
  if (atype->GetKind() == kTypeFArray || atype->GetKind() == kTypeJArray) {
    MIRFarrayType *farraytype = static_cast<MIRFarrayType *>(atype);
    return GlobalTables::GetTypeTable().GetTypeFromTyIdx(farraytype->GetElemTyIdx());
  }
  return GlobalTables::GetTypeTable().GetTypeFromTyIdx(pointerty->GetPointedTyIdx());
}

BaseNode *LMBCLowerer::LowerIread(IreadNode *expr) {
  int32 offset = 0;
  MIRType *type = GlobalTables::GetTypeTable().GetTypeFromTyIdx(expr->GetTyIdx());
  MIRPtrType *pointerty = static_cast<MIRPtrType *>(type);
  CHECK_FATAL(pointerty, "expect a pointer type at iread node");
  if (expr->GetFieldID() != 0) {
    MIRStructType *structty = dynamic_cast<MIRStructType *>(GlobalTables::GetTypeTable().GetTypeFromTyIdx(pointerty->GetPointedTyIdx()));
    CHECK_FATAL(structty, "SelectIread: non-zero fieldID for non-structure");
    FieldPair thepair = structty->TraverseToField(expr->GetFieldID());
    type = GlobalTables::GetTypeTable().GetTypeFromTyIdx(thepair.second.first);
    offset = becommon->GetFieldOffset(*structty, expr->GetFieldID()).first;
  } else {
    type = GetPointedToType(pointerty);
  }
  BaseNode *ireadoff = mirBuilder->CreateExprIreadoff(type->GetPrimType(), offset, expr->Opnd(0));
  return ireadoff;
}

BaseNode *LMBCLowerer::LowerExpr(BaseNode *expr) {
  for (size_t i = 0; i < expr->NumOpnds(); ++i) {
    expr->SetOpnd(LowerExpr(expr->Opnd(i)), i);
  }
  switch (expr->GetOpCode()) {
  case OP_dread: return LowerDread(static_cast<DreadNode *>(expr));
  case OP_addrof: return LowerAddrof(static_cast<AddrofNode *>(expr));
  case OP_iread: return LowerIread(static_cast<IreadNode *>(expr));
  default: ;
  }
  return expr;
}

void LMBCLowerer::LowerAggDassign(BlockNode *newblk, const DassignNode *dsnode) {
  MIRSymbol *lhssymbol = func->GetLocalOrGlobalSymbol(dsnode->GetStIdx());
  int32 lhsoffset = 0;
  MIRType *lhsty = lhssymbol->GetType();
  if (dsnode->GetFieldID() != 0) {
    MIRStructType *structty = dynamic_cast<MIRStructType *>(lhssymbol->GetType());
    CHECK_FATAL(structty, "LowerAggDassign: non-zero fieldID for non-structure");
    FieldPair thepair = structty->TraverseToField(dsnode->GetFieldID());
    lhsty = GlobalTables::GetTypeTable().GetTypeFromTyIdx(thepair.second.first);
    lhsoffset = becommon->GetFieldOffset(*structty, dsnode->GetFieldID()).first;
  }
  uint32 lhsalign = becommon->GetTypeAlign(lhsty->GetTypeIndex());
  uint32 lhssize = becommon->GetTypeSize(lhsty->GetTypeIndex());

  uint32 rhsalign;
  uint32 alignused;
  int32 rhsoffset = 0;
  BaseNode *loadnode = nullptr;
  IassignoffNode *iassignoff = nullptr;
  if (dsnode->Opnd(0)->GetOpCode() == OP_dread) {
    AddrofNode *rhsdread = static_cast<AddrofNode *>(dsnode->Opnd(0));
    MIRSymbol *rhssymbol = func->GetLocalOrGlobalSymbol(rhsdread->GetStIdx());
    MIRType *rhsty = rhssymbol->GetType();
    if (rhsdread->GetFieldID() != 0) {
      MIRStructType *structty = dynamic_cast<MIRStructType *>(rhssymbol->GetType());
      CHECK_FATAL(structty, "SelectDassign: non-zero fieldID for non-structure");
      FieldPair thepair = structty->TraverseToField(rhsdread->GetFieldID());
      rhsty = GlobalTables::GetTypeTable().GetTypeFromTyIdx(thepair.second.first);
      rhsoffset = becommon->GetFieldOffset(*structty, rhsdread->GetFieldID()).first;
    }
    rhsalign = becommon->GetTypeAlign(rhsty->GetTypeIndex());
    BaseNode *rRrn = ReadregNodeForSymbol(rhssymbol);
    SymbolAlloc &rsymalloc = rhssymbol->IsLocal() ? memlayout->sym_alloc_table[rhssymbol->GetStIndex()]
                                                  : globmemlayout->sym_alloc_table[rhssymbol->GetStIndex()];
    BaseNode *lRrn = ReadregNodeForSymbol(lhssymbol);
    SymbolAlloc &lsymalloc = lhssymbol->IsLocal() ? memlayout->sym_alloc_table[lhssymbol->GetStIndex()]
                                                  : globmemlayout->sym_alloc_table[lhssymbol->GetStIndex()];

    alignused = std::min(lhsalign, rhsalign);
    alignused = std::min(alignused, 4u);  // max alignment is 32-bit
    if (!alignused) {
      alignused = 1u;
    }
    for (uint32 i = 0; i < (lhssize / alignused); i++) {
      // generate the load
      loadnode = mirBuilder->CreateExprIreadoff(UnsignedPrimType(alignused),
                                                           rsymalloc.offset + rhsoffset + i * alignused, rRrn);
      // generate the store
      iassignoff = mirBuilder->CreateStmtIassignoff(
        UnsignedPrimType(alignused), lsymalloc.offset + lhsoffset + i * alignused, lRrn, loadnode);
      newblk->AddStatement(iassignoff);
    }
    // take care of extra content at the end less than the unit of alignused
    uint32 lhssizeCovered = (lhssize / alignused) * alignused;
    uint32 newalignused = alignused;
    while (lhssizeCovered < lhssize) {
      newalignused = newalignused >> 1;
      if (lhssizeCovered + newalignused > lhssize) {
        continue;
      }
      // generate the load
      loadnode = mirBuilder->CreateExprIreadoff(UnsignedPrimType(newalignused),
                                                           rsymalloc.offset + rhsoffset + lhssizeCovered, rRrn);
      // generate the store
      iassignoff = mirBuilder->CreateStmtIassignoff(
        UnsignedPrimType(newalignused), lsymalloc.offset + lhsoffset + lhssizeCovered, lRrn, loadnode);
      newblk->AddStatement(iassignoff);
      lhssizeCovered += newalignused;
    }
  } else if (dsnode->Opnd(0)->GetOpCode() == OP_regread) {
    RegreadNode *regread = static_cast<RegreadNode *>(dsnode->Opnd(0));
    CHECK_FATAL(regread->GetRegIdx() == -kSregRetval0 && regread->GetPrimType() == PTY_agg, "");

    BaseNode *lRrn = ReadregNodeForSymbol(lhssymbol);
    SymbolAlloc &lsymalloc = lhssymbol->IsLocal() ? memlayout->sym_alloc_table[lhssymbol->GetStIndex()]
                                                  : globmemlayout->sym_alloc_table[lhssymbol->GetStIndex()];

    alignused = std::min(lhsalign, 4u);  // max alignment is 32-bit
    PregIdx ridx = -kSregRetval0;
    for (uint32 i = 0; i < (lhssize / alignused); i++) {
      // generate the load
      loadnode = mirBuilder->CreateExprRegread(UnsignedPrimType(alignused), ridx - i);
      // generate the store
      iassignoff = mirBuilder->CreateStmtIassignoff(
        UnsignedPrimType(alignused), lsymalloc.offset + lhsoffset + i * alignused, lRrn, loadnode);
      newblk->AddStatement(iassignoff);
    }
    // take care of extra content at the end less than the unit of alignused
    uint32 lhssizeCovered = (lhssize / alignused) * alignused;
    ridx = -kSregRetval0 - (lhssize / alignused);
    uint32 newalignused = alignused;
    while (lhssizeCovered < lhssize) {
      newalignused = newalignused >> 1;
      if (lhssizeCovered + newalignused > lhssize) {
        continue;
      }
      // generate the load
      loadnode = mirBuilder->CreateExprRegread(UnsignedPrimType(newalignused), ridx--);
      // generate the store
      iassignoff = mirBuilder->CreateStmtIassignoff(
        UnsignedPrimType(newalignused), lsymalloc.offset + lhsoffset + lhssizeCovered, lRrn, loadnode);
      newblk->AddStatement(iassignoff);
      lhssizeCovered += newalignused;
    }
  } else {  // iread
    IreadNode *rhsiread = static_cast<IreadNode *>(dsnode->Opnd(0));
    CHECK_FATAL(rhsiread, "LowerAggDassign: illegal rhs for dassign node of structure type");
    rhsiread->SetOpnd(LowerExpr(rhsiread->Opnd(0)), 0);
    MIRType *rhsRdTy = GlobalTables::GetTypeTable().GetTypeFromTyIdx(rhsiread->GetTyIdx());
    MIRPtrType *pointerty = static_cast<MIRPtrType *>(rhsRdTy);
    CHECK_FATAL(pointerty, "LowerAggDassign: expect a pointer type at iread node");
    if (rhsiread->GetFieldID() != 0) {
      MIRStructType *structty = dynamic_cast<MIRStructType *>(GlobalTables::GetTypeTable().GetTypeFromTyIdx(pointerty->GetPointedTyIdx()));
      CHECK_FATAL(structty, "LowerAggDassign: non-zero fieldID for non-structure");
      FieldPair thepair = structty->TraverseToField(rhsiread->GetFieldID());
      rhsRdTy = GlobalTables::GetTypeTable().GetTypeFromTyIdx(thepair.second.first);
      rhsoffset = becommon->GetFieldOffset(*structty, rhsiread->GetFieldID()).first;
    } else {
      rhsRdTy = GetPointedToType(pointerty);
    }
    rhsalign = becommon->GetTypeAlign(rhsRdTy->GetTypeIndex());
    BaseNode *lRrn = ReadregNodeForSymbol(lhssymbol);
    CHECK(lhssymbol->GetStIndex() < memlayout->sym_alloc_table.size() &&
            lhssymbol->GetStIndex() < globmemlayout->sym_alloc_table.size(),
          "index oout of range in LMBCLowerer::LowerAggDassign");
    SymbolAlloc &lsymalloc = lhssymbol->IsLocal() ? memlayout->sym_alloc_table[lhssymbol->GetStIndex()]
                                                  : globmemlayout->sym_alloc_table[lhssymbol->GetStIndex()];

    alignused = std::min(lhsalign, rhsalign);
    alignused = std::min(alignused, 4u);  // max alignment is 32-bit
    for (uint32 i = 0; i < (lhssize / alignused); i++) {
      // generate the load
      loadnode = mirBuilder->CreateExprIreadoff(UnsignedPrimType(alignused), rhsoffset + i * alignused,
                                                           rhsiread->Opnd(0));
      // generate the store
      iassignoff = mirBuilder->CreateStmtIassignoff(
        UnsignedPrimType(alignused), lsymalloc.offset + lhsoffset + i * alignused, lRrn, loadnode);
      newblk->AddStatement(iassignoff);
    }
    // take care of extra content at the end less than the unit of alignused
    uint32 lhssizeCovered = (lhssize / alignused) * alignused;
    uint32 newalignused = alignused;
    while (lhssizeCovered < lhssize) {
      newalignused = newalignused >> 1;
      if (lhssizeCovered + newalignused > lhssize) {
        continue;
      }
      // generate the load
      loadnode = mirBuilder->CreateExprIreadoff(UnsignedPrimType(newalignused), rhsoffset + lhssizeCovered,
                                                           rhsiread->Opnd(0));
      // generate the store
      iassignoff = mirBuilder->CreateStmtIassignoff(
        UnsignedPrimType(newalignused), lsymalloc.offset + lhsoffset + lhssizeCovered, lRrn, loadnode);
      newblk->AddStatement(iassignoff);
      lhssizeCovered += newalignused;
    }
  }
}

void LMBCLowerer::LowerDassign(DassignNode *dsnode, BlockNode *newblk) {
  if (dsnode->Opnd(0)->GetPrimType() != PTY_agg) {
    dsnode->SetOpnd(LowerExpr(dsnode->Opnd(0)), 0);
    MIRSymbol *symbol = func->GetLocalOrGlobalSymbol(dsnode->GetStIdx());
    int32 offset = 0;
    PrimType ptypused = symbol->GetType()->GetPrimType();
    if (dsnode->GetFieldID() != 0) {
      MIRStructType *structty = dynamic_cast<MIRStructType *>(symbol->GetType());
      CHECK_FATAL(structty, "LMBCLowerer::LowerDassign: non-zero fieldID for non-structure");
      offset = becommon->GetFieldOffset(*structty, dsnode->GetFieldID()).first;
      TyIdx ftyidx = structty->TraverseToField(dsnode->GetFieldID()).second.first;
      ptypused = GlobalTables::GetTypeTable().GetTypeFromTyIdx(ftyidx)->GetPrimType();
    }
    PregIdx spcreg = GetSpecialRegFromSt(symbol);
    if (spcreg == -kSregFp) {
      IassignFPoffNode *iassignoff = mirBuilder->CreateStmtIassignFPoff(OP_iassignfpoff,
        ptypused, memlayout->sym_alloc_table[symbol->GetStIndex()].offset + offset, dsnode->Opnd(0));
      newblk->AddStatement(iassignoff);
    } else {
      BaseNode *rrn = ReadregNodeForSymbol(symbol);
      SymbolAlloc &symalloc = symbol->IsLocal() ? memlayout->sym_alloc_table[symbol->GetStIndex()]
                                                : globmemlayout->sym_alloc_table[symbol->GetStIndex()];
      IassignoffNode *iassignoff =
        mirBuilder->CreateStmtIassignoff(ptypused, symalloc.offset + offset, rrn, dsnode->Opnd(0));
      newblk->AddStatement(iassignoff);
    }
  } else {
    LowerAggDassign(newblk, dsnode);
  }
}

void LMBCLowerer::LowerAggIassign(BlockNode *newblk, IassignNode *iassign) {
  int32 lhsoffset = 0;
  MIRType *lhsty = GlobalTables::GetTypeTable().GetTypeFromTyIdx(iassign->GetTyIdx());
  MIRPtrType *pointerty = static_cast<MIRPtrType *>(lhsty);
  if (pointerty->GetKind() != kTypePointer) {
    TypeAttrs typeAttrs;
    pointerty = static_cast<MIRPtrType *>(GlobalTables::GetTypeTable().GetOrCreatePointerType(*lhsty, GetExactPtrPrimType(), typeAttrs));
  }
  if (iassign->GetFieldID() != 0) {
    MIRStructType *structty = dynamic_cast<MIRStructType *>(GlobalTables::GetTypeTable().GetTypeFromTyIdx(pointerty->GetPointedTyIdx()));
    CHECK_FATAL(structty, "LowerAggDassign: non-zero fieldID for non-structure");
    FieldPair thepair = structty->TraverseToField(iassign->GetFieldID());
    lhsty = GlobalTables::GetTypeTable().GetTypeFromTyIdx(thepair.second.first);
    lhsoffset = becommon->GetFieldOffset(*structty, iassign->GetFieldID()).first;
  } else {
    lhsty = GetPointedToType(pointerty);
  }
  uint32 lhsalign = becommon->GetTypeAlign(lhsty->GetTypeIndex());
  uint32 lhssize = becommon->GetTypeSize(lhsty->GetTypeIndex());

  uint32 rhsalign;
  uint32 alignused;
  int32 rhsoffset = 0;
  BaseNode *loadnode = nullptr;
  IassignoffNode *iassignoff = nullptr;
  if (iassign->GetRHS()->GetOpCode() == OP_dread) {
    AddrofNode *rhsdread = static_cast<AddrofNode *>(iassign->GetRHS());
    MIRSymbol *rhssymbol = func->GetLocalOrGlobalSymbol(rhsdread->GetStIdx());
    MIRType *rhsty = rhssymbol->GetType();
    if (rhsdread->GetFieldID() != 0) {
      MIRStructType *structty = dynamic_cast<MIRStructType *>(rhssymbol->GetType());
      CHECK_FATAL(structty, "SelectDassign: non-zero fieldID for non-structure");
      FieldPair thepair = structty->TraverseToField(rhsdread->GetFieldID());
      rhsty = GlobalTables::GetTypeTable().GetTypeFromTyIdx(thepair.second.first);
      rhsoffset = becommon->GetFieldOffset(*structty, rhsdread->GetFieldID()).first;
    }
    rhsalign = becommon->GetTypeAlign(rhsty->GetTypeIndex());
    BaseNode *rRrn = ReadregNodeForSymbol(rhssymbol);
    CHECK(rhssymbol->GetStIndex() < memlayout->sym_alloc_table.size() &&
            rhssymbol->GetStIndex() < globmemlayout->sym_alloc_table.size(),
          "index out of range in LMBCLowerer::LowerAggIassign");
    SymbolAlloc &rsymalloc = rhssymbol->IsLocal() ? memlayout->sym_alloc_table[rhssymbol->GetStIndex()]
                                                  : globmemlayout->sym_alloc_table[rhssymbol->GetStIndex()];

    alignused = std::min(lhsalign, rhsalign);
    alignused = std::min(alignused, 4u);  // max alignment is 32-bit
    for (uint32 i = 0; i < (lhssize / alignused); i++) {
      // generate the load
      loadnode = mirBuilder->CreateExprIreadoff(UnsignedPrimType(alignused),
                                                           rsymalloc.offset + rhsoffset + i * alignused, rRrn);
      // generate the store
      iassignoff = mirBuilder->CreateStmtIassignoff(UnsignedPrimType(alignused), lhsoffset + i * alignused,
                                                               iassign->addrExpr, loadnode);
      newblk->AddStatement(iassignoff);
    }
    // take care of extra content at the end less than the unit of alignused
    uint32 lhssizeCovered = (lhssize / alignused) * alignused;
    uint32 newalignused = alignused;
    while (lhssizeCovered < lhssize) {
      newalignused = newalignused >> 1;
      if (lhssizeCovered + newalignused > lhssize) {
        continue;
      }
      // generate the load
      loadnode = mirBuilder->CreateExprIreadoff(UnsignedPrimType(newalignused),
                                                           rsymalloc.offset + rhsoffset + lhssizeCovered, rRrn);
      // generate the store
      iassignoff = mirBuilder->CreateStmtIassignoff(UnsignedPrimType(newalignused),
                                                               lhsoffset + lhssizeCovered, iassign->addrExpr, loadnode);
      newblk->AddStatement(iassignoff);
      lhssizeCovered += newalignused;
    }
  } else if (iassign->GetRHS()->GetOpCode() == OP_regread) {
    RegreadNode *regread = static_cast<RegreadNode *>(iassign->GetRHS());
    CHECK_FATAL(regread->GetRegIdx() == -kSregRetval0 && regread->GetPrimType() == PTY_agg, "");

    alignused = std::min(lhsalign, 4u);  // max alignment is 32-bit
    PregIdx ridx = -kSregRetval0;
    for (uint32 i = 0; i < (lhssize / alignused); i++) {
      // generate the load
      loadnode = mirBuilder->CreateExprRegread(UnsignedPrimType(alignused), ridx - i);
      // generate the store
      iassignoff = mirBuilder->CreateStmtIassignoff(UnsignedPrimType(alignused), lhsoffset + i * alignused,
                                                               iassign->addrExpr, loadnode);
      newblk->AddStatement(iassignoff);
    }
    // take care of extra content at the end less than the unit of alignused
    uint32 lhssizeCovered = (lhssize / alignused) * alignused;
    ridx = -kSregRetval0 - (lhssize / alignused);
    uint32 newalignused = alignused;
    while (lhssizeCovered < lhssize) {
      newalignused = newalignused >> 1;
      if (lhssizeCovered + newalignused > lhssize) {
        continue;
      }
      // generate the load
      loadnode = mirBuilder->CreateExprRegread(UnsignedPrimType(newalignused), ridx--);
      // generate the store
      iassignoff = mirBuilder->CreateStmtIassignoff(UnsignedPrimType(newalignused),
                                                               lhsoffset + lhssizeCovered, iassign->addrExpr, loadnode);
      newblk->AddStatement(iassignoff);
      lhssizeCovered += newalignused;
    }
  } else {  // iread
    IreadNode *rhsiread = static_cast<IreadNode *>(iassign->GetRHS());
    CHECK_FATAL(rhsiread, "LowerAggIassign: illegal rhs for dassign node of structure type");
    rhsiread->SetOpnd(LowerExpr(rhsiread->Opnd(0)), 0);
    MIRType *rhsRdTy = GlobalTables::GetTypeTable().GetTypeFromTyIdx(rhsiread->GetTyIdx());
    MIRPtrType *pointerty = static_cast<MIRPtrType *>(rhsRdTy);
    CHECK_FATAL(pointerty, "LowerAggIassign: expect a pointer type at iread node");
    if (rhsiread->GetFieldID() != 0) {
      MIRStructType *structty = dynamic_cast<MIRStructType *>(GlobalTables::GetTypeTable().GetTypeFromTyIdx(pointerty->GetPointedTyIdx()));
      CHECK_FATAL(structty, "LowerAggIassign: non-zero fieldID for non-structure");
      FieldPair thepair = structty->TraverseToField(rhsiread->GetFieldID());
      rhsRdTy = GlobalTables::GetTypeTable().GetTypeFromTyIdx(thepair.second.first);
      rhsoffset = becommon->GetFieldOffset(*structty, rhsiread->GetFieldID()).first;
    } else {
      rhsRdTy = GetPointedToType(pointerty);
    }
    rhsalign = becommon->GetTypeAlign(rhsRdTy->GetTypeIndex());

    alignused = std::min(lhsalign, rhsalign);
    alignused = std::min(alignused, 4u);  // max alignment is 32-bit
    for (uint32 i = 0; i < (lhssize / alignused); i++) {
      // generate the load
      loadnode = mirBuilder->CreateExprIreadoff(UnsignedPrimType(alignused), rhsoffset + i * alignused,
                                                           rhsiread->Opnd(0));
      // generate the store
      iassignoff = mirBuilder->CreateStmtIassignoff(UnsignedPrimType(alignused), lhsoffset + i * alignused,
                                                               iassign->addrExpr, loadnode);
      newblk->AddStatement(iassignoff);
    }
    // take care of extra content at the end less than the unit of alignused
    uint32 lhssizeCovered = (lhssize / alignused) * alignused;
    uint32 newalignused = alignused;
    while (lhssizeCovered < lhssize) {
      newalignused = newalignused >> 1;
      if (lhssizeCovered + newalignused > lhssize) {
        continue;
      }
      // generate the load
      loadnode = mirBuilder->CreateExprIreadoff(UnsignedPrimType(newalignused), rhsoffset + lhssizeCovered,
                                                           rhsiread->Opnd(0));
      // generate the store
      iassignoff = mirBuilder->CreateStmtIassignoff(UnsignedPrimType(newalignused),
                                                               lhsoffset + lhssizeCovered, iassign->addrExpr, loadnode);
      newblk->AddStatement(iassignoff);
      lhssizeCovered += newalignused;
    }
  }
}

void LMBCLowerer::LowerIassign(IassignNode *iassign, BlockNode *newblk) {
  iassign->addrExpr = LowerExpr(iassign->Opnd(0));
  if (iassign->GetRHS()->GetPrimType() != PTY_agg) {
    iassign->SetRHS(LowerExpr(iassign->GetRHS()));
    int32 offset = 0;
    MIRType *type = GlobalTables::GetTypeTable().GetTypeFromTyIdx(iassign->GetTyIdx());
    MIRPtrType *pointerty = static_cast<MIRPtrType *>(type);
    CHECK_FATAL(pointerty, "LowerIassign::expect a pointer type at iassign node");
    if (iassign->GetFieldID() != 0) {
      MIRStructType *structty = dynamic_cast<MIRStructType *>(GlobalTables::GetTypeTable().GetTypeFromTyIdx(pointerty->GetPointedTyIdx()));
      CHECK_FATAL(structty, "LowerAggIassign: non-zero fieldID for non-structure");
      offset = becommon->GetFieldOffset(*structty, iassign->GetFieldID()).first;
      TyIdx ftyidx = structty->TraverseToField(iassign->GetFieldID()).second.first;
      type = GlobalTables::GetTypeTable().GetTypeFromTyIdx(ftyidx);
    } else {
      type = GetPointedToType(pointerty);
    }
    PrimType ptypused = type->GetPrimType();
    IassignoffNode *iassignoff =
      mirBuilder->CreateStmtIassignoff(ptypused, offset, iassign->addrExpr, iassign->GetRHS());
    newblk->AddStatement(iassignoff);
  } else {
    LowerAggIassign(newblk, iassign);
  }
}

// called only if the return has > 1 operand; assume prior lowering already
// converted any return of structs to be via fake parameter
void LMBCLowerer::LowerReturn(NaryStmtNode *retNode, BlockNode *newblk) {
  CHECK_FATAL(retNode->NumOpnds() <= 2, "LMBCLowerer::LowerReturn: more than 2 return values NYI");
  for (int i = 0; i < retNode->NumOpnds(); i++) {
    CHECK_FATAL(retNode->Opnd(i)->GetPrimType() != PTY_agg, "LMBCLowerer::LowerReturn: return of aggregate needs to be handled first");
    // insert regassign for the returned value
    BaseNode *rhs = LowerExpr(retNode->Opnd(i));
    RegassignNode *regasgn = mirBuilder->CreateStmtRegassign(rhs->GetPrimType(), i == 0 ? -kSregRetval0 : -kSregRetval1, rhs);
    newblk->AddStatement(regasgn);
  }
  retNode->GetNopnd().clear();  // remove the return operands
  retNode->SetNumOpnds(0);
  newblk->AddStatement(retNode);
}

void LMBCLowerer::LowerCall(NaryStmtNode *naryStmt, BlockNode *newblk) {
  // go through each parameter
  uint32 i = 0;
  if (naryStmt->GetOpCode() == OP_icall || naryStmt->GetOpCode() == OP_icallassigned) {
    i = 1;
  }
  ParmLocator parmlocator;
  for (; i < naryStmt->NumOpnds(); i++) {
    BaseNode *opnd = naryStmt->Opnd(i);
    MIRType *ty = nullptr;
    // get ty for this parameter
    if (opnd->GetPrimType() != PTY_agg) {
      ty = GlobalTables::GetTypeTable().GetTypeFromTyIdx(static_cast<TyIdx>(opnd->GetPrimType()));
    } else {
      Opcode opnd_opcode = opnd->GetOpCode();
      CHECK_FATAL(opnd_opcode == OP_dread || opnd_opcode == OP_iread, "");
      if (opnd_opcode == OP_dread) {
        AddrofNode *dread = static_cast<AddrofNode *>(opnd);
        MIRSymbol *sym = func->GetLocalOrGlobalSymbol(dread->GetStIdx());
        ty = GlobalTables::GetTypeTable().GetTypeFromTyIdx(sym->GetTyIdx());
        if (dread->GetFieldID() != 0) {
          CHECK_FATAL(ty->GetKind() == kTypeStruct || ty->GetKind() == kTypeClass, "");
          FieldPair thepair = static_cast<MIRStructType *>(ty)->TraverseToField(dread->GetFieldID());
          ty = GlobalTables::GetTypeTable().GetTypeFromTyIdx(thepair.second.first);
        }
      } else {  // OP_iread
        IreadNode *iread = static_cast<IreadNode *>(opnd);
        ty = GlobalTables::GetTypeTable().GetTypeFromTyIdx(iread->GetTyIdx());
        CHECK_FATAL(ty->GetKind() == kTypePointer, "");
        ty = GlobalTables::GetTypeTable().GetTypeFromTyIdx(static_cast<MIRPtrType *>(ty)->GetPointedTyIdx());
        if (iread->GetFieldID() != 0) {
          CHECK_FATAL(ty->GetKind() == kTypeStruct || ty->GetKind() == kTypeClass, "");
          FieldPair thepair = static_cast<MIRStructType *>(ty)->TraverseToField(iread->GetFieldID());
          ty = GlobalTables::GetTypeTable().GetTypeFromTyIdx(thepair.second.first);
        }
      }
    }
    PLocInfo ploc;
    parmlocator.LocateNextParm(ty, ploc);
    if (opnd->GetPrimType() != PTY_agg) {
      IassignFPoffNode *iass = mirBuilder->CreateStmtIassignFPoff(OP_iassignspoff, opnd->GetPrimType(), ploc.memoffset, LowerExpr(opnd));
      newblk->AddStatement(iass);
    } else {
      BlkassignoffNode *bass = mirModule->CurFuncCodeMemPool()->New<BlkassignoffNode>(ploc.memoffset, ploc.memsize);
      bass->SetBOpnd(mirBuilder->CreateExprRegread(PTY_a64, -kSregSp), 0);
      // the operand is either OP_dread or OP_iread; use its address instead
      if (opnd->GetOpCode() == OP_dread) {
        opnd->SetOpCode(OP_addrof);
      } else {
        opnd->SetOpCode(OP_iaddrof);
      }
      bass->SetBOpnd(opnd, 1);
      newblk->AddStatement(bass);
    }
  }
  BaseNode *opnd0 = nullptr;
  if (naryStmt->GetOpCode() == OP_icall || naryStmt->GetOpCode() == OP_icallassigned) {
    opnd0 = naryStmt->Opnd(0);
    naryStmt->GetNopnd().clear();  // remove the call operands
    naryStmt->GetNopnd().push_back(opnd0);
    naryStmt->SetNumOpnds(1);
  } else {
    naryStmt->GetNopnd().clear();  // remove the call operands
    naryStmt->SetNumOpnds(0);
  }
  newblk->AddStatement(naryStmt);
}

BlockNode *LMBCLowerer::LowerBlock(BlockNode *block) {
  BlockNode *newblk = mirModule->CurFuncCodeMemPool()->New<BlockNode>();
  if (!block->GetFirst()) {
    return newblk;
  }
  StmtNode *nextstmt = block->GetFirst();
  do {
    StmtNode *stmt = nextstmt;
    if (stmt == block->GetLast()) {
      nextstmt = nullptr;
    } else {
      nextstmt = stmt->GetNext();
    }
    stmt->SetNext(nullptr);
    switch (stmt->GetOpCode()) {
    case OP_dassign: {
      LowerDassign(static_cast<DassignNode*>(stmt), newblk);
      break;
    }
    case OP_iassign: {
      LowerIassign(static_cast<IassignNode*>(stmt), newblk);
      break;
    }
    case OP_return: {
      NaryStmtNode *retNode = static_cast<NaryStmtNode*>(stmt);
      if (retNode->GetNopndSize() == 0) {
        newblk->AddStatement(stmt);
      } else {
        LowerReturn(retNode, newblk);
      }
    }
    case OP_call:
    case OP_icall: {
      LowerCall(static_cast<NaryStmtNode*>(stmt), newblk);
      break;
    }
    default: {
      for (size_t i = 0; i < stmt->NumOpnds(); ++i) {
        stmt->SetOpnd(LowerExpr(stmt->Opnd(i)), i);
      }
      newblk->AddStatement(stmt);
      break;
    }
    }
  } while (nextstmt != nullptr);
  return newblk;
}

void LMBCLowerer::LoadFormalsAssignedToPregs() {
  // go through each formals
  for (int32 i = func->GetFormalDefVec().size()-1; i >= 0; i--) {
    MIRSymbol *formalSt = func->GetFormalDefVec()[i].formalSym;
    if (formalSt->GetSKind() != kStPreg) {
      continue;
    }
    MIRPreg *preg = formalSt->GetPreg();
    uint32 stindex = formalSt->GetStIndex();
    PrimType pty = formalSt->GetType()->GetPrimType();
    IreadFPoffNode *ireadfpoff = mirBuilder->CreateExprIreadFPoff(pty, memlayout->sym_alloc_table[stindex].offset);
    RegassignNode *rass = mirBuilder->CreateStmtRegassign(pty, func->GetPregTab()->GetPregIdxFromPregno(preg->GetPregNo()), ireadfpoff);
    func->GetBody()->InsertFirst(rass);
  }
}

void LMBCLowerer::LowerFunction() {
  BlockNode *origbody = func->GetBody();
  BlockNode *newbody = LowerBlock(origbody);
  func->SetBody(newbody);
  LoadFormalsAssignedToPregs();
}

}  // namespace maple
