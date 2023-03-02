/*
 * Copyright (c) [2021] Huawei Technologies Co.,Ltd.All rights reserved.
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
#ifndef MAPLE_ME_INCLUDE_ME_VALUE_RANGE_PROP_H
#define MAPLE_ME_INCLUDE_ME_VALUE_RANGE_PROP_H

#include "me_ir.h"
#include "me_function.h"
#include "me_cfg.h"
#include "me_dominance.h"
#include "me_loop_analysis.h"
#include "me_scalar_analysis.h"
namespace maple {
constexpr size_t kFourByte = 4;
constexpr size_t kEightByte = 8;
constexpr uint32 kRecursionThresholdOfFindVROfLoopVar = 10;
constexpr uint32 kRecursionThresholdOfFindVROfPhi = 35;
constexpr uint32 kRecursionThreshold = 50;

class ValueRangePropagation;
class ValueRange;
class SafetyCheck {
 public:
  SafetyCheck() = default;
  explicit SafetyCheck(MeFunction &f) : func(&f) {}
  virtual ~SafetyCheck() = default;

  bool NeedDeleteTheAssertAfterErrorOrWarn(const MeStmt &stmt, bool isNullablePtr = false) const;
  virtual void HandleAssignWithDeadBeef(BB &bb, MeStmt &meStmt, MeExpr &indexOpnd, MeExpr &boundOpnd) {}
  virtual void HandleAssertNonnull(const MeStmt &meStmt, const ValueRange *valueRangeOfIndex) {}
  virtual bool HandleAssertError(const MeStmt &meStmt) {
    return false;
  }

  virtual bool HandleAssertltOrAssertle(const MeStmt &meStmt, Opcode op, int64 indexValue, int64 lengthValue) {
    return false;
  }

 protected:
  MeFunction *func = nullptr;
};

class SafetyCheckWithNonnullError : public SafetyCheck {
 public:
  explicit SafetyCheckWithNonnullError(MeFunction &f)
      : SafetyCheck(f) {}
  ~SafetyCheckWithNonnullError() override = default;
  void HandleAssertNonnull(const MeStmt &meStmt, const ValueRange *valueRangeOfIndex) override;
};

class SafetyCheckWithBoundaryError : public SafetyCheck {
 public:
  SafetyCheckWithBoundaryError(MeFunction &f, ValueRangePropagation &valueRangeProp)
      : SafetyCheck(f), vrp(valueRangeProp) {}
  ~SafetyCheckWithBoundaryError() override = default;
  void HandleAssignWithDeadBeef(BB &bb, MeStmt &meStmt, MeExpr &indexOpnd, MeExpr &boundOpnd) override;
  bool HandleAssertError(const MeStmt &meStmt) override;
  bool HandleAssertltOrAssertle(const MeStmt &meStmt, Opcode op, int64 indexValue, int64 lengthValue) override;

 private:
  ValueRangePropagation &vrp;
};

int64 GetMinNumber(PrimType pType);
int64 GetMaxNumber(PrimType primType);
bool IsNeededPrimType(PrimType pType);
int64 GetRealValue(int64 value, PrimType pType);
bool IsPrimTypeUint64(PrimType pType);

class Bound {
 public:
  Bound() : var(nullptr), constant(0), primType(PTY_begin) {}
  Bound(MeExpr *argVar, int64 argConstant, PrimType pType) : var(argVar), constant(argConstant), primType(pType) {}
  Bound(MeExpr *argVar, PrimType pType) : var(argVar), constant(0), primType(pType) {}
  Bound(int64 argConstant, PrimType pType) : var(nullptr), constant(argConstant), primType(pType) {}
  ~Bound() = default;

  const MeExpr *GetVar() const {
    return var;
  }

  MeExpr *GetVar() {
    return var;
  }

  void SetVar(MeExpr *expr) {
    var = expr;
  }

  int64 GetConstant() const {
    return constant;
  }

  void SetConstant(int64 val) {
    constant = val;
  }

  PrimType GetPrimType() const {
    return primType;
  }

  void SetPrimType(PrimType pType) {
    primType = pType;
  }

  bool IsGreaterThan(const Bound rightBound, PrimType pType) const {
    CHECK_FATAL(IsNeededPrimType(pType), "must not be here");
    return IsPrimTypeUint64(pType) ? static_cast<uint64>(constant) > static_cast<uint64>(rightBound.GetConstant()) :
        GetRealValue(constant, pType) > GetRealValue(rightBound.GetConstant(), pType);
  }

  bool IsLessThanOrEqualToMax(PrimType pType) const {
    CHECK_FATAL(IsNeededPrimType(pType), "must not be here");
    return IsPrimTypeUint64(pType) ? static_cast<uint64>(constant) <= static_cast<uint64>(GetMaxNumber(pType)) :
        GetRealValue(constant, pType) <= GetRealValue(GetMaxNumber(pType), pType);
  }

  bool IsGreaterThanOrEqualToMin(PrimType pType) const {
    CHECK_FATAL(IsNeededPrimType(pType), "must not be here");
    return IsPrimTypeUint64(pType) ? static_cast<uint64>(constant) >= static_cast<uint64>(GetMinNumber(pType)):
        GetRealValue(constant, pType) >= GetRealValue(GetMinNumber(pType), pType);
  }

  bool IsLessThanOrEqualTo(const Bound rightBound, PrimType pType) const {
    CHECK_FATAL(IsNeededPrimType(pType), "must not be here");
    return IsPrimTypeUint64(pType) ? static_cast<uint64>(constant) <= static_cast<uint64>(rightBound.GetConstant()) :
        GetRealValue(constant, pType) <= GetRealValue(rightBound.GetConstant(), pType);
  }

  bool IsLessThan(const Bound rightBound, PrimType pType) const {
    CHECK_FATAL(IsNeededPrimType(pType), "must not be here");
    return IsPrimTypeUint64(pType) ? static_cast<uint64>(constant) < static_cast<uint64>(rightBound.GetConstant()) :
        GetRealValue(constant, pType) < GetRealValue(rightBound.GetConstant(), pType);
  }

  bool IsGreaterThanOrEqualTo(const Bound rightBound, PrimType pType) const {
    CHECK_FATAL(IsNeededPrimType(pType), "must not be here");
    return IsPrimTypeUint64(pType) ? static_cast<uint64>(constant) >= static_cast<uint64>(rightBound.GetConstant()):
        GetRealValue(constant, pType) >= GetRealValue(rightBound.GetConstant(), pType);
  }

  bool IsEqual(const Bound rightBound, PrimType pType) const {
    CHECK_FATAL(IsNeededPrimType(pType), "must not be here");
    return IsPrimTypeUint64(pType) ? static_cast<uint64>(constant) == static_cast<uint64>(rightBound.GetConstant()) :
        GetRealValue(constant, pType) == GetRealValue(rightBound.GetConstant(), pType);
  }

  bool IsEqualAfterCVT(PrimType fromType, PrimType toType) const {
    if (!IsNeededPrimType(fromType) || !IsNeededPrimType(toType)) {
      return false;
    }
    if (fromType == toType) {
      return true;
    }
    if (IsPrimTypeUint64(fromType)) {
      return constant == GetRealValue(constant, toType);
    } else if (IsPrimTypeUint64(toType)) {
      return constant == GetRealValue(constant, fromType);
    } else {
      return GetRealValue(constant, fromType) == GetRealValue(constant, toType);
    }
  }

  bool IsEqualToMax(PrimType pType) const {
    CHECK_FATAL(IsNeededPrimType(pType), "must not be here");
    return IsPrimTypeUint64(pType) ? static_cast<uint64>(constant) == static_cast<uint64>(GetMaxNumber(pType)) :
        GetRealValue(constant, pType) == GetRealValue(GetMaxNumber(pType), pType);
  }

  bool IsEqualToMin(PrimType pType) const {
    CHECK_FATAL(IsNeededPrimType(pType), "must not be here");
    return IsPrimTypeUint64(pType) ? static_cast<uint64>(constant) == static_cast<uint64>(GetMinNumber(pType)) :
        GetRealValue(constant, pType) == GetRealValue(GetMinNumber(pType), pType);
  }

  static int64 GetRemResult(int64 lhsConstant, int64 rhsConstant, PrimType pType) {
    CHECK_FATAL(IsNeededPrimType(pType), "must not be here");
    return IsPrimTypeUint64(pType) ? static_cast<uint64>(lhsConstant) % static_cast<uint64>(rhsConstant) :
        GetRealValue(lhsConstant, pType) % GetRealValue(rhsConstant, pType);
  }

  bool CanBeComparedWith(const Bound &bound) const;

  bool operator==(const Bound &bound) const {
    return var == bound.GetVar() && constant == bound.GetConstant() &&
           (GetPrimTypeActualBitSize(primType) == GetPrimTypeActualBitSize(bound.GetPrimType()) &&
            IsSignedInteger(primType) == IsSignedInteger(bound.GetPrimType()));
  }

  bool operator!=(const Bound &bound) const {
    return var != bound.GetVar() || constant != bound.GetConstant() ||
        (GetPrimTypeActualBitSize(primType) != GetPrimTypeActualBitSize(bound.GetPrimType()) ||
        IsSignedInteger(primType) == IsSignedInteger(bound.GetPrimType()));
  }

  bool operator<(const Bound &bound) const;

  bool operator<=(const Bound &bound) const;

  Bound &operator++() { // prefix inc
    if (primType == PTY_u64) {
      constant = static_cast<int64>(static_cast<uint64>(constant) + 1);
    } else {
      ++constant;
    }
    return *this;
  }
  Bound &operator--() { // prefix dec
    if (primType == PTY_u64) {
      constant = static_cast<int64>(static_cast<uint64>(constant) - 1);
    } else {
      --constant;
    }
    return *this;
  }

  static Bound MinBound(PrimType pType) {
    return Bound(nullptr, GetMinNumber(pType), pType);
  }

  static Bound MaxBound(PrimType pType) {
    return Bound(nullptr, GetMaxNumber(pType), pType);
  }

  bool IsConstantBound() const {
    return var == nullptr;
  }

  bool IsClosedInterval() const {
    return isClosedInterval;
  }

  void SetClosedInterval(bool isClose) {
    isClosedInterval = isClose;
  }

 private:
  MeExpr *var = nullptr;
  int64 constant = 0;
  PrimType primType = PTY_begin;
  bool isClosedInterval = true; // The upper or lower bound of value range is an closed interval.
};

enum RangeType {
  kRTEmpty,
  kLowerAndUpper,
  kSpecialUpperForLoop,
  kSpecialLowerForLoop,
  kOnlyHasLowerBound,
  kOnlyHasUpperBound,
  kNotEqual,
  kEqual,
  kRTComplete
};

enum StrideType {
  kStrideIsConstant,
  kStrideIsValueRange,
  kCanNotCompute,
};

enum InRangeType {
  kLowerInRange,
  kUpperInRange,
  kInRange,
  kNotInRange,
};

using Lower = Bound;
using Upper = Bound;

struct RangePair {
  RangePair() : lower(Bound()), upper(Bound()) {}
  RangePair(const Bound &l, const Bound &u) : lower(l), upper(u) {}
  Lower lower = Bound();
  Upper upper = Bound();
};

class ValueRange {
 public:
  union Range {
    Range() {};
    RangePair pair;
    Bound bound;
  };

  ValueRange(const Bound &bound, int64 argStride, RangeType type) : stride(argStride), rangeType(type) {
    range.bound = bound;
  }

  ValueRange(const Bound &lower, const Bound &upper, RangeType type, bool argAccurate = false)
      : rangeType(type), isAccurate(argAccurate) {
    range.pair.lower = lower;
    range.pair.upper = upper;
  }

  ValueRange(const Bound &bound, RangeType type, bool argAccurate = false) : rangeType(type), isAccurate(argAccurate) {
    range.bound = bound;
  }

  explicit ValueRange(RangeType type) : rangeType(type) {} // only used for kRTEmpty/kRTComplete

  ~ValueRange() = default;

  void SetRangePair(const Bound &lower, const Bound &upper) {
    range.pair.lower = lower;
    range.pair.upper = upper;
  }

  const RangePair GetLowerAndupper() const {
    return range.pair;
  }

  void SetLower(const Lower &lower) {
    range.pair.lower = lower;
  }

  void SetUpper(const Upper &upper) {
    range.pair.upper = upper;
  }

  Bound GetUpper() const {
    switch (rangeType) {
      case kLowerAndUpper:
      case kSpecialLowerForLoop:
      case kSpecialUpperForLoop:
        return range.pair.upper;
      case kOnlyHasUpperBound:
      case kEqual:
      case kNotEqual:
        return range.bound;
      case kOnlyHasLowerBound:
        return MaxBound(range.bound.GetPrimType());
      default:
        CHECK_FATAL(false, "can not be here");
    }
  }

  Bound GetLower() const {
    switch (rangeType) {
      case kLowerAndUpper:
      case kSpecialLowerForLoop:
      case kSpecialUpperForLoop:
        return range.pair.lower;
      case kOnlyHasLowerBound:
      case kEqual:
      case kNotEqual:
        return range.bound;
      case kOnlyHasUpperBound:
        return MinBound(range.bound.GetPrimType());
      default:
        CHECK_FATAL(false, "can not be here");
    }
  }

  bool IfLowerEqualToUpper() const {
    return rangeType == kEqual ||
           (rangeType == kLowerAndUpper &&
            range.pair.lower.GetVar() == range.pair.upper.GetVar() &&
            range.pair.lower.GetConstant() == range.pair.upper.GetConstant());
  }

  bool UpperIsMax(PrimType pType) const {
    return GetUpper().IsEqualToMax(pType);
  }

  bool LowerIsMin(PrimType pType) const {
    return GetLower().IsEqualToMin(pType);
  }

  void SetBound(const Bound &argBound) {
    range.bound = argBound;
  }

  const Bound GetBound() const {
    return range.bound;
  }

  Bound GetBound() {
    return range.bound;
  }

  int64 GetStride() const {
    return stride;
  }

  RangeType GetRangeType() const {
    return rangeType;
  }

  void SetRangeType(RangeType type) {
    rangeType = type;
  }

  void SetPrimType(PrimType pType) {
    range.bound.SetPrimType(pType);
    range.pair.upper.SetPrimType(pType);
    range.pair.lower.SetPrimType(pType);
  }

  PrimType GetPrimType() const {
    switch (rangeType) {
      case kLowerAndUpper:
      case kSpecialLowerForLoop:
      case kSpecialUpperForLoop:
        return range.pair.lower.GetPrimType();
      case kEqual:
      case kNotEqual:
      case kOnlyHasLowerBound:
      case kOnlyHasUpperBound:
        return range.bound.GetPrimType();
      default:
        CHECK_FATAL(false, "can not be here");
    }
  }

  bool IsZeroInRange() const {
    return IsConstantLowerAndUpper() && GetUpper().GetConstant() >= 0 &&
        GetLower().GetConstant() < GetUpper().GetConstant();
  }
  bool IsConstantRange() {
    return (rangeType == kEqual && range.bound.GetVar() == nullptr) ||
           (rangeType == kNotEqual && range.bound.GetVar() == nullptr) ||
           (rangeType == kLowerAndUpper && range.pair.lower.GetVar() == nullptr &&
            range.pair.lower.GetVar() == range.pair.upper.GetVar());
  }

  bool IsConstant() const {
    return (rangeType == kEqual && range.bound.GetVar() == nullptr) ||
           (rangeType == kNotEqual && range.bound.GetVar() == nullptr) ||
           (rangeType == kLowerAndUpper && range.pair.lower.GetVar() == nullptr &&
            range.pair.lower.GetVar() == range.pair.upper.GetVar() &&
            range.pair.lower.GetConstant() == range.pair.upper.GetConstant());
  }

  bool IsConstantLowerAndUpper() const {
    return (rangeType == kEqual && range.bound.GetVar() == nullptr) ||
           (rangeType == kLowerAndUpper && range.pair.lower.GetVar() == nullptr &&
            range.pair.upper.GetVar() == nullptr);
  }

  bool IsGreaterThanOrEqualToZero() const {
    auto pType = GetPrimType();
    Bound zeroBound(nullptr, 0, pType);
    return GetLower().IsGreaterThanOrEqualTo(zeroBound, pType) &&
      GetUpper().IsGreaterThanOrEqualTo(zeroBound, pType);
  }

  bool IsNotConstantVR() const {
    return GetLower().GetVar() != nullptr || GetUpper().GetVar() != nullptr;
  }

  static Bound MinBound(PrimType pType) {
    return Bound(nullptr, GetMinNumber(pType), pType);
  }

  static Bound MaxBound(PrimType pType) {
    return Bound(nullptr, GetMaxNumber(pType), pType);
  }

  bool IsNotEqualZero() const {
    return rangeType == kNotEqual && range.bound.GetVar() == nullptr && range.bound.GetConstant() == 0;
  }

  bool IsEqualZero() const {
    return rangeType == kEqual && range.bound.GetVar() == nullptr && range.bound.GetConstant() == 0;
  }

  bool IsAccurate() const {
    return isAccurate;
  }

  void SetAccurate(bool argAccurate) {
    isAccurate = argAccurate;
  }

  bool IsEqualAfterCVT(PrimType fromType, PrimType toType) const {
    if (fromType == toType) {
      return true;
    }
    if (rangeType != kEqual && rangeType != kLowerAndUpper) {
      return false;
    }
    if (!IsConstantLowerAndUpper()) {
      return false;
    }
    return GetLower().IsEqualAfterCVT(fromType, toType) && GetUpper().IsEqualAfterCVT(fromType, toType);
  }

  bool IsEqual(ValueRange *valueRangeRight) const;

 private:
  Range range;
  int64 stride = 0;
  RangeType rangeType;
  bool isAccurate = false;
};

// return nullptr means cannot merge, intersect = true : intersection set, intersect = false : union set
std::unique_ptr<ValueRange> MergeVR(const ValueRange &vr1, const ValueRange &vr2, bool intersect = false);
// give expr and its value range, use irmap to create a cmp expr
MeExpr *GetCmpExprFromVR(const ValueRange *vr, MeExpr &expr, MeIRMap *irmap);

class ValueRangePropagation {
 public:
  static bool isDebug;

  ValueRangePropagation(MeFunction &meFunc, MeIRMap &argIRMap, Dominance &argDom,
                        IdentifyLoops *argLoops, MemPool &pool,
                        std::map<OStIdx, std::unique_ptr<std::set<BBId>>> &candsTem, LoopScalarAnalysisResult &currSA,
                        bool dealWithAssertArg, bool onlyProp = false)
      : func(meFunc), irMap(argIRMap), dom(argDom), memPool(pool), mpAllocator(&pool), loops(argLoops),
        caches(meFunc.GetCfg()->GetAllBBs().size()), analysisedLowerBoundChecks(meFunc.GetCfg()->GetAllBBs().size()),
        analysisedUpperBoundChecks(meFunc.GetCfg()->GetAllBBs().size()),
        analysisedAssignBoundChecks(meFunc.GetCfg()->GetAllBBs().size()),
        cands(candsTem), sa(currSA), dealWithAssert(dealWithAssertArg), onlyPropVR(onlyProp) {
    onlyRecordValueRangeInTempCache.push(false);
  }
  ~ValueRangePropagation() = default;

  void DumpCaches();
  void Execute();

  bool IsCFGChange() const {
    return isCFGChange;
  }

  bool NeedUpdateSSA() const {
    return needUpdateSSA;
  }

  void SetSafetyNonnullCheck(SafetyCheck &check) {
    safetyCheckNonnull = &check;
  }

  void SetSafetyBoundaryCheck(SafetyCheck &check) {
    safetyCheckBoundary = &check;
  }

  void JudgeTheConsistencyOfDefPointsOfBoundaryCheck(
      BB &bb, MeExpr &expr, std::set<MeExpr*> &visitedLHS, std::vector<MeStmt*> &stmts, bool &crossPhiNode);
  bool TheValueOfOpndIsInvaliedInABCO(const BB &bb, const MeStmt *meStmt, MeExpr &boundOpnd, bool updateCaches = true);
  ValueRange *FindValueRange(const BB &bb, MeExpr &expr, uint32 &numberOfRecursions,
      std::unordered_set<int32> &foundExprs, uint32 maxThreshold);
  bool BrStmtInRange(const BB &bb, const ValueRange &leftRange, const ValueRange &rightRange, Opcode op,
                     PrimType opndType, bool judgeNotInRange = false);
  std::unique_ptr<ValueRange> CreateValueRangeOfNotEqualZero(PrimType pType) const {
    return std::make_unique<ValueRange>(Bound(nullptr, 0, pType), kNotEqual);
  }

  std::unique_ptr<ValueRange> CreateValueRangeOfEqualZero(PrimType pType) const {
    return std::make_unique<ValueRange>(Bound(nullptr, 0, pType), kEqual);
  }

  ValueRange CreateTempValueRangeOfNotEqualZero(PrimType pType) const {
    return ValueRange(Bound(nullptr, 0, pType), kNotEqual);
  }

  ValueRange CreateTempValueRangeOfEqualZero(PrimType pType) const {
    return ValueRange(Bound(nullptr, 0, pType), kEqual);
  }

  void ComputeCodeSize(const MeExpr &meExpr, uint32 &cost);
  void ComputeCodeSize(const MeStmt &meStmt, uint32 &cost);
  bool TowCompareOperandsAreInSameIterOfLoop(const MeExpr &lhs, const MeExpr &rhs) const;

  ValueRange *FindValueRangeAndInitNumOfRecursion(
      const BB &bb, MeExpr &expr, uint32 maxThreshold = kRecursionThreshold) {
    uint32 numOfRecursion = 0;
    std::unordered_set<int32> foundExprs{expr.GetExprID()};
    return FindValueRange(bb, expr, numOfRecursion, foundExprs, maxThreshold);
  }

 private:
  struct CompareExpr {
    bool operator()(const MeExpr *expr1, const MeExpr *expr2) const {
      ASSERT_NOT_NULL(expr1);
      ASSERT_NOT_NULL(expr2);
      return expr1->GetExprID() < expr2->GetExprID();
    }
  };

  struct CompareBB {
    bool operator()(const BB *bb1, const BB *bb2) const {
      ASSERT_NOT_NULL(bb1);
      ASSERT_NOT_NULL(bb2);
      return bb1->GetBBId() < bb2->GetBBId();
    }
  };

  bool Insert2Caches(const BBId &bbID, int32 exprID, std::unique_ptr<ValueRange> valueRange,
      const MeExpr *opnd = nullptr);

  ValueRange *FindValueRangeInCurrentBB(BBId bbID, int32 exprID) {
    auto it = caches.at(bbID).find(exprID);
    return it != caches.at(bbID).end() ? it->second.get() : nullptr;
  }

  ValueRange *GetVRAfterCvt(ValueRange &vr, PrimType pty) const;

  ValueRange *FindValueRangeInCaches(
      BBId bbID, int32 exprID, uint32 &numberOfRecursions, uint32 maxThreshold, PrimType pty) {
    if (numberOfRecursions++ > maxThreshold) {
      return nullptr;
    }
    auto it = caches.at(bbID).find(exprID);
    if (it != caches.at(bbID).end()) {
      auto vr = it->second.get();
      return (vr == nullptr) ? nullptr : GetVRAfterCvt(*vr, pty);
    }
    if (onlyRecordValueRangeInTempCache.top()) {
      auto itOfTemp = tempCaches.find(bbID);
      if (itOfTemp != tempCaches.end()) {
        auto itOfVR = itOfTemp->second.find(exprID);
        if (itOfVR != itOfTemp->second.end()) {
          return itOfVR->second.get();
        }
      }
    }
    auto *domBB = dom.GetDom(static_cast<Dominance::NodeId>(bbID.GetIdx()));
    return (domBB == nullptr || domBB->GetID() == 0) ?
        nullptr : FindValueRangeInCaches(BBId(domBB->GetID()), exprID, numberOfRecursions, maxThreshold, pty);
  }

  void Insert2AnalysisedArrayChecks(BBId bbID, MeExpr &array, MeExpr &index, Opcode op) {
    auto &analysisedArrayChecks = kOpcodeInfo.IsAssertLowerBoundary(op) ? analysisedLowerBoundChecks :
        kOpcodeInfo.IsAssertLeBoundary(op) ? analysisedAssignBoundChecks : analysisedUpperBoundChecks;
    if (analysisedArrayChecks.at(bbID).empty() ||
        analysisedArrayChecks.at(bbID).find(&array) == analysisedArrayChecks.at(bbID).end()) {
      analysisedArrayChecks.at(bbID)[&array] = std::set<MeExpr*>{ &index };
    } else {
      analysisedArrayChecks.at(bbID)[&array].insert(&index);
    }
  }

  void Insert2NewMergeBB2Opnd(BB &bb, const MeExpr &opnd) {
    auto exprID = static_cast<uint32>(opnd.GetExprID());
    auto ret = newMergeBB2Opnd.emplace(&bb, exprID);
    if (!ret.second) {
      CHECK_FATAL(ret.first->second == exprID, "must be equal");
    }
  }

  BB *GetNewCopyFallthruBB(BB &trueOrFalseBranch, const BB &bb) {
    auto it = trueOrFalseBranch2NewCopyFallthru.find(&trueOrFalseBranch);
    if (it == trueOrFalseBranch2NewCopyFallthru.end()) {
      return nullptr;
    }
    for (auto &pair : it->second) {
      if (pair.first == &bb) {
        return pair.second;
      }
    }
    return nullptr;
  }

  void Insert2TrueOrFalseBranch2NewCopyFallthru(BB &trueOrFalseBranch, BB &fallthruBB, BB &newFalthru) {
    auto it = trueOrFalseBranch2NewCopyFallthru.find(&trueOrFalseBranch);
    if (it == trueOrFalseBranch2NewCopyFallthru.end()) {
      trueOrFalseBranch2NewCopyFallthru[&trueOrFalseBranch] =
          std::vector<std::pair<BB*, BB*>>{ std::make_pair(&fallthruBB, &newFalthru) };
    } else {
      trueOrFalseBranch2NewCopyFallthru[&trueOrFalseBranch].push_back(std::make_pair(&fallthruBB, &newFalthru));
    }
  }

  void ResizeWhenCreateNewBB() {
    caches.resize(caches.size() + 1);
    analysisedLowerBoundChecks.resize(analysisedLowerBoundChecks.size() + 1);
    analysisedUpperBoundChecks.resize(analysisedUpperBoundChecks.size() + 1);
    analysisedAssignBoundChecks.resize(analysisedAssignBoundChecks.size() + 1);
  }

  bool IsGotoOrFallthruBB(const BB &bb) const {
    return bb.GetKind() == kBBGoto || bb.GetKind() == kBBFallthru;
  }

  void Insert2PairOfExprs(MeExpr &lhs, MeExpr &rhs, BB &bb) {
    if (lhs.IsVolatile() || rhs.IsVolatile() || lhs.GetMeOp() == kMeOpConst || rhs.GetMeOp() == kMeOpConst) {
      return;
    }
    auto it = pairOfExprs.find(&lhs);
    if (it == pairOfExprs.end()) {
      std::map<BB*, std::set<MeExpr*, CompareExpr>, CompareBB> valueOfPairOfExprs{
          std::make_pair<BB*, std::set<MeExpr*, CompareExpr>>(&bb, std::set<MeExpr*, CompareExpr>{&rhs }) };
      pairOfExprs[&lhs] = valueOfPairOfExprs;
    } else if (it->second.find(&bb) == it->second.end()) {
      pairOfExprs[&lhs][&bb] = std::set<MeExpr*, CompareExpr>{&rhs };
    } else {
      pairOfExprs[&lhs][&bb].insert(&rhs);
    }
  }

  // If the vr of lhs is equal to the vr of rhs in bb.
  bool FindPairOfExprs(MeExpr &lhs, const MeExpr &rhs, const BB &bb) const {
    auto it = pairOfExprs.find(&lhs);
    if (it == pairOfExprs.end()) {
      return false;
    }
    for (auto itOfValueMap = it->second.begin(); itOfValueMap != it->second.end(); ++itOfValueMap) {
      auto bbOfPair = itOfValueMap->first;
      auto exprs = itOfValueMap->second;
      if (!dom.Dominate(*bbOfPair, bb)) {
        continue;
      }
      for (auto itOfExprs = exprs.begin(); itOfExprs != exprs.end(); ++itOfExprs) {
        if (*itOfExprs == &rhs && (*itOfExprs)->GetPrimType() == rhs.GetPrimType()) {
          return true;
        }
      }
    }
    return false;
  }

  void JudgeEqual(MeExpr &expr, ValueRange &vrOfLHS, ValueRange &vrOfRHS, std::unique_ptr<ValueRange> &valueRangePtr);

  // The pairOfExprs map collects the exprs which have the same valueRange in bbs,
  // the pair of expr and preExpr is element of pairOfExprs.
  ValueRange *FindValueRangeWithCompareOp(const BB &bb, MeExpr &expr, uint32 &numberOfRecursions,
      std::unordered_set<int32> &foundExprs, uint32 maxThreshold);

  void DealWithPhi(const BB &bb);
  void DealWithCondGoto(BB &bb, MeStmt &stmt);
  void DealWithCondGotoWithOneOpnd(BB &bb, CondGotoMeStmt &brMeStmt);
  void InsertValueRangeOfCondExpr2Caches(BB &bb, const MeStmt &stmt);
  void DealWithBrStmtWithOneOpnd(BB &bb, const CondGotoMeStmt &stmt, MeExpr &opnd, Opcode op);
  bool OverflowOrUnderflow(PrimType pType, int64 lhs, int64 rhs) const;
  void DealWithAssign(BB &bb, const MeStmt &stmt);
  bool IsConstant(const BB &bb, MeExpr &expr, int64 &constant, bool canNotBeNotEqual = true);
  std::unique_ptr<ValueRange> CreateValueRangeForPhi(
      LoopDesc &loop, const BB &bb, ScalarMeExpr &init, ScalarMeExpr &backedge,
      const ScalarMeExpr &lhsOfPhi);
  bool AddOrSubWithConstant(PrimType pType, Opcode op, int64 lhsConstant, int64 rhsConstant, int64 &res) const;
  std::unique_ptr<ValueRange> NegValueRange(const BB &bb, MeExpr &opnd, uint32 &numberOfRecursions,
      std::unordered_set<int32> &foundExprs);
  bool AddOrSubWithBound(Bound oldBound, Bound &resBound, int64 rhsConstant, Opcode op);
  std::unique_ptr<ValueRange> AddOrSubWithValueRange(Opcode op, ValueRange &valueRange, int64 rhsConstant);
  std::unique_ptr<ValueRange> AddOrSubWithValueRange(
      Opcode op, ValueRange &valueRangeLeft, ValueRange &valueRangeRight);
  std::unique_ptr<ValueRange> DealWithAddOrSub(const BB &bb, const OpMeExpr &opMeExpr);
  bool CanComputeLoopIndVar(const MeExpr &phiLHS, MeExpr &expr, int64 &constant);
  std::unique_ptr<ValueRange> RemWithValueRange(const BB &bb, const OpMeExpr &opMeExpr, int64 rhsConstant);
  std::unique_ptr<ValueRange> RemWithRhsValueRange(const OpMeExpr &opMeExpr, int64 rhsConstant) const;
  std::unique_ptr<ValueRange> DealWithRem(const BB &bb, const MeExpr &lhsVar, const OpMeExpr &opMeExpr);
  Bound Max(Bound leftBound, Bound rightBound);
  Bound Min(Bound leftBound, Bound rightBound);
  InRangeType InRange(const BB &bb, const ValueRange &rangeTemp, const ValueRange &range, bool lowerIsZero = false);
  std::unique_ptr<ValueRange> CombineTwoValueRange(const ValueRange &leftRange,
                                                   const ValueRange &rightRange, bool merge = false);
  void DealWithArrayLength(const BB &bb, MeExpr &lhs, MeExpr &rhs);
  void DealWithArrayCheck(BB &bb, MeStmt &meStmt, MeExpr &meExpr);
  void DealWithArrayCheck();
  bool IfAnalysisedBefore(const BB &bb, const MeStmt &stmt);
  std::unique_ptr<ValueRange> MergeValueRangeOfPhiOperands(const BB &bb, MePhiNode &mePhiNode);
  void ReplaceBoundForSpecialLoopRangeValue(LoopDesc &loop, ValueRange &valueRangeOfIndex, bool upperIsSpecial);
  std::unique_ptr<ValueRange> CreateValueRangeForMonotonicIncreaseVar(
      const LoopDesc &loop, BB &exitBB, const BB &bb, OpMeExpr &opMeExpr,
      MeExpr &opnd1, Bound &initBound, int64 stride);
  std::unique_ptr<ValueRange> CreateValueRangeForMonotonicDecreaseVar(
      const LoopDesc &loop, BB &exitBB, const BB &bb, OpMeExpr &opMeExpr,
      MeExpr &opnd1, Bound &initBound, int64 stride);
  void CreateValueRangeForLeOrLt(const MeExpr &opnd, const ValueRange *leftRange, Bound newRightUpper,
                                 Bound newRightLower, const BB &trueBranch, const BB &falseBranch, bool isAccurate);
  void CreateValueRangeForGeOrGt(const MeExpr &opnd, const ValueRange *leftRange, Bound newRightUpper,
                                 Bound newRightLower, const BB &trueBranch, const BB &falseBranch, bool isAccurate);
  void CreateValueRangeForCondGoto(
      const MeExpr &opnd, Opcode op, ValueRange *leftRange,
      ValueRange &rightRange, const BB &trueBranch, const BB &falseBranch);
  void DealWithCondGoto(BB &bb, Opcode op, ValueRange *leftRange, ValueRange &rightRange,
                        const CondGotoMeStmt &brMeStmt);
  bool CreateNewBoundWhenAddOrSub(Opcode op, Bound bound, int64 rhsConstant, Bound &res);
  std::unique_ptr<ValueRange> CopyValueRange(ValueRange &valueRange, PrimType pType = PTY_begin);
  bool LowerInRange(const BB &bb, Bound lowerTemp, Bound lower, bool lowerIsZero);
  bool UpperInRange(const BB &bb, Bound upperTemp, Bound upper, bool upperIsArrayLength);
  void PrepareForSSAUpdateWhenPredBBIsRemoved(const BB &pred, BB &bb, ScalarMeExpr *updateSSAExceptTheScalarExpr,
      std::map<OStIdx, std::set<BB*>> &ssaupdateCandsForCondExpr);
  void InsertOstOfPhi2Cands(BB &bb, size_t i, ScalarMeExpr *updateSSAExceptTheScalarExpr,
                            std::map<OStIdx, std::set<BB*>> &ssaupdateCandsForCondExpr, bool setPhiIsDead = false);
  void InsertOstOfPhi2Cands(BB &bb, size_t i);
  void AnalysisUnreachableBBOrEdge(BB &bb, BB &unreachableBB, BB &succBB);
  void CreateValueRangeForNeOrEq(const MeExpr &opnd, const ValueRange *leftRange,
                                 ValueRange &rightRange, const BB &trueBranch, const BB &falseBranch);
  void DeleteUnreachableBBs();
  void ChangeLoop2Goto(LoopDesc &loop, BB &bb, BB &succBB, const BB &unreachableBB);
  void UpdateTryAttribute(BB &bb);
  void GetTrueAndFalseBranch(Opcode op, BB &bb, BB *&trueBranch, BB *&falseBranch) const;
  Opcode GetTheOppositeOp(Opcode op) const;
  bool GetValueRangeOfCondGotoOpnd(const BB &bb, OpMeExpr &opMeExpr, MeExpr &opnd, ValueRange *&valueRange,
                                   std::unique_ptr<ValueRange> &rightRangePtr);
  bool ConditionBBCanBeDeletedAfterOPNeOrEq(BB &bb, ValueRange &leftRange, ValueRange &rightRange, BB &falseBranch,
                                            BB &trueBranch);
  bool ConditionEdgeCanBeDeleted(BB &pred, BB &bb, const ValueRange *leftRange,
      const ValueRange *rightRange, BB &falseBranch, BB &trueBranch, PrimType opndType, Opcode op,
      ScalarMeExpr *updateSSAExceptTheScalarExpr, std::map<OStIdx, std::set<BB*>> &ssaupdateCandsForCondExpr);
  void GetSizeOfUnreachableBBsAndReachableBB(BB &bb, size_t &unreachableBB, BB *&reachableBB);
  bool ConditionEdgeCanBeDeleted(BB &bb, MeExpr &opnd0, ValueRange *rightRange, PrimType opndType, Opcode op);
  bool OnlyHaveCondGotoStmt(BB &bb) const;
  bool RemoveUnreachableEdge(BB &pred, BB &bb, BB &trueBranch, ScalarMeExpr *updateSSAExceptTheScalarExpr,
                             std::map<OStIdx, std::set<BB*>> &ssaupdateCandsForCondExpr);
  void RemoveUnreachableBB(BB &condGotoBB, BB &trueBranch, ScalarMeExpr *updateSSAExceptTheScalarExpr,
                           std::map<OStIdx, std::set<BB*>> &ssaupdateCandsForCondExpr);
  BB *CreateNewGotoBBWithoutCondGotoStmt(BB &bb);
  void CopyMeStmts(BB &fromBB, BB &toBB);
  bool ChangeTheSuccOfPred2TrueBranch(BB &pred, BB &bb, BB &trueBranch, ScalarMeExpr *updateSSAExceptTheScalarExpr,
                                      std::map<OStIdx, std::set<BB*>> &ssaupdateCandsForCondExpr);
  bool CopyFallthruBBAndRemoveUnreachableEdge(
      BB &pred, BB &bb, BB &trueBranch, ScalarMeExpr *updateSSAExceptTheScalarExpr,
      std::map<OStIdx, std::set<BB*>> &ssaupdateCandsForCondExpr);
  size_t GetRealPredSize(const BB &bb) const;
  bool RemoveTheEdgeOfPredBB(BB &pred, BB &bb, BB &trueBranch, ScalarMeExpr *updateSSAExceptTheScalarExpr,
                             std::map<OStIdx, std::set<BB*>> &ssaupdateCandsForCondExpr);
  void DealWithCondGotoWhenRightRangeIsNotExist(BB &bb, const MeExpr &opnd0, MeExpr &opnd1,
                                                Opcode opOfBrStmt, Opcode conditionalOp, ValueRange *valueRangeOfLeft);
  MeExpr *GetDefOfBase(const IvarMeExpr &ivar) const;
  std::unique_ptr<ValueRange> DealWithMeOp(const BB &bb, const MeStmt &stmt);
  void ReplaceOpndByDef(const BB &bb, MeExpr &currOpnd, MeExpr *&predOpnd, MePhiNode *&phi, bool &thePhiIsInBB);
  bool AnalysisValueRangeInPredsOfCondGotoBB(BB &bb, MeExpr *opnd0, MeExpr &currOpnd,
      ValueRange *rightRange, BB &falseBranch, BB &trueBranch, PrimType opndType, Opcode op, BB &condGoto);
  void CreateLabelForTargetBB(BB &pred, BB &newBB);
  size_t FindBBInSuccs(const BB &bb, const BB &succBB) const;
  void DealWithOperand(const BB &bb, MeStmt &stmt, MeExpr &meExpr);
  void CollectMeExpr(const BB &bb, MeStmt &stmt, MeExpr &meExpr,
                     std::map<MeExpr*, std::pair<int64, PrimType>> &expr2ConstantValue);
  void ReplaceOpndWithConstMeExpr(const BB &bb, MeStmt &stmt);
  bool OnlyHaveOneCondGotoPredBB(const BB &bb, const BB &condGotoBB) const;
  void GetValueRangeForUnsignedInt(const BB &bb, OpMeExpr &opMeExpr, const MeExpr &opnd, ValueRange *&valueRange,
                                   std::unique_ptr<ValueRange> &rightRangePtr);
  void GetValueRangeOfCRNode(const BB &bb, CRNode &opndOfCRNode, std::unique_ptr<ValueRange> &resValueRange,
                             PrimType pTypeOfArray);
  std::unique_ptr<ValueRange> GetValueRangeOfCRNodes(
      const BB &bb, PrimType pTypeOfArray, std::vector<CRNode*> &crNodes);
  bool DealWithAssertNonnull(BB &bb, const MeStmt &meStmt);
  bool DealWithBoundaryCheck(BB &bb, MeStmt &meStmt);
  MeExpr *GetAddressOfIndexOrBound(MeExpr &expr) const;
  std::unique_ptr<ValueRange> FindValueRangeInCurrBBOrDominateBBs(const BB &bb, MeExpr &opnd);
  bool ThePhiLHSIsLoopVariable(const LoopDesc &loop, const MeExpr &opnd) const;
  bool IsLoopVariable(const LoopDesc &loop, const MeExpr &opnd) const;
  void CollectIndexOpndWithBoundInLoop(
      LoopDesc &loop, BB &bb, MeStmt &meStmt, MeExpr &opnd, std::map<MeExpr*, MeExpr*> &index2NewExpr);
  bool CompareConstantOfIndexAndLength(
      const MeStmt &meStmt, const ValueRange &valueRangeOfIndex, ValueRange &valueRangeOfLengthPtr, Opcode op);
  bool CompareIndexWithUpper(const BB &bb, const MeStmt &meStmt, const ValueRange &valueRangeOfIndex,
                             ValueRange &valueRangeOfLengthPtr, Opcode op, const MeExpr *indexOpnd = nullptr);
  bool DealWithAssertLtOrLe(BB &bb, MeStmt &meStmt, CRNode &indexCR, CRNode &boundCR, Opcode op);
  void DealWithCVT(const BB &bb, MeStmt &stmt, MeExpr *operand, size_t i, bool dealWithStmt = false);
  std::unique_ptr<ValueRange> ZeroIsInRange(const ValueRange &valueRange);
  void DealWithNeg(const BB &bb, const OpMeExpr &opMeExpr);
  bool DealWithCVT(const BB &bb, MeStmt &stmt, OpMeExpr &opMeExpr);
  bool IfTheLowerOrUpperOfLeftRangeEqualToTheRightRange(
          const ValueRange &leftRange, ValueRange &rightRange, bool isLower) const;
  bool DealWithSpecialCondGoto(const BB &bb, OpMeExpr &opMeExpr, const ValueRange &leftRange, ValueRange &rightRange,
                               CondGotoMeStmt &brMeStmt);
  void UpdateOrDeleteValueRange(const MeExpr &opnd, std::unique_ptr<ValueRange> valueRange, const BB &branch);
  void Insert2UnreachableBBs(BB &unreachableBB);
  void DeleteThePhiNodeWhichOnlyHasOneOpnd(BB &bb, ScalarMeExpr *updateSSAExceptTheScalarExpr,
      std::map<OStIdx, std::set<BB*>> &ssaupdateCandsForCondExpr);
  void DealWithCallassigned(const BB &bb, MeStmt &stmt);
  void DeleteAssertNonNull();
  void DeleteBoundaryCheck();
  bool MustBeFallthruOrGoto(const BB &defBB, const BB &bb) const;
  std::unique_ptr<ValueRange> AntiValueRange(ValueRange &valueRange);
  void DeleteUnreachableBBs(BB &curBB, BB &falseBranch, BB &trueBranch);
  void PropValueRangeFromCondGotoToTrueAndFalseBranch(
      const MeExpr &opnd0, ValueRange &rightRange, const BB &falseBranch, const BB &trueBranch);
  bool CodeSizeIsOverflowOrTheOpOfStmtIsNotSupported(const BB &bb);
  void DealWithSwitch(BB &bb, MeStmt &stmt);
  bool DealWithSwitchWhenOpndIsConstant(BB &bb, BB *defaultBB, const ValueRange *valueRange,
      const SwitchMeStmt &switchMeStmt);
  bool AnalysisUnreachableForGeOrGt(BB &bb, const CondGotoMeStmt &brMeStmt, const ValueRange &leftRange);
  bool AnalysisUnreachableForLeOrLt(BB &bb, const CondGotoMeStmt &brMeStmt, const ValueRange &leftRange);
  bool AnalysisUnreachableForEqOrNe(BB &bb, const CondGotoMeStmt &brMeStmt, const ValueRange &leftRange);
  bool DealWithVariableRange(BB &bb, const CondGotoMeStmt &brMeStmt, const ValueRange &leftRange);
  std::unique_ptr<ValueRange> MergeValuerangeOfPhi(std::vector<std::unique_ptr<ValueRange>> &valueRangeOfPhi);
  std::unique_ptr<ValueRange> MakeMonotonicIncreaseOrDecreaseValueRangeForPhi(int stride, Bound &initBound) const;
  void CreateVRForPhi(const LoopDesc &loop);
  void TravelBBs(std::vector<BB *> &reversePostOrderOfLoopBBs);
  std::unique_ptr<ValueRange> CreateInitVRForPhi(LoopDesc &loop, const BB &bb, ScalarMeExpr &init,
      ScalarMeExpr &backedge, const ScalarMeExpr &lhsOfPhi);
  void MergeValueRangeOfPhiOperands(const LoopDesc &loop, const BB &bb,
      std::vector<std::unique_ptr<ValueRange>> &valueRangeOfInitExprs, size_t indexOfInitExpr);
  std::unique_ptr<ValueRange> MakeMonotonicIncreaseOrDecreaseValueRangeForPhi(int64 stride, Bound &initBound) const;
  bool MergeVrOrInitAndBackedge(MePhiNode &mePhiNode, ValueRange &vrOfInitExpr,
      ValueRange &valueRange, Bound &resBound);
  void ReplaceUsePoints(MePhiNode *phi);
  void CreateVRWithBitsSize(const BB &bb, const OpMeExpr &opMeExpr);
  MeExpr &GetVersionOfOpndInPred(const BB &pred, const BB &bb, MeExpr &expr, const BB &condGoto);
  std::unique_ptr<ValueRange> GetValueRangeOfLHS(const BB &pred, const BB &bb, MeExpr &expr, const BB &condGoto);
  Opcode GetOpAfterSwapThePositionsOfTwoOperands(Opcode op) const;
  bool IsSubOpndOfExpr(const MeExpr &expr, const MeExpr &subExpr) const;
  void UpdateProfile(BB &pred, BB &bb, const BB &targetBB) const;
  bool TheValueRangeOfOpndAndSubOpndAreEqual(const MeExpr &opnd) const;
  void CalculateVROfSubOpnd(BBId bbID, const MeExpr &opnd, ValueRange &valueRange);
  void CreateValueRangeForSubOpnd(const MeExpr &opnd, const BB &trueBranch, const BB &falseBranch,
      ValueRange &resTrueBranchVR, ValueRange &resFalseBranchVR);
  ValueRange *DealWithNegWhenFindValueRange(const BB &bb, const MeExpr &expr,
      uint32 &numberOfRecursions, std::unordered_set<int32> &foundExprs, uint32 maxThreshold);
  void MergeNotEqualRanges(const MeExpr &opnd, const ValueRange *leftRange, ValueRange &rightRange,
      const BB &trueBranch);
  bool DealWithMulNode(const BB &bb, CRNode &opndOfCRNode,
      std::unique_ptr<ValueRange> &resValueRange, PrimType pTypeOfArray);

  template<typename T>
  bool IsOverflowAfterMul(T lhs, T rhs, PrimType pty);
  bool HasDefPointInPred(const BB &begin, const BB &end, const ScalarMeExpr &opnd);
  bool CanIgnoreTheDefPoint(const MeStmt &stmt, const BB &end, const ScalarMeExpr &expr) const;

  MeFunction &func;
  MeIRMap &irMap;
  Dominance &dom;
  MemPool &memPool;
  MapleAllocator mpAllocator;
  IdentifyLoops *loops;
  std::vector<std::map<int32, std::unique_ptr<ValueRange>>> caches;
  std::map<BBId, std::map<int32, std::unique_ptr<ValueRange>>> tempCaches;
  std::set<MeExpr*> lengthSet;
  std::vector<std::map<MeExpr*, std::set<MeExpr*>>> analysisedLowerBoundChecks;
  std::vector<std::map<MeExpr*, std::set<MeExpr*>>> analysisedUpperBoundChecks;
  std::vector<std::map<MeExpr*, std::set<MeExpr*>>> analysisedAssignBoundChecks;
  std::map<MeExpr*, MeExpr*> length2Def;
  std::set<BB*> unreachableBBs;
  std::map<OStIdx, std::unique_ptr<std::set<BBId>>> &cands;
  LoopScalarAnalysisResult &sa;
  std::unordered_map<BB*, BB*> loopHead2TrueBranch;
  std::unordered_map<BB*, uint32> newMergeBB2Opnd;
  std::set<std::unique_ptr<ValueRange>> valueRanges;
  std::unordered_map<BB*, std::vector<std::pair<BB*, BB*>>> trueOrFalseBranch2NewCopyFallthru;
  // NewMergeBB must be true or false branch of condGoto bb and some preds of condGotoBB jump to this after opt.
  BB *newMergeBB = nullptr;
  bool hasCondGotoFromDomToPredBB = false;
  bool isCFGChange = false;
  bool needUpdateSSA = false;
  uint32 codeSizeCost = 0;
  SafetyCheck *safetyCheckNonnull = nullptr;
  SafetyCheck *safetyCheckBoundary = nullptr;
  // The map collects the exprs which have the same valueRange in bbs.
  std::map<MeExpr*, std::map<BB*, std::set<MeExpr*, CompareExpr>, CompareBB>, CompareExpr> pairOfExprs;
  bool dealWithPhi = false;
  bool dealWithAssert = false;
  bool onlyPropVR = false; // When need deal with check, do not opt cfg.
  MeExprUseInfo *useInfo = nullptr;
  std::stack<bool> onlyPropVRStack;
  std::stack<bool> onlyRecordValueRangeInTempCache;
  MapleVector<BaseGraphNode*>::iterator currItOfTravelReversePostOrder;
  bool defByStmtInUDChain = false;
};

MAPLE_FUNC_PHASE_DECLARE(MEValueRangePropagation, MeFunction)
}  // namespace maple
#endif
