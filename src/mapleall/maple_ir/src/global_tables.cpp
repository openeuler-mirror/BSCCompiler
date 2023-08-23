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
#include "global_tables.h"
#include "mir_type.h"
#include "mir_symbol.h"

#if MIR_FEATURE_FULL
namespace maple {
MIRType *TypeTable::CreateMirType(uint32 primTypeIdx) const {
  MIRTypeKind defaultKind = (primTypeIdx == PTY_constStr ? kTypeConstString : kTypeScalar);
  auto primType = static_cast<PrimType>(primTypeIdx);
  auto *mirType = new MIRType(defaultKind, primType);
  return mirType;
}

TypeTable::TypeTable() {
  // enter the primitve types in type_table_
  typeTable.push_back(static_cast<MIRType*>(nullptr));
  ASSERT(typeTable.size() == static_cast<size_t>(PTY_void), "use PTY_void as the first index to type table");
  uint32 primTypeIdx;
  for (primTypeIdx = static_cast<uint32>(PTY_begin) + 1; primTypeIdx <= static_cast<uint32>(PTY_end); ++primTypeIdx) {
    MIRType *type = CreateMirType(primTypeIdx);
    type->SetTypeIndex(TyIdx{ primTypeIdx });
    typeTable.push_back(type);
    PutToHashTable(type);
  }
  if (voidPtrType == nullptr) {
    voidPtrType = GetOrCreatePointerType(*GetVoid(), PTY_ptr);
  }
  lastDefaultTyIdx.SetIdx(primTypeIdx);
}

void TypeTable::SetTypeWithTyIdx(const TyIdx &tyIdx, MIRType &type) {
  CHECK_FATAL(tyIdx < typeTable.size(), "array index out of range");
  MIRType *oldType = typeTable.at(tyIdx);
  typeTable.at(tyIdx) = &type;
  if (oldType != nullptr && oldType != &type) {
    (void)typeHashTable.erase(oldType);
    (void)typeHashTable.insert(&type);
    delete oldType;
  }
}

TypeTable::~TypeTable() {
  for (auto index = static_cast<uint32>(PTY_void); index < typeTable.size(); ++index) {
    delete typeTable[index];
    typeTable[index] = nullptr;
  }
}

void TypeTable::PutToHashTable(MIRType *mirType) {
  (void)typeHashTable.insert(mirType);
}

void TypeTable::UpdateMIRType(const MIRType &pType, const TyIdx tyIdx) {
  MIRType *nType = pType.CopyMIRTypeNode();
  nType->SetTypeIndex(tyIdx);
  SetTypeWithTyIdx(tyIdx, *nType);
}

void TypeTable::FillinTypeMapOrHashTable(MIRType &pType, MIRType &nType) {
  if (pType.IsMIRPtrType()) {
    auto &pty = static_cast<MIRPtrType&>(pType);
    if (pty.GetTypeAttrs() == TypeAttrs()) {
      if (pty.GetPrimType() != PTY_ref) {
        ptrTypeMap[pty.GetPointedTyIdx()] = nType.GetTypeIndex();
      } else {
        refTypeMap[pty.GetPointedTyIdx()] = nType.GetTypeIndex();
      }
    } else {
      (void)typeHashTable.insert(&nType);
    }
  } else {
    (void)typeHashTable.insert(&nType);
  }
}

// used only by bin_mpl_import
void TypeTable::CreateMirTypeNodeAt(MIRType &pType, TyIdx tyIdxUsed, MIRModule *module,
                                    bool isObject, bool isIncomplete) {
  MIRType *nType = pType.CopyMIRTypeNode();
  nType->SetTypeIndex(tyIdxUsed);
  typeTable[tyIdxUsed] = nType;

  FillinTypeMapOrHashTable(pType, *nType);

  GStrIdx stridx = pType.GetNameStrIdx();
  if (stridx != 0) {
    module->GetTypeNameTab()->SetGStrIdxToTyIdx(stridx, tyIdxUsed);
    module->PushbackTypeDefOrder(stridx);
    if (isObject) {
      module->AddClass(tyIdxUsed);
      if (!isIncomplete) {
        GlobalTables::GetTypeNameTable().SetGStrIdxToTyIdx(stridx, tyIdxUsed);
      }
    }
  }
}

MIRType *TypeTable::CreateAndUpdateMirTypeNode(MIRType &pType) {
  MIRType *nType = pType.CopyMIRTypeNode();
  nType->SetTypeIndex(TyIdx(typeTable.size()));
  typeTable.push_back(nType);

  FillinTypeMapOrHashTable(pType, *nType);

  return nType;
}

MIRType* TypeTable::GetOrCreateMIRTypeNode(MIRType &pType) {
  if (pType.IsMIRPtrType()) {
    auto &type = static_cast<MIRPtrType&>(pType);
    if (type.GetTypeAttrs() == TypeAttrs()) {
      auto *pMap = (type.GetPrimType() != PTY_ref ? &ptrTypeMap : &refTypeMap);
      auto *otherPMap = (type.GetPrimType() == PTY_ref ? &ptrTypeMap : &refTypeMap);
      {
        std::shared_lock<std::shared_timed_mutex> lock(mtx);
        const auto it = pMap->find(type.GetPointedTyIdx());
        if (it != pMap->end()) {
          return GetTypeFromTyIdx(it->second);
        }
      }
      std::unique_lock<std::shared_timed_mutex> lock(mtx);
      CHECK_FATAL(!(type.GetPointedTyIdx().GetIdx() >= kPtyDerived && type.GetPrimType() == PTY_ref &&
                    otherPMap->find(type.GetPointedTyIdx()) != otherPMap->end()),
                  "GetOrCreateMIRType: ref pointed-to type %d has previous ptr occurrence",
                  type.GetPointedTyIdx().GetIdx());
      return CreateAndUpdateMirTypeNode(pType);
    }
  }
  {
    std::shared_lock<std::shared_timed_mutex> lock(mtx);
    const auto it = typeHashTable.find(&pType);
    if (it != typeHashTable.end()) {
      return *it;
    }
  }
  std::unique_lock<std::shared_timed_mutex> lock(mtx);
  return CreateAndUpdateMirTypeNode(pType);
}

MIRType *TypeTable::voidPtrType = nullptr;
// get or create a type that pointing to pointedTyIdx
MIRType *TypeTable::GetOrCreatePointerType(const TyIdx &pointedTyIdx, PrimType primType, const TypeAttrs &attrs) {
  MIRPtrType type(pointedTyIdx, primType);
  type.SetTypeAttrs(attrs);
  TyIdx tyIdx = GetOrCreateMIRType(&type);
  ASSERT(tyIdx < typeTable.size(), "index out of range in TypeTable::GetOrCreatePointerType");
  return typeTable.at(tyIdx);
}

MIRType *TypeTable::GetOrCreatePointerType(const MIRType &pointTo, PrimType primType, const TypeAttrs &attrs) {
  if (pointTo.GetPrimType() == PTY_constStr) {
    primType = PTY_ptr;
  }
  return GetOrCreatePointerType(pointTo.GetTypeIndex(), primType, attrs);
}

const MIRType *TypeTable::GetPointedTypeIfApplicable(MIRType &type) const {
  if (type.GetKind() != kTypePointer) {
    return &type;
  }
  auto &ptrType = static_cast<MIRPtrType&>(type);
  return GetTypeFromTyIdx(ptrType.GetPointedTyIdx());
}

MIRType *TypeTable::GetPointedTypeIfApplicable(MIRType &type) {
  if (type.GetKind() != kTypePointer) {
    return &type;
  }
  auto &ptrType = static_cast<MIRPtrType&>(type);
  return GetTypeFromTyIdx(ptrType.GetPointedTyIdx());
}

MIRArrayType *TypeTable::GetOrCreateArrayType(const MIRType &elem, uint8 dim, const uint32 *sizeArray,
                                              const TypeAttrs &attrs) {
  std::vector<uint32> sizeVector;
  for (size_t i = 0; i < dim; ++i) {
    sizeVector.push_back(sizeArray != nullptr ? sizeArray[i] : 0);
  }
  MIRArrayType arrayType(elem.GetTypeIndex(), sizeVector);
  arrayType.SetTypeAttrs(attrs);
  TyIdx tyIdx = GetOrCreateMIRType(&arrayType);
  return static_cast<MIRArrayType*>(typeTable[tyIdx]);
}

// For one dimension array
MIRArrayType *TypeTable::GetOrCreateArrayType(const MIRType &elem, uint32 size, const TypeAttrs &attrs) {
  return GetOrCreateArrayType(elem, 1, &size, attrs);
}

MIRType *TypeTable::GetOrCreateFarrayType(const MIRType &elem) {
  MIRFarrayType type;
  type.SetElemtTyIdx(elem.GetTypeIndex());
  TyIdx tyIdx = GetOrCreateMIRType(&type);
  ASSERT(tyIdx < typeTable.size(), "index out of range in TypeTable::GetOrCreateFarrayType");
  return typeTable.at(tyIdx);
}

MIRType *TypeTable::GetOrCreateJarrayType(const MIRType &elem) {
  MIRJarrayType type;
  type.SetElemtTyIdx(elem.GetTypeIndex());
  TyIdx tyIdx = GetOrCreateMIRType(&type);
  ASSERT(tyIdx < typeTable.size(), "index out of range in TypeTable::GetOrCreateJarrayType");
  return typeTable.at(tyIdx);
}

MIRType *TypeTable::GetOrCreateFunctionType(const TyIdx &retTyIdx,
                                            const std::vector<TyIdx> &vecType,
                                            const std::vector<TypeAttrs> &vecAttrs,
                                            const FuncAttrs &funcAttrs,
                                            const TypeAttrs &retAttrs) {
  MIRFuncType funcType(retTyIdx, vecType, vecAttrs, funcAttrs, retAttrs);
  TyIdx tyIdx = GetOrCreateMIRType(&funcType);
  ASSERT(tyIdx < typeTable.size(), "index out of range in TypeTable::GetOrCreateFunctionType");
  return typeTable.at(tyIdx);
}


MIRType *TypeTable::GetOrCreateFunctionType(const TyIdx &retTyIdx, const std::vector<TyIdx> &vecType,
                                            const std::vector<TypeAttrs> &vecAttrs, bool isVarg,
                                            const TypeAttrs &retAttrs) {
  FuncAttrs funcAttrs;
  if (isVarg) {
    funcAttrs.SetAttr(FUNCATTR_varargs);
  }
  return GetOrCreateFunctionType(retTyIdx, vecType, vecAttrs, funcAttrs, retAttrs);
}

MIRType *TypeTable::GetOrCreateStructOrUnion(const std::string &name, const FieldVector &fields,
                                             const FieldVector &parentFields, MIRModule &module, bool forStruct,
                                             const TypeAttrs &attrs) {
  GStrIdx strIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(name);
  MIRStructType type(forStruct ? kTypeStruct : kTypeUnion, strIdx);
  type.SetFields(fields);
  type.SetParentFields(parentFields);
  type.SetTypeAttrs(attrs);

  TyIdx tyIdx = GetOrCreateMIRType(&type);
  // Global?
  module.GetTypeNameTab()->SetGStrIdxToTyIdx(strIdx, tyIdx);
  module.PushbackTypeDefOrder(strIdx);
  ASSERT(tyIdx < typeTable.size(), "index out of range in TypeTable::GetOrCreateStructOrUnion");
  return typeTable.at(tyIdx);
}

void TypeTable::PushIntoFieldVector(FieldVector &fields, const std::string &name, const MIRType &type) const {
  GStrIdx strIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(name);
  fields.push_back(FieldPair(strIdx, TyIdxFieldAttrPair(type.GetTypeIndex(), FieldAttrs())));
}

MIRType *TypeTable::GetOrCreateClassOrInterface(const std::string &name, MIRModule &module, bool forClass) {
  GStrIdx strIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(name);
  TyIdx tyIdx = module.GetTypeNameTab()->GetTyIdxFromGStrIdx(strIdx);
  if (!tyIdx) {
    if (forClass) {
      MIRClassType type(kTypeClassIncomplete, strIdx);  // for class type
      tyIdx = GetOrCreateMIRType(&type);
    } else {
      MIRInterfaceType type(kTypeInterfaceIncomplete, strIdx);  // for interface type
      tyIdx = GetOrCreateMIRType(&type);
    }
    module.PushbackTypeDefOrder(strIdx);
    module.GetTypeNameTab()->SetGStrIdxToTyIdx(strIdx, tyIdx);
    if (typeTable[tyIdx]->GetNameStrIdx() == 0u) {
      typeTable[tyIdx]->SetNameStrIdx(strIdx);
    }
  }
  ASSERT(tyIdx < typeTable.size(), "index out of range in TypeTable::GetOrCreateClassOrInterface");
  return typeTable.at(tyIdx);
}

void TypeTable::AddFieldToStructType(MIRStructType &structType, const std::string &fieldName,
                                     const MIRType &fieldType) const {
  GStrIdx strIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(fieldName);
  FieldAttrs fieldAttrs;
  fieldAttrs.SetAttr(FLDATTR_final);  // Mark compiler-generated struct fields as final to improve AliasAnalysis
  structType.GetFields().push_back(FieldPair(strIdx, TyIdxFieldAttrPair(fieldType.GetTypeIndex(), fieldAttrs)));
}

void FPConstTable::PostInit() {
  MIRType &typeFloat = *GlobalTables::GetTypeTable().GetPrimType(PTY_f32);
  nanFloatConst = new MIRFloatConst(NAN, typeFloat);
  infFloatConst = new MIRFloatConst(INFINITY, typeFloat);
  minusInfFloatConst = new MIRFloatConst(-INFINITY, typeFloat);
  minusZeroFloatConst = new MIRFloatConst(-0.0, typeFloat);
  MIRType &typeDouble = *GlobalTables::GetTypeTable().GetPrimType(PTY_f64);
  nanDoubleConst = new MIRDoubleConst(NAN, typeDouble);
  infDoubleConst = new MIRDoubleConst(INFINITY, typeDouble);
  minusInfDoubleConst = new MIRDoubleConst(-INFINITY, typeDouble);
  minusZeroDoubleConst = new MIRDoubleConst(-0.0, typeDouble);
  MIRType &typeLongDouble = *GlobalTables::GetTypeTable().GetPrimType(PTY_f128);
  nanFloat128Const = new MIRFloat128Const({0x7fff800000000000ULL, 0x0ULL}, typeLongDouble);
  infFloat128Const = new MIRFloat128Const({0x7fff000000000000ULL, 0x0ULL}, typeLongDouble);
  minusInfFloat128Const = new MIRFloat128Const({0xffff000000000000ULL, 0x0ULL}, typeLongDouble);
  minusZeroFloat128Const = new MIRFloat128Const({0x8000000000000000ULL, 0x0ULL}, typeLongDouble);
}

MIRIntConst *IntConstTable::GetOrCreateIntConst(const IntVal &val, MIRType &type) {
  PrimType pt = type.GetPrimType();
  if (ThreadEnv::IsMeParallel()) {
    if (IsInt128Ty(pt)) {
      return DoGetOrCreateInt128ConstTreadSafe(val.GetRawData(), type);
    }
    return DoGetOrCreateIntConstTreadSafe(static_cast<uint64>(val.GetExtValue()), type);
  }

  if (IsInt128Ty(pt)) {
    return DoGetOrCreateInt128Const(val.GetRawData(), type);
  }
  return DoGetOrCreateIntConst(static_cast<uint64>(val.GetExtValue()), type);
}

MIRIntConst *IntConstTable::GetOrCreateIntConst(uint64 val, MIRType &type) {
  if (ThreadEnv::IsMeParallel()) {
    return DoGetOrCreateIntConstTreadSafe(val, type);
  }
  return DoGetOrCreateIntConst(val, type);
}

MIRIntConst *IntConstTable::DoGetOrCreateIntConst(uint64 val, MIRType &type) {
  IntConstKey key(val, type.GetTypeIndex());
  if (intConstTable.find(key) != intConstTable.end()) {
    return intConstTable[key];
  }
  intConstTable[key] = new MIRIntConst(val, type);
  return intConstTable[key];
}

MIRIntConst *IntConstTable::DoGetOrCreateInt128Const(const Int128ElemTy *pVal, MIRType &type) {
  Int128ConstKey key(pVal, type.GetTypeIndex());
  if (int128ConstTable.find(key) != int128ConstTable.end()) {
    return int128ConstTable[key];
  }
  int128ConstTable[key] = new MIRIntConst(pVal, type);
  return int128ConstTable[key];
}

MIRIntConst *IntConstTable::DoGetOrCreateIntConstTreadSafe(uint64 val, MIRType &type) {
  IntConstKey key(val, type.GetTypeIndex());
  {
    std::shared_lock<std::shared_timed_mutex> lock(mtx);
    if (intConstTable.find(key) != intConstTable.end()) {
      return intConstTable[key];
    }
  }
  std::unique_lock<std::shared_timed_mutex> lock(mtx);
  intConstTable[key] = new MIRIntConst(val, type);
  return intConstTable[key];
}

MIRIntConst *IntConstTable::DoGetOrCreateInt128ConstTreadSafe(const Int128ElemTy *pVal, MIRType &type) {
  Int128ConstKey key(pVal, type.GetTypeIndex());
  {
    std::shared_lock<std::shared_timed_mutex> lock(mtx);
    if (int128ConstTable.find(key) != int128ConstTable.end()) {
      return int128ConstTable[key];
    }
  }
  std::unique_lock<std::shared_timed_mutex> lock(mtx);
  int128ConstTable[key] = new MIRIntConst(pVal, type);
  return int128ConstTable[key];
}

IntConstTable::~IntConstTable() {
  for (auto pair : intConstTable) {
    delete pair.second;
  }
}

MIRFloatConst *FPConstTable::GetOrCreateFloatConst(float floatVal) {
  if (std::isnan(floatVal)) {
    return nanFloatConst;
  }
  if (std::isinf(floatVal)) {
    return (floatVal < 0) ? minusInfFloatConst : infFloatConst;
  }
  if (floatVal == 0.0 && std::signbit(floatVal)) {
    return minusZeroFloatConst;
  }
  if (ThreadEnv::IsMeParallel()) {
    return DoGetOrCreateFloatConstThreadSafe(floatVal);
  }
  return DoGetOrCreateFloatConst(floatVal);
}

MIRFloatConst *FPConstTable::DoGetOrCreateFloatConst(float floatVal) {
  const auto it = floatConstTable.find(floatVal);
  if (it != floatConstTable.cend()) {
    return it->second;
  }
  // create a new one
  auto *floatConst =
      new MIRFloatConst(floatVal, *GlobalTables::GetTypeTable().GetTypeFromTyIdx(TyIdx{ PTY_f32 }));
  floatConstTable[floatVal] = floatConst;
  return floatConst;
}

MIRFloatConst *FPConstTable::DoGetOrCreateFloatConstThreadSafe(float floatVal) {
  {
    std::shared_lock<std::shared_timed_mutex> lock(floatMtx);
    const auto it = floatConstTable.find(floatVal);
    if (it != floatConstTable.cend()) {
      return it->second;
    }
  }
  // create a new one
  std::unique_lock<std::shared_timed_mutex> lock(floatMtx);
  auto *floatConst =
      new MIRFloatConst(floatVal, *GlobalTables::GetTypeTable().GetTypeFromTyIdx(TyIdx{ PTY_f32 }));
  floatConstTable[floatVal] = floatConst;
  return floatConst;
}

MIRDoubleConst *FPConstTable::GetOrCreateDoubleConst(double doubleVal) {
  if (std::isnan(doubleVal)) {
    return nanDoubleConst;
  }
  if (std::isinf(doubleVal)) {
    return (doubleVal < 0) ? minusInfDoubleConst : infDoubleConst;
  }
  if (doubleVal == 0.0 && std::signbit(doubleVal)) {
    return minusZeroDoubleConst;
  }
  if (ThreadEnv::IsMeParallel()) {
    return DoGetOrCreateDoubleConstThreadSafe(doubleVal);
  }
  return DoGetOrCreateDoubleConst(doubleVal);
}

MIRDoubleConst *FPConstTable::DoGetOrCreateDoubleConst(double doubleVal) {
  const auto it = doubleConstTable.find(doubleVal);
  if (it != doubleConstTable.cend()) {
    return it->second;
  }
  // create a new one
  auto *doubleConst = new MIRDoubleConst(
      doubleVal, *GlobalTables::GetTypeTable().GetTypeFromTyIdx(static_cast<TyIdx>(PTY_f64)));
  doubleConstTable[doubleVal] = doubleConst;
  return doubleConst;
}

MIRDoubleConst *FPConstTable::DoGetOrCreateDoubleConstThreadSafe(double doubleVal) {
  {
    std::shared_lock<std::shared_timed_mutex> lock(doubleMtx);
    const auto it = doubleConstTable.find(doubleVal);
    if (it != doubleConstTable.cend()) {
      return it->second;
    }
  }
  // create a new one
  std::unique_lock<std::shared_timed_mutex> lock(doubleMtx);
  auto *doubleConst = new MIRDoubleConst(
      doubleVal, *GlobalTables::GetTypeTable().GetTypeFromTyIdx(static_cast<TyIdx>(PTY_f64)));
  doubleConstTable[doubleVal] = doubleConst;
  return doubleConst;
}

MIRFloat128Const *FPConstTable::GetOrCreateFloat128Const(const uint64 *fvalPtr) {
  auto f128Const = MIRFloat128Const(fvalPtr, *GlobalTables::GetTypeTable().GetPrimType(PTY_f128));
  if (f128Const.IsNan()) {
    return nanFloat128Const;
  }
  if (f128Const.IsInf()) {
    return (f128Const.GetSign() != 0) ? minusInfFloat128Const : infFloat128Const;
  }
  if (f128Const.IsZero() && f128Const.GetSign() != 0) {
    return minusZeroFloat128Const;
  }
  if (ThreadEnv::IsMeParallel()) {
    return DoGetOrCreateFloat128ConstThreadSafe(fvalPtr);
  }
  return DoGetOrCreateFloat128Const(fvalPtr);
}

MIRFloat128Const *FPConstTable::DoGetOrCreateFloat128Const(const uint64_t *v) {
  std::pair<uint64, uint64> f128Pair = {v[0], v[1]};
  const auto it = float128ConstTable.find(f128Pair);
  if (it != float128ConstTable.cend()) {
    return it->second;
  }
  // create a new one
  auto *ldConst = new MIRFloat128Const(v,
                                       *GlobalTables::GetTypeTable().GetTypeFromTyIdx(static_cast<TyIdx>(PTY_f128)));
  float128ConstTable[f128Pair] = ldConst;
  return ldConst;
}

MIRFloat128Const *FPConstTable::DoGetOrCreateFloat128ConstThreadSafe(const uint64_t *v) {
  {
    const auto it = float128ConstTable.find({v[0], v[1]});
    if (it != float128ConstTable.cend()) {
      return it->second;
    }
  }
  // create a new one
  std::pair<uint64, uint64> f128Pair = {v[0], v[1]};
  auto *ldConst = new MIRFloat128Const(v,
                                       *GlobalTables::GetTypeTable().GetTypeFromTyIdx(static_cast<TyIdx>(PTY_f128)));
  float128ConstTable[f128Pair] = ldConst;
  return ldConst;
}

FPConstTable::~FPConstTable() {
  delete nanFloatConst;
  delete infFloatConst;
  delete minusInfFloatConst;
  delete minusZeroFloatConst;
  delete nanDoubleConst;
  delete infDoubleConst;
  delete minusInfDoubleConst;
  delete minusZeroDoubleConst;
  delete nanFloat128Const;
  delete infFloat128Const;
  delete minusInfFloat128Const;
  delete minusZeroFloat128Const;
  for (const auto &floatConst : floatConstTable) {
    delete floatConst.second;
  }
  for (const auto &doubleConst : doubleConstTable) {
    delete doubleConst.second;
  }
  for (const auto &float128Const : float128ConstTable) {
    delete float128Const.second;
  }
}

GSymbolTable::GSymbolTable() {
  symbolTable.push_back(static_cast<MIRSymbol*>(nullptr));
}

GSymbolTable::~GSymbolTable() {
  for (MIRSymbol *symbol : symbolTable) {
    delete symbol;
  }
}

MIRSymbol *GSymbolTable::CreateSymbol(uint8 scopeID) {
  auto *st = new MIRSymbol(symbolTable.size(), scopeID);
  CHECK_FATAL(st != nullptr, "CreateSymbol failure");
  symbolTable.push_back(st);
  module->AddSymbol(st);
  return st;
}

bool GSymbolTable::AddToStringSymbolMap(const MIRSymbol &st) {
  GStrIdx strIdx = st.GetNameStrIdx();
  if (strIdxToStIdxMap[strIdx].FullIdx() != 0) {
    return false;
  }
  strIdxToStIdxMap[strIdx] = st.GetStIdx();
  return true;
}

bool GSymbolTable::RemoveFromStringSymbolMap(const MIRSymbol &st) {
  const auto it = strIdxToStIdxMap.find(st.GetNameStrIdx());
  if (it != strIdxToStIdxMap.cend()) {
    strIdxToStIdxMap.erase(it);
    return true;
  }
  return false;
}

void GSymbolTable::Dump(bool isLocal, int32 indent) const {
  for (size_t i = 1; i < symbolTable.size(); ++i) {
    const MIRSymbol *symbol = symbolTable[i];
    if (symbol != nullptr) {
      symbol->Dump(isLocal, indent);
    }
  }
}

GlobalTables GlobalTables::globalTables;
GlobalTables &GlobalTables::GetGlobalTables() {
  return globalTables;
}
}  // namespace maple
#endif  // MIR_FEATURE_FULL
