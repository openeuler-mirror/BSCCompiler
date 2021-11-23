/*
 * Copyright (c) [2021] Huawei Technologies Co., Ltd. All rights reserved.
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

#include "me_irmap.h"
#include "lfo_function.h"
#include "lfo_pre_emit.h"
#include "mir_lower.h"
#include "constantfold.h"

namespace maple {
// convert x to use OP_array if possible; return nullptr if unsuccessful;
// ptrTyIdx is the high level pointer type of x
ArrayNode *LfoPreEmitter::ConvertToArray(BaseNode *x, TyIdx ptrTyIdx) {
  if (x->GetOpCode() != OP_add) {
    return nullptr;
  }
  MIRType *mirType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(ptrTyIdx);
  if (mirType->GetKind() != kTypePointer) {
    return nullptr;
  }
  BaseNode *opnd0 = x->Opnd(0);
  if (!GetMexpr(opnd0)->HasAddressValue()) {
    return nullptr;
  }
  BaseNode *opnd1 = x->Opnd(1);
  MIRType *elemType = static_cast<MIRPtrType *>(mirType)->GetPointedType();
  size_t elemSize = elemType->GetSize();
  BaseNode *indexOpnd = opnd1;
  if (elemSize > 1) {
    if (opnd1->GetOpCode() != OP_mul) {
      return nullptr;
    }
    BaseNode *mulopnd1 = opnd1->Opnd(1);
    if (mulopnd1->GetOpCode() != OP_constval) {
      return nullptr;
    }
    MIRConst *constVal = static_cast<ConstvalNode *>(mulopnd1)->GetConstVal();
    if (constVal->GetKind() != kConstInt) {
      return nullptr;
    }
    if (static_cast<MIRIntConst *>(constVal)->GetValue() != elemSize) {
      return nullptr;
    }
    indexOpnd = opnd1->Opnd(0);
  }
  if (indexOpnd->GetOpCode() == OP_cvt) {
    if (IsPrimitiveInteger(static_cast<TypeCvtNode *>(indexOpnd)->FromType())) {
      indexOpnd = indexOpnd->Opnd(0);
    }
  }
  // form the pointer to array type
  MIRArrayType *arryType = GlobalTables::GetTypeTable().GetOrCreateArrayType(*elemType, 0);
  MIRType *ptArryType = GlobalTables::GetTypeTable().GetOrCreatePointerType(*arryType);
  ArrayNode *arryNode = codeMP->New<ArrayNode>(*codeMPAlloc, x->GetPrimType(), ptArryType->GetTypeIndex());
  arryNode->SetBoundsCheck(false);
  arryNode->GetNopnd().push_back(opnd0);
  arryNode->GetNopnd().push_back(indexOpnd);
  arryNode->SetNumOpnds(2);
  lfoExprParts[arryNode] = lfoExprParts[x];
  return arryNode;
}

BaseNode *LfoPreEmitter::EmitLfoExpr(MeExpr *meexpr, BaseNode *parent) {
  LfoPart *lfopart = lfoMP->New<LfoPart>(parent, meexpr);
  switch (meexpr->GetOp()) {
    case OP_constval: {
      MIRConst *constval = static_cast<ConstMeExpr *>(meexpr)->GetConstVal();
      ConstvalNode *lcvlNode = codeMP->New<ConstvalNode>(constval->GetType().GetPrimType(), constval);
      lfoExprParts[lcvlNode] = lfopart;
      return lcvlNode;
    }
    case OP_dread: {
      VarMeExpr *varmeexpr = static_cast<VarMeExpr *>(meexpr);
      MIRSymbol *sym = varmeexpr->GetOst()->GetMIRSymbol();
      if (sym->IsLocal()) {
        sym->ResetIsDeleted();
      }
      AddrofNode *dreadnode = codeMP->New<AddrofNode>(OP_dread, varmeexpr->GetPrimType(), sym->GetStIdx(),
                                                      varmeexpr->GetOst()->GetFieldID());
      lfoExprParts[dreadnode] = lfopart;
      return dreadnode;
    }
    case OP_eq:
    case OP_ne:
    case OP_ge:
    case OP_gt:
    case OP_le:
    case OP_cmp:
    case OP_cmpl:
    case OP_cmpg:
    case OP_lt: {
      OpMeExpr *cmpexpr = static_cast<OpMeExpr *>(meexpr);
      CompareNode *cmpNode =
          codeMP->New<CompareNode>(meexpr->GetOp(), cmpexpr->GetPrimType(), cmpexpr->GetOpndType(), nullptr, nullptr);
      BaseNode *opnd0 = EmitLfoExpr(cmpexpr->GetOpnd(0), cmpNode);
      BaseNode *opnd1 = EmitLfoExpr(cmpexpr->GetOpnd(1), cmpNode);
      cmpNode->SetBOpnd(opnd0, 0);
      cmpNode->SetBOpnd(opnd1, 1);
      cmpNode->SetOpndType(cmpNode->GetOpndType());
      lfoExprParts[cmpNode] = lfopart;
      return cmpNode;
    }
    case OP_array: {
      NaryMeExpr *arrExpr = static_cast<NaryMeExpr *>(meexpr);
      ArrayNode *arrNode =
        codeMP->New<ArrayNode>(*codeMPAlloc, arrExpr->GetPrimType(), arrExpr->GetTyIdx());
      arrNode->SetBoundsCheck(arrExpr->GetBoundCheck());
      for (uint32 i = 0; i < arrExpr->GetNumOpnds(); i++) {
        BaseNode *opnd = EmitLfoExpr(arrExpr->GetOpnd(i), arrNode);
        arrNode->GetNopnd().push_back(opnd);
      }
      arrNode->SetNumOpnds(meexpr->GetNumOpnds());
      lfoExprParts[arrNode] = lfopart;
      return arrNode;
    }
    case OP_ashr:
    case OP_band:
    case OP_bior:
    case OP_bxor:
    case OP_cand:
    case OP_cior:
    case OP_div:
    case OP_land:
    case OP_lior:
    case OP_lshr:
    case OP_max:
    case OP_min:
    case OP_mul:
    case OP_rem:
    case OP_shl:
    case OP_sub:
    case OP_add: {
      OpMeExpr *opExpr = static_cast<OpMeExpr *>(meexpr);
      BinaryNode *binNode = codeMP->New<BinaryNode>(meexpr->GetOp(), meexpr->GetPrimType());
      binNode->SetBOpnd(EmitLfoExpr(opExpr->GetOpnd(0), binNode), 0);
      binNode->SetBOpnd(EmitLfoExpr(opExpr->GetOpnd(1), binNode), 1);
      lfoExprParts[binNode] = lfopart;
      return binNode;
    }
    case OP_iread: {
      IvarMeExpr *ivarExpr = static_cast<IvarMeExpr *>(meexpr);
      IreadNode *irdNode = codeMP->New<IreadNode>(meexpr->GetOp(), meexpr->GetPrimType());
      ASSERT(ivarExpr->GetOffset() == 0, "offset in iread should be 0");
      irdNode->SetOpnd(EmitLfoExpr(ivarExpr->GetBase(), irdNode), 0);
      irdNode->SetTyIdx(ivarExpr->GetTyIdx());
      irdNode->SetFieldID(ivarExpr->GetFieldID());
      lfoExprParts[irdNode] = lfopart;
      if (irdNode->Opnd(0)->GetOpCode() != OP_array) {
        ArrayNode *arryNode = ConvertToArray(irdNode->Opnd(0), irdNode->GetTyIdx());
        if (arryNode != nullptr) {
          irdNode->SetOpnd(arryNode, 0);
        }
      }
      return irdNode;
    }
    case OP_ireadoff: {
      IvarMeExpr *ivarExpr = static_cast<IvarMeExpr *>(meexpr);
      IreadNode *irdNode = codeMP->New<IreadNode>(OP_iread, meexpr->GetPrimType());
      MeExpr *baseexpr = ivarExpr->GetBase();
      if (ivarExpr->GetOffset() == 0) {
        irdNode->SetOpnd(EmitLfoExpr(baseexpr, irdNode), 0);
      } else {
        MIRType *mirType = GlobalTables::GetTypeTable().GetInt32();
        MIRIntConst *mirConst = GlobalTables::GetIntConstTable().GetOrCreateIntConst(ivarExpr->GetOffset(), *mirType);
        ConstvalNode *constValNode = codeMP->New<ConstvalNode>(mirType->GetPrimType(), mirConst);
        LfoPart *lfopart2 = lfoMP->New<LfoPart>(irdNode, baseexpr);
        lfoExprParts[constValNode] = lfopart2;
        BinaryNode *newAddrNode =
            codeMP->New<BinaryNode>(OP_add, baseexpr->GetPrimType(), EmitLfoExpr(baseexpr, irdNode), constValNode);
        lfoExprParts[newAddrNode] = lfopart2;
        irdNode->SetOpnd(newAddrNode, 0);
      }
      irdNode->SetTyIdx(ivarExpr->GetTyIdx());
      irdNode->SetFieldID(ivarExpr->GetFieldID());
      lfoExprParts[irdNode] = lfopart;
      return irdNode;
    }
    case OP_addrof: {
      AddrofMeExpr *addrMeexpr = static_cast<AddrofMeExpr *> (meexpr);
      OriginalSt *ost = meirmap->GetSSATab().GetOriginalStFromID(addrMeexpr->GetOstIdx());
      MIRSymbol *sym = ost->GetMIRSymbol();
      AddrofNode *addrofNode =
          codeMP->New<AddrofNode>(OP_addrof, addrMeexpr->GetPrimType(), sym->GetStIdx(), ost->GetFieldID());
      lfoExprParts[addrofNode] = lfopart;
      return addrofNode;
    }
    case OP_addroflabel: {
      AddroflabelMeExpr *addroflabelexpr = static_cast<AddroflabelMeExpr *>(meexpr);
      AddroflabelNode *addroflabel = codeMP->New<AddroflabelNode>(addroflabelexpr->labelIdx);
      addroflabel->SetPrimType(meexpr->GetPrimType());
      lfoExprParts[addroflabel] = lfopart;
      return addroflabel;
    }
    case OP_addroffunc: {
      AddroffuncMeExpr *addrMeexpr = static_cast<AddroffuncMeExpr *>(meexpr);
      AddroffuncNode *addrfunNode = codeMP->New<AddroffuncNode>(addrMeexpr->GetPrimType(), addrMeexpr->GetPuIdx());
      lfoExprParts[addrfunNode] = lfopart;
      return addrfunNode;
    }
    case OP_gcmalloc:
    case OP_gcpermalloc:
    case OP_stackmalloc: {
      GcmallocMeExpr *gcMeexpr = static_cast<GcmallocMeExpr *> (meexpr);
      GCMallocNode *gcMnode =
          codeMP->New<GCMallocNode>(meexpr->GetOp(), meexpr->GetPrimType(), gcMeexpr->GetTyIdx());
      gcMnode->SetTyIdx(gcMeexpr->GetTyIdx());
      lfoExprParts[gcMnode] = lfopart;
      return gcMnode;
    }
    case OP_retype: {
      OpMeExpr *opMeexpr = static_cast<OpMeExpr *>(meexpr);
      RetypeNode *retypeNode = codeMP->New<RetypeNode>(meexpr->GetPrimType());
      retypeNode->SetFromType(opMeexpr->GetOpndType());
      retypeNode->SetTyIdx(opMeexpr->GetTyIdx());
      retypeNode->SetOpnd(EmitLfoExpr(opMeexpr->GetOpnd(0), retypeNode), 0);
      lfoExprParts[retypeNode] = lfopart;
      return retypeNode;
    }
    case OP_ceil:
    case OP_cvt:
    case OP_floor:
    case OP_trunc: {
      OpMeExpr *opMeexpr = static_cast<OpMeExpr *>(meexpr);
      TypeCvtNode *tycvtNode = codeMP->New<TypeCvtNode>(meexpr->GetOp(), meexpr->GetPrimType());
      tycvtNode->SetFromType(opMeexpr->GetOpndType());
      tycvtNode->SetOpnd(EmitLfoExpr(opMeexpr->GetOpnd(0), tycvtNode), 0);
      lfoExprParts[tycvtNode] = lfopart;
      return tycvtNode;
    }
    case OP_sext:
    case OP_zext:
    case OP_extractbits: {
      OpMeExpr *opMeexpr = static_cast<OpMeExpr *>(meexpr);
      ExtractbitsNode *extNode = codeMP->New<ExtractbitsNode>(meexpr->GetOp(), meexpr->GetPrimType());
      extNode->SetOpnd(EmitLfoExpr(opMeexpr->GetOpnd(0), extNode), 0);
      extNode->SetBitsOffset(opMeexpr->GetBitsOffSet());
      extNode->SetBitsSize(opMeexpr->GetBitsSize());
      lfoExprParts[extNode] = lfopart;
      return extNode;
    }
    case OP_regread: {
      RegMeExpr *regMeexpr = static_cast<RegMeExpr *>(meexpr);
      RegreadNode *regNode = codeMP->New<RegreadNode>();
      regNode->SetPrimType(regMeexpr->GetPrimType());
      regNode->SetRegIdx(regMeexpr->GetRegIdx());
      lfoExprParts[regNode] = lfopart;
      return regNode;
    }
    case OP_sizeoftype: {
      SizeoftypeMeExpr *sizeofMeexpr = static_cast<SizeoftypeMeExpr *>(meexpr);
      SizeoftypeNode *sizeofTynode =
          codeMP->New<SizeoftypeNode>(sizeofMeexpr->GetPrimType(), sizeofMeexpr->GetTyIdx());
      lfoExprParts[sizeofTynode] = lfopart;
      return sizeofTynode;
    }
    case OP_fieldsdist: {
      FieldsDistMeExpr *fdMeexpr = static_cast<FieldsDistMeExpr *>(meexpr);
      FieldsDistNode *fieldsNode =
          codeMP->New<FieldsDistNode>(fdMeexpr->GetPrimType(), fdMeexpr->GetTyIdx(), fdMeexpr->GetFieldID1(),
                                      fdMeexpr->GetFieldID2());
      lfoExprParts[fieldsNode] = lfopart;
      return fieldsNode;
    }
    case OP_conststr: {
      ConststrMeExpr *constrMeexpr = static_cast<ConststrMeExpr *>(meexpr);
      ConststrNode *constrNode =
          codeMP->New<ConststrNode>(constrMeexpr->GetPrimType(), constrMeexpr->GetStrIdx());
      lfoExprParts[constrNode] = lfopart;
      return constrNode;
    }
    case OP_conststr16: {
      Conststr16MeExpr *constr16Meexpr = static_cast<Conststr16MeExpr *>(meexpr);
      Conststr16Node *constr16Node =
          codeMP->New<Conststr16Node>(constr16Meexpr->GetPrimType(), constr16Meexpr->GetStrIdx());
      lfoExprParts[constr16Node] = lfopart;
      return constr16Node;
    }
    case OP_abs:
    case OP_bnot:
    case OP_lnot:
    case OP_neg:
    case OP_recip:
    case OP_sqrt:
    case OP_alloca:
    case OP_malloc: {
      OpMeExpr *opMeexpr = static_cast<OpMeExpr *>(meexpr);
      UnaryNode *unNode = codeMP->New<UnaryNode>(meexpr->GetOp(), meexpr->GetPrimType());
      unNode->SetOpnd(EmitLfoExpr(opMeexpr->GetOpnd(0), unNode), 0);
      lfoExprParts[unNode] = lfopart;
      return unNode;
    }
    case OP_iaddrof: {
      OpMeExpr *opMeexpr = static_cast<OpMeExpr *>(meexpr);
      IreadNode *ireadNode = codeMP->New<IreadNode>(meexpr->GetOp(), meexpr->GetPrimType());
      ireadNode->SetOpnd(EmitLfoExpr(opMeexpr->GetOpnd(0), ireadNode), 0);
      ireadNode->SetTyIdx(opMeexpr->GetTyIdx());
      ireadNode->SetFieldID(opMeexpr->GetFieldID());
      lfoExprParts[ireadNode] = lfopart;
      return ireadNode;
    }
    case OP_select: {
      OpMeExpr *opMeexpr = static_cast<OpMeExpr *>(meexpr);
      TernaryNode *tNode = codeMP->New<TernaryNode>(OP_select, meexpr->GetPrimType());
      tNode->SetOpnd(EmitLfoExpr(opMeexpr->GetOpnd(0), tNode), 0);
      tNode->SetOpnd(EmitLfoExpr(opMeexpr->GetOpnd(1), tNode), 1);
      tNode->SetOpnd(EmitLfoExpr(opMeexpr->GetOpnd(2), tNode), 2);
      lfoExprParts[tNode] = lfopart;
      return tNode;
    }
    case OP_intrinsicop:
    case OP_intrinsicopwithtype: {
      NaryMeExpr *nMeexpr = static_cast<NaryMeExpr *>(meexpr);
      IntrinsicopNode *intrnNode =
          codeMP->New<IntrinsicopNode>(*codeMPAlloc, meexpr->GetOp(), meexpr->GetPrimType(), nMeexpr->GetTyIdx());
      intrnNode->SetIntrinsic(nMeexpr->GetIntrinsic());
      for (uint32 i = 0; i < nMeexpr->GetNumOpnds(); i++) {
        BaseNode *opnd = EmitLfoExpr(nMeexpr->GetOpnd(i), intrnNode);
        intrnNode->GetNopnd().push_back(opnd);
      }
      intrnNode->SetNumOpnds(nMeexpr->GetNumOpnds());
      lfoExprParts[intrnNode] = lfopart;
      return intrnNode;
    }
    default:
      CHECK_FATAL(false, "NYI");
  }
}

StmtNode* LfoPreEmitter::EmitLfoStmt(MeStmt *mestmt, BaseNode *parent) {
  LfoPart *lfopart = lfoMP->New<LfoPart>(parent, mestmt);
  switch (mestmt->GetOp()) {
    case OP_dassign: {
      DassignMeStmt *dsmestmt = static_cast<DassignMeStmt *>(mestmt);
      DassignNode *dass = codeMP->New<DassignNode>();
      MIRSymbol *sym = dsmestmt->GetLHS()->GetOst()->GetMIRSymbol();
      dass->SetStIdx(sym->GetStIdx());
      dass->SetFieldID(static_cast<VarMeExpr *>(dsmestmt->GetLHS())->GetOst()->GetFieldID());
      dass->SetOpnd(EmitLfoExpr(dsmestmt->GetRHS(), dass), 0);
      dass->SetSrcPos(dsmestmt->GetSrcPosition());
      lfoStmtParts[dass->GetStmtID()] = lfopart;
      return dass;
    }
    case OP_regassign: {
      AssignMeStmt *asMestmt = static_cast<AssignMeStmt *>(mestmt);
      RegassignNode *rssnode = codeMP->New<RegassignNode>();
      rssnode->SetPrimType(asMestmt->GetLHS()->GetPrimType());
      rssnode->SetRegIdx(asMestmt->GetLHS()->GetRegIdx());
      rssnode->SetOpnd(EmitLfoExpr(asMestmt->GetRHS(), rssnode), 0);
      rssnode->SetSrcPos(asMestmt->GetSrcPosition());
      lfoStmtParts[rssnode->GetStmtID()] = lfopart;
      return rssnode;
    }
    case OP_iassign: {
      IassignMeStmt *iass = static_cast<IassignMeStmt *>(mestmt);
      IvarMeExpr *lhsVar = iass->GetLHSVal();
      IassignNode *iassignNode = codeMP->New<IassignNode>();
      iassignNode->SetTyIdx(iass->GetTyIdx());
      iassignNode->SetFieldID(lhsVar->GetFieldID());
      if (lhsVar->GetOffset() == 0) {
        iassignNode->SetAddrExpr(EmitLfoExpr(lhsVar->GetBase(), iassignNode));
      } else {
        auto *mirType = GlobalTables::GetTypeTable().GetInt32();
        auto *mirConst = GlobalTables::GetIntConstTable().GetOrCreateIntConst(lhsVar->GetOffset(), *mirType);
        auto *constValNode = codeMP->New<ConstvalNode>(mirType->GetPrimType(), mirConst);
        auto *newAddrNode =
            codeMP->New<BinaryNode>(OP_add, lhsVar->GetBase()->GetPrimType(),
                                    EmitLfoExpr(lhsVar->GetBase(), iassignNode), constValNode);
        iassignNode->SetAddrExpr(newAddrNode);
      }
      if (iassignNode->Opnd(0)->GetOpCode() != OP_array) {
        ArrayNode *arryNode = ConvertToArray(iassignNode->Opnd(0), iassignNode->GetTyIdx());
        if (arryNode != nullptr) {
          iassignNode->SetAddrExpr(arryNode);
        }
      }
      iassignNode->rhs = EmitLfoExpr(iass->GetRHS(), iassignNode);
      iassignNode->SetSrcPos(iass->GetSrcPosition());
      lfoStmtParts[iassignNode->GetStmtID()] = lfopart;
      return iassignNode;
    }
    case OP_return: {
      RetMeStmt *retMestmt = static_cast<RetMeStmt *>(mestmt);
      NaryStmtNode *retNode = codeMP->New<NaryStmtNode>(*codeMPAlloc, OP_return);
      for (uint32 i = 0; i < retMestmt->GetOpnds().size(); i++) {
        retNode->GetNopnd().push_back(EmitLfoExpr(retMestmt->GetOpnd(i), retNode));
      }
      retNode->SetNumOpnds(retMestmt->GetOpnds().size());
      retNode->SetSrcPos(retMestmt->GetSrcPosition());
      lfoStmtParts[retNode->GetStmtID()] = lfopart;
      return retNode;
    }
    case OP_goto: {
      GotoMeStmt *gotoStmt = static_cast<GotoMeStmt *>(mestmt);
      if (lfoFunc->WhileLabelCreatedByLfo(gotoStmt->GetOffset())) {
        return nullptr;
      }
      if (lfoFunc->IfLabelCreatedByLfo(gotoStmt->GetOffset())) {
        return nullptr;
      }
      GotoNode *gto = codeMP->New<GotoNode>(OP_goto);
      gto->SetOffset(gotoStmt->GetOffset());
      gto->SetSrcPos(gotoStmt->GetSrcPosition());
      lfoStmtParts[gto->GetStmtID()] = lfopart;
      return gto;
    }
    case OP_igoto: {
      UnaryMeStmt *igotoMeStmt = static_cast<UnaryMeStmt *>(mestmt);
      UnaryStmtNode *igto = codeMP->New<UnaryStmtNode>(OP_igoto);
      igto->SetOpnd(EmitLfoExpr(igotoMeStmt->GetOpnd(), igto), 0);
      igto->SetSrcPos(igotoMeStmt->GetSrcPosition());
      lfoStmtParts[igto->GetStmtID()] = lfopart;
      return igto;
    }
    case OP_comment: {
      CommentMeStmt *cmtmeNode = static_cast<CommentMeStmt *>(mestmt);
      CommentNode *cmtNode = codeMP->New<CommentNode>(*codeMPAlloc);
      cmtNode->SetComment(cmtmeNode->GetComment());
      cmtNode->SetSrcPos(cmtmeNode->GetSrcPosition());
      lfoStmtParts[cmtNode->GetStmtID()] = lfopart;
      return cmtNode;
    }
    case OP_call:
    case OP_virtualcall:
    case OP_virtualicall:
    case OP_superclasscall:
    case OP_interfacecall:
    case OP_interfaceicall:
    case OP_customcall:
    case OP_callassigned:
    case OP_virtualcallassigned:
    case OP_virtualicallassigned:
    case OP_superclasscallassigned:
    case OP_interfacecallassigned:
    case OP_interfaceicallassigned:
    case OP_customcallassigned:
    case OP_polymorphiccall:
    case OP_polymorphiccallassigned: {
      CallMeStmt *callMeStmt = static_cast<CallMeStmt *>(mestmt);
      CallNode *callnode = codeMP->New<CallNode>(*codeMPAlloc, mestmt->GetOp());
      callnode->SetPUIdx(callMeStmt->GetPUIdx());
      callnode->SetTyIdx(callMeStmt->GetTyIdx());
      callnode->SetNumOpnds(callMeStmt->GetOpnds().size());
      callnode->SetSrcPos(callMeStmt->GetSrcPosition());
      mestmt->EmitCallReturnVector(callnode->GetReturnVec());
      for (uint32 i = 0; i < callMeStmt->GetOpnds().size(); i++) {
        callnode->GetNopnd().push_back(EmitLfoExpr(callMeStmt->GetOpnd(i), callnode));
      }
      lfoStmtParts[callnode->GetStmtID()] = lfopart;
      return callnode;
    }
    case OP_icall:
    case OP_icallassigned: {
      IcallMeStmt *icallMeStmt = static_cast<IcallMeStmt *> (mestmt);
      IcallNode *icallnode =
          codeMP->New<IcallNode>(*codeMPAlloc, OP_icallassigned, icallMeStmt->GetRetTyIdx());
      for (uint32 i = 0; i < icallMeStmt->GetOpnds().size(); i++) {
        icallnode->GetNopnd().push_back(EmitLfoExpr(icallMeStmt->GetOpnd(i), icallnode));
      }
      icallnode->SetNumOpnds(icallMeStmt->GetOpnds().size());
      icallnode->SetSrcPos(mestmt->GetSrcPosition());
      mestmt->EmitCallReturnVector(icallnode->GetReturnVec());
      icallnode->SetRetTyIdx(TyIdx(PTY_void));
      for (uint32 j = 0; j < icallnode->GetReturnVec().size(); j++) {
        CallReturnPair retpair = icallnode->GetReturnVec()[j];
        if (!retpair.second.IsReg()) {
          StIdx stIdx = retpair.first;
          MIRSymbolTable *symtab = mirFunc->GetSymTab();
          MIRSymbol *sym = symtab->GetSymbolFromStIdx(stIdx.Idx());
          icallnode->SetRetTyIdx(sym->GetType()->GetTypeIndex());
        } else {
          PregIdx pregidx = (PregIdx)retpair.second.GetPregIdx();
          MIRPreg *preg = mirFunc->GetPregTab()->PregFromPregIdx(pregidx);
          icallnode->SetRetTyIdx(TyIdx(preg->GetPrimType()));
        }
      }
      lfoStmtParts[icallnode->GetStmtID()] = lfopart;
      return icallnode;
    }
    case OP_intrinsiccall:
    case OP_xintrinsiccall:
    case OP_intrinsiccallassigned:
    case OP_xintrinsiccallassigned:
    case OP_intrinsiccallwithtype:
    case OP_intrinsiccallwithtypeassigned: {
      IntrinsiccallMeStmt *callMeStmt = static_cast<IntrinsiccallMeStmt *> (mestmt);
      IntrinsiccallNode *callnode =
          codeMP->New<IntrinsiccallNode>(*codeMPAlloc, mestmt->GetOp(), callMeStmt->GetIntrinsic());
      callnode->SetIntrinsic(callMeStmt->GetIntrinsic());
      callnode->SetTyIdx(callMeStmt->GetTyIdx());
      for (uint32 i = 0; i < callMeStmt->GetOpnds().size(); i++) {
        callnode->GetNopnd().push_back(EmitLfoExpr(callMeStmt->GetOpnd(i), callnode));
      }
      callnode->SetNumOpnds(callnode->GetNopndSize());
      callnode->SetSrcPos(mestmt->GetSrcPosition());
      if (kOpcodeInfo.IsCallAssigned(mestmt->GetOp())) {
        mestmt->EmitCallReturnVector(callnode->GetReturnVec());
      }
      lfoStmtParts[callnode->GetStmtID()] = lfopart;
      return callnode;
    }
    case OP_asm: {
      AsmMeStmt *asmMeStmt = static_cast<AsmMeStmt *>(mestmt);
      AsmNode *asmNode = codeMP->New<AsmNode>(codeMPAlloc);
      for (size_t i = 0; i < asmMeStmt->NumMeStmtOpnds(); ++i) {
        asmNode->GetNopnd().push_back(EmitLfoExpr(asmMeStmt->GetOpnd(i), asmNode));
      }
      asmNode->SetNumOpnds(asmNode->GetNopndSize());
      asmNode->SetSrcPos(mestmt->GetSrcPosition());
      mestmt->EmitCallReturnVector(*asmNode->GetCallReturnVector());
      asmNode->asmString = asmMeStmt->asmString;
      asmNode->inputConstraints = asmMeStmt->inputConstraints;
      asmNode->outputConstraints = asmMeStmt->outputConstraints;
      asmNode->clobberList = asmMeStmt->clobberList;
      asmNode->gotoLabels = asmMeStmt->gotoLabels;
      return asmNode;
    }
    case OP_jscatch:
    case OP_finally:
    case OP_endtry:
    case OP_cleanuptry:
    case OP_membaracquire:
    case OP_membarrelease:
    case OP_membarstorestore:
    case OP_membarstoreload: {
      StmtNode *stmtNode = codeMP->New<StmtNode>(mestmt->GetOp());
      stmtNode->SetSrcPos(mestmt->GetSrcPosition());
      lfoStmtParts[stmtNode->GetStmtID()] = lfopart;
      return stmtNode;
    }
    case OP_retsub: {
      StmtNode * usesStmtNode = codeMP->New<StmtNode>(mestmt->GetOp());
      usesStmtNode->SetSrcPos(mestmt->GetSrcPosition());
      lfoStmtParts[usesStmtNode->GetStmtID()] = lfopart;
      return usesStmtNode;
    }
    case OP_brfalse:
    case OP_brtrue: {
      CondGotoNode *CondNode = codeMP->New<CondGotoNode>(mestmt->GetOp());
      CondGotoMeStmt *condMeStmt = static_cast<CondGotoMeStmt *> (mestmt);
      CondNode->SetOffset(condMeStmt->GetOffset());
      CondNode->SetSrcPos(mestmt->GetSrcPosition());
      CondNode->SetOpnd(EmitLfoExpr(condMeStmt->GetOpnd(), CondNode), 0);
      lfoStmtParts[CondNode->GetStmtID()] = lfopart;
      return CondNode;
    }
    case OP_cpptry:
    case OP_try: {
      TryNode *jvTryNode = codeMP->New<TryNode>(*codeMPAlloc);
      TryMeStmt *tryMeStmt = static_cast<TryMeStmt *> (mestmt);
      uint32 offsetsSize = tryMeStmt->GetOffsets().size();
      jvTryNode->ResizeOffsets(offsetsSize);
      for (uint32 i = 0; i < offsetsSize; i++) {
        jvTryNode->SetOffset(tryMeStmt->GetOffsets()[i], i);
      }
      jvTryNode->SetSrcPos(tryMeStmt->GetSrcPosition());
      lfoStmtParts[jvTryNode->GetStmtID()] = lfopart;
      return jvTryNode;
    }
    case OP_cppcatch: {
      CppCatchNode *cppCatchNode = codeMP->New<CppCatchNode>();
      CppCatchMeStmt *catchMestmt = static_cast<CppCatchMeStmt *> (mestmt);
      cppCatchNode->exceptionTyIdx = catchMestmt->exceptionTyIdx;
      cppCatchNode->SetSrcPos(catchMestmt->GetSrcPosition());
      lfoStmtParts[cppCatchNode->GetStmtID()] = lfopart;
      return cppCatchNode;
    }
    case OP_catch: {
      CatchNode *jvCatchNode = codeMP->New<CatchNode>(*codeMPAlloc);
      CatchMeStmt *catchMestmt = static_cast<CatchMeStmt *> (mestmt);
      jvCatchNode->SetExceptionTyIdxVec(catchMestmt->GetExceptionTyIdxVec());
      jvCatchNode->SetSrcPos(catchMestmt->GetSrcPosition());
      lfoStmtParts[jvCatchNode->GetStmtID()] = lfopart;
      return jvCatchNode;
    }
    case OP_throw: {
      UnaryStmtNode *throwStmtNode = codeMP->New<UnaryStmtNode>(mestmt->GetOp());
      ThrowMeStmt *throwMeStmt = static_cast<ThrowMeStmt *>(mestmt);
      throwStmtNode->SetOpnd(EmitLfoExpr(throwMeStmt->GetOpnd(), throwStmtNode), 0);
      throwStmtNode->SetSrcPos(throwMeStmt->GetSrcPosition());
      lfoStmtParts[throwStmtNode->GetStmtID()] = lfopart;
      return throwStmtNode;
    }
    CASE_OP_ASSERT_NONNULL
    case OP_eval:
    case OP_free: {
      UnaryStmtNode *unaryStmtNode = codeMP->New<UnaryStmtNode>(mestmt->GetOp());
      UnaryMeStmt *uMeStmt = static_cast<UnaryMeStmt *>(mestmt);
      unaryStmtNode->SetOpnd(EmitLfoExpr(uMeStmt->GetOpnd(), unaryStmtNode), 0);
      unaryStmtNode->SetSrcPos(uMeStmt->GetSrcPosition());
      lfoStmtParts[unaryStmtNode->GetStmtID()] = lfopart;
      return unaryStmtNode;
    }
    case OP_switch: {
      SwitchNode *switchNode = codeMP->New<SwitchNode>(*codeMPAlloc);
      SwitchMeStmt *meSwitch = static_cast<SwitchMeStmt *>(mestmt);
      switchNode->SetSwitchOpnd(EmitLfoExpr(meSwitch->GetOpnd(), switchNode));
      switchNode->SetDefaultLabel(meSwitch->GetDefaultLabel());
      switchNode->SetSwitchTable(meSwitch->GetSwitchTable());
      switchNode->SetSrcPos(meSwitch->GetSrcPosition());
      lfoStmtParts[switchNode->GetStmtID()] = lfopart;
      return switchNode;
    }
    default:
      CHECK_FATAL(false, "nyi");
  }
}

void LfoPreEmitter::EmitBB(BB *bb, BlockNode *curblk) {
  CHECK_FATAL(curblk != nullptr, "null ptr check");
  // emit head. label
  LabelIdx labidx = bb->GetBBLabel();
  if (labidx != 0 && !lfoFunc->WhileLabelCreatedByLfo(labidx) && !lfoFunc->IfLabelCreatedByLfo(labidx)) {
    // not a empty bb
    LabelNode *lbnode = codeMP->New<LabelNode>();
    lbnode->SetLabelIdx(labidx);
    curblk->AddStatement(lbnode);
    LfoPart *lfopart = lfoMP->New<LfoPart>(curblk);
    lfoStmtParts[lbnode->GetStmtID()] = lfopart;
  }
  for (auto& mestmt : bb->GetMeStmts()) {
    StmtNode *stmt = EmitLfoStmt(&mestmt, curblk);
    if (!stmt) // can be null i.e, a goto to a label that was created by lno lower
      continue;
    curblk->AddStatement(stmt);
  }
  if (bb->GetAttributes(kBBAttrIsTryEnd)) {
    /* generate op_endtry */
    StmtNode *endtry = codeMP->New<StmtNode>(OP_endtry);
    curblk->AddStatement(endtry);
    LfoPart *lfopart = lfoMP->New<LfoPart>(curblk);
    lfoStmtParts[endtry->GetStmtID()] = lfopart;
  }
}

DoloopNode *LfoPreEmitter::EmitLfoDoloop(BB *mewhilebb, BlockNode *curblk, LfoWhileInfo *whileInfo) {
  MeStmt *lastmestmt = mewhilebb->GetLastMe();
  CHECK_FATAL(lastmestmt->GetPrev() == nullptr || dynamic_cast<AssignMeStmt *>(lastmestmt->GetPrev()) == nullptr,
              "EmitLfoDoLoop: there are other statements at while header bb");
  DoloopNode *Doloopnode = codeMP->New<DoloopNode>();
  LfoPart *lfopart = lfoMP->New<LfoPart>(curblk);
  lfopart->mestmt = lastmestmt;
  lfoStmtParts[Doloopnode->GetStmtID()] = lfopart;
  Doloopnode->SetDoVarStIdx(whileInfo->ivOst->GetMIRSymbol()->GetStIdx());
  CondGotoMeStmt *condGotostmt = static_cast<CondGotoMeStmt *>(lastmestmt);
  Doloopnode->SetStartExpr(EmitLfoExpr(whileInfo->initExpr, Doloopnode));
  Doloopnode->SetContExpr(EmitLfoExpr(condGotostmt->GetOpnd(), Doloopnode));
  BlockNode *dobodyNode = codeMP->New<BlockNode>();
  Doloopnode->SetDoBody(dobodyNode);
  LfoPart *dolooplfopart = lfoMP->New<LfoPart>(Doloopnode);
  lfoStmtParts[dobodyNode->GetStmtID()] = dolooplfopart;
  MIRIntConst *intConst =
      mirFunc->GetModule()->GetMemPool()->New<MIRIntConst>(whileInfo->stepValue, *whileInfo->ivOst->GetType());
  ConstvalNode *constnode = codeMP->New<ConstvalNode>(intConst->GetType().GetPrimType(), intConst);
  lfoExprParts[constnode] = dolooplfopart;
  Doloopnode->SetIncrExpr(constnode);
  Doloopnode->SetIsPreg(false);
  curblk->AddStatement(Doloopnode);
  return Doloopnode;
}

WhileStmtNode *LfoPreEmitter::EmitLfoWhile(BB *meWhilebb, BlockNode *curblk) {
  MeStmt *lastmestmt = meWhilebb->GetLastMe();
  CHECK_FATAL(lastmestmt->GetPrev() == nullptr || dynamic_cast<AssignMeStmt *>(lastmestmt->GetPrev()) == nullptr,
              "EmitLfoWhile: there are other statements at while header bb");
  WhileStmtNode *Whilestmt = codeMP->New<WhileStmtNode>(OP_while);
  LfoPart *lfopart = lfoMP->New<LfoPart>(curblk);
  lfoStmtParts[Whilestmt->GetStmtID()] = lfopart;
  CondGotoMeStmt *condGotostmt = static_cast<CondGotoMeStmt *>(lastmestmt);
  Whilestmt->SetOpnd(EmitLfoExpr(condGotostmt->GetOpnd(), Whilestmt), 0);
  BlockNode *whilebodyNode = codeMP->New<BlockNode>();
  LfoPart *whilenodelfopart = lfoMP->New<LfoPart>(Whilestmt);
  lfoStmtParts[whilebodyNode->GetStmtID()] = whilenodelfopart;
  Whilestmt->SetBody(whilebodyNode);
  curblk->AddStatement(Whilestmt);
  return Whilestmt;
}

uint32 LfoPreEmitter::Raise2LfoWhile(uint32 curj, BlockNode *curblk) {
  MapleVector<BB *> &bbvec = cfg->GetAllBBs();
  BB *curbb = bbvec[curj];
  LabelIdx whilelabidx = curbb->GetBBLabel();
  LfoWhileInfo *whileInfo = lfoFunc->label2WhileInfo[whilelabidx];

  // find the end label bb
  BB *suc0 = curbb->GetSucc(0);
  BB *suc1 = curbb->GetSucc(1);
  MeStmt *laststmt = curbb->GetLastMe();
  CHECK_FATAL(laststmt->GetOp() == OP_brfalse, "Riase2While: NYI");
  CondGotoMeStmt *condgotomestmt = static_cast<CondGotoMeStmt *>(laststmt);
  BB *endlblbb = condgotomestmt->GetOffset() == suc1->GetBBLabel() ? suc1 : suc0;
  BlockNode *Dobody = nullptr;
  if (whileInfo->canConvertDoloop) {  // emit doloop
    DoloopNode *doloopnode = EmitLfoDoloop(curbb, curblk, whileInfo);
    ++curj;
    Dobody = static_cast<BlockNode *>(doloopnode->GetDoBody());
  } else { // emit while loop
    WhileStmtNode *whileNode = EmitLfoWhile(curbb, curblk);
    ++curj;
    Dobody = static_cast<BlockNode *> (whileNode->GetBody());
  }
  // emit loop body
  while (bbvec[curj]->GetBBId() != endlblbb->GetBBId()) {
    curj = EmitLfoBB(curj, Dobody);
    while (bbvec[curj] == nullptr) {
      curj++;
    }
  }
  if (whileInfo->canConvertDoloop) {  // delete the increment statement
    StmtNode *bodylaststmt = Dobody->GetLast();
    CHECK_FATAL(bodylaststmt->GetOpCode() == OP_dassign, "Raise2LfoWhile: cannot find increment stmt");
    DassignNode *dassnode = static_cast<DassignNode *>(bodylaststmt);
    CHECK_FATAL(dassnode->GetStIdx() == whileInfo->ivOst->GetMIRSymbol()->GetStIdx(),
                "Raise2LfoWhile: cannot find IV increment");
    Dobody->RemoveStmt(dassnode);
  }
  return curj;
}

uint32 LfoPreEmitter::Raise2LfoIf(uint32 curj, BlockNode *curblk) {
  MapleVector<BB *> &bbvec = cfg->GetAllBBs();
  BB *curbb = bbvec[curj];
  // emit BB contents before the if statement
  LabelIdx labidx = curbb->GetBBLabel();
  if (labidx != 0 && !lfoFunc->IfLabelCreatedByLfo(labidx)) {
    LabelNode *lbnode = mirFunc->GetCodeMempool()->New<LabelNode>();
    lbnode->SetLabelIdx(labidx);
    curblk->AddStatement(lbnode);
    LfoPart *lfopart = lfoMP->New<LfoPart>(curblk);
    lfoStmtParts[lbnode->GetStmtID()] = lfopart;
  }
  MeStmt *mestmt = curbb->GetFirstMe();
  while (mestmt->GetOp() != OP_brfalse && mestmt->GetOp() != OP_brtrue) {
    StmtNode *stmt = EmitLfoStmt(mestmt, curblk);
    curblk->AddStatement(stmt);
    mestmt = mestmt->GetNext();
  }
  // emit the if statement
  CHECK_FATAL(mestmt != nullptr && (mestmt->GetOp() == OP_brfalse || mestmt->GetOp() == OP_brtrue),
              "Raise2LfoIf: cannot find conditional branch");
  CondGotoMeStmt *condgoto = static_cast <CondGotoMeStmt *>(mestmt);
  LfoIfInfo *ifInfo = lfoFunc->label2IfInfo[condgoto->GetOffset()];
  CHECK_FATAL(ifInfo->endLabel != 0, "Raise2LfoIf: endLabel not found");
  //IfStmtNode *lnoIfstmtNode = mirFunc->GetCodeMempool()->New<IfStmtNode>(curblk);
  IfStmtNode *IfstmtNode = mirFunc->GetCodeMempool()->New<IfStmtNode>();
  LfoPart *lfopart = lfoMP->New<LfoPart>(curblk);
  lfoStmtParts[IfstmtNode->GetStmtID()] = lfopart;
  BaseNode *condnode = EmitLfoExpr(condgoto->GetOpnd(), IfstmtNode);
  IfstmtNode->SetOpnd(condnode, 0);
  curblk->AddStatement(IfstmtNode);
  LfoPart *iflfopart = lfoMP->New<LfoPart>(IfstmtNode);
  if (ifInfo->elseLabel != 0) {  // both else and then are not empty;
    BlockNode *elseBlk = codeMP->New<BlockNode>();
    lfoStmtParts[elseBlk->GetStmtID()] = iflfopart;
    BlockNode *thenBlk = codeMP->New<BlockNode>();
    lfoStmtParts[thenBlk->GetStmtID()] = iflfopart;
    IfstmtNode->SetThenPart(thenBlk);
    IfstmtNode->SetElsePart(elseBlk);
    BB *elsemebb = cfg->GetLabelBBAt(ifInfo->elseLabel);
    BB *endmebb = cfg->GetLabelBBAt(ifInfo->endLabel);
    CHECK_FATAL(elsemebb, "Raise2LfoIf: cannot find else BB");
    CHECK_FATAL(endmebb, "Raise2LfoIf: cannot find BB at end of IF");
    // emit then branch;
    uint32 j = curj + 1;
    while (j != elsemebb->GetBBId()) {
      j = EmitLfoBB(j, thenBlk);
    }
    CHECK_FATAL(j < bbvec.size(), "");
    while (j != endmebb->GetBBId()) {
      j = EmitLfoBB(j, elseBlk);
    }
    CHECK_FATAL(j < bbvec.size(), "");
    return j;
  } else {  // there is only then or else part in this if stmt
    BlockNode *branchBlock = codeMP->New<BlockNode>();
    lfoStmtParts[branchBlock->GetStmtID()] = iflfopart;
    BlockNode *emptyBlock = codeMP->New<BlockNode>();
    lfoStmtParts[emptyBlock->GetStmtID()] = iflfopart;
    if (condgoto->GetOp() == OP_brtrue) {
      IfstmtNode->SetElsePart(branchBlock);
      IfstmtNode->SetThenPart(emptyBlock);
    } else {
      IfstmtNode->SetThenPart(branchBlock);
      IfstmtNode->SetElsePart(emptyBlock);
    }
    BB *endmebb = cfg->GetLabelBBAt(ifInfo->endLabel);
    uint32 j = curj + 1;
    while (j != endmebb->GetBBId()) {
      j = EmitLfoBB(j, branchBlock);
    }
    CHECK_FATAL(j < bbvec.size(), "");
    return j;
  }
}

uint32 LfoPreEmitter::EmitLfoBB(uint32 curj, BlockNode *curblk) {
  MapleVector<BB *> &bbvec = cfg->GetAllBBs();
  BB *mebb = bbvec[curj];
  if (!mebb || mebb == cfg->GetCommonEntryBB() || mebb == cfg->GetCommonEntryBB()) {
    return curj + 1;
  }
  if (mebb->GetBBLabel() != 0) {
    MapleMap<LabelIdx, LfoWhileInfo*>::iterator it = lfoFunc->label2WhileInfo.find(mebb->GetBBLabel());
    if (it != lfoFunc->label2WhileInfo.end()) {
      if (mebb->GetSucc().size() == 2) {
        curj = Raise2LfoWhile(curj, curblk);
        return curj;
      } else {
        lfoFunc->lfoCreatedWhileLabelSet.erase(mebb->GetBBLabel());
      }
    }
  }
  if (!mebb->GetMeStmts().empty() &&
      (mebb->GetLastMe()->GetOp() == OP_brfalse ||
       mebb->GetLastMe()->GetOp() == OP_brtrue)) {
    CondGotoMeStmt *condgoto = static_cast<CondGotoMeStmt *>(mebb->GetLastMe());
    MapleMap<LabelIdx, LfoIfInfo*>::iterator it = lfoFunc->label2IfInfo.find(condgoto->GetOffset());
    if (it != lfoFunc->label2IfInfo.end()) {
      curj = Raise2LfoIf(curj, curblk);
      return curj;
    }
  }
  EmitBB(mebb, curblk);
  return ++curj;
}

bool MELfoPreEmission::PhaseRun(MeFunction &f) {
  if (f.GetCfg()->NumBBs() == 0) {
    f.SetLfo(false);
    return false;
  }
  MeIRMap *hmap = GET_ANALYSIS(MEIRMapBuild, f);
  ASSERT(hmap != nullptr, "irmapbuild has problem");

  MIRFunction *mirfunction = f.GetMirFunc();
  if (mirfunction->GetCodeMempool() != nullptr) {
    memPoolCtrler.DeleteMemPool(mirfunction->GetCodeMempool());
  }
  mirfunction->SetMemPool(new ThreadLocalMemPool(memPoolCtrler, "IR from preemission::Emit()"));
  MemPool *lfoMP = GetPhaseMemPool();
  emitter = lfoMP->New<LfoPreEmitter>(hmap, f.GetLfoFunc(), lfoMP);
  BlockNode *curblk = mirfunction->GetCodeMempool()->New<BlockNode>();
  mirfunction->SetBody(curblk);
  emitter->InitFuncBodyLfoPart(curblk); // set lfopart for curblk
  uint32 i = 0;
  while (i < f.GetCfg()->GetAllBBs().size()) {
    i = emitter->EmitLfoBB(i, curblk);
  }

  f.SetLfo(false);

  ConstantFold cf(f.GetMIRModule());
  cf.Simplify(mirfunction->GetBody());

  if (DEBUGFUNC_NEWPM(f)) {
    LogInfo::MapleLogger() << "\n**** After lfopreemit phase ****\n";
    mirfunction->Dump(false);
  }

  return emitter;
}

void MELfoPreEmission::GetAnalysisDependence(maple::AnalysisDep &aDep) const {
  aDep.AddRequired<MEIRMapBuild>();
  aDep.SetPreservedAll();
}
}  // namespace maple
