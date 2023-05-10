/*
 * Copyright (c) [2019-2022] Huawei Technologies Co.,Ltd.All rights reserved.
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
#ifndef MAPLE_IR_INCLUDE_MIR_PARSER_H
#define MAPLE_IR_INCLUDE_MIR_PARSER_H
#include "mir_module.h"
#include "lexer.h"
#include "mir_nodes.h"
#include "mir_preg.h"
#include "mir_scope.h"
#include "parser_opt.h"

namespace maple {
using BaseNodePtr = BaseNode*;
using StmtNodePtr = StmtNode*;
using BlockNodePtr = BlockNode*;

class FormalDef;

class MIRParser {
 public:
  explicit MIRParser(MIRModule &md)
      : lexer(md.GetDbgInfo(), md.GetMPAllocator()),
        mod(md),
        definedLabels(mod.GetMPAllocator().Adapter()) {
    safeRegionFlag.push(false);
  }

  ~MIRParser() = default;

  MIRFunction *CreateDummyFunction();
  void ResetCurrentFunction() {
    mod.SetCurFunction(dummyFunction);
  }

  bool ParseLoc();
  bool ParseLocStmt(StmtNodePtr &stmt);
  bool ParsePosition(SrcPosition &pos);
  bool ParseOneScope(MIRScope &scope);
  bool ParseScope();
  bool ParseScopeStmt(StmtNodePtr &stmt);
  bool ParseOneAlias(GStrIdx &strIdx, MIRAliasVars &aliasVar);
  bool ParseAlias();
  bool ParseAliasStmt(StmtNodePtr &stmt);
  bool ParseTypeAlias(MIRScope &scope);
  uint8 *ParseWordsInfo(uint32 size);
  bool ParseSwitchCase(int64 &constVal, LabelIdx &lblIdx);
  bool ParseExprOneOperand(BaseNodePtr &expr);
  bool ParseExprTwoOperand(BaseNodePtr &opnd0, BaseNodePtr &opnd1);
  bool ParseExprNaryOperand(MapleVector<BaseNode*> &opndVec);
  bool IsDelimitationTK(TokenKind tk) const;
  Opcode GetOpFromToken(TokenKind tk) const;
  bool IsStatement(TokenKind tk) const;
  PrimType GetPrimitiveType(TokenKind tk) const;
  MIRIntrinsicID GetIntrinsicID(TokenKind tk) const;
  bool ParseScalarValue(MIRConstPtr &stype, MIRType &type);
  bool ParseConstAddrLeafExpr(MIRConstPtr &cexpr);
  bool ParseInitValue(MIRConstPtr &theConst, TyIdx tyIdx, bool allowEmpty = false);
  bool ParseDeclaredSt(StIdx &stidx);
  void CreateFuncMIRSymbol(PUIdx &puidx, GStrIdx strIdx);
  bool ParseDeclaredFunc(PUIdx &puidx);
  bool ParseTypeAttrs(TypeAttrs &attrs);
  bool ParseVarTypeAttrs(MIRSymbol &st);
  bool CheckAlignTk();
  bool ParseAlignAttrs(TypeAttrs &tA);
  bool ParsePackAttrs();
  bool ParseFieldAttrs(FieldAttrs &attrs);
  bool ParseFuncAttrs(FuncAttrs &attrs);
  void SetAttrContent(FuncAttrs &attrs, FuncAttrKind x, const MIRLexer &mirLexer) const;
  bool CheckPrimAndDerivedType(TokenKind tokenKind, TyIdx &tyIdx);
  bool ParsePrimType(TyIdx &tyIdx);
  bool ParseFarrayType(TyIdx &arrayTyIdx);
  bool ParseArrayType(TyIdx &arrayTyIdx);
  bool ParseBitFieldType(TyIdx &fieldTyIdx);
  bool ParsePragmaElement(MIRPragmaElement &elem);
  bool ParsePragmaElementForArray(MIRPragmaElement &elem);
  bool ParsePragmaElementForAnnotation(MIRPragmaElement &elem);
  bool ParsePragma(MIRStructType &type);
  bool ParseFields(MIRStructType &type);
  bool ParseStructType(TyIdx &styIdx, const GStrIdx &strIdx = GStrIdx(0));
  bool ParseClassType(TyIdx &styidx, const GStrIdx &strIdx = GStrIdx(0));
  bool ParseInterfaceType(TyIdx &sTyIdx, const GStrIdx &strIdx = GStrIdx(0));
  bool ParseDefinedTypename(TyIdx &definedTyIdx, MIRTypeKind kind = kTypeUnknown);
  bool ParseTypeParam(TyIdx &definedTyIdx);
  bool ParsePointType(TyIdx &tyIdx);
  bool ParseFuncType(TyIdx &tyIdx);
  bool ParseGenericInstantVector(MIRInstantVectorType &insVecType);
  bool ParseDerivedType(TyIdx &tyIdx, MIRTypeKind kind = kTypeUnknown, const GStrIdx &strIdx = GStrIdx(0));
  bool ParseType(TyIdx &tyIdx);
  bool ParseEnumeration();
  bool ParseStatement(StmtNodePtr &stmt);
  bool ParseSpecialReg(PregIdx &pRegIdx);
  bool ParsePseudoReg(PrimType primType, PregIdx &pRegIdx);
  bool ParseStmtBlock(BlockNodePtr &blk);
  bool ParsePrototype(MIRFunction &func, MIRSymbol &funcSymbol, TyIdx &funcTyIdx);
  bool ParseFunction(uint32 fileIdx = 0);
  bool ParseStorageClass(MIRSymbol &symbol) const;
  bool ParseDeclareVarInitValue(MIRSymbol &symbol);
  bool ParseDeclareVar(MIRSymbol &symbol);
  bool ParseDeclareReg(MIRSymbol &symbol, const MIRFunction &func);
  bool ParseDeclareFormal(FormalDef &formalDef);
  bool ParsePrototypeRemaining(MIRFunction &func, std::vector<TyIdx> &vecTyIdx,
                               std::vector<TypeAttrs> &vecAttrs, bool &varArgs);

  // Stmt Parser
  bool ParseStmtDassign(StmtNodePtr &stmt);
  bool ParseStmtDassignoff(StmtNodePtr &stmt);
  bool ParseStmtRegassign(StmtNodePtr &stmt);
  bool ParseStmtIassign(StmtNodePtr &stmt);
  bool ParseStmtIassignoff(StmtNodePtr &stmt);
  bool ParseStmtIassignFPoff(StmtNodePtr &stmt);
  bool ParseStmtBlkassignoff(StmtNodePtr &stmt);
  bool ParseStmtDoloop(StmtNodePtr &stmt);
  bool ParseStmtForeachelem(StmtNodePtr &stmt);
  bool ParseStmtDowhile(StmtNodePtr &stmt);
  bool ParseStmtIf(StmtNodePtr &stmt);
  bool ParseStmtWhile(StmtNodePtr &stmt);
  bool ParseStmtLabel(StmtNodePtr &stmt);
  bool ParseStmtGoto(StmtNodePtr &stmt);
  bool ParseStmtBr(StmtNodePtr &stmt);
  bool ParseStmtSwitch(StmtNodePtr &stmt);
  bool ParseStmtRangegoto(StmtNodePtr &stmt);
  bool ParseStmtMultiway(StmtNodePtr &stmt);
  PUIdx EnterUndeclaredFunction(bool isMcount = false);  // for -pg in order to add "void _mcount()"
  bool ParseStmtCall(StmtNodePtr &stmt);
  bool ParseStmtCallMcount(StmtNodePtr &stmt);  // for -pg in order to add "void _mcount()" to all the functions
  bool ParseStmtIcall(StmtNodePtr &stmt, Opcode op);
  bool ParseStmtIcall(StmtNodePtr &stmt);
  bool ParseStmtIcallassigned(StmtNodePtr &stmt);
  bool ParseStmtIcallproto(StmtNodePtr &stmt);
  bool ParseStmtIcallprotoassigned(StmtNodePtr &stmt);
  bool ParseStmtIntrinsiccall(StmtNodePtr &stmt, bool isAssigned);
  bool ParseStmtIntrinsiccall(StmtNodePtr &stmt);
  bool ParseStmtIntrinsiccallassigned(StmtNodePtr &stmt);
  bool ParseStmtIntrinsiccallwithtype(StmtNodePtr &stmt, bool isAssigned);
  bool ParseStmtIntrinsiccallwithtype(StmtNodePtr &stmt);
  bool ParseStmtIntrinsiccallwithtypeassigned(StmtNodePtr &stmt);
  bool ParseCallReturnPair(CallReturnPair &retpair);
  bool ParseCallReturns(CallReturnVector &retsvec);
  bool ParseBinaryStmt(StmtNodePtr &stmt, Opcode op);
  bool ParseNaryStmtAssert(StmtNodePtr &stmt, Opcode op);
  bool ParseNaryStmtAssertGE(StmtNodePtr &stmt);
  bool ParseNaryStmtAssertLT(StmtNodePtr &stmt);
  bool ParseNaryStmtCalcassertGE(StmtNodePtr &stmt);
  bool ParseNaryStmtCalcassertLT(StmtNodePtr &stmt);
  bool ParseNaryStmtCallAssertLE(StmtNodePtr &stmt);
  bool ParseNaryStmtReturnAssertLE(StmtNodePtr &stmt);
  bool ParseNaryStmtAssignAssertLE(StmtNodePtr &stmt);
  bool ParseNaryStmt(StmtNodePtr &stmt, Opcode op);
  bool ParseNaryStmtReturn(StmtNodePtr &stmt);
  bool ParseNaryStmtSyncEnter(StmtNodePtr &stmt);
  bool ParseNaryStmtSyncExit(StmtNodePtr &stmt);
  bool ParseStmtJsTry(StmtNodePtr &stmt);
  bool ParseStmtTry(StmtNodePtr &stmt);
  bool ParseStmtCatch(StmtNodePtr &stmt);
  bool ParseUnaryStmt(Opcode op, StmtNodePtr &stmt);
  bool ParseUnaryStmtThrow(StmtNodePtr &stmt);
  bool ParseUnaryStmtDecRef(StmtNodePtr &stmt);
  bool ParseUnaryStmtIncRef(StmtNodePtr &stmt);
  bool ParseUnaryStmtDecRefReset(StmtNodePtr &stmt);
  bool ParseUnaryStmtIGoto(StmtNodePtr &stmt);
  bool ParseUnaryStmtEval(StmtNodePtr &stmt);
  bool ParseUnaryStmtFree(StmtNodePtr &stmt);
  bool ParseUnaryStmtAssertNonNullCheck(Opcode op, StmtNodePtr &stmt);
  bool ParseUnaryStmtAssertNonNull(StmtNodePtr &stmt);
  bool ParseUnaryStmtCallAssertNonNull(StmtNodePtr &stmt);
  bool ParseUnaryStmtAssignAssertNonNull(StmtNodePtr &stmt);
  bool ParseUnaryStmtReturnAssertNonNull(StmtNodePtr &stmt);
  bool ParseStmtMarker(StmtNodePtr &stmt);
  bool ParseStmtGosub(StmtNodePtr &stmt);
  bool ParseStmtAsm(StmtNodePtr &stmt);
  bool ParseStmtSafeRegion(StmtNodePtr &stmt);

  // Expression Parser
  bool ParseExpression(BaseNodePtr &expr);
  bool ParseExprDread(BaseNodePtr &expr);
  bool ParseExprDreadoff(BaseNodePtr &expr);
  bool ParseExprRegread(BaseNodePtr &expr);
  bool ParseExprBinary(BaseNodePtr &expr);
  bool ParseExprCompare(BaseNodePtr &expr);
  bool ParseExprDepositbits(BaseNodePtr &expr);
  bool ParseExprConstval(BaseNodePtr &expr);
  bool ParseExprConststr(BaseNodePtr &expr);
  bool ParseExprConststr16(BaseNodePtr &expr);
  bool ParseExprSizeoftype(BaseNodePtr &expr);
  bool ParseExprFieldsDist(BaseNodePtr &expr);
  bool ParseExprIreadIaddrof(IreadNode &expr);
  bool ParseExprIread(BaseNodePtr &expr);
  bool ParseExprIreadoff(BaseNodePtr &expr);
  bool ParseExprIreadFPoff(BaseNodePtr &expr);
  bool ParseExprIaddrof(BaseNodePtr &expr);
  bool ParseExprAddrof(BaseNodePtr &expr);
  bool ParseExprAddrofoff(BaseNodePtr &expr);
  bool ParseExprAddroffunc(BaseNodePtr &expr);
  bool ParseExprAddroflabel(BaseNodePtr &expr);
  bool ParseExprUnary(BaseNodePtr &expr);
  bool ParseExprJarray(BaseNodePtr &expr);
  bool ParseExprSTACKJarray(BaseNodePtr &expr);
  bool ParseExprGCMalloc(BaseNodePtr &expr);
  bool ParseExprExtractbits(BaseNodePtr &expr);
  bool ParseExprTyconvert(BaseNodePtr &expr);
  bool ParseExprRetype(BaseNodePtr &expr);
  bool ParseExprTernary(BaseNodePtr &expr);
  bool ParseExprArray(BaseNodePtr &expr);
  bool ParseExprIntrinsicop(BaseNodePtr &expr);
  bool ParseNaryExpr(NaryStmtNode &stmtNode);

  // funcName and paramIndex is out parameter
  bool ParseCallAssertInfo(std::string &funcName, int *paramIndex, std::string &stmtFuncName);
  bool ParseAssertInfo(std::string &funcName);
  bool ParseTypeDefine();
  bool ParseJavaClassInterface(MIRSymbol &symbol, bool isClass);
  bool ParseIntrinsicId(IntrinsicopNode &intrnOpNode);
  void Error(const std::string &str);
  void Warning(const std::string &str);
  void FixForwardReferencedTypeForOneAgg(MIRType *type);
  void FixupForwardReferencedTypeByMap();

  const std::string &GetError();
  const std::string &GetWarning() const;
  bool ParseFuncInfo(void);
  void PrepareParsingMIR();
  void PrepareParsingMplt();
  bool ParseSrcLang(MIRSrcLang &srcLang);
  bool ParseMIR(uint32 fileIdx = 0, uint32 option = 0, bool isIPA = false, bool isComb = false);
  bool ParseMIR(std::ifstream &mplFile);  // the main entry point
  bool ParseInlineFuncBody(std::ifstream &mplFile);
  bool ParseMPLT(std::ifstream &mpltFile, const std::string &importFileName);
  bool ParseMPLTStandalone(std::ifstream &mpltFile, const std::string &importFileName);
  bool ParseTypeFromString(const std::string &src, TyIdx &tyIdx);
  void EmitError(const std::string &fileName);
  void EmitWarning(const std::string &fileName) const;
  uint32 GetOptions() const {
    return options;
  }

 private:
  // func ptr map for ParseMIR()
  using FuncPtrParseMIRForElem = bool (MIRParser::*)();
  static std::map<TokenKind, FuncPtrParseMIRForElem> funcPtrMapForParseMIR;
  static std::map<TokenKind, FuncPtrParseMIRForElem> InitFuncPtrMapForParseMIR();

  bool TypeCompatible(const TyIdx &typeIdx1, const TyIdx &typeIdx2) const;
  bool IsTypeIncomplete(MIRType *type) const;

  // func for ParseMIR
  bool ParseMIRForFunc();
  bool ParseMIRForVar();
  bool ParseMIRForClass();
  bool ParseMIRForInterface();
  bool ParseMIRForFlavor();
  bool ParseMIRForSrcLang();
  bool ParseMIRForGlobalMemSize();
  bool ParseMIRForGlobalMemMap();
  bool ParseMIRForGlobalWordsTypeTagged();
  bool ParseMIRForGlobalWordsRefCounted();
  bool ParseMIRForID();
  bool ParseMIRForNumFuncs();
  bool ParseMIRForEntryFunc();
  bool ParseMIRForFileInfo();
  bool ParseMIRForFileData();
  bool ParseMIRForSrcFileInfo();
  bool ParseMIRForImport();
  bool ParseMIRForImportPath();
  bool ParseMIRForAsmdecl();

  // func for ParseExpr
  using FuncPtrParseExpr = bool (MIRParser::*)(BaseNodePtr &ptr);
  static std::map<TokenKind, FuncPtrParseExpr> funcPtrMapForParseExpr;
  static std::map<TokenKind, FuncPtrParseExpr> InitFuncPtrMapForParseExpr();

  // func and param for ParseStmt
  using FuncPtrParseStmt = bool (MIRParser::*)(StmtNodePtr &stmt);
  static std::map<TokenKind, FuncPtrParseStmt> funcPtrMapForParseStmt;
  static std::map<TokenKind, FuncPtrParseStmt> InitFuncPtrMapForParseStmt();

  // func and param for ParseStmtBlock
  using FuncPtrParseStmtBlock = bool (MIRParser::*)();
  static std::map<TokenKind, FuncPtrParseStmtBlock> funcPtrMapForParseStmtBlock;
  static std::map<TokenKind, FuncPtrParseStmtBlock> InitFuncPtrMapForParseStmtBlock();
  void ParseStmtBlockForSeenComment(BlockNodePtr blk, uint32 mplNum);
  bool ParseStmtBlockForVar(TokenKind stmtTK);
  bool ParseStmtBlockForVar();
  bool ParseStmtBlockForTempVar();
  bool ParseStmtBlockForReg();
  bool ParseStmtBlockForType();
  bool ParseStmtBlockForFrameSize();
  bool ParseStmtBlockForUpformalSize();
  bool ParseStmtBlockForModuleID();
  bool ParseStmtBlockForFuncSize();
  bool ParseStmtBlockForFuncID();
  bool ParseStmtBlockForFormalWordsTypeTagged();
  bool ParseStmtBlockForLocalWordsTypeTagged();
  bool ParseStmtBlockForFormalWordsRefCounted();
  bool ParseStmtBlockForLocalWordsRefCounted();
  bool ParseStmtBlockForFuncInfo();

  // common func
  void SetSrcPos(SrcPosition &srcPosition, uint32 mplNum) const;

  // func for ParseExpr
  Opcode paramOpForStmt = OP_undef;
  TokenKind paramTokenKindForStmt = TK_invalid;
  // func and param for ParseStmtBlock
  MIRFunction *paramCurrFuncForParseStmtBlock = nullptr;
  MIRLexer lexer;
  MIRModule &mod;
  std::string message;
  std::string warningMessage;
  uint32 options = kKeepFirst;
  MapleVector<bool> definedLabels;           // true if label at labidx is defined
  MIRFunction *dummyFunction = nullptr;
  MIRFunction *curFunc = nullptr;
  uint16 lastFileNum = 0;                    // to remember first number after LOC
  uint32 lastLineNum = 0;                    // to remember second number after LOC
  uint16 lastColumnNum = 0;                  // to remember third number after LOC
  uint32 firstLineNum = 0;                   // to track function starting line
  std::map<TyIdx, TyIdx> typeDefIdxMap;      // map previous declared tyIdx
  bool firstImport = true;                   // Mark the first imported mplt file
  bool paramParseLocalType = false;          // param for ParseTypeDefine
  uint32 paramFileIdx = 0;                   // param for ParseMIR()
  bool paramIsIPA = false;
  bool paramIsComb = false;
  TokenKind paramTokenKind = TK_invalid;
  std::vector<std::string> paramImportFileList;
  std::stack<bool> safeRegionFlag;
};
}  // namespace maple
#endif  // MAPLE_IR_INCLUDE_MIR_PARSER_H
