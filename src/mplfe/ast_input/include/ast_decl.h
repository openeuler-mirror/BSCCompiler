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
#ifndef MPLFE_AST_INPUT_INCLUDE_AST_DECL_H
#define MPLFE_AST_INPUT_INCLUDE_AST_DECL_H
#include <string>
#include <list>
#include <vector>
#include "types_def.h"
#include "ast_stmt.h"
#include "feir_var.h"
#include "fe_function.h"

namespace maple {
using Pos = std::pair<uint32, uint32>;
enum DeclKind {
  kUnknownDecl = 0,
  kASTDecl,
  kASTField,
  kASTFunc,
  kASTStruct,
  kASTVar,
  kASTEnumConstant,
  kASTEnumDecl,
  kASTFileScopeAsm,
};

class ASTDecl {
 public:
  ASTDecl(const std::string &srcFile, const std::string &nameIn, const std::vector<MIRType*> &typeDescIn)
      : isGlobalDecl(false), srcFileName(srcFile), name(nameIn), typeDesc(typeDescIn) {}
  virtual ~ASTDecl() = default;
  const std::string &GetSrcFileName() const;
  const std::string &GetName() const;
  const std::vector<MIRType*> &GetTypeDesc() const;
  GenericAttrs GetGenericAttrs() const {
    return genAttrs;
  }

  void SetGlobal(bool isGlobal) {
    isGlobalDecl = isGlobal;
  }

  bool IsGlobal() const {
    return isGlobalDecl;
  }

  void SetIsParam(bool flag) {
    isParam = flag;
  }

  bool IsParam() const {
    return isParam;
  }

  void SetAlign(uint32 n) {
    if (n > align) {
      align = n;
    }
  }

  uint32 GetAlign() const {
    return align;
  }

  void GenerateInitStmt(std::list<UniqueFEIRStmt> &stmts) {
    return GenerateInitStmtImpl(stmts);
  }

  void SetDeclPos(Pos p) {
    pos = p;
  }

  void SetSrcLOC(uint32 fileIdx, uint32 lineNum) {
    srcFileIdx = fileIdx;
    srcFileLineNum = lineNum;
  }

  uint32 GetSrcFileIdx() const {
    return srcFileIdx;
  }

  uint32 GetSrcFileLineNum() const {
    return srcFileLineNum;
  }

  DeclKind GetDeclKind() const {
    return declKind;
  }

  MIRConst *Translate2MIRConst() const;

  std::string GenerateUniqueVarName() const;

 protected:
  virtual MIRConst *Translate2MIRConstImpl() const {
    CHECK_FATAL(false, "Maybe implemented for other ASTDecls");
    return nullptr;
  }
  virtual void GenerateInitStmtImpl(std::list<UniqueFEIRStmt> &stmts) {}
  bool isGlobalDecl;
  bool isParam = false;
  uint32 align = 1; // in byte
  const std::string srcFileName;
  std::string name;
  std::vector<MIRType*> typeDesc;
  GenericAttrs genAttrs;
  Pos pos = { 0, 0 };
  uint32 srcFileIdx = 0;
  uint32 srcFileLineNum = 0;
  DeclKind declKind = kASTDecl;
};

class ASTField : public ASTDecl {
 public:
  ASTField(const std::string &srcFile, const std::string &nameIn, const std::vector<MIRType*> &typeDescIn,
           const GenericAttrs &genAttrsIn, bool isAnonymous = false)
      : ASTDecl(srcFile, nameIn, typeDescIn), isAnonymousField(isAnonymous) {
    genAttrs = genAttrsIn;
    declKind = kASTField;
  }
  ~ASTField() = default;
  bool IsAnonymousField() const {
    return isAnonymousField;
  }

 private:
  bool isAnonymousField = false;
};

class ASTFunc : public ASTDecl {
 public:
  ASTFunc(const std::string &srcFile, const std::string &nameIn, const std::vector<MIRType*> &typeDescIn,
          const GenericAttrs &genAttrsIn, const std::vector<std::string> &parmNamesIn)
      : ASTDecl(srcFile, nameIn, typeDescIn), compound(nullptr), parmNames(parmNamesIn) {
    genAttrs = genAttrsIn;
    declKind = kASTFunc;
  }
  ~ASTFunc() {
    compound = nullptr;
  }
  void SetCompoundStmt(ASTStmt*);
  void InsertStmtsIntoCompoundStmtAtFront(const std::list<ASTStmt*> &stmts);
  const ASTStmt *GetCompoundStmt() const;
  const std::vector<std::string> &GetParmNames() const {
    return parmNames;
  }
  std::vector<std::unique_ptr<FEIRVar>> GenArgVarList() const;
  std::list<UniqueFEIRStmt> EmitASTStmtToFEIR() const;

 private:
  // typeDesc format: [funcType, retType, arg0, arg1 ... argN]
  ASTStmt *compound;  // func body
  std::vector<std::string> parmNames;
};

class ASTStruct : public ASTDecl {
 public:
  ASTStruct(const std::string &srcFile, const std::string &nameIn, const std::vector<MIRType*> &typeDescIn,
            const GenericAttrs &genAttrsIn)
      : ASTDecl(srcFile, nameIn, typeDescIn), isUnion(false) {
    genAttrs = genAttrsIn;
    declKind = kASTStruct;
  }
  ~ASTStruct() = default;

  std::string GetStructName(bool mapled) const;

  void SetField(ASTField *f) {
    fields.emplace_back(f);
  }

  const std::list<ASTField*> &GetFields() const {
    return fields;
  }

  void SetIsUnion() {
    isUnion = true;
  }

  bool IsUnion() const {
    return isUnion;
  }

 private:
  bool isUnion;
  std::list<ASTField*> fields;
  std::list<ASTFunc*> methods;
};

class ASTVar : public ASTDecl {
 public:
  ASTVar(const std::string &srcFile, const std::string &nameIn, const std::vector<MIRType*> &typeDescIn,
         const GenericAttrs &genAttrsIn)
      : ASTDecl(srcFile, nameIn, typeDescIn) {
    genAttrs = genAttrsIn;
    declKind = kASTVar;
  }
  virtual ~ASTVar() = default;

  void SetInitExpr(ASTExpr *init) {
    initExpr = init;
  }

  ASTExpr *GetInitExpr() const {
    return initExpr;
  }

  std::unique_ptr<FEIRVar> Translate2FEIRVar() const;
  MIRSymbol *Translate2MIRSymbol() const;

 private:
  MIRConst *Translate2MIRConstImpl() const override;
  void GenerateInitStmtImpl(std::list<UniqueFEIRStmt> &stmts) override;
  void GenerateInitStmt4StringLiteral(ASTExpr *initASTExpr, UniqueFEIRVar feirVar, UniqueFEIRExpr initFeirExpr,
                                      std::list<UniqueFEIRStmt> &stmts);
  ASTExpr *initExpr = nullptr;
};

class ASTFileScopeAsm : public ASTDecl {
 public:
  ASTFileScopeAsm(const std::string &srcFile)
      : ASTDecl(srcFile, "", std::vector<MIRType*>{}) {
    declKind = kASTFileScopeAsm;
  }
  ~ASTFileScopeAsm() = default;

  void SetAsmStr(const std::string &str) {
    asmStr = str;
  }

  const std::string &GetAsmStr() const {
    return asmStr;
  }

 private:
  std::string asmStr;
};

class ASTEnumConstant : public ASTDecl {
 public:
  ASTEnumConstant(const std::string &srcFile, const std::string &nameIn, const std::vector<MIRType*> &typeDescIn,
         const GenericAttrs &genAttrsIn)
      : ASTDecl(srcFile, nameIn, typeDescIn) {
    genAttrs = genAttrsIn;
    declKind = kASTEnumConstant;
  }
  ~ASTEnumConstant() = default;

  void SetValue(int32 val);
  int32 GetValue() const;
 private:
  MIRConst *Translate2MIRConstImpl() const override;
  int32 value = 0;
};

// only process local `EnumDecl` here
class ASTEnumDecl : public ASTDecl {
 public:
  ASTEnumDecl(const std::string &srcFile, const std::string &nameIn, const std::vector<MIRType*> &typeDescIn,
          const GenericAttrs &genAttrsIn)
      : ASTDecl(srcFile, nameIn, typeDescIn) {
    genAttrs = genAttrsIn;
    declKind = kASTEnumDecl;
  }
  ~ASTEnumDecl() = default;

  void PushConstant(ASTEnumConstant *c) {
    consts.emplace_back(c);
  }

 private:
  std::list<ASTEnumConstant*> consts;
};
}  // namespace maple
#endif // MPLFE_AST_INPUT_INCLUDE_AST_DECL_H
