/*
 * Copyright (c) [2022] Huawei Technologies Co.,Ltd.All rights reserved.
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
#ifndef MAPLE_MPL2MPL_INCLUDE_INLINE_SUMMARY_H
#define MAPLE_MPL2MPL_INCLUDE_INLINE_SUMMARY_H
#include "me_function.h"
#include "cast_opt.h"
#include "stmt_cost_analyzer.h"
#include "me_loop_analysis.h"
#include "me_predict.h"

#include <optional>

namespace maple {
enum ExprKind : uint32 {
  kExprKindParam = 0x01,        // expr is a unmodified parameter
  kExprKindConst = 0x02,        // expr is a constant
  kExprKindConstNumber = 0x04,  // expr is a constant with number type (integer, float or double)
                                // a kExprKindConstNumber must be a kExprKindConst
};

enum ExprBoolResult {
  kExprResUnknown,
  kExprResTrue,
  kExprResFalse
};

constexpr double kColdFreqPercent = 0.01;
constexpr int32 kCallFreqBase = 1000;
constexpr int32 kCallFreqMax = 100000;
constexpr int32 kCallFreqDefault = 1;   // based on kCallFreqBase

inline double CallFreqPercent(int32 callFreq) {
  return static_cast<double>(callFreq) / static_cast<double>(kCallFreqBase);
}

using NumInsns = int32;
using NumCycles = double;
class InlineCost {
 public:
  InlineCost() = default;
  InlineCost(NumInsns numInsns, NumCycles numCycles) : insns(numInsns), cycles(numCycles) {}

  void Reset(NumInsns numInsns, NumCycles numCycles) {
    insns = numInsns;
    cycles = numCycles;
  }

  NumInsns GetInsns() const {
    return insns;
  }

  NumCycles GetCycles() const {
    return cycles;
  }

  void AddInsns(NumInsns detInsns) {
    insns += detInsns;
  }

  void AddCycles(NumCycles detCycles) {
    cycles += detCycles;
  }

  void Add(NumInsns detInsns, NumCycles detCycles) {
    insns += detInsns;
    cycles += detCycles;
  }

  void Add(const InlineCost &rhs) {
    insns += rhs.insns;
    cycles += rhs.cycles;
  }

  void Add(const InlineCost &rhs, int32 frequency) {
    insns += rhs.insns;
    auto detTime = rhs.cycles;
    if (frequency > 0) {
      detTime *= CallFreqPercent(frequency);
    }
    cycles += detTime;
  }

  InlineCost Scale(int32 frequency) const {
    InlineCost cost(0, 0);
    cost.Add(*this, frequency);
    return cost;
  }

 private:
  NumInsns insns = 0;     // The dimension is number of instructions
  NumCycles cycles = 0;   // The dimension is cycles
};

ExprBoolResult GetReverseExprBoolResult(ExprBoolResult res);
void DumpStmtSafe(const StmtNode &stmtNode, MIRFunction &func);
std::pair<PrimType, int64> TryFoldConst(BaseNode &expr);
std::optional<uint32> GetZeroVersionParamIndex(const MeExpr &expr, MeFunction &func);
bool IsExprSpecifiedKinds(const MeExpr &expr, uint32 kinds, MeFunction &func);
bool IsExprOnlyComposedOf(const MeExpr &expr, uint32 kinds, MeFunction &func);
MIRConst *EvaluateMeExpr(MeExpr &meExpr, MeFunction &func, MapleAllocator &alloc);
InlineSummary *GetSummaryByPuIdx(uint32 puIdx);
int32 CostSize2NumInsn(int32 costSize);
InlineCost GetCondInlineCost(const MIRFunction &caller, MIRFunction &callee, const CallNode &callStmt);

// Real argument info of a callsite, including the following info:
// (1) real argument value range: [lower, upper], it is a single value if lower == upper. lower and upper
// will be reinterpreted according to `valueType`.
// (2) pass through info of caller's formals
class ArgInfo {
 public:
  // ctor for value range argInfo
  ArgInfo(MIRConst *low, MIRConst *up) : lower(low), upper(up) {}

  // ctor for pass through argInfo
  explicit ArgInfo(int32 index) : callerFormalIdx(index) {}

  bool IsSingleValue() const {
    return lower == upper && lower != nullptr;
  }

  const MIRConst *GetLower() const {
    return lower;
  }

  MIRConst *GetLower() {
    return lower;
  }

  const MIRConst *GetUpper() const {
    return upper;
  }
  MIRConst *GetUpper() {
    return upper;
  }

  int32 GetCallerFormalIdx() const {
    return callerFormalIdx;
  }

  bool IsParamPassThrough() const {
    return callerFormalIdx != -1;
  }

 private:
  // (1) value range
  // Init with an invalid range
  MIRConst *lower = nullptr;
  MIRConst *upper = nullptr;
  // (2) pass through
  // callerFormalIdx means which caller's formal is passed to current arg, -1 is invalid
  // Example: foo(int a, int b) { bar(b); }
  // Then we have a param index mapping: for 'bar(b)' from 0 (call argument index) to 1 (caller parameter index)
  int32 callerFormalIdx = -1;
};

using ArgInfoVec = MapleVector<ArgInfo*>;  // index is real arguments index

// A simple expression representation for condition expr (in the condgoto stmt and the switch stmt).
// LiteExpr is a lightweight expression used to pass information between MEIR and mapleIR.
// Currently it is used to pass conditional expr for inline summary. The memory cost is quite considerable
// because all function's inline summary will occupy memory at the same time. Using LiteExpr is good for saving memory
// Leaf nodes of LiteExpr must be "constant expr" or "parameter expr" currently.
class LiteExpr {
 public:
  static LiteExpr *TryBuildLiteExpr(MeExpr &expr, uint32 kinds, MeFunction &func, MapleAllocator &alloc,
      uint32 *paramsUsed = nullptr);
  static LiteExpr &DoBuildLiteExpr(MeExpr &expr, MeFunction &func, MapleAllocator &alloc, uint32 *paramsUsed);

  // Construct parameter liteExpr
  // we use OP_dread to mark parameter liteExpr
  LiteExpr(MapleAllocator &alloc, PrimType primType, uint32 paramIndex)
      : op(OP_dread), type(primType), opnds(alloc.Adapter()) {
    data.paramIdx = paramIndex;
  }

  // Construct constval liteExpr
  LiteExpr(MapleAllocator &alloc, PrimType primType, MIRConst *mirConst)
      : op(OP_constval), type(primType), opnds(alloc.Adapter()) {
    data.cst = mirConst;
  }

  // For other liteExpr, use SetXXX member function to init `data` field
  LiteExpr(MapleAllocator &alloc, Opcode opcode, PrimType primType)
      : op(opcode), type(primType), opnds(alloc.Adapter()) {}

  LiteExpr(MapleAllocator &alloc, const LiteExpr &rhs)
      :op(rhs.op), type(rhs.type), opnds(rhs.opnds, alloc.Adapter()) {
    data = rhs.data;
  }

  void Dump(uint32 indent = 0) const;
  void DumpWithoutEndl(uint32 indent = 0) const;
  LiteExpr *Clone(MapleAllocator &newAlloc) const;
  BaseNode *ConvertToMapleIR(MapleAllocator &alloc, const ArgInfoVec *argInfoVec) const;
  MIRConst *Evaluate(MapleAllocator &alloc, const ArgInfoVec *argInfoVec) const;
  ExprBoolResult EvaluateToBool(MapleAllocator &alloc, const ArgInfoVec *argInfoVec) const;

  void RemapParamIdx(const std::vector<int32> &paramMapping) {
    if (IsParam()) {
      uint32 oldIdx = GetParamIndex();
      if (oldIdx >= paramMapping.size()) {  // index out of range, no need to remap
        return;
      }
      int32 newIdx = paramMapping[oldIdx];
      if (newIdx != -1) {
        SetParamIndex(static_cast<uint32>(newIdx));
      }
      return;
    }
    for (auto *opnd : opnds) {
      opnd->RemapParamIdx(paramMapping);
    }
  }

  PrimType GetType() const {
    return type;
  }

  bool IsConst() const {
    return op == OP_constval;
  }

  bool IsParam() const {
    return op == OP_dread;
  }

  bool IsCmp() const {
    return CastOpt::IsCompareOp(op);
  }

  bool IsCvt() const {
    return op == OP_cvt;
  }

  bool IsExtractbits() const {
    return op == OP_zext || op == OP_sext || op == OP_extractbits;
  }

  int64 GetInt() const {
    ASSERT(IsConst(), "must be");
    ASSERT(IsPrimitiveInteger(type), "must be");
    return static_cast<MIRIntConst*>(data.cst)->GetSXTValue();
  }

  float GetFloat() const {
    ASSERT(IsConst(), "must be");
    ASSERT(type == PTY_f32, "must be");
    return static_cast<MIRFloatConst*>(data.cst)->GetValue();
  }

  double GetDouble() const {
    ASSERT(IsConst(), "must be");
    ASSERT(type == PTY_f64, "must be");
    return static_cast<MIRDoubleConst*>(data.cst)->GetValue();
  }

  uint32 GetParamIndex() const {
    ASSERT(IsParam(), "must be");
    return data.paramIdx;
  }

  void SetParamIndex(uint32 newParamIdx) {
    ASSERT(IsParam(), "must be");
    data.paramIdx = newParamIdx;
  }

  MIRConst *GetMIRConst() {
    return data.cst;
  }

  PrimType GetOpndType() const {
    return data.opndType;
  }

  void SetOpndType(PrimType ty) {
    data.opndType = ty;
  }

  uint8 GetBitsOffset() const {
    return data.bitData.bitsOffset;
  }

  uint8 GetBitsSize() const {
    return data.bitData.bitsSize;
  }

  void SetBitsOffset(uint8 offset) {
    data.bitData.bitsOffset = offset;
  }

  void SetBitsSize(uint8 size) {
    data.bitData.bitsSize = size;
  }

  MapleVector<LiteExpr*> &GetOpnds() {
    return opnds;
  }

 private:
  Opcode op = OP_undef;
  PrimType type = PTY_unknown;
  // `data` field has different meaning for different op, see the following comment:
  union {
    MIRConst *cst = nullptr;  // (1) mirConst for const expr with opcode OP_constval
    uint32 paramIdx;          // (2) parameter index for parameter expr with opcode OP_dread
    PrimType opndType;        // (3) opngType for TypeCvtNode and CompareNode
    struct {                  // (4) bitData for ExtractbitsNode
      uint8 bitsOffset;
      uint8 bitsSize;
    } bitData;
  } data;
  MapleVector<LiteExpr*> opnds;
};

class Condition {
 public:
  explicit Condition(uint32 index, LiteExpr *liteExpr, bool rev, uint32 params)
      : idx(index),
        paramsUsed(params),
        reverse(rev),
        expr(liteExpr) {}

  Condition *Clone(MapleAllocator &newAlloc) const {
    auto *exprCloned = expr->Clone(newAlloc);
    return newAlloc.New<Condition>(idx, exprCloned, reverse, paramsUsed);
  }

  ExprBoolResult EvaluateToBool(MapleAllocator &alloc, const ArgInfoVec *argInfoVec) const {
    ExprBoolResult res = expr->EvaluateToBool(alloc, argInfoVec);
    if (IsReverse()) {
      res = GetReverseExprBoolResult(res);
    }
    return res;
  }

  void Dump() const;

  uint32 GetIndex() const {
    return idx;
  }

  LiteExpr *GetExpr() {
    return expr;
  }

  bool IsReverse() const {
    return reverse;
  }

  bool Equal(const Condition &rhs) const {
    if (expr == rhs.expr && reverse == rhs.reverse) {
      return true;
    }
    return false;
  }

  bool IsReverseWithCondition(const Condition &rhs) const {
    if (expr == rhs.expr && reverse != rhs.reverse) {
      return true;
    }
    return false;
  }

  uint32 GetParamsUsed() const {
    return paramsUsed;
  }

 private:
  const uint32 idx = 0;  // position in summary conditions vector
  uint32 paramsUsed = 0; // This is a bit map, indicates which formal parameters are used
  bool reverse = false;
  LiteExpr *expr = nullptr;
};

using Assert = uint64;
constexpr Assert kAssertOne = 1;
constexpr uint32 kNumBitsPerByte = 8;
constexpr uint32 kMaxNumCondition = sizeof(Assert) * kNumBitsPerByte;
constexpr uint32 kCondIdxNotInline = 1;
constexpr uint32 kCondIdxRealBegin = 2;
constexpr uint32 kCondIdxOverflow = kMaxNumCondition;

class Predicate {
 public:
  static const Predicate *TruePredicate();
  static const Predicate *FalsePredicate();
  static const Predicate *NotInlinePredicate();
  static const Predicate *And(const Predicate &a, const Predicate &b, MapleAllocator &alloc);
  static const Predicate *Or(const Predicate &a, const Predicate &b, const MapleVector<Condition*> &conditions,
      MapleAllocator &alloc);

  explicit Predicate(MapleAllocator &alloc) : asserts(alloc.Adapter()) {}

  Predicate(std::initializer_list<Assert> assertList, MapleAllocator &alloc)
      : asserts(assertList, alloc.Adapter()) {}

  Predicate(const MapleSet<Assert> &assertList, MapleAllocator &alloc)
      : Predicate(alloc) {
    for (auto assert : assertList) {
      (void)asserts.insert(assert);
    }
  }

  Predicate(const std::vector<Assert> &assertList, MapleAllocator &alloc)
      : Predicate(alloc) {
    for (auto assert : assertList) {
      (void)asserts.insert(assert);
    }
  }

  Predicate(uint32 condIdx, MapleAllocator &alloc)
      : Predicate(alloc) {
    (void)asserts.insert(kAssertOne << condIdx);
  }

  bool operator==(const Predicate &rhs) const {
    return asserts == rhs.asserts;
  }

  const Predicate *Clone(MapleAllocator &newAlloc) const;

  // True predicate asserts: { 0 }
  bool IsTruePredicate() const {
    return asserts.size() == 1 && *asserts.begin() == 0;
  }

  // False predicate asserts: { 1 }
  bool IsFalsePredicate() const {
    return asserts.size() == 1 && *asserts.begin() == 1;
  }

  // NotInline predicate asserts: { 2 }
  bool IsNotInlinePredicate() const {
    const Assert notInlineAssertVal = 2;
    return asserts.size() == 1 && *asserts.begin() == notInlineAssertVal;
  }

  MapleSet<Assert> &GetAsserts() {
    return asserts;
  }

  void AddAssert(Assert newAssert, const MapleVector<Condition*> &conditions);

  // predicateResult = assertResult0 && assertResult1 && ... && assertResultN
  ExprBoolResult Evaluate(const std::vector<ExprBoolResult> &condResultVec) const {
    if (IsTruePredicate()) {
      return kExprResTrue;
    }
    if (IsFalsePredicate()) {
      return kExprResFalse;
    }
    bool foundUnknown = false;
    for (auto assert : asserts) {
      auto res = EvaluateAssert(assert, condResultVec);
      if (res == kExprResFalse) {
        return res;
      } else if (res == kExprResUnknown) {
        foundUnknown = true;
      }
    }
    if (foundUnknown) {
      return kExprResUnknown;
    }
    return kExprResTrue;
  }

  const Predicate *RemapCondIdx(const std::array<int8, kMaxNumCondition> &oldCondIdx2New,
      MapleAllocator &alloc) const {
    if (IsTruePredicate() || IsFalsePredicate()) {
      return this;
    }
    std::vector<Assert> newAssertVec;
    for (auto assert : asserts) {
      Assert newAssert = 0;
      for (uint32 j = 0; j < kMaxNumCondition; ++j) {
        if ((assert & (kAssertOne << j)) == 0) {
          continue;
        }
        auto newCondIdx = oldCondIdx2New[j];
        if (newCondIdx == static_cast<int32>(kCondIdxOverflow)) {
          // condition out of range, we treat it as true condition
          newAssert = 0;
          break;
        }
        newAssert |= (kAssertOne << newCondIdx);
      }
      if (newAssert != 0) {
        newAssertVec.push_back(newAssert);
      }
    }
    if (newAssertVec.empty()) {
      return Predicate::TruePredicate();
    }
    return alloc.New<Predicate>(newAssertVec, alloc);
  }

  Assert GetCondsUsed() const {
    Assert condsUsed = 0;
    for (auto assert : asserts) {
      condsUsed |= assert;
    }
    return condsUsed;
  }

  uint32 GetParamsUsed(const MapleVector<Condition*> &conditions) const {
    uint32 paramsUsed = 0;
    for (auto assert : asserts) {
      // Skip the first element that is a placeholder for falsePredicate and the second
      // element for notInline
      for (uint32 i = kCondIdxRealBegin; i < kMaxNumCondition; ++i) {
        if ((assert & (kAssertOne << i)) == 0) {
          continue;
        }
        paramsUsed |= conditions[i]->GetParamsUsed();
      }
    }
    return paramsUsed;
  }

  bool AreAllUsedParamsInParamMapping(const std::vector<int32> &paramMapping,
      const MapleVector<Condition*> &conditions) const;
  void Dump(const MapleVector<Condition*> &conditions) const;

 private:
  // assertResult = condResult0 || condResult1 || ... || condResultN
  ExprBoolResult EvaluateAssert(Assert assert, const std::vector<ExprBoolResult> &condResultVec) const {
    bool foundUnknown = false;
    for (uint32 i = 0; i < kMaxNumCondition; ++i) {
      if ((assert & (kAssertOne << i)) == 0) {
        continue;
      }
      auto res = condResultVec[i];
      if (res == kExprResTrue) {
        return res;
      } else if (res == kExprResUnknown) {
        foundUnknown = true;
      }
    }
    if (foundUnknown) {
      return kExprResUnknown;
    }
    return kExprResFalse;
  }

  MapleSet<Assert> asserts;
};

struct InlineEdgeSummary {
  InlineEdgeSummary(const Predicate *pred, int32 freq, int32 stmtSz, int32 stmtTm)
      : predicate(pred), frequency(freq), callCost(stmtSz, stmtTm) {}
  const Predicate *predicate = nullptr;  // predicate of the bb that contains the callStmt
  int32 frequency = -1;   // frequency of the bb that contains the callStmt, -1 means invalid frequency
  InlineCost callCost;
};

class InlineSummary {
 public:
  InlineSummary(MapleAllocator &allocater, MIRFunction *f)
      : summaryAlloc(allocater),
        func(f),
        conditions(kCondIdxRealBegin, nullptr, summaryAlloc.Adapter()), // The 1st is a placeholder for falsePredicate
        costTable(summaryAlloc.Adapter()),                              // The 2ed is a placeholder for notInline
        edgeSummaryMap(summaryAlloc.Adapter()),
        argInfosMap(summaryAlloc.Adapter()) {
    // Init the first cost item: truePredicate
    (void)costTable.emplace_back(Predicate::TruePredicate(), InlineCost{ 0, 0 });
  }

  void Dump() const;
  void DumpConditions() const;
  void DumpCostTable() const;
  void DumpArgInfosMap() const;
  void DumpEdgeSummaryMap() const;

  MapleAllocator &GetSummaryAlloc() {
    return summaryAlloc;
  }

  NumInsns GetStaticInsns() const {
    return staticCost.GetInsns();
  }

  NumCycles GetStaticCycles() const {
    return staticCost.GetCycles();
  }

  void AddStaticInsns(NumInsns detInsns) {
    staticCost.AddInsns(detInsns);
  }

  void AddStaticCycles(NumCycles detCycles) {
    staticCost.AddCycles(detCycles);
  }

  void SetStaticCost(int32 insns, double cycles) {
    staticCost.Reset(insns, cycles);
  }

  const MapleVector<Condition*> &GetConditions() const {
    return conditions;
  }

  // Given arguments info, evalutate all conditions. The result will be saved in `condResultVec` indexed by condIdx
  void EvaluateConditions(MapleAllocator &alloc, const ArgInfoVec *argInfoVec,
      std::vector<ExprBoolResult> &condResultVec, bool willInline) const {
    condResultVec.resize(conditions.size(), kExprResUnknown);
    for (uint32 i = 0; i < conditions.size(); ++i) {
      auto *cond = conditions[i];
      if (cond == nullptr) {
        continue;
      }
      ExprBoolResult res = cond->EvaluateToBool(alloc, argInfoVec);
      if (res != kExprResUnknown) {
        condResultVec[i] = res;
      }
    }
    condResultVec[kCondIdxNotInline] = !willInline ? kExprResTrue : kExprResFalse;
  }

  // Return a predicate with single condition spcified by `liteExpr` and `reverse`
  const Predicate *AddCondition(LiteExpr *liteExpr, bool reverse, uint32 params, MapleAllocator &tmpAlloc) {
    if (liteExpr->IsConst() && IsPrimitiveInteger(liteExpr->GetType())) {
      // liteExpr can be evaluated now
      bool isTrue = liteExpr->GetInt() != 0;
      if (reverse) {
        isTrue = !isTrue;
      }
      return isTrue ? Predicate::TruePredicate() : Predicate::FalsePredicate();
    }
    Condition *targetCond = nullptr;
    // Find cond in conditions first
    for (uint32 i = 0; i < conditions.size(); ++i) {
      auto *cond = conditions[i];
      if (cond == nullptr) {
        continue;
      }
      if (cond->GetExpr() == liteExpr && cond->IsReverse() == reverse) {
        targetCond = cond;
        break;
      }
    }
    if (targetCond == nullptr) {
      if (conditions.size() == kMaxNumCondition) {
        // conditions out of range
        return Predicate::TruePredicate();
      }
      targetCond = summaryAlloc.New<Condition>(conditions.size(), liteExpr, reverse, params);
      conditions.push_back(targetCond);
    }
    // Create a single condition predicate.
    // New generated predicate is always allocated in tmpAlloc because we are not sure how long it will live
    return tmpAlloc.New<Predicate>(targetCond->GetIndex(), tmpAlloc);
  }

  void CloneAllPredicatesToSummaryAlloc() {
    for (auto &pair : costTable) {
      pair.first = pair.first->Clone(summaryAlloc);
    }
  }

  void AddCondCostItem(const Predicate &predicate, maple::NumInsns insns, NumCycles cycles, bool debug) {
    if (predicate.IsFalsePredicate()) {
      return;  // Adding false predicate make no sense
    }
    if (debug) {
      LogInfo::MapleLogger() << "  Accounting insns: " << insns << ", cycles: " << cycles << " on predicate: ";
      predicate.Dump(conditions);
    }
    ASSERT(costTable[0].first->IsTruePredicate(), "The first item of costTable must be truePredicate");
    if (predicate.IsTruePredicate()) {
      costTable[0].second.Add(insns, cycles);
      return;
    }
    for (size_t i = costTable.size() - 1; i > 0; --i) {  // look from back to front, skip the first item
      if (costTable[i].first == &predicate) {
        costTable[i].second.Add(insns, cycles);
        return;
      }
    }
    (void)costTable.emplace_back(&predicate, InlineCost{ insns, cycles });
  }

  void AddArgInfo(uint32 callStmtId, uint32 argIndex, ArgInfo *argInfo) {
    const auto &res = argInfosMap.try_emplace(callStmtId, nullptr);
    if (res.second) {
      res.first->second = summaryAlloc.New<ArgInfoVec>(summaryAlloc.Adapter());
    }
    auto &argInfoVec = *res.first->second;
    if (argIndex >= argInfoVec.size()) {
      argInfoVec.resize(argIndex + 1, nullptr);
    }
    argInfoVec[argIndex] = argInfo;
  }

  void AddEdgeSummary(uint32 callStmtId, const Predicate *bbPredicate, int32 frequency,
                      int32 insns, int32 cycles) {
    const auto &res = edgeSummaryMap.try_emplace(callStmtId, nullptr);
    if (res.second) {
      res.first->second = summaryAlloc.New<InlineEdgeSummary>(bbPredicate, frequency, insns, cycles);
    }
  }

  void RemoveEdgeSummary(uint32 callStmtId) {
    (void)edgeSummaryMap.erase(callStmtId);
  }

  InlineEdgeSummary *GetEdgeSummary(uint32 callStmtId) const {
    if (edgeSummaryMap.find(callStmtId) == edgeSummaryMap.end()) {
      return nullptr;
    }
    return edgeSummaryMap.at(callStmtId);
  }

  bool IsUnlikelyCallsite(uint32 callStmtId) const {
    auto it = edgeSummaryMap.find(callStmtId);
    if (it == edgeSummaryMap.end()) {
      return false;
    }
    auto callFreq = it->second->frequency;
    // Attention: sometimes freq 0 is also invalid
    if (callFreq == -1 || callFreq == 0) {
      return false;
    }
    double freqPercent = static_cast<double>(callFreq) / static_cast<double>(kCallFreqBase);
    return freqPercent <= kColdFreqPercent;
  }

  MapleVector<std::pair<const Predicate*, InlineCost>> &GetCostTable() {
    return costTable;
  }

  const MapleVector<std::pair<const Predicate*, InlineCost>> &GetCostTable() const {
    return costTable;
  }

  MapleMap<uint32, ArgInfoVec*> &GetArgInfosMap() {
    return argInfosMap;
  }

  const MapleMap<uint32, ArgInfoVec*> &GetArgInfosMap() const {
    return argInfosMap;
  }

  bool IsArgInfoCollected() const {
    return argInfoCollected;
  }

  void SetArgInfoCollected(bool flag) {
    argInfoCollected = flag;
  }

  const char *GetMustNotInlineReason() const {
    return mustNotInlineReason;
  }

  void SetMustNotInlineReason(const char *reason) {
    mustNotInlineReason = reason;
  }

  InlineFailedCode GetInlineFailedCode() const {
    return inlineFailedCode;
  }

  void SetInlineFailedCode(InlineFailedCode code) {
    inlineFailedCode = code;
  }

  bool HasBigSwitch() const {
    return hasBigSwitch;
  }

  void SetHasBigSwitch(bool val) {
    hasBigSwitch = val;
  }

  void GetParamMappingForCallsite(uint32 callStmtId, std::vector<int32> &paramMapping) const {
    auto it = argInfosMap.find(callStmtId);
    if (it == argInfosMap.end()) {
      return;
    }
    auto &argInfoVec = *it->second;
    for (uint32 i = 0; i < argInfoVec.size(); ++i) {
      if (argInfoVec[i] == nullptr || !argInfoVec[i]->IsParamPassThrough()) {
        continue;
      }
      paramMapping.resize(argInfoVec.size(), -1);
      paramMapping[i] = argInfoVec[i]->GetCallerFormalIdx();
    }
  }

  // Inline summary hold some callsite-level information such as arguments info (argInfosMap), one of the most
  // important issue is how we indexed these information. Inline summaries are generated based on MEIR in IPA
  // and consumed by inline based on maple IR in mpl2mpl. We can not use callMeStmt as callsite-level info key
  // because MEIR will be released after IPA. So we use callMeStmt id as key. However, inline summary merging
  // during inlining is based on mapleIR (we need clone callee's maple IR body), callMeStmt id is invalid at
  // that time. Finally, we solve it by using a wired solution: Using callMeStmt id as key when generating summary
  // in IPA. After pemit, we call the followinig `RefreshMapKeyIfNeeded` to refresh the key with callMapleStmt id.
  void RefreshMapKeyIfNeeded() {
    if (keyRefreshed) {
      return;
    }
    keyRefreshed = true;
    std::vector<std::pair<uint32, ArgInfoVec*>> copyArgInfos;
    for (const auto &pair : std::as_const(argInfosMap)) {
      auto meStmtId = pair.first;
      auto *callStmt = func->GetStmtNodeFromMeId(meStmtId);
      CHECK_NULL_FATAL(callStmt);
      (void)copyArgInfos.emplace_back(callStmt->GetStmtID(), pair.second);
    }
    argInfosMap.clear();
    for (const auto &pair : copyArgInfos) {
      (void)argInfosMap.emplace(pair.first, pair.second);
    }

    std::vector<std::pair<uint32, InlineEdgeSummary*>> copyEdgeSummaries;
    for (const auto &pair : std::as_const(edgeSummaryMap)) {
      auto meStmtId = pair.first;
      auto *callStmt = func->GetStmtNodeFromMeId(meStmtId);
      CHECK_NULL_FATAL(callStmt);
      (void)copyEdgeSummaries.emplace_back(callStmt->GetStmtID(), pair.second);
    }
    // clear old data and push new data
    edgeSummaryMap.clear();
    for (const auto &pair : copyEdgeSummaries) {
      (void)edgeSummaryMap.emplace(pair.first, pair.second);
    }
  }

  bool IsTrustworthy() const {
    return trustworthy;
  }

  void MergeSummary(const InlineSummary &fromSummary, uint32 callStmtId, MapleAllocator &tmpAlloc,
      const std::vector<ExprBoolResult> &condResultVec, const std::map<uint32, uint32> &callMeStmtIdMap);

 private:
  void MergeAndRemapConditions(const InlineSummary &fromSummary, Assert condsNeedCopy,
      const std::vector<int32> &paramMapping, std::array<int8, kMaxNumCondition> &oldCondIdx2New);

  void MergeEdgeSummary(const InlineSummary &fromSummary, std::pair<uint32, InlineEdgeSummary*> callEdgeSummaryPair,
      MapleAllocator &tmpAlloc, const std::array<int8, kMaxNumCondition> &oldCondIdx2New,
      const std::map<uint32, uint32> &callMeStmtIdMap);

  void MergeArgInfosMap(const InlineSummary &fromSummary, uint32 callStmtId,
      const std::map<uint32, uint32> &callMeStmtIdMap);

  // Return condIdx bitMap that need be copied to toSummary
  Assert SpecializeCostTable(const std::vector<ExprBoolResult> &condResultVec, const std::vector<int32> &paramMapping,
      int32 callFrequency, std::vector<std::pair<const Predicate*, InlineCost>> &spCostTable,
      InlineCost &unnecessaryCost) const;

  // Do not add too many unnecessary fields unless you want to MergeSummary more and more complicated
  MapleAllocator &summaryAlloc;
  MIRFunction *func = nullptr;
  InlineCost staticCost;  // callsite-insensitive static cost

  MapleVector<Condition*> conditions;
  MapleVector<std::pair<const Predicate*, InlineCost>> costTable;  // The first element is always truePredicate

  // key: callStmtId, value: inline edge summary of callsite
  MapleMap<uint32, InlineEdgeSummary*> edgeSummaryMap;

  // key: callStmtId, value: argInfoVec for current callsite, see `ArgInfo` for details
  MapleMap<uint32, ArgInfoVec*> argInfosMap;
  const char *mustNotInlineReason = nullptr;  // If not null, the function must not be inlined
  InlineFailedCode inlineFailedCode = kIFC_NeedFurtherAnalysis;
  bool hasBigSwitch = false;
  bool argInfoCollected = false;
  bool keyRefreshed = false;
  bool trustworthy = true;
};

void MergeInlineSummary(MIRFunction &caller, MIRFunction &callee, const StmtNode &callStmt,
    const std::map<uint32, uint32> &callMeStmtIdMap);

struct BBPredicate {
  const Predicate *predicate = nullptr;
  MapleVector<const Predicate*> succEdgePredicates;
  explicit BBPredicate(MapleAllocator &alloc) : succEdgePredicates(alloc.Adapter()) {}
};

// Function level inline summary collector
class InlineSummaryCollector {
 public:
  static void CollectArgInfo(MeFunction &meFunc);

  InlineSummaryCollector(MapleAllocator &allocator, MeFunction &meFunc, Dominance &dominance, Dominance &postDominance,
                         IdentifyLoops &loop)
      : summaryAlloc(allocator),
        tmpAlloc(memPoolCtrler.NewMemPool("tmp alloc", true)),
        func(&meFunc),
        stmtCostAnalyzer(tmpAlloc.GetMemPool(), func->GetMirFunc()),
        dom(dominance),
        pdom(postDominance),
        meLoop(loop),
        allBBPred(tmpAlloc.Adapter()) {}

  ~InlineSummaryCollector() {
    memPoolCtrler.DeleteMemPool(tmpAlloc.GetMemPool());
  }

  bool IsDebug() const {
    return debug;
  }

  void SetDebug(bool dbg) {
    debug = dbg;
  }

  void PrepareSummary() {
    if (inlineSummary != nullptr) {
      return;
    }
    inlineSummary = func->GetMirFunc()->GetOrCreateInlineSummary();
  }

  void PreparePredicateForBB(BB &bb) {
    auto bbId = bb.GetBBId().get();
    if (allBBPred[bbId] == nullptr) {
      allBBPred[bbId] = tmpAlloc.New<BBPredicate>(tmpAlloc);
    }
  }

  void ComputeEdgePredicate();
  void PropBBPredicate();
  void ComputeConditionalCost();
  std::pair<int32, double> AnalyzeCondCostForStmt(MeStmt &stmt, int32 callFrequency, const Predicate *bbPredClone,
      const BBPredicate &bbPredicate);

  void CollectInlineSummary() {
    InlineSummaryCollector::CollectArgInfo(*func);
    BuiltinExpectInfo savedExpectInfo;
    if (!Options::profileUse) {  // If there is real profile, no need to predict freq
      MePrediction::RebuildFreq(*func, dom, pdom, meLoop, &savedExpectInfo);  // ipa prepare should rebuild freq before
    }
    PrepareSummary();
    if (debug) {
      LogInfo::MapleLogger() << "Collect inline summary for function: " << func->GetName() << "/" <<
          func->GetMirFunc()->GetPuidx() << std::endl;
    }
    ComputeEdgePredicate();
    PropBBPredicate();
    ComputeConditionalCost();
    if (!Options::profileUse) {
      MePrediction::RecoverBuiltinExpectInfo(savedExpectInfo);
    }
    if (IsDebug()) {
      inlineSummary->Dump();
    }
  }

  // See comment for LiteExpr::TryBuildLiteExpr
  LiteExpr *GetOrCreateLiteExpr(MeExpr &meExpr, uint32 kinds, uint32 &paramsUsed) {
    // search cache first
    auto it = exprCache.find(&meExpr);
    if (it != exprCache.end()) {
      paramsUsed = it->second.second;
      return it->second.first;
    }
    // All liteExprs should be allocated in summaryAlloc
    auto *liteExpr = LiteExpr::TryBuildLiteExpr(meExpr, kinds, *func, summaryAlloc, &paramsUsed);
    if (liteExpr == nullptr) {
      return nullptr;
    }
    (void)exprCache.emplace(&meExpr, std::pair<LiteExpr*, uint32>(liteExpr, paramsUsed));
    return liteExpr;
  }

 private:
  MapleAllocator &summaryAlloc; // summaryAlloc will not be released util ipa-inlining finish
  MapleAllocator tmpAlloc;      // tmpAlloc will be released once inlineSummaryCollector is destructed
  MeFunction *func = nullptr;
  StmtCostAnalyzer stmtCostAnalyzer;
  Dominance &dom;
  Dominance &pdom;
  IdentifyLoops &meLoop;
  InlineSummary *inlineSummary = nullptr;
  // All bb predicates are allocated in tmpAlloc, necessary predicates will be cloned into summaryAlloc
  MapleVector<BBPredicate*> allBBPred;
  // We use `exprCache` to avoid build multiple different liteExpr for a single meExpr
  // key: meExpr, value: liteExpr and paramsUsed
  std::unordered_map<MeExpr*, std::pair<LiteExpr*, uint32>> exprCache;
  bool debug = false;
};
}  // namespace maple
#endif  // MAPLE_MPL2MPL_INCLUDE_INLINE_SUMMARY_H

