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
#ifndef HIR2MPL_INCLUDE_COMMON_FEIR_STMT_H
#define HIR2MPL_INCLUDE_COMMON_FEIR_STMT_H
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
#include "feir_type_helper.h"

namespace maple {
class FEIRBuilder;

enum FEIRNodeKind : uint8 {
#define FEIR_NODE_KIND(kind, description) \
  k##kind,
#include "feir_node_kind.def"
#undef FEIR_NODE_KIND
};

std::string GetFEIRNodeKindDescription(FEIRNodeKind kindArg);
constexpr uint32 kOpHashShift = 24;
constexpr uint32 kTypeHashShift = 8;
constexpr uint32 kOtherShift = 16;
constexpr uint32 kRandomNum = 0x9e3779b9;

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
    SetMIRStmtSrcPos(stmts);
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

  void SetHexPC(uint32 argHexPC) {
    return SetHexPCImpl(argHexPC);
  }

  uint32 GetHexPC(void) const {
    return GetHexPCImpl();
  }

  bool IsStmtInstComment() const;
  bool ShouldHaveLOC() const;
  BaseNode *ReplaceAddrOfNode(BaseNode *node) const;
  void SetSrcFileInfo(uint32 srcFileIdxIn, uint32 srcFileLineNumIn) {
    srcFileIndex = srcFileIdxIn;
    srcFileLineNum = srcFileLineNumIn;
  }

  uint32 GetSrcFileIdx() const {
    return srcFileIndex;
  }

  uint32 GetSrcFileLineNum() const {
    return srcFileLineNum;
  }

  bool HasSetLOCInfo() const {
    return (srcFileLineNum != 0 || srcFileIndex != 0);
  }

  bool IsDummy() const {
    return isDummy;
  }

  void SetDummy() {
    isDummy = true;
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

  void SetHexPCImpl(uint32 argHexPC) {
    hexPC = argHexPC;
  }

  uint32 GetHexPCImpl(void) const {
    return hexPC;
  }

  void SetMIRStmtSrcPos(std::list<StmtNode*> &stmts) const {
    if (FEOptions::GetInstance().IsDumpLOC() && !stmts.empty()) {
      (*stmts.begin())->GetSrcPos().SetFileNum(srcFileIndex);
      (*stmts.begin())->GetSrcPos().SetLineNum(srcFileLineNum);
    }
  }

  FEIRNodeKind kind;
  uint32 srcFileIndex = 0;
  uint32 srcFileLineNum = 0;
  uint32 hexPC = UINT32_MAX;
  bool isDummy = false;
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

  virtual bool operator==(const FEIRExpr &expr) const {
    return false;
  }

  virtual bool operator!=(const FEIRExpr &expr) const {
    return true;
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

  void SetAddrof(bool flag) {
    isAddrof = flag;
  }

  bool HasException() const {
    return HasExceptionImpl();
  }

  void SetType(std::unique_ptr<FEIRType> argType) {
    SetTypeImpl(std::move(argType));
  }

  FEIRNodeKind GetKind() const {
    return kind;
  }

  void SetKind(FEIRNodeKind specKind) {
    kind = specKind;
  }

  FEIRType *GetType() const {
    return GetTypeImpl();
  }

  const FEIRType &GetTypeRef() const {
    return GetTypeRefImpl();
  }

  PrimType GetPrimType() const {
    return GetPrimTypeImpl();
  }

  FieldID GetFieldID() const {
    return GetFieldIDImpl();
  }

  void SetFieldID(FieldID fieldID) {
    return SetFieldIDImpl(fieldID);
  }

  void SetFieldType(std::unique_ptr<FEIRType> fieldType) {
    return SetFieldTypeImpl(std::move(fieldType));
  }

  void RegisterDFGNodes2CheckPoint(FEIRStmtCheckPoint &checkPoint) {
    RegisterDFGNodes2CheckPointImpl(checkPoint);
  }

  bool CalculateDefs4AllUses(FEIRStmtCheckPoint &checkPoint, FEIRUseDefChain &udChain) {
    return CalculateDefs4AllUsesImpl(checkPoint, udChain);
  }

  uint32 Hash() const {
    return HashImpl();
  }

 protected:
  virtual std::unique_ptr<FEIRExpr> CloneImpl() const = 0;
  virtual BaseNode *GenMIRNodeImpl(MIRBuilder &mirBuilder) const = 0;
  virtual std::vector<FEIRVar*> GetVarUsesImpl() const;
  virtual void RegisterDFGNodes2CheckPointImpl(FEIRStmtCheckPoint &checkPoint) {}
  virtual bool CalculateDefs4AllUsesImpl(FEIRStmtCheckPoint &checkPoint, FEIRUseDefChain &udChain) {
    return true;
  }

  virtual void SetTypeImpl(std::unique_ptr<FEIRType> argType) {
    ASSERT_NOT_NULL(argType);
    type = std::move(argType);
  }

  virtual PrimType GetPrimTypeImpl() const {
    return type->GetPrimType();
  }

  virtual  FEIRType *GetTypeImpl() const {
    ASSERT(type != nullptr, "type is nullptr");
    return type.get();
  }

  virtual const FEIRType &GetTypeRefImpl() const {
    ASSERT(GetTypeImpl() != nullptr, "type is nullptr");
    return *GetTypeImpl();
  }

  virtual FieldID GetFieldIDImpl() const {
    CHECK_FATAL(false, "unsupported in base class");
  }

  virtual void SetFieldIDImpl(FieldID fieldID) {
    CHECK_FATAL(false, "unsupported in base class");
  }

  virtual void SetFieldTypeImpl(std::unique_ptr<FEIRType> fieldType) {
    CHECK_FATAL(false, "unsupported in base class");
  }

  virtual uint32 HashImpl() const {
    CHECK_FATAL(false, "unsupported in base class");
    return 0;
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
  explicit FEIRExprConst(uint32 val);
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
  uint32 HashImpl() const override;

 private:
  void CheckRawValue2SetZero();
  ConstExprValue value;
};

// ---------- FEIRExprSizeOfType ----------
class FEIRExprSizeOfType : public FEIRExpr {
 public:
  explicit FEIRExprSizeOfType(UniqueFEIRType ty);
  ~FEIRExprSizeOfType() = default;

 protected:
  std::unique_ptr<FEIRExpr> CloneImpl() const override;
  BaseNode *GenMIRNodeImpl(MIRBuilder &mirBuilder) const override;

  uint32 HashImpl() const override {
    return (static_cast<uint32>(kind) << kOpHashShift) + (type->Hash() << kTypeHashShift) + feirType->Hash();
  }

 private:
  UniqueFEIRType feirType;
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

  bool operator==(const FEIRExpr &expr) const override {
    return expr.GetKind() == kExprDRead && *varSrc == *(expr.GetVarUses().front()) && fieldID == expr.GetFieldID();
  }

  bool operator!=(const FEIRExpr &expr) const override {
    return expr.GetKind() != kExprDRead || *varSrc != *(expr.GetVarUses().front()) || fieldID != expr.GetFieldID();
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

  const UniqueFEIRVar &GetVar() const {
    return varSrc;
  }

  std::unique_ptr<FEIRType> GetFieldType() const {
    return fieldType->Clone();
  }

 protected:
  std::unique_ptr<FEIRExpr> CloneImpl() const override;
  void RegisterDFGNodes2CheckPointImpl(FEIRStmtCheckPoint &checkPoint) override;
  bool CalculateDefs4AllUsesImpl(FEIRStmtCheckPoint &checkPoint, FEIRUseDefChain &udChain) override;
  BaseNode *GenMIRNodeImpl(MIRBuilder &mirBuilder) const override;
  std::vector<FEIRVar*> GetVarUsesImpl() const override;
  PrimType GetPrimTypeImpl() const override;
  void SetTypeImpl(std::unique_ptr<FEIRType> argType) override;
  FEIRType *GetTypeImpl() const override;
  const FEIRType &GetTypeRefImpl() const override;

  FieldID GetFieldIDImpl() const override {
    return fieldID;
  }

  void SetFieldTypeImpl(std::unique_ptr<FEIRType> type) override {
    fieldType = std::move(type);
  }

  void SetFieldIDImpl(FieldID argFieldID) override {
    fieldID = argFieldID;
  }

  uint32 HashImpl() const override {
    return (static_cast<uint32>(kind) << kOpHashShift) + (type->Hash() << kTypeHashShift) +
           (static_cast<uint32>(fieldID) << kOtherShift) + varSrc->Hash();
  }

 private:
  std::unique_ptr<FEIRVar> varSrc;
  FieldID fieldID = 0;
  std::unique_ptr<FEIRType> fieldType;
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

// ---------- FEIRExprAddrofConstArray ----------
class FEIRExprAddrofConstArray : public FEIRExpr {
 public:
  FEIRExprAddrofConstArray(const std::vector<uint32> &arrayIn, MIRType *typeIn);
  FEIRExprAddrofConstArray(const std::string &arrayNameIn, const std::vector<uint32> &arrayIn, MIRType *typeIn)
      : FEIRExpr(FEIRNodeKind::kExprAddrof,
                 FEIRTypeHelper::CreateTypeNative(*GlobalTables::GetTypeTable().GetPtrType())),
        arrayName(arrayNameIn),
        elemType(typeIn) {
    std::copy(arrayIn.begin(), arrayIn.end(), std::back_inserter(array));
  }

  ~FEIRExprAddrofConstArray() = default;

  uint32 GetStringLiteralSize() const {
    return array.size();
  }

  const MIRType *GetElemType() const {
    return elemType;
  }

 protected:
  std::unique_ptr<FEIRExpr> CloneImpl() const override;
  BaseNode *GenMIRNodeImpl(MIRBuilder &mirBuilder) const override;

  uint32 HashImpl() const override {
    uint32 hash = (static_cast<uint32>(kind) << kOpHashShift);
    for (uint32 elem : array) {
      hash += static_cast<uint32>(std::hash<uint32>{}(elem));
    }
    std::size_t seed = array.size();
    for (uint32 elem : array) {
      seed ^= elem + kRandomNum + (seed << 6) + (seed >> 2);
    }
    hash += static_cast<uint32>(seed);
    if (elemType != nullptr) {
      hash += static_cast<uint32>(elemType->GetHashIndex());
    }
    return hash;
  }

 private:
  std::string arrayName;
  std::vector<uint32> array;
  MIRType *elemType = nullptr;
};

// ---------- FEIRExprAddrOfLabel ----------
class FEIRExprAddrOfLabel : public FEIRExpr {
 public:
  FEIRExprAddrOfLabel(const std::string &lbName, UniqueFEIRType exprType)
      : FEIRExpr(FEIRNodeKind::kExprAddrofLabel, std::move(exprType)), labelName(lbName) {}
  ~FEIRExprAddrOfLabel() = default;

 protected:
  std::unique_ptr<FEIRExpr> CloneImpl() const override;
  BaseNode *GenMIRNodeImpl(MIRBuilder &mirBuilder) const override;

  uint32 HashImpl() const override {
    return (static_cast<uint32>(kind) << kOpHashShift) + (type->Hash() << kTypeHashShift) +
            static_cast<uint32>(std::hash<std::string>{}(labelName));
  }

 private:
  std::string labelName;
};

// ---------- FEIRExprAddrofVar ----------
class FEIRExprAddrofVar : public FEIRExpr {
 public:
  explicit FEIRExprAddrofVar(std::unique_ptr<FEIRVar> argVarSrc)
      : FEIRExpr(FEIRNodeKind::kExprAddrofVar,
                 FEIRTypeHelper::CreateTypeNative(*GlobalTables::GetTypeTable().GetPtrType())),
        varSrc(std::move(argVarSrc)) {}

  FEIRExprAddrofVar(std::unique_ptr<FEIRVar> argVarSrc, FieldID id)
      : FEIRExpr(FEIRNodeKind::kExprAddrofVar,
                 FEIRTypeHelper::CreateTypeNative(*GlobalTables::GetTypeTable().GetPtrType())),
        varSrc(std::move(argVarSrc)), fieldID(id) {}
  ~FEIRExprAddrofVar() = default;

  void SetVarValue(MIRConst *val) {
    cst = val;
  }

  MIRConst *GetVarValue() const {
    return cst;
  }

 protected:
  std::unique_ptr<FEIRExpr> CloneImpl() const override;
  BaseNode *GenMIRNodeImpl(MIRBuilder &mirBuilder) const override;
  std::vector<FEIRVar*> GetVarUsesImpl() const override;

  FieldID GetFieldIDImpl() const override {
    return fieldID;
  }

  void SetFieldIDImpl(FieldID id) override {
    fieldID = id;
  }

  uint32 HashImpl() const override {
    return (static_cast<uint32>(kind) << kOpHashShift) + (type->Hash() << kTypeHashShift) +
           (static_cast<uint32>(fieldID) << kOtherShift) + varSrc->Hash();
  }

 private:
  std::unique_ptr<FEIRVar> varSrc;
  FieldID fieldID = 0;
  MIRConst *cst = nullptr;
};

// ---------- FEIRExprIAddrof ----------
class FEIRExprIAddrof : public FEIRExpr {
 public:
  FEIRExprIAddrof(UniqueFEIRType pointeeType, FieldID id, UniqueFEIRExpr expr)
      : FEIRExpr(FEIRNodeKind::kExprIAddrof,
                 FEIRTypeHelper::CreateTypeNative(*GlobalTables::GetTypeTable().GetPtrType())),
        ptrType(std::move(pointeeType)),
        fieldID(id),
        subExpr(std::move(expr)) {}
  ~FEIRExprIAddrof() = default;

  UniqueFEIRType GetClonedRetType() const {
    return type->Clone();
  }

  UniqueFEIRExpr GetClonedOpnd() const {
    return subExpr->Clone();
  }

  UniqueFEIRType GetClonedPtrType() const {
    return ptrType->Clone();
  }

 protected:
  std::unique_ptr<FEIRExpr> CloneImpl() const override;
  std::vector<FEIRVar*> GetVarUsesImpl() const override;
  BaseNode *GenMIRNodeImpl(MIRBuilder &mirBuilder) const override;

  FieldID GetFieldIDImpl() const override {
    return fieldID;
  }

  void SetFieldIDImpl(FieldID argFieldID) override {
    fieldID = argFieldID;
  }

  uint32 HashImpl() const override {
    return (static_cast<uint32>(kind) << kOpHashShift) + (type->Hash() << kTypeHashShift) +
           (static_cast<uint32>(fieldID) << kOtherShift) + ptrType->Hash() + subExpr->Hash();
  }

 private:
  UniqueFEIRType ptrType;
  FieldID fieldID = 0;
  UniqueFEIRExpr subExpr;
};

// ---------- FEIRExprAddrofFunc ----------
class FEIRExprAddrofFunc : public FEIRExpr {
 public:
  explicit FEIRExprAddrofFunc(const std::string &addr)
      : FEIRExpr(FEIRNodeKind::kExprAddrofFunc,
                 FEIRTypeHelper::CreateTypeNative(*GlobalTables::GetTypeTable().GetPtrType())),
        funcAddr(addr) {}
  ~FEIRExprAddrofFunc() = default;

  const std::string &GetFuncAddr() const {
    return funcAddr;
  }

 protected:
  std::unique_ptr<FEIRExpr> CloneImpl() const override;
  BaseNode *GenMIRNodeImpl(MIRBuilder &mirBuilder) const override;

  uint32 HashImpl() const override {
    return (static_cast<uint32>(kind) << kOpHashShift) + (type->Hash() << kTypeHashShift) +
            static_cast<uint32>(std::hash<std::string>{}(funcAddr));
  }

 private:
  std::string funcAddr;
};

// ---------- FEIRExprAddrofArray ----------
class FEIRExprAddrofArray : public FEIRExpr {
 public:
  FEIRExprAddrofArray(UniqueFEIRType argTypeNativeArray, UniqueFEIRExpr argExprArray, std::string argArrayName,
                      std::list<UniqueFEIRExpr> &argExprIndexs);
  ~FEIRExprAddrofArray() = default;

  void SetIndexsExprs(std::list<UniqueFEIRExpr> &exprs) {
    exprIndexs.clear();
    for (auto &e : exprs) {
      auto ue = e->Clone();
      exprIndexs.push_back(std::move(ue));
    }
  }

  const UniqueFEIRType &GetTypeArray() const {
    return typeNativeArray;
  }

  const std::list<UniqueFEIRExpr> &GetExprIndexs() const {
    return exprIndexs;
  }

  const UniqueFEIRExpr &GetExprArray() const {
    return exprArray;
  }

 protected:
  std::unique_ptr<FEIRExpr> CloneImpl() const override;
  std::vector<FEIRVar*> GetVarUsesImpl() const override;
  BaseNode *GenMIRNodeImpl(MIRBuilder &mirBuilder) const override;

  uint32 HashImpl() const override {
    uint32 hash = (static_cast<uint32>(kind) << kOpHashShift) + (type->Hash() << kTypeHashShift) +
                  (static_cast<uint32>(std::hash<std::string>{}(arrayName)) << kOtherShift);
    if (typeNativeArray != nullptr) {
      hash += typeNativeArray->Hash();
    }
    if (exprArray != nullptr) {
      hash += exprArray->Hash();
    }
    std::size_t seed = exprIndexs.size();
    for (auto &idx : exprIndexs) {
      if (idx != nullptr) {
        seed ^= idx->Hash() + kRandomNum + (seed << 6) + (seed >> 2);
      }
    }
    return hash + static_cast<uint32>(seed);
  }

 private:
  mutable std::list<UniqueFEIRExpr> exprIndexs;
  UniqueFEIRType typeNativeArray = nullptr;
  UniqueFEIRExpr exprArray = nullptr;
  std::string arrayName;
};

// ---------- FEIRExprUnary ----------
class FEIRExprUnary : public FEIRExpr {
 public:
  FEIRExprUnary(Opcode argOp, std::unique_ptr<FEIRExpr> argOpnd);
  FEIRExprUnary(std::unique_ptr<FEIRType> argType, Opcode argOp, std::unique_ptr<FEIRExpr> argOpnd);
  ~FEIRExprUnary() = default;
  void SetOpnd(std::unique_ptr<FEIRExpr> argOpnd);
  const UniqueFEIRExpr &GetOpnd() const;
  static std::map<Opcode, bool> InitMapOpNestableForExprUnary();

  Opcode GetOp() const {
    return op;
  }

 protected:
  virtual std::unique_ptr<FEIRExpr> CloneImpl() const override;
  void RegisterDFGNodes2CheckPointImpl(FEIRStmtCheckPoint &checkPoint) override;
  bool CalculateDefs4AllUsesImpl(FEIRStmtCheckPoint &checkPoint, FEIRUseDefChain &udChain) override;
  virtual BaseNode *GenMIRNodeImpl(MIRBuilder &mirBuilder) const override;
  std::vector<FEIRVar*> GetVarUsesImpl() const override;

  uint32 HashImpl() const override {
    return (static_cast<uint32>(kind) << kOpHashShift) + (type->Hash() << kTypeHashShift) +
           (static_cast<uint32>(op) << kOtherShift) + opnd->Hash();
  }

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

  void SetSrcPrimType(PrimType pty) {
    srcPrimType = pty;
  }

 protected:
  std::unique_ptr<FEIRExpr> CloneImpl() const override;
  void RegisterDFGNodes2CheckPointImpl(FEIRStmtCheckPoint &checkPoint) override;
  bool CalculateDefs4AllUsesImpl(FEIRStmtCheckPoint &checkPoint, FEIRUseDefChain &udChain) override;
  BaseNode *GenMIRNodeImpl(MIRBuilder &mirBuilder) const override;

  uint32 HashImpl() const override {
    return FEIRExprUnary::HashImpl() + static_cast<uint32>(srcPrimType);
  }

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
  PrimType srcPrimType = PTY_unknown;
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

  uint32 HashImpl() const override {
    return FEIRExprUnary::HashImpl() + (static_cast<uint32>(bitOffset) << 16) + static_cast<uint32>(bitSize);
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
      : FEIRExpr(FEIRNodeKind::kExprIRead, std::move(returnType)),
        ptrType(std::move(pointeeType)),
        fieldID(id),
        subExpr(std::move(expr)) {}
  ~FEIRExprIRead() override = default;

  UniqueFEIRType GetClonedRetType() const {
    return type->Clone();
  }

  UniqueFEIRExpr GetClonedOpnd() const {
    return subExpr->Clone();
  }

  UniqueFEIRType GetClonedPtrType() const {
    return ptrType->Clone();
  }

 protected:
  std::unique_ptr<FEIRExpr> CloneImpl() const override;
  std::vector<FEIRVar*> GetVarUsesImpl() const override;
  BaseNode *GenMIRNodeImpl(MIRBuilder &mirBuilder) const override;

  FieldID GetFieldIDImpl() const override {
    return fieldID;
  }

  void SetFieldIDImpl(FieldID argFieldID) override {
    fieldID = argFieldID;
  }

  void SetFieldTypeImpl(UniqueFEIRType argFieldType) override {
    type = std::move(argFieldType);
  }

  uint32 HashImpl() const override {
    return (static_cast<uint32>(kind) << kOpHashShift) + (type->Hash() << kTypeHashShift) +
           (static_cast<uint32>(fieldID) << kOtherShift) + ptrType->Hash() + subExpr->Hash();
  }

 private:
  UniqueFEIRType ptrType = nullptr;
  FieldID fieldID = 0;
  UniqueFEIRExpr subExpr = nullptr;
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
  const std::unique_ptr<FEIRExpr> &GetOpnd0() const;
  const std::unique_ptr<FEIRExpr> &GetOpnd1() const;
  bool IsComparative() const;

  Opcode GetOp() const {
    return op;
  }

 protected:
  std::unique_ptr<FEIRExpr> CloneImpl() const override;
  void RegisterDFGNodes2CheckPointImpl(FEIRStmtCheckPoint &checkPoint) override;
  bool CalculateDefs4AllUsesImpl(FEIRStmtCheckPoint &checkPoint, FEIRUseDefChain &udChain) override;
  BaseNode *GenMIRNodeImpl(MIRBuilder &mirBuilder) const override;
  std::vector<FEIRVar*> GetVarUsesImpl() const override;
  bool IsNestableImpl() const override;
  bool IsAddrofImpl() const override;

  uint32 HashImpl() const override {
    return (static_cast<uint32>(kind) << kOpHashShift) + (type->Hash() << kTypeHashShift) +
           (static_cast<uint32>(op) << kOtherShift) + (opnd0->Hash() << 8) + opnd1->Hash();
  }

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
  FEIRExprTernary(Opcode argOp, std::unique_ptr<FEIRType> argType, std::unique_ptr<FEIRExpr> argOpnd0,
                  std::unique_ptr<FEIRExpr> argOpnd1, std::unique_ptr<FEIRExpr> argOpnd2);
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

  uint32 HashImpl() const override {
    return (static_cast<uint32>(kind) << kOpHashShift) + (type->Hash() << kTypeHashShift) +
           (static_cast<uint32>(op) << kOtherShift) + (opnd0->Hash() << 8) + (opnd1->Hash() << 16) + opnd1->Hash();
  }

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
  virtual ~FEIRExprNary() = default;
  void AddOpnd(std::unique_ptr<FEIRExpr> argOpnd);
  void AddOpnds(const std::vector<std::unique_ptr<FEIRExpr>> &argOpnds);
  void ResetOpnd();

 protected:
  void RegisterDFGNodes2CheckPointImpl(FEIRStmtCheckPoint &checkPoint) override;
  bool CalculateDefs4AllUsesImpl(FEIRStmtCheckPoint &checkPoint, FEIRUseDefChain &udChain) override;
  std::vector<FEIRVar*> GetVarUsesImpl() const override;

  uint32 HashImpl() const override {
    uint32 hash = (static_cast<uint32>(kind) << kOpHashShift) + (type->Hash() << kTypeHashShift) +
                  (static_cast<uint32>(op) << kOtherShift);
    std::size_t seed = opnds.size();
    for (auto &opnd : opnds) {
      seed ^= opnd->Hash() + kRandomNum + (seed << 6) + (seed >> 2);
    }
    hash += static_cast<uint32>(seed);
    return hash;
  }

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

  uint32 HashImpl() const override {
    uint32 hash = FEIRExprNary::HashImpl() + (static_cast<uint32>(intrinsicID) << 16) + typeID;
    if (paramType != nullptr) {
      hash += paramType->Hash();
    }
    return hash;
  }

 private:
  MIRIntrinsicID intrinsicID;
  std::unique_ptr<FEIRType> paramType;
  uint32 typeID = UINT32_MAX;
};  // class FEIRExprIntrinsicop

class FEIRExprIntrinsicopForC : public FEIRExprNary {
 public:
  FEIRExprIntrinsicopForC(std::unique_ptr<FEIRType> exprType, MIRIntrinsicID argIntrinsicID,
                          const std::vector<std::unique_ptr<FEIRExpr>> &argOpnds);
  ~FEIRExprIntrinsicopForC() = default;

 protected:
  std::unique_ptr<FEIRExpr> CloneImpl() const override;
  BaseNode *GenMIRNodeImpl(MIRBuilder &mirBuilder) const override;

  uint32 HashImpl() const override {
    return FEIRExprNary::HashImpl() + static_cast<uint32>(intrinsicID);
  }

 private:
  MIRIntrinsicID intrinsicID;
}; // class FEIRExprIntrinsicopForC

class FEIRExprJavaMerge : public FEIRExprNary {
 public:
  FEIRExprJavaMerge(std::unique_ptr<FEIRType> mergedTypeArg, const std::vector<std::unique_ptr<FEIRExpr>> &argOpnds);
  ~FEIRExprJavaMerge() = default;

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

  void SetRefName(const std::string &name) {
    refName = name;
  }

  const UniqueFEIRExpr &GetSubExpr() const {
    return subExpr;
  }

  MIRType *GetMIRType() const {
    CHECK_NULL_FATAL(destType);
    return destType;
  }

 protected:
  std::unique_ptr<FEIRExpr> CloneImpl() const override;
  std::vector<FEIRVar*> GetVarUsesImpl() const override;
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
  FEIRExprAtomic(MIRType *ty, MIRType *ref, UniqueFEIRExpr obj, ASTAtomicOp atomOp);
  ~FEIRExprAtomic() = default;

  void SetVal1Type(MIRType *ty) {
    val1Type = ty;
  }

  void SetVal1Expr(UniqueFEIRExpr expr) {
    valExpr1 = std::move(expr);
  }

  void SetVal2Type(MIRType *ty) {
    val2Type = ty;
  }

  void SetVal2Expr(UniqueFEIRExpr expr) {
    valExpr2 = std::move(expr);
  }

  void SetValVar(UniqueFEIRVar value) {
    val = std::move(value);
  }

  void SetLockVar(UniqueFEIRVar value) {
    lock = std::move(value);
  }

 protected:
  std::unique_ptr<FEIRExpr> CloneImpl() const override;
  BaseNode *GenMIRNodeImpl(MIRBuilder &mirBuilder) const override;

 private:
  void ProcessAtomicBinary(MIRBuilder &mirBuilder, BlockNode &block, BaseNode &lockNode,
                           MIRSymbol &valueVar) const;
  void ProcessAtomicLoad(MIRBuilder &mirBuilder, BlockNode &block, BaseNode &lockNode,
                         const MIRSymbol &valueVar) const;
  void ProcessAtomicStore(MIRBuilder &mirBuilder, BlockNode &block, BaseNode &lockNode) const;
  void ProcessAtomicExchange(MIRBuilder &mirBuilder, BlockNode &block, BaseNode &lockNode,
                             const MIRSymbol &valueVar) const;
  void ProcessAtomicCompareExchange(MIRBuilder &mirBuilder, BlockNode &block, BaseNode &lockNode,
                                    const MIRSymbol *valueVar) const;
  MIRType *mirType = nullptr;
  MIRType *refType = nullptr;
  MIRType *val1Type = nullptr;
  MIRType *val2Type = nullptr;
  UniqueFEIRExpr objExpr;
  UniqueFEIRExpr valExpr1;
  UniqueFEIRExpr valExpr2;
  ASTAtomicOp atomicOp;
  UniqueFEIRVar lock;
  UniqueFEIRVar val;
};

// ---------- FEIRStmtNary ----------
class FEIRStmtNary : public FEIRStmt {
 public:
  FEIRStmtNary(Opcode opIn, std::list<std::unique_ptr<FEIRExpr>> argExprsIn);
  ~FEIRStmtNary() = default;

  Opcode GetOP() const {
    return op;
  }

  const std::list<std::unique_ptr<FEIRExpr>> &GetArgExprs() const {
    return argExprs;
  }

 protected:
  std::list<StmtNode*> GenMIRStmtsImpl(MIRBuilder &mirBuilder) const override;
  MapleVector<BaseNode*> ReplaceBoundaryChecking(MIRBuilder &mirBuilder, size_t paramIdx = SIZE_MAX) const;

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

  void AddExprArg(UniqueFEIRExpr exprArg) {
    exprArgs.push_back(std::move(exprArg));
  }

  void AddExprArgReverse(UniqueFEIRExpr exprArg) {
    exprArgs.push_front(std::move(exprArg));
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
  std::unique_ptr<FEIRVar> var = nullptr;
  std::list<UniqueFEIRExpr> exprArgs;
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

 protected:
  std::string DumpDotStringImpl() const override;
  void RegisterDFGNodes2CheckPointImpl(FEIRStmtCheckPoint &checkPoint) override;
  bool CalculateDefs4AllUsesImpl(FEIRStmtCheckPoint &checkPoint, FEIRUseDefChain &udChain) override;
  void InitTrans4AllVarsImpl() override;
  std::list<StmtNode*> GenMIRStmtsImpl(MIRBuilder &mirBuilder) const override;

 private:
  void InsertNonnullChecking(MIRBuilder &mirBuilder, const MIRSymbol &dstSym, std::list<StmtNode*> &ans) const;
  void AssignBoundaryVarAndChecking(MIRBuilder &mirBuilder, std::list<StmtNode*> &ans) const;
  void CheckNonnullArgsAndRetForFuncPtr(MIRBuilder &mirBuilder) const;
  void CheckBoundaryArgsAndRetForFuncPtr(MIRBuilder &mirBuilder) const;

  std::unique_ptr<FEIRExpr> expr;
  FieldID fieldID;
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

 protected:
  std::list<StmtNode *> GenMIRStmtsImpl(MIRBuilder &mirBuilder) const override;

 private:
  void InsertNonnullChecking(MIRBuilder &mirBuilder, MIRType &baseType, std::list<StmtNode*> &ans) const;
  void CheckNonnullArgsAndRetForFuncPtr(MIRBuilder &mirBuilder, const MIRType &baseType) const;
  void CheckBoundaryArgsAndRetForFuncPtr(MIRBuilder &mirBuilder, const MIRType &baseType) const;
  void AssignBoundaryVarAndChecking(MIRBuilder &mirBuilder, std::list<StmtNode*> &ans) const;

  UniqueFEIRType addrType;
  UniqueFEIRExpr addrExpr;
  UniqueFEIRExpr baseExpr;
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
                             uint32 elemSize) const;

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

  const std::unique_ptr<FEIRExpr> &GetExpr() const {
    return expr;
  }

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
  bool SkipNonnullChecking(MIRBuilder &mirBuilder) const;

  Opcode op;
  std::unique_ptr<FEIRExpr> expr;
};

// ---------- FEIRStmtSafetyCallAssert ----------
class FEIRStmtSafetyCallAssert {
 public:
  FEIRStmtSafetyCallAssert(const std::string &funcName, size_t paramIndex)
      : funcNameIdx(GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(funcName)), paramIndex(paramIndex) {}

  virtual ~FEIRStmtSafetyCallAssert() = default;

  const std::string& GetFuncName() const {
    return GlobalTables::GetStrTable().GetStringFromStrIdx(funcNameIdx);
  }

  GStrIdx GetFuncNameIdx() const {
    return funcNameIdx;
  }

  size_t GetParamIndex() const {
    return paramIndex;
  }

 private:
  GStrIdx funcNameIdx;
  size_t paramIndex;
};

// ---------- FEIRStmtAssertNonnull ----------
class FEIRStmtAssertNonnull : public FEIRStmtUseOnly {
 public:
  FEIRStmtAssertNonnull(Opcode argOp, std::unique_ptr<FEIRExpr> argExpr)
      : FEIRStmtUseOnly(argOp, std::move(argExpr)) {}

  ~FEIRStmtAssertNonnull() = default;

 protected:
  std::list<StmtNode*> GenMIRStmtsImpl(MIRBuilder &mirBuilder) const override;
};

// ---------- FEIRStmtCallAssertNonnull ----------
class FEIRStmtCallAssertNonnull : public FEIRStmtUseOnly, public FEIRStmtSafetyCallAssert {
 public:
  FEIRStmtCallAssertNonnull(Opcode argOp, std::unique_ptr<FEIRExpr> argExpr, const std::string &funcName,
                            size_t paramIndex)
      : FEIRStmtUseOnly(argOp, std::move(argExpr)), FEIRStmtSafetyCallAssert(funcName, paramIndex) {}

  ~FEIRStmtCallAssertNonnull() = default;

 protected:
  std::list<StmtNode*> GenMIRStmtsImpl(MIRBuilder &mirBuilder) const override;
};

// ---------- FEIRStmtCallAssertBoundary ----------
class FEIRStmtCallAssertBoundary : public FEIRStmtNary, public FEIRStmtSafetyCallAssert {
 public:
  FEIRStmtCallAssertBoundary(Opcode opIn, std::list<std::unique_ptr<FEIRExpr>> argExprsIn, const std::string &funcName,
                             size_t paramIndex)
      : FEIRStmtNary(opIn, std::move(argExprsIn)), FEIRStmtSafetyCallAssert(funcName, paramIndex) {}

  ~FEIRStmtCallAssertBoundary() = default;

 protected:
  std::list<StmtNode*> GenMIRStmtsImpl(MIRBuilder &mirBuilder) const override;
};

// ---------- FEIRStmtAssertBoundary ----------
class FEIRStmtAssertBoundary : public FEIRStmtNary {
 public:
  FEIRStmtAssertBoundary(Opcode opIn,
      std::list<std::unique_ptr<FEIRExpr>> argExprsIn) : FEIRStmtNary(opIn, std::move(argExprsIn)) {}

  ~FEIRStmtAssertBoundary() = default;

 protected:
  std::list<StmtNode*> GenMIRStmtsImpl(MIRBuilder &mirBuilder) const override;
};

// ---------- FEIRStmtReturn ----------
class FEIRStmtReturn : public FEIRStmtUseOnly {
 public:
  explicit FEIRStmtReturn(std::unique_ptr<FEIRExpr> argExpr);
  ~FEIRStmtReturn() = default;

  const std::unique_ptr<FEIRExpr> &GetExpr() const {
    return expr;
  }

 protected:
  std::list<StmtNode*> GenMIRStmtsImpl(MIRBuilder &mirBuilder) const override;

 private:
  void InsertNonnullChecking(MIRBuilder &mirBuilder, std::list<StmtNode*> &ans) const;
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
  explicit FEIRStmtGotoForC(const std::string &labelName);
  virtual ~FEIRStmtGotoForC() = default;
  void SetLabelName(const std::string &name) {
    labelName = name;
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

// ---------- FEIRStmtIGoto ----------
class FEIRStmtIGoto : public FEIRStmt {
 public:
  explicit FEIRStmtIGoto(UniqueFEIRExpr expr);
  virtual ~FEIRStmtIGoto() = default;

 protected:
  bool IsFallThroughImpl() const override {
    return false;
  }

  bool IsBranchImpl() const override {
    return true;
  }

  std::list<StmtNode*> GenMIRStmtsImpl(MIRBuilder &mirBuilder) const override;

  UniqueFEIRExpr targetExpr;
};

// ---------- FEIRStmtCondGotoForC ----------
class FEIRStmtCondGotoForC : public FEIRStmt {
 public:
  explicit FEIRStmtCondGotoForC(UniqueFEIRExpr argExpr, Opcode op, const std::string &name)
      : FEIRStmt(FEIRNodeKind::kStmtCondGoto), expr(std::move(argExpr)), opCode(op), labelName(std::move(name)) {}
  virtual ~FEIRStmtCondGotoForC() = default;
  void SetLabelName(const std::string &name) {
    labelName = name;
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
  UniqueFEIRExpr expr;
  Opcode opCode;
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

  void SetBreakLabelName(std::string name) {
    breakLabelName = std::move(name);
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
  std::string breakLabelName;
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
  const std::map<int64, std::unique_ptr<FEIRStmtPesudoLabel>> &GetPesudoLabelMap() const {
    return pesudoLabelMap;
  }

 protected:
  std::string DumpDotStringImpl() const override;
  std::list<StmtNode*> GenMIRStmtsImpl(MIRBuilder &mirBuilder) const override;

 private:
  int64 lCaseLabel;
  std::map<int64, std::unique_ptr<FEIRStmtPesudoLabel>> pesudoLabelMap =
      std::map<int64, std::unique_ptr<FEIRStmtPesudoLabel>>();
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

  // for C
  FEIRStmtArrayStore(UniqueFEIRExpr argExprElem, UniqueFEIRExpr argExprArray, UniqueFEIRExpr argExprIndex,
                     UniqueFEIRType argTypeArray, const std::string &argArrayName);

  // for C mul array
  FEIRStmtArrayStore(UniqueFEIRExpr argExprElem, UniqueFEIRExpr argExprArray, UniqueFEIRExpr argExprIndex,
                     UniqueFEIRType argTypeArray, UniqueFEIRType argTypeElem, const std::string &argArrayName);
  // for C mul array
  FEIRStmtArrayStore(UniqueFEIRExpr argExprElem, UniqueFEIRExpr argExprArray, std::list<UniqueFEIRExpr> &argExprIndexs,
                     UniqueFEIRType argTypeArray, const std::string &argArrayName);

  // for C array in struct
  FEIRStmtArrayStore(UniqueFEIRExpr argExprElem, UniqueFEIRExpr argExprArray, std::list<UniqueFEIRExpr> &argExprIndexs,
                     UniqueFEIRType argTypeArray, UniqueFEIRExpr argExprStruct, UniqueFEIRType argTypeStruct,
                     const std::string &argArrayName);

  ~FEIRStmtArrayStore() = default;

  void SetIndexsExprs(std::list<UniqueFEIRExpr> &exprs) {
    exprIndexs.clear();
    for (auto &e : exprs) {
      auto ue = e->Clone();
      exprIndexs.push_back(std::move(ue));
    }
  }

 protected:
  std::string DumpDotStringImpl() const override;
  void RegisterDFGNodes2CheckPointImpl(FEIRStmtCheckPoint &checkPoint) override;
  bool CalculateDefs4AllUsesImpl(FEIRStmtCheckPoint &checkPoint, FEIRUseDefChain &udChain) override;
  void InitTrans4AllVarsImpl() override;
  std::list<StmtNode*> GenMIRStmtsImpl(MIRBuilder &mirBuilder) const override;
  void GenMIRStmtsImplForCPart(MIRBuilder &mirBuilder, MIRType *ptrMIRArrayType, MIRType **mIRElemType,
                               BaseNode **arrayExpr) const;

 private:
  UniqueFEIRExpr exprElem;
  UniqueFEIRExpr exprArray;
  UniqueFEIRExpr exprIndex;
  // for C mul array
  mutable std::list<UniqueFEIRExpr> exprIndexs;
  UniqueFEIRType typeArray;
  mutable UniqueFEIRType typeElem = nullptr;

  // for C array in struct
  UniqueFEIRExpr exprStruct;
  UniqueFEIRType typeStruct;
  std::string arrayName;
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
  void InsertNonnullInRetVar(MIRSymbol &retVarSym) const;
  void InsertNonnullCheckingInArgs(const UniqueFEIRExpr &expr, size_t index, MIRBuilder &mirBuilder,
                                   std::list<StmtNode*> &ans, const std::string &funcName) const;
  FEStructMethodInfo &methodInfo;
  Opcode mirOp;
  bool isStatic;
  static std::map<Opcode, Opcode> mapOpAssignToOp;
  static std::map<Opcode, Opcode> mapOpToOpAssign;
};

// ---------- FEIRStmtICallAssign ----------
class FEIRStmtICallAssign : public FEIRStmtAssign {
 public:
  FEIRStmtICallAssign();
  ~FEIRStmtICallAssign() = default;

 protected:
  std::list<StmtNode*> GenMIRStmtsImpl(MIRBuilder &mirBuilder) const override;
  std::string DumpDotStringImpl() const override;
  void RegisterDFGNodes2CheckPointImpl(FEIRStmtCheckPoint &checkPoint) override;
  bool CalculateDefs4AllUsesImpl(FEIRStmtCheckPoint &checkPoint, FEIRUseDefChain &udChain) override;

 private:
  void InsertNonnullCheckingInArgs(MIRBuilder &mirBuilder, std::list<StmtNode*> &ans) const;
  void InsertNonnullInRetVar(MIRSymbol &retVarSym) const;
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

class FEIRStmtPesudoSafe : public FEIRStmt {
 public:
  FEIRStmtPesudoSafe(bool isEnd);
  ~FEIRStmtPesudoSafe() = default;

 protected:
  std::string DumpDotStringImpl() const override;
  std::list<StmtNode*> GenMIRStmtsImpl(MIRBuilder &mirBuilder) const override;
  bool end = false;
};

class FEIRStmtPesudoUnsafe : public FEIRStmt {
 public:
  FEIRStmtPesudoUnsafe(bool isEnd);
  ~FEIRStmtPesudoUnsafe() = default;

 protected:
  std::string DumpDotStringImpl() const override;
  std::list<StmtNode*> GenMIRStmtsImpl(MIRBuilder &mirBuilder) const override;
  bool end = false;
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

  std::list<UniqueFEIRStmt> &GetThenStmt() {
    return thenStmts;
  }

  std::list<UniqueFEIRStmt> &GetElseStmt() {
    return elseStmts;
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

  void SetBreakLabelName(std::string name) {
    breakLabelName = std::move(name);
  }

 protected:
  bool IsBranchImpl() const override {
    return true;
  }

  std::list<StmtNode*> GenMIRStmtsImpl(MIRBuilder &mirBuilder) const override;

 private:
  std::string breakLabelName;
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

class FEIRStmtLabel : public FEIRStmt {
 public:
  explicit FEIRStmtLabel(const std::string &name) : FEIRStmt(FEIRNodeKind::kStmtLabel), labelName(name) {}
  ~FEIRStmtLabel() = default;

 protected:
  bool IsBranchImpl() const override {
    return true;
  }

  std::list<StmtNode*> GenMIRStmtsImpl(MIRBuilder &mirBuilder) const override;

 private:
  std::string labelName;
};

class FEIRStmtAtomic : public FEIRStmt {
 public:
  FEIRStmtAtomic(UniqueFEIRExpr expr);
  ~FEIRStmtAtomic() = default;

 protected:
  std::list<StmtNode*> GenMIRStmtsImpl(MIRBuilder &mirBuilder) const override;
  UniqueFEIRExpr atomicExpr;
};

class FEIRStmtGCCAsm : public FEIRStmt {
 public:
  FEIRStmtGCCAsm(const std::string &str, bool isGotoArg, bool isVolatileArg)
      : FEIRStmt(FEIRNodeKind::kStmtGCCAsm), asmStr(str), isGoto(isGotoArg), isVolatile(isVolatileArg) {}
  ~FEIRStmtGCCAsm() = default;

  void SetLabels(const std::vector<std::string> &labelsArg) {
    labels = labelsArg;
  }

  void SetClobbers(const std::vector<std::string> &clobbersArg) {
    clobbers = clobbersArg;
  }

  void SetInputs(const std::vector<std::pair<std::string, std::string>> &inputsArg) {
    inputs = inputsArg;
  }

  void SetInputsExpr(std::vector<UniqueFEIRExpr> &expr) {
    std::move(begin(expr), end(expr), std::inserter(inputsExprs, end(inputsExprs)));
  }

  void SetOutputs(const std::vector<std::tuple<std::string, std::string, bool>> &outputsArg) {
    outputs = outputsArg;
  }

  void SetOutputsExpr(std::vector<UniqueFEIRExpr> &expr) {
    std::move(begin(expr), end(expr), std::inserter(outputsExprs, end(outputsExprs)));
  }

 protected:
  std::list<StmtNode*> GenMIRStmtsImpl(MIRBuilder &mirBuilder) const override;
  bool HandleConstraintPlusQm(MIRBuilder &mirBuilder, AsmNode *asmNode, uint32 index) const;

 private:
  std::vector<std::tuple<std::string, std::string, bool>> outputs;
  std::vector<UniqueFEIRExpr> outputsExprs;
  std::vector<std::pair<std::string, std::string>> inputs;
  std::vector<UniqueFEIRExpr> inputsExprs;
  std::vector<std::string> clobbers;
  std::vector<std::string> labels;
  std::string asmStr;
  bool isGoto = false;
  bool isVolatile = false;
};
}  // namespace maple
#endif  // HIR2MPL_INCLUDE_COMMON_FEIR_STMT_H
