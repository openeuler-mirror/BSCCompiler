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
#include "lfo_mir_nodes.h"
#include "lfo_function.h"
#include "lfo_pre_emit.h"
#include "mir_lower.h"
#include "constantfold.h"

namespace maple {
LfoParentPart *LfoPreEmitter::EmitLfoExpr(MeExpr *meexpr, LfoParentPart *parent) {
  switch (meexpr->GetOp()) {
    case OP_constval: {
      LfoConstvalNode *lcvlNode =
        codeMP->New<LfoConstvalNode>(static_cast<ConstMeExpr *>(meexpr)->GetConstVal(), parent);
      return lcvlNode;
    }
    case OP_dread: {
      VarMeExpr *varmeexpr = static_cast<VarMeExpr *>(meexpr);
      MIRSymbol *sym = varmeexpr->GetOst()->GetMIRSymbol();
      LfoDreadNode *ldnode = codeMP->New<LfoDreadNode>(varmeexpr->GetPrimType(), sym->GetStIdx(),
                                                       varmeexpr->GetOst()->GetFieldID(), parent, varmeexpr);
      return ldnode;
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
      OpMeExpr *cmpNode = static_cast<OpMeExpr *>(meexpr);
      LfoCompareNode *lnocmpNode =
        codeMP->New<LfoCompareNode>(meexpr->GetOp(), cmpNode->GetPrimType(), cmpNode->GetOpndType(),
                                     nullptr, nullptr, parent);
      LfoParentPart *opnd0 = EmitLfoExpr(cmpNode->GetOpnd(0), lnocmpNode);
      LfoParentPart *opnd1 = EmitLfoExpr(cmpNode->GetOpnd(1), lnocmpNode);
      lnocmpNode->SetBOpnd(opnd0->Cvt2BaseNode(), 0);
      lnocmpNode->SetBOpnd(opnd1->Cvt2BaseNode(), 1);
      lnocmpNode->SetOpndType(cmpNode->GetOpndType());
      return lnocmpNode;
    }
    case OP_array: {
      NaryMeExpr *arrExpr = static_cast<NaryMeExpr *>(meexpr);
      LfoArrayNode *lnoarrNode =
        codeMP->New<LfoArrayNode>(codeMPAlloc, arrExpr->GetPrimType(), arrExpr->GetTyIdx(), parent);
      lnoarrNode->SetTyIdx(arrExpr->GetTyIdx());
      lnoarrNode->SetBoundsCheck(arrExpr->GetBoundCheck());
      for (uint32 i = 0; i < arrExpr->GetNumOpnds(); i++) {
        LfoParentPart *opnd = EmitLfoExpr(arrExpr->GetOpnd(i), lnoarrNode);
        lnoarrNode->GetNopnd().push_back(opnd->Cvt2BaseNode());
      }
      lnoarrNode->SetNumOpnds(meexpr->GetNumOpnds());
      return lnoarrNode;
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
      LfoBinaryNode *lnobinNode =
        codeMP->New<LfoBinaryNode>(meexpr->GetOp(), meexpr->GetPrimType(), parent);
      lnobinNode->SetBOpnd(EmitLfoExpr(opExpr->GetOpnd(0), lnobinNode)->Cvt2BaseNode(), 0);
      lnobinNode->SetBOpnd(EmitLfoExpr(opExpr->GetOpnd(1), lnobinNode)->Cvt2BaseNode(), 1);
      return lnobinNode;
    }
    case OP_iread: {
      IvarMeExpr *ivarExpr = static_cast<IvarMeExpr *>(meexpr);
      LfoIreadNode *lnoirdNode = codeMP->New<LfoIreadNode>(meexpr->GetOp(), meexpr->GetPrimType(), parent, ivarExpr);
      lnoirdNode->SetOpnd(EmitLfoExpr(ivarExpr->GetBase(), lnoirdNode)->Cvt2BaseNode(), 0);
      lnoirdNode->SetTyIdx(ivarExpr->GetTyIdx());
      lnoirdNode->SetFieldID(ivarExpr->GetFieldID());
      return lnoirdNode;
    }
    case OP_addrof: {
      AddrofMeExpr *addrMeexpr = static_cast<AddrofMeExpr *> (meexpr);
      OriginalSt *ost = meirmap->GetSSATab().GetOriginalStFromID(addrMeexpr->GetOstIdx());
      MIRSymbol *sym = ost->GetMIRSymbol();
      LfoAddrofNode *lnoaddrofNode =
          codeMP->New<LfoAddrofNode>(addrMeexpr->GetPrimType(), sym->GetStIdx(), ost->GetFieldID(), parent);
      return lnoaddrofNode;
    }
    case OP_addroflabel: {
      AddroflabelMeExpr *addroflabel = static_cast<AddroflabelMeExpr *>(meexpr);
      LfoAddroflabelNode *lnoaddroflabel = codeMP->New<LfoAddroflabelNode>(addroflabel->labelIdx, parent);
      // lnoaddroflabel->SetPrimType(PTY_ptr);
      lnoaddroflabel->SetPrimType(meexpr->GetPrimType());
      return lnoaddroflabel;
    }
    case OP_addroffunc: {
      AddroffuncMeExpr *addrMeexpr = static_cast<AddroffuncMeExpr *>(meexpr);
      LfoAddroffuncNode *addrfunNode =
          codeMP->New<LfoAddroffuncNode>(addrMeexpr->GetPrimType(), addrMeexpr->GetPuIdx(), parent);
      return addrfunNode;
    }
    case OP_gcmalloc:
    case OP_gcpermalloc:
    case OP_stackmalloc: {
      GcmallocMeExpr *gcMeexpr = static_cast<GcmallocMeExpr *> (meexpr);
      LfoGCMallocNode *gcMnode =
          codeMP->New<LfoGCMallocNode>(meexpr->GetOp(), meexpr->GetPrimType(), gcMeexpr->GetTyIdx(), parent);
      gcMnode->SetTyIdx(gcMeexpr->GetTyIdx());
      return gcMnode;
    }
    case OP_retype: {
      OpMeExpr *opMeexpr = static_cast<OpMeExpr *>(meexpr);
      LfoRetypeNode *lnoRetNode = codeMP->New<LfoRetypeNode>(OP_retype, meexpr->GetPrimType(), parent);
      lnoRetNode->SetFromType(opMeexpr->GetOpndType());
      lnoRetNode->SetTyIdx(opMeexpr->GetTyIdx());
      lnoRetNode->SetOpnd(EmitLfoExpr(opMeexpr->GetOpnd(0), lnoRetNode)->Cvt2BaseNode(), 0);
      return lnoRetNode;
    }
    case OP_ceil:
    case OP_cvt:
    case OP_floor:
    case OP_trunc: {
      OpMeExpr *opMeexpr = static_cast<OpMeExpr *>(meexpr);
      LfoTypeCvtNode *tycvtNode = codeMP->New<LfoTypeCvtNode>(meexpr->GetOp(), meexpr->GetPrimType(), parent);
      tycvtNode->SetFromType(opMeexpr->GetOpndType());
      tycvtNode->SetOpnd(EmitLfoExpr(opMeexpr->GetOpnd(0), tycvtNode)->Cvt2BaseNode(), 0);
      return tycvtNode;
    }
    case OP_sext:
    case OP_zext:
    case OP_extractbits: {
      OpMeExpr *opMeexpr = static_cast<OpMeExpr *>(meexpr);
      LfoExtractbitsNode *extNode = codeMP->New<LfoExtractbitsNode>(meexpr->GetOp(), meexpr->GetPrimType(), parent);
      extNode->SetOpnd(EmitLfoExpr(opMeexpr->GetOpnd(0), extNode)->Cvt2BaseNode(), 0);
      extNode->SetBitsOffset(opMeexpr->GetBitsOffSet());
      extNode->SetBitsSize(opMeexpr->GetBitsSize());
      return extNode;
    }
    case OP_regread: {
      RegMeExpr *regMeexpr = static_cast<RegMeExpr *>(meexpr);
      LfoRegreadNode *regNode = codeMP->New<LfoRegreadNode>(parent, regMeexpr);
      regNode->SetPrimType(regMeexpr->GetPrimType());
      regNode->SetRegIdx(regMeexpr->GetRegIdx());
      return regNode;
    }
    case OP_sizeoftype: {
      SizeoftypeMeExpr *sizeofMeexpr = static_cast<SizeoftypeMeExpr *>(meexpr);
      LfoSizeoftypeNode *sizeofTynode =
          codeMP->New<LfoSizeoftypeNode>(sizeofMeexpr->GetPrimType(), sizeofMeexpr->GetTyIdx(), parent);
      return sizeofTynode;
    }
    case OP_fieldsdist: {
      FieldsDistMeExpr *fdMeexpr = static_cast<FieldsDistMeExpr *>(meexpr);
      LfoFieldsDistNode *fieldsNode = codeMP->New<LfoFieldsDistNode>(
        fdMeexpr->GetPrimType(), fdMeexpr->GetTyIdx(), fdMeexpr->GetFieldID1(),
        fdMeexpr->GetFieldID2(), parent);
      return fieldsNode;
    }
    case OP_conststr: {
      ConststrMeExpr *constrMeexpr = static_cast<ConststrMeExpr *>(meexpr);
      LfoConststrNode *constrNode =
          codeMP->New<LfoConststrNode>(constrMeexpr->GetPrimType(), constrMeexpr->GetStrIdx(), parent);
      return constrNode;
    }
    case OP_conststr16: {
      Conststr16MeExpr *constr16Meexpr = static_cast<Conststr16MeExpr *>(meexpr);
      LfoConststr16Node *constr16Node =
          codeMP->New<LfoConststr16Node>(constr16Meexpr->GetPrimType(), constr16Meexpr->GetStrIdx(), parent);
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
      LfoUnaryNode *unNode = codeMP->New<LfoUnaryNode>(meexpr->GetOp(), meexpr->GetPrimType(), parent);
      unNode->SetOpnd(EmitLfoExpr(opMeexpr->GetOpnd(0), unNode)->Cvt2BaseNode(), 0);
      return unNode;
    }
    case OP_iaddrof: {
      OpMeExpr *opMeexpr = static_cast<OpMeExpr *>(meexpr);
      LfoIaddrofNode *unNode = codeMP->New<LfoIaddrofNode>(meexpr->GetOp(), meexpr->GetPrimType(), parent);
      unNode->SetOpnd(EmitLfoExpr(opMeexpr->GetOpnd(0), unNode)->Cvt2BaseNode(), 0);
      unNode->SetTyIdx(opMeexpr->GetTyIdx());
      unNode->SetFieldID(opMeexpr->GetFieldID());
      return unNode;
    }
    case OP_select: {
      OpMeExpr *opMeexpr = static_cast<OpMeExpr *>(meexpr);
      LfoTernaryNode *tNode = codeMP->New<LfoTernaryNode>(OP_select, meexpr->GetPrimType(), parent);
      tNode->SetOpnd(EmitLfoExpr(opMeexpr->GetOpnd(0), tNode)->Cvt2BaseNode(), 0);
      tNode->SetOpnd(EmitLfoExpr(opMeexpr->GetOpnd(1), tNode)->Cvt2BaseNode(), 1);
      tNode->SetOpnd(EmitLfoExpr(opMeexpr->GetOpnd(2), tNode)->Cvt2BaseNode(), 2);
      return tNode;
    }
    case OP_intrinsicop:
    case OP_intrinsicopwithtype: {
      NaryMeExpr *nMeexpr = static_cast<NaryMeExpr *>(meexpr);
      LfoIntrinsicopNode *intrnNode = codeMP->New<LfoIntrinsicopNode>(codeMPAlloc, meexpr->GetOp(),
                                         meexpr->GetPrimType(), nMeexpr->GetTyIdx(), parent);
      intrnNode->SetIntrinsic(nMeexpr->GetIntrinsic());
      for (uint32 i = 0; i < nMeexpr->GetNumOpnds(); i++) {
        LfoParentPart *opnd = EmitLfoExpr(nMeexpr->GetOpnd(i), intrnNode);
        intrnNode->GetNopnd().push_back(opnd->Cvt2BaseNode());
      }
      intrnNode->SetNumOpnds(nMeexpr->GetNumOpnds());
      return intrnNode;
    }
    default:
      CHECK_FATAL(false, "NYI");
  }
}

StmtNode* LfoPreEmitter::EmitLfoStmt(MeStmt *mestmt, LfoParentPart *parent) {
  switch (mestmt->GetOp()) {
    case OP_dassign: {
      DassignMeStmt *dsmestmt = static_cast<DassignMeStmt *>(mestmt);
      LfoDassignNode *dass = codeMP->New<LfoDassignNode>(parent, dsmestmt);
      MIRSymbol *sym = dsmestmt->GetLHS()->GetOst()->GetMIRSymbol();
      dass->SetStIdx(sym->GetStIdx());
      dass->SetFieldID(static_cast<VarMeExpr *>(dsmestmt->GetLHS())->GetOst()->GetFieldID());
      dass->SetOpnd(EmitLfoExpr(dsmestmt->GetRHS(), dass)->Cvt2BaseNode(), 0);
      dass->SetSrcPos(dsmestmt->GetSrcPosition());
      return dass;
    }
    case OP_regassign: {
      AssignMeStmt *asMestmt = static_cast<AssignMeStmt *>(mestmt);
      LfoRegassignNode *lrssnode = codeMP->New<LfoRegassignNode>(parent, asMestmt);
      lrssnode->SetPrimType(asMestmt->GetLHS()->GetPrimType());
      lrssnode->SetRegIdx(asMestmt->GetLHS()->GetRegIdx());
      lrssnode->SetOpnd(EmitLfoExpr(asMestmt->GetRHS(), lrssnode)->Cvt2BaseNode(), 0);
      lrssnode->SetSrcPos(asMestmt->GetSrcPosition());
      return lrssnode;
    }
    case OP_iassign: {
      IassignMeStmt *iass = static_cast<IassignMeStmt *>(mestmt);
      LfoIassignNode *lnoIassign =  codeMP->New<LfoIassignNode>(parent, iass);
      lnoIassign->SetTyIdx(iass->GetTyIdx());
      lnoIassign->SetFieldID(iass->GetLHSVal()->GetFieldID());
      lnoIassign->addrExpr = EmitLfoExpr(iass->GetLHSVal()->GetBase(), lnoIassign)->Cvt2BaseNode();
      lnoIassign->rhs = EmitLfoExpr(iass->GetRHS(), lnoIassign)->Cvt2BaseNode();
      lnoIassign->SetSrcPos(iass->GetSrcPosition());
      return lnoIassign;
    }
    case OP_return: {
      RetMeStmt *retMestmt = static_cast<RetMeStmt *>(mestmt);
      LfoReturnStmtNode *lnoRet = codeMP->New<LfoReturnStmtNode>(codeMPAlloc, parent, retMestmt);
      for (uint32 i = 0; i < retMestmt->GetOpnds().size(); i++) {
        lnoRet->GetNopnd().push_back(EmitLfoExpr(retMestmt->GetOpnd(i), lnoRet)->Cvt2BaseNode());
      }
      lnoRet->SetNumOpnds(retMestmt->GetOpnds().size());
      lnoRet->SetSrcPos(retMestmt->GetSrcPosition());
      return lnoRet;
    }
    case OP_goto: {
      GotoMeStmt *gotoStmt = static_cast<GotoMeStmt *>(mestmt);
      if (lfoFunc->LabelCreatedByLfo(gotoStmt->GetOffset())) {
        return nullptr;
      }
      LfoGotoNode *gto = codeMP->New<LfoGotoNode>(OP_goto, parent);
      gto->SetOffset(gotoStmt->GetOffset());
      gto->SetSrcPos(gotoStmt->GetSrcPosition());
      return gto;
    }
    case OP_igoto: {
      UnaryMeStmt *igotoMeStmt = static_cast<UnaryMeStmt *>(mestmt);
      LfoUnaryStmtNode *igto = codeMP->New<LfoUnaryStmtNode>(OP_igoto, parent);
      igto->SetOpnd(EmitLfoExpr(igotoMeStmt->GetOpnd(), igto)->Cvt2BaseNode(), 0);
      igto->SetSrcPos(igotoMeStmt->GetSrcPosition());
      return igto;
    }
    case OP_comment: {
      CommentMeStmt *cmtmeNode = static_cast<CommentMeStmt *>(mestmt);
      CommentNode *cmtNode = codeMP->New<CommentNode>(*codeMPAlloc);
      cmtNode->SetComment(cmtmeNode->GetComment());
      cmtNode->SetSrcPos(cmtmeNode->GetSrcPosition());
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
      LfoCallNode *callnode = codeMP->New<LfoCallNode>(codeMPAlloc, mestmt->GetOp(), parent, callMeStmt);
      callnode->SetPUIdx(callMeStmt->GetPUIdx());
      callnode->SetTyIdx(callMeStmt->GetTyIdx());
      callnode->SetNumOpnds(callMeStmt->GetOpnds().size());
      callnode->SetSrcPos(callMeStmt->GetSrcPosition());
      mestmt->EmitCallReturnVector(callnode->GetReturnVec());
      for (uint32 i = 0; i < callMeStmt->GetOpnds().size(); i++) {
        callnode->GetNopnd().push_back(EmitLfoExpr(callMeStmt->GetOpnd(i), callnode)->Cvt2BaseNode());
      }
      return callnode;
    }
    case OP_icall:
    case OP_icallassigned: {
      IcallMeStmt *icallMeStmt = static_cast<IcallMeStmt *> (mestmt);
      LfoIcallNode *icallnode =
          codeMP->New<LfoIcallNode>(codeMPAlloc, OP_icallassigned, icallMeStmt->GetRetTyIdx(), parent, icallMeStmt);
      for (uint32 i = 0; i < icallMeStmt->GetOpnds().size(); i++) {
        icallnode->GetNopnd().push_back(EmitLfoExpr(icallMeStmt->GetOpnd(i), icallnode)->Cvt2BaseNode());
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
      return icallnode;
    }
    case OP_intrinsiccall:
    case OP_xintrinsiccall:
    case OP_intrinsiccallassigned:
    case OP_xintrinsiccallassigned:
    case OP_intrinsiccallwithtype:
    case OP_intrinsiccallwithtypeassigned: {
      IntrinsiccallMeStmt *callMeStmt = static_cast<IntrinsiccallMeStmt *> (mestmt);
      LfoIntrinsiccallNode *callnode = codeMP->New<LfoIntrinsiccallNode>(codeMPAlloc,
                mestmt->GetOp(), callMeStmt->GetIntrinsic(), parent, callMeStmt);
      callnode->SetIntrinsic(callMeStmt->GetIntrinsic());
      callnode->SetTyIdx(callMeStmt->GetTyIdx());
      for (uint32 i = 0; i < callMeStmt->GetOpnds().size(); i++) {
        callnode->GetNopnd().push_back(EmitLfoExpr(callMeStmt->GetOpnd(i), callnode)->Cvt2BaseNode());
      }
      callnode->SetNumOpnds(callnode->GetNopndSize());
      callnode->SetSrcPos(mestmt->GetSrcPosition());
      if (kOpcodeInfo.IsCallAssigned(mestmt->GetOp())) {
        mestmt->EmitCallReturnVector(callnode->GetReturnVec());
        for (uint32 j = 0; j < callnode->GetReturnVec().size(); j++) {
          CallReturnPair retpair = callnode->GetReturnVec()[j];
          if (!retpair.second.IsReg()) {
            StIdx stIdx = retpair.first;
            if (stIdx.Islocal()) {

            }
          }
        }
      }
      return callnode;
    }
    case OP_jscatch:
    case OP_finally:
    case OP_endtry:
    case OP_cleanuptry:
    case OP_membaracquire:
    case OP_membarrelease:
    case OP_membarstorestore:
    case OP_membarstoreload: {
      LfoStmtNode *lnoStmtNode = codeMP->New<LfoStmtNode>(parent, mestmt->GetOp());
      lnoStmtNode->SetSrcPos(mestmt->GetSrcPosition());
      return lnoStmtNode;
    }
    case OP_retsub: {
      LfoStmtNode * usesStmtNode = codeMP->New<LfoStmtNode>(parent, mestmt->GetOp());
      usesStmtNode->SetSrcPos(mestmt->GetSrcPosition());
      return usesStmtNode;
    }
    case OP_brfalse:
    case OP_brtrue: {
      LfoCondGotoNode *lnoCondNode = codeMP->New<LfoCondGotoNode>(mestmt->GetOp(), parent);
      CondGotoMeStmt *condMeStmt = static_cast<CondGotoMeStmt *> (mestmt);
      lnoCondNode->SetOffset(condMeStmt->GetOffset());
      lnoCondNode->SetSrcPos(mestmt->GetSrcPosition());
      lnoCondNode->SetOpnd(EmitLfoExpr(condMeStmt->GetOpnd(), lnoCondNode)->Cvt2BaseNode(), 0);
      return lnoCondNode;
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
      return jvTryNode;
    }
    case OP_cppcatch: {
      CppCatchNode *cppCatchNode = codeMP->New<CppCatchNode>();
      CppCatchMeStmt *catchMestmt = static_cast<CppCatchMeStmt *> (mestmt);
      cppCatchNode->exceptionTyIdx = catchMestmt->exceptionTyIdx;
      cppCatchNode->SetSrcPos(catchMestmt->GetSrcPosition());
      return cppCatchNode;
    }
    case OP_catch: {
      CatchNode *jvCatchNode = codeMP->New<CatchNode>(*codeMPAlloc);
      CatchMeStmt *catchMestmt = static_cast<CatchMeStmt *> (mestmt);
      jvCatchNode->SetExceptionTyIdxVec(catchMestmt->GetExceptionTyIdxVec());
      jvCatchNode->SetSrcPos(catchMestmt->GetSrcPosition());
      return jvCatchNode;
    }
    case OP_throw: {
      LfoUnaryStmtNode *throwStmt = codeMP->New<LfoUnaryStmtNode>(mestmt->GetOp(), parent);
      ThrowMeStmt *throwMeStmt = static_cast<ThrowMeStmt *>(mestmt);
      throwStmt->SetOpnd(EmitLfoExpr(throwMeStmt->GetOpnd(), throwStmt)->Cvt2BaseNode(), 0);
      throwStmt->SetSrcPos(throwMeStmt->GetSrcPosition());
      return throwStmt;
    }
    case OP_assertnonnull:
    case OP_eval:
    case OP_free: {
      LfoUnaryStmtNode *unaryStmt = codeMP->New<LfoUnaryStmtNode>(mestmt->GetOp(), parent);
      UnaryMeStmt *uMeStmt = static_cast<UnaryMeStmt *>(mestmt);
      unaryStmt->SetOpnd(EmitLfoExpr(uMeStmt->GetOpnd(), unaryStmt)->Cvt2BaseNode(), 0);
      unaryStmt->SetSrcPos(uMeStmt->GetSrcPosition());
      return unaryStmt;
    }
    case OP_switch: {
      LfoSwitchNode *switchNode = codeMP->New<LfoSwitchNode>(codeMPAlloc, parent);
      SwitchMeStmt *meSwitch = static_cast<SwitchMeStmt *>(mestmt);
      switchNode->SetSwitchOpnd(EmitLfoExpr(meSwitch->GetOpnd(), switchNode)->Cvt2BaseNode());
      switchNode->SetDefaultLabel(meSwitch->GetDefaultLabel());
      switchNode->SetSwitchTable(meSwitch->GetSwitchTable());
      switchNode->SetSrcPos(meSwitch->GetSrcPosition());
      return switchNode;
    }
    default:
      CHECK_FATAL(false, "nyi");
  }
}

void LfoPreEmitter::EmitBB(BB *bb, LfoBlockNode *curblk) {
  CHECK_FATAL(curblk != nullptr, "null ptr check");
  // emit head. label
  LabelIdx labidx = bb->GetBBLabel();
  if (labidx != 0 && !lfoFunc->LabelCreatedByLfo(labidx)) {
    // not a empty bb
    LabelNode *lbnode = codeMP->New<LabelNode>();
    lbnode->SetLabelIdx(labidx);
    curblk->AddStatement(lbnode);
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
  }
}

DoloopNode *LfoPreEmitter::EmitLfoDoloop(BB *mewhilebb, LfoBlockNode *curblk, LfoWhileInfo *whileInfo) {
  MeStmt *lastmestmt = mewhilebb->GetLastMe();
  CHECK_FATAL(lastmestmt->GetPrev() == nullptr || dynamic_cast<AssignMeStmt *>(lastmestmt->GetPrev()) == nullptr,
              "EmitLfoWhile: there are other statements at while header bb");
  LfoDoloopNode *lnoDoloopnode = codeMP->New<LfoDoloopNode>(curblk);
  lnoDoloopnode->SetDoVarStIdx(whileInfo->ivOst->GetMIRSymbol()->GetStIdx());
  CondGotoMeStmt *condGotostmt = static_cast<CondGotoMeStmt *>(lastmestmt);
  lnoDoloopnode->SetStartExpr(EmitLfoExpr(whileInfo->initExpr, lnoDoloopnode)->Cvt2BaseNode());
  lnoDoloopnode->SetContExpr(EmitLfoExpr(condGotostmt->GetOpnd(), lnoDoloopnode)->Cvt2BaseNode());
  lnoDoloopnode->SetDoBody(codeMP->New<LfoBlockNode>(lnoDoloopnode));
  MIRIntConst *intConst =
      mirFunc->GetModule()->GetMemPool()->New<MIRIntConst>(whileInfo->stepValue, *whileInfo->ivOst->GetType());
  LfoConstvalNode *lfoconstnode = codeMP->New<LfoConstvalNode>(intConst, lnoDoloopnode);
  lnoDoloopnode->SetIncrExpr(lfoconstnode);
  lnoDoloopnode->SetIsPreg(false);
  curblk->AddStatement(lnoDoloopnode);
  return lnoDoloopnode;
}

WhileStmtNode *LfoPreEmitter::EmitLfoWhile(BB *meWhilebb, LfoBlockNode *curblk) {
  MeStmt *lastmestmt = meWhilebb->GetLastMe();
  CHECK_FATAL(lastmestmt->GetPrev() == nullptr || dynamic_cast<AssignMeStmt *>(lastmestmt->GetPrev()) == nullptr,
              "EmitLfoWhile: there are other statements at while header bb");
  LfoWhileStmtNode *lnoWhilestmt = codeMP->New<LfoWhileStmtNode>(curblk);
  CondGotoMeStmt *condGotostmt = static_cast<CondGotoMeStmt *>(lastmestmt);
  lnoWhilestmt->SetOpnd(EmitLfoExpr(condGotostmt->GetOpnd(), lnoWhilestmt)->Cvt2BaseNode(), 0);
  lnoWhilestmt->SetBody(codeMP->New<LfoBlockNode>(lnoWhilestmt));
  curblk->AddStatement(lnoWhilestmt);
  return lnoWhilestmt;
}

uint32 LfoPreEmitter::Raise2LfoWhile(uint32 curj, LfoBlockNode *curblk) {
  MapleVector<BB *> &bbvec = cfg->GetAllBBs();
  BB *curbb = bbvec[curj];
  LabelIdx whilelabidx = curbb->GetBBLabel();
  LfoWhileInfo *whileInfo = lfoFunc->label2WhileInfo[whilelabidx];

  // find the end label bb
  BB *suc0 = curbb->GetSucc(0);
  BB *suc1 = curbb->GetSucc(1);
  MeStmt *laststmt = curbb->GetLastMe();
  CHECK_FATAL(laststmt->GetOp() == OP_brfalse, "Riase2LfoWhile: NYI");
  CondGotoMeStmt *condgotomestmt = static_cast<CondGotoMeStmt *>(laststmt);
  BB *endlblbb = condgotomestmt->GetOffset() == suc1->GetBBLabel() ? suc1 : suc0;
  LfoBlockNode *lfoDobody = nullptr;
  if (whileInfo->canConvertDoloop) {  // emit doloop
    DoloopNode *doloopnode = EmitLfoDoloop(curbb, curblk, whileInfo);
    ++curj;
    lfoDobody = static_cast<LfoBlockNode *>(doloopnode->GetDoBody());
  } else { // emit while loop
    WhileStmtNode *whileNode = EmitLfoWhile(curbb, curblk);
    ++curj;
    lfoDobody = static_cast<LfoBlockNode *> (whileNode->GetBody());
  }
  // emit loop body
  while (bbvec[curj]->GetBBId() != endlblbb->GetBBId()) {
    curj = EmitLfoBB(curj, lfoDobody);
    while (bbvec[curj] == nullptr) {
      curj++;
    }
  }
  if (whileInfo->canConvertDoloop) {  // delete the increment statement
    StmtNode *bodylaststmt = lfoDobody->GetLast();
    CHECK_FATAL(bodylaststmt->GetOpCode() == OP_dassign, "Raise2LfoWhile: cannot find increment stmt");
    DassignNode *dassnode = static_cast<DassignNode *>(bodylaststmt);
    CHECK_FATAL(dassnode->GetStIdx() == whileInfo->ivOst->GetMIRSymbol()->GetStIdx(),
                "Raise2LfoWhile: cannot find IV increment");
    lfoDobody->RemoveStmt(dassnode);
  }
  return curj;
}

uint32 LfoPreEmitter::Raise2LfoIf(uint32 curj, LfoBlockNode *curblk) {
  MapleVector<BB *> &bbvec = cfg->GetAllBBs();
  BB *curbb = bbvec[curj];
  // emit BB contents before the if statement
  LabelIdx labidx = curbb->GetBBLabel();
  if (labidx != 0 && !lfoFunc->LabelCreatedByLfo(labidx)) {
    LabelNode *lbnode = mirFunc->GetCodeMempool()->New<LabelNode>();
    lbnode->SetLabelIdx(labidx);
    curblk->AddStatement(lbnode);
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
  LfoIfStmtNode *lnoIfstmtNode = mirFunc->GetCodeMempool()->New<LfoIfStmtNode>(curblk);
  LfoParentPart *condnode = EmitLfoExpr(condgoto->GetOpnd(), lnoIfstmtNode);
  lnoIfstmtNode->SetOpnd(condnode->Cvt2BaseNode(), 0);
  curblk->AddStatement(lnoIfstmtNode);
  if (ifInfo->elseLabel != 0) {  // both else and then are not empty;
    LfoBlockNode *elseBlk = codeMP->New<LfoBlockNode>(lnoIfstmtNode);
    LfoBlockNode *thenBlk = codeMP->New<LfoBlockNode>(lnoIfstmtNode);
    lnoIfstmtNode->SetThenPart(thenBlk);
    lnoIfstmtNode->SetElsePart(elseBlk);
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
    LfoBlockNode *branchBlock = codeMP->New<LfoBlockNode>(lnoIfstmtNode);
    LfoBlockNode *emptyBlock = codeMP->New<LfoBlockNode>(lnoIfstmtNode);
    if (condgoto->GetOp() == OP_brtrue) {
      lnoIfstmtNode->SetElsePart(branchBlock);
      lnoIfstmtNode->SetThenPart(emptyBlock);
    } else {
      lnoIfstmtNode->SetThenPart(branchBlock);
      lnoIfstmtNode->SetElsePart(emptyBlock);
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

uint32 LfoPreEmitter::EmitLfoBB(uint32 curj, LfoBlockNode *curblk) {
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
        lfoFunc->lfoCreatedLabelSet.erase(mebb->GetBBLabel());
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

AnalysisResult *DoLfoPreEmission::Run(MeFunction *func, MeFuncResultMgr *m, ModuleResultMgr *) {
  if (func->GetCfg()->NumBBs() == 0) {
    func->SetLfo(false);
    return nullptr;
  }
  MeIRMap *hmap = static_cast<MeIRMap *>(m->GetAnalysisResult(MeFuncPhase_IRMAPBUILD, func));
  ASSERT(hmap != nullptr, "irmapbuild has problem");
  MIRFunction *mirfunction = func->GetMirFunc();
  if (mirfunction->GetCodeMempool() != nullptr) {
    memPoolCtrler.DeleteMemPool(mirfunction->GetCodeMempool());
  }
  mirfunction->SetMemPool(new ThreadLocalMemPool(memPoolCtrler, "IR from lfopreemission::Emit()"));
#if 0
  mirfunction->body = mirfunction->codeMemPool->New<BlockNode>();
  for (uint32 i = 0; i < func->theCFG->bbVec.size(); i++) {
    BB *bb = func->theCFG->bbVec[i];
    if (bb == nullptr) {
      continue;
    }
    bb->EmitBB(func->meSSATab, func->mirFunc->body, func->mirFunc->freqMap, true);
  }
#else
  LfoPreEmitter emitter(hmap, func->GetLfoFunc());
  LfoBlockNode *curblk = mirfunction->GetCodeMempool()->New<LfoBlockNode>(nullptr);
  mirfunction->SetBody(curblk);
  uint32 i = 0;
  while (i < func->GetCfg()->GetAllBBs().size()) {
    i = emitter.EmitLfoBB(i, curblk);
  }
#endif

  m->InvalidAllResults();
  func->SetMeSSATab(nullptr);
  func->SetIRMap(nullptr);
  func->SetLfo(false);

  ConstantFold cf(func->GetMIRModule());
  cf.Simplify(mirfunction->GetBody());

  if (DEBUGFUNC(func)) {
    LogInfo::MapleLogger() << "\n**** After lfopreemit phase ****\n";
    mirfunction->Dump(false);
  }

#if 1  // use this only if directly feeding to mainopt
  MIRLower mirlowerer(func->GetMIRModule(), mirfunction);
  mirlowerer.SetLowerME();
  mirlowerer.SetLowerExpandArray();
  mirlowerer.LowerFunc(*mirfunction);
#endif

  return nullptr;
}
}  // namespace maple
