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

PregIdx LMBCLowerer::GetSpecialRegFromSt(const MIRSymbol &sym) const {
  MIRStorageClass storageClass = sym.GetStorageClass();
  PregIdx specreg = 0;
  if (storageClass == kScAuto) {
    CHECK(sym.GetStIndex() < memlayout->symAllocTable.size(),
          "index out of range in LMBCLowerer::GetSpecialRegFromSt");
    SymbolAlloc *symalloc = &memlayout->symAllocTable[sym.GetStIndex()];
    if (symalloc->memSegment->kind == MS_FPbased) {
      specreg = -kSregFp;
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

BaseNode *LMBCLowerer::LowerAddrof(const AddrofNode *expr) {
  MIRSymbol *symbol = func->GetLocalOrGlobalSymbol(expr->GetStIdx());
  ASSERT_NOT_NULL(symbol);
  symbol->ResetIsDeleted();
  int32 offset = 0;
  if (expr->GetFieldID() != 0) {
    MIRStructType *structty = static_cast<MIRStructType *>(symbol->GetType());
    offset = static_cast<int32>(structty->GetFieldOffsetFromBaseAddr(expr->GetFieldID()).byteOffset);
  }
  PrimType symty = (expr->GetPrimType() == PTY_simplestr ||
                    expr->GetPrimType() == PTY_simpleobj) ? expr->GetPrimType() : GetLoweredPtrType();
  if (!symbol->LMBCAllocateOffSpecialReg()) {
    return mirBuilder->CreateExprDreadoff(OP_addrofoff, GetLoweredPtrType(), *symbol, offset);
  }
  BaseNode *rrn = mirBuilder->CreateExprRegread(symty, GetSpecialRegFromSt(*symbol));
  offset += symbol->IsLocal() ? memlayout->symAllocTable[symbol->GetStIndex()].offset
                              : globmemlayout->symAllocTable[symbol->GetStIndex()].offset;
  return (offset == 0) ? rrn : mirBuilder->CreateExprBinary(OP_add,
                                                            expr->GetPrimType(),
                                                            rrn,
                                                            mirBuilder->GetConstInt(offset));}

BaseNode *LMBCLowerer::LowerDread(const AddrofNode *expr) {
  MIRSymbol *symbol = func->GetLocalOrGlobalSymbol(expr->GetStIdx());
  ASSERT_NOT_NULL(symbol);
  symbol->ResetIsDeleted();
  PrimType symty = symbol->GetType()->GetPrimType();
  int32 offset = 0;
  if (expr->GetFieldID() != 0) {
    MIRStructType *structty = static_cast<MIRStructType *>(symbol->GetType());
    FieldPair thepair = structty->TraverseToField(expr->GetFieldID());
    symty = GlobalTables::GetTypeTable().GetTypeFromTyIdx(thepair.second.first)->GetPrimType();
    offset = static_cast<int32>(structty->GetFieldOffsetFromBaseAddr(expr->GetFieldID()).byteOffset);
  }
  if (!symbol->LMBCAllocateOffSpecialReg()) {
    BaseNode *base = mirBuilder->CreateExprDreadoff(OP_addrofoff, GetLoweredPtrType(), *symbol, 0);
    IreadoffNode *ireadoff = mirBuilder->CreateExprIreadoff(symty, offset, base);
    if (GetPrimTypeSize(ireadoff->GetPrimType()) == GetPrimTypeSize(expr->GetPrimType())) {
      return ireadoff;
    }
    return mirBuilder->CreateExprTypeCvt(OP_cvt, expr->GetPrimType(), GetRegPrimType(ireadoff->GetPrimType()),
                                         *ireadoff);
  }
  PregIdx spcreg = GetSpecialRegFromSt(*symbol);
  if (spcreg == -kSregFp) {
    CHECK_FATAL(symbol->IsLocal(), "load from fp non local?");
    IreadFPoffNode *ireadoff = mirBuilder->CreateExprIreadFPoff(
        symty, memlayout->symAllocTable[symbol->GetStIndex()].offset + offset);
    if (GetPrimTypeSize(ireadoff->GetPrimType()) == GetPrimTypeSize(expr->GetPrimType())) {
      return ireadoff;
    }
    return mirBuilder->CreateExprTypeCvt(OP_cvt, expr->GetPrimType(), GetRegPrimType(ireadoff->GetPrimType()),
                                         *ireadoff);
  } else {
    BaseNode *rrn = mirBuilder->CreateExprRegread(GetLoweredPtrType(), spcreg);
    SymbolAlloc &symalloc = symbol->IsLocal() ? memlayout->symAllocTable[symbol->GetStIndex()]
                                              : globmemlayout->symAllocTable[symbol->GetStIndex()];
    IreadoffNode *ireadoff = mirBuilder->CreateExprIreadoff(symty, symalloc.offset + offset, rrn);
    if (GetPrimTypeSize(ireadoff->GetPrimType()) == GetPrimTypeSize(expr->GetPrimType())) {
      return ireadoff;
    }
    return mirBuilder->CreateExprTypeCvt(OP_cvt, expr->GetPrimType(), GetRegPrimType(ireadoff->GetPrimType()),
                                         *ireadoff);
  }
}

BaseNode *LMBCLowerer::LowerDreadoff(DreadoffNode *dreadoff) {
  MIRSymbol *symbol = func->GetLocalOrGlobalSymbol(dreadoff->stIdx);
  ASSERT_NOT_NULL(symbol);
  symbol->ResetIsDeleted();
  if (!symbol->LMBCAllocateOffSpecialReg()) {
    return dreadoff;
  }
  PrimType symty = symbol->GetType()->GetPrimType();
  PregIdx spcreg = GetSpecialRegFromSt(*symbol);
  if (spcreg == -kSregFp) {
    CHECK_FATAL(symbol->IsLocal(), "load from fp non local?");
    IreadFPoffNode *ireadoff = mirBuilder->CreateExprIreadFPoff(
        symty, memlayout->symAllocTable[symbol->GetStIndex()].offset + dreadoff->offset);
    return ireadoff;
  } else {
    BaseNode *rrn = mirBuilder->CreateExprRegread(GetLoweredPtrType(), spcreg);
    SymbolAlloc &symalloc = symbol->IsLocal() ? memlayout->symAllocTable[symbol->GetStIndex()]
                                              : globmemlayout->symAllocTable[symbol->GetStIndex()];
    IreadoffNode *ireadoff = mirBuilder->CreateExprIreadoff(symty, symalloc.offset + dreadoff->offset, rrn);
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

BaseNode *LMBCLowerer::LowerIread(const IreadNode &expr) const {
  int32 offset = 0;
  MIRPtrType *ptrType = static_cast<MIRPtrType*>(GlobalTables::GetTypeTable().GetTypeFromTyIdx(expr.GetTyIdx()));
  MIRType *type = ptrType->GetPointedType();
  if (expr.GetFieldID() != 0) {
    MIRStructType *structty = static_cast<MIRStructType *>(type);
    offset = static_cast<int32>(structty->GetFieldOffsetFromBaseAddr(expr.GetFieldID()).byteOffset);
    type = structty->GetFieldType(expr.GetFieldID());
  }
  BaseNode *ireadoff = mirBuilder->CreateExprIreadoff(type->GetPrimType(), offset, expr.Opnd(0));
  if (GetPrimTypeSize(ireadoff->GetPrimType()) == GetPrimTypeSize(expr.GetPrimType())) {
    return ireadoff;
  }
  return mirBuilder->CreateExprTypeCvt(OP_cvt, expr.GetPrimType(), GetRegPrimType(ireadoff->GetPrimType()), *ireadoff);
}

BaseNode *LMBCLowerer::LowerIaddrof(const IaddrofNode &expr) const {
  uint32 offset = 0;
  if (expr.GetFieldID() != 0) {
    MIRType *type = GlobalTables::GetTypeTable().GetTypeFromTyIdx(expr.GetTyIdx());
    MIRStructType *structty =
        static_cast<MIRStructType*>(
            GlobalTables::GetTypeTable().GetTypeFromTyIdx(static_cast<MIRPtrType*>(type)->GetPointedTyIdx()));
    offset = structty->GetFieldOffsetFromBaseAddr(expr.GetFieldID()).byteOffset;
  }
  if (offset == 0) {
    return expr.Opnd(0);
  }
  return mirBuilder->CreateExprBinary(OP_add, expr.GetPrimType(), expr.Opnd(0),
                                      mirBuilder->CreateIntConst(offset, expr.GetPrimType()));
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
      symbol->ResetIsDeleted();
      CHECK_FATAL(!symbol->LMBCAllocateOffSpecialReg(),
                  "LMBCLowerer:: illegal addrofoff instruction");
      break;
    }
    case OP_dread:
      return LowerDread(static_cast<DreadNode *>(expr));
    case OP_dreadoff:
      return LowerDreadoff(static_cast<DreadoffNode *>(expr));
    case OP_iread:
      return LowerIread(*(static_cast<IreadNode *>(expr)));
    case OP_iaddrof:
      return LowerIaddrof(static_cast<IreadNode &>(*expr));
    default:
      break;
  }
  return expr;
}

// lower using OP_blkassignoff
void LMBCLowerer::LowerAggDassign(const DassignNode &dsnode, const MIRType *lhsty,
                                  int32 offset, BlockNode *newblk) {
  BaseNode *rhs = dsnode.Opnd(0);
  CHECK_FATAL(rhs->GetOpCode() == OP_dread || rhs->GetOpCode() == OP_iread,
              "LowerAggDassign: rhs inconsistent");
  // change rhs to address of rhs
  if (rhs->GetOpCode() == OP_dread) {
    rhs->SetOpCode(OP_addrof);
  } else {  // OP_iread
    rhs->SetOpCode(OP_iaddrof);
  }
  rhs->SetPrimType(GetLoweredPtrType());
  // generate lhs address expression
  BaseNode *lhs = nullptr;
  MIRSymbol *symbol = func->GetLocalOrGlobalSymbol(dsnode.GetStIdx());
  ASSERT_NOT_NULL(symbol);
  symbol->ResetIsDeleted();
  if (!symbol->LMBCAllocateOffSpecialReg()) {
    lhs = mirBuilder->CreateExprDreadoff(OP_addrofoff, GetLoweredPtrType(), *symbol, offset);
  } else {
    lhs = mirBuilder->CreateExprRegread(GetLoweredPtrType(), GetSpecialRegFromSt(*symbol));
    SymbolAlloc &symalloc = symbol->IsLocal() ? memlayout->symAllocTable[symbol->GetStIndex()]
                                              : globmemlayout->symAllocTable[symbol->GetStIndex()];
    offset = symalloc.offset + offset;
  }
  // generate the blkassignoff
  BlkassignoffNode *bass = mirModule->CurFuncCodeMemPool()->New<BlkassignoffNode>(offset, lhsty->GetSize());
  bass->SetAlign(lhsty->GetAlign());
  bass->SetBOpnd(lhs, 0);
  bass->SetBOpnd(LowerExpr(rhs), 1);
  newblk->AddStatement(bass);
}

void LMBCLowerer::LowerDassign(DassignNode *dsnode, BlockNode *newblk) {
  MIRSymbol *symbol = func->GetLocalOrGlobalSymbol(dsnode->GetStIdx());
  symbol->ResetIsDeleted();
  MIRType *symty = symbol->GetType();
  int32 offset = 0;
  if (dsnode->GetFieldID() != 0) {
    MIRStructType *structty = static_cast<MIRStructType *>(symbol->GetType());
    ASSERT_NOT_NULL(structty);
    FieldPair thepair = structty->TraverseToField(dsnode->GetFieldID());
    symty = GlobalTables::GetTypeTable().GetTypeFromTyIdx(thepair.second.first);
    offset = static_cast<int32>(structty->GetFieldOffsetFromBaseAddr(dsnode->GetFieldID()).byteOffset);
  }
  BaseNode *rhs = LowerExpr(dsnode->Opnd(0));
  if (rhs->GetPrimType() != PTY_agg || rhs->GetOpCode() == OP_regread) {
    PrimType ptypUsed = symty->GetPrimType();
    if (ptypUsed == PTY_agg) {
      ptypUsed = rhs->GetPrimType();
    }
    if (!symbol->LMBCAllocateOffSpecialReg()) {
      BaseNode *base = mirBuilder->CreateExprDreadoff(OP_addrofoff, GetLoweredPtrType(), *symbol, 0);
      IassignoffNode *iassignoff = mirBuilder->CreateStmtIassignoff(ptypUsed,
                                                                    offset, base, rhs);
      newblk->AddStatement(iassignoff);
      return;
    }
    PregIdx spcreg = GetSpecialRegFromSt(*symbol);
    if (spcreg == -kSregFp) {
      IassignFPoffNode *iassignoff = mirBuilder->CreateStmtIassignFPoff(OP_iassignfpoff,
          ptypUsed,
          memlayout->symAllocTable[symbol->GetStIndex()].offset + offset, rhs);
      newblk->AddStatement(iassignoff);
    } else {
      BaseNode *rrn = mirBuilder->CreateExprRegread(GetLoweredPtrType(), spcreg);
      SymbolAlloc &symalloc = symbol->IsLocal() ?
          memlayout->symAllocTable[symbol->GetStIndex()] :
          globmemlayout->symAllocTable[symbol->GetStIndex()];
      IassignoffNode *iassignoff = mirBuilder->CreateStmtIassignoff(ptypUsed,
                                                                    symalloc.offset + offset,
                                                                    rrn,
                                                                    rhs);
      newblk->AddStatement(iassignoff);
    }
  } else {
    LowerAggDassign(*dsnode, symty, offset, newblk);
  }
}

void LMBCLowerer::LowerDassignoff(DassignoffNode *dsnode, BlockNode *newblk) {
  MIRSymbol *symbol = func->GetLocalOrGlobalSymbol(dsnode->stIdx);
  ASSERT_NOT_NULL(symbol);
  symbol->ResetIsDeleted();
  CHECK_FATAL(dsnode->Opnd(0)->GetPrimType() != PTY_agg, "LowerDassignoff: agg primitive type NYI");
  BaseNode *rhs = LowerExpr(dsnode->Opnd(0));
  if (!symbol->LMBCAllocateOffSpecialReg()) {
    newblk->AddStatement(dsnode);
    return;
  }
  PregIdx spcreg = GetSpecialRegFromSt(*symbol);
  if (spcreg == -kSregFp) {
    IassignFPoffNode *iassignoff = mirBuilder->CreateStmtIassignFPoff(OP_iassignfpoff,
        dsnode->GetPrimType(),
        memlayout->symAllocTable[symbol->GetStIndex()].offset + dsnode->offset, rhs);
    newblk->AddStatement(iassignoff);
  } else {
    BaseNode *rrn = mirBuilder->CreateExprRegread(GetLoweredPtrType(), spcreg);
    SymbolAlloc &symalloc = symbol->IsLocal() ? memlayout->symAllocTable[symbol->GetStIndex()]
                                              : globmemlayout->symAllocTable[symbol->GetStIndex()];
    IassignoffNode *iassignoff =
        mirBuilder->CreateStmtIassignoff(dsnode->GetPrimType(),
                                         symalloc.offset + dsnode->offset, rrn, rhs);
    newblk->AddStatement(iassignoff);
  }
}
  // lower using OP_blkassignoff
void LMBCLowerer::LowerAggIassign(const IassignNode &iassign, const MIRType *lhsty,
                                  int32 offset, BlockNode &newblk) const {
  BaseNode *rhs = iassign.rhs;
  CHECK_FATAL(rhs->GetOpCode() == OP_dread || rhs->GetOpCode() == OP_iread ||
              rhs->GetOpCode() == OP_ireadoff || rhs->GetOpCode() == OP_ireadfpoff,
              "LowerAggIassign: rhs inconsistent");
  // change rhs to address of rhs
  switch (rhs->GetOpCode()) {
    case OP_dread: rhs->SetOpCode(OP_addrof); break;
    case OP_iread: rhs->SetOpCode(OP_iaddrof); break;
    case OP_ireadoff: {
      IreadoffNode *ireadoff = static_cast<IreadoffNode*>(rhs);
      rhs = mirBuilder->CreateExprBinary(OP_add, GetLoweredPtrType(), rhs->Opnd(0),
                                         mirBuilder->GetConstInt(ireadoff->GetOffset()));
      break;
    }
    case OP_ireadfpoff: {
      IreadFPoffNode *ireadfpoff = static_cast<IreadFPoffNode*>(rhs);
      rhs = mirBuilder->CreateExprBinary(OP_add, GetLoweredPtrType(),
                                         mirBuilder->CreateExprRegread(GetLoweredPtrType(), -kSregFp),
                                         mirBuilder->GetConstInt(ireadfpoff->GetOffset()));
      break;
    }
    default: ;
  }
  rhs->SetPrimType(GetLoweredPtrType());
  // generate the blkassignoff
  BlkassignoffNode *bass = mirModule->CurFuncCodeMemPool()->New<BlkassignoffNode>(offset,
                                                                                  lhsty->GetSize());
  bass->SetAlign(lhsty->GetAlign());
  bass->SetBOpnd(iassign.addrExpr, 0);
  bass->SetBOpnd(rhs, 1);
  newblk.AddStatement(bass);
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
    offset = static_cast<int32>(structty->GetFieldOffsetFromBaseAddr(iassign->GetFieldID()).byteOffset);
    TyIdx ftyidx = structty->TraverseToField(iassign->GetFieldID()).second.first;
    type = GlobalTables::GetTypeTable().GetTypeFromTyIdx(ftyidx);
  } else {
    type = GetPointedToType(pointerty);
  }
  if (iassign->GetRHS()->GetPrimType() != PTY_agg) {
    PrimType ptypused = type->GetPrimType();
    if (ptypused == PTY_agg) {
      ptypused = iassign->GetRHS()->GetPrimType();
    }
    IassignoffNode *iassignoff = mirBuilder->CreateStmtIassignoff(ptypused, offset, iassign->addrExpr, iassign->rhs);
    newblk->AddStatement(iassignoff);
  } else {
    LowerAggIassign(*iassign, type, offset, *newblk);
  }
}

// called only if the return has > 1 operand; assume prior lowering already
// converted any return of structs to be via fake parameter
void LMBCLowerer::LowerReturn(NaryStmtNode &retNode, BlockNode &newblk) {
  if (retNode.Opnd(0)->GetPrimType() != PTY_agg) {
    CHECK_FATAL(retNode.NumOpnds() <= 2, "LMBCLowerer::LowerReturn: more than 2 return values NYI");
    for (size_t i = 0; i < retNode.NumOpnds(); ++i) {
      // insert regassign for the returned value
      PrimType ptyp = retNode.Opnd(i)->GetPrimType();
      BaseNode *rhs = LowerExpr(retNode.Opnd(i));
      RegassignNode *regasgn = mirBuilder->CreateStmtRegassign(ptyp,
                                                               i == 0 ? -kSregRetval0 : -kSregRetval1,
                                                               rhs);
      newblk.AddStatement(regasgn);
    }
  } else {  // handle return of small struct using only %%retval0
    BaseNode *rhs = LowerExpr(retNode.Opnd(0));
    RegassignNode *regasgn = mirBuilder->CreateStmtRegassign(PTY_agg, -kSregRetval0, rhs);
    newblk.AddStatement(regasgn);
  }
  retNode.GetNopnd().clear();  // remove the return operands
  retNode.SetNumOpnds(0);
  newblk.AddStatement(&retNode);
}

void LMBCLowerer::LowerCall(NaryStmtNode *stmt, BlockNode *newblk) {
  for (size_t i = 0; i < stmt->NumOpnds(); ++i) {
    if (stmt->Opnd(i)->GetPrimType() != PTY_agg) {
      stmt->SetOpnd(LowerExpr(stmt->Opnd(i)), i);
      continue;
    }
    bool paramInPrototype = false;
    if (stmt->GetOpCode() != OP_asm) {
      MIRFuncType *funcType = nullptr;
      if (stmt->GetOpCode() == OP_icallproto) {
        IcallNode *icallproto = static_cast<IcallNode*>(stmt);
        funcType = static_cast<MIRFuncType*>(GlobalTables::GetTypeTable().GetTypeFromTyIdx(icallproto->GetRetTyIdx()));
        paramInPrototype = (i == 0) ? false : (i - 1) < funcType->GetParamTypeList().size();
      } else {
        CallNode *callNode = static_cast<CallNode*>(stmt);
        MIRFunction *calleeFunc = GlobalTables::GetFunctionTable().GetFunctionFromPuidx(callNode->GetPUIdx());
        funcType = calleeFunc->GetMIRFuncType();
        paramInPrototype = i < funcType->GetParamTypeList().size();
      }
    }
    if (paramInPrototype) {
      stmt->SetOpnd(LowerExpr(stmt->Opnd(i)), i);
      continue;
    }
    // lower to iread so the type can be provided
    if (stmt->Opnd(i)->GetOpCode() ==  OP_iread) {
      IreadNode *iread = static_cast<IreadNode *>(stmt->Opnd(i));
      iread->SetOpnd(LowerExpr(iread->Opnd(0)), 0);
    } else if (stmt->Opnd(i)->GetOpCode() ==  OP_dread) {
      AddrofNode *addrof = static_cast<AddrofNode *>(stmt->Opnd(i));
      FieldID fid = addrof->GetFieldID();
      addrof->SetOpCode(OP_addrof);
      addrof->SetPrimType(GetExactPtrPrimType());
      addrof->SetFieldID(0);
      MIRSymbol *symbol = func->GetLocalOrGlobalSymbol(addrof->GetStIdx());
      ASSERT_NOT_NULL(symbol);
      MIRPtrType ptrType(symbol->GetTyIdx(), GetExactPtrPrimType());
      ptrType.SetTypeAttrs(symbol->GetAttrs());
      TyIdx addrTyIdx = GlobalTables::GetTypeTable().GetOrCreateMIRType(&ptrType);
      if (addrTyIdx == becommon->GetSizeOfTypeSizeTable()) {
        MIRType *newType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(addrTyIdx);
        becommon->UpdateTypeTable(*newType);
      }
      IreadNode *newIread = mirModule->CurFuncCodeMemPool()->New<IreadNode>(
          OP_iread, PTY_agg, addrTyIdx, fid, LowerExpr(addrof));
      stmt->SetOpnd(newIread, i);
    }
  }
  newblk->AddStatement(stmt);
}

void LMBCLowerer::FixPrototype4FirstArgReturn(const IcallNode *icall) const {
  MIRFuncType *ftype = static_cast<MIRFuncType*>(GlobalTables::GetTypeTable().GetTypeFromTyIdx(icall->GetRetTyIdx()));
  if (!ftype->FirstArgReturn()) {
    return;
  }
  MIRType *retType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(ftype->GetRetTyIdx());
  if (retType->GetPrimType() == PTY_void) {
    return;
  }
  // insert return type as fake first parameter
  size_t oldSize = ftype->GetParamTypeList().size();
  ftype->GetParamTypeList().emplace_back(TyIdx(0));
  ftype->GetParamAttrsList().emplace_back(TypeAttrs());
  for (size_t i = oldSize; i > 0; i--) {
    ftype->GetParamTypeList()[i] = ftype->GetParamTypeList()[i - 1];
    ftype->GetParamAttrsList()[i] = ftype->GetParamAttrsList()[i - 1];
  }
  MIRType *newType = GlobalTables::GetTypeTable().GetOrCreatePointerType(
      ftype->GetRetTyIdx(), GetExactPtrPrimType(), ftype->GetRetAttrs());
  ftype->GetParamTypeList()[0] = newType->GetTypeIndex();
  ftype->GetParamAttrsList()[0] = TypeAttrs();
  // change return type to void
  ftype->SetRetTyIdx(GlobalTables::GetTypeTable().GetVoid()->GetTypeIndex());
  ftype->SetRetAttrs(TypeAttrs());
}

BlockNode *LMBCLowerer::LowerBlock(BlockNode *block) {
  BlockNode *newblk = mirModule->CurFuncCodeMemPool()->New<BlockNode>();
  if (block->IsEmpty()) {
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
          LowerReturn(*retNode, *newblk);
        }
        break;
      }
      case OP_icallproto:
        FixPrototype4FirstArgReturn(static_cast<IcallNode *>(stmt));
        // fall thru
      case OP_asm:
      case OP_call: {
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

void LMBCLowerer::LowerFunction() {
  // set extern global vars' isDeleted field; will reset when visited
  MapleVector<StIdx>::const_iterator sit = mirModule->GetSymbolDefOrder().begin();
  for (; sit != mirModule->GetSymbolDefOrder().end(); ++sit) {
    MIRSymbol *s = GlobalTables::GetGsymTable().GetSymbolFromStidx(sit->Idx());
    ASSERT_NOT_NULL(s);
    if (s->GetSKind() == kStVar && s->GetStorageClass() == kScExtern && !s->HasPotentialAssignment()) {
      s->SetIsDeleted();
    }
  }

  BlockNode *origbody = func->GetBody();
  BlockNode *newbody = LowerBlock(origbody);
  func->SetBody(newbody);
}

}  // namespace maple
