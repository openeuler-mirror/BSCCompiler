/*
 * Copyright (c) [2019-2021] Huawei Technologies Co.,Ltd.All rights reserved.
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
#ifndef MAPLE_ME_INCLUDE_ME_IR_H
#define MAPLE_ME_INCLUDE_ME_IR_H
#include <array>
#include "orig_symbol.h"
#include "bb.h"
#include "safe_cast.h"

namespace maple {
class PhiNode;    // circular dependency exists, no other choice
class MeStmt;     // circular dependency exists, no other choice
class IRMap;      // circular dependency exists, no other choice
class SSATab;     // circular dependency exists, no other choice
class VarMeExpr;  // circular dependency exists, no other choice
class Dominance;  // circular dependency exists, no other choice
using MeStmtPtr = MeStmt*;
enum MeExprOp : uint8 {
  kMeOpUnknown,
  kMeOpVar,
  kMeOpIvar,
  kMeOpAddrof,
  kMeOpAddroffunc,
  kMeOpAddroflabel,
  kMeOpGcmalloc,
  kMeOpReg,
  kMeOpConst,
  kMeOpConststr,
  kMeOpConststr16,
  kMeOpSizeoftype,
  kMeOpFieldsDist,
  kMeOpOp,
  kMeOpNary
};  // cache the op to avoid dynamic cast

constexpr int kInvalidExprID = -1;
class MeExpr {
 public:
  MeExpr(int32 exprId, MeExprOp meop, Opcode op, PrimType t, size_t n)
      : op(op), primType(t), numOpnds(n), meOp(meop), exprID(exprId) {}

  virtual ~MeExpr() = default;

  void SetOp(Opcode opcode) {
    op = opcode;
  }

  void SetPtyp(PrimType primTypeVal) {
    primType = primTypeVal;
  }

  void SetNumOpnds(uint8 val) {
    numOpnds = val;
  }

  void SetTreeID(uint32 val) {
    treeID = val;
  }

  void SetNext(MeExpr *nextPara) {
    next = nextPara;
  }

  Opcode GetOp() const {
    return op;
  }

  PrimType GetPrimType() const {
    return primType;
  }

  uint8 GetNumOpnds() const {
    return numOpnds;
  }

  MeExprOp GetMeOp() const {
    return meOp;
  }

  int32 GetExprID() const {
    return exprID;
  }

  uint32 GetTreeID() const {
    return treeID;
  }

  MeExpr *GetNext() const {
    return next;
  }

  virtual void Dump(const IRMap*, int32 indent = 0) const {
    (void)indent;
  }

  virtual uint8 GetDepth() const {
    return 0;
  }

  virtual bool IsZero() const {
    return false;
  }

  virtual bool IsIntZero() const {
    return false;
  }

  virtual bool IsUseSameSymbol(const MeExpr &expr) const {
    return exprID == expr.exprID;
  }

  virtual BaseNode &EmitExpr(MapleAllocator&) = 0;
  bool IsLeaf() const {
    return numOpnds == 0;
  }

  bool IsScalar() const {
    return meOp == kMeOpVar || meOp == kMeOpReg;
  }

  bool IsGcmalloc() const {
    return op == OP_gcmalloc || op == OP_gcmallocjarray || op == OP_gcpermalloc || op == OP_gcpermallocjarray;
  }

  virtual bool IsVolatile() const {
    return false;
  }

  bool ContainsVolatile() const;

  bool IsTheSameWorkcand(const MeExpr &expr) const;
  virtual void SetDefByStmt(MeStmt&) {}

  virtual MeExpr *GetOpnd(size_t) const {
    return nullptr;
  }

  virtual void SetOpnd(size_t, MeExpr*) {
    ASSERT(false, "Should not reach here");
  }

  virtual MeExpr *GetIdenticalExpr(MeExpr&, bool) const {
    ASSERT(false, "Should not reach here");
    return nullptr;
  }

  virtual MIRType *GetType() const { return nullptr; }

  MeExpr &GetAddrExprBase();               // get the base of the address expression
  // in the expression; nullptr otherwise
  bool SymAppears(OStIdx oidx);  // check if symbol appears in the expression
  bool HasIvar() const;          // whether there is any iread node in the expression
  bool Pure() const {
    return !kOpcodeInfo.NotPure(op);
  }

  virtual bool IsSameVariableValue(const VarMeExpr &expr) const;
  MeExpr *ResolveMeExprValue();
  bool CouldThrowException() const;
  bool IsAllOpndsIdentical(const MeExpr &meExpr) const;
  bool PointsToSomethingThatNeedsIncRef();
  virtual uint32 GetHashIndex() const {
    return 0;
  }
  virtual bool HasAddressValue() { return false; }
  virtual bool StrengthReducible() { return false; }
  virtual int64 SRMultiplier(OriginalSt *ost) {
    (void)ost;
    return 1;
  }

  virtual bool IsOpMeExpr() const {
    return false;
  }

 protected:
  MeExpr *FindSymAppearance(OStIdx oidx);  // find the appearance of the symbol
  bool IsDexMerge() const;

  Opcode op;
  PrimType primType;
  uint8 numOpnds;
  MeExprOp meOp;
  int32 exprID;
  uint32 treeID = 0;  // for bookkeeping purpose during SSAPRE
  MeExpr *next = nullptr;
};

enum MeDefBy {
  kDefByNo,
  kDefByStmt,
  kDefByPhi,
  kDefByChi,
  kDefByMustDef  // only applies to callassigned and its siblings
};

class ChiMeNode;      // circular dependency exists, no other choice
class MustDefMeNode;  // circular dependency exists, no other choice
class IassignMeStmt;  // circular dependency exists, no other choice

// base class for VarMeExpr and RegMeExpr
class ScalarMeExpr : public MeExpr {
 public:
  ScalarMeExpr(int32 exprid, OriginalSt *origSt, uint32 vidx, MeExprOp meop, Opcode o, PrimType ptyp)
      : MeExpr(exprid, meop, o, ptyp, 0),
        ost(origSt),
        vstIdx(vidx),
        defBy(kDefByNo) {
    def.defStmt = nullptr;
  }

  ~ScalarMeExpr() override = default;

  bool IsIdentical(MeExpr&) const {
    CHECK_FATAL(false, "ScalarMeExpr::IsIdentical() should not be called");
    return true;
  }

  bool IsUseSameSymbol(const MeExpr &expr) const override;

  void SetDefByStmt(MeStmt &defStmt) override {
    defBy = kDefByStmt;
    def.defStmt = &defStmt;
  }

  MePhiNode *GetMePhiDef() const {
    return IsDefByPhi() ? def.defPhi : nullptr;
  }

  bool IsDefByNo() const {
    return defBy == kDefByNo;
  }

  bool IsDefByPhi() const {
    return defBy == kDefByPhi;
  }

  bool IsDefByChi() const {
    return defBy == kDefByChi;
  }

  bool IsDefByStmt() const {
    return defBy == kDefByStmt;
  }

  bool IsDefByMustDef() const {
    return defBy == kDefByMustDef;
  }

  BB *DefByBB() const;

  OriginalSt *GetOst() const {
    return ost;
  }

  void SetOst(OriginalSt *o) {
    ost = o;
  }

  OStIdx GetOstIdx() const {
    return ost->GetIndex();
  }

  size_t GetVstIdx() const {
    return vstIdx;
  }

  void SetVstIdx(size_t vstIdxVal) {
    vstIdx = static_cast<uint32>(vstIdxVal);
  }

  MeDefBy GetDefBy() const {
    return defBy;
  }

  void SetDefBy(MeDefBy defByVal) {
    defBy = defByVal;
  }

  MeStmt *GetDefStmt() const {
    return def.defStmt;
  }

  void SetDefStmt(MeStmt *defStmt) {
    def.defStmt = defStmt;
  }

  MePhiNode &GetDefPhi() {
    return *def.defPhi;
  }

  const MePhiNode &GetDefPhi() const {
    return *def.defPhi;
  }

  void SetDefPhi(MePhiNode &defPhi) {
    def.defPhi = &defPhi;
  }

  ChiMeNode &GetDefChi() {
    return *def.defChi;
  }

  const ChiMeNode &GetDefChi() const {
    return *def.defChi;
  }

  void SetDefChi(ChiMeNode &defChi) {
    def.defChi = &defChi;
  }

  MustDefMeNode &GetDefMustDef() {
    return *def.defMustDef;
  }

  const MustDefMeNode &GetDefMustDef() const {
    return *def.defMustDef;
  }

  void SetDefMustDef(MustDefMeNode &defMustDef) {
    def.defMustDef = &defMustDef;
  }

  PregIdx GetRegIdx() const {
    ASSERT(ost->IsPregOst(), "GetPregIdx: not a preg");
    return ost->GetPregIdx();
  }

  bool IsNormalReg() const {
    ASSERT(ost->IsPregOst(), "GetPregIdx: not a preg");
    return ost->GetPregIdx() >= 0;
  }

  MeStmt *GetDefByMeStmt() const;
  BB *GetDefByBBMeStmt(const MeCFG &cfg, MeStmtPtr &defMeStmt) const;
  void Dump(const IRMap *irMap, int32 indent = 0) const override;
  BaseNode &EmitExpr(MapleAllocator &alloc) override;
  bool IsSameVariableValue(const VarMeExpr &expr) const override;
  ScalarMeExpr *FindDefByStmt(std::set<ScalarMeExpr*> &visited);
  bool IsZeroVersion() const;

 protected:
  OriginalSt *ost;
  uint32 vstIdx;    // the index in MEOptimizer's VersionStTable, 0 if not in VersionStTable
  MeDefBy defBy : 3;
  union {
    MeStmt *defStmt;  // definition stmt of this var
    MePhiNode *defPhi;
    ChiMeNode *defChi;          // definition node by Chi
    MustDefMeNode *defMustDef;  // definition by callassigned
  } def;
};

using RegMeExpr = ScalarMeExpr;

// represant dread
class VarMeExpr final : public ScalarMeExpr {
 public:
  VarMeExpr(int32 exprid, OriginalSt *ost, size_t vidx, PrimType ptyp)
      : ScalarMeExpr(exprid, ost, vidx, kMeOpVar, OP_dread, ptyp) {}

  ~VarMeExpr() override = default;

  void Dump(const IRMap *irMap, int32 indent = 0) const override;
  BaseNode &EmitExpr(MapleAllocator &alloc) override;
  bool IsValidVerIdx() const;

  bool IsVolatile() const override;
  // indicate if the variable is local variable but not a function formal variable
  bool IsPureLocal(const MIRFunction &irFunc) const;
  bool IsSameVariableValue(const VarMeExpr &expr) const override;
  VarMeExpr &ResolveVarMeValue();
  bool PointsToStringLiteral();

  FieldID GetFieldID() const {
    return GetOst()->GetFieldID();
  }

  TyIdx GetInferredTyIdx() const {
    return inferredTyIdx;
  }

  void SetInferredTyIdx(TyIdx inferredTyIdxVal) {
    inferredTyIdx = inferredTyIdxVal;
  }
  bool GetMaybeNull() const {
    return maybeNull;
  }

  void SetMaybeNull(bool maybeNullVal) {
    maybeNull = maybeNullVal;
  }

  bool GetNoDelegateRC() const {
    return noDelegateRC;
  }

  void SetNoDelegateRC(bool noDelegateRCVal) {
    noDelegateRC = noDelegateRCVal;
  }

  bool GetNoSubsumeRC() const {
    return noSubsumeRC;
  }

  void SetNoSubsumeRC(bool noSubsumeRCVal) {
    noSubsumeRC = noSubsumeRCVal;
  }

  MIRType *GetType() const override {
    return GlobalTables::GetTypeTable().GetTypeFromTyIdx(ost->GetTyIdx());
  }

  bool HasAddressValue() override { return MustBeAddress(GetPrimType()) || GetType()->GetKind() == kTypePointer; }

 private:
  bool noDelegateRC = false;  // true if this cannot be optimized by delegaterc
  bool noSubsumeRC = false;   // true if this cannot be optimized by subsumrc
  TyIdx inferredTyIdx{ 0 }; /* Non zero if it has a known type (allocation type is seen). */
  bool maybeNull = true;  // false if definitely not null
};

class MePhiNode {
 public:
  explicit MePhiNode(MapleAllocator *alloc)
      : opnds(kOperandNumBinary, nullptr, alloc->Adapter()) {
    opnds.pop_back();
    opnds.pop_back();
  }

  MePhiNode(ScalarMeExpr *expr, MapleAllocator *alloc)
      : lhs(expr), opnds(kOperandNumBinary, nullptr, alloc->Adapter()) {
    expr->SetDefPhi(*this);
    expr->SetDefBy(kDefByPhi);
    opnds.pop_back();
    opnds.pop_back();
  }

  ~MePhiNode() {
    lhs = nullptr;
    defBB = nullptr;
  };

  void UpdateLHS(ScalarMeExpr &expr) {
    lhs = &expr;
    expr.SetDefBy(kDefByPhi);
    expr.SetDefPhi(*this);
  }

  void Dump(const IRMap *irMap) const;

  ScalarMeExpr *GetOpnd(size_t idx) const {
    ASSERT(idx < opnds.size(), "out of range in MeVarPhiNode::GetOpnd");
    return opnds.at(idx);
  }

  void SetOpnd(size_t idx, ScalarMeExpr *opnd) {
    CHECK_FATAL(idx < opnds.size(), "out of range in MePhiNode::SetOpnd");
    opnds[idx] = opnd;
  }

  const MapleVector<ScalarMeExpr*> &GetOpnds() const {
    return opnds;
  }

  MapleVector<ScalarMeExpr*> &GetOpnds() {
    return opnds;
  }

  void SetIsLive(bool isLiveVal) {
    isLive = isLiveVal;
  }

  bool GetIsLive() const {
    return isLive;
  }

  void SetDefBB(BB *defBBVal) {
    defBB = defBBVal;
  }

  BB *GetDefBB() {
    return defBB;
  }

  const BB *GetDefBB() const {
    return defBB;
  }

  ScalarMeExpr *GetLHS() const {
    return lhs;
  }

  void SetLHS(ScalarMeExpr *value) {
    value->SetDefBy(kDefByPhi);
    value->SetDefPhi(*this);
    lhs = value;
  }

  void SetPiAdded() {
    isPiAdded = true;
  }

  bool IsPiAdded() const {
    return isPiAdded;
  }

  bool UseReg() const {
    return lhs->GetMeOp() == kMeOpReg;
  }

 private:
  ScalarMeExpr *lhs = nullptr;
  MapleVector<ScalarMeExpr*> opnds;
  bool isLive = true;
  BB *defBB = nullptr;  // the bb that defines this phi
  bool isPiAdded = false;
};

class ConstMeExpr : public MeExpr {
 public:
  ConstMeExpr(int32 exprID, MIRConst *constValParam, PrimType t)
      : MeExpr(exprID, kMeOpConst, OP_constval, t, 0), constVal(constValParam) {}

  ~ConstMeExpr() override = default;

  void Dump(const IRMap*, int32 indent = 0) const override;
  BaseNode &EmitExpr(MapleAllocator &alloc) override;
  bool GeZero() const;
  bool GtZero() const;
  bool IsZero() const override;
  bool IsIntZero() const override;
  bool IsOne() const;
  // Get int value represented by IntVal class
  IntVal GetIntValue() const;
  // Get value extended by its sign
  int64 GetExtIntValue() const;
  // Get zero extended value
  uint64 GetZXTIntValue() const;
  // Get sign extended value
  int64 GetSXTIntValue() const;

  MIRConst *GetConstVal() {
    return constVal;
  }

  const MIRConst *GetConstVal() const {
    return constVal;
  }

  MeExpr *GetIdenticalExpr(MeExpr &expr, bool isConstructor) const override;

  uint32 GetHashIndex() const override {
    CHECK_FATAL(constVal != nullptr, "constVal is null");
    if (constVal->GetKind() == kConstInt) {
      auto *intConst = safe_cast<MIRIntConst>(constVal);
      CHECK_NULL_FATAL(intConst);
      const IntVal &intValConst = intConst->GetValue();
      if (!intValConst.IsOneSignificantWord()) {
        const uint64 *val = intValConst.GetRawData();
        return static_cast<uint32>(std::hash<uint64>{}(val[0]) ^ std::hash<uint64>{}(val[1]));
      } else {
        return static_cast<uint32>(intValConst.GetExtValue());
      }
    }
    if (constVal->GetKind() == kConstFloatConst) {
      auto *floatConst = safe_cast<MIRFloatConst>(constVal);
      CHECK_NULL_FATAL(floatConst);
      return floatConst->GetIntValue();
    }
    if (constVal->GetKind() == kConstDoubleConst) {
      auto *doubleConst = safe_cast<MIRDoubleConst>(constVal);
      CHECK_NULL_FATAL(doubleConst);
      return doubleConst->GetIntValue();
    }
    if (constVal->GetKind() == kConstFloat128Const) {
      auto *float128Const = safe_cast<MIRFloat128Const>(constVal);
      CHECK_NULL_FATAL(float128Const);
      return static_cast<uint32>(float128Const->GetDoubleValue());
    }
    if (constVal->GetKind() == kConstLblConst) {
      auto *lblConst = safe_cast<MIRLblConst>(constVal);
      CHECK_NULL_FATAL(lblConst);
      return lblConst->GetValue();
    }
    ASSERT(false, "ComputeHash: const type not yet implemented");
    return 0;
  }

 private:
  MIRConst *constVal;
};

class ConststrMeExpr : public MeExpr {
 public:
  ConststrMeExpr(int32 exprID, UStrIdx idx, PrimType t)
      : MeExpr(exprID, kMeOpConststr, OP_conststr, t, 0), strIdx(idx) {}

  ~ConststrMeExpr() override = default;

  void Dump(const IRMap*, int32 indent = 0) const override;
  BaseNode &EmitExpr(MapleAllocator &alloc) override;
  MeExpr *GetIdenticalExpr(MeExpr &expr, bool isConstructor) const override;

  UStrIdx GetStrIdx() const {
    return strIdx;
  }

  uint32 GetHashIndex() const override {
    constexpr uint32 kConststrHashShift = 6;
    return static_cast<uint32>(strIdx) << kConststrHashShift;
  }

 private:
  UStrIdx strIdx;
};

class Conststr16MeExpr : public MeExpr {
 public:
  Conststr16MeExpr(int32 exprID, U16StrIdx idx, PrimType t)
      : MeExpr(exprID, kMeOpConststr16, OP_conststr16, t, 0), strIdx(idx) {}

  ~Conststr16MeExpr() override = default;

  void Dump(const IRMap*, int32 indent = 0) const override;
  BaseNode &EmitExpr(MapleAllocator &alloc) override;
  MeExpr *GetIdenticalExpr(MeExpr &expr, bool isConstructor) const override;

  U16StrIdx GetStrIdx() {
    return strIdx;
  }

  uint32 GetHashIndex() const override {
    constexpr uint32 kConststr16HashShift = 6;
    return static_cast<uint32>(strIdx) << kConststr16HashShift;
  }

 private:
  U16StrIdx strIdx;
};

class SizeoftypeMeExpr : public MeExpr {
 public:
  SizeoftypeMeExpr(int32 exprid, PrimType t, TyIdx idx)
      : MeExpr(exprid, kMeOpSizeoftype, OP_sizeoftype, t, 0), tyIdx(idx) {}

  ~SizeoftypeMeExpr() override = default;

  void Dump(const IRMap*, int32 indent = 0) const override;
  BaseNode &EmitExpr(MapleAllocator &alloc) override;
  MeExpr *GetIdenticalExpr(MeExpr &expr, bool isConstructor) const override;

  TyIdx GetTyIdx() const {
    return tyIdx;
  }

  uint32 GetHashIndex() const override {
    constexpr uint32 sizeoftypeHashShift = 5;
    return static_cast<uint32>(tyIdx) << sizeoftypeHashShift;
  }

 private:
  TyIdx tyIdx;
};

class FieldsDistMeExpr : public MeExpr {
 public:
  FieldsDistMeExpr(int32 exprid, PrimType t, TyIdx idx, FieldID f1, FieldID f2)
      : MeExpr(exprid, kMeOpFieldsDist, OP_fieldsdist, t, 0), tyIdx(idx), fieldID1(f1), fieldID2(f2) {}

  ~FieldsDistMeExpr() override = default;
  void Dump(const IRMap*, int32 indent = 0) const override;
  BaseNode &EmitExpr(MapleAllocator &alloc) override;
  MeExpr *GetIdenticalExpr(MeExpr &expr, bool isConstructor) const override;

  TyIdx GetTyIdx() const {
    return tyIdx;
  }

  FieldID GetFieldID1() const {
    return fieldID1;
  }

  FieldID GetFieldID2() const {
    return fieldID2;
  }

  uint32 GetHashIndex() const override {
    constexpr uint32 kFieldsDistHashShift = 5;
    constexpr uint32 kTyIdxShiftFactor = 10;
    return (static_cast<uint32>(tyIdx) << kTyIdxShiftFactor) +
           (static_cast<uint32>(fieldID1) << kFieldsDistHashShift) + fieldID2;
  }

 private:
  TyIdx tyIdx;
  FieldID fieldID1;
  FieldID fieldID2;
};

class AddrofMeExpr : public MeExpr {
 public:
  AddrofMeExpr(int32 exprid, PrimType t, OriginalSt *ost)
      : MeExpr(exprid, kMeOpAddrof, OP_addrof, t, 0), ost(ost) {}

  ~AddrofMeExpr() override = default;

  void Dump(const IRMap*, int32 indent = 0) const override;
  bool IsUseSameSymbol(const MeExpr &expr) const override;
  BaseNode &EmitExpr(MapleAllocator &alloc) override;
  MeExpr *GetIdenticalExpr(MeExpr &expr, bool isConstructor) const override;

  OStIdx GetOstIdx() const {
    return ost->GetIndex();
  }

  OriginalSt *GetOst() const {
    return ost;
  }

  FieldID GetFieldID() const {
    return ost->GetFieldID();
  }

  uint32 GetHashIndex() const override {
    constexpr uint32 addrofHashShift = 4;
    return static_cast<uint32>(GetOstIdx()) << addrofHashShift;
  }

  bool HasAddressValue() override { return true; }

 private:
  OriginalSt *ost;
};

class AddroffuncMeExpr : public MeExpr {
 public:
  AddroffuncMeExpr(int32 exprID, PUIdx puIdx)
      : MeExpr(exprID, kMeOpAddroffunc, OP_addroffunc, PTY_ptr, 0), puIdx(puIdx) {}

  ~AddroffuncMeExpr() override = default;

  void Dump(const IRMap*, int32 indent = 0) const override;
  BaseNode &EmitExpr(MapleAllocator &alloc) override;
  MeExpr *GetIdenticalExpr(MeExpr &expr, bool isConstructor) const override;

  PUIdx GetPuIdx() const {
    return puIdx;
  }

  uint32 GetHashIndex() const override {
    constexpr uint32 addroffuncHashShift = 5;
    return puIdx << addroffuncHashShift;
  }

  MIRType *GetType() const override {
    MIRFunction *func = GlobalTables::GetFunctionTable().GetFunctionFromPuidx(puIdx);
    return GlobalTables::GetTypeTable().GetOrCreatePointerType(*func->GetMIRFuncType(), PTY_ptr);
  }

 private:
  PUIdx puIdx;
};

class AddroflabelMeExpr : public MeExpr {
 public:
  LabelIdx labelIdx;

  AddroflabelMeExpr(int32 exprid, LabelIdx lidx)
      : MeExpr(exprid, kMeOpAddroflabel, OP_addroflabel, PTY_ptr, 0), labelIdx(lidx) {}

  ~AddroflabelMeExpr() override {}

  void Dump(const IRMap *irMap, int32 indent = 0) const override;
  bool IsIdentical(const MeExpr *meexpr) const {
    if (meexpr->GetOp() != GetOp()) {
      return false;
    }
    const AddroflabelMeExpr *x = static_cast<const AddroflabelMeExpr*>(meexpr);
    if (labelIdx != x->labelIdx) {
      return false;
    }
    return true;
  }
  BaseNode &EmitExpr(MapleAllocator &alloc) override;
  MeExpr *GetIdenticalExpr(MeExpr &expr, bool isConstructor) const override;

  uint32 GetHashIndex() const override {
    constexpr uint32 shiftNum = 4;
    return labelIdx << shiftNum;
  }
};

class GcmallocMeExpr : public MeExpr {
 public:
  GcmallocMeExpr(int32 exprid, Opcode o, PrimType t, TyIdx tyid)
      : MeExpr(exprid, kMeOpGcmalloc, o, t, 0), tyIdx(tyid) {}

  ~GcmallocMeExpr() override = default;

  void Dump(const IRMap*, int32 indent = 0) const override;
  BaseNode &EmitExpr(MapleAllocator &alloc) override;

  TyIdx GetTyIdx() const {
    return tyIdx;
  }

  uint32 GetHashIndex() const override {
    constexpr uint32 kGcmallocHashShift = 4;
    return static_cast<uint32>(tyIdx) << kGcmallocHashShift;
  }

 private:
  TyIdx tyIdx;
};

class OpMeExpr : public MeExpr {
 public:
  OpMeExpr(int32 exprID, Opcode o, PrimType t, size_t n)
      : MeExpr(exprID, kMeOpOp, o, t, n), tyIdx(TyIdx(0)) {}

  // unary
  OpMeExpr(int32 exprID, Opcode o, PrimType t, MeExpr *opnd0)
      : MeExpr(exprID, kMeOpOp, o, t, 1), tyIdx(TyIdx(0)) {
    SetOpndCheck(0, opnd0);
  }

  // binary
  OpMeExpr(int32 exprID, Opcode o, PrimType t, MeExpr *opnd0, MeExpr *opnd1, bool order = true)
      : MeExpr(exprID, kMeOpOp, o, t, 2), tyIdx(TyIdx(0)) {
    if (order) {
      SetOpndCheck(0, opnd0);
      SetOpndCheck(1, opnd1);
    } else {
      SetOpndCheck(0, opnd1);
      SetOpndCheck(1, opnd0);
    }
    hasAddressValue = opnd0->HasAddressValue() || opnd1->HasAddressValue();
  }

  OpMeExpr(const OpMeExpr &opMeExpr, int32 exprID)
      : MeExpr(exprID, kMeOpOp, opMeExpr.GetOp(), opMeExpr.GetPrimType(), opMeExpr.GetNumOpnds()),
        opnds(opMeExpr.opnds),
        opndType(opMeExpr.opndType),
        bitsOffset(opMeExpr.bitsOffset),
        bitsSize(opMeExpr.bitsSize),
        depth(opMeExpr.depth),
        tyIdx(opMeExpr.tyIdx),
        fieldID(opMeExpr.fieldID),
        hasAddressValue(opMeExpr.hasAddressValue) {}

  ~OpMeExpr() override = default;

  OpMeExpr(const OpMeExpr&) = delete;
  OpMeExpr &operator=(const OpMeExpr&) = delete;

  bool IsIdentical(const OpMeExpr &meExpr) const;
  bool IsAllOpndsIdentical(const OpMeExpr &meExpr) const;
  bool IsCompareIdentical(const OpMeExpr &meExpr) const;
  void Dump(const IRMap *irMap, int32 indent = 0) const override;
  bool IsUseSameSymbol(const MeExpr &expr) const override;
  MeExpr *GetIdenticalExpr(MeExpr &expr, bool isConstructor) const override;
  BaseNode &EmitExpr(MapleAllocator &alloc) override;
  uint8 GetDepth() const override {
    return depth;
  }
  MeExpr *GetOpnd(size_t i) const override {
    CHECK_FATAL(i < kOperandNumTernary, "OpMeExpr cannot have more than 3 operands");
    return opnds[i];
  }

  void SetOpndCheck(size_t idx, MeExpr *x)  {
    CHECK_FATAL(idx < kOperandNumTernary, "out of range in  OpMeExpr::SetOpnd");
    opnds[idx] = x;
    if (depth <= x->GetDepth()) {
      depth = x->GetDepth();
      if (depth != UINT8_MAX) {
        depth++;
      }
    }
  }

  void SetOpnd(size_t idx, MeExpr *x) override {
    SetOpndCheck(idx, x);
  }

  PrimType GetOpndType() const {
    return opndType;
  }

  void SetOpndType(PrimType opndTypeVal) {
    opndType = opndTypeVal;
  }

  uint8 GetBitsOffSet() const {
    return bitsOffset;
  }

  void SetBitsOffSet(uint8 bitsOffSetVal) {
    bitsOffset = bitsOffSetVal;
  }

  uint8 GetBitsSize() const {
    return bitsSize;
  }

  void SetBitsSize(uint8 bitsSizeVal) {
    bitsSize = bitsSizeVal;
  }

  TyIdx GetTyIdx() const {
    return tyIdx;
  }

  void SetTyIdx(TyIdx tyIdxVal) {
    tyIdx = tyIdxVal;
  }

  FieldID GetFieldID() const {
    return fieldID;
  }

  void SetFieldID(FieldID fieldIDVal) {
    fieldID = fieldIDVal;
  }

  uint32 GetHashIndex() const override {
    uint32 hashIdx = static_cast<uint32>(GetOp());
    constexpr uint32 kOpMeHashShift = 3;
    for (const auto &opnd : opnds) {
      if (opnd == nullptr) {
        break;
      }
      hashIdx += static_cast<uint32>(opnd->GetExprID()) << kOpMeHashShift;
    }
    return hashIdx;
  }

  MIRType *GetType() const override {
    if (tyIdx != TyIdx(0)) {
      return GlobalTables::GetTypeTable().GetTypeFromTyIdx(tyIdx);
    }
    return nullptr;
  }
  void SetHasAddressValue() {
    if (hasAddressValue) {
      return;
    }
    if (op != OP_select) {
      hasAddressValue = opnds[0]->HasAddressValue();
      if (!hasAddressValue && numOpnds > 1) {
        hasAddressValue = opnds[1]->HasAddressValue();
      }
    } else {
      hasAddressValue = opnds[1]->HasAddressValue() || opnds[2]->HasAddressValue();
    }
  }
  bool HasAddressValue() override { return hasAddressValue || op == OP_iaddrof; }
  bool StrengthReducible() override;
  int64 SRMultiplier(OriginalSt *ost) override;

  bool IsOpMeExpr() const override {
    return true;
  }

 private:
  std::array<MeExpr*, kOperandNumTernary> opnds = { nullptr };  // kid
  PrimType opndType = kPtyInvalid;  // from type
  uint8 bitsOffset = 0;
  uint8 bitsSize = 0;
  uint8 depth = 0;
  TyIdx tyIdx;
  FieldID fieldID = 0;  // this is also used to store puIdx
 public:
  bool hasAddressValue = false;    // used only when op is ADD or SUB
};

class IvarMeExpr : public MeExpr {
 public:
  IvarMeExpr(MapleAllocator *alloc, int32 exprid, PrimType t, const TyIdx &tidx, FieldID fid, Opcode op)
      : MeExpr(exprid, kMeOpIvar, op, t, 1), tyIdx(tidx), fieldID(fid), muList(1, nullptr, alloc->Adapter()) {}

  IvarMeExpr(MapleAllocator *alloc, int32 exprid, PrimType t, const TyIdx &tidx, FieldID fid)
      : IvarMeExpr(alloc, exprid, t, tidx, fid, OP_iread) {}

  IvarMeExpr(MapleAllocator *alloc, int32 exprid, const IvarMeExpr &ivarme)
      : MeExpr(exprid, kMeOpIvar, ivarme.op, ivarme.GetPrimType(), 1),
        defStmt(ivarme.defStmt),
        base(ivarme.base),
        tyIdx(ivarme.tyIdx),
        fieldID(ivarme.fieldID),
        offset(ivarme.offset),
        volatileFromBaseSymbol(ivarme.volatileFromBaseSymbol),
        muList(ivarme.muList, alloc->Adapter()) {}

  IvarMeExpr() = delete;  // Disable default ctor

  ~IvarMeExpr() override = default;

  void Dump(const IRMap *irMap, int32 indent = 0) const override;
  uint8 GetDepth() const override {
    return base->GetDepth() + 1;
  }
  BaseNode &EmitExpr(MapleAllocator &alloc) override;
  bool IsVolatile() const override;
  bool IsFinal();
  bool IsRCWeak() const;
  bool IsUseSameSymbol(const MeExpr &expr) const override;
  bool IsIdentical(IvarMeExpr &expr, bool inConstructor) const;
  bool IsMuListIdentical(IvarMeExpr &expr) const;
  MeExpr *GetIdenticalExpr(MeExpr &expr, bool inConstructor) const override;
  MeExpr *GetOpnd(size_t) const override {
    return base;
  }

  IassignMeStmt *GetDefStmt() const {
    return defStmt;
  }

  void SetDefStmt(IassignMeStmt *defStmtPara) {
    defStmt = defStmtPara;
  }

  const MeExpr *GetBase() const {
    return base;
  }

  MeExpr *GetBase() {
    return base;
  }

  void SetBase(MeExpr *value) {
    base = value;
  }

  TyIdx GetTyIdx() const {
    return tyIdx;
  }

  void SetTyIdx(TyIdx tyIdxVal) {
    tyIdx = tyIdxVal;
  }

  TyIdx GetInferredTyIdx() const {
    return inferredTyIdx;
  }

  void SetInferredTyidx(TyIdx inferredTyIdxVal) {
    inferredTyIdx = inferredTyIdxVal;
  }

  FieldID GetFieldID() const {
    return fieldID;
  }

  void SetFieldID(FieldID fieldIDVal) {
    fieldID = fieldIDVal;
  }

  int32 GetOffset() const {
    return offset;
  }

  void SetOffset(int32 val) {
    offset = val;
  }

  bool GetMaybeNull() const {
    return maybeNull;
  }

  void SetMaybeNull(bool maybeNullVal) {
    maybeNull = maybeNullVal;
  }

  bool GetVolatileFromBaseSymbol() const {
    return volatileFromBaseSymbol;
  }

  void SetVolatileFromBaseSymbol(bool value) {
    volatileFromBaseSymbol = value;
  }

  MapleVector<ScalarMeExpr*> &GetMuList() {
    return muList;
  }

  const MapleVector<ScalarMeExpr*> &GetMuList() const {
    return muList;
  }

  ScalarMeExpr *GetUniqueMu() {
    CHECK_FATAL(muList.size() == 1, "scalar ivar should has only 1 mu");
    return muList[0];
  }

  const ScalarMeExpr *GetUniqueMu() const {
    CHECK_FATAL(muList.size() == 1, "scalar ivar should has only 1 mu");
    return muList[0];
  }

  void SetMuItem(size_t i, ScalarMeExpr *muVal) {
    CHECK_FATAL(i < muList.size(), "container check");
    muList[i] = muVal;
  }

  void SetMuList(const MapleVector<ScalarMeExpr*> &inputMuList) {
    size_t len = inputMuList.size();
    if (muList.size() != len) {
      muList.resize(len, nullptr);
    }
    for (size_t i = 0; i < len; ++i) {
      muList[i] = inputMuList[i];
    }
  }

  void SetMuList(const std::vector<ScalarMeExpr*> &inputMuList) {
    size_t len = inputMuList.size();
    if (muList.size() != len) {
      muList.resize(len, nullptr);
    }
    for (size_t i = 0; i < len; ++i) {
      muList[i] = inputMuList[i];
    }
  }

  void Push2MuList(ScalarMeExpr &expr) {
    muList.push_back(&expr);
  }

  uint32 GetMuCount() const {
    return static_cast<uint32>(muList.size());
  }

  bool HasMultipleMu() const {
    return muList.size() > 1;
  }

  uint32 GetHashIndex() const override {
    constexpr uint32 kIvarHashShift = 4;
    return static_cast<uint32>(op) + static_cast<uint32>(fieldID) + static_cast<uint32>(offset) +
           (static_cast<uint32>(base->GetExprID()) << kIvarHashShift);
  }

  MIRType *GetType() const override {
    MIRPtrType *ptrtype = static_cast<MIRPtrType *>(GlobalTables::GetTypeTable().GetTypeFromTyIdx(tyIdx));
    if (fieldID == 0) {
      return ptrtype->GetPointedType();
    }
    return GlobalTables::GetTypeTable().GetTypeFromTyIdx(ptrtype->GetPointedTyIdxWithFieldID(fieldID));
  }

  bool HasAddressValue() override { return GetType()->GetKind() == kTypePointer; }
  bool simplifiedWithConstOffset = false;

 private:
  IassignMeStmt *defStmt = nullptr;
  MeExpr *base = nullptr;
  TyIdx tyIdx{ 0 };
  TyIdx inferredTyIdx{ 0 };  // may be a subclass of above tyIdx
  FieldID fieldID = 0;
  int32 offset = 0;
  bool maybeNull = true;  // false if definitely not null
  bool volatileFromBaseSymbol = false;  // volatile due to its base symbol being volatile
  // muList size must be >= 1, can not be empty
  MapleVector<ScalarMeExpr*> muList;  // vector type ivar may have multiple mu, non-vector type ivar has only 1 mu
};

// for array, intrinsicop and intrinsicopwithtype
class NaryMeExpr : public MeExpr {
 public:
  NaryMeExpr(MapleAllocator *alloc, int32 expID, Opcode o, PrimType t,
             size_t n, TyIdx tyIdx, MIRIntrinsicID intrinID, bool bCheck)
      : MeExpr(expID, kMeOpNary, o, t, n),
        tyIdx(tyIdx),
        intrinsic(intrinID),
        opnds(alloc->Adapter()),
        boundCheck(bCheck) {}

  NaryMeExpr(MapleAllocator *alloc, int32 expID, const NaryMeExpr &meExpr)
      : MeExpr(expID, kMeOpNary, meExpr.GetOp(), meExpr.GetPrimType(), meExpr.GetNumOpnds()),
        tyIdx(meExpr.tyIdx),
        intrinsic(meExpr.intrinsic),
        opnds(alloc->Adapter()),
        depth(meExpr.depth),
        boundCheck(meExpr.boundCheck) {
    for (size_t i = 0; i < meExpr.opnds.size(); ++i) {
      opnds.push_back(meExpr.opnds[i]);
    }
  }

  ~NaryMeExpr() override = default;

  void Dump(const IRMap *irMap, int32 indent = 0) const override;
  uint8 GetDepth() const override {
    return depth;
  }
  bool IsIdentical(const NaryMeExpr& meExpr) const;
  bool IsUseSameSymbol(const MeExpr &expr) const override;
  BaseNode &EmitExpr(MapleAllocator &alloc) override;
  MeExpr *GetIdenticalExpr(MeExpr &expr, bool isConstructor) const override;
  MeExpr *GetOpnd(size_t idx) const override {
    ASSERT(idx < opnds.size(), "NaryMeExpr operand out of bounds");
    return opnds[idx];
  }

  TyIdx GetTyIdx() const {
    return tyIdx;
  }

  MIRIntrinsicID GetIntrinsic() const {
    return intrinsic;
  }

  const MapleVector<MeExpr*> &GetOpnds() const {
    return opnds;
  }

  MapleVector<MeExpr*> &GetOpnds() {
    return opnds;
  }

  void SetOpnd(size_t idx, MeExpr *val) override {
    ASSERT(idx < opnds.size(), "out of range in NaryMeStmt::GetOpnd");
    opnds[idx] = val;
    if (depth <= val->GetDepth()) {
      depth = val->GetDepth();
      if (depth != UINT8_MAX) {
        depth++;
      }
    }
  }

  void PushOpnd(MeExpr *x) {
    opnds.push_back(x);
    if (depth <= x->GetDepth()) {
      depth = x->GetDepth();
      if (depth != UINT8_MAX) {
        depth++;
      }
    }
  }

  bool GetBoundCheck() const {
    return boundCheck;
  }

  void SetBoundCheck(bool ch) {
    boundCheck = ch;
  }

  uint32 GetHashIndex() const override {
    auto hashIdx = static_cast<uint32>(GetOp());
    constexpr uint32 kNaryHashShift = 3;
    for (uint32 i = 0; i < GetNumOpnds(); ++i) {
      hashIdx += static_cast<uint32>(opnds[i]->GetExprID()) << kNaryHashShift;
    }
    hashIdx += static_cast<uint32>(boundCheck);
    return hashIdx;
  }

  bool HasAddressValue() override { return op == OP_array; }

 private:
  TyIdx tyIdx;
  MIRIntrinsicID intrinsic;
  MapleVector<MeExpr*> opnds;
  uint8 depth = 0;
  bool boundCheck;
};

class MeStmt {
 public:
  explicit MeStmt(const StmtNode *sst) {
    ASSERT(sst != nullptr, "StmtNode nullptr check");
    op = sst->GetOpCode();
    srcPos = sst->GetSrcPos();
    originalId = sst->GetOriginalID();
    meStmtId = sst->GetStmtID();
    if (sst->IsInSafeRegion()) {
      SetInSafeRegion();
    }
  }

  explicit MeStmt(Opcode op1) : meStmtId(StmtNode::stmtIDNext), op(op1) {
    ++StmtNode::stmtIDNext;
  }

  virtual ~MeStmt() = default;

  bool GetIsLive() const {
    return isLive;
  }

  void SetIsLive(bool value) {
    isLive = value;
  }

  virtual void Dump(const IRMap *irMap) const;
  MeStmt *GetNextMeStmt() const;
  virtual size_t NumMeStmtOpnds() const {
    return 0;
  }

  virtual MeExpr *GetOpnd(size_t) const {
    return nullptr;
  }

  virtual void SetOpnd(size_t, MeExpr*) {}

  bool IsReturn() const {
    return op == OP_gosub || op == OP_retsub || op == OP_throw || op == OP_return;
  }

  bool IsCondBr() const {
    return op == OP_brtrue || op == OP_brfalse;
  }

  bool IsAssign() const {
    return op == OP_dassign || op == OP_maydassign || op == OP_iassign || op == OP_regassign || op == OP_piassign;
  }

  virtual MIRType *GetReturnType() const {
    return nullptr;
  }

  void EmitCallReturnVector(CallReturnVector &nRets);
  virtual MapleVector<MustDefMeNode> *GetMustDefList() {
    return nullptr;
  }

  const virtual MapleVector<MustDefMeNode> *GetMustDefList() const {
    return nullptr;
  }

  virtual ScalarMeExpr *GetAssignedLHS() {
    return nullptr;
  }

  const virtual ScalarMeExpr *GetAssignedLHS() const {
    return nullptr;
  }

  virtual MapleMap<OStIdx, ChiMeNode*> *GetChiList() {
    return nullptr;
  }

  virtual const MapleMap<OStIdx, ChiMeNode*> *GetChiList() const {
    return nullptr;
  }

  virtual MapleMap<OStIdx, ScalarMeExpr*> *GetMuList() {
    return nullptr;
  }

  void CopyBase(const MeStmt &meStmt) {
    bb = meStmt.bb;
    srcPos = meStmt.srcPos;
    isLive = meStmt.isLive;
    originalId = meStmt.originalId;
    meStmtId = meStmt.meStmtId;
    stmtAttrs = meStmt.stmtAttrs;
  }

  bool IsTheSameWorkcand(const MeStmt& mestmt) const;
  virtual bool NeedDecref() const {
    return false;
  }

  virtual void EnableNeedDecref() {}

  virtual void DisableNeedDecref() {}

  virtual bool NeedIncref() const {
    return false;
  }

  virtual void SetNeedIncref(bool value = true) {}

  virtual void EnableNeedIncref() {}

  virtual void DisableNeedIncref() {}

  virtual ScalarMeExpr *GetLHS() const {
    return nullptr;
  }

  virtual MeExpr *GetRHS() const {
    return nullptr;
  }

  virtual ScalarMeExpr *GetVarLHS() const {
    return nullptr;
  }

  virtual ScalarMeExpr *GetVarLHS() {
    return nullptr;
  }

  virtual MeExpr *GetLHSRef(bool) {
    return nullptr;
  }

  virtual StmtNode &EmitStmt(MapleAllocator &alloc);

  void RemoveNode() {
    // remove this node from the double link list
    if (prev != nullptr) {
      prev->next = next;
    }
    if (next != nullptr) {
      next->prev = prev;
    }
  }

  void AddNext(MeStmt *node) {
    // add node to the next of this list
    node->next = next;
    node->prev = this;
    if (next != nullptr) {
      next->prev = node;
    }
    next = node;
  }

  void AddPrev(MeStmt *node) {
    // add node to the prev of this list
    node->prev = prev;
    node->next = this;
    if (prev != nullptr) {
      prev->next = node;
    }
    prev = node;
  }

  const BB *GetBB() const { return bb; }
  BB *GetBB() { return bb; }

  void SetBB(BB *curBB) {
    bb = curBB;
  }

  const SrcPosition &GetSrcPosition() const {
    return srcPos;
  }

  void SetSrcPos(SrcPosition pos) {
    srcPos = pos;
  }

  void CopyInfo(const MeStmt &stmt) {
    this->srcPos = stmt.srcPos;
    this->originalId = stmt.originalId;
    this->stmtAttrs = stmt.stmtAttrs;
  }

  void SetPrev(MeStmt *v) {
    prev = v;
  }

  void SetNext(MeStmt *n) {
    next = n;
  }

  MeStmt *GetPrev() const {
    return prev;
  }

  MeStmt *GetNext() const {
    return next;
  }

  Opcode GetOp() const {
    return op;
  }

  void SetOp(Opcode currOp) {
    op = currOp;
  }

  void SetStmtInfoId(size_t index) {
    stmtInfoId = index;
  }

  const size_t GetStmtInfoId() const {
    return stmtInfoId;
  }

  uint32 GetOriginalId() const {
    return originalId;
  }

  void SetOriginalId(uint32 id) {
    originalId = id;
  }

  uint32 GetMeStmtId() const {
    return meStmtId;
  }

  void SetMeStmtId(uint32 id) {
    meStmtId = id;
  }

  bool IsInSafeRegion() const {
    return stmtAttrs.GetAttr(STMTATTR_insaferegion);
  }

  void SetInSafeRegion() {
    stmtAttrs.SetAttr(STMTATTR_insaferegion);
  }

  void CopySafeRegionAttr(const StmtAttrs &stmtAttr) {
    this->stmtAttrs.AppendAttr(stmtAttr.GetTargetAttrFlag(STMTATTR_insaferegion));
  }

  void SetMayTailcall() {
    stmtAttrs.SetAttr(STMTATTR_mayTailcall);
  }

  bool GetMayTailCall() const {
    return stmtAttrs.GetAttr(STMTATTR_mayTailcall);
  }

  const StmtAttrs &GetStmtAttr() const {
    return stmtAttrs;
  }

 protected:
  StmtAttrs stmtAttrs;

 private:
  uint32 originalId = 0xdeadbeef;
  uint32 meStmtId = 0xdeadbeef;
  size_t stmtInfoId = utils::kInvalidIndex;
  Opcode op;
  bool isLive = true;
  BB *bb = nullptr;
  SrcPosition srcPos;
  MeStmt *prev = nullptr;
  MeStmt *next = nullptr;
};

class ChiMeNode {
 public:
  explicit ChiMeNode(MeStmt *meStmt) : base(meStmt) {}

  ~ChiMeNode() = default;

  bool GetIsLive() const {
    return isLive;
  }

  void SetIsLive(bool value) {
    isLive = value;
  }

  ScalarMeExpr *GetRHS() {
    return rhs;
  }

  const ScalarMeExpr *GetRHS() const {
    return rhs;
  }

  void SetRHS(ScalarMeExpr *value) {
    rhs = value;
  }

  ScalarMeExpr *GetLHS() const {
    return lhs;
  }

  void SetLHS(ScalarMeExpr *value) {
    lhs = value;
  }

  void Dump(const IRMap *irMap) const;

  MeStmt *GetBase() const {
    return base;
  }

  void SetBase(MeStmt *value) {
    base = value;
  }

 private:
  ScalarMeExpr *rhs = nullptr;
  ScalarMeExpr *lhs = nullptr;
  MeStmt *base;
  bool isLive = true;
};

class MustDefMeNode {
 public:
  MustDefMeNode(ScalarMeExpr *x, MeStmt *meStmt) : lhs(x), base(meStmt) {
    x->SetDefBy(kDefByMustDef);
    x->SetDefMustDef(*this);
  }

  const ScalarMeExpr *GetLHS() const {
    return lhs;
  }

  ScalarMeExpr *GetLHS() {
    return lhs;
  }

  void SetLHS(ScalarMeExpr *value) {
    lhs = value;
  }

  const MeStmt *GetBase() const {
    return base;
  }

  MeStmt *GetBase() {
    return base;
  }

  void SetBase(MeStmt *value) {
    base = value;
  }

  bool GetIsLive() const {
    return isLive;
  }

  void SetIsLive(bool value) {
    isLive = value;
  }

  MustDefMeNode(const MustDefMeNode &mustDef) {
    lhs = mustDef.lhs;
    base = mustDef.base;
    isLive = mustDef.isLive;
    UpdateLHS(*lhs);
  }

  MustDefMeNode &operator=(const MustDefMeNode &mustDef) {
    if (&mustDef != this) {
      lhs = mustDef.lhs;
      base = mustDef.base;
      isLive = mustDef.isLive;
      UpdateLHS(*lhs);
    }
    return *this;
  }

  void UpdateLHS(ScalarMeExpr &x) {
    lhs = &x;
    x.SetDefBy(kDefByMustDef);
    x.SetDefMustDef(*this);
  }

  ~MustDefMeNode() = default;

  void Dump(const IRMap*) const;

 private:
  ScalarMeExpr *lhs;  // could be var or register, can we make this private?
  MeStmt *base;
  bool isLive = true;
};

class PiassignMeStmt : public MeStmt {
 public:
  explicit PiassignMeStmt(MapleAllocator*)
      : MeStmt(OP_piassign) {
  }
  ~PiassignMeStmt() override = default;

  void SetLHS(VarMeExpr &l) {
    lhs = &l;
  }

  void SetRHS(VarMeExpr &r) {
    rhs = &r;
  }

  void SetGeneratedBy(MeStmt &meStmt) {
    generatedBy = &meStmt;
  }

  VarMeExpr *GetLHS() const override {
    return lhs;
  }

  VarMeExpr *GetRHS() const override {
    return rhs;
  }

  MeStmt *GetGeneratedBy() const {
    return generatedBy;
  }

  void SetIsToken(bool t) {
    isToken = t;
  }

  bool GetIsToken() const {
    return isToken;
  }

  void Dump(const IRMap *irMap) const override;

 private:
  VarMeExpr *rhs = nullptr;
  VarMeExpr *lhs = nullptr;
  MeStmt *generatedBy = nullptr;
  bool isToken = false;
};

class AssignMeStmt : public MeStmt {
 public:
  explicit AssignMeStmt(const StmtNode *stt) : MeStmt(stt) {}

  AssignMeStmt(Opcode op, ScalarMeExpr *theLhs, MeExpr *rhsVal) : MeStmt(op), rhs(rhsVal), lhs(theLhs) {}

  ~AssignMeStmt() override = default;

  size_t NumMeStmtOpnds() const override {
    return kOperandNumUnary;
  }

  MeExpr *GetOpnd(size_t) const override {
    return rhs;
  }

  void SetOpnd(size_t, MeExpr *val) override {
    rhs = val;
  }

  void Dump(const IRMap *irMap) const override;

  bool NeedIncref() const override {
    return needIncref;
  }

  void SetNeedIncref(bool value) override {
    needIncref = value;
  }

  void EnableNeedIncref() override {
    needIncref = true;
  }

  void DisableNeedIncref() override {
    needIncref = false;
  }

  bool NeedDecref() const override {
    return needDecref;
  }

  void SetNeedDecref(bool value) {
    needDecref = value;
  }

  void EnableNeedDecref() override {
    needDecref = true;
  }

  void DisableNeedDecref() override {
    needDecref = false;
  }

  ScalarMeExpr *GetLHS() const override {
    return lhs;
  }

  MeExpr *GetRHS() const override {
    return rhs;
  }

  void SetRHS(MeExpr *value) {
    rhs = value;
  }

  void SetLHS(ScalarMeExpr *value) {
    lhs = value;
  }

  void UpdateLhs(ScalarMeExpr *var) {
    lhs = var;
    var->SetDefBy(kDefByStmt);
    var->SetDefStmt(this);
  }

  StmtNode &EmitStmt(MapleAllocator &alloc) override;

 protected:
  MeExpr *rhs = nullptr;
  ScalarMeExpr *lhs = nullptr;
  bool needIncref = false;  // to be determined by analyzerc phase
  bool needDecref = false;  // to be determined by analyzerc phase

 public:
  bool isIncDecStmt = false;  // has the form of an increment or decrement stmt
};

class DassignMeStmt : public AssignMeStmt {
 public:
  DassignMeStmt(MapleAllocator *alloc, const StmtNode *stt)
      : AssignMeStmt(stt),
        chiList(std::less<OStIdx>(), alloc->Adapter()) {}

  DassignMeStmt(MapleAllocator *alloc, VarMeExpr *theLhs, MeExpr *rhsVal)
      : AssignMeStmt(OP_dassign, theLhs, rhsVal),
        chiList(std::less<OStIdx>(), alloc->Adapter()) {}

  DassignMeStmt(MapleAllocator *alloc, const DassignMeStmt *dass)
      : AssignMeStmt(dass->GetOp(), dass->GetLHS(), dass->GetRHS()),
        chiList(std::less<OStIdx>(), alloc->Adapter()) {}

  ~DassignMeStmt() override = default;

  const MapleMap<OStIdx, ChiMeNode *> *GetChiList() const override {
    return &chiList;
  }

  MapleMap<OStIdx, ChiMeNode *> *GetChiList() override {
    return &chiList;
  }

  void SetChiList(const MapleMap<OStIdx, ChiMeNode*> &value) {
    chiList = value;
  }

  bool Propagated() const {
    return propagated;
  }

  void SetPropagated(bool value) {
    propagated = value;
  }

  bool GetWasMayDassign() const {
    return wasMayDassign;
  }

  void SetWasMayDassign(bool value) {
    wasMayDassign = value;
  }

  void Dump(const IRMap *irMap) const override;

  ScalarMeExpr *GetVarLHS() const override {
    return static_cast<VarMeExpr *>(lhs);
  }

  ScalarMeExpr *GetVarLHS() override {
    return static_cast<VarMeExpr *>(lhs);
  }

  bool GetOmitEmit() const {
    return omitEmit;
  }

  void SetOmitEmit(bool val) {
    omitEmit = val;
  }

  bool GetEmitDassignoff() const {
    return emitDassignoff;
  }

  void SetEmitDassignoff(bool val) {
    emitDassignoff = val;
  }

  MeExpr *GetLHSRef(bool excludeLocalRefVar) override;
  StmtNode &EmitStmt(MapleAllocator &alloc) override;

 private:
  MapleMap<OStIdx, ChiMeNode*> chiList;
  bool propagated = false;     // the RHS has been propagated forward
  bool wasMayDassign = false;  // was converted back to dassign by may2dassign phase
  bool emitDassignoff = false; // Emit Iassignoff instead
  bool omitEmit = false;       // Skip this stmt instead
};

class MaydassignMeStmt : public MeStmt {
 public:
  MaydassignMeStmt(MapleAllocator *alloc, const StmtNode *stt)
      : MeStmt(stt),
        chiList(std::less<OStIdx>(), alloc->Adapter()) {}

  MaydassignMeStmt(MapleAllocator *alloc, MaydassignMeStmt &maydass)
      : MeStmt(maydass.GetOp()), rhs(maydass.GetRHS()), mayDSSym(maydass.GetMayDassignSym()),
        fieldID(maydass.GetFieldID()), chiList(std::less<OStIdx>(), alloc->Adapter()),
        needDecref(maydass.NeedDecref()), needIncref(maydass.NeedIncref()) {}

  ~MaydassignMeStmt() override = default;

  size_t NumMeStmtOpnds() const override {
    return kOperandNumUnary;
  }

  MeExpr *GetOpnd(size_t) const override {
    return rhs;
  }

  void SetOpnd(size_t, MeExpr *val) override {
    rhs = val;
  }

  const MapleMap<OStIdx, ChiMeNode *> *GetChiList() const override {
    return &chiList;
  }

  MapleMap<OStIdx, ChiMeNode *> *GetChiList() override {
    return &chiList;
  }

  void SetChiList(const MapleMap<OStIdx, ChiMeNode*> &value) {
    chiList = value;
  }

  bool NeedDecref() const override {
    return needDecref;
  }

  void EnableNeedDecref() override {
    needDecref = true;
  }

  void DisableNeedDecref() override {
    needDecref = false;
  }

  bool NeedIncref() const override {
    return needIncref;
  }

  void SetNeedIncref(bool val) override {
    needIncref = val;
  }

  void EnableNeedIncref() override {
    needIncref = true;
  }

  void DisableNeedIncref() override {
    needIncref = false;
  }

  OriginalSt *GetMayDassignSym() {
    return mayDSSym;
  }

  const OriginalSt *GetMayDassignSym() const {
    return mayDSSym;
  }

  void SetMayDassignSym(OriginalSt *sym) {
    mayDSSym = sym;
  }

  FieldID GetFieldID() const {
    return fieldID;
  }

  void SetFieldID(FieldID fieldIDVal) {
    fieldID = fieldIDVal;
  }

  void Dump(const IRMap *irMap) const override;
  ScalarMeExpr *GetLHS() const override {
    return chiList.find(mayDSSym->GetIndex())->second->GetLHS();
  }

  MeExpr *GetRHS() const override {
    return rhs;
  }

  void SetRHS(MeExpr *value) {
    rhs = value;
  }

  ScalarMeExpr *GetVarLHS() const override {
    return chiList.find(mayDSSym->GetIndex())->second->GetLHS();
  }

  ScalarMeExpr *GetVarLHS() override {
    return chiList.find(mayDSSym->GetIndex())->second->GetLHS();
  }

  MeExpr *GetLHSRef(bool excludeLocalRefVar) override;
  StmtNode &EmitStmt(MapleAllocator &alloc) override;

 private:
  MeExpr *rhs = nullptr;
  OriginalSt *mayDSSym = nullptr;
  FieldID fieldID = 0;
  MapleMap<OStIdx, ChiMeNode*> chiList;
  bool needDecref = false;  // to be determined by analyzerc phase
  bool needIncref = false;  // to be determined by analyzerc phase
};

class IassignMeStmt : public MeStmt {
 public:
  IassignMeStmt(MapleAllocator *alloc, const StmtNode *stt)
      : MeStmt(stt),
        chiList(std::less<OStIdx>(), alloc->Adapter()) {}

  IassignMeStmt(MapleAllocator*, const IassignMeStmt &iss)
      : MeStmt(iss),
        tyIdx(iss.tyIdx),
        lhsVar(iss.lhsVar),
        rhs(iss.rhs),
        chiList(iss.chiList) {
  }

  IassignMeStmt(MapleAllocator*, TyIdx tidx, IvarMeExpr *l, MeExpr *r, const MapleMap<OStIdx, ChiMeNode*> *clist)
      : MeStmt(OP_iassign), tyIdx(tidx), lhsVar(l), rhs(r), chiList(*clist) {
    l->SetDefStmt(this);
  }

  IassignMeStmt(MapleAllocator *alloc, TyIdx tidx, IvarMeExpr &l, MeExpr &r)
      : MeStmt(OP_iassign), tyIdx(tidx), lhsVar(&l), rhs(&r), chiList(std::less<OStIdx>(), alloc->Adapter()) {
    l.SetDefStmt(this);
  }

  ~IassignMeStmt() override = default;

  TyIdx GetTyIdx() const {
    return tyIdx;
  }

  void SetTyIdx(TyIdx idx) {
    tyIdx = idx;
  }

  size_t NumMeStmtOpnds() const override {
    return kOperandNumBinary;
  }

  MeExpr *GetOpnd(size_t idx) const override {
    return idx == 0 ? lhsVar->GetBase() : rhs;
  }

  void SetOpnd(size_t idx, MeExpr *val) override {
    if (idx == 0) {
      lhsVar->SetBase(val);
    } else {
      rhs = val;
    }
  }

  const MapleMap<OStIdx, ChiMeNode*> *GetChiList() const override {
    return &chiList;
  }

  MapleMap<OStIdx, ChiMeNode*> *GetChiList() override{
    return &chiList;
  }

  void SetChiList(const MapleMap<OStIdx, ChiMeNode*> &value) {
    chiList = value;
  }

  MeExpr *GetLHSRef(bool excludeLocalRefVar) override;
  bool NeedDecref() const override {
    return needDecref;
  }

  void EnableNeedDecref() override {
    needDecref = true;
  }

  void DisableNeedDecref() override {
    needDecref = false;
  }

  bool NeedIncref() const override {
    return needIncref;
  }

  void SetNeedIncref(bool val) override {
    needIncref = val;
  }

  void EnableNeedIncref() override {
    needIncref = true;
  }

  void DisableNeedIncref() override {
    needIncref = false;
  }

  void Dump(const IRMap *irMap) const override;
  MeExpr *GetRHS() const override {
    return rhs;
  }

  void SetRHS(MeExpr *val) {
    rhs = val;
  }

  IvarMeExpr *GetLHSVal() const {
    return lhsVar;
  }

  void SetLHSVal(IvarMeExpr *val) {
    lhsVar = val;
    tyIdx = val->GetTyIdx();
  }

  bool GetOmitEmit() const {
    return omitEmit;
  }

  void SetOmitEmit(bool val) {
    omitEmit = val;
  }

  bool GetEmitIassignoff() const {
    return emitIassignoff;
  }

  void SetEmitIassignoff(bool val) {
    emitIassignoff = val;
  }

  StmtNode &EmitStmt(MapleAllocator &alloc) override;

  void SetExpandFromArrayOfCharFunc(bool flag = true) {
    fromAoCFunc = flag;
  }

  bool IsExpandedFromArrayOfCharFunc() const {
    return fromAoCFunc;
  }

 private:
  TyIdx tyIdx{ 0 };
  IvarMeExpr *lhsVar = nullptr;
  MeExpr *rhs = nullptr;
  MapleMap<OStIdx, ChiMeNode*> chiList;
  bool needDecref = false;  // to be determined by analyzerc phase
  bool needIncref = false;  // to be determined by analyzerc phase
  bool emitIassignoff = false; // Emit Iassignoff instead
  bool omitEmit = false;       // Skip this stmt instead
  bool fromAoCFunc = false;    // Array of char func, defined in <string.h>
};

class NaryMeStmt : public MeStmt {
 public:
  NaryMeStmt(MapleAllocator *alloc, const StmtNode *stt) : MeStmt(stt), opnds(alloc->Adapter()) {}

  NaryMeStmt(MapleAllocator *alloc, Opcode op) : MeStmt(op), opnds(alloc->Adapter()) {}

  NaryMeStmt(MapleAllocator *alloc, const NaryMeStmt *nstmt) : MeStmt(nstmt->GetOp()), opnds(alloc->Adapter()) {
    for (MeExpr *o : nstmt->opnds) {
      opnds.push_back(o);
    }
  }

  ~NaryMeStmt() override = default;

  size_t NumMeStmtOpnds() const override {
    return opnds.size();
  }

  MeExpr *GetOpnd(size_t idx) const override {
    ASSERT(idx < opnds.size(), "out of range in NaryMeStmt::GetOpnd");
    return opnds.at(idx);
  }

  void SetOpnd(size_t idx, MeExpr *val) override {
    opnds[idx] = val;
  }

  const MapleVector<MeExpr*> &GetOpnds() const {
    return opnds;
  }

  void PushBackOpnd(MeExpr *val) {
    opnds.push_back(val);
  }

  void PopBackOpnd() {
    opnds.pop_back();
  }

  void SetOpnds(const MapleVector<MeExpr*> &opndsVal) {
    opnds = opndsVal;
  }

  void EraseOpnds(const MapleVector<MeExpr*>::const_iterator begin, const MapleVector<MeExpr*>::const_iterator end) {
    (void)opnds.erase(begin, end);
  }

  void EraseOpnds(const MapleVector<MeExpr *>::const_iterator it) {
    (void)opnds.erase(it);
  }

  void InsertOpnds(const MapleVector<MeExpr*>::const_iterator begin, MeExpr *expr) {
    (void)opnds.insert(begin, expr);
  }

  void DumpOpnds(const IRMap *irMap) const;
  void Dump(const IRMap *irMap) const override;
  MapleMap<OStIdx, ScalarMeExpr*> *GetMuList() override {
    return nullptr;
  }

  StmtNode &EmitStmt(MapleAllocator &alloc) override;

 private:
  MapleVector<MeExpr*> opnds;
};

class MuChiMePart {
 public:
  explicit MuChiMePart(MapleAllocator *alloc)
      : muList(std::less<OStIdx>(), alloc->Adapter()), chiList(std::less<OStIdx>(), alloc->Adapter()) {}

  virtual MapleMap<OStIdx, ChiMeNode*> *GetChiList() {
    return &chiList;
  }

  void SetChiList(const MapleMap<OStIdx, ChiMeNode*> &value) {
    chiList = value;
  }

  virtual ~MuChiMePart() = default;

 protected:
  MapleMap<OStIdx, ScalarMeExpr*> muList;
  MapleMap<OStIdx, ChiMeNode*> chiList;
};

class AssignedPart {
 public:
  explicit AssignedPart(MapleAllocator *alloc) : mustDefList(alloc->Adapter()) {}

  AssignedPart(MapleAllocator *alloc, MeStmt *defStmt, const MapleVector<MustDefMeNode> &mustDefList)
      : mustDefList(alloc->Adapter()) {
    for (auto &mustDef : mustDefList) {
      auto newMustDefNode = alloc->GetMemPool()->New<MustDefMeNode>(mustDef);
      newMustDefNode->SetBase(defStmt);
      this->mustDefList.push_back(*newMustDefNode);
    }
  }

  virtual ~AssignedPart() = default;

  void DumpAssignedPart(const IRMap *irMap) const;
  VarMeExpr *GetAssignedPartLHSRef(bool excludeLocalRefVar);

 protected:
  MapleVector<MustDefMeNode> mustDefList;
  bool needDecref = false;  // to be determined by analyzerc phase
  bool needIncref = false;  // to be determined by analyzerc phase
};

class CallMeStmt : public NaryMeStmt, public MuChiMePart, public AssignedPart {
 public:
  CallMeStmt(MapleAllocator *alloc, const StmtNode *stt)
      : NaryMeStmt(alloc, stt),
        MuChiMePart(alloc),
        AssignedPart(alloc),
        puIdx(static_cast<const CallNode*>(stt)->GetPUIdx()),
        stmtID(stt->GetStmtID()),
        tyIdx(static_cast<const CallNode*>(stt)->GetTyIdx()) {}

  CallMeStmt(MapleAllocator *alloc, Opcode op)
      : NaryMeStmt(alloc, op), MuChiMePart(alloc), AssignedPart(alloc) {}

  CallMeStmt(MapleAllocator *alloc, Opcode op, PUIdx idx)
      : NaryMeStmt(alloc, op), MuChiMePart(alloc), AssignedPart(alloc), puIdx(idx) {}

  CallMeStmt(MapleAllocator *alloc, const NaryMeStmt *cstmt, PUIdx idx)
      : NaryMeStmt(alloc, cstmt),
        MuChiMePart(alloc),
        AssignedPart(alloc),
        puIdx(idx) {}

  CallMeStmt(MapleAllocator *alloc, const CallMeStmt *cstmt)
      : NaryMeStmt(alloc, cstmt),
        MuChiMePart(alloc),
        AssignedPart(alloc, this, cstmt->mustDefList),
        puIdx(cstmt->GetPUIdx()) {}

  ~CallMeStmt() override = default;

  PUIdx GetPUIdx() const {
    return puIdx;
  }

  void SetPUIdx(PUIdx idx) {
    puIdx = idx;
  }

  TyIdx GetTyIdx() const { return tyIdx; }

  uint32 GetStmtID() const {
    return stmtID;
  }

  void Dump(const IRMap *irMap) const override;
  MapleMap<OStIdx, ScalarMeExpr*> *GetMuList() override {
    return &muList;
  }

  const MapleMap<OStIdx, ChiMeNode*> *GetChiList() const override {
    return &chiList;
  }

  MapleMap<OStIdx, ChiMeNode *> *GetChiList() override {
    return &chiList;
  }

  void SetChiListAndUpdateBase(const MapleMap<OStIdx, ChiMeNode *> &list) {
    chiList = list;
    for (auto &chiNode : chiList) {
      chiNode.second->SetBase(this);
    }
  }

  MapleVector<MustDefMeNode> *GetMustDefList() override {
    return &mustDefList;
  }

  const MapleVector<MustDefMeNode> *GetMustDefList() const override {
    return &mustDefList;
  }

  void SetMustDefListAndUpdateBase(const MapleVector<MustDefMeNode> &list) {
    mustDefList = list;
    for (auto &mustDef : mustDefList) {
      mustDef.SetBase(this);
    }
  }

  MustDefMeNode &GetMustDefListItem(int i) {
    return mustDefList[i];
  }

  size_t MustDefListSize() const {
    return mustDefList.size();
  }

  const ScalarMeExpr *GetAssignedLHS() const override {
    return mustDefList.empty() ? nullptr : mustDefList.front().GetLHS();
  }

  ScalarMeExpr *GetAssignedLHS() override {
    return mustDefList.empty() ? nullptr : mustDefList.front().GetLHS();
  }

  MeExpr *GetLHSRef(bool excludeLocalRefVar) override {
    return GetAssignedPartLHSRef(excludeLocalRefVar);
  }

  ScalarMeExpr *GetVarLHS() override {
    if (mustDefList.empty() || mustDefList.front().GetLHS()->GetMeOp() != kMeOpVar) {
      return nullptr;
    }
    return static_cast<VarMeExpr*>(mustDefList.front().GetLHS());
  }

  bool NeedDecref() const override {
    return needDecref;
  }

  void EnableNeedDecref() override {
    needDecref = true;
  }

  void DisableNeedDecref() override {
    needDecref = false;
  }

  bool NeedIncref() const override {
    return needIncref;
  }

  void EnableNeedIncref() override {
    needIncref = true;
  }

  void DisableNeedIncref() override {
    needIncref = false;
  }

  MIRType *GetReturnType() const override {
    MIRFunction *callee = GlobalTables::GetFunctionTable().GetFunctionFromPuidx(puIdx);
    return callee->GetReturnType();
  }

  const MIRFunction &GetTargetFunction() const;
  MIRFunction &GetTargetFunction();
  StmtNode &EmitStmt(MapleAllocator &alloc) override;

  void SetCallReturn(ScalarMeExpr& curexpr);

 private:
  PUIdx puIdx = 0;
  // Used in trim call graph
  uint32 stmtID = 0;
  TyIdx tyIdx;
};

class IcallMeStmt : public NaryMeStmt, public MuChiMePart, public AssignedPart {
 public:
  IcallMeStmt(MapleAllocator *alloc, const StmtNode *stt)
      : NaryMeStmt(alloc, stt),
        MuChiMePart(alloc),
        AssignedPart(alloc),
        retTyIdx(static_cast<const IcallNode*>(stt)->GetRetTyIdx()),
        stmtID(stt->GetStmtID()) {}

  IcallMeStmt(MapleAllocator *alloc, Opcode op)
      : NaryMeStmt(alloc, op), MuChiMePart(alloc), AssignedPart(alloc) {}

  IcallMeStmt(MapleAllocator *alloc, const IcallMeStmt *cstmt)
      : NaryMeStmt(alloc, cstmt),
        MuChiMePart(alloc),
        AssignedPart(alloc, this, cstmt->mustDefList),
        retTyIdx(cstmt->retTyIdx),
        stmtID(cstmt->stmtID) {}

  IcallMeStmt(MapleAllocator *alloc, const NaryMeStmt *nary, TyIdx idx, uint32 id)
      : NaryMeStmt(alloc, nary),
        MuChiMePart(alloc),
        AssignedPart(alloc),
        retTyIdx(idx),
        stmtID(id) {}

  ~IcallMeStmt() override = default;

  void Dump(const IRMap *irMap) const override;
  MapleMap<OStIdx, ScalarMeExpr*> *GetMuList() override {
    return &muList;
  }

  const MapleMap<OStIdx, ChiMeNode*> *GetChiList() const override {
    return &chiList;
  }

  MapleMap<OStIdx, ChiMeNode *> *GetChiList() override {
    return &chiList;
  }

  MapleVector<MustDefMeNode> *GetMustDefList() override {
    return &mustDefList;
  }

  const MapleVector<MustDefMeNode> *GetMustDefList() const override {
    return &mustDefList;
  }

  const ScalarMeExpr *GetAssignedLHS() const override {
    return mustDefList.empty() ? nullptr : mustDefList.front().GetLHS();
  }

  ScalarMeExpr *GetAssignedLHS() override {
    return mustDefList.empty() ? nullptr : mustDefList.front().GetLHS();
  }

  ScalarMeExpr *GetVarLHS() override {
    if (mustDefList.empty() || mustDefList.front().GetLHS()->GetMeOp() != kMeOpVar) {
      return nullptr;
    }
    return static_cast<VarMeExpr*>(mustDefList.front().GetLHS());
  }

  MeExpr *GetLHSRef(bool excludeLocalRefVar) override {
    return GetAssignedPartLHSRef(excludeLocalRefVar);
  }

  bool NeedDecref() const override {
    return needDecref;
  }

  void EnableNeedDecref() override {
    needDecref = true;
  }

  void DisableNeedDecref() override {
    needDecref = false;
  }

  bool NeedIncref() const override {
    return needIncref;
  }

  void EnableNeedIncref() override {
    needIncref = true;
  }

  void DisableNeedIncref() override {
    needIncref = false;
  }

  MIRType *GetReturnType() const override {
    return GlobalTables::GetTypeTable().GetTypeFromTyIdx(retTyIdx);
  }

  StmtNode &EmitStmt(MapleAllocator &alloc) override;

  TyIdx GetRetTyIdx() const {
    return retTyIdx;
  }

  void SetRetTyIdx(TyIdx idx) {
    retTyIdx = idx;
  }

  uint32 GetStmtID() const {
    return stmtID;
  }

 private:
  // return type for callee
  TyIdx retTyIdx{ 0 };
  // Used in trim call graph
  uint32 stmtID = 0;
};

class IntrinsiccallMeStmt : public NaryMeStmt, public MuChiMePart, public AssignedPart {
 public:
  IntrinsiccallMeStmt(MapleAllocator *alloc, const StmtNode *stt)
      : NaryMeStmt(alloc, stt),
        MuChiMePart(alloc),
        AssignedPart(alloc),
        intrinsic(static_cast<const IntrinsiccallNode*>(stt)->GetIntrinsic()),
        tyIdx(static_cast<const IntrinsiccallNode*>(stt)->GetTyIdx()),
        retPType(stt->GetPrimType()) {}

  IntrinsiccallMeStmt(MapleAllocator *alloc, Opcode op, MIRIntrinsicID id, TyIdx tyid = TyIdx())
      : NaryMeStmt(alloc, op),
        MuChiMePart(alloc),
        AssignedPart(alloc),
        intrinsic(id),
        tyIdx(tyid) {}

  IntrinsiccallMeStmt(MapleAllocator *alloc, const IntrinsiccallMeStmt *intrn)
      : NaryMeStmt(alloc, intrn),
        MuChiMePart(alloc),
        AssignedPart(alloc, this, intrn->mustDefList),
        intrinsic(intrn->GetIntrinsic()),
        tyIdx(intrn->tyIdx),
        retPType(intrn->retPType) {}

  IntrinsiccallMeStmt(MapleAllocator *alloc, const NaryMeStmt *nary, MIRIntrinsicID id, TyIdx idx, PrimType type)
      : NaryMeStmt(alloc, nary),
        MuChiMePart(alloc),
        AssignedPart(alloc),
        intrinsic(id),
        tyIdx(idx),
        retPType(type) {}

  ~IntrinsiccallMeStmt() override = default;

  void Dump(const IRMap *irMap) const override;
  MapleMap<OStIdx, ScalarMeExpr*> *GetMuList() override {
    return &muList;
  }

  const MapleMap<OStIdx, ChiMeNode*> *GetChiList() const override {
    return &chiList;
  }

  MapleMap<OStIdx, ChiMeNode *> *GetChiList() override {
    return &chiList;
  }

  MIRType *GetReturnType() const override {
    if (!mustDefList.empty()) {
      return GlobalTables::GetTypeTable().GetPrimType(mustDefList.front().GetLHS()->GetPrimType());
    }
    return GlobalTables::GetTypeTable().GetPrimType(retPType);
  }

  MapleVector<MustDefMeNode> *GetMustDefList() override {
    return &mustDefList;
  }

  const MapleVector<MustDefMeNode> *GetMustDefList() const override {
    return &mustDefList;
  }

  MustDefMeNode &GetMustDefListItem(int i) {
    return mustDefList[i];
  }

  const ScalarMeExpr *GetAssignedLHS() const override {
    return mustDefList.empty() ? nullptr : mustDefList.front().GetLHS();
  }

  ScalarMeExpr *GetAssignedLHS() override {
    return mustDefList.empty() ? nullptr : mustDefList.front().GetLHS();
  }

  MeExpr *GetLHSRef(bool excludeLocalRefVar) override {
    return GetAssignedPartLHSRef(excludeLocalRefVar);
  }

  bool NeedDecref() const override {
    return needDecref;
  }

  void EnableNeedDecref() override {
    needDecref = true;
  }

  void DisableNeedDecref() override {
    needDecref = false;
  }

  bool NeedIncref() const  override{
    return needIncref;
  }

  void EnableNeedIncref() override {
    needIncref = true;
  }

  void DisableNeedIncref() override {
    needIncref = false;
  }

  StmtNode &EmitStmt(MapleAllocator &alloc) override;

  MIRIntrinsicID GetIntrinsic() const {
    return intrinsic;
  }

  void SetIntrinsic(MIRIntrinsicID id) {
    intrinsic = id;
  }

  TyIdx GetTyIdx() const {
    return tyIdx;
  }

  PrimType GetReturnPrimType() const {
    return retPType;
  }

  IntrinDesc &GetIntrinsicDescription() const {
    return IntrinDesc::intrinTable[intrinsic];
  }

 private:
  MIRIntrinsicID intrinsic;
  TyIdx tyIdx;
  // Used to store return value type
  PrimType retPType = kPtyInvalid;
};

class AsmMeStmt : public NaryMeStmt, public MuChiMePart, public AssignedPart {
 public:
  AsmMeStmt(MapleAllocator *alloc, const StmtNode *stt)
      : NaryMeStmt(alloc, stt),
        MuChiMePart(alloc),
        AssignedPart(alloc),
        asmString(alloc->GetMemPool()),
        inputConstraints(alloc->Adapter()),
        outputConstraints(alloc->Adapter()),
        clobberList(alloc->Adapter()),
        gotoLabels(alloc->Adapter()),
        qualifiers(static_cast<const AsmNode *>(stt)->qualifiers) {
            asmString = static_cast<const AsmNode *>(stt)->asmString;
            inputConstraints = static_cast<const AsmNode *>(stt)->inputConstraints;
            outputConstraints = static_cast<const AsmNode *>(stt)->outputConstraints;
            clobberList = static_cast<const AsmNode *>(stt)->clobberList;
            gotoLabels = static_cast<const AsmNode *>(stt)->gotoLabels;
        }

  AsmMeStmt(MapleAllocator *alloc, const AsmMeStmt *stt)
      : NaryMeStmt(alloc, stt),
        MuChiMePart(alloc),
        AssignedPart(alloc),
        asmString(alloc->GetMemPool()),
        inputConstraints(alloc->Adapter()),
        outputConstraints(alloc->Adapter()),
        clobberList(alloc->Adapter()),
        gotoLabels(alloc->Adapter()),
        qualifiers(stt->qualifiers) {
    asmString = stt->asmString;
    inputConstraints = stt->inputConstraints;
    outputConstraints = stt->outputConstraints;
    clobberList = stt->clobberList;
    gotoLabels = stt->gotoLabels;
  }

  ~AsmMeStmt() override = default;
  void Dump(const IRMap *irMap) const override;
  MapleMap<OStIdx, ScalarMeExpr *> *GetMuList() override {
    return &muList;
  }
  const MapleMap<OStIdx, ChiMeNode*> *GetChiList() const override {
    return &chiList;
  }
  MapleMap<OStIdx, ChiMeNode *> *GetChiList() override {
    return &chiList;
  }
  MapleVector<MustDefMeNode> *GetMustDefList() override {
    return &mustDefList;
  }

  const MapleVector<MustDefMeNode> *GetMustDefList() const override {
    return &mustDefList;
  }

  StmtNode &EmitStmt(MapleAllocator &alloc) override;

  MapleString asmString;
  MapleVector<UStrIdx> inputConstraints;  // length is numOpnds
  MapleVector<UStrIdx> outputConstraints; // length is returnValues.size()
  MapleVector<UStrIdx> clobberList;
  MapleVector<LabelIdx> gotoLabels;
  uint32 qualifiers;
};

class RetMeStmt : public NaryMeStmt {
 public:
  RetMeStmt(MapleAllocator *alloc, const StmtNode *stt)
      : NaryMeStmt(alloc, stt), muList(std::less<OStIdx>(), alloc->Adapter()) {}

  ~RetMeStmt() override = default;

  void Dump(const IRMap *irMap) const override;
  MapleMap<OStIdx, ScalarMeExpr*> *GetMuList() override {
    return &muList;
  }

 private:
  MapleMap<OStIdx, ScalarMeExpr*> muList;
};

// eval, free, decref, incref, decrefreset, assertnonnull, igoto
class UnaryMeStmt : public MeStmt {
 public:
  explicit UnaryMeStmt(const StmtNode *stt) : MeStmt(stt) {}

  explicit UnaryMeStmt(Opcode o) : MeStmt(o) {}

  explicit UnaryMeStmt(const UnaryMeStmt *umestmt) : MeStmt(umestmt->GetOp()), opnd(umestmt->opnd) {}

  ~UnaryMeStmt() override = default;

  UnaryMeStmt(const UnaryMeStmt &other) = default;

  size_t NumMeStmtOpnds() const override {
    return kOperandNumUnary;
  }

  MeExpr *GetOpnd(size_t) const override {
    return opnd;
  }

  void SetOpnd(size_t, MeExpr *val) override {
    opnd = val;
  }

  MeExpr *GetOpnd() const {
    return opnd;
  }

  void SetMeStmtOpndValue(MeExpr *val) {
    opnd = val;
  }

  void Dump(const IRMap *irMap) const override;

  StmtNode &EmitStmt(MapleAllocator &alloc) override;

 private:
  MeExpr *opnd = nullptr;
  UnaryMeStmt &operator=(const UnaryMeStmt &other) = default;
};

class SafetyCallCheckMeStmt {
 public:
  SafetyCallCheckMeStmt(GStrIdx funcNameIdx, size_t paramIndex, GStrIdx stmtFuncNameIdx)
      : funcNameIdx(funcNameIdx), paramIndex(paramIndex), stmtFuncNameIdx(stmtFuncNameIdx) {}

  // for this copy constructor, no need to add explicit
  SafetyCallCheckMeStmt(const SafetyCallCheckMeStmt& stmt)
      : funcNameIdx(stmt.GetFuncNameIdx()), paramIndex(stmt.GetParamIndex()),
        stmtFuncNameIdx(stmt.GetStmtFuncNameIdx()) {}

  SafetyCallCheckMeStmt& operator=(const SafetyCallCheckMeStmt&) = default;

  virtual ~SafetyCallCheckMeStmt() = default;

  const std::string& GetFuncName() const {
    return GlobalTables::GetStrTable().GetStringFromStrIdx(funcNameIdx);
  }

  const std::string& GetStmtFuncName() const {
    return GlobalTables::GetStrTable().GetStringFromStrIdx(stmtFuncNameIdx);
  }

  GStrIdx GetFuncNameIdx() const {
    return funcNameIdx;
  }

  size_t GetParamIndex() const {
    return paramIndex;
  }

  GStrIdx GetStmtFuncNameIdx() const {
    return stmtFuncNameIdx;
  }

 private:
  GStrIdx funcNameIdx;
  size_t paramIndex;
  GStrIdx stmtFuncNameIdx;
};

class SafetyCheckMeStmt {
 public:
  virtual ~SafetyCheckMeStmt() = default;

  const std::string& GetFuncName() const {
    return GlobalTables::GetStrTable().GetStringFromStrIdx(funcNameIdx);
  }

  GStrIdx GetFuncNameIdx() const {
    return funcNameIdx;
  }

 protected:
  explicit SafetyCheckMeStmt(GStrIdx funcNameIdx)
      : funcNameIdx(funcNameIdx) {}

  SafetyCheckMeStmt(const SafetyCheckMeStmt& stmt)
      : funcNameIdx(stmt.GetFuncNameIdx()) {}

  SafetyCheckMeStmt& operator=(const SafetyCheckMeStmt& stmt) {
    if (this != &stmt) {
      funcNameIdx = stmt.GetFuncNameIdx();
    }
    return *this;
  }
  SafetyCheckMeStmt() {}

 private:
  GStrIdx funcNameIdx;
};

class AssertNonnullMeStmt : public UnaryMeStmt, public SafetyCheckMeStmt {
 public:
  explicit AssertNonnullMeStmt(const AssertNonnullStmtNode *stt)
      : UnaryMeStmt(stt), SafetyCheckMeStmt(stt->GetFuncNameIdx()) {}
  explicit AssertNonnullMeStmt(const UnaryStmtNode *stt)
      : UnaryMeStmt(stt), SafetyCheckMeStmt() {}
  ~AssertNonnullMeStmt() override = default;
  StmtNode &EmitStmt(MapleAllocator &alloc) override;
};

class CallAssertNonnullMeStmt : public UnaryMeStmt, public SafetyCallCheckMeStmt {
 public:
  explicit CallAssertNonnullMeStmt(const CallAssertNonnullStmtNode *stt)
      : UnaryMeStmt(stt), SafetyCallCheckMeStmt(stt->GetFuncNameIdx(), stt->GetParamIndex(),
                                                stt->GetStmtFuncNameIdx()) {}

  CallAssertNonnullMeStmt(Opcode o, GStrIdx funcNameIdx, size_t paramIndex, GStrIdx stmtFuncNameIdx)
      : UnaryMeStmt(o), SafetyCallCheckMeStmt(funcNameIdx, paramIndex, stmtFuncNameIdx) {}

  CallAssertNonnullMeStmt(const CallAssertNonnullMeStmt &stt)
      : UnaryMeStmt(stt), SafetyCallCheckMeStmt(static_cast<const SafetyCallCheckMeStmt&>(stt)) {}

  explicit CallAssertNonnullMeStmt(const CallAssertNonnullMeStmt *stt)
      : UnaryMeStmt(*stt), SafetyCallCheckMeStmt(static_cast<const SafetyCallCheckMeStmt&>(*stt)) {}

  ~CallAssertNonnullMeStmt() override = default;

  StmtNode &EmitStmt(MapleAllocator &alloc) override;

 private:
  CallAssertNonnullMeStmt& operator=(const CallAssertNonnullMeStmt &condGoto) = delete;
};

class AssertBoundaryMeStmt : public NaryMeStmt, public SafetyCheckMeStmt {
 public:
  AssertBoundaryMeStmt(MapleAllocator *alloc, const AssertBoundaryStmtNode *stt)
      : NaryMeStmt(alloc, stt), SafetyCheckMeStmt(stt->GetFuncNameIdx()) {}
  AssertBoundaryMeStmt(MapleAllocator *alloc, const AssertBoundaryMeStmt &stt)
      : NaryMeStmt(alloc, static_cast<const NaryMeStmt*>(&stt)),
        SafetyCheckMeStmt(static_cast<const SafetyCheckMeStmt&>(stt)) {}
  ~AssertBoundaryMeStmt() override = default;
  StmtNode &EmitStmt(MapleAllocator &alloc) override;
};

class CallAssertBoundaryMeStmt : public NaryMeStmt, public SafetyCallCheckMeStmt {
 public:
  CallAssertBoundaryMeStmt(MapleAllocator *alloc, const CallAssertBoundaryStmtNode *stt)
      : NaryMeStmt(alloc, stt), SafetyCallCheckMeStmt(stt->GetFuncNameIdx(), stt->GetParamIndex(),
                                                      stt->GetStmtFuncNameIdx()) {}
  CallAssertBoundaryMeStmt(MapleAllocator *alloc, const CallAssertBoundaryMeStmt &stt)
      : NaryMeStmt(alloc, static_cast<const NaryMeStmt*>(&stt)),
        SafetyCallCheckMeStmt(static_cast<const SafetyCallCheckMeStmt&>(stt)) {}
  ~CallAssertBoundaryMeStmt() override = default;
  StmtNode &EmitStmt(MapleAllocator &alloc) override;
};

class GotoMeStmt : public MeStmt {
 public:
  explicit GotoMeStmt(const StmtNode *stt) : MeStmt(stt), offset(static_cast<const GotoNode*>(stt)->GetOffset()) {}
  explicit GotoMeStmt(const GotoMeStmt &condGoto) : MeStmt(MeStmt(condGoto.GetOp())), offset(condGoto.GetOffset()) {}
  GotoMeStmt& operator=(const GotoMeStmt &condGoto) = default;

  explicit GotoMeStmt(uint32 o) : MeStmt(OP_goto), offset(o) {}

  ~GotoMeStmt() override = default;

  uint32 GetOffset() const {
    return offset;
  }

  void SetOffset(uint32 o) {
    offset = o;
  }

  StmtNode &EmitStmt(MapleAllocator &alloc) override;

 private:
  uint32 offset;  // the label
};

class CondGotoMeStmt : public UnaryMeStmt {
 public:
  explicit CondGotoMeStmt(const StmtNode *stt)
      : UnaryMeStmt(stt), offset(static_cast<const CondGotoNode*>(stt)->GetOffset()),
        branchProb(static_cast<const CondGotoNode*>(stt)->GetBranchProb()) {}

  explicit CondGotoMeStmt(const CondGotoMeStmt &condGoto)
      : UnaryMeStmt(static_cast<const UnaryMeStmt*>(&condGoto)), offset(condGoto.GetOffset()),
        branchProb(condGoto.branchProb) {}

  CondGotoMeStmt(const UnaryMeStmt &unaryMeStmt, uint32 o)
      : UnaryMeStmt(&unaryMeStmt), offset(o) {}

  ~CondGotoMeStmt() override = default;

  uint32 GetOffset() const {
    return offset;
  }

  void SetOffset(uint32 currOffset) {
    offset = currOffset;
  }

  bool IsBranchProbValid() const {
    return branchProb > 0 && branchProb < CondGotoNode::probAll;
  }

  int32 GetBranchProb() const {
    return branchProb;
  }

  void SetBranchProb(int32 prob) {
    branchProb = prob;
  }

  void ReverseBranchProb() {
    if (IsBranchProbValid()) {
      branchProb = CondGotoNode::probAll - branchProb;
    }
  }

  void InvalidateBranchProb() {
    if (IsBranchProbValid()) {
      branchProb = -1;
    }
  }

  void Dump(const IRMap *irMap) const override;
  StmtNode &EmitStmt(MapleAllocator &alloc) override;

 private:
  CondGotoMeStmt& operator=(const CondGotoMeStmt &condGoto) = delete;
  uint32 offset;  // the label
  int32 branchProb = -1;  // branch probability, a negative number indicates that the probability is invalid
};

class JsTryMeStmt : public MeStmt {
 public:
  explicit JsTryMeStmt(const StmtNode *stt)
      : MeStmt(stt),
        catchOffset(static_cast<const JsTryNode*>(stt)->GetCatchOffset()),
        finallyOffset(static_cast<const JsTryNode*>(stt)->GetFinallyOffset()) {}

  ~JsTryMeStmt() override = default;

  StmtNode &EmitStmt(MapleAllocator &alloc) override;

 private:
  uint16 catchOffset;
  uint16 finallyOffset;
};

class TryMeStmt : public MeStmt {
 public:
  TryMeStmt(MapleAllocator *alloc, const StmtNode *stt) : MeStmt(stt), offsets(alloc->Adapter()) {}

  ~TryMeStmt() override = default;

  void OffsetsPushBack(LabelIdx curr) {
    offsets.push_back(curr);
  }

  const MapleVector<LabelIdx> &GetOffsets() const {
    return offsets;
  }

  StmtNode &EmitStmt(MapleAllocator &alloc) override;

 private:
  MapleVector<LabelIdx> offsets;
};

class CatchMeStmt : public MeStmt {
 public:
  CatchMeStmt(MapleAllocator *alloc, const StmtNode *stt) : MeStmt(stt), exceptionTyIdxVec(alloc->Adapter()) {
    for (auto it : static_cast<const CatchNode*>(stt)->GetExceptionTyIdxVec()) {
      exceptionTyIdxVec.push_back(it);
    }
  }

  ~CatchMeStmt() override = default;

  StmtNode &EmitStmt(MapleAllocator &alloc) override;
  const MapleVector<TyIdx> &GetExceptionTyIdxVec() const {
    return exceptionTyIdxVec;
  }
 private:
  MapleVector<TyIdx> exceptionTyIdxVec;
};

class CppCatchMeStmt : public MeStmt {
 public:
  TyIdx exceptionTyIdx;

  CppCatchMeStmt(const MapleAllocator *alloc, const StmtNode *stt) : MeStmt(stt) {
    (void)alloc;
  }

  ~CppCatchMeStmt() override = default;
  StmtNode &EmitStmt(MapleAllocator &alloc) override;
};

class SwitchMeStmt : public UnaryMeStmt {
 public:
  SwitchMeStmt(MapleAllocator *alloc, StmtNode *stt)
      : UnaryMeStmt(stt),
        defaultLabel(static_cast<SwitchNode*>(stt)->GetDefaultLabel()),
        switchTable(alloc->Adapter()) {
    switchTable = static_cast<SwitchNode*>(stt)->GetSwitchTable();
  }

  ~SwitchMeStmt() override = default;

  LabelIdx GetDefaultLabel() const {
    return defaultLabel;
  }

  void SetDefaultLabel(LabelIdx curr) {
    defaultLabel = curr;
  }

  void SetCaseLabel(size_t caseIdx, LabelIdx label) {
    switchTable[caseIdx].second = label;
  }

  CaseVector &GetSwitchTable() {
    return switchTable;
  }

  const CaseVector &GetSwitchTable() const {
    return switchTable;
  }

  void Dump(const IRMap *irMap) const override;
  StmtNode &EmitStmt(MapleAllocator &alloc) override;

 private:
  LabelIdx defaultLabel;
  CaseVector switchTable;
};

class CommentMeStmt : public MeStmt {
 public:
  CommentMeStmt(const MapleAllocator *alloc, const StmtNode *stt) : MeStmt(stt), comment(alloc->GetMemPool()) {
    comment = static_cast<const CommentNode*>(stt)->GetComment();
  }

  ~CommentMeStmt() override = default;

  StmtNode &EmitStmt(MapleAllocator &alloc) override;
  const MapleString& GetComment() { return comment; }
  const MapleString& GetComment() const { return comment; }
 private:
  MapleString comment;
};

class WithMuMeStmt : public MeStmt {
 public:
  WithMuMeStmt(MapleAllocator *alloc, const StmtNode *stt)
      : MeStmt(stt), muList(std::less<OStIdx>(), alloc->Adapter()) {}

  ~WithMuMeStmt() override = default;

  MapleMap<OStIdx, ScalarMeExpr*> *GetMuList() override {
    return &muList;
  }

  const MapleMap<OStIdx, ScalarMeExpr*> *GetMuList() const {
    return &muList;
  }

 private:
  MapleMap<OStIdx, ScalarMeExpr*> muList;
};

class GosubMeStmt : public WithMuMeStmt {
 public:
  GosubMeStmt(MapleAllocator *alloc, const StmtNode *stt)
      : WithMuMeStmt(alloc, stt), offset(static_cast<const GotoNode*>(stt)->GetOffset()) {}

  ~GosubMeStmt() override = default;

  void Dump(const IRMap *irMap) const override;
  StmtNode &EmitStmt(MapleAllocator &alloc) override;

 private:
  uint32 offset;  // the label
};

class ThrowMeStmt : public WithMuMeStmt {
 public:
  ThrowMeStmt(MapleAllocator *alloc, const StmtNode *stt) : WithMuMeStmt(alloc, stt) {}

  ~ThrowMeStmt() override = default;

  size_t NumMeStmtOpnds() const override {
    return kOperandNumUnary;
  }

  MeExpr *GetOpnd(size_t) const override {
    return opnd;
  }

  void SetOpnd(size_t, MeExpr *val) override {
    opnd = val;
  }

  MeExpr *GetOpnd() const {
    return opnd;
  }

  void SetMeStmtOpndValue(MeExpr *val) {
    opnd = val;
  }

  void Dump(const IRMap *irMap) const override;
  StmtNode &EmitStmt(MapleAllocator &alloc) override;

 private:
  MeExpr *opnd = nullptr;
};

class SyncMeStmt : public NaryMeStmt, public MuChiMePart {
 public:
  SyncMeStmt(MapleAllocator *alloc, const StmtNode *stt) : NaryMeStmt(alloc, stt), MuChiMePart(alloc) {}

  ~SyncMeStmt() override = default;

  void Dump(const IRMap *irMap) const override;
  MapleMap<OStIdx, ScalarMeExpr *> *GetMuList() override {
    return &muList;
  }

  const MapleMap<OStIdx, ChiMeNode*> *GetChiList() const override {
    return &chiList;
  }

  MapleMap<OStIdx, ChiMeNode *> *GetChiList() override {
    return &chiList;
  }
};

MapleMap<OStIdx, ChiMeNode*> *GenericGetChiListFromVarMeExpr(ScalarMeExpr &expr);
void DumpMuList(const IRMap *irMap, const MapleMap<OStIdx, ScalarMeExpr*> &muList);
void DumpChiList(const IRMap *irMap, const MapleMap<OStIdx, ChiMeNode*> &chiList);
class DumpOptions {
 public:
  static bool GetSimpleDump() {
    return simpleDump;
  }

  static int GetDumpVsyNum() {
    return dumpVsymNum;
  }

 private:
  static bool simpleDump;
  static int dumpVsymNum;
};
}  // namespace maple
#define LOAD_SAFE_CAST_FOR_ME_EXPR
#define LOAD_SAFE_CAST_FOR_ME_STMT
#include "me_safe_cast_traits.def"
#endif  // MAPLE_ME_INCLUDE_ME_IR_H
