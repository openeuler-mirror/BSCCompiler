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
#include "mir_builder.h"
#include "mir_symbol_builder.h"

namespace maple {
// This is for compiler-generated metadata 1-level struct
void MIRBuilder::AddIntFieldConst(const MIRStructType &sType, MIRAggConst &newConst,
                                  uint32 fieldID, int64 constValue) const {
  auto *fieldConst = GlobalTables::GetIntConstTable().GetOrCreateIntConst(
      static_cast<uint64_t>(constValue), *sType.GetElemType(fieldID - 1));
  newConst.AddItem(fieldConst, fieldID);
}

// This is for compiler-generated metadata 1-level struct
void MIRBuilder::AddAddrofFieldConst(const MIRStructType &structType, MIRAggConst &newConst, uint32 fieldID,
                                     const MIRSymbol &fieldSymbol) {
  AddrofNode *fieldExpr = CreateExprAddrof(0, fieldSymbol, mirModule->GetMemPool());
  auto *fieldConst = mirModule->GetMemPool()->New<MIRAddrofConst>(fieldExpr->GetStIdx(), fieldExpr->GetFieldID(),
                                                                  *structType.GetElemType(fieldID - 1));
  newConst.AddItem(fieldConst, fieldID);
}

// This is for compiler-generated metadata 1-level struct
void MIRBuilder::AddAddroffuncFieldConst(const MIRStructType &structType, MIRAggConst &newConst, uint32 fieldID,
                                         const MIRSymbol &funcSymbol) {
  MIRConst *fieldConst = nullptr;
  MIRFunction *vMethod = funcSymbol.GetFunction();
  if (vMethod->IsAbstract()) {
    fieldConst = GlobalTables::GetIntConstTable().GetOrCreateIntConst(0, *structType.GetElemType(fieldID - 1));
  } else {
    AddroffuncNode *addrofFuncExpr =
        CreateExprAddroffunc(funcSymbol.GetFunction()->GetPuidx(), mirModule->GetMemPool());
    fieldConst = mirModule->GetMemPool()->New<MIRAddroffuncConst>(addrofFuncExpr->GetPUIdx(),
                                                                  *structType.GetElemType(fieldID - 1));
  }
  newConst.AddItem(fieldConst, fieldID);
}

// fieldID is continuously being updated during traversal;
// when the field is found, its field id is returned via fieldID
bool MIRBuilder::TraverseToNamedField(MIRStructType &structType, GStrIdx nameIdx, uint32 &fieldID) {
  TyIdx tid(0);
  return TraverseToNamedFieldWithTypeAndMatchStyle(structType, nameIdx, tid, fieldID, kMatchAnyField);
}

// traverse parent first but match self first.
void MIRBuilder::TraverseToNamedFieldWithType(MIRStructType &structType, GStrIdx nameIdx, TyIdx typeIdx,
                                              uint32 &fieldID, uint32 &idx) {
  if (structType.IsIncomplete()) {
    (void)incompleteTypeRefedSet.insert(structType.GetTypeIndex());
  }
  // process parent
  if (structType.GetKind() == kTypeClass || structType.GetKind() == kTypeClassIncomplete) {
    auto &classType = static_cast<MIRClassType&>(structType);
    MIRType *type = GlobalTables::GetTypeTable().GetTypeFromTyIdx(classType.GetParentTyIdx());
    auto *parentType = static_cast<MIRStructType*>(type);
    if (parentType != nullptr) {
      ++fieldID;
      TraverseToNamedFieldWithType(*parentType, nameIdx, typeIdx, fieldID, idx);
    }
  }
  for (uint32 fieldIdx = 0; fieldIdx < structType.GetFieldsSize(); ++fieldIdx) {
    ++fieldID;
    TyIdx fieldTyIdx = structType.GetFieldsElemt(fieldIdx).second.first;
    MIRType *fieldType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(fieldTyIdx);
    if (structType.GetFieldsElemt(fieldIdx).first == nameIdx) {
      if (typeIdx == 0u || fieldTyIdx == typeIdx) {
        idx = fieldID;
        continue;
      }
      // for pointer type, check their pointed type
      MIRType *type = GlobalTables::GetTypeTable().GetTypeFromTyIdx(typeIdx);
      if (type->IsOfSameType(*fieldType)) {
        idx = fieldID;
      }
    }

    if (fieldType->IsStructType()) {
      auto *subStructType = static_cast<MIRStructType*>(fieldType);
      TraverseToNamedFieldWithType(*subStructType, nameIdx, typeIdx, fieldID, idx);
    }
  }
}

// fieldID is continuously being updated during traversal;
// when the field is found, its field id is returned via fieldID
// typeidx: TyIdx(0) means do not check types.
// matchstyle: 0: do not match but traverse to update fieldID
//             1: match top level field only
//             2: match any field
//             4: traverse parent first
//          0xc: do not match but traverse to update fieldID, traverse parent first, found in child
bool MIRBuilder::TraverseToNamedFieldWithTypeAndMatchStyle(MIRStructType &structType, GStrIdx nameIdx, TyIdx typeIdx,
                                                           uint32 &fieldID, unsigned int matchStyle) {
  if (structType.IsIncomplete()) {
    (void)incompleteTypeRefedSet.insert(structType.GetTypeIndex());
  }
  if ((matchStyle & kParentFirst) != 0) {
    // process parent
    if ((structType.GetKind() != kTypeClass) && (structType.GetKind() != kTypeClassIncomplete)) {
      return false;
    }

    auto &classType = static_cast<MIRClassType&>(structType);
    MIRType *type = GlobalTables::GetTypeTable().GetTypeFromTyIdx(classType.GetParentTyIdx());
    auto *parentType = static_cast<MIRStructType*>(type);
    if (parentType != nullptr) {
      ++fieldID;
      if (matchStyle == (kFoundInChild | kParentFirst | kUpdateFieldID)) {
        matchStyle = kParentFirst;
        uint32 idxBackup = nameIdx;
        nameIdx.reset();
        // do not match but traverse to update fieldID, traverse parent first
        TraverseToNamedFieldWithTypeAndMatchStyle(*parentType, nameIdx, typeIdx, fieldID, matchStyle);
        nameIdx.reset(idxBackup);
      } else if (TraverseToNamedFieldWithTypeAndMatchStyle(*parentType, nameIdx, typeIdx, fieldID, matchStyle)) {
        return true;
      }
    }
  }
  for (uint32 fieldIdx = 0; fieldIdx < structType.GetFieldsSize(); ++fieldIdx) {
    ++fieldID;
    TyIdx fieldTyIdx = structType.GetFieldsElemt(fieldIdx).second.first;
    MIRType *fieldType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(fieldTyIdx);
    ASSERT(fieldType != nullptr, "fieldType is null");
    if (matchStyle != 0 && structType.GetFieldsElemt(fieldIdx).first == nameIdx) {
      if (typeIdx == 0u || fieldTyIdx == typeIdx ||
          fieldType->IsOfSameType(*GlobalTables::GetTypeTable().GetTypeFromTyIdx(typeIdx))) {
        return true;
      }
    }
    unsigned int style = matchStyle & kMatchAnyField;
    if (fieldType->IsStructType()) {
      auto *subStructType = static_cast<MIRStructType*>(fieldType);
      if (TraverseToNamedFieldWithTypeAndMatchStyle(*subStructType, nameIdx, typeIdx, fieldID, style)) {
        return true;
      }
    }
  }
  return false;
}

FieldID MIRBuilder::GetStructFieldIDFromNameAndType(MIRType &type, const std::string &name, TyIdx idx,
                                                    unsigned int matchStyle) {
  auto &structType = static_cast<MIRStructType&>(type);
  uint32 fieldID = 0;
  GStrIdx strIdx = GetStringIndex(name);
  if (TraverseToNamedFieldWithTypeAndMatchStyle(structType, strIdx, idx, fieldID, matchStyle)) {
    return fieldID;
  }
  return 0;
}

FieldID MIRBuilder::GetStructFieldIDFromNameAndType(MIRType &type, const std::string &name, TyIdx idx) {
  return GetStructFieldIDFromNameAndType(type, name, idx, kMatchAnyField);
}

FieldID MIRBuilder::GetStructFieldIDFromNameAndTypeParentFirst(MIRType &type, const std::string &name, TyIdx idx) {
  return GetStructFieldIDFromNameAndType(type, name, idx, kParentFirst);
}

FieldID MIRBuilder::GetStructFieldIDFromNameAndTypeParentFirstFoundInChild(MIRType &type, const std::string &name,
                                                                           TyIdx idx) {
  // do not match but traverse to update fieldID, traverse parent first, found in child
  return GetStructFieldIDFromNameAndType(type, name, idx, kFoundInChild | kParentFirst | kUpdateFieldID);
}

FieldID MIRBuilder::GetStructFieldIDFromFieldName(MIRType &type, const std::string &name) {
  return GetStructFieldIDFromNameAndType(type, name, TyIdx(0), kMatchAnyField);
}

FieldID MIRBuilder::GetStructFieldIDFromFieldNameParentFirst(MIRType *type, const std::string &name) {
  if (type == nullptr) {
    return 0;
  }
  return GetStructFieldIDFromNameAndType(*type, name, TyIdx(0), kParentFirst);
}

void MIRBuilder::SetStructFieldIDFromFieldName(MIRStructType &structType, const std::string &name, GStrIdx newStrIdx,
                                               const MIRType &newFieldType) const {
  uint32 fieldID = 0;
  GStrIdx strIdx = GetStringIndex(name);
  while (true) {
    if (structType.GetElemStrIdx(fieldID) == strIdx) {
      if (newStrIdx != 0u) {
        structType.SetElemStrIdx(fieldID, newStrIdx);
      }
      structType.SetElemtTyIdx(fieldID, newFieldType.GetTypeIndex());
      return;
    }
    ++fieldID;
  }
}

// create a function named str
MIRFunction *MIRBuilder::GetOrCreateFunction(const std::string &str, const TyIdx &retTyIdx, bool *isNewCreated) {
  GStrIdx strIdx = GetStringIndex(str);
  MIRSymbol *funcSt = nullptr;
  if (strIdx != 0u) {
    funcSt = GlobalTables::GetGsymTable().GetSymbolFromStrIdx(strIdx);
    if (funcSt == nullptr) {
      funcSt = CreateSymbol(TyIdx(0), strIdx, kStFunc, kScText, nullptr, kScopeGlobal);
    } else {
      ASSERT(funcSt->GetSKind() == kStFunc, "runtime check error");
      return funcSt->GetFunction();
    }
  } else {
    strIdx = GetOrCreateStringIndex(str);
    funcSt = CreateSymbol(TyIdx(0), strIdx, kStFunc, kScText, nullptr, kScopeGlobal);
  }
  if (isNewCreated != nullptr) {
    *isNewCreated = true;
  }
  auto *fn = mirModule->GetMemPool()->New<MIRFunction>(mirModule, funcSt->GetStIdx());
  fn->SetPuidx(GlobalTables::GetFunctionTable().GetFuncTable().size());
  MIRFuncType funcType;
  funcType.SetRetTyIdx(retTyIdx);
  auto funcTyIdx = GlobalTables::GetTypeTable().GetOrCreateMIRType(&funcType);
  auto *funcTypeInTypeTable = static_cast<MIRFuncType*>(GlobalTables::GetTypeTable().GetTypeFromTyIdx(funcTyIdx));
  fn->SetMIRFuncType(funcTypeInTypeTable);
  fn->SetReturnTyIdx(retTyIdx);
  GlobalTables::GetFunctionTable().GetFuncTable().push_back(fn);
  funcSt->SetFunction(fn);
  funcSt->SetTyIdx(funcTyIdx);
  return fn;
}

MIRFunction *MIRBuilder::GetFunctionFromSymbol(const MIRSymbol &funcSymbol) const {
  ASSERT(funcSymbol.GetSKind() == kStFunc, "Symbol %s is not a function symbol", funcSymbol.GetName().c_str());
  return funcSymbol.GetFunction();
}

MIRFunction *MIRBuilder::GetFunctionFromName(const std::string &str) const {
  auto *funcSymbol =
      GlobalTables::GetGsymTable().GetSymbolFromStrIdx(GlobalTables::GetStrTable().GetStrIdxFromName(str));
  return funcSymbol != nullptr ? GetFunctionFromSymbol(*funcSymbol) : nullptr;
}

MIRFunction *MIRBuilder::GetFunctionFromStidx(const StIdx &stIdx) const {
  auto *funcSymbol = GlobalTables::GetGsymTable().GetSymbolFromStidx(stIdx.Idx());
  return funcSymbol != nullptr ? GetFunctionFromSymbol(*funcSymbol) : nullptr;
}

MIRFunction *MIRBuilder::CreateFunction(const std::string &name, const MIRType &returnType, const ArgVector &arguments,
                                        bool isVarg, bool createBody) const {
  MIRSymbol *funcSymbol = GlobalTables::GetGsymTable().CreateSymbol(kScopeGlobal);
  GStrIdx strIdx = GetOrCreateStringIndex(name);
  funcSymbol->SetNameStrIdx(strIdx);
  if (!GlobalTables::GetGsymTable().AddToStringSymbolMap(*funcSymbol)) {
    return nullptr;
  }
  funcSymbol->SetStorageClass(kScText);
  funcSymbol->SetSKind(kStFunc);
  auto *fn = mirModule->GetMemPool()->New<MIRFunction>(mirModule, funcSymbol->GetStIdx());
  fn->SetPuidx(GlobalTables::GetFunctionTable().GetFuncTable().size());
  GlobalTables::GetFunctionTable().GetFuncTable().push_back(fn);
  std::vector<TyIdx> funcVecType;
  std::vector<TypeAttrs> funcVecAttrs;
  for (size_t i = 0; i < arguments.size(); ++i) {
    MIRType *ty = arguments[i].second;
    FormalDef formalDef(GetOrCreateStringIndex(arguments[i].first.c_str()), nullptr, ty->GetTypeIndex(), TypeAttrs());
    fn->GetFormalDefVec().push_back(formalDef);
    funcVecType.push_back(ty->GetTypeIndex());
    funcVecAttrs.push_back(TypeAttrs());
    if (fn->GetSymTab() != nullptr && formalDef.formalSym != nullptr) {
      (void)fn->GetSymTab()->AddToStringSymbolMap(*formalDef.formalSym);
    }
  }
  FuncAttrs funcAttrs;
  if (isVarg) {
    funcAttrs.SetAttr(FUNCATTR_varargs);
  }
  funcSymbol->SetTyIdx(GlobalTables::GetTypeTable().GetOrCreateFunctionType(
      returnType.GetTypeIndex(), funcVecType, funcVecAttrs, funcAttrs)->GetTypeIndex());
  auto *funcType = static_cast<MIRFuncType*>(funcSymbol->GetType());
  fn->SetMIRFuncType(funcType);
  funcSymbol->SetFunction(fn);
  if (createBody) {
    fn->NewBody();
  }
  return fn;
}

MIRFunction *MIRBuilder::CreateFunction(StIdx stIdx, bool addToTable) const {
  auto *fn = mirModule->GetMemPool()->New<MIRFunction>(mirModule, stIdx);
  fn->SetPuidx(GlobalTables::GetFunctionTable().GetFuncTable().size());
  if (addToTable) {
    GlobalTables::GetFunctionTable().GetFuncTable().push_back(fn);
  }

  auto *funcType = mirModule->GetMemPool()->New<MIRFuncType>();
  fn->SetMIRFuncType(funcType);
  return fn;
}

MIRSymbol *MIRBuilder::GetOrCreateGlobalDecl(const std::string &str, TyIdx tyIdx, bool &created) const {
  GStrIdx strIdx = GetStringIndex(str);
  if (strIdx != 0u) {
    StIdx stIdx = GlobalTables::GetGsymTable().GetStIdxFromStrIdx(strIdx);
    if (stIdx.Idx() != 0) {
      created = false;
      return GlobalTables::GetGsymTable().GetSymbolFromStidx(stIdx.Idx());
    }
  }
  created = true;
  strIdx = GetOrCreateStringIndex(str);
  MIRSymbol *st = GlobalTables::GetGsymTable().CreateSymbol(kScopeGlobal);
  st->SetNameStrIdx(strIdx);
  st->SetTyIdx(tyIdx);
  (void)GlobalTables::GetGsymTable().AddToStringSymbolMap(*st);
  return st;
}

MIRSymbol *MIRBuilder::GetOrCreateLocalDecl(const std::string &str, TyIdx tyIdx, MIRSymbolTable &symbolTable,
                                            bool &created) const {
  GStrIdx strIdx = GetStringIndex(str);
  if (strIdx != 0u) {
    StIdx stIdx = symbolTable.GetStIdxFromStrIdx(strIdx);
    if (stIdx.Idx() != 0) {
      created = false;
      return symbolTable.GetSymbolFromStIdx(stIdx.Idx());
    }
  }
  created = true;
  strIdx = GetOrCreateStringIndex(str);
  MIRSymbol *st = symbolTable.CreateSymbol(kScopeLocal);
  ASSERT(st != nullptr, "null ptr check");
  st->SetNameStrIdx(strIdx);
  st->SetTyIdx(tyIdx);
  (void)symbolTable.AddToStringSymbolMap(*st);
  return st;
}

MIRSymbol *MIRBuilder::GetOrCreateDeclInFunc(const std::string &str, const MIRType &type, MIRFunction &func) const {
  MIRSymbolTable *symbolTable = func.GetSymTab();
  ASSERT(symbolTable != nullptr, "symbol_table is null");
  bool isCreated = false;
  MIRSymbol *st = GetOrCreateLocalDecl(str, type.GetTypeIndex(), *symbolTable, isCreated);
  if (isCreated) {
    st->SetStorageClass(kScAuto);
    st->SetSKind(kStVar);
  }
  return st;
}

MIRSymbol *MIRBuilder::GetOrCreateLocalDecl(const std::string &str, const MIRType &type) {
  MIRFunction *currentFunc = GetCurrentFunction();
  CHECK_FATAL(currentFunc != nullptr, "null ptr check");
  return GetOrCreateDeclInFunc(str, type, *currentFunc);
}

MIRSymbol *MIRBuilder::CreateLocalDecl(const std::string &str, const MIRType &type) const {
  MIRFunction *currentFunctionInner = GetCurrentFunctionNotNull();
  return MIRSymbolBuilder::Instance().CreateLocalDecl(*currentFunctionInner->GetSymTab(),
                                                      GetOrCreateStringIndex(str), type);
}

MIRSymbol *MIRBuilder::GetGlobalDecl(const std::string &str) const {
  return MIRSymbolBuilder::Instance().GetGlobalDecl(GetStringIndex(str));
}

MIRSymbol *MIRBuilder::GetLocalDecl(const std::string &str) const {
  MIRFunction *currentFunctionInner = GetCurrentFunctionNotNull();
  return MIRSymbolBuilder::Instance().GetLocalDecl(*currentFunctionInner->GetSymTab(), GetStringIndex(str));
}

// search the scope hierarchy
MIRSymbol *MIRBuilder::GetDecl(const std::string &str) const {
  GStrIdx strIdx = GetStringIndex(str);
  MIRSymbol *sym = nullptr;
  if (strIdx != 0u) {
    // try to find the decl in local scope first
    MIRFunction *currentFunctionInner = GetCurrentFunction();
    if (currentFunctionInner != nullptr) {
      sym = currentFunctionInner->GetSymTab()->GetSymbolFromStrIdx(strIdx);
    }
    if (sym == nullptr) {
      sym = GlobalTables::GetGsymTable().GetSymbolFromStrIdx(strIdx);
    }
  }
  return sym;
}

MIRSymbol *MIRBuilder::CreateGlobalDecl(const std::string &str, const MIRType &type, MIRStorageClass sc) const {
  return MIRSymbolBuilder::Instance().CreateGlobalDecl(GetOrCreateStringIndex(str), type, sc);
}

MIRSymbol *MIRBuilder::GetOrCreateGlobalDecl(const std::string &str, const MIRType &type) const {
  bool isCreated = false;
  MIRSymbol *st = GetOrCreateGlobalDecl(str, type.GetTypeIndex(), isCreated);
  ASSERT(st != nullptr, "null ptr check");
  if (isCreated) {
    st->SetStorageClass(kScGlobal);
    st->SetSKind(kStVar);
  } else {
    // Existing symbol may come from anther module. We need to register it
    // in the current module so that per-module mpl file is self-sustained.
    mirModule->AddSymbol(st);
  }
  MIRConst *cst = GlobalTables::GetConstPool().GetConstFromPool(st->GetNameStrIdx());
  if (cst != nullptr) {
    st->SetKonst(cst);
  }
  return st;
}

MIRSymbol *MIRBuilder::GetSymbolFromEnclosingScope(StIdx stIdx) const {
  if (stIdx.FullIdx() == 0) {
    return nullptr;
  }
  if (stIdx.Islocal()) {
    MIRFunction *fun = GetCurrentFunctionNotNull();
    MIRSymbol *st = fun->GetSymTab()->GetSymbolFromStIdx(stIdx.Idx());
    if (st != nullptr) {
      return st;
    }
  }
  return GlobalTables::GetGsymTable().GetSymbolFromStidx(stIdx.Idx());
}

MIRSymbol *MIRBuilder::GetSymbol(TyIdx tyIdx, const std::string &name, MIRSymKind mClass, MIRStorageClass sClass,
                                 uint8 scpID, bool sameType = false) const {
  return GetSymbol(tyIdx, GetOrCreateStringIndex(name), mClass, sClass, scpID, sameType);
}

// when sametype is true, it means match everything the of the symbol
MIRSymbol *MIRBuilder::GetSymbol(TyIdx tyIdx, GStrIdx strIdx, MIRSymKind mClass, MIRStorageClass sClass,
                                 uint8 scpID, bool sameType = false) const {
  if (scpID != kScopeGlobal) {
    ERR(kLncErr, "not yet implemented");
    return nullptr;
  }
  return MIRSymbolBuilder::Instance().GetSymbol(tyIdx, strIdx, mClass, sClass, sameType);
}

MIRSymbol *MIRBuilder::GetOrCreateSymbol(TyIdx tyIdx, const std::string &name, MIRSymKind mClass,
                                         MIRStorageClass sClass, MIRFunction *func, uint8 scpID,
                                         bool sametype = false) const {
  return GetOrCreateSymbol(tyIdx, GetOrCreateStringIndex(name), mClass, sClass, func, scpID, sametype);
}

MIRSymbol *MIRBuilder::GetOrCreateSymbol(TyIdx tyIdx, GStrIdx strIdx, MIRSymKind mClass, MIRStorageClass sClass,
                                         MIRFunction *func, uint8 scpID, bool sameType = false) const {
  if (MIRSymbol *st = GetSymbol(tyIdx, strIdx, mClass, sClass, scpID, sameType)) {
    return st;
  }
  return CreateSymbol(tyIdx, strIdx, mClass, sClass, func, scpID);
}

// when func is null, create global symbol, otherwise create local symbol
MIRSymbol *MIRBuilder::CreateSymbol(TyIdx tyIdx, const std::string &name, MIRSymKind mClass, MIRStorageClass sClass,
                                    MIRFunction *func, uint8 scpID) const {
  return CreateSymbol(tyIdx, GetOrCreateStringIndex(name), mClass, sClass, func, scpID);
}

// when func is null, create global symbol, otherwise create local symbol
MIRSymbol *MIRBuilder::CreateSymbol(TyIdx tyIdx, GStrIdx strIdx, MIRSymKind mClass, MIRStorageClass sClass,
                                    MIRFunction *func, uint8 scpID) const {
  return MIRSymbolBuilder::Instance().CreateSymbol(tyIdx, strIdx, mClass, sClass, func, scpID);
}

MIRSymbol *MIRBuilder::CreateConstStringSymbol(const std::string &symbolName, const std::string &content) {
  auto elemPrimType = PTY_u8;
  MIRType *type = GlobalTables::GetTypeTable().GetPrimType(elemPrimType);
  uint32 sizeIn = static_cast<uint32>(content.length());
  MIRType *arrayTypeWithSize = GlobalTables::GetTypeTable().GetOrCreateArrayType(
      *GlobalTables::GetTypeTable().GetPrimType(elemPrimType), 1, &sizeIn);

  if (GetLocalDecl(symbolName)) {
    return GetLocalDecl(symbolName);
  }
  MIRSymbol *arrayVar = GetOrCreateGlobalDecl(symbolName, *arrayTypeWithSize);
  arrayVar->SetAttr(ATTR_readonly);
  arrayVar->SetStorageClass(kScFstatic);
  MIRAggConst *val = mirModule->GetMemPool()->New<MIRAggConst>(*mirModule, *arrayTypeWithSize);
  for (uint32 i = 0; i < sizeIn; ++i) {
    MIRConst *cst = mirModule->GetMemPool()->New<MIRIntConst>(content[i], *type);
    val->PushBack(cst);
  }
  // This interface is only for string literal, 0 is added to the end of the string.
  MIRConst *cst0 = mirModule->GetMemPool()->New<MIRIntConst>(0, *type);
  val->PushBack(cst0);
  arrayVar->SetKonst(val);
  return arrayVar;
}

MIRSymbol *MIRBuilder::CreatePregFormalSymbol(TyIdx tyIdx, PregIdx pRegIdx, MIRFunction &func) const {
  return MIRSymbolBuilder::Instance().CreatePregFormalSymbol(tyIdx, pRegIdx, func);
}

ConstvalNode *MIRBuilder::CreateConstval(MIRConst *mirConst) {
  return GetCurrentFuncCodeMp()->New<ConstvalNode>(mirConst->GetType().GetPrimType(), mirConst);
}

ConstvalNode *MIRBuilder::CreateInt128Const(const Int128ElemTy *value, PrimType pty) {
  ASSERT(IsInt128Ty(pty), "unxecpected prim type");
  IntVal intVal(value, pty);
  auto *mirConst =
      GlobalTables::GetIntConstTable().GetOrCreateIntConst(intVal, *GlobalTables::GetTypeTable().GetPrimType(pty));
  return GetCurrentFuncCodeMp()->New<ConstvalNode>(pty, mirConst);
}

ConstvalNode *MIRBuilder::CreateIntConst(uint64 val, PrimType pty) {
  auto *mirConst =
      GlobalTables::GetIntConstTable().GetOrCreateIntConst(val, *GlobalTables::GetTypeTable().GetPrimType(pty));
  return GetCurrentFuncCodeMp()->New<ConstvalNode>(pty, mirConst);
}

ConstvalNode *MIRBuilder::CreateFloatConst(float val) {
  auto *mirConst = GetCurrentFuncDataMp()->New<MIRFloatConst>(
      val, *GlobalTables::GetTypeTable().GetPrimType(PTY_f32));
  return GetCurrentFuncCodeMp()->New<ConstvalNode>(PTY_f32, mirConst);
}

ConstvalNode *MIRBuilder::CreateDoubleConst(double val) {
  auto *mirConst = GetCurrentFuncDataMp()->New<MIRDoubleConst>(
      val, *GlobalTables::GetTypeTable().GetPrimType(PTY_f64));
  return GetCurrentFuncCodeMp()->New<ConstvalNode>(PTY_f64, mirConst);
}

ConstvalNode *MIRBuilder::CreateFloat128Const(const uint64 *val) {
  auto *mirConst = GetCurrentFuncDataMp()->New<MIRFloat128Const>(
      val, *GlobalTables::GetTypeTable().GetPrimType(PTY_f128));
  return GetCurrentFuncCodeMp()->New<ConstvalNode>(PTY_f128, mirConst);
}

ConstvalNode *MIRBuilder::GetConstInt(MemPool &memPool, int val) const {
  auto *mirConst = GlobalTables::GetIntConstTable().GetOrCreateIntConst(
      static_cast<uint64_t>(val), *GlobalTables::GetTypeTable().GetInt64());
  return memPool.New<ConstvalNode>(PTY_i32, mirConst);
}

ConstvalNode *MIRBuilder::CreateAddrofConst(BaseNode &node) {
  ASSERT(node.GetOpCode() == OP_addrof, "illegal op for addrof const");
  MIRFunction *currentFunctionInner = GetCurrentFunctionNotNull();

  // determine the type of 'node' and create a pointer type, accordingly
  auto &aNode = static_cast<AddrofNode&>(node);
  const MIRSymbol *var = currentFunctionInner->GetLocalOrGlobalSymbol(aNode.GetStIdx());
  TyIdx ptyIdx = var->GetTyIdx();
  MIRPtrType ptrType(ptyIdx);
  ptyIdx = GlobalTables::GetTypeTable().GetOrCreateMIRType(&ptrType);
  MIRType &exprType = *GlobalTables::GetTypeTable().GetTypeFromTyIdx(ptyIdx);
  auto *temp = mirModule->GetMemPool()->New<MIRAddrofConst>(aNode.GetStIdx(), aNode.GetFieldID(), exprType);
  return GetCurrentFuncCodeMp()->New<ConstvalNode>(PTY_ptr, temp);
}

ConstvalNode *MIRBuilder::CreateAddroffuncConst(const BaseNode &node) {
  ASSERT(node.GetOpCode() == OP_addroffunc, "illegal op for addroffunc const");

  const auto &aNode = static_cast<const AddroffuncNode&>(node);
  MIRFunction *f = GlobalTables::GetFunctionTable().GetFunctionFromPuidx(aNode.GetPUIdx());
  TyIdx ptyIdx = f->GetFuncSymbol()->GetTyIdx();
  MIRPtrType ptrType(ptyIdx);
  ptyIdx = GlobalTables::GetTypeTable().GetOrCreateMIRType(&ptrType);
  MIRType *exprType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(ptyIdx);
  auto *mirConst = mirModule->GetMemPool()->New<MIRAddroffuncConst>(aNode.GetPUIdx(), *exprType);
  return GetCurrentFuncCodeMp()->New<ConstvalNode>(PTY_ptr, mirConst);
}

ConstvalNode *MIRBuilder::CreateStrConst(const BaseNode &node) {
  ASSERT(node.GetOpCode() == OP_conststr, "illegal op for conststr const");
  UStrIdx strIdx = static_cast<const ConststrNode&>(node).GetStrIdx();
  CHECK_FATAL(GlobalTables::GetTypeTable().GetTypeTable().size() > PTY_u8,
              "index is out of range in MIRBuilder::CreateStrConst");
  TyIdx tyIdx = GlobalTables::GetTypeTable().GetTypeFromTyIdx(TyIdx(PTY_u8))->GetTypeIndex();
  MIRPtrType ptrType(tyIdx);
  tyIdx = GlobalTables::GetTypeTable().GetOrCreateMIRType(&ptrType);
  MIRType *exprType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(tyIdx);
  auto *mirConst = mirModule->GetMemPool()->New<MIRStrConst>(strIdx, *exprType);
  return GetCurrentFuncCodeMp()->New<ConstvalNode>(PTY_ptr, mirConst);
}

ConstvalNode *MIRBuilder::CreateStr16Const(const BaseNode &node) {
  ASSERT(node.GetOpCode() == OP_conststr16, "illegal op for conststr16 const");
  U16StrIdx strIdx = static_cast<const Conststr16Node&>(node).GetStrIdx();
  CHECK_FATAL(GlobalTables::GetTypeTable().GetTypeTable().size() > PTY_u16,
              "index out of range in MIRBuilder::CreateStr16Const");
  TyIdx ptyIdx = GlobalTables::GetTypeTable().GetTypeFromTyIdx(TyIdx(PTY_u16))->GetTypeIndex();
  MIRPtrType ptrType(ptyIdx);
  ptyIdx = GlobalTables::GetTypeTable().GetOrCreateMIRType(&ptrType);
  MIRType *exprType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(ptyIdx);
  auto *mirConst = mirModule->GetMemPool()->New<MIRStr16Const>(strIdx, *exprType);
  return GetCurrentFuncCodeMp()->New<ConstvalNode>(PTY_ptr, mirConst);
}

SizeoftypeNode *MIRBuilder::CreateExprSizeoftype(const MIRType &type) {
  return GetCurrentFuncCodeMp()->New<SizeoftypeNode>(PTY_u32, type.GetTypeIndex());
}

FieldsDistNode *MIRBuilder::CreateExprFieldsDist(const MIRType &type, FieldID field1, FieldID field2) {
  return GetCurrentFuncCodeMp()->New<FieldsDistNode>(PTY_i32, type.GetTypeIndex(), field1, field2);
}

AddrofNode *MIRBuilder::CreateExprAddrof(FieldID fieldID, const MIRSymbol &symbol, MemPool *memPool) {
  return CreateExprAddrof(fieldID, symbol.GetStIdx(), memPool);
}

AddrofNode *MIRBuilder::CreateExprAddrof(FieldID fieldID, StIdx symbolStIdx, MemPool *memPool) {
  if (memPool == nullptr) {
    memPool = GetCurrentFuncCodeMp();
  }
  return memPool->New<AddrofNode>(OP_addrof, PTY_ptr, symbolStIdx, fieldID);
}

AddroffuncNode *MIRBuilder::CreateExprAddroffunc(PUIdx puIdx, MemPool *memPool) {
  if (memPool == nullptr) {
    memPool = GetCurrentFuncCodeMp();
  }
  return memPool->New<AddroffuncNode>(PTY_ptr, puIdx);
}

AddrofNode *MIRBuilder::CreateExprDread(const MIRType &type, FieldID fieldID, const MIRSymbol &symbol) {
  return CreateExprDread(type.GetPrimType(), fieldID, symbol);
}

AddrofNode *MIRBuilder::CreateExprDread(PrimType ptyp, FieldID fieldID, const MIRSymbol &symbol) {
  auto *node = GetCurrentFuncCodeMp()->New<AddrofNode>(OP_dread, kPtyInvalid, symbol.GetStIdx(), fieldID);
  node->SetPrimType(GetRegPrimType(ptyp));
  return node;
}

RegreadNode *MIRBuilder::CreateExprRegread(PrimType pty, PregIdx regIdx) {
  return GetCurrentFuncCodeMp()->New<RegreadNode>(pty, regIdx);
}

AddrofNode *MIRBuilder::CreateExprDread(MIRType &type, MIRSymbol &symbol) {
  return CreateExprDread(type, 0, symbol);
}

AddrofNode *MIRBuilder::CreateExprDread(MIRSymbol &symbol, uint16 fieldID) {
  if (fieldID == 0) {
    return CreateExprDread(symbol);
  }
  ASSERT(false, "NYI");
  return nullptr;
}

AddrofNode *MIRBuilder::CreateExprDread(MIRSymbol &symbol) {
  return CreateExprDread(*symbol.GetType(), 0, symbol);
}

AddrofNode *MIRBuilder::CreateExprDread(PregIdx pregID, PrimType pty) {
  auto *dread = GetCurrentFuncCodeMp()->New<AddrofNode>(OP_dread, pty);
  dread->SetStFullIdx(pregID);
  return dread;
}

DreadoffNode *MIRBuilder::CreateExprDreadoff(Opcode op, PrimType pty, const MIRSymbol &symbol, int32 offset) {
  DreadoffNode *node = GetCurrentFuncCodeMp()->New<DreadoffNode>(op, pty);
  node->stIdx = symbol.GetStIdx();
  node->offset = offset;
  return node;
}

IreadNode *MIRBuilder::CreateExprIread(PrimType primType, TyIdx ptrTypeIdx, FieldID fieldID, BaseNode *addr) {
  return GetCurrentFuncCodeMp()->New<IreadNode>(OP_iread, GetRegPrimType(primType), ptrTypeIdx, fieldID, addr);
}

IreadNode *MIRBuilder::CreateExprIread(const MIRType &returnType, const MIRType &ptrType, FieldID fieldID,
                                       BaseNode *addr) {
  TyIdx returnTypeIdx = returnType.GetTypeIndex();
  CHECK(returnTypeIdx < GlobalTables::GetTypeTable().GetTypeTable().size(),
        "index out of range in MIRBuilder::CreateExprIread");
  ASSERT(fieldID != 0 || ptrType.GetPrimType() != PTY_agg,
         "Error: Fieldid should not be 0 when trying to iread a field from type ");
  return CreateExprIread(returnType.GetPrimType(), ptrType.GetTypeIndex(), fieldID, addr);
}

IreadoffNode *MIRBuilder::CreateExprIreadoff(PrimType pty, int32 offset, BaseNode *opnd0) {
  return GetCurrentFuncCodeMp()->New<IreadoffNode>(pty, opnd0, offset);
}

IreadFPoffNode *MIRBuilder::CreateExprIreadFPoff(PrimType pty, int32 offset) {
  return GetCurrentFuncCodeMp()->New<IreadFPoffNode>(pty, offset);
}

IaddrofNode *MIRBuilder::CreateExprIaddrof(const MIRType &returnType, const MIRType &ptrType, FieldID fieldID,
                                           BaseNode *addr) {
  IaddrofNode *iAddrOfNode = CreateExprIread(returnType, ptrType, fieldID, addr);
  iAddrOfNode->SetOpCode(OP_iaddrof);
  return iAddrOfNode;
}

IaddrofNode *MIRBuilder::CreateExprIaddrof(PrimType returnTypePty, TyIdx ptrTypeIdx, FieldID fieldID, BaseNode *addr) {
  return GetCurrentFuncCodeMp()->New<IreadNode>(OP_iaddrof, returnTypePty, ptrTypeIdx, fieldID, addr);
}

UnaryNode *MIRBuilder::CreateExprUnary(Opcode opcode, const MIRType &type, BaseNode *opnd) {
  return GetCurrentFuncCodeMp()->New<UnaryNode>(opcode, type.GetPrimType(), opnd);
}

GCMallocNode *MIRBuilder::CreateExprGCMalloc(Opcode opcode, const MIRType &pType, const MIRType &type) {
  return GetCurrentFuncCodeMp()->New<GCMallocNode>(opcode, pType.GetPrimType(), type.GetTypeIndex());
}

JarrayMallocNode *MIRBuilder::CreateExprJarrayMalloc(Opcode opcode, const MIRType &pType, const MIRType &type,
                                                     BaseNode *opnd) {
  return GetCurrentFuncCodeMp()->New<JarrayMallocNode>(opcode, pType.GetPrimType(), type.GetTypeIndex(), opnd);
}

TypeCvtNode *MIRBuilder::CreateExprTypeCvt(Opcode o, PrimType  toPrimType, PrimType fromPrimType, BaseNode &opnd) {
  return GetCurrentFuncCodeMp()->New<TypeCvtNode>(o, toPrimType, fromPrimType, &opnd);
}

TypeCvtNode *MIRBuilder::CreateExprTypeCvt(Opcode o, const MIRType &type, const MIRType &fromType, BaseNode *opnd) {
  return CreateExprTypeCvt(o, type.GetPrimType(), fromType.GetPrimType(), *opnd);
}

ExtractbitsNode *MIRBuilder::CreateExprExtractbits(Opcode o, const MIRType &type, uint32 bOffset, uint32 bSize,
                                                   BaseNode *opnd) {
  return CreateExprExtractbits(o, type.GetPrimType(), bOffset, bSize, opnd);
}

ExtractbitsNode *MIRBuilder::CreateExprExtractbits(Opcode o, PrimType type, uint32 bOffset, uint32 bSize,
                                                   BaseNode *opnd) {
  return GetCurrentFuncCodeMp()->New<ExtractbitsNode>(o, type, bOffset, bSize, opnd);
}

DepositbitsNode *MIRBuilder::CreateExprDepositbits(Opcode o, PrimType type, uint32 bOffset, uint32 bSize,
                                                   BaseNode *leftOpnd, BaseNode* rightOpnd) {
  return GetCurrentFuncCodeMp()->New<DepositbitsNode>(o, type, bOffset, bSize, leftOpnd, rightOpnd);
}

RetypeNode *MIRBuilder::CreateExprRetype(const MIRType &type, const MIRType &fromType, BaseNode *opnd) {
  return CreateExprRetype(type, fromType.GetPrimType(), opnd);
}

RetypeNode *MIRBuilder::CreateExprRetype(const MIRType &type, PrimType fromType, BaseNode *opnd) {
  return GetCurrentFuncCodeMp()->New<RetypeNode>(type.GetPrimType(), fromType, type.GetTypeIndex(), opnd);
}

BinaryNode *MIRBuilder::CreateExprBinary(Opcode opcode, const MIRType &type, BaseNode *opnd0, BaseNode *opnd1) {
  return GetCurrentFuncCodeMp()->New<BinaryNode>(opcode, type.GetPrimType(), opnd0, opnd1);
}

TernaryNode *MIRBuilder::CreateExprTernary(Opcode opcode, const MIRType &type, BaseNode *opnd0, BaseNode *opnd1,
                                           BaseNode *opnd2) {
  return GetCurrentFuncCodeMp()->New<TernaryNode>(opcode, type.GetPrimType(), opnd0, opnd1, opnd2);
}

CompareNode *MIRBuilder::CreateExprCompare(Opcode opcode, const MIRType &type, const MIRType &opndType, BaseNode *opnd0,
                                           BaseNode *opnd1) {
  return GetCurrentFuncCodeMp()->New<CompareNode>(opcode, type.GetPrimType(), opndType.GetPrimType(), opnd0, opnd1);
}

ArrayNode *MIRBuilder::CreateExprArray(const MIRType &arrayType) {
  MIRType *addrType = GlobalTables::GetTypeTable().GetOrCreatePointerType(arrayType);
  ASSERT(addrType != nullptr, "addrType is null");
  auto *arrayNode = GetCurrentFuncCodeMp()->New<ArrayNode>(*GetCurrentFuncCodeMpAllocator(),
                                                           addrType->GetPrimType(), addrType->GetTypeIndex());
  arrayNode->SetNumOpnds(0);
  return arrayNode;
}

ArrayNode *MIRBuilder::CreateExprArray(const MIRType &arrayType, BaseNode *op) {
  ArrayNode *arrayNode = CreateExprArray(arrayType);
  arrayNode->GetNopnd().push_back(op);
  arrayNode->SetNumOpnds(1);
  return arrayNode;
}

ArrayNode *MIRBuilder::CreateExprArray(const MIRType &arrayType, BaseNode *op1, BaseNode *op2) {
  ArrayNode *arrayNode = CreateExprArray(arrayType, op1);
  arrayNode->GetNopnd().push_back(op2);
  arrayNode->SetNumOpnds(2);
  return arrayNode;
}

ArrayNode *MIRBuilder::CreateExprArray(const MIRType &arrayType, std::vector<BaseNode *> ops) {
  MIRType *addrType = GlobalTables::GetTypeTable().GetOrCreatePointerType(arrayType);
  ASSERT(addrType != nullptr, "addrType is null");
  auto *arrayNode = GetCurrentFuncCodeMp()->New<ArrayNode>(*GetCurrentFuncCodeMpAllocator(),
                                                           addrType->GetPrimType(), addrType->GetTypeIndex());
  arrayNode->GetNopnd().insert(arrayNode->GetNopnd().begin(), ops.begin(), ops.end());
  arrayNode->SetNumOpnds(static_cast<uint8>(ops.size()));
  return arrayNode;
}

IntrinsicopNode *MIRBuilder::CreateExprIntrinsicop(MIRIntrinsicID id, Opcode op, PrimType primType, const TyIdx &tyIdx,
                                                   const MapleVector<BaseNode*> &ops) {
  auto *expr = GetCurrentFuncCodeMp()->New<IntrinsicopNode>(*GetCurrentFuncCodeMpAllocator(), op, primType);
  expr->SetIntrinsic(id);
  expr->SetNOpnd(ops);
  expr->SetNumOpnds(ops.size());
  if (op == OP_intrinsicopwithtype) {
    expr->SetTyIdx(tyIdx);
  }
  return expr;
}

IntrinsicopNode *MIRBuilder::CreateExprIntrinsicop(MIRIntrinsicID idx, Opcode opCode, const MIRType &type,
                                                   const MapleVector<BaseNode*> &ops) {
  return CreateExprIntrinsicop(idx, opCode, type.GetPrimType(), type.GetTypeIndex(), ops);
}

DassignNode *MIRBuilder::CreateStmtDassign(const MIRSymbol &symbol, FieldID fieldID, BaseNode *src) {
  return GetCurrentFuncCodeMp()->New<DassignNode>(src, symbol.GetStIdx(), fieldID);
}

RegassignNode *MIRBuilder::CreateStmtRegassign(PrimType pty, PregIdx regIdx, BaseNode *src) {
  return GetCurrentFuncCodeMp()->New<RegassignNode>(pty, regIdx, src);
}

DassignNode *MIRBuilder::CreateStmtDassign(StIdx sIdx, FieldID fieldID, BaseNode *src) {
  return GetCurrentFuncCodeMp()->New<DassignNode>(src, sIdx, fieldID);
}

IassignNode *MIRBuilder::CreateStmtIassign(const MIRType &type, FieldID fieldID, BaseNode *addr, BaseNode *src) {
  return GetCurrentFuncCodeMp()->New<IassignNode>(type.GetTypeIndex(), fieldID, addr, src);
}

IassignoffNode *MIRBuilder::CreateStmtIassignoff(PrimType pty, int32 offset, BaseNode *addr, BaseNode *src) {
  return GetCurrentFuncCodeMp()->New<IassignoffNode>(pty, offset, addr, src);
}

IassignFPoffNode *MIRBuilder::CreateStmtIassignFPoff(Opcode op, PrimType pty,
                                                     int32 offset, BaseNode *src) {
  return GetCurrentFuncCodeMp()->New<IassignFPoffNode>(op, pty, offset, src);
}

CallNode *MIRBuilder::CreateStmtCall(PUIdx puIdx, const MapleVector<BaseNode*> &args, Opcode opCode) {
  auto *stmt = GetCurrentFuncCodeMp()->New<CallNode>(*GetCurrentFuncCodeMpAllocator(), opCode, puIdx, TyIdx());
  stmt->SetNOpnd(args);
  stmt->SetNumOpnds(args.size());
  return stmt;
}

CallNode *MIRBuilder::CreateStmtCall(const std::string &callee, const MapleVector<BaseNode*> &args) {
  GStrIdx strIdx = GlobalTables::GetStrTable().GetStrIdxFromName(callee);
  StIdx stIdx = GlobalTables::GetGsymTable().GetStIdxFromStrIdx(strIdx);
  MIRSymbol *st = GlobalTables::GetGsymTable().GetSymbolFromStidx(stIdx.Idx());
  ASSERT(st != nullptr, "MIRSymbol st is null");
  MIRFunction *func = st->GetFunction();
  return CreateStmtCall(func->GetPuidx(), args, OP_call);
}

IcallNode *MIRBuilder::CreateStmtIcall(const MapleVector<BaseNode*> &args) {
  auto *stmt = GetCurrentFuncCodeMp()->New<IcallNode>(*GetCurrentFuncCodeMpAllocator(), OP_icall);
  ASSERT(stmt != nullptr, "stmt is null");
  stmt->SetOpnds(args);
  return stmt;
}

IcallNode *MIRBuilder::CreateStmtIcallproto(const MapleVector<BaseNode*> &args, const TyIdx &prototypeIdx) {
  auto *stmt = GetCurrentFuncCodeMp()->New<IcallNode>(*GetCurrentFuncCodeMpAllocator(), OP_icallproto);
  ASSERT(stmt != nullptr, "stmt is null");
  stmt->SetOpnds(args);
  stmt->SetRetTyIdx(prototypeIdx);
  return stmt;
}

IcallNode *MIRBuilder::CreateStmtIcallAssigned(const MapleVector<BaseNode*> &args, const MIRSymbol &ret) {
  auto *stmt = GetCurrentFuncCodeMp()->New<IcallNode>(*GetCurrentFuncCodeMpAllocator(), OP_icallassigned);
  CallReturnVector nrets(GetCurrentFuncCodeMpAllocator()->Adapter());
  CHECK_FATAL((ret.GetStorageClass() == kScAuto || ret.GetStorageClass() == kScFormal ||
               ret.GetStorageClass() == kScExtern || ret.GetStorageClass() == kScGlobal),
              "unknown classtype! check it!");
  nrets.emplace_back(CallReturnPair(ret.GetStIdx(), RegFieldPair(0, 0)));
  stmt->SetNumOpnds(args.size());
  stmt->GetNopnd().resize(stmt->GetNumOpnds());
  stmt->SetReturnVec(nrets);
  for (size_t i = 0; i < stmt->GetNopndSize(); ++i) {
    stmt->SetNOpndAt(i, args.at(i));
  }
  stmt->SetRetTyIdx(ret.GetTyIdx());
  return stmt;
}

IcallNode *MIRBuilder::CreateStmtIcallprotoAssigned(const MapleVector<BaseNode*> &args, const MIRSymbol &ret,
                                                    const TyIdx &prototypeIdx) {
  auto *stmt = GetCurrentFuncCodeMp()->New<IcallNode>(*GetCurrentFuncCodeMpAllocator(), OP_icallprotoassigned);
  CallReturnVector nrets(GetCurrentFuncCodeMpAllocator()->Adapter());
  CHECK_FATAL((ret.GetStorageClass() == kScAuto || ret.GetStorageClass() == kScFormal ||
               ret.GetStorageClass() == kScExtern || ret.GetStorageClass() == kScGlobal),
              "unknown classtype! check it!");
  nrets.emplace_back(CallReturnPair(ret.GetStIdx(), RegFieldPair(0, 0)));
  stmt->SetNumOpnds(static_cast<uint8>(args.size()));
  stmt->GetNopnd().resize(stmt->GetNumOpnds());
  stmt->SetReturnVec(nrets);
  for (size_t i = 0; i < stmt->GetNopndSize(); ++i) {
    stmt->SetNOpndAt(i, args.at(i));
  }
  stmt->SetRetTyIdx(prototypeIdx);
  return stmt;
}

IntrinsiccallNode *MIRBuilder::CreateStmtIntrinsicCall(MIRIntrinsicID idx, const MapleVector<BaseNode*> &arguments,
                                                       TyIdx tyIdx) {
  auto *stmt = GetCurrentFuncCodeMp()->New<IntrinsiccallNode>(
      *GetCurrentFuncCodeMpAllocator(), tyIdx == 0u ? OP_intrinsiccall : OP_intrinsiccallwithtype, idx);
  stmt->SetTyIdx(tyIdx);
  stmt->SetOpnds(arguments);
  return stmt;
}

IntrinsiccallNode *MIRBuilder::CreateStmtXintrinsicCall(MIRIntrinsicID idx, const MapleVector<BaseNode*> &arguments) {
  auto *stmt =
      GetCurrentFuncCodeMp()->New<IntrinsiccallNode>(*GetCurrentFuncCodeMpAllocator(), OP_xintrinsiccall, idx);
  ASSERT(stmt != nullptr, "stmt is null");
  stmt->SetOpnds(arguments);
  return stmt;
}

CallNode *MIRBuilder::CreateStmtCallAssigned(PUIdx puIdx, const MIRSymbol *ret, Opcode op) {
  auto *stmt = GetCurrentFuncCodeMp()->New<CallNode>(*GetCurrentFuncCodeMpAllocator(), op, puIdx);
  if (ret) {
    ASSERT(ret->IsLocal(), "Not Excepted ret");
    stmt->GetReturnVec().push_back(CallReturnPair(ret->GetStIdx(), RegFieldPair(0, 0)));
  }
  return stmt;
}

CallNode *MIRBuilder::CreateStmtCallAssigned(PUIdx puIdx, const MapleVector<BaseNode*> &args, const MIRSymbol *ret,
                                             Opcode opcode, TyIdx tyIdx) {
  auto *stmt = GetCurrentFuncCodeMp()->New<CallNode>(*GetCurrentFuncCodeMpAllocator(), opcode, puIdx, tyIdx);
  ASSERT(stmt != nullptr, "stmt is null");
  stmt->SetOpnds(args);
  if (ret != nullptr) {
    ASSERT(ret->IsLocal(), "Not Excepted ret");
    stmt->GetReturnVec().push_back(CallReturnPair(ret->GetStIdx(), RegFieldPair(0, 0)));
  }
  return stmt;
}

CallNode *MIRBuilder::CreateStmtCallRegassigned(PUIdx puIdx, PregIdx pRegIdx, Opcode opcode, BaseNode *arg) {
  auto *stmt = GetCurrentFuncCodeMp()->New<CallNode>(*GetCurrentFuncCodeMpAllocator(), opcode, puIdx);
  stmt->GetNopnd().push_back(arg);
  stmt->SetNumOpnds(stmt->GetNopndSize());
  if (pRegIdx > 0) {
    stmt->GetReturnVec().push_back(CallReturnPair(StIdx(), RegFieldPair(0, pRegIdx)));
  }
  return stmt;
}

CallNode *MIRBuilder::CreateStmtCallRegassigned(PUIdx puIdx, const MapleVector<BaseNode*> &args, PregIdx pRegIdx,
                                                Opcode opcode) {
  auto *stmt = GetCurrentFuncCodeMp()->New<CallNode>(*GetCurrentFuncCodeMpAllocator(), opcode, puIdx);
  ASSERT(stmt != nullptr, "stmt is null");
  stmt->SetOpnds(args);
  if (pRegIdx > 0) {
    stmt->GetReturnVec().push_back(CallReturnPair(StIdx(), RegFieldPair(0, pRegIdx)));
  }
  return stmt;
}

IntrinsiccallNode *MIRBuilder::CreateStmtIntrinsicCallAssigned(MIRIntrinsicID idx, const MapleVector<BaseNode*> &args,
                                                               PregIdx retPregIdx) {
  auto *stmt =
      GetCurrentFuncCodeMp()->New<IntrinsiccallNode>(*GetCurrentFuncCodeMpAllocator(), OP_intrinsiccallassigned, idx);
  ASSERT(stmt != nullptr, "stmt is null");
  stmt->SetOpnds(args);
  if (retPregIdx > 0) {
    stmt->GetReturnVec().push_back(CallReturnPair(StIdx(), RegFieldPair(0, retPregIdx)));
  }
  return stmt;
}

IntrinsiccallNode *MIRBuilder::CreateStmtIntrinsicCallAssigned(MIRIntrinsicID idx, const MapleVector<BaseNode*> &args,
                                                               const MIRSymbol *ret, TyIdx tyIdx) {
  auto *stmt = GetCurrentFuncCodeMp()->New<IntrinsiccallNode>(
      *GetCurrentFuncCodeMpAllocator(), tyIdx == 0u ? OP_intrinsiccallassigned : OP_intrinsiccallwithtypeassigned, idx);
  stmt->SetTyIdx(tyIdx);
  stmt->SetOpnds(args);
  CallReturnVector nrets(GetCurrentFuncCodeMpAllocator()->Adapter());
  if (ret != nullptr) {
    ASSERT(ret->IsLocal(), "Not Excepted ret");
    nrets.push_back(CallReturnPair(ret->GetStIdx(), RegFieldPair(0, 0)));
  }
  stmt->SetReturnVec(nrets);
  return stmt;
}

IntrinsiccallNode *MIRBuilder::CreateStmtXintrinsicCallAssigned(MIRIntrinsicID idx, const MapleVector<BaseNode*> &args,
                                                                const MIRSymbol *ret) {
  auto *stmt =
      GetCurrentFuncCodeMp()->New<IntrinsiccallNode>(*GetCurrentFuncCodeMpAllocator(), OP_xintrinsiccallassigned, idx);
  ASSERT(stmt != nullptr, "stmt is null");
  stmt->SetOpnds(args);
  CallReturnVector nrets(GetCurrentFuncCodeMpAllocator()->Adapter());
  if (ret != nullptr) {
    ASSERT(ret->IsLocal(), "Not Excepted ret");
    nrets.push_back(CallReturnPair(ret->GetStIdx(), RegFieldPair(0, 0)));
  }
  stmt->SetReturnVec(nrets);
  return stmt;
}

NaryStmtNode *MIRBuilder::CreateStmtReturn(BaseNode *rVal) {
  return CreateStmtNary(OP_return, rVal);
}

NaryStmtNode *MIRBuilder::CreateStmtNary(Opcode op, const MapleVector<BaseNode*> &rVals) {
  auto *stmt = GetCurrentFuncCodeMp()->New<NaryStmtNode>(*GetCurrentFuncCodeMpAllocator(), op);
  ASSERT(stmt != nullptr, "stmt is null");
  stmt->SetOpnds(rVals);
  return stmt;
}

CallAssertBoundaryStmtNode *MIRBuilder::CreateStmtCallAssertBoundary(Opcode op, const MapleVector<BaseNode*> &rVals,
                                                                     GStrIdx funcNameIdx, size_t paramIndex,
                                                                     GStrIdx stmtFuncNameIdx) {
  auto *stmt = GetCurrentFuncCodeMp()->New<CallAssertBoundaryStmtNode>(*GetCurrentFuncCodeMpAllocator(), op,
                                                                       funcNameIdx, paramIndex, stmtFuncNameIdx);
  ASSERT(stmt != nullptr, "stmt is null");
  stmt->SetOpnds(rVals);
  return stmt;
}

NaryStmtNode *MIRBuilder::CreateStmtNary(Opcode op, BaseNode *rVal) {
  auto *stmt = GetCurrentFuncCodeMp()->New<NaryStmtNode>(*GetCurrentFuncCodeMpAllocator(), op);
  ASSERT(stmt != nullptr, "stmt is null");
  stmt->PushOpnd(rVal);
  return stmt;
}

AssertNonnullStmtNode *MIRBuilder::CreateStmtAssertNonnull(Opcode op, BaseNode* rVal, GStrIdx funcNameIdx) {
  auto *stmt = GetCurrentFuncCodeMp()->New<AssertNonnullStmtNode>(op, funcNameIdx);
  ASSERT(stmt != nullptr, "stmt is null");
  stmt->SetRHS(rVal);
  return stmt;
}

AssertBoundaryStmtNode *MIRBuilder::CreateStmtAssertBoundary(Opcode op, const MapleVector<BaseNode*> &rVals,
                                                             GStrIdx funcNameIdx) {
  auto *stmt = GetCurrentFuncCodeMp()->New<AssertBoundaryStmtNode>(*GetCurrentFuncCodeMpAllocator(), op, funcNameIdx);
  ASSERT(stmt != nullptr, "stmt is null");
  stmt->SetOpnds(rVals);
  return stmt;
}

CallAssertNonnullStmtNode *MIRBuilder::CreateStmtCallAssertNonnull(Opcode op, BaseNode* rVal, GStrIdx callFuncNameIdx,
                                                                   size_t paramIndex, GStrIdx stmtFuncNameIdx) {
  auto *stmt = GetCurrentFuncCodeMp()->New<CallAssertNonnullStmtNode>(op, callFuncNameIdx, paramIndex, stmtFuncNameIdx);
  ASSERT(stmt != nullptr, "stmt is null");
  stmt->SetRHS(rVal);
  return stmt;
}

UnaryStmtNode *MIRBuilder::CreateStmtUnary(Opcode op, BaseNode *rVal) {
  return GetCurrentFuncCodeMp()->New<UnaryStmtNode>(op, kPtyInvalid, rVal);
}

UnaryStmtNode *MIRBuilder::CreateStmtThrow(BaseNode *rVal) {
  return CreateStmtUnary(OP_throw, rVal);
}

IfStmtNode *MIRBuilder::CreateStmtIf(BaseNode *cond) {
  auto *ifStmt = GetCurrentFuncCodeMp()->New<IfStmtNode>();
  ifStmt->SetOpnd(cond, 0);
  BlockNode *thenBlock = GetCurrentFuncCodeMp()->New<BlockNode>();
  ifStmt->SetThenPart(thenBlock);
  return ifStmt;
}

IfStmtNode *MIRBuilder::CreateStmtIfThenElse(BaseNode *cond) {
  auto *ifStmt = GetCurrentFuncCodeMp()->New<IfStmtNode>();
  ifStmt->SetOpnd(cond, 0);
  auto *thenBlock = GetCurrentFuncCodeMp()->New<BlockNode>();
  ifStmt->SetThenPart(thenBlock);
  auto *elseBlock = GetCurrentFuncCodeMp()->New<BlockNode>();
  ifStmt->SetElsePart(elseBlock);
  ifStmt->SetNumOpnds(3);
  return ifStmt;
}

DoloopNode *MIRBuilder::CreateStmtDoloop(StIdx doVarStIdx, bool isPReg, BaseNode *startExp, BaseNode *contExp,
                                         BaseNode *incrExp) {
  return GetCurrentFuncCodeMp()->New<DoloopNode>(doVarStIdx, isPReg, startExp, contExp, incrExp,
                                                 GetCurrentFuncCodeMp()->New<BlockNode>());
}

SwitchNode *MIRBuilder::CreateStmtSwitch(BaseNode *opnd, LabelIdx defaultLabel, const CaseVector &switchTable) {
  auto *switchNode = GetCurrentFuncCodeMp()->New<SwitchNode>(*GetCurrentFuncCodeMpAllocator(),
                                                             defaultLabel, opnd);
  switchNode->SetSwitchTable(switchTable);
  return switchNode;
}

GotoNode *MIRBuilder::CreateStmtGoto(Opcode o, LabelIdx labIdx) {
  return GetCurrentFuncCodeMp()->New<GotoNode>(o, labIdx);
}

JsTryNode *MIRBuilder::CreateStmtJsTry(LabelIdx cLabIdx, LabelIdx fLabIdx) {
  return GetCurrentFuncCodeMp()->New<JsTryNode>(static_cast<uint16>(cLabIdx), static_cast<uint16>(fLabIdx));
}

TryNode *MIRBuilder::CreateStmtTry(const MapleVector<LabelIdx> &cLabIdxs) {
  return GetCurrentFuncCodeMp()->New<TryNode>(cLabIdxs);
}

CatchNode *MIRBuilder::CreateStmtCatch(const MapleVector<TyIdx> &tyIdxVec) {
  return GetCurrentFuncCodeMp()->New<CatchNode>(tyIdxVec);
}

LabelNode *MIRBuilder::CreateStmtLabel(LabelIdx labIdx) {
  return GetCurrentFuncCodeMp()->New<LabelNode>(labIdx);
}

StmtNode *MIRBuilder::CreateStmtComment(const std::string &cmnt) {
  return GetCurrentFuncCodeMp()->New<CommentNode>(*GetCurrentFuncCodeMpAllocator(), cmnt);
}

AddrofNode *MIRBuilder::CreateAddrof(const MIRSymbol &st, PrimType pty) {
  return GetCurrentFuncCodeMp()->New<AddrofNode>(OP_addrof, pty, st.GetStIdx(), 0);
}

AddrofNode *MIRBuilder::CreateDread(const MIRSymbol &st, PrimType pty) {
  return GetCurrentFuncCodeMp()->New<AddrofNode>(OP_dread, pty, st.GetStIdx(), 0);
}

CondGotoNode *MIRBuilder::CreateStmtCondGoto(BaseNode *cond, Opcode op, LabelIdx labIdx) {
  return GetCurrentFuncCodeMp()->New<CondGotoNode>(op, labIdx, cond);
}

LabelIdx MIRBuilder::GetOrCreateMIRLabel(const std::string &name) const {
  GStrIdx strIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(name);
  MIRFunction *currentFunctionInner = GetCurrentFunctionNotNull();
  LabelIdx lableIdx = currentFunctionInner->GetLabelTab()->GetLabelIdxFromStrIdx(strIdx);
  if (lableIdx == 0) {
    lableIdx = currentFunctionInner->GetLabelTab()->CreateLabel();
    currentFunctionInner->GetLabelTab()->SetSymbolFromStIdx(lableIdx, strIdx);
    currentFunctionInner->GetLabelTab()->AddToStringLabelMap(lableIdx);
  }
  return lableIdx;
}

LabelIdx MIRBuilder::CreateLabIdx(MIRFunction &mirFunc) const {
  LabelIdx lableIdx = mirFunc.GetLabelTab()->CreateLabel();
  mirFunc.GetLabelTab()->AddToStringLabelMap(lableIdx);
  return lableIdx;
}

void MIRBuilder::AddStmtInCurrentFunctionBody(StmtNode &stmt) const {
  MIRFunction *fun = GetCurrentFunctionNotNull();
  stmt.GetSrcPos().CondSetLineNum(lineNum);
  fun->GetBody()->AddStatement(&stmt);
}

MemPool *MIRBuilder::GetCurrentFuncCodeMp() {
  if (MIRFunction *curFunction = GetCurrentFunction()) {
    return curFunction->GetCodeMemPool();
  }
  return mirModule->GetMemPool();
}

MapleAllocator *MIRBuilder::GetCurrentFuncCodeMpAllocator() {
  if (MIRFunction *curFunction = GetCurrentFunction()) {
    return &curFunction->GetCodeMPAllocator();
  }
  return &mirModule->GetMPAllocator();
}

MemPool *MIRBuilder::GetCurrentFuncDataMp() {
  if (MIRFunction *curFunction = GetCurrentFunction()) {
    return curFunction->GetDataMemPool();
  }
  return mirModule->GetMemPool();
}

MIRBuilderExt::MIRBuilderExt(MIRModule *module, pthread_mutex_t *mutex) : MIRBuilder(module), mutex(mutex) {}

MemPool *MIRBuilderExt::GetCurrentFuncCodeMp() {
  ASSERT(curFunction, "curFunction is null");
  return curFunction->GetCodeMemPool();
}

MapleAllocator *MIRBuilderExt::GetCurrentFuncCodeMpAllocator() {
  ASSERT(curFunction, "curFunction is null");
  return &curFunction->GetCodeMemPoolAllocator();
}

void MIRBuilderExt::GlobalLock() {
  if (mutex) {
    ASSERT(pthread_mutex_lock(mutex) == 0, "lock failed");
  }
}

void MIRBuilderExt::GlobalUnlock() {
  if (mutex) {
    ASSERT(pthread_mutex_unlock(mutex) == 0, "unlock failed");
  }
}
}  // namespace maple
