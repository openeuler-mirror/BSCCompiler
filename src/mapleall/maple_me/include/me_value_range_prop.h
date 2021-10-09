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
int64 GetMinNumber(PrimType primType);
int64 GetMaxNumber(PrimType primType);
bool IsNeededPrimType(PrimType prim);

class Bound {
 public:
  Bound() = default;
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

  int64 GetConstant() const {
    return constant;
  }

  PrimType GetPrimType() const {
    return primType;
  }

  void SetPrimType(PrimType type) {
    primType = type;
  }

  bool IsEqual(Bound bound) {
    return var == bound.GetVar() && constant == bound.GetConstant() && primType && bound.GetPrimType();
  }

 private:
  MeExpr *var;
  int64 constant;
  PrimType primType;
};

enum RangeType {
  kLowerAndUpper,
  kSpecialUpperForLoop,
  kSpecialLowerForLoop,
  kOnlyHasLowerBound,
  kOnlyHasUpperBound,
  kNotEqual,
  kEqual
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
  RangePair() = default;
  RangePair(Bound l, Bound u) : lower(l), upper(u) {};
  Lower lower;
  Upper upper;
};

class ValueRange {
 public:
  ValueRange(Bound bound, int64 argStride, RangeType type) : stride(argStride), rangeType(type) {
    range.bound = bound;
  }

  ValueRange(Bound lower, Bound upper, RangeType type) : rangeType(type) {
    range.pair.lower = lower;
    range.pair.upper = upper;
  }

  ValueRange(Bound bound, RangeType type) : rangeType(type) {
    range.bound = bound;
  }

  ~ValueRange() = default;

  void SetRangePair(Bound lower, Bound upper) {
    range.pair.lower = lower;
    range.pair.upper = upper;
  }

  const RangePair GetLowerAndupper() const {
    return range.pair;
  }

  void SetLower(Lower lower) {
    range.pair.lower = lower;
  }

  Bound GetLower() const {
    return range.pair.lower;
  }

  void SetUpper(Upper upper) {
    range.pair.upper = upper;
  }

  Bound GetUpper() {
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

  Bound GetLower() {
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

  void SetBound(Bound argBound) {
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

  bool IsConstant() {
    return (rangeType == kEqual && range.bound.GetVar() == nullptr) ||
           (rangeType == kNotEqual && range.bound.GetVar() == nullptr) ||
           (rangeType == kLowerAndUpper && range.pair.lower.GetVar() == nullptr &&
            range.pair.lower.GetVar() == range.pair.upper.GetVar() &&
            range.pair.lower.GetConstant() == range.pair.upper.GetConstant());
  }

  bool IsConstantLowerAndUpper() {
    return (rangeType == kEqual && range.bound.GetVar() == nullptr) ||
           (rangeType == kLowerAndUpper && range.pair.lower.GetVar() == nullptr &&
            range.pair.upper.GetVar() == nullptr);
  }

  bool IsBiggerThanZero() {
    return (IsConstantLowerAndUpper() && GetLower().GetConstant() >= 0 && GetUpper().GetConstant() >= 0 &&
        GetLower().GetConstant() <= GetUpper().GetConstant()) ||
        (rangeType == kSpecialUpperForLoop && GetLower().GetConstant() >= 0 && GetLower().GetVar() == 0) ||
        (rangeType == kOnlyHasLowerBound && GetLower().GetConstant() >= 0 && GetLower().GetVar() == 0);
  }

  static Bound MinBound(PrimType pType) {
    return Bound(nullptr, GetMinNumber(pType), pType);
  }

  static Bound MaxBound(PrimType pType) {
    return Bound(nullptr, GetMaxNumber(pType), pType);
  }

  bool IsNotEqualZero() {
    return rangeType == kNotEqual && range.bound.GetVar() == nullptr && range.bound.GetConstant() == 0;
  }

  bool IsEqual(ValueRange *valueRangeRight);

 private:
  union {
    RangePair pair;
    Bound bound;
  } range;
  int64 stride = 0;
  RangeType rangeType;
};

class ValueRangePropagation {
 public:
  static bool isDebug;

  ValueRangePropagation(MeFunction &meFunc, MeIRMap &argIRMap, Dominance &argDom,
                        IdentifyLoops *argLoops, MemPool &pool, MapleMap<OStIdx, MapleSet<BBId>*> &candsTem,
                        LoopScalarAnalysisResult &currSA)
      : func(meFunc), irMap(argIRMap), dom(argDom), memPool(pool), mpAllocator(&pool), loops(argLoops),
        caches(meFunc.GetCfg()->GetAllBBs().size()), analysisedLowerBoundChecks(meFunc.GetCfg()->GetAllBBs().size()),
        analysisedUpperBoundChecks(meFunc.GetCfg()->GetAllBBs().size()), cands(candsTem), sa(currSA) {}
  ~ValueRangePropagation() = default;

  void DumpCaches();
  void Execute();

  bool IsCFGChange() const {
    return isCFGChange;
  }

  bool NeedUpdateSSA() const {
    return needUpdateSSA;
  }

 private:
  bool IsBiggerThanMaxInt64(ValueRange &valueRange) const {
    PrimType lowerPrim = valueRange.GetLower().GetPrimType();
    PrimType upperPrim = valueRange.GetUpper().GetPrimType();

    if (IsUnsignedInteger(lowerPrim) && GetPrimTypeSize(lowerPrim) == 4) { // 32bit
      lowerPrim = PTY_u32;
    }
    if (IsUnsignedInteger(upperPrim) && GetPrimTypeSize(upperPrim) == 4) { // 32bit
      upperPrim = PTY_u32;
    }
    if (IsUnsignedInteger(lowerPrim) && GetPrimTypeSize(lowerPrim) == 8) { // 64bit
      lowerPrim = PTY_u64;
    }
    if (IsUnsignedInteger(upperPrim) && GetPrimTypeSize(upperPrim) == 8) { // 64bit
      upperPrim = PTY_u64;
    }

    if (lowerPrim == PTY_u64) {
      if (static_cast<uint64>(valueRange.GetLower().GetConstant()) > GetMaxNumber(PTY_i64)) {
        return true;
      }
    }

    if (upperPrim == PTY_u64) {
      if (static_cast<uint64>(valueRange.GetUpper().GetConstant()) > GetMaxNumber(PTY_i64)) {
        return true;
      }
    }
    return false;
  }

  bool Insert2Caches(BBId bbID, int32 exprID, std::unique_ptr<ValueRange> valueRange) {
    if (valueRange == nullptr) {
      caches.at(bbID)[exprID] = nullptr;
      return true;
    }
    if (IsBiggerThanMaxInt64(*valueRange)) {
      return false;
    }
    if (use2Defs.find(exprID) != use2Defs.end() && valueRange->IsConstant()) {
      for (auto it : use2Defs[exprID]) {
        caches.at(bbID)[it] = CopyValueRange(*valueRange.get(), valueRange->GetBound().GetPrimType());
      }
    }
    caches.at(bbID)[exprID] = std::move(valueRange);
    return true;
  }

  ValueRange *FindValueRangeInCurrentBB(BBId bbID, int32 exprID) {
    auto it = caches.at(bbID).find(exprID);
    return it != caches.at(bbID).end() ? it->second.get() : nullptr;
  }

  ValueRange *FindValueRangeInCaches(BBId bbID, int32 exprID) {
    auto *bb = func.GetCfg()->GetBBFromID(bbID);
    if (bb->GetKind() != kBBFallthru && bb->GetKind() != kBBGoto) {
      hasCondGotoFromDomToPredBB = true;
    }
    auto itOfTrueBranch2Pred = trueOrFalseBranch2NewPred.find(bb);
    if (itOfTrueBranch2Pred != trueOrFalseBranch2NewPred.end() && !itOfTrueBranch2Pred->second.empty()) {
      auto itOfBranch2Opnd = newMergeBB2Opnd.find(bb);
      CHECK_FATAL(itOfBranch2Opnd != newMergeBB2Opnd.end(), "must find");
      // need consider when the exprID is not equal to the one in trueOrFalseBranch2NewPred
      if (itOfBranch2Opnd->second == exprID) {
        newMergeBB = bb;
        return nullptr;
      }
    }
    auto it = caches.at(bbID).find(exprID);
    if (it != caches.at(bbID).end()) {
      return it->second.get();
    }
    auto *domBB = dom.GetDom(bbID);
    return (domBB == nullptr || domBB->GetBBId() == 0) ? nullptr : FindValueRangeInCaches(domBB->GetBBId(), exprID);
  }

  void Insert2AnalysisedArrayChecks(BBId bbID, MeExpr &array, MeExpr &index, Opcode op) {
    auto &analysisedArrayChecks = (op == OP_assertge) ? analysisedLowerBoundChecks : analysisedUpperBoundChecks;
    if (analysisedArrayChecks.at(bbID).empty() ||
        analysisedArrayChecks.at(bbID).find(&array) == analysisedArrayChecks.at(bbID).end()) {
      analysisedArrayChecks.at(bbID)[&array] = std::set<MeExpr*>{ &index };
    } else {
      analysisedArrayChecks.at(bbID)[&array].insert(&index);
    }
  }

  void Insert2NewMergeBB2Opnd(BB &bb, MeExpr &opnd) {
    auto ret = newMergeBB2Opnd.emplace(&bb, opnd.GetExprID());
    if (!ret.second) {
      CHECK_FATAL(ret.first->second == opnd.GetExprID(), "must be equal");
    }
  }

  bool IsConditionalOp(Opcode op) const {
    return op == OP_le || op == OP_lt || op == OP_ge || op == OP_gt || op == OP_ne || op == OP_eq;
  }

  BB *GetNewCopyFallthruBB(BB &trueOrFalseBranch, BB &bb) {
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

  void Insert2NeedDeleteBoundaryCheck(BB &bb, MeStmt &meStmt) {
    auto it = needDeletedBoundaryCheck.find(&bb);
    if (it == needDeletedBoundaryCheck.end()) {
      needDeletedBoundaryCheck[&bb] = std::set<MeStmt*>{ &meStmt };
    } else {
      it->second.insert(&meStmt);
    }
  }

  void DealWithPhi(BB &bb, MePhiNode &mePhiNode);
  void DealWithCondGoto(BB &bb, MeStmt &stmt);
  void DealWithCondGotoWithOneOpnd(BB &bb, CondGotoMeStmt &brMeStmt);
  void DealWithBrStmtWithOneOpnd(BB &bb, CondGotoMeStmt &stmt, MeExpr &opnd, Opcode op);
  bool OverflowOrUnderflow(PrimType primType, int64 lhs, int64 rhs);
  void DealWithAssign(const BB &bb, MeStmt &stmt);
  bool IsConstant(const BB &bb, MeExpr &expr, int64 &constant, bool canNotBeNotEqual = true);
  std::unique_ptr<ValueRange> CreateValueRangeForPhi(
      LoopDesc &loop, BB &bb, ScalarMeExpr &init, ScalarMeExpr &backedge, ScalarMeExpr &lhsOfPhi);
  bool AddOrSubWithConstant(PrimType primType, Opcode op, int64 lhsConstant, int64 rhsConstant, int64 &res);
  std::unique_ptr<ValueRange> AddOrSubWithValueRange(Opcode op, ValueRange &valueRange, int64 rhsConstant);
  std::unique_ptr<ValueRange> AddOrSubWithValueRange(
      Opcode op, ValueRange &valueRangeLeft, ValueRange &valueRangeRight);
  std::unique_ptr<ValueRange> DealWithAddOrSub(const BB &bb, const MeExpr &lhsVar, OpMeExpr &opMeExpr);
  bool CanComputeLoopIndVar(MeExpr &phiLHS, MeExpr &expr, int &constant);
  Bound Max(Bound leftBound, Bound rightBound);
  Bound Min(Bound leftBound, Bound rightBound);
  InRangeType InRange(const BB &bb, ValueRange &rangeTemp, ValueRange &range, bool lowerIsZero = false);
  std::unique_ptr<ValueRange> CombineTwoValueRange(ValueRange &leftRange, ValueRange &rightRange, bool merge = false);
  void DealWithArrayLength(const BB &bb, MeExpr &lhs, MeExpr &rhs);
  void DealWithArrayCheck(BB &bb, MeStmt &meStmt, MeExpr &meExpr);
  void DealWithArrayCheck();
  bool IfAnalysisedBefore(BB &bb, MeStmt &stmt);
  std::unique_ptr<ValueRange> MergeValueRangeOfPhiOperands(const BB &bb, MePhiNode &mePhiNode);
  void ReplaceBoundForSpecialLoopRangeValue(LoopDesc &loop, ValueRange &valueRangeOfIndex, bool upperIsSpecial);
  std::unique_ptr<ValueRange> CreateValueRangeForMonotonicIncreaseVar(
      LoopDesc &loop, BB &exitBB, BB &bb, OpMeExpr &opMeExpr, MeExpr &opnd1, Bound &initBound);
  std::unique_ptr<ValueRange> CreateValueRangeForMonotonicDecreaseVar(
      LoopDesc &loop, BB &exitBB, BB &bb, OpMeExpr &opMeExpr, MeExpr &opnd1, Bound &initBound);
  void DealWithOPLeOrLt(BB &bb, ValueRange *leftRange, Bound newRightUpper, Bound newRightLower,
                        const CondGotoMeStmt &brMeStmt);
  void DealWithOPGeOrGt(BB &bb, ValueRange *leftRange, Bound newRightUpper, Bound newRightLower,
                        const CondGotoMeStmt &brMeStmt);
  void DealWithCondGoto(BB &bb, Opcode op, ValueRange *leftRange, ValueRange &rightRange,
                        const CondGotoMeStmt &brMeStmt);
  bool CreateNewBoundWhenAddOrSub(Opcode op, Bound bound, int64 rhsConstant, Bound &res);
  std::unique_ptr<ValueRange> CopyValueRange(ValueRange &valueRange, PrimType primType = PTY_begin);
  bool LowerInRange(const BB &bb, Bound lowerTemp, Bound lower, bool lowerIsZero);
  bool UpperInRange(const BB &bb, Bound upperTemp, Bound upper, bool upperIsArrayLength);
  void InsertCandsForSSAUpdate(OStIdx ostIdx, const BB &bb);
  void AnalysisUnreachableBBOrEdge(BB &bb, BB &unreachableBB, BB &succBB);
  void DealWithOPNeOrEq(Opcode op, BB &bb, ValueRange *leftRange, ValueRange &rightRange,
                        const CondGotoMeStmt &brMeStmt);
  void CreateValueRangeForNeOrEq(
      MeExpr &opnd, ValueRange *leftRange, ValueRange &rightRange, BB &trueBranch, BB &falseBranch);
  void DeleteUnreachableBBs();
  bool BrStmtInRange(BB &bb, ValueRange &leftRange, ValueRange &rightRange, Opcode op,
                     bool judgeNotInRange = false);
  void ChangeLoop2WontExit(LoopDesc &loop, BB &bb, BB &succBB, BB &unreachableBB);
  int64 GetRealValue(int64 value, PrimType primType) const;
  void UpdateTryAttribute(BB &bb);
  void GetTrueAndFalseBranch(Opcode op, BB &bb, BB *&trueBranch, BB *&falseBranch) const;
  Opcode GetTheOppositeOp(Opcode op) const;
  bool GetValueRangeOfCondGotoOpnd(BB &bb, OpMeExpr &opMeExpr, MeExpr &opnd, ValueRange *&valueRange,
                                   std::unique_ptr<ValueRange> &rightRangePtr);
  bool ConditionBBCanBeDeletedAfterOPNeOrEq(BB &bb, ValueRange &leftRange, ValueRange &rightRange, BB &falseBranch,
                                            BB &trueBranch);
  bool ConditionEdgeCanBeDeletedAfterOPNeOrEq(MeExpr &opnd, BB &pred, BB &bb, ValueRange *leftRange,
      ValueRange &rightRange, BB &falseBranch, BB &trueBranch, PrimType opndType, bool dealWithShortCircuit = false);
  void DealWithOPNeOrEq(BB &bb, ValueRange *leftRange, ValueRange &rightRange, const CondGotoMeStmt &brMeStmt);
  void InsertCandsForSSAUpdate(BB &bb, bool insertDefBBOfPhiOpnds2Cands = false);
  void GetSizeOfUnreachableBBsAndReachableBB(BB &bb, size_t &unreachableBB, BB *&reachableBB);
  bool ConditionEdgeCanBeDeletedAfterOPNeOrEq(BB &bb, MeExpr &opnd0, ValueRange &rightRange, BB &falseBranch,
                                              BB &trueBranch, PrimType opndType, BB *condGoto = nullptr);
  bool OnlyHaveCondGotoStmt(BB &bb) const;
  bool RemoveUnreachableEdge(MeExpr &opnd, BB &pred, BB &bb, BB &trueBranch, bool dealWithShortCircuit = false);
  void RemoveUnreachableBB(BB &condGotoBB, BB &trueBranch);
  BB *CreateNewBasicBlockWithoutCondGotoStmt(BB &bb);
  void InsertCandsForSSAUpdate(MeStmt &meStmt, BB &bb);
  void CopyMeStmts(BB &fromBB, BB &toBB, bool copyWithoutCondGotoStmt = false);
  bool ChangeTheSuccOfPred2TrueBranch(BB &pred, BB &bb, BB &trueBranch);
  bool CopyFallthruBBAndRemoveUnreachableEdge(BB &pred, BB &bb, BB &trueBranch, bool dealWithShortCircuit = false);
  bool CopyFallthruBBAndRemoveUnreachableEdgeWhenDealWithShortCircuit(BB &pred, BB &bb, BB &trueBranch);
  size_t GetRealPredSize(const BB &bb) const;
  bool RemoveTheEdgeOfPredBB(BB &pred, BB &bb, BB &trueBranch);
  void DealWithCondGotoWhenRightRangeIsNotExist(BB &bb, MeExpr &opnd0, MeExpr &opnd1, Opcode op);
  MeExpr *GetDefOfBase(const IvarMeExpr &ivar) const;
  void DealWithMeOp(const BB &bb, MeStmt &stmt);
  void IsTheOpndOfCondGotoStmtIsPhi(
      BB &bb, MeExpr &opnd0, BB *&defBB, MapleVector<ScalarMeExpr*> &opnds, bool &thePhiIsInBB);
  bool AnalysisValueRangeInPredsOfCondGotoBB(BB &bb, MeExpr &opnd0, ValueRange &rightRange, BB &falseBranch,
                                             BB &trueBranch, PrimType opndType, BB *condGoto = nullptr);
  void CreateLabelForTargetBB(BB &pred, BB &newBB);
  size_t FindBBInSuccs(const BB &bb, const BB &succBB) const;
  void DealWithOperand(const BB &bb, MeStmt &stmt, MeExpr &meExpr);
  bool OnlyHaveOneCondGotoPredBB(const BB &bb, const BB &condGotoBB) const;
  void GetValueRangeForUnsignedInt(BB &bb, OpMeExpr &opMeExpr, MeExpr &opnd, ValueRange *&valueRange,
                                   std::unique_ptr<ValueRange> &rightRangePtr);
  void DealWithAssertNonnull(BB &bb, MeStmt &meStmt);
  void DealWithBoundaryCheck(BB &bb, MeStmt &meStmt);
  std::unique_ptr<ValueRange> FindValueRangeInCurrBBOrDominateBBs(BB &bb, MeExpr &opnd);
  std::unique_ptr<ValueRange> ComputeTheValueRangeOfIndex(
      std::unique_ptr<ValueRange> &valueRangeOfIndex, std::unique_ptr<ValueRange> &valueRangOfOpnd,
      int64 constant);
  std::unique_ptr<ValueRange> ComputeTheValueRangeOfIndex(
      std::unique_ptr<ValueRange> &valueRangeOfIndex, std::unique_ptr<ValueRange> &valueRangOfOpnd,
      ValueRange &constantValueRange);
  std::unique_ptr<ValueRange> GetTheValueRangeOfIndex(
      BB &bb, MeStmt &meStmt, CRAddNode &crAddNode, uint32 &byteSize, MeExpr *lengthExpr,
      ValueRange *valueRangeOfLengthPtr);
  bool DealWithAssertGe(BB &bb, MeStmt &meStmt, CRNode &indexCR, CRNode &boundCR);
  bool CompareIndexWithUpper(MeStmt &meStmt, ValueRange &valueRangeOfIndex,
                             int64 lengthValue, ValueRange &valueRangeOfLengthPtr, uint32 byteSize);
  bool GetTheValueRangeOfArrayLength(BB &bb, MeStmt &meStmt, CRAddNode &crADDNodeOfBound,
                                     MeExpr *&lengthExpr, int64 &lengthValue, ValueRange *&valueRangeOfLengthPtr,
                                     std::unique_ptr<ValueRange> &valueRangeOfLength, uint32 &byteSize);
  bool DealWithAssertlt(BB &bb, MeStmt &meStmt, CRNode &indexCR, CRNode &boundCR);
  void DealWithShortCricuit(MeExpr &opnd, ValueRange &rightRange, BB &falseBranch, BB &trueBranch, BB &newMerge,
                            PrimType opndType, bool &opt, std::unordered_map<BB*, ValueRange*> &pred2ValueRange,
      bool hasCondGotoFromDomToPredBB = false);
  void DealWithCVT(const BB &bb, MeStmt &stmt, MeExpr *operand, size_t i, bool dealWithStmt = false);
  std::unique_ptr<ValueRange> ZeroIsInRange(ValueRange &valueRange);
  void DealWithCVT(const BB &bb, OpMeExpr &opMeExpr);
  bool IfTheLowerOrUpperOfLeftRangeEqualToTheRightRange(ValueRange &leftRange, ValueRange &rightRange, bool isLower);
  bool DealWithSpecialCondGoto(OpMeExpr &opMeExpr, ValueRange &leftRange, ValueRange &rightRange,
                               CondGotoMeStmt &brMeStmt);
  void UpdateOrDeleteValueRange(const MeExpr &opnd, std::unique_ptr<ValueRange> valueRange, const BB &branch);
  void AnalysisUnreachableBBOrEdge(BB &unreachableBB);
  void MergeValueRangeOfPred(BB &bb, const MeExpr &opnd);
  void DeleteThePhiNodeWhichOnlyHasOneOpnd(BB &bb);
  void DealWithCallassigned(BB &bb, MeStmt &stmt);
  void DeleteAssertNonNull();
  void DeleteBoundaryCheck();
  bool MustBeFallthruOrGoto(const BB &defBB, const BB &bb);
  std::unique_ptr<ValueRange> AntiValueRange(ValueRange &valueRange);
  void PropValueRangeOfNewPredOfTrue2FalseBranch(
      MeExpr &opnd, BB &condGoto, BB &bb, std::unique_ptr<ValueRange> valueRange);
  void DeleteUnreachableBBs(BB &curBB, BB &falseBranch, BB &trueBranch);
  void PropValueRangeFromCondGotoToTrueAndFalseBranch(
      BB &curBB, MeExpr &opnd0, ValueRange &rightRange, BB &falseBranch, BB &trueBranch,
      std::unordered_map<BB*, ValueRange*> &pred2ValueRange);
  void PropValueRangeFromCondGotoToTrueAndFalseBranch(
      MeExpr &opnd0, ValueRange &rightRange, BB &falseBranch, BB &trueBranch);
  bool CodeSizeIsOverflow(const BB &bb);
  void ComputeCodeSize(MeExpr &meExpr, uint32 &cost);
  void ComputeCodeSize(const MeStmt &meStmt, uint32 &cost);
  void DealWithSwitch(MeStmt &stmt);
  std::unique_ptr<ValueRange> MergeValuerangeOfPhi(std::vector<std::unique_ptr<ValueRange>> &valueRangeOfPhi);
  std::unique_ptr<ValueRange> MakeMonotonicIncreaseOrDecreaseValueRangeForPhi(int stride, Bound &initBound);

  MeFunction &func;
  MeIRMap &irMap;
  Dominance &dom;
  MemPool &memPool;
  MapleAllocator mpAllocator;
  IdentifyLoops *loops;
  std::vector<std::map<int32, std::unique_ptr<ValueRange>>> caches;
  std::set<MeExpr*> lengthSet;
  std::vector<std::map<MeExpr*, std::set<MeExpr*>>> analysisedLowerBoundChecks;
  std::vector<std::map<MeExpr*, std::set<MeExpr*>>> analysisedUpperBoundChecks;
  std::map<MeExpr*, MeExpr*> length2Def;
  std::set<BB*> unreachableBBs;
  std::unordered_map<int32, std::set<int32>> use2Defs;
  MapleMap<OStIdx, MapleSet<BBId>*> &cands;
  LoopScalarAnalysisResult &sa;
  std::unordered_map<BB*, BB*> loopHead2TrueBranch;
  std::unordered_map<BB*, std::set<std::pair<BB*, MeExpr*>>> pred2NewSuccs;
  std::unordered_map<BB*, std::set<MeStmt*>> needDeletedAssertNonNull;
  std::unordered_map<BB*, std::set<MeStmt*>> needDeletedBoundaryCheck;
  // Collection of newMergeBB.
  std::unordered_map<BB*, std::vector<std::pair<BB*, ValueRange*>>> trueOrFalseBranch2NewPred;
  std::unordered_map<BB*, uint32> newMergeBB2Opnd;
  std::set<std::unique_ptr<ValueRange>> valueRanges;
  std::unordered_map<BB*, std::vector<std::pair<BB*, BB*>>> trueOrFalseBranch2NewCopyFallthru;
  // NewMergeBB must be true or false branch of condGoto bb and some preds of condGotoBB jump to this after opt.
  BB *newMergeBB = nullptr;
  bool hasCondGotoFromDomToPredBB = false;
  bool isCFGChange = false;
  bool needUpdateSSA = false;
  bool thePredEdgeIsRemoved = false;
  uint32 codeSizeCost = 0;
};

MAPLE_FUNC_PHASE_DECLARE(MEValueRangePropagation, MeFunction)
}  // namespace maple
#endif
