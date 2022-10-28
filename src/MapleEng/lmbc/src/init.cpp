/*
 * Copyright (c) [2022] Futurewei Technologies, Inc. All rights reserved.
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
#include "massert.h"
#include "lmbc_eng.h"
#include "eng_shim.h"

namespace maple {

// Align offset to required alignment
inline void AlignOffset(uint32 &offset, uint32 align) {
  offset = (offset + align-1) & ~(align-1);
}

LmbcFunc::LmbcFunc(LmbcMod *mod, MIRFunction *func) : lmbcMod(mod), mirFunc(func) {
  frameSize = ((func->GetFrameSize()+7)>>3)<<3;  // round up to nearest 8
  isVarArgs = func->GetMIRFuncType()->IsVarargs();
  numPregs  = func->GetPregTab()->Size();
}

void LmbcMod::InitModule(void) {
  CalcGlobalAndStaticVarSize();
  for (MIRFunction *mirFunc : mirMod->GetFunctionList()) {
    if (auto node = mirFunc->GetBody()) {
      LmbcFunc* fn = new LmbcFunc(this, mirFunc);
      ASSERT(fn, "Create Lmbc function failed");
      fn->ScanFormals();
      fn->ScanLabels(node);
      funcMap[mirFunc->GetPuidx()] = fn;
      if (mirFunc->GetName().compare("main") == 0) {
        mainFn = fn;
      }
    }
  }
  InitGlobalVars();
}

void LmbcFunc::ScanFormals(void) {
  MapleVector<FormalDef> formalDefVec = mirFunc->GetFormalDefVec();
  formalsNum  = formalDefVec.size();
  formalsSize = 0;
  formalsNumVars = 0;
  formalsAggSize = 0;
  MASSERT(mirFunc->GetReturnType() != nullptr, "mirFunc return type is null");
  retSize = mirFunc->GetReturnType()->GetSize();
  for (uint32 i = 0; i < formalDefVec.size(); i++) {
    MIRSymbol* symbol = formalDefVec[i].formalSym;
    MIRType* ty = GlobalTables::GetTypeTable().GetTypeFromTyIdx(formalDefVec[i].formalTyIdx);
    bool isPreg = (symbol->GetSKind() == kStPreg);
    int32 storageIdx;
    if (ty->GetPrimType() == PTY_agg) {
      storageIdx = formalsAggSize;
      formalsAggSize += ty->GetSize();
    } else {
      storageIdx = isPreg? symbol->GetPreg()->GetPregNo(): ++formalsNumVars;
    }
    // collect formal params info
    ParmInf* pInf = new ParmInf(ty->GetPrimType(), ty->GetSize(), isPreg, storageIdx);
    stidx2Parm[symbol->GetStIdx().FullIdx()] = pInf; // formals info map keyed on formals stidx
    pos2Parm.push_back(pInf);                        // vector of formals info in formalDefVec order
    formalsSize += ty->GetSize();
  }
}

void LmbcFunc::ScanLabels(StmtNode* stmt) {
  while (stmt != nullptr) {
    switch (stmt->op) {
      case OP_block:
        stmt = static_cast<BlockNode*>(stmt)->GetFirst();
        ScanLabels(stmt);
        break;
      case OP_label:
        labelMap[static_cast<LabelNode*>(stmt)->GetLabelIdx()] = stmt;
        break;
      default:
        break;
    }
    stmt= stmt->GetNext();
  }
}

// Check for initialized flex array struct member and return size. The number
// of elements is not specified in the array type declaration but determined
// by array initializer.
// Note::
// - Flex array must be last field of a top level struct
// - Only 1st dim of multi-dim array can be unspecified. Other dims must have bounds.
// - In Maple AST, array with unspecified num elements is defined with 1 element in
//   1st array dim, and is the storage for 1st initialiazed array element.
// The interpreter identifies such arrays by a dim of 1 in the array's 1st dim
// together with an array const initializer with > 1 element (should identify using
// kTypeFArray return from GetKind() but it's returning kTypeArray now ).
uint32 CheckFlexArrayMember(MIRSymbol &sym, MIRType &ty) {
  auto &stType = static_cast<MIRStructType&>(ty);
  auto &stConst = static_cast<MIRAggConst&>(*sym.GetKonst());
  TyIdxFieldAttrPair tfap = stType.GetTyidxFieldAttrPair(stType.GetFieldsSize()-1); // last struct fd
  MIRType *lastFdType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(tfap.first);
  if (lastFdType->GetKind() == kTypeArray &&  // last struct field is array
      static_cast<MIRArrayType*>(lastFdType)->GetSizeArrayItem(0) == 1 &&  // 1st dim of array is 1
      stConst.GetConstVec().size() == stType.GetFieldsSize()) {  // there is an initializer for the array
    MIRConst &elemConst = *stConst.GetConstVecItem(stConst.GetConstVec().size()-1);  // get array initializer
    MASSERT(elemConst.GetType().GetKind() == kTypeArray, "array initializer expected");
    auto &arrCt = static_cast<MIRAggConst&>(elemConst);
    if (arrCt.GetConstVec().size() > 1) {
      return (arrCt.GetConstVec().size()-1) * elemConst.GetType().GetSize();  // 1st elem already in arr type def
    }
  }
  return 0;
}

// Calcluate total memory needed for global vars, Fstatic vars and initialized PUstatic vars.
// Walks global sym table and local sym table of all funcs to gather size info and adds
// var to table for looking up its size, type and offset within the storage segment.
void LmbcMod::CalcGlobalAndStaticVarSize() {
  uint32 offset = 0;
  for (size_t i = 0; i < GlobalTables::GetGsymTable().GetSymbolTableSize(); ++i) {
    MIRSymbol *sym = GlobalTables::GetGsymTable().GetSymbolFromStidx(i);
    if (!sym ||
        !(sym->GetSKind() == kStVar) ||
        !(sym->GetStorageClass() == kScGlobal || sym->GetStorageClass() == kScFstatic)) { 
      continue;
    }
    if (MIRType *ty = sym->GetType()) {
      AlignOffset(offset, ty->GetAlign());   // check and align var
      VarInf* pInf = new VarInf(ty->GetPrimType(), ty->GetSize(), false, offset, sym);
      AddGlobalVar(*sym, pInf);              // add var to lookup table
      offset += ty->GetSize();
      if (ty->GetKind() == kTypeStruct) {    // check and account for flex array member
        offset += CheckFlexArrayMember(*sym, *ty);
      }
    }
  }
  globalsSize = offset;
  // get total size of nitialized function static vars
  for (MIRFunction *func : mirMod->GetFunctionList()) {
    if (auto node = func->GetBody()) {
      ScanPUStatic(func);
    }
  }
}

void LmbcMod::ScanPUStatic(MIRFunction *func) {
  size_t size = func->GetSymbolTabSize();
  for (size_t i = 0; i < size; ++i) {
    MIRSymbol *sym = func->GetSymbolTabItem(i);
    if (!sym || !sym->IsPUStatic() || !sym->IsConst()) {  // exclude un-init PUStatic
      continue;
    }
    if (MIRType *ty = sym->GetType()) {
      VarInf* pInf = new VarInf(ty->GetPrimType(), ty->GetSize(), false, globalsSize, sym, func->GetPuidx());
      AddPUStaticVar(func->GetPuidx(), *sym, pInf);  // add var to lookup table
      globalsSize += ty->GetSize();
    } 
  }
}

// Get the address of a non-agg global var or next field within a global var to init.
// If a PTY_agg var, align offset to next init addr (aggrInitOffset) for ptyp.
// Otherwise, storeIdx should be aligned properly already in CalcGlobalVarsSize.
// - storeIdx: offset of the global var in global var segment
// - aggrInitOffset: offset of the field to init within global var of type PTY_agg
uint8 *LmbcMod::GetGlobalVarInitAddr(VarInf* pInf, uint32 align) {
  if (pInf->ptyp != PTY_agg) {    // init non-aggr global var
    return globals + pInf->storeIdx;
  }
  AlignOffset(aggrInitOffset, align);
  return globals + pInf->storeIdx + aggrInitOffset;
}

inline void LmbcMod::UpdateGlobalVarInitAddr(VarInf* pInf, uint32 size) {
  if (pInf->ptyp == PTY_agg) {
    aggrInitOffset += size;
  }
}

// Check for un-named bitfields in initialized global struct vars and
// include in global var memory sizing and field offset calcuations. Un-named
// bit fields appears as gaps in field id between initialized struct fields
// of a global var.
void LmbcMod::CheckUnamedBitField(MIRStructType &stType, uint32 &prevInitFd, uint32 curFd, int32 &allocdBits) {

  if (curFd - 1 == prevInitFd) {
    prevInitFd = curFd;
    return;
  }

  for (auto i = prevInitFd; i < curFd -1; ++i)  {  // struct fd idx 0 based; agg const fd 1 based
    TyIdxFieldAttrPair tfap = stType.GetTyidxFieldAttrPair(i);
    MIRType *ty = GlobalTables::GetTypeTable().GetTypeFromTyIdx(tfap.first); // type of struct fd
    // Gaps in struct fields with initializer are either un-named bit fields or empty struct fields
    // Account for bits in un-named bit fields. Skip over emtpy struct fields.
    if (ty->GetKind() != kTypeBitField) {
      continue;
    }
    MASSERT(ty->GetKind()==kTypeBitField, "Un-named bitfield expected");
    uint8 bitFdWidth = static_cast<MIRBitFieldType*>(ty)->GetFieldSize();
    uint32 baseFdSz = GetPrimTypeSize(ty->GetPrimType());

    uint32 align = allocdBits ? 1 : baseFdSz;        // align with base fd if no bits have been allocated
    AlignOffset(aggrInitOffset, align);

    if (allocdBits + bitFdWidth > (baseFdSz * 8)) {  // alloc bits will cross align boundary of base type
      aggrInitOffset+= baseFdSz;
      allocdBits = bitFdWidth;                       // alloc bits at new boundary
    } else {
      allocdBits += bitFdWidth;
    }
  }
  prevInitFd = curFd;
}

void LmbcMod::InitStrConst(VarInf* pInf, MIRStrConst &mirStrConst, uint8* dst) {
  UStrIdx ustrIdx = mirStrConst.GetValue();
  auto it = globalStrTbl.insert(
      std::pair<uint32, std::string>(ustrIdx, GlobalTables::GetUStrTable().GetStringFromStrIdx(ustrIdx)));
  *(const char **)dst = it.first->second.c_str();
}

inline void LmbcMod::InitFloatConst(VarInf *pInf, MIRFloatConst &f32Const, uint8* dst) {
  *(float*)dst = f32Const.GetValue();
}

inline void LmbcMod::InitDoubleConst(VarInf *pInf, MIRDoubleConst &f64Const, uint8* dst) {
  *(double*)dst = f64Const.GetValue();
}

void LmbcMod::InitLblConst(VarInf *pInf, MIRLblConst &labelConst, uint8 *dst) {
  LabelIdx labelIdx = labelConst.GetValue();
  LmbcFunc *fn = LkupLmbcFunc(labelConst.GetPUIdx());
  StmtNode* label = fn->labelMap[labelIdx];
  MASSERT(label, "InitLblConst label not foound");
  *(StmtNode **)dst = label;
}

void LmbcMod::InitIntConst(VarInf* pInf, MIRIntConst &intConst, uint8* dst) {
  int64 val = intConst.GetExtValue();
  switch(intConst.GetType().GetPrimType()) {
    case PTY_i64:
      *(int64*)dst = (int64)val;
      break;
    case PTY_i32:
      *(int32*)dst = (int32)val;
      break;
    case PTY_i16:
      *(int16*)dst = (int16)val;
      break;
    case PTY_i8:
      *(int8*)dst = (int8)val;
      break;
    case PTY_u64:
      *(uint64*)dst = (uint64)val;
      break;
    case PTY_u32:
      *(uint32*)dst = (uint32)val;
      break;
    case PTY_u16:
      *(uint16*)dst = (uint16)val;
      break;
    case PTY_u8:
      *(uint8*)dst = (uint8)val;
      break;
    default:
      break;
  }
}

void LmbcMod::InitPointerConst(VarInf *pInf, MIRConst &mirConst) {
  uint8 *dst = GetGlobalVarInitAddr(pInf, mirConst.GetType().GetAlign());
  switch(mirConst.GetKind()) {
    case kConstAddrof:
      InitAddrofConst(pInf, static_cast<MIRAddrofConst&>(mirConst), dst);
      break;
    case kConstStrConst:
      InitStrConst(pInf, static_cast<MIRStrConst&>(mirConst), dst);
      break;
    case kConstInt: {
      InitIntConst(pInf, static_cast<MIRIntConst&>(mirConst), dst);
      break;
    }
    case kConstAddrofFunc:
    default:
      MASSERT(false, "InitPointerConst %d kind NYI", mirConst.GetKind());
      break;
  }
  UpdateGlobalVarInitAddr(pInf, mirConst.GetType().GetSize());
}

void SetBitFieldConst(uint8* baseFdAddr, uint32 baseFdSz, uint32 bitsOffset, uint8 bitsSize, MIRConst &elemConst) {
  MIRIntConst &intConst = static_cast<MIRIntConst&>(elemConst); (void)intConst;
  int64  val = intConst.GetExtValue();
  uint64 mask = ~(0xffffffffffffffff << bitsSize);
  uint64 from = (val & mask) << bitsOffset;
  mask = mask << bitsOffset;
  switch(elemConst.GetType().GetPrimType()) {
   case PTY_i64:
     *(int64*)baseFdAddr  = ((*(int64*)baseFdAddr) & ~(mask))  | from;
     break;
   case PTY_i32:
     *(int32*)baseFdAddr  = ((*(int32*)baseFdAddr) & ~(mask))  | from;
     break;
   case PTY_i16:
     *(int16*)baseFdAddr  = ((*(int16*)baseFdAddr) & ~(mask))  | from;
     break;
   case PTY_i8:
     *(int8*)baseFdAddr   = ((*(int8*)baseFdAddr) & ~(mask))   | from;
     break;
   case PTY_u64:
     *(uint64*)baseFdAddr = ((*(uint64*)baseFdAddr) & ~(mask)) | from;
     break;
   case PTY_u32:
     *(uint32*)baseFdAddr = ((*(uint32*)baseFdAddr) & ~(mask)) | from;
     break;
   case PTY_u16:
     *(uint16*)baseFdAddr = ((*(uint16*)baseFdAddr) & ~(mask)) | from;
     break;
   case PTY_u8:
     *(uint8*)baseFdAddr  = ((*(uint8*)baseFdAddr) & ~(mask))  | from;
     break;
   default:
     MASSERT(false, "Unexpected primary type");
     break;
  }
}

void LmbcMod::InitBitFieldConst(VarInf *pInf, MIRConst &elemConst, int32 &allocdBits, bool &forceAlign) {
  uint8 bitFdWidth = static_cast<MIRBitFieldType&>(elemConst.GetType()).GetFieldSize();
  if (!bitFdWidth) {  // flag to force align immediate following bit field
    forceAlign = true;
    return;
  }
  if (forceAlign) {   // align to next boundary
    aggrInitOffset += (allocdBits + 7) >> 3;
    forceAlign = false;
  }
  uint32 baseFdSz  = GetPrimTypeSize(elemConst.GetType().GetPrimType());
  uint32 align = allocdBits ? 1 : baseFdSz;        // align with base fd if no bits have been allocated
  uint8* baseFdAddr = GetGlobalVarInitAddr(pInf, align);

  if (allocdBits + bitFdWidth > (baseFdSz * 8)) {  // alloc bits will cross align boundary of base type
    baseFdAddr = baseFdAddr + baseFdSz;            // inc addr & offset by size of base type
    SetBitFieldConst(baseFdAddr, baseFdSz, 0, bitFdWidth, elemConst);
    aggrInitOffset+= baseFdSz;
    allocdBits = bitFdWidth;                       // alloc bits at new boundary
  } else {
    SetBitFieldConst(baseFdAddr, baseFdSz, allocdBits, bitFdWidth, elemConst);
    allocdBits += bitFdWidth;
  }
}

void LmbcMod::InitAggConst(VarInf *pInf, MIRConst &mirConst) {
  auto &stType = static_cast<MIRStructType&>(mirConst.GetType());
  auto &aggConst = static_cast<MIRAggConst&>(mirConst);
  bool forceAlign = false;
  int32 allocdBits = 0;

  AlignOffset(aggrInitOffset, aggConst.GetType().GetAlign());  // next init offset in global var mem
  MIRTypeKind prevElemKind = kTypeUnknown;
  for (uint32 i = 0, prevInitFd = 0; i < aggConst.GetConstVec().size(); ++i) {
    MIRConst &elemConst = *aggConst.GetConstVecItem(i);
    MIRType  &elemType  = elemConst.GetType();

    // if non bit fd preceded by bit fd, round bit fd to byte boundary
    // so next bit fd will start on new boundary
    if (prevElemKind == kTypeBitField && elemType.GetKind() != kTypeBitField) {
      forceAlign = false;
      if (allocdBits) {
        aggrInitOffset += (allocdBits + 7) >> 3; // pad preceding bit fd to byte boundary
        allocdBits = 0;
      }
    }

    // No need to check for un-named bit fd if aggr is an array
    if (stType.GetKind() != kTypeArray) {
      CheckUnamedBitField(stType, prevInitFd, aggConst.GetFieldIdItem(i), allocdBits);
    }
    switch(elemType.GetKind()) {
      case kTypeScalar:
        InitScalarConst(pInf, elemConst);
        break;
      case kTypeStruct:
      case kTypeUnion:
        InitAggConst(pInf, elemConst);
        break;
      case kTypeArray:
        InitArrayConst(pInf, elemConst);
        break;
      case kTypePointer:
        InitPointerConst(pInf, elemConst);
        break;
      case kTypeBitField: {
        InitBitFieldConst(pInf, elemConst, allocdBits, forceAlign);
        break;
      }
      default: {
        MASSERT(false, "init struct type %d NYI", elemType.GetKind());
        break;
      }    
    }
    prevElemKind = elemType.GetKind();
  }
}

void LmbcMod::InitScalarConst(VarInf *pInf, MIRConst &mirConst) {
  uint8 *dst = GetGlobalVarInitAddr(pInf, mirConst.GetType().GetAlign());
  switch (mirConst.GetKind()) {
    case kConstInt:
      InitIntConst(pInf, static_cast<MIRIntConst&>(mirConst), dst);
      break;
    case kConstFloatConst:
      InitFloatConst(pInf, static_cast<MIRFloatConst&>(mirConst), dst);
      break;
    case kConstDoubleConst:
      InitDoubleConst(pInf, static_cast<MIRDoubleConst&>(mirConst), dst);
      break;
    case kConstStrConst:
      InitStrConst(pInf, static_cast<MIRStrConst&>(mirConst), dst);
      break;
    case kConstLblConst:
      InitLblConst(pInf, static_cast<MIRLblConst&>(mirConst), dst);
      break;
    case kConstStr16Const:
    case kConstAddrof:
    case kConstAddrofFunc:
    default:
      MASSERT(false, "Scalar Const Type %d NYI", mirConst.GetKind());
      break;
  }
  UpdateGlobalVarInitAddr(pInf, mirConst.GetType().GetSize());
}

void LmbcMod::InitArrayConst(VarInf *pInf, MIRConst &mirConst) {
  MIRArrayType &arrayType = static_cast<MIRArrayType&>(mirConst.GetType());
  MIRAggConst  &arrayCt = static_cast<MIRAggConst&>(mirConst);
  AlignOffset(aggrInitOffset, arrayType.GetAlign());

  size_t uNum = arrayCt.GetConstVec().size();
  uint32 dim = arrayType.GetSizeArrayItem(0);
  TyIdx scalarIdx = arrayType.GetElemTyIdx();
  MIRType *subTy = GlobalTables::GetTypeTable().GetTypeFromTyIdx(scalarIdx);
  if (uNum == 0 && dim != 0) {
    while (subTy->GetKind() == kTypeArray) {
      MIRArrayType *aSubTy = static_cast<MIRArrayType *>(subTy);
      if (aSubTy->GetSizeArrayItem(0) > 0) {
        dim *= (aSubTy->GetSizeArrayItem(0));
      }
      scalarIdx = aSubTy->GetElemTyIdx();
      subTy = GlobalTables::GetTypeTable().GetTypeFromTyIdx(scalarIdx);
    }
  }
  for (size_t i = 0; i < uNum; ++i) {
    MIRConst *elemConst = arrayCt.GetConstVecItem(i);
    if (IsPrimitiveVector(subTy->GetPrimType())) {
      MASSERT(false, "Unexpected primitive vector");
    } else if (IsPrimitiveScalar(elemConst->GetType().GetPrimType())) {
      bool strLiteral = false;
      if (arrayType.GetDim() == 1) {
        MIRType *ety = arrayType.GetElemType();
        if (ety->GetPrimType() == PTY_i8 || ety->GetPrimType() == PTY_u8) {
          strLiteral = true;
        }
      }
      InitScalarConst(pInf, *elemConst);
    } else if (elemConst->GetType().GetKind() == kTypeArray) {
      InitArrayConst(pInf, *elemConst);
    } else if (elemConst->GetType().GetKind() == kTypeStruct || 
               elemConst->GetType().GetKind() == kTypeClass  ||
               elemConst->GetType().GetKind() == kTypeUnion) {
      InitAggConst(pInf, *elemConst);
//  } else if (elemConst->GetKind() == kConstAddrofFunc) {
//    InitScalarConstant(pInf, *elemConst);
    } else {
      ASSERT(false, "should not run here");
    }
  }
}

void LmbcMod::InitAddrofConst(VarInf *pInf, MIRAddrofConst &addrofConst, uint8* dst) {
  StIdx stIdx = addrofConst.GetSymbolIndex();
  int32 offset = addrofConst.GetOffset();
  uint8 *addr = pInf->sym->IsPUStatic() ? GetVarAddr(pInf->puIdx, stIdx) : GetVarAddr(stIdx);
  *(uint8**)dst = addr + offset;
}

void LmbcMod::InitGlobalVariable(VarInf *pInf) {
  MIRConst *mirConst = pInf->sym->GetKonst();
  uint8 *dst = GetGlobalVarInitAddr(pInf, mirConst->GetType().GetAlign());

  switch(mirConst->GetKind()) {
    case kConstAggConst:
      aggrInitOffset = 0;
      InitAggConst(pInf, *mirConst);
      return;
    case kConstInt:
      InitIntConst(pInf, *static_cast<MIRIntConst*>(mirConst), dst);
      break;
    case kConstFloatConst:
      InitFloatConst(pInf, *static_cast<MIRFloatConst*>(mirConst), dst);
      break;
    case kConstDoubleConst:
      InitDoubleConst(pInf, *static_cast<MIRDoubleConst*>(mirConst), dst);
      break;
    case kConstAddrof:
      InitAddrofConst(pInf, *static_cast<MIRAddrofConst*>(mirConst), dst);
      break;
    case kConstStrConst:
      InitStrConst(pInf, *static_cast<MIRStrConst*>(mirConst), dst);
      break;
    default:
      MASSERT(false, "Init MIRConst type %d NYI", mirConst->GetKind());
      break;
  }
  UpdateGlobalVarInitAddr(pInf, mirConst->GetType().GetSize());
}

void LmbcMod::InitGlobalVars(void) {
  // alloc mem for global vars 
  this->globals = (uint8*)malloc(this->globalsSize);
  this->unInitPUStatics = (uint8*)malloc(this->unInitPUStaticsSize);
  memset(this->globals, 0, this->globalsSize);
  memset(this->unInitPUStatics, 0, this->unInitPUStaticsSize);

  // init global vars and static vars
  for (const auto it : globalAndStaticVars) {
    VarInf *pInf = it.second;
    if (pInf->sym->IsConst()) {
      InitGlobalVariable(pInf);
    }
  }
}

inline void LmbcMod::AddGlobalVar(MIRSymbol &sym, VarInf *pInf) {
  globalAndStaticVars[sym.GetStIdx().FullIdx()] = pInf;
}

inline void LmbcMod::AddPUStaticVar(PUIdx puIdx, MIRSymbol &sym, VarInf *pInf) {
  globalAndStaticVars[(uint64)puIdx << 32 | sym.GetStIdx().FullIdx()] = pInf;
}

// global var
uint8 *LmbcMod::GetVarAddr(StIdx stIdx) {
  auto it = globalAndStaticVars.find(stIdx.FullIdx());
  MASSERT(it != globalAndStaticVars.end(), "global var not found");
  return globals + it->second->storeIdx; 
}

// PUStatic var
uint8 *LmbcMod::GetVarAddr(PUIdx puIdx, StIdx stIdx) {
  auto it = globalAndStaticVars.find((long)puIdx << 32 | stIdx.FullIdx());
  MASSERT(it != globalAndStaticVars.end(), "PUStatic var not found");
  return globals + it->second->storeIdx; 
}

LmbcFunc*
LmbcMod::LkupLmbcFunc(PUIdx puIdx) {
  auto it = funcMap.find(puIdx);
  return it == funcMap.end()? nullptr: it->second;
}

FuncAddr::FuncAddr(bool lmbcFunc, void *func, std::string name, uint32 formalsAggSz) {
  funcName   = name;
  isLmbcFunc = lmbcFunc;
  formalsAggSize = formalsAggSz;
  if (isLmbcFunc) {
    funcPtr.lmbcFunc = (LmbcFunc*)func;
  } else {
    funcPtr.nativeFunc = func;
  }
}

// Get size total of all func parameters of type TY_agg.
uint32 GetAggFormalsSize(MIRFunction *func) {
  uint32 totalSize = 0;
  MapleVector<FormalDef> &formalDefVec = func->GetFormalDefVec();
  for (int i = 0; i < formalDefVec.size(); i++) {
    MIRType* ty = GlobalTables::GetTypeTable().GetTypeFromTyIdx(formalDefVec[i].formalTyIdx);
    if (ty->GetPrimType() == PTY_agg) {
      totalSize += ty->GetSize();
    }
  }
  return totalSize;
}

FuncAddr* LmbcMod::GetFuncAddr(PUIdx idx) {
  FuncAddr *faddr;
  if (PUIdx2FuncAddr[idx]) {
    return PUIdx2FuncAddr[idx];
  }
  MIRFunction *func = GlobalTables::GetFunctionTable().GetFunctionFromPuidx(idx);
  MASSERT(func, "Function not found in global table");
  if (IsExtFunc(idx, *this)) {
    faddr = new FuncAddr(false, FindExtFunc(idx), func->GetName(), GetAggFormalsSize(func));
  } else {
    faddr = new FuncAddr(true, LkupLmbcFunc(idx), func->GetName());
  }
  PUIdx2FuncAddr[idx] = faddr;
  return faddr;
}


} // namespace maple
