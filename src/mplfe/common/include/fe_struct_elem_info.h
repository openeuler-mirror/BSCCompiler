/*
 * Copyright (c) [2020] Huawei Technologies Co.,Ltd.All rights reserved.
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
#ifndef MPLFE_INCLUDE_COMMON_FE_STRUCT_ELEM_INFO_H
#define MPLFE_INCLUDE_COMMON_FE_STRUCT_ELEM_INFO_H
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
  ~StructElemNameIdx() = default;
};

class FEStructElemInfo {
 public:
  FEStructElemInfo(const StructElemNameIdx &argStructElemNameIdx,
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
  const std::string &GetActualContainerName() const {
    return actualContainer;
  }

 LLT_PROTECTED:
  virtual void PrepareImpl(MIRBuilder &mirBuilder, bool argIsStatic) = 0;

  StructElemNameIdx structElemNameIdx;
  MIRSrcLang srcLang : 8;
  bool isStatic : 1;
  bool isMethod : 1;
  bool isDefined : 1;
  bool isFromDex : 1;
  bool isPrepared : 1;
  std::string actualContainer; // in maple format
};

using UniqueFEStructElemInfo = std::unique_ptr<FEStructElemInfo>;

class FEStructFieldInfo : public FEStructElemInfo {
 public:
  FEStructFieldInfo(const StructElemNameIdx &argStructElemNameIdx,
                    MIRSrcLang argSrcLang, bool argIsStatic);
  ~FEStructFieldInfo() = default;
  GStrIdx GetFieldNameIdx() const {
    return fieldNameIdx;
  }

  FieldID GetFieldID() const {
    return fieldID;
  }

  void SetFieldID(const FieldID argFieldID) {
    fieldID = argFieldID;
  }

  const UniqueFEIRType &GetType() const {
    return fieldType;
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

  UniqueFEIRType fieldType;
  GStrIdx fieldNameIdx;
  FieldID fieldID;
};

class FEStructMethodInfo : public FEStructElemInfo {
 public:
  FEStructMethodInfo(const StructElemNameIdx &argStructElemNameIdx,
                     MIRSrcLang argSrcLang, bool argIsStatic);
  ~FEStructMethodInfo();
  PUIdx GetPuIdx() const;
  bool IsConstructor() const {
    return isConstructor;
  }

  bool IsReturnVoid() const {
    return isReturnVoid;
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

  const UniqueFEIRType &GetReturnType() const {
    return retType;
  }

  const UniqueFEIRType &GetOwnerType() const {
    return ownerType;
  }

  const std::vector<UniqueFEIRType> &GetArgTypes() const {
    return argTypes;
  }

  static void InitJavaPolymorphicWhiteList();

 LLT_PROTECTED:
  void PrepareImpl(MIRBuilder &mirBuilder, bool argIsStatic) override;

 LLT_PRIVATE:
  void LoadMethodType();
  void LoadMethodTypeJava();
  void PrepareMethod();
  void PrepareImplJava(MIRBuilder &mirBuilder, bool argIsStatic);
  bool SearchStructMethodJava(MIRStructType &structType, MIRBuilder &mirBuilder, bool argIsStatic,
                              bool allowPrivate = true);
  bool SearchStructMethodJavaInParent(MIRStructType &structType, MIRBuilder &mirBuilder, bool argIsStatic);
  bool SearchStructMethodJava(const TyIdx &tyIdx, MIRBuilder &mirBuilder, bool argIsStatic, bool allowPrivate = true);
  bool CheckJavaPolymorphicCall() const;
  std::vector<UniqueFEIRType> argTypes;
  UniqueFEIRType retType;
  UniqueFEIRType ownerType;
  GStrIdx methodNameIdx;
  MIRFunction *mirFunc;
  bool isReturnVoid;
  bool isConstructor;
  bool isJavaPolymorphicCall;
  bool isJavaDynamicCall;
  static std::map<GStrIdx, std::set<GStrIdx>> javaPolymorphicWhiteList;
};
}  // namespace maple
#endif  // MPLFE_INCLUDE_COMMON_FE_STRUCT_ELEM_INFO_H
