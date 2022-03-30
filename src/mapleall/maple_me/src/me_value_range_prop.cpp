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
#include "me_value_range_prop.h"
#include "me_dominance.h"
#include "me_abco.h"
#include "me_ssa_update.h"
#include "me_phase_manager.h"
#include "me_safety_warning.h"

namespace maple {
bool ValueRangePropagation::isDebug = false;
constexpr size_t kNumOperands = 2;
constexpr size_t kCodeSizeLimit = 2000;
constexpr std::uint64_t kInvaliedBound = 0xdeadbeef;
constexpr size_t kLimitOfLoopNestDepth = 10;
constexpr size_t kLimitOfLoopBBs = 500;

bool Bound::CanBeComparedWith(const Bound &bound) const {
  // Only bounds with same var can be compared
  // Only comparison between bounds with same primtype makes sense!
  if (primType != bound.GetPrimType()) {
    return false;
  }
  if (var == bound.GetVar()) {
    return true;
  }
  return (*this == MinBound(primType) || *this == MaxBound(primType) ||
          bound == MinBound(primType) || bound == MaxBound(primType));
}

bool Bound::operator<(const Bound &bound) const {
  if (!CanBeComparedWith(bound)) {
    return false;
  }
  if (*this == bound) {
    return false;
  }
  if (*this == MaxBound(primType) || bound == MinBound(primType)) {
    return false;
  }
  if (*this == MinBound(primType) || bound == MaxBound(primType)) {
    return true;
  }
  if (IsPrimitiveUnsigned(primType)) {
    if (var == nullptr && bound.var == nullptr) {
      return static_cast<uint64>(constant) < static_cast<uint64>(bound.GetConstant());
    }
  }
  return constant < bound.GetConstant();
}

void ValueRangePropagation::Execute() {
  useInfo = &irMap.GetExprUseInfo();
  if (!useInfo->UseInfoOfAllIsValid()) {
    useInfo->CollectUseInfoInFunc(&irMap, &dom, kUseInfoOfAllExpr);
  }
  // In reverse post order traversal, a bb is accessed before any of its successor bbs. So the range of def points would
  // be calculated before and need not calculate the range of use points repeatedly.
  for (auto bIt = dom.GetReversePostOrder().begin(); bIt != dom.GetReversePostOrder().end(); ++bIt) {
    currItOfTravelReversePostOrder = bIt;
    if (unreachableBBs.find(*bIt) != unreachableBBs.end()) {
      continue;
    }
    DealWithPhi(**bIt);
    for (auto it = (*bIt)->GetMeStmts().begin(); it != (*bIt)->GetMeStmts().end();) {
      bool deleteStmt = false;
      if (!onlyPropVR) {
        ReplaceOpndWithConstMeExpr(**bIt, *it);
      }
      bool isNotNeededPType = false;
      for (size_t i = 0; i < it->NumMeStmtOpnds(); ++i) {
        if (!IsNeededPrimType(it->GetOpnd(i)->GetPrimType())) {
          isNotNeededPType = true;
          continue;
        }
        DealWithOperand(**bIt, *it, *it->GetOpnd(i));
      }
      if (isNotNeededPType) {
        ++it;
        continue;
      }
      if (MeOption::safeRegionMode && (it->IsInSafeRegion()) &&
          kOpcodeInfo.IsCall(it->GetOp()) && instance_of<CallMeStmt>(*it)) {
        MIRFunction *callFunc =
            GlobalTables::GetFunctionTable().GetFunctionFromPuidx(static_cast<CallMeStmt&>(*it).GetPUIdx());
        if (callFunc->IsUnSafe()) {
          auto srcPosition = it->GetSrcPosition();
          FATAL(kLncFatal, "%s %d error: call unsafe function %s from safe region that requires safe function",
                func.GetMIRModule().GetFileNameFromFileNum(srcPosition.FileNum()).c_str(), srcPosition.LineNum(),
                callFunc->GetName().c_str());
        }
      }
      switch (it->GetOp()) {
        case OP_dassign:
        case OP_maydassign:
        case OP_regassign: {
          if (it->GetLHS() != nullptr && it->GetLHS()->IsVolatile()) {
            break;
          }
          DealWithAssign(**bIt, *it);
          break;
        }
        case OP_brfalse:
        case OP_brtrue: {
          DealWithCondGoto(**bIt, *it);
          InsertValueRangeOfCondExpr2Caches(**bIt, *it);
          break;
        }
        CASE_OP_ASSERT_NONNULL {
          // If the option safeRegion is open and the stmt is not in safe region, delete it.
          if (MeOption::safeRegionMode && !it->IsInSafeRegion()) {
            deleteStmt = true;
          } else if (DealWithAssertNonnull(**bIt, *it)) {
            if (ValueRangePropagation::isDebug) {
              LogInfo::MapleLogger() << "=========delete assert nonnull=========\n";
              it->Dump(&irMap);
              LogInfo::MapleLogger() << "=========delete assert nonnull=========\n";
            }
            deleteStmt = true;
          }
          break;
        }
        CASE_OP_ASSERT_BOUNDARY {
          // If the option safeRegion is open and the stmt is not in safe region, delete it.
          if (MeOption::safeRegionMode && !it->IsInSafeRegion()) {
            deleteStmt = true;
          } else if (DealWithBoundaryCheck(**bIt, *it)) {
            if (ValueRangePropagation::isDebug) {
              LogInfo::MapleLogger() << "=========delete boundary check=========\n";
              it->Dump(&irMap);
              LogInfo::MapleLogger() << "=========delete boundary check=========\n";
            }
            deleteStmt = true;
          }
          break;
        }
        case OP_callassigned: {
          DealWithCallassigned(**bIt, *it);
          break;
        }
        case OP_switch: {
          DealWithSwitch(**bIt, *it);
          break;
        }
        case OP_igoto:
        default:
          break;
      }
      if (deleteStmt) {
        (*bIt)->GetMeStmts().erase(it++);
      } else {
        ++it;
      }
    }
  }
  if (ValueRangePropagation::isDebug) {
    DumpCaches();
  }
  DeleteUnreachableBBs();
  useInfo->InvalidUseInfo();
}

void ValueRangePropagation::DealWithCallassigned(const BB &bb, MeStmt &stmt) {
  auto &callassign = static_cast<CallMeStmt&>(stmt);
  MIRFunction *callFunc = GlobalTables::GetFunctionTable().GetFunctionFromPuidx(callassign.GetPUIdx());
  if (callFunc->GetAttr(FUNCATTR_nonnull) && callassign.GetAssignedLHS() != nullptr) {
    (void)Insert2Caches(bb.GetBBId(), callassign.GetAssignedLHS()->GetExprID(),
        std::make_unique<ValueRange>(Bound(nullptr, 0, callassign.GetAssignedLHS()->GetPrimType()), kNotEqual));
  }
}

void ValueRangePropagation::DealWithSwitch(BB &bb, MeStmt &stmt) {
  auto &switchMeStmt = static_cast<SwitchMeStmt&>(stmt);
  auto *opnd = switchMeStmt.GetOpnd();
  std::set<BBId> bbOfCases;
  auto *defalutBB = func.GetCfg()->GetLabelBBAt(switchMeStmt.GetDefaultLabel());
  if (defalutBB != nullptr) {
    bbOfCases.insert(defalutBB->GetBBId());
  }
  std::vector<int64> valueOfCase;
  for (auto &pair : switchMeStmt.GetSwitchTable()) {
    valueOfCase.push_back(pair.first);
    auto *currbb = func.GetCfg()->GetLabelBBAt(pair.second);
    // Can prop value range to target bb only when the pred size of target bb is one and
    // only one case can jump to the target bb.
    if (currbb->GetPred().size() == 1 && bbOfCases.insert(currbb->GetBBId()).second) {
      (void)Insert2Caches(currbb->GetBBId(), opnd->GetExprID(),
                    std::make_unique<ValueRange>(Bound(nullptr, pair.first, PTY_i64), kEqual));
    } else {
      (void)Insert2Caches(currbb->GetBBId(), opnd->GetExprID(), nullptr);
    }
  }
  if (onlyPropVR || defalutBB == nullptr) {
    return;
  }
  // Delete the default branch when it is unreachable.
  auto *currBB = &bb;
  auto *valueRange = FindValueRange(*currBB, *opnd);
  while (valueRange == nullptr && currBB->GetPred().size() == 1) {
    currBB = currBB->GetPred(0);
    valueRange = FindValueRange(*currBB, *opnd);
  }
  if (valueRange == nullptr || !valueRange->IsConstantRange()) {
    return;
  }
  auto upper = valueRange->GetUpper().GetConstant();
  auto lower = valueRange->GetLower().GetConstant();
  if (upper < lower || static_cast<uint64>(upper - lower + 1) != valueOfCase.size()) {
    return;
  }
  std::sort(valueOfCase.begin(), valueOfCase.end());
  if (valueOfCase.front() == lower && valueOfCase.back() == upper) {
    if (ValueRangePropagation::isDebug) {
      LogInfo::MapleLogger() << "===========delete defaultBranch " << defalutBB->GetBBId() << "===========\n";
    }
    switchMeStmt.SetDefaultLabel(0);
    defalutBB->RemovePred(bb);
    isCFGChange = true;
  }
}

void ValueRangePropagation::DeleteThePhiNodeWhichOnlyHasOneOpnd(
    BB &bb, ScalarMeExpr *updateSSAExceptTheScalarExpr, std::map<OStIdx, std::set<BB*>> &ssaupdateCandsForCondExpr) {
  if (unreachableBBs.find(&bb) != unreachableBBs.end()) {
    return;
  }
  if (bb.GetMePhiList().empty()) {
    return;
  }
  if (bb.GetPred().size() == 1) {
    InsertOstOfPhi2Cands(bb, 0, updateSSAExceptTheScalarExpr, ssaupdateCandsForCondExpr, true);
    bb.GetMePhiList().clear();
  }
}

bool ValueRange::IsEqual(ValueRange *valueRangeRight) const  {
  if (valueRangeRight == nullptr) {
    return false;
  }
  if (rangeType != valueRangeRight->GetRangeType()) {
    return false;
  }
  switch (rangeType) {
    case kLowerAndUpper:
    case kSpecialUpperForLoop:
    case kSpecialLowerForLoop:
      return range.pair.upper == (valueRangeRight->GetUpper()) &&
             range.pair.lower == (valueRangeRight->GetLower());
    case kOnlyHasLowerBound:
      return range.pair.lower == (valueRangeRight->GetLower()) &&
             stride == valueRangeRight->GetStride();
    case kOnlyHasUpperBound:
      return range.pair.upper == (valueRangeRight->GetUpper()) &&
             stride == valueRangeRight->GetStride();
    case kNotEqual:
    case kEqual:
      return range.bound == (valueRangeRight->GetBound());
    default:
      CHECK_FATAL(false, "can not be here");
  }
}

// return nullptr means cannot merge, intersect = true : intersection set, intersect = false : union set
std::unique_ptr<ValueRange> MergeVR(const ValueRange &vr1, const ValueRange &vr2, bool intersect) {
  RangeType vr1Type = vr1.GetRangeType();
  RangeType vr2Type = vr2.GetRangeType();
  // Only these range type can be merged
  if ((vr1Type != kEqual && vr1Type != kNotEqual && vr1Type != kLowerAndUpper) ||
      (vr2Type != kEqual && vr2Type != kNotEqual && vr2Type != kLowerAndUpper)) {
    return nullptr;
  }
  // Enumerate all possible combinations of vr1Type and vr2Type
  switch (vr1Type) {
    case kLowerAndUpper: {
      Bound lower1 = vr1.GetLower();
      Bound upper1 = vr1.GetUpper();
      switch (vr2Type) {
        case kLowerAndUpper: {
          // [lower1 ...... upper1]
          //     [lower2 .......... upper2]
          Bound lower2 = vr2.GetLower();
          Bound upper2 = vr2.GetUpper();
          if (!lower1.CanBeComparedWith(lower2) || !upper1.CanBeComparedWith(upper2) ||
              !lower1.CanBeComparedWith(upper2) || !upper1.CanBeComparedWith(lower2)) {
            return nullptr;
          }
          if (upper2 < lower1 || upper1 < lower2) {
            return intersect ? std::make_unique<ValueRange>(kRTEmpty) /* always false */ : nullptr;
          } else if (upper2 == lower1) {
            return intersect ? std::make_unique<ValueRange>(lower1, kEqual)
                             : std::make_unique<ValueRange>(lower2, upper1, kLowerAndUpper);
          } else if (upper1 == lower2) {
            return intersect ? std::make_unique<ValueRange>(lower2, kEqual)
                             : std::make_unique<ValueRange>(lower1, upper2, kLowerAndUpper);
          } else {
            return intersect ?
                std::make_unique<ValueRange>(std::max(lower1, lower2), std::min(upper1, upper2), kLowerAndUpper) :
                std::make_unique<ValueRange>(std::min(lower1, lower2), std::max(upper1, upper2), kLowerAndUpper);
          }
        }
        case kEqual: {
          //                [lower1 ...... upper1]
          // bound : ^pos1          ^pos2          ^pos3
          Bound bound = vr2.GetBound();
          if (!lower1.CanBeComparedWith(bound) || !upper1.CanBeComparedWith(bound)) {
            return nullptr;
          }
          if (lower1 <= bound && bound <= upper1) {
            return intersect ? std::make_unique<ValueRange>(bound, kEqual)
                             : std::make_unique<ValueRange>(lower1, upper1, kLowerAndUpper);
          } else { // if (bound < lower1 || upper1 < bound)
            return intersect ? std::make_unique<ValueRange>(kRTEmpty) // always false
                             : nullptr; // can not merge
          }
        }
        case kNotEqual: {
          //                [lower1 ...... upper1]
          // bound : ^pos1  ^pos2      ^pos3     ^pos4   ^pos5
          Bound bound = vr2.GetBound();
          if (!lower1.CanBeComparedWith(bound) || !upper1.CanBeComparedWith(bound)) {
            return nullptr;
          }
          if (bound < lower1 || upper1 < bound) {
            return intersect ? std::make_unique<ValueRange>(lower1, upper1, kLowerAndUpper)
                             : std::make_unique<ValueRange>(bound, kNotEqual);
          } else if (lower1 < bound && bound < upper1) {
            return intersect ? nullptr : std::make_unique<ValueRange>(kRTComplete);
          } else if (bound == lower1) {
            return intersect ? std::make_unique<ValueRange>(++lower1, upper1, kLowerAndUpper)
                             : std::make_unique<ValueRange>(kRTComplete); // always true;
          } else { // bound == upper1
            return intersect ? std::make_unique<ValueRange>(lower1, --upper1, kLowerAndUpper)
                             : std::make_unique<ValueRange>(kRTComplete); // always true;
          }
        }
        default:
          return nullptr;
      }
    }
    case kEqual: {
      Bound b1 = vr1.GetBound();
      Bound b2 = vr2.GetBound();
      if (!b1.CanBeComparedWith(b2)) {
        return nullptr;
      }
      switch (vr2Type) {
        case kLowerAndUpper: {
          return MergeVR(vr2, vr1, intersect); // swap two vr
        }
        case kEqual: {
          if (b1 == b2) {
            return std::make_unique<ValueRange>(b1, kEqual);
          } else {
            return intersect ? std::make_unique<ValueRange>(kRTEmpty) : nullptr;
          }
        }
        case kNotEqual: {
          if (b1 == b2) {
            return intersect ? std::make_unique<ValueRange>(kRTEmpty) : std::make_unique<ValueRange>(kRTComplete);
          } else {
            return intersect ? std::make_unique<ValueRange>(b1, kEqual) : std::make_unique<ValueRange>(b2, kNotEqual);
          }
        }
        default:
          return nullptr;
      }
    }
    case kNotEqual: {
      switch (vr2Type) {
        case kLowerAndUpper:
        case kEqual: {
          return MergeVR(vr2, vr1, intersect); // swap two vr
        }
        case kNotEqual: {
          Bound b1 = vr1.GetBound();
          Bound b2 = vr2.GetBound();
          if (!b1.CanBeComparedWith(b2)) {
            return nullptr;
          }
          if (b1 == b2) {
            return std::make_unique<ValueRange>(b1, kNotEqual);
          } else {
            return intersect ? nullptr : std::make_unique<ValueRange>(kRTComplete);
          }
        }
        default:
          return nullptr;
      }
    }
    default:
      return nullptr;
  }
}

MeExpr *GetCmpExprFromBound(Bound b, MeExpr &expr, MeIRMap *irmap, Opcode op) {
  int64 val = b.GetConstant();
  MeExpr *var = const_cast<MeExpr *>(b.GetVar());
  PrimType ptyp = b.GetPrimType();
  MeExpr *constExpr = irmap->CreateIntConstMeExpr(val, ptyp);
  MeExpr *resExpr = nullptr;
  if (var == nullptr) {
    resExpr = irmap->CreateMeExprBinary(op, PTY_u1, expr, *constExpr);
  } else {
    if (val == 0) {
      resExpr = irmap->CreateMeExprBinary(op, PTY_u1, expr, *var);
    } else if (val > 0) {
      MeExpr *addExpr = irmap->CreateMeExprBinary(OP_add, ptyp, *var, *constExpr);
      static_cast<OpMeExpr*>(addExpr)->SetOpndType(ptyp);
      resExpr = irmap->CreateMeExprBinary(op, PTY_u1, expr, *addExpr);
    } else {
      MeExpr *subExpr = irmap->CreateMeExprBinary(OP_sub, ptyp, *var, *constExpr);
      static_cast<OpMeExpr*>(subExpr)->SetOpndType(ptyp);
      resExpr = irmap->CreateMeExprBinary(op, PTY_u1, expr, *subExpr);
    }
  }
  static_cast<OpMeExpr*>(resExpr)->SetOpndType(ptyp);
  return resExpr;
}
// give expr and its value range, use irmap to create a cmp expr
MeExpr *GetCmpExprFromVR(const ValueRange *vr, MeExpr &expr, MeIRMap *irmap) {
  if (vr == nullptr) {
    return nullptr;
  }
  RangeType rt = vr->GetRangeType();
  switch (rt) {
    case kLowerAndUpper: {
      Bound lower = vr->GetLower();
      Bound upper = vr->GetUpper();
      if (lower == Bound::MinBound(lower.GetPrimType())) {
        // expr <= upper
        return GetCmpExprFromBound(upper, expr, irmap, OP_le);
      } else if (upper == Bound::MaxBound(upper.GetPrimType())) {
        // lower <= expr
        return GetCmpExprFromBound(lower, expr, irmap, OP_ge);
      } else {
        // lower <= expr <= upper
        MeExpr *upperExpr = GetCmpExprFromBound(upper, expr, irmap, OP_le);
        MeExpr *lowerExpr = GetCmpExprFromBound(lower, expr, irmap, OP_ge);
        return irmap->CreateMeExprBinary(OP_land, PTY_u1, *upperExpr, *lowerExpr);
      }
    }
    case kOnlyHasLowerBound: {
      // expr >= val
      Bound lower = vr->GetLower();
      return GetCmpExprFromBound(lower, expr, irmap, OP_ge);
    }
    case kOnlyHasUpperBound: {
      // expr <= val
      Bound upper = vr->GetUpper();
      return GetCmpExprFromBound(upper, expr, irmap, OP_le);
    }
    case kNotEqual: {
      // expr != val
      Bound bound = vr->GetBound();
      return GetCmpExprFromBound(bound, expr, irmap, OP_ne);
    }
    case kEqual: {
      // expr == val
      Bound bound = vr->GetBound();
      return GetCmpExprFromBound(bound, expr, irmap, OP_eq);
    }
    case kRTEmpty: {
      // false
      return irmap->CreateIntConstMeExpr(0, PTY_u1);
    }
    case kRTComplete: {
      // true
      return irmap->CreateIntConstMeExpr(1, PTY_u1);
    }
    default:
      return nullptr;
  }
}

// DealWith this case :
// a: valueRange(0, constant) || valueRange(constant, 0)
// ==>
// a: valueRange(1, constant) || valueRange(constant, -1)
std::unique_ptr<ValueRange> ValueRangePropagation::ZeroIsInRange(const ValueRange &valueRange) {
  if (valueRange.GetRangeType() == kLowerAndUpper && valueRange.GetLower().GetVar() == nullptr &&
      valueRange.GetUpper().GetVar() == nullptr &&
      valueRange.GetLower().GetConstant() < valueRange.GetUpper().GetConstant()) {
    if (valueRange.GetLower().GetConstant() == 0) {
      return std::make_unique<ValueRange>(
          Bound(1, valueRange.GetLower().GetPrimType()), valueRange.GetUpper(), kLowerAndUpper);
    }
    if (valueRange.GetUpper().GetConstant() == 0) {
      return std::make_unique<ValueRange>(
          valueRange.GetLower(), Bound(-1, valueRange.GetUpper().GetPrimType()), kLowerAndUpper);
    }
  }
  return nullptr;
}

std::unique_ptr<ValueRange> ValueRangePropagation::FindValueRangeInCurrBBOrDominateBBs(
    const BB &bb, MeExpr &opnd) {
  auto *res = FindValueRangeInCurrentBB(bb.GetBBId(), opnd.GetExprID());
  if (res != nullptr) {
    return CopyValueRange(*res);
  }
  auto *valueRangeOfOpnd = FindValueRange(bb, opnd);
  if (valueRangeOfOpnd != nullptr) {
    return CopyValueRange(*valueRangeOfOpnd);
  }
  return nullptr;
}

std::unique_ptr<ValueRange> ValueRangePropagation::ComputeTheValueRangeOfIndex(
    const std::unique_ptr<ValueRange> &valueRangeOfIndex,
    std::unique_ptr<ValueRange> &valueRangOfOpnd, int64 constant) {
  return (valueRangeOfIndex == nullptr) ?
      std::move(valueRangOfOpnd) : AddOrSubWithValueRange(OP_add, *valueRangeOfIndex, constant);
}

void SafetyCheckWithBoundaryError::HandleAssignWithDeadBeef(
    const BB &bb, MeStmt &meStmt, MeExpr &indexOpnd,
    MeExpr &boundOpnd) {
  std::set<MeExpr*> visitedLHS;
  std::vector<MeStmt*> stmts{ &meStmt };
  vrp.JudgeTheConsistencyOfDefPointsOfBoundaryCheck(bb, indexOpnd, visitedLHS, stmts);
  visitedLHS.clear();
  stmts.clear();
  stmts.push_back(&meStmt);
  vrp.JudgeTheConsistencyOfDefPointsOfBoundaryCheck(bb, boundOpnd, visitedLHS, stmts);
}

// Deal with this case :
// valueRangeOfIndex zero is in range and in safe region mode, compiler err
// valueRangeOfIndex is zero and not in safe region mode, compiler err
void SafetyCheckWithNonnullError::HandleAssertNonnull(const MeStmt &meStmt, const ValueRange *valueRangeOfIndex) {
  if ((valueRangeOfIndex == nullptr || valueRangeOfIndex->IsZeroInRange()) &&
      meStmt.IsInSafeRegion() && MeOption::safeRegionMode) {
    (void)NeedDeleteTheAssertAfterErrorOrWarn(meStmt, true);
  }
  if (valueRangeOfIndex != nullptr && valueRangeOfIndex->IsEqualZero()) {
    (void)NeedDeleteTheAssertAfterErrorOrWarn(meStmt);
  }
}

bool SafetyCheckWithBoundaryError::HandleAssertError(const MeStmt &meStmt) {
  return NeedDeleteTheAssertAfterErrorOrWarn(meStmt);
}

bool SafetyCheckWithBoundaryError::HandleAssertltOrAssertle(
    const MeStmt &meStmt, Opcode op, int64 indexValue, int64 lengthValue) {
  if (kOpcodeInfo.IsAssertLeBoundary((op))) {
    if (indexValue > lengthValue) {
      return NeedDeleteTheAssertAfterErrorOrWarn(meStmt);
    }
  } else {
    if (indexValue >= lengthValue) {
      return NeedDeleteTheAssertAfterErrorOrWarn(meStmt);
    }
  }
  return false;
}

bool ValueRangePropagation::CompareConstantOfIndexAndLength(
    const MeStmt &meStmt, const ValueRange &valueRangeOfIndex, ValueRange &valueRangeOfLengthPtr, Opcode op) {
  if (safetyCheckBoundary->HandleAssertltOrAssertle(meStmt, op, valueRangeOfIndex.GetUpper().GetConstant(),
                                                    valueRangeOfLengthPtr.GetBound().GetConstant())) {
    return true;
  }
  return (kOpcodeInfo.IsAssertLeBoundary(op) ?
      valueRangeOfIndex.GetUpper().GetConstant() <= valueRangeOfLengthPtr.GetBound().GetConstant() :
      valueRangeOfIndex.GetUpper().GetConstant() < valueRangeOfLengthPtr.GetBound().GetConstant());
}

bool ValueRangePropagation::CompareIndexWithUpper(const BB &bb, const MeStmt &meStmt,
    const ValueRange &valueRangeOfIndex, ValueRange &valueRangeOfLengthPtr, Opcode op, const MeExpr *indexOpnd) {
  if (valueRangeOfIndex.GetRangeType() == kNotEqual || valueRangeOfLengthPtr.GetRangeType() == kNotEqual) {
    return false;
  }
  // Opt array boundary check when the array length is a constant.
  if (valueRangeOfLengthPtr.IsConstantLowerAndUpper()) {
    auto lowerOfLength = valueRangeOfLengthPtr.GetLower().GetConstant();
    auto upperOfLength = valueRangeOfLengthPtr.GetUpper().GetConstant();
    if (valueRangeOfIndex.GetRangeType() == kSpecialLowerForLoop ||
        (valueRangeOfIndex.GetRangeType() == kLowerAndUpper && valueRangeOfIndex.IsAccurate())) {
      if (safetyCheckBoundary->HandleAssertltOrAssertle(meStmt, op, valueRangeOfIndex.GetUpper().GetConstant(),
                                                        upperOfLength)) {
        return true;
      }
      return (kOpcodeInfo.IsAssertLeBoundary((op))) ? valueRangeOfIndex.GetUpper().GetConstant() <= lowerOfLength :
          valueRangeOfIndex.GetUpper().GetConstant() < lowerOfLength;
    }
    if (!valueRangeOfIndex.IsConstantLowerAndUpper() ||
        valueRangeOfIndex.GetLower().GetConstant() > valueRangeOfIndex.GetUpper().GetConstant() ||
        valueRangeOfLengthPtr.GetLower().GetConstant() > valueRangeOfLengthPtr.GetUpper().GetConstant()) {
      return false;
    }
    if (valueRangeOfLengthPtr.GetLower().GetConstant() < 0) {
      // The value of length is overflow, need deal with this case later.
    }
    auto *loop = loops->GetBBLoopParent(bb.GetBBId());
    // When some values of index which is loop var are out of bound, error during static compilation
    // only if the loop is canonical and the range of var is accurate.
    if (valueRangeOfIndex.IsAccurate() && indexOpnd != nullptr && loop != nullptr &&
        loop->IsCanonicalAndOnlyHasOneExitBBLoop() && IsLoopVariable(*loop, *indexOpnd)) {
      if (safetyCheckBoundary->HandleAssertltOrAssertle(meStmt, op, valueRangeOfIndex.GetUpper().GetConstant(),
                                                        upperOfLength)) {
        return true;
      }
    } else {
      if (safetyCheckBoundary->HandleAssertltOrAssertle(meStmt, op, valueRangeOfIndex.GetLower().GetConstant(),
                                                        upperOfLength)) {
        return true;
      }
    }
    return (kOpcodeInfo.IsAssertLeBoundary((op))) ? valueRangeOfIndex.GetUpper().GetConstant() <= lowerOfLength :
        valueRangeOfIndex.GetUpper().GetConstant() < lowerOfLength;
  } else if (valueRangeOfLengthPtr.GetRangeType() == kEqual) {
    // Opt array boundary check when the array length is a var.
    if (valueRangeOfIndex.GetRangeType() == kSpecialUpperForLoop ||
        valueRangeOfIndex.GetRangeType() == kLowerAndUpper) {
      // When the var of index is not equal to the var of length, need analysis if the var of index is related to the
      // var of length. For example:
      // Example: i = len - i;
      // Example: p[i];
      // Example: assertlt(p + i * 4, p + len * 4)
      if (valueRangeOfIndex.GetUpper().GetVar() != valueRangeOfLengthPtr.GetBound().GetVar()) {
        // deal with the case like i = len + x:
        auto *upperVar = valueRangeOfIndex.GetUpper().GetVar();
        if (upperVar == nullptr) {
          return false;
        }
        auto *crNode = sa.GetOrCreateCRNode(*upperVar);
        if (crNode == nullptr) {
          return false;
        }
        if (crNode->GetCRType() == kCRVarNode) {
          return (crNode->GetExpr() == valueRangeOfLengthPtr.GetBound().GetVar()) &&
              CompareConstantOfIndexAndLength(meStmt, valueRangeOfIndex, valueRangeOfLengthPtr, op);
        }
        if (crNode->GetCRType() != kCRAddNode ||
            static_cast<CRAddNode*>(crNode)->GetOpndsSize() != kNumOperands ||
            static_cast<CRAddNode*>(crNode)->GetOpnd(1)->GetExpr() == nullptr ||
            static_cast<CRAddNode*>(crNode)->GetOpnd(1)->GetExpr() != valueRangeOfLengthPtr.GetBound().GetVar() ||
            static_cast<CRAddNode*>(crNode)->GetOpnd(0)->GetCRType() != kCRConstNode) {
          return false;
        }
        // Update the valueRange of index after analysis the relationship of expr and length.
        // valueRangeOfIndex : (lower(nullptr, 1), upper: (expr, -1), kLowerAndUpper)
        // valueRangeOfLength: (Bound(length, 0), kEqual)
        // expr = length - 1
        // after analysis: valueRangeOfIndex :  (lower(length), upper: (-2), kLowerAndUpper)
        int64 res = 0;
        auto constantValue =
            static_cast<CRConstNode*>(static_cast<CRAddNode*>(crNode)->GetOpnd(0))->GetConstValue();
        if (!AddOrSubWithConstant(valueRangeOfLengthPtr.GetUpper().GetPrimType(), OP_add, constantValue,
                                  valueRangeOfIndex.GetUpper().GetConstant(), res)) {
          return false;
        }
        if (safetyCheckBoundary->HandleAssertltOrAssertle(meStmt, op, res,
                                                          valueRangeOfLengthPtr.GetBound().GetConstant())) {
          return true;
        }
        return (kOpcodeInfo.IsAssertLeBoundary(op) ? res <= valueRangeOfLengthPtr.GetBound().GetConstant() :
            res < valueRangeOfLengthPtr.GetBound().GetConstant());
      } else {
        return CompareConstantOfIndexAndLength(meStmt, valueRangeOfIndex, valueRangeOfLengthPtr, op);
      }
    }
    if ((valueRangeOfIndex.GetLower().GetVar() != valueRangeOfIndex.GetUpper().GetVar()) ||
        valueRangeOfIndex.GetUpper().GetVar() != valueRangeOfLengthPtr.GetBound().GetVar()) {
      return false;
    }
    if (safetyCheckBoundary->HandleAssertltOrAssertle(meStmt, op, valueRangeOfIndex.GetUpper().GetConstant(),
                                                      valueRangeOfLengthPtr.GetBound().GetConstant())) {
      return true;
    }
    return kOpcodeInfo.IsAssertLeBoundary(op) ?
        valueRangeOfIndex.GetUpper().GetConstant() <= valueRangeOfLengthPtr.GetBound().GetConstant() :
        valueRangeOfIndex.GetUpper().GetConstant() < valueRangeOfLengthPtr.GetBound().GetConstant();
  }
  return false;
}

// If the bb of the boundary check which is analysised befor dominates the bb of the boundary check which is
// analysised now, then eliminate the current boundary check.
bool ValueRangePropagation::IfAnalysisedBefore(const BB &bb, const MeStmt &stmt) {
  MeExpr &index = *stmt.GetOpnd(0);
  MeExpr &bound = *stmt.GetOpnd(1);
  auto &analysisedArrayChecks = (kOpcodeInfo.IsAssertLowerBoundary(stmt.GetOp())) ? analysisedLowerBoundChecks :
      (kOpcodeInfo.IsAssertLeBoundary(stmt.GetOp())) ? analysisedAssignBoundChecks : analysisedUpperBoundChecks;
  for (size_t i = 0; i < analysisedArrayChecks.size(); ++i) {
    auto *analysisedBB = func.GetCfg()->GetBBFromID(BBId(i));
    if (!dom.Dominate(*analysisedBB, bb)) {
      continue;
    }
    auto &analysisedChecks = analysisedArrayChecks[i];
    auto it = analysisedChecks.find(&bound);
    // Determine if they are the same array.
    if (it == analysisedChecks.end()) {
      return false;
    }
    // Determine if the index of the array check which is analysised before equals to the index of the array check
    // which is analysised now.
    if (it->second.find(&index) != it->second.end()) {
      return true;
    }
  }
  return false;
}

bool ValueRangePropagation::TheValueOfOpndIsInvaliedInABCO(
    const BB &bb, const MeStmt *meStmt, MeExpr &boundOpnd, bool updateCaches) {
  if (boundOpnd.GetMeOp() == kMeOpConst &&
      static_cast<const ConstMeExpr&>(boundOpnd).GetConstVal()->GetKind() == kConstInt &&
      static_cast<uint64>(static_cast<const ConstMeExpr&>(boundOpnd).GetIntValue()) == kInvaliedBound) {
    if (updateCaches) {
      Insert2AnalysisedArrayChecks(bb.GetBBId(), *meStmt->GetOpnd(1), *meStmt->GetOpnd(0), meStmt->GetOp());
    }
    return true;
  } else {
    auto *valueRange = FindValueRange(bb, boundOpnd);
    if (valueRange != nullptr && valueRange->GetRangeType() != kNotEqual &&
        valueRange->IsConstant() && static_cast<uint64>(valueRange->GetLower().GetConstant()) == kInvaliedBound) {
      if (updateCaches) {
        Insert2AnalysisedArrayChecks(bb.GetBBId(), *meStmt->GetOpnd(1), *meStmt->GetOpnd(0), meStmt->GetOp());
      }
      return true;
    }
  }
  return false;
}

// Pointer assigned from multibranch requires the bounds info for all branches.
// if:
//    p = GetBoundaryPtr
// else:
//    p = GetBoundarylessPtr
// error: p + i
void ValueRangePropagation::JudgeTheConsistencyOfDefPointsOfBoundaryCheck(
    const BB &bb, MeExpr &expr, std::set<MeExpr*> &visitedLHS, std::vector<MeStmt*> &stmts) {
  if (TheValueOfOpndIsInvaliedInABCO(bb, nullptr, expr, false)) {
    std::string errorLog = "error: pointer assigned from multibranch requires the boundary info for all branches.\n";
    for (size_t i = 0; i < stmts.size(); ++i) {
      if (i == 0) {
        errorLog += "Where the pointer is used at the statement:\n";
      }
      if (i == 1) {
        errorLog += "Pointer is assigned from pointer which has no boundary info. The data flow is:\n";
      }
      auto &srcPosition = stmts[i]->GetSrcPosition();
      errorLog += func.GetMIRModule().GetFileNameFromFileNum(srcPosition.FileNum()) + ":" +
          std::to_string(srcPosition.LineNum()) + "\n";
    }
    FATAL(kLncFatal, "%s", errorLog.c_str());
  }
  if (!expr.IsScalar()) {
    for (size_t i = 0; i < expr.GetNumOpnds(); ++i) {
      JudgeTheConsistencyOfDefPointsOfBoundaryCheck(bb, *expr.GetOpnd(i), visitedLHS, stmts);
    }
    return;
  }
  ScalarMeExpr &scalar = static_cast<ScalarMeExpr&>(expr);
  if (scalar.GetDefBy() == kDefByStmt) {
    stmts.push_back(scalar.GetDefStmt());
    if (visitedLHS.find(scalar.GetDefStmt()->GetRHS()) != visitedLHS.end()) {
      return;
    }
    JudgeTheConsistencyOfDefPointsOfBoundaryCheck(bb, *scalar.GetDefStmt()->GetRHS(), visitedLHS, stmts);
    stmts.pop_back();
    visitedLHS.insert(scalar.GetDefStmt()->GetRHS());
  } else if (scalar.GetDefBy() == kDefByPhi) {
    MePhiNode &phi = scalar.GetDefPhi();
    if (visitedLHS.find(phi.GetLHS()) != visitedLHS.end()) {
      return;
    }
    visitedLHS.insert(phi.GetLHS());
    for (auto &opnd : phi.GetOpnds()) {
      JudgeTheConsistencyOfDefPointsOfBoundaryCheck(bb, *opnd, visitedLHS, stmts);
    }
  }
}

bool ValueRangePropagation::IsLoopVariable(const LoopDesc &loop, const MeExpr &opnd) const {
  if (!opnd.IsScalar()) {
    return false;
  }
  bool opndIsLoopVar = false;
  auto defBy = static_cast<const ScalarMeExpr&>(opnd).GetDefBy();
  // Check whether it is a loop variable.
  if (defBy == kDefByStmt) {
    // If the def stmt is inc or dec stmt, the opnd is loop variable.
    auto *defStmt = static_cast<const ScalarMeExpr&>(opnd).GetDefByMeStmt();
    if (defStmt != nullptr && kOpcodeInfo.AssignActualVar(defStmt->GetOp())) {
      MeExpr *rhs = defStmt->GetRHS();
      if (rhs->GetOp() == OP_add || rhs->GetOp() == OP_sub) {
        OpMeExpr *opRHS = static_cast<OpMeExpr*>(rhs);
        if (defStmt->GetLHS()->GetOst() == static_cast<ScalarMeExpr*>(opRHS->GetOpnd(0))->GetOst()) {
          opndIsLoopVar = ThePhiLHSIsLoopVariable(loop, *opRHS->GetOpnd(0));
        }
      }
    }
  } else if (defBy == kDefByPhi) {
    opndIsLoopVar = ThePhiLHSIsLoopVariable(loop, opnd);
  }
  return opndIsLoopVar;
}

bool ValueRangePropagation::ThePhiLHSIsLoopVariable(const LoopDesc &loop, const MeExpr &opnd) const {
  if (!opnd.IsScalar()) {
    return false;
  }
  auto defBy = static_cast<const ScalarMeExpr&>(opnd).GetDefBy();
  if (defBy != kDefByPhi) {
    return false;
  }
  auto &defPhi = static_cast<const ScalarMeExpr&>(opnd).GetDefPhi();
  MeStmt *defStmt = nullptr;
  if (defPhi.GetOpnds().size() == kNumOperands) {
    auto *defBBOfOpnd0 = defPhi.GetOpnd(0)->GetDefByBBMeStmt(dom, defStmt);
    auto *defBBOfOpnd1 = defPhi.GetOpnd(1)->GetDefByBBMeStmt(dom, defStmt);
    // if the def bb of opnd0 is out of loop and the def bb of opnd1 is in loop, the opnd is loop variable.
    if (defPhi.GetDefBB() == loop.head && !loop.Has(*defBBOfOpnd0) && loop.Has(*defBBOfOpnd1)) {
      return true;
    }
  }
  return false;
}

static inline bool CanNotDoReplaceLoopVarOpt(const ValueRange *valueRange) {
  if (valueRange == nullptr) {
    return true;
  }
  auto type = valueRange->GetRangeType();
  return type != kEqual && !(type == kLowerAndUpper && valueRange->IsAccurate());
}

void ValueRangePropagation::CollectIndexOpndWithBoundInLoop(
    LoopDesc &loop, BB &bb, MeStmt &meStmt, MeExpr &opnd, std::map<MeExpr*, MeExpr*> &index2NewExpr) {
  // If the var is opnd of ivar, can not replace it with the bound.
  // For example: array[s[i]]
  // assert(array + iread(s + i) * 4, array + 1024)
  // Can not replace i with its upper bound.
  if (!opnd.IsScalar()) {
    index2NewExpr[&opnd] = nullptr;
    return;
  } else {
    if (!IsLoopVariable(loop, opnd)) {
      index2NewExpr[&opnd] = nullptr;
    } else {
      auto *valueRange = FindValueRange(bb, opnd);
      if (CanNotDoReplaceLoopVarOpt(valueRange)) {
        index2NewExpr[&opnd] = nullptr;
      } else {
        // If the assert stmt is lower boundary check, replace the index opnd with the lower bound, otherwise,
        // replace the index opnd with the upper bound.
        auto bound =
            kOpcodeInfo.IsAssertLowerBoundary(meStmt.GetOp()) ? valueRange->GetLower() : valueRange->GetUpper();
        MeExpr *newExpr = nullptr;
        if (bound.GetVar() == nullptr) {
          newExpr = irMap.CreateIntConstMeExpr(bound.GetConstant(), bound.GetPrimType());
        } else if (bound.GetConstant() == 0) {
          newExpr = bound.GetVar();
        } else {
          auto *newConstExpr = irMap.CreateIntConstMeExpr(bound.GetConstant(), bound.GetPrimType());
          newExpr = irMap.CreateMeExprBinary(OP_add, bound.GetPrimType(), *bound.GetVar(), *newConstExpr);
        }
        index2NewExpr[&opnd] = newExpr;
      }
    }
  }
  for (uint8 i = 0; i < opnd.GetNumOpnds(); ++i) {
    CollectIndexOpndWithBoundInLoop(loop, bb, meStmt, *(opnd.GetOpnd(i)), index2NewExpr);
  }
}

MeExpr *ValueRangePropagation::GetAddressOfIndexOrBound(MeExpr &expr) const {
  if (expr.IsLeaf()) {
    return (expr.IsScalar() && static_cast<ScalarMeExpr&>(expr).GetDefBy() == kDefByStmt) ?
        GetAddressOfIndexOrBound(*static_cast<ScalarMeExpr&>(expr).GetDefStmt()->GetRHS()) : &expr;
  }
  return GetAddressOfIndexOrBound(*expr.GetOpnd(0));
}

void ValueRangePropagation::GetValueRangeOfCRNode(
    const BB &bb, CRNode &opndOfCRNode, std::unique_ptr<ValueRange> &resValueRange, PrimType pTypeOfArray) {
  // Deal with the operand which is a constant.
  if (opndOfCRNode.GetCRType() == kCRConstNode) {
    int64 constant = static_cast<CRConstNode&>(opndOfCRNode).GetConstValue();
    resValueRange = (resValueRange == nullptr) ?
        std::make_unique<ValueRange>(Bound(constant, pTypeOfArray), kEqual) :
        AddOrSubWithValueRange(OP_add, *resValueRange, constant);
    return;
  }
  // Deal with the operand which is a meexpr.
  if (opndOfCRNode.GetExpr() == nullptr) {
    resValueRange = nullptr;
    return;
  }
  auto valueRangOfOpnd = FindValueRangeInCurrBBOrDominateBBs(bb, *opndOfCRNode.GetExpr());
  if (valueRangOfOpnd == nullptr) {
    valueRangOfOpnd = std::make_unique<ValueRange>(
        Bound(opndOfCRNode.GetExpr(), opndOfCRNode.GetExpr()->GetPrimType()), kEqual);
    if (resValueRange == nullptr) {
      resValueRange = std::move(valueRangOfOpnd);
    } else if (resValueRange->IsNotConstantVR()) {
      resValueRange = nullptr;
    } else {
      resValueRange = AddOrSubWithValueRange(OP_add, *resValueRange, *valueRangOfOpnd);
      if (resValueRange == nullptr) {
        resValueRange = nullptr;
      }
    }
  } else if (valueRangOfOpnd->GetRangeType() == kNotEqual && resValueRange == nullptr) {
    resValueRange = std::make_unique<ValueRange>(
        Bound(opndOfCRNode.GetExpr(), opndOfCRNode.GetExpr()->GetPrimType()), kEqual);
  } else if (valueRangOfOpnd->GetRangeType() == kOnlyHasUpperBound ||
             valueRangOfOpnd->GetRangeType() == kOnlyHasLowerBound) {
    resValueRange = nullptr;
  } else {
    resValueRange = (resValueRange == nullptr) ? std::move(valueRangOfOpnd) :
        AddOrSubWithValueRange(OP_add, *resValueRange, *valueRangOfOpnd);
  }
}

// Get the valueRange of index and bound, for example:
// assertge(ptr, ptr + 8) -> the valueRange of index is 8
// assertge(ptr, ptr + len * 4 - 8) -> the valueRange of index is len - 2
std::unique_ptr<ValueRange> ValueRangePropagation::GetValueRangeOfCRNodes(
    const BB &bb, PrimType pTypeOfArray, std::vector<CRNode*> &crNodes) {
  if (crNodes.empty()) {
    return CreateValueRangeOfEqualZero(pTypeOfArray);
  }
  std::unique_ptr<ValueRange> resValueRange = nullptr;
  for (size_t i = 0; i < crNodes.size(); ++i) {
    auto *opndOfCRNode = crNodes[i];
    if (opndOfCRNode->GetCRType() != kCRConstNode && opndOfCRNode->GetCRType() != kCRVarNode &&
        opndOfCRNode->GetCRType() != kCRAddNode) {
      return nullptr;
    }
    if (opndOfCRNode->GetCRType() != kCRAddNode) {
      GetValueRangeOfCRNode(bb, *opndOfCRNode, resValueRange, pTypeOfArray);
    } else {
      auto *addCRNOde = static_cast<CRAddNode*>(opndOfCRNode);
      for (size_t addNodeIndex = 0; addNodeIndex < addCRNOde->GetOpndsSize(); ++addNodeIndex) {
        GetValueRangeOfCRNode(bb, *addCRNOde->GetOpnd(addNodeIndex), resValueRange, pTypeOfArray);
        if (resValueRange == nullptr) {
          return nullptr;
        }
      }
    }
    if (resValueRange == nullptr) {
      return nullptr;
    }
  }
  return resValueRange;
}

bool ValueRangePropagation::DealWithBoundaryCheck(BB &bb, MeStmt &meStmt) {
  // If the stmt is analysised before, delete the boundary check.
  if (IfAnalysisedBefore(bb, meStmt)) {
    return true;
  }
  // Add the stmt to cache to prevent duplicate checks.
  Insert2AnalysisedArrayChecks(bb.GetBBId(), *meStmt.GetOpnd(1), *meStmt.GetOpnd(0), meStmt.GetOp());
  CHECK_FATAL(meStmt.NumMeStmtOpnds() == kNumOperands, "must have two opnds");
  auto &naryMeStmt = static_cast<NaryMeStmt&>(meStmt);
  auto *boundOpnd = naryMeStmt.GetOpnd(1);
  auto *indexOpnd = naryMeStmt.GetOpnd(0);
  safetyCheckBoundary->HandleAssignWithDeadBeef(bb, meStmt, *indexOpnd, *boundOpnd);
  CRNode *indexCR = sa.GetOrCreateCRNode(*indexOpnd);
  CRNode *boundCR = sa.GetOrCreateCRNode(*boundOpnd);
  if (indexCR == nullptr || boundCR == nullptr) {
    return false;
  }
  MeExpr *addrExpr = GetAddressOfIndexOrBound(*indexOpnd);
  if (addrExpr != GetAddressOfIndexOrBound(*boundOpnd)) {
    if (ValueRangePropagation::isDebug) {
      meStmt.Dump(&irMap);
      LogInfo::MapleLogger() << "The address of bases is not equal\n";
    }
    return false;
  }
  if (ValueRangePropagation::isDebug) {
    meStmt.Dump(&irMap);
    sa.Dump(*indexCR);
    LogInfo::MapleLogger() << "\n";
    sa.Dump(*boundCR);
    LogInfo::MapleLogger() << "\n";
  }
  bool indexIsEqualToBound = indexCR->IsEqual(*boundCR);
  if (indexIsEqualToBound) {
    if (kOpcodeInfo.IsAssertLowerBoundary(meStmt.GetOp()) || kOpcodeInfo.IsAssertLeBoundary(meStmt.GetOp())) {
      return true;
    }
    if (safetyCheckBoundary->HandleAssertError(meStmt)) {
      return true;
    }
  }
  std::vector<CRNode*> indexVector;
  std::vector<CRNode*> boundVector;
  indexCR->GetTheUnequalSubNodesOfIndexAndBound(*boundCR, indexVector, boundVector);
  PrimType primType = sa.GetPrimType(indexVector);
  auto byteSizeFromIndexVector = sa.GetByteSize(indexVector);
  auto byteSizeFromBoundVector = sa.GetByteSize(boundVector);
  uint8 byteSize = (byteSizeFromIndexVector > 1) ? byteSizeFromIndexVector : byteSizeFromBoundVector;
  // If can not normalization the vector, return false.
  if ((!sa.NormalizationWithByteCount(indexVector, byteSize) ||
       !sa.NormalizationWithByteCount(boundVector, byteSize))) {
    return false;
  }
  if (ValueRangePropagation::isDebug) {
    std::for_each(indexVector.begin(), indexVector.end(), [this](const CRNode *cr) {
        sa.Dump(*cr); std::cout << " ";});
    LogInfo::MapleLogger() << "\n";
    std::for_each(boundVector.begin(), boundVector.end(), [this](const CRNode *cr) {
        sa.Dump(*cr); std::cout << " ";});
    LogInfo::MapleLogger() << "\n";
  }
  // Get the valueRange of index and bound
  auto valueRangeOfIndex = GetValueRangeOfCRNodes(bb, primType, indexVector);
  auto valueRangeOfbound = GetValueRangeOfCRNodes(bb, primType, boundVector);
  if (valueRangeOfIndex == nullptr || valueRangeOfbound == nullptr) {
    return false;
  }
  if (kOpcodeInfo.IsAssertLowerBoundary(meStmt.GetOp())) {
    if (BrStmtInRange(bb, *valueRangeOfIndex, *valueRangeOfbound, OP_ge,
                      valueRangeOfIndex->GetLower().GetPrimType())) {
      // Can opt the boundary check.
      return true;
    } else if (BrStmtInRange(bb, *valueRangeOfIndex, *valueRangeOfbound, OP_lt,
                             valueRangeOfIndex->GetLower().GetPrimType())) {
      // Error during static compilation.
      if (safetyCheckBoundary->HandleAssertError(meStmt)) {
        return true;
      }
    } else if ((valueRangeOfIndex->GetRangeType() == kSpecialUpperForLoop ||
               valueRangeOfIndex->GetRangeType() == kOnlyHasLowerBound ||
               (valueRangeOfIndex->GetRangeType() == kLowerAndUpper && valueRangeOfIndex->IsAccurate())) &&
               valueRangeOfIndex->GetLower().GetVar() == nullptr && valueRangeOfbound->GetLower().GetVar() == nullptr &&
               valueRangeOfbound->GetUpper().GetVar() == nullptr) {
      if (valueRangeOfIndex->GetLower().GetConstant() < valueRangeOfbound->GetUpper().GetConstant()) {
        // Error during static compilation.
        if (safetyCheckBoundary->HandleAssertError(meStmt)) {
          return true;
        }
        return false;
      } else {
        // Can opt the boundary check.
        return true;
      }
    } else if (valueRangeOfIndex->GetRangeType() == kLowerAndUpper && valueRangeOfIndex->IsAccurate() &&
        valueRangeOfIndex->IsConstantLowerAndUpper() && valueRangeOfbound->IsEqualZero() &&
        valueRangeOfIndex->GetLower().GetConstant() < 0) {
      // Error during static compilation.
      if (safetyCheckBoundary->HandleAssertError(meStmt)) {
        return true;
      }
    }
  } else {
    auto *currIndexOpnd = (indexVector.size() == 1) ? indexVector[0]->GetExpr() : nullptr;
    if (CompareIndexWithUpper(bb, meStmt, *valueRangeOfIndex, *valueRangeOfbound, meStmt.GetOp(), currIndexOpnd)) {
      return true;
    }
  }
  auto *loop = loops->GetBBLoopParent(bb.GetBBId());
  if (loop != nullptr && loop->IsCanonicalLoop() && loop->inloopBB2exitBBs.size() == 1 &&
      loop->inloopBB2exitBBs.begin()->second->size() == 1) {
    std::map<MeExpr*, MeExpr*> index2NewExpr;
    for (uint8 i = 0; i < indexOpnd->GetNumOpnds(); ++i) {
      auto *opnd = indexOpnd->GetOpnd(i);
      if (index2NewExpr.find(opnd) != index2NewExpr.end()) {
        continue;
      }
      CollectIndexOpndWithBoundInLoop(*loop, bb, meStmt, *opnd, index2NewExpr);
    }
    for (auto it = index2NewExpr.begin(); it != index2NewExpr.end(); ++it) {
      if (it->second == nullptr) {
        continue;
      }
      (void)irMap.ReplaceMeExprStmt(meStmt, *it->first, *it->second);
    }
  }
  return false;
}

// If error in Compile return false, else return true.
bool SafetyCheck::NeedDeleteTheAssertAfterErrorOrWarn(const MeStmt &stmt, bool isNullablePtr) const {
  auto srcPosition = stmt.GetSrcPosition();
  const char *fileName = func->GetMIRModule().GetFileNameFromFileNum(srcPosition.FileNum()).c_str();
  switch (stmt.GetOp()) {
    case OP_calcassertlt:
    case OP_calcassertge: {
      auto &newStmt = static_cast<const AssertBoundaryMeStmt &>(stmt);
      GStrIdx curFuncNameIdx = func->GetMirFunc()->GetNameStrIdx();
      GStrIdx stmtFuncNameIdx = GlobalTables::GetStrTable().GetStrIdxFromName(newStmt.GetFuncName().c_str());
      if (curFuncNameIdx == stmtFuncNameIdx) {
        if (stmt.GetOp() == OP_calcassertlt) {
          WARN(kLncWarn, "%s:%d warning: the pointer >= the upper bounds after calculation",
               fileName, srcPosition.LineNum());
        } else if (stmt.GetOp() == OP_calcassertge) {
          WARN(kLncWarn, "%s:%d warning: the pointer < the lower bounds after calculation",
               fileName, srcPosition.LineNum());
        }
      } else {
        if (stmt.GetOp() == OP_calcassertlt) {
          WARN(kLncWarn, "%s:%d warning: the pointer >= the upper bounds after calculation when inlined to %s",
               fileName, srcPosition.LineNum(), func->GetName().c_str());
        } else if (stmt.GetOp() == OP_calcassertge) {
          WARN(kLncWarn, "%s:%d warning: the pointer < the lower bounds after calculation when inlined to %s",
               fileName, srcPosition.LineNum(), func->GetName().c_str());
        }
      }
      return true;
    }
    case OP_assignassertnonnull:
    case OP_assertnonnull:
    case OP_returnassertnonnull: {
      auto &newStmt = static_cast<const AssertNonnullMeStmt &>(stmt);
      GStrIdx curFuncNameIdx = func->GetMirFunc()->GetNameStrIdx();
      GStrIdx stmtFuncNameIdx = GlobalTables::GetStrTable().GetStrIdxFromName(newStmt.GetFuncName().c_str());
      if (curFuncNameIdx == stmtFuncNameIdx) {
        if (isNullablePtr) {
          if (stmt.GetOp() == OP_assertnonnull) {
            FATAL(kLncFatal, "%s:%d error: Dereference of nullable pointer in safe region",
                  fileName, srcPosition.LineNum());
          } else if (stmt.GetOp() == OP_returnassertnonnull) {
            FATAL(kLncFatal, "%s:%d error: %s return nonnull but got nullable pointer in safe region",
                  fileName, srcPosition.LineNum(), newStmt.GetFuncName().c_str());
          } else {
            FATAL(kLncFatal, "%s:%d error: nullable pointer assignment of nonnull pointer in safe region",
                  fileName, srcPosition.LineNum());
          }
        } else {
          if (stmt.GetOp() == OP_assertnonnull) {
            FATAL(kLncFatal, "%s:%d error: Dereference of null pointer", fileName, srcPosition.LineNum());
          } else if (stmt.GetOp() == OP_returnassertnonnull) {
            FATAL(kLncFatal, "%s:%d error: %s return nonnull but got null pointer", fileName, srcPosition.LineNum(),
                  newStmt.GetFuncName().c_str());
          } else {
            FATAL(kLncFatal, "%s:%d error: null assignment of nonnull pointer", fileName, srcPosition.LineNum());
          }
        }
      } else {
        if (isNullablePtr) {
          if (stmt.GetOp() == OP_assertnonnull) {
            FATAL(kLncFatal, "%s:%d error: Dereference of nullable pointer in safe region when inlined to %s",
                  fileName, srcPosition.LineNum(), func->GetName().c_str());
          } else if (stmt.GetOp() == OP_returnassertnonnull) {
            FATAL(kLncFatal,
                  "%s:%d error: %s return nonnull but got nullable pointer in safe region when inlined to %s",
                  fileName, srcPosition.LineNum(), newStmt.GetFuncName().c_str(), func->GetName().c_str());
          } else {
            FATAL(kLncFatal,
                  "%s:%d error: nullable pointer assignment of nonnull pointer in safe region when inlined to %s",
                  fileName, srcPosition.LineNum(), func->GetName().c_str());
          }
        } else {
          if (stmt.GetOp() == OP_assertnonnull) {
            FATAL(kLncFatal, "%s:%d error: Dereference of null pointer when inlined to %s",
                  fileName, srcPosition.LineNum(), func->GetName().c_str());
          } else if (stmt.GetOp() == OP_returnassertnonnull) {
            FATAL(kLncFatal, "%s:%d error: %s return nonnull but got null pointer when inlined to %s",
                  fileName, srcPosition.LineNum(), newStmt.GetFuncName().c_str(), func->GetName().c_str());
          } else {
            FATAL(kLncFatal, "%s:%d error: null assignment of nonnull pointer when inlined to %s",
                  fileName, srcPosition.LineNum(), func->GetName().c_str());
          }
        }
      }
      break;
    }
    case OP_callassertnonnull: {
      auto &callStmt = static_cast<const CallAssertNonnullMeStmt &>(stmt);
      GStrIdx curFuncNameIdx = func->GetMirFunc()->GetNameStrIdx();
      GStrIdx stmtFuncNameIdx = GlobalTables::GetStrTable().GetStrIdxFromName(callStmt.GetStmtFuncName().c_str());
      if (curFuncNameIdx == stmtFuncNameIdx) {
        if (isNullablePtr) {
          FATAL(kLncFatal, "%s:%d error: nullable pointer passed to %s that requires a nonnull pointer "\
                "for %s argument in safe region", fileName, srcPosition.LineNum(), callStmt.GetFuncName().c_str(),
                GetNthStr(callStmt.GetParamIndex()).c_str());
        } else {
          FATAL(kLncFatal, "%s:%d error: NULL passed to %s that requires a nonnull pointer for %s argument", fileName,
                srcPosition.LineNum(), callStmt.GetFuncName().c_str(), GetNthStr(callStmt.GetParamIndex()).c_str());
        }
      } else {
        if (isNullablePtr) {
          FATAL(kLncFatal, "%s:%d error: nullable pointer passed to %s that requires a nonnull pointer "\
                "for %s argument in safe region when inlined to %s", fileName, srcPosition.LineNum(),
                callStmt.GetFuncName().c_str(), GetNthStr(callStmt.GetParamIndex()).c_str(), func->GetName().c_str());
        } else {
          FATAL(kLncFatal, "%s:%d error: NULL passed to %s that requires a nonnull pointer for %s argument "\
                "when inlined to %s", fileName, srcPosition.LineNum(), callStmt.GetFuncName().c_str(),
                GetNthStr(callStmt.GetParamIndex()).c_str(), func->GetName().c_str());
        }
      }
      break;
    }
    case OP_returnassertle:
    case OP_assignassertle:
    case OP_assertlt:
    case OP_assertge: {
      auto &newStmt = static_cast<const AssertBoundaryMeStmt &>(stmt);
      GStrIdx curFuncNameIdx = func->GetMirFunc()->GetNameStrIdx();
      GStrIdx stmtFuncNameIdx = GlobalTables::GetStrTable().GetStrIdxFromName(newStmt.GetFuncName().c_str());
      if (curFuncNameIdx == stmtFuncNameIdx) {
        if (stmt.GetOp() == OP_assertlt) {
          FATAL(kLncFatal, "%s:%d error: the pointer >= the upper bounds when accessing the memory",
                fileName, srcPosition.LineNum());
        } else if (stmt.GetOp() == OP_assertge) {
          FATAL(kLncFatal, "%s:%d error: the pointer < the lower bounds when accessing the memory",
                fileName, srcPosition.LineNum());
        } else if (stmt.GetOp() == OP_assignassertle) {
          FATAL(kLncFatal, "%s:%d error: l-value boundary should not be larger than r-value boundary", fileName,
                srcPosition.LineNum());
        } else {
          FATAL(kLncFatal, "%s:%d error: return value's bounds does not match the function declaration for %s",
                fileName, srcPosition.LineNum(), newStmt.GetFuncName().c_str());
        }
      } else {
        if (stmt.GetOp() == OP_assertlt) {
          FATAL(kLncFatal, "%s:%d error: the pointer >= the upper bounds when accessing the memory and inlined to %s",
                fileName, srcPosition.LineNum(), func->GetName().c_str());
        } else if (stmt.GetOp() == OP_assertge) {
          FATAL(kLncFatal, "%s:%d error: the pointer < the lower bounds when accessing the memory and inlined to %s",
                fileName, srcPosition.LineNum(), func->GetName().c_str());
        } else if (stmt.GetOp() == OP_assignassertle) {
          FATAL(kLncFatal,
                "%s:%d error: l-value boundary should not be larger than r-value boundary when inlined to %s",
                fileName, srcPosition.LineNum(), func->GetName().c_str());
        } else {
          FATAL(kLncFatal,
                "%s:%d error: return value's bounds does not match the function declaration for %s when inlined to %s",
                fileName, srcPosition.LineNum(), newStmt.GetFuncName().c_str(), func->GetName().c_str());
        }
      }
      break;
    }
    case OP_callassertle: {
      auto &callStmt = static_cast<const CallAssertBoundaryMeStmt&>(stmt);
      GStrIdx curFuncNameIdx = func->GetMirFunc()->GetNameStrIdx();
      GStrIdx stmtFuncNameIdx = GlobalTables::GetStrTable().GetStrIdxFromName(callStmt.GetStmtFuncName().c_str());
      if (curFuncNameIdx == stmtFuncNameIdx) {
        FATAL(kLncFatal,
              "%s:%d error: the pointer's bounds does not match the function %s declaration for the %s argument",
              func->GetMIRModule().GetFileNameFromFileNum(srcPosition.FileNum()).c_str(), srcPosition.LineNum(),
              callStmt.GetFuncName().c_str(), GetNthStr(callStmt.GetParamIndex()).c_str());
      } else {
        FATAL(kLncFatal, "%s:%d error: the pointer's bounds does not match the function %s declaration "\
              "for the %s argument when inlined to %s", fileName, srcPosition.LineNum(),
              callStmt.GetFuncName().c_str(), GetNthStr(callStmt.GetParamIndex()).c_str(), func->GetName().c_str());
      }
      break;
    }
    default:
      CHECK_FATAL(false, "can not be here");
  }
  return false;
}

bool ValueRangePropagation::DealWithAssertNonnull(BB &bb, const MeStmt &meStmt) {
  auto *opnd = static_cast<const UnaryMeStmt&>(meStmt).GetOpnd();
  auto *valueRange = FindValueRange(bb, *opnd);
  if (valueRange == nullptr) {
    // If the cfg is changed and dom is not update, need find the valuerange in pred bbs
    // when pred bbs are goto or fallthru. The cfg changed like this :
    // pred0   pred1             pred0  pred1
    //     \   /                   |      |
    //     condgoto     ---->      |      |
    //     /  \                    |      |
    //    bb   succ                bb     succ
    if (bb.GetPred().size() == 1) {
      auto *pred = bb.GetPred(0);
      while (IsGotoOrFallthruBB(*pred) && pred->GetPred().size() == 1 && IsGotoOrFallthruBB(*pred->GetPred(0))) {
        pred = pred->GetPred(0);
      }
      auto *predValueRange = FindValueRange(*pred, *opnd);
      if (predValueRange != nullptr && predValueRange->IsNotEqualZero()) {
        return true;
      }
    }
    safetyCheckNonnull->HandleAssertNonnull(meStmt, valueRange);
    (void)Insert2Caches(bb.GetBBId(), opnd->GetExprID(),
                        std::make_unique<ValueRange>(Bound(nullptr, 0, opnd->GetPrimType()), kNotEqual));
    return false;
  }
  safetyCheckNonnull->HandleAssertNonnull(meStmt, valueRange);
  auto newValueRange = ZeroIsInRange(*valueRange);
  if (newValueRange != nullptr) {
    (void)Insert2Caches(bb.GetBBId(), opnd->GetExprID(), std::move(newValueRange));
  }
  return valueRange->IsNotEqualZero() ? true : false;
}

// When the valuerange of expr is constant, replace it with constMeExpr.
void ValueRangePropagation::ReplaceOpndWithConstMeExpr(const BB &bb, MeStmt &stmt) {
  std::map<MeExpr*, std::pair<int64, PrimType>> expr2ConstantValue;
  for (size_t i = 0; i < stmt.NumMeStmtOpnds(); ++i) {
    CollectMeExpr(bb, stmt, *stmt.GetOpnd(i), expr2ConstantValue);
  }
  if (expr2ConstantValue.empty()) {
    return;
  }
  for (auto it = expr2ConstantValue.begin(); it != expr2ConstantValue.end(); ++it) {
    auto *newConstExpr = irMap.CreateIntConstMeExpr(it->second.first, it->second.second);
    (void)irMap.ReplaceMeExprStmt(stmt, *it->first, *newConstExpr);
  }
  irMap.UpdateIncDecAttr(stmt);
}

// When the valuerange of expr is constant, collect it and do replace later.
void ValueRangePropagation::CollectMeExpr(
    const BB &bb, MeStmt &stmt, MeExpr &meExpr, std::map<MeExpr*, std::pair<int64, PrimType>> &expr2ConstantValue) {
  if (meExpr.GetMeOp() == kMeOpConst) {
    return;
  }
  auto *valueRangeOfOperand = FindValueRange(bb, meExpr);
  if (valueRangeOfOperand != nullptr && valueRangeOfOperand->IsConstant() &&
      valueRangeOfOperand->GetRangeType() != kNotEqual && stmt.GetOp() != OP_asm) {
    auto rangeType = valueRangeOfOperand->GetRangeType();
    int64 value = (rangeType == kEqual) ? valueRangeOfOperand->GetBound().GetConstant() :
         valueRangeOfOperand->GetLower().GetConstant();
    PrimType pType = (rangeType == kEqual) ? valueRangeOfOperand->GetBound().GetPrimType() :
        valueRangeOfOperand->GetLower().GetPrimType();
    expr2ConstantValue[&meExpr] = std::make_pair(value, pType);
    return;
  }
  for (uint8 i = 0; i < meExpr.GetNumOpnds(); ++i) {
    CollectMeExpr(bb, stmt, *(meExpr.GetOpnd(i)), expr2ConstantValue);
  }
}

void ValueRangePropagation::DealWithOperand(const BB &bb, MeStmt &stmt, MeExpr &meExpr) {
  switch (meExpr.GetMeOp()) {
    case kMeOpConst: {
      if (static_cast<ConstMeExpr&>(meExpr).GetConstVal()->GetKind() == kConstInt) {
        (void)Insert2Caches(bb.GetBBId(), meExpr.GetExprID(), std::make_unique<ValueRange>(
            Bound(nullptr, static_cast<ConstMeExpr&>(meExpr).GetIntValue(), meExpr.GetPrimType()), kEqual));
      }
      break;
    }
    case kMeOpIvar: {
      auto &ivarMeExpr = static_cast<IvarMeExpr&>(meExpr);
      // prop value range of base
      auto *base = ivarMeExpr.GetBase();
      auto *valueRange = FindValueRange(bb, *base);
      if (valueRange == nullptr) {
        (void)Insert2Caches(bb.GetBBId(), base->GetExprID(), CreateValueRangeOfNotEqualZero(base->GetPrimType()));
      } else {
        auto newValueRange = ZeroIsInRange(*valueRange);
        if (newValueRange != nullptr) {
          (void)Insert2Caches(bb.GetBBId(), base->GetExprID(), std::move(newValueRange));
        }
      }
      // prop value range of field
      auto filedID = static_cast<IvarMeExpr&>(meExpr).GetFieldID();
      auto *ptrType = static_cast<MIRPtrType*>(GlobalTables::GetTypeTable().GetTypeFromTyIdx(ivarMeExpr.GetTyIdx()));
      if (ptrType->GetPointedType()->IsStructType() &&
          static_cast<MIRStructType*>(ptrType->GetPointedType())->GetFieldAttrs(filedID).GetAttr(FLDATTR_nonnull)) {
        (void)Insert2Caches(bb.GetBBId(), meExpr.GetExprID(), CreateValueRangeOfNotEqualZero(meExpr.GetPrimType()));
      }
      break;
    }
    case kMeOpVar: {
      auto &varMeExpr = static_cast<VarMeExpr&>(meExpr);
      // get nonnull attr from symbol or fieldPair
      if (varMeExpr.GetOst()->GetMIRSymbol()->GetAttr(ATTR_nonnull)) {
        (void)Insert2Caches(bb.GetBBId(), meExpr.GetExprID(), CreateValueRangeOfNotEqualZero(meExpr.GetPrimType()));
      } else if (varMeExpr.GetFieldID() != 0) {
        auto filedID = varMeExpr.GetFieldID();
        auto *ty = varMeExpr.GetOst()->GetMIRSymbol()->GetType();
        if (ty->IsStructType() && static_cast<MIRStructType*>(ty)->GetFieldAttrs(filedID).GetAttr(FLDATTR_nonnull)) {
          (void)Insert2Caches(bb.GetBBId(), meExpr.GetExprID(), CreateValueRangeOfNotEqualZero(meExpr.GetPrimType()));
        }
      }
      break;
    }
    case kMeOpOp: {
      auto &opMeExpr = static_cast<OpMeExpr&>(meExpr);
      switch (opMeExpr.GetOp()) {
        case OP_select: {
          return;
        }
        case OP_cvt: {
          DealWithCVT(bb, static_cast<OpMeExpr&>(meExpr));
          break;
        }
        case OP_zext: {
          auto *opnd = opMeExpr.GetOpnd(0);
          if (opMeExpr.GetBitsSize() >= GetPrimTypeBitSize(opnd->GetPrimType())) {
            auto *valueRange = FindValueRange(bb, *opnd);
            if (valueRange != nullptr) {
              (void)Insert2Caches(bb.GetBBId(), opMeExpr.GetExprID(), CopyValueRange(*valueRange));
            }
          }
          break;
        }
        case OP_iaddrof: {
          auto *mirType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(opMeExpr.GetTyIdx());
          CHECK_FATAL(mirType->IsMIRPtrType(), "must be pointer type");
          auto *pointerType = static_cast<MIRPtrType*>(mirType);
          auto *opnd = opMeExpr.GetOpnd(0);
          auto *valueRange = FindValueRange(bb, *opnd);
          if (pointerType->GetPointedType()->GetBitOffsetFromBaseAddr(opMeExpr.GetFieldID()) != 0 ||
              (valueRange != nullptr && valueRange->IsNotEqualZero())) {
            (void)Insert2Caches(bb.GetBBId(), opMeExpr.GetExprID(),
                                CreateValueRangeOfNotEqualZero(meExpr.GetPrimType()));
          }
          break;
        }
        case OP_neg: {
          DealWithNeg(bb, opMeExpr);
          break;
        }
        case OP_add:
        case OP_sub: {
          (void)DealWithAddOrSub(bb, opMeExpr, opMeExpr);
          break;
        }
        default:
          break;
      }
      break;
    }
    case kMeOpAddrof:
    case kMeOpAddroffunc:
    case kMeOpAddroflabel: {
      auto *valueRangeOfAddrof = FindValueRange(bb, meExpr);
      if (valueRangeOfAddrof == nullptr) {
        (void)Insert2Caches(bb.GetBBId(), meExpr.GetExprID(), CreateValueRangeOfNotEqualZero(meExpr.GetPrimType()));
      }
      break;
    }
    default:
      break;
  }
  for (uint8 i = 0; i < meExpr.GetNumOpnds(); ++i) {
    DealWithOperand(bb, stmt, *(meExpr.GetOpnd(i)));
  }
}

void ValueRangePropagation::DealWithNeg(const BB &bb, const OpMeExpr &opMeExpr) {
  CHECK_FATAL(opMeExpr.GetNumOpnds() == 1, "must have one opnd");
  auto *opnd = opMeExpr.GetOpnd(0);
  auto res = NegValueRange(bb, *opnd);
  (void)Insert2Caches(bb.GetBBId(), opMeExpr.GetExprID(), std::move(res));
}

// If the value of opnd of cvt would not overflow or underflow after convert, delete the cvt.
void ValueRangePropagation::DealWithCVT(const BB &bb, OpMeExpr &opMeExpr) {
  CHECK_FATAL(opMeExpr.GetNumOpnds() == 1, "must have one opnd");
  auto toType = opMeExpr.GetPrimType();
  auto fromType = opMeExpr.GetOpndType();
  if ((fromType == PTY_u1 || fromType == PTY_u8 || fromType == PTY_u16 || fromType == PTY_u32 || fromType == PTY_a32 ||
      ((fromType == PTY_ref || fromType == PTY_ptr) && GetPrimTypeSize(fromType) == 4)) &&
      GetPrimTypeBitSize(toType) > GetPrimTypeBitSize(fromType)) {
    auto *valueRange = FindValueRange(bb, opMeExpr);
    if (valueRange != nullptr) {
      return;
    }
    (void)Insert2Caches(bb.GetBBId(), opMeExpr.GetExprID(), std::make_unique<ValueRange>(
        Bound(nullptr, 0, fromType), Bound(GetMaxNumber(fromType), fromType), kLowerAndUpper));
  }
}

// When unreachable bb has trystmt or endtry attribute, need update try and endtry bbs.
void ValueRangePropagation::UpdateTryAttribute(BB &bb) {
  // update try end bb
  if (bb.GetAttributes(kBBAttrIsTryEnd) &&
      (bb.GetMeStmts().empty() || (bb.GetMeStmts().front().GetOp() != OP_try))) {
    auto *startTryBB = func.GetCfg()->GetTryBBFromEndTryBB(&bb);
    auto *newEndTry = func.GetCfg()->GetBBFromID(bb.GetBBId() - 1);
    while (newEndTry == nullptr) {
      newEndTry = func.GetCfg()->GetBBFromID(newEndTry->GetBBId() - 1);
    }
    CHECK_NULL_FATAL(newEndTry);
    CHECK_FATAL(!newEndTry->GetAttributes(kBBAttrIsTryEnd), "must not be try end");
    if (newEndTry->GetAttributes(kBBAttrIsTry) && !newEndTry->GetAttributes(kBBAttrIsTryEnd)) {
      newEndTry->SetAttributes(kBBAttrIsTryEnd);
      func.GetCfg()->SetBBTryBBMap(newEndTry, startTryBB);
    }
  }
  // update start try bb
  if (!bb.GetMeStmts().empty() && bb.GetMeStmts().front().GetOp() == OP_try &&
      !bb.GetAttributes(kBBAttrIsTryEnd)) {
    for (auto &pair : func.GetCfg()->GetEndTryBB2TryBB()) {
      if (pair.second == &bb) {
        auto *newStartTry = func.GetCfg()->GetBBFromID(bb.GetBBId() + 1);
        CHECK_NULL_FATAL(newStartTry);
        CHECK_FATAL(newStartTry->GetAttributes(kBBAttrIsTry), "must be try");
        newStartTry->AddMeStmtFirst(&bb.GetMeStmts().front());
        func.GetCfg()->SetBBTryBBMap(pair.first, newStartTry);
        break;
      }
    }
  }
}

// Insert the ost of phi opnds to their def bbs.
void ValueRangePropagation::InsertOstOfPhi2Cands(
    BB &bb, size_t i, ScalarMeExpr *updateSSAExceptTheScalarExpr,
    std::map<OStIdx, std::set<BB*>> &ssaupdateCandsForCondExpr, bool setPhiIsDead) {
  for (auto &it : bb.GetMePhiList()) {
    if (setPhiIsDead) {
      it.second->SetIsLive(false);
    }
    auto *opnd = it.second->GetOpnd(i);
    MeStmt *stmt = nullptr;
    auto *defBB = opnd->GetDefByBBMeStmt(dom, stmt);
    // At the time of optimizing redundant branches, when the all preds of condgoto BB can jump directly to
    // the successors of condgoto BB and the conditional expr is only used for conditional stmt, after opt,
    // the ssa of expr not need to be updated, otherwise the ssa of epr still needs to be updated.
    if (updateSSAExceptTheScalarExpr != nullptr && it.first == updateSSAExceptTheScalarExpr->GetOstIdx()) {
      // when do opt
      ssaupdateCandsForCondExpr[it.first].insert(defBB);
    } else {
      MeSSAUpdate::InsertOstToSSACands(it.first, *defBB, &cands);
      needUpdateSSA = true;
    }
  }
}

// Deal with the cfg changed like this:
//  pred                  pred
//    |                     |
//    bb       -->          |   bb
//   /  \                   |  /  \
// true false              true   false
void ValueRangePropagation::PrepareForSSAUpdateWhenPredBBIsRemoved(
    const BB &pred, BB &bb, ScalarMeExpr *updateSSAExceptTheScalarExpr,
    std::map<OStIdx, std::set<BB*>> &ssaupdateCandsForCondExpr) {
  int index = bb.GetPredIndex(pred);
  CHECK_FATAL(index != -1, "pred is not in preds of bb");
  InsertOstOfPhi2Cands(bb, static_cast<size_t>(index), updateSSAExceptTheScalarExpr, ssaupdateCandsForCondExpr);
  auto updateSSAExceptTheOstIdx = updateSSAExceptTheScalarExpr == nullptr ? OStIdx(0) :
      updateSSAExceptTheScalarExpr->GetOstIdx();
  MeSSAUpdate::InsertDefPointsOfBBToSSACands(bb, cands, updateSSAExceptTheOstIdx);
}

void ValueRangePropagation::DeleteUnreachableBBs() {
  if (unreachableBBs.empty()) {
    return;
  }
  isCFGChange = true;
  for (BB *bb : unreachableBBs) {
    auto succs = bb->GetSucc();
    bb->RemoveAllPred();
    bb->RemoveAllSucc();
    for (auto &succ : succs) {
      std::map<OStIdx, std::set<BB*>> ssaupdateCandsForCondExpr;
      DeleteThePhiNodeWhichOnlyHasOneOpnd(*succ, nullptr, ssaupdateCandsForCondExpr);
    }
    UpdateTryAttribute(*bb);
    func.GetCfg()->NullifyBBByID(bb->GetBBId());
    // remove the bb from common_exit_bb's pred list if it is there
    auto &predsOfCommonExit = func.GetCfg()->GetCommonExitBB()->GetPred();
    auto it = std::find(predsOfCommonExit.begin(), predsOfCommonExit.end(), bb);
    if (it != predsOfCommonExit.end()) {
      func.GetCfg()->GetCommonExitBB()->RemoveExit(*bb);
    }
  }
}

bool IsEqualPrimType(PrimType lhsType, PrimType rhsType) {
  if (lhsType == rhsType) {
    return true;
  }
  if (IsUnsignedInteger(lhsType) == IsUnsignedInteger(rhsType) &&
      GetPrimTypeSize(lhsType) == GetPrimTypeSize(rhsType)) {
    return true;
  }
  return false;
}

bool IsNeededPrimType(PrimType prim) {
  return (prim == PTY_i8 || prim == PTY_i16 || prim == PTY_i32 || prim == PTY_i64 || prim == PTY_u8 ||
          prim == PTY_u16 || prim == PTY_u32 || prim == PTY_a32 || prim == PTY_ref || prim == PTY_ptr ||
          prim == PTY_u64 || prim == PTY_a64 || prim == PTY_u1);
}

int64 GetMinNumber(PrimType primType) {
  switch (primType) {
    case PTY_i8:
      return std::numeric_limits<int8_t>::min();
      break;
    case PTY_i16:
      return std::numeric_limits<int16_t>::min();
      break;
    case PTY_i32:
      return std::numeric_limits<int32_t>::min();
      break;
    case PTY_i64:
      return std::numeric_limits<int64_t>::min();
      break;
    case PTY_u8:
      return std::numeric_limits<uint8_t>::min();
      break;
    case PTY_u16:
      return std::numeric_limits<uint16_t>::min();
      break;
    case PTY_u32:
    case PTY_a32:
      return std::numeric_limits<uint32_t>::min();
      break;
    case PTY_ref:
    case PTY_ptr:
      if (GetPrimTypeSize(primType) == kFourByte) { // 32 bit
        return std::numeric_limits<uint32_t>::min();
      } else { // 64 bit
        CHECK_FATAL(GetPrimTypeSize(primType) == kEightByte, "must be 64 bit");
        return std::numeric_limits<uint64_t>::min();
      }
      break;
    case PTY_u64:
    case PTY_a64:
      return std::numeric_limits<uint64_t>::min();
      break;
    case PTY_u1:
      return 0;
      break;
    default:
      CHECK_FATAL(false, "must not be here");
      break;
  }
}

int64 GetMaxNumber(PrimType primType) {
  switch (primType) {
    case PTY_i8:
      return std::numeric_limits<int8_t>::max();
      break;
    case PTY_i16:
      return std::numeric_limits<int16_t>::max();
      break;
    case PTY_i32:
      return std::numeric_limits<int32_t>::max();
      break;
    case PTY_i64:
      return std::numeric_limits<int64_t>::max();
      break;
    case PTY_u8:
      return std::numeric_limits<uint8_t>::max();
      break;
    case PTY_u16:
      return std::numeric_limits<uint16_t>::max();
      break;
    case PTY_u32:
    case PTY_a32:
      return std::numeric_limits<uint32_t>::max();
      break;
    case PTY_ref:
    case PTY_ptr:
      if (GetPrimTypeSize(primType) == kFourByte) { // 32 bit
        return std::numeric_limits<uint32_t>::max();
      } else { // 64 bit
        CHECK_FATAL(GetPrimTypeSize(primType) == kEightByte, "must be 64 bit");
        return std::numeric_limits<uint64_t>::max();
      }
      break;
    case PTY_u64:
    case PTY_a64:
      return std::numeric_limits<uint64_t>::max();
      break;
    case PTY_u1:
      return 1;
      break;
    default:
      CHECK_FATAL(false, "must not be here");
      break;
  }
}

// If the result of operator add or sub is overflow or underflow, return false.
bool ValueRangePropagation::AddOrSubWithConstant(
    PrimType primType, Opcode op, int64 lhsConstant, int64 rhsConstant, int64 &res) const {
  if (ConstantFold::IntegerOpIsOverflow(op, primType, lhsConstant, rhsConstant)) {
    return false;
  }
  if (primType == PTY_u64) {
    res = (op == OP_add) ?
        (static_cast<uint64>(lhsConstant) + static_cast<uint64>(rhsConstant)) :
        (static_cast<uint64>(lhsConstant) - static_cast<uint64>(rhsConstant));
  } else {
    res = (op == OP_add) ? (lhsConstant + rhsConstant) : (lhsConstant - rhsConstant);
  }
  return true;
}

// Create new bound when old bound add or sub with a constant.
bool ValueRangePropagation::CreateNewBoundWhenAddOrSub(Opcode op, Bound bound, int64 rhsConstant, Bound &res) {
  int64 constant = 0;
  if (AddOrSubWithConstant(bound.GetPrimType(), op, bound.GetConstant(), rhsConstant, constant)) {
    res = Bound(bound.GetVar(), constant, bound.GetPrimType());
    return true;
  }
  return false;
}

// Judge whether the value is constant.
bool ValueRangePropagation::IsConstant(const BB &bb, MeExpr &expr, int64 &value, bool canNotBeNotEqual) {
  if (expr.GetMeOp() == kMeOpConst && static_cast<ConstMeExpr&>(expr).GetConstVal()->GetKind() == kConstInt) {
    value = static_cast<ConstMeExpr&>(expr).GetIntValue();
    return true;
  }
  auto *valueRange = FindValueRange(bb, expr);
  if (valueRange == nullptr) {
    if (expr.GetMeOp() == kMeOpVar && static_cast<VarMeExpr&>(expr).GetDefBy() == kDefByStmt &&
        static_cast<VarMeExpr&>(expr).GetDefStmt()->GetRHS()->GetMeOp() == kMeOpConst &&
        static_cast<ConstMeExpr*>(static_cast<VarMeExpr&>(expr).GetDefStmt()->GetRHS())->GetConstVal()->GetKind() ==
            kConstInt) {
      value = static_cast<ConstMeExpr*>(static_cast<VarMeExpr&>(expr).GetDefStmt()->GetRHS())->GetIntValue();
      (void)Insert2Caches(static_cast<VarMeExpr&>(expr).GetDefStmt()->GetBB()->GetBBId(), expr.GetExprID(),
                          std::make_unique<ValueRange>(Bound(value, expr.GetPrimType()), kEqual));
      return true;
    }
    return false;
  }
  if (valueRange->IsConstant()) {
    if (canNotBeNotEqual && valueRange->GetRangeType() == kNotEqual) {
      return false;
    }
    value = valueRange->GetBound().GetConstant();
    return true;
  }
  return false;
}

// Create new valueRange when old valueRange add or sub with a valuerange.
std::unique_ptr<ValueRange> ValueRangePropagation::AddOrSubWithValueRange(
    Opcode op, ValueRange &valueRangeLeft, ValueRange &valueRangeRight) {
  if (valueRangeLeft.GetRangeType() == kNotEqual || valueRangeRight.GetRangeType() == kNotEqual ||
      valueRangeLeft.GetRangeType() == kOnlyHasLowerBound || valueRangeRight.GetRangeType() == kOnlyHasLowerBound ||
      valueRangeLeft.GetRangeType() == kOnlyHasUpperBound || valueRangeRight.GetRangeType() == kOnlyHasUpperBound) {
    return nullptr;
  }
  // Deal with the case like :
  // x: vr(var: expr, constant: 4, kEqual)
  // y: vr(var: nullptr, constant: 2, kEqual)
  // x + y => vr(expr, 6, kEqual)
  if (valueRangeLeft.GetRangeType() == kEqual && valueRangeRight.GetRangeType() == kEqual) {
    auto *varOfLHS = valueRangeLeft.GetBound().GetVar();
    auto *varOfRHS = valueRangeRight.GetBound().GetVar();
    if ((varOfLHS == nullptr && varOfRHS != nullptr) || (varOfLHS != nullptr && varOfRHS == nullptr)) {
      auto *resVar = (varOfLHS == nullptr) ? varOfRHS : varOfLHS;
      int64 rhsConst = valueRangeRight.GetBound().GetConstant();
      int64 lhsConst = valueRangeLeft.GetBound().GetConstant();
      int64 res = 0;
      if (AddOrSubWithConstant(valueRangeLeft.GetLower().GetPrimType(), op, rhsConst, lhsConst, res)) {
        return std::make_unique<ValueRange>(Bound(resVar, res, valueRangeLeft.GetLower().GetPrimType()), kEqual);
      }
    }
  }
  if (valueRangeLeft.GetLower().GetVar() != valueRangeRight.GetLower().GetVar() ||
      valueRangeLeft.GetUpper().GetVar() != valueRangeRight.GetUpper().GetVar()) {
    return nullptr;
  }
  int64 rhsLowerConst = valueRangeRight.GetLower().GetConstant();
  int64 rhsUpperConst = valueRangeRight.GetUpper().GetConstant();
  int64 res = 0;
  if (!AddOrSubWithConstant(valueRangeLeft.GetLower().GetPrimType(), op,
                            valueRangeLeft.GetLower().GetConstant(), rhsLowerConst, res)) {
    return nullptr;
  }
  Bound lower = Bound(valueRangeLeft.GetLower().GetVar(), res, valueRangeLeft.GetLower().GetPrimType());
  res = 0;
  if (!AddOrSubWithConstant(valueRangeLeft.GetLower().GetPrimType(), op,
                            valueRangeLeft.GetUpper().GetConstant(), rhsUpperConst, res)) {
    return nullptr;
  }
  Bound upper = Bound(valueRangeLeft.GetUpper().GetVar(), res, valueRangeLeft.GetUpper().GetPrimType());
  return std::make_unique<ValueRange>(lower, upper, kLowerAndUpper,
      valueRangeLeft.IsAccurate() || valueRangeRight.IsAccurate());
}

bool ValueRangePropagation::AddOrSubWithBound(Bound oldBound, Bound &resBound, int64 rhsConstant, Opcode op) {
  int64 res = 0;
  if (!AddOrSubWithConstant(oldBound.GetPrimType(), op,
                            oldBound.GetConstant(), rhsConstant, res)) {
    return false;
  }
  resBound = Bound(oldBound.GetVar(), res, oldBound.GetPrimType());
  return true;
}

// Create new valueRange when old valueRange add or sub with a constant.
std::unique_ptr<ValueRange> ValueRangePropagation::AddOrSubWithValueRange(
    Opcode op, ValueRange &valueRange, int64 rhsConstant) {
  if (valueRange.GetRangeType() == kLowerAndUpper) {
    Bound lower;
    if (!AddOrSubWithBound(valueRange.GetLower(), lower, rhsConstant, op)) {
      return nullptr;
    }
    Bound upper;
    if (!AddOrSubWithBound(valueRange.GetUpper(), upper, rhsConstant, op)) {
      return nullptr;
    }
    return std::make_unique<ValueRange>(lower, upper, kLowerAndUpper, valueRange.IsAccurate());
  } else if (valueRange.GetRangeType() == kEqual) {
    Bound bound;
    if (!AddOrSubWithBound(valueRange.GetBound(), bound, rhsConstant, op)) {
      return nullptr;
    }
    return std::make_unique<ValueRange>(bound, valueRange.GetRangeType(), valueRange.IsAccurate());
  } else if (valueRange.GetRangeType() == kOnlyHasLowerBound || valueRange.GetRangeType() == kOnlyHasUpperBound) {
    Bound bound;
    if (!AddOrSubWithBound(valueRange.GetBound(), bound, rhsConstant, op)) {
      return nullptr;
    }
    return std::make_unique<ValueRange>(bound, valueRange.GetStride(), valueRange.GetRangeType());
  }
  return nullptr;
}

// deal with the case like var % x, range is [1-x, x-1]
std::unique_ptr<ValueRange> ValueRangePropagation::RemWithRhsValueRange(
    const OpMeExpr &opMeExpr, int64 rhsConstant) const {
  int64 res = 0;
  int64 upperRes = 0;
  int64 lowerRes = 0;
  if (rhsConstant > 0) {
    if (AddOrSubWithConstant(opMeExpr.GetPrimType(), OP_sub, 1, rhsConstant, res)) {
      lowerRes = res;
    } else {
      return nullptr;
    }
    if (AddOrSubWithConstant(opMeExpr.GetPrimType(), OP_sub, rhsConstant, 1, res)) {
      upperRes = res;
    } else {
      return nullptr;
    }
  } else {
    if (AddOrSubWithConstant(opMeExpr.GetPrimType(), OP_add, 1, rhsConstant, res)) {
      lowerRes = res;
    } else {
      return nullptr;
    }
    if (AddOrSubWithConstant(opMeExpr.GetPrimType(), OP_sub, -rhsConstant, 1, res)) {
      upperRes = res;
    } else {
      return nullptr;
    }
  }
  Bound lower = Bound(nullptr, lowerRes, opMeExpr.GetPrimType());
  Bound upper = Bound(nullptr, upperRes, opMeExpr.GetPrimType());
  return std::make_unique<ValueRange>(lower, upper, kLowerAndUpper);
}

// Create new valueRange when old valueRange rem with a constant.
std::unique_ptr<ValueRange> ValueRangePropagation::RemWithValueRange(const BB &bb, const OpMeExpr &opMeExpr,
    int64 rhsConstant) {
  auto remValueRange = RemWithRhsValueRange(opMeExpr, rhsConstant);
  if (remValueRange == nullptr) {
    return nullptr;
  }
  auto *valueRange = FindValueRange(bb, *opMeExpr.GetOpnd(0));
  if (valueRange == nullptr) {
    return remValueRange;
  }
  if (!valueRange->IsConstantLowerAndUpper()) {
    return remValueRange;
  } else if (valueRange->GetRangeType() == kEqual) {
    int64 res = Bound::GetRemResult(valueRange->GetBound().GetConstant(), rhsConstant, opMeExpr.GetPrimType());
    Bound bound = Bound(nullptr, res, valueRange->GetBound().GetPrimType());
    return std::make_unique<ValueRange>(bound, valueRange->GetRangeType());
  } else {
    std::unique_ptr<ValueRange> combineRes = CombineTwoValueRange(*remValueRange, *valueRange);
    int64 lowerRes = combineRes->GetLower().GetConstant();
    int64 upperRes = combineRes->GetUpper().GetConstant();
    if (upperRes <= lowerRes) {
      return remValueRange;
    }
    if (lowerRes > 0 && valueRange->GetUpper().GetConstant() > upperRes) {
      lowerRes = 0;
    }
    Bound lower = Bound(nullptr, lowerRes, opMeExpr.GetPrimType());
    Bound upper = Bound(nullptr, upperRes, opMeExpr.GetPrimType());
    return std::make_unique<ValueRange>(lower, upper, kLowerAndUpper);
  }
  return nullptr;
}

// Create valueRange when deal with OP_rem.
std::unique_ptr<ValueRange> ValueRangePropagation::DealWithRem(
    const BB &bb, const MeExpr &lhsVar, const OpMeExpr &opMeExpr) {
  if (!IsNeededPrimType(opMeExpr.GetPrimType())) {
    return nullptr;
  }
  auto *opnd0 = opMeExpr.GetOpnd(0);
  auto *opnd1 = opMeExpr.GetOpnd(1);
  int64 lhsConstant = 0;
  int64 rhsConstant = 0;
  bool lhsIsConstant = IsConstant(bb, *opnd0, lhsConstant);
  bool rhsIsConstant = IsConstant(bb, *opnd1, rhsConstant);
  if (rhsIsConstant == 0) {
    return nullptr;
  }
  std::unique_ptr<ValueRange> newValueRange;
  if (lhsIsConstant && rhsIsConstant) {
    int64 res = Bound::GetRemResult(lhsConstant, rhsConstant, opMeExpr.GetPrimType());
    newValueRange = std::make_unique<ValueRange>(Bound(res, opMeExpr.GetPrimType()), kEqual);
  } else if (rhsIsConstant) {
    newValueRange = RemWithValueRange(bb, opMeExpr, rhsConstant);
  }
  if (newValueRange != nullptr) {
    auto *valueRangePtr = newValueRange.get();
    if (Insert2Caches(bb.GetBBId(), lhsVar.GetExprID(), CopyValueRange(*valueRangePtr))) {
      (void)Insert2Caches(bb.GetBBId(), opMeExpr.GetExprID(), CopyValueRange(*valueRangePtr));
      return CopyValueRange(*valueRangePtr);
    }
  }
  return nullptr;
}

// Create valueRange when deal with OP_add or OP_sub.
std::unique_ptr<ValueRange> ValueRangePropagation::DealWithAddOrSub(
    const BB &bb, const MeExpr &lhsVar, const OpMeExpr &opMeExpr) {
  auto *opnd0 = opMeExpr.GetOpnd(0);
  auto *opnd1 = opMeExpr.GetOpnd(1);
  int64 lhsConstant = 0;
  int64 rhsConstant = 0;
  bool lhsIsConstant = IsConstant(bb, *opnd0, lhsConstant);
  bool rhsIsConstant = IsConstant(bb, *opnd1, rhsConstant);
  std::unique_ptr<ValueRange> newValueRange;
  if (lhsIsConstant && rhsIsConstant) {
    int64 res = 0;
    if (AddOrSubWithConstant(opMeExpr.GetPrimType(), opMeExpr.GetOp(), lhsConstant, rhsConstant, res)) {
      newValueRange = std::make_unique<ValueRange>(Bound(res, opMeExpr.GetPrimType()), kEqual);
    }
  } else if (rhsIsConstant) {
    auto *valueRange = FindValueRange(bb, *opnd0);
    if (valueRange == nullptr) {
      return nullptr;
    }
    newValueRange = AddOrSubWithValueRange(opMeExpr.GetOp(), *valueRange, rhsConstant);
  } else if (lhsIsConstant && opMeExpr.GetOp() == OP_add) {
    auto *valueRange = FindValueRange(bb, *opnd1);
    if (valueRange == nullptr) {
      return nullptr;
    }
    newValueRange = AddOrSubWithValueRange(opMeExpr.GetOp(), *valueRange, lhsConstant);
  }
  if (newValueRange != nullptr) {
    auto *valueRangePtr = newValueRange.get();
    if (Insert2Caches(bb.GetBBId(), lhsVar.GetExprID(), CopyValueRange(*valueRangePtr))) {
      (void)Insert2Caches(bb.GetBBId(), opMeExpr.GetExprID(), CopyValueRange(*valueRangePtr));
      return CopyValueRange(*valueRangePtr);
    }
  }
  return nullptr;
}

// Save array length to caches for eliminate array boundary check.
void ValueRangePropagation::DealWithArrayLength(const BB &bb, MeExpr &lhs, MeExpr &rhs) {
  if (rhs.GetMeOp() == kMeOpVar) {
    auto *valueRangeOfLength = FindValueRange(bb, rhs);
    if (valueRangeOfLength == nullptr) {
      (void)Insert2Caches(bb.GetBBId(), lhs.GetExprID(),
                          std::make_unique<ValueRange>(Bound(&rhs, rhs.GetPrimType()), kEqual));
      (void)Insert2Caches(bb.GetBBId(), rhs.GetExprID(),
                          std::make_unique<ValueRange>(Bound(&rhs, rhs.GetPrimType()), kEqual));
    } else {
      (void)Insert2Caches(bb.GetBBId(), lhs.GetExprID(), CopyValueRange(*valueRangeOfLength));
    }
  } else {
    CHECK_FATAL(rhs.GetMeOp() == kMeOpConst, "must be constant");
  }
  length2Def[&lhs] = &rhs;
  lengthSet.insert(&lhs);
  lengthSet.insert(&rhs);
}

bool IsPrimTypeUint64(PrimType pType) {
  if (pType == PTY_u64 || pType == PTY_a64 || ((pType == PTY_ref || pType == PTY_ptr) &&
      GetPrimTypeSize(pType) == kEightByte)) {
    return true;
  }
  return false;
}

// Get the real value with primType.
int64 GetRealValue(int64 value, PrimType primType) {
  switch (primType) {
    case PTY_i8:
      return static_cast<int8>(value);
      break;
    case PTY_i16:
      return static_cast<int16>(value);
      break;
    case PTY_i32:
      return static_cast<int32>(value);
      break;
    case PTY_i64:
      return static_cast<int64>(value);
      break;
    case PTY_u8:
      return static_cast<uint8>(value);
      break;
    case PTY_u16:
      return static_cast<uint16>(value);
      break;
    case PTY_u32:
    case PTY_a32:
      return static_cast<uint32>(value);
      break;
    case PTY_ref:
    case PTY_ptr:
      if (GetPrimTypeSize(primType) == kFourByte) { // 32 bit
        return static_cast<uint32>(value);
      } else { // 64 bit
        CHECK_FATAL(GetPrimTypeSize(primType) == kEightByte, "must be 64 bit");
        return static_cast<uint64>(value);
      }
      break;
    case PTY_u64:
    case PTY_a64:
      return static_cast<uint64>(value);
      break;
    case PTY_u1:
      return static_cast<bool>(value);
      break;
    default:
      CHECK_FATAL(false, "must not be here");
      break;
  }
}

std::unique_ptr<ValueRange> ValueRangePropagation::CopyValueRange(ValueRange &valueRange, PrimType primType) {
  switch (valueRange.GetRangeType()) {
    case kEqual:
    case kNotEqual:
      if (primType == PTY_begin) {
        return std::make_unique<ValueRange>(valueRange.GetBound(), valueRange.GetRangeType(), valueRange.IsAccurate());
      } else {
        Bound bound = Bound(valueRange.GetBound().GetVar(), valueRange.GetBound().GetConstant(), primType);
        return std::make_unique<ValueRange>(bound, valueRange.GetRangeType(), valueRange.IsAccurate());
      }
    case kLowerAndUpper:
    case kSpecialUpperForLoop:
    case kSpecialLowerForLoop:
      if (primType == PTY_begin) {
        return std::make_unique<ValueRange>(valueRange.GetLower(), valueRange.GetUpper(), valueRange.GetRangeType(),
            valueRange.IsAccurate());
      } else {
        Bound lower = Bound(valueRange.GetLower().GetVar(), valueRange.GetLower().GetConstant(), primType);
        Bound upper = Bound(valueRange.GetUpper().GetVar(), valueRange.GetUpper().GetConstant(), primType);
        return std::make_unique<ValueRange>(lower, upper, valueRange.GetRangeType(), valueRange.IsAccurate());
      }
    case kOnlyHasLowerBound:
      if (primType == PTY_begin) {
        return std::make_unique<ValueRange>(valueRange.GetLower(), valueRange.GetStride(), kOnlyHasLowerBound);
      } else {
        Bound lower = Bound(valueRange.GetLower().GetVar(), valueRange.GetLower().GetConstant(), primType);
        return std::make_unique<ValueRange>(lower, valueRange.GetStride(), kOnlyHasLowerBound);
      }

    case kOnlyHasUpperBound:
      if (primType == PTY_begin) {
        return std::make_unique<ValueRange>(valueRange.GetUpper(), valueRange.GetStride(), kOnlyHasUpperBound);
      } else {
        Bound upper = Bound(valueRange.GetUpper().GetVar(), valueRange.GetUpper().GetConstant(), primType);
        return std::make_unique<ValueRange>(upper, kOnlyHasUpperBound, valueRange.IsAccurate());
      }
    default:
      CHECK_FATAL(false, "can not be here");
      break;
  }
}

void ValueRangePropagation::DealWithMeOp(const BB &bb, const MeStmt &stmt) {
  auto *lhs = stmt.GetLHS();
  auto *rhs = stmt.GetRHS();
  auto *opMeExpr = static_cast<OpMeExpr*>(rhs);
  switch (rhs->GetOp()) {
    case OP_add:
    case OP_sub: {
      (void)DealWithAddOrSub(bb, *lhs, *opMeExpr);
      break;
    }
    case OP_gcmallocjarray: {
      DealWithArrayLength(bb, *lhs, *opMeExpr->GetOpnd(0));
      break;
    }
    case OP_rem: {
      (void)DealWithRem(bb, *lhs, *opMeExpr);
      break;
    }
    default:
      break;
  }
}

// Create new value range when deal with assign.
void ValueRangePropagation::DealWithAssign(BB &bb, const MeStmt &stmt) {
  auto *lhs = stmt.GetLHS();
  auto *rhs = stmt.GetRHS();
  if (lhs == nullptr || rhs == nullptr) {
    return;
  }
  Insert2PairOfExprs(*lhs, *rhs, bb);
  Insert2PairOfExprs(*rhs, *lhs, bb);
  auto *existValueRange = FindValueRange(bb, *rhs);
  if (existValueRange != nullptr && existValueRange->GetRangeType() != kOnlyHasLowerBound &&
      existValueRange->GetRangeType() != kOnlyHasUpperBound) {
    (void)Insert2Caches(bb.GetBBId(), lhs->GetExprID(), CopyValueRange(*existValueRange));
    return;
  }
  if (rhs->GetMeOp() == kMeOpOp) {
    DealWithMeOp(bb, stmt);
  } else if (rhs->GetMeOp() == kMeOpConst && static_cast<ConstMeExpr*>(rhs)->GetConstVal()->GetKind() == kConstInt) {
    if (FindValueRange(bb, *lhs) != nullptr) {
      return;
    }
    std::unique_ptr<ValueRange> valueRange =
        std::make_unique<ValueRange>(Bound(static_cast<ConstMeExpr*>(rhs)->GetIntValue(), rhs->GetPrimType()), kEqual);
    (void)Insert2Caches(bb.GetBBId(), lhs->GetExprID(), std::move(valueRange));
  } else if (rhs->GetMeOp() == kMeOpVar) {
    if (lengthSet.find(rhs) != lengthSet.end()) {
      std::unique_ptr<ValueRange> valueRange = std::make_unique<ValueRange>(Bound(rhs, rhs->GetPrimType()), kEqual);
      (void)Insert2Caches(bb.GetBBId(), lhs->GetExprID(), std::move(valueRange));
      return;
    }
  } else if (rhs->GetMeOp() == kMeOpNary) {
    auto *nary = static_cast<NaryMeExpr*>(rhs);
    if (nary->GetIntrinsic() == INTRN_JAVA_ARRAY_LENGTH) {
      ASSERT(nary->GetOpnds().size() == 1, "must be");
      if (IsPrimitivePureScalar(nary->GetOpnd(0)->GetPrimType())) {
        return;
      }
      ASSERT(nary->GetOpnd(0)->GetPrimType() == PTY_ref, "must be");
      DealWithArrayLength(bb, *lhs, *nary->GetOpnd(0));
    }
  }
}

// Analysis the stride of loop induction var, like this pattern:
// i1 = phi(i0, i2),
// i2 = i1 + 1,
// stride is 1.
bool ValueRangePropagation::CanComputeLoopIndVar(const MeExpr &phiLHS, MeExpr &expr, int64 &constant) {
  auto *curExpr = &expr;
  while (true) {
    if (!curExpr->IsScalar()) {
      break;
    }
    ScalarMeExpr *scalarMeExpr = static_cast<ScalarMeExpr*>(curExpr);
    if (scalarMeExpr->GetDefBy() != kDefByStmt) {
      break;
    }
    MeStmt *defStmt = scalarMeExpr->GetDefStmt();
    if (defStmt->GetRHS()->GetOp() != OP_add && defStmt->GetRHS()->GetOp() != OP_sub) {
      break;
    }
    OpMeExpr &opMeExpr = static_cast<OpMeExpr&>(*defStmt->GetRHS());
    if (opMeExpr.GetOpnd(1)->GetMeOp() == kMeOpConst &&
        static_cast<ConstMeExpr*>(opMeExpr.GetOpnd(1))->GetConstVal()->GetKind() == kConstInt) {
      ConstMeExpr *rhsExpr = static_cast<ConstMeExpr*>(opMeExpr.GetOpnd(1));
      int64 res = 0;
      auto rhsConst = rhsExpr->GetIntValue();
      if (rhsExpr->GetPrimType() == PTY_u64 && static_cast<uint64>(rhsConst) > GetMaxNumber(PTY_i64)) {
        return false;
      }
      if (AddOrSubWithConstant(opMeExpr.GetPrimType(), defStmt->GetRHS()->GetOp(), constant, rhsConst, res)) {
        constant = res;
        curExpr = opMeExpr.GetOpnd(0);
        if (curExpr == &phiLHS) {
          return true;
        }
        continue;
      }
    }
    break;
  }
  return false;
}

// Create new value range when the loop induction var is monotonic increase.
std::unique_ptr<ValueRange> ValueRangePropagation::CreateValueRangeForMonotonicIncreaseVar(
    const LoopDesc &loop, BB &exitBB, const BB &bb, OpMeExpr &opMeExpr, MeExpr &opnd1,
    Bound &initBound, int64 stride) {
  BB *inLoopBB = nullptr;
  BB *outLoopBB = nullptr;
  if (loop.Has(*exitBB.GetSucc(0))) {
    inLoopBB = exitBB.GetSucc(0);
    outLoopBB = exitBB.GetSucc(1);
    CHECK_FATAL(!loop.Has(*exitBB.GetSucc(1)), "must not in loop");
  } else {
    inLoopBB = exitBB.GetSucc(1);
    outLoopBB = exitBB.GetSucc(0);
    CHECK_FATAL(!loop.Has(*exitBB.GetSucc(0)), "must not in loop");
  }
  int64 rightConstant = 0;
  if (opMeExpr.GetOp() == OP_lt || opMeExpr.GetOp() == OP_ge ||
      opMeExpr.GetOp() == OP_ne || opMeExpr.GetOp() == OP_eq) {
    rightConstant = -stride;
  }
  Bound upperBound;
  int64 constantValue = 0;
  if (IsConstant(bb, opnd1, constantValue)) {
    int64 res = 0;
    if (AddOrSubWithConstant(opnd1.GetPrimType(), OP_add, constantValue, rightConstant, res)) {
      upperBound = Bound(res, opnd1.GetPrimType());
    } else {
      return nullptr;
    }
    if (initBound.GetVar() == upperBound.GetVar() && initBound.IsGreaterThan(upperBound, opMeExpr.GetOpndType())) {
      if (opMeExpr.GetOp() != OP_ne && opMeExpr.GetOp() != OP_eq) {
        AnalysisUnreachableBBOrEdge(exitBB, *inLoopBB, *outLoopBB);
      }
      return nullptr;
    }
    if (initBound.GetVar() != nullptr) {
      return std::make_unique<ValueRange>(initBound, upperBound, kSpecialLowerForLoop);
    } else {
      return std::make_unique<ValueRange>(initBound, upperBound, kLowerAndUpper);
    }
  } else if (opnd1.GetOp() == OP_add || opnd1.GetOp() == OP_sub) {
    auto valueRangeOfUpper = DealWithAddOrSub(bb, opnd1, static_cast<OpMeExpr&>(opnd1));
    if (valueRangeOfUpper != nullptr) {
      int64 res = 0;
      if (AddOrSubWithConstant(opnd1.GetPrimType(), OP_add, valueRangeOfUpper->GetUpper().GetConstant(),
                               rightConstant, res)) {
        upperBound = Bound(valueRangeOfUpper->GetUpper().GetVar(), res, opnd1.GetPrimType());
      }
      if (initBound.GetVar() == nullptr) {
        if (valueRangeOfUpper->GetRangeType() == kLowerAndUpper || valueRangeOfUpper->GetRangeType() == kEqual) {
          if (valueRangeOfUpper->GetUpper().GetVar() == nullptr) {
            return std::make_unique<ValueRange>(initBound, upperBound, kLowerAndUpper);
          } else {
            return std::make_unique<ValueRange>(initBound, upperBound, kSpecialUpperForLoop);
          }
        }
      } else if ((valueRangeOfUpper->GetRangeType() == kLowerAndUpper || valueRangeOfUpper->GetRangeType() == kEqual) &&
                 valueRangeOfUpper->GetUpper().GetVar() == nullptr) {
        return std::make_unique<ValueRange>(initBound, upperBound, kSpecialLowerForLoop);
      }
    }
  }
  if (initBound.GetVar() == nullptr) {
    return std::make_unique<ValueRange>(
        initBound, Bound(&opnd1, rightConstant, initBound.GetPrimType()), kSpecialUpperForLoop);
  }
  return nullptr;
}

// Create new value range when the loop induction var is monotonic decrease.
std::unique_ptr<ValueRange> ValueRangePropagation::CreateValueRangeForMonotonicDecreaseVar(
    const LoopDesc &loop, BB &exitBB, const BB &bb, OpMeExpr &opMeExpr, MeExpr &opnd1,
    Bound &initBound, int64 stride) {
  BB *inLoopBB = nullptr;
  BB *outLoopBB = nullptr;
  if (loop.Has(*exitBB.GetSucc(0))) {
    inLoopBB = exitBB.GetSucc(0);
    outLoopBB = exitBB.GetSucc(1);
    CHECK_FATAL(!loop.Has(*exitBB.GetSucc(1)), "must not in loop");
  } else {
    inLoopBB = exitBB.GetSucc(1);
    outLoopBB = exitBB.GetSucc(0);
    CHECK_FATAL(!loop.Has(*exitBB.GetSucc(0)), "must not in loop");
  }
  int64 rightConstant = 0;
  if (opMeExpr.GetOp() == OP_le || opMeExpr.GetOp() == OP_gt ||
      opMeExpr.GetOp() == OP_ne || opMeExpr.GetOp() == OP_eq) {
    rightConstant = -stride;
  }
  Bound lowerBound;
  int64 constantValue = 0;
  if (IsConstant(bb, opnd1, constantValue)) {
    int64 res = 0;
    if (AddOrSubWithConstant(opnd1.GetPrimType(), OP_add, constantValue, rightConstant, res)) {
      lowerBound = Bound(res, opnd1.GetPrimType());
    } else {
      return nullptr;
    }
    if (lowerBound.GetVar() == initBound.GetVar() && lowerBound.IsGreaterThan(initBound, opMeExpr.GetOpndType())) {
      if (opMeExpr.GetOp() != OP_ne && opMeExpr.GetOp() != OP_eq) {
        AnalysisUnreachableBBOrEdge(exitBB, *inLoopBB, *outLoopBB);
      }
      return nullptr;
    }
    return initBound.GetVar() == nullptr ? std::make_unique<ValueRange>(lowerBound, initBound, kLowerAndUpper) :
        std::make_unique<ValueRange>(lowerBound, initBound, kSpecialUpperForLoop);
  } else {
    if (initBound.GetVar() == nullptr) {
      return std::make_unique<ValueRange>(
          Bound(&opnd1, rightConstant, opnd1.GetPrimType()), initBound, kSpecialLowerForLoop);
    }
  }
  return nullptr;
}

std::unique_ptr<ValueRange> ValueRangePropagation::MergeValuerangeOfPhi(
    std::vector<std::unique_ptr<ValueRange>> &valueRangeOfPhi) {
  std::unique_ptr<ValueRange> res = std::move(valueRangeOfPhi.at(0));
  for (size_t i = 1; i < valueRangeOfPhi.size(); ++i) {
    res = CombineTwoValueRange(*res, *valueRangeOfPhi[i]);
  }
  return res;
}

std::unique_ptr<ValueRange> ValueRangePropagation::MakeMonotonicIncreaseOrDecreaseValueRangeForPhi(
    int64 stride, Bound &initBound) const {
  if (stride > 0) {
    return std::make_unique<ValueRange>(initBound, stride, kOnlyHasLowerBound);
  }
  if (stride < 0) {
    return std::make_unique<ValueRange>(initBound, stride, kOnlyHasUpperBound);
  }
  return nullptr;
}

// Traverse the bbs in the loop in order to create vr for the loop variable.
void ValueRangePropagation::TravelBBs(std::vector<BB*> &reversePostOrderOfLoopBBs) {
  for (size_t i = 0; i < reversePostOrderOfLoopBBs.size(); ++i) {
    auto *bb = reversePostOrderOfLoopBBs[i];
    if (unreachableBBs.find(bb) != unreachableBBs.end()) {
      continue;
    }
    if (i != 0) { // The phi has already been dealed with.
      DealWithPhi(*bb);
    }
    for (auto it = bb->GetMeStmts().begin(); it != bb->GetMeStmts().end(); ++it) {
      for (size_t i = 0; i < it->NumMeStmtOpnds(); ++i) {
        DealWithOperand(*bb, *it, *it->GetOpnd(i));
      }
      switch (it->GetOp()) {
        case OP_dassign:
        case OP_maydassign:
        case OP_regassign: {
          if (it->GetLHS() != nullptr && it->GetLHS()->IsVolatile()) {
            break;
          }
          DealWithAssign(*bb, *it);
          break;
        }
        case OP_brfalse:
        case OP_brtrue: {
          DealWithCondGoto(*bb, *it);
          break;
        }
        case OP_callassigned: {
          DealWithCallassigned(*bb, *it);
          break;
        }
        case OP_switch: {
          DealWithSwitch(*bb, *it);
          break;
        }
        case OP_igoto:
        default:
          break;
      }
    }
  }
}

void ValueRangePropagation::CreateVRForPhi(LoopDesc &loop, const BB &bb) {
  onlyPropVRStack.push(onlyPropVR);
  onlyPropVR = true;
  std::vector<BB*> reversePostOrderOfLoopBBs;
  // Collect loop bbs.
  for (auto it = currItOfTravelReversePostOrder; it != dom.GetReversePostOrder().end(); ++it) {
    if (reversePostOrderOfLoopBBs.size() == loop.loopBBs.size()) {
      break;
    }
    if (loop.Has(**it)) {
      reversePostOrderOfLoopBBs.push_back(*it);
    }
  }
  // Travels loop bbs.
  TravelBBs(reversePostOrderOfLoopBBs);
  onlyPropVR = onlyPropVRStack.top();
  onlyPropVRStack.pop();
}

// Calculate the valuerange of def-operand according to the valuerange of each rhs operand.
std::unique_ptr<ValueRange> ValueRangePropagation::MergeValueRangeOfPhiOperands(const BB &bb, MePhiNode &mePhiNode) {
  auto *resValueRange = FindValueRange(*bb.GetPred(0), *mePhiNode.GetOpnd(0));
  if (resValueRange == nullptr || !resValueRange->IsConstant()) {
    return nullptr;
  }
  bool theRangeTypeIsNotEqual = (resValueRange->GetRangeType() == kNotEqual);
  auto resValueRangePtr = CopyValueRange(*resValueRange);
  for (size_t i = 1; i < mePhiNode.GetOpnds().size(); ++i) {
    auto *operand = mePhiNode.GetOpnd(i);
    auto *valueRange = FindValueRange(*bb.GetPred(i), *operand);
    // If one valuerange is nullptr, the result is nullptr.
    if (valueRange == nullptr || !valueRange->IsConstant()) {
      return nullptr;
    }
    theRangeTypeIsNotEqual |= (valueRange->GetRangeType() == kNotEqual);
    if (resValueRange->IsEqual(valueRange)) {
      continue;
    }
    // If the range type of phi operands is kNotEqual and the values are not equal, return nullptr.
    if (theRangeTypeIsNotEqual) {
      return nullptr;
    }
    if (resValueRange->GetUpper().GetPrimType() != valueRange->GetUpper().GetPrimType()) {
      return nullptr;
    }
    resValueRangePtr = CombineTwoValueRange(*resValueRangePtr, *valueRange, true);
  }
  return resValueRangePtr;
}

bool ValueRangePropagation::MergeVrOrInitAndBackedge(MePhiNode &mePhiNode, ValueRange &vrOfInitExpr,
    ValueRange &valueRange, Bound &resBound) {
  bool isOnlyHasLowerBound = vrOfInitExpr.GetRangeType() == kOnlyHasLowerBound;
  auto pType = mePhiNode.GetLHS()->GetPrimType();
  auto upperBound = isOnlyHasLowerBound ? valueRange.GetBound() : vrOfInitExpr.GetBound();
  auto LowerBound = isOnlyHasLowerBound ? vrOfInitExpr.GetBound() : valueRange.GetBound();
  if (valueRange.GetRangeType() == kNotEqual) {
    if (vrOfInitExpr.GetBound().IsConstantBound() && valueRange.IsConstantRange()) {
      if (LowerBound.IsGreaterThan(upperBound, pType)) {
        return false;
      }
      int64 res = 0;
      if (!AddOrSubWithConstant(pType, OP_sub, upperBound.GetConstant(), LowerBound.GetConstant(), res)) {
        return false;
      }
      if (Bound(res, pType).IsLessThanOrEqualTo(Bound(vrOfInitExpr.GetStride(), pType), pType)) {
        return false;
      }
    }
    int64 res = 0;
    if (AddOrSubWithConstant(pType, isOnlyHasLowerBound ? OP_sub : OP_add,
        valueRange.GetBound().GetConstant(), 1, res)) {
      resBound = Bound(valueRange.GetBound().GetVar(), res, pType);
    } else {
      return false;
    }
  } else {
    resBound = isOnlyHasLowerBound ? valueRange.GetUpper() : valueRange.GetLower();
    resBound.SetPrimType(pType);
  }
  return true;
}

// Calculate the valuerange of def-operand according to the valuerange of each rhs operand.
void ValueRangePropagation::MergeValueRangeOfPhiOperands(const LoopDesc &loop, const BB &bb,
    std::vector<std::unique_ptr<ValueRange>> &valueRangeOfInitExprs, size_t indexOfInitExpr) {
  size_t index = 0;
  for (auto &it : bb.GetMePhiList()) {
    if (!it.second->GetIsLive()) {
      continue;
    }
    if (it.second->GetOpnds().size() != kNumOperands) {
      return;
    }
    auto *mePhiNode = it.second;
    auto *vrOfInitExpr = valueRangeOfInitExprs.at(index).get();
    auto pType = it.second->GetLHS()->GetPrimType();
    if (vrOfInitExpr == nullptr ||
        (vrOfInitExpr->GetRangeType() != kOnlyHasLowerBound && vrOfInitExpr->GetRangeType() != kOnlyHasUpperBound) ||
        (GetVecLanes(pType) > 0)) {
      index++;
      continue;
    }
    Bound resBound = vrOfInitExpr->GetRangeType() == kOnlyHasLowerBound ?
        Bound( nullptr, GetMaxNumber(pType), pType) : Bound( nullptr, GetMinNumber(pType), pType);
    bool vrCanBeComputed = true;
    index++;
    for (size_t i = 0; i < mePhiNode->GetOpnds().size(); ++i) {
      if (i == indexOfInitExpr) {
        continue;
      }
      auto *operand = mePhiNode->GetOpnd(i);
      auto *valueRange = FindValueRange(*bb.GetPred(i), *operand);
      if (valueRange == nullptr ||
          valueRange->GetRangeType() == kOnlyHasUpperBound ||
          valueRange->GetRangeType() == kOnlyHasLowerBound ||
          vrOfInitExpr->IsEqual(valueRange) ||
          !MergeVrOrInitAndBackedge(*mePhiNode, *vrOfInitExpr, *valueRange, resBound)) {
        vrCanBeComputed = false;
        break;
      }
    }
    if (!vrCanBeComputed) {
      continue;
    }
    auto stride = abs(vrOfInitExpr->GetStride());
    onlyRecordValueRangeInTempCache.push(false);

    bool isOnlyHasLowerBound = vrOfInitExpr->GetRangeType() == kOnlyHasLowerBound;
    auto valueRangeOfPhi = isOnlyHasLowerBound ?
        std::make_unique<ValueRange>(vrOfInitExpr->GetBound(), resBound, kLowerAndUpper) :
        std::make_unique<ValueRange>(resBound, vrOfInitExpr->GetBound(), kLowerAndUpper);
    if (!valueRangeOfPhi->IsConstantRange() || stride == 1) {
      if (loop.IsCanonicalAndOnlyHasOneExitBBLoop()) {
        valueRangeOfPhi->SetAccurate(true);
      }
    } else if (stride != 1) {
      auto constLower = valueRangeOfPhi->GetLower().GetConstant();
      auto constUpper = valueRangeOfPhi->GetUpper().GetConstant();
      if (!ConstantFold::IntegerOpIsOverflow(OP_sub, pType, constUpper, constLower) &&
          Bound(constUpper - constLower, pType).IsGreaterThan(Bound(stride, pType), pType) &&
          valueRangeOfPhi->GetUpper().IsGreaterThan(valueRangeOfPhi->GetLower(), pType)) {
        int64 rem = (constUpper - constLower) % stride;
        bool isAccurate = false;
        if (isOnlyHasLowerBound) {
          // If the vr of phi node is (lower: 0, upper: 19, stride: 2),
          // update the vr with (lower: 0, upper: 18, stride: 2)
          if (!ConstantFold::IntegerOpIsOverflow(OP_sub, pType, constUpper, rem)) {
            int64 constantOfNewUpper = constUpper - rem;
            if (constantOfNewUpper >= constLower && constantOfNewUpper <= constUpper) {
              valueRangeOfPhi->SetUpper(Bound(nullptr, constantOfNewUpper, pType));
              isAccurate = true;
            }
          }
        } else {
          if (!ConstantFold::IntegerOpIsOverflow(OP_add, pType, constUpper, rem)) {
            int64 constantOfNewLower = constLower + rem;
            if (constantOfNewLower >= constLower && constantOfNewLower <= constUpper) {
              valueRangeOfPhi->SetLower(Bound(nullptr, constantOfNewLower, pType));
              isAccurate = true;
            }
          }
        }
        if (loop.IsCanonicalAndOnlyHasOneExitBBLoop() && isAccurate) {
          valueRangeOfPhi->SetAccurate(true);
        }
      }
    }
    Insert2Caches(bb.GetBBId(), mePhiNode->GetLHS()->GetExprID(), std::move(valueRangeOfPhi));
    onlyRecordValueRangeInTempCache.pop();
  }
}

bool ValueRangePropagation::Insert2Caches(BBId bbID, int32 exprID, std::unique_ptr<ValueRange> valueRange) {
  if (valueRange == nullptr) {
    if (onlyRecordValueRangeInTempCache.top()) {
      tempCaches[bbID].insert(std::make_pair(exprID, nullptr));
    } else {
      caches.at(bbID)[exprID] = nullptr;
    }
    return true;
  }
  if (valueRange->IsConstant() && valueRange->GetRangeType() == kLowerAndUpper) {
    valueRange->SetRangeType(kEqual);
    valueRange->SetBound(valueRange->GetLower());
  }
  if (onlyRecordValueRangeInTempCache.top()) {
    tempCaches[bbID].insert(std::make_pair(exprID, std::move(valueRange)));
  } else {
    caches.at(bbID)[exprID] = std::move(valueRange);
  }
  return true;
}

// The rangeType of vrOfRHS is kEqual and the rangeType of vrOfLHS is kEqual, kNotEqual or kLowerAndUpper
void ValueRangePropagation::JudgeEqual(MeExpr &expr, ValueRange &vrOfLHS, ValueRange &vrOfRHS,
    std::unique_ptr<ValueRange> &valueRangePtr) {
  if (vrOfRHS.GetRangeType() != kEqual) {
    return;
  }
  auto primType = static_cast<OpMeExpr&>(expr).GetOpndType();
  auto resPType = static_cast<OpMeExpr&>(expr).GetPrimType();
  auto op = expr.GetOp();
  if (vrOfLHS.GetRangeType() == kEqual) {
    if (vrOfRHS.GetBound().IsEqual(vrOfLHS.GetBound(), primType)) {
      // dealwith this case:
      // a = (b == c)
      // valueRange(b): kEqual 1
      // valueRange(c): kEqual 1
      // ==>
      // valueRange(a): kNotEqual 0
      valueRangePtr = (op == OP_eq) ? CreateValueRangeOfNotEqualZero(resPType) : CreateValueRangeOfEqualZero(resPType);
    } else {
      // dealwith this case:
      // a = (b == c)
      // valueRange(b): kEqual 1
      // valueRange(c): kEqual 2
      // ==>
      // valueRange(a): kEqual 0
      valueRangePtr = (op == OP_eq) ? CreateValueRangeOfEqualZero(resPType) : CreateValueRangeOfNotEqualZero(resPType);
    }
  } else if (vrOfLHS.GetRangeType() == kNotEqual) {
    if (vrOfRHS.GetBound().IsEqual(vrOfLHS.GetBound(), primType)) {
      // dealwith this case:
      // a = (b == c)
      // valueRange(b): kNotEqual 1
      // valueRange(c): kEqual 1
      // ==>
      // valueRange(a): kEqual 0
      valueRangePtr = (op == OP_eq) ? CreateValueRangeOfEqualZero(resPType) : CreateValueRangeOfNotEqualZero(resPType);
    }
  } else if (vrOfLHS.GetRangeType() == kLowerAndUpper) {
    if (!vrOfLHS.GetLower().IsGreaterThan(vrOfLHS.GetUpper(), primType) &&
        (vrOfLHS.GetLower().IsGreaterThan(vrOfRHS.GetBound(), primType) ||
        vrOfRHS.GetBound().IsGreaterThan(vrOfLHS.GetUpper(), primType))) {
      // dealwith this case:
      // a = (b == c)
      // valueRange(b): kLowerAndUpper (1, Max)
      // valueRange(c): kEqual 0
      // ==>
      // valueRange(a): kEqual 0
      valueRangePtr = (op == OP_eq) ? CreateValueRangeOfEqualZero(resPType) : CreateValueRangeOfNotEqualZero(resPType);
    }
  }
}

std::unique_ptr<ValueRange> ValueRangePropagation::NegValueRange(const BB &bb, MeExpr &opnd) {
  auto *valueRange = FindValueRange(bb, opnd);
  if (valueRange == nullptr) {
    return nullptr;
  }
  auto constantZero = CreateValueRangeOfEqualZero(opnd.GetPrimType());
  auto valueRangePtr = AddOrSubWithValueRange(OP_sub, *constantZero, *valueRange);
  if (valueRangePtr == nullptr) {
    return nullptr;
  }
  auto lower = valueRangePtr->GetLower();
  valueRangePtr->SetLower(valueRangePtr->GetUpper());
  valueRangePtr->SetUpper(lower);
  return valueRangePtr;
}

ValueRange *ValueRangePropagation::FindValueRangeWithCompareOp(const BB &bb, MeExpr &expr) {
  auto op = expr.GetOp();
  if (op == OP_neg) {
    auto *opnd = expr.GetOpnd(0);
    auto valueRangePtr = NegValueRange(bb, *opnd);
    auto *resValueRange = valueRangePtr.get();
    (void)Insert2Caches(bb.GetBBId(), expr.GetExprID(), std::move(valueRangePtr));
    return resValueRange;
  }
  if (!IsCompareHasReverseOp(op)) {
    return nullptr;
  }
  if (expr.GetNumOpnds() != kNumOperands) {
    return nullptr;
  }
  auto *opnd0 = expr.GetOpnd(0);
  auto *opnd1 = expr.GetOpnd(1);
  auto *valueRangeOfOpnd0 = FindValueRange(bb, *opnd0);
  if (valueRangeOfOpnd0 == nullptr) {
    return nullptr;
  }
  auto *valueRangeOfOpnd1 = FindValueRange(bb, *opnd1);
  if (valueRangeOfOpnd1 == nullptr) {
    return nullptr;
  }
  if (!valueRangeOfOpnd0->IsConstantRange() || !valueRangeOfOpnd1->IsConstantRange()) {
    return nullptr;
  }
  auto rangeTypeOfOpnd0 = valueRangeOfOpnd0->GetRangeType();
  auto rangeTypeOfOpnd1 = valueRangeOfOpnd1->GetRangeType();
  std::unique_ptr<ValueRange> valueRangePtr;
  ValueRange *resValueRange = nullptr;
  auto resPType = static_cast<OpMeExpr&>(expr).GetPrimType();
  auto opndType = static_cast<OpMeExpr&>(expr).GetOpndType();
  // Deal with op ne and eq.
  if ((op == OP_eq || op == OP_ne)) {
    if (rangeTypeOfOpnd1 == kEqual) {
      JudgeEqual(expr, *valueRangeOfOpnd0, *valueRangeOfOpnd1, valueRangePtr);
    } else if (rangeTypeOfOpnd0 == kEqual) {
      JudgeEqual(expr, *valueRangeOfOpnd1, *valueRangeOfOpnd0, valueRangePtr);
    } else if (rangeTypeOfOpnd0 == kLowerAndUpper && rangeTypeOfOpnd1 == kLowerAndUpper &&
               (BrStmtInRange(bb, *valueRangeOfOpnd0, *valueRangeOfOpnd1, OP_lt, opndType) ||
                BrStmtInRange(bb, *valueRangeOfOpnd0, *valueRangeOfOpnd1, OP_gt, opndType))) {
      valueRangePtr = (op == OP_eq) ? CreateValueRangeOfEqualZero(resPType) : CreateValueRangeOfNotEqualZero(resPType);
    }
  }
  // Deal with op gt, ge, lt and le.
  if ((rangeTypeOfOpnd0 == kEqual || rangeTypeOfOpnd0 == kLowerAndUpper) &&
      (rangeTypeOfOpnd1 == kEqual || rangeTypeOfOpnd1 == kLowerAndUpper)) {
    if (BrStmtInRange(bb, *valueRangeOfOpnd0, *valueRangeOfOpnd1, op, opndType)) {
      valueRangePtr = CreateValueRangeOfNotEqualZero(resPType);
    } else if (BrStmtInRange(bb, *valueRangeOfOpnd0, *valueRangeOfOpnd1, GetReverseCmpOp(op), opndType)) {
      valueRangePtr = CreateValueRangeOfEqualZero(resPType);
    }
  }
  if (valueRangePtr != nullptr) {
    resValueRange = valueRangePtr.get();
    (void)Insert2Caches(bb.GetBBId(), expr.GetExprID(), std::move(valueRangePtr));
  }
  return resValueRange;
}

// When the valueRange of expr is not exist, need find the valueRange of the def point or use points.
ValueRange *ValueRangePropagation::FindValueRange(const BB &bb, MeExpr &expr) {
  auto *valueRange = FindValueRangeInCaches(bb.GetBBId(), expr.GetExprID());
  if (valueRange != nullptr) {
    return valueRange;
  }
  valueRange = FindValueRangeWithCompareOp(bb, expr);
  if (valueRange != nullptr) {
    return valueRange;
  }
  auto it = pairOfExprs.find(&expr);
  if (it == pairOfExprs.end()) {
    return nullptr;
  }
  for (auto itOfValueMap = it->second.begin(); itOfValueMap != it->second.end(); ++itOfValueMap) {
    auto bbOfPair = itOfValueMap->first;
    auto exprs = itOfValueMap->second;
    if (!dom.Dominate(*bbOfPair, bb)) {
      continue;
    }
    for (auto itOfExprs = exprs.begin(); itOfExprs != exprs.end(); ++itOfExprs) {
      valueRange = FindValueRangeInCaches(bb.GetBBId(), (*itOfExprs)->GetExprID());
      if (valueRange != nullptr) {
        return valueRange;
      }
      valueRange = FindValueRangeWithCompareOp(bb, **itOfExprs);
      if (valueRange != nullptr) {
        return valueRange;
      }
    }
  }
  return nullptr;
}

std::unique_ptr<ValueRange> ValueRangePropagation::CreateInitVRForPhi(LoopDesc &loop,
    const BB &bb, ScalarMeExpr &init, ScalarMeExpr &backedge, const ScalarMeExpr &lhsOfPhi) {
  Bound initBound;
  ValueRange *valueRangeOfInit = FindValueRange(bb, init);
  bool initIsConstant = false;
  if (valueRangeOfInit != nullptr && valueRangeOfInit->IsConstant() && valueRangeOfInit->GetRangeType() != kNotEqual) {
    initIsConstant = true;
    auto bound = valueRangeOfInit->GetBound();
    initBound = Bound(GetRealValue(bound.GetConstant(), bound.GetPrimType()), bound.GetPrimType());
  } else {
    initBound = Bound(&init, init.GetPrimType());
  }
  int64 stride = 0;
  if (!CanComputeLoopIndVar(lhsOfPhi, backedge, stride) || stride == 0) {
    if (valueRangeOfInit == nullptr || !valueRangeOfInit->IsConstant()) {
      return nullptr;
    }
    if (loop.inloopBB2exitBBs.size() != 1) {
      return nullptr;
    }
    auto *exitBB = func.GetCfg()->GetBBFromID(loop.inloopBB2exitBBs.begin()->first);
    if (exitBB->GetKind() != kBBCondGoto) {
      return nullptr;
    }
    auto *brMeStmt = static_cast<CondGotoMeStmt*>(exitBB->GetLastMe());
    auto *opMeExpr = static_cast<OpMeExpr*>(brMeStmt->GetOpnd());
    MeExpr *opnd0 = opMeExpr->GetOpnd(0);
    MeExpr *opnd1 = opMeExpr->GetOpnd(1);
    BB *trueBranch = nullptr;
    BB *falseBranch = nullptr;
    GetTrueAndFalseBranch(brMeStmt->GetOp(), *exitBB, trueBranch, falseBranch);
    if (opnd0 != &backedge) {
      return nullptr;
    }
    if (opMeExpr->GetOp() != OP_ne && opMeExpr->GetOp() != OP_eq) {
      return nullptr;
    }
    int64 constantValue = 0;
    if (IsConstant(*exitBB, *opnd1, constantValue) && valueRangeOfInit != nullptr &&
        valueRangeOfInit->GetBound().GetConstant() == constantValue) {
      RangeType initRangeType = valueRangeOfInit->GetRangeType();
      std::unique_ptr<ValueRange> vr;
      if ((opMeExpr->GetOp() == OP_ne && loop.Has(*trueBranch) && initRangeType == kNotEqual) ||
          (opMeExpr->GetOp() == OP_eq && loop.Has(*falseBranch) && initRangeType == kNotEqual)) {
        vr = std::make_unique<ValueRange>(Bound(constantValue, opMeExpr->GetOpndType()), kNotEqual);

      } else if ((opMeExpr->GetOp() == OP_ne && loop.Has(*falseBranch) && initRangeType == kEqual) ||
                 (opMeExpr->GetOp() == OP_eq && loop.Has(*trueBranch) && initRangeType == kEqual)) {
        vr = std::make_unique<ValueRange>(Bound(constantValue, opMeExpr->GetOpndType()), kEqual);
      } else {
        vr = nullptr;
      }
      Insert2Caches(bb.GetBBId(), lhsOfPhi.GetExprID(), std::move(vr));
    }
    return nullptr;
  }
  auto valueRangeForPhi = MakeMonotonicIncreaseOrDecreaseValueRangeForPhi(stride, initBound);
  if (valueRangeForPhi == nullptr) {
    return nullptr;
  }
  if (Insert2Caches(bb.GetBBId(), lhsOfPhi.GetExprID(), CopyValueRange(*valueRangeForPhi, init.GetPrimType()))) {
    return valueRangeForPhi;
  }
  return nullptr;
}

// Calculate the valuerange of the operands according to the mePhiNode.
void ValueRangePropagation::DealWithPhi(const BB &bb) {
  if (bb.GetMePhiList().empty()) {
    return;
  }
  // If the bb is not in loop or the bb is not the head of loop, merge the value range of phi operands.
  if (bb.GetPred().size() == 1 || loops == nullptr || loops->GetMeLoops().size() == 0 ||
      loops->GetBBLoopParent(bb.GetBBId()) == nullptr || loops->GetBBLoopParent(bb.GetBBId())->head != &bb) {
    for (auto &pair : bb.GetMePhiList()) {
      auto *mePhiNode = pair.second;
      std::unique_ptr<ValueRange> valueRangeOfPhi = MergeValueRangeOfPhiOperands(bb, *mePhiNode);
      if (valueRangeOfPhi != nullptr) {
        (void)Insert2Caches(bb.GetBBId(), mePhiNode->GetLHS()->GetExprID(), std::move(valueRangeOfPhi));
      }
    }
    return;
  }
  auto *loop = loops->GetBBLoopParent(bb.GetBBId());
  std::set<ScalarMeExpr*> initExprsOfPhi;
  std::set<ScalarMeExpr*> backExprOfPhi;
  size_t indexOfInitExpr = -1;
  std::vector<std::unique_ptr<ValueRange>> valueRangeOfInitExprs;
  auto index = 0;
  // Collect the initial value and the back edge value of phi node.
  for (auto &pair : bb.GetMePhiList()) {
    if (!pair.second->GetIsLive()) {
      continue;
    }
    if (pair.second->GetOpnds().size() != kNumOperands) {
      return;
    }
    for (size_t i = 0; i < pair.second->GetOpnds().size(); ++i) {
      MeStmt *defStmt = nullptr;
      auto *opnd = pair.second->GetOpnd(i);
      BB *defBB = opnd->GetDefByBBMeStmt(dom, defStmt);
      if (!loop->Has(*defBB)) {
        initExprsOfPhi.insert(opnd);
        if (indexOfInitExpr == -1) {
          indexOfInitExpr = i;
        }
      } else {
        backExprOfPhi.insert(opnd);
      }
    }
    if (backExprOfPhi.size() == 0) {
      valueRangeOfInitExprs.push_back(nullptr);
    } else if (initExprsOfPhi.size() == 1 && backExprOfPhi.size() == 1) { // Create vr for initial value of phi node.
      auto vrOfInitExpr = CreateInitVRForPhi(
          *loop, bb, **initExprsOfPhi.begin(), **backExprOfPhi.begin(), *pair.second->GetLHS());
      valueRangeOfInitExprs.push_back(std::move(vrOfInitExpr));
    } else {
      valueRangeOfInitExprs.push_back(nullptr);
    }
    initExprsOfPhi.clear();
    backExprOfPhi.clear();
    index++;
  }
  // When the number of nested layers of the loop is too deep, do not continue to calculate vr of phi nodes.
  if (loop->nestDepth > kLimitOfLoopNestDepth || loop->loopBBs.size() > kLimitOfLoopBBs) {
    return;
  }
  onlyRecordValueRangeInTempCache.push(true);
  CreateVRForPhi(*loop, bb);
  MergeValueRangeOfPhiOperands(*loop, bb, valueRangeOfInitExprs, indexOfInitExpr);
  onlyRecordValueRangeInTempCache.pop();
  tempCaches.clear();
}

// Return the max of leftBound or rightBound.
Bound ValueRangePropagation::Max(Bound leftBound, Bound rightBound) {
  if (leftBound.GetVar() == rightBound.GetVar()) {
    return (leftBound.GetConstant() > rightBound.GetConstant()) ? leftBound : rightBound;
  } else {
    if (leftBound.GetVar() == nullptr && leftBound.IsEqualToMin(leftBound.GetPrimType()) &&
        rightBound.GetConstant() < 1 && lengthSet.find(rightBound.GetVar()) != lengthSet.end()) {
      return rightBound;
    }
    if (rightBound.GetVar() == nullptr && rightBound.IsEqualToMin(rightBound.GetPrimType()) &&
        leftBound.GetConstant() < 1 && lengthSet.find(leftBound.GetVar()) != lengthSet.end()) {
      return leftBound;
    }
  }
  return leftBound;
}

// Return the min of leftBound or rightBound.
Bound ValueRangePropagation::Min(Bound leftBound, Bound rightBound) {
  if (leftBound.GetVar() == rightBound.GetVar()) {
    return (leftBound.GetConstant() < rightBound.GetConstant()) ? leftBound : rightBound;
  } else {
    if (leftBound.GetVar() == nullptr && leftBound.IsEqualToMax(leftBound.GetPrimType()) &&
        rightBound.GetConstant() < 1 && lengthSet.find(rightBound.GetVar()) != lengthSet.end()) {
      return rightBound;
    }
    if (rightBound.GetVar() == nullptr && rightBound.IsEqualToMax(rightBound.GetPrimType()) &&
        leftBound.GetConstant() < 1 && lengthSet.find(leftBound.GetVar()) != lengthSet.end()) {
      return leftBound;
    }
  }
  return leftBound;
}

// Judge whether the lower is in range.
bool ValueRangePropagation::LowerInRange(const BB &bb, Bound lowerTemp, Bound lower, bool lowerIsZero) {
  if ((lowerIsZero && lowerTemp.GetVar() == nullptr) ||
      (!lowerIsZero && lowerTemp.GetVar() == lower.GetVar())) {
    int64 lowerConstant = lowerIsZero ? 0 : lower.GetConstant();
    return (lowerTemp.GetConstant() >= lowerConstant);
  } else {
    if (lowerTemp.GetVar() != nullptr) {
      auto *lowerVar = lowerTemp.GetVar();
      auto *lowerRangeValue = FindValueRange(bb, *lowerVar);
      if (lowerRangeValue != nullptr && lowerRangeValue->IfLowerEqualToUpper() &&
          lowerRangeValue->GetLower().GetVar() != lowerVar) {
        if (lowerTemp.GetConstant() == 0) {
          return LowerInRange(bb, lowerRangeValue->GetLower(), lower, lowerIsZero);
        } else {
          Bound newLower;
          if (CreateNewBoundWhenAddOrSub(OP_add, lowerRangeValue->GetBound(),
                                         lowerTemp.GetConstant(), newLower)) {
            return LowerInRange(bb, newLower, lower, lowerIsZero);
          }
        }
      }
    }
  }
  return false;
}

// Judge whether the upper is in range.
bool ValueRangePropagation::UpperInRange(const BB &bb, Bound upperTemp, Bound upper, bool upperIsArrayLength) {
  if (upperTemp.GetVar() == upper.GetVar()) {
    return upperIsArrayLength ? (upperTemp.GetConstant() < upper.GetConstant()) :
        (upperTemp.GetConstant() <= upper.GetConstant());
  }
  if (upperTemp.GetVar() == nullptr) {
    return false;
  }
  auto *upperVar = upperTemp.GetVar();
  auto *upperRangeValue = FindValueRange(bb, *upperVar);
  if (upperRangeValue == nullptr) {
    auto *currVar = upperVar;
    while (length2Def.find(currVar) != length2Def.end()) {
      currVar = length2Def[currVar];
      if (currVar == upper.GetVar()) {
        Bound newUpper;
        if (CreateNewBoundWhenAddOrSub(
            OP_add, upper, upperTemp.GetConstant(), newUpper)) {
          return UpperInRange(bb, newUpper, upper, upperIsArrayLength);
        } else {
          return false;
        }
      }
    }
  } else if (upperRangeValue->IfLowerEqualToUpper() &&
             upperRangeValue->GetUpper().GetVar() != upperVar) {
    if (upperTemp.GetConstant() == 0) {
      return UpperInRange(bb, upperRangeValue->GetUpper(), upper, upperIsArrayLength);
    } else {
      Bound newUpper;
      if (CreateNewBoundWhenAddOrSub(OP_add, upperRangeValue->GetBound(),
                                     upperTemp.GetConstant(), newUpper)) {
        return UpperInRange(bb, newUpper, upper, upperIsArrayLength);
      } else {
        return false;
      }
    }
  } else if (!upperRangeValue->IfLowerEqualToUpper()) {
    if (length2Def.find(upperVar) != length2Def.end() && length2Def[upperVar] == upper.GetVar()) {
      return upperIsArrayLength ? (upperTemp.GetConstant() < upper.GetConstant()) :
          (upperTemp.GetConstant() <= upper.GetConstant());
    }
  }
  return false;
}

// Judge whether the lower and upper are in range.
InRangeType ValueRangePropagation::InRange(const BB &bb, const ValueRange &rangeTemp,
                                           const ValueRange &range, bool lowerIsZero) {
  bool lowerInRange = LowerInRange(bb, rangeTemp.GetLower(), range.GetLower(), lowerIsZero);
  Bound upperBound;
  if (lowerIsZero && !range.IfLowerEqualToUpper()) {
    upperBound = range.GetLower();
  } else {
    upperBound = range.GetUpper();
  }
  bool upperInRange = UpperInRange(bb, rangeTemp.GetUpper(), upperBound, lowerIsZero);
  if (lowerInRange && upperInRange) {
    return kInRange;
  } else if (lowerInRange) {
    return kLowerInRange;
  } else if (upperInRange) {
    return kUpperInRange;
  } else {
    return kNotInRange;
  }
}

std::unique_ptr<ValueRange> ValueRangePropagation::CombineTwoValueRange(
    const ValueRange &leftRange, const ValueRange &rightRange, bool merge) {
  if (merge) {
    return std::make_unique<ValueRange>(Min(leftRange.GetLower(), rightRange.GetLower()),
                                        Max(leftRange.GetUpper(), rightRange.GetUpper()), kLowerAndUpper);
  } else {
    if (rightRange.GetRangeType() == kOnlyHasLowerBound) {
      return std::make_unique<ValueRange>(rightRange.GetLower(),Max(leftRange.GetUpper(),
          Bound(GetMaxNumber(rightRange.GetUpper().GetPrimType()), rightRange.GetUpper().GetPrimType())),
              kLowerAndUpper);
    }
    if (rightRange.GetRangeType() == kOnlyHasUpperBound) {
      return std::make_unique<ValueRange>(Min(leftRange.GetLower(),
          Bound(GetMinNumber(rightRange.GetLower().GetPrimType()), rightRange.GetLower().GetPrimType())),
              leftRange.GetUpper(), kLowerAndUpper);
    }
    return std::make_unique<ValueRange>(Max(leftRange.GetLower(), rightRange.GetLower()),
                                        Min(leftRange.GetUpper(), rightRange.GetUpper()), kLowerAndUpper);
  }
}

// When delete the exit bb of loop and delete the condgoto stmt,
// the loop would not exit, so need change bb to gotobb for the f.GetCfg()->WontExitAnalysis().
void ValueRangePropagation::ChangeLoop2Goto(LoopDesc &loop, BB &bb, BB &succBB, const BB &unreachableBB) {
  auto it = loop.inloopBB2exitBBs.find(bb.GetBBId());
  bool theLoopWontExit = false;
  if (it != loop.inloopBB2exitBBs.end()) {
    for (auto *exitedBB : *it->second) {
      if (exitedBB == &unreachableBB) {
        theLoopWontExit = true;
        break;
      }
    }
  }
  if (!theLoopWontExit) {
    return;
  }
  auto *gotoMeStmt = irMap.New<GotoMeStmt>(func.GetOrCreateBBLabel(succBB));
  bb.AddMeStmtLast(gotoMeStmt);
  bb.SetKind(kBBGoto);
}

void ValueRangePropagation::Insert2UnreachableBBs(BB &unreachableBB) {
  if (ValueRangePropagation::isDebug) {
    LogInfo::MapleLogger() << "=====================delete bb " << unreachableBB.GetBBId() <<
        "========================\n";
  }
  unreachableBBs.insert(&unreachableBB);
  return;
}

// when determine remove the condgoto stmt, need analysis which bbs need be deleted.
void ValueRangePropagation::AnalysisUnreachableBBOrEdge(BB &bb, BB &unreachableBB, BB &succBB) {
  if (onlyPropVR) {
    return;
  }
  Insert2UnreachableBBs(unreachableBB);
  bb.RemoveSucc(unreachableBB);
  bb.RemoveMeStmt(bb.GetLastMe());
  bb.SetKind(kBBFallthru);
  auto *loop = loops->GetBBLoopParent(bb.GetBBId());
  if (loop == nullptr) {
    return;
  }
  ChangeLoop2Goto(*loop, bb, succBB, unreachableBB);
}

// Judge whether the value range is in range.
bool ValueRangePropagation::BrStmtInRange(const BB &bb, const ValueRange &leftRange,
                                          const ValueRange &rightRange, Opcode op, PrimType opndType,
                                          bool judgeNotInRange) {
  Bound leftLower = leftRange.GetLower();
  Bound leftUpper = leftRange.GetUpper();
  Bound rightLower = rightRange.GetLower();
  Bound rightUpper = rightRange.GetUpper();
  RangeType leftRangeType = leftRange.GetRangeType();
  RangeType rightRangeType = rightRange.GetRangeType();
  if (!IsNeededPrimType(opndType) ||
      leftLower.GetVar() != leftUpper.GetVar() || leftUpper.GetVar() != rightUpper.GetVar() ||
      rightUpper.GetVar() != rightLower.GetVar() || leftRangeType == kSpecialLowerForLoop ||
      leftRangeType == kSpecialUpperForLoop ||
      leftRange.GetRangeType() == kOnlyHasLowerBound ||
      leftRange.GetRangeType() == kOnlyHasUpperBound) {
    return false;
  }
  if (judgeNotInRange) {
    if (leftLower.GetVar() != nullptr) {
      return false;
    }
  } else {
    if (leftLower.GetVar() != nullptr) {
      auto *valueRange = FindValueRange(bb, *leftLower.GetVar());
      if (valueRange == nullptr || !valueRange->IsConstant()) {
        return false;
      }
    }
  }
  // deal the difference between i32 and u32.
  if (leftLower.IsGreaterThan(leftUpper, opndType) ||
      rightLower.IsGreaterThan(rightUpper, opndType)) {
    return false;
  }
  if ((op == OP_eq && !judgeNotInRange) || (op == OP_ne && judgeNotInRange)) {
    // If the range of leftOpnd equal to the range of rightOpnd, remove the falseBranch.
    return leftRangeType != kNotEqual && rightRangeType != kNotEqual &&
        leftLower.IsEqual(leftUpper, opndType) &&
        rightLower.IsEqual(rightUpper, opndType) &&
        leftLower.IsEqual(rightLower, opndType);
  } else if ((op == OP_ne && !judgeNotInRange) || (op == OP_eq && judgeNotInRange)) {
    // If the range type of leftOpnd is kLowerAndUpper and the rightRange is not in range of it,
    // remove the trueBranch, such as :
    // brstmt a == b
    // valueRange a: [1, max]
    // valueRange b: 0
    return (leftRangeType != kNotEqual && rightRangeType != kNotEqual &&
        (leftLower.IsGreaterThan(rightUpper, opndType) || rightLower.IsGreaterThan(leftUpper, opndType))) ||
        (leftRangeType == kNotEqual && rightRangeType == kEqual && leftLower.IsEqual(rightLower, opndType)) ||
        (leftRangeType == kEqual && rightRangeType == kNotEqual && leftLower.IsEqual(rightLower, opndType));
  } else if ((op == OP_lt && !judgeNotInRange) || (op == OP_ge && judgeNotInRange)) {
    return leftRangeType != kNotEqual && rightRangeType != kNotEqual && rightLower.IsGreaterThan(leftUpper, opndType);
  } else if ((op == OP_ge && !judgeNotInRange) || (op == OP_lt && judgeNotInRange)) {
    return leftRangeType != kNotEqual && rightRangeType != kNotEqual && !rightUpper.IsGreaterThan(leftLower, opndType);
  } else if ((op == OP_le && !judgeNotInRange) || (op == OP_gt && judgeNotInRange)) {
    return leftRangeType != kNotEqual && rightRangeType != kNotEqual && !leftUpper.IsGreaterThan(rightLower, opndType);
  } else if ((op == OP_gt && !judgeNotInRange) || (op == OP_le && judgeNotInRange)) {
    return leftRangeType != kNotEqual && rightRangeType != kNotEqual && leftLower.IsGreaterThan(rightUpper, opndType);
  }
  return false;
}

void ValueRangePropagation::GetTrueAndFalseBranch(Opcode op, BB &bb, BB *&trueBranch, BB *&falseBranch) const {
  if (op == OP_brtrue) {
    trueBranch = bb.GetSucc(1);
    falseBranch = bb.GetSucc(0);
  } else {
    trueBranch = bb.GetSucc(0);
    falseBranch = bb.GetSucc(1);
  }
}

void ValueRangePropagation::CreateValueRangeForLeOrLt(
    const MeExpr &opnd, const ValueRange *leftRange, Bound newRightUpper, Bound newRightLower,
    const BB &trueBranch, const BB &falseBranch) {
  CHECK_FATAL(IsEqualPrimType(newRightUpper.GetPrimType(), newRightLower.GetPrimType()), "must be equal");
  if (leftRange == nullptr) {
    std::unique_ptr<ValueRange> newTrueBranchRange =
        std::make_unique<ValueRange>(ValueRange::MinBound(newRightUpper.GetPrimType()), newRightUpper, kLowerAndUpper);
    (void)Insert2Caches(trueBranch.GetBBId(), opnd.GetExprID(), std::move(newTrueBranchRange));
    std::unique_ptr<ValueRange> newFalseBranchRange =
        std::make_unique<ValueRange>(newRightLower, ValueRange::MaxBound(newRightUpper.GetPrimType()), kLowerAndUpper);
    (void)Insert2Caches(falseBranch.GetBBId(), opnd.GetExprID(), std::move(newFalseBranchRange));
  } else if (leftRange->GetRangeType() == kOnlyHasLowerBound) {
    std::unique_ptr<ValueRange> newRightRange = std::make_unique<ValueRange>(
        ValueRange::MinBound(newRightUpper.GetPrimType()), newRightUpper, kLowerAndUpper);
    std::unique_ptr<ValueRange> newLeftRange = std::make_unique<ValueRange>(leftRange->GetLower(),
        ValueRange::MaxBound(leftRange->GetLower().GetPrimType()), kLowerAndUpper);
    (void)Insert2Caches(trueBranch.GetBBId(), opnd.GetExprID(),
                        CombineTwoValueRange(*leftRange, *newRightRange));
    newRightRange = std::make_unique<ValueRange>(
        newRightLower, ValueRange::MaxBound(newRightUpper.GetPrimType()), kLowerAndUpper);
  } else if (leftRange->GetRangeType() == kOnlyHasUpperBound) {
    std::unique_ptr<ValueRange> newRightRange = std::make_unique<ValueRange>(
        ValueRange::MinBound(newRightUpper.GetPrimType()), newRightUpper, kLowerAndUpper);
    std::unique_ptr<ValueRange> newLeftRange = std::make_unique<ValueRange>(
        ValueRange::MinBound(newRightUpper.GetPrimType()), leftRange->GetBound(), kLowerAndUpper);
    newRightRange = std::make_unique<ValueRange>(
        newRightLower, ValueRange::MaxBound(newRightUpper.GetPrimType()), kLowerAndUpper);
    (void)Insert2Caches(falseBranch.GetBBId(), opnd.GetExprID(),
                        CombineTwoValueRange(*leftRange, *newRightRange));
  } else {
    std::unique_ptr<ValueRange> newRightRange = std::make_unique<ValueRange>(
        ValueRange::MinBound(newRightUpper.GetPrimType()), newRightUpper, kLowerAndUpper);
    (void)Insert2Caches(trueBranch.GetBBId(), opnd.GetExprID(),
                        CombineTwoValueRange(*leftRange, *newRightRange));
    newRightRange = std::make_unique<ValueRange>(
        newRightLower, ValueRange::MaxBound(newRightUpper.GetPrimType()), kLowerAndUpper);
    (void)Insert2Caches(falseBranch.GetBBId(), opnd.GetExprID(),
        CombineTwoValueRange(*leftRange, *newRightRange));
  }
}

void ValueRangePropagation::CreateValueRangeForGeOrGt(
    const MeExpr &opnd, const ValueRange *leftRange, Bound newRightUpper, Bound newRightLower,
    const BB &trueBranch, const BB &falseBranch) {
  CHECK_FATAL(IsEqualPrimType(newRightUpper.GetPrimType(), newRightLower.GetPrimType()), "must be equal");
  if (leftRange == nullptr) {
    std::unique_ptr<ValueRange> newTrueBranchRange =
        std::make_unique<ValueRange>(newRightLower, ValueRange::MaxBound(newRightUpper.GetPrimType()), kLowerAndUpper);
    (void)Insert2Caches(trueBranch.GetBBId(), opnd.GetExprID(), std::move(newTrueBranchRange));
    std::unique_ptr<ValueRange> newFalseBranchRange =
        std::make_unique<ValueRange>(ValueRange::MinBound(newRightUpper.GetPrimType()), newRightUpper, kLowerAndUpper);
    (void)Insert2Caches(falseBranch.GetBBId(), opnd.GetExprID(), std::move(newFalseBranchRange));
  } else if (leftRange->GetRangeType() == kOnlyHasLowerBound) {
    auto newRightRange = std::make_unique<ValueRange>(
        ValueRange::MinBound(newRightUpper.GetPrimType()), newRightUpper, kLowerAndUpper);
    (void)Insert2Caches(falseBranch.GetBBId(), opnd.GetExprID(),
                        CombineTwoValueRange(*leftRange, *newRightRange));
  } else if (leftRange->GetRangeType() == kOnlyHasUpperBound) {
    std::unique_ptr<ValueRange> newRightRange =
        std::make_unique<ValueRange>(newRightLower, ValueRange::MaxBound(newRightUpper.GetPrimType()), kLowerAndUpper);
    (void)Insert2Caches(trueBranch.GetBBId(), opnd.GetExprID(),
                        CombineTwoValueRange(*leftRange, *newRightRange));
  } else {
    std::unique_ptr<ValueRange> newRightRange =
        std::make_unique<ValueRange>(newRightLower, ValueRange::MaxBound(newRightUpper.GetPrimType()), kLowerAndUpper);
    (void)Insert2Caches(trueBranch.GetBBId(), opnd.GetExprID(), CombineTwoValueRange(*leftRange, *newRightRange));
    newRightRange = std::make_unique<ValueRange>(
        ValueRange::MinBound(newRightUpper.GetPrimType()), newRightUpper, kLowerAndUpper);
    (void)Insert2Caches(falseBranch.GetBBId(), opnd.GetExprID(),
                        CombineTwoValueRange(*leftRange, *newRightRange));
  }
}

// Return true if the lower or upper of leftRange is equal to the bound of rightRange, like this:
// leftRang: (5, constant) rightRange: (5) ==> return true;
// leftRang: (constant, 5) rightRange: (5) ==> return true;
bool ValueRangePropagation::IfTheLowerOrUpperOfLeftRangeEqualToTheRightRange(
    const ValueRange &leftRange, ValueRange &rightRange, bool isLower) const {
  bool lowerOrUpperIsEqual = isLower ? leftRange.GetLower().GetConstant() == rightRange.GetBound().GetConstant() :
      leftRange.GetUpper().GetConstant() == rightRange.GetBound().GetConstant();
  return leftRange.GetRangeType() == kLowerAndUpper && leftRange.IsConstantLowerAndUpper() &&
         leftRange.GetLower().GetConstant() < leftRange.GetUpper().GetConstant() && lowerOrUpperIsEqual;
}

void ValueRangePropagation::CreateValueRangeForNeOrEq(
    const MeExpr &opnd, const ValueRange *leftRange, ValueRange &rightRange, const BB &trueBranch,
    const BB &falseBranch) {
  if (rightRange.GetRangeType() == kEqual) {
    std::unique_ptr<ValueRange> newTrueBranchRange =
        std::make_unique<ValueRange>(rightRange.GetBound(), kEqual);
    (void)Insert2Caches(trueBranch.GetBBId(), opnd.GetExprID(), std::move(newTrueBranchRange));
    if (leftRange == nullptr) {
      std::unique_ptr<ValueRange> newFalseBranchRange =
          std::make_unique<ValueRange>(rightRange.GetBound(), kNotEqual);
      (void)Insert2Caches(falseBranch.GetBBId(), opnd.GetExprID(), std::move(newFalseBranchRange));
    } else if (IfTheLowerOrUpperOfLeftRangeEqualToTheRightRange(*leftRange, rightRange, true)) {
      // Deal with this case:
      // leftRang: (5, constant)
      // rightRange: (5)
      // ==>
      // leftRang: (6, constant)
      Bound newLower;
      if (CreateNewBoundWhenAddOrSub(OP_add, leftRange->GetLower(), 1, newLower)) {
        std::unique_ptr<ValueRange> newFalseBranchRange =
            std::make_unique<ValueRange>(newLower, leftRange->GetUpper(), kLowerAndUpper);
        (void)Insert2Caches(falseBranch.GetBBId(), opnd.GetExprID(), std::move(newFalseBranchRange));
      }
    } else if (IfTheLowerOrUpperOfLeftRangeEqualToTheRightRange(*leftRange, rightRange, false)) {
      // Deal with this case:
      // leftRang: (constant, 5)
      // rightRange: (5)
      // ==>
      // leftRang: (constant, 4)
      Bound newUpper;
      if (CreateNewBoundWhenAddOrSub(OP_sub, leftRange->GetUpper(), 1, newUpper)) {
        std::unique_ptr<ValueRange> newFalseBranchRange =
            std::make_unique<ValueRange>(leftRange->GetLower(), newUpper, kLowerAndUpper);
        (void)Insert2Caches(falseBranch.GetBBId(), opnd.GetExprID(), std::move(newFalseBranchRange));
      }
    } else if (leftRange->GetRangeType() == kLowerAndUpper) {
      std::unique_ptr<ValueRange> newFalseBranchRange =
          std::make_unique<ValueRange>(rightRange.GetBound(), kNotEqual);
      (void)Insert2Caches(falseBranch.GetBBId(), opnd.GetExprID(), std::move(newFalseBranchRange));
    } else if (leftRange->GetRangeType() == kOnlyHasLowerBound || leftRange->GetRangeType() == kOnlyHasUpperBound) {
      std::unique_ptr<ValueRange> newFalseBranchRange =
          std::make_unique<ValueRange>(rightRange.GetBound(), kNotEqual);
      (void)Insert2Caches(falseBranch.GetBBId(), opnd.GetExprID(), std::move(newFalseBranchRange));
    }
  } else if (rightRange.GetRangeType() == kNotEqual) {
    if (leftRange == nullptr) {
      std::unique_ptr<ValueRange> newTrueBranchRange =
          std::make_unique<ValueRange>(rightRange.GetBound(), kNotEqual);
      (void)Insert2Caches(trueBranch.GetBBId(), opnd.GetExprID(), std::move(newTrueBranchRange));
    } else if (IfTheLowerOrUpperOfLeftRangeEqualToTheRightRange(*leftRange, rightRange, true)) {
      Bound newLower;
      if (CreateNewBoundWhenAddOrSub(OP_add, leftRange->GetLower(), 1, newLower)) {
        std::unique_ptr<ValueRange> newTrueBranchRange =
            std::make_unique<ValueRange>(newLower, leftRange->GetUpper(), kLowerAndUpper);
        (void)Insert2Caches(trueBranch.GetBBId(), opnd.GetExprID(), std::move(newTrueBranchRange));
      }
    } else if (IfTheLowerOrUpperOfLeftRangeEqualToTheRightRange(*leftRange, rightRange, false)) {
      Bound newUpper;
      if (CreateNewBoundWhenAddOrSub(OP_sub, leftRange->GetUpper(), 1, newUpper)) {
        std::unique_ptr<ValueRange> newTrueBranchRange =
            std::make_unique<ValueRange>(leftRange->GetLower(), newUpper, kLowerAndUpper);
        (void)Insert2Caches(trueBranch.GetBBId(), opnd.GetExprID(), std::move(newTrueBranchRange));
      }
    } else if (leftRange->GetRangeType() == kOnlyHasLowerBound || leftRange->GetRangeType() == kOnlyHasUpperBound) {
      std::unique_ptr<ValueRange> newTrueBranchRange =
          std::make_unique<ValueRange>(rightRange.GetBound(), kNotEqual);
      (void)Insert2Caches(trueBranch.GetBBId(), opnd.GetExprID(), std::move(newTrueBranchRange));
    }
  }
}

// Deal with the case like this:
// if a == 5
// If the valueRange of a is valuerange(5, kEqual), delete the false branch.
// Else if the valeuRange of a is valueRange(5, kNotEqual), delete the ture branch.
bool ValueRangePropagation::ConditionBBCanBeDeletedAfterOPNeOrEq(
    BB &bb, ValueRange &leftRange, ValueRange &rightRange, BB &falseBranch, BB &trueBranch) {
  if ((leftRange.GetRangeType() == kEqual && rightRange.GetRangeType() == kEqual) &&
      leftRange.GetBound().GetVar() == rightRange.GetBound().GetVar()) {
    if (leftRange.GetBound().GetConstant() == rightRange.GetBound().GetConstant()) {
      AnalysisUnreachableBBOrEdge(bb, falseBranch, trueBranch);
      return true;
    } else {
      AnalysisUnreachableBBOrEdge(bb, trueBranch, falseBranch);
      return true;
    }
  } else if (leftRange.GetRangeType() == kNotEqual && rightRange.GetRangeType() == kEqual &&
             leftRange.GetBound().GetVar() == rightRange.GetBound().GetVar() &&
             leftRange.GetBound().GetConstant() == rightRange.GetBound().GetConstant()) {
    AnalysisUnreachableBBOrEdge(bb, trueBranch, falseBranch);
    return true;
  }
  return false;
}

bool ValueRangePropagation::OnlyHaveCondGotoStmt(BB &bb) const {
  CHECK_FATAL(!bb.GetMeStmts().empty(), "must not be empty");
  MeStmt *stmt = bb.GetFirstMe();
  if (stmt->GetOp() == OP_comment) {
    stmt = stmt->GetNextMeStmt();
  }
  return stmt == bb.GetLastMe();
}

bool ValueRangePropagation::OnlyHaveOneCondGotoPredBB(const BB &bb, const BB &condGotoBB) const {
  return bb.GetPred().size() == 1 && bb.GetPred(0) == &condGotoBB;
}

// If the pred vector of false branch only have one bb, delete the false branch:
//       condGotoBB          condGotoBB
//            |                   |
//           / \     ---->        |
//          /   \                 |
//       false  true             true
// else remove the false branch from the succ vecotr of condGotoBB:
//    bb    condGotoBB         bb   condGotoBB
//      \       |               |       |
//       \     / \     ---->    |       |
//        \   /   \             |       |
//        false  true          false   true
void ValueRangePropagation::RemoveUnreachableBB(
    BB &condGotoBB, BB &trueBranch, ScalarMeExpr *updateSSAExceptTheScalarExpr,
    std::map<OStIdx, std::set<BB*>> &ssaupdateCandsForCondExpr) {
  CHECK_FATAL(condGotoBB.GetSucc().size() == kNumOperands, "must have 2 succ");
  auto *succ0 = condGotoBB.GetSucc(0);
  auto *succ1 = condGotoBB.GetSucc(1);
  if (succ0 == &trueBranch) {
    if (OnlyHaveOneCondGotoPredBB(*succ1, condGotoBB)) {
      AnalysisUnreachableBBOrEdge(condGotoBB, *succ1, trueBranch);
    } else {
      condGotoBB.SetKind(kBBFallthru);
      condGotoBB.RemoveSucc(*succ1);
      DeleteThePhiNodeWhichOnlyHasOneOpnd(*succ1, updateSSAExceptTheScalarExpr, ssaupdateCandsForCondExpr);
      condGotoBB.RemoveMeStmt(condGotoBB.GetLastMe());
    }
  } else {
    if (OnlyHaveOneCondGotoPredBB(*succ0, condGotoBB)) {
      AnalysisUnreachableBBOrEdge(condGotoBB, *succ0, trueBranch);
    } else {
      condGotoBB.SetKind(kBBFallthru);
      condGotoBB.RemoveSucc(*succ0);
      DeleteThePhiNodeWhichOnlyHasOneOpnd(*succ0, updateSSAExceptTheScalarExpr, ssaupdateCandsForCondExpr);
      condGotoBB.RemoveMeStmt(condGotoBB.GetLastMe());
    }
  }
}

BB *ValueRangePropagation::CreateNewGotoBBWithoutCondGotoStmt(BB &bb) {
  if (CodeSizeIsOverflowOrTheOpOfStmtIsNotSupported(bb)) {
    return nullptr;
  }
  BB *newBB = func.GetCfg()->NewBasicBlock();
  ResizeWhenCreateNewBB();
  CopyMeStmts(bb, *newBB);
  /* create at least one gotoBB in the new loop to facilitate WontExitAnalysis */
  newBB->SetKind(kBBGoto);
  return newBB;
}

void ValueRangePropagation::CopyMeStmts(BB &fromBB, BB &toBB) {
  if (fromBB.GetMeStmts().empty()) {
    return;
  }
  /* Do not copy the last stmt of condGoto and Goto BB */
  bool copyWithoutLastStmt = false;
  if (fromBB.GetKind() == kBBCondGoto || fromBB.GetKind() == kBBGoto) {
    copyWithoutLastStmt = true;
  } else if (fromBB.GetKind() == kBBFallthru) {
    copyWithoutLastStmt = false;
  } else {
    CHECK_FATAL(false, "must be condGoto, goto or fallthru bb");
  }
  func.CloneBBMeStmts(fromBB, toBB, &cands, copyWithoutLastStmt);
  needUpdateSSA = true;
}

size_t ValueRangePropagation::GetRealPredSize(const BB &bb) const {
  size_t unreachablePredSize = 0;
  for (auto &pred : bb.GetPred()) {
    if (unreachableBBs.find(pred) != unreachableBBs.end()) {
      unreachablePredSize++;
    }
  }
  auto res = bb.GetPred().size() - unreachablePredSize;
  return res;
}

void ValueRangePropagation::CreateLabelForTargetBB(BB &pred, BB &newBB) {
  if (pred.GetLastMe() == nullptr) {
    return;
  }
  switch (pred.GetKind()) {
    case kBBGoto:
      static_cast<GotoMeStmt*>(pred.GetLastMe())->SetOffset(func.GetOrCreateBBLabel(newBB));
      break;
    case kBBIgoto:
      CHECK_FATAL(false, "can not be here");
    case kBBCondGoto: {
      auto *condGotoStmt = static_cast<CondGotoMeStmt *>(pred.GetLastMe());
      if (&newBB == pred.GetSucc().at(1)) {
        condGotoStmt->SetOffset(func.GetOrCreateBBLabel(newBB));
      }
      break;
    }
    case kBBSwitch: {
      auto *switchStmt = static_cast<SwitchMeStmt*>(pred.GetLastMe());
      LabelIdx oldLabIdx = pred.GetBBLabel();
      LabelIdx label = func.GetOrCreateBBLabel(newBB);
      if (switchStmt->GetDefaultLabel() == oldLabIdx) {
        switchStmt->SetDefaultLabel(label);
      }
      for (size_t i = 0; i < switchStmt->GetSwitchTable().size(); i++) {
        LabelIdx labelIdx = switchStmt->GetSwitchTable().at(i).second;
        if (labelIdx == oldLabIdx) {
          switchStmt->SetCaseLabel(i, label);
        }
      }
      break;
    }
    default:
      break;
  }
}

size_t ValueRangePropagation::FindBBInSuccs(const BB &bb, const BB &succBB) const {
  for (size_t i = 0; i < bb.GetSucc().size(); ++i) {
    if (bb.GetSucc(i) == &succBB) {
      return i;
    }
  }
  CHECK_FATAL(false, "find fail");
}

void ValueRangePropagation::ComputeCodeSize(const MeExpr &meExpr, uint32 &cost) {
  for (uint8 i = 0; i < meExpr.GetNumOpnds(); ++i) {
    cost++;
    ComputeCodeSize(*(meExpr.GetOpnd(i)), cost);
  }
}

void ValueRangePropagation::ComputeCodeSize(const MeStmt &meStmt, uint32 &cost) {
  for (size_t i = 0; i < meStmt.NumMeStmtOpnds(); ++i) {
    cost++;
    ComputeCodeSize(*meStmt.GetOpnd(i), cost);
  }
}

bool ValueRangePropagation::CodeSizeIsOverflowOrTheOpOfStmtIsNotSupported(const BB &bb) {
  auto *currBB = &bb;
  uint32 currCost = 0;
  // When the succ of pred0 can be optimized to succ0, need copy the fallthru bbs and compute the code size of stmts.
  // pred0  pred1                 pred0    pred1
  //     \ /                        |       |
  //      |                         |       |
  //  fallthru0                    copy  fallthru0
  //      |                         |       |
  //  fallthru1      ----->         |    fallthru1
  //      |                         |       |
  //   condgoto                      \    condgoto
  //     / \                           \    / \
  // succ0 succ1                      succ0  succ1

  while (currBB != nullptr) {
    for (auto &meStmt : currBB->GetMeStmts()) {
      if (!IsSupportedOpForCopyInPhasesLoopUnrollAndVRP(meStmt.GetOp())) {
        return true;
      }
      if (meStmt.GetOp() == OP_goto || meStmt.GetOp() == OP_brfalse || meStmt.GetOp() == OP_brtrue) {
        continue;
      }
      ComputeCodeSize(meStmt, currCost);
    }
    if (currBB->GetKind() == kBBCondGoto) {
      break;
    }
    currBB = currBB->GetSucc(0);
  }
  if (currCost > kCodeSizeLimit) {
    if (ValueRangePropagation::isDebug) {
      LogInfo::MapleLogger() << "code size is overflow\n";
    }
    return true;
  } else {
    codeSizeCost += currCost;
    return false;
  }
}

bool ValueRangePropagation::ChangeTheSuccOfPred2TrueBranch(
    BB &pred, BB &bb, BB &trueBranch, ScalarMeExpr *updateSSAExceptTheScalarExpr,
    std::map<OStIdx, std::set<BB*>> &ssaupdateCandsForCondExpr) {
  auto *exitCopyFallthru = GetNewCopyFallthruBB(trueBranch, bb);
  if (exitCopyFallthru != nullptr) {
    PrepareForSSAUpdateWhenPredBBIsRemoved(pred, bb, updateSSAExceptTheScalarExpr, ssaupdateCandsForCondExpr);
    size_t index = FindBBInSuccs(pred, bb);
    pred.RemoveSucc(bb);
    DeleteThePhiNodeWhichOnlyHasOneOpnd(bb, updateSSAExceptTheScalarExpr, ssaupdateCandsForCondExpr);
    pred.AddSucc(*exitCopyFallthru, index);
    CreateLabelForTargetBB(pred, *exitCopyFallthru);
    return true;
  }
  if (CodeSizeIsOverflowOrTheOpOfStmtIsNotSupported(bb)) {
    return false;
  }
  auto *mergeAllFallthruBBs = CreateNewGotoBBWithoutCondGotoStmt(bb);
  if (mergeAllFallthruBBs == nullptr) {
    return false;
  }
  if (ValueRangePropagation::isDebug) {
    LogInfo::MapleLogger() << "old: " << pred.GetBBId() << " new: " << mergeAllFallthruBBs->GetBBId() << "\n";
  }
  auto *currBB = &bb;
  BB *predOfCurrBB = nullptr;
  while (currBB->GetKind() != kBBCondGoto) {
    if (currBB->GetKind() != kBBFallthru && currBB->GetKind() != kBBGoto) {
      CHECK_FATAL(false, "must be fallthru or goto bb");
    }
    predOfCurrBB = currBB;
    currBB = currBB->GetSucc(0);
    CopyMeStmts(*currBB, *mergeAllFallthruBBs);
    PrepareForSSAUpdateWhenPredBBIsRemoved(
        *predOfCurrBB, *currBB, updateSSAExceptTheScalarExpr, ssaupdateCandsForCondExpr);
  }
  if (predOfCurrBB != nullptr) {
    PrepareForSSAUpdateWhenPredBBIsRemoved(
        *predOfCurrBB, *currBB, updateSSAExceptTheScalarExpr, ssaupdateCandsForCondExpr);
  }
  CHECK_FATAL(currBB->GetKind() == kBBCondGoto, "must be condgoto bb");
  auto *gotoMeStmt = irMap.New<GotoMeStmt>(func.GetOrCreateBBLabel(trueBranch));
  mergeAllFallthruBBs->AddMeStmtLast(gotoMeStmt);
  PrepareForSSAUpdateWhenPredBBIsRemoved(pred, bb, updateSSAExceptTheScalarExpr, ssaupdateCandsForCondExpr);
  size_t index = FindBBInSuccs(pred, bb);
  pred.RemoveSucc(bb);
  pred.AddSucc(*mergeAllFallthruBBs, index);
  mergeAllFallthruBBs->AddSucc(trueBranch);
  DeleteThePhiNodeWhichOnlyHasOneOpnd(bb, updateSSAExceptTheScalarExpr, ssaupdateCandsForCondExpr);
  CreateLabelForTargetBB(pred, *mergeAllFallthruBBs);
  return true;
}

// If the valuerange of opnd in pred0 and pred1 is equal the valuerange in true branch:
//      pred0                                pred0                                 pred0
//        |                                   |                                      |
//        |                                   |                                      |
//        bb  pred1                          bb      pred1                           bb     pred1
//         \  /                               |        |                             |        |
//          \/                                |        |                             |        |
//          bb0  [fallthru/goto]           bb0(copy)  bb0                         bb0(copy)  bb0
//          |                    case1        |        |            case2            |        |
//          bb1 [fallthru/goto] ------>    bb1(copy)   bb1        -------->       bb1(copy)   bb1
//          |                                 |        |                             |        |
//          bb2 [condgoto]                    |        bb2 [condgoto]                |        bb2 [fallthru]
//          /\                                 \     / |                             |        |
//         /  \                                 \   /  |                             |        |
//      true  false                             true false                         true     false
bool ValueRangePropagation::CopyFallthruBBAndRemoveUnreachableEdge(
    BB &pred, BB &bb, BB &trueBranch, ScalarMeExpr *updateSSAExceptTheScalarExpr,
    std::map<OStIdx, std::set<BB*>> &ssaupdateCandsForCondExpr) {
  /* case1: the number of branches reaching to condBB > 1 */
  auto *tmpPred = &pred;
  do {
    tmpPred = tmpPred->GetSucc(0);
    if (GetRealPredSize(*tmpPred) > 1) {
      return ChangeTheSuccOfPred2TrueBranch(
          pred, bb, trueBranch, updateSSAExceptTheScalarExpr, ssaupdateCandsForCondExpr);
    }
  } while (tmpPred->GetKind() != kBBCondGoto);
  // step2
  CHECK_FATAL(GetRealPredSize(bb) == 1, "must have one succ");
  auto *currBB = &bb;
  while (currBB->GetKind() != kBBCondGoto) {
    currBB = currBB->GetSucc(0);
  }
  /* case2: the number of branches reaching to condBB == 1 */
  RemoveUnreachableBB(*currBB, trueBranch, updateSSAExceptTheScalarExpr, ssaupdateCandsForCondExpr);
  return true;
}

// If the valuerange of opnd in pred0 and pred1 is equal the valuerange in true branch:
//     pred0   pred1                        pred0    pred1                         pred0    pred1
//         \  /                               |        |                             |        |
//          \/                case1           |        |                case2        |        |
//          bb2 [condgoto]   ------->         |        bb2 [condgoto]  ------->      |        bb2 [fallthru]
//          /\                                 \     / |                             |        |
//         /  \                                 \   /  |                             |        |
//      true  false                             true false                         true     false
// case1: the number of branches reaching to condBB > 1
// case2: the number of branches reaching to condBB == 1
bool ValueRangePropagation::RemoveTheEdgeOfPredBB(
    BB &pred, BB &bb, BB &trueBranch, ScalarMeExpr *updateSSAExceptTheScalarExpr,
    std::map<OStIdx, std::set<BB*>> &ssaupdateCandsForCondExpr) {
  CHECK_FATAL(bb.GetKind() == kBBCondGoto, "must be condgoto bb");
  if (GetRealPredSize(bb) >= kNumOperands) {
    if (OnlyHaveCondGotoStmt(bb)) {
      PrepareForSSAUpdateWhenPredBBIsRemoved(pred, bb, updateSSAExceptTheScalarExpr, ssaupdateCandsForCondExpr);
      size_t index = FindBBInSuccs(pred, bb);
      pred.RemoveSucc(bb);
      DeleteThePhiNodeWhichOnlyHasOneOpnd(bb, updateSSAExceptTheScalarExpr, ssaupdateCandsForCondExpr);
      pred.AddSucc(trueBranch, index);
      CreateLabelForTargetBB(pred, trueBranch);
    } else {
      auto *exitCopyFallthru = GetNewCopyFallthruBB(trueBranch, bb);
      if (exitCopyFallthru != nullptr) {
        PrepareForSSAUpdateWhenPredBBIsRemoved(pred, bb, updateSSAExceptTheScalarExpr, ssaupdateCandsForCondExpr);
        size_t index = FindBBInSuccs(pred, bb);
        pred.RemoveSucc(bb);
        DeleteThePhiNodeWhichOnlyHasOneOpnd(bb, updateSSAExceptTheScalarExpr, ssaupdateCandsForCondExpr);
        pred.AddSucc(*exitCopyFallthru, index);
        CreateLabelForTargetBB(pred, *exitCopyFallthru);
        return true;
      }
      auto *newBB = CreateNewGotoBBWithoutCondGotoStmt(bb);
      if (newBB == nullptr) {
        return false;
      }
      if (ValueRangePropagation::isDebug) {
        LogInfo::MapleLogger() << "old: " << pred.GetBBId() << " new: " << newBB->GetBBId() << "\n";
      }
      auto *gotoMeStmt = irMap.New<GotoMeStmt>(func.GetOrCreateBBLabel(trueBranch));
      newBB->AddMeStmtLast(gotoMeStmt);
      PrepareForSSAUpdateWhenPredBBIsRemoved(pred, bb, updateSSAExceptTheScalarExpr, ssaupdateCandsForCondExpr);
      size_t index = FindBBInSuccs(pred, bb);
      pred.RemoveSucc(bb);
      pred.AddSucc(*newBB, index);
      newBB->AddSucc(trueBranch);
      DeleteThePhiNodeWhichOnlyHasOneOpnd(bb, updateSSAExceptTheScalarExpr, ssaupdateCandsForCondExpr);
      (void)func.GetOrCreateBBLabel(trueBranch);
      CreateLabelForTargetBB(pred, *newBB);
    }
  } else {
    CHECK_FATAL(GetRealPredSize(bb) == 1, "must have one pred");
    RemoveUnreachableBB(bb, trueBranch, updateSSAExceptTheScalarExpr, ssaupdateCandsForCondExpr);
  }
  return true;
}

// tmpPred, tmpBB, reachableBB
bool ValueRangePropagation::RemoveUnreachableEdge(
    BB &pred, BB &bb, BB &trueBranch, ScalarMeExpr *updateSSAExceptTheScalarExpr,
    std::map<OStIdx, std::set<BB*>> &ssaupdateCandsForCondExpr) {
  if (onlyPropVR) {
    return false;
  }
  if (bb.GetKind() == kBBFallthru || bb.GetKind() == kBBGoto) {
    if (!CopyFallthruBBAndRemoveUnreachableEdge(
        pred, bb, trueBranch, updateSSAExceptTheScalarExpr, ssaupdateCandsForCondExpr)) {
      return false;
    }
  } else {
    if (!RemoveTheEdgeOfPredBB(pred, bb, trueBranch, updateSSAExceptTheScalarExpr, ssaupdateCandsForCondExpr)) {
      return false;
    }
  }
  if (ValueRangePropagation::isDebug) {
    LogInfo::MapleLogger() << "===========delete edge " << pred.GetBBId() << " " << bb.GetBBId() << " " <<
        trueBranch.GetBBId() << "===========\n";
  }
  if (trueBranch.GetPred().size() > 1) {
    (void)func.GetOrCreateBBLabel(trueBranch);
  }
  isCFGChange = true;
  return true;
}

bool ValueRangePropagation::ConditionEdgeCanBeDeleted(BB &pred, BB &bb, const ValueRange *leftRange,
    const ValueRange &rightRange, BB &falseBranch, BB &trueBranch, PrimType opndType, Opcode op,
    ScalarMeExpr *updateSSAExceptTheScalarExpr, std::map<OStIdx, std::set<BB*>> &ssaupdateCandsForCondExpr) {
  if (leftRange == nullptr) {
    return false;
  }
  if (ValueRangePropagation::isDebug) {
    LogInfo::MapleLogger() << "try to delete edge " << pred.GetBBId() << " " << bb.GetBBId() << " " <<
        trueBranch.GetBBId() << "\n";
  }
  /* Find the first bb with more than one pred */
  auto *tmpPred = &pred;
  auto *tmpBB = &bb;
  while (GetRealPredSize(*tmpBB) == 1 && tmpBB->GetKind() != kBBCondGoto) {
    tmpPred = tmpBB;
    tmpBB = tmpBB->GetSucc(0);
  }
  Opcode antiOp = GetTheOppositeOp(op);
  if (BrStmtInRange(bb, *leftRange, rightRange, op, opndType)) {
    // opnd, tmpPred, tmpBB, reachableBB
    // remove falseBranch
    return RemoveUnreachableEdge(
        *tmpPred, *tmpBB, trueBranch, updateSSAExceptTheScalarExpr, ssaupdateCandsForCondExpr);
  } else if (BrStmtInRange(bb, *leftRange, rightRange, antiOp, opndType)) {
    return RemoveUnreachableEdge(
        *tmpPred, *tmpBB, falseBranch, updateSSAExceptTheScalarExpr, ssaupdateCandsForCondExpr);
  }
  return false;
}

bool ValueRangePropagation::MustBeFallthruOrGoto(const BB &defBB, const BB &bb) const {
  if (&defBB == &bb) {
    return true;
  }
  auto *currBB = &defBB;
  while (currBB->GetSucc().size() == 1) {
    if (currBB == &bb) {
      return true;
    }
    currBB = currBB->GetSucc(0);
  }
  return false;
}

std::unique_ptr<ValueRange> ValueRangePropagation::AntiValueRange(ValueRange &valueRange) {
  RangeType oldType = valueRange.GetRangeType();
  if (oldType != kEqual && oldType != kNotEqual) {
    return nullptr;
  }
  RangeType newType = (oldType == kEqual) ? kNotEqual : kEqual;
  return std::make_unique<ValueRange>(Bound(valueRange.GetBound().GetVar(), valueRange.GetBound().GetConstant(),
      valueRange.GetBound().GetPrimType()), newType);
}

void ValueRangePropagation::DeleteUnreachableBBs(BB &curBB, BB &falseBranch, BB &trueBranch) {
  size_t sizeOfUnreachables = 0;
  for (auto &pred : curBB.GetPred()) {
    if (unreachableBBs.find(pred) != unreachableBBs.end()) {
      sizeOfUnreachables++;
    }
  }
  if (curBB.GetPred().size() - sizeOfUnreachables == 0) {
    // If the preds of curBB which is condgoto and analysised is empty, delete the curBB.
    Insert2UnreachableBBs(curBB);
    // If the preds of falseBranch is empty, delete the falseBranch.
    if (falseBranch.GetPred().size() == 1 && falseBranch.GetPred(0) == &curBB) {
      Insert2UnreachableBBs(falseBranch);
    }
    // If the preds of trueBranch is empty, delete the trueBranch.
    if (trueBranch.GetPred().size() == 1 && trueBranch.GetPred(0) == &curBB) {
      Insert2UnreachableBBs(trueBranch);
    }
  }
}

void ValueRangePropagation::PropValueRangeFromCondGotoToTrueAndFalseBranch(
    const MeExpr &opnd0, ValueRange &rightRange, const BB &falseBranch, const BB &trueBranch) {
  std::unique_ptr<ValueRange> trueBranchValueRange;
  std::unique_ptr<ValueRange> falseBranchValueRange;
  trueBranchValueRange = CopyValueRange(rightRange);
  falseBranchValueRange = AntiValueRange(rightRange);
  (void)Insert2Caches(trueBranch.GetBBId(), opnd0.GetExprID(), std::move(trueBranchValueRange));
  (void)Insert2Caches(falseBranch.GetBBId(), opnd0.GetExprID(), std::move(falseBranchValueRange));
}

// a: phi(b, c); d: a; e: d
// currOpnd: e
// predOpnd: a, rhs of currOpnd in this bb
// phiOpnds: (b, c), phi rhs of opnd in this bb
// predOpnd or phiOpnds is uesd to find the valuerange in the pred of bb
void ValueRangePropagation::ReplaceOpndByDef(const BB &bb, MeExpr &currOpnd, MeExpr *&predOpnd,
    MePhiNode *&phi, bool &thePhiIsInBB) {
  /* If currOpnd is not defined in bb, set opnd to currOpnd */
  predOpnd = &currOpnd;
  /* find the rhs of opnd */
  while ((predOpnd->GetMeOp() == kMeOpVar || predOpnd->GetMeOp() == kMeOpReg) &&
         static_cast<ScalarMeExpr&>(*predOpnd).GetDefBy() == kDefByStmt &&
         static_cast<ScalarMeExpr&>(*predOpnd).GetDefStmt()->GetBB() == &bb) {
    predOpnd = static_cast<ScalarMeExpr&>(*predOpnd).GetDefStmt()->GetRHS();
  }
  /* find the phi rhs of opnd */
  if ((predOpnd->GetMeOp() == kMeOpVar || predOpnd->GetMeOp() == kMeOpReg) &&
      static_cast<ScalarMeExpr&>(*predOpnd).GetDefBy() == kDefByPhi &&
      static_cast<ScalarMeExpr&>(*predOpnd).GetDefPhi().GetDefBB() == &bb) {
    phi = &(static_cast<ScalarMeExpr&>(*predOpnd).GetDefPhi());
    thePhiIsInBB = true;
  }
}

bool ValueRangePropagation::AnalysisValueRangeInPredsOfCondGotoBB(
    BB &bb, MeExpr &opnd0, MeExpr &currOpnd, ValueRange &rightRange,
    BB &falseBranch, BB &trueBranch, PrimType opndType, Opcode op, BB &condGoto) {
  bool opt = false;
  /* Records the currOpnd of pred */
  MeExpr *predOpnd = nullptr;
  bool thePhiIsInBB = false;
  MapleVector<ScalarMeExpr*> phiOpnds(mpAllocator.Adapter());
  MePhiNode *phi = nullptr;
  /* find the rhs of currOpnd, which is used as the currOpnd of pred */
  ReplaceOpndByDef(bb, currOpnd, predOpnd, phi, thePhiIsInBB);
  if (phi != nullptr) {
    phiOpnds = static_cast<ScalarMeExpr&>(*predOpnd).GetDefPhi().GetOpnds();
  }
  size_t indexOfOpnd = 0;
  ScalarMeExpr *exprOnlyUsedByCondMeStmt = nullptr;
  std::map<OStIdx, std::set<BB*>> ssaupdateCandsForCondExpr;
  if (!thePhiIsInBB) {
    auto *useListOfPredOpnd = useInfo->GetUseSitesOfExpr(predOpnd);
    if (useListOfPredOpnd != nullptr &&  useListOfPredOpnd->size() == 1 && useListOfPredOpnd->front().IsUseByStmt()) {
      auto *useStmt = useListOfPredOpnd->front().GetStmt();
      if (condGoto.GetKind() == kBBCondGoto && condGoto.GetLastMe()->IsCondBr() && condGoto.GetLastMe() == useStmt &&
          predOpnd->IsScalar()) {
        // PredOpnd is only used be condGoto stmt, if the condGoto stmt can be deleted, need not update ssa
        // of predOpnd and the def point of predOpnd can be deleted.
        exprOnlyUsedByCondMeStmt = static_cast<ScalarMeExpr*>(predOpnd);
      }
    }
  }
  for (size_t i = 0; i < bb.GetPred().size();) {
    predOpnd = thePhiIsInBB ? phiOpnds.at(indexOfOpnd) : predOpnd;
    indexOfOpnd++;
    ScalarMeExpr *updateSSAExceptTheScalarExpr = nullptr;
    if (!thePhiIsInBB) {
      updateSSAExceptTheScalarExpr = exprOnlyUsedByCondMeStmt;
    } else {
      auto *useListOfPredOpnd = useInfo->GetUseSitesOfExpr(predOpnd);
      if (useListOfPredOpnd != nullptr && useListOfPredOpnd->size() == 1) {
        if (useListOfPredOpnd->front().IsUseByPhi() && useListOfPredOpnd->front().GetPhi() == phi) {
          auto *useListOfPhi = useInfo->GetUseSitesOfExpr(phi->GetLHS());
          if (useListOfPhi != nullptr && useListOfPhi->size() == 1 && useListOfPhi->front().IsUseByStmt()) {
            auto *useStmt = useListOfPhi->front().GetStmt();
            if (condGoto.GetKind() == kBBCondGoto && condGoto.GetLastMe()->IsCondBr() &&
                condGoto.GetLastMe() == useStmt && predOpnd->IsScalar()) {
              // PredOpnd is only used be the phi and condGoto stmt, if the pred edge can be opt, need not update ssa
              // of predOpnd and the def point of predOpnd can be deleted.
              updateSSAExceptTheScalarExpr = static_cast<ScalarMeExpr*>(predOpnd);
            }
          }
        }
      }
    }
    size_t predSize = bb.GetPred().size();
    auto *pred = bb.GetPred(i);
    // The infinite loop path does not occur during actual execution and then optimization is not required.
    if (pred->GetKind() == kBBIgoto || GetRealPredSize(*pred) == 0 ||
        unreachableBBs.find(pred) != unreachableBBs.end()) {
      ++i;
      continue;
    }
    // Resolve the scenario where a outside-pointer points to a loop.
    //   outside-pointer
    //      \        /
    //       \      /
    //      pred   pred1
    //       ^ \  /
    //       |  \/
    //       |  bb [condgoto]
    //       |  /\
    //       | /  \
    //      true  false
    if (bb.GetBBId() < dom.iterDomFrontier.size() && !dom.Dominate(*pred, bb)) {
      MapleSet<BBId> &frontier = dom.iterDomFrontier[bb.GetBBId()];
      if (frontier.find(pred->GetBBId()) != frontier.end()) {
        ++i;
        continue;
      }
    }
    auto *valueRangeInPred = FindValueRange(*pred, *predOpnd);
    if (ConditionEdgeCanBeDeleted(*pred, bb, valueRangeInPred, rightRange, falseBranch, trueBranch, opndType, op,
        updateSSAExceptTheScalarExpr, ssaupdateCandsForCondExpr)) {
      if (updateSSAExceptTheScalarExpr != nullptr && phi != nullptr) {
        if (updateSSAExceptTheScalarExpr->GetDefBy() == kDefByStmt) {
          // PredOpnd is only used by condGoto stmt and phi, if the condGoto stmt can be deleted, need not update ssa
          // of predOpnd and the def point of predOpnd can be deleted.
          phiOpnds[indexOfOpnd - 1] = nullptr;
          bool opndIsRemovedFromPhi = true;
          for (auto &temp : phiOpnds) {
            if (temp == updateSSAExceptTheScalarExpr) {
              opndIsRemovedFromPhi = false;
              break;
            }
          }
          if (opndIsRemovedFromPhi) {
            updateSSAExceptTheScalarExpr->GetDefStmt()->GetBB()->RemoveMeStmt(
                updateSSAExceptTheScalarExpr->GetDefStmt());
            updateSSAExceptTheScalarExpr->SetDefBy(kDefByNo);
          }
        }
      }
      opt = true;
    } else {
      /* avoid infinite loop, pred->GetKind() maybe kBBUnknown */
      if ((pred->GetKind() == kBBFallthru || pred->GetKind() == kBBGoto) &&
           pred->GetBBId() != falseBranch.GetBBId() && pred->GetBBId() != trueBranch.GetBBId()) {
        (void)AnalysisValueRangeInPredsOfCondGotoBB(
            *pred, opnd0, *predOpnd, rightRange, falseBranch, trueBranch, opndType, op, condGoto);
      }
    }
    if (bb.GetPred().size() == predSize) {
      ++i;
    } else if (bb.GetPred().size() != predSize - 1) {
      CHECK_FATAL(false, "immpossible");
    }
  }
  if (opt) {
    if (condGoto.GetKind() == kBBCondGoto) {
      // vrp modified condgoto, branch probability is no longer trustworthy, so we invalidate it
      static_cast<CondGotoMeStmt*>(condGoto.GetLastMe())->InvalidateBranchProb();
      // If the condGoto stmt can not be deleted, need update the ssa of conditional expr
      for (auto &pair : ssaupdateCandsForCondExpr) {
        for (auto &tempBB : pair.second) {
          MeSSAUpdate::InsertOstToSSACands(pair.first, *tempBB, &cands);
          needUpdateSSA = true;
        }
      }
    } else if (condGoto.GetKind() == kBBFallthru && exprOnlyUsedByCondMeStmt) {
      if (exprOnlyUsedByCondMeStmt->GetDefBy() == kDefByStmt) {
        // PredOpnd is only used by condGoto stmt, if the pred edge can be opt, need not update ssa
        // of predOpnd and the def point of predOpnd can be deleted.
        exprOnlyUsedByCondMeStmt->GetDefStmt()->GetBB()->RemoveMeStmt(exprOnlyUsedByCondMeStmt->GetDefStmt());
        exprOnlyUsedByCondMeStmt->SetDefBy(kDefByNo);
      }
    }
  }
  ssaupdateCandsForCondExpr.clear();
  return opt;
}

void ValueRangePropagation::GetSizeOfUnreachableBBsAndReachableBB(BB &bb, size_t &unreachableBB, BB *&reachableBB) {
  for (size_t i = 0; i < bb.GetPred().size(); ++i) {
    if (unreachableBBs.find(bb.GetPred(i)) != unreachableBBs.end()) {
      unreachableBB++;
    } else {
      reachableBB = bb.GetPred(i);
    }
  }
}

bool ValueRangePropagation::ConditionEdgeCanBeDeleted(BB &bb, MeExpr &opnd0, ValueRange &rightRange,
    BB &falseBranch, BB &trueBranch, PrimType opndType, Opcode op) {
  if (onlyPropVR) {
    return false;
  }
  auto *currOpnd = &opnd0;
  if (opnd0.GetOp() == OP_zext) {
    auto &opMeExpr = static_cast<OpMeExpr&>(opnd0);
    auto *opnd = opMeExpr.GetOpnd(0);
    if (opMeExpr.GetBitsSize() >= GetPrimTypeBitSize(opnd->GetPrimType())) {
      currOpnd = opnd;
    }
  }
  bool opt = AnalysisValueRangeInPredsOfCondGotoBB(
      bb, *currOpnd, *currOpnd, rightRange, falseBranch, trueBranch, opndType, op, bb);
  bool canDeleteBB = false;
  for (size_t i = 0; i < bb.GetPred().size(); ++i) {
    if (unreachableBBs.find(bb.GetPred(i)) == unreachableBBs.end()) {
      return opt;
    }
    canDeleteBB = true;
  }
  if (canDeleteBB) {
    Insert2UnreachableBBs(bb);
    isCFGChange = true;
  }
  return opt;
}

Opcode ValueRangePropagation::GetTheOppositeOp(Opcode op) const {
  if (op == OP_eq) {
    return OP_ne;
  } else if (op == OP_ne) {
    return OP_eq;
  } else if (op == OP_lt) {
    return OP_ge;
  } else if (op == OP_le) {
    return OP_gt;
  } else if (op == OP_ge) {
    return OP_lt;
  } else if (op == OP_gt) {
    return OP_le;
  } else {
    CHECK_FATAL(false, "can not support");
  }
}

void ValueRangePropagation::CreateValueRangeForCondGoto(const MeExpr &opnd, Opcode op, ValueRange *leftRange,
    ValueRange &rightRange, const BB &trueBranch, const BB &falseBranch) {
  auto newRightUpper = rightRange.GetUpper();
  auto newRightLower = rightRange.GetLower();
  CHECK_FATAL(IsEqualPrimType(newRightUpper.GetPrimType(), newRightLower.GetPrimType()), "must be equal");
  if (op == OP_eq) {
    CreateValueRangeForNeOrEq(opnd, leftRange, rightRange, trueBranch, falseBranch);
  } else if (op == OP_ne) {
    CreateValueRangeForNeOrEq(opnd, leftRange, rightRange, falseBranch, trueBranch);
  }
  if (rightRange.GetRangeType() == kNotEqual) {
    return;
  }
  if ((op == OP_lt) || (op == OP_ge)) {
    int64 constant = 0;
    if (!AddOrSubWithConstant(newRightUpper.GetPrimType(), OP_sub, newRightUpper.GetConstant(), 1, constant)) {
      return;
    }
    newRightUpper = Bound(newRightUpper.GetVar(), constant, newRightUpper.GetPrimType());
  } else if ((op == OP_le) || (op == OP_gt)) {
    int64 constant = 0;
    if (!AddOrSubWithConstant(newRightUpper.GetPrimType(), OP_add, newRightLower.GetConstant(), 1, constant)) {
      return;
    }
    newRightLower = Bound(newRightLower.GetVar(), constant, newRightUpper.GetPrimType());
  }
  if (op == OP_lt || op == OP_le) {
    if (leftRange != nullptr && leftRange->GetRangeType() == kNotEqual) {
      CreateValueRangeForLeOrLt(opnd, nullptr, newRightUpper, newRightLower, trueBranch, falseBranch);
    } else {
      CreateValueRangeForLeOrLt(opnd, leftRange, newRightUpper, newRightLower, trueBranch, falseBranch);
    }
  } else if (op == OP_gt || op == OP_ge) {
    if (leftRange != nullptr && leftRange->GetRangeType() == kNotEqual) {
      CreateValueRangeForGeOrGt(opnd, nullptr, newRightUpper, newRightLower, trueBranch, falseBranch);
    } else {
      CreateValueRangeForGeOrGt(opnd, leftRange, newRightUpper, newRightLower, trueBranch, falseBranch);
    }
  }
}

void ValueRangePropagation::DealWithCondGoto(
    BB &bb, Opcode op, ValueRange *leftRange, ValueRange &rightRange, const CondGotoMeStmt &brMeStmt) {
  MeExpr *opnd0 = nullptr;
  PrimType primType;
  if (IsCompareHasReverseOp(brMeStmt.GetOpnd()->GetOp())) {
    // deal with the stmt like : brfalse ne (a, b)
    auto *opMeExpr = static_cast<OpMeExpr*>(brMeStmt.GetOpnd());
    opnd0 = opMeExpr->GetOpnd(0);
    primType = opMeExpr->GetOpndType();
  } else {
    // deal with the stmt like : brfalse (a)
    opnd0 = brMeStmt.GetOpnd();
    primType = opnd0->GetPrimType();
  }
  BB *trueBranch = nullptr;
  BB *falseBranch = nullptr;
  GetTrueAndFalseBranch(brMeStmt.GetOp(), bb, trueBranch, falseBranch);
  if (onlyPropVR) {
    CreateValueRangeForCondGoto(*opnd0, op, leftRange, rightRange, *trueBranch, *falseBranch);
    return;
  }
  // When the redundant branch can be inferred directly from the condGoto stmt
  Opcode antiOp = GetTheOppositeOp(op);
  if (leftRange != nullptr && BrStmtInRange(bb, *leftRange, rightRange, op, primType)) {
    // bb, unreachableBB, succBB
    AnalysisUnreachableBBOrEdge(bb, *falseBranch, *trueBranch);
  } else if (leftRange != nullptr && BrStmtInRange(bb, *leftRange, rightRange, antiOp, primType)) {
    AnalysisUnreachableBBOrEdge(bb, *trueBranch, *falseBranch);
  } else {
    ConditionEdgeCanBeDeleted(bb, *opnd0, rightRange, *falseBranch, *trueBranch, primType, op);
  }
  CreateValueRangeForCondGoto(*opnd0, op, leftRange, rightRange, *trueBranch, *falseBranch);
}

bool ValueRangePropagation::GetValueRangeOfCondGotoOpnd(const BB &bb, OpMeExpr &opMeExpr, MeExpr &opnd,
    ValueRange *&valueRange, std::unique_ptr<ValueRange> &rightRangePtr) {
  valueRange = FindValueRange(bb, opnd);
  if (valueRange == nullptr) {
    if (opnd.GetMeOp() == kMeOpConst && static_cast<ConstMeExpr&>(opnd).GetConstVal()->GetKind() == kConstInt) {
      rightRangePtr = std::make_unique<ValueRange>(Bound(static_cast<ConstMeExpr&>(opnd).GetIntValue(),
          opnd.GetPrimType()), kEqual);
      valueRange = rightRangePtr.get();
      if (!Insert2Caches(bb.GetBBId(), opnd.GetExprID(), std::move(rightRangePtr))) {
        valueRange = nullptr;
        return false;
      }
    }
    if (opnd.GetOp() == OP_ne || opnd.GetOp() == OP_eq) {
      if (static_cast<OpMeExpr&>(opnd).GetNumOpnds() != kNumOperands) {
        return false;
      }
      MeExpr *lhs = static_cast<OpMeExpr&>(opnd).GetOpnd(0);
      auto *valueRangeOfLHS = FindValueRange(bb, *lhs);
      if (valueRangeOfLHS == nullptr || !valueRangeOfLHS->IsConstant()) {
        valueRange = nullptr;
        return false;
      }
      MeExpr *rhs = static_cast<OpMeExpr&>(opnd).GetOpnd(1);
      if (rhs->GetMeOp() != kMeOpConst || static_cast<ConstMeExpr*>(rhs)->GetConstVal()->GetKind() != kConstInt) {
        valueRange = nullptr;
        return false;
      }
      RangeType lhsRangeType = valueRangeOfLHS->GetRangeType();
      Bound lhsBound = valueRangeOfLHS->GetBound();
      PrimType rhsPrimType = rhs->GetPrimType();
      Bound rhsBound = Bound(static_cast<ConstMeExpr*>(rhs)->GetIntValue(), rhsPrimType);
      // If the type of operands is OpMeExpr, need compute the valueRange of operand:
      // Example: if ((a != c) < b),
      // a: ValueRange(5, kEqual)
      // c: ValueRange(6, kEqual)
      // ===>
      // (a != c) : ValueRange(1, kEqual)
      if ((opnd.GetOp() == OP_ne && !lhsBound.IsEqual(rhsBound, rhsPrimType) && lhsRangeType == kEqual) ||
          (opnd.GetOp() == OP_eq && lhsBound.IsEqual(rhsBound, rhsPrimType) && lhsRangeType == kEqual) ||
          (opnd.GetOp() == OP_ne && lhsBound.IsEqual(rhsBound, rhsPrimType) && lhsRangeType == kNotEqual)) {
        rightRangePtr = std::make_unique<ValueRange>(Bound(1, PTY_u1), kEqual);
        valueRange = rightRangePtr.get();
        (void)Insert2Caches(bb.GetBBId(), opnd.GetExprID(), std::move(rightRangePtr));
      } else if ((opnd.GetOp() == OP_ne && lhsBound.IsEqual(rhsBound, rhsPrimType) && lhsRangeType == kEqual) ||
          (opnd.GetOp() == OP_eq && !lhsBound.IsEqual(rhsBound, rhsPrimType) && lhsRangeType == kEqual) ||
          (opnd.GetOp() == OP_eq && lhsBound.IsEqual(rhsBound, rhsPrimType) && lhsRangeType == kNotEqual)) {
        rightRangePtr = std::make_unique<ValueRange>(Bound(nullptr, 0, PTY_u1), kEqual);
        valueRange = rightRangePtr.get();
        (void)Insert2Caches(bb.GetBBId(), opnd.GetExprID(), std::move(rightRangePtr));
      }
    }
  }
  if (valueRange != nullptr && valueRange->GetLower().GetPrimType() != opMeExpr.GetOpndType() &&
      IsNeededPrimType(opMeExpr.GetOpndType())) {
    rightRangePtr = CopyValueRange(*valueRange, opMeExpr.GetOpndType());
    valueRange = rightRangePtr.get();
  }
  return true;
}

MeExpr *ValueRangePropagation::GetDefOfBase(const IvarMeExpr &ivar) const {
  if (ivar.GetBase()->GetMeOp() != kMeOpVar) {
    return nullptr;
  }
  auto *var = static_cast<const VarMeExpr*>(ivar.GetBase());
  if (var->GetDefBy() != kDefByStmt) {
    return nullptr;
  }
  return var->GetDefStmt()->GetLHS();
}

void ValueRangePropagation::DealWithCondGotoWhenRightRangeIsNotExist(
    BB &bb, const MeExpr &opnd0, MeExpr &opnd1, Opcode opOfBrStmt, Opcode conditionalOp, ValueRange *valueRangeOfLeft) {
  PrimType prim = opnd1.GetPrimType();
  if (!IsNeededPrimType(prim)) {
    return;
  }
  BB *trueBranch = nullptr;
  BB *falseBranch = nullptr;
  GetTrueAndFalseBranch(opOfBrStmt, bb, trueBranch, falseBranch);
  switch (conditionalOp) {
    case OP_ne: {
      (void)Insert2Caches(trueBranch->GetBBId(), opnd0.GetExprID(),
          std::make_unique<ValueRange>(Bound(&opnd1, prim), kNotEqual));
      (void)Insert2Caches(falseBranch->GetBBId(), opnd0.GetExprID(),
          std::make_unique<ValueRange>(Bound(&opnd1, prim), kEqual));
      break;
    }
    case OP_eq: {
      (void)Insert2Caches(trueBranch->GetBBId(), opnd0.GetExprID(),
          std::make_unique<ValueRange>(Bound(&opnd1, prim), kEqual));
      (void)Insert2Caches(falseBranch->GetBBId(), opnd0.GetExprID(),
          std::make_unique<ValueRange>(Bound(&opnd1, prim), kNotEqual));
      break;
    }
    case OP_le: {
      if (valueRangeOfLeft != nullptr && valueRangeOfLeft->GetRangeType() == kOnlyHasLowerBound) {
        (void)Insert2Caches(trueBranch->GetBBId(), opnd0.GetExprID(),
                            std::make_unique<ValueRange>(valueRangeOfLeft->GetLower(),
                                                         Bound(&opnd1, prim), kLowerAndUpper));
      } else {
        (void)Insert2Caches(trueBranch->GetBBId(), opnd0.GetExprID(),
                            std::make_unique<ValueRange>(Bound(GetMinNumber(prim), prim),
                                                         Bound(&opnd1, prim), kLowerAndUpper));
      }
      (void)Insert2Caches(falseBranch->GetBBId(), opnd0.GetExprID(),
          std::make_unique<ValueRange>(Bound(&opnd1, 1, prim),
              Bound(GetMaxNumber(prim), prim), kLowerAndUpper));
      break;
    }
    case OP_lt: {
      if (valueRangeOfLeft != nullptr && valueRangeOfLeft->GetRangeType() == kOnlyHasLowerBound) {
        (void)Insert2Caches(trueBranch->GetBBId(), opnd0.GetExprID(),
                            std::make_unique<ValueRange>(valueRangeOfLeft->GetLower(),
                                                         Bound(&opnd1, -1, prim), kLowerAndUpper));
      } else {
        (void)Insert2Caches(trueBranch->GetBBId(), opnd0.GetExprID(),
                            std::make_unique<ValueRange>(Bound(GetMinNumber(prim), prim),
                                                         Bound(&opnd1, -1, prim), kLowerAndUpper));
      }
      (void)Insert2Caches(falseBranch->GetBBId(), opnd0.GetExprID(),
          std::make_unique<ValueRange>(Bound(&opnd1, prim), Bound(GetMaxNumber(prim), prim), kLowerAndUpper));
      break;
    }
    case OP_ge: {
      if (valueRangeOfLeft != nullptr && valueRangeOfLeft->GetRangeType() == kOnlyHasUpperBound) {
        (void)Insert2Caches(trueBranch->GetBBId(), opnd0.GetExprID(),
            std::make_unique<ValueRange>(Bound(&opnd1, prim), valueRangeOfLeft->GetBound(), kLowerAndUpper));
      } else {
        (void)Insert2Caches(trueBranch->GetBBId(), opnd0.GetExprID(),
            std::make_unique<ValueRange>(Bound(&opnd1, prim),
                Bound(GetMaxNumber(prim), prim), kLowerAndUpper));
      }
      (void)Insert2Caches(falseBranch->GetBBId(), opnd0.GetExprID(),
          std::make_unique<ValueRange>(Bound(GetMinNumber(prim), prim),
              Bound(&opnd1, -1, prim), kLowerAndUpper));
      break;
    }
    case OP_gt: {
      if (valueRangeOfLeft != nullptr && valueRangeOfLeft->GetRangeType() == kOnlyHasUpperBound) {
        (void)Insert2Caches(trueBranch->GetBBId(), opnd0.GetExprID(),
            std::make_unique<ValueRange>(Bound(&opnd1, 1, prim), valueRangeOfLeft->GetBound(), kLowerAndUpper));
      } else {
        (void)Insert2Caches(trueBranch->GetBBId(), opnd0.GetExprID(),
            std::make_unique<ValueRange>(Bound(&opnd1, 1, prim), Bound(GetMaxNumber(prim), prim), kLowerAndUpper));
      }
      (void)Insert2Caches(falseBranch->GetBBId(), opnd0.GetExprID(),
          std::make_unique<ValueRange>(Bound(GetMinNumber(prim), prim),
              Bound(&opnd1, prim), kLowerAndUpper));
      break;
    }
    default:
      break;
  }
}

void ValueRangePropagation::GetValueRangeForUnsignedInt(const BB &bb, OpMeExpr &opMeExpr, const MeExpr &opnd,
    ValueRange *&valueRange, std::unique_ptr<ValueRange> &rightRangePtr) {
  if (onlyPropVR) {
    return;
  }
  PrimType prim = opMeExpr.GetOpndType();
  if (prim == PTY_u1 || prim == PTY_u8 || prim == PTY_u16 || prim == PTY_u32 || prim == PTY_a32 ||
      ((prim == PTY_ref || prim == PTY_ptr) && GetPrimTypeSize(prim) == kFourByte)) {
    rightRangePtr = std::make_unique<ValueRange>(
        Bound(nullptr, 0, prim), Bound(GetMaxNumber(prim), prim), kLowerAndUpper);
    valueRange = rightRangePtr.get();
    (void)Insert2Caches(bb.GetBBId(), opnd.GetExprID(), std::move(rightRangePtr));
  }
}

// This function deals with the case like this:
// Example: if (a > 0)
// a: valuerange(0, Max)
// ==>
// Example: if (a != 0)
bool ValueRangePropagation::DealWithSpecialCondGoto(
    OpMeExpr &opMeExpr, const ValueRange &leftRange, ValueRange &rightRange, CondGotoMeStmt &brMeStmt) {
  if (onlyPropVR) {
    return false;
  }
  if (opMeExpr.GetOp() != OP_gt && opMeExpr.GetOp() != OP_lt) {
    return false;
  }
  if (rightRange.GetRangeType() != kEqual || !rightRange.IsConstant()) {
    return false;
  }
  if (leftRange.GetRangeType() != kLowerAndUpper) {
    return false;
  }
  if (leftRange.GetLower().GetVar() != nullptr || leftRange.GetUpper().GetVar() != nullptr ||
      leftRange.GetLower().GetConstant() >= leftRange.GetUpper().GetConstant()) {
    return false;
  }
  if (leftRange.GetLower().GetConstant() != rightRange.GetBound().GetConstant()) {
    return false;
  }
  if (opMeExpr.GetNumOpnds() != kNumOperands) {
    return false;
  }
  auto *newExpr = irMap.CreateMeExprCompare(
      OP_ne, opMeExpr.GetPrimType(), opMeExpr.GetOpndType(), *opMeExpr.GetOpnd(0), *opMeExpr.GetOpnd(1));
  (void)irMap.ReplaceMeExprStmt(brMeStmt, opMeExpr, *newExpr);
  return true;
}

// Deal with the special brstmt, which only has one opnd.
void ValueRangePropagation::DealWithBrStmtWithOneOpnd(BB &bb, const CondGotoMeStmt &stmt, MeExpr &opnd, Opcode op) {
  if (op != OP_eq && op != OP_ne) {
    return;
  }
  // brfalse/brtrue a  --->  brfalse ne(a, 0)
  // brfalse/brtrue 1  --->  brture ne(1,0)
  // op: OP_ne
  // rightRange: 0
  // leftRange: const 1 or var a
  std::unique_ptr<ValueRange> rightRangePtr = std::make_unique<ValueRange>(
      Bound(nullptr, 0, opnd.GetPrimType()), kEqual);
  if (opnd.GetMeOp() == kMeOpConst && static_cast<ConstMeExpr&>(opnd).GetConstVal()->GetKind() == kConstInt) {
    std::unique_ptr<ValueRange> leftRangePtr =
        std::make_unique<ValueRange>(Bound(static_cast<ConstMeExpr&>(opnd).GetIntValue(), opnd.GetPrimType()), kEqual);
    DealWithCondGoto(bb, op, leftRangePtr.get(), *rightRangePtr.get(), stmt);
  } else {
    ValueRange *leftRange = FindValueRange(bb, opnd);
    DealWithCondGoto(bb, op, leftRange, *rightRangePtr.get(), stmt);
  }
}

void ValueRangePropagation::InsertValueRangeOfCondExpr2Caches(BB &bb, const MeStmt &stmt) {
  if (bb.GetKind() != kBBCondGoto) {
    return;
  }
  BB *trueBranch = nullptr;
  BB *falseBranch = nullptr;
  const CondGotoMeStmt &brMeStmt = static_cast<const CondGotoMeStmt&>(stmt);
  GetTrueAndFalseBranch(stmt.GetOp(), bb, trueBranch, falseBranch);
  (void)Insert2Caches(trueBranch->GetBBId(), brMeStmt.GetOpnd()->GetExprID(),
                      CreateValueRangeOfNotEqualZero(brMeStmt.GetOpnd()->GetPrimType()));
  (void)Insert2Caches(falseBranch->GetBBId(), brMeStmt.GetOpnd()->GetExprID(),
                      CreateValueRangeOfEqualZero(brMeStmt.GetOpnd()->GetPrimType()));
  if (!IsCompareHasReverseOp(brMeStmt.GetOpnd()->GetOp()) || brMeStmt.GetOpnd()->GetNumOpnds() != kNumOperands) {
    return;
  }
  auto *opMeExpr = static_cast<OpMeExpr*>(brMeStmt.GetOpnd());
  MeExpr *opnd0 = opMeExpr->GetOpnd(0);
  MeExpr *opnd1 = opMeExpr->GetOpnd(1);
  if (opnd0->IsVolatile() || opnd1->IsVolatile()) {
    return;
  }
  auto op = opMeExpr->GetOp();
  if (op == OP_eq) {
    Insert2PairOfExprs(*opnd0, *opnd1, *trueBranch);
    Insert2PairOfExprs(*opnd1, *opnd0, *trueBranch);
  }
  if (op == OP_ne) {
    Insert2PairOfExprs(*opnd0, *opnd1, *falseBranch);
    Insert2PairOfExprs(*opnd1, *opnd0, *falseBranch);
  }
}

// This function deals with the case like this:
// Example1: first brfalse mx1 ge mx2
//          second brfalse mx1 ge/lt mx2 ==>  remove falseBranch or trueBranch
// Example2: first brfalse mx1 gt mx2
//          second brfalse mx1 gt/le mx2 ==>  remove falseBranch or trueBranch
// mx1: valuerange [mx2+1, max(mx2_type)] or [mx2, max(mx2_type)]
// ==>
// remove falseBranch or trueBranch
bool ValueRangePropagation::AnalysisUnreachableForGeOrGt(
        BB &bb, const CondGotoMeStmt &brMeStmt, const ValueRange &leftRange) {
  Opcode op = static_cast<OpMeExpr*>(brMeStmt.GetOpnd())->GetOp();
  BB *trueBranch = nullptr;
  BB *falseBranch = nullptr;
  GetTrueAndFalseBranch(brMeStmt.GetOp(), bb, trueBranch, falseBranch);
  // When the redundant branch can be inferred directly from the condGoto stmt
  if (op == OP_gt && leftRange.GetLower().GetConstant() == 1) {
    // falseBranch is unreachable
    AnalysisUnreachableBBOrEdge(bb, *falseBranch, *trueBranch);
    return true;
  } else if (op == OP_le && leftRange.GetLower().GetConstant() == 1) {
    AnalysisUnreachableBBOrEdge(bb, *trueBranch, *falseBranch);
    return true;
  } else if (op == OP_ge && leftRange.GetLower().GetConstant() == 0) {
    AnalysisUnreachableBBOrEdge(bb, *falseBranch, *trueBranch);
    return true;
  } else if (op == OP_lt && leftRange.GetLower().GetConstant() == 0) {
    AnalysisUnreachableBBOrEdge(bb, *trueBranch, *falseBranch);
    return true;
  }
  return false;
}

// This function deals with the case like this:
// Example1: first brfalse mx1 le mx2
//          second brfalse mx1 lt/ge mx2 ==>  remove falseBranch or trueBranch
// Example2: first brfalse mx1 lt mx2
//          second brfalse mx1 le/gt mx2 ==>  remove falseBranch or trueBranch
// mx1: valuerange [min(mx2_type), mx2-1] or [min(mx2_type), mx2]
// ==>
// remove falseBranch or trueBranch
bool ValueRangePropagation::AnalysisUnreachableForLeOrLt(
    BB &bb, const CondGotoMeStmt &brMeStmt, const ValueRange &leftRange) {
  Opcode op = static_cast<OpMeExpr*>(brMeStmt.GetOpnd())->GetOp();
  BB *trueBranch = nullptr;
  BB *falseBranch = nullptr;
  GetTrueAndFalseBranch(brMeStmt.GetOp(), bb, trueBranch, falseBranch);
  // When the redundant branch can be inferred directly from the condGoto stmt
  if (op == OP_lt && leftRange.GetUpper().GetConstant()  == -1) {
    // falseBranch is unreachable
    AnalysisUnreachableBBOrEdge(bb, *falseBranch, *trueBranch);
    return true;
  } else if (op == OP_ge && leftRange.GetUpper().GetConstant() == -1) {
    AnalysisUnreachableBBOrEdge(bb, *trueBranch, *falseBranch);
    return true;
  } else if (op == OP_le && leftRange.GetUpper().GetConstant() == 0) {
    // falseBranch is the unreachableBB
    AnalysisUnreachableBBOrEdge(bb, *falseBranch, *trueBranch);
    return true;
  } else if (op == OP_gt && leftRange.GetUpper().GetConstant() == 0) {
    AnalysisUnreachableBBOrEdge(bb, *trueBranch, *falseBranch);
    return true;
  }
  return false;
}

// This function deals with the case like this:
// Example1: first brfalse mx1 eg mx2
//          second brfalse mx1 eq/ne mx2 ==>  remove falseBranch or trueBranch
// Example2: first brfalse mx1 ne mx2
//          second brfalse mx1 eq/ne mx2 ==>  remove falseBranch or trueBranch
// mx1: valuerange [mx2, mx2]
// ==>
// remove falseBranch or trueBranch
bool ValueRangePropagation::AnalysisUnreachableForEqOrNe(
        BB &bb, const CondGotoMeStmt &brMeStmt, const ValueRange &leftRange) {
  Opcode op = static_cast<OpMeExpr*>(brMeStmt.GetOpnd())->GetOp();
  BB *trueBranch = nullptr;
  BB *falseBranch = nullptr;
  GetTrueAndFalseBranch(brMeStmt.GetOp(), bb, trueBranch, falseBranch);
  // When the redundant branch can be inferred directly from the condGoto stmt
  if ((op == OP_eq && leftRange.GetRangeType() == kEqual) ||
      (op == OP_ne && leftRange.GetRangeType() == kNotEqual)) {
    // falseBranch is unreachable
    AnalysisUnreachableBBOrEdge(bb, *falseBranch, *trueBranch);
    return true;
  } else if ((op == OP_ne && leftRange.GetRangeType() == kEqual) ||
             (op == OP_eq && leftRange.GetRangeType() == kNotEqual)) {
    AnalysisUnreachableBBOrEdge(bb, *trueBranch, *falseBranch);
    return true;
  }
  return false;
}

// This function deals with the case like this:
// Example: first brfalse mx1 eq(ne/gt/le/ge/lt) mx2
//          second brfalse mx1 eq(ne/gt/le/ge/lt) mx2 ==>  remove falseBranch or trueBranch
bool ValueRangePropagation::DealWithVariableRange(BB &bb, const CondGotoMeStmt &brMeStmt,
                                                  const ValueRange &leftRange) {
  MeExpr *opnd1 = static_cast<OpMeExpr*>(brMeStmt.GetOpnd())->GetOpnd(1);
  // Example1: deal with like if (mx1 > mx2), when the valuerange of mx1 is [mx2+1, max(mx2_type)]
  // Example2: deal with like if (mx1 >= mx2), when the valuerange of mx1 is [mx2, max(mx2_type)]
  if (leftRange.GetLower().GetVar() == opnd1 && leftRange.GetUpper().GetVar() == nullptr &&
      leftRange.UpperIsMax(opnd1->GetPrimType())) {
    return AnalysisUnreachableForGeOrGt(bb, brMeStmt, leftRange);
  // Example1: deal with like if (mx1 < mx2), when the valuerange of mx1 is [min(mx2_type), mx2-1]
  // Example2: deal with like if (mx1 <= mx2), when the valuerange of mx1 is [min(mx2_type), mx2]
  } else if (leftRange.GetLower().GetVar() == nullptr && leftRange.GetUpper().GetVar() == opnd1 &&
      leftRange.LowerIsMin(opnd1->GetPrimType())) {
    return AnalysisUnreachableForLeOrLt(bb, brMeStmt, leftRange);
  // Example: deal with like if (mx1 == mx2), when the valuerange of mx1 is [mx2, mx2]
  // Example: deal with like if (mx1 != mx2), when the valuerange of mx1 is [mx2, mx2]
  } else if (leftRange.GetLower().GetVar() == opnd1 && leftRange.GetLower().GetConstant() == 0 &&
             leftRange.GetUpper().GetVar() == opnd1 && leftRange.GetUpper().GetConstant() == 0) {
    return AnalysisUnreachableForEqOrNe(bb, brMeStmt, leftRange);
  }
  return false;
}

void ValueRangePropagation::DealWithCondGoto(BB &bb, MeStmt &stmt) {
  CondGotoMeStmt &brMeStmt = static_cast<CondGotoMeStmt&>(stmt);
  const BB *brTarget = bb.GetSucc(1);
  CHECK_FATAL(brMeStmt.GetOffset() == brTarget->GetBBLabel(), "must be");
  if (!IsCompareHasReverseOp(brMeStmt.GetOpnd()->GetOp())) {
    return DealWithBrStmtWithOneOpnd(bb, brMeStmt, *brMeStmt.GetOpnd(), OP_ne);
  }
  auto *opMeExpr = static_cast<OpMeExpr*>(brMeStmt.GetOpnd());
  if (opMeExpr->GetNumOpnds() != kNumOperands) {
    return;
  }
  MeExpr *opnd0 = opMeExpr->GetOpnd(0);
  MeExpr *opnd1 = opMeExpr->GetOpnd(1);
  if (opnd0->IsVolatile() || opnd1->IsVolatile()) {
    return;
  }
  ValueRange *rightRange = nullptr;
  ValueRange *leftRange = nullptr;
  std::unique_ptr<ValueRange> rightRangePtr;
  std::unique_ptr<ValueRange> leftRangePtr;
  if (!GetValueRangeOfCondGotoOpnd(bb, *opMeExpr, *opnd0, leftRange, leftRangePtr)) {
    return;
  }
  // Example: deal with brfalse mx1 eq(ne/gt/le/ge/lt) mx2,
  //          mx1: valuerange [mx2+1, max(mx2_type)] or [min(mx2_type), mx2] or [mx2,mx2], etc
  //          remove falseBranch or trueBranch
  if (leftRange != nullptr && DealWithVariableRange(bb, brMeStmt, *leftRange)) {
    return;
  }
  if (!GetValueRangeOfCondGotoOpnd(bb, *opMeExpr, *opnd1, rightRange, rightRangePtr)) {
    return;
  }
  if (rightRange == nullptr) {
    DealWithCondGotoWhenRightRangeIsNotExist(bb, *opnd0, *opnd1, brMeStmt.GetOp(), opMeExpr->GetOp(), leftRange);
    return;
  }
  if (leftRange == nullptr && rightRange->GetRangeType() != kEqual) {
    return;
  }
  if (leftRange == nullptr) {
    GetValueRangeForUnsignedInt(bb, *opMeExpr, *opnd0, leftRange, leftRangePtr);
  }
  if (leftRange != nullptr) {
    if (opMeExpr->GetOp() == OP_gt && DealWithSpecialCondGoto(*opMeExpr, *leftRange, *rightRange, brMeStmt)) {
      return DealWithCondGoto(bb, stmt);
    }
    if (opMeExpr->GetOp() == OP_lt && DealWithSpecialCondGoto(*opMeExpr, *rightRange, *leftRange, brMeStmt)) {
      return DealWithCondGoto(bb, stmt);
    }
  }
  DealWithCondGoto(bb, opMeExpr->GetOp(), leftRange, *rightRange, brMeStmt);
}

void ValueRangePropagation::DumpCaches() {
  LogInfo::MapleLogger() << "================Dump value range===================" << "\n";
  for (size_t i = 0; i < caches.size(); ++i) {
    LogInfo::MapleLogger() << "BBId: " << i << "\n";
    auto &it = caches[i];
    for (auto bIt = it.begin(), eIt = it.end(); bIt != eIt; ++bIt) {
      if (bIt->second == nullptr) {
        continue;
      }
      if (bIt->second->GetRangeType() == kLowerAndUpper ||
          bIt->second->GetRangeType() == kSpecialLowerForLoop ||
          bIt->second->GetRangeType() == kSpecialUpperForLoop) {
        std::string lower = (bIt->second->GetLower().GetVar() == nullptr) ?
            std::to_string(bIt->second->GetLower().GetConstant()) :
            "mx" + std::to_string(bIt->second->GetLower().GetVar()->GetExprID()) + " " +
            std::to_string(bIt->second->GetLower().GetConstant());
        std::string upper = (bIt->second->GetUpper().GetVar() == nullptr) ?
            std::to_string(bIt->second->GetUpper().GetConstant()) :
            "mx" + std::to_string(bIt->second->GetUpper().GetVar()->GetExprID()) + " " +
            std::to_string(bIt->second->GetUpper().GetConstant());
        LogInfo::MapleLogger() << "mx" << bIt->first << " lower: " << lower << " upper: " << upper;
        if (bIt->second->GetRangeType() == kLowerAndUpper) {
          LogInfo::MapleLogger() << " kLowerAndUpper\n";
        } else if (bIt->second->GetRangeType() == kSpecialLowerForLoop) {
          LogInfo::MapleLogger() << " kSpecialLowerForLoop\n";
        } else {
          LogInfo::MapleLogger() << " kSpecialUpperForLoop\n";
        }
      } else if (bIt->second->GetRangeType() == kOnlyHasLowerBound) {
        std::string lower = (bIt->second->GetBound().GetVar() == nullptr) ?
            std::to_string(bIt->second->GetBound().GetConstant()) :
            "mx" + std::to_string(bIt->second->GetBound().GetVar()->GetExprID()) + " " +
            std::to_string(bIt->second->GetBound().GetConstant());
        LogInfo::MapleLogger() << "mx" << bIt->first << " lower: " << lower << " upper: max " << "kOnlyHasLowerBound\n";
      } else if (bIt->second->GetRangeType() == kOnlyHasUpperBound) {
        std::string upper = (bIt->second->GetBound().GetVar() == nullptr) ?
            std::to_string(bIt->second->GetBound().GetConstant()) :
            "mx" + std::to_string(bIt->second->GetBound().GetVar()->GetExprID()) + " " +
            std::to_string(bIt->second->GetBound().GetConstant());
        LogInfo::MapleLogger() << "mx" << bIt->first << " lower: min upper: " << upper << "kOnlyHasUpperBound\n";
      } else if (bIt->second->GetRangeType() == kEqual || bIt->second->GetRangeType() == kNotEqual) {
        std::string lower = (bIt->second->GetBound().GetVar() == nullptr) ?
            std::to_string(bIt->second->GetBound().GetConstant()) :
            "mx" + std::to_string(bIt->second->GetBound().GetVar()->GetExprID()) + " " +
            std::to_string(bIt->second->GetBound().GetConstant());
        LogInfo::MapleLogger() << "mx" << bIt->first << " lower and upper: " << lower;
        if (bIt->second->GetRangeType() == kEqual) {
          LogInfo::MapleLogger() << " kEqual\n";
        } else {
          LogInfo::MapleLogger() << " kNotEqual\n";
        }
      }
    }
  }
  LogInfo::MapleLogger() << "================Dump value range===================" << "\n";
}

void MEValueRangePropagation::GetAnalysisDependence(maple::AnalysisDep &aDep) const {
  aDep.AddRequired<MEIRMapBuild>();
  aDep.AddRequired<MELoopCanon>();
  aDep.AddRequired<MEDominance>();
  aDep.AddRequired<MELoopAnalysis>();
  aDep.SetPreservedAll();
}

bool MEValueRangePropagation::PhaseRun(maple::MeFunction &f) {
  f.vrpRuns++;
  auto *irMap = GET_ANALYSIS(MEIRMapBuild, f);
  CHECK_FATAL(irMap != nullptr, "irMap phase has problem");
  auto *dom = GET_ANALYSIS(MEDominance, f);
  CHECK_FATAL(dom != nullptr, "dominance phase has problem");
  auto *meLoop = GET_ANALYSIS(MELoopAnalysis, f);
  if (ValueRangePropagation::isDebug) {
    LogInfo::MapleLogger() << f.GetName() << "\n";
    f.Dump(false);
    f.GetCfg()->DumpToFile("valuerange-before" + std::to_string(f.vrpRuns));
  }
  auto *valueRangeMemPool = GetPhaseMemPool();
  std::map<OStIdx, std::unique_ptr<std::set<BBId>>> cands((std::less<OStIdx>()));
  LoopScalarAnalysisResult sa(*irMap, nullptr);
  sa.SetComputeTripCountForLoopUnroll(false);
  ValueRangePropagation valueRangePropagation(f, *irMap, *dom, meLoop, *valueRangeMemPool, cands, sa);

  SafetyCheck safetyCheck; // dummy
  valueRangePropagation.SetSafetyBoundaryCheck(safetyCheck);
  valueRangePropagation.SetSafetyNonnullCheck(safetyCheck);

  valueRangePropagation.Execute();
  (void)f.GetCfg()->UnreachCodeAnalysis(true);
  f.GetCfg()->WontExitAnalysis();
  // split critical edges
  MeSplitCEdge(false).SplitCriticalEdgeForMeFunc(f);
  if (valueRangePropagation.IsCFGChange()) {
    if (ValueRangePropagation::isDebug) {
      f.GetCfg()->DumpToFile("valuerange-after" + std::to_string(f.vrpRuns));
    }
    GetAnalysisInfoHook()->ForceEraseAnalysisPhase(f.GetUniqueID(), &MEDominance::id);
    dom = FORCE_GET(MEDominance);
    if (valueRangePropagation.NeedUpdateSSA()) {
      MeSSAUpdate ssaUpdate(f, *f.GetMeSSATab(), *dom, cands, *valueRangeMemPool);
      MPLTimer timer;
      timer.Start();
      if (ValueRangePropagation::isDebug) {
        LogInfo::MapleLogger() << "***************ssaupdate value range prop***************" << "\n";
        LogInfo::MapleLogger() << "========size of ost " << cands.size() << " " <<
            f.GetMeSSATab()->GetOriginalStTableSize() << "========\n";
      }
      ssaUpdate.Run();
      timer.Stop();
      if (ValueRangePropagation::isDebug) {
        LogInfo::MapleLogger() << "ssaupdate consumes cumulatively " << timer.Elapsed() << "seconds " << '\n';
        LogInfo::MapleLogger() << "***************ssaupdate value range prop***************" << "\n";
      }
    }
    GetAnalysisInfoHook()->ForceEraseAnalysisPhase(f.GetUniqueID(), &MELoopAnalysis::id);
    GetAnalysisInfoHook()->ForceRunTransFormPhase<meFuncOptTy, MeFunction>(&MELoopCanon::id, f);
  }
  // Run vrp twice when need check boundary and nullable pointer.
  if (MeOption::boundaryCheckMode != kNoCheck || MeOption::npeCheckMode != kNoCheck) {
    auto *hook = GetAnalysisInfoHook();
    dom = static_cast<MEDominance*>(
        hook->ForceRunAnalysisPhase<MapleFunctionPhase<MeFunction>>(&MEDominance::id, f))->GetResult();
    meLoop = static_cast<MELoopAnalysis*>(
        hook->ForceRunAnalysisPhase<MapleFunctionPhase<MeFunction>>(&MELoopAnalysis::id, f))->GetResult();
    ValueRangePropagation valueRangePropagationWithOPTAssert(
        f, *irMap, *dom, meLoop, *valueRangeMemPool, cands, sa, true);
    SafetyCheckWithBoundaryError safetyCheckBoundaryError(f, valueRangePropagationWithOPTAssert);
    SafetyCheckWithNonnullError safetyCheckNonnullError(f);
    if (MeOption::boundaryCheckMode == kNoCheck) {
      valueRangePropagationWithOPTAssert.SetSafetyBoundaryCheck(safetyCheck);
    } else {
      valueRangePropagationWithOPTAssert.SetSafetyBoundaryCheck(safetyCheckBoundaryError);
    }
    if (MeOption::npeCheckMode == kNoCheck || f.vrpRuns == 1) {
      valueRangePropagationWithOPTAssert.SetSafetyNonnullCheck(safetyCheck);
    } else {
      valueRangePropagationWithOPTAssert.SetSafetyNonnullCheck(safetyCheckNonnullError);
    }
    valueRangePropagationWithOPTAssert.Execute();
    CHECK_FATAL(!valueRangePropagationWithOPTAssert.IsCFGChange(), "must not change the cfg!");
  }
  if (ValueRangePropagation::isDebug) {
    LogInfo::MapleLogger() << "***************after value range prop***************" << "\n";
    f.Dump(false);
    f.GetCfg()->DumpToFile("valuerange-after");
  }
  if (DEBUGFUNC_NEWPM(f)) {
    LogInfo::MapleLogger() << "\n============== After boundary check optimization  =============" << "\n";
    irMap->Dump();
  }
  return true;
}
}  // namespace maple
