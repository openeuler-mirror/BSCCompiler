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

#include "mir_function.h"
#include "opcode_info.h"
#include "mir_pragma.h"
#include "mir_builder.h"
#include "bin_mplt.h"
#include <sstream>
#include <vector>

using namespace std;
namespace maple {
void BinaryMplExport::OutputInfoVector(const MIRInfoVector &infoVector, const MapleVector<bool> &infoVectorIsString) {
  WriteNum(infoVector.size());
  for (uint32 i = 0; i < infoVector.size(); i++) {
    OutputStr(infoVector[i].first);
    WriteNum(infoVectorIsString[i] ? 1 : 0);
    if (!infoVectorIsString[i]) {
      WriteNum(infoVector[i].second);
    } else {
      OutputStr(GStrIdx(infoVector[i].second));
    }
  }
}

void BinaryMplExport::OutputFuncIdInfo(MIRFunction *func) {
  WriteNum(kBinFuncIdInfoStart);
  WriteNum(func->GetPuidxOrigin());  // the funcid
  OutputInfoVector(func->GetInfoVector(), func->InfoIsString());
  if (mod.GetFlavor() == kFlavorLmbc) {
    WriteNum(func->GetUpFormalSize());
    WriteNum(func->GetFrameSize());
    WriteNum(func->GetOutParmSize());
  }
  WriteNum(~kBinFuncIdInfoStart);
}

void BinaryMplExport::OutputBaseNode(const BaseNode *b) {
  WriteNum(b->GetOpCode());
  WriteNum(b->GetPrimType());
}

void BinaryMplExport::OutputLocalSymbol(MIRSymbol *sym) {
  if (sym == nullptr) {
    WriteNum(0);
    return;
  }
  WriteNum(kBinSymbol);
  WriteNum(sym->GetStIndex());  // preserve original st index
  OutputStr(sym->GetNameStrIdx());
  WriteNum(sym->GetSKind());
  WriteNum(sym->GetStorageClass());
  OutputTypeAttrs(sym->GetAttrs());
  WriteNum(static_cast<int64>(sym->GetIsTmp()));
  if (sym->GetSKind() == kStVar || sym->GetSKind() == kStFunc) {
    OutputSrcPos(sym->GetSrcPosition());
  }
  OutputTypeViaTypeName(sym->GetTyIdx());
  if (sym->GetSKind() == kStPreg) {
    WriteNum(sym->GetPreg()->GetPregNo());
  } else if (sym->GetSKind() == kStConst || sym->GetSKind() == kStVar) {
    OutputConst(sym->GetKonst());
  } else if (sym->GetSKind() == kStFunc) {
    OutputFuncViaSymName(sym->GetFunction()->GetPuidx());
    OutputTypeViaTypeName(sym->GetTyIdx());
  } else {
    CHECK_FATAL(false, "should not used");
  }
}

void BinaryMplExport::OutputLocalSymTab(const MIRFunction *func) {
  WriteNum(kBinSymStart);
  uint64 outsymSizeIdx = buf.size();
  ExpandFourBuffSize();  /// size of OutSym
  int32 size = 0;

  for (uint32 i = 1; i < func->GetSymTab()->GetSymbolTableSize(); i++) {
    MIRSymbol *s = func->GetSymTab()->GetSymbolFromStIdx(i);
    if (s->IsDeleted()) {
      OutputLocalSymbol(nullptr);
    } else {
      OutputLocalSymbol(s);
    }
    size++;
  }

  Fixup(outsymSizeIdx, size);
  WriteNum(~kBinSymStart);
}

void BinaryMplExport::OutputPregTab(const MIRFunction *func) {
  WriteNum(kBinPregStart);
  uint64 outRegSizeIdx = buf.size();
  ExpandFourBuffSize();  /// size of OutReg
  int32 size = 0;

  for (uint32 i = 1; i < func->GetPregTab()->Size(); ++i) {
    MIRPreg *mirpreg = func->GetPregTab()->PregFromPregIdx(static_cast<int32>(i));
    if (mirpreg == nullptr) {
      WriteNum(0);
      size++;
      continue;
    }
    WriteNum(kBinPreg);
    WriteNum(mirpreg->GetPregNo());
    TyIdx tyIdx = (mirpreg->GetMIRType() == nullptr) ? TyIdx(0) : mirpreg->GetMIRType()->GetTypeIndex();
    OutputTypeViaTypeName(tyIdx);
    WriteNum(mirpreg->GetPrimType());
    size++;
  }

  Fixup(outRegSizeIdx, size);
  WriteNum(~kBinPregStart);
}

void BinaryMplExport::OutputLabelTab(const MIRFunction *func) {
  WriteNum(kBinLabelStart);
  WriteNum(static_cast<int64>(func->GetLabelTab()->Size()-1));  // entry 0 is skipped
  for (uint32 i = 1; i < func->GetLabelTab()->Size(); i++) {
    OutputStr(func->GetLabelTab()->GetLabelTable()[i]);
  }
  WriteNum(~kBinLabelStart);
}

void BinaryMplExport::OutputLocalTypeNameTab(const MIRTypeNameTable *typeNameTab) {
  WriteNum(kBinTypenameStart);
  WriteNum(static_cast<int64>(typeNameTab->Size()));
  for (std::pair<GStrIdx, TyIdx> it : typeNameTab->GetGStrIdxToTyIdxMap()) {
    OutputStr(it.first);
    OutputTypeViaTypeName(it.second);
  }
  WriteNum(~kBinTypenameStart);
}

void BinaryMplExport::OutputFormalsStIdx(MIRFunction *func) {
  WriteNum(kBinFormalStart);
  WriteNum(func->GetFormalDefVec().size());
  for (FormalDef formalDef : func->GetFormalDefVec()) {
    WriteNum(formalDef.formalSym->GetStIndex());
  }
  WriteNum(~kBinFormalStart);
}

void BinaryMplExport::OutputAliasMap(MapleMap<GStrIdx, MIRAliasVars> &aliasVarMap) {
  WriteNum(kBinAliasMapStart);
  WriteInt(static_cast<int32>(aliasVarMap.size()));
  for (std::pair<GStrIdx, MIRAliasVars> it : aliasVarMap) {
    OutputStr(it.first);
    OutputStr(it.second.memPoolStrIdx);
    OutputTypeViaTypeName(it.second.tyIdx);
    OutputStr(it.second.sigStrIdx);
  }
  WriteNum(~kBinAliasMapStart);
}

void BinaryMplExport::OutputFuncViaSymName(PUIdx puIdx) {
  MIRFunction *func = GlobalTables::GetFunctionTable().GetFunctionFromPuidx(puIdx);
  MIRSymbol *funcSt = GlobalTables::GetGsymTable().GetSymbolFromStidx(func->GetStIdx().Idx());
  WriteNum(kBinKindFuncViaSymname);
  OutputStr(funcSt->GetNameStrIdx());
}

void BinaryMplExport::OutputExpression(BaseNode *e) {
  WriteNum(kBinOpExpression);
  OutputBaseNode(e);
  switch (e->GetOpCode()) {
    // leaf
    case OP_constval: {
      MIRConst *constVal = static_cast<ConstvalNode *>(e)->GetConstVal();
      OutputConst(constVal);
      return;
    }
    case OP_conststr: {
      UStrIdx strIdx = static_cast<ConststrNode *>(e)->GetStrIdx();
      OutputUsrStr(strIdx);
      return;
    }
    case OP_addroflabel: {
      AddroflabelNode *lNode = static_cast<AddroflabelNode *>(e);
      WriteNum(lNode->GetOffset());
      return;
    }
    case OP_addroffunc: {
      AddroffuncNode *addrNode = static_cast<AddroffuncNode *>(e);
      OutputFuncViaSymName(addrNode->GetPUIdx());
      return;
    }
    case OP_sizeoftype: {
      SizeoftypeNode *sot = static_cast<SizeoftypeNode *>(e);
      OutputTypeViaTypeName(sot->GetTyIdx());
      return;
    }
    case OP_addrof:
    case OP_addrofoff:
    case OP_dread:
    case OP_dreadoff: {
      StIdx stIdx;
      if (e->GetOpCode() == OP_addrof || e->GetOpCode() == OP_dread) {
        AddrofNode *drNode = static_cast<AddrofNode *>(e);
        WriteNum(drNode->GetFieldID());
        stIdx = drNode->GetStIdx();
      } else {
        DreadoffNode *droff = static_cast<DreadoffNode *>(e);
        WriteNum(droff->offset);
        stIdx = droff->stIdx;
      }
      WriteNum(stIdx.Scope());
      if (stIdx.Islocal()) {
        WriteNum(stIdx.Idx());  // preserve original st index
      } else {
        MIRSymbol *sym = GlobalTables::GetGsymTable().GetSymbolFromStidx(stIdx.Idx());
        WriteNum(kBinKindSymViaSymname);
        OutputStr(sym->GetNameStrIdx());
      }
      return;
    }
    case OP_regread: {
      RegreadNode *regreadNode = static_cast<RegreadNode *>(e);
      WriteNum(regreadNode->GetRegIdx());
      return;
    }
    case OP_gcmalloc:
    case OP_gcpermalloc:
    case OP_stackmalloc: {
      GCMallocNode *gcNode = static_cast<GCMallocNode *>(e);
      OutputTypeViaTypeName(gcNode->GetTyIdx());
      return;
    }
    // unary
    case OP_ceil:
    case OP_cvt:
    case OP_floor:
    case OP_trunc: {
      TypeCvtNode *typecvtNode = static_cast<TypeCvtNode *>(e);
      WriteNum(typecvtNode->FromType());
      break;
    }
    case OP_retype: {
      RetypeNode *retypeNode = static_cast<RetypeNode *>(e);
      OutputTypeViaTypeName(retypeNode->GetTyIdx());
      break;
    }
    case OP_iread:
    case OP_iaddrof: {
      IreadNode *irNode = static_cast<IreadNode *>(e);
      OutputTypeViaTypeName(irNode->GetTyIdx());
      WriteNum(irNode->GetFieldID());
      break;
    }
    case OP_ireadoff: {
      IreadoffNode *irNode = static_cast<IreadoffNode *>(e);
      WriteNum(irNode->GetOffset());
      break;
    }
    case OP_ireadfpoff: {
      IreadFPoffNode *irNode = static_cast<IreadFPoffNode *>(e);
      WriteNum(irNode->GetOffset());
      break;
    }
    case OP_sext:
    case OP_zext:
    case OP_extractbits: {
      ExtractbitsNode *extNode = static_cast<ExtractbitsNode *>(e);
      WriteNum(extNode->GetBitsOffset());
      WriteNum(extNode->GetBitsSize());
      break;
    }
    case OP_depositbits: {
      DepositbitsNode *dbNode = static_cast<DepositbitsNode *>(e);
      WriteNum(dbNode->GetBitsOffset());
      WriteNum(dbNode->GetBitsSize());
      break;
    }
    case OP_gcmallocjarray:
    case OP_gcpermallocjarray: {
      JarrayMallocNode *gcNode = static_cast<JarrayMallocNode *>(e);
      OutputTypeViaTypeName(gcNode->GetTyIdx());
      break;
    }
    // binary
    case OP_sub:
    case OP_mul:
    case OP_div:
    case OP_rem:
    case OP_ashr:
    case OP_lshr:
    case OP_shl:
    case OP_max:
    case OP_min:
    case OP_band:
    case OP_bior:
    case OP_bxor:
    case OP_cand:
    case OP_cior:
    case OP_land:
    case OP_lior:
    case OP_add: {
      break;
    }
    case OP_eq:
    case OP_ne:
    case OP_lt:
    case OP_gt:
    case OP_le:
    case OP_ge:
    case OP_cmpg:
    case OP_cmpl:
    case OP_cmp: {
      CompareNode *cmpNode = static_cast<CompareNode *>(e);
      WriteNum(cmpNode->GetOpndType());
      break;
    }
    case OP_resolveinterfacefunc:
    case OP_resolvevirtualfunc: {
      ResolveFuncNode *rsNode = static_cast<ResolveFuncNode *>(e);
      OutputFuncViaSymName(rsNode->GetPuIdx());
      break;
    }
    // ternary
    case OP_select: {
      break;
    }
    // nary
    case OP_array: {
      ArrayNode *arrNode = static_cast<ArrayNode *>(e);
      OutputTypeViaTypeName(arrNode->GetTyIdx());
      WriteNum(static_cast<int64>(arrNode->GetBoundsCheck()));
      WriteNum(static_cast<int64>(arrNode->NumOpnds()));
      break;
    }
    case OP_intrinsicop: {
      IntrinsicopNode *intrnNode = static_cast<IntrinsicopNode *>(e);
      WriteNum(intrnNode->GetIntrinsic());
      WriteNum(static_cast<int64>(intrnNode->NumOpnds()));
      break;
    }
    case OP_intrinsicopwithtype: {
      IntrinsicopNode *intrnNode = static_cast<IntrinsicopNode *>(e);
      WriteNum(intrnNode->GetIntrinsic());
      OutputTypeViaTypeName(intrnNode->GetTyIdx());
      WriteNum(static_cast<int64>(intrnNode->NumOpnds()));
      break;
    }
    default:
      break;
  }
  for (uint32 i = 0; i < e->NumOpnds(); ++i) {
    OutputExpression(e->Opnd(i));
  }
}

static SrcPosition lastOutputSrcPosition;

void BinaryMplExport::OutputSrcPos(const SrcPosition &pos) {
  if (pos.FileNum() == 0 || pos.LineNum() == 0) {  // error case, so output 0
    WriteNum(lastOutputSrcPosition.RawData());
    WriteNum(lastOutputSrcPosition.LineNum());
    return;
  }
  WriteNum(pos.RawData());
  WriteNum(pos.LineNum());
  lastOutputSrcPosition = pos;
}

void BinaryMplExport::OutputReturnValues(const CallReturnVector *retv) {
  WriteNum(kBinReturnvals);
  WriteNum(static_cast<int64>(retv->size()));
  for (uint32 i = 0; i < retv->size(); i++) {
    WriteNum((*retv)[i].first.Idx());
    WriteNum((*retv)[i].second.GetFieldID());
    WriteNum((*retv)[i].second.GetPregIdx());
  }
}

void BinaryMplExport::OutputBlockNode(BlockNode *block) {
  WriteNum(kBinNodeBlock);
  if (!block->GetStmtNodes().empty()) {
    OutputSrcPos(block->GetSrcPos());
  } else {
    OutputSrcPos(SrcPosition());  // output 0
  }
  int32 num = 0;
  uint64 idx = buf.size();
  ExpandFourBuffSize();  // place holder, Fixup later
  for (StmtNode *s = block->GetFirst(); s; s = s->GetNext()) {
    bool doneWithOpnds = false;
    WriteNum(kBinOpStatement);
    OutputSrcPos(s->GetSrcPos());
    WriteNum(s->GetOpCode());
    switch (s->GetOpCode()) {
      case OP_dassign:
      case OP_dassignoff: {
        StIdx stIdx;
        if (s->GetOpCode() == OP_dassign) {
          DassignNode *dass = static_cast<DassignNode *>(s);
          WriteNum(dass->GetFieldID());
          stIdx = dass->GetStIdx();
        } else {
          DassignoffNode *dassoff = static_cast<DassignoffNode *>(s);
          WriteNum(dassoff->GetPrimType());
          WriteNum(dassoff->offset);
          stIdx = dassoff->stIdx;
        }
        WriteNum(stIdx.Scope());
        if (stIdx.Islocal()) {
          WriteNum(stIdx.Idx());  // preserve original st index
        } else {
          MIRSymbol *sym = GlobalTables::GetGsymTable().GetSymbolFromStidx(stIdx.Idx());
          WriteNum(kBinKindSymViaSymname);
          OutputStr(sym->GetNameStrIdx());
        }
        break;
      }
      case OP_regassign: {
        RegassignNode *rass = static_cast<RegassignNode *>(s);
        WriteNum(rass->GetPrimType());
        WriteNum(rass->GetRegIdx());
        break;
      }
      case OP_iassign: {
        IassignNode *iass = static_cast<IassignNode *>(s);
        OutputTypeViaTypeName(iass->GetTyIdx());
        WriteNum(iass->GetFieldID());
        break;
      }
      case OP_iassignoff: {
        IassignoffNode *iassoff = static_cast<IassignoffNode *>(s);
        WriteNum(iassoff->GetPrimType());
        WriteNum(iassoff->GetOffset());
        break;
      }
      case OP_iassignspoff:
      case OP_iassignfpoff: {
        IassignFPoffNode *iassfpoff = static_cast<IassignFPoffNode *>(s);
        WriteNum(iassfpoff->GetPrimType());
        WriteNum(iassfpoff->GetOffset());
        break;
      }
      case OP_blkassignoff: {
        BlkassignoffNode *bass = static_cast<BlkassignoffNode *>(s);
        int32 offsetAlign = (bass->offset << 4) | bass->alignLog2;
        WriteNum(offsetAlign);
        WriteNum(bass->blockSize);
        break;
      }
      case OP_call:
      case OP_virtualcall:
      case OP_virtualicall:
      case OP_superclasscall:
      case OP_interfacecall:
      case OP_interfaceicall:
      case OP_customcall:
      case OP_polymorphiccall: {
        CallNode *callnode = static_cast<CallNode *>(s);
        OutputFuncViaSymName(callnode->GetPUIdx());
        if (s->GetOpCode() == OP_polymorphiccall) {
          OutputTypeViaTypeName(static_cast<CallNode *>(callnode)->GetTyIdx());
        }
        WriteNum(static_cast<int64>(s->NumOpnds()));
        break;
      }
      case OP_callassigned:
      case OP_virtualcallassigned:
      case OP_virtualicallassigned:
      case OP_superclasscallassigned:
      case OP_interfacecallassigned:
      case OP_interfaceicallassigned:
      case OP_customcallassigned: {
        CallNode *callnode = static_cast<CallNode *>(s);
        OutputFuncViaSymName(callnode->GetPUIdx());
        OutputReturnValues(&callnode->GetReturnVec());
        WriteNum(static_cast<int64>(s->NumOpnds()));
        break;
      }
      case OP_polymorphiccallassigned: {
        CallNode *callnode = static_cast<CallNode *>(s);
        OutputFuncViaSymName(callnode->GetPUIdx());
        OutputTypeViaTypeName(callnode->GetTyIdx());
        OutputReturnValues(&callnode->GetReturnVec());
        WriteNum(static_cast<int64>(s->NumOpnds()));
        break;
      }
      case OP_icallproto:
      case OP_icall: {
        IcallNode *icallnode = static_cast<IcallNode *>(s);
        OutputTypeViaTypeName(icallnode->GetRetTyIdx());
        WriteNum(static_cast<int64>(s->NumOpnds()));
        break;
      }
      case OP_icallassigned: {
        IcallNode *icallnode = static_cast<IcallNode *>(s);
        OutputTypeViaTypeName(icallnode->GetRetTyIdx());
        OutputReturnValues(&icallnode->GetReturnVec());
        WriteNum(static_cast<int64>(s->NumOpnds()));
        break;
      }
      case OP_intrinsiccall:
      case OP_xintrinsiccall: {
        IntrinsiccallNode *intrnNode = static_cast<IntrinsiccallNode *>(s);
        WriteNum(intrnNode->GetIntrinsic());
        WriteNum(static_cast<int64>(s->NumOpnds()));
        break;
      }
      case OP_intrinsiccallassigned:
      case OP_xintrinsiccallassigned: {
        IntrinsiccallNode *intrnNode = static_cast<IntrinsiccallNode *>(s);
        WriteNum(intrnNode->GetIntrinsic());
        OutputReturnValues(&intrnNode->GetReturnVec());
        WriteNum(static_cast<int64>(s->NumOpnds()));
        break;
      }
      case OP_intrinsiccallwithtype: {
        IntrinsiccallNode *intrnNode = static_cast<IntrinsiccallNode *>(s);
        WriteNum(intrnNode->GetIntrinsic());
        OutputTypeViaTypeName(intrnNode->GetTyIdx());
        WriteNum(static_cast<int64>(s->NumOpnds()));
        break;
      }
      case OP_intrinsiccallwithtypeassigned: {
        IntrinsiccallNode *intrnNode = static_cast<IntrinsiccallNode *>(s);
        WriteNum(intrnNode->GetIntrinsic());
        OutputTypeViaTypeName(intrnNode->GetTyIdx());
        OutputReturnValues(&intrnNode->GetReturnVec());
        WriteNum(static_cast<int64>(s->NumOpnds()));
        break;
      }
      case OP_syncenter:
      case OP_syncexit:
      case OP_return: {
        WriteNum(static_cast<int64>(s->NumOpnds()));
        break;
      }
      case OP_jscatch:
      case OP_cppcatch:
      case OP_finally:
      case OP_endtry:
      case OP_cleanuptry:
      case OP_retsub:
      case OP_membaracquire:
      case OP_membarrelease:
      case OP_membarstorestore:
      case OP_membarstoreload: {
        break;
      }
      case OP_eval:
      case OP_throw:
      case OP_free:
      case OP_decref:
      case OP_incref:
      case OP_decrefreset:
      CASE_OP_ASSERT_NONNULL
      case OP_igoto: {
        break;
      }
      case OP_label: {
        LabelNode *lNode = static_cast<LabelNode *>(s);
        WriteNum(lNode->GetLabelIdx());
        break;
      }
      case OP_goto:
      case OP_gosub: {
        GotoNode *gtoNode = static_cast<GotoNode *>(s);
        WriteNum(gtoNode->GetOffset());
        break;
      }
      case OP_brfalse:
      case OP_brtrue: {
        CondGotoNode *cgotoNode = static_cast<CondGotoNode *>(s);
        WriteNum(cgotoNode->GetOffset());
        break;
      }
      case OP_switch: {
        SwitchNode *swNode = static_cast<SwitchNode *>(s);
        WriteNum(swNode->GetDefaultLabel());
        WriteNum(static_cast<int64>(swNode->GetSwitchTable().size()));
        for (CasePair cpair : swNode->GetSwitchTable()) {
          WriteNum(cpair.first);
          WriteNum(cpair.second);
        }
        break;
      }
      case OP_rangegoto: {
        RangeGotoNode *rgoto = static_cast<RangeGotoNode *>(s);
        WriteNum(rgoto->GetTagOffset());
        WriteNum(static_cast<int64>(rgoto->GetRangeGotoTable().size()));
        for (SmallCasePair cpair : rgoto->GetRangeGotoTable()) {
          WriteNum(cpair.first);
          WriteNum(cpair.second);
        }
        break;
      }
      case OP_jstry: {
        JsTryNode *tryNode = static_cast<JsTryNode *>(s);
        WriteNum(tryNode->GetCatchOffset());
        WriteNum(tryNode->GetFinallyOffset());
        break;
      }
      case OP_cpptry:
      case OP_try: {
        TryNode *tryNode = static_cast<TryNode *>(s);
        WriteNum(static_cast<int64>(tryNode->GetOffsetsCount()));
        for (LabelIdx lidx : tryNode->GetOffsets()) {
          WriteNum(lidx);
        }
        break;
      }
      case OP_catch: {
        CatchNode *catchNode = static_cast<CatchNode *>(s);
        WriteNum(static_cast<int64>(catchNode->GetExceptionTyIdxVec().size()));
        for (TyIdx tidx : catchNode->GetExceptionTyIdxVec()) {
          OutputTypeViaTypeName(tidx);
        }
        break;
      }
      case OP_comment: {
        string str(static_cast<CommentNode *>(s)->GetComment().c_str());
        WriteAsciiStr(str);
        break;
      }
      case OP_dowhile:
      case OP_while: {
        WhileStmtNode *whileNode = static_cast<WhileStmtNode *>(s);
        OutputBlockNode(whileNode->GetBody());
        OutputExpression(whileNode->Opnd());
        doneWithOpnds = true;
        break;
      }
      case OP_if: {
        IfStmtNode *ifNode = static_cast<IfStmtNode *>(s);
        bool hasElsePart = ifNode->GetElsePart() != nullptr;
        WriteNum(static_cast<int64>(hasElsePart));
        OutputBlockNode(ifNode->GetThenPart());
        if (hasElsePart) {
          OutputBlockNode(ifNode->GetElsePart());
        }
        OutputExpression(ifNode->Opnd());
        doneWithOpnds = true;
        break;
      }
      case OP_block: {
        BlockNode *blockNode = static_cast<BlockNode *>(s);
        OutputBlockNode(blockNode);
        doneWithOpnds = true;
        break;
      }
      case OP_asm: {
        AsmNode *asmNode = static_cast<AsmNode *>(s);
        WriteNum(asmNode->qualifiers);
        string str(asmNode->asmString.c_str());
        WriteAsciiStr(str);
        // the outputs
        size_t count = asmNode->asmOutputs.size();
        WriteNum(static_cast<int64>(count));
        for (size_t i = 0; i < count; ++i) {
          OutputUsrStr(asmNode->outputConstraints[i]);
        }
        OutputReturnValues(&asmNode->asmOutputs);
        // the clobber list
        count = asmNode->clobberList.size();
        WriteNum(static_cast<int64>(count));
        for (size_t i = 0; i < count; ++i) {
          OutputUsrStr(asmNode->clobberList[i]);
        }
        // the labels
        count = asmNode->gotoLabels.size();
        WriteNum(static_cast<int64>(count));
        for (size_t i = 0; i < count; ++i) {
          WriteNum(asmNode->gotoLabels[i]);
        }
        // the inputs
        WriteNum(asmNode->NumOpnds());
        for (uint8 i = 0; i < asmNode->numOpnds; ++i) {
          OutputUsrStr(asmNode->inputConstraints[i]);
        }
        break;
      }
      default:
        CHECK_FATAL(false, "Unhandled opcode %d", s->GetOpCode());
        break;
    }
    num++;
    if (!doneWithOpnds) {
      for (uint32 i = 0; i < s->NumOpnds(); ++i) {
        OutputExpression(s->Opnd(i));
      }
    }
  }
  Fixup(idx, num);
}

void BinaryMplExport::WriteFunctionBodyField(uint64 contentIdx, std::unordered_set<std::string> *dumpFuncSet) {
  Fixup(contentIdx, static_cast<int32>(buf.size()));
  // LogInfo::MapleLogger() << "Write FunctionBody Field " << std::endl;
  WriteNum(kBinFunctionBodyStart);
  uint64 totalSizeIdx = buf.size();
  ExpandFourBuffSize();  /// total size of this field to ~BIN_FUNCTIONBODY_START
  uint64 outFunctionBodySizeIdx = buf.size();
  ExpandFourBuffSize();  /// size of outFunctionBody
  int32 size = 0;

  if (not2mplt) {
    for (MIRFunction *func : GetMIRModule().GetFunctionList()) {
      if (func->GetAttr(FUNCATTR_optimized)) {
        continue;
      }
      if (func->GetCodeMemPool() == nullptr || func->GetBody() == nullptr) {
        continue;
      }
      if (dumpFuncSet != nullptr && !dumpFuncSet->empty()) {
        // output only if this func matches any name in *dumpFuncSet
        const std::string &name = func->GetName();
        bool matched = false;
        for (std::string elem : *dumpFuncSet) {
          if (name.find(elem.c_str()) != string::npos) {
            matched = true;
            break;
          }
        }
        if (!matched) {
          continue;
        }
      }
      OutputFunction(func->GetPuidx());
      CHECK_FATAL(func->GetBody() != nullptr, "WriteFunctionBodyField: no function body");
      OutputFuncIdInfo(func);
      OutputPregTab(func);
      OutputLocalSymTab(func);
      OutputLabelTab(func);
      OutputLocalTypeNameTab(func->GetTypeNameTab());
      OutputFormalsStIdx(func);
      if (mod.GetFlavor() < kMmpl) {
        OutputAliasMap(func->GetAliasVarMap());
      }
      lastOutputSrcPosition = SrcPosition();
      OutputBlockNode(func->GetBody());
      size++;
    }
  }

  Fixup(totalSizeIdx, static_cast<int32>(buf.size() - totalSizeIdx));
  Fixup(outFunctionBodySizeIdx, size);
  WriteNum(~kBinFunctionBodyStart);
  return;
}
}  // namespace maple
