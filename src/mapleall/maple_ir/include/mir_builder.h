/*
 * Copyright (c) [2019-2020] Huawei Technologies Co.,Ltd.All rights reserved.
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
#ifndef MAPLE_IR_INCLUDE_MIR_BUILDER_H
#define MAPLE_IR_INCLUDE_MIR_BUILDER_H
#include <string>
#include <utility>
#include <vector>
#include <map>
#ifdef _WIN32
#include <pthread.h>
#endif
#include "opcodes.h"
#include "prim_types.h"
#include "mir_type.h"
#include "mir_const.h"
#include "mir_symbol.h"
#include "mir_nodes.h"
#include "mir_module.h"
#include "mir_preg.h"
#include "mir_function.h"
#include "printing.h"
#include "intrinsic_op.h"
#include "opcode_info.h"
#include "global_tables.h"

namespace maple {
using ArgPair = std::pair<std::string, MIRType*>;
using ArgVector = MapleVector<ArgPair>;
class MIRBuilder {
 public:
  enum MatchStyle {
    kUpdateFieldID = 0,  // do not match but traverse to update fieldID
    kMatchTopField = 1,  // match top level field only
    kMatchAnyField = 2,  // match any field
    kParentFirst = 4,    // traverse parent first
    kFoundInChild = 8,   // found in child
  };

  explicit MIRBuilder(MIRModule *module)
      : mirModule(module),
        incompleteTypeRefedSet(mirModule->GetMPAllocator().Adapter()) {}

  virtual ~MIRBuilder() = default;

  virtual void SetCurrentFunction(MIRFunction &fun) {
    mirModule->SetCurFunction(&fun);
  }

  virtual MIRFunction *GetCurrentFunction() const {
    return mirModule->CurFunction();
  }
  MIRFunction *GetCurrentFunctionNotNull() const {
    MIRFunction *func = GetCurrentFunction();
    CHECK_FATAL(func != nullptr, "nullptr check");
    return func;
  }

  MIRModule &GetMirModule() const {
    return *mirModule;
  }

  const MapleSet<TyIdx> &GetIncompleteTypeRefedSet() const {
    return incompleteTypeRefedSet;
  }

  std::vector<std::tuple<uint32, uint32, uint32, uint32>> &GetExtraFieldsTuples() {
    return extraFieldsTuples;
  }

  unsigned int GetLineNum() const {
    return lineNum;
  }
  void SetLineNum(unsigned int num) {
    lineNum = num;
  }

  GStrIdx GetOrCreateStringIndex(const std::string &str) const {
    return GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(str);
  }

  GStrIdx GetOrCreateStringIndex(GStrIdx strIdx, const std::string &str) const {
    std::string firstString(GlobalTables::GetStrTable().GetStringFromStrIdx(strIdx));
    firstString.append(str);
    return GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(firstString);
  }

  GStrIdx GetStringIndex(const std::string &str) const {
    return GlobalTables::GetStrTable().GetStrIdxFromName(str);
  }

  MIRFunction *GetOrCreateFunction(const std::string &str, TyIdx retTyIdx);
  MIRFunction *GetFunctionFromSymbol(const MIRSymbol &funcSymbol) const;
  MIRFunction *GetFunctionFromStidx(StIdx stIdx) const;
  MIRFunction *GetFunctionFromName(const std::string &str) const;
  // For compiler-generated metadata struct
  void AddIntFieldConst(const MIRStructType &sType, MIRAggConst &newConst, uint32 fieldID, int64 constValue) const;
  void AddAddrofFieldConst(const MIRStructType &structType, MIRAggConst &newConst, uint32 fieldID,
                           const MIRSymbol &fieldSymbol);
  void AddAddroffuncFieldConst(const MIRStructType &structType, MIRAggConst &newConst, uint32 fieldID,
                               const MIRSymbol &funcSymbol);

  bool TraverseToNamedField(MIRStructType &structType, GStrIdx nameIdx, uint32 &fieldID);
  bool TraverseToNamedFieldWithTypeAndMatchStyle(MIRStructType &structType, GStrIdx nameIdx, TyIdx typeIdx,
                                                 uint32 &fieldID, unsigned int matchStyle);
  void TraverseToNamedFieldWithType(MIRStructType &structType, GStrIdx nameIdx, TyIdx typeIdx, uint32 &fieldID,
                                    uint32 &idx);

  FieldID GetStructFieldIDFromNameAndType(MIRType &type, const std::string &name, TyIdx idx, unsigned int matchStyle);
  FieldID GetStructFieldIDFromNameAndType(MIRType &type, const std::string &name, TyIdx idx);
  FieldID GetStructFieldIDFromNameAndTypeParentFirst(MIRType &type, const std::string &name, TyIdx idx);
  FieldID GetStructFieldIDFromNameAndTypeParentFirstFoundInChild(MIRType &type, const std::string &name, TyIdx idx);

  FieldID GetStructFieldIDFromFieldName(MIRType &type, const std::string &name);
  FieldID GetStructFieldIDFromFieldNameParentFirst(MIRType *type, const std::string &name);

  void SetStructFieldIDFromFieldName(MIRStructType &structType, const std::string &name, GStrIdx newStrIdx,
                                     const MIRType &newFieldType) const;
  // for creating Function.
  MIRSymbol *GetFunctionArgument(MIRFunction &fun, uint32 index) const {
    CHECK(index < fun.GetFormalCount(), "index out of range in GetFunctionArgument");
    return fun.GetFormal(index);
  }

  MIRFunction *CreateFunction(const std::string &name, const MIRType &returnType, const ArgVector &arguments,
                              bool isVarg = false, bool createBody = true) const;
  MIRFunction *CreateFunction(StIdx stIdx, bool addToTable = true) const;
  virtual void UpdateFunction(MIRFunction&, const MIRType*, const ArgVector&) {}

  MIRSymbol *GetSymbolFromEnclosingScope(StIdx stIdx) const;
  virtual MIRSymbol *GetOrCreateLocalDecl(const std::string &str, const MIRType &type);
  MIRSymbol *GetLocalDecl(const std::string &str) const;
  MIRSymbol *CreateLocalDecl(const std::string &str, const MIRType &type) const;
  MIRSymbol *GetOrCreateGlobalDecl(const std::string &str, const MIRType &type) const;
  MIRSymbol *GetGlobalDecl(const std::string &str) const;
  MIRSymbol *GetDecl(const std::string &str) const;
  MIRSymbol *CreateGlobalDecl(const std::string &str,
                              const MIRType &type,
                              MIRStorageClass sc = kScGlobal) const;
  MIRSymbol *GetOrCreateDeclInFunc(const std::string &str, const MIRType &type, MIRFunction &func) const;
  // for creating Expression
  ConstvalNode *CreateConstval(MIRConst *mirConst);
  ConstvalNode *CreateIntConst(uint64 val, PrimType pty);
  ConstvalNode *CreateInt128Const(const uint64 *value, PrimType pty);
  ConstvalNode *CreateFloatConst(float val);
  ConstvalNode *CreateDoubleConst(double val);
  ConstvalNode *CreateFloat128Const(const uint64 *val);
  ConstvalNode *GetConstInt(MemPool &memPool, int val) const;
  ConstvalNode *GetConstInt(int val) {
    return CreateIntConst(static_cast<uint64_t>(static_cast<int64_t>(val)), PTY_i32);
  }

  ConstvalNode *GetConstUInt1(bool val) {
    return CreateIntConst(val, PTY_u1);
  }

  ConstvalNode *GetConstUInt8(uint8 val) {
    return CreateIntConst(val, PTY_u8);
  }

  ConstvalNode *GetConstUInt16(uint16 val) {
    return CreateIntConst(val, PTY_u16);
  }

  ConstvalNode *GetConstUInt32(uint32 val) {
    return CreateIntConst(val, PTY_u32);
  }

  ConstvalNode *GetConstUInt64(uint64 val) {
    return CreateIntConst(val, PTY_u64);
  }

  ConstvalNode *CreateAddrofConst(BaseNode &node);
  ConstvalNode *CreateAddroffuncConst(const BaseNode &node);
  ConstvalNode *CreateStrConst(const BaseNode &node);
  ConstvalNode *CreateStr16Const(const BaseNode &node);
  SizeoftypeNode *CreateExprSizeoftype(const MIRType &type);
  FieldsDistNode *CreateExprFieldsDist(const MIRType &type, FieldID field1, FieldID field2);
  AddrofNode *CreateExprAddrof(FieldID fieldID, const MIRSymbol &symbol, MemPool *memPool = nullptr);
  AddrofNode *CreateExprAddrof(FieldID fieldID, StIdx symbolStIdx, MemPool *memPool = nullptr);
  AddroffuncNode *CreateExprAddroffunc(PUIdx puIdx, MemPool *memPool = nullptr);
  AddrofNode *CreateExprDread(const MIRType &type, FieldID fieldID, const MIRSymbol &symbol);
  AddrofNode *CreateExprDread(PrimType ptyp, FieldID fieldID, const MIRSymbol &symbol);
  virtual AddrofNode *CreateExprDread(MIRType &type, MIRSymbol &symbol);
  virtual AddrofNode *CreateExprDread(MIRSymbol &symbol);
  AddrofNode *CreateExprDread(PregIdx pregID, PrimType pty);
  AddrofNode *CreateExprDread(MIRSymbol &symbol, uint16 fieldID);
  DreadoffNode *CreateExprDreadoff(Opcode op, PrimType pty, const MIRSymbol &symbol, int32 offset);
  RegreadNode *CreateExprRegread(PrimType pty, PregIdx regIdx);
  IreadNode *CreateExprIread(PrimType primType, TyIdx ptrTypeIdx, FieldID fieldID, BaseNode *addr);
  IreadNode *CreateExprIread(const MIRType &returnType, const MIRType &ptrType, FieldID fieldID, BaseNode *addr);
  IreadoffNode *CreateExprIreadoff(PrimType pty, int32 offset, BaseNode *opnd0);
  IreadFPoffNode *CreateExprIreadFPoff(PrimType pty, int32 offset);
  IaddrofNode *CreateExprIaddrof(const MIRType &returnType, const MIRType &ptrType, FieldID fieldID, BaseNode *addr);
  IaddrofNode *CreateExprIaddrof(PrimType returnTypePty, TyIdx ptrTypeIdx, FieldID fieldID, BaseNode *addr);
  BinaryNode *CreateExprBinary(Opcode opcode, const MIRType &type, BaseNode *opnd0, BaseNode *opnd1);
  BinaryNode *CreateExprBinary(Opcode opcode, PrimType pty, BaseNode *opnd0, BaseNode *opnd1) {
    MIRType *ty = GlobalTables::GetTypeTable().GetTypeFromTyIdx(static_cast<TyIdx>(pty));
    return CreateExprBinary(opcode, *ty, opnd0, opnd1);
  }
  TernaryNode *CreateExprTernary(Opcode opcode, const MIRType &type, BaseNode *opnd0, BaseNode *opnd1, BaseNode *opnd2);
  CompareNode *CreateExprCompare(Opcode opcode, const MIRType &type, const MIRType &opndType, BaseNode *opnd0,
                                 BaseNode *opnd1);
  UnaryNode *CreateExprUnary(Opcode opcode, const MIRType &type, BaseNode *opnd);
  GCMallocNode *CreateExprGCMalloc(Opcode opcode, const MIRType &pType, const MIRType &type);
  JarrayMallocNode *CreateExprJarrayMalloc(Opcode opcode, const MIRType &pType, const MIRType &type, BaseNode *opnd);
  TypeCvtNode *CreateExprTypeCvt(Opcode o, PrimType toPrimType, PrimType fromPrimType, BaseNode &opnd);
  TypeCvtNode *CreateExprTypeCvt(Opcode o, const MIRType &type, const MIRType &fromType, BaseNode *opnd);
  ExtractbitsNode *CreateExprExtractbits(Opcode o, const MIRType &type, uint32 bOffset, uint32 bSize, BaseNode *opnd);
  ExtractbitsNode *CreateExprExtractbits(Opcode o, PrimType type, uint32 bOffset, uint32 bSize, BaseNode *opnd);
  DepositbitsNode *CreateExprDepositbits(Opcode o, PrimType type, uint32 bOffset, uint32 bSize,
                                         BaseNode *leftOpnd, BaseNode* rightOpnd);
  RetypeNode *CreateExprRetype(const MIRType &type, const MIRType &fromType, BaseNode *opnd);
  RetypeNode *CreateExprRetype(const MIRType &type, PrimType fromType, BaseNode *opnd);
  ArrayNode *CreateExprArray(const MIRType &arrayType);
  ArrayNode *CreateExprArray(const MIRType &arrayType, BaseNode *op);
  ArrayNode *CreateExprArray(const MIRType &arrayType, BaseNode *op1, BaseNode *op2);
  ArrayNode *CreateExprArray(const MIRType &arrayType, std::vector<BaseNode *> ops);
  IntrinsicopNode *CreateExprIntrinsicop(MIRIntrinsicID id, Opcode op, PrimType primType, const TyIdx &tyIdx,
                                         const MapleVector<BaseNode*> &ops);
  IntrinsicopNode *CreateExprIntrinsicop(MIRIntrinsicID idx, Opcode opCode, const MIRType &type,
                                         const MapleVector<BaseNode*> &ops);
  // for creating Statement.
  NaryStmtNode *CreateStmtReturn(BaseNode *rVal);
  NaryStmtNode *CreateStmtNary(Opcode op, BaseNode *rVal);
  NaryStmtNode *CreateStmtNary(Opcode op, const MapleVector<BaseNode*> &rVals);
  AssertNonnullStmtNode *CreateStmtAssertNonnull(Opcode op, BaseNode *rVal, GStrIdx funcNameIdx);
  CallAssertNonnullStmtNode *CreateStmtCallAssertNonnull(Opcode op, BaseNode *rVal, GStrIdx callFuncNameIdx,
                                                         size_t paramIndex, GStrIdx stmtFuncNameIdx);
  CallAssertBoundaryStmtNode *CreateStmtCallAssertBoundary(Opcode op, const MapleVector<BaseNode*> &rVals,
                                                           GStrIdx funcNameIdx, size_t paramIndex,
                                                           GStrIdx stmtFuncNameIdx);
  AssertBoundaryStmtNode *CreateStmtAssertBoundary(Opcode op, const MapleVector<BaseNode*> &rVals, GStrIdx funcNameIdx);
  UnaryStmtNode *CreateStmtUnary(Opcode op, BaseNode *rVal);
  UnaryStmtNode *CreateStmtThrow(BaseNode *rVal);
  DassignNode *CreateStmtDassign(const MIRSymbol &symbol, FieldID fieldID, BaseNode *src);
  DassignNode *CreateStmtDassign(StIdx sIdx, FieldID fieldID, BaseNode *src);
  RegassignNode *CreateStmtRegassign(PrimType pty, PregIdx regIdx, BaseNode *src);
  IassignNode *CreateStmtIassign(const MIRType &type, FieldID fieldID, BaseNode *addr, BaseNode *src);
  IassignoffNode *CreateStmtIassignoff(PrimType pty, int32 offset, BaseNode *addr, BaseNode *src);
  IassignFPoffNode *CreateStmtIassignFPoff(Opcode op, PrimType pty, int32 offset, BaseNode *src);
  CallNode *CreateStmtCall(PUIdx puIdx, const MapleVector<BaseNode*> &args, Opcode opCode = OP_call);
  CallNode *CreateStmtCall(const std::string &callee, const MapleVector<BaseNode*> &args);
  CallNode *CreateStmtVirtualCall(PUIdx puIdx, const MapleVector<BaseNode*> &args) {
    return CreateStmtCall(puIdx, args, OP_virtualcall);
  }

  CallNode *CreateStmtSuperclassCall(PUIdx puIdx, const MapleVector<BaseNode*> &args) {
    return CreateStmtCall(puIdx, args, OP_superclasscall);
  }

  CallNode *CreateStmtInterfaceCall(PUIdx puIdx, const MapleVector<BaseNode*> &args) {
    return CreateStmtCall(puIdx, args, OP_interfacecall);
  }

  IcallNode *CreateStmtIcall(const MapleVector<BaseNode*> &args);
  IcallNode *CreateStmtIcallAssigned(const MapleVector<BaseNode*> &args, const MIRSymbol &ret);
  IcallNode *CreateStmtIcallproto(const MapleVector<BaseNode*> &args, const TyIdx &prototypeIdx);
  IcallNode *CreateStmtIcallprotoAssigned(const MapleVector<BaseNode*> &args, const MIRSymbol &ret,
                                          const TyIdx &prototypeIdx);
  // For Call, VirtualCall, SuperclassCall, InterfaceCall
  IntrinsiccallNode *CreateStmtIntrinsicCall(MIRIntrinsicID idx, const MapleVector<BaseNode*> &arguments,
                                             TyIdx tyIdx = TyIdx());
  IntrinsiccallNode *CreateStmtXintrinsicCall(MIRIntrinsicID idx, const MapleVector<BaseNode*> &arguments);
  CallNode *CreateStmtCallAssigned(PUIdx puIdx, const MIRSymbol *ret, Opcode op = OP_callassigned);
  CallNode *CreateStmtCallAssigned(PUIdx puIdx, const MapleVector<BaseNode*> &args, const MIRSymbol *ret,
                                   Opcode opcode = OP_callassigned, TyIdx tyIdx = TyIdx());
  CallNode *CreateStmtCallRegassigned(PUIdx, PregIdx, Opcode);
  CallNode *CreateStmtCallRegassigned(PUIdx puIdx, PregIdx pRegIdx, Opcode opcode, BaseNode *arg);
  CallNode *CreateStmtCallRegassigned(PUIdx puIdx, const MapleVector<BaseNode*> &args, PregIdx pRegIdx, Opcode opcode);
  IntrinsiccallNode *CreateStmtIntrinsicCallAssigned(MIRIntrinsicID idx, const MapleVector<BaseNode*> &args,
                                                     PregIdx retPregIdx);
  IntrinsiccallNode *CreateStmtIntrinsicCallAssigned(MIRIntrinsicID idx, const MapleVector<BaseNode*> &args,
                                                     const MIRSymbol *ret, TyIdx tyIdx = TyIdx());
  IntrinsiccallNode *CreateStmtXintrinsicCallAssigned(MIRIntrinsicID idx, const MapleVector<BaseNode*> &args,
                                                      const MIRSymbol *ret);
  IfStmtNode *CreateStmtIf(BaseNode *cond);
  IfStmtNode *CreateStmtIfThenElse(BaseNode *cond);
  DoloopNode *CreateStmtDoloop(StIdx doVarStIdx, bool isPReg, BaseNode *startExp, BaseNode *contExp,
                               BaseNode *incrExp);
  SwitchNode *CreateStmtSwitch(BaseNode *opnd, LabelIdx defaultLabel, const CaseVector &switchTable);
  GotoNode *CreateStmtGoto(Opcode o, LabelIdx labIdx);
  JsTryNode *CreateStmtJsTry(LabelIdx cLabIdx, LabelIdx fLabIdx);
  TryNode *CreateStmtTry(const MapleVector<LabelIdx> &cLabIdxs);
  CatchNode *CreateStmtCatch(const MapleVector<TyIdx> &tyIdxVec);
  LabelIdx GetOrCreateMIRLabel(const std::string &name) const;
  LabelIdx CreateLabIdx(MIRFunction &mirFunc) const;
  LabelNode *CreateStmtLabel(LabelIdx labIdx);
  StmtNode *CreateStmtComment(const std::string &cmnt);
  CondGotoNode *CreateStmtCondGoto(BaseNode *cond, Opcode op, LabelIdx labIdx);
  void AddStmtInCurrentFunctionBody(StmtNode &stmt) const;
  MIRSymbol *GetSymbol(TyIdx tyIdx, const std::string &name, MIRSymKind mClass, MIRStorageClass sClass,
                       uint8 scpID, bool sameType) const;
  MIRSymbol *GetSymbol(TyIdx tyIdx, GStrIdx strIdx, MIRSymKind mClass, MIRStorageClass sClass,
                       uint8 scpID, bool sameType) const;
  MIRSymbol *GetOrCreateSymbol(TyIdx tyIdx, const std::string &name, MIRSymKind mClass, MIRStorageClass sClass,
                               MIRFunction *func, uint8 scpID, bool sametype) const;
  MIRSymbol *GetOrCreateSymbol(TyIdx tyIdx, GStrIdx strIdx, MIRSymKind mClass, MIRStorageClass sClass,
                               MIRFunction *func, uint8 scpID, bool sameType) const;
  MIRSymbol *CreatePregFormalSymbol(TyIdx tyIdx, PregIdx pRegIdx, MIRFunction &func) const;
  // for creating symbol
  MIRSymbol *CreateSymbol(TyIdx tyIdx, const std::string &name, MIRSymKind mClass, MIRStorageClass sClass,
                          MIRFunction *func, uint8 scpID) const;
  MIRSymbol *CreateSymbol(TyIdx tyIdx, GStrIdx strIdx, MIRSymKind mClass, MIRStorageClass sClass,
                          MIRFunction *func, uint8 scpID) const;
  MIRSymbol *CreateConstStringSymbol(const std::string &symbolName, const std::string &content);
  MIRSymbol *GetOrCreateLocalDecl(const std::string &str, TyIdx tyIdx, MIRSymbolTable &symbolTable,
                                  bool &created) const;
  // for creating nodes
  AddrofNode *CreateAddrof(const MIRSymbol &st, PrimType pty = PTY_ptr);
  AddrofNode *CreateDread(const MIRSymbol &st, PrimType pty);
  virtual MemPool *GetCurrentFuncCodeMp();
  virtual MapleAllocator *GetCurrentFuncCodeMpAllocator();
  virtual MemPool *GetCurrentFuncDataMp();

  virtual void GlobalLock() {}
  virtual void GlobalUnlock() {}

 private:
  MIRSymbol *GetOrCreateGlobalDecl(const std::string &str, TyIdx tyIdx, bool &created) const;

  MIRModule *mirModule;
  MapleSet<TyIdx> incompleteTypeRefedSet;
  // <className strIdx, fieldname strIdx, typename strIdx, attr list strIdx>
  std::vector<std::tuple<uint32, uint32, uint32, uint32>> extraFieldsTuples;
  unsigned int lineNum = 0;
};

class MIRBuilderExt : public MIRBuilder {
 public:
  explicit MIRBuilderExt(MIRModule *module, pthread_mutex_t *mutex = nullptr);
  virtual ~MIRBuilderExt() = default;

  void SetCurrentFunction(MIRFunction &func) override {
    curFunction = &func;
  }

  MIRFunction *GetCurrentFunction() const override {
    return curFunction;
  }

  MemPool *GetCurrentFuncCodeMp() override;
  MapleAllocator *GetCurrentFuncCodeMpAllocator() override;
  void GlobalLock() override;
  void GlobalUnlock() override;

 private:
  MIRFunction *curFunction = nullptr;
  pthread_mutex_t *mutex = nullptr;
};
}  // namespace maple
#endif  // MAPLE_IR_INCLUDE_MIR_BUILDER_H
