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
#include "parser_opt.h"

namespace maple {
using BaseNodePtr = BaseNode*;
using StmtNodePtr = StmtNode*;
using BlockNodePtr = BlockNode*;

class FormalDef;

class MIRParser {
 public:
  explicit MIRParser(MIRModule &md)
      : lexer(md),
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
  bool ParseAlias(StmtNodePtr &stmt);
  uint8 *ParseWordsInfo(uint32 size);
  bool ParseSwitchCase(int64&, LabelIdx&);
  bool ParseExprOneOperand(BaseNodePtr &expr);
  bool ParseExprTwoOperand(BaseNodePtr &opnd0, BaseNodePtr &opnd1);
  bool ParseExprNaryOperand(MapleVector<BaseNode*>&);
  bool IsDelimitationTK(TokenKind tk) const;
  Opcode GetOpFromToken(TokenKind tk) const;
  bool IsStatement(TokenKind tk) const;
  PrimType GetPrimitiveType(TokenKind tk) const;
  MIRIntrinsicID GetIntrinsicID(TokenKind tk) const;
  bool ParseScalarValue(MIRConstPtr&, MIRType&);
  bool ParseConstAddrLeafExpr(MIRConstPtr&);
  bool ParseInitValue(MIRConstPtr&, TyIdx, bool allowEmpty = false);
  bool ParseDeclaredSt(StIdx&);
  void CreateFuncMIRSymbol(PUIdx &puidx, GStrIdx strIdx);
  bool ParseDeclaredFunc(PUIdx&);
  bool ParseTypeAttrs(TypeAttrs&);
  bool ParseVarTypeAttrs(MIRSymbol &st);
  bool CheckAlignTk();
  bool ParseAlignAttrs(TypeAttrs &tA);
  bool ParsePackAttrs();
  bool ParseFieldAttrs(FieldAttrs &tA);
  bool ParseFuncAttrs(FuncAttrs &tA);
  void SetAttrContent(FuncAttrs &attrs, FuncAttrKind x, const MIRLexer &lexer);
  bool CheckPrimAndDerivedType(TokenKind tk, TyIdx &tyIdx);
  bool ParsePrimType(TyIdx &tyIdx);
  bool ParseFarrayType(TyIdx &tyIdx);
  bool ParseArrayType(TyIdx &tyIdx);
  bool ParseBitFieldType(TyIdx &tyIdx);
  bool ParsePragmaElement(MIRPragmaElement &lem);
  bool ParsePragmaElementForArray(MIRPragmaElement &elem);
  bool ParsePragmaElementForAnnotation(MIRPragmaElement &elem);
  bool ParsePragma(MIRStructType &type);
  bool ParseFields(MIRStructType &type);
  bool ParseStructType(TyIdx &styIdx, const GStrIdx &strIdx = GStrIdx(0));
  bool ParseClassType(TyIdx &tyIdx, const GStrIdx &strIdx = GStrIdx(0));
  bool ParseInterfaceType(TyIdx &sTyIdx, const GStrIdx &strIdx = GStrIdx(0));
  bool ParseDefinedTypename(TyIdx &tyIdx, MIRTypeKind kind = kTypeUnknown);
  bool ParseTypeParam(TyIdx &tyIdx);
  bool ParsePointType(TyIdx &tyIdx);
  bool ParseFuncType(TyIdx &tyIdx);
  bool ParseGenericInstantVector(MIRInstantVectorType &insVecType);
  bool ParseDerivedType(TyIdx &tyIdx, MIRTypeKind kind = kTypeUnknown, const GStrIdx &strIdx = GStrIdx(0));
  bool ParseType(TyIdx &tyIdx);
  bool ParseStatement(StmtNodePtr &stmt);
  bool ParseSpecialReg(PregIdx &pregIdx);
  bool ParsePseudoReg(PrimType pty, PregIdx &pregIdx);
  bool ParseStmtBlock(BlockNodePtr &blk);
  bool ParsePrototype(MIRFunction &fn, MIRSymbol &funcSt, TyIdx &funcTyIdx);
  bool ParseFunction(uint32 fileIdx = 0);
  bool ParseStorageClass(MIRSymbol &st) const;
  bool ParseDeclareVarInitValue(MIRSymbol &symbol);
  bool ParseDeclareVar(MIRSymbol&);
  bool ParseDeclareReg(MIRSymbol&, const MIRFunction&);
  bool ParseDeclareFormal(FormalDef&);
  bool ParsePrototypeRemaining(MIRFunction&, std::vector<TyIdx> &, std::vector<TypeAttrs>&, bool&);

  // Stmt Parser
  bool ParseStmtDassign(StmtNodePtr &stmt);
  bool ParseStmtDassignoff(StmtNodePtr &stmt);
  bool ParseStmtRegassign(StmtNodePtr &stmt);
  bool ParseStmtIassign(StmtNodePtr &stmt);
  bool ParseStmtIassignoff(StmtNodePtr &stmt);
  bool ParseStmtIassignFPoff(StmtNodePtr &stmt);
  bool ParseStmtBlkassignoff(StmtNodePtr &stmt);
  bool ParseStmtDoloop(StmtNodePtr&);
  bool ParseStmtForeachelem(StmtNodePtr&);
  bool ParseStmtDowhile(StmtNodePtr&);
  bool ParseStmtIf(StmtNodePtr&);
  bool ParseStmtWhile(StmtNodePtr&);
  bool ParseStmtLabel(StmtNodePtr&);
  bool ParseStmtGoto(StmtNodePtr&);
  bool ParseStmtBr(StmtNodePtr&);
  bool ParseStmtSwitch(StmtNodePtr&);
  bool ParseStmtRangegoto(StmtNodePtr&);
  bool ParseStmtMultiway(StmtNodePtr&);
  PUIdx EnterUndeclaredFunction(bool isMcount = false);  // for -pg in order to add "void _mcount()"
  bool ParseStmtCall(StmtNodePtr&);
  bool ParseStmtCallMcount(StmtNodePtr&);  // for -pg in order to add "void _mcount()" to all the functions
  bool ParseStmtIcall(StmtNodePtr&, Opcode op);
  bool ParseStmtIcall(StmtNodePtr&);
  bool ParseStmtIcallassigned(StmtNodePtr&);
  bool ParseStmtIcallproto(StmtNodePtr&);
  bool ParseStmtIntrinsiccall(StmtNodePtr&, bool isAssigned);
  bool ParseStmtIntrinsiccall(StmtNodePtr&);
  bool ParseStmtIntrinsiccallassigned(StmtNodePtr&);
  bool ParseStmtIntrinsiccallwithtype(StmtNodePtr&, bool isAssigned);
  bool ParseStmtIntrinsiccallwithtype(StmtNodePtr&);
  bool ParseStmtIntrinsiccallwithtypeassigned(StmtNodePtr&);
  bool ParseCallReturnPair(CallReturnPair&);
  bool ParseCallReturns(CallReturnVector&);
  bool ParseBinaryStmt(StmtNodePtr&, Opcode op);
  bool ParseNaryStmtAssert(StmtNodePtr&, Opcode op);
  bool ParseNaryStmtAssertGE(StmtNodePtr&);
  bool ParseNaryStmtAssertLT(StmtNodePtr&);
  bool ParseNaryStmtCalcassertGE(StmtNodePtr &stmt);
  bool ParseNaryStmtCalcassertLT(StmtNodePtr &stmt);
  bool ParseNaryStmtCallAssertLE(StmtNodePtr&);
  bool ParseNaryStmtReturnAssertLE(StmtNodePtr&);
  bool ParseNaryStmtAssignAssertLE(StmtNodePtr&);
  bool ParseNaryStmt(StmtNodePtr&, Opcode op);
  bool ParseNaryStmtReturn(StmtNodePtr&);
  bool ParseNaryStmtSyncEnter(StmtNodePtr&);
  bool ParseNaryStmtSyncExit(StmtNodePtr&);
  bool ParseStmtJsTry(StmtNodePtr&);
  bool ParseStmtTry(StmtNodePtr&);
  bool ParseStmtCatch(StmtNodePtr&);
  bool ParseUnaryStmt(Opcode op, StmtNodePtr&);
  bool ParseUnaryStmtThrow(StmtNodePtr&);
  bool ParseUnaryStmtDecRef(StmtNodePtr&);
  bool ParseUnaryStmtIncRef(StmtNodePtr&);
  bool ParseUnaryStmtDecRefReset(StmtNodePtr&);
  bool ParseUnaryStmtIGoto(StmtNodePtr&);
  bool ParseUnaryStmtEval(StmtNodePtr&);
  bool ParseUnaryStmtFree(StmtNodePtr&);
  bool ParseUnaryStmtAssertNonNullCheck(Opcode op, StmtNodePtr&);
  bool ParseUnaryStmtAssertNonNull(StmtNodePtr&);
  bool ParseUnaryStmtCallAssertNonNull(StmtNodePtr&);
  bool ParseUnaryStmtAssignAssertNonNull(StmtNodePtr&);
  bool ParseUnaryStmtReturnAssertNonNull(StmtNodePtr&);
  bool ParseStmtMarker(StmtNodePtr&);
  bool ParseStmtGosub(StmtNodePtr&);
  bool ParseStmtAsm(StmtNodePtr&);
  bool ParseStmtSafeRegion(StmtNodePtr&);

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
  bool ParseTypedef();
  bool ParseJavaClassInterface(MIRSymbol&, bool);
  bool ParseIntrinsicId(IntrinsicopNode&);
  void Error(const std::string&);
  void Warning(const std::string&);
  void FixForwardReferencedTypeForOneAgg(MIRType *ty);
  void FixupForwardReferencedTypeByMap();

  const std::string &GetError();
  const std::string &GetWarning() const;
  bool ParseFuncInfo(void);
  void PrepareParsingMIR();
  void PrepareParsingMplt();
  bool ParseSrcLang(MIRSrcLang &srcLang);
  bool ParseMIR(uint32 fileIdx = 0, uint32 option = 0, bool isIpa = false, bool isComb = false);
  bool ParseMIR(std::ifstream&);  // the main entry point
  bool ParseInlineFuncBody(std::ifstream&);
  bool ParseMPLT(std::ifstream&, const std::string&);
  bool ParseMPLTStandalone(std::ifstream &mpltfile, const std::string &importfilename);
  bool ParseTypeFromString(const std::string&, TyIdx&);
  void EmitError(const std::string&);
  void EmitWarning(const std::string&);
  uint32 GetOptions() const {
    return options;
  }

 private:
  // func ptr map for ParseMIR()
  using FuncPtrParseMIRForElem = bool (MIRParser::*)();
  static std::map<TokenKind, FuncPtrParseMIRForElem> funcPtrMapForParseMIR;
  static std::map<TokenKind, FuncPtrParseMIRForElem> InitFuncPtrMapForParseMIR();

  bool TypeCompatible(TyIdx, TyIdx);
  bool IsTypeIncomplete(MIRType*);

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
  bool ParseStmtBlockForOutParmSize();
  bool ParseStmtBlockForModuleID();
  bool ParseStmtBlockForFuncSize();
  bool ParseStmtBlockForFuncID();
  bool ParseStmtBlockForFormalWordsTypeTagged();
  bool ParseStmtBlockForLocalWordsTypeTagged();
  bool ParseStmtBlockForFormalWordsRefCounted();
  bool ParseStmtBlockForLocalWordsRefCounted();
  bool ParseStmtBlockForFuncInfo();

  // common func
  void SetSrcPos(SrcPosition &srcPosition, uint32 mplNum);

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
  uint32 firstLineNum = 0;                   // to track function starting line
  std::map<TyIdx, TyIdx> typeDefIdxMap;      // map previous declared tyIdx
  bool firstImport = true;                   // Mark the first imported mplt file
  bool paramParseLocalType = false;          // param for ParseTypedef
  uint32 paramFileIdx = 0;                   // param for ParseMIR()
  bool paramIsIPA = false;
  bool paramIsComb = false;
  TokenKind paramTokenKind = TK_invalid;
  std::vector<std::string> paramImportFileList;
  std::stack<bool> safeRegionFlag;
};
}  // namespace maple
#endif  // MAPLE_IR_INCLUDE_MIR_PARSER_H
