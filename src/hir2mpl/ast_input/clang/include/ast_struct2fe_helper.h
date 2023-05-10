/*
 * Copyright (c) [2020-2022] Huawei Technologies Co.,Ltd.All rights reserved.
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
#ifndef HIR2MPL_AST_INPUT_INCLUDE_AST_STRUCT2FE_HELPER_H
#define HIR2MPL_AST_INPUT_INCLUDE_AST_STRUCT2FE_HELPER_H
#include "fe_input_helper.h"
#include "ast_decl.h"
#include "mempool_allocator.h"

namespace maple {
class ASTStruct2FEHelper : public FEInputStructHelper {
 public:
  ASTStruct2FEHelper(MapleAllocator &allocator, ASTStruct &structIn);
  ~ASTStruct2FEHelper() override = default;

  const ASTStruct &GetASTStruct() const {
    return astStruct;
  }

 protected:
  bool ProcessDeclImpl() override;
  void InitFieldHelpersImpl() override;
  void InitMethodHelpersImpl() override;
  TypeAttrs GetStructAttributeFromInputImpl() const override;
  std::string GetStructNameOrinImpl() const override;
  std::string GetStructNameMplImpl() const override;
  std::list<std::string> GetSuperClassNamesImpl() const override;
  std::vector<std::string> GetInterfaceNamesImpl() const override;
  std::string GetSourceFileNameImpl() const override;
  MIRStructType *CreateMIRStructTypeImpl(bool &error) const override;
  uint64 GetRawAccessFlagsImpl() const override;
  GStrIdx GetIRSrcFileSigIdxImpl() const override;
  bool IsMultiDefImpl() const override;
  std::string GetSrcFileNameImpl() const override;

  ASTStruct &astStruct;
};

class ASTGlobalVar2FEHelper : public FEInputGlobalVarHelper {
 public:
  ASTGlobalVar2FEHelper(MapleAllocator &allocatorIn, const ASTVar &varIn)
      : FEInputGlobalVarHelper(allocatorIn),
        astVar(varIn) {}
  ~ASTGlobalVar2FEHelper() override = default;

 protected:
  bool ProcessDeclImpl(MapleAllocator &allocator) override;
  const ASTVar &astVar;
};

class ASTFileScopeAsm2FEHelper : public FEInputFileScopeAsmHelper {
 public:
  ASTFileScopeAsm2FEHelper(MapleAllocator &allocatorIn, const ASTFileScopeAsm &astAsmIn)
      : FEInputFileScopeAsmHelper(allocatorIn),
        astAsm(astAsmIn) {}
  ~ASTFileScopeAsm2FEHelper() override = default;

 protected:
  bool ProcessDeclImpl(MapleAllocator &allocator) override;
  const ASTFileScopeAsm &astAsm;
};

class ASTEnum2FEHelper : public FEInputEnumHelper {
 public:
  ASTEnum2FEHelper(MapleAllocator &allocatorIn, const ASTEnumDecl &astEnumIn)
      : FEInputEnumHelper(allocatorIn),
        astEnum(astEnumIn) {}
  ~ASTEnum2FEHelper() override = default;

 protected:
  bool ProcessDeclImpl(MapleAllocator &allocator) override;
  const ASTEnumDecl &astEnum;
};

class ASTStructField2FEHelper : public FEInputFieldHelper {
 public:
  ASTStructField2FEHelper(MapleAllocator &allocator, ASTField &fieldIn, MIRType &structTypeIn)
      : FEInputFieldHelper(allocator),
        field(fieldIn), structType(structTypeIn) {}
  ~ASTStructField2FEHelper() override = default;

 protected:
  bool ProcessDeclImpl(MapleAllocator &allocator) override;
  bool ProcessDeclWithContainerImpl(MapleAllocator &allocator) override;
  ASTField &field;
  MIRType &structType;
};

class ASTFunc2FEHelper : public FEInputMethodHelper {
 public:
  ASTFunc2FEHelper(MapleAllocator &allocator, ASTFunc &funcIn)
      : FEInputMethodHelper(allocator),
        func(funcIn) {
    srcLang = kSrcLangC;
  }
  ~ASTFunc2FEHelper() override = default;
  ASTFunc &GetMethod() const {
    return func;
  }

  const std::string GetSrcFileName() const;

 protected:
  bool ProcessDeclImpl(MapleAllocator &allocator) override;
  void SolveReturnAndArgTypesImpl(MapleAllocator &allocator) override;
  std::string GetMethodNameImpl(bool inMpl, bool full) const override;
  bool IsVargImpl() const override;
  bool HasThisImpl() const override;
  MIRType *GetTypeForThisImpl() const override;
  FuncAttrs GetAttrsImpl() const override;
  bool IsStaticImpl() const override;
  bool IsVirtualImpl() const override;
  bool IsNativeImpl() const override;
  bool HasCodeImpl() const override;

  void SolveFunctionArguments() const;
  void SolveFunctionAttributes();
  ASTFunc &func;
  bool firstArgRet = false;
};
}  // namespace maple
#endif // HIR2MPL_AST_INPUT_INCLUDE_AST_STRUCT2FE_HELPER_H
