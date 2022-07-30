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
#ifndef HIR2MPL_INCLUDE_FE_INPUT_HELPER_H
#define HIR2MPL_INCLUDE_FE_INPUT_HELPER_H
#include "mempool_allocator.h"
#include "mir_type.h"
#include "mir_pragma.h"
#include "mir_symbol.h"
#include "feir_type.h"
#include "fe_function.h"

namespace maple {
struct ExtraField {
  std::string klass;
  std::string field;
  std::string type;
  std::string attr;
};

class FEInputContainer {
 public:
  FEInputContainer() = default;
  virtual ~FEInputContainer() = default;
  MIRStructType *GetContainer() {
    return GetContainerImpl();
  }

 protected:
  virtual MIRStructType *GetContainerImpl() = 0;
};

class FEInputPragmaHelper {
 public:
  FEInputPragmaHelper() = default;
  virtual ~FEInputPragmaHelper() = default;
  std::vector<MIRPragma*> &GenerateMIRPragmas() {
    return GenerateMIRPragmasImpl();
  }

 protected:
  virtual std::vector<MIRPragma*> &GenerateMIRPragmasImpl() = 0;
};

class FEInputStructHelper;

class FEInputGlobalVarHelper {
 public:
  explicit FEInputGlobalVarHelper(MapleAllocator &allocatorIn) : allocator(allocatorIn) {}
  virtual ~FEInputGlobalVarHelper() = default;

  bool ProcessDecl() {
    return ProcessDecl(allocator);
  }

  bool ProcessDecl(MapleAllocator &allocatorIn) {
    return ProcessDeclImpl(allocatorIn);
  }

 protected:
  MapleAllocator &allocator;
  virtual bool ProcessDeclImpl(MapleAllocator &allocator) = 0;
};

class FEInputFileScopeAsmHelper {
 public:
  explicit FEInputFileScopeAsmHelper(MapleAllocator &allocatorIn) : allocator(allocatorIn) {}
  virtual ~FEInputFileScopeAsmHelper() = default;

  bool ProcessDecl() {
    return ProcessDecl(allocator);
  }

  bool ProcessDecl(MapleAllocator &allocatorIn) {
    return ProcessDeclImpl(allocatorIn);
  }

 protected:
  MapleAllocator &allocator;
  virtual bool ProcessDeclImpl(MapleAllocator &allocator) = 0;
};

class FEInputEnumHelper {
 public:
  explicit FEInputEnumHelper(MapleAllocator &allocatorIn) : allocator(allocatorIn) {}
  virtual ~FEInputEnumHelper() = default;

  bool ProcessDecl() {
    return ProcessDecl(allocator);
  }

  bool ProcessDecl(MapleAllocator &allocatorIn) {
    return ProcessDeclImpl(allocatorIn);
  }

 protected:
  MapleAllocator &allocator;
  virtual bool ProcessDeclImpl(MapleAllocator &allocator) = 0;
};

class FEInputFieldHelper {
 public:
  explicit FEInputFieldHelper(MapleAllocator &allocatorIn) : allocator(allocatorIn) {}
  virtual ~FEInputFieldHelper() = default;
  const FieldPair &GetMIRFieldPair() const {
    return mirFieldPair;
  }

  bool IsStatic() const {
    return mirFieldPair.second.second.GetAttr(FLDATTR_static);
  }

  bool ProcessDecl() {
    return ProcessDecl(allocator);
  }

  bool ProcessDecl(MapleAllocator &alloc) {
    return ProcessDeclImpl(alloc);
  }

  bool ProcessDeclWithContainer(MapleAllocator &alloc) {
    return ProcessDeclWithContainerImpl(alloc);
  }

  static void SetFieldAttribute(const std::string &name, FieldAttrs &attr) {
#define FIELD_ATTR
#define ATTR(A)                     \
    if (!strcmp(name.c_str(), #A)) {  \
      attr.SetAttr(FLDATTR_##A);      \
      return;                         \
    }
#include "all_attributes.def"
#undef ATTR
#undef FIELD_ATTR
  }

 protected:
  virtual bool ProcessDeclImpl(MapleAllocator &allocator) = 0;
  virtual bool ProcessDeclWithContainerImpl(MapleAllocator &allocator) = 0;

  MapleAllocator &allocator;
  FieldPair mirFieldPair;
};

class FEInputMethodHelper {
 public:
  explicit FEInputMethodHelper(MapleAllocator &allocatorIn)
      : allocator(allocatorIn),
        srcLang(kSrcLangUnknown),
        feFunc(nullptr),
        mirFunc(nullptr),
        retType(nullptr),
        argTypes(allocator.Adapter()),
        retMIRType(nullptr),
        argMIRTypes(allocator.Adapter()),
        methodNameIdx(GStrIdx(0)) {}

  virtual ~FEInputMethodHelper() {
    feFunc = nullptr;
    mirFunc = nullptr;
    retType = nullptr;
    retMIRType = nullptr;
  }

  const MethodPair &GetMIRMethodPair() const {
    return mirMethodPair;
  }

  const FEIRType *GetReturnType() const {
    return retType;
  }

  const MapleVector<FEIRType*> &GetArgTypes() const {
    return argTypes;
  }

  bool ProcessDecl() {
    return ProcessDecl(allocator);
  }

  bool ProcessDecl(MapleAllocator &allocatorIn) {
    return ProcessDeclImpl(allocatorIn);
  }

  void SolveReturnAndArgTypes(MapleAllocator &allocatorIn) {
    SolveReturnAndArgTypesImpl(allocatorIn);
  }

  std::string GetMethodName(bool inMpl, bool full = true) const {
    return GetMethodNameImpl(inMpl, full);
  }

  FuncAttrs GetAttrs() const {
    return GetAttrsImpl();
  }

  bool IsStatic() const {
    return IsStaticImpl();
  }

  bool IsVirtual() const {
    return IsVirtualImpl();
  }

  bool IsNative() const {
    return IsNativeImpl();
  }

  bool IsVarg() const {
    return IsVargImpl();
  }

  bool HasThis() const {
    return HasThisImpl();
  }

  MIRType *GetTypeForThis() const {
    return GetTypeForThisImpl();
  }

  GStrIdx GetMethodNameIdx() const {
    return methodNameIdx;
  }

  bool HasCode() const {
    return HasCodeImpl();
  }

  void SetClassTypeInfo(const MIRStructType &structType) {
    mirFunc->SetClassTyIdx(structType.GetTypeIndex());
  }

 protected:
  virtual bool ProcessDeclImpl(MapleAllocator &allocatorIn);
  virtual void SolveReturnAndArgTypesImpl(MapleAllocator &allocator) = 0;
  virtual std::string GetMethodNameImpl(bool inMpl, bool full) const = 0;
  virtual FuncAttrs GetAttrsImpl() const = 0;
  virtual bool IsStaticImpl() const = 0;
  virtual bool IsVirtualImpl() const = 0;
  virtual bool IsNativeImpl() const = 0;
  virtual bool IsVargImpl() const = 0;
  virtual bool HasThisImpl() const = 0;
  virtual MIRType *GetTypeForThisImpl() const = 0;
  virtual bool HasCodeImpl() const = 0;

  MapleAllocator &allocator;
  MIRSrcLang srcLang;
  FEFunction *feFunc;
  MIRFunction *mirFunc;
  MethodPair mirMethodPair;
  FEIRType *retType;
  MapleVector<FEIRType*> argTypes;
  // Added MIRType to support C and extended to Java later.
  MIRType *retMIRType;
  MapleVector<MIRType*> argMIRTypes;
  GStrIdx methodNameIdx;
};

class FEInputStructHelper : public FEInputContainer {
 public:
  explicit FEInputStructHelper(MapleAllocator &allocatorIn)
      : allocator(allocatorIn),
        mirStructType(nullptr),
        mirSymbol(nullptr),
        fieldHelpers(allocator.Adapter()),
        methodHelpers(allocator.Adapter()),
        pragmaHelper(nullptr),
        staticFieldsConstVal(allocator.Adapter()),
        isSkipped(false),
        srcLang(kSrcLangJava) {}

  ~FEInputStructHelper() override {
    mirStructType = nullptr;
    mirSymbol = nullptr;
    pragmaHelper = nullptr;
  }

  bool IsSkipped() const {
    return isSkipped;
  }

  const TypeAttrs GetStructAttributeFromSymbol() const {
    if (mirSymbol != nullptr) {
      return mirSymbol->GetAttrs();
    }
    return TypeAttrs();
  }

  MIRSrcLang GetSrcLang() const {
    return srcLang;
  }

  const MapleList<FEInputMethodHelper*> &GetMethodHelpers() const {
    return methodHelpers;
  }

  bool PreProcessDecl() {
    return PreProcessDeclImpl();
  }

  bool ProcessDecl() {
    return ProcessDeclImpl();
  }

  std::string GetStructNameOrin() const {
    return GetStructNameOrinImpl();
  }

  std::string GetStructNameMpl() const {
    return GetStructNameMplImpl();
  }

  std::list<std::string> GetSuperClassNames() const {
    return GetSuperClassNamesImpl();
  }

  std::vector<std::string> GetInterfaceNames() const {
    return GetInterfaceNamesImpl();
  }

  std::string GetSourceFileName() const {
    return GetSourceFileNameImpl();
  }

  std::string GetSrcFileName() const {
    return GetSrcFileNameImpl();
  }

  MIRStructType *CreateMIRStructType(bool &error) const {
    return CreateMIRStructTypeImpl(error);
  }

  TypeAttrs GetStructAttributeFromInput() const {
    return GetStructAttributeFromInputImpl();
  }

  uint64 GetRawAccessFlags() const {
    return GetRawAccessFlagsImpl();
  }

  GStrIdx GetIRSrcFileSigIdx() const {
    return GetIRSrcFileSigIdxImpl();
  }

  bool IsMultiDef() const {
    return IsMultiDefImpl();
  }

  void InitFieldHelpers() {
    InitFieldHelpersImpl();
  }

  void InitMethodHelpers() {
    InitMethodHelpersImpl();
  }

  void SetPragmaHelper(FEInputPragmaHelper *pragmaHelperIn) {
    pragmaHelper = pragmaHelperIn;
  }

  void SetStaticFieldsConstVal(const std::vector<MIRConst*> &val) {
    staticFieldsConstVal.resize(val.size());
    for (uint32 i = 0; i < val.size(); ++i) {
      staticFieldsConstVal[i] = val[i];
    }
  }

  void SetFinalStaticStringIDVec(const std::vector<uint32> &stringIDVec) {
    finalStaticStringID = stringIDVec;
  }

  void SetIsOnDemandLoad(bool flag) {
    isOnDemandLoad = flag;
  }

  void ProcessPragma();

 protected:
  MIRStructType *GetContainerImpl() override;
  virtual bool PreProcessDeclImpl();
  virtual bool ProcessDeclImpl();
  virtual std::string GetStructNameOrinImpl() const = 0;
  virtual std::string GetStructNameMplImpl() const = 0;
  virtual std::list<std::string> GetSuperClassNamesImpl() const = 0;
  virtual std::vector<std::string> GetInterfaceNamesImpl() const = 0;
  virtual std::string GetSourceFileNameImpl() const = 0;
  virtual std::string GetSrcFileNameImpl() const;
  virtual MIRStructType *CreateMIRStructTypeImpl(bool &error) const = 0;
  virtual TypeAttrs GetStructAttributeFromInputImpl() const = 0;
  virtual uint64 GetRawAccessFlagsImpl() const = 0;
  virtual GStrIdx GetIRSrcFileSigIdxImpl() const = 0;
  virtual bool IsMultiDefImpl() const = 0;
  virtual void InitFieldHelpersImpl() = 0;
  virtual void InitMethodHelpersImpl() = 0;
  void CreateSymbol();
  void ProcessDeclSuperClass();
  void ProcessDeclSuperClassForJava();
  void ProcessDeclImplements();
  void ProcessDeclDefInfo();
  void ProcessDeclDefInfoSuperNameForJava();
  void ProcessDeclDefInfoImplementNameForJava();
  void ProcessFieldDef();
  void ProcessExtraFields();
  void ProcessMethodDef();
  void ProcessStaticFields();

  MapleAllocator &allocator;
  MIRStructType *mirStructType;
  MIRSymbol *mirSymbol;
  MapleList<FEInputFieldHelper*> fieldHelpers;
  MapleList<FEInputMethodHelper*> methodHelpers;
  FEInputPragmaHelper *pragmaHelper;
  MapleVector<MIRConst*> staticFieldsConstVal;
  std::vector<uint32> finalStaticStringID;
  bool isSkipped;
  MIRSrcLang srcLang;
  bool isOnDemandLoad = false;
};

class FEInputHelper {
 public:
  explicit FEInputHelper(MapleAllocator &allocatorIn)
      : allocator(allocatorIn),
        fieldHelpers(allocator.Adapter()),
        methodHelpers(allocator.Adapter()),
        structHelpers(allocator.Adapter()) {}
  ~FEInputHelper() = default;
  bool PreProcessDecl();
  bool ProcessDecl();
  bool ProcessImpl() const;
  void RegisterFieldHelper(FEInputFieldHelper &helper) {
    fieldHelpers.push_back(&helper);
  }

  void RegisterMethodHelper(FEInputMethodHelper &helper) {
    methodHelpers.push_back(&helper);
  }

  void RegisterStructHelper(FEInputStructHelper &helper) {
    structHelpers.push_back(&helper);
  }

 private:
  MapleAllocator &allocator;
  MapleList<FEInputFieldHelper*> fieldHelpers;
  MapleList<FEInputMethodHelper*> methodHelpers;
  MapleList<FEInputStructHelper*> structHelpers;
};
}  // namespace maple
#endif  // HIR2MPL_INCLUDE_FE_INPUT_HELPER_H
