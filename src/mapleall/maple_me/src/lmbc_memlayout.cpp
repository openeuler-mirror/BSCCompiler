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

// For each function being compiled, lay out its parameters, return values and
// local variables on its stack frame.  This involves determining how parameters
// and return values are passed from analyzing their types.
//
// Allocate all the global variables within the global memory block which is
// addressed via offset from the global pointer GP during execution.  Allocate
// this block pointed to by mirModule.globalBlkMap and perform the static
// initializations.

#include "lmbc_memlayout.h"
#include "mir_symbol.h"

namespace maple {

uint32 LMBCMemLayout::FindLargestActualArea(StmtNode *stmt, int &maxActualSize) {
  if (!stmt) {
    return maxActualSize;
  }
  Opcode opcode = stmt->op;
  switch (opcode) {
    case OP_block: {
      BlockNode *blcknode = static_cast<BlockNode *>(stmt);
      for (StmtNode &s : blcknode->GetStmtNodes()) {
        FindLargestActualArea(&s, maxActualSize);
      }
      break;
    }
    case OP_if: {
      IfStmtNode *ifnode = static_cast<IfStmtNode *>(stmt);
      FindLargestActualArea(ifnode->GetThenPart(), maxActualSize);
      FindLargestActualArea(ifnode->GetElsePart(), maxActualSize);
      break;
    }
    case OP_doloop: {
      FindLargestActualArea(static_cast<DoloopNode *>(stmt)->GetDoBody(), maxActualSize);
      break;
    }
    case OP_dowhile:
    case OP_while:
      FindLargestActualArea(static_cast<WhileStmtNode *>(stmt)->GetBody(), maxActualSize);
      break;
    case OP_call:
    case OP_icall:
    case OP_intrinsiccall: {
      ParmLocator parmlocator;  // instantiate a parm locator
      NaryStmtNode *callstmt = static_cast<NaryStmtNode *>(stmt);
      for (uint32 i = 0; i < callstmt->NumOpnds(); i++) {
        BaseNode *opnd = callstmt->Opnd(i);
        CHECK_FATAL(opnd->GetPrimType() != PTY_void, "");
        MIRType *ty = nullptr;
        if (opnd->GetPrimType() != PTY_agg) {
          ty = GlobalTables::GetTypeTable().GetTypeFromTyIdx(
              static_cast<TyIdx>(opnd->GetPrimType()));
        } else {
          Opcode opnd_opcode = opnd->GetOpCode();
          CHECK_FATAL(opnd_opcode == OP_dread || opnd_opcode == OP_iread, "");
          if (opnd_opcode == OP_dread) {
            AddrofNode *dread = static_cast<AddrofNode *>(opnd);
            MIRSymbol *sym = func->GetLocalOrGlobalSymbol(dread->GetStIdx());
            ty = GlobalTables::GetTypeTable().GetTypeFromTyIdx(sym->GetTyIdx());
            if (dread->GetFieldID() != 0) {
              CHECK_FATAL(ty->GetKind() == kTypeStruct || ty->GetKind() == kTypeClass || ty->GetKind() == kTypeUnion,
                          "expect struct or class or union");
              FieldPair thepair =
                  static_cast<MIRStructType *>(ty)->TraverseToField(dread->GetFieldID());
              ty = GlobalTables::GetTypeTable().GetTypeFromTyIdx(thepair.second.first);
            }
          } else {  // OP_iread
            IreadNode *iread = static_cast<IreadNode *>(opnd);
            ty = GlobalTables::GetTypeTable().GetTypeFromTyIdx(iread->GetTyIdx());
            CHECK_FATAL(ty->GetKind() == kTypePointer, "");
            ty = GlobalTables::GetTypeTable().GetTypeFromTyIdx(
                static_cast<MIRPtrType *>(ty)->GetPointedTyIdx());
            if (iread->GetFieldID() != 0) {
              CHECK_FATAL(ty->GetKind() == kTypeStruct || ty->GetKind() == kTypeClass || ty->GetKind() == kTypeUnion,
                          "expect struct or class or union");
              FieldPair thepair =
                  static_cast<MIRStructType *>(ty)->TraverseToField(iread->GetFieldID());
              ty = GlobalTables::GetTypeTable().GetTypeFromTyIdx(thepair.second.first);
            }
          }
        }
        PLocInfo ploc;
        parmlocator.LocateNextParm(ty, ploc);
        maxActualSize = std::max(maxActualSize, ploc.memoffset + ploc.memsize);
        maxActualSize = maplebe::RoundUp(maxActualSize, GetPrimTypeSize(PTY_ptr));
      }
      break;
    }
    default:
      return maxActualSize;
  }
  maxActualSize = maplebe::RoundUp(maxActualSize, GetPrimTypeSize(PTY_ptr));
  return maxActualSize;
}

// go over all outgoing calls in the function body and get the maximum space
// needed for storing the actuals based on the actual parameters and the ABI;
// this assumes that all nesting of statements has been removed, so that all
// the statements are at only one block level
uint32 LMBCMemLayout::FindLargestActualArea(void) {
  int32 maxActualSize = 0;
  FindLargestActualArea(func->GetBody(), maxActualSize);
  return static_cast<uint32>(maxActualSize);
}

void LMBCMemLayout::LayoutStackFrame(void) {
  MIRSymbol *sym = nullptr;
  // go through formal parameters
  ParmLocator parmlocator;  // instantiate a parm locator
  PLocInfo ploc;
  for (uint32 i = 0; i < func->GetFormalDefVec().size(); i++) {
    FormalDef formalDef = func->GetFormalDefAt(i);
    sym = formalDef.formalSym;
    MIRType *ty = GlobalTables::GetTypeTable().GetTypeFromTyIdx(formalDef.formalTyIdx);
    parmlocator.LocateNextParm(ty, ploc);
    uint32 stindex = sym->GetStIndex();
    // always passed in memory, so allocate in seg_upformal
    sym_alloc_table[stindex].mem_segment = &seg_upformal;
    seg_upformal.size = maplebe::RoundUp(seg_upformal.size, ty->GetAlign());
    sym_alloc_table[stindex].offset = seg_upformal.size;
    seg_upformal.size += ty->GetSize();
    seg_upformal.size = maplebe::RoundUp(seg_upformal.size, GetPrimTypeSize(PTY_ptr));
  }

  // allocate seg_formal in seg_FPbased
  seg_formal.how_alloc.mem_segment = &seg_FPbased;
  seg_FPbased.size = maplebe::RoundDown(seg_FPbased.size, GetPrimTypeSize(PTY_ptr));
  seg_FPbased.size -= seg_formal.size;
  seg_FPbased.size = maplebe::RoundDown(seg_FPbased.size, GetPrimTypeSize(PTY_ptr));
  seg_formal.how_alloc.offset = seg_FPbased.size;

  // allocate the local variables
  uint32 symtabsize = func->GetSymTab()->GetSymbolTableSize();
  for (uint32 i = 0; i < symtabsize; i++) {
    sym = func->GetSymTab()->GetSymbolFromStIdx(i);
    if (!sym) {
      continue;
    }
    if (sym->IsDeleted()) {
      continue;
    }
    if (sym->GetStorageClass() != kScAuto) {
      continue;
    }
    uint32 stindex = sym->GetStIndex();
    sym_alloc_table[stindex].mem_segment = &seg_FPbased;
    seg_FPbased.size -= sym->GetType()->GetSize();
    seg_FPbased.size = maplebe::RoundDown(seg_FPbased.size, sym->GetType()->GetAlign());
    sym_alloc_table[stindex].offset = seg_FPbased.size;
  }
  seg_FPbased.size = maplebe::RoundDown(seg_FPbased.size, GetPrimTypeSize(PTY_ptr));

  // allocate seg_actual for storing the outgoing parameters; this requires
  // going over all outgoing calls and get the maximum space needed for the
  // actuals
  seg_actual.size = FindLargestActualArea();
  func->SetOutParmSize(seg_actual.size);

  // allocate seg_actual in seg_SPbased
  seg_actual.how_alloc.mem_segment = &seg_SPbased;
  seg_actual.how_alloc.offset = seg_SPbased.size;
  seg_SPbased.size = maplebe::RoundUp(seg_SPbased.size, GetPrimTypeSize(PTY_ptr));
  seg_SPbased.size += seg_actual.size;
  seg_SPbased.size = maplebe::RoundUp(seg_SPbased.size, GetPrimTypeSize(PTY_ptr));
}

inline uint8 GetU8Const(MIRConst *c) {
  MIRIntConst *intconst = static_cast<MIRIntConst *>(c);
  return static_cast<uint8>(intconst->GetValue());
}

inline uint16 GetU16Const(MIRConst *c) {
  MIRIntConst *intconst = static_cast<MIRIntConst *>(c);
  return static_cast<uint16>(intconst->GetValue());
}

inline uint32 GetU32Const(MIRConst *c) {
  MIRIntConst *intconst = static_cast<MIRIntConst *>(c);
  return static_cast<uint32>(intconst->GetValue());
}

inline uint64 GetU64Const(MIRConst *c) {
  MIRIntConst *intconst = static_cast<MIRIntConst *>(c);
  return static_cast<uint64>(intconst->GetValue());
}

inline uint32 GetF32Const(MIRConst *c) {
  MIRFloatConst *floatconst = static_cast<MIRFloatConst *>(c);
  return static_cast<uint32>(floatconst->GetIntValue());
}

inline uint64 GetF64Const(MIRConst *c) {
  MIRDoubleConst *doubleconst = static_cast<MIRDoubleConst *>(c);
  return static_cast<uint64>(doubleconst->GetIntValue());
}

void GlobalMemLayout::FillScalarValueInMap(uint32 startaddress, PrimType pty, MIRConst *c) {
  switch (pty) {
    case PTY_u1:
    case PTY_u8:
    case PTY_i8: {
      uint8 *p = &mirModule->GetGlobalBlockMap()[startaddress];
      *p = GetU8Const(c);
      break;
    }
    case PTY_u16:
    case PTY_i16: {
      uint16 *p = (uint16 *)(&mirModule->GetGlobalBlockMap()[startaddress]);
      *p = GetU16Const(c);
      break;
    }
    case PTY_u32:
    case PTY_i32: {
      uint32 *p = (uint32 *)(&mirModule->GetGlobalBlockMap()[startaddress]);
      *p = GetU32Const(c);
      break;
    }
    case PTY_u64:
    case PTY_i64: {
      uint64 *p = (uint64 *)(&mirModule->GetGlobalBlockMap()[startaddress]);
      *p = GetU64Const(c);
      break;
    }
    case PTY_f32: {
      uint32 *p = (uint32 *)(&mirModule->GetGlobalBlockMap()[startaddress]);
      *p = GetF32Const(c);
      break;
    }
    case PTY_f64: {
      uint64 *p = (uint64 *)(&mirModule->GetGlobalBlockMap()[startaddress]);
      *p = GetF64Const(c);
      break;
    }
    default:
      CHECK_FATAL(false, "FillScalarValueInMap: NYI");
  }
  return;
}

void GlobalMemLayout::FillTypeValueInMap(uint32 startaddress, MIRType *ty, MIRConst *c) {
  switch (ty->GetKind()) {
    case kTypeScalar:
      FillScalarValueInMap(startaddress, ty->GetPrimType(), c);
      break;
    case kTypeArray: {
      MIRArrayType *arraytype = static_cast<MIRArrayType *>(ty);
      MIRType *elemtype = arraytype->GetElemType();
      int32 elemsize = elemtype->GetSize();
      MIRAggConst *aggconst = static_cast<MIRAggConst *>(c);
      CHECK_FATAL(aggconst, "FillTypeValueInMap: inconsistent array initialization specification");
      MapleVector<MIRConst *> &constvec = aggconst->GetConstVec();
      for (MapleVector<MIRConst *>::iterator it = constvec.begin(); it != constvec.end();
           it++, startaddress += elemsize) {
        FillTypeValueInMap(startaddress, elemtype, *it);
      }
      break;
    }
    case kTypeStruct: {
      MIRStructType *structty = static_cast<MIRStructType *>(ty);
      MIRAggConst *aggconst = static_cast<MIRAggConst *>(c);
      CHECK_FATAL(aggconst, "FillTypeValueInMap: inconsistent struct initialization specification");
      MapleVector<MIRConst *> &constvec = aggconst->GetConstVec();
      for (uint32 i = 0; i < constvec.size(); i++) {
        uint32 fieldID = aggconst->GetFieldIdItem(i);
        FieldPair thepair = structty->TraverseToField(fieldID);
        MIRType *fieldty = GlobalTables::GetTypeTable().GetTypeFromTyIdx(thepair.second.first);
        uint32 offset = be->GetFieldOffset(*structty, fieldID).first;
        FillTypeValueInMap(startaddress + offset, fieldty, constvec[i]);
      }
      break;
    }
    case kTypeClass: {
      MIRClassType *classty = static_cast<MIRClassType *>(ty);
      MIRAggConst *aggconst = static_cast<MIRAggConst *>(c);
      CHECK_FATAL(aggconst, "FillTypeValueInMap: inconsistent class initialization specification");
      MapleVector<MIRConst *> &constvec = aggconst->GetConstVec();
      for (uint32 i = 0; i < constvec.size(); i++) {
        uint32 fieldID = aggconst->GetFieldIdItem(i);
        FieldPair thepair = classty->TraverseToField(fieldID);
        MIRType *fieldty = GlobalTables::GetTypeTable().GetTypeFromTyIdx(thepair.second.first);
        uint32 offset = be->GetFieldOffset(*classty, fieldID).first;
        FillTypeValueInMap(startaddress + offset, fieldty, constvec[i]);
      }
      break;
    }
    default:
      CHECK_FATAL(false, "FillTypeValueInMap: NYI");
  }
}

void GlobalMemLayout::FillSymbolValueInMap(const MIRSymbol *sym) {
  if (sym->GetKonst() == nullptr) {
    return;
  }
  uint32 stindex = sym->GetStIndex();
  CHECK(stindex < sym_alloc_table.size(),
        "index out of range in GlobalMemLayout::FillSymbolValueInMap");
  uint32 symaddress = sym_alloc_table[stindex].offset;
  FillTypeValueInMap(symaddress, sym->GetType(), sym->GetKonst());
  return;
}

GlobalMemLayout::GlobalMemLayout(maplebe::BECommon *b,  MIRModule *mod, MapleAllocator *mallocator) :
    seg_GPbased(MS_GPbased), sym_alloc_table(mallocator->Adapter()), be(b), mirModule(mod) {
  uint32 symtabsize = GlobalTables::GetGsymTable().GetSymbolTableSize();
  sym_alloc_table.resize(symtabsize);
  MIRSymbol *sym = nullptr;
  // allocate the global variables ordered based on alignments
  for (int32 curalign = 8; curalign != 0; curalign >>= 1) {
    for (uint32 i = 0; i < symtabsize; i++) {
      sym = GlobalTables::GetGsymTable().GetSymbolFromStidx(i);
      if (!sym) {
        continue;
      }
      if (sym->GetStorageClass() != kScGlobal && sym->GetStorageClass() != kScFstatic) {
        continue;
      }
      if (sym->GetType()->GetAlign() != curalign) {
        continue;
      }
      uint32 stindex = sym->GetStIndex();
      sym_alloc_table[stindex].mem_segment = &seg_GPbased;
      seg_GPbased.size = maplebe::RoundUp(seg_GPbased.size, sym->GetType()->GetAlign());
      sym_alloc_table[stindex].offset = seg_GPbased.size;
      seg_GPbased.size += sym->GetType()->GetSize();
    }
  }
  seg_GPbased.size = maplebe::RoundUp(seg_GPbased.size, GetPrimTypeSize(PTY_ptr));
  mirModule->SetGlobalMemSize(seg_GPbased.size);
  // allocate the memory map for the GP block
  mirModule->SetGlobalBlockMap(
      static_cast<uint8 *>(mirModule->GetMemPool()->Calloc(seg_GPbased.size)));
  // perform initialization on globalblkmap
  for (uint32 i = 0; i < symtabsize; i++) {
    sym = GlobalTables::GetGsymTable().GetSymbolFromStidx(i);
    if (!sym) {
      continue;
    }
    if (sym->GetStorageClass() != kScGlobal && sym->GetStorageClass() != kScFstatic) {
      continue;
    }
  }
}

// LocateNextParm should be called with each parameter in the parameter list
// starting from the beginning, one call per parameter in sequence; it returns
// the information on how each parameter is passed in ploc
void ParmLocator::LocateNextParm(const MIRType *ty, PLocInfo &ploc) {
  ploc.memoffset = maplebe::RoundUp(lastMemOffset, 8);
  ploc.memsize = ty->GetSize();

  uint32 rightpad = 0;
  parmNum++;

  switch (ty->GetPrimType()) {
    case PTY_u1:
    case PTY_u8:
    case PTY_i8:
    case PTY_u16:
    case PTY_i16:
      rightpad = GetPrimTypeSize(PTY_i32) - ploc.memsize;
      break;
    case PTY_a32:
    case PTY_u32:
    case PTY_i32:
    case PTY_a64:
    case PTY_u64:
    case PTY_i64:
    case PTY_ptr:
    case PTY_ref:
#ifdef DYNAMICLANG
    case PTY_simplestr:
    case PTY_simpleobj:
    case PTY_dynany:
    case PTY_dyni32:
    case PTY_dynf64:
    case PTY_dynstr:
    case PTY_dynobj:
    case PTY_dynundef:
    case PTY_dynbool:
    case PTY_dynf32:
    case PTY_dynnone:
    case PTY_dynnull:
#endif
      break;

    case PTY_f32:
      rightpad = GetPrimTypeSize(PTY_f64) - ploc.memsize;
      break;
    case PTY_c64:
    case PTY_f64:
    case PTY_v2i32:
    case PTY_v4i16:
    case PTY_v8i8:
    case PTY_v2u32:
    case PTY_v4u16:
    case PTY_v8u8:
    case PTY_v2f32:
      break;

    case PTY_c128:
    case PTY_v2i64:
    case PTY_v4i32:
    case PTY_v8i16:
    case PTY_v16i8:
    case PTY_v2u64:
    case PTY_v4u32:
    case PTY_v8u16:
    case PTY_v16u8:
    case PTY_v2f64:
    case PTY_v4f32:
      break;

    case PTY_agg: {
      // compute rightpad
      int32 paddedSize = maplebe::RoundUp(ploc.memsize, 8);
      rightpad = paddedSize - ploc.memsize;
      break;
    }
    default:
      CHECK_FATAL(false, "unsupported type");
  }

  lastMemOffset = ploc.memoffset + ploc.memsize + rightpad;
  return;
}

// instantiated with the type of the function return value, it describes how
// the return value is to be passed back to the caller
ReturnMechanism::ReturnMechanism(const MIRType *retty) : fake_first_parm(false) {
  switch (retty->GetPrimType()) {
    case PTY_u1:
    case PTY_u8:
    case PTY_i8:
    case PTY_u16:
    case PTY_i16:
    case PTY_a32:
    case PTY_u32:
    case PTY_i32:
    case PTY_a64:
    case PTY_u64:
    case PTY_i64:
    case PTY_f32:
    case PTY_f64:
#ifdef DYNAMICLANG
    case PTY_simplestr:
    case PTY_simpleobj:
    case PTY_dynany:
    case PTY_dyni32:
    case PTY_dynstr:
    case PTY_dynobj:
#endif
      ptype0 = retty->GetPrimType();
      return;

    case PTY_c64:
    case PTY_c128:
      fake_first_parm = true;
      ptype0 = PTY_a32;
      return;

    case PTY_agg: {
      uint32 size = retty->GetSize();
      if (size > 4) {
        fake_first_parm = true;
        ptype0 = PTY_a32;
      } else {
        ptype0 = PTY_u32;
      }
      return;
    }

    default:
      return;
  }
}

}  // namespace maple
