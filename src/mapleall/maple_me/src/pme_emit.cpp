/*
 * Copyright (c) [2021] Futurewei Technologies Co., Ltd. All rights reserved.
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

#include "pme_emit.h"
#include "me_irmap.h"
#include "pme_function.h"
#include "mir_lower.h"
#include "constantfold.h"
#include "inline_summary.h"
#include "ipa_phase_manager.h"

namespace maple {
// convert x to use OP_array if possible; return nullptr if unsuccessful;
// ptrTyIdx is the high level pointer type of x
ArrayNode *PreMeEmitter::ConvertToArray(BaseNode *x, TyIdx ptrTyIdx) {
  if (x->GetOpCode() != OP_add) {
    return nullptr;
  }
  MIRType *mirType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(ptrTyIdx);
  if (mirType->GetKind() != kTypePointer) {
    return nullptr;
  }
  BaseNode *opnd0 = x->Opnd(0);
  ASSERT_NOT_NULL(GetMexpr(opnd0));
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
    if (static_cast<MIRIntConst *>(constVal)->GetValue() != static_cast<int64>(elemSize)) {
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
  // The number of operands of arryNode is set to 2
  arryNode->SetNumOpnds(2);
  preMeExprExtensionMap[arryNode] = preMeExprExtensionMap[x];
  // update opnds' parent info if it has
  if (preMeExprExtensionMap[opnd0]) {
    preMeExprExtensionMap[opnd0]->SetParent(arryNode);
  }
  if (preMeExprExtensionMap[indexOpnd]) {
    preMeExprExtensionMap[indexOpnd]->SetParent(arryNode);
  }
  return arryNode;
}

BaseNode *PreMeEmitter::EmitPreMeExpr(MeExpr *meExpr, BaseNode *parent) {
  PreMeMIRExtension *pmeExt = preMeMP->New<PreMeMIRExtension>(parent, meExpr);
  switch (meExpr->GetOp()) {
    case OP_constval: {
      MIRConst *constval = static_cast<ConstMeExpr *>(meExpr)->GetConstVal();
      ConstvalNode *lcvlNode = codeMP->New<ConstvalNode>(constval->GetType().GetPrimType(), constval);
      preMeExprExtensionMap[lcvlNode] = pmeExt;
      return lcvlNode;
    }
    case OP_dread: {
      VarMeExpr *varmeexpr = static_cast<VarMeExpr *>(meExpr);
      MIRSymbol *sym = varmeexpr->GetOst()->GetMIRSymbol();
      if (sym->IsLocal()) {
        sym->ResetIsDeleted();
      }
      AddrofNode *dreadnode = codeMP->New<AddrofNode>(OP_dread, GetRegPrimType(varmeexpr->GetPrimType()),
                                                      sym->GetStIdx(), varmeexpr->GetOst()->GetFieldID());
      preMeExprExtensionMap[dreadnode] = pmeExt;
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
      OpMeExpr *cmpexpr = static_cast<OpMeExpr *>(meExpr);
      CompareNode *cmpNode =
          codeMP->New<CompareNode>(meExpr->GetOp(), cmpexpr->GetPrimType(), cmpexpr->GetOpndType(), nullptr, nullptr);
      BaseNode *opnd0 = EmitPreMeExpr(cmpexpr->GetOpnd(0), cmpNode);
      BaseNode *opnd1 = EmitPreMeExpr(cmpexpr->GetOpnd(1), cmpNode);
      cmpNode->SetBOpnd(opnd0, 0);
      cmpNode->SetBOpnd(opnd1, 1);
      cmpNode->SetOpndType(cmpNode->GetOpndType());
      preMeExprExtensionMap[cmpNode] = pmeExt;
      return cmpNode;
    }
    case OP_array: {
      NaryMeExpr *arrExpr = static_cast<NaryMeExpr *>(meExpr);
      ArrayNode *arrNode =
        codeMP->New<ArrayNode>(*codeMPAlloc, arrExpr->GetPrimType(), arrExpr->GetTyIdx());
      arrNode->SetBoundsCheck(arrExpr->GetBoundCheck());
      for (uint32 i = 0; i < arrExpr->GetNumOpnds(); i++) {
        BaseNode *opnd = EmitPreMeExpr(arrExpr->GetOpnd(i), arrNode);
        arrNode->GetNopnd().push_back(opnd);
      }
      arrNode->SetNumOpnds(meExpr->GetNumOpnds());
      preMeExprExtensionMap[arrNode] = pmeExt;
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
      OpMeExpr *opExpr = static_cast<OpMeExpr *>(meExpr);
      BinaryNode *binNode = codeMP->New<BinaryNode>(meExpr->GetOp(), meExpr->GetPrimType());
      binNode->SetBOpnd(EmitPreMeExpr(opExpr->GetOpnd(0), binNode), 0);
      binNode->SetBOpnd(EmitPreMeExpr(opExpr->GetOpnd(1), binNode), 1);
      preMeExprExtensionMap[binNode] = pmeExt;
      return binNode;
    }
    case OP_iread: {
      IvarMeExpr *ivarExpr = static_cast<IvarMeExpr *>(meExpr);
      IreadNode *irdNode = codeMP->New<IreadNode>(meExpr->GetOp(), meExpr->GetPrimType());
      ASSERT(ivarExpr->GetOffset() == 0, "offset in iread should be 0");
      irdNode->SetOpnd(EmitPreMeExpr(ivarExpr->GetBase(), irdNode), 0);
      irdNode->SetTyIdx(ivarExpr->GetTyIdx());
      irdNode->SetFieldID(ivarExpr->GetFieldID());
      preMeExprExtensionMap[irdNode] = pmeExt;
      if (irdNode->Opnd(0)->GetOpCode() != OP_array) {
        ArrayNode *arryNode = ConvertToArray(irdNode->Opnd(0), irdNode->GetTyIdx());
        if (arryNode != nullptr) {
          irdNode->SetOpnd(arryNode, 0);
        }
      }
      return irdNode;
    }
    case OP_ireadoff: {
      IvarMeExpr *ivarExpr = static_cast<IvarMeExpr *>(meExpr);
      IreadNode *irdNode = codeMP->New<IreadNode>(OP_iread, meExpr->GetPrimType());
      MeExpr *baseexpr = ivarExpr->GetBase();
      if (ivarExpr->GetOffset() == 0) {
        irdNode->SetOpnd(EmitPreMeExpr(baseexpr, irdNode), 0);
      } else {
        MIRType *mirType = GlobalTables::GetTypeTable().GetInt32();
        MIRIntConst *mirConst = GlobalTables::GetIntConstTable().GetOrCreateIntConst(
            static_cast<uint64>(static_cast<uint32>(ivarExpr->GetOffset())), *mirType);
        ConstvalNode *constValNode = codeMP->New<ConstvalNode>(mirType->GetPrimType(), mirConst);
        PreMeMIRExtension *pmeExt2 = preMeMP->New<PreMeMIRExtension>(irdNode, baseexpr);
        preMeExprExtensionMap[constValNode] = pmeExt2;
        BinaryNode *newAddrNode =
            codeMP->New<BinaryNode>(OP_add, baseexpr->GetPrimType(), EmitPreMeExpr(baseexpr, irdNode), constValNode);
        preMeExprExtensionMap[newAddrNode] = pmeExt2;
        irdNode->SetOpnd(newAddrNode, 0);
      }
      irdNode->SetTyIdx(ivarExpr->GetTyIdx());
      irdNode->SetFieldID(ivarExpr->GetFieldID());
      preMeExprExtensionMap[irdNode] = pmeExt;
      return irdNode;
    }
    case OP_addrof: {
      AddrofMeExpr *addrMeexpr = static_cast<AddrofMeExpr *>(meExpr);
      OriginalSt *ost = addrMeexpr->GetOst();
      MIRSymbol *sym = ost->GetMIRSymbol();
      AddrofNode *addrofNode =
          codeMP->New<AddrofNode>(OP_addrof, addrMeexpr->GetPrimType(), sym->GetStIdx(), ost->GetFieldID());
      preMeExprExtensionMap[addrofNode] = pmeExt;
      return addrofNode;
    }
    case OP_addroflabel: {
      AddroflabelMeExpr *addroflabelexpr = static_cast<AddroflabelMeExpr *>(meExpr);
      AddroflabelNode *addroflabel = codeMP->New<AddroflabelNode>(addroflabelexpr->labelIdx);
      addroflabel->SetPrimType(meExpr->GetPrimType());
      preMeExprExtensionMap[addroflabel] = pmeExt;
      return addroflabel;
    }
    case OP_addroffunc: {
      AddroffuncMeExpr *addrMeexpr = static_cast<AddroffuncMeExpr *>(meExpr);
      AddroffuncNode *addrfunNode = codeMP->New<AddroffuncNode>(addrMeexpr->GetPrimType(), addrMeexpr->GetPuIdx());
      preMeExprExtensionMap[addrfunNode] = pmeExt;
      return addrfunNode;
    }
    case OP_gcmalloc:
    case OP_gcpermalloc:
    case OP_stackmalloc: {
      GcmallocMeExpr *gcMeexpr = static_cast<GcmallocMeExpr *>(meExpr);
      GCMallocNode *gcMnode =
          codeMP->New<GCMallocNode>(meExpr->GetOp(), meExpr->GetPrimType(), gcMeexpr->GetTyIdx());
      gcMnode->SetTyIdx(gcMeexpr->GetTyIdx());
      preMeExprExtensionMap[gcMnode] = pmeExt;
      return gcMnode;
    }
    case OP_retype: {
      OpMeExpr *opMeexpr = static_cast<OpMeExpr *>(meExpr);
      RetypeNode *retypeNode = codeMP->New<RetypeNode>(meExpr->GetPrimType());
      retypeNode->SetFromType(opMeexpr->GetOpndType());
      retypeNode->SetTyIdx(opMeexpr->GetTyIdx());
      retypeNode->SetOpnd(EmitPreMeExpr(opMeexpr->GetOpnd(0), retypeNode), 0);
      preMeExprExtensionMap[retypeNode] = pmeExt;
      return retypeNode;
    }
    case OP_ceil:
    case OP_cvt:
    case OP_floor:
    case OP_trunc: {
      OpMeExpr *opMeexpr = static_cast<OpMeExpr *>(meExpr);
      TypeCvtNode *tycvtNode = codeMP->New<TypeCvtNode>(meExpr->GetOp(), meExpr->GetPrimType());
      tycvtNode->SetFromType(opMeexpr->GetOpndType());
      tycvtNode->SetOpnd(EmitPreMeExpr(opMeexpr->GetOpnd(0), tycvtNode), 0);
      preMeExprExtensionMap[tycvtNode] = pmeExt;
      return tycvtNode;
    }
    case OP_sext:
    case OP_zext:
    case OP_extractbits: {
      OpMeExpr *opMeexpr = static_cast<OpMeExpr *>(meExpr);
      ExtractbitsNode *extNode = codeMP->New<ExtractbitsNode>(meExpr->GetOp(), meExpr->GetPrimType());
      extNode->SetOpnd(EmitPreMeExpr(opMeexpr->GetOpnd(0), extNode), 0);
      extNode->SetBitsOffset(opMeexpr->GetBitsOffSet());
      extNode->SetBitsSize(opMeexpr->GetBitsSize());
      preMeExprExtensionMap[extNode] = pmeExt;
      return extNode;
    }
    case OP_depositbits: {
      OpMeExpr *opMeexpr = static_cast<OpMeExpr *>(meExpr);
      DepositbitsNode *depNode = codeMP->New<DepositbitsNode>(meExpr->GetOp(), meExpr->GetPrimType());
      depNode->SetOpnd(EmitPreMeExpr(opMeexpr->GetOpnd(0), depNode), 0);
      depNode->SetOpnd(EmitPreMeExpr(opMeexpr->GetOpnd(1), depNode), 1);
      depNode->SetBitsOffset(opMeexpr->GetBitsOffSet());
      depNode->SetBitsSize(opMeexpr->GetBitsSize());
      preMeExprExtensionMap[depNode] = pmeExt;
      return depNode;
    }
    case OP_regread: {
      RegMeExpr *regMeexpr = static_cast<RegMeExpr *>(meExpr);
      RegreadNode *regNode = codeMP->New<RegreadNode>();
      regNode->SetPrimType(regMeexpr->GetPrimType());
      regNode->SetRegIdx(regMeexpr->GetRegIdx());
      preMeExprExtensionMap[regNode] = pmeExt;
      return regNode;
    }
    case OP_sizeoftype: {
      SizeoftypeMeExpr *sizeofMeexpr = static_cast<SizeoftypeMeExpr *>(meExpr);
      SizeoftypeNode *sizeofTynode =
          codeMP->New<SizeoftypeNode>(sizeofMeexpr->GetPrimType(), sizeofMeexpr->GetTyIdx());
      preMeExprExtensionMap[sizeofTynode] = pmeExt;
      return sizeofTynode;
    }
    case OP_fieldsdist: {
      FieldsDistMeExpr *fdMeexpr = static_cast<FieldsDistMeExpr *>(meExpr);
      FieldsDistNode *fieldsNode =
          codeMP->New<FieldsDistNode>(fdMeexpr->GetPrimType(), fdMeexpr->GetTyIdx(), fdMeexpr->GetFieldID1(),
                                      fdMeexpr->GetFieldID2());
      preMeExprExtensionMap[fieldsNode] = pmeExt;
      return fieldsNode;
    }
    case OP_conststr: {
      ConststrMeExpr *constrMeexpr = static_cast<ConststrMeExpr *>(meExpr);
      ConststrNode *constrNode =
          codeMP->New<ConststrNode>(constrMeexpr->GetPrimType(), constrMeexpr->GetStrIdx());
      preMeExprExtensionMap[constrNode] = pmeExt;
      return constrNode;
    }
    case OP_conststr16: {
      Conststr16MeExpr *constr16Meexpr = static_cast<Conststr16MeExpr *>(meExpr);
      Conststr16Node *constr16Node =
          codeMP->New<Conststr16Node>(constr16Meexpr->GetPrimType(), constr16Meexpr->GetStrIdx());
      preMeExprExtensionMap[constr16Node] = pmeExt;
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
      OpMeExpr *opMeexpr = static_cast<OpMeExpr *>(meExpr);
      UnaryNode *unNode = codeMP->New<UnaryNode>(meExpr->GetOp(), meExpr->GetPrimType());
      unNode->SetOpnd(EmitPreMeExpr(opMeexpr->GetOpnd(0), unNode), 0);
      preMeExprExtensionMap[unNode] = pmeExt;
      return unNode;
    }
    case OP_iaddrof: {
      OpMeExpr *opMeexpr = static_cast<OpMeExpr *>(meExpr);
      IreadNode *ireadNode = codeMP->New<IreadNode>(meExpr->GetOp(), meExpr->GetPrimType());
      ireadNode->SetOpnd(EmitPreMeExpr(opMeexpr->GetOpnd(0), ireadNode), 0);
      ireadNode->SetTyIdx(opMeexpr->GetTyIdx());
      ireadNode->SetFieldID(opMeexpr->GetFieldID());
      preMeExprExtensionMap[ireadNode] = pmeExt;
      return ireadNode;
    }
    case OP_select: {
      OpMeExpr *opMeexpr = static_cast<OpMeExpr *>(meExpr);
      TernaryNode *tNode = codeMP->New<TernaryNode>(OP_select, meExpr->GetPrimType());
      tNode->SetOpnd(EmitPreMeExpr(opMeexpr->GetOpnd(0), tNode), 0);
      tNode->SetOpnd(EmitPreMeExpr(opMeexpr->GetOpnd(1), tNode), 1);
      tNode->SetOpnd(EmitPreMeExpr(opMeexpr->GetOpnd(2), tNode), 2);
      preMeExprExtensionMap[tNode] = pmeExt;
      return tNode;
    }
    case OP_intrinsicop:
    case OP_intrinsicopwithtype: {
      NaryMeExpr *nMeexpr = static_cast<NaryMeExpr *>(meExpr);
      IntrinsicopNode *intrnNode =
          codeMP->New<IntrinsicopNode>(*codeMPAlloc, meExpr->GetOp(), meExpr->GetPrimType(), nMeexpr->GetTyIdx());
      intrnNode->SetIntrinsic(nMeexpr->GetIntrinsic());
      for (uint32 i = 0; i < nMeexpr->GetNumOpnds(); i++) {
        BaseNode *opnd = EmitPreMeExpr(nMeexpr->GetOpnd(i), intrnNode);
        intrnNode->GetNopnd().push_back(opnd);
      }
      intrnNode->SetNumOpnds(nMeexpr->GetNumOpnds());
      preMeExprExtensionMap[intrnNode] = pmeExt;
      return intrnNode;
    }
    default:
      CHECK_FATAL(false, "NYI");
  }
}

StmtNode* PreMeEmitter::EmitPreMeStmt(MeStmt *meStmt, BaseNode *parent) {
  PreMeMIRExtension *pmeExt = preMeMP->New<PreMeMIRExtension>(parent, meStmt);
  switch (meStmt->GetOp()) {
    case OP_dassign: {
      DassignMeStmt *dsmestmt = static_cast<DassignMeStmt *>(meStmt);
      if (dsmestmt->GetRHS()->GetMeOp() == kMeOpVar && 
          static_cast<VarMeExpr*>(dsmestmt->GetRHS())->GetOst() == dsmestmt->GetLHS()->GetOst()) {
        return nullptr;  // identity assignment introduced by LFO
      }
      DassignNode *dass = codeMP->New<DassignNode>();
      MIRSymbol *sym = dsmestmt->GetLHS()->GetOst()->GetMIRSymbol();
      dass->SetStIdx(sym->GetStIdx());
      dass->SetFieldID(static_cast<VarMeExpr *>(dsmestmt->GetLHS())->GetOst()->GetFieldID());
      dass->SetOpnd(EmitPreMeExpr(dsmestmt->GetRHS(), dass), 0);
      dass->SetSrcPos(dsmestmt->GetSrcPosition());
      dass->CopySafeRegionAttr(meStmt->GetStmtAttr());
      dass->SetOriginalID(meStmt->GetOriginalId());
      preMeStmtExtensionMap[dass->GetStmtID()] = pmeExt;
      return dass;
    }
    case OP_regassign: {
      AssignMeStmt *asMestmt = static_cast<AssignMeStmt *>(meStmt);
      RegassignNode *rssnode = codeMP->New<RegassignNode>();
      rssnode->SetPrimType(asMestmt->GetLHS()->GetPrimType());
      rssnode->SetRegIdx(asMestmt->GetLHS()->GetRegIdx());
      rssnode->SetOpnd(EmitPreMeExpr(asMestmt->GetRHS(), rssnode), 0);
      rssnode->SetSrcPos(asMestmt->GetSrcPosition());
      rssnode->CopySafeRegionAttr(meStmt->GetStmtAttr());
      rssnode->SetOriginalID(meStmt->GetOriginalId());
      preMeStmtExtensionMap[rssnode->GetStmtID()] = pmeExt;
      return rssnode;
    }
    case OP_iassign: {
      IassignMeStmt *iass = static_cast<IassignMeStmt *>(meStmt);
      IvarMeExpr *lhsVar = iass->GetLHSVal();
      IassignNode *iassignNode = codeMP->New<IassignNode>();
      iassignNode->SetTyIdx(iass->GetTyIdx());
      iassignNode->SetFieldID(lhsVar->GetFieldID());
      if (lhsVar->GetOffset() == 0) {
        iassignNode->SetAddrExpr(EmitPreMeExpr(lhsVar->GetBase(), iassignNode));
      } else {
        auto *mirType = GlobalTables::GetTypeTable().GetInt32();
        auto *mirConst = GlobalTables::GetIntConstTable().GetOrCreateIntConst(
            static_cast<uint64>(static_cast<uint32>(lhsVar->GetOffset())), *mirType);
        auto *constValNode = codeMP->New<ConstvalNode>(mirType->GetPrimType(), mirConst);
        auto *newAddrNode =
            codeMP->New<BinaryNode>(OP_add, lhsVar->GetBase()->GetPrimType(),
                                    EmitPreMeExpr(lhsVar->GetBase(), iassignNode), constValNode);
        iassignNode->SetAddrExpr(newAddrNode);
      }
      if (iassignNode->Opnd(0)->GetOpCode() != OP_array) {
        ArrayNode *arryNode = ConvertToArray(iassignNode->Opnd(0), iassignNode->GetTyIdx());
        if (arryNode != nullptr) {
          iassignNode->SetAddrExpr(arryNode);
        }
      }
      iassignNode->rhs = EmitPreMeExpr(iass->GetRHS(), iassignNode);
      iassignNode->SetSrcPos(iass->GetSrcPosition());
      iassignNode->CopySafeRegionAttr(meStmt->GetStmtAttr());
      iassignNode->SetOriginalID(meStmt->GetOriginalId());
      preMeStmtExtensionMap[iassignNode->GetStmtID()] = pmeExt;
      iassignNode->SetExpandFromArrayOfCharFunc(iass->IsExpandedFromArrayOfCharFunc());
      return iassignNode;
    }
    case OP_return: {
      RetMeStmt *retMestmt = static_cast<RetMeStmt *>(meStmt);
      NaryStmtNode *retNode = codeMP->New<NaryStmtNode>(*codeMPAlloc, OP_return);
      for (uint32 i = 0; i < retMestmt->GetOpnds().size(); i++) {
        retNode->GetNopnd().push_back(EmitPreMeExpr(retMestmt->GetOpnd(i), retNode));
      }
      retNode->SetNumOpnds(static_cast<uint8>(retMestmt->GetOpnds().size()));
      retNode->SetSrcPos(retMestmt->GetSrcPosition());
      retNode->CopySafeRegionAttr(meStmt->GetStmtAttr());
      retNode->SetOriginalID(meStmt->GetOriginalId());
      preMeStmtExtensionMap[retNode->GetStmtID()] = pmeExt;
      return retNode;
    }
    case OP_goto: {
      GotoMeStmt *gotoStmt = static_cast<GotoMeStmt *>(meStmt);
      if (preMeFunc->WhileLabelCreatedByPreMe(gotoStmt->GetOffset())) {
        return nullptr;
      }
      if (preMeFunc->IfLabelCreatedByPreMe(gotoStmt->GetOffset())) {
        return nullptr;
      }
      GotoNode *gto = codeMP->New<GotoNode>(OP_goto);
      gto->SetOffset(gotoStmt->GetOffset());
      gto->SetSrcPos(gotoStmt->GetSrcPosition());
      gto->CopySafeRegionAttr(meStmt->GetStmtAttr());
      gto->SetOriginalID(meStmt->GetOriginalId());
      preMeStmtExtensionMap[gto->GetStmtID()] = pmeExt;
      return gto;
    }
    case OP_igoto: {
      UnaryMeStmt *igotoMeStmt = static_cast<UnaryMeStmt *>(meStmt);
      UnaryStmtNode *igto = codeMP->New<UnaryStmtNode>(OP_igoto);
      igto->SetOpnd(EmitPreMeExpr(igotoMeStmt->GetOpnd(), igto), 0);
      igto->SetSrcPos(igotoMeStmt->GetSrcPosition());
      igto->CopySafeRegionAttr(meStmt->GetStmtAttr());
      igto->SetOriginalID(meStmt->GetOriginalId());
      preMeStmtExtensionMap[igto->GetStmtID()] = pmeExt;
      return igto;
    }
    case OP_comment: {
      CommentMeStmt *cmtmeNode = static_cast<CommentMeStmt *>(meStmt);
      CommentNode *cmtNode = codeMP->New<CommentNode>(*codeMPAlloc);
      cmtNode->SetComment(cmtmeNode->GetComment());
      cmtNode->SetSrcPos(cmtmeNode->GetSrcPosition());
      cmtNode->CopySafeRegionAttr(meStmt->GetStmtAttr());
      cmtNode->SetOriginalID(meStmt->GetOriginalId());
      preMeStmtExtensionMap[cmtNode->GetStmtID()] = pmeExt;
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
      CallMeStmt *callMeStmt = static_cast<CallMeStmt *>(meStmt);
      CallNode *callnode = codeMP->New<CallNode>(*codeMPAlloc, meStmt->GetOp());
      callnode->SetPUIdx(callMeStmt->GetPUIdx());
      callnode->SetTyIdx(callMeStmt->GetTyIdx());
      callnode->SetNumOpnds(static_cast<uint8>(callMeStmt->GetOpnds().size()));
      callnode->SetSrcPos(callMeStmt->GetSrcPosition());
      meStmt->EmitCallReturnVector(callnode->GetReturnVec());
      for (uint32 i = 0; i < callMeStmt->GetOpnds().size(); i++) {
        callnode->GetNopnd().push_back(EmitPreMeExpr(callMeStmt->GetOpnd(i), callnode));
      }
      callnode->CopySafeRegionAttr(meStmt->GetStmtAttr());
      callnode->SetOriginalID(meStmt->GetOriginalId());
      callnode->SetMeStmtID(callMeStmt->GetMeStmtId());
      preMeStmtExtensionMap[callnode->GetStmtID()] = pmeExt;
      callnode->SetEnclosingBlock(static_cast<BlockNode *>(parent));
      return callnode;
    }
    case OP_icall:
    case OP_icallassigned:
    case OP_icallproto:
    case OP_icallprotoassigned: {
      IcallMeStmt *icallMeStmt = static_cast<IcallMeStmt *>(meStmt);
      IcallNode *icallnode =
          codeMP->New<IcallNode>(*codeMPAlloc, OP_icallprotoassigned, icallMeStmt->GetRetTyIdx());
      for (uint32 i = 0; i < icallMeStmt->GetOpnds().size(); i++) {
        icallnode->GetNopnd().push_back(EmitPreMeExpr(icallMeStmt->GetOpnd(i), icallnode));
      }
      icallnode->SetNumOpnds(static_cast<uint8>(icallMeStmt->GetOpnds().size()));
      icallnode->SetSrcPos(meStmt->GetSrcPosition());
      meStmt->EmitCallReturnVector(icallnode->GetReturnVec());
      icallnode->SetRetTyIdx(TyIdx(PTY_void));
      for (uint32 j = 0; j < icallnode->GetReturnVec().size(); j++) {
        CallReturnPair retpair = icallnode->GetReturnVec()[j];
        if (!retpair.second.IsReg()) {
          StIdx stIdx = retpair.first;
          MIRSymbolTable *symtab = mirFunc->GetSymTab();
          MIRSymbol *sym = symtab->GetSymbolFromStIdx(stIdx.Idx());
          ASSERT_NOT_NULL(sym);
          icallnode->SetRetTyIdx(sym->GetType()->GetTypeIndex());
        } else {
          PregIdx pregidx(retpair.second.GetPregIdx());
          MIRPreg *preg = mirFunc->GetPregTab()->PregFromPregIdx(pregidx);
          icallnode->SetRetTyIdx(TyIdx(preg->GetPrimType()));
        }
      }
      if (meStmt->GetOp() == OP_icallproto || meStmt->GetOp() == OP_icallprotoassigned) {
        icallnode->SetRetTyIdx(icallMeStmt->GetRetTyIdx());
      }
      icallnode->CopySafeRegionAttr(meStmt->GetStmtAttr());
      icallnode->SetOriginalID(meStmt->GetOriginalId());
      preMeStmtExtensionMap[icallnode->GetStmtID()] = pmeExt;
      return icallnode;
    }
    case OP_intrinsiccall:
    case OP_xintrinsiccall:
    case OP_intrinsiccallassigned:
    case OP_xintrinsiccallassigned:
    case OP_intrinsiccallwithtype:
    case OP_intrinsiccallwithtypeassigned: {
      IntrinsiccallMeStmt *callMeStmt = static_cast<IntrinsiccallMeStmt *>(meStmt);
      IntrinsiccallNode *callnode =
          codeMP->New<IntrinsiccallNode>(*codeMPAlloc, meStmt->GetOp(), callMeStmt->GetIntrinsic());
      callnode->SetIntrinsic(callMeStmt->GetIntrinsic());
      callnode->SetTyIdx(callMeStmt->GetTyIdx());
      for (uint32 i = 0; i < callMeStmt->GetOpnds().size(); i++) {
        callnode->GetNopnd().push_back(EmitPreMeExpr(callMeStmt->GetOpnd(i), callnode));
      }
      callnode->SetNumOpnds(static_cast<uint8>(callnode->GetNopndSize()));
      callnode->SetSrcPos(meStmt->GetSrcPosition());
      if (kOpcodeInfo.IsCallAssigned(meStmt->GetOp())) {
        meStmt->EmitCallReturnVector(callnode->GetReturnVec());
      }
      callnode->CopySafeRegionAttr(meStmt->GetStmtAttr());
      callnode->SetOriginalID(meStmt->GetOriginalId());
      preMeStmtExtensionMap[callnode->GetStmtID()] = pmeExt;
      return callnode;
    }
    case OP_asm: {
      AsmMeStmt *asmMeStmt = static_cast<AsmMeStmt *>(meStmt);
      AsmNode *asmNode = codeMP->New<AsmNode>(codeMPAlloc);
      for (size_t i = 0; i < asmMeStmt->NumMeStmtOpnds(); ++i) {
        asmNode->GetNopnd().push_back(EmitPreMeExpr(asmMeStmt->GetOpnd(i), asmNode));
      }
      asmNode->SetNumOpnds(static_cast<uint8>(asmNode->GetNopndSize()));
      asmNode->SetSrcPos(meStmt->GetSrcPosition());
      meStmt->EmitCallReturnVector(*asmNode->GetCallReturnVector());
      asmNode->asmString = asmMeStmt->asmString;
      asmNode->inputConstraints = asmMeStmt->inputConstraints;
      asmNode->outputConstraints = asmMeStmt->outputConstraints;
      asmNode->clobberList = asmMeStmt->clobberList;
      asmNode->gotoLabels = asmMeStmt->gotoLabels;
      asmNode->SetOriginalID(meStmt->GetOriginalId());
      asmNode->CopySafeRegionAttr(meStmt->GetStmtAttr());
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
      StmtNode *stmtNode = codeMP->New<StmtNode>(meStmt->GetOp());
      stmtNode->SetSrcPos(meStmt->GetSrcPosition());
      stmtNode->CopySafeRegionAttr(meStmt->GetStmtAttr());
      stmtNode->SetOriginalID(meStmt->GetOriginalId());
      preMeStmtExtensionMap[stmtNode->GetStmtID()] = pmeExt;
      return stmtNode;
    }
    case OP_retsub: {
      StmtNode *usesStmtNode = codeMP->New<StmtNode>(meStmt->GetOp());
      usesStmtNode->SetSrcPos(meStmt->GetSrcPosition());
      usesStmtNode->CopySafeRegionAttr(meStmt->GetStmtAttr());
      usesStmtNode->SetOriginalID(meStmt->GetOriginalId());
      preMeStmtExtensionMap[usesStmtNode->GetStmtID()] = pmeExt;
      return usesStmtNode;
    }
    case OP_brfalse:
    case OP_brtrue: {
      CondGotoNode *CondNode = codeMP->New<CondGotoNode>(meStmt->GetOp());
      CondGotoMeStmt *condMeStmt = static_cast<CondGotoMeStmt *>(meStmt);
      CondNode->SetBranchProb(condMeStmt->GetBranchProb());
      CondNode->SetOffset(condMeStmt->GetOffset());
      CondNode->SetSrcPos(meStmt->GetSrcPosition());
      CondNode->SetOpnd(EmitPreMeExpr(condMeStmt->GetOpnd(), CondNode), 0);
      CondNode->CopySafeRegionAttr(meStmt->GetStmtAttr());
      CondNode->SetOriginalID(meStmt->GetOriginalId());
      CondNode->SetMeStmtID(meStmt->GetMeStmtId());
      preMeStmtExtensionMap[CondNode->GetStmtID()] = pmeExt;
      return CondNode;
    }
    case OP_cpptry:
    case OP_try: {
      TryNode *jvTryNode = codeMP->New<TryNode>(*codeMPAlloc);
      TryMeStmt *tryMeStmt = static_cast<TryMeStmt *>(meStmt);
      size_t offsetsSize = tryMeStmt->GetOffsets().size();
      jvTryNode->ResizeOffsets(offsetsSize);
      for (size_t i = 0; i < offsetsSize; i++) {
        jvTryNode->SetOffset(tryMeStmt->GetOffsets()[i], i);
      }
      jvTryNode->SetSrcPos(tryMeStmt->GetSrcPosition());
      jvTryNode->CopySafeRegionAttr(meStmt->GetStmtAttr());
      jvTryNode->SetOriginalID(meStmt->GetOriginalId());
      preMeStmtExtensionMap[jvTryNode->GetStmtID()] = pmeExt;
      return jvTryNode;
    }
    case OP_cppcatch: {
      CppCatchNode *cppCatchNode = codeMP->New<CppCatchNode>();
      CppCatchMeStmt *catchMestmt = static_cast<CppCatchMeStmt *>(meStmt);
      cppCatchNode->exceptionTyIdx = catchMestmt->exceptionTyIdx;
      cppCatchNode->SetSrcPos(catchMestmt->GetSrcPosition());
      cppCatchNode->CopySafeRegionAttr(meStmt->GetStmtAttr());
      cppCatchNode->SetOriginalID(meStmt->GetOriginalId());
      preMeStmtExtensionMap[cppCatchNode->GetStmtID()] = pmeExt;
      return cppCatchNode;
    }
    case OP_catch: {
      CatchNode *jvCatchNode = codeMP->New<CatchNode>(*codeMPAlloc);
      CatchMeStmt *catchMestmt = static_cast<CatchMeStmt *>(meStmt);
      jvCatchNode->SetExceptionTyIdxVec(catchMestmt->GetExceptionTyIdxVec());
      jvCatchNode->SetSrcPos(catchMestmt->GetSrcPosition());
      jvCatchNode->CopySafeRegionAttr(meStmt->GetStmtAttr());
      jvCatchNode->SetOriginalID(meStmt->GetOriginalId());
      preMeStmtExtensionMap[jvCatchNode->GetStmtID()] = pmeExt;
      return jvCatchNode;
    }
    case OP_throw: {
      UnaryStmtNode *throwStmtNode = codeMP->New<UnaryStmtNode>(meStmt->GetOp());
      ThrowMeStmt *throwMeStmt = static_cast<ThrowMeStmt *>(meStmt);
      throwStmtNode->SetOpnd(EmitPreMeExpr(throwMeStmt->GetOpnd(), throwStmtNode), 0);
      throwStmtNode->SetSrcPos(throwMeStmt->GetSrcPosition());
      throwStmtNode->CopySafeRegionAttr(meStmt->GetStmtAttr());
      throwStmtNode->SetOriginalID(meStmt->GetOriginalId());
      preMeStmtExtensionMap[throwStmtNode->GetStmtID()] = pmeExt;
      return throwStmtNode;
    }
    case OP_callassertnonnull: {
      CallAssertNonnullMeStmt *assertNullStmt = static_cast<CallAssertNonnullMeStmt *>(meStmt);
      CallAssertNonnullStmtNode *assertNullNode = codeMP->New<CallAssertNonnullStmtNode>(meStmt->GetOp(),
          assertNullStmt->GetFuncNameIdx(), assertNullStmt->GetParamIndex(), assertNullStmt->GetStmtFuncNameIdx());
      assertNullNode->SetSrcPos(meStmt->GetSrcPosition());
      assertNullNode->SetOpnd(EmitPreMeExpr(assertNullStmt->GetOpnd(), assertNullNode), 0);
      assertNullNode->SetNumOpnds(1);
      assertNullNode->CopySafeRegionAttr(meStmt->GetStmtAttr());
      assertNullNode->SetOriginalID(meStmt->GetOriginalId());
      preMeStmtExtensionMap[assertNullNode->GetStmtID()] = pmeExt;
      return assertNullNode;
    }
    case OP_callassertle: {
      CallAssertBoundaryMeStmt *assertBoundaryStmt = static_cast<CallAssertBoundaryMeStmt *>(meStmt);
      CallAssertBoundaryStmtNode *assertBoundaryNode = codeMP->New<CallAssertBoundaryStmtNode>(
          *codeMPAlloc, meStmt->GetOp(), assertBoundaryStmt->GetFuncNameIdx(), assertBoundaryStmt->GetParamIndex(),
          assertBoundaryStmt->GetStmtFuncNameIdx());
      assertBoundaryNode->SetSrcPos(meStmt->GetSrcPosition());
      for (uint32 i = 0; i < assertBoundaryStmt->GetOpnds().size(); i++) {
        assertBoundaryNode->GetNopnd().push_back(EmitPreMeExpr(assertBoundaryStmt->GetOpnd(i), assertBoundaryNode));
      }
      assertBoundaryNode->SetNumOpnds(static_cast<uint8>(assertBoundaryNode->GetNopndSize()));
      assertBoundaryNode->CopySafeRegionAttr(meStmt->GetStmtAttr());
      assertBoundaryNode->SetOriginalID(meStmt->GetOriginalId());
      preMeStmtExtensionMap[assertBoundaryNode->GetStmtID()] = pmeExt;
      return assertBoundaryNode;
    }
    case OP_eval:
    case OP_free: {
      UnaryStmtNode *unaryStmtNode = codeMP->New<UnaryStmtNode>(meStmt->GetOp());
      UnaryMeStmt *uMeStmt = static_cast<UnaryMeStmt *>(meStmt);
      unaryStmtNode->SetOpnd(EmitPreMeExpr(uMeStmt->GetOpnd(), unaryStmtNode), 0);
      unaryStmtNode->SetSrcPos(uMeStmt->GetSrcPosition());
      unaryStmtNode->CopySafeRegionAttr(meStmt->GetStmtAttr());
      unaryStmtNode->SetOriginalID(meStmt->GetOriginalId());
      preMeStmtExtensionMap[unaryStmtNode->GetStmtID()] = pmeExt;
      return unaryStmtNode;
    }
    case OP_switch: {
      SwitchNode *switchNode = codeMP->New<SwitchNode>(*codeMPAlloc);
      SwitchMeStmt *meSwitch = static_cast<SwitchMeStmt *>(meStmt);
      switchNode->SetSwitchOpnd(EmitPreMeExpr(meSwitch->GetOpnd(), switchNode));
      switchNode->SetDefaultLabel(meSwitch->GetDefaultLabel());
      switchNode->SetSwitchTable(meSwitch->GetSwitchTable());
      switchNode->SetSrcPos(meSwitch->GetSrcPosition());
      switchNode->CopySafeRegionAttr(meStmt->GetStmtAttr());
      switchNode->SetOriginalID(meStmt->GetOriginalId());
      switchNode->SetMeStmtID(meStmt->GetMeStmtId());
      preMeStmtExtensionMap[switchNode->GetStmtID()] = pmeExt;
      return switchNode;
    }
    case OP_assertnonnull:
    case OP_assignassertnonnull:
    case OP_returnassertnonnull: {
      AssertNonnullMeStmt *assertNullStmt = static_cast<AssertNonnullMeStmt *>(meStmt);
      AssertNonnullStmtNode *assertNullNode = codeMP->New<AssertNonnullStmtNode>(
          meStmt->GetOp(), assertNullStmt->GetFuncNameIdx());
      assertNullNode->SetSrcPos(meStmt->GetSrcPosition());
      assertNullNode->SetOpnd(EmitPreMeExpr(assertNullStmt->GetOpnd(), assertNullNode), 0);
      assertNullNode->SetNumOpnds(1);
      assertNullNode->CopySafeRegionAttr(meStmt->GetStmtAttr());
      assertNullNode->SetOriginalID(meStmt->GetOriginalId());
      preMeStmtExtensionMap[assertNullNode->GetStmtID()] = pmeExt;
      return assertNullNode;
    }
    case OP_calcassertge:
    case OP_calcassertlt:
    case OP_assertge:
    case OP_assertlt:
    case OP_assignassertle:
    case OP_returnassertle: {
      AssertBoundaryMeStmt *assertBoundaryStmt = static_cast<AssertBoundaryMeStmt *>(meStmt);
      AssertBoundaryStmtNode *assertBoundaryNode = codeMP->New<AssertBoundaryStmtNode>(
          *codeMPAlloc, meStmt->GetOp(), assertBoundaryStmt->GetFuncNameIdx());
      assertBoundaryNode->SetSrcPos(meStmt->GetSrcPosition());
      for (uint32 i = 0; i < assertBoundaryStmt->GetOpnds().size(); i++) {
        assertBoundaryNode->GetNopnd().push_back(EmitPreMeExpr(assertBoundaryStmt->GetOpnd(i), assertBoundaryNode));
      }
      assertBoundaryNode->SetNumOpnds(static_cast<uint8>(assertBoundaryNode->GetNopndSize()));
      assertBoundaryNode->CopySafeRegionAttr(meStmt->GetStmtAttr());
      assertBoundaryNode->SetOriginalID(meStmt->GetOriginalId());
      preMeStmtExtensionMap[assertBoundaryNode->GetStmtID()] = pmeExt;
      return assertBoundaryNode;
    }
    case OP_syncenter:
    case OP_syncexit: {
      auto naryMeStmt = static_cast<NaryMeStmt *>(meStmt);
      auto syncStmt = codeMP->New<NaryStmtNode>(*codeMPAlloc, meStmt->GetOp());
      for (uint32 i = 0; i < naryMeStmt->GetOpnds().size(); i++) {
        syncStmt->GetNopnd().push_back(EmitPreMeExpr(naryMeStmt->GetOpnd(i), syncStmt));
      }
      syncStmt->SetNumOpnds(syncStmt->GetNopndSize());
      return syncStmt;
    }
    default:
      CHECK_FATAL(false, "nyi");
  }
}

void PreMeEmitter::UpdateStmtInfoForLabelNode(LabelNode &label, BB &bb) {
  if (ipaInfo == nullptr) {
    return;
  }
  label.SetStmtInfoId(ipaInfo->GetRealFirstStmtInfoId(bb));
}

void PreMeEmitter::UpdateStmtInfo(const MeStmt &meStmt, StmtNode &stmt, BlockNode &currBlock, uint64 frequency) {
  if (ipaInfo == nullptr || meStmt.GetStmtInfoId() == kInvalidIndex) {
    return;
  }
  auto &stmtInfo = ipaInfo->GetStmtInfo()[meStmt.GetStmtInfoId()];
  stmtInfo.SetStmtNode(&stmt);
  stmtInfo.SetCurrBlock(&currBlock);
  stmtInfo.SetFrequency(frequency);
  stmt.SetStmtInfoId(meStmt.GetStmtInfoId());
}

void PreMeEmitter::EmitBB(BB *bb, BlockNode *curBlk) {
  CHECK_FATAL(curBlk != nullptr, "null ptr check");
  bool setFirstFreq = (GetFuncProfData() != nullptr);
  bool setLastFreq = false;
  bool bbIsEmpty = bb->GetMeStmts().empty();
  // emit head. label
  LabelIdx labidx = bb->GetBBLabel();
  if (labidx != 0 && !preMeFunc->WhileLabelCreatedByPreMe(labidx) && !preMeFunc->IfLabelCreatedByPreMe(labidx)) {
    // not a empty bb
    LabelNode *lbnode = codeMP->New<LabelNode>();
    UpdateStmtInfoForLabelNode(*lbnode, *bb);
    lbnode->SetLabelIdx(labidx);
    curBlk->AddStatement(lbnode);
    PreMeMIRExtension *pmeExt = preMeMP->New<PreMeMIRExtension>(curBlk);
    preMeStmtExtensionMap[lbnode->GetStmtID()] = pmeExt;
    if (GetFuncProfData()) {
      GetFuncProfData()->SetStmtFreq(lbnode->GetStmtID(), bb->GetFrequency());
    }
  }
  for (auto& mestmt : bb->GetMeStmts()) {
    StmtNode *stmt = EmitPreMeStmt(&mestmt, curBlk);
    if (!stmt) {
      // can be null i.e, a goto to a label that was created by lno lower
      continue;
    }
    UpdateStmtInfo(mestmt, *stmt, *curBlk, bb->GetFrequency());
    curBlk->AddStatement(stmt);
    // add <stmtID, freq> for first stmt in bb in curblk
    if (GetFuncProfData() != nullptr) {
      if (setFirstFreq || (stmt->GetOpCode() == OP_call) || IsCallAssigned(stmt->GetOpCode())) {
        GetFuncProfData()->SetStmtFreq(stmt->GetStmtID(), bb->GetFrequency());
        setFirstFreq = false;
      } else {
        setLastFreq = true;
      }
    }
  }
  if (bb->GetAttributes(kBBAttrIsTryEnd)) {
    /* generate op_endtry */
    StmtNode *endtry = codeMP->New<StmtNode>(OP_endtry);
    curBlk->AddStatement(endtry);
    PreMeMIRExtension *pmeExt = preMeMP->New<PreMeMIRExtension>(curBlk);
    preMeStmtExtensionMap[endtry->GetStmtID()] = pmeExt;
    setLastFreq = true;
  }
  // add stmtnode to last
  if (GetFuncProfData()) {
    if (setLastFreq) {
      GetFuncProfData()->SetStmtFreq(curBlk->GetLast()->GetStmtID(), bb->GetFrequency());
    } else if (bbIsEmpty) {
      LogInfo::MapleLogger() << " bb " << bb->GetBBId() << "no stmt used to add frequency, add commentnode\n";
      CommentNode *commentNode = codeMP->New<CommentNode>(*(mirFunc->GetModule()));
      commentNode->SetComment("freqStmt"+std::to_string(commentNode->GetStmtID()));
      GetFuncProfData()->SetStmtFreq(commentNode->GetStmtID(), bb->GetFrequency());
      curBlk->AddStatement(commentNode);
    }
  }
}

DoloopNode *PreMeEmitter::EmitPreMeDoloop(BB *meWhileBB, BlockNode *curBlk, PreMeWhileInfo *whileInfo) {
  MeStmt *lastmestmt = meWhileBB->GetLastMe();
  ASSERT_NOT_NULL(lastmestmt);
  CHECK_FATAL(lastmestmt->GetPrev() == nullptr || dynamic_cast<AssignMeStmt *>(lastmestmt->GetPrev()) == nullptr,
              "EmitPreMeDoLoop: there are other statements at while header bb");
  DoloopNode *Doloopnode = codeMP->New<DoloopNode>();
  PreMeMIRExtension *pmeExt = preMeMP->New<PreMeMIRExtension>(curBlk);
  pmeExt->mestmt = lastmestmt;
  preMeStmtExtensionMap[Doloopnode->GetStmtID()] = pmeExt;
  Doloopnode->SetDoVarStIdx(whileInfo->ivOst->GetMIRSymbol()->GetStIdx());
  CondGotoMeStmt *condGotostmt = static_cast<CondGotoMeStmt *>(lastmestmt);
  Doloopnode->SetStartExpr(EmitPreMeExpr(whileInfo->initExpr, Doloopnode));
  Doloopnode->SetContExpr(EmitPreMeExpr(condGotostmt->GetOpnd(), Doloopnode));
  CompareNode *compare = static_cast<CompareNode *>(Doloopnode->GetCondExpr());
  if (compare->Opnd(0)->GetOpCode() == OP_cvt && compare->Opnd(0)->Opnd(0)->GetOpCode() == OP_cvt) {
    PrimType resPrimType = compare->Opnd(0)->GetPrimType();
    PrimType opndPrimType = static_cast<TypeCvtNode*>(compare->Opnd(0))->FromType();
    TypeCvtNode *secondCvtX = static_cast<TypeCvtNode*>(compare->Opnd(0)->Opnd(0));
    if (IsNoCvtNeeded(resPrimType, secondCvtX->FromType()) &&
        IsNoCvtNeeded(opndPrimType, secondCvtX->GetPrimType())) {
      compare->SetOpnd(secondCvtX->Opnd(0), 0);
    }
  }
  BlockNode *dobodyNode = codeMP->New<BlockNode>();
  Doloopnode->SetDoBody(dobodyNode);
  PreMeMIRExtension *doloopExt = preMeMP->New<PreMeMIRExtension>(Doloopnode);
  preMeStmtExtensionMap[dobodyNode->GetStmtID()] = doloopExt;
  MIRIntConst *intConst =
      mirFunc->GetModule()->GetMemPool()->New<MIRIntConst>(whileInfo->stepValue, *whileInfo->ivOst->GetType());
  ConstvalNode *constnode = codeMP->New<ConstvalNode>(intConst->GetType().GetPrimType(), intConst);
  preMeExprExtensionMap[constnode] = doloopExt;
  Doloopnode->SetIncrExpr(constnode);
  Doloopnode->SetIsPreg(false);
  curBlk->AddStatement(Doloopnode);
  // add stmtfreq
  if (GetFuncProfData()) {
    GetFuncProfData()->SetStmtFreq(Doloopnode->GetStmtID(), meWhileBB->GetFrequency());
  }
  return Doloopnode;
}

WhileStmtNode *PreMeEmitter::EmitPreMeWhile(BB *meWhileBB, BlockNode *curBlk) {
  MeStmt *lastmestmt = meWhileBB->GetLastMe();
  ASSERT_NOT_NULL(lastmestmt);
  CHECK_FATAL(lastmestmt->GetPrev() == nullptr || dynamic_cast<AssignMeStmt *>(lastmestmt->GetPrev()) == nullptr,
              "EmitPreMeWhile: there are other statements at while header bb");
  WhileStmtNode *Whilestmt = codeMP->New<WhileStmtNode>(OP_while);
  PreMeMIRExtension *pmeExt = preMeMP->New<PreMeMIRExtension>(curBlk);
  preMeStmtExtensionMap[Whilestmt->GetStmtID()] = pmeExt;
  CondGotoMeStmt *condGotostmt = static_cast<CondGotoMeStmt *>(lastmestmt);
  Whilestmt->SetOpnd(EmitPreMeExpr(condGotostmt->GetOpnd(), Whilestmt), 0);
  BlockNode *whilebodyNode = codeMP->New<BlockNode>();
  PreMeMIRExtension *whilenodeExt = preMeMP->New<PreMeMIRExtension>(Whilestmt);
  preMeStmtExtensionMap[whilebodyNode->GetStmtID()] = whilenodeExt;
  Whilestmt->SetBody(whilebodyNode);
  // add stmtfreq
  if (GetFuncProfData()) {
    GetFuncProfData()->SetStmtFreq(Whilestmt->GetStmtID(), meWhileBB->GetFrequency());
  }
  curBlk->AddStatement(Whilestmt);
  return Whilestmt;
}

uint32 PreMeEmitter::Raise2PreMeWhile(uint32 curJ, BlockNode *curBlk) {
  MapleVector<BB *> &bbvec = cfg->GetAllBBs();
  BB *curbb = bbvec[curJ];
  LabelIdx whilelabidx = curbb->GetBBLabel();
  PreMeWhileInfo *whileInfo = preMeFunc->label2WhileInfo[whilelabidx];

  // find the end label bb
  BB *suc0 = curbb->GetSucc(0);
  BB *suc1 = curbb->GetSucc(1);
  MeStmt *laststmt = curbb->GetLastMe();
  CHECK_FATAL(laststmt->GetOp() == OP_brfalse, "Riase2While: NYI");
  CondGotoMeStmt *condgotomestmt = static_cast<CondGotoMeStmt *>(laststmt);
  BB *endlblbb = condgotomestmt->GetOffset() == suc1->GetBBLabel() ? suc1 : suc0;
  BlockNode *dobody = nullptr;
  StmtNode *loop = nullptr;
  ++curJ;
  if (whileInfo->canConvertDoloop) {  // emit doloop
    auto *doloopNode = EmitPreMeDoloop(curbb, curBlk, whileInfo);
    loop = doloopNode;
    dobody = doloopNode->GetDoBody();
  } else { // emit while loop
    auto *whileNode = EmitPreMeWhile(curbb, curBlk);
    loop = whileNode;
    dobody = whileNode->GetBody();
  }
  UpdateStmtInfo(*laststmt, *loop, *curBlk, curbb->GetFrequency());

  // emit loop body
  while (bbvec[curJ]->GetBBId() != endlblbb->GetBBId()) {
    curJ = EmitPreMeBB(curJ, dobody);
    while (bbvec[curJ] == nullptr) {
      ++curJ;
    }
  }
  if (whileInfo->canConvertDoloop) {  // delete the increment statement
    StmtNode *bodylaststmt = dobody->GetLast();
    CHECK_FATAL(bodylaststmt->GetOpCode() == OP_dassign, "Raise2PreMeWhile: cannot find increment stmt");
    DassignNode *dassnode = static_cast<DassignNode *>(bodylaststmt);
    CHECK_FATAL(dassnode->GetStIdx() == whileInfo->ivOst->GetMIRSymbol()->GetStIdx(),
                "Raise2PreMeWhile: cannot find IV increment");
    dobody->RemoveStmt(dassnode);
  }
  // set dobody freq
  if (GetFuncProfData()) {
    int64_t freq = (endlblbb == suc0) ? suc1->GetFrequency() : suc0->GetFrequency();
    GetFuncProfData()->SetStmtFreq(dobody->GetStmtID(), static_cast<uint64>(freq));
  }
  return curJ;
}

uint32 PreMeEmitter::Raise2PreMeIf(uint32 curJ, BlockNode *curBlk) {
  MapleVector<BB *> &bbvec = cfg->GetAllBBs();
  BB *curbb = bbvec[curJ];
  bool setFirstFreq = (GetFuncProfData() != nullptr);
  // emit BB contents before the if statement
  LabelIdx labidx = curbb->GetBBLabel();
  if (labidx != 0 && !preMeFunc->IfLabelCreatedByPreMe(labidx)) {
    LabelNode *lbnode = mirFunc->GetCodeMempool()->New<LabelNode>();
    UpdateStmtInfoForLabelNode(*lbnode, *curbb);
    lbnode->SetLabelIdx(labidx);
    curBlk->AddStatement(lbnode);
    PreMeMIRExtension *pmeExt = preMeMP->New<PreMeMIRExtension>(curBlk);
    preMeStmtExtensionMap[lbnode->GetStmtID()] = pmeExt;
  }
  MeStmt *mestmt = curbb->GetFirstMe();
  while (mestmt->GetOp() != OP_brfalse && mestmt->GetOp() != OP_brtrue) {
    StmtNode *stmt = EmitPreMeStmt(mestmt, curBlk);
    if (stmt == nullptr) {
      mestmt = mestmt->GetNext();
      continue;
    }
    UpdateStmtInfo(*mestmt, *stmt, *curBlk, curbb->GetFrequency());
    curBlk->AddStatement(stmt);
    if (GetFuncProfData() &&
        (setFirstFreq || (stmt->GetOpCode() == OP_call) || IsCallAssigned(stmt->GetOpCode()))) {
      // add frequency of first/call stmt of curbb
      GetFuncProfData()->SetStmtFreq(stmt->GetStmtID(), curbb->GetFrequency());
      setFirstFreq = false;
    }
    mestmt = mestmt->GetNext();
  }
  // emit the if statement
  CHECK_FATAL(mestmt != nullptr && (mestmt->GetOp() == OP_brfalse || mestmt->GetOp() == OP_brtrue),
              "Raise2PreMeIf: cannot find conditional branch");
  CondGotoMeStmt *condgoto = static_cast <CondGotoMeStmt *>(mestmt);
  PreMeIfInfo *ifInfo = preMeFunc->label2IfInfo[condgoto->GetOffset()];
  CHECK_FATAL(ifInfo->endLabel != 0, "Raise2PreMeIf: endLabel not found");
  IfStmtNode *ifStmtNode = mirFunc->GetCodeMempool()->New<IfStmtNode>();
  PreMeMIRExtension *pmeExt = preMeMP->New<PreMeMIRExtension>(curBlk);
  preMeStmtExtensionMap[ifStmtNode->GetStmtID()] = pmeExt;
  BaseNode *condnode = EmitPreMeExpr(condgoto->GetOpnd(), ifStmtNode);
  if (condgoto->IsBranchProbValid() && (condgoto->GetBranchProb() == kProbLikely ||
                                        condgoto->GetBranchProb() == kProbUnlikely)) {
    IntrinsicopNode *expectNode = codeMP->New<IntrinsicopNode>(*mirFunc->GetModule(), OP_intrinsicop, PTY_i64);
    expectNode->SetIntrinsic(INTRN_C___builtin_expect);
    expectNode->GetNopnd().push_back(condnode);
    MIRType *type = GlobalTables::GetTypeTable().GetPrimType(PTY_i64);
    // | op\jmpProp | likelyJump (1) | unlikelyJump(0) |
    // +-----------------------------------------------+
    // | brtrue (1) | expectTrue (1) | expectFalse (0) |
    // | brfalse(0) | expectFalse(0) | expectTrue  (1) |
    // XNOR
    uint32 val = !(static_cast<uint32>(mestmt->GetOp() == OP_brtrue) ^ (condgoto->GetBranchProb() == kProbLikely));
    MIRIntConst *constVal = GlobalTables::GetIntConstTable().GetOrCreateIntConst(val, *type);
    ConstvalNode *constNode = codeMP->New<ConstvalNode>(constVal->GetType().GetPrimType(), constVal);
    expectNode->GetNopnd().push_back(constNode);
    expectNode->SetNumOpnds(expectNode->GetNopnd().size());
    MIRIntConst *constZeroVal = GlobalTables::GetIntConstTable().GetOrCreateIntConst(0, *type);
    ConstvalNode *constZeroNode = codeMP->New<ConstvalNode>(constVal->GetType().GetPrimType(), constZeroVal);
    CompareNode *cmpNode =
        codeMP->New<CompareNode>(OP_ne, PTY_i32, PTY_i32, expectNode, constZeroNode);
    condnode = cmpNode;
  }
  ifStmtNode->SetOpnd(condnode, 0);
  ifStmtNode->SetMeStmtID(condgoto->GetMeStmtId());
  UpdateStmtInfo(*mestmt, *ifStmtNode, *curBlk, curbb->GetFrequency());
  curBlk->AddStatement(ifStmtNode);
  if (GetFuncProfData()) {
    // set ifstmt freq
    GetFuncProfData()->SetStmtFreq(ifStmtNode->GetStmtID(), curbb->GetFrequency());
  }
  PreMeMIRExtension *ifpmeExt = preMeMP->New<PreMeMIRExtension>(ifStmtNode);
  if (ifInfo->elseLabel != 0) {  // both else and then are not empty;
    BlockNode *elseBlk = codeMP->New<BlockNode>();
    preMeStmtExtensionMap[elseBlk->GetStmtID()] = ifpmeExt;
    BlockNode *thenBlk = codeMP->New<BlockNode>();
    preMeStmtExtensionMap[thenBlk->GetStmtID()] = ifpmeExt;
    ifStmtNode->SetThenPart(thenBlk);
    ifStmtNode->SetElsePart(elseBlk);
    BB *elsemebb = cfg->GetLabelBBAt(ifInfo->elseLabel);
    BB *endmebb = cfg->GetLabelBBAt(ifInfo->endLabel);
    CHECK_FATAL(elsemebb, "Raise2PreMeIf: cannot find else BB");
    CHECK_FATAL(endmebb, "Raise2PreMeIf: cannot find BB at end of IF");
    // emit then branch;
    uint32 j = curJ + 1;
    while (j != elsemebb->GetBBId()) {
      j = EmitPreMeBB(j, thenBlk);
    }
    CHECK_FATAL(j < bbvec.size(), "");
    while (j != endmebb->GetBBId()) {
      j = EmitPreMeBB(j, elseBlk);
    }
    if (GetFuncProfData()) {
      // set then part/else part frequency
      GetFuncProfData()->SetStmtFreq(thenBlk->GetStmtID(), bbvec[curJ + 1]->GetFrequency());
      GetFuncProfData()->SetStmtFreq(elseBlk->GetStmtID(), elsemebb->GetFrequency());
    }
    CHECK_FATAL(j < bbvec.size(), "");
    return j;
  } else {  // there is only then or else part in this if stmt
    BlockNode *branchBlock = codeMP->New<BlockNode>();
    preMeStmtExtensionMap[branchBlock->GetStmtID()] = ifpmeExt;
    BlockNode *emptyBlock = codeMP->New<BlockNode>();
    preMeStmtExtensionMap[emptyBlock->GetStmtID()] = ifpmeExt;
    if (condgoto->GetOp() == OP_brtrue) {
      ifStmtNode->SetElsePart(branchBlock);
      ifStmtNode->SetThenPart(emptyBlock);
    } else {
      ifStmtNode->SetThenPart(branchBlock);
      ifStmtNode->SetElsePart(emptyBlock);
    }
    BB *endmebb = cfg->GetLabelBBAt(ifInfo->endLabel);
    uint32 j = curJ + 1;
    ASSERT_NOT_NULL(endmebb);
    while (j != endmebb->GetBBId()) {
      j = EmitPreMeBB(j, branchBlock);
    }
    CHECK_FATAL(j < bbvec.size(), "");
    if (GetFuncProfData()) {
      // set then part/else part frequency
      uint64 ifFreq = GetFuncProfData()->GetStmtFreq(ifStmtNode->GetStmtID());
      uint64 branchFreq = bbvec[curJ + 1]->GetFrequency();
      GetFuncProfData()->SetStmtFreq(branchBlock->GetStmtID(), branchFreq);
      GetFuncProfData()->SetStmtFreq(emptyBlock->GetStmtID(), ifFreq - branchFreq);
    }
    return j;
  }
}

uint32 PreMeEmitter::EmitPreMeBB(uint32 curJ, BlockNode *curBlk) {
  MapleVector<BB *> &bbvec = cfg->GetAllBBs();
  BB *mebb = bbvec[curJ];
  if (!mebb || mebb == cfg->GetCommonEntryBB() || mebb == cfg->GetCommonExitBB()) {
    return curJ + 1;
  }
  if (mebb->GetBBLabel() != 0) {
    MapleMap<LabelIdx, PreMeWhileInfo*>::const_iterator it = preMeFunc->label2WhileInfo.find(mebb->GetBBLabel());
    if (it != preMeFunc->label2WhileInfo.end()) {
      if (mebb->GetSucc().size() == 2) {
        curJ = Raise2PreMeWhile(curJ, curBlk);
        return curJ;
      } else {
        preMeFunc->pmeCreatedWhileLabelSet.erase(mebb->GetBBLabel());
      }
    }
  }
  if (!mebb->GetMeStmts().empty() &&
      (mebb->GetLastMe()->GetOp() == OP_brfalse ||
       mebb->GetLastMe()->GetOp() == OP_brtrue)) {
    CondGotoMeStmt *condgoto = static_cast<CondGotoMeStmt *>(mebb->GetLastMe());
    MapleMap<LabelIdx, PreMeIfInfo*>::const_iterator it = preMeFunc->label2IfInfo.find(condgoto->GetOffset());
    if (it != preMeFunc->label2IfInfo.end()) {
      curJ = Raise2PreMeIf(curJ, curBlk);
      return curJ;
    }
  }
  EmitBB(mebb, curBlk);
  return ++curJ;
}

void MEPreMeEmission::SetIpaInfo(MIRFunction &mirFunc) {
  if (!mirFunc.GetModule()->IsInIPA() || !Options::doOutline) {
    return;
  }
  auto sccEmitPm = dynamic_cast<MaplePhase*>(GetAnalysisInfoHook()->GetBindingPM());
  auto *ipaInfo = static_cast<IpaSccPM*>(sccEmitPm->GetAnalysisInfoHook()->GetBindingPM())->GetResult();
  emitter->SetIpaInfo(ipaInfo);
}

bool MEPreMeEmission::PhaseRun(MeFunction &f) {
  if (f.GetCfg()->NumBBs() == 0) {
    f.SetLfo(false);
    f.SetPme(false);
    return false;
  }
  MeIRMap *hmap = GET_ANALYSIS(MEIRMapBuild, f);
  ASSERT(hmap != nullptr, "irmapbuild has problem");

  MIRFunction *mirfunction = f.GetMirFunc();
  if (mirfunction->GetCodeMempool() != nullptr) {
    memPoolCtrler.DeleteMemPool(mirfunction->GetCodeMempool());
  }
  mirfunction->SetMemPool(new ThreadLocalMemPool(memPoolCtrler, "IR from preemission::Emit()"));
  MemPool *preMeMP = GetPhaseMemPool();
  emitter = preMeMP->New<PreMeEmitter>(hmap, f.GetPreMeFunc(), preMeMP);
  SetIpaInfo(*mirfunction);
  BlockNode *curblk = mirfunction->GetCodeMempool()->New<BlockNode>();
  mirfunction->SetBody(curblk);
  // restore bb frequency to stmt
  if (Options::profileUse && emitter->GetFuncProfData()) {
    emitter->GetFuncProfData()->SetStmtFreq(curblk->GetStmtID(), emitter->GetFuncProfData()->entryFreq);
  }
  uint32 i = 0;
  while (i < f.GetCfg()->GetAllBBs().size()) {
    i = emitter->EmitPreMeBB(i, curblk);
  }

  f.SetLfo(false);
  f.SetPme(false);

  if (DEBUGFUNC_NEWPM(f)) {
    LogInfo::MapleLogger() << "\n**** After premeemit phase ****\n";
    mirfunction->Dump(false);
  }

  if (Options::enableGInline) {
    auto *inlineSummary = f.GetMirFunc()->GetInlineSummary();
    if (inlineSummary != nullptr) {
      inlineSummary->RefreshMapKeyIfNeeded();
    }
  }

  return emitter;
}

void MEPreMeEmission::GetAnalysisDependence(maple::AnalysisDep &aDep) const {
  aDep.AddRequired<MEIRMapBuild>();
  aDep.SetPreservedAll();
}
}  // namespace maple
