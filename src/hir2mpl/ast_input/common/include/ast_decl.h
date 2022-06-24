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
#ifndef HIR2MPL_AST_INPUT_INCLUDE_AST_DECL_H
#define HIR2MPL_AST_INPUT_INCLUDE_AST_DECL_H
#include <string>
#include <list>
#include <vector>
#include "types_def.h"
#include "ast_stmt.h"
#include "feir_var.h"
#include "fe_function.h"
#include "generic_attrs.h"

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

struct BoundaryInfo {
  ASTExpr *lenExpr = nullptr;
  int8 lenParamIdx = -1;  // -1 means not on the parameter
  bool isBytedLen = false;
};

class ASTDecl {
 public:
  ASTDecl(const MapleString &srcFile, const MapleString &nameIn, const MapleVector<MIRType*> &typeDescIn)
      : isGlobalDecl(false), srcFileName(srcFile), name(nameIn), typeDesc(typeDescIn) {
        isDbgFriendly = FEOptions::GetInstance().IsDbgFriendly();
      }
  virtual ~ASTDecl() = default;
  const std::string GetSrcFileName() const;
  const std::string GetName() const;
  const MapleVector<MIRType*> &GetTypeDesc() const;
  void SetTypeDesc(const MapleVector<MIRType*> &typeVecIn);
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

  void SetIsMacro(bool flag) {
    if (flag) {
      isMacroID = FEUtils::GetSequentialNumber();
    } else {
      isMacroID = flag;
    }
  }

  uint32 IsMacroID() const {
    return isMacroID;
  }

  void SetAlign(uint32 n) {
    if (n > align) {
      align = n;
    }
  }

  uint32 GetAlign() const {
    return align;
  }

  void SetAttr(GenericAttrKind attrKind) {
    genAttrs.SetAttr(attrKind);
  }

  void SetSectionAttr(const std::string &str) {
    sectionAttr = str;
  }

  const std::string &GetSectionAttr() const {
    return sectionAttr;
  }

  void GenerateInitStmt(std::list<UniqueFEIRStmt> &stmts) {
    return GenerateInitStmtImpl(stmts);
  }

  void SetDeclPos(const Pos &p) {
    pos = p;
  }

  void SetSrcLoc(const Loc &l) {
    loc = l;
  }

  const Loc &GetSrclOC() const {
    return loc;
  }

  uint32 GetSrcFileIdx() const {
    return loc.fileIdx;
  }

  uint32 GetSrcFileLineNum() const {
    return loc.line;
  }

  uint32 GetSrcFileColumn() const {
    return loc.column;
  }

  DeclKind GetDeclKind() const {
    return declKind;
  }

  MIRConst *Translate2MIRConst() const;

  std::string GenerateUniqueVarName() const;

  void SetBoundaryLenExpr(ASTExpr *expr) {
    boundary.lenExpr = expr;
  }

  const BoundaryInfo &GetBoundaryInfo() const {
    return boundary;
  }

  ASTExpr *GetBoundaryLenExpr() const {
    return boundary.lenExpr;
  }

  void SetBoundaryLenParamIdx(int8 idx) {
    boundary.lenParamIdx = idx;
  }

  int8 GetBoundaryLenParamIdx() const {
    return boundary.lenParamIdx;
  }

  void SetIsBytedLen(bool flag) {
    boundary.isBytedLen = flag;
  }

  bool IsBytedLen() const {
    return boundary.isBytedLen;
  }

  void ClearGenericAttrsContentMap() {
    genAttrs.ClearContentMap();
  }

 protected:
  virtual MIRConst *Translate2MIRConstImpl() const {
    CHECK_FATAL(false, "Maybe implemented for other ASTDecls");
    return nullptr;
  }
  virtual void GenerateInitStmtImpl(std::list<UniqueFEIRStmt> &stmts) {}
  bool isGlobalDecl;
  bool isParam = false;
  bool isDbgFriendly = false;
  uint32 align = 1; // in byte
  const MapleString srcFileName;

  MapleString name;
  MapleVector<MIRType*> typeDesc;
  GenericAttrs genAttrs;
  Loc loc = { 0, 0, 0 };
  Pos pos = {0 , 0};
  uint32 isMacroID = false;
  DeclKind declKind = kASTDecl;
  BoundaryInfo boundary;
  std::string sectionAttr;
};

class ASTField : public ASTDecl {
 public:
  ASTField(const MapleString &srcFile, const MapleString &nameIn, const MapleVector<MIRType*> &typeDescIn,
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
  ASTFunc(const MapleString &srcFile, const MapleString &nameIn, const MapleVector<MIRType*> &typeDescIn,
          const GenericAttrs &genAttrsIn, const MapleVector<ASTDecl*> &paramDeclsIn)
      : ASTDecl(srcFile, nameIn, typeDescIn), compound(nullptr), paramDecls(paramDeclsIn) {
    genAttrs = genAttrsIn;
    declKind = kASTFunc;
  }
  ~ASTFunc() {
    compound = nullptr;
  }
  void SetCompoundStmt(ASTStmt*);
  void InsertStmtsIntoCompoundStmtAtFront(const std::list<ASTStmt*> &stmts);
  const ASTStmt *GetCompoundStmt() const;
  const MapleVector<ASTDecl*> &GetParamDecls() const {
    return paramDecls;
  }
  std::vector<std::unique_ptr<FEIRVar>> GenArgVarList() const;
  std::list<UniqueFEIRStmt> EmitASTStmtToFEIR() const;
  std::list<UniqueFEIRStmt> InitArgsBoundaryVar(MIRFunction &mirFunc) const;
  void InsertBoundaryCheckingInRet(std::list<UniqueFEIRStmt> &stmts) const;

  void SetWeakrefAttr(const std::pair<bool, std::string> &attr) {
    weakrefAttr = attr;
  }

  const std::pair<bool, std::string> &GetWeakrefAttr() const {
    return weakrefAttr;
  }

  uint32 GetSize() const {
    return bodySize;
  }

  void SetSize(uint32 size) {
    bodySize = size;
  }

  bool HasCode() const {
    if (compound == nullptr) {
      return false;
    }
    return true;
  }

 private:
  // typeDesc format: [funcType, retType, arg0, arg1 ... argN]
  ASTStmt *compound = nullptr;  // func body
  MapleVector<ASTDecl*> paramDecls;
  std::pair<bool, std::string> weakrefAttr;
  uint32 bodySize = 0;
};

class ASTStruct : public ASTDecl {
 public:
  ASTStruct(MapleAllocator &allocatorIn, const MapleString &srcFile, const MapleString &nameIn,
            const MapleVector<MIRType*> &typeDescIn, const GenericAttrs &genAttrsIn)
      : ASTDecl(srcFile, nameIn, typeDescIn),
        isUnion(false), fields(allocatorIn.Adapter()), methods(allocatorIn.Adapter()) {
    genAttrs = genAttrsIn;
    declKind = kASTStruct;
  }
  ~ASTStruct() = default;

  std::string GetStructName(bool mapled) const;

  void SetField(ASTField *f) {
    fields.emplace_back(f);
  }

  const MapleList<ASTField*> &GetFields() const {
    return fields;
  }

  void SetIsUnion() {
    isUnion = true;
  }

  bool IsUnion() const {
    return isUnion;
  }

 private:
  bool isUnion = false;
  MapleList<ASTField*> fields;
  MapleList<ASTFunc*> methods;
};

class ASTVar : public ASTDecl {
 public:
  ASTVar(const MapleString &srcFile, const MapleString &nameIn, const MapleVector<MIRType*> &typeDescIn,
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

  void SetAsmAttr(const std::string &str) {
    asmAttr = str;
  }

  const std::string &GetAsmAttr() const {
    return asmAttr;
  }

  void SetVariableArrayExpr(ASTExpr *expr) {
    variableArrayExpr = expr;
  }

  void SetPromotedType(PrimType primType) {
    promotedType = primType;
  }

  PrimType GetPromotedType() const {
    return promotedType;
  }

  std::unique_ptr<FEIRVar> Translate2FEIRVar() const;
  MIRSymbol *Translate2MIRSymbol() const;

 private:
  MIRConst *Translate2MIRConstImpl() const override;
  void GenerateInitStmtImpl(std::list<UniqueFEIRStmt> &stmts) override;
  void GenerateInitStmt4StringLiteral(ASTExpr *initASTExpr, const UniqueFEIRVar &feirVar,
                                      const UniqueFEIRExpr &initFeirExpr, std::list<UniqueFEIRStmt> &stmts);
  ASTExpr *initExpr = nullptr;
  std::string asmAttr;
  ASTExpr *variableArrayExpr = nullptr;
  PrimType promotedType = PTY_void;
  bool hasAddedInMIRScope = false;
};

class ASTFileScopeAsm : public ASTDecl {
 public:
  ASTFileScopeAsm(MapleAllocator &allocatorIn, const MapleString &srcFile)
      : ASTDecl(srcFile, MapleString("", allocatorIn.GetMemPool()), MapleVector<MIRType*>(allocatorIn.Adapter())) {
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
  ASTEnumConstant(const MapleString &srcFile, const MapleString &nameIn, const MapleVector<MIRType*> &typeDescIn,
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
  ASTEnumDecl(MapleAllocator &allocatorIn, const MapleString &srcFile, const MapleString &nameIn,
              const MapleVector<MIRType*> &typeDescIn, const GenericAttrs &genAttrsIn)
      : ASTDecl(srcFile, nameIn, typeDescIn), consts(allocatorIn.Adapter()) {
    genAttrs = genAttrsIn;
    declKind = kASTEnumDecl;
  }
  ~ASTEnumDecl() = default;

  void PushConstant(ASTEnumConstant *c) {
    consts.emplace_back(c);
  }

 private:
  MapleList<ASTEnumConstant*> consts;
};
}  // namespace maple
#endif // HIR2MPL_AST_INPUT_INCLUDE_AST_DECL_H
