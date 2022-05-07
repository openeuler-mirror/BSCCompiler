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
    } else if (symalloc.mem_segment->kind == MS_actual ||
               symalloc.mem_segment->kind == MS_SPbased) {
      specreg = -kSregSp;
    } else {
      CHECK_FATAL(false, "LMBCLowerer::LowerDread: bad memory layout for local variable");
    }
  } else if (storageClass == kScGlobal || storageClass == kScFstatic ||
             storageClass == kScExtern || storageClass == kScPstatic) {
    specreg = -kSregGp;
  } else {
    CHECK_FATAL(false, "LMBCLowerer::LowerDread: NYI");
  }
  return specreg;
}

BaseNode *LMBCLowerer::LowerAddrof(AddrofNode *expr) {
  MIRSymbol *symbol = func->GetLocalOrGlobalSymbol(expr->GetStIdx());
  int32 offset = 0;
  if (expr->GetFieldID() != 0) {
    MIRStructType *structty = static_cast<MIRStructType *>(symbol->GetType());
    offset = becommon->GetFieldOffset(*structty, expr->GetFieldID()).first;
  }
  PrimType symty = (expr->GetPrimType() == PTY_simplestr ||
                    expr->GetPrimType() == PTY_simpleobj) ? expr->GetPrimType() : LOWERED_PTR_TYPE;
  if (!symbol->LMBCAllocateOffSpecialReg()) {
    return mirBuilder->CreateExprDreadoff(OP_addrofoff, LOWERED_PTR_TYPE, *symbol, offset);
  }
  BaseNode *rrn = mirBuilder->CreateExprRegread(symty, GetSpecialRegFromSt(symbol));
  offset += symbol->IsLocal() ? memlayout->sym_alloc_table[symbol->GetStIndex()].offset
                              : globmemlayout->sym_alloc_table[symbol->GetStIndex()].offset;
  return (offset == 0) ? rrn : mirBuilder->CreateExprBinary(OP_add,
                                                            expr->GetPrimType(),
                                                            rrn,
                                                            mirBuilder->GetConstInt(offset));}

BaseNode *LMBCLowerer::LowerDread(AddrofNode *expr) {
  MIRSymbol *symbol = func->GetLocalOrGlobalSymbol(expr->GetStIdx());
  PrimType symty = symbol->GetType()->GetPrimType();
  int32 offset = 0;
  if (expr->GetFieldID() != 0) {
    MIRStructType *structty = static_cast<MIRStructType *>(symbol->GetType());
    FieldPair thepair = structty->TraverseToField(expr->GetFieldID());
    symty = GlobalTables::GetTypeTable().GetTypeFromTyIdx(thepair.second.first)->GetPrimType();
    offset = becommon->GetFieldOffset(*structty, expr->GetFieldID()).first;
  }
  if (!symbol->LMBCAllocateOffSpecialReg()) {
    BaseNode *base = mirBuilder->CreateExprDreadoff(OP_addrofoff, LOWERED_PTR_TYPE, *symbol, 0);
    IreadoffNode *ireadoff = mirBuilder->CreateExprIreadoff(symty, offset, base);
    return ireadoff;
  }
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

BaseNode *LMBCLowerer::LowerDreadoff(DreadoffNode *dreadoff) {
  MIRSymbol *symbol = func->GetLocalOrGlobalSymbol(dreadoff->stIdx);
  if (!symbol->LMBCAllocateOffSpecialReg()) {
    return dreadoff;
  }
  PregIdx spcreg = GetSpecialRegFromSt(symbol);
  if (spcreg == -kSregFp) {
    CHECK_FATAL(symbol->IsLocal(), "load from fp non local?");
    IreadFPoffNode *ireadoff = mirBuilder->CreateExprIreadFPoff(
      dreadoff->GetPrimType(), memlayout->sym_alloc_table[symbol->GetStIndex()].offset + dreadoff->offset);
    return ireadoff;
  } else {
    BaseNode *rrn = mirBuilder->CreateExprRegread(LOWERED_PTR_TYPE, spcreg);
    SymbolAlloc &symalloc = symbol->IsLocal() ? memlayout->sym_alloc_table[symbol->GetStIndex()]
                                              : globmemlayout->sym_alloc_table[symbol->GetStIndex()];
    IreadoffNode *ireadoff = mirBuilder->CreateExprIreadoff(dreadoff->GetPrimType(), symalloc.offset + dreadoff->offset, rrn);
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
  if (expr->GetFieldID() != 0) {
    MIRType *type = GlobalTables::GetTypeTable().GetTypeFromTyIdx(expr->GetTyIdx());
    MIRStructType *structty =
        static_cast<MIRStructType *>(GlobalTables::GetTypeTable().GetTypeFromTyIdx(
        static_cast<MIRPtrType*>(type)->GetPointedTyIdx()));
    offset = becommon->GetFieldOffset(*structty, expr->GetFieldID()).first;
  }
  BaseNode *ireadoff = mirBuilder->CreateExprIreadoff(expr->GetPrimType(), offset, expr->Opnd(0));
  return ireadoff;
}

BaseNode *LMBCLowerer::LowerIaddrof(IaddrofNode *expr) {
  int32 offset = 0;
  if (expr->GetFieldID() != 0) {
    MIRType *type = GlobalTables::GetTypeTable().GetTypeFromTyIdx(expr->GetTyIdx());
    MIRStructType *structty = static_cast<MIRStructType *>(
        GlobalTables::GetTypeTable().GetTypeFromTyIdx(
        static_cast<MIRPtrType*>(type)->GetPointedTyIdx()));
    offset = becommon->GetFieldOffset(*structty, expr->GetFieldID()).first;
  }
  if (offset == 0) {
    return expr->Opnd(0);
  }
  return mirBuilder->CreateExprBinary(OP_add, expr->GetPrimType(), expr->Opnd(0),
                                      mirBuilder->CreateIntConst(offset, expr->GetPrimType()));
}

BaseNode *LMBCLowerer::LowerExpr(BaseNode *expr) {
  for (size_t i = 0; i < expr->NumOpnds(); ++i) {
    expr->SetOpnd(LowerExpr(expr->Opnd(i)), i);
  }
  switch (expr->GetOpCode()) {
    case OP_addrof:
      return LowerAddrof(static_cast<AddrofNode *>(expr));
    case OP_addrofoff: {
      MIRSymbol *symbol = func->GetLocalOrGlobalSymbol(static_cast<AddrofoffNode*>(expr)->stIdx);
      CHECK_FATAL(!symbol->LMBCAllocateOffSpecialReg(),
                  "LMBCLowerer:: illegal addrofoff instruction");
      break;
    }
    case OP_dread:
      return LowerDread(static_cast<DreadNode *>(expr));
    case OP_dreadoff:
      return LowerDreadoff(static_cast<DreadoffNode *>(expr));
    case OP_iread:
      return LowerIread(static_cast<IreadNode *>(expr));
    case OP_iaddrof:
      return LowerIaddrof(static_cast<IreadNode *>(expr));
    default:
      break;
  }
  return expr;
}

// lower using OP_blkassignoff
void LMBCLowerer::LowerAggDassign(const DassignNode *dsnode, MIRType *lhsty,
                                  int32 offset, BlockNode *newblk) {
  BaseNode *rhs = dsnode->Opnd(0);
  CHECK_FATAL(rhs->GetOpCode() == OP_dread || rhs->GetOpCode() == OP_iread,
              "LowerAggDassign: rhs inconsistent");
  // change rhs to address of rhs
  if (rhs->GetOpCode() == OP_dread) {
    rhs->SetOpCode(OP_addrof);
  } else {  // OP_iread
    rhs->SetOpCode(OP_iaddrof);
  }
  rhs->SetPrimType(LOWERED_PTR_TYPE);
  // generate lhs address expression
  BaseNode *lhs = nullptr;
  MIRSymbol *symbol = func->GetLocalOrGlobalSymbol(dsnode->GetStIdx());
  if (!symbol->LMBCAllocateOffSpecialReg()) {
    lhs = mirBuilder->CreateExprDreadoff(OP_addrofoff, LOWERED_PTR_TYPE, *symbol, offset);
  } else {
    lhs = mirBuilder->CreateExprRegread(LOWERED_PTR_TYPE, GetSpecialRegFromSt(symbol));
    SymbolAlloc &symalloc = symbol->IsLocal() ? memlayout->sym_alloc_table[symbol->GetStIndex()]
                                              : globmemlayout->sym_alloc_table[symbol->GetStIndex()];
    offset = symalloc.offset + offset;
   }
  // generate the blkassignoff
  BlkassignoffNode *bass = mirModule->CurFuncCodeMemPool()->New<BlkassignoffNode>(offset,
                                                                                  lhsty->GetSize());
  bass->SetAlign(lhsty->GetAlign());
  bass->SetBOpnd(lhs, 0);
  bass->SetBOpnd(LowerExpr(rhs), 1);
  newblk->AddStatement(bass);
}

void LMBCLowerer::LowerDassign(DassignNode *dsnode, BlockNode *newblk) {
  MIRSymbol *symbol = func->GetLocalOrGlobalSymbol(dsnode->GetStIdx());
  MIRType *symty = symbol->GetType();
  int32 offset = 0;
  if (dsnode->GetFieldID() != 0) {
    MIRStructType *structty = static_cast<MIRStructType *>(symbol->GetType());
    FieldPair thepair = structty->TraverseToField(dsnode->GetFieldID());
    symty = GlobalTables::GetTypeTable().GetTypeFromTyIdx(thepair.second.first);
    offset = becommon->GetFieldOffset(*structty, dsnode->GetFieldID()).first;
  }
  BaseNode *rhs = LowerExpr(dsnode->Opnd(0));
  if (rhs->GetPrimType() != PTY_agg || rhs->GetOpCode() == OP_regread) {
    if (!symbol->LMBCAllocateOffSpecialReg()) {
      BaseNode *base = mirBuilder->CreateExprDreadoff(OP_addrofoff, LOWERED_PTR_TYPE, *symbol, 0);
      IassignoffNode *iassignoff = mirBuilder->CreateStmtIassignoff(symty->GetPrimType(),
                                                                    offset, base, rhs);
      newblk->AddStatement(iassignoff);
      return;
    }
    PregIdx spcreg = GetSpecialRegFromSt(symbol);
    if (spcreg == -kSregFp) {
      IassignFPoffNode *iassignoff = mirBuilder->CreateStmtIassignFPoff(OP_iassignfpoff,
          symty->GetPrimType(),
          memlayout->sym_alloc_table[symbol->GetStIndex()].offset + offset, rhs);
      newblk->AddStatement(iassignoff);
    } else {
      BaseNode *rrn = mirBuilder->CreateExprRegread(LOWERED_PTR_TYPE, spcreg);
      SymbolAlloc &symalloc = symbol->IsLocal() ?
          memlayout->sym_alloc_table[symbol->GetStIndex()] :
          globmemlayout->sym_alloc_table[symbol->GetStIndex()];
      IassignoffNode *iassignoff = mirBuilder->CreateStmtIassignoff(symty->GetPrimType(),
                                                                    symalloc.offset + offset,
                                                                    rrn,
                                                                    rhs);
      newblk->AddStatement(iassignoff);
    }
  } else {
    LowerAggDassign(dsnode, symty, offset, newblk);
  }
}

void LMBCLowerer::LowerDassignoff(DassignoffNode *dsnode, BlockNode *newblk) {
  MIRSymbol *symbol = func->GetLocalOrGlobalSymbol(dsnode->stIdx);
  CHECK_FATAL(dsnode->Opnd(0)->GetPrimType() != PTY_agg, "LowerDassignoff: agg primitive type NYI");
  BaseNode *rhs = LowerExpr(dsnode->Opnd(0));
  if (!symbol->LMBCAllocateOffSpecialReg()) {
    newblk->AddStatement(dsnode);
    return;
  }
  PregIdx spcreg = GetSpecialRegFromSt(symbol);
  if (spcreg == -kSregFp) {
    IassignFPoffNode *iassignoff = mirBuilder->CreateStmtIassignFPoff(OP_iassignfpoff,
        dsnode->GetPrimType(),
        memlayout->sym_alloc_table[symbol->GetStIndex()].offset + dsnode->offset, rhs);
    newblk->AddStatement(iassignoff);
  } else {
    BaseNode *rrn = mirBuilder->CreateExprRegread(LOWERED_PTR_TYPE, spcreg);
    SymbolAlloc &symalloc = symbol->IsLocal() ? memlayout->sym_alloc_table[symbol->GetStIndex()]
                                              : globmemlayout->sym_alloc_table[symbol->GetStIndex()];
    IassignoffNode *iassignoff =
        mirBuilder->CreateStmtIassignoff(dsnode->GetPrimType(),
                                         symalloc.offset + dsnode->offset, rrn, rhs);
    newblk->AddStatement(iassignoff);
  }
}
  // lower using OP_blkassignoff
void LMBCLowerer::LowerAggIassign(IassignNode *iassign, MIRType *lhsty,
                                  int32 offset, BlockNode *newblk) {
  BaseNode *rhs = iassign->rhs;
  CHECK_FATAL(rhs->GetOpCode() == OP_dread || rhs->GetOpCode() == OP_iread ||
              rhs->GetOpCode() == OP_ireadoff || rhs->GetOpCode() == OP_ireadfpoff,
              "LowerAggIassign: rhs inconsistent");
  // change rhs to address of rhs
  switch (rhs->GetOpCode()) {
    case OP_dread: rhs->SetOpCode(OP_addrof); break;
    case OP_iread: rhs->SetOpCode(OP_iaddrof); break;
    case OP_ireadoff: {
      IreadoffNode *ireadoff = static_cast<IreadoffNode*>(rhs);
      rhs = mirBuilder->CreateExprBinary(OP_add, LOWERED_PTR_TYPE, rhs->Opnd(0),
                                         mirBuilder->GetConstInt(ireadoff->GetOffset()));
      break;
    }
    case OP_ireadfpoff: {
      IreadFPoffNode *ireadfpoff = static_cast<IreadFPoffNode*>(rhs);
      rhs = mirBuilder->CreateExprBinary(OP_add, LOWERED_PTR_TYPE,
                                         mirBuilder->CreateExprRegread(LOWERED_PTR_TYPE, -kSregFp),
                                         mirBuilder->GetConstInt(ireadfpoff->GetOffset()));
      break;
    }
    default: ;
  }
  rhs->SetPrimType(LOWERED_PTR_TYPE);
  // generate the blkassignoff
  BlkassignoffNode *bass = mirModule->CurFuncCodeMemPool()->New<BlkassignoffNode>(offset,
                                                                                  lhsty->GetSize());
  bass->SetAlign(lhsty->GetAlign());
  bass->SetBOpnd(iassign->addrExpr, 0);
  bass->SetBOpnd(rhs, 1);
  newblk->AddStatement(bass);
}

void LMBCLowerer::LowerIassign(IassignNode *iassign, BlockNode *newblk) {
  iassign->addrExpr = LowerExpr(iassign->Opnd(0));
  iassign->rhs = LowerExpr(iassign->GetRHS());
  int32 offset = 0;
  MIRType *type = GlobalTables::GetTypeTable().GetTypeFromTyIdx(iassign->GetTyIdx());
  MIRPtrType *pointerty = static_cast<MIRPtrType *>(type);
  if (iassign->GetFieldID() != 0) {
    MIRStructType *structty = static_cast<MIRStructType *>(
        GlobalTables::GetTypeTable().GetTypeFromTyIdx(pointerty->GetPointedTyIdx()));
    offset = becommon->GetFieldOffset(*structty, iassign->GetFieldID()).first;
    TyIdx ftyidx = structty->TraverseToField(iassign->GetFieldID()).second.first;
    type = GlobalTables::GetTypeTable().GetTypeFromTyIdx(ftyidx);
  } else {
    type = GetPointedToType(pointerty);
  }
  if (iassign->GetRHS()->GetPrimType() != PTY_agg) {
    PrimType ptypused = type->GetPrimType();
    IassignoffNode *iassignoff = mirBuilder->CreateStmtIassignoff(ptypused,
                                                                  offset,
                                                                  iassign->addrExpr,
                                                                  iassign->rhs);
    newblk->AddStatement(iassignoff);
  } else {
    LowerAggIassign(iassign, type, offset, newblk);
  }
}

// called only if the return has > 1 operand; assume prior lowering already
// converted any return of structs to be via fake parameter
void LMBCLowerer::LowerReturn(NaryStmtNode *retNode, BlockNode *newblk) {
  CHECK_FATAL(retNode->NumOpnds() <= 2, "LMBCLowerer::LowerReturn: more than 2 return values NYI");
  for (int i = 0; i < retNode->NumOpnds(); i++) {
    CHECK_FATAL(retNode->Opnd(i)->GetPrimType() != PTY_agg,
                "LMBCLowerer::LowerReturn: return of aggregate needs to be handled first");
    // insert regassign for the returned value
    BaseNode *rhs = LowerExpr(retNode->Opnd(i));
    RegassignNode *regasgn = mirBuilder->CreateStmtRegassign(rhs->GetPrimType(),
                                                             i == 0 ? -kSregRetval0 : -kSregRetval1,
                                                             rhs);
    newblk->AddStatement(regasgn);
  }
  retNode->GetNopnd().clear();  // remove the return operands
  retNode->SetNumOpnds(0);
  newblk->AddStatement(retNode);
}

MIRFuncType *LMBCLowerer::FuncTypeFromFuncPtrExpr(BaseNode *x) {
  MIRFuncType *res = nullptr;
  switch (x->GetOpCode()) {
    case OP_regread: {
      RegreadNode *regread = static_cast<RegreadNode *>(x);
      MIRPreg *preg = func->GetPregTab()->PregFromPregIdx(regread->GetRegIdx());
      // see if it is promoted from a symbol
      if (preg->GetOp() == OP_dread) {
        const MIRSymbol *symbol = preg->rematInfo.sym;
        MIRType *mirType = symbol->GetType();
        if (mirType->GetKind() == kTypePointer) {
          res = static_cast<MIRPtrType *>(mirType)->GetPointedFuncType();
        }
        if (res != nullptr) {
          break;
        }
      }
      // check if a formal promoted to preg
      for (FormalDef &formalDef : func->GetFormalDefVec()) {
        if (!formalDef.formalSym->IsPreg()) {
          continue;
        }
        if (formalDef.formalSym->GetPreg() == preg) {
          MIRType *mirType = formalDef.formalSym->GetType();
          if (mirType->GetKind() == kTypePointer) {
            res = static_cast<MIRPtrType *>(mirType)->GetPointedFuncType();
          }
          break;
        }
      }
      break;
    }
    case OP_dread: {
      DreadNode *dread = static_cast<DreadNode *>(x);
      MIRSymbol *symbol = func->GetLocalOrGlobalSymbol(dread->GetStIdx());
      MIRType *mirType = symbol->GetType();
      if (dread->GetFieldID() != 0) {
        MIRStructType *structty = static_cast<MIRStructType *>(mirType);
        FieldPair thepair = structty->TraverseToField(dread->GetFieldID());
        mirType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(thepair.second.first);
      }
      if (mirType->GetKind() == kTypePointer) {
        res = static_cast<MIRPtrType *>(mirType)->GetPointedFuncType();
      }
      break;
    }
    case OP_iread: {
      IreadNode *iread = static_cast<IreadNode *>(x);
      MIRPtrType *ptrType = static_cast<MIRPtrType *>(iread->GetType());
      MIRType *mirType = ptrType->GetPointedType();
      if (mirType->GetKind() == kTypePointer) {
        res = static_cast<MIRPtrType *>(mirType)->GetPointedFuncType();
      }
      break;
    }
    case OP_addroffunc: {
      AddroffuncNode *addrofFunc = static_cast<AddroffuncNode *>(x);
      PUIdx puIdx = addrofFunc->GetPUIdx();
      MIRFunction *f = GlobalTables::GetFunctionTable().GetFunctionFromPuidx(puIdx);
      res = f->GetMIRFuncType();
      break;
    }
    case OP_retype: {
      MIRType *mirType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(
          static_cast<RetypeNode*>(x)->GetTyIdx());
      if (mirType->GetKind() == kTypePointer) {
        res = static_cast<MIRPtrType *>(mirType)->GetPointedFuncType();
      }
      if (res == nullptr) {
        res = FuncTypeFromFuncPtrExpr(x->Opnd(0));
      }
      break;
    }
    case OP_select: {
      res = FuncTypeFromFuncPtrExpr(x->Opnd(1));
      if (res == nullptr) {
        res = FuncTypeFromFuncPtrExpr(x->Opnd(2));
      }
      break;
    }
    default: CHECK_FATAL(false, "LMBCLowerer::FuncTypeFromFuncPtrExpr: NYI");
  }
  return res;
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
          CHECK_FATAL(ty->GetKind() == kTypeStruct || ty->GetKind() == kTypeClass || ty->GetKind() == kTypeUnion, "");
          FieldPair thepair = static_cast<MIRStructType *>(ty)->TraverseToField(dread->GetFieldID());
          ty = GlobalTables::GetTypeTable().GetTypeFromTyIdx(thepair.second.first);
        }
      } else {  // OP_iread
        IreadNode *iread = static_cast<IreadNode *>(opnd);
        ty = GlobalTables::GetTypeTable().GetTypeFromTyIdx(iread->GetTyIdx());
        CHECK_FATAL(ty->GetKind() == kTypePointer, "");
        ty = GlobalTables::GetTypeTable().GetTypeFromTyIdx(
            static_cast<MIRPtrType *>(ty)->GetPointedTyIdx());
        if (iread->GetFieldID() != 0) {
          CHECK_FATAL(ty->GetKind() == kTypeStruct || ty->GetKind() == kTypeClass || ty->GetKind() == kTypeUnion, "");
          FieldPair thepair = static_cast<MIRStructType *>(ty)->TraverseToField(iread->GetFieldID());
          ty = GlobalTables::GetTypeTable().GetTypeFromTyIdx(thepair.second.first);
        }
      }
    }
    PLocInfo ploc;
    parmlocator.LocateNextParm(ty, ploc);
    if (opnd->GetPrimType() != PTY_agg) {
      IassignFPoffNode *iass = mirBuilder->CreateStmtIassignFPoff(OP_iassignspoff,
                                                                  opnd->GetPrimType(),
                                                                  ploc.memoffset,
                                                                  LowerExpr(opnd));
      newblk->AddStatement(iass);
    } else {
      BlkassignoffNode *bass =
          mirModule->CurFuncCodeMemPool()->New<BlkassignoffNode>(ploc.memoffset, ploc.memsize);
      bass->SetAlign(std::min(ty->GetAlign(), 8u));
      bass->SetBOpnd(mirBuilder->CreateExprRegread(LOWERED_PTR_TYPE, -kSregSp), 0);
      // the operand is either OP_dread or OP_iread; use its address instead
      if (opnd->GetOpCode() == OP_dread) {
        opnd->SetOpCode(OP_addrof);
      } else {
        opnd->SetOpCode(OP_iaddrof);
      }
      opnd->SetPrimType(LOWERED_PTR_TYPE);
      bass->SetBOpnd(LowerExpr(opnd), 1);
      newblk->AddStatement(bass);
    }
  }
  BaseNode *opnd0 = nullptr;
  if (naryStmt->GetOpCode() == OP_icall || naryStmt->GetOpCode() == OP_icallassigned) {
    opnd0 = naryStmt->Opnd(0);
    naryStmt->GetNopnd().clear();  // remove the call operands
    // convert to OP_icallproto by finding the function prototype and record in stmt
    naryStmt->SetOpCode(OP_icallproto);
    MIRFuncType *funcType = FuncTypeFromFuncPtrExpr(opnd0);
    CHECK_FATAL(funcType != nullptr, "LMBCLowerer::LowerCall: cannot find prototype for icall");
    static_cast<IcallNode *>(naryStmt)->SetRetTyIdx(funcType->GetTypeIndex());
    // add back the function pointer operand
    naryStmt->GetNopnd().push_back(LowerExpr(opnd0));
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
    case OP_dassignoff: {
      LowerDassignoff(static_cast<DassignoffNode*>(stmt), newblk);
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
    IreadFPoffNode *ireadfpoff = mirBuilder->CreateExprIreadFPoff(pty,
        memlayout->sym_alloc_table[stindex].offset);
    RegassignNode *rass = mirBuilder->CreateStmtRegassign(pty,
        func->GetPregTab()->GetPregIdxFromPregno(preg->GetPregNo()), ireadfpoff);
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
