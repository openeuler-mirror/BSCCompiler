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

#include "bin_mpl_export.h"
#include "bin_mpl_import.h"
#include "mir_function.h"
#include "opcode_info.h"
#include "mir_pragma.h"
#include "mir_builder.h"
#include <sstream>
#include <vector>
#include <unordered_set>
using namespace std;

namespace maple {
void BinaryMplImport::ImportInfoVector(MIRInfoVector &infoVector, MapleVector<bool> &infoVectorIsString) {
  int64 size = ReadNum();
  for (int64 i = 0; i < size; ++i) {
    GStrIdx gStrIdx = ImportStr();
    bool isstring = (ReadNum() != 0);
    infoVectorIsString.push_back(isstring);
    if (isstring) {
      GStrIdx fieldval = ImportStr();
      infoVector.emplace_back(MIRInfoPair(gStrIdx, fieldval.GetIdx()));
    } else {
      auto fieldval = static_cast<uint32>(ReadNum());
      infoVector.emplace_back(MIRInfoPair(gStrIdx, fieldval));
    }
  }
}

void BinaryMplImport::ImportFuncIdInfo(MIRFunction *func) {
  int64 tag = ReadNum();
  CHECK_FATAL(tag == kBinFuncIdInfoStart, "kBinFuncIdInfoStart expected");
  func->SetPuidxOrigin(static_cast<PUIdx>(ReadNum()));
  ImportInfoVector(func->GetInfoVector(), func->InfoIsString());
  if (mod.GetFlavor() == kFlavorLmbc) {
    func->SetUpFormalSize(ReadNum());
    func->SetFrameSize(ReadNum());
    func->SetOutParmSize(ReadNum());
  }
  tag = ReadNum();
  CHECK_FATAL(tag == ~kBinFuncIdInfoStart, "pattern mismatch in ImportFuncIdInfo()");
}

void BinaryMplImport::ImportBaseNode(Opcode &o, PrimType &typ) {
  o = (Opcode)ReadNum();
  typ = (PrimType)ReadNum();
}

void BinaryMplImport::ImportLocalSymbol(MIRFunction *func) {
  int64 tag = ReadNum();
  if (tag == 0) {
    func->GetSymTab()->PushNullSymbol();
    return;
  }
  CHECK_FATAL(tag == kBinSymbol, "expecting kBinSymbol in ImportLocalSymbol()");
  auto indx = static_cast<uint32>(ReadNum());
  CHECK_FATAL(indx == func->GetSymTab()->GetSymbolTableSize(), "inconsistant local stIdx");
  MIRSymbol *sym = func->GetSymTab()->CreateSymbol(kScopeLocal);
  sym->SetNameStrIdx(ImportStr());
  (void)func->GetSymTab()->AddToStringSymbolMap(*sym);
  sym->SetSKind((MIRSymKind)ReadNum());
  sym->SetStorageClass((MIRStorageClass)ReadNum());
  sym->SetAttrs(ImportTypeAttrs());
  sym->SetIsTmp(ReadNum() != 0);
  if (sym->GetSKind() == kStVar || sym->GetSKind() == kStFunc) {
    ImportSrcPos(sym->GetSrcPosition());
  }
  sym->SetTyIdx(ImportType());
  if (sym->GetSKind() == kStPreg) {
    auto thepregno = static_cast<uint32>(ReadNum());
    MIRType *mirType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(sym->GetTyIdx());
    PregIdx pregidx = func->GetPregTab()->EnterPregNo(thepregno, mirType->GetPrimType(), mirType);
    MIRPregTable *pregTab = func->GetPregTab();
    MIRPreg *preg = pregTab->PregFromPregIdx(pregidx);
    preg->SetPrimType(mirType->GetPrimType());
    sym->SetPreg(preg);
  } else if (sym->GetSKind() == kStConst || sym->GetSKind() == kStVar) {
    sym->SetKonst(ImportConst(func));
  } else if (sym->GetSKind() == kStFunc) {
    PUIdx puIdx = ImportFuncViaSymName();
    TyIdx tyIdx = ImportType();
    sym->SetTyIdx(tyIdx);
    sym->SetFunction(GlobalTables::GetFunctionTable().GetFunctionFromPuidx(puIdx));
  }
}

void BinaryMplImport::ImportLocalSymTab(MIRFunction *func) {
  int64 tag = ReadNum();
  CHECK_FATAL(tag == kBinSymStart, "kBinSymStart expected in ImportLocalSymTab()");
  int32 size = ReadInt();
  for (int64 i = 0; i < size; ++i) {
    ImportLocalSymbol(func);
  }
  tag = ReadNum();
  CHECK_FATAL(tag == ~kBinSymStart, "pattern mismatch in ImportLocalSymTab()");
}

void BinaryMplImport::ImportPregTab(const MIRFunction *func) {
  int64 tag = ReadNum();
  CHECK_FATAL(tag == kBinPregStart, "kBinPregStart expected in ImportPregTab()");
  int32 size = ReadInt();
  for (int64 i = 0; i < size; ++i) {
    int64 nextTag = ReadNum();
    if (nextTag == 0) {
      func->GetPregTab()->GetPregTable().push_back(nullptr);
      continue;
    }
    CHECK_FATAL(nextTag == kBinPreg, "expecting kBinPreg in ImportPregTab()");
    auto pregNo = static_cast<uint32>(ReadNum());
    TyIdx tyIdx = ImportType();
    MIRType *ty = (tyIdx == 0) ? nullptr : GlobalTables::GetTypeTable().GetTypeFromTyIdx(tyIdx);
    PrimType primType = (PrimType)ReadNum();
    CHECK_FATAL(ty == nullptr || primType == ty->GetPrimType(), "ImportPregTab: inconsistent primitive type");
    (void)func->GetPregTab()->EnterPregNo(pregNo, primType, ty);
  }
  tag = ReadNum();
  CHECK_FATAL(tag == ~kBinPregStart, "pattern mismatch in ImportPregTab()");
}

void BinaryMplImport::ImportLabelTab(MIRFunction *func) {
  int64 tag = ReadNum();
  CHECK_FATAL(tag == kBinLabelStart, "kBinLabelStart expected in ImportLabelTab()");
  int64 size = ReadNum();
  for (int64 i = 0; i < size; ++i) {
    GStrIdx gStrIdx = ImportStr();
    (void)func->GetLabelTab()->AddLabel(gStrIdx);
  }
  tag = ReadNum();
  CHECK_FATAL(tag == ~kBinLabelStart, "pattern mismatch in ImportLabelTab()");
}

void BinaryMplImport::ImportLocalTypeNameTable(MIRTypeNameTable *typeNameTab) {
  int64 tag = ReadNum();
  CHECK_FATAL(tag == kBinTypenameStart, "kBinTypenameStart expected in ImportLocalTypeNameTable()");
  int64 size = ReadNum();
  for (int64 i = 0; i < size; ++i) {
    GStrIdx strIdx = ImportStr();
    TyIdx tyIdx = ImportType();
    typeNameTab->SetGStrIdxToTyIdx(strIdx, tyIdx);
  }
  tag = ReadNum();
  CHECK_FATAL(tag == ~kBinTypenameStart, "pattern mismatch in ImportTypenametab()");
}

void BinaryMplImport::ImportFormalsStIdx(MIRFunction *func) {
  auto tag = ReadNum();
  CHECK_FATAL(tag == kBinFormalStart, "kBinFormalStart expected in ImportFormalsStIdx()");
  auto size = ReadNum();
  for (int64 i = 0; i < size; ++i) {
    uint32 indx = static_cast<uint32>(ReadNum());
    func->GetFormalDefVec()[static_cast<uint64>(i)].formalSym = func->GetSymTab()->GetSymbolFromStIdx(indx);
  }
  tag = ReadNum();
  CHECK_FATAL(tag == ~kBinFormalStart, "pattern mismatch in ImportFormalsStIdx()");
}

void BinaryMplImport::ImportAliasMap(MIRFunction *func) {
  int64 tag = ReadNum();
  CHECK_FATAL(tag == kBinAliasMapStart, "kBinAliasMapStart expected in ImportAliasMap()");
  int32 size = ReadInt();
  for (int32 i = 0; i < size; ++i) {
    MIRAliasVars aliasvars;
    GStrIdx strIdx = ImportStr();
    aliasvars.memPoolStrIdx = ImportStr();
    aliasvars.tyIdx = ImportType();
    (void)ImportStr();  // not assigning to mimic parser
    func->GetAliasVarMap()[strIdx] = aliasvars;
  }
  tag = ReadNum();
  CHECK_FATAL(tag == ~kBinAliasMapStart, "pattern mismatch in ImportAliasMap()");
}

PUIdx BinaryMplImport::ImportFuncViaSymName() {
  int64 tag = ReadNum();
  CHECK_FATAL(tag == kBinKindFuncViaSymname, "kBinKindFuncViaSymname expected");
  GStrIdx strIdx = ImportStr();
  MIRSymbol *sym = GlobalTables::GetGsymTable().GetSymbolFromStrIdx(strIdx);
  MIRFunction *func = sym->GetFunction();
  return func->GetPuidx();
}

BaseNode *BinaryMplImport::ImportExpression(MIRFunction *func) {
  int64 tag = ReadNum();
  CHECK_FATAL(tag == kBinOpExpression, "kBinOpExpression expected");
  Opcode op;
  PrimType typ;
  ImportBaseNode(op, typ);
  switch (op) {
    // leaf
    case OP_constval: {
      MIRConst *constv = ImportConst(func);
      ConstvalNode *constNode = mod.CurFuncCodeMemPool()->New<ConstvalNode>(constv);
      constNode->SetPrimType(typ);
      return constNode;
    }
    case OP_conststr: {
      UStrIdx strIdx = ImportUsrStr();
      ConststrNode *constNode = mod.CurFuncCodeMemPool()->New<ConststrNode>(typ, strIdx);
      constNode->SetPrimType(typ);
      return constNode;
    }
    case OP_addroflabel: {
      AddroflabelNode *alabNode = mod.CurFuncCodeMemPool()->New<AddroflabelNode>();
      alabNode->SetOffset(static_cast<uint32>(ReadNum()));
      alabNode->SetPrimType(typ);
      (void)func->GetLabelTab()->addrTakenLabels.insert(alabNode->GetOffset());
      return alabNode;
    }
    case OP_addroffunc: {
      PUIdx puIdx = ImportFuncViaSymName();
      AddroffuncNode *addrNode = mod.CurFuncCodeMemPool()->New<AddroffuncNode>(typ, puIdx);
      return addrNode;
    }
    case OP_sizeoftype: {
      TyIdx tidx = ImportType();
      SizeoftypeNode *sot = mod.CurFuncCodeMemPool()->New<SizeoftypeNode>(tidx);
      return sot;
    }
    case OP_addrof:
    case OP_addrofoff:
    case OP_dread:
    case OP_dreadoff: {
      int32 num = static_cast<int32>(ReadNum());
      StIdx stIdx;
      stIdx.SetScope(static_cast<uint32>(ReadNum()));
      if (stIdx.Islocal()) {
        stIdx.SetIdx(static_cast<uint32>(ReadNum()));
      } else {
        int32 stag = static_cast<int32>(ReadNum());
        CHECK_FATAL(stag == kBinKindSymViaSymname, "kBinKindSymViaSymname expected");
        GStrIdx strIdx = ImportStr();
        MIRSymbol *sym = GlobalTables::GetGsymTable().GetSymbolFromStrIdx(strIdx);
        if (op == OP_addrof) {
          sym->SetHasPotentialAssignment();
        }
        stIdx.SetIdx(sym->GetStIdx().Idx());
      }
      if (op == OP_addrof || op == OP_dread) {
        AddrofNode *drNode = mod.CurFuncCodeMemPool()->New<AddrofNode>(op);
        drNode->SetPrimType(typ);
        drNode->SetStIdx(stIdx);
        drNode->SetFieldID(num);
        return drNode;
      } else {
        DreadoffNode *dreadoff = mod.CurFuncCodeMemPool()->New<DreadoffNode>(op);
        dreadoff->SetPrimType(typ);
        dreadoff->stIdx = stIdx;
        dreadoff->offset = num;
        return dreadoff;
      }
    }
    case OP_regread: {
      RegreadNode *regreadNode = mod.CurFuncCodeMemPool()->New<RegreadNode>();
      regreadNode->SetRegIdx(static_cast<PregIdx>(ReadNum()));
      regreadNode->SetPrimType(typ);
      return regreadNode;
    }
    case OP_gcmalloc:
    case OP_gcpermalloc:
    case OP_stackmalloc: {
      TyIdx tyIdx = ImportType();
      GCMallocNode *gcNode = mod.CurFuncCodeMemPool()->New<GCMallocNode>(op, typ, tyIdx);
      return gcNode;
    }
    // unary
    case OP_abs:
    case OP_bnot:
    case OP_lnot:
    case OP_neg:
    case OP_recip:
    case OP_sqrt:
    case OP_alloca:
    case OP_malloc: {
      UnaryNode *unNode = mod.CurFuncCodeMemPool()->New<UnaryNode>(op, typ);
      unNode->SetOpnd(ImportExpression(func), 0);
      return unNode;
    }
    case OP_ceil:
    case OP_cvt:
    case OP_floor:
    case OP_trunc: {
      TypeCvtNode *typecvtNode = mod.CurFuncCodeMemPool()->New<TypeCvtNode>(op, typ);
      typecvtNode->SetFromType((PrimType)ReadNum());
      typecvtNode->SetOpnd(ImportExpression(func), 0);
      return typecvtNode;
    }
    case OP_retype: {
      RetypeNode *retypeNode = mod.CurFuncCodeMemPool()->New<RetypeNode>(typ);
      retypeNode->SetTyIdx(ImportType());
      retypeNode->SetOpnd(ImportExpression(func), 0);
      return retypeNode;
    }
    case OP_iread:
    case OP_iaddrof: {
      IreadNode *irNode = mod.CurFuncCodeMemPool()->New<IreadNode>(op, typ);
      irNode->SetTyIdx(ImportType());
      irNode->SetFieldID(static_cast<FieldID>(ReadNum()));
      irNode->SetOpnd(ImportExpression(func), 0);
      return irNode;
    }
    case OP_ireadoff: {
      int32 ofst = ReadNum();
      IreadoffNode *irNode = mod.CurFuncCodeMemPool()->New<IreadoffNode>(typ, ofst);
      irNode->SetOpnd(ImportExpression(func), 0);
      return irNode;
    }
    case OP_ireadfpoff: {
      int32 ofst = ReadNum();
      IreadFPoffNode *irNode = mod.CurFuncCodeMemPool()->New<IreadFPoffNode>(typ, ofst);
      return irNode;
    }
    case OP_sext:
    case OP_zext:
    case OP_extractbits: {
      ExtractbitsNode *extNode = mod.CurFuncCodeMemPool()->New<ExtractbitsNode>(op, typ);
      extNode->SetBitsOffset(static_cast<uint8>(ReadNum()));
      extNode->SetBitsSize(static_cast<uint8>(ReadNum()));
      extNode->SetOpnd(ImportExpression(func), 0);
      return extNode;
    }
    case OP_depositbits: {
      DepositbitsNode *dbNode = mod.CurFuncCodeMemPool()->New<DepositbitsNode>(op, typ);
      dbNode->SetBitsOffset(static_cast<uint8>(ReadNum()));
      dbNode->SetBitsSize(static_cast<uint8>(ReadNum()));
      dbNode->SetOpnd(ImportExpression(func), 0);
      dbNode->SetOpnd(ImportExpression(func), 1);
      return dbNode;
    }
    case OP_gcmallocjarray:
    case OP_gcpermallocjarray: {
      JarrayMallocNode *gcNode = mod.CurFuncCodeMemPool()->New<JarrayMallocNode>(op, typ);
      gcNode->SetTyIdx(ImportType());
      gcNode->SetOpnd(ImportExpression(func), 0);
      return gcNode;
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
      BinaryNode *binNode = mod.CurFuncCodeMemPool()->New<BinaryNode>(op, typ);
      binNode->SetOpnd(ImportExpression(func), 0);
      binNode->SetOpnd(ImportExpression(func), 1);
      return binNode;
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
      CompareNode *cmpNode = mod.CurFuncCodeMemPool()->New<CompareNode>(op, typ);
      cmpNode->SetOpndType((PrimType)ReadNum());
      cmpNode->SetOpnd(ImportExpression(func), 0);
      cmpNode->SetOpnd(ImportExpression(func), 1);
      return cmpNode;
    }
    case OP_resolveinterfacefunc:
    case OP_resolvevirtualfunc: {
      ResolveFuncNode *rsNode = mod.CurFuncCodeMemPool()->New<ResolveFuncNode>(op, typ);
      rsNode->SetPUIdx(ImportFuncViaSymName());
      rsNode->SetOpnd(ImportExpression(func), 0);
      rsNode->SetOpnd(ImportExpression(func), 1);
      return rsNode;
    }
    // ternary
    case OP_select: {
      TernaryNode *tNode = mod.CurFuncCodeMemPool()->New<TernaryNode>(op, typ);
      tNode->SetOpnd(ImportExpression(func), 0);
      tNode->SetOpnd(ImportExpression(func), 1);
      tNode->SetOpnd(ImportExpression(func), 2);
      return tNode;
    }
    // nary
    case OP_array: {
      TyIdx tidx = ImportType();
      auto boundsCheck = static_cast<uint32>(ReadNum());
      ArrayNode *arrNode =
          mod.CurFuncCodeMemPool()->New<ArrayNode>(func->GetCodeMPAllocator(), typ, tidx, boundsCheck);
      auto n = static_cast<uint32>(ReadNum());
      for (uint32 i = 0; i < n; ++i) {
        arrNode->GetNopnd().push_back(ImportExpression(func));
      }
      arrNode->SetNumOpnds(static_cast<uint8>(arrNode->GetNopnd().size()));
      return arrNode;
    }
    case OP_intrinsicop: {
      IntrinsicopNode *intrnNode = mod.CurFuncCodeMemPool()->New<IntrinsicopNode>(func->GetCodeMPAllocator(), op, typ);
      intrnNode->SetIntrinsic((MIRIntrinsicID)ReadNum());
      auto n = static_cast<uint32>(ReadNum());
      for (uint32 i = 0; i < n; ++i) {
        intrnNode->GetNopnd().push_back(ImportExpression(func));
      }
      intrnNode->SetNumOpnds(static_cast<uint8>(intrnNode->GetNopnd().size()));
      return intrnNode;
    }
    case OP_intrinsicopwithtype: {
      IntrinsicopNode *intrnNode =
          mod.CurFuncCodeMemPool()->New<IntrinsicopNode>(func->GetCodeMPAllocator(), OP_intrinsicopwithtype, typ);
      intrnNode->SetIntrinsic((MIRIntrinsicID)ReadNum());
      intrnNode->SetTyIdx(ImportType());
      auto n = static_cast<uint32>(ReadNum());
      for (uint32 i = 0; i < n; ++i) {
        intrnNode->GetNopnd().push_back(ImportExpression(func));
      }
      intrnNode->SetNumOpnds(static_cast<uint8>(intrnNode->GetNopnd().size()));
      return intrnNode;
    }
    default:
      CHECK_FATAL(false, "Unhandled op %d", tag);
      break;
  }
}

void BinaryMplImport::ImportSrcPos(SrcPosition &pos) {
  pos.SetRawData(static_cast<uint32>(ReadNum()));
  pos.SetLineNum(static_cast<uint32>(ReadNum()));
}

void BinaryMplImport::ImportReturnValues(MIRFunction *func, CallReturnVector *retv) {
  int64 tag = ReadNum();
  CHECK_FATAL(tag == kBinReturnvals, "expecting return values");
  auto size = static_cast<uint32>(ReadNum());
  for (uint32 i = 0; i < size; ++i) {
    uint32 idx = static_cast<uint32>(ReadNum());
    FieldID fid = static_cast<FieldID>(ReadNum());
    PregIdx ridx = static_cast<PregIdx>(ReadNum());
    retv->push_back(std::make_pair(StIdx(kScopeLocal, idx), RegFieldPair(fid, ridx)));
    if (idx == 0) {
      continue;
    }
    MIRSymbol *lsym = func->GetSymTab()->GetSymbolFromStIdx(idx, false);
    if (lsym->GetName().find("L_STR") == 0) {
      MIRType *ty = GlobalTables::GetTypeTable().GetTypeFromTyIdx(lsym->GetTyIdx());
      CHECK_FATAL(ty->GetKind() == kTypePointer, "Pointer type expected for L_STR prefix");
      MIRPtrType tempType(static_cast<MIRPtrType *>(ty)->GetPointedTyIdx(), PTY_ptr);
      TyIdx newTyidx = GlobalTables::GetTypeTable().GetOrCreateMIRType(&tempType);
      lsym->SetTyIdx(newTyidx);
    }
  }
}

BlockNode *BinaryMplImport::ImportBlockNode(MIRFunction *func) {
  int64 tag = ReadNum();
  ASSERT(tag == kBinNodeBlock, "expecting a BlockNode");

  BlockNode *block = func->GetCodeMemPool()->New<BlockNode>();
  Opcode op;
  uint8 numOpr;
  ImportSrcPos(block->GetSrcPos());
  int32 size = ReadInt();
  for (int32 k = 0; k < size; ++k) {
    tag = ReadNum();
    CHECK_FATAL(tag == kBinOpStatement, "kBinOpStatement expected");
    SrcPosition thesrcPosition;
    ImportSrcPos(thesrcPosition);
    op = (Opcode)ReadNum();
    StmtNode *stmt = nullptr;
    switch (op) {
      case OP_dassign:
      case OP_dassignoff: {
        PrimType primType = PTY_void;
        if (op == OP_dassignoff) {
          primType = (PrimType)ReadNum();
        }
        int32 num = static_cast<int32>(ReadNum());
        StIdx stIdx;
        stIdx.SetScope(static_cast<uint32>(ReadNum()));
        if (stIdx.Islocal()) {
          stIdx.SetIdx(static_cast<uint32>(ReadNum()));
        } else {
          int32 stag = static_cast<int32>(ReadNum());
          CHECK_FATAL(stag == kBinKindSymViaSymname, "kBinKindSymViaSymname expected");
          GStrIdx strIdx = ImportStr();
          MIRSymbol *sym = GlobalTables::GetGsymTable().GetSymbolFromStrIdx(strIdx);
          sym->SetHasPotentialAssignment();
          stIdx.SetIdx(sym->GetStIdx().Idx());
        }
        if (op == OP_dassign) {
          DassignNode *s = func->GetCodeMemPool()->New<DassignNode>();
          s->SetStIdx(stIdx);
          s->SetFieldID(num);
          s->SetOpnd(ImportExpression(func), 0);
          stmt = s;
        } else {
          DassignoffNode *s = func->GetCodeMemPool()->New<DassignoffNode>();
          s->SetPrimType(primType);
          s->stIdx = stIdx;
          s->offset = num;
          s->SetOpnd(ImportExpression(func), 0);
          stmt = s;
        }
        break;
      }
      case OP_regassign: {
        RegassignNode *s = func->GetCodeMemPool()->New<RegassignNode>();
        s->SetPrimType((PrimType)ReadNum());
        s->SetRegIdx(static_cast<PregIdx>(ReadNum()));
        s->SetOpnd(ImportExpression(func), 0);
        stmt = s;
        break;
      }
      case OP_iassign: {
        IassignNode *s = func->GetCodeMemPool()->New<IassignNode>();
        s->SetTyIdx(ImportType());
        s->SetFieldID(static_cast<FieldID>(ReadNum()));
        s->SetAddrExpr(ImportExpression(func));
        s->SetRHS(ImportExpression(func));
        stmt = s;
        break;
      }
      case OP_iassignoff: {
        IassignoffNode *s = func->GetCodeMemPool()->New<IassignoffNode>();
        s->SetPrimType((PrimType)ReadNum());
        s->SetOffset(static_cast<int32>(ReadNum()));
        s->SetOpnd(ImportExpression(func), 0);
        s->SetOpnd(ImportExpression(func), 1);
        stmt = s;
        break;
      }
      case OP_iassignspoff:
      case OP_iassignfpoff: {
        IassignFPoffNode *s = func->GetCodeMemPool()->New<IassignFPoffNode>(op);
        s->SetPrimType((PrimType)ReadNum());
        s->SetOffset(static_cast<int32>(ReadNum()));
        s->SetOpnd(ImportExpression(func), 0);
        stmt = s;
        break;
      }
      case OP_blkassignoff: {
        BlkassignoffNode *s = func->GetCodeMemPool()->New<BlkassignoffNode>();
        s->offset = static_cast<int32>(ReadNum());
        s->blockSize = static_cast<int32>(ReadNum());
        s->SetOpnd(ImportExpression(func), 0);
        s->SetOpnd(ImportExpression(func), 1);
        stmt = s;
        break;
      }
      case OP_call:
      case OP_virtualcall:
      case OP_virtualicall:
      case OP_superclasscall:
      case OP_interfacecall:
      case OP_interfaceicall:
      case OP_customcall: {
        CallNode *s = func->GetCodeMemPool()->New<CallNode>(mod, op);
        s->SetPUIdx(ImportFuncViaSymName());
        numOpr = static_cast<uint8>(ReadNum());
        s->SetNumOpnds(numOpr);
        for (int32 i = 0; i < numOpr; ++i) {
          s->GetNopnd().push_back(ImportExpression(func));
        }
        stmt = s;
        break;
      }
      case OP_callassigned:
      case OP_virtualcallassigned:
      case OP_virtualicallassigned:
      case OP_superclasscallassigned:
      case OP_interfacecallassigned:
      case OP_interfaceicallassigned:
      case OP_customcallassigned: {
        CallNode *s = func->GetCodeMemPool()->New<CallNode>(mod, op);
        s->SetPUIdx(ImportFuncViaSymName());
        ImportReturnValues(func, &s->GetReturnVec());
        numOpr = static_cast<uint8>(ReadNum());
        s->SetNumOpnds(numOpr);
        const auto &calleeName = GlobalTables::GetFunctionTable().GetFunctionFromPuidx(s->GetPUIdx())->GetName();
        if (calleeName == "setjmp") {
          func->SetHasSetjmp();
        }
        for (int32 i = 0; i < numOpr; ++i) {
          s->GetNopnd().push_back(ImportExpression(func));
        }
        stmt = s;
        break;
      }
      case OP_polymorphiccall: {
        CallNode *s = func->GetCodeMemPool()->New<CallNode>(mod, op);
        s->SetPUIdx(ImportFuncViaSymName());
        s->SetTyIdx(ImportType());
        numOpr = static_cast<uint8>(ReadNum());
        s->SetNumOpnds(numOpr);
        for (int32 i = 0; i < numOpr; ++i) {
          s->GetNopnd().push_back(ImportExpression(func));
        }
        stmt = s;
        break;
      }
      case OP_polymorphiccallassigned: {
        CallNode *s = func->GetCodeMemPool()->New<CallNode>(mod, op);
        s->SetPUIdx(ImportFuncViaSymName());
        s->SetTyIdx(ImportType());
        ImportReturnValues(func, &s->GetReturnVec());
        numOpr = static_cast<uint8>(ReadNum());
        s->SetNumOpnds(numOpr);
        for (int32 i = 0; i < numOpr; ++i) {
          s->GetNopnd().push_back(ImportExpression(func));
        }
        stmt = s;
        break;
      }
      case OP_icall: {
        IcallNode *s = func->GetCodeMemPool()->New<IcallNode>(mod, op);
        s->SetRetTyIdx(ImportType());
        numOpr = static_cast<uint8>(ReadNum());
        s->SetNumOpnds(numOpr);
        for (int32 i = 0; i < numOpr; ++i) {
          s->GetNopnd().push_back(ImportExpression(func));
        }
        stmt = s;
        break;
      }
      case OP_icallassigned: {
        IcallNode *s = func->GetCodeMemPool()->New<IcallNode>(mod, op);
        s->SetRetTyIdx(ImportType());
        ImportReturnValues(func, &s->GetReturnVec());
        numOpr = static_cast<uint8>(ReadNum());
        s->SetNumOpnds(numOpr);
        for (int32 i = 0; i < numOpr; ++i) {
          s->GetNopnd().push_back(ImportExpression(func));
        }
        stmt = s;
        break;
      }
      case OP_intrinsiccall:
      case OP_xintrinsiccall: {
        IntrinsiccallNode *s = func->GetCodeMemPool()->New<IntrinsiccallNode>(mod, op);
        s->SetIntrinsic((MIRIntrinsicID)ReadNum());
        numOpr = static_cast<uint8>(ReadNum());
        s->SetNumOpnds(numOpr);
        for (int32 i = 0; i < numOpr; ++i) {
          s->GetNopnd().push_back(ImportExpression(func));
        }
        stmt = s;
        break;
      }
      case OP_intrinsiccallassigned:
      case OP_xintrinsiccallassigned: {
        IntrinsiccallNode *s = func->GetCodeMemPool()->New<IntrinsiccallNode>(mod, op);
        s->SetIntrinsic((MIRIntrinsicID)ReadNum());
        ImportReturnValues(func, &s->GetReturnVec());
        numOpr = static_cast<uint8>(ReadNum());
        s->SetNumOpnds(numOpr);
        for (int32 i = 0; i < numOpr; ++i) {
          s->GetNopnd().push_back(ImportExpression(func));
        }
        if (s->GetReturnVec().size() == 1 && s->GetReturnVec()[0].first.Idx() != 0) {
          MIRSymbol *retsymbol = func->GetSymTab()->GetSymbolFromStIdx(s->GetReturnVec()[0].first.Idx());
          MIRType *rettype = GlobalTables::GetTypeTable().GetTypeFromTyIdx(retsymbol->GetTyIdx());
          CHECK_FATAL(rettype != nullptr, "rettype is null in MIRParser::ParseStmtIntrinsiccallAssigned");
          s->SetPrimType(rettype->GetPrimType());
        }
        stmt = s;
        break;
      }
      case OP_intrinsiccallwithtype: {
        IntrinsiccallNode *s = func->GetCodeMemPool()->New<IntrinsiccallNode>(mod, op);
        s->SetIntrinsic((MIRIntrinsicID)ReadNum());
        s->SetTyIdx(ImportType());
        numOpr = static_cast<uint8>(ReadNum());
        s->SetNumOpnds(numOpr);
        for (int32 i = 0; i < numOpr; ++i) {
          s->GetNopnd().push_back(ImportExpression(func));
        }
        stmt = s;
        break;
      }
      case OP_intrinsiccallwithtypeassigned: {
        IntrinsiccallNode *s = func->GetCodeMemPool()->New<IntrinsiccallNode>(mod, op);
        s->SetIntrinsic((MIRIntrinsicID)ReadNum());
        s->SetTyIdx(ImportType());
        ImportReturnValues(func, &s->GetReturnVec());
        numOpr = static_cast<uint8>(ReadNum());
        s->SetNumOpnds(numOpr);
        for (int32 i = 0; i < numOpr; ++i) {
          s->GetNopnd().push_back(ImportExpression(func));
        }
        if (s->GetReturnVec().size() == 1 && s->GetReturnVec()[0].first.Idx() != 0) {
          MIRSymbol *retsymbol = func->GetSymTab()->GetSymbolFromStIdx(s->GetReturnVec()[0].first.Idx());
          MIRType *rettype = GlobalTables::GetTypeTable().GetTypeFromTyIdx(retsymbol->GetTyIdx());
          CHECK_FATAL(rettype != nullptr, "rettype is null in MIRParser::ParseStmtIntrinsiccallAssigned");
          s->SetPrimType(rettype->GetPrimType());
        }
        stmt = s;
        break;
      }
      case OP_syncenter:
      case OP_syncexit:
      case OP_return: {
        NaryStmtNode *s = func->GetCodeMemPool()->New<NaryStmtNode>(mod, op);
        numOpr = static_cast<uint8>(ReadNum());
        s->SetNumOpnds(numOpr);
        for (int32 i = 0; i < numOpr; ++i) {
          s->GetNopnd().push_back(ImportExpression(func));
        }
        stmt = s;
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
        stmt = mod.CurFuncCodeMemPool()->New<StmtNode>(op);
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
        UnaryStmtNode *s = mod.CurFuncCodeMemPool()->New<UnaryStmtNode>(op);
        s->SetOpnd(ImportExpression(func), 0);
        stmt = s;
        break;
      }
      case OP_label: {
        LabelNode *s = mod.CurFuncCodeMemPool()->New<LabelNode>();
        s->SetLabelIdx(static_cast<LabelIdx>(ReadNum()));
        stmt = s;
        break;
      }
      case OP_goto:
      case OP_gosub: {
        GotoNode *s = mod.CurFuncCodeMemPool()->New<GotoNode>(op);
        s->SetOffset(static_cast<uint32>(ReadNum()));
        stmt = s;
        break;
      }
      case OP_brfalse:
      case OP_brtrue: {
        CondGotoNode *s = mod.CurFuncCodeMemPool()->New<CondGotoNode>(op);
        s->SetOffset(static_cast<uint32>(ReadNum()));
        s->SetOpnd(ImportExpression(func), 0);
        stmt = s;
        break;
      }
      case OP_switch: {
        SwitchNode *s = mod.CurFuncCodeMemPool()->New<SwitchNode>(mod);
        s->SetDefaultLabel(static_cast<LabelIdx>(ReadNum()));
        auto tagSize = static_cast<uint32>(ReadNum());
        for (uint32 i = 0; i < tagSize; ++i) {
          int64 casetag = ReadNum();
          LabelIdx lidx(ReadNum());
          CasePair cpair = std::make_pair(casetag, lidx);
          s->GetSwitchTable().push_back(cpair);
        }
        s->SetSwitchOpnd(ImportExpression(func));
        stmt = s;
        break;
      }
      case OP_rangegoto: {
        RangeGotoNode *s = mod.CurFuncCodeMemPool()->New<RangeGotoNode>(mod);
        s->SetTagOffset(ReadNum());
        uint32 tagSize = static_cast<uint32>(ReadNum());
        for (uint32 i = 0; i < tagSize; ++i) {
          uint16 casetag = ReadNum();
          LabelIdx lidx(ReadNum());
          s->AddRangeGoto(casetag, lidx);
        }
        s->SetOpnd(ImportExpression(func), 0);
        stmt = s;
        break;
      }
      case OP_jstry: {
        JsTryNode *s = mod.CurFuncCodeMemPool()->New<JsTryNode>();
        s->SetCatchOffset(static_cast<uint32>(ReadNum()));
        s->SetFinallyOffset(static_cast<uint32>(ReadNum()));
        stmt = s;
        break;
      }
      case OP_cpptry:
      case OP_try: {
        TryNode *s = mod.CurFuncCodeMemPool()->New<TryNode>(mod);
        auto numLabels = static_cast<uint32>(ReadNum());
        for (uint32 i = 0; i < numLabels; ++i) {
          s->GetOffsets().push_back(ReadNum());
        }
        stmt = s;
        break;
      }
      case OP_catch: {
        CatchNode *s = mod.CurFuncCodeMemPool()->New<CatchNode>(mod);
        auto numTys = static_cast<uint32>(ReadNum());
        for (uint32 i = 0; i < numTys; ++i) {
          s->PushBack(ImportType());
        }
        stmt = s;
        break;
      }
      case OP_comment: {
        CommentNode *s = mod.CurFuncCodeMemPool()->New<CommentNode>(mod);
        string str;
        ReadAsciiStr(str);
        s->SetComment(str);
        stmt = s;
        break;
      }
      case OP_dowhile:
      case OP_while: {
        WhileStmtNode *s = mod.CurFuncCodeMemPool()->New<WhileStmtNode>(op);
        s->SetBody(ImportBlockNode(func));
        s->SetOpnd(ImportExpression(func), 0);
        stmt = s;
        break;
      }
      case OP_if: {
        IfStmtNode *s = mod.CurFuncCodeMemPool()->New<IfStmtNode>();
        bool hasElsePart = (ReadNum() != 0);
        s->SetThenPart(ImportBlockNode(func));
        if (hasElsePart) {
          s->SetElsePart(ImportBlockNode(func));
          s->SetNumOpnds(kOperandNumTernary);
        }
        s->SetOpnd(ImportExpression(func), 0);
        stmt = s;
        break;
      }
      case OP_block: {
        stmt = ImportBlockNode(func);
        break;
      }
      case OP_asm: {
        AsmNode *s = mod.CurFuncCodeMemPool()->New<AsmNode>(&mod.GetCurFuncCodeMPAllocator());
        mod.CurFunction()->SetHasAsm();
        s->qualifiers = static_cast<uint32>(ReadNum());
        string str;
        ReadAsciiStr(str);
        s->asmString = str;
        // the outputs
        auto count = static_cast<size_t>(ReadNum());
        UStrIdx strIdx;
        for (size_t i = 0; i < count; ++i) {
          strIdx = ImportUsrStr();
          s->outputConstraints.push_back(strIdx);
        }
        ImportReturnValues(func, &s->asmOutputs);
        // the clobber list
        count = static_cast<size_t>(ReadNum());
        for (size_t i = 0; i < count; ++i) {
          strIdx = ImportUsrStr();
          s->clobberList.push_back(strIdx);
        }
        // the labels
        count = static_cast<size_t>(ReadNum());
        for (size_t i = 0; i < count; ++i) {
          auto lidx = static_cast<LabelIdx>(ReadNum());
          s->gotoLabels.push_back(lidx);
        }
        // the inputs
        numOpr = static_cast<uint8>(ReadNum());
        s->SetNumOpnds(numOpr);
        for (int32 i = 0; i < numOpr; ++i) {
          strIdx = ImportUsrStr();
          s->inputConstraints.push_back(strIdx);
          const std::string &inStr = GlobalTables::GetUStrTable().GetStringFromStrIdx(strIdx);
          if (inStr[0] == '+') {
            s->SetHasWriteInputs();
          }
        }
        for (int32 i = 0; i < numOpr; ++i) {
          s->GetNopnd().push_back(ImportExpression(func));
        }
        stmt = s;
        break;
      }
      default:
        CHECK_FATAL(false, "Unhandled opcode tag %d", tag);
        break;
    }
    stmt->SetSrcPos(thesrcPosition);
    block->AddStatement(stmt);
  }
  if (func != nullptr) {
    func->SetBody(block);
  }
  return block;
}

void BinaryMplImport::ReadFunctionBodyField() {
  (void)ReadInt();  /// skip total size
  int32 size = ReadInt();
  for (int64 i = 0; i < size; ++i) {
    PUIdx puIdx = ImportFunction();
    MIRFunction *fn = GlobalTables::GetFunctionTable().GetFunctionFromPuidx(puIdx);
    mod.SetCurFunction(fn);

    fn->AllocSymTab();
    fn->AllocPregTab();
    fn->AllocTypeNameTab();
    fn->AllocLabelTab();

    ImportFuncIdInfo(fn);
    ImportPregTab(fn);
    ImportLocalSymTab(fn);
    ImportLabelTab(fn);
    ImportLocalTypeNameTable(fn->GetTypeNameTab());
    ImportFormalsStIdx(fn);
    if (mod.GetFlavor() < kMmpl) {
      ImportAliasMap(fn);
    }
    (void)ImportBlockNode(fn);
    mod.AddFunction(fn);
  }
  int64 tag = ReadNum();
  CHECK_FATAL(tag == ~kBinFunctionBodyStart, "pattern mismatch in Read FunctionBody");
  return;
}
}  // namespace maple
