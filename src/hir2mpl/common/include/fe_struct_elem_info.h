/*
 * Copyright (c) [2020-2021] Huawei Technologies Co.,Ltd.All rights reserved.
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
#ifndef HIR2MPL_INCLUDE_COMMON_FE_STRUCT_ELEM_INFO_H
#define HIR2MPL_INCLUDE_COMMON_FE_STRUCT_ELEM_INFO_H
#include <memory>
#include <unordered_set>
#include "global_tables.h"
#include "fe_configs.h"
#include "feir_type.h"

namespace maple {
struct StructElemNameIdx {
  GStrIdx klass;
  GStrIdx elem;
  GStrIdx type;
  GStrIdx full;

  StructElemNameIdx(const GStrIdx &argKlass, const GStrIdx &argElem, const GStrIdx &argType, const GStrIdx &argFull)
      : klass(argKlass), elem(argElem), type(argType), full(argFull) {}
  StructElemNameIdx(const std::string &klassStr, const std::string &elemStr, const std::string &typeStr) {
    const std::string &fullName = klassStr + "|" + elemStr + "|" + typeStr;
    klass = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(namemangler::EncodeName(klassStr));
    elem = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(namemangler::EncodeName(elemStr));
    type = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(namemangler::EncodeName(typeStr));
    full = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(namemangler::EncodeName(fullName));
  }
  StructElemNameIdx(const std::string &funcStr) {
    klass = GStrIdx(0);
    elem = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(funcStr);
    type = GStrIdx(0);
    full = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(funcStr);
  }
  ~StructElemNameIdx() = default;
};

class FEStructElemInfo {
 public:
  FEStructElemInfo(MapleAllocator &allocatorIn, const StructElemNameIdx &argStructElemNameIdx,
                   MIRSrcLang argSrcLang, bool argIsStatic);
  virtual ~FEStructElemInfo() = default;

  void Prepare(MIRBuilder &mirBuilder, bool argIsStatic) {
    PrepareImpl(mirBuilder, argIsStatic);
  }

  const std::string &GetStructName() const {
    return GlobalTables::GetStrTable().GetStringFromStrIdx(structElemNameIdx.klass);
  }

  const std::string &GetElemName() const {
    return GlobalTables::GetStrTable().GetStringFromStrIdx(structElemNameIdx.elem);
  }

  const GStrIdx &GetElemNameIdx() const {
    return structElemNameIdx.elem;
  }

  const std::string &GetSignatureName() const {
    return GlobalTables::GetStrTable().GetStringFromStrIdx(structElemNameIdx.type);
  }

  bool IsStatic() const {
    return isStatic;
  }

  bool IsMethod() const {
    return isMethod;
  }

  bool IsDefined() const {
    return isDefined;
  }

  void SetDefined() {
    isDefined = true;
  }

  void SetUndefined() {
    isDefined = false;
  }

  bool IsFromDex() const {
    return isFromDex;
  }

  void SetFromDex() {
    isFromDex = true;
  }

  MIRSrcLang GetSrcLang() const {
    return srcLang;
  }

  void SetSrcLang(MIRSrcLang lang) {
    srcLang = lang;
  }

  UniqueFEIRType GetActualContainerType() const;
  const std::string GetActualContainerName() const {
    return actualContainer.c_str();
  }

 LLT_PROTECTED:
  virtual void PrepareImpl(MIRBuilder &mirBuilder, bool argIsStatic) = 0;

  bool isStatic : 1;
  bool isMethod : 1;
  bool isDefined : 1;
  bool isFromDex : 1;
  bool isPrepared : 1;
  MIRSrcLang srcLang : 8;
  MapleAllocator &allocator;
  StructElemNameIdx structElemNameIdx;
  MapleString actualContainer; // in maple format
};

using UniqueFEStructElemInfo = std::unique_ptr<FEStructElemInfo>;

class FEStructFieldInfo : public FEStructElemInfo {
 public:
  FEStructFieldInfo(MapleAllocator &allocatorIn, const StructElemNameIdx &argStructElemNameIdx,
                    MIRSrcLang argSrcLang, bool argIsStatic);
  ~FEStructFieldInfo() {
    fieldType = nullptr;
  }
  GStrIdx GetFieldNameIdx() const {
    return fieldNameIdx;
  }

  FieldID GetFieldID() const {
    return fieldID;
  }

  void SetFieldID(const FieldID argFieldID) {
    fieldID = argFieldID;
  }

  const FEIRType *GetType() const {
    return fieldType;
  }

  bool IsVolatile() const {
    return isVolatile;
  }

 LLT_PROTECTED:
  void PrepareImpl(MIRBuilder &mirBuilder, bool argIsStatic) override;

 LLT_PRIVATE:
  void LoadFieldType();
  void LoadFieldTypeJava();
  void PrepareStaticField(const MIRStructType &structType);
  void PrepareNonStaticField(MIRBuilder &mirBuilder);
  bool SearchStructFieldJava(MIRStructType &structType, MIRBuilder &mirBuilder, bool argIsStatic,
                             bool allowPrivate = true);
  bool SearchStructFieldJava(const TyIdx &tyIdx, MIRBuilder &mirBuilder, bool argIsStatic, bool allowPrivate = true);
  bool CompareFieldType(const FieldPair &fieldPair) const;

  FEIRType *fieldType;
  GStrIdx fieldNameIdx;
  FieldID fieldID;
  bool isVolatile;
};

class FEStructMethodInfo : public FEStructElemInfo {
 public:
  FEStructMethodInfo(MapleAllocator &allocatorIn, const StructElemNameIdx &argStructElemNameIdx,
                     MIRSrcLang argSrcLang, bool argIsStatic);
  ~FEStructMethodInfo();
  PUIdx GetPuIdx() const;
  bool IsConstructor() const {
    return isConstructor;
  }

  bool IsReturnVoid() const {
    return isReturnVoid;
  }

  void SetReturnVoid() {
    isReturnVoid = true;
  }

  bool IsJavaPolymorphicCall() const {
    return isJavaPolymorphicCall;
  }

  bool IsJavaDynamicCall() const {
    return isJavaDynamicCall;
  }

  void SetJavaDyamicCall() {
    isJavaDynamicCall = true;
  }

  void UnsetJavaDynamicCall() {
    isJavaDynamicCall = false;
  }

  const FEIRType *GetReturnType() const {
    return retType;
  }

  void SetReturnType(FEIRType *type) {
    retType = type;
  }

  const FEIRType *GetOwnerType() const {
    return ownerType;
  }

  const MapleVector<FEIRType*> &GetArgTypes() const {
    return argTypes;
  }

  void SetArgTypes(const MapleVector<FEIRType*> &argTypesIn) {
    argTypes = argTypesIn;
  }

  void SetArgsName(const std::vector<std::string> &names) {
    argNames = names;
  }

  const std::vector<std::string> &GetArgsName() const {
    return argNames;
  }

  void SetFuncAttrs(const FuncAttrs &attrs) {
    funcAttrs = attrs;
  }

  const FuncAttrs &GetFuncAttrs() const {
    return funcAttrs;
  }

  MIRFunction *GetMirFunc() const {
    CHECK_NULL_FATAL(mirFunc);
    return mirFunc;
  }

 LLT_PROTECTED:
  void PrepareImpl(MIRBuilder &mirBuilder, bool argIsStatic) override;

 LLT_PRIVATE:
  void LoadMethodType();
  void LoadMethodTypeJava();
  void PrepareMethod();
  void PrepareMethodC();
  void PrepareImplJava(MIRBuilder &mirBuilder, bool argIsStatic);
  bool SearchStructMethodJava(MIRStructType &structType, MIRBuilder &mirBuilder, bool argIsStatic,
                              bool allowPrivate = true);
  bool SearchStructMethodJavaInParent(MIRStructType &structType, MIRBuilder &mirBuilder, bool argIsStatic);
  bool SearchStructMethodJava(const TyIdx &tyIdx, MIRBuilder &mirBuilder, bool argIsStatic, bool allowPrivate = true);

  bool isReturnVoid;
  bool isConstructor;
  bool isJavaPolymorphicCall;
  bool isJavaDynamicCall;
  GStrIdx methodNameIdx;
  FEIRType *retType;
  FEIRType *ownerType;
  MIRFunction *mirFunc;
  MapleVector<FEIRType*> argTypes;
  std::vector<std::string> argNames;
  FuncAttrs funcAttrs;
};
}  // namespace maple
#endif  // HIR2MPL_INCLUDE_COMMON_FE_STRUCT_ELEM_INFO_H
