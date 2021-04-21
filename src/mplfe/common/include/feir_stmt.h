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
#ifndef MPLFE_INCLUDE_COMMON_FEIR_STMT_H
#define MPLFE_INCLUDE_COMMON_FEIR_STMT_H
#include <vector>
#include <list>
#include <set>
#include <map>
#include <unordered_set>
#include <unordered_map>
#include <memory>
#include <tuple>
#include "types_def.h"
#include "mempool_allocator.h"
#include "mir_builder.h"
#include "factory.h"
#include "safe_ptr.h"
#include "fe_utils.h"
#include "general_stmt.h"
#include "feir_var.h"
#include "fe_struct_elem_info.h"
#include "feir_var_type_scatter.h"
#include "fe_options.h"

namespace maple {
class FEIRBuilder;

enum FEIRNodeKind : uint8 {
#define FEIR_NODE_KIND(kind, description) \
  k##kind,
#include "feir_node_kind.def"
#undef FEIR_NODE_KIND
};

std::string GetFEIRNodeKindDescription(FEIRNodeKind kindArg);

// ---------- FEIRNode ----------
class FEIRNode {
 public:
  explicit FEIRNode(FEIRNodeKind argKind)
      : kind(argKind) {}
  virtual ~FEIRNode() = default;

 protected:
  FEIRNodeKind kind;
};  // class FEIRNode

// ---------- FEIRDFGNode ----------
class FEIRDFGNode {
 public:
  explicit FEIRDFGNode(const UniqueFEIRVar &argVar)
      : var(argVar) {
    CHECK_NULL_FATAL(argVar);
  }

  virtual ~FEIRDFGNode() = default;
  bool operator==(const FEIRDFGNode &node) const {
    return var->EqualsTo(node.var);
  }

  size_t Hash() const {
    return var->Hash();
  }

  std::string GetNameRaw() const {
    return var->GetNameRaw();
  }

 private:
  const UniqueFEIRVar &var;
};

class FEIRDFGNodeHash {
 public:
  std::size_t operator()(const FEIRDFGNode &node) const {
    return node.Hash();
  }
};

using UniqueFEIRDFGNode = std::unique_ptr<FEIRDFGNode>;

class FEIRStmtCheckPoint;
// ---------- FEIRStmt ----------
class FEIRStmt : public GeneralStmt {
 public:
  explicit FEIRStmt(FEIRNodeKind argKind)
      : kind(argKind) {}

  FEIRStmt(GeneralStmtKind argGenKind, FEIRNodeKind argKind)
      : GeneralStmt(argGenKind),
        kind(argKind) {}

  virtual ~FEIRStmt() = default;
  void RegisterDFGNodes2CheckPoint(FEIRStmtCheckPoint &checkPoint) {
    RegisterDFGNodes2CheckPointImpl(checkPoint);
  }

  bool CalculateDefs4AllUses(FEIRStmtCheckPoint &checkPoint, FEIRUseDefChain &udChain) {
    return CalculateDefs4AllUsesImpl(checkPoint, udChain);
  }

  void InitTrans4AllVars() {
    InitTrans4AllVarsImpl();
  }

  FEIRVarTypeScatter* GetTypeScatterDefVar() const {
    return GetTypeScatterDefVarImpl();
  }

  std::list<StmtNode*> GenMIRStmts(MIRBuilder &mirBuilder) const {
    std::list<StmtNode*> stmts = GenMIRStmtsImpl(mirBuilder);
    SetSrcPos(stmts);
    return stmts;
  }

  FEIRNodeKind GetKind() const {
    return kind;
  }

  void SetKind(FEIRNodeKind argKind) {
    kind = argKind;
  }

  bool IsFallThrough() const {
    return IsFallThroughImpl();
  }

  bool IsTarget() const {
    return IsTargetImpl();
  }

  bool HasDef() const {
    return HasDefImpl();
  }

  bool SetHexPC(uint32 argHexPC) {
    return SetHexPCImpl(argHexPC);
  }

  uint32 GetHexPC(void) const {
    return GetHexPCImpl();
  }

  bool IsStmtInstComment() const;
  bool ShouldHaveLOC() const;
  void SetSrcFileInfo(uint32 srcFileIdxIn, uint32 srcFileLineNumIn) {
    srcFileIndex = srcFileIdxIn;
    srcFileLineNum = srcFileLineNumIn;
  }

 protected:
  std::string DumpDotStringImpl() const override;
  virtual void RegisterDFGNodes2CheckPointImpl(FEIRStmtCheckPoint &checkPoint) {}
  virtual bool CalculateDefs4AllUsesImpl(FEIRStmtCheckPoint &checkPoint, FEIRUseDefChain &udChain) {
    return true;
  }

  virtual FEIRVarTypeScatter* GetTypeScatterDefVarImpl() const {
    return nullptr;
  }

  virtual void InitTrans4AllVarsImpl() {}
  virtual std::list<StmtNode*> GenMIRStmtsImpl(MIRBuilder &mirBuilder) const;
  virtual bool IsStmtInstImpl() const override;
  virtual bool IsFallThroughImpl() const {
    return true;
  }

  bool IsBranchImpl() const override {
    return false;
  }

  virtual bool IsTargetImpl() const {
    return false;
  }

  virtual bool HasDefImpl() const {
    return false;
  }

  bool SetHexPCImpl(uint32 argHexPC) {
    hexPC = argHexPC;
    return true;
  }

  uint32 GetHexPCImpl(void) const {
    return hexPC;
  }

  void SetSrcPos(std::list<StmtNode*> &stmts) const {
#ifdef DEBUG
    if (FEOptions::GetInstance().IsDumpLOC() && !stmts.empty()) {
      (*stmts.begin())->GetSrcPos().SetFileNum(srcFileIndex);
      (*stmts.begin())->GetSrcPos().SetLineNum(srcFileLineNum);
    }
#endif
  }

  FEIRNodeKind kind;
  uint32 srcFileIndex = 0;
  uint32 srcFileLineNum = 0;
  uint32 hexPC = UINT32_MAX;
};

using UniqueFEIRStmt = std::unique_ptr<FEIRStmt>;

// ---------- FEIRStmtCheckPoint ----------
class FEIRStmtCheckPoint : public FEIRStmt {
 public:
  FEIRStmtCheckPoint()
      : FEIRStmt(FEIRNodeKind::kStmtCheckPoint),
        firstVisibleStmt(nullptr) {}
  ~FEIRStmtCheckPoint() {
    firstVisibleStmt = nullptr;
  }

  void Reset();
  void RegisterDFGNode(UniqueFEIRVar &var);
  void RegisterDFGNodes(const std::list<UniqueFEIRVar*> &vars);
  void RegisterDFGNodeFromAllVisibleStmts();
  void AddPredCheckPoint(FEIRStmtCheckPoint &stmtCheckPoint);
  std::set<UniqueFEIRVar*> &CalcuDef(UniqueFEIRVar &use);
  void SetFirstVisibleStmt(FEIRStmt &stmt) {
    CHECK_FATAL((stmt.GetKind() != FEIRNodeKind::kStmtCheckPoint), "check point should not be DFG Node.");
    firstVisibleStmt = &stmt;
  }

 protected:
  std::string DumpDotStringImpl() const override;

 private:
  void CalcuDefDFS(std::set<UniqueFEIRVar*> &result, const UniqueFEIRVar &use, const FEIRStmtCheckPoint &cp,
                   std::set<const FEIRStmtCheckPoint*> &visitSet) const;
  std::set<FEIRStmtCheckPoint*> predCPs;
  std::list<UniqueFEIRVar*> defs;
  std::list<UniqueFEIRVar*> uses;
  FEIRUseDefChain localUD;
  std::unordered_map<FEIRDFGNode, UniqueFEIRVar*, FEIRDFGNodeHash> lastDef;
  std::unordered_map<FEIRDFGNode, std::set<UniqueFEIRVar*>, FEIRDFGNodeHash> cacheUD;
  FEIRStmt *firstVisibleStmt;
};

// ---------- FEIRExpr ----------
class FEIRExpr {
 public:
  explicit FEIRExpr(FEIRNodeKind argKind);
  FEIRExpr(FEIRNodeKind argKind, std::unique_ptr<FEIRType> argType);
  virtual ~FEIRExpr() = default;
  FEIRExpr(const FEIRExpr&) = delete;
  FEIRExpr& operator=(const FEIRExpr&) = delete;
  std::string DumpDotString() const;
  std::unique_ptr<FEIRExpr> Clone() {
    return CloneImpl();
  }

  BaseNode *GenMIRNode(MIRBuilder &mirBuilder) const {
    return GenMIRNodeImpl(mirBuilder);
  }

  std::vector<FEIRVar*> GetVarUses() const {
    return GetVarUsesImpl();
  }

  bool IsNestable() const {
    return IsNestableImpl();
  }

  bool IsAddrof() const {
    return IsAddrofImpl();
  }

  bool HasException() const {
    return HasExceptionImpl();
  }

  void SetType(std::unique_ptr<FEIRType> argType) {
    CHECK_NULL_FATAL(argType);
    type = std::move(argType);
  }

  FEIRNodeKind GetKind() const {
    return kind;
  }

  FEIRType *GetType() const {
    ASSERT(type != nullptr, "type is nullptr");
    return type.get();
  }

  const FEIRType &GetTypeRef() const {
    ASSERT(type != nullptr, "type is nullptr");
    return *type.get();
  }

  PrimType GetPrimType() const {
    return type->GetPrimType();
  }

  void RegisterDFGNodes2CheckPoint(FEIRStmtCheckPoint &checkPoint) {
    RegisterDFGNodes2CheckPointImpl(checkPoint);
  }

  bool CalculateDefs4AllUses(FEIRStmtCheckPoint &checkPoint, FEIRUseDefChain &udChain) {
    return CalculateDefs4AllUsesImpl(checkPoint, udChain);
  }

 protected:
  virtual std::unique_ptr<FEIRExpr> CloneImpl() const = 0;
  virtual BaseNode *GenMIRNodeImpl(MIRBuilder &mirBuilder) const = 0;
  virtual std::vector<FEIRVar*> GetVarUsesImpl() const;
  virtual void RegisterDFGNodes2CheckPointImpl(FEIRStmtCheckPoint &checkPoint) {}
  virtual bool CalculateDefs4AllUsesImpl(FEIRStmtCheckPoint &checkPoint, FEIRUseDefChain &udChain) {
    return true;
  }

  virtual bool IsNestableImpl() const;
  virtual bool IsAddrofImpl() const;
  virtual bool HasExceptionImpl() const;

  FEIRNodeKind kind;
  bool isNestable : 1;
  bool isAddrof : 1;
  bool hasException : 1;
  std::unique_ptr<FEIRType> type;
};  // class FEIRExpr

using UniqueFEIRExpr = std::unique_ptr<FEIRExpr>;

// ---------- FEIRExprConst ----------
union ConstExprValue {
  bool b;
  uint8 u8;
  int8 i8;
  uint16 u16;
  int16 i16;
  uint32 u32;
  int32 i32;
  float f32;
  uint64 u64 = 0;
  int64 i64;
  double f64;
};

class FEIRExprConst : public FEIRExpr {
 public:
  FEIRExprConst();
  FEIRExprConst(int64 val, PrimType argType);
  FEIRExprConst(uint64 val, PrimType argType);
  explicit FEIRExprConst(float val);
  explicit FEIRExprConst(double val);
  ~FEIRExprConst() = default;
  FEIRExprConst(const FEIRExprConst&) = delete;
  FEIRExprConst& operator=(const FEIRExprConst&) = delete;

  ConstExprValue GetValue() const {
    return value;
  }

 protected:
  std::unique_ptr<FEIRExpr> CloneImpl() const override;
  BaseNode *GenMIRNodeImpl(MIRBuilder &mirBuilder) const override;

 private:
  void CheckRawValue2SetZero();
  ConstExprValue value;
};

// ---------- FEIRExprDRead ----------
class FEIRExprDRead : public FEIRExpr {
 public:
  explicit FEIRExprDRead(std::unique_ptr<FEIRVar> argVarSrc);
  FEIRExprDRead(std::unique_ptr<FEIRType> argType, std::unique_ptr<FEIRVar> argVarSrc);
  ~FEIRExprDRead() = default;
  void SetVarSrc(std::unique_ptr<FEIRVar> argVarSrc);
  void SetTrans(UniqueFEIRVarTrans argTrans) {
    varSrc->SetTrans(std::move(argTrans));
  }

  UniqueFEIRVarTrans CreateTransDirect() {
    UniqueFEIRVarTrans trans = std::make_unique<FEIRVarTrans>(FEIRVarTransKind::kFEIRVarTransDirect, varSrc);
    return trans;
  }

  UniqueFEIRVarTrans CreateTransArrayDimDecr() {
    UniqueFEIRVarTrans trans = std::make_unique<FEIRVarTrans>(FEIRVarTransKind::kFEIRVarTransArrayDimDecr, varSrc);
    return trans;
  }

  UniqueFEIRVarTrans CreateTransArrayDimIncr() {
    UniqueFEIRVarTrans trans = std::make_unique<FEIRVarTrans>(FEIRVarTransKind::kFEIRVarTransArrayDimIncr, varSrc);
    return trans;
  }

  FieldID GetFieldID() {
    return fieldID;
  }

  UniqueFEIRVar &GetVar() {
    return varSrc;
  }

  void SetFieldName(std::string argFieldName){
    fieldName = argFieldName;
  }

  std::string GetFieldName(){
    return fieldName;
  }

  void SetFieldID(FieldID argFieldID){
    fieldID = argFieldID;
  }

 protected:
  std::unique_ptr<FEIRExpr> CloneImpl() const override;
  void RegisterDFGNodes2CheckPointImpl(FEIRStmtCheckPoint &checkPoint) override;
  bool CalculateDefs4AllUsesImpl(FEIRStmtCheckPoint &checkPoint, FEIRUseDefChain &udChain) override;
  BaseNode *GenMIRNodeImpl(MIRBuilder &mirBuilder) const override;
  std::vector<FEIRVar*> GetVarUsesImpl() const override;

 private:
  std::unique_ptr<FEIRVar> varSrc;
  FieldID fieldID = 0;
  std::string fieldName;
};

// ---------- FEIRExprRegRead ----------
class FEIRExprRegRead : public FEIRExpr {
 public:
  FEIRExprRegRead(PrimType pty, int32 regNumIn);
  ~FEIRExprRegRead() = default;

 protected:
  std::unique_ptr<FEIRExpr> CloneImpl() const override;
  BaseNode *GenMIRNodeImpl(MIRBuilder &mirBuilder) const override;

  PrimType prmType;
  int32 regNum;
};

// ---------- FEIRExprAddrof ----------
class FEIRExprAddrof : public FEIRExpr {
 public:
  explicit FEIRExprAddrof(const std::vector<uint32> &arrayIn)
      : FEIRExpr(FEIRNodeKind::kExprAddrof), array(arrayIn) {}
  ~FEIRExprAddrof() = default;

 protected:
  std::unique_ptr<FEIRExpr> CloneImpl() const override;
  BaseNode *GenMIRNodeImpl(MIRBuilder &mirBuilder) const override;

 private:
  std::vector<uint32> array;
};

// ---------- FEIRExprAddrofVar ----------
class FEIRExprAddrofVar : public FEIRExpr {
 public:
  explicit FEIRExprAddrofVar(std::unique_ptr<FEIRVar> argVarSrc)
      : FEIRExpr(FEIRNodeKind::kExprAddrofVar), varSrc(std::move(argVarSrc)) {}
  ~FEIRExprAddrofVar() = default;

 protected:
  std::unique_ptr<FEIRExpr> CloneImpl() const override;
  BaseNode *GenMIRNodeImpl(MIRBuilder &mirBuilder) const override;

 private:
  std::unique_ptr<FEIRVar> varSrc;
};

// ---------- FEIRExprUnary ----------
class FEIRExprUnary : public FEIRExpr {
 public:
  FEIRExprUnary(Opcode argOp, std::unique_ptr<FEIRExpr> argOpnd);
  FEIRExprUnary(std::unique_ptr<FEIRType> argType, Opcode argOp, std::unique_ptr<FEIRExpr> argOpnd);
  ~FEIRExprUnary() = default;
  void SetOpnd(std::unique_ptr<FEIRExpr> argOpnd);
  static std::map<Opcode, bool> InitMapOpNestableForExprUnary();

 protected:
  virtual std::unique_ptr<FEIRExpr> CloneImpl() const override;
  void RegisterDFGNodes2CheckPointImpl(FEIRStmtCheckPoint &checkPoint) override;
  bool CalculateDefs4AllUsesImpl(FEIRStmtCheckPoint &checkPoint, FEIRUseDefChain &udChain) override;
  virtual BaseNode *GenMIRNodeImpl(MIRBuilder &mirBuilder) const override;
  std::vector<FEIRVar*> GetVarUsesImpl() const override;

  Opcode op;
  std::unique_ptr<FEIRExpr> opnd;

 private:
  void SetExprTypeByOp();

  static std::map<Opcode, bool> mapOpNestable;
};  // class FEIRExprUnary

// ---------- FEIRExprTypeCvt ----------
class FEIRExprTypeCvt : public FEIRExprUnary {
 public:
  FEIRExprTypeCvt(Opcode argOp, std::unique_ptr<FEIRExpr> argOpnd);
  FEIRExprTypeCvt(std::unique_ptr<FEIRType> exprType, Opcode argOp, std::unique_ptr<FEIRExpr> argOpnd);
  ~FEIRExprTypeCvt() = default;
  static std::map<Opcode, bool> InitMapOpNestableForTypeCvt();
  static Opcode ChooseOpcodeByFromVarAndToVar(const FEIRVar &fromVar, const FEIRVar &toVar);

 protected:
  std::unique_ptr<FEIRExpr> CloneImpl() const override;
  void RegisterDFGNodes2CheckPointImpl(FEIRStmtCheckPoint &checkPoint) override;
  bool CalculateDefs4AllUsesImpl(FEIRStmtCheckPoint &checkPoint, FEIRUseDefChain &udChain) override;
  BaseNode *GenMIRNodeImpl(MIRBuilder &mirBuilder) const override;

 private:
  using FuncPtrGenMIRNode = BaseNode* (FEIRExprTypeCvt::*)(MIRBuilder &mirBuilder) const;
  static std::map<Opcode, FuncPtrGenMIRNode> InitFuncPtrMapForParseExpr();

  // GenMIRNodeMode1:
  //   MIR: op <to-type> <from-type> (<opnd0>)
  BaseNode *GenMIRNodeMode1(MIRBuilder &mirBuilder) const;

  // GenMIRNodeMode2:
  //   MIR: op <prim-type> <float-type> (<opnd0>)
  BaseNode *GenMIRNodeMode2(MIRBuilder &mirBuilder) const;

  // GenMIRNodeMode3:
  //   MIR: retype <prim-type> <type> (<opnd0>)
  BaseNode *GenMIRNodeMode3(MIRBuilder &mirBuilder) const;

  static std::map<Opcode, bool> mapOpNestable;
  static std::map<Opcode, FuncPtrGenMIRNode> funcPtrMapForParseExpr;
};  // FEIRExprTypeCvt

// ---------- FEIRExprExtractBits ----------
class FEIRExprExtractBits : public FEIRExprUnary {
 public:
  FEIRExprExtractBits(Opcode argOp, PrimType argPrimType, uint8 argBitOffset, uint8 argBitSize,
                      std::unique_ptr<FEIRExpr> argOpnd);
  FEIRExprExtractBits(Opcode argOp, PrimType argPrimType, std::unique_ptr<FEIRExpr> argOpnd);
  ~FEIRExprExtractBits() = default;
  static std::map<Opcode, bool> InitMapOpNestableForExtractBits();
  void SetBitOffset(uint8 offset) {
    bitOffset = offset;
  }

  void SetBitSize(uint8 size) {
    bitSize = size;
  }

 protected:
  std::unique_ptr<FEIRExpr> CloneImpl() const override;
  void RegisterDFGNodes2CheckPointImpl(FEIRStmtCheckPoint &checkPoint) override;
  bool CalculateDefs4AllUsesImpl(FEIRStmtCheckPoint &checkPoint, FEIRUseDefChain &udChain) override;
  BaseNode *GenMIRNodeImpl(MIRBuilder &mirBuilder) const override;

 private:
  using FuncPtrGenMIRNode = BaseNode* (FEIRExprExtractBits::*)(MIRBuilder &mirBuilder) const;
  static std::map<Opcode, FuncPtrGenMIRNode> InitFuncPtrMapForParseExpr();
  BaseNode *GenMIRNodeForExtrabits(MIRBuilder &mirBuilder) const;
  BaseNode *GenMIRNodeForExt(MIRBuilder &mirBuilder) const;

  uint8 bitOffset;
  uint8 bitSize;
  static std::map<Opcode, bool> mapOpNestable;
  static std::map<Opcode, FuncPtrGenMIRNode> funcPtrMapForParseExpr;
};  // FEIRExprExtractBit

// ---------- FEIRExprIRead ----------
class FEIRExprIRead : public FEIRExpr {
 public:
  FEIRExprIRead(UniqueFEIRType returnType, UniqueFEIRType pointeeType, FieldID id, UniqueFEIRExpr expr)
      : FEIRExpr(FEIRNodeKind::kExprIRead), retType(std::move(returnType)), ptrType(std::move(pointeeType)),
        fieldID(id), subExpr(std::move(expr)) {}
  ~FEIRExprIRead() override = default;

  void SetFieldID(FieldID argFieldID){
    fieldID = argFieldID;
  }

  FieldID GetFieldID(){
    return fieldID;
  }

  void SetFieldName(std::string argFieldName){
    fieldName = argFieldName;
  }

  std::string GetFieldName(){
    return fieldName;
  }

  UniqueFEIRExpr &GetOpnd(){
    return opnd;
  }

  UniqueFEIRType &GetType(){
    return type;
  }

 protected:
  std::unique_ptr<FEIRExpr> CloneImpl() const override;
  BaseNode *GenMIRNodeImpl(MIRBuilder &mirBuilder) const override;

 private:
  UniqueFEIRType retType;
  UniqueFEIRType ptrType;
  FieldID fieldID;
  UniqueFEIRExpr subExpr;
  UniqueFEIRExpr opnd;
  UniqueFEIRType type;
  std::string fieldName;
};

// ---------- FEIRExprBinary ----------
class FEIRExprBinary : public FEIRExpr {
 public:
  FEIRExprBinary(Opcode argOp, std::unique_ptr<FEIRExpr> argOpnd0, std::unique_ptr<FEIRExpr> argOpnd1);
  FEIRExprBinary(std::unique_ptr<FEIRType> exprType, Opcode argOp, std::unique_ptr<FEIRExpr> argOpnd0,
                 std::unique_ptr<FEIRExpr> argOpnd1);
  ~FEIRExprBinary() = default;
  void SetOpnd0(std::unique_ptr<FEIRExpr> argOpnd);
  void SetOpnd1(std::unique_ptr<FEIRExpr> argOpnd);

 protected:
  std::unique_ptr<FEIRExpr> CloneImpl() const override;
  void RegisterDFGNodes2CheckPointImpl(FEIRStmtCheckPoint &checkPoint) override;
  bool CalculateDefs4AllUsesImpl(FEIRStmtCheckPoint &checkPoint, FEIRUseDefChain &udChain) override;
  BaseNode *GenMIRNodeImpl(MIRBuilder &mirBuilder) const override;
  std::vector<FEIRVar*> GetVarUsesImpl() const override;
  bool IsNestableImpl() const override;
  bool IsAddrofImpl() const override;

 private:
  using FuncPtrGenMIRNode = BaseNode* (FEIRExprBinary::*)(MIRBuilder &mirBuilder) const;
  static std::map<Opcode, FuncPtrGenMIRNode> InitFuncPtrMapForGenMIRNode();
  BaseNode *GenMIRNodeNormal(MIRBuilder &mirBuilder) const;
  BaseNode *GenMIRNodeCompare(MIRBuilder &mirBuilder) const;
  BaseNode *GenMIRNodeCompareU1(MIRBuilder &mirBuilder) const;
  void SetExprTypeByOp();
  void SetExprTypeByOpNormal();
  void SetExprTypeByOpShift();
  void SetExprTypeByOpLogic();
  void SetExprTypeByOpCompare();

  Opcode op;
  std::unique_ptr<FEIRExpr> opnd0;
  std::unique_ptr<FEIRExpr> opnd1;
  static std::map<Opcode, FuncPtrGenMIRNode> funcPtrMapForGenMIRNode;
};  // class FEIRExprUnary

// ---------- FEIRExprTernary ----------
class FEIRExprTernary : public FEIRExpr {
 public:
  FEIRExprTernary(Opcode argOp, std::unique_ptr<FEIRExpr> argOpnd0, std::unique_ptr<FEIRExpr> argOpnd1,
                  std::unique_ptr<FEIRExpr> argOpnd2);
  ~FEIRExprTernary() = default;
  void SetOpnd(std::unique_ptr<FEIRExpr> argOpnd, uint32 idx);

 protected:
  std::unique_ptr<FEIRExpr> CloneImpl() const override;
  void RegisterDFGNodes2CheckPointImpl(FEIRStmtCheckPoint &checkPoint) override;
  bool CalculateDefs4AllUsesImpl(FEIRStmtCheckPoint &checkPoint, FEIRUseDefChain &udChain) override;
  BaseNode *GenMIRNodeImpl(MIRBuilder &mirBuilder) const override;
  std::vector<FEIRVar*> GetVarUsesImpl() const override;
  bool IsNestableImpl() const override;
  bool IsAddrofImpl() const override;

 private:
  void SetExprTypeByOp();

  Opcode op;
  std::unique_ptr<FEIRExpr> opnd0;
  std::unique_ptr<FEIRExpr> opnd1;
  std::unique_ptr<FEIRExpr> opnd2;
};

// ---------- FEIRExprNary ----------
class FEIRExprNary : public FEIRExpr {
 public:
  explicit FEIRExprNary(Opcode argOp);
  ~FEIRExprNary() = default;
  void AddOpnd(std::unique_ptr<FEIRExpr> argOpnd);
  void AddOpnds(const std::vector<std::unique_ptr<FEIRExpr>> &argOpnds);
  void ResetOpnd();

 protected:
  void RegisterDFGNodes2CheckPointImpl(FEIRStmtCheckPoint &checkPoint) override;
  bool CalculateDefs4AllUsesImpl(FEIRStmtCheckPoint &checkPoint, FEIRUseDefChain &udChain) override;
  std::vector<FEIRVar*> GetVarUsesImpl() const override;

  Opcode op;
  std::vector<std::unique_ptr<FEIRExpr>> opnds;
};  // class FEIRExprNary

// ---------- FEIRExprArray ----------
class FEIRExprArray : public FEIRExprNary {
 public:
  FEIRExprArray(Opcode argOp, std::unique_ptr<FEIRExpr> argArray, std::unique_ptr<FEIRExpr> argIndex);
  ~FEIRExprArray() = default;
  void SetOpndArray(std::unique_ptr<FEIRExpr> opndArray);
  void SetOpndIndex(std::unique_ptr<FEIRExpr> opndIndex);

 protected:
  std::unique_ptr<FEIRExpr> CloneImpl() const override;
  void RegisterDFGNodes2CheckPointImpl(FEIRStmtCheckPoint &checkPoint) override;
  bool CalculateDefs4AllUsesImpl(FEIRStmtCheckPoint &checkPoint, FEIRUseDefChain &udChain) override;
  BaseNode *GenMIRNodeImpl(MIRBuilder &mirBuilder) const override;
  bool IsNestableImpl() const override;
  bool IsAddrofImpl() const override;
};  // class FEIRExprArray

// ---------- FEIRExprIntrinsicop ----------
class FEIRExprIntrinsicop : public FEIRExprNary {
 public:
  FEIRExprIntrinsicop(std::unique_ptr<FEIRType> exprType, MIRIntrinsicID argIntrinsicID);
  FEIRExprIntrinsicop(std::unique_ptr<FEIRType> exprType, MIRIntrinsicID argIntrinsicID,
                      std::unique_ptr<FEIRType> argParamType);
  FEIRExprIntrinsicop(std::unique_ptr<FEIRType> exprType, MIRIntrinsicID argIntrinsicID,
                      const std::vector<std::unique_ptr<FEIRExpr>> &argOpnds);
  FEIRExprIntrinsicop(std::unique_ptr<FEIRType> exprType, MIRIntrinsicID argIntrinsicID,
                      std::unique_ptr<FEIRType> argParamType, uint32 argTypeID);
  FEIRExprIntrinsicop(std::unique_ptr<FEIRType> exprType, MIRIntrinsicID argIntrinsicID,
                      std::unique_ptr<FEIRType> argParamType,
                      const std::vector<std::unique_ptr<FEIRExpr>> &argOpnds);
  ~FEIRExprIntrinsicop() = default;

 protected:
  std::unique_ptr<FEIRExpr> CloneImpl() const override;
  void RegisterDFGNodes2CheckPointImpl(FEIRStmtCheckPoint &checkPoint) override;
  bool CalculateDefs4AllUsesImpl(FEIRStmtCheckPoint &checkPoint, FEIRUseDefChain &udChain) override;
  BaseNode *GenMIRNodeImpl(MIRBuilder &mirBuilder) const override;
  bool IsNestableImpl() const override;
  bool IsAddrofImpl() const override;

 private:
  MIRIntrinsicID intrinsicID;
  std::unique_ptr<FEIRType> paramType;
  uint32 typeID = UINT32_MAX;
};  // class FEIRExprIntrinsicop

class FEIRExprJavaMerge : public FEIRExprNary {
 public:
  FEIRExprJavaMerge(std::unique_ptr<FEIRType> mergedTypeArg, const std::vector<std::unique_ptr<FEIRExpr>> &argOpnds);

 protected:
  std::unique_ptr<FEIRExpr> CloneImpl() const override;
  void RegisterDFGNodes2CheckPointImpl(FEIRStmtCheckPoint &checkPoint) override;
  bool CalculateDefs4AllUsesImpl(FEIRStmtCheckPoint &checkPoint, FEIRUseDefChain &udChain) override;
  BaseNode *GenMIRNodeImpl(MIRBuilder &mirBuilder) const override;
};

// ---------- FEIRExprJavaNewInstance ----------
class FEIRExprJavaNewInstance : public FEIRExpr {
 public:
  explicit FEIRExprJavaNewInstance(UniqueFEIRType argType);
  FEIRExprJavaNewInstance(UniqueFEIRType argType, uint32 argTypeID);
  FEIRExprJavaNewInstance(UniqueFEIRType argType, uint32 argTypeID, bool argIsRcPermanent);
  ~FEIRExprJavaNewInstance() = default;

 protected:
  std::unique_ptr<FEIRExpr> CloneImpl() const override;
  BaseNode *GenMIRNodeImpl(MIRBuilder &mirBuilder) const override;

  uint32 typeID = UINT32_MAX;
  // isRcPermanent is true means the rc annotation @Permanent is used
  bool isRcPermanent = false;
};

// ---------- FEIRExprJavaNewArray ----------
class FEIRExprJavaNewArray : public FEIRExpr {
 public:
  FEIRExprJavaNewArray(UniqueFEIRType argArrayType, UniqueFEIRExpr argExprSize);
  FEIRExprJavaNewArray(UniqueFEIRType argArrayType, UniqueFEIRExpr argExprSize, uint32 argTypeID);
  FEIRExprJavaNewArray(UniqueFEIRType argArrayType, UniqueFEIRExpr argExprSize, uint32 argTypeID,
                       bool argIsRcPermanent);
  ~FEIRExprJavaNewArray() = default;
  void SetArrayType(UniqueFEIRType argArrayType) {
    CHECK_NULL_FATAL(argArrayType);
    arrayType = std::move(argArrayType);
  }

  void SetExprSize(UniqueFEIRExpr argExprSize) {
    CHECK_NULL_FATAL(argExprSize);
    exprSize = std::move(argExprSize);
  }

 protected:
  std::unique_ptr<FEIRExpr> CloneImpl() const override;
  std::vector<FEIRVar*> GetVarUsesImpl() const override;
  void RegisterDFGNodes2CheckPointImpl(FEIRStmtCheckPoint &checkPoint) override;
  bool CalculateDefs4AllUsesImpl(FEIRStmtCheckPoint &checkPoint, FEIRUseDefChain &udChain) override;
  BaseNode *GenMIRNodeImpl(MIRBuilder &mirBuilder) const override;

 private:
  UniqueFEIRType arrayType;
  UniqueFEIRExpr exprSize;
  uint32 typeID = UINT32_MAX;
  // isRcPermanent is true means the rc annotation @Permanent is used
  bool isRcPermanent = false;
};

// ---------- FEIRExprJavaArrayLength ----------
class FEIRExprJavaArrayLength : public FEIRExpr {
 public:
  FEIRExprJavaArrayLength(UniqueFEIRExpr argExprArray);
  ~FEIRExprJavaArrayLength() = default;
  void SetExprArray(UniqueFEIRExpr argExprArray) {
    CHECK_NULL_FATAL(argExprArray);
    exprArray = std::move(argExprArray);
  }

 protected:
  std::unique_ptr<FEIRExpr> CloneImpl() const override;
  std::vector<FEIRVar*> GetVarUsesImpl() const override;
  void RegisterDFGNodes2CheckPointImpl(FEIRStmtCheckPoint &checkPoint) override;
  bool CalculateDefs4AllUsesImpl(FEIRStmtCheckPoint &checkPoint, FEIRUseDefChain &udChain) override;
  BaseNode *GenMIRNodeImpl(MIRBuilder &mirBuilder) const override;

 private:
  UniqueFEIRExpr exprArray;
};

// ---------- FEIRExprArrayStoreForC ----------
class FEIRExprArrayStoreForC : public FEIRExpr {
 public:
  FEIRExprArrayStoreForC(UniqueFEIRExpr argExprArray, UniqueFEIRExpr argExprIndex, UniqueFEIRType argTypeNative);
  ~FEIRExprArrayStoreForC() = default;

  std::unique_ptr<FEIRExpr> CloneImpl() const override;

  FEIRExpr &GetExprArray() const {
    ASSERT(exprArray != nullptr, "exprArray is nullptr");
    return *exprArray.get();
  }

  FEIRExpr &GetExprIndex() const {
    ASSERT(exprIndex != nullptr, "exprIndex is nullptr");
    return *exprIndex.get();
  }

  FEIRType &GetTypeArray() const {
    ASSERT(typeNative != nullptr, "typeNative is nullptr");
    return *typeNative.get();
  }

 private:
  BaseNode *GenMIRNodeImpl(MIRBuilder &mirBuilder) const override;
  UniqueFEIRExpr exprArray;
  UniqueFEIRExpr exprIndex;
  UniqueFEIRType typeNative;
};

// ---------- FEIRExprArrayLoad ----------
class FEIRExprArrayLoad : public FEIRExpr {
 public:
  FEIRExprArrayLoad(UniqueFEIRExpr argExprArray, UniqueFEIRExpr argExprIndex, UniqueFEIRType argTypeArray);
  ~FEIRExprArrayLoad() = default;
  const UniqueFEIRType GetElemType() const {
    UniqueFEIRType typeElem = typeArray->Clone();
    (void)typeElem->ArrayDecrDim();
    return typeElem;
  }

  UniqueFEIRVarTrans CreateTransArrayDimDecr() {
    FEIRExprDRead *dRead = static_cast<FEIRExprDRead*>(exprArray.get());
    return dRead->CreateTransArrayDimDecr();
  }

  void SetTrans(UniqueFEIRVarTrans argTrans) {
    CHECK_FATAL(argTrans->GetTransKind() == kFEIRVarTransArrayDimIncr, "ArrayLoad must hold DimIncr Transfer Function");
    FEIRExprDRead *dRead = static_cast<FEIRExprDRead*>(exprArray.get());
    dRead->SetTrans(std::move(argTrans));
  }

 protected:
  std::unique_ptr<FEIRExpr> CloneImpl() const override;
  std::vector<FEIRVar*> GetVarUsesImpl() const override;
  void RegisterDFGNodes2CheckPointImpl(FEIRStmtCheckPoint &checkPoint) override;
  bool CalculateDefs4AllUsesImpl(FEIRStmtCheckPoint &checkPoint, FEIRUseDefChain &udChain) override;
  BaseNode *GenMIRNodeImpl(MIRBuilder &mirBuilder) const override;

 private:
  UniqueFEIRExpr exprArray;
  UniqueFEIRExpr exprIndex;
  UniqueFEIRType typeArray;
};

class FEIRExprCStyleCast : public FEIRExpr {
 public:
  FEIRExprCStyleCast(MIRType *src, MIRType *dest, UniqueFEIRExpr sub, bool isArr2Pty);
  ~FEIRExprCStyleCast() = default;
  void SetArray2Pointer(bool isArr2Ptr) {
    isArray2Pointer = isArr2Ptr;
  }
  void SetRefName(std::string name) {
    refName = name;
  }

 protected:
  std::unique_ptr<FEIRExpr> CloneImpl() const override;
  BaseNode *GenMIRNodeImpl(MIRBuilder &mirBuilder) const override;

 private:
  MIRType *srcType = nullptr;
  MIRType *destType = nullptr;
  UniqueFEIRExpr subExpr;
  bool isArray2Pointer;
  std::string refName;
};

enum ASTAtomicOp {
  kAtomicBinaryOpAdd,
  kAtomicBinaryOpSub,
  kAtomicBinaryOpAnd,
  kAtomicBinaryOpOr,
  kAtomicBinaryOpXor,
  kAtomicOpLoad,
  kAtomicOpStore,
  kAtomicOpExchange,
  kAtomicOpCompareExchange,
  kAtomicOpLast,
};

class FEIRExprAtomic : public FEIRExpr {
 public:
  FEIRExprAtomic(MIRType *ty, MIRType *ref, UniqueFEIRExpr obj, UniqueFEIRExpr val1,
                 UniqueFEIRExpr val2,
                 ASTAtomicOp atomOp);
  ~FEIRExprAtomic() = default;
  void SetVal1Type(MIRType *ty) {
    val1Type = ty;
  }
  void SetVal2Type(MIRType *ty) {
    val2Type = ty;
  }

 protected:
  std::unique_ptr<FEIRExpr> CloneImpl() const override;
  BaseNode *GenMIRNodeImpl(MIRBuilder &mirBuilder) const override;

 private:
  void ProcessAtomicBinary(MIRBuilder &mirBuilder, BlockNode &block, BaseNode &lockNode,
                           MIRSymbol &valueVar, ASTAtomicOp opcode) const;
  void ProcessAtomicLoad(MIRBuilder &mirBuilder, BlockNode &block, BaseNode &lockNode,
                         MIRSymbol &valueVar) const;
  void ProcessAtomicStore(MIRBuilder &mirBuilder, BlockNode &block, BaseNode &lockNode,
                          MIRSymbol &valueVar) const;
  void ProcessAtomicExchange(MIRBuilder &mirBuilder, BlockNode &block, BaseNode &lockNode,
                             MIRSymbol &valueVar) const;
  void ProcessAtomicCompareExchange(MIRBuilder &mirBuilder, BlockNode &block, BaseNode &lockNode,
                                    MIRSymbol *valueVar) const;
  MIRType *type = nullptr;
  MIRType *refType = nullptr;
  MIRType *val1Type = nullptr;
  MIRType *val2Type = nullptr;
  UniqueFEIRExpr objExpr;
  UniqueFEIRExpr valExpr1;
  UniqueFEIRExpr valExpr2;
  ASTAtomicOp atomicOp;
};

// ---------- FEIRStmtNary ----------
class FEIRStmtNary : public FEIRStmt {
 public:
  FEIRStmtNary(Opcode opIn, std::list<std::unique_ptr<FEIRExpr>> argExprsIn);
  ~FEIRStmtNary() = default;

 protected:
  std::list<StmtNode*> GenMIRStmtsImpl(MIRBuilder &mirBuilder) const override;
  Opcode op;
  std::list<std::unique_ptr<FEIRExpr>> argExprs;
};

// ---------- FEIRStmtAssign ----------
class FEIRStmtAssign : public FEIRStmt {
 public:
  FEIRStmtAssign(FEIRNodeKind argKind, std::unique_ptr<FEIRVar> argVar);
  ~FEIRStmtAssign() = default;
  FEIRVar *GetVar() const {
    return var.get();
  }

  void SetVar(std::unique_ptr<FEIRVar> argVar) {
    var = std::move(argVar);
    var->SetDef(HasDef());
  }

  bool HasException() const {
    return hasException;
  }

  void SetHasException(bool arg) {
    hasException = arg;
  }

 protected:
  bool HasDefImpl() const override {
    return ((var != nullptr) && (var.get() != nullptr));
  }

  FEIRVarTypeScatter* GetTypeScatterDefVarImpl() const override {
    if (!HasDefImpl()) {
      return nullptr;
    }
    if (var->GetKind() == kFEIRVarTypeScatter) {
      FEIRVarTypeScatter *varTypeScatter = static_cast<FEIRVarTypeScatter*>(var.get());
      return varTypeScatter;
    }
    return nullptr;
  }

  std::string DumpDotStringImpl() const override;
  void RegisterDFGNodes2CheckPointImpl(FEIRStmtCheckPoint &checkPoint) override;
  bool hasException;
  std::unique_ptr<FEIRVar> var;
};

// ---------- FEIRStmtDAssign ----------
class FEIRStmtDAssign : public FEIRStmtAssign {
 public:
  FEIRStmtDAssign(std::unique_ptr<FEIRVar> argVar, std::unique_ptr<FEIRExpr> argExpr, int32 argFieldID = 0);
  ~FEIRStmtDAssign() = default;
  FEIRExpr *GetExpr() const {
    return expr.get();
  }

  void SetExpr(std::unique_ptr<FEIRExpr> argExpr) {
    expr = std::move(argExpr);
  }

  void SetFieldName(std::string argFieldName) {
    fieldName = argFieldName;
  }

 protected:
  std::string DumpDotStringImpl() const override;
  void RegisterDFGNodes2CheckPointImpl(FEIRStmtCheckPoint &checkPoint) override;
  bool CalculateDefs4AllUsesImpl(FEIRStmtCheckPoint &checkPoint, FEIRUseDefChain &udChain) override;
  void InitTrans4AllVarsImpl() override;
  std::list<StmtNode*> GenMIRStmtsImpl(MIRBuilder &mirBuilder) const override;
  std::unique_ptr<FEIRExpr> expr;
  int32 fieldID;
  std::string fieldName;
};

// ---------- FEIRStmtIAssign ----------
class FEIRStmtIAssign : public FEIRStmt {
 public:
  FEIRStmtIAssign(UniqueFEIRType argAddrType, UniqueFEIRExpr argAddrExpr, UniqueFEIRExpr argBaseExpr, FieldID id)
      : FEIRStmt(FEIRNodeKind::kStmtIAssign),
        addrType(std::move(argAddrType)),
        addrExpr(std::move(argAddrExpr)),
        baseExpr(std::move(argBaseExpr)),
        fieldID(id) {}
  ~FEIRStmtIAssign() = default;

  void SetFieldName(std::string name){
    fieldName = std::move(name);
  }

 protected:
  std::list<StmtNode *> GenMIRStmtsImpl(MIRBuilder &mirBuilder) const override;
  UniqueFEIRType addrType;
  UniqueFEIRExpr addrExpr;
  UniqueFEIRExpr baseExpr;
  std::string fieldName;
  FieldID fieldID;
};

// ---------- FEIRStmtJavaTypeCheck ----------
class FEIRStmtJavaTypeCheck : public FEIRStmtAssign {
 public:
  enum CheckKind {
    kCheckCast,
    kInstanceOf
  };

  FEIRStmtJavaTypeCheck(std::unique_ptr<FEIRVar> argVar, std::unique_ptr<FEIRExpr> argExpr,
                        std::unique_ptr<FEIRType> argType, CheckKind argCheckKind);
  FEIRStmtJavaTypeCheck(std::unique_ptr<FEIRVar> argVar, std::unique_ptr<FEIRExpr> argExpr,
                        std::unique_ptr<FEIRType> argType, CheckKind argCheckKind, uint32 argTypeID);
  ~FEIRStmtJavaTypeCheck() = default;

 protected:
  std::string DumpDotStringImpl() const override;
  void RegisterDFGNodes2CheckPointImpl(FEIRStmtCheckPoint &checkPoint) override;
  bool CalculateDefs4AllUsesImpl(FEIRStmtCheckPoint &checkPoint, FEIRUseDefChain &udChain) override;
  std::list<StmtNode*> GenMIRStmtsImpl(MIRBuilder &mirBuilder) const override;
  CheckKind checkKind;
  std::unique_ptr<FEIRExpr> expr;
  std::unique_ptr<FEIRType> type;
  uint32 typeID = UINT32_MAX;
};

// ---------- FEIRStmtJavaConstClass ----------
class FEIRStmtJavaConstClass : public FEIRStmtAssign {
 public:
  FEIRStmtJavaConstClass(std::unique_ptr<FEIRVar> argVar, std::unique_ptr<FEIRType> argType);
  ~FEIRStmtJavaConstClass() = default;

 protected:
  void RegisterDFGNodes2CheckPointImpl(FEIRStmtCheckPoint &checkPoint) override;
  std::list<StmtNode*> GenMIRStmtsImpl(MIRBuilder &mirBuilder) const override;
  std::unique_ptr<FEIRType> type;
};

// ---------- FEIRStmtJavaConstString ----------
class FEIRStmtJavaConstString : public FEIRStmtAssign {
 public:
  FEIRStmtJavaConstString(std::unique_ptr<FEIRVar> argVar, const std::string &argStrVal,
                          uint32 argFileIdx, uint32 argStringID);
  ~FEIRStmtJavaConstString() = default;

 protected:
  void RegisterDFGNodes2CheckPointImpl(FEIRStmtCheckPoint &checkPoint) override;
  std::list<StmtNode*> GenMIRStmtsImpl(MIRBuilder &mirBuilder) const override;

 private:
  std::string strVal;
  uint32 fileIdx;
  uint32 stringID = UINT32_MAX;
};

// ---------- FEIRStmtJavaFillArrayData ----------
class FEIRStmtJavaFillArrayData : public FEIRStmtAssign {
 public:
  FEIRStmtJavaFillArrayData(std::unique_ptr<FEIRExpr> arrayExprIn, const int8 *arrayDataIn,
                            uint32 sizeIn, const std::string &arrayNameIn);
  ~FEIRStmtJavaFillArrayData() = default;

 protected:
  void RegisterDFGNodes2CheckPointImpl(FEIRStmtCheckPoint &checkPoint) override;
  bool CalculateDefs4AllUsesImpl(FEIRStmtCheckPoint &checkPoint, FEIRUseDefChain &udChain) override;
  std::list<StmtNode*> GenMIRStmtsImpl(MIRBuilder &mirBuilder) const override;

 LLT_PRIVATE:
  PrimType ProcessArrayElemPrimType() const;
  MIRSymbol *ProcessArrayElemData(MIRBuilder &mirBuilder, PrimType elemPrimType) const;
  MIRAggConst *FillArrayElem(MIRBuilder &mirBuilder, PrimType elemPrimType, MIRType &arrayTypeWithSize,
                             const int8 *arrayData, uint32 size) const;

  std::unique_ptr<FEIRExpr> arrayExpr;
  const int8 *arrayData = nullptr;
  uint32 size = 0;
  const std::string arrayName;
};

// ---------- FEIRStmtJavaMultiANewArray ----------
class FEIRStmtJavaMultiANewArray : public FEIRStmtAssign {
 public:
  FEIRStmtJavaMultiANewArray(std::unique_ptr<FEIRVar> argVar, std::unique_ptr<FEIRType> argElemType,
                             std::unique_ptr<FEIRType> argArrayType);
  ~FEIRStmtJavaMultiANewArray() = default;
  void AddVarSize(std::unique_ptr<FEIRVar> argVarSize);
  void AddVarSizeRev(std::unique_ptr<FEIRVar> argVarSize);
  void SetArrayType(std::unique_ptr<FEIRType> argArrayType) {
    arrayType = std::move(argArrayType);
  }

 protected:
  std::string DumpDotStringImpl() const override;
  void RegisterDFGNodes2CheckPointImpl(FEIRStmtCheckPoint &checkPoint) override;
  bool CalculateDefs4AllUsesImpl(FEIRStmtCheckPoint &checkPoint, FEIRUseDefChain &udChain) override;
  std::list<StmtNode*> GenMIRStmtsImpl(MIRBuilder &mirBuilder) const override;

 private:
  static const UniqueFEIRVar &GetVarSize();
  static const UniqueFEIRVar &GetVarClass();
  static const UniqueFEIRType &GetTypeAnnotation();
  static FEStructMethodInfo &GetMethodInfoNewInstance();

  std::unique_ptr<FEIRType> elemType;
  std::unique_ptr<FEIRType> arrayType;
  std::list<std::unique_ptr<FEIRExpr>> exprSizes;
  static UniqueFEIRVar varSize;
  static UniqueFEIRVar varClass;
  static UniqueFEIRType typeAnnotation;
  static FEStructMethodInfo *methodInfoNewInstance;
};

// ---------- FEIRStmtUseOnly ----------
class FEIRStmtUseOnly : public FEIRStmt {
 public:
  FEIRStmtUseOnly(FEIRNodeKind argKind, Opcode argOp, std::unique_ptr<FEIRExpr> argExpr);
  FEIRStmtUseOnly(Opcode argOp, std::unique_ptr<FEIRExpr> argExpr);
  ~FEIRStmtUseOnly() = default;

 protected:
  bool IsFallThroughImpl() const override {
    if ((op == OP_return) || (op == OP_throw)) {
      return false;
    }
    return true;
  }

  std::string DumpDotStringImpl() const override;
  void RegisterDFGNodes2CheckPointImpl(FEIRStmtCheckPoint &checkPoint) override;
  bool CalculateDefs4AllUsesImpl(FEIRStmtCheckPoint &checkPoint, FEIRUseDefChain &udChain) override;
  std::list<StmtNode*> GenMIRStmtsImpl(MIRBuilder &mirBuilder) const override;
  Opcode op;
  std::unique_ptr<FEIRExpr> expr;
};

// ---------- FEIRStmtReturn ----------
class FEIRStmtReturn : public FEIRStmtUseOnly {
 public:
  explicit FEIRStmtReturn(std::unique_ptr<FEIRExpr> argExpr);
  ~FEIRStmtReturn() = default;

 protected:
  std::list<StmtNode*> GenMIRStmtsImpl(MIRBuilder &mirBuilder) const override;
};

// ---------- FEIRStmtPesudoLabel ----------
class FEIRStmtPesudoLabel : public FEIRStmt {
 public:
  FEIRStmtPesudoLabel(uint32 argLabelIdx);
  ~FEIRStmtPesudoLabel() = default;
  void GenerateLabelIdx(MIRBuilder &mirBuilder);

  uint32 GetLabelIdx() const {
    return labelIdx;
  }

  LabelIdx GetMIRLabelIdx() const {
    return mirLabelIdx;
  }

 protected:
  bool IsTargetImpl() const override {
    return true;
  }

  std::string DumpDotStringImpl() const override;
  std::list<StmtNode*> GenMIRStmtsImpl(MIRBuilder &mirBuilder) const override;
  uint32 labelIdx;
  LabelIdx mirLabelIdx;
};

class FEIRStmtPesudoLabel2 : public FEIRStmt {
 public:
  FEIRStmtPesudoLabel2(uint32 qIdx0, uint32 qIdx1)
      : FEIRStmt(FEIRNodeKind::kStmtPesudoLabel), labelIdxOuter(qIdx0), labelIdxInner(qIdx1) {}

  ~FEIRStmtPesudoLabel2() = default;
  static LabelIdx GenMirLabelIdx(MIRBuilder &mirBuilder, uint32 qIdx0, uint32 qIdx1);
  std::pair<uint32, uint32> GetLabelIdx() const;
  uint32 GetPos() const {
    return labelIdxInner;
  }

 protected:
  bool IsTargetImpl() const override {
    return true;
  }
  std::list<StmtNode*> GenMIRStmtsImpl(MIRBuilder &mirBuilder) const override;

  uint32 labelIdxOuter;
  uint32 labelIdxInner;
};

// ---------- FEIRStmtGoto ----------
class FEIRStmtGoto : public FEIRStmt {
 public:
  explicit FEIRStmtGoto(uint32 argLabelIdx);
  virtual ~FEIRStmtGoto();
  void SetLabelIdx(uint32 argLabelIdx) {
    labelIdx = argLabelIdx;
  }

  uint32 GetLabelIdx() const {
    return labelIdx;
  }

  void SetStmtTarget(FEIRStmtPesudoLabel &argStmtTarget) {
    stmtTarget = &argStmtTarget;
  }

  const FEIRStmtPesudoLabel &GetStmtTargetRef() const {
    CHECK_NULL_FATAL(stmtTarget);
    return *stmtTarget;
  }

 protected:
  bool IsFallThroughImpl() const override {
    return false;
  }

  bool IsBranchImpl() const override {
    return true;
  }

  std::string DumpDotStringImpl() const override;
  std::list<StmtNode*> GenMIRStmtsImpl(MIRBuilder &mirBuilder) const override;
  uint32 labelIdx;
  FEIRStmtPesudoLabel *stmtTarget;
};

// ---------- FEIRStmtGoto2 ----------
class FEIRStmtGoto2 : public FEIRStmt {
 public:
  FEIRStmtGoto2(uint32 qIdx0, uint32 qIdx1);
  virtual ~FEIRStmtGoto2() = default;
  std::pair<uint32, uint32> GetLabelIdx() const;
  uint32 GetTarget() const {
    return labelIdxInner;
  }

  void SetStmtTarget(FEIRStmtPesudoLabel2 &argStmtTarget) {
    stmtTarget = &argStmtTarget;
  }

  const FEIRStmtPesudoLabel2 &GetStmtTargetRef() const {
    CHECK_NULL_FATAL(stmtTarget);
    return *stmtTarget;
  }

 protected:
  bool IsFallThroughImpl() const override {
    return false;
  }

  bool IsBranchImpl() const override {
    return true;
  }

  std::list<StmtNode*> GenMIRStmtsImpl(MIRBuilder &mirBuilder) const override;

  uint32 labelIdxOuter;
  uint32 labelIdxInner;
  FEIRStmtPesudoLabel2 *stmtTarget = nullptr;
};

// ---------- FEIRStmtGoto ----------
class FEIRStmtGotoForC : public FEIRStmt {
 public:
  explicit FEIRStmtGotoForC(std::string labelName);
  virtual ~FEIRStmtGotoForC() = default;
  void SetLabelName(std::string name) {
    labelName = std::move(name);
  }

  std::string GetLabelName() const {
    return labelName;
  }

 protected:
  bool IsFallThroughImpl() const override {
    return false;
  }

  bool IsBranchImpl() const override {
    return true;
  }

  std::string DumpDotStringImpl() const override;
  std::list<StmtNode*> GenMIRStmtsImpl(MIRBuilder &mirBuilder) const override;
  std::string labelName;
};

// ---------- FEIRStmtCondGoto ----------
class FEIRStmtCondGoto : public FEIRStmtGoto {
 public:
  FEIRStmtCondGoto(Opcode argOp, uint32 argLabelIdx, UniqueFEIRExpr argExpr);
  ~FEIRStmtCondGoto() = default;
  void SetOpcode(Opcode argOp) {
    op = argOp;
  }

  Opcode GetOpcode() const {
    return op;
  }

  void SetExpr(UniqueFEIRExpr argExpr) {
    CHECK_NULL_FATAL(argExpr);
    expr = std::move(argExpr);
  }

 protected:
  bool IsFallThroughImpl() const override {
    return true;
  }

  bool IsBranchImpl() const override {
    return true;
  }

  std::string DumpDotStringImpl() const override;
  void RegisterDFGNodes2CheckPointImpl(FEIRStmtCheckPoint &checkPoint) override;
  bool CalculateDefs4AllUsesImpl(FEIRStmtCheckPoint &checkPoint, FEIRUseDefChain &udChain) override;
  std::list<StmtNode*> GenMIRStmtsImpl(MIRBuilder &mirBuilder) const override;

 private:
  Opcode op;
  UniqueFEIRExpr expr;
};

// ---------- FEIRStmtCondGoto2 ----------
class FEIRStmtCondGoto2 : public FEIRStmtGoto2 {
 public:
  FEIRStmtCondGoto2(Opcode argOp, uint32 qIdx0, uint32 qIdx1, UniqueFEIRExpr argExpr);
  ~FEIRStmtCondGoto2() = default;

 protected:
  bool IsFallThroughImpl() const override {
    return true;
  }

  bool IsBranchImpl() const override {
    return true;
  }

  std::list<StmtNode*> GenMIRStmtsImpl(MIRBuilder &mirBuilder) const override;

 private:
  Opcode op;
  UniqueFEIRExpr expr;
};

// ---------- FEIRStmtSwitch ----------
class FEIRStmtSwitch : public FEIRStmt {
 public:
  explicit FEIRStmtSwitch(UniqueFEIRExpr argExpr);
  ~FEIRStmtSwitch();
  void SetDefaultLabelIdx(uint32 labelIdx) {
    defaultLabelIdx = labelIdx;
  }

  uint32 GetDefaultLabelIdx() const {
    return defaultLabelIdx;
  }

  void SetDefaultTarget(FEIRStmtPesudoLabel &stmtTarget) {
    defaultTarget = &stmtTarget;
  }

  const FEIRStmtPesudoLabel &GetDefaultTarget() const {
    return *defaultTarget;
  }

  const std::map<int32, uint32> &GetMapValueLabelIdx() const {
    return mapValueLabelIdx;
  }

  const std::map<int32, FEIRStmtPesudoLabel*> &GetMapValueTargets() const {
    return mapValueTargets;
  }

  void AddTarget(int32 value, uint32 labelIdx) {
    mapValueLabelIdx[value] = labelIdx;
  }

  void AddTarget(int32 value, FEIRStmtPesudoLabel &target) {
    mapValueTargets[value] = &target;
  }

  void SetExpr(UniqueFEIRExpr argExpr) {
    CHECK_NULL_FATAL(argExpr);
    expr = std::move(argExpr);
  }

 protected:
  bool IsFallThroughImpl() const override {
    return true;
  }

  bool IsBranchImpl() const override {
    return true;
  }

  std::string DumpDotStringImpl() const override;
  void RegisterDFGNodes2CheckPointImpl(FEIRStmtCheckPoint &checkPoint) override;
  bool CalculateDefs4AllUsesImpl(FEIRStmtCheckPoint &checkPoint, FEIRUseDefChain &udChain) override;
  std::list<StmtNode*> GenMIRStmtsImpl(MIRBuilder &mirBuilder) const override;

 private:
  uint32 defaultLabelIdx;
  FEIRStmtPesudoLabel *defaultTarget;
  std::map<int32, uint32> mapValueLabelIdx;
  std::map<int32, FEIRStmtPesudoLabel*> mapValueTargets;
  UniqueFEIRExpr expr;
};

// ---------- FEIRStmtSwitch2 ----------
class FEIRStmtSwitch2 : public FEIRStmt {
 public:
  explicit FEIRStmtSwitch2(uint32 outerIdxIn, UniqueFEIRExpr argExpr);
  ~FEIRStmtSwitch2();
  void SetDefaultLabelIdx(uint32 labelIdx) {
    defaultLabelIdx = labelIdx;
  }

  uint32 GetDefaultLabelIdx() const {
    return defaultLabelIdx;
  }

  void SetDefaultTarget(FEIRStmtPesudoLabel2 *stmtTarget) {
    defaultTarget = stmtTarget;
  }

  const FEIRStmtPesudoLabel2 &GetDefaultTarget() const {
    return *defaultTarget;
  }

  const std::map<int32, uint32> &GetMapValueLabelIdx() const {
    return mapValueLabelIdx;
  }

  const std::map<int32, FEIRStmtPesudoLabel2*> &GetMapValueTargets() const {
    return mapValueTargets;
  }

  void AddTarget(int32 value, uint32 labelIdx) {
    mapValueLabelIdx[value] = labelIdx;
  }

  void AddTarget(int32 value, FEIRStmtPesudoLabel2 *target) {
    mapValueTargets[value] = target;
  }

  void SetExpr(UniqueFEIRExpr argExpr) {
    CHECK_NULL_FATAL(argExpr);
    expr = std::move(argExpr);
  }

 protected:
  bool IsFallThroughImpl() const override {
    return true;
  }

  bool IsBranchImpl() const override {
    return true;
  }

  std::string DumpDotStringImpl() const override;
  std::list<StmtNode*> GenMIRStmtsImpl(MIRBuilder &mirBuilder) const override;

 private:
  uint32 outerIdx;
  uint32 defaultLabelIdx = UINT32_MAX;
  FEIRStmtPesudoLabel2 *defaultTarget;
  std::map<int32, uint32> mapValueLabelIdx;
  std::map<int32, FEIRStmtPesudoLabel2*> mapValueTargets;
  UniqueFEIRExpr expr;
};

// ---------- FEIRStmtSwitchForC ----------
class FEIRStmtSwitchForC : public FEIRStmt {
 public:
  FEIRStmtSwitchForC(UniqueFEIRExpr argCondExpr, bool argHasDefault);
  ~FEIRStmtSwitchForC() = default;
  void AddFeirStmt(UniqueFEIRStmt stmt) {
    subStmts.emplace_back(std::move(stmt));
  }

  void SetExpr(UniqueFEIRExpr argExpr) {
    CHECK_NULL_FATAL(argExpr);
    expr = std::move(argExpr);
  }

  void SetHasDefault(bool argHasDefault) {
    hasDefault = argHasDefault;
  }

 protected:
  std::string DumpDotStringImpl() const override;
  void RegisterDFGNodes2CheckPointImpl(FEIRStmtCheckPoint &checkPoint) override;
  bool CalculateDefs4AllUsesImpl(FEIRStmtCheckPoint &checkPoint, FEIRUseDefChain &udChain) override;
  std::list<StmtNode*> GenMIRStmtsImpl(MIRBuilder &mirBuilder) const override;

 private:
  UniqueFEIRExpr expr;
  bool hasDefault = true;
  std::list<UniqueFEIRStmt> subStmts;
};

// ---------- FEIRStmtCaseForC ----------
class FEIRStmtCaseForC : public FEIRStmt {
 public:
  FEIRStmtCaseForC(int64 lCaseLabel);
  void AddCaseTag2CaseVec(int64 lCaseTag, int64 rCaseTag);
  ~FEIRStmtCaseForC() = default;
  void AddFeirStmt(UniqueFEIRStmt stmt) {
    subStmts.emplace_back(std::move(stmt));
  }
  std::map<int32, FEIRStmtPesudoLabel*> &GetPesudoLabelMap() {
    return pesudoLabelMap;
  }
 protected:
  std::string DumpDotStringImpl() const override;
  std::list<StmtNode*> GenMIRStmtsImpl(MIRBuilder &mirBuilder) const override;

 private:
  int64 lCaseLabel;
  std::map<int32, FEIRStmtPesudoLabel*> pesudoLabelMap = std::map<int32, FEIRStmtPesudoLabel*>();
  std::list<UniqueFEIRStmt> subStmts;
};

// ---------- FEIRStmtDefaultForC ----------
class FEIRStmtDefaultForC : public FEIRStmt {
 public:
  explicit FEIRStmtDefaultForC();
  ~FEIRStmtDefaultForC() = default;
  void AddFeirStmt(UniqueFEIRStmt stmt) {
    subStmts.emplace_back(std::move(stmt));
  }

 protected:
  std::string DumpDotStringImpl() const override;
  std::list<StmtNode*> GenMIRStmtsImpl(MIRBuilder &mirBuilder) const override;
  std::list<UniqueFEIRStmt> subStmts;
};

// ---------- FEIRStmtArrayStore ----------
class FEIRStmtArrayStore : public FEIRStmt {
 public:
  FEIRStmtArrayStore(UniqueFEIRExpr argExprElem, UniqueFEIRExpr argExprArray, UniqueFEIRExpr argExprIndex,
                     UniqueFEIRType argTypeArray);
  ~FEIRStmtArrayStore() = default;

 protected:
  std::string DumpDotStringImpl() const override;
  void RegisterDFGNodes2CheckPointImpl(FEIRStmtCheckPoint &checkPoint) override;
  bool CalculateDefs4AllUsesImpl(FEIRStmtCheckPoint &checkPoint, FEIRUseDefChain &udChain) override;
  void InitTrans4AllVarsImpl() override;
  std::list<StmtNode*> GenMIRStmtsImpl(MIRBuilder &mirBuilder) const override;

 private:
  UniqueFEIRExpr exprElem;
  UniqueFEIRExpr exprArray;
  UniqueFEIRExpr exprIndex;
  UniqueFEIRType typeArray;
};

// ---------- FEIRStmtFieldStore ----------
class FEIRStmtFieldStore : public FEIRStmt {
 public:
  FEIRStmtFieldStore(UniqueFEIRVar argVarObj, UniqueFEIRVar argVarField, FEStructFieldInfo &argFieldInfo,
                     bool argIsStatic);
  FEIRStmtFieldStore(UniqueFEIRVar argVarObj, UniqueFEIRVar argVarField, FEStructFieldInfo &argFieldInfo,
                     bool argIsStatic, int32 argDexFileHashCode);
  ~FEIRStmtFieldStore() = default;

 protected:
  std::string DumpDotStringImpl() const override;
  void RegisterDFGNodes2CheckPointImpl(FEIRStmtCheckPoint &checkPoint) override;
  bool CalculateDefs4AllUsesImpl(FEIRStmtCheckPoint &checkPoint, FEIRUseDefChain &udChain) override;
  std::list<StmtNode*> GenMIRStmtsImpl(MIRBuilder &mirBuilder) const override;

 private:
  void RegisterDFGNodes2CheckPointForStatic(FEIRStmtCheckPoint &checkPoint);
  void RegisterDFGNodes2CheckPointForNonStatic(FEIRStmtCheckPoint &checkPoint);
  bool CalculateDefs4AllUsesForStatic(FEIRStmtCheckPoint &checkPoint, FEIRUseDefChain &udChain);
  bool CalculateDefs4AllUsesForNonStatic(FEIRStmtCheckPoint &checkPoint, FEIRUseDefChain &udChain);
  bool NeedMCCForStatic(uint32 &typeID) const;
  void InitPrimTypeFuncNameIdxMap (std::map<PrimType, GStrIdx> &primTypeFuncNameIdxMap) const;
  std::list<StmtNode*> GenMIRStmtsImplForStatic(MIRBuilder &mirBuilder) const;
  std::list<StmtNode*> GenMIRStmtsImplForNonStatic(MIRBuilder &mirBuilder) const;

  UniqueFEIRVar varObj;
  UniqueFEIRVar varField;
  FEStructFieldInfo &fieldInfo;
  bool isStatic;
  int32 dexFileHashCode = -1;
};

// ---------- FEIRStmtFieldLoad ----------
class FEIRStmtFieldLoad : public FEIRStmtAssign {
 public:
  FEIRStmtFieldLoad(UniqueFEIRVar argVarObj, UniqueFEIRVar argVarField, FEStructFieldInfo &argFieldInfo,
                    bool argIsStatic);
  FEIRStmtFieldLoad(UniqueFEIRVar argVarObj, UniqueFEIRVar argVarField, FEStructFieldInfo &argFieldInfo,
                    bool argIsStatic, int32 argDexFileHashCode);
  ~FEIRStmtFieldLoad() = default;

 protected:
  std::string DumpDotStringImpl() const override;
  void RegisterDFGNodes2CheckPointImpl(FEIRStmtCheckPoint &checkPoint) override;
  bool CalculateDefs4AllUsesImpl(FEIRStmtCheckPoint &checkPoint, FEIRUseDefChain &udChain) override;
  std::list<StmtNode*> GenMIRStmtsImpl(MIRBuilder &mirBuilder) const override;

 private:
  void RegisterDFGNodes2CheckPointForStatic(FEIRStmtCheckPoint &checkPoint);
  void RegisterDFGNodes2CheckPointForNonStatic(FEIRStmtCheckPoint &checkPoint);
  bool CalculateDefs4AllUsesForStatic(FEIRStmtCheckPoint &checkPoint, FEIRUseDefChain &udChain);
  bool CalculateDefs4AllUsesForNonStatic(FEIRStmtCheckPoint &checkPoint, FEIRUseDefChain &udChain);
  bool NeedMCCForStatic(uint32 &typeID) const;
  std::list<StmtNode*> GenMIRStmtsImplForStatic(MIRBuilder &mirBuilder) const;
  std::list<StmtNode*> GenMIRStmtsImplForNonStatic(MIRBuilder &mirBuilder) const;

  UniqueFEIRVar varObj;
  FEStructFieldInfo &fieldInfo;
  bool isStatic;
  int32 dexFileHashCode = -1;
};

// ---------- FEIRStmtCallAssign ----------
class FEIRStmtCallAssign : public FEIRStmtAssign {
 public:
  FEIRStmtCallAssign(FEStructMethodInfo &argMethodInfo, Opcode argMIROp, UniqueFEIRVar argVarRet, bool argIsStatic);
  ~FEIRStmtCallAssign() = default;
  void AddExprArg(UniqueFEIRExpr exprArg) {
    exprArgs.push_back(std::move(exprArg));
  }

  void AddExprArgReverse(UniqueFEIRExpr exprArg) {
    exprArgs.push_front(std::move(exprArg));
  }

  static std::map<Opcode, Opcode> InitMapOpAssignToOp();
  static std::map<Opcode, Opcode> InitMapOpToOpAssign();

 protected:
  std::string DumpDotStringImpl() const override;
  void RegisterDFGNodes2CheckPointImpl(FEIRStmtCheckPoint &checkPoint) override;
  bool CalculateDefs4AllUsesImpl(FEIRStmtCheckPoint &checkPoint, FEIRUseDefChain &udChain) override;
  std::list<StmtNode*> GenMIRStmtsImpl(MIRBuilder &mirBuilder) const override;
  std::list<StmtNode*> GenMIRStmtsUseZeroReturn(MIRBuilder &mirBuilder) const;

 private:
  Opcode AdjustMIROp() const;

  FEStructMethodInfo &methodInfo;
  Opcode mirOp;
  bool isStatic;
  std::list<UniqueFEIRExpr> exprArgs;
  static std::map<Opcode, Opcode> mapOpAssignToOp;
  static std::map<Opcode, Opcode> mapOpToOpAssign;
};

// ---------- FEIRStmtIntrinsicCallAssign ----------
class FEIRStmtIntrinsicCallAssign : public FEIRStmtAssign {
 public:
  FEIRStmtIntrinsicCallAssign(MIRIntrinsicID id, UniqueFEIRType typeIn, UniqueFEIRVar argVarRet);
  FEIRStmtIntrinsicCallAssign(MIRIntrinsicID id, UniqueFEIRType typeIn, UniqueFEIRVar argVarRet,
                              std::unique_ptr<std::list<UniqueFEIRExpr>> exprListIn);
  FEIRStmtIntrinsicCallAssign(MIRIntrinsicID id, const std::string &funcNameIn, const std::string &protoIN,
                              std::unique_ptr<std::list<UniqueFEIRVar>> argsIn);
  FEIRStmtIntrinsicCallAssign(MIRIntrinsicID id, const std::string &funcNameIn, const std::string &protoIN,
                              std::unique_ptr<std::list<UniqueFEIRVar>> argsIn, uint32 callerClassTypeIDIn,
                              bool isInStaticFuncIn);
  FEIRStmtIntrinsicCallAssign(MIRIntrinsicID id, UniqueFEIRType typeIn, UniqueFEIRVar argVarRet,
                              uint32 typeIDIn);
  ~FEIRStmtIntrinsicCallAssign() = default;

 protected:
  std::string DumpDotStringImpl() const override;
  void RegisterDFGNodes2CheckPointImpl(FEIRStmtCheckPoint &checkPoint) override;
  std::list<StmtNode*> GenMIRStmtsImpl(MIRBuilder &mirBuilder) const override;

 private:
  void ConstructArgsForInvokePolyMorphic(MIRBuilder &mirBuilder, MapleVector<BaseNode*> &intrnCallargs) const;
  std::list<StmtNode*> GenMIRStmtsForInvokePolyMorphic(MIRBuilder &mirBuilder) const;
  MIRIntrinsicID intrinsicId;
  UniqueFEIRType type;
  std::unique_ptr<std::list<UniqueFEIRExpr>> exprList;
  // for polymorphic
  const std::string funcName;
  const std::string proto;
  std::unique_ptr<std::list<UniqueFEIRVar>> polyArgs;
  uint32 typeID = UINT32_MAX;
  uint32 callerClassTypeID = UINT32_MAX;
  bool isInStaticFunc = false;
};

// ---------- FEIRStmtPesudoLOC ----------
class FEIRStmtPesudoLOC : public FEIRStmt {
 public:
  FEIRStmtPesudoLOC(uint32 argSrcFileIdx, uint32 argLineNumber);
  ~FEIRStmtPesudoLOC() = default;
  uint32 GetSrcFileIdx() const {
    return srcFileIdx;
  }

  uint32 GetLineNumber() const {
    return lineNumber;
  }

 protected:
  std::string DumpDotStringImpl() const override;
  std::list<StmtNode*> GenMIRStmtsImpl(MIRBuilder &mirBuilder) const override;

 private:
  uint32 srcFileIdx;
  uint32 lineNumber;
};

// ---------- FEIRStmtPesudoJavaTry ----------
class FEIRStmtPesudoJavaTry : public FEIRStmt {
 public:
  FEIRStmtPesudoJavaTry();
  ~FEIRStmtPesudoJavaTry() = default;
  void AddCatchLabelIdx(uint32 labelIdx) {
    catchLabelIdxVec.push_back(labelIdx);
  }

  const std::vector<uint32> GetCatchLabelIdxVec() const {
    return catchLabelIdxVec;
  }

  void AddCatchTarget(FEIRStmtPesudoLabel &stmtLabel) {
    catchTargets.push_back(&stmtLabel);
  }

  const std::vector<FEIRStmtPesudoLabel*> &GetCatchTargets() const {
    return catchTargets;
  }

 protected:
  std::string DumpDotStringImpl() const override;
  std::list<StmtNode*> GenMIRStmtsImpl(MIRBuilder &mirBuilder) const override;

 private:
  std::vector<uint32> catchLabelIdxVec;
  std::vector<FEIRStmtPesudoLabel*> catchTargets;
};

// ---------- FEIRStmtPesudoJavaTry2 ----------
class FEIRStmtPesudoJavaTry2 : public FEIRStmt {
 public:
  FEIRStmtPesudoJavaTry2(uint32 outerIdxIn);
  ~FEIRStmtPesudoJavaTry2() = default;
  void AddCatchLabelIdx(uint32 labelIdx) {
    catchLabelIdxVec.push_back(labelIdx);
  }

  const std::vector<uint32> GetCatchLabelIdxVec() const {
    return catchLabelIdxVec;
  }

  void AddCatchTarget(FEIRStmtPesudoLabel2 *stmtLabel) {
    catchTargets.push_back(stmtLabel);
  }

  const std::vector<FEIRStmtPesudoLabel2*> &GetCatchTargets() const {
    return catchTargets;
  }

 protected:
  std::string DumpDotStringImpl() const override;
  std::list<StmtNode*> GenMIRStmtsImpl(MIRBuilder &mirBuilder) const override;

 private:
  uint32 outerIdx;
  std::vector<uint32> catchLabelIdxVec;
  std::vector<FEIRStmtPesudoLabel2*> catchTargets;
};

// ---------- FEIRStmtPesudoEndTry ----------
class FEIRStmtPesudoEndTry : public FEIRStmt {
 public:
  FEIRStmtPesudoEndTry();
  ~FEIRStmtPesudoEndTry() = default;

 protected:
  std::string DumpDotStringImpl() const override;
  std::list<StmtNode*> GenMIRStmtsImpl(MIRBuilder &mirBuilder) const override;
};

// ---------- FEIRStmtPesudoCatch ----------
class FEIRStmtPesudoCatch : public FEIRStmtPesudoLabel {
 public:
  explicit FEIRStmtPesudoCatch(uint32 argLabelIdx);
  ~FEIRStmtPesudoCatch() = default;
  void AddCatchTypeNameIdx(GStrIdx typeNameIdx);

 protected:
  std::string DumpDotStringImpl() const override;
  std::list<StmtNode*> GenMIRStmtsImpl(MIRBuilder &mirBuilder) const override;

 private:
  std::list<UniqueFEIRType> catchTypes;
};

// ---------- FEIRStmtPesudoCatch2 ----------
class FEIRStmtPesudoCatch2 : public FEIRStmtPesudoLabel2 {
 public:
  explicit FEIRStmtPesudoCatch2(uint32 qIdx0, uint32 qIdx1);
  ~FEIRStmtPesudoCatch2() = default;
  void AddCatchTypeNameIdx(GStrIdx typeNameIdx);

 protected:
  std::string DumpDotStringImpl() const override;
  std::list<StmtNode*> GenMIRStmtsImpl(MIRBuilder &mirBuilder) const override;

 private:
  std::list<UniqueFEIRType> catchTypes;
};

// ---------- FEIRStmtPesudoComment ----------
class FEIRStmtPesudoComment : public FEIRStmt {
 public:
  explicit FEIRStmtPesudoComment(FEIRNodeKind argKind = kStmtPesudoComment);
  explicit FEIRStmtPesudoComment(const std::string &argContent);
  ~FEIRStmtPesudoComment() = default;
  void SetContent(const std::string &argContent) {
    content = argContent;
  }

 protected:
  std::string DumpDotStringImpl() const override;
  std::list<StmtNode*> GenMIRStmtsImpl(MIRBuilder &mirBuilder) const override;

  std::string content = "";
};

// ---------- FEIRStmtPesudoCommentForInst ----------
class FEIRStmtPesudoCommentForInst : public FEIRStmtPesudoComment {
 public:
  FEIRStmtPesudoCommentForInst();
  ~FEIRStmtPesudoCommentForInst() = default;
  void SetFileIdx(uint32 argFileIdx) {
    fileIdx = argFileIdx;
  }

  void SetLineNum(uint32 argLineNum) {
    lineNum = argLineNum;
  }

  void SetPC(uint32 argPC) {
    pc = argPC;
  }

 protected:
  std::string DumpDotStringImpl() const override;
  std::list<StmtNode*> GenMIRStmtsImpl(MIRBuilder &mirBuilder) const override;

 private:
  constexpr static uint32 invalid = 0xFFFFFFFF;
  uint32 fileIdx = invalid;
  uint32 lineNum = invalid;
  uint32 pc = invalid;
};

// ---------- FEIRStmtIf ----------
class FEIRStmtIf : public FEIRStmt {
 public:
  FEIRStmtIf(UniqueFEIRExpr argCondExpr, std::list<UniqueFEIRStmt> &argThenStmts);
  FEIRStmtIf(UniqueFEIRExpr argCondExpr,
             std::list<UniqueFEIRStmt> &argThenStmts,
             std::list<UniqueFEIRStmt> &argElseStmts);
  ~FEIRStmtIf() = default;

  void SetCondExpr(UniqueFEIRExpr argCondExpr) {
    CHECK_NULL_FATAL(argCondExpr);
    condExpr = std::move(argCondExpr);
  }

  void SetHasElse(bool argHasElse) {
    hasElse = argHasElse;
  }

  void SetThenStmts(std::list<UniqueFEIRStmt> &stmts) {
    std::move(begin(stmts), end(stmts), std::inserter(thenStmts, end(thenStmts)));
  }

  void SetElseStmts(std::list<UniqueFEIRStmt> &stmts) {
    std::move(begin(stmts), end(stmts), std::inserter(elseStmts, end(elseStmts)));
  }

 protected:
  std::string DumpDotStringImpl() const override;
  std::list<StmtNode*> GenMIRStmtsImpl(MIRBuilder &mirBuilder) const override;

  bool IsFallThroughImpl() const override {
    return true;
  }

  bool IsBranchImpl() const override {
    return true;
  }

 private:
  UniqueFEIRExpr condExpr;
  bool hasElse = false;
  std::list<UniqueFEIRStmt> thenStmts;
  std::list<UniqueFEIRStmt> elseStmts;
};

class FEIRStmtDoWhile : public FEIRStmt {
 public:
  FEIRStmtDoWhile(Opcode argOpcode, UniqueFEIRExpr argCondExpr, std::list<UniqueFEIRStmt> argBodyStmts)
      : FEIRStmt(FEIRNodeKind::kStmtDoWhile),
        opcode(argOpcode),
        condExpr(std::move(argCondExpr)),
        bodyStmts(std::move(argBodyStmts)) {}
  ~FEIRStmtDoWhile() = default;

 protected:
  bool IsBranchImpl() const override {
    return true;
  }

  std::list<StmtNode*> GenMIRStmtsImpl(MIRBuilder &mirBuilder) const override;

 private:
  Opcode opcode;
  UniqueFEIRExpr condExpr;
  std::list<UniqueFEIRStmt> bodyStmts;
};

class FEIRStmtBreak : public FEIRStmt {
 public:
  FEIRStmtBreak(): FEIRStmt(FEIRNodeKind::kStmtBreak) {}
  ~FEIRStmtBreak() = default;

  void SetLabelName(std::string name){
    labelName = std::move(name);
  }

 protected:
  bool IsBranchImpl() const override {
    return true;
  }

  std::list<StmtNode*> GenMIRStmtsImpl(MIRBuilder &mirBuilder) const override;
 private:
  std::string labelName;
};

class FEIRStmtContinue : public FEIRStmt {
 public:
  FEIRStmtContinue(): FEIRStmt(FEIRNodeKind::kStmtContinue) {}
  ~FEIRStmtContinue() = default;

  void SetLabelName(std::string name){
    labelName = std::move(name);
  }

 protected:
  bool IsBranchImpl() const override {
    return true;
  }

  std::list<StmtNode*> GenMIRStmtsImpl(MIRBuilder &mirBuilder) const override;
 private:
  std::string labelName;
};
}  // namespace maple
#endif  // MPLFE_INCLUDE_COMMON_FEIR_STMT_H
