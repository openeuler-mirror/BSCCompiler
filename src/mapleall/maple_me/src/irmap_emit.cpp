/*
 * Copyright (c) [2019-2021] Huawei Technologies Co.,Ltd.All rights reserved.
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
// This file contains methods to emit Maple IR nodes from MeExpr/MeStmt
#include "me_ir.h"
#include "irmap.h"
#include "mir_builder.h"
#include "orig_symbol.h"

namespace maple {
static MIRFunction *GetCurFunction() {
  return theMIRModule->CurFunction();
}

bool VarMeExpr::IsValidVerIdx() const {
  if (!GetOst()->IsSymbolOst()) {
    return false;
  }
  StIdx stIdx = GetOst()->GetMIRSymbol()->GetStIdx();
  return stIdx.Islocal() ? GetCurFunction()->GetSymTab()->IsValidIdx(stIdx.Idx())
                         : GlobalTables::GetGsymTable().IsValidIdx(stIdx.Idx());
}

BaseNode &VarMeExpr::EmitExpr(MapleAllocator &alloc) {
  MIRSymbol *symbol = GetOst()->GetMIRSymbol();
  if (symbol->IsLocal()) {
    symbol->ResetIsDeleted();
  }
  auto *addrofNode = alloc.New<AddrofNode>(
      OP_dread, PrimType(GetPrimType()), symbol->GetStIdx(), GetOst()->GetFieldID());
  ASSERT(addrofNode->GetPrimType() != kPtyInvalid, "runtime check error");
  ASSERT(IsValidVerIdx(), "runtime check error");
  return *addrofNode;
}

BaseNode &RegMeExpr::EmitExpr(MapleAllocator &alloc) {
  auto *regRead = alloc.New<RegreadNode>();
  regRead->SetPrimType(GetPrimType());
  regRead->SetRegIdx(GetRegIdx());
  ASSERT(GetRegIdx() < 0 ||
         static_cast<uint32>(static_cast<int32>(GetRegIdx())) < GetCurFunction()->GetPregTab()->Size(),
         "RegMeExpr::EmitExpr: pregIdx exceeds preg table size");
  return *regRead;
}

BaseNode &ConstMeExpr::EmitExpr(MapleAllocator &alloc) {
  auto *exprConst = alloc.New<ConstvalNode>(PrimType(GetPrimType()), constVal);
  // if int const has been promoted from dyn int const, remove the type tag
  if (IsPrimitiveInteger(exprConst->GetPrimType())) {
    auto *intConst = safe_cast<MIRIntConst>(exprConst->GetConstVal());
    MIRIntConst *newIntConst = GlobalTables::GetIntConstTable().GetOrCreateIntConst(
        static_cast<uint64>(intConst->GetExtValue()), intConst->GetType());
    exprConst->SetConstVal(newIntConst);
  }
  return *exprConst;
}

BaseNode &ConststrMeExpr::EmitExpr(MapleAllocator &alloc) {
  auto *exprConst = alloc.New<ConststrNode>(PrimType(GetPrimType()), strIdx);
  return *exprConst;
}

BaseNode &Conststr16MeExpr::EmitExpr(MapleAllocator &alloc) {
  auto *exprConst = alloc.New<Conststr16Node>(PrimType(GetPrimType()), strIdx);
  return *exprConst;
}

BaseNode &SizeoftypeMeExpr::EmitExpr(MapleAllocator &alloc) {
  auto *exprSizeoftype = alloc.New<SizeoftypeNode>(PrimType(GetPrimType()), tyIdx);
  return *exprSizeoftype;
}

BaseNode &FieldsDistMeExpr::EmitExpr(MapleAllocator &alloc) {
  auto *exprSizeofType = alloc.New<FieldsDistNode>(PrimType(GetPrimType()), tyIdx, fieldID1, fieldID2);
  return *exprSizeofType;
}

BaseNode &AddrofMeExpr::EmitExpr(MapleAllocator &alloc) {
  MIRSymbol *symbol = GetOst()->GetMIRSymbol();
  if (symbol->IsLocal()) {
    symbol->ResetIsDeleted();
  }
  auto *addrofNode = alloc.New<AddrofNode>(OP_addrof, PrimType(GetPrimType()), symbol->GetStIdx(), GetFieldID());
  return *addrofNode;
}

BaseNode &AddroffuncMeExpr::EmitExpr(MapleAllocator &alloc) {
  auto *addroffuncNode = alloc.New<AddroffuncNode>(PrimType(GetPrimType()), puIdx);
  return *addroffuncNode;
}

BaseNode &AddroflabelMeExpr::EmitExpr(MapleAllocator &alloc) {
  auto *addroflabelNode = alloc.New<AddroflabelNode>(labelIdx);
  addroflabelNode->SetPrimType(PTY_ptr);
  return *addroflabelNode;
}

BaseNode &GcmallocMeExpr::EmitExpr(MapleAllocator &alloc) {
  auto *gcMallocNode = alloc.New<GCMallocNode>(Opcode(GetOp()), PrimType(GetPrimType()), tyIdx);
  return *gcMallocNode;
}

BaseNode &OpMeExpr::EmitExpr(MapleAllocator &alloc) {
  switch (GetOp()) {
    case OP_add:
    case OP_ashr:
    case OP_band:
    case OP_bior:
    case OP_bxor:
    case OP_div:
    case OP_land:
    case OP_lior:
    case OP_cand:
    case OP_cior:
    case OP_lshr:
    case OP_max:
    case OP_min:
    case OP_mul:
    case OP_rem:
    case OP_shl:
    case OP_ror:
    case OP_sub: {
      auto *binaryNode = alloc.New<BinaryNode>(Opcode(GetOp()), PrimType(GetPrimType()));
      binaryNode->SetBOpnd(&opnds[0]->EmitExpr(alloc), 0);
      binaryNode->SetBOpnd(&opnds[1]->EmitExpr(alloc), 1);
      return *binaryNode;
    }
    case OP_eq:
    case OP_ne:
    case OP_lt:
    case OP_gt:
    case OP_le:
    case OP_ge:
    case OP_cmpl:
    case OP_cmpg:
    case OP_cmp: {
      auto *cmpNode = alloc.New<CompareNode>(Opcode(GetOp()), PrimType(GetPrimType()));
      cmpNode->SetBOpnd(&opnds[0]->EmitExpr(alloc), 0);
      cmpNode->SetBOpnd(&opnds[1]->EmitExpr(alloc), 1);
      cmpNode->SetOpndType(opndType);
      return *cmpNode;
    }
    case OP_abs:
    case OP_bnot:
    case OP_lnot:
    case OP_neg:
    case OP_recip:
    case OP_sqrt:
    case OP_alloca:
    case OP_malloc: {
      auto *unaryNode = alloc.New<UnaryNode>(Opcode(GetOp()), PrimType(GetPrimType()));
      unaryNode->SetOpnd(&opnds[0]->EmitExpr(alloc), 0);
      return *unaryNode;
    }
    case OP_sext:
    case OP_zext:
    case OP_extractbits: {
      auto *unode = alloc.New<ExtractbitsNode>(Opcode(GetOp()), PrimType(GetPrimType()));
      unode->SetOpnd(&opnds[0]->EmitExpr(alloc), 0);
      unode->SetBitsOffset(bitsOffset);
      unode->SetBitsSize(bitsSize);
      return *unode;
    }
    case OP_depositbits: {
      auto *bnode = alloc.New<DepositbitsNode>(Opcode(GetOp()), PrimType(GetPrimType()));
      bnode->SetOpnd(&opnds[0]->EmitExpr(alloc), 0);
      bnode->SetOpnd(&opnds[1]->EmitExpr(alloc), 1);
      bnode->SetBitsOffset(bitsOffset);
      bnode->SetBitsSize(bitsSize);
      return *bnode;
    }
    case OP_select: {
      auto *ternaryNode = alloc.New<TernaryNode>(Opcode(GetOp()), PrimType(GetPrimType()));
      const size_t opndNumOfTernary = 3;
      for (size_t i = 0; i < opndNumOfTernary; ++i) {
        ternaryNode->SetOpnd(&opnds[i]->EmitExpr(alloc), i);
      }
      return *ternaryNode;
    }
    case OP_ceil:
    case OP_cvt:
    case OP_floor:
    case OP_round:
    case OP_trunc: {
      auto *cvtNode = alloc.New<TypeCvtNode>(Opcode(GetOp()), PrimType(GetPrimType()));
      cvtNode->SetOpnd(&opnds[0]->EmitExpr(alloc), 0);
      cvtNode->SetFromType(opndType);
      return *cvtNode;
    }
    case OP_retype: {
      auto *cvtNode = alloc.New<RetypeNode>(PrimType(GetPrimType()));
      cvtNode->SetOpnd(&opnds[0]->EmitExpr(alloc), 0);
      cvtNode->SetFromType(opndType);
      cvtNode->SetTyIdx(tyIdx);
      return *cvtNode;
    }
    case OP_gcmallocjarray:
    case OP_gcpermallocjarray: {
      auto *arrayMalloc = alloc.New<JarrayMallocNode>(Opcode(GetOp()), PrimType(GetPrimType()));
      arrayMalloc->SetOpnd(&opnds[0]->EmitExpr(alloc), 0);
      arrayMalloc->SetTyIdx(tyIdx);
      return *arrayMalloc;
    }
    case OP_resolveinterfacefunc:
    case OP_resolvevirtualfunc: {
      auto *resolveNode = alloc.New<ResolveFuncNode>(Opcode(GetOp()), PrimType(GetPrimType()));
      resolveNode->SetBOpnd(&opnds[0]->EmitExpr(alloc), 0);
      resolveNode->SetBOpnd(&opnds[1]->EmitExpr(alloc), 1);
      resolveNode->SetPUIdx(fieldID);
      return *resolveNode;
    }
    case OP_iaddrof: {
      auto *iaddrof = alloc.New<IaddrofNode>(OP_iaddrof, PrimType(GetPrimType()));
      iaddrof->SetOpnd(&opnds[0]->EmitExpr(alloc), 0);
      iaddrof->SetTyIdx(tyIdx);
      iaddrof->SetFieldID(fieldID);
      return *iaddrof;
    }
    default:
      CHECK_FATAL(false, "unexpected op");
  }
}

BaseNode &NaryMeExpr::EmitExpr(MapleAllocator &alloc) {
  BaseNode *nodeToReturn = nullptr;
  NaryOpnds *nopndPart = nullptr;
  if (GetOp() == OP_array) {
    auto *arrayNode = alloc.New<ArrayNode>(alloc, PrimType(GetPrimType()), tyIdx);
    arrayNode->SetNumOpnds(GetNumOpnds());
    arrayNode->SetBoundsCheck(GetBoundCheck());
    nopndPart = arrayNode;
    nodeToReturn = arrayNode;
  } else {
    auto *intrinNode = alloc.New<IntrinsicopNode>(alloc, GetOp(), PrimType(GetPrimType()), tyIdx);
    intrinNode->SetNumOpnds(GetNumOpnds());
    intrinNode->SetIntrinsic(intrinsic);
    nopndPart = intrinNode;
    nodeToReturn = intrinNode;
  }
  for (auto it = GetOpnds().begin(); it != GetOpnds().end(); ++it) {
    nopndPart->GetNopnd().push_back(&(*it)->EmitExpr(alloc));
  }
  return *nodeToReturn;
}

BaseNode &IvarMeExpr::EmitExpr(MapleAllocator &alloc) {
  CHECK_NULL_FATAL(base);
  auto *ireadNode = alloc.New<IreadNode>(OP_iread, PrimType(GetPrimType()));
  if (offset == 0) {
    ireadNode->SetOpnd(&base->EmitExpr(alloc), 0);
  } else {
    auto *mirType = GlobalTables::GetTypeTable().GetInt32();
    auto *mirConst = GlobalTables::GetIntConstTable().GetOrCreateIntConst(static_cast<uint32>(offset), *mirType);
    auto *constValNode = alloc.New<ConstvalNode>(mirType->GetPrimType(), mirConst);
    auto *newAddrNode =
        alloc.New<BinaryNode>(OP_add, base->GetPrimType(), &(base->EmitExpr(alloc)), constValNode);
    ireadNode->SetOpnd(newAddrNode, 0);
  }
  ireadNode->SetFieldID(fieldID);
  ireadNode->SetTyIdx(tyIdx);
  ASSERT(ireadNode->GetPrimType() != kPtyInvalid, "");
  ASSERT(tyIdx != 0, "wrong tyIdx for iread node in me emit");
  ireadNode->SetTyIdx(tyIdx);
  return *ireadNode;
}

StmtNode &MeStmt::EmitStmt(MapleAllocator &alloc) {
  auto *stmt = alloc.New<StmtNode>(Opcode(GetOp()));
  return *stmt;
}

StmtNode &DassignMeStmt::EmitStmt(MapleAllocator &alloc) {
  if (GetEmitDassignoff()) {
    OriginalSt *lhsSt = GetLHS()->GetOst();
    MIRSymbol *lhsMirSt = lhsSt->GetMIRSymbol();
    if (lhsMirSt->IsLocal()) {
      lhsMirSt->ResetIsDeleted();
    }
    StIdx lhsStIdx = lhsMirSt->GetStIdx();
    int32 offset = (GetVarLHS()->GetOst()->GetOffset().val) / 8;
    PrimType rhsType = GetRHS()->GetPrimType();
    int64 val = static_cast<ConstMeExpr *>(GetRHS())->GetExtIntValue();
    ConstvalNode *rhsNode = theMIRModule->GetMIRBuilder()->CreateIntConst(static_cast<uint64>(val), rhsType);
    DassignoffNode *dassignoffNode = alloc.New<DassignoffNode>(lhsStIdx, offset, rhsType, rhsNode);
    return *dassignoffNode;
  } else {
    auto *dassignStmt = alloc.New<DassignNode>();
    MIRSymbol *symbol = GetVarLHS()->GetOst()->GetMIRSymbol();
    if (symbol->IsLocal()) {
      symbol->ResetIsDeleted();
    }
    dassignStmt->SetStIdx(symbol->GetStIdx());
    dassignStmt->SetFieldID(GetVarLHS()->GetOst()->GetFieldID());
    dassignStmt->SetRHS(&GetRHS()->EmitExpr(alloc));
    return *dassignStmt;
  }
}

StmtNode &AssignMeStmt::EmitStmt(MapleAllocator &alloc) {
  CHECK_NULL_FATAL(lhs);
  CHECK_NULL_FATAL(rhs);
  RegassignNode *regassignStmt =
      alloc.New<RegassignNode>(lhs->GetPrimType(), lhs->GetRegIdx(), &rhs->EmitExpr(alloc));
  return *regassignStmt;
}

StmtNode &MaydassignMeStmt::EmitStmt(MapleAllocator &alloc) {
  CHECK_NULL_FATAL(rhs);
  auto *dassignStmt = alloc.New<DassignNode>();
  MIRSymbol *symbol = mayDSSym->GetMIRSymbol();
  if (symbol->IsLocal()) {
    symbol->ResetIsDeleted();
  }
  dassignStmt->SetStIdx(symbol->GetStIdx());
  dassignStmt->SetFieldID(fieldID);
  dassignStmt->SetRHS(&rhs->EmitExpr(alloc));
  return *dassignStmt;
}

void MeStmt::EmitCallReturnVector(CallReturnVector &nRets) {
  MapleVector<MustDefMeNode> *mustDefs = GetMustDefList();
  CHECK_FATAL(mustDefs != nullptr, "EmitCallReturnVector: mustDefList cannot be null");
  for (MustDefMeNode &mustdef : *mustDefs) {
    MeExpr *meExpr = mustdef.GetLHS();
    if (meExpr->GetMeOp() == kMeOpVar) {
      OriginalSt *ost = static_cast<VarMeExpr*>(meExpr)->GetOst();
      MIRSymbol *symbol = ost->GetMIRSymbol();
      nRets.push_back(CallReturnPair(symbol->GetStIdx(), RegFieldPair(0, 0)));
    } else if (meExpr->GetMeOp() == kMeOpReg) {
      nRets.push_back(CallReturnPair(StIdx(), RegFieldPair(0, static_cast<RegMeExpr*>(meExpr)->GetRegIdx())));
    }
  }
}

StmtNode &IassignMeStmt::EmitStmt(MapleAllocator &alloc) {
  CHECK_NULL_FATAL(lhsVar);
  CHECK_NULL_FATAL(rhs);
  if (GetEmitIassignoff()) {
    PrimType rhsType = GetOpnd(1)->GetPrimType();
    TyIdx lhsTyIdx = GetLHSVal()->GetTyIdx();
    MIRPtrType *lhsMirPtrType = static_cast<MIRPtrType*>(GlobalTables::GetTypeTable().GetTypeFromTyIdx(lhsTyIdx));
    MIRStructType *lhsStructType = static_cast<MIRStructType *>(lhsMirPtrType->GetPointedType());
    IvarMeExpr *iVar = GetLHSVal();
    int32 offset = static_cast<int32>(lhsStructType->GetBitOffsetFromBaseAddr(iVar->GetFieldID()) / 8);

    ASSERT(static_cast<ConstMeExpr *> (GetOpnd(1)), "IassignMeStmt does NOT have Const RHS");
    BaseNode *addrNode = nullptr;
    if (lhsVar->GetOffset() == 0) {
      addrNode = &lhsVar->GetBase()->EmitExpr(alloc);
    } else {
      MIRType *mirType = GlobalTables::GetTypeTable().GetInt32();
      MIRIntConst *mirOffsetConst = GlobalTables::GetIntConstTable().GetOrCreateIntConst(
          static_cast<uint32>(lhsVar->GetOffset()), *mirType);
      ConstvalNode *offsetNode = alloc.New<ConstvalNode>(mirType->GetPrimType(), mirOffsetConst);
      addrNode = alloc.New<BinaryNode>(
          OP_add, lhsVar->GetBase()->GetPrimType(), &(lhsVar->GetBase()->EmitExpr(alloc)), offsetNode);
    }
    int64 val = static_cast<ConstMeExpr *>(GetOpnd(1))->GetExtIntValue();
    ConstvalNode *rhsNode = theMIRModule->GetMIRBuilder()->CreateIntConst(static_cast<uint64>(val), rhsType);
    IassignoffNode *iassignoffNode = alloc.New<IassignoffNode>(rhsType, offset, addrNode, rhsNode);
    return *iassignoffNode;
  } else {
    auto *iassignNode = alloc.New<IassignNode>();
    iassignNode->SetTyIdx(tyIdx);
    iassignNode->SetFieldID(lhsVar->GetFieldID());
    if (lhsVar->GetOffset() == 0) {
      iassignNode->SetAddrExpr(&lhsVar->GetBase()->EmitExpr(alloc));
    } else {
      auto *mirType = GlobalTables::GetTypeTable().GetInt32();
      auto *mirConst = GlobalTables::GetIntConstTable().GetOrCreateIntConst(
          static_cast<uint32>(lhsVar->GetOffset()), *mirType);
      auto *constValNode = alloc.New<ConstvalNode>(mirType->GetPrimType(), mirConst);
      auto *newAddrNode = alloc.New<BinaryNode>(
          OP_add, lhsVar->GetBase()->GetPrimType(), &(lhsVar->GetBase()->EmitExpr(alloc)), constValNode);
      iassignNode->SetAddrExpr(newAddrNode);
    }
    iassignNode->SetRHS(&rhs->EmitExpr(alloc));
    return *iassignNode;
  }
}
const MIRFunction &CallMeStmt::GetTargetFunction() const {
  return *GlobalTables::GetFunctionTable().GetFunctionFromPuidx(puIdx);
}

MIRFunction &CallMeStmt::GetTargetFunction() {
  return *GlobalTables::GetFunctionTable().GetFunctionFromPuidx(puIdx);
}

StmtNode &CallMeStmt::EmitStmt(MapleAllocator &alloc) {
  if (GetOp() != OP_icall && GetOp() != OP_icallassigned &&
      GetOp() != OP_icallproto && GetOp() != OP_icallprotoassigned) {
    auto *callNode = alloc.New<CallNode>(alloc, Opcode(GetOp()));
    callNode->SetPUIdx(puIdx);
    callNode->SetTyIdx(tyIdx);
    callNode->GetNopnd().resize(NumMeStmtOpnds());
    for (size_t i = 0; i < NumMeStmtOpnds(); ++i) {
      callNode->SetOpnd(&GetOpnd(i)->EmitExpr(alloc), i);
    }
    callNode->SetNumOpnds(callNode->GetNopndSize());
    if (kOpcodeInfo.IsCallAssigned(GetOp())) {
      EmitCallReturnVector(callNode->GetReturnVec());
      for (size_t j = 0; j < callNode->GetReturnVec().size(); ++j) {
        CallReturnPair retPair = callNode->GetReturnVec()[j];
        if (!retPair.second.IsReg()) {
          StIdx stIdx = retPair.first;
          if (stIdx.Islocal()) {
            MIRSymbolTable *symbolTab = GetCurFunction()->GetSymTab();
            MIRSymbol *symbol = symbolTab->GetSymbolFromStIdx(stIdx.Idx());
            CHECK_FATAL(symbol, "symbol is nullptr!");
            symbol->ResetIsDeleted();
          }
        }
      }
    }
    return *callNode;
  }
  auto *icallNode = alloc.New<IcallNode>(alloc, Opcode(GetOp()));
  icallNode->GetNopnd().resize(NumMeStmtOpnds());
  for (size_t i = 0; i < NumMeStmtOpnds(); ++i) {
    icallNode->SetOpnd(&GetOpnd(i)->EmitExpr(alloc), i);
  }
  icallNode->SetNumOpnds(icallNode->GetNopndSize());
  if (kOpcodeInfo.IsCallAssigned(GetOp())) {
    EmitCallReturnVector(icallNode->GetReturnVec());
    icallNode->SetRetTyIdx(TyIdx(PTY_void));
    for (size_t j = 0; j < icallNode->GetReturnVec().size(); ++j) {
      CallReturnPair retPair = icallNode->GetReturnVec()[j];
      if (!retPair.second.IsReg()) {
        StIdx stIdx = retPair.first;
        MIRSymbolTable *symbolTab = GetCurFunction()->GetSymTab();
        MIRSymbol *symbol = symbolTab->GetSymbolFromStIdx(stIdx.Idx());
        ASSERT_NOT_NULL(symbol);
        icallNode->SetRetTyIdx(symbol->GetType()->GetTypeIndex());
        if (stIdx.Islocal()) {
          symbol->ResetIsDeleted();
        }
      } else {
        auto pregIdx = PregIdx(retPair.second.GetPregIdx());
        MIRPreg *preg = GetCurFunction()->GetPregTab()->PregFromPregIdx(pregIdx);
        icallNode->SetRetTyIdx(TyIdx(preg->GetPrimType()));
      }
    }
  }
  return *icallNode;
}

StmtNode &IcallMeStmt::EmitStmt(MapleAllocator &alloc) {
  auto *icallNode = alloc.New<IcallNode>(alloc, Opcode(GetOp()));
  icallNode->GetNopnd().resize(NumMeStmtOpnds());
  for (size_t i = 0; i < NumMeStmtOpnds(); ++i) {
    icallNode->SetOpnd(&GetOpnd(i)->EmitExpr(alloc), i);
  }
  icallNode->SetNumOpnds(icallNode->GetNopndSize());
  if (kOpcodeInfo.IsCallAssigned(GetOp())) {
    EmitCallReturnVector(icallNode->GetReturnVec());
    icallNode->SetRetTyIdx(TyIdx(PTY_void));
    for (size_t j = 0; j < icallNode->GetReturnVec().size(); ++j) {
      CallReturnPair retPair = icallNode->GetReturnVec()[j];
      if (!retPair.second.IsReg()) {
        StIdx stIdx = retPair.first;
        MIRSymbolTable *symbolTab = GetCurFunction()->GetSymTab();
        MIRSymbol *symbol = symbolTab->GetSymbolFromStIdx(stIdx.Idx());
        icallNode->SetRetTyIdx(symbol->GetType()->GetTypeIndex());
        if (stIdx.Islocal()) {
          symbol->ResetIsDeleted();
        }
      } else {
        auto pregIdx = PregIdx(retPair.second.GetPregIdx());
        MIRPreg *preg = GetCurFunction()->GetPregTab()->PregFromPregIdx(pregIdx);
        icallNode->SetRetTyIdx(TyIdx(preg->GetPrimType()));
      }
    }
  }
  if (GetOp() == OP_icallproto || GetOp() == OP_icallprotoassigned) {
    icallNode->SetRetTyIdx(retTyIdx);
  }
  return *icallNode;
}

StmtNode &IntrinsiccallMeStmt::EmitStmt(MapleAllocator &alloc) {
  auto *callNode = alloc.New<IntrinsiccallNode>(alloc, Opcode(GetOp()));
  callNode->SetIntrinsic(intrinsic);
  callNode->SetTyIdx(tyIdx);
  callNode->GetNopnd().resize(NumMeStmtOpnds());
  for (size_t i = 0; i < NumMeStmtOpnds(); ++i) {
    callNode->SetOpnd(&GetOpnd(i)->EmitExpr(alloc), i);
  }
  callNode->SetNumOpnds(callNode->GetNopndSize());
  if (kOpcodeInfo.IsCallAssigned(GetOp())) {
    EmitCallReturnVector(callNode->GetReturnVec());
    for (size_t j = 0; j < callNode->GetReturnVec().size(); ++j) {
      CallReturnPair retPair = callNode->GetReturnVec()[j];
      if (!retPair.second.IsReg()) {
        StIdx stIdx = retPair.first;
        if (stIdx.Islocal()) {
          MIRSymbolTable *symbolTab = GetCurFunction()->GetSymTab();
          MIRSymbol *symbol = symbolTab->GetSymbolFromStIdx(stIdx.Idx());
          ASSERT_NOT_NULL(symbol);
          symbol->ResetIsDeleted();
        }
      }
    }
  }
  return *callNode;
}

StmtNode &AsmMeStmt::EmitStmt(MapleAllocator &alloc) {
  AsmNode *asmNode = alloc.New<AsmNode>(&alloc);
  asmNode->GetNopnd().resize(NumMeStmtOpnds());
  for (size_t i = 0; i < NumMeStmtOpnds(); ++i) {
    asmNode->SetOpnd(&GetOpnd(i)->EmitExpr(alloc), i);
  }
  asmNode->SetNumOpnds(static_cast<uint8>(asmNode->GetNopndSize()));
  EmitCallReturnVector(*asmNode->GetCallReturnVector());
  for (size_t j = 0; j < asmNode->GetCallReturnVector()->size(); ++j) {
    CallReturnPair retPair = (*asmNode->GetCallReturnVector())[j];
    if (!retPair.second.IsReg()) {
      StIdx stIdx = retPair.first;
      if (stIdx.Islocal()) {
        MIRSymbolTable *symbolTab = GetCurFunction()->GetSymTab();
        MIRSymbol *symbol = symbolTab->GetSymbolFromStIdx(stIdx.Idx());
        ASSERT_NOT_NULL(symbol);
        symbol->ResetIsDeleted();
      }
    }
  }
  asmNode->asmString = asmString;
  asmNode->inputConstraints = inputConstraints;
  asmNode->outputConstraints = outputConstraints;
  asmNode->clobberList = clobberList;
  asmNode->gotoLabels = gotoLabels;
  asmNode->qualifiers = qualifiers;
  return *asmNode;
}

StmtNode &NaryMeStmt::EmitStmt(MapleAllocator &alloc) {
  auto *naryStmt = alloc.New<NaryStmtNode>(alloc, Opcode(GetOp()));
  naryStmt->GetNopnd().resize(NumMeStmtOpnds());
  for (size_t i = 0; i < NumMeStmtOpnds(); ++i) {
    naryStmt->SetOpnd(&opnds[i]->EmitExpr(alloc), i);
  }
  naryStmt->SetNumOpnds(naryStmt->GetNopndSize());
  return *naryStmt;
}

StmtNode &UnaryMeStmt::EmitStmt(MapleAllocator &alloc) {
  auto *unaryStmt = alloc.New<UnaryStmtNode>(Opcode(GetOp()));
  CHECK_NULL_FATAL(opnd);
  unaryStmt->SetOpnd(&opnd->EmitExpr(alloc), 0);
  return *unaryStmt;
}

StmtNode &CallAssertNonnullMeStmt::EmitStmt(MapleAllocator &alloc) {
  std::string funcName = GetFuncName();
  std::string stmtFuncName = GetStmtFuncName();
  GStrIdx stridx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(funcName);
  GStrIdx stmtStridx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(stmtFuncName);
  auto *ret = alloc.New<CallAssertNonnullStmtNode>(GetOp(), stridx, GetParamIndex(), stmtStridx);
  ret->SetOpnd(&GetOpnd()->EmitExpr(alloc), 0);
  return *ret;
}

StmtNode &AssertNonnullMeStmt::EmitStmt(MapleAllocator &alloc) {
  GStrIdx stridx = GetFuncNameIdx();
  auto *ret = alloc.New<AssertNonnullStmtNode>(GetOp(), stridx);
  ret->SetOpnd(&GetOpnd()->EmitExpr(alloc), 0);
  return *ret;
}

StmtNode &AssertBoundaryMeStmt::EmitStmt(MapleAllocator &alloc) {
  std::string funcName = GetFuncName();
  GStrIdx stridx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(funcName);
  auto *ret = alloc.New<AssertBoundaryStmtNode>(alloc, GetOp(), stridx);
  ret->GetNopnd().resize(NumMeStmtOpnds());
  for (size_t i = 0; i < NumMeStmtOpnds(); ++i) {
    ret->SetOpnd(&GetOpnd(i)->EmitExpr(alloc), i);
  }
  ret->SetNumOpnds(static_cast<uint8>(ret->GetNopndSize()));
  return *ret;
}

StmtNode &CallAssertBoundaryMeStmt::EmitStmt(MapleAllocator &alloc) {
  std::string funcName = GetFuncName();
  std::string stmtFuncName = GetStmtFuncName();
  GStrIdx stridx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(funcName);
  GStrIdx stmtStridx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(stmtFuncName);
  auto *ret = alloc.New<CallAssertBoundaryStmtNode>(alloc, GetOp(), stridx, GetParamIndex(), stmtStridx);
  ret->GetNopnd().resize(NumMeStmtOpnds());
  for (size_t i = 0; i < NumMeStmtOpnds(); ++i) {
    ret->SetOpnd(&GetOpnd(i)->EmitExpr(alloc), i);
  }
  ret->SetNumOpnds(static_cast<uint8>(ret->GetNopndSize()));
  return *ret;
}

StmtNode &GotoMeStmt::EmitStmt(MapleAllocator &alloc) {
  auto *gotoNode = alloc.New<GotoNode>(OP_goto);
  gotoNode->SetOffset(offset);
  return *gotoNode;
}

StmtNode &CondGotoMeStmt::EmitStmt(MapleAllocator &alloc) {
  auto *cgNode = alloc.New<CondGotoNode>(Opcode(GetOp()));
  cgNode->SetOffset(offset);
  cgNode->SetBranchProb(branchProb);
  cgNode->SetOpnd(&GetOpnd()->EmitExpr(alloc), 0);
  return *cgNode;
}

StmtNode &JsTryMeStmt::EmitStmt(MapleAllocator &alloc) {
  auto *jtNode = alloc.New<JsTryNode>();
  jtNode->SetCatchOffset(catchOffset);
  jtNode->SetFinallyOffset(finallyOffset);
  return *jtNode;
}

StmtNode &TryMeStmt::EmitStmt(MapleAllocator &alloc) {
  auto *tryNode = alloc.New<TryNode>(alloc);
  tryNode->ResizeOffsets(offsets.size());
  for (size_t i = 0; i < offsets.size(); ++i) {
    tryNode->SetOffset(offsets[i], i);
  }
  return *tryNode;
}

StmtNode &CatchMeStmt::EmitStmt(MapleAllocator &alloc) {
  auto *catchNode = alloc.New<CatchNode>(alloc);
  catchNode->SetExceptionTyIdxVec(exceptionTyIdxVec);
  return *catchNode;
}

StmtNode &SwitchMeStmt::EmitStmt(MapleAllocator &alloc) {
  auto *switchNode = alloc.New<SwitchNode>(alloc);
  switchNode->SetDefaultLabel(defaultLabel);
  switchNode->SetSwitchTable(switchTable);
  switchNode->SetSwitchOpnd(&GetOpnd()->EmitExpr(alloc));
  return *switchNode;
}

StmtNode &CommentMeStmt::EmitStmt(MapleAllocator &alloc) {
  auto *commentNode = alloc.New<CommentNode>(alloc);
  commentNode->SetComment(comment);
  return *commentNode;
}

StmtNode &ThrowMeStmt::EmitStmt(MapleAllocator &alloc) {
  auto *unaryStmt = alloc.New<UnaryStmtNode>(OP_throw);
  CHECK_NULL_FATAL(opnd);
  unaryStmt->SetOpnd(&opnd->EmitExpr(alloc), 0);
  return *unaryStmt;
}

StmtNode &GosubMeStmt::EmitStmt(MapleAllocator &alloc) {
  auto *gosubNode = alloc.New<GotoNode>(OP_gosub);
  gosubNode->SetOffset(offset);
  return *gosubNode;
}

void BB::EmitBB(BlockNode &curblk, bool needAnotherPass) {
  StmtNode *bbFirstStmt = nullptr;
  StmtNode *bbLastStmt = nullptr;
  // emit head. label
  LabelIdx labidx = GetBBLabel();
  if (labidx != 0) {
    LabelNode *lbnode = GetCurFunction()->GetCodeMemPool()->New<LabelNode>();
    lbnode->SetLabelIdx(labidx);
    curblk.AddStatement(lbnode);
    bbFirstStmt = lbnode;
    bbLastStmt = lbnode;
  }
  auto &meStmts = GetMeStmts();
  for (auto &meStmt : meStmts) {
    if (!needAnotherPass) {
      if (meStmt.GetOp() == OP_interfaceicall) {
        meStmt.SetOp(OP_icall);
      } else if (meStmt.GetOp() == OP_interfaceicallassigned) {
        meStmt.SetOp(OP_icallassigned);
      }
    }
    StmtNode *stmt = &meStmt.EmitStmt(GetCurFunction()->GetCodeMPAllocator());
    stmt->SetSrcPos(meStmt.GetSrcPosition());
    stmt->SetOriginalID(meStmt.GetOriginalId());
    stmt->CopySafeRegionAttr(meStmt.GetStmtAttr());
    if (meStmt.GetMayTailCall()) {
      stmt->SetMayTailcall();
    }
    curblk.AddStatement(stmt);
    if (bbFirstStmt == nullptr) {
      bbFirstStmt = stmt;
    }
    bbLastStmt = stmt;
    if (Options::profileUse && GetCurFunction()->GetFuncProfData() &&
        (IsCallAssigned(stmt->GetOpCode()) || stmt->GetOpCode() == OP_call)) {
      GetCurFunction()->GetFuncProfData()->SetStmtFreq(stmt->GetStmtID(), frequency);
    }
  }
  if (GetAttributes(kBBAttrIsTryEnd)) {
    // generate op_endtry
    StmtNode *endtry = GetCurFunction()->GetCodeMemPool()->New<StmtNode>(OP_endtry);
    curblk.AddStatement(endtry);
    if (bbFirstStmt == nullptr) {
      bbFirstStmt = endtry;
    }
    bbLastStmt = endtry;
  }
  stmtNodeList.set_first(bbFirstStmt);
  stmtNodeList.set_last(bbLastStmt);
  // set stmt freq
  if (Options::profileUse && GetCurFunction()->GetFuncProfData()) {
    if (bbFirstStmt) {
      GetCurFunction()->GetFuncProfData()->SetStmtFreq(bbFirstStmt->GetStmtID(), frequency);
    }
    if (bbLastStmt && (bbFirstStmt != bbLastStmt)) {
      GetCurFunction()->GetFuncProfData()->SetStmtFreq(bbLastStmt->GetStmtID(), frequency);
    }
    if (stmtNodeList.empty()) {
      auto *commentNode = GetCurFunction()->GetCodeMemPool()->New<CommentNode>(*theMIRModule);
      commentNode->SetComment("profileStmt");
      GetCurFunction()->GetFuncProfData()->SetStmtFreq(commentNode->GetStmtID(), frequency);
      curblk.AddStatement(commentNode);
      stmtNodeList.set_first(commentNode);
      stmtNodeList.set_last(commentNode);
    }
  }
}
}  // namespace maple
