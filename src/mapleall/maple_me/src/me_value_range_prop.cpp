/*
 * Copyright (c) [2021-2022] Huawei Technologies Co.,Ltd.All rights reserved.
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
static constexpr int32 kProbLikely = 9000;
static constexpr int32 kProbUnlikely = 1000;

bool Bound::CanBeComparedWith(const Bound &bound) const {
  // Only bounds with same var can be compared
  // Only comparison between bounds with same primtype makes sense!
  if (primType != bound.GetPrimType()) {
    return false;
  }
  if (var == bound.GetVar()) {
    return var == nullptr ||
        // If the var of bounds are equal, but one of them is open and the other is not open,
        // the bounds can not be compared.
        // Such as:
        // condition1: a < b      => VR(a): [min, b - 1]
        // condition2: a <= b - 1 => VR(a): [min, b - 1]
        isClosedInterval == bound.IsClosedInterval();
  }
  // If a bound is min or max and the other bound is not closed, the bounds can not be compared.
  // Such as:(the prim types of var a and var b are unsigned)
  // condition1: a == 0   => VR(a): [0, 0]
  // condition2: a < b    => VR(a): [0, b - 1]
  return (((*this == MinBound(primType) || *this == MaxBound(primType)) && bound.IsClosedInterval()) ||
      ((bound == MinBound(primType) || bound == MaxBound(primType)) && this->IsClosedInterval()));
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
  if (IsPrimitiveUnsigned(primType)) {
    if (var == nullptr && bound.var == nullptr) {
      return static_cast<uint64>(constant) < static_cast<uint64>(bound.GetConstant());
    }
  }
  return constant < bound.GetConstant();
}

bool Bound::operator<=(const Bound &bound) const {
  if (!CanBeComparedWith(bound)) {
    return false;
  }
  if (*this == bound) {
    return true;
  }
  if (*this == MinBound(primType) || bound == MaxBound(primType)) {
    return true;
  }
  if (IsPrimitiveUnsigned(primType)) {
    if (var == nullptr && bound.var == nullptr) {
      return static_cast<uint64>(constant) <= static_cast<uint64>(bound.GetConstant());
    }
  }
  return (var == bound.var && constant <= bound.GetConstant());
}

// When the value range is converted between different primtypes,
// need to determine whether the range overflows or underflows after conversion.
ValueRange *ValueRangePropagation::GetVRAfterCvt(ValueRange &vr, PrimType pty) const {
  auto ptyOfVR = vr.GetPrimType();
  if (pty == ptyOfVR) {
    return &vr;
  }
  if (!vr.GetUpper().IsEqualAfterCVT(ptyOfVR, pty) ||
      !vr.GetLower().IsEqualAfterCVT(ptyOfVR, pty)) {
    return nullptr;
  }
  auto ptyIsUnsigned = IsUnsignedInteger(pty);
  auto ptyOfVRIsUnsigned = IsUnsignedInteger(ptyOfVR);
  auto sizeOfVRPty = GetPrimTypeSize(ptyOfVR);
  auto sizeOfPty = GetPrimTypeSize(pty);
  if (ptyIsUnsigned == ptyOfVRIsUnsigned) {
    if (sizeOfVRPty <= sizeOfPty) {
      return &vr;
    }
    if (vr.GetUpper().IsGreaterThan(Bound(GetMaxNumber(pty), pty), ptyOfVR) ||
        vr.GetLower().IsLessThan(Bound(GetMinNumber(pty), pty), ptyOfVR)) {
      // Overflows or underflows after conversion.
      // for example:
      // ptyOfVR: PTY_u64
      // pty: PTY_u32
      // vr(8, max)
      return nullptr;
    }
    return &vr;
  }
  if (ptyOfVRIsUnsigned) {
    if (sizeOfVRPty <= sizeOfPty) {
      return &vr;
    }
    if (vr.GetUpper().IsGreaterThan(Bound(GetMaxNumber(pty), pty), ptyOfVR)) {
      // Overflows or underflows after conversion.
      // for example:
      // ptyOfVR: PTY_u32
      // pty: PTY_i16
      // vr(0, max)
      return nullptr;
    }
    return &vr;
  }
  if (ptyIsUnsigned) {
    auto comparePty = (sizeOfPty >= sizeOfVRPty) ? pty : ptyOfVR;
    if (vr.GetUpper().IsGreaterThan(Bound(GetMaxNumber(pty), pty), comparePty) ||
        vr.GetLower().IsLessThan(Bound(GetMinNumber(pty), pty), comparePty)) {
      // Overflows or underflows after conversion.
      // for example:
      // ptyOfVR: PTY_i32
      // pty: PTY_u16
      // vr(min, 8)
      return nullptr;
    }
    return &vr;
  }
  return nullptr;
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
    auto curBB = func.GetCfg()->GetBBFromID(BBId((*bIt)->GetID()));
    if (unreachableBBs.find(curBB) != unreachableBBs.end()) {
      continue;
    }
    dealWithPhi = true;
    DealWithPhi(*curBB);
    dealWithPhi = false;
    if (func.GetCfg()->UpdateCFGFreq()) {
      // update edge frequency
      curBB->UpdateEdgeFreqs(false);
    }
    for (auto it = curBB->GetMeStmts().begin(); it != curBB->GetMeStmts().end();) {
      bool deleteStmt = false;
      if (!onlyPropVR) {
        ReplaceOpndWithConstMeExpr(*curBB, *it);
      }
      if (it->GetLHS() != nullptr && !CanCreateVRForExpr(*it->GetLHS())) {
        ++it;
        continue;
      }
      bool isNotNeededPType = false;
      bool hasVolatileOperand = false;
      for (size_t i = 0; i < it->NumMeStmtOpnds(); ++i) {
        if (it->GetOpnd(i)->ContainsVolatile()) {
          hasVolatileOperand = true;
          break;
        }
        if (!IsNeededPrimType(it->GetOpnd(i)->GetPrimType())) {
          isNotNeededPType = true;
          break;
        }
        DealWithOperand(*curBB, *it, *it->GetOpnd(i));
      }
      if (isNotNeededPType || hasVolatileOperand) {
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
          if (it->GetLHS() != nullptr && !CanCreateVRForExpr(*it->GetLHS())) {
            break;
          }
          DealWithAssign(*curBB, *it);
          break;
        }
        case OP_brfalse:
        case OP_brtrue: {
          DealWithCondGoto(*curBB, *it);
          InsertValueRangeOfCondExpr2Caches(*curBB, *it);
          break;
        }
        CASE_OP_ASSERT_NONNULL {
          if (!dealWithAssert) {
            break;
          }
          // If the option safeRegion is open and the stmt is not in safe region, delete it.
          if (MeOption::safeRegionMode && !it->IsInSafeRegion()) {
            deleteStmt = true;
          } else if (DealWithAssertNonnull(*curBB, *it)) {
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
          if (!dealWithAssert) {
            break;
          }
          // If the option safeRegion is open and the stmt is not in safe region, delete it.
          if (MeOption::safeRegionMode && !it->IsInSafeRegion()) {
            deleteStmt = true;
          } else if (DealWithBoundaryCheck(*curBB, *it)) {
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
          DealWithCallassigned(*curBB, *it);
          break;
        }
        case OP_switch: {
          DealWithSwitch(*curBB, *it);
          break;
        }
        case OP_igoto:
        default:
          break;
      }
      if (deleteStmt) {
        curBB->GetMeStmts().erase(it++);
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

bool ValueRangePropagation::DealWithSwitchWhenOpndIsConstant(BB &bb, BB *defaultBB,
    const ValueRange *valueRange, const SwitchMeStmt &switchMeStmt) {
  if (onlyPropVR || valueRange == nullptr || valueRange->GetRangeType() != kEqual ||
      !valueRange->GetBound().IsConstantBound()) {
    return false;
  }
  bool findBB = false;
  for (auto &pair : switchMeStmt.GetSwitchTable()) {
    auto *caseBB = func.GetCfg()->GetLabelBBAt(pair.second);
    CHECK_FATAL(caseBB, "caseBB is nullptr!");
    // The value ranges of opnd and case are equal.
    if (valueRange->GetBound().GetPrimType() == PTY_u1) {
      // If the opnd of the switch is of type bool, there is no need to convert the primtype of the case,
      // and the values are directly compared for equality.
      if (valueRange->GetBound().GetConstant() == pair.first) {
        findBB = true;
        continue;
      }
    } else if (valueRange->GetBound().IsEqual(Bound(nullptr, pair.first, PTY_i64),
        switchMeStmt.GetOpnd()->GetPrimType())) {
      findBB = true;
      continue;
    }
    // When case bb and default bb are the same bb, case bb cannot be removed when case bb is not the target bb.
    if (caseBB == defaultBB) {
      continue;
    }
    bb.RemoveSucc(*caseBB);
  }
  // If the value ranges of opnd and one of cases are equal, the defaultBB is unreachable.
  if (findBB && defaultBB != nullptr) {
    bb.RemoveSucc(*defaultBB);
  }
  bb.RemoveLastMeStmt();
  bb.SetKind(kBBFallthru);
  isCFGChange = true;
  if (bb.GetAttributes() == 0 && bb.GetSucc().size() != 1) {
    ASSERT(false, "must only has one succ bb");
  }
  return true;
}

void ValueRangePropagation::DealWithSwitch(BB &bb, MeStmt &stmt) {
  auto &switchMeStmt = static_cast<SwitchMeStmt&>(stmt);
  auto *opnd = switchMeStmt.GetOpnd();
  auto *defaultBB = func.GetCfg()->GetLabelBBAt(switchMeStmt.GetDefaultLabel());
  auto *valueRange = FindValueRangeAndInitNumOfRecursion(bb, *opnd);
  if (DealWithSwitchWhenOpndIsConstant(bb, defaultBB, valueRange, switchMeStmt)) {
    return;
  }
  std::set<BBId> bbOfCases;
  if (defaultBB != nullptr) {
    bbOfCases.insert(defaultBB->GetBBId());
  }
  std::vector<int64> valueOfCase;
  for (auto &pair : switchMeStmt.GetSwitchTable()) {
    valueOfCase.push_back(pair.first);
    auto *currbb = func.GetCfg()->GetLabelBBAt(pair.second);
    CHECK_FATAL(currbb, "currbb is nullptr!");
    // Can prop value range to target bb only when the pred size of target bb is one and
    // only one case can jump to the target bb.
    if (currbb->GetPred().size() == 1 && bbOfCases.insert(currbb->GetBBId()).second) {
      (void)Insert2Caches(
          currbb->GetBBId(), opnd->GetExprID(),
          std::make_unique<ValueRange>(Bound(nullptr, pair.first, PTY_i64), kEqual));
    } else {
      (void)Insert2Caches(currbb->GetBBId(), opnd->GetExprID(), nullptr);
    }
  }
  if (onlyPropVR || defaultBB == nullptr) {
    return;
  }
  // Delete the default branch when it is unreachable.
  auto *currBB = &bb;
  while (valueRange == nullptr && currBB->GetPred().size() == 1) {
    currBB = currBB->GetPred(0);
    valueRange = FindValueRangeAndInitNumOfRecursion(*currBB, *opnd);
  }
  if (valueRange == nullptr || !valueRange->IsConstantRange()) {
    return;
  }
  auto upper = valueRange->GetUpper().GetConstant();
  auto lower = valueRange->GetLower().GetConstant();
  int64 res = 0;
  if (!AddOrSubWithConstant(PTY_i64, OP_sub, upper, lower, res) ||
      !AddOrSubWithConstant(PTY_i64, OP_add, res, 1, res)) {
    return;
  }
  if (upper < lower || static_cast<uint64>(res) != valueOfCase.size()) {
    return;
  }
  std::sort(valueOfCase.begin(), valueOfCase.end());
  if (valueOfCase.front() == lower && valueOfCase.back() == upper) {
    if (ValueRangePropagation::isDebug) {
      LogInfo::MapleLogger() << "===========delete defaultBranch " << defaultBB->GetBBId() << "===========\n";
    }
    switchMeStmt.SetDefaultLabel(0);
    defaultBB->RemovePred(bb);
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
            return intersect ? std::make_unique<ValueRange>(kRTEmpty) : nullptr; // always false
          } else if (upper2 == lower1) {
            return intersect ? std::make_unique<ValueRange>(lower1, kEqual)
                             : std::make_unique<ValueRange>(lower2, upper1, kLowerAndUpper);
          } else if (upper1 == lower2) {
            return intersect ? std::make_unique<ValueRange>(lower2, kEqual)
                             : std::make_unique<ValueRange>(lower1, upper2, kLowerAndUpper);
          } else if (lower1 < upper2 && lower2 < upper1) {
            return intersect ?
                std::make_unique<ValueRange>(std::max(lower1, lower2), std::min(upper1, upper2), kLowerAndUpper) :
                std::make_unique<ValueRange>(std::min(lower1, lower2), std::max(upper1, upper2), kLowerAndUpper);
          }
          return nullptr;
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
          } else if (bound < lower1 || upper1 < bound) {
            return intersect ? std::make_unique<ValueRange>(kRTEmpty) // always false
                             : nullptr; // can not merge
          }
          return nullptr;
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
          } else if (bound == upper1) {
            return intersect ? std::make_unique<ValueRange>(lower1, --upper1, kLowerAndUpper)
                             : std::make_unique<ValueRange>(kRTComplete); // always true;
          }
          return nullptr;
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
          } else if (b1 < b2 || b2 < b1) {
            return intersect ? std::make_unique<ValueRange>(kRTEmpty) : nullptr;
          }
          return nullptr;
        }
        case kNotEqual: {
          if (b1 == b2) {
            return intersect ? std::make_unique<ValueRange>(kRTEmpty) : std::make_unique<ValueRange>(kRTComplete);
          } else if (b1 < b2 || b2 < b1) {
            return intersect ? std::make_unique<ValueRange>(b1, kEqual) : std::make_unique<ValueRange>(b2, kNotEqual);
          }
          return nullptr;
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
          } else if (b1 < b2 || b2 < b1) {
            return intersect ? nullptr : std::make_unique<ValueRange>(kRTComplete);
          }
          return nullptr;
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
  MeExpr *var = b.GetVar();
  PrimType ptyp = b.GetPrimType();
  MeExpr *constExpr = irmap->CreateIntConstMeExpr(val, ptyp);
  MeExpr *resExpr = nullptr;
  if (var == nullptr) {
    resExpr = irmap->CreateMeExprBinary(op, PTY_u1, expr, *constExpr);
  } else {
    if (val == 0) {
      resExpr = irmap->CreateMeExprBinary(op, PTY_u1, expr, *var);
    } else {
      MeExpr *addExpr = irmap->CreateMeExprBinary(OP_add, ptyp, *var, *constExpr);
      static_cast<OpMeExpr*>(addExpr)->SetOpndType(ptyp);
      resExpr = irmap->CreateMeExprBinary(op, PTY_u1, expr, *addExpr);
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
        // expr < upper || expr <= upper
        return upper.IsClosedInterval() ? GetCmpExprFromBound(upper, expr, irmap, OP_le) :
            GetCmpExprFromBound(upper, expr, irmap, OP_lt);
      } else if (upper == Bound::MaxBound(upper.GetPrimType())) {
        // lower < expr || lower <= expr
        return lower.IsClosedInterval() ? GetCmpExprFromBound(lower, expr, irmap, OP_ge) :
            GetCmpExprFromBound(lower, expr, irmap, OP_gt);
      } else {
        // lower <=(<) expr <=(<) upper
        MeExpr *upperExpr = upper.IsClosedInterval() ? GetCmpExprFromBound(upper, expr, irmap, OP_le) :
            GetCmpExprFromBound(upper, expr, irmap, OP_lt);
        MeExpr *lowerExpr = lower.IsClosedInterval() ? GetCmpExprFromBound(lower, expr, irmap, OP_ge) :
            GetCmpExprFromBound(lower, expr, irmap, OP_gt);
        return irmap->CreateMeExprBinary(OP_land, PTY_u1, *upperExpr, *lowerExpr);
      }
    }
    case kOnlyHasLowerBound: {
      Bound lower = vr->GetLower();
      // expr > val || expr >= val
      return lower.IsClosedInterval() ? GetCmpExprFromBound(lower, expr, irmap, OP_ge) :
          GetCmpExprFromBound(lower, expr, irmap, OP_gt);
    }
    case kOnlyHasUpperBound: {
      Bound upper = vr->GetUpper();
      // expr < val || expr <= val
      return upper.IsClosedInterval() ? GetCmpExprFromBound(upper, expr, irmap, OP_le) :
          GetCmpExprFromBound(upper, expr, irmap, OP_lt);
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

// DealWith this case
// a: valueRange(0, constant) || valueRange(constant, 0)
// ==>
// a: valueRange(1, constant) || valueRange(constant, -1)
std::unique_ptr<ValueRange> ValueRangePropagation::ZeroIsInRange(const ValueRange &valueRange) const {
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
  auto *valueRangeOfOpnd = FindValueRangeAndInitNumOfRecursion(bb, opnd);
  if (valueRangeOfOpnd != nullptr) {
    return CopyValueRange(*valueRangeOfOpnd);
  }
  return nullptr;
}

void SafetyCheckWithBoundaryError::HandleAssignWithDeadBeef(
    BB &bb, MeStmt &meStmt, MeExpr &indexOpnd,
    MeExpr &boundOpnd) {
  std::set<MeExpr*> visitedLHS;
  std::vector<MeStmt*> stmts{ &meStmt };
  bool crossPhiNode = false;
  vrp.JudgeTheConsistencyOfDefPointsOfBoundaryCheck(bb, indexOpnd, visitedLHS, stmts, crossPhiNode);
  visitedLHS.clear();
  stmts.clear();
  stmts.push_back(&meStmt);
  crossPhiNode = false;
  vrp.JudgeTheConsistencyOfDefPointsOfBoundaryCheck(bb, boundOpnd, visitedLHS, stmts, crossPhiNode);
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
    const MeStmt &meStmt, const ValueRange &valueRangeOfIndex, ValueRange &valueRangeOfLengthPtr, Opcode op) const {
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
      static_cast<uint64>(static_cast<const ConstMeExpr&>(boundOpnd).GetExtIntValue()) == kInvaliedBound) {
    if (updateCaches) {
      Insert2AnalysisedArrayChecks(bb.GetBBId(), *meStmt->GetOpnd(1), *meStmt->GetOpnd(0), meStmt->GetOp());
    }
    return true;
  } else {
    auto *valueRange = FindValueRangeAndInitNumOfRecursion(bb, boundOpnd);
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
// if
//    p = GetBoundaryPtr
// else
//    p = GetBoundarylessPtr
// error: p + i
void ValueRangePropagation::JudgeTheConsistencyOfDefPointsOfBoundaryCheck(
    BB &bb, MeExpr &expr, std::set<MeExpr*> &visitedLHS, std::vector<MeStmt*> &stmts, bool &crossPhiNode) {
  if (TheValueOfOpndIsInvaliedInABCO(bb, nullptr, expr, false)) {
    std::string errorLog = "";
    if (!crossPhiNode) {
      if (stmts[0]->IsInSafeRegion() &&
          (stmts[0]->GetOp() == OP_calcassertge || stmts[0]->GetOp() == OP_calcassertlt)) {
        errorLog += "error: the pointer has no boundary info.\n";
      } else {
        bb.RemoveMeStmt(stmts[0]);
        return;
      }
    } else {
      errorLog += "error: pointer assigned from multibranch requires the boundary info for all branches.\n";
    }
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
      JudgeTheConsistencyOfDefPointsOfBoundaryCheck(bb, *expr.GetOpnd(i), visitedLHS, stmts, crossPhiNode);
    }
    return;
  }
  ScalarMeExpr &scalar = static_cast<ScalarMeExpr&>(expr);
  if (scalar.GetDefBy() == kDefByStmt) {
    stmts.push_back(scalar.GetDefStmt());
    if (visitedLHS.find(scalar.GetDefStmt()->GetRHS()) != visitedLHS.end()) {
      return;
    }
    JudgeTheConsistencyOfDefPointsOfBoundaryCheck(bb, *scalar.GetDefStmt()->GetRHS(), visitedLHS, stmts, crossPhiNode);
    stmts.pop_back();
    visitedLHS.insert(scalar.GetDefStmt()->GetRHS());
  } else if (scalar.GetDefBy() == kDefByPhi) {
    crossPhiNode = true;
    MePhiNode &phi = scalar.GetDefPhi();
    if (visitedLHS.find(phi.GetLHS()) != visitedLHS.end()) {
      return;
    }
    visitedLHS.insert(phi.GetLHS());
    for (auto &opnd : phi.GetOpnds()) {
      JudgeTheConsistencyOfDefPointsOfBoundaryCheck(bb, *opnd, visitedLHS, stmts, crossPhiNode);
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
    auto *defBBOfOpnd0 = defPhi.GetOpnd(0)->GetDefByBBMeStmt(*func.GetCfg(), defStmt);
    auto *defBBOfOpnd1 = defPhi.GetOpnd(1)->GetDefByBBMeStmt(*func.GetCfg(), defStmt);
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
      auto *valueRange = FindValueRangeAndInitNumOfRecursion(bb, opnd);
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
        AddOrSubWithValueRange(pTypeOfArray, OP_add, *resValueRange, constant);
    return;
  }
  // Deal with the operand which is a meexpr.
  if (opndOfCRNode.GetExpr() == nullptr) {
    resValueRange = nullptr;
    return;
  }
  auto valueRangOfOpnd = FindValueRangeInCurrBBOrDominateBBs(bb, *opndOfCRNode.GetExpr());
  if (valueRangOfOpnd == nullptr || valueRangOfOpnd->IsUniversalSet()) {
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

template<typename T>
bool ValueRangePropagation::IsOverflowAfterMul(T lhs, T rhs, PrimType pty) const {
  if (!IsNeededPrimType(pty)) {
    return true;
  }
  if (lhs == 0 || rhs == 0) {
    return false;
  }
  if (lhs > 0 && rhs > 0) {
    return (GetMaxNumber(pty) / lhs) < rhs;
  }
  if (lhs < 0 && rhs < 0) {
    return (GetMaxNumber(pty) / lhs) > rhs;
  }
  if (lhs * rhs == GetMinNumber(pty)) {
    return false;
  }
  return (lhs > 0) ? (GetMinNumber(pty) / lhs) > rhs : (GetMinNumber(pty) / rhs) > lhs;
}

bool ValueRangePropagation::DealWithMulNode(const BB &bb, CRNode &opndOfCRNode,
    std::unique_ptr<ValueRange> &resValueRange, PrimType pTypeOfArray) {
  auto &mulCRNode = static_cast<CRMulNode&>(opndOfCRNode);
  if (mulCRNode.GetOpndsSize() != kNumOperands) {
    return false;
  }
  auto *lhs = mulCRNode.GetOpnd(0);
  auto *rhs = mulCRNode.GetOpnd(1);
  if (lhs->GetCRType() != kCRConstNode || rhs->GetCRType() != kCRVarNode || rhs->GetExpr() == nullptr) {
    return false;
  }
  auto valueRangOfOpnd = FindValueRangeInCurrBBOrDominateBBs(bb, *rhs->GetExpr());
  if (valueRangOfOpnd == nullptr || !valueRangOfOpnd->IsConstantLowerAndUpper() ||
      !valueRangOfOpnd->IsAccurate()) {
    return false;
  }
  int64 constant = static_cast<CRConstNode*>(lhs)->GetConstValue();
  auto lowerConst = valueRangOfOpnd->GetLower().GetConstant();
  auto upperConst = valueRangOfOpnd->GetUpper().GetConstant();
  if (pTypeOfArray == PTY_u64) {
    if (IsOverflowAfterMul<uint64>(lowerConst, constant, pTypeOfArray) ||
        IsOverflowAfterMul<uint64>(upperConst, constant, pTypeOfArray)) {
      return false;
    }
  } else {
    if (IsOverflowAfterMul(lowerConst, constant, pTypeOfArray) ||
        IsOverflowAfterMul(upperConst, constant, pTypeOfArray)) {
      return false;
    }
  }
  auto vrAfterMul = std::make_unique<ValueRange>(Bound(lowerConst * constant, pTypeOfArray),
      Bound(upperConst * constant, pTypeOfArray), kLowerAndUpper, true);
  resValueRange = (resValueRange == nullptr) ? std::move(vrAfterMul) :
      AddOrSubWithValueRange(OP_add, *resValueRange, *vrAfterMul);
  return true;
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
    switch (opndOfCRNode->GetCRType()) {
      case kCRVarNode:
      case kCRConstNode: {
        GetValueRangeOfCRNode(bb, *opndOfCRNode, resValueRange, pTypeOfArray);
        break;
      }
      case kCRAddNode: {
        auto *addCRNOde = static_cast<CRAddNode*>(opndOfCRNode);
        for (size_t addNodeIndex = 0; addNodeIndex < addCRNOde->GetOpndsSize(); ++addNodeIndex) {
          GetValueRangeOfCRNode(bb, *addCRNOde->GetOpnd(addNodeIndex), resValueRange, pTypeOfArray);
          if (resValueRange == nullptr) {
            return nullptr;
          }
        }
        break;
      }
      case kCRMulNode: {
        if (!DealWithMulNode(bb, *opndOfCRNode, resValueRange, pTypeOfArray)) {
          return nullptr;
        }
        break;
      }
      default:
        return nullptr;
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
  std::vector<CRNode*> tmpIndexVector(indexVector);
  std::vector<CRNode*> tmpBoundVector(boundVector);
  if (sa.NormalizationWithByteCount(tmpIndexVector, byteSize) &&
      sa.NormalizationWithByteCount(tmpBoundVector, byteSize)) {
    indexVector = tmpIndexVector;
    boundVector = tmpBoundVector;
  }
  if (ValueRangePropagation::isDebug) {
    std::for_each(indexVector.begin(), indexVector.end(), [this](const CRNode *cr) {
        sa.Dump(*cr);
        std::cout << " ";});
    LogInfo::MapleLogger() << "\n";
    std::for_each(boundVector.begin(), boundVector.end(), [this](const CRNode *cr) {
        sa.Dump(*cr);
        std::cout << " ";});
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
  const std::string &fileName = func->GetMIRModule().GetFileNameFromFileNum(srcPosition.FileNum());
  switch (stmt.GetOp()) {
    case OP_calcassertlt:
    case OP_calcassertge: {
      auto &newStmt = static_cast<const AssertBoundaryMeStmt &>(stmt);
      GStrIdx curFuncNameIdx = func->GetMirFunc()->GetNameStrIdx();
      GStrIdx stmtFuncNameIdx = GlobalTables::GetStrTable().GetStrIdxFromName(newStmt.GetFuncName().c_str());
      if (curFuncNameIdx == stmtFuncNameIdx) {
        if (stmt.GetOp() == OP_calcassertlt) {
          WARN_USER(kLncWarn, srcPosition, func->GetMIRModule(), "the pointer >= the upper bounds after calculation");
        } else if (stmt.GetOp() == OP_calcassertge) {
          WARN_USER(kLncWarn, srcPosition, func->GetMIRModule(), "the pointer < the lower bounds after calculation");
        }
      } else {
        if (stmt.GetOp() == OP_calcassertlt) {
          WARN_USER(kLncWarn, srcPosition, func->GetMIRModule(),
              "the pointer >= the upper bounds after calculation when inlined to %s", func->GetName().c_str());
        } else if (stmt.GetOp() == OP_calcassertge) {
          WARN_USER(kLncWarn, srcPosition, func->GetMIRModule(),
              "the pointer < the lower bounds after calculation when inlined to %s", func->GetName().c_str());
        }
      }
      return !opts::enableArithCheck;
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
                  fileName.c_str(), srcPosition.LineNum());
          } else if (stmt.GetOp() == OP_returnassertnonnull) {
            FATAL(kLncFatal, "%s:%d error: %s return nonnull but got nullable pointer in safe region",
                  fileName.c_str(), srcPosition.LineNum(), newStmt.GetFuncName().c_str());
          } else {
            FATAL(kLncFatal, "%s:%d error: nullable pointer assignment of nonnull pointer in safe region",
                  fileName.c_str(), srcPosition.LineNum());
          }
        } else {
          if (stmt.GetOp() == OP_assertnonnull) {
            FATAL(kLncFatal, "%s:%d error: Dereference of null pointer", fileName.c_str(), srcPosition.LineNum());
          } else if (stmt.GetOp() == OP_returnassertnonnull) {
            FATAL(kLncFatal, "%s:%d error: %s return nonnull but got null pointer", fileName.c_str(),
                  srcPosition.LineNum(), newStmt.GetFuncName().c_str());
          } else {
            FATAL(kLncFatal, "%s:%d error: null assignment of nonnull pointer", fileName.c_str(),
                  srcPosition.LineNum());
          }
        }
      } else {
        if (isNullablePtr) {
          if (stmt.GetOp() == OP_assertnonnull) {
            FATAL(kLncFatal, "%s:%d error: Dereference of nullable pointer in safe region when inlined to %s",
                  fileName.c_str(), srcPosition.LineNum(), func->GetName().c_str());
          } else if (stmt.GetOp() == OP_returnassertnonnull) {
            FATAL(kLncFatal,
                  "%s:%d error: %s return nonnull but got nullable pointer in safe region when inlined to %s",
                  fileName.c_str(), srcPosition.LineNum(), newStmt.GetFuncName().c_str(), func->GetName().c_str());
          } else {
            FATAL(kLncFatal,
                  "%s:%d error: nullable pointer assignment of nonnull pointer in safe region when inlined to %s",
                  fileName.c_str(), srcPosition.LineNum(), func->GetName().c_str());
          }
        } else {
          if (stmt.GetOp() == OP_assertnonnull) {
            FATAL(kLncFatal, "%s:%d error: Dereference of null pointer when inlined to %s",
                  fileName.c_str(), srcPosition.LineNum(), func->GetName().c_str());
          } else if (stmt.GetOp() == OP_returnassertnonnull) {
            FATAL(kLncFatal, "%s:%d error: %s return nonnull but got null pointer when inlined to %s",
                  fileName.c_str(), srcPosition.LineNum(), newStmt.GetFuncName().c_str(), func->GetName().c_str());
          } else {
            FATAL(kLncFatal, "%s:%d error: null assignment of nonnull pointer when inlined to %s",
                  fileName.c_str(), srcPosition.LineNum(), func->GetName().c_str());
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
                "for %s argument in safe region", fileName.c_str(), srcPosition.LineNum(),
                callStmt.GetFuncName().c_str(), GetNthStr(callStmt.GetParamIndex()).c_str());
        } else {
          FATAL(kLncFatal, "%s:%d error: NULL passed to %s that requires a nonnull pointer for %s argument",
                fileName.c_str(), srcPosition.LineNum(), callStmt.GetFuncName().c_str(),
                GetNthStr(callStmt.GetParamIndex()).c_str());
        }
      } else {
        if (isNullablePtr) {
          FATAL(kLncFatal, "%s:%d error: nullable pointer passed to %s that requires a nonnull pointer "\
                "for %s argument in safe region when inlined to %s", fileName.c_str(), srcPosition.LineNum(),
                callStmt.GetFuncName().c_str(), GetNthStr(callStmt.GetParamIndex()).c_str(), func->GetName().c_str());
        } else {
          FATAL(kLncFatal, "%s:%d error: NULL passed to %s that requires a nonnull pointer for %s argument "\
                "when inlined to %s", fileName.c_str(), srcPosition.LineNum(), callStmt.GetFuncName().c_str(),
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
                fileName.c_str(), srcPosition.LineNum());
        } else if (stmt.GetOp() == OP_assertge) {
          FATAL(kLncFatal, "%s:%d error: the pointer < the lower bounds when accessing the memory",
                fileName.c_str(), srcPosition.LineNum());
        } else if (stmt.GetOp() == OP_assignassertle) {
          FATAL(kLncFatal, "%s:%d error: l-value boundary should not be larger than r-value boundary",
                fileName.c_str(), srcPosition.LineNum());
        } else {
          FATAL(kLncFatal, "%s:%d error: return value's bounds does not match the function declaration for %s",
                fileName.c_str(), srcPosition.LineNum(), newStmt.GetFuncName().c_str());
        }
      } else {
        if (stmt.GetOp() == OP_assertlt) {
          FATAL(kLncFatal, "%s:%d error: the pointer >= the upper bounds when accessing the memory and inlined to %s",
                fileName.c_str(), srcPosition.LineNum(), func->GetName().c_str());
        } else if (stmt.GetOp() == OP_assertge) {
          FATAL(kLncFatal, "%s:%d error: the pointer < the lower bounds when accessing the memory and inlined to %s",
                fileName.c_str(), srcPosition.LineNum(), func->GetName().c_str());
        } else if (stmt.GetOp() == OP_assignassertle) {
          FATAL(kLncFatal,
                "%s:%d error: l-value boundary should not be larger than r-value boundary when inlined to %s",
                fileName.c_str(), srcPosition.LineNum(), func->GetName().c_str());
        } else {
          FATAL(kLncFatal,
                "%s:%d error: return value's bounds does not match the function declaration for %s when inlined to %s",
                fileName.c_str(), srcPosition.LineNum(), newStmt.GetFuncName().c_str(), func->GetName().c_str());
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
              "for the %s argument when inlined to %s", fileName.c_str(), srcPosition.LineNum(),
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
  auto *valueRange = FindValueRangeAndInitNumOfRecursion(bb, *opnd);
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
      auto *predValueRange = FindValueRangeAndInitNumOfRecursion(*pred, *opnd);
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
  auto *valueRangeOfOperand = FindValueRangeAndInitNumOfRecursion(bb, meExpr);
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

// Create VR for bitSize like
// bitsSize : 12
// =>
// vr: (0, 0xfff)
void ValueRangePropagation::CreateVRWithBitsSize(const BB &bb, const OpMeExpr &opMeExpr) {
  auto bitsSize = opMeExpr.GetBitsSize();
  constexpr uint8_t k64Bits = 64;
  if (bitsSize < k64Bits) {
    auto pTypeOfOpMeExpr = opMeExpr.GetPrimType();
    uint64 maxNumber = (1ULL << bitsSize) - 1;
    (void)Insert2Caches(bb.GetBBId(), opMeExpr.GetExprID(), std::make_unique<ValueRange>(
        Bound(nullptr, 0, pTypeOfOpMeExpr),
        Bound(maxNumber, pTypeOfOpMeExpr), kLowerAndUpper));
  }
}

void ValueRangePropagation::DealWithOperand(BB &bb, MeStmt &stmt, MeExpr &meExpr) {
  if (!IsNeededPrimType(meExpr.GetPrimType())) {
    return;
  }
  switch (meExpr.GetMeOp()) {
    case kMeOpConst: {
      if (static_cast<ConstMeExpr&>(meExpr).GetConstVal()->GetKind() == kConstInt) {
        (void)Insert2Caches(bb.GetBBId(), meExpr.GetExprID(), std::make_unique<ValueRange>(
            Bound(nullptr, static_cast<ConstMeExpr&>(meExpr).GetExtIntValue(), meExpr.GetPrimType()), kEqual));
      }
      break;
    }
    case kMeOpIvar: {
      auto &ivarMeExpr = static_cast<IvarMeExpr&>(meExpr);
      // prop value range of base
      auto *base = ivarMeExpr.GetBase();
      auto *valueRange = FindValueRangeAndInitNumOfRecursion(bb, *base);
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
      auto filedPTy = ivarMeExpr.GetType()->GetPrimType();
      auto ivarPTy = ivarMeExpr.GetPrimType();
      if (GetPrimTypeSize(filedPTy) < GetPrimTypeSize(ivarPTy) &&
          IsPrimitiveUnsigned(filedPTy) == IsPrimitiveUnsigned(ivarPTy)) {
        // There is an implicit conversion here, for example, field's primtype is u16 and ivar's primtype is u32.
        (void)Insert2Caches(bb.GetBBId(), ivarMeExpr.GetExprID(), std::make_unique<ValueRange>(Bound(nullptr,
            GetMinNumber(filedPTy), ivarPTy), Bound(nullptr, GetMaxNumber(filedPTy), ivarPTy), kLowerAndUpper));
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
      auto *vrOfOpMeExpr = FindValueRangeAndInitNumOfRecursion(bb, meExpr);
      if (vrOfOpMeExpr != nullptr) {
        break;
      }
      auto &opMeExpr = static_cast<OpMeExpr&>(meExpr);
      switch (opMeExpr.GetOp()) {
        case OP_select: {
          return;
        }
        case OP_cvt: {
          if (DealWithCVT(bb, stmt, static_cast<OpMeExpr&>(meExpr))) {
            return;
          }
          break;
        }
        case OP_zext: {
          // zext <unsigned-int-type> <bsize> (<opnd>)
          auto *opnd = opMeExpr.GetOpnd(0);
          auto pTypeOpnd = opnd->GetPrimType();
          auto *valueRange = FindValueRangeAndInitNumOfRecursion(bb, *opnd);
          if (valueRange != nullptr && valueRange->GetRangeType() != kNotEqual &&
              (IsPrimitiveUnsigned(pTypeOpnd) || valueRange->IsGreaterThanOrEqualToZero())) {
            auto bSize = opMeExpr.GetBitsSize();
            constexpr uint8_t k64Bits = 64;
            if (bSize < k64Bits) {
              uint64 maxNumber = (1ULL << bSize) - 1;
              // Judge whether the truncated range is the same as the previous range.
              if (valueRange->IsEqualAfterCVT(valueRange->GetLower().GetPrimType(), pTypeOpnd) &&
                  valueRange->GetUpper().IsLessThanOrEqualTo(Bound(maxNumber, PTY_u64), PTY_u64)) {
                (void) Insert2Caches(bb.GetBBId(), opMeExpr.GetExprID(), CopyValueRange(*valueRange));
                auto vrPTy = valueRange->GetPrimType();
                if (GetRealValue(valueRange->GetLower().GetConstant(), vrPTy) == 0 &&
                    static_cast<uint64>(GetRealValue(valueRange->GetUpper().GetConstant(), vrPTy)) == maxNumber) {
                  // If the vr of opnd before and after zext is equal and is the complete set
                  // of the corresponding primtype's range, then opnd and opmeexpr are considered equivalent exprs.
                  // Such as:
                  // ||MEIR|| regassign REGINDX:7 u32 %7 mx187
                  //          rhs = IVAR mx178 u32 TYIDX:198<* <$TbmFwdTbl>> (field)3
                  //            base = REGINDX:6 a64 %6 mx176
                  //            - MU: {}
                  // ||MEIR|| regassign REGINDX:8 i32 %8 mx196
                  //          rhs = OP zext i32 16 kPtyInvalid mx188
                  //            opnd[0] = REGINDX:7 u32 %7 mx187
                  // Primtype of ivar's field 3 is u16, ivar's primtype is u32,
                  // so mx187 and mx188 are considered equivalent exprs.
                  Insert2PairOfExprs(opMeExpr, *opnd, bb);
                  Insert2PairOfExprs(*opnd, opMeExpr, bb);
                }
                break;
              }
            }
          }
        }
        [[clang::fallthrough]];
        case OP_extractbits: {
          // extractbits <int-type> <boffset> <bsize> (<opnd>)
          // Deal with the case like :
          // zext u64 12 u32 (<opnd>) or extractbits u64 13 12 (<opnd>):
          // vr: (0, 0xfff)
          if (opMeExpr.GetOp() == OP_zext ||
              (opMeExpr.GetOp() == OP_extractbits && IsPrimitiveUnsigned(opMeExpr.GetPrimType()))) {
            CreateVRWithBitsSize(bb, opMeExpr);
          }
          break;
        }
        case OP_iaddrof: {
          auto *mirType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(opMeExpr.GetTyIdx());
          CHECK_FATAL(mirType->IsMIRPtrType(), "must be pointer type");
          auto *pointerType = static_cast<MIRPtrType*>(mirType);
          auto *opnd = opMeExpr.GetOpnd(0);
          auto *valueRange = FindValueRangeAndInitNumOfRecursion(bb, *opnd);
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
          (void)Insert2Caches(bb.GetBBId(), opMeExpr.GetExprID(), DealWithAddOrSub(bb, opMeExpr));
          break;
        }
        case OP_band: {
          auto opnd0 = opMeExpr.GetOpnd(0);
          GetVROfBandOpnd(bb, opMeExpr, *opnd0) || GetVROfBandOpnd(bb, opMeExpr, *opnd0);
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
      auto *valueRangeOfAddrof = FindValueRangeAndInitNumOfRecursion(bb, meExpr);
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

bool ValueRangePropagation::GetVROfBandOpnd(const BB &bb, const OpMeExpr &opMeExpr, MeExpr &opnd) {
  auto opMeExprPTy = opMeExpr.GetPrimType();
  auto vrOfOpnd0 = FindValueRangeAndInitNumOfRecursion(bb, opnd);
  if (vrOfOpnd0 != nullptr && vrOfOpnd0->IsConstant(true) &&
      vrOfOpnd0->IsEqualAfterCVT(vrOfOpnd0->GetPrimType(), opMeExprPTy) &&
      vrOfOpnd0->GetLower().IsGreaterThanOrEqualTo(Bound(nullptr, 0, opMeExprPTy), opMeExprPTy)) {
    (void)Insert2Caches(bb.GetBBId(), opMeExpr.GetExprID(), std::make_unique<ValueRange>(
        Bound(nullptr, 0, opMeExprPTy), Bound(vrOfOpnd0->GetLower().GetConstant(), opMeExprPTy), kLowerAndUpper));
    return true;
  } else if (opnd.GetMeOp() == kMeOpConst) {
    auto opndPrim = opnd.GetPrimType();
    auto constant = static_cast<ConstMeExpr&>(opnd).GetExtIntValue();
    Bound bandConst = Bound(constant, opndPrim);
    if (bandConst.IsEqualAfterCVT(opndPrim, opMeExprPTy) && bandConst.IsEqualAfterCVT(opndPrim, opMeExprPTy)) {
      (void)Insert2Caches(bb.GetBBId(), opMeExpr.GetExprID(),
          std::make_unique<ValueRange>(Bound(nullptr, 0, opMeExprPTy), Bound(constant, opMeExprPTy), kLowerAndUpper));
      return true;
    }
  }
  return false;
}

void ValueRangePropagation::DealWithNeg(const BB &bb, const OpMeExpr &opMeExpr) {
  CHECK_FATAL(opMeExpr.GetNumOpnds() == 1, "must have one opnd");
  auto *opnd = opMeExpr.GetOpnd(0);
  uint32 numberOfRecursions = 0;
  std::unordered_set<int32> foundExprs;
  auto res = NegValueRange(bb, *opnd, numberOfRecursions, foundExprs);
  (void)Insert2Caches(bb.GetBBId(), opMeExpr.GetExprID(), std::move(res));
}

// If the value of opnd of cvt would not overflow or underflow after convert, delete the cvt.
bool ValueRangePropagation::DealWithCVT(const BB &bb, MeStmt &stmt, OpMeExpr &opMeExpr) {
  CHECK_FATAL(opMeExpr.GetNumOpnds() == 1, "must have one opnd");
  auto toType = opMeExpr.GetPrimType();
  auto fromType = opMeExpr.GetOpndType();
  auto *opnd = opMeExpr.GetOpnd(0);
  if (opnd->GetOp() == OP_add || opnd->GetOp() == OP_sub) {
    auto *addOrSubExpr = static_cast<OpMeExpr*>(opnd);
    auto *opnd0 = addOrSubExpr->GetOpnd(0);
    auto *opnd1 = addOrSubExpr->GetOpnd(1);
    if (opnd0->GetMeOp() == kMeOpConst || opnd1->GetMeOp() == kMeOpConst) {
      auto *opnd0VR = FindValueRangeAndInitNumOfRecursion(bb, *opnd0);
      auto *opnd1VR = FindValueRangeAndInitNumOfRecursion(bb, *opnd1);
      if ((opnd0VR != nullptr &&
          (!opnd0VR->IsEqualAfterCVT(fromType, toType) || !opnd0VR->IsConstantLowerAndUpper())) ||
          (opnd1VR != nullptr &&
          (!opnd1VR->IsEqualAfterCVT(fromType, toType) || !opnd1VR->IsConstantLowerAndUpper()))) {
        return false;
      }
      // If the op of opnd is OP_add or OP_sub and the vr before and after cvt is the same, then fold cvt:
      // rhs = OP cvt i64 i32 mx4
      //   opnd[0] = OP add i32 kPtyInvalid mx3
      //     opnd[0] = REGINDX:7 %7 mx1
      //     opnd[1] = CONST 1 mx2
      // =>
      // rhs = OP add i64 kPtyInvalid mx7
      //     opnd[0] = cvt i64 i32 mx6
      //       opnd[0] = REGINDX:7 %7 mx1
      //     opnd[1] = cvt i64 i32 mx5
      //       opnd[1] = CONST 1 mx2
      auto valueRange = DealWithAddOrSub(bb, *addOrSubExpr);
      if (valueRange != nullptr && valueRange->IsEqualAfterCVT(fromType, toType)) {
        auto *cvtOfOpnd0 = irMap.CreateMeExprTypeCvt(toType, fromType, *opnd0);
        auto *cvtOfOpnd1 = irMap.CreateMeExprTypeCvt(toType, fromType, *opnd1);
        auto *newOpMeExpr = irMap.CreateMeExprBinary(opnd->GetOp(), toType, *cvtOfOpnd0, *cvtOfOpnd1);
        (void)irMap.ReplaceMeExprStmt(stmt, opMeExpr, *newOpMeExpr);
        return true;
      }
    }
  }

  if ((fromType == PTY_u1 || fromType == PTY_u8 || fromType == PTY_u16 || fromType == PTY_u32 || fromType == PTY_a32 ||
      ((fromType == PTY_ref || fromType == PTY_ptr) && GetPrimTypeSize(fromType) == 4)) &&
      GetPrimTypeBitSize(toType) > GetPrimTypeBitSize(fromType)) {
    auto *valueRange = FindValueRangeAndInitNumOfRecursion(bb, opMeExpr);
    if (valueRange != nullptr) {
      return false;
    }
    (void)Insert2Caches(bb.GetBBId(), opMeExpr.GetExprID(), std::make_unique<ValueRange>(
        Bound(nullptr, 0, fromType), Bound(GetMaxNumber(fromType), fromType), kLowerAndUpper));
  }
  return false;
}

// When unreachable bb has trystmt or endtry attribute, need update try and endtry bbs.
void ValueRangePropagation::UpdateTryAttribute(BB &bb) {
  // update try end bb
  if (bb.GetAttributes(kBBAttrIsTryEnd) &&
      (bb.GetMeStmts().empty() || (bb.GetMeStmts().front().GetOp() != OP_try))) {
    auto *startTryBB = func.GetCfg()->GetTryBBFromEndTryBB(&bb);
    auto *newEndTry = func.GetCfg()->GetBBFromID(bb.GetBBId() - 1);
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
    BB &bb, size_t i, const ScalarMeExpr *updateSSAExceptTheScalarExpr,
    std::map<OStIdx, std::set<BB*>> &ssaupdateCandsForCondExpr, bool setPhiIsDead) {
  for (auto &it : bb.GetMePhiList()) {
    if (setPhiIsDead) {
      it.second->SetIsLive(false);
    }
    auto *opnd = it.second->GetOpnd(i);
    MeStmt *stmt = nullptr;
    auto *defBB = opnd->GetDefByBBMeStmt(*func.GetCfg(), stmt);
    // At the time of optimizing redundant branches, when the all preds of condgoto BB can jump directly to
    // the successors of condgoto BB and the conditional expr is only used for conditional stmt, after opt,
    // the ssa of expr not need to be updated, otherwise the ssa of epr still needs to be updated.
    if (updateSSAExceptTheScalarExpr != nullptr && it.first == updateSSAExceptTheScalarExpr->GetOstIdx()) {
      // when do opt
      (void)ssaupdateCandsForCondExpr[it.first].insert(defBB);
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
    const BB &pred, BB &bb, BB *trueBr, ScalarMeExpr *updateSSAExceptTheScalarExpr,
    std::map<OStIdx, std::set<BB*>> &ssaupdateCandsForCondExpr) {
  int index = bb.GetPredIndex(pred);
  CHECK_FATAL(index != -1, "pred is not in preds of bb");
  InsertOstOfPhi2Cands(bb, static_cast<uint>(index), updateSSAExceptTheScalarExpr, ssaupdateCandsForCondExpr);
  for (size_t i = 0; i < bb.GetSucc().size(); ++i) {
    auto succ = bb.GetSucc(i);
    int indexOfBBInSuccPreds = succ->GetPredIndex(bb);
    InsertOstOfPhi2Cands(*succ, static_cast<size_t>(static_cast<int64>(indexOfBBInSuccPreds)),
                         updateSSAExceptTheScalarExpr, ssaupdateCandsForCondExpr);
  }
  auto updateSSAExceptTheOstIdx = updateSSAExceptTheScalarExpr == nullptr ? OStIdx(0) :
      updateSSAExceptTheScalarExpr->GetOstIdx();
  MeSSAUpdate::InsertDefPointsOfBBToSSACands(bb, cands, updateSSAExceptTheOstIdx);
  if (trueBr == nullptr) {
    return;
  }
  // When bb2 remove succ trueBr, the phiList while be update by func RemoveBB(), after bb1 add succ trueBr, the size of
  // phi opnds in trueBr will not equal to the pred size of trueBr. So need insert the ost of philist to trueBr.
  //       bb1
  //        |
  //       bb2 bb3   ==>    bb1  bb3
  //       / \  /             \  /
  // falseBr trueBr          trueBr
  for (auto &it : std::as_const(trueBr->GetMePhiList())) {
    if (it.first == updateSSAExceptTheOstIdx) {
      continue;
    }
    MeSSAUpdate::InsertOstToSSACands(it.first, *trueBr, &cands);
  }
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
    case PTY_i16:
      return std::numeric_limits<int16_t>::min();
    case PTY_i32:
      return std::numeric_limits<int32_t>::min();
    case PTY_i64:
      return std::numeric_limits<int64_t>::min();
    case PTY_u8:
      return std::numeric_limits<uint8_t>::min();
    case PTY_u16:
      return std::numeric_limits<uint16_t>::min();
    case PTY_u32:
    case PTY_a32:
      return std::numeric_limits<uint32_t>::min();
    case PTY_ref:
    case PTY_ptr:
      if (GetPrimTypeSize(primType) == kFourByte) { // 32 bit
        return std::numeric_limits<uint32_t>::min();
      } else { // 64 bit
        CHECK_FATAL(GetPrimTypeSize(primType) == kEightByte, "must be 64 bit");
        return std::numeric_limits<uint64_t>::min();
      }
    case PTY_u64:
    case PTY_a64:
      return std::numeric_limits<uint64_t>::min();
    case PTY_u1:
      return 0;
    default:
      CHECK_FATAL(false, "must not be here");
  }
}

int64 GetMaxNumber(PrimType primType) {
  switch (primType) {
    case PTY_i8:
      return std::numeric_limits<int8_t>::max();
    case PTY_i16:
      return std::numeric_limits<int16_t>::max();
    case PTY_i32:
      return std::numeric_limits<int32_t>::max();
    case PTY_i64:
      return std::numeric_limits<int64_t>::max();
    case PTY_u8:
      return std::numeric_limits<uint8_t>::max();
    case PTY_u16:
      return std::numeric_limits<uint16_t>::max();
    case PTY_u32:
    case PTY_a32:
      return std::numeric_limits<uint32_t>::max();
    case PTY_ref:
    case PTY_ptr:
      if (GetPrimTypeSize(primType) == kFourByte) { // 32 bit
        return std::numeric_limits<uint32_t>::max();
      } else { // 64 bit
        CHECK_FATAL(GetPrimTypeSize(primType) == kEightByte, "must be 64 bit");
        return std::numeric_limits<uint64_t>::max();
      }
    case PTY_u64:
    case PTY_a64:
      return std::numeric_limits<uint64_t>::max();
    case PTY_u1:
      return 1;
    default:
      CHECK_FATAL(false, "must not be here");
  }
}

// If the result of operator add or sub is overflow or underflow, return false.
bool ValueRangePropagation::AddOrSubWithConstant(
    PrimType primType, Opcode op, int64 lhsConstant, int64 rhsConstant, int64 &res) const {
  if (!IsNeededPrimType(primType)) {
    return false;
  }
  if (ConstantFold::IntegerOpIsOverflow(op, primType, lhsConstant, rhsConstant)) {
    return false;
  }
  if (IsPrimTypeUint64(primType)) {
    res = (op == OP_add) ?
        static_cast<int64>((static_cast<uint64>(lhsConstant) + static_cast<uint64>(rhsConstant))) :
        static_cast<int64>((static_cast<uint64>(lhsConstant) - static_cast<uint64>(rhsConstant)));
  } else {
    if (op == OP_add) {
      if ((rhsConstant > 0 && lhsConstant > GetMaxNumber(primType) - rhsConstant) ||
          (rhsConstant < 0 && lhsConstant < GetMinNumber(primType) - rhsConstant)) {
        return false;
      } else {
        res = lhsConstant + rhsConstant;
      }
    } else {
      if ((rhsConstant < 0 && lhsConstant > GetMaxNumber(primType) + rhsConstant) ||
          (rhsConstant > 0 && lhsConstant < GetMinNumber(primType) + rhsConstant)) {
        return false;
      } else {
        res = lhsConstant - rhsConstant;
      }
    }
  }
  return true;
}

// Create new bound when old bound add or sub with a constant.
bool ValueRangePropagation::CreateNewBoundWhenAddOrSub(Opcode op, Bound bound, int64 rhsConstant, Bound &res) const {
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
    value = static_cast<ConstMeExpr&>(expr).GetExtIntValue();
    return true;
  }
  auto *valueRange = FindValueRangeAndInitNumOfRecursion(bb, expr);
  if (valueRange == nullptr) {
    if (expr.GetMeOp() == kMeOpVar && static_cast<VarMeExpr&>(expr).GetDefBy() == kDefByStmt &&
        static_cast<VarMeExpr&>(expr).GetDefStmt()->GetRHS()->GetMeOp() == kMeOpConst &&
        static_cast<ConstMeExpr*>(static_cast<VarMeExpr&>(expr).GetDefStmt()->GetRHS())->GetConstVal()->GetKind() ==
            kConstInt) {
      value = static_cast<ConstMeExpr*>(static_cast<VarMeExpr&>(expr).GetDefStmt()->GetRHS())->GetExtIntValue();
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

bool ValueRangePropagation::IsNotNeededVRType(const ValueRange &vrLeft, const ValueRange &vrRight) const {
  return vrLeft.GetRangeType() == kNotEqual || vrRight.GetRangeType() == kNotEqual ||
         vrLeft.GetRangeType() == kOnlyHasLowerBound || vrRight.GetRangeType() == kOnlyHasLowerBound ||
         vrLeft.GetRangeType() == kOnlyHasUpperBound || vrRight.GetRangeType() == kOnlyHasUpperBound;
}

// Create new valueRange when old valueRange add or sub with a valuerange.
std::unique_ptr<ValueRange> ValueRangePropagation::AddOrSubWithValueRange(
    Opcode op, ValueRange &valueRangeLeft, ValueRange &valueRangeRight) const {
  if (IsNotNeededVRType(valueRangeLeft, valueRangeRight)) {
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
  if (valueRangeLeft.IsNotConstantVR() || valueRangeRight.IsNotConstantVR()) {
    return nullptr;
  }
  if (IsInvalidVR(valueRangeLeft) || IsInvalidVR(valueRangeRight)) {
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

bool ValueRangePropagation::AddOrSubWithBound(
    PrimType pTy, Bound oldBound, Bound &resBound, int64 rhsConstant, Opcode op) const {
  int64 res = 0;
  if (!AddOrSubWithConstant(pTy, op, oldBound.GetConstant(), rhsConstant, res)) {
    return false;
  }
  resBound = Bound(oldBound.GetVar(), res, pTy);
  return true;
}

// Create new valueRange when old valueRange add or sub with a constant.
std::unique_ptr<ValueRange> ValueRangePropagation::AddOrSubWithValueRange(
    PrimType pTy, Opcode op, ValueRange &valueRange, int64 rhsConstant) const {
  if (valueRange.GetRangeType() == kLowerAndUpper) {
    if (valueRange.IsConstantLowerAndUpper() &&
        valueRange.IsInvalidVR()) {
      return nullptr;
    }
    Bound lower;
    if (!AddOrSubWithBound(pTy, valueRange.GetLower(), lower, rhsConstant, op)) {
      return nullptr;
    }
    Bound upper;
    if (!AddOrSubWithBound(pTy, valueRange.GetUpper(), upper, rhsConstant, op)) {
      return nullptr;
    }
    return std::make_unique<ValueRange>(lower, upper, kLowerAndUpper, valueRange.IsAccurate());
  } else if (valueRange.GetRangeType() == kEqual) {
    Bound bound;
    if (!AddOrSubWithBound(pTy, valueRange.GetBound(), bound, rhsConstant, op)) {
      return nullptr;
    }
    return std::make_unique<ValueRange>(bound, valueRange.GetRangeType(), valueRange.IsAccurate());
  } else if (valueRange.GetRangeType() == kOnlyHasLowerBound || valueRange.GetRangeType() == kOnlyHasUpperBound) {
    Bound bound;
    if (!AddOrSubWithBound(pTy, valueRange.GetBound(), bound, rhsConstant, op)) {
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
  auto pType = opMeExpr.GetOpnd(1)->GetPrimType();
  if (pType == PTY_i64 && rhsConstant == GetMinNumber(PTY_i64)) { // var % x, if var is negative unlimited
    return nullptr;
  }
  if (Bound(rhsConstant, pType).IsGreaterThan(Bound(nullptr, 0, pType), pType)) {
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
  } else if (Bound(rhsConstant, pType).IsLessThanOrEqualTo(Bound(nullptr, 0, pType), pType)) {
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
  } else {
    return nullptr;
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
  auto *valueRange = FindValueRangeAndInitNumOfRecursion(bb, *opMeExpr.GetOpnd(0));
  if (valueRange == nullptr) {
    return remValueRange;
  }
  if (!valueRange->IsConstantLowerAndUpper()) {
    return remValueRange;
  } else if (valueRange->GetRangeType() == kEqual) {
    if (!valueRange->GetBound().IsEqualAfterCVT(valueRange->GetBound().GetPrimType(), opMeExpr.GetPrimType()) ||
        valueRange->GetBound().IsEqualToMin(opMeExpr.GetPrimType())) {
      return nullptr;
    }
    int64 res = Bound::GetRemResult(valueRange->GetBound().GetConstant(), rhsConstant, opMeExpr.GetPrimType());
    Bound bound = Bound(nullptr, res, valueRange->GetBound().GetPrimType());
    return std::make_unique<ValueRange>(bound, valueRange->GetRangeType());
  } else {
    if (valueRange->GetLower().IsLessThanOrEqualTo(remValueRange->GetLower(), remValueRange->GetPrimType()) ||
        valueRange->GetUpper().IsGreaterThanOrEqualTo(remValueRange->GetUpper(), remValueRange->GetPrimType())) {
      return remValueRange;
    }
    std::unique_ptr<ValueRange> combineRes = CombineTwoValueRange(*remValueRange, *valueRange);
    if (combineRes == nullptr) {
      return nullptr;
    }
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
  if (rhsConstant == 0) {
    return nullptr;
  }
  std::unique_ptr<ValueRange> newValueRange;
  if (lhsIsConstant && rhsIsConstant) {
    if (Bound(lhsConstant, opMeExpr.GetPrimType()).IsEqualToMin(opMeExpr.GetPrimType())) {
      return nullptr;
    }
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
std::unique_ptr<ValueRange> ValueRangePropagation::DealWithAddOrSub(const BB &bb, const OpMeExpr &opMeExpr) {
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
    auto *valueRange = FindValueRangeAndInitNumOfRecursion(bb, *opnd0);
    if (valueRange == nullptr) {
      return nullptr;
    }
    newValueRange = AddOrSubWithValueRange(opMeExpr.GetPrimType(), opMeExpr.GetOp(), *valueRange, rhsConstant);
  } else if (lhsIsConstant && opMeExpr.GetOp() == OP_add) {
    auto *valueRange = FindValueRangeAndInitNumOfRecursion(bb, *opnd1);
    if (valueRange == nullptr) {
      return nullptr;
    }
    newValueRange = AddOrSubWithValueRange(opMeExpr.GetPrimType(), opMeExpr.GetOp(), *valueRange, lhsConstant);
  }
  return newValueRange;
}

// Save array length to caches for eliminate array boundary check.
void ValueRangePropagation::DealWithArrayLength(const BB &bb, MeExpr &lhs, MeExpr &rhs) {
  if (rhs.GetMeOp() == kMeOpVar) {
    auto *valueRangeOfLength = FindValueRangeAndInitNumOfRecursion(bb, rhs);
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
    case PTY_i16:
      return static_cast<int16>(value);
    case PTY_i32:
      return static_cast<int32>(value);
    case PTY_i64:
      return static_cast<int64>(value);
    case PTY_u8:
      return static_cast<uint8>(value);
    case PTY_u16:
      return static_cast<uint16>(value);
    case PTY_u32:
    case PTY_a32:
      return static_cast<uint32>(value);
    case PTY_ref:
    case PTY_ptr:
      if (GetPrimTypeSize(primType) == kFourByte) { // 32 bit
        return static_cast<uint32>(value);
      } else { // 64 bit
        CHECK_FATAL(GetPrimTypeSize(primType) == kEightByte, "must be 64 bit");
        return static_cast<uint64>(value);
      }
    case PTY_u64:
    case PTY_a64:
      return static_cast<uint64>(value);
    case PTY_u1:
      return static_cast<bool>(value);
    default:
      CHECK_FATAL(false, "must not be here");
  }
}

std::unique_ptr<ValueRange> ValueRangePropagation::CopyValueRange(ValueRange &valueRange, PrimType primType) const {
  if (primType != PTY_begin && (!valueRange.GetLower().IsEqualAfterCVT(valueRange.GetPrimType(), primType) ||
                                !valueRange.GetUpper().IsEqualAfterCVT(valueRange.GetPrimType(), primType))) {
    // When the valueRange changes after conversion according to the parameter primType, return nullptr.
    return nullptr;
  }
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

std::unique_ptr<ValueRange> ValueRangePropagation::DealWithMeOp(const BB &bb, const MeStmt &stmt) {
  auto *lhs = stmt.GetLHS();
  auto *rhs = stmt.GetRHS();
  auto *opMeExpr = static_cast<OpMeExpr*>(rhs);
  switch (rhs->GetOp()) {
    case OP_add:
    case OP_sub: {
      return DealWithAddOrSub(bb, *opMeExpr);
    }
    case OP_gcmallocjarray: {
      DealWithArrayLength(bb, *lhs, *opMeExpr->GetOpnd(0));
      break;
    }
    case OP_rem: {
      return DealWithRem(bb, *lhs, *opMeExpr);
    }
    default:
      break;
  }
  return nullptr;
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
  std::unique_ptr<ValueRange> resVR = nullptr;
  auto *existValueRange = FindValueRangeAndInitNumOfRecursion(bb, *rhs);
  if (existValueRange != nullptr && existValueRange->GetRangeType() != kOnlyHasLowerBound &&
      existValueRange->GetRangeType() != kOnlyHasUpperBound) {
    resVR = CopyValueRange(*existValueRange);
  } else if (rhs->GetMeOp() == kMeOpOp) {
    resVR = DealWithMeOp(bb, stmt);
  } else if (rhs->GetMeOp() == kMeOpConst && static_cast<ConstMeExpr*>(rhs)->GetConstVal()->GetKind() == kConstInt) {
    resVR = std::make_unique<ValueRange>(
        Bound(static_cast<ConstMeExpr*>(rhs)->GetExtIntValue(), rhs->GetPrimType()), kEqual);
  }
  if (resVR != nullptr) {
    auto pTypeOfLHS = lhs->GetPrimType();
    auto pTypeOfRHS = rhs->GetPrimType();
    auto primTypeOfVR = resVR->GetLower().GetPrimType();
    // The rhs may be truncated when assigned to lhs,
    // so it is necessary to judge whether the range before and after is consistent.
    if (resVR->IsEqualAfterCVT(pTypeOfLHS, pTypeOfRHS) &&
        resVR->IsEqualAfterCVT(pTypeOfLHS, primTypeOfVR) &&
        resVR->IsEqualAfterCVT(pTypeOfRHS, primTypeOfVR)) {
      Insert2Caches(bb.GetBBId(), lhs->GetExprID(), CopyValueRange(*resVR, pTypeOfLHS));
    } else {
      Insert2Caches(bb.GetBBId(), lhs->GetExprID(), nullptr);
    }
    return;
  }
  auto pType = lhs->GetPrimType();
  if (!IsPrimitivePureScalar(pType)) {
    return;
  }
  (void)Insert2Caches(bb.GetBBId(), lhs->GetExprID(), std::make_unique<ValueRange>(
      Bound(nullptr, GetMinNumber(pType), pType), Bound(nullptr, GetMaxNumber(pType), pType), kLowerAndUpper));
}

// Analysis the stride of loop induction var, like this pattern:
// i1 = phi(i0, i2),
// i2 = i1 + 1,
// stride is 1.
bool ValueRangePropagation::CanComputeLoopIndVar(const MeExpr &phiLHS, MeExpr &expr, int64 &constant) const {
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
      auto rhsConst = rhsExpr->GetExtIntValue();
      if (rhsExpr->GetPrimType() == PTY_u64 && rhsConst > GetMaxNumber(PTY_i64)) {
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
      for (size_t j = 0; j < it->NumMeStmtOpnds(); ++j) {
        DealWithOperand(*bb, *it, *it->GetOpnd(j));
      }
      switch (it->GetOp()) {
        case OP_dassign:
        case OP_maydassign:
        case OP_regassign: {
          if (it->GetLHS() != nullptr && !CanCreateVRForExpr(*it->GetLHS())) {
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

void ValueRangePropagation::CreateVRForPhi(const LoopDesc &loop) {
  onlyPropVRStack.push(onlyPropVR);
  onlyPropVR = true;
  std::vector<BB*> reversePostOrderOfLoopBBs;
  // Collect loop bbs.
  for (auto it = currItOfTravelReversePostOrder; it != dom.GetReversePostOrder().end(); ++it) {
    if (reversePostOrderOfLoopBBs.size() == loop.loopBBs.size()) {
      break;
    }
    auto bb = func.GetCfg()->GetBBFromID(BBId((*it)->GetID()));
    if (loop.Has(*bb)) {
      reversePostOrderOfLoopBBs.push_back(bb);
    }
  }
  // Travels loop bbs.
  TravelBBs(reversePostOrderOfLoopBBs);
  onlyPropVR = onlyPropVRStack.top();
  onlyPropVRStack.pop();
}

// Calculate the valuerange of def-operand according to the valuerange of each rhs operand.
std::unique_ptr<ValueRange> ValueRangePropagation::MergeValueRangeOfPhiOperands(const BB &bb, MePhiNode &mePhiNode) {
  auto *resValueRange = FindValueRangeAndInitNumOfRecursion(
      *bb.GetPred(0), *mePhiNode.GetOpnd(0), kRecursionThresholdOfFindVROfPhi);
  if (resValueRange == nullptr || !resValueRange->IsConstantBoundOrLowerAndUpper()) {
    return nullptr;
  }
  bool theRangeTypeIsNotEqual = (resValueRange->GetRangeType() == kNotEqual);
  auto resValueRangePtr = CopyValueRange(*resValueRange);
  for (size_t i = 1; i < mePhiNode.GetOpnds().size(); ++i) {
    auto *operand = mePhiNode.GetOpnd(i);
    auto *valueRange =
        FindValueRangeAndInitNumOfRecursion(*bb.GetPred(i), *operand, kRecursionThresholdOfFindVROfPhi);
    // If one valuerange is nullptr, the result is nullptr.
    if (valueRange == nullptr || !valueRange->IsConstantBoundOrLowerAndUpper()) {
      return nullptr;
    }
    theRangeTypeIsNotEqual = (valueRange->GetRangeType() == kNotEqual) || theRangeTypeIsNotEqual;
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
    if (resValueRangePtr == nullptr) {
      return nullptr;
    }
  }
  return resValueRangePtr;
}

bool ValueRangePropagation::MergeVrOrInitAndBackedge(const MePhiNode &mePhiNode, ValueRange &vrOfInitExpr,
    ValueRange &valueRange, Bound &resBound) const {
  bool isOnlyHasLowerBound = vrOfInitExpr.GetRangeType() == kOnlyHasLowerBound;
  auto pType = mePhiNode.GetLHS()->GetPrimType();
  auto upperBound = isOnlyHasLowerBound ? valueRange.GetBound() : vrOfInitExpr.GetBound();
  auto lowerBound = isOnlyHasLowerBound ? vrOfInitExpr.GetBound() : valueRange.GetBound();
  if (valueRange.GetRangeType() == kNotEqual) {
    if (vrOfInitExpr.GetBound().IsConstantBound() && valueRange.IsConstantRange()) {
      if (!lowerBound.IsLessThanOrEqualTo(upperBound, pType)) {
        return false;
      }
      int64 res = 0;
      if (!AddOrSubWithConstant(pType, OP_sub, upperBound.GetConstant(), lowerBound.GetConstant(), res)) {
        return false;
      }
      if (!Bound(res, pType).IsGreaterThan(Bound(vrOfInitExpr.GetStride(), pType), pType)) {
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
        Bound(nullptr, GetMaxNumber(pType), pType) : Bound(nullptr, GetMinNumber(pType), pType);
    bool vrCanBeComputed = true;
    index++;
    bool isAccurateBound = false;
    for (size_t i = 0; i < mePhiNode->GetOpnds().size(); ++i) {
      if (i == indexOfInitExpr) {
        continue;
      }
      auto *operand = mePhiNode->GetOpnd(i);
      auto *valueRange = FindValueRangeAndInitNumOfRecursion(*bb.GetPred(i), *operand);
      if (valueRange == nullptr ||
          valueRange->GetRangeType() == kOnlyHasUpperBound ||
          valueRange->GetRangeType() == kOnlyHasLowerBound ||
          vrOfInitExpr->IsEqual(valueRange) ||
          !MergeVrOrInitAndBackedge(*mePhiNode, *vrOfInitExpr, *valueRange, resBound)) {
        vrCanBeComputed = false;
        break;
      }
      // The loop variable overflow or underflow during iteration, can not recorded vr of the loop variable. such as:
      // label1:
      //   a1 = phi(a0, a2),
      //   a2 = a1 +/- 1,
      //    goto label1,
      // If the step of loop var a is greater than 0 and the lower bound of a2 is less than the lower bound of a0,
      // the loop variable overflow during iteration.
      // If the step of loop var a is less than 0 and the lower bound of a2 is greater than the upper bound of a0,
      // the loop variable underflow during iteration.
      if (valueRange->GetRangeType() == kLowerAndUpper &&
          valueRange->IsConstantRange() &&
          vrOfInitExpr->GetBound().IsConstantBound() &&
          ((vrOfInitExpr->GetRangeType() == kOnlyHasLowerBound &&
            valueRange->GetLower().IsLessThanOrEqualTo(vrOfInitExpr->GetLower(), vrOfInitExpr->GetPrimType())) ||
           (vrOfInitExpr->GetRangeType() == kOnlyHasUpperBound &&
            valueRange->GetUpper().IsGreaterThanOrEqualTo(vrOfInitExpr->GetUpper(), vrOfInitExpr->GetPrimType())))) {
        vrCanBeComputed = false;
        break;
      }
      isAccurateBound = valueRange->IsAccurate() || valueRange->GetRangeType() == kEqual ||
          valueRange->GetRangeType() == kNotEqual;
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
      if (loop.IsCanonicalAndOnlyHasOneExitBBLoop() && isAccurateBound) {
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
        if (loop.IsCanonicalAndOnlyHasOneExitBBLoop() && isAccurate && isAccurateBound) {
          valueRangeOfPhi->SetAccurate(true);
        }
      }
    }
    Insert2Caches(bb.GetBBId(), mePhiNode->GetLHS()->GetExprID(), std::move(valueRangeOfPhi));
    onlyRecordValueRangeInTempCache.pop();
  }
}

void ValueRangePropagation::CalculateVROfSubOpnd(const BBId bbID, const MeExpr &opnd, ValueRange &valueRange) {
  // Deal with the case like:
  // opnd[0] = OP sub u32 u32 mx1
  //   opnd[1] = REGINDX:15 u32 %15 mx2
  //   opnd[2] = const 5 mx3
  // If the vr of opnd mx1 has been calculated, the vr of opnd mx2 can be calculated.
  if (opnd.GetOp() != OP_add && opnd.GetOp() != OP_sub) {
    return;
  }
  auto &opMeExpr = static_cast<const OpMeExpr&>(opnd);
  auto *opnd0 = opMeExpr.GetOpnd(0);
  auto *opnd1 = opMeExpr.GetOpnd(1);
  if (opnd1->GetMeOp() != kMeOpConst || static_cast<ConstMeExpr*>(opnd1)->GetConstVal()->GetKind() != kConstInt) {
    return;
  }
  int64 rhsConstant = static_cast<ConstMeExpr*>(opnd1)->GetExtIntValue();
  auto antiOp = (opnd.GetOp() == OP_add) ? OP_sub : OP_add;
  Insert2Caches(bbID, opnd0->GetExprID(), AddOrSubWithValueRange(
      opMeExpr.GetPrimType(), antiOp, valueRange, rhsConstant), opnd0);
}

bool ValueRangePropagation::Insert2Caches(
    const BBId &bbID, int32 exprID, std::unique_ptr<ValueRange> valueRange, const MeExpr *opnd) {
  if (valueRange == nullptr) {
    if (onlyRecordValueRangeInTempCache.top()) {
      tempCaches[bbID].insert(std::make_pair(exprID, nullptr));
    } else {
      caches.at(bbID)[exprID] = nullptr;
    }
    return true;
  }
  if (!IsNeededPrimType(valueRange->GetPrimType())) {
    return false;
  }
  auto tempVR = CopyValueRange(*valueRange);
  if (valueRange->IsConstant() && valueRange->GetRangeType() == kLowerAndUpper) {
    valueRange->SetRangeType(kEqual);
    valueRange->SetBound(valueRange->GetLower());
  }

  if (onlyRecordValueRangeInTempCache.top()) {
    tempCaches[bbID][exprID] = std::move(valueRange);
  } else {
    if (valueRange->IsConstantLowerAndUpper() && IsInvalidVR(*valueRange)) {
      return false;
    }
    caches.at(bbID)[exprID] = std::move(valueRange);
  }
  if (opnd != nullptr) {
    CalculateVROfSubOpnd(bbID, *opnd, *tempVR);
  }
  return true;
}

// The rangeType of vrOfRHS is kEqual and the rangeType of vrOfLHS is kEqual, kNotEqual or kLowerAndUpper
void ValueRangePropagation::JudgeEqual(const MeExpr &expr, ValueRange &vrOfLHS, ValueRange &vrOfRHS,
    std::unique_ptr<ValueRange> &valueRangePtr) const {
  if (vrOfRHS.GetRangeType() != kEqual) {
    return;
  }
  auto primType = static_cast<const OpMeExpr&>(expr).GetOpndType();
  auto resPType = static_cast<const OpMeExpr&>(expr).GetPrimType();
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
    if (!vrOfLHS.IsInvalidVR() && vrOfLHS.IsEqualAfterCVT(vrOfLHS.GetPrimType(), primType) &&
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

std::unique_ptr<ValueRange> ValueRangePropagation::NegValueRange(
    const BB &bb, MeExpr &opnd, uint32 &numberOfRecursions, std::unordered_set<int32> &foundExprs) {
  auto *valueRange = FindValueRange(bb, opnd, numberOfRecursions, foundExprs, kRecursionThreshold);
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

ValueRange *ValueRangePropagation::DealWithNegWhenFindValueRange(const BB &bb, const MeExpr &expr,
    uint32 &numberOfRecursions, std::unordered_set<int32> &foundExprs) {
  auto *opnd = expr.GetOpnd(0);
  if (!foundExprs.insert(opnd->GetExprID()).second) {
    return nullptr;
  }
  auto valueRangePtr = NegValueRange(bb, *opnd, numberOfRecursions, foundExprs);
  if (valueRangePtr == nullptr) {
    return nullptr;
  }
  auto *resValueRange = valueRangePtr.get();
  if (!Insert2Caches(bb.GetBBId(), expr.GetExprID(), std::move(valueRangePtr))) {
    return nullptr;
  }
  return resValueRange;
}

ValueRange *ValueRangePropagation::FindValueRangeWithCompareOp(const BB &bb, const MeExpr &expr,
    uint32 &numberOfRecursions, std::unordered_set<int32> &foundExprs, uint32 maxThreshold) {
  auto op = expr.GetOp();
  if (op == OP_neg) {
    return DealWithNegWhenFindValueRange(bb, expr, numberOfRecursions, foundExprs);
  }
  if (!IsCompareHasReverseOp(op) || expr.GetNumOpnds() != kNumOperands) {
    return nullptr;
  }
  auto *opnd0 = expr.GetOpnd(0);
  auto *opnd1 = expr.GetOpnd(1);
  auto *valueRangeOfOpnd0 = FindValueRange(bb, *opnd0, numberOfRecursions, foundExprs, maxThreshold);
  if (valueRangeOfOpnd0 == nullptr) {
    return nullptr;
  }
  auto *valueRangeOfOpnd1 = FindValueRange(bb, *opnd1, numberOfRecursions, foundExprs, maxThreshold);
  if (valueRangeOfOpnd1 == nullptr || !valueRangeOfOpnd0->IsConstantRange() || !valueRangeOfOpnd1->IsConstantRange()) {
    return nullptr;
  }
  auto rangeTypeOfOpnd0 = valueRangeOfOpnd0->GetRangeType();
  auto rangeTypeOfOpnd1 = valueRangeOfOpnd1->GetRangeType();
  std::unique_ptr<ValueRange> valueRangePtr = nullptr;
  ValueRange *resValueRange = nullptr;
  auto resPType = static_cast<const OpMeExpr&>(expr).GetPrimType();
  auto opndType = static_cast<const OpMeExpr&>(expr).GetOpndType();
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
    if (!Insert2Caches(bb.GetBBId(), expr.GetExprID(), std::move(valueRangePtr))) {
      return nullptr;
    }
  }
  return resValueRange;
}

bool ValueRangePropagation::IsSubOpndOfExpr(const MeExpr &expr, const MeExpr &subExpr) const {
  if (&expr == &subExpr) {
    return true;
  }
  for (uint8 i = 0; i < expr.GetNumOpnds(); ++i) {
    if (IsSubOpndOfExpr(*(expr.GetOpnd(i)), subExpr)) {
      return true;
    }
  }
  return false;
}

ValueRange *ValueRangePropagation::GetMinimumRange(ValueRange *vr1, ValueRange *vr2, PrimType pTy) const {
  if (vr1 == nullptr) {
    return vr2;
  }
  if (vr2 == nullptr) {
    return vr1;
  }
  if (!vr1->IsConstantRange() || !vr2->IsConstantRange()) {
    return vr2;
  }
  if (!vr1->IsEqualAfterCVT(vr1->GetPrimType(), pTy) || !vr2->IsEqualAfterCVT(vr2->GetPrimType(), pTy)) {
    return vr2;
  }
  return (vr1->GetLower().IsGreaterThanOrEqualTo(vr2->GetLower(), pTy) &&
          vr1->GetUpper().IsLessThan(vr2->GetUpper(), pTy)) ||
         (vr1->GetLower().IsGreaterThan(vr2->GetLower(), pTy) &&
          vr1->GetUpper().IsLessThanOrEqualTo(vr2->GetUpper(), pTy)) ? vr1 : vr2;
}

// When the valueRange of expr is not exist, need find the valueRange of the def point or use points.
ValueRange *ValueRangePropagation::FindValueRange(const BB &bb, MeExpr &expr, uint32 &numberOfRecursionsArg,
                                                  std::unordered_set<int32> &foundExprs, uint32 maxThreshold) {
  if (numberOfRecursionsArg++ > maxThreshold) {
    return nullptr;
  }
  uint32 recursions = 0;
  ValueRange *resVR = nullptr;
  auto *valueRange = FindValueRangeInCaches(
      bb.GetBBId(), expr.GetExprID(), recursions, maxThreshold, expr.GetPrimType());
  resVR = GetMinimumRange(valueRange, resVR, expr.GetPrimType());
  if (resVR == nullptr) {
    valueRange = FindValueRangeWithCompareOp(bb, expr, numberOfRecursionsArg, foundExprs, maxThreshold);
  }
  resVR = GetMinimumRange(valueRange, resVR, expr.GetPrimType());
  auto it = pairOfExprs.find(&expr);
  if (it == pairOfExprs.end()) {
    return resVR;
  }
  for (auto itOfValueMap = it->second.begin(); itOfValueMap != it->second.end(); ++itOfValueMap) {
    auto bbOfPair = itOfValueMap->first;
    auto exprs = itOfValueMap->second;
    if (!dom.Dominate(*bbOfPair, bb)) {
      continue;
    }
    for (auto itOfExprs = exprs.begin(); itOfExprs != exprs.end(); ++itOfExprs) {
      if (!foundExprs.insert((*itOfExprs)->GetExprID()).second) {
        continue;
      }
      valueRange = FindValueRange(bb, **itOfExprs, numberOfRecursionsArg, foundExprs, maxThreshold);
      if (valueRange != nullptr && valueRange->IsEqualAfterCVT(expr.GetPrimType(), (*itOfExprs)->GetPrimType())) {
        resVR = GetMinimumRange(valueRange, resVR, expr.GetPrimType());
      }
      if (IsSubOpndOfExpr(**itOfExprs, expr)) {
        // When the condition is true, mx1297 and mx553 are equivalent. When the vr of mx533 cannot be found,
        // there is no need to continue to find the vr of mx1297, because it will fall into a dead cycle.
        // For example:
        // ||MEIR|| brtrue
        //     opnd[0] = OP eq u1 u32 mx1297
        //       opnd[0] = i32 i64 mx553
        //     opnd[1] = OP cvt i32 i64 mx553
        continue;
      }
      if (resVR == nullptr) {
        valueRange = FindValueRangeWithCompareOp(bb, **itOfExprs, numberOfRecursionsArg, foundExprs, maxThreshold);
      }
      resVR = GetMinimumRange(valueRange, resVR, expr.GetPrimType());
    }
  }
  return resVR;
}

std::unique_ptr<ValueRange> ValueRangePropagation::CreateInitVRForPhi(LoopDesc &loop,
    const BB &bb, ScalarMeExpr &init, ScalarMeExpr &backedge, const ScalarMeExpr &lhsOfPhi) {
  Bound initBound;
  ValueRange *valueRangeOfInit = FindValueRangeAndInitNumOfRecursion(bb, init, kRecursionThresholdOfFindVROfLoopVar);
  if (valueRangeOfInit != nullptr && valueRangeOfInit->IsConstant() && valueRangeOfInit->GetRangeType() != kNotEqual) {
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
    ASSERT_NOT_NULL(brMeStmt);
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
  size_t indexOfInitExpr = 0;
  std::vector<std::unique_ptr<ValueRange>> valueRangeOfInitExprs;
  // Collect the initial value and the back edge value of phi node.
  for (auto &pair : bb.GetMePhiList()) {
    if (!pair.second->GetIsLive()) {
      continue;
    }
    if (pair.second->GetOpnds().size() != kNumOperands) {
      return;
    }
    auto *initExpr = pair.second->GetOpnd(0);
    auto *backExpr = pair.second->GetOpnd(1);
    MeStmt *defStmtOfInit = nullptr;
    BB *defBBOfInit = initExpr->GetDefByBBMeStmt(*func.GetCfg(), defStmtOfInit);
    MeStmt *defStmtOfBack = nullptr;
    BB *defBBOfInitOfBack = backExpr->GetDefByBBMeStmt(*func.GetCfg(), defStmtOfBack);
    bool defBBOfInitInLoop = loop->Has(*defBBOfInit);
    bool defBBOfInitOfBackInLoop = loop->Has(*defBBOfInitOfBack);
    std::unique_ptr<ValueRange> vrOfInitExpr = nullptr;
    if (defBBOfInitInLoop && !defBBOfInitOfBackInLoop) {
      indexOfInitExpr = 1;
      vrOfInitExpr = CreateInitVRForPhi(*loop, bb, *backExpr, *initExpr, *pair.second->GetLHS());
    } else if (!defBBOfInitInLoop && defBBOfInitOfBackInLoop) {
      vrOfInitExpr = CreateInitVRForPhi(*loop, bb, *initExpr, *backExpr, *pair.second->GetLHS());
    }
    valueRangeOfInitExprs.emplace_back(std::move(vrOfInitExpr));
  }
  // When the number of nested layers of the loop is too deep, do not continue to calculate vr of phi nodes.
  if (loop->nestDepth > kLimitOfLoopNestDepth || loop->loopBBs.size() > kLimitOfLoopBBs) {
    return;
  }
  onlyRecordValueRangeInTempCache.push(true);
  CreateVRForPhi(*loop);
  MergeValueRangeOfPhiOperands(*loop, bb, valueRangeOfInitExprs, indexOfInitExpr);
  onlyRecordValueRangeInTempCache.pop();
  tempCaches.clear();
}

// Return the max of leftBound or rightBound.
Bound ValueRangePropagation::Max(Bound leftBound, Bound rightBound) {
  if (leftBound.GetVar() == rightBound.GetVar()) {
    return (leftBound.IsGreaterThan(rightBound, leftBound.GetPrimType())) ? leftBound : rightBound;
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
    return (leftBound.IsLessThan(rightBound, leftBound.GetPrimType())) ? leftBound : rightBound;
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
      auto *lowerRangeValue = FindValueRangeAndInitNumOfRecursion(bb, *lowerVar);
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
  auto *upperRangeValue = FindValueRangeAndInitNumOfRecursion(bb, *upperVar);
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

bool ValueRangePropagation::CanNotCombineVRs(const Bound &resLower, const Bound &resUpper, PrimType primType) const {
  // C:    for (int j = 4; j < 5; j++)
  // meir: j0=4;
  //       j1=phi(j0,j2)
  //       j2=j1+1
  //       if: j2 < 5
  // When dealwith phi, compute vr of j0 first, the vr of j0 is [4,kOnlyHasLowerBound] and
  // vr of j1 is [5,kOnlyHasLowerBound]
  // when deal with the stmt if (j2 < 5), the vr of j2 is [5,4] in ture branch, if abandon the vr, can not compute the
  // vr of j1, so keep the vr in tempCaches.
  return (!onlyRecordValueRangeInTempCache.top() && resLower.IsConstantBound() && resUpper.IsConstantBound() &&
          resLower.IsGreaterThan(resUpper, primType));
}

std::unique_ptr<ValueRange> ValueRangePropagation::CombineTwoValueRange(
    const ValueRange &leftRange, const ValueRange &rightRange, bool merge) {
  if (merge) {
    return std::make_unique<ValueRange>(Min(leftRange.GetLower(), rightRange.GetLower()),
                                        Max(leftRange.GetUpper(), rightRange.GetUpper()), kLowerAndUpper);
  } else {
    if (rightRange.GetRangeType() == kOnlyHasLowerBound) {
      return std::make_unique<ValueRange>(rightRange.GetLower(), Max(leftRange.GetUpper(),
          Bound(GetMaxNumber(rightRange.GetUpper().GetPrimType()), rightRange.GetUpper().GetPrimType())),
              kLowerAndUpper, rightRange.IsAccurate());
    }
    if (rightRange.GetRangeType() == kOnlyHasUpperBound) {
      return std::make_unique<ValueRange>(Min(leftRange.GetLower(),
          Bound(GetMinNumber(rightRange.GetLower().GetPrimType()), rightRange.GetLower().GetPrimType())),
              leftRange.GetUpper(), kLowerAndUpper, rightRange.IsAccurate());
    }
    return std::make_unique<ValueRange>(Max(leftRange.GetLower(), rightRange.GetLower()),
        Min(leftRange.GetUpper(), rightRange.GetUpper()), kLowerAndUpper, rightRange.IsAccurate());
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
  isCFGChange = true;
  if (ValueRangePropagation::isDebug) {
    LogInfo::MapleLogger() << "=====================delete bb " << unreachableBB.GetBBId() <<
        "========================\n";
  }
  if (unreachableBB.GetPred().size() == 1) {
    unreachableBBs.insert(&unreachableBB);
  }
  return;
}

// when determine remove the condgoto stmt, need analysis which bbs need be deleted.
void ValueRangePropagation::AnalysisUnreachableBBOrEdge(BB &bb, BB &unreachableBB, BB &succBB) {
  if (onlyPropVR) {
    return;
  }
  for (auto &pred : bb.GetPred()) {
    UpdateProfile(*pred, bb, unreachableBB);
  }
  Insert2UnreachableBBs(unreachableBB);
  // update frequency before cfg changed
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
      leftRange.GetRangeType() == kOnlyHasUpperBound ||
      rightRangeType == kSpecialLowerForLoop ||
      rightRangeType == kSpecialUpperForLoop ||
      rightRange.GetRangeType() == kOnlyHasLowerBound ||
      rightRange.GetRangeType() == kOnlyHasUpperBound) {
    return false;
  }
  if (leftRange.IsInvalidVR() || rightRange.IsInvalidVR()) {
    return false;
  }
  if (!leftLower.IsEqualAfterCVT(leftLower.GetPrimType(), opndType) ||
      !leftUpper.IsEqualAfterCVT(leftUpper.GetPrimType(), opndType) ||
      !rightLower.IsEqualAfterCVT(rightLower.GetPrimType(), opndType) ||
      !rightUpper.IsEqualAfterCVT(rightUpper.GetPrimType(), opndType)) {
    return false;
  }
  if (judgeNotInRange) {
    if (leftLower.GetVar() != nullptr) {
      return false;
    }
  } else {
    if (leftLower.GetVar() != nullptr) {
      auto *valueRange = FindValueRangeAndInitNumOfRecursion(bb, *leftLower.GetVar());
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

void ValueRangePropagation::CreateValueRangeForSubOpnd(const MeExpr &opnd, const BB &trueBranch, const BB &falseBranch,
    ValueRange &resTrueBranchVR, ValueRange &resFalseBranchVR) {
  // Deal with the case like:
  // brtrue @@m357 (lt u1 i32 (zext u32 8 (regread u8 %102), constval i32 1))
  // The vr of opnd in curr bb is leftRange and in true branch is resTrueBranchVR.
  // If the op of opnd is zext, and the vr before and after zext is the same,
  // after judgment, the vr of before zext is the same as the opnd in true or false branch.
  if (opnd.GetOp() != OP_zext) {
    return;
  }
  auto &opMeExpr = static_cast<const OpMeExpr&>(opnd);
  auto pTypeOfBitSize = GetIntegerPrimTypeBySizeAndSign(opMeExpr.GetBitsSize(), false);
  if (pTypeOfBitSize == opMeExpr.GetOpnd(0)->GetPrimType()) {
    (void)Insert2Caches(trueBranch.GetBBId(), opnd.GetOpnd(0)->GetExprID(), CopyValueRange(resTrueBranchVR), &opnd);
    (void)Insert2Caches(falseBranch.GetBBId(), opnd.GetOpnd(0)->GetExprID(), CopyValueRange(resFalseBranchVR), &opnd);
  }
}

void ValueRangePropagation::CreateValueRangeForLeOrLt(const MeExpr &opnd, const ValueRange *leftRange,
    Bound newRightUpper, Bound newRightLower, const BB &trueBranch, const BB &falseBranch, bool isAccurate) {
  CHECK_FATAL(IsEqualPrimType(newRightUpper.GetPrimType(), newRightLower.GetPrimType()), "must be equal");
  if (leftRange == nullptr) {
    std::unique_ptr<ValueRange> newTrueBranchRange = std::make_unique<ValueRange>(
        ValueRange::MinBound(newRightUpper.GetPrimType()), newRightUpper, kLowerAndUpper, isAccurate);
    (void)Insert2Caches(trueBranch.GetBBId(), opnd.GetExprID(), std::move(newTrueBranchRange), &opnd);
    std::unique_ptr<ValueRange> newFalseBranchRange = std::make_unique<ValueRange>(newRightLower,
        ValueRange::MaxBound(newRightUpper.GetPrimType()), kLowerAndUpper, isAccurate);
    (void)Insert2Caches(falseBranch.GetBBId(), opnd.GetExprID(), std::move(newFalseBranchRange), &opnd);
  } else if (leftRange->GetRangeType() == kOnlyHasLowerBound) {
    std::unique_ptr<ValueRange> newRightRange = std::make_unique<ValueRange>(
        ValueRange::MinBound(newRightUpper.GetPrimType()), newRightUpper, kLowerAndUpper, isAccurate);
    if (leftRange->IsConstantLowerAndUpper() && newRightRange->IsConstantLowerAndUpper()) {
      (void)Insert2Caches(trueBranch.GetBBId(), opnd.GetExprID(),
                          CombineTwoValueRange(*leftRange, *newRightRange), &opnd);
    } else {
      (void)Insert2Caches(trueBranch.GetBBId(), opnd.GetExprID(), std::make_unique<ValueRange>(
          leftRange->GetLower(), newRightUpper, kLowerAndUpper, isAccurate), &opnd);
    }
    newRightRange = std::make_unique<ValueRange>(
        newRightLower, ValueRange::MaxBound(newRightUpper.GetPrimType()), kLowerAndUpper, isAccurate);
  } else if (leftRange->GetRangeType() == kOnlyHasUpperBound) {
    std::unique_ptr<ValueRange> newRightRange = std::make_unique<ValueRange>(
        ValueRange::MinBound(newRightUpper.GetPrimType()), newRightUpper, kLowerAndUpper, isAccurate);
    newRightRange = std::make_unique<ValueRange>(
        newRightLower, ValueRange::MaxBound(newRightUpper.GetPrimType()), kLowerAndUpper, isAccurate);
    (void)Insert2Caches(falseBranch.GetBBId(), opnd.GetExprID(),
                        CombineTwoValueRange(*leftRange, *newRightRange), &opnd);
  } else {
    std::unique_ptr<ValueRange> newRightRange = std::make_unique<ValueRange>(
        ValueRange::MinBound(newRightUpper.GetPrimType()), newRightUpper, kLowerAndUpper, isAccurate);
    auto resTrueBranchVR = CombineTwoValueRange(*leftRange, *newRightRange);
    if (resTrueBranchVR != nullptr) {
      (void)Insert2Caches(trueBranch.GetBBId(), opnd.GetExprID(), CopyValueRange(*resTrueBranchVR), &opnd);
    }
    newRightRange = std::make_unique<ValueRange>(
        newRightLower, ValueRange::MaxBound(newRightUpper.GetPrimType()), kLowerAndUpper, isAccurate);
    auto resFalseBranchVR = CombineTwoValueRange(*leftRange, *newRightRange);
    if (resFalseBranchVR != nullptr) {
      (void)Insert2Caches(falseBranch.GetBBId(), opnd.GetExprID(), CopyValueRange(*resFalseBranchVR), &opnd);
    }
    if (leftRange->IsConstantLowerAndUpper() && resTrueBranchVR != nullptr && resFalseBranchVR != nullptr) {
      CreateValueRangeForSubOpnd(opnd, trueBranch, falseBranch, *resTrueBranchVR, *resFalseBranchVR);
    }
  }
}

void ValueRangePropagation::CreateValueRangeForGeOrGt(
    const MeExpr &opnd, const ValueRange *leftRange, Bound newRightUpper, Bound newRightLower,
    const BB &trueBranch, const BB &falseBranch, bool isAccurate) {
  CHECK_FATAL(IsEqualPrimType(newRightUpper.GetPrimType(), newRightLower.GetPrimType()), "must be equal");
  if (leftRange == nullptr) {
    std::unique_ptr<ValueRange> newTrueBranchRange = std::make_unique<ValueRange>(newRightLower,
        ValueRange::MaxBound(newRightUpper.GetPrimType()), kLowerAndUpper, isAccurate);
    (void)Insert2Caches(trueBranch.GetBBId(), opnd.GetExprID(), std::move(newTrueBranchRange), &opnd);
    std::unique_ptr<ValueRange> newFalseBranchRange = std::make_unique<ValueRange>(
        ValueRange::MinBound(newRightUpper.GetPrimType()), newRightUpper, kLowerAndUpper, isAccurate);
    (void)Insert2Caches(falseBranch.GetBBId(), opnd.GetExprID(), std::move(newFalseBranchRange), &opnd);
  } else if (leftRange->GetRangeType() == kOnlyHasLowerBound) {
    auto newRightRange = std::make_unique<ValueRange>(
        ValueRange::MinBound(newRightUpper.GetPrimType()), newRightUpper, kLowerAndUpper, isAccurate);
    (void)Insert2Caches(falseBranch.GetBBId(), opnd.GetExprID(),
                        CombineTwoValueRange(*leftRange, *newRightRange), &opnd);
  } else if (leftRange->GetRangeType() == kOnlyHasUpperBound) {
    std::unique_ptr<ValueRange> newRightRange = std::make_unique<ValueRange>(newRightLower,
        ValueRange::MaxBound(newRightUpper.GetPrimType()), kLowerAndUpper, isAccurate);
    (void)Insert2Caches(trueBranch.GetBBId(), opnd.GetExprID(),
                        CombineTwoValueRange(*leftRange, *newRightRange), &opnd);
  } else {
    std::unique_ptr<ValueRange> newRightRange = std::make_unique<ValueRange>(newRightLower,
        ValueRange::MaxBound(newRightUpper.GetPrimType()), kLowerAndUpper, isAccurate);
    (void)Insert2Caches(trueBranch.GetBBId(), opnd.GetExprID(),
        CombineTwoValueRange(*leftRange, *newRightRange), &opnd);
    newRightRange = std::make_unique<ValueRange>(ValueRange::MinBound(newRightUpper.GetPrimType()), newRightUpper,
        kLowerAndUpper, isAccurate);
    (void)Insert2Caches(falseBranch.GetBBId(), opnd.GetExprID(),
                        CombineTwoValueRange(*leftRange, *newRightRange), &opnd);
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

// case 1:
// cond: if mx1 != mx2
// mx1 vr: (kNotEqual 0)
// mx2 vr: (kEqual 1)
// trueBranch mx1 vr: [2, max]
// case 2:
// cond: if mx1 != mx2
// mx1 vr: (kNotEqual 1)
// mx2 vr: (kEqual 0)
// trueBranch mx1 vr: [2, max]
void ValueRangePropagation::MergeNotEqualRanges(
    const MeExpr &opnd, const ValueRange *leftRange, ValueRange &rightRange, const BB &trueBranch) {
  if (leftRange == nullptr) {
    return;
  }
  auto primType = leftRange->GetPrimType();
  if (primType != rightRange.GetPrimType() || !IsPrimitiveUnsigned(primType)) {
    return;
  }
  if (leftRange->GetRangeType() != kNotEqual || !leftRange->IsConstant()) {
    return;
  }
  if (rightRange.GetRangeType() != kEqual || !rightRange.IsConstant()) {
    return;
  }
  auto leftBound = leftRange->GetBound();
  auto rightBound = rightRange.GetBound();
  auto constZeroBound = Bound(nullptr, 0, primType);
  auto constOneBound = Bound(nullptr, 1, primType);
  if ((leftBound.IsEqual(constZeroBound, primType) && rightBound.IsEqual(constOneBound, primType)) ||
      (leftBound.IsEqual(constOneBound, primType) && rightBound.IsEqual(constZeroBound, primType))) {
    (void)Insert2Caches(trueBranch.GetBBId(), opnd.GetExprID(),
        std::make_unique<ValueRange>(Bound(2, primType), Bound(GetMaxNumber(primType), primType), kLowerAndUpper));
  }
}

void ValueRangePropagation::CreateValueRangeForNeOrEq(
    const MeExpr &opnd, const ValueRange *leftRange, ValueRange &rightRange, const BB &trueBranch,
    const BB &falseBranch) {
  if (rightRange.GetRangeType() == kEqual) {
    std::unique_ptr<ValueRange> newTrueBranchRange =
        std::make_unique<ValueRange>(rightRange.GetBound(), kEqual);
    auto &opMeExpr = static_cast<const OpMeExpr&>(opnd);
    if (opMeExpr.GetOp() == OP_zext && rightRange.IsConstant() &&
        opMeExpr.GetOpnd(0)->GetPrimType() == GetIntegerPrimTypeBySizeAndSign(opMeExpr.GetBitsSize(), false)) {
      (void)Insert2Caches(trueBranch.GetBBId(), opnd.GetOpnd(0)->GetExprID(), CopyValueRange(*newTrueBranchRange));
    }
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

void ValueRangePropagation::UpdateProfile(BB &pred, BB &bb, const BB &targetBB) const {
  if (bb.GetKind() != kBBCondGoto || bb.IsMeStmtEmpty() || !bb.GetLastMe() || !bb.GetLastMe()->IsCondBr()) {
    return;
  }
  auto *condGotoStmt = static_cast<CondGotoMeStmt*>(bb.GetLastMe());
  if (condGotoStmt->GetBranchProb() != kProbLikely && condGotoStmt->GetBranchProb() != kProbUnlikely) {
    return; // need not update profile
  }
  int32 targetBranchProb = 0;
  if (&targetBB == bb.GetSucc(0)) {
    targetBranchProb = CondGotoNode::probAll - condGotoStmt->GetBranchProb();
  } else {
    ASSERT(&targetBB == bb.GetSucc(1), "must equal");
    targetBranchProb = condGotoStmt->GetBranchProb();
  }
  BB *predCondGoto = &pred;
  BB *succBB = &bb; // succ of predCondGoto to targetBB
  while (predCondGoto != nullptr) {
    if (predCondGoto->GetKind() == kBBCondGoto) {
      break;
    }
    if (predCondGoto->GetPred().size() != 1) {
      predCondGoto = nullptr;
      break;
    }
    succBB = predCondGoto;
    predCondGoto = predCondGoto->GetPred(0);
  }
  if (!predCondGoto || predCondGoto->IsMeStmtEmpty() || !predCondGoto->GetLastMe()->IsCondBr()) {
    return;
  }
  auto *targetCondGotoStmt = static_cast<CondGotoMeStmt*>(predCondGoto->GetLastMe());
  if (targetCondGotoStmt->GetBranchProb() == kProbUnlikely || targetCondGotoStmt->GetBranchProb() == kProbLikely) {
    return;
  }
  int size = predCondGoto->GetSuccIndex(*succBB);
  ASSERT(size != -1, "must find");
  if (size == 1) {
    targetCondGotoStmt->SetBranchProb(targetBranchProb);
  } else {
    targetCondGotoStmt->SetBranchProb(CondGotoNode::probAll - targetBranchProb);
  }
  return;
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
      if (condGotoBB.GetPred().size() == 1) {
        UpdateProfile(*condGotoBB.GetPred(0), condGotoBB, trueBranch);
      }
      condGotoBB.SetKind(kBBFallthru);
      condGotoBB.RemoveSucc(*succ1);
      DeleteThePhiNodeWhichOnlyHasOneOpnd(*succ1, updateSSAExceptTheScalarExpr, ssaupdateCandsForCondExpr);
      condGotoBB.RemoveMeStmt(condGotoBB.GetLastMe());
    }
  } else {
    if (OnlyHaveOneCondGotoPredBB(*succ0, condGotoBB)) {
      AnalysisUnreachableBBOrEdge(condGotoBB, *succ0, trueBranch);
    } else {
      if (condGotoBB.GetPred().size() == 1) {
        UpdateProfile(*condGotoBB.GetPred(0), condGotoBB, trueBranch);
      }
      condGotoBB.SetKind(kBBFallthru);
      if (func.GetCfg()->UpdateCFGFreq()) {
        condGotoBB.SetSuccFreq(0, condGotoBB.GetSuccFreq()[1]);
      }
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
      ASSERT_NOT_NULL(condGotoStmt);
      if (&newBB == pred.GetSucc().at(1)) {
        condGotoStmt->SetOffset(func.GetOrCreateBBLabel(newBB));
      }
      break;
    }
    case kBBSwitch: {
      auto *switchStmt = static_cast<SwitchMeStmt*>(pred.GetLastMe());
      ASSERT_NOT_NULL(switchStmt);
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
  auto codeSizeLimit = MeOption::optForSize ? 0 : kCodeSizeLimit;
  if (currCost > codeSizeLimit) {
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
    PrepareForSSAUpdateWhenPredBBIsRemoved(
        pred, bb, &trueBranch, updateSSAExceptTheScalarExpr, ssaupdateCandsForCondExpr);
    size_t index = FindBBInSuccs(pred, bb);
    FreqType edgeFreq = 0;
    if (func.GetCfg()->UpdateCFGFreq()) {
      edgeFreq = pred.GetSuccFreq()[index];
    }
    pred.RemoveSucc(bb);
    DeleteThePhiNodeWhichOnlyHasOneOpnd(bb, updateSSAExceptTheScalarExpr, ssaupdateCandsForCondExpr);
    pred.AddSucc(*exitCopyFallthru, index);
    CreateLabelForTargetBB(pred, *exitCopyFallthru);
    if (func.GetCfg()->UpdateCFGFreq()) {
      exitCopyFallthru->SetFrequency(exitCopyFallthru->GetFrequency() + edgeFreq);
      pred.AddSuccFreq(edgeFreq, index);
      // update bb frequency
      ASSERT(bb.GetFrequency() >= edgeFreq, "sanity check");
      bb.SetFrequency(bb.GetFrequency() - edgeFreq);
      bb.UpdateEdgeFreqs();
    }
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
        *predOfCurrBB, *currBB, nullptr, updateSSAExceptTheScalarExpr, ssaupdateCandsForCondExpr);
  }
  if (predOfCurrBB != nullptr) {
    PrepareForSSAUpdateWhenPredBBIsRemoved(
        *predOfCurrBB, *currBB, nullptr, updateSSAExceptTheScalarExpr, ssaupdateCandsForCondExpr);
  }
  CHECK_FATAL(currBB->GetKind() == kBBCondGoto, "must be condgoto bb");
  auto *gotoMeStmt = irMap.New<GotoMeStmt>(func.GetOrCreateBBLabel(trueBranch));
  mergeAllFallthruBBs->AddMeStmtLast(gotoMeStmt);
  PrepareForSSAUpdateWhenPredBBIsRemoved(
      pred, bb, &trueBranch, updateSSAExceptTheScalarExpr, ssaupdateCandsForCondExpr);
  size_t index = FindBBInSuccs(pred, bb);
  FreqType edgeFreq = 0;
  if (func.GetCfg()->UpdateCFGFreq()) {
    edgeFreq = pred.GetSuccFreq()[index];
  }
  pred.RemoveSucc(bb);
  pred.AddSucc(*mergeAllFallthruBBs, index);
  if (func.GetCfg()->UpdateCFGFreq()) {
    mergeAllFallthruBBs->SetFrequency(edgeFreq);
    mergeAllFallthruBBs->PushBackSuccFreq(edgeFreq);
    pred.AddSuccFreq(edgeFreq, index);
    // update bb frequency
    if (bb.GetFrequency() >= edgeFreq) {
      bb.SetFrequency(bb.GetFrequency() - edgeFreq);
      bb.UpdateEdgeFreqs();
    }
  }
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
    if (tmpPred->GetSuccIndex(bb) != -1) {
      tmpPred = &bb;
    } else {
      tmpPred = tmpPred->GetSucc(0);
    }
    if (GetRealPredSize(*tmpPred) > 1) {
      return ChangeTheSuccOfPred2TrueBranch(
          pred, bb, trueBranch, updateSSAExceptTheScalarExpr, ssaupdateCandsForCondExpr);
    }
  } while (tmpPred->GetKind() != kBBCondGoto);
  if (GetRealPredSize(bb) != 1) {
    return false;
  }
  // step2
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
  if (GetRealPredSize(bb) < kNumOperands) {
    CHECK_FATAL(GetRealPredSize(bb) == 1, "must have one pred");
    RemoveUnreachableBB(bb, trueBranch, updateSSAExceptTheScalarExpr, ssaupdateCandsForCondExpr);
    return true;
  }
  if (OnlyHaveCondGotoStmt(bb)) {
    PrepareForSSAUpdateWhenPredBBIsRemoved(
        pred, bb, &trueBranch, updateSSAExceptTheScalarExpr, ssaupdateCandsForCondExpr);
    size_t index = FindBBInSuccs(pred, bb);
    FreqType edgeFreq = 0;
    if (func.GetCfg()->UpdateCFGFreq()) {
      edgeFreq = pred.GetSuccFreq()[index];
    }
    pred.RemoveSucc(bb);
    DeleteThePhiNodeWhichOnlyHasOneOpnd(bb, updateSSAExceptTheScalarExpr, ssaupdateCandsForCondExpr);
    pred.AddSucc(trueBranch, index);
    CreateLabelForTargetBB(pred, trueBranch);
    if (func.GetCfg()->UpdateCFGFreq()) {
      if (bb.GetFrequency() > edgeFreq) {
        bb.SetFrequency(bb.GetFrequency() - edgeFreq);
        size_t trueBranchIdx = static_cast<uint32>(bb.GetSuccIndex(trueBranch));
        FreqType updatedtrueFreq = bb.GetSuccFreq()[trueBranchIdx] - edgeFreq;
        // transform may not be consistent with frequency value
        updatedtrueFreq = updatedtrueFreq > 0 ? updatedtrueFreq : 0;
        bb.SetSuccFreq(static_cast<int>(trueBranchIdx), updatedtrueFreq);
      }
      pred.AddSuccFreq(edgeFreq, index);
    }
  } else {
    auto *exitCopyFallthru = GetNewCopyFallthruBB(trueBranch, bb);
    if (exitCopyFallthru != nullptr) {
      PrepareForSSAUpdateWhenPredBBIsRemoved(
          pred, bb, &trueBranch, updateSSAExceptTheScalarExpr, ssaupdateCandsForCondExpr);
      size_t index = FindBBInSuccs(pred, bb);
      FreqType edgeFreq = 0;
      if (func.GetCfg()->UpdateCFGFreq()) {
        edgeFreq = pred.GetSuccFreq()[index];
        ASSERT(bb.GetFrequency() >= edgeFreq, "sanity check");
      }
      pred.RemoveSucc(bb);
      DeleteThePhiNodeWhichOnlyHasOneOpnd(bb, updateSSAExceptTheScalarExpr, ssaupdateCandsForCondExpr);
      pred.AddSucc(*exitCopyFallthru, index);
      CreateLabelForTargetBB(pred, *exitCopyFallthru);
      if (func.GetCfg()->UpdateCFGFreq()) {
        if (bb.GetFrequency() > edgeFreq) {
          bb.SetFrequency(static_cast<uint32>(bb.GetFrequency() - edgeFreq));
          exitCopyFallthru->SetFrequency(static_cast<uint32>(edgeFreq));
          exitCopyFallthru->PushBackSuccFreq(edgeFreq);
          size_t trueBranchIdx = static_cast<uint32>(bb.GetSuccIndex(trueBranch));
          FreqType updatedtrueFreq = static_cast<int64_t>(
              bb.GetSuccFreq()[trueBranchIdx] - edgeFreq);
          ASSERT(updatedtrueFreq >= 0, "sanity check");
          bb.SetSuccFreq(static_cast<int>(trueBranchIdx), updatedtrueFreq);
        }
        pred.AddSuccFreq(edgeFreq, index);
      }
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
    PrepareForSSAUpdateWhenPredBBIsRemoved(
        pred, bb, &trueBranch, updateSSAExceptTheScalarExpr, ssaupdateCandsForCondExpr);
    size_t index = FindBBInSuccs(pred, bb);
    FreqType edgeFreq = 0;
    if (func.GetCfg()->UpdateCFGFreq()) {
      edgeFreq = pred.GetSuccFreq()[index];
    }
    pred.RemoveSucc(bb);
    pred.AddSucc(*newBB, index);
    newBB->AddSucc(trueBranch);
    if (func.GetCfg()->UpdateCFGFreq()) {
      if (bb.GetFrequency() >= edgeFreq) {
        bb.SetFrequency(static_cast<uint32>(bb.GetFrequency() - edgeFreq));
        bb.UpdateEdgeFreqs();
      }
      newBB->SetFrequency(static_cast<uint32>(edgeFreq));
      newBB->PushBackSuccFreq(edgeFreq);
      pred.AddSuccFreq(edgeFreq, index);
    }
    DeleteThePhiNodeWhichOnlyHasOneOpnd(bb, updateSSAExceptTheScalarExpr, ssaupdateCandsForCondExpr);
    (void)func.GetOrCreateBBLabel(trueBranch);
    CreateLabelForTargetBB(pred, *newBB);
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
    if (bb.GetSucc().size() != 1) {
      // When bb is wont exit bb, there will be multiple succs, copying this type of bb is not supported.
      return false;
    }
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
    const ValueRange *rightRange, BB &falseBranch, BB &trueBranch, PrimType opndType, Opcode op,
    ScalarMeExpr *updateSSAExceptTheScalarExpr, std::map<OStIdx, std::set<BB*>> &ssaupdateCandsForCondExpr) {
  if (leftRange == nullptr || rightRange == nullptr) {
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
  if (BrStmtInRange(bb, *leftRange, *rightRange, op, opndType)) {
    // opnd, tmpPred, tmpBB, reachableBB
    // remove falseBranch
    UpdateProfile(*tmpPred, *tmpBB, trueBranch);
    return RemoveUnreachableEdge(
        *tmpPred, *tmpBB, trueBranch, updateSSAExceptTheScalarExpr, ssaupdateCandsForCondExpr);
  } else if (BrStmtInRange(bb, *leftRange, *rightRange, antiOp, opndType)) {
    UpdateProfile(*tmpPred, *tmpBB, falseBranch);
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

std::unique_ptr<ValueRange> ValueRangePropagation::AntiValueRange(ValueRange &valueRange) const {
  RangeType oldType = valueRange.GetRangeType();
  if (oldType != kEqual && oldType != kNotEqual) {
    return nullptr;
  }
  RangeType newType = (oldType == kEqual) ? kNotEqual : kEqual;
  return std::make_unique<ValueRange>(Bound(valueRange.GetBound().GetVar(), valueRange.GetBound().GetConstant(),
      valueRange.GetBound().GetPrimType()), newType);
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
    MePhiNode *&phi, bool &thePhiIsInBB) const {
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

// Find the version expr(a2) of a3 in pred bb.
// If find failed, return expr a3.
//   pred1(a1) pred(a2)
//         \  /
//        succ0(a3 = phi(a1,a2))
//          |
//        succ1
//          |
//          bb if (a3)
MeExpr &ValueRangePropagation::GetVersionOfOpndInPred(const BB &pred, const BB &bb, MeExpr &expr, const BB &condGoto) {
  if (!expr.IsScalar()) {
    return expr;
  }
  auto &scalar = static_cast<ScalarMeExpr&>(expr);
  MeStmt *defStmt = nullptr;
  auto *defBB = scalar.GetDefByBBMeStmt(*func.GetCfg(), defStmt);
  if (dom.Dominate(*defBB, pred)) {
    return expr;
  }
  if (HasDefPointInPred(pred, condGoto, scalar)) {
    defByStmtInUDChain = true;
    return expr;
  }
  auto ostIndex = scalar.GetOstIdx();
  auto *curPred = &pred;
  while (curPred->GetSucc().size() == 1) {
    auto *succ = curPred->GetSucc(0);
    if (!succ->GetMePhiList().empty()) {
      auto it = succ->GetMePhiList().find(ostIndex);
      if (it != succ->GetMePhiList().end()) {
        int sizeOfPred = succ->GetPredIndex(*curPred);
        CHECK_FATAL(sizeOfPred != -1, "must find");
        return *(it->second->GetOpnd(static_cast<uint>(sizeOfPred)));
      }
    }
    if (succ == &bb) {
      break;
    }
  }
  return expr;
}

// \ /
// bb1
// a1 = phi(a0, a2)
// a2 = a1 + 1 ignore def point a2 when judge the way has def point from begin to end
//  |
// bb2
// if a2 < 1
bool ValueRangePropagation::CanIgnoreTheDefPoint(const MeStmt &stmt, const BB &end, const ScalarMeExpr &expr) const {
  if (end.GetKind() != kBBCondGoto) {
    return false;
  }
  auto opMeExpr = static_cast<OpMeExpr*>(static_cast<const CondGotoMeStmt*>(end.GetLastMe())->GetOpnd());
  auto opnd0 = opMeExpr->GetOpnd(0);
  auto opnd1 = opMeExpr->GetOpnd(1);
  if (opnd0 != stmt.GetLHS() && opnd1 != stmt.GetLHS()) {
    return false;
  }
  auto rhs = stmt.GetRHS();
  if (rhs->GetMeOp() != kMeOpOp) {
    return false;
  }
  if (rhs->GetOp() != OP_add && rhs->GetOp() != OP_sub) {
    return false;
  }
  auto opnd0OfOp = rhs->GetOpnd(0);
  auto opnd1OfOp = rhs->GetOpnd(1);
  return (opnd0OfOp->IsScalar() && static_cast<ScalarMeExpr*>(opnd0OfOp)->GetOstIdx() == expr.GetOstIdx() &&
          opnd1OfOp->GetMeOp() == kMeOpConst) ||
         (opnd1OfOp->IsScalar() && static_cast<ScalarMeExpr*>(opnd1OfOp)->GetOstIdx() == expr.GetOstIdx() &&
          opnd0OfOp->GetMeOp() == kMeOpConst);
}

// Find the opnd of phi recursively. If there is a definition point between begin and end,
// can not use the opnd in begin to opt the cfg.
//             begin a1 = 1 can not
//           \ /
//          pred0(a2 = phi(a0,a1)
//            |
//   pred1  pred2(a3 = 5) symbol a has def point in a3
//        \  /
//       succ0(a4 = phi(a0,a3))
//         |
//       end if (a4 - 1 > constant)
bool ValueRangePropagation::HasDefPointInPred(const BB &begin, const BB &end, const ScalarMeExpr &opnd) const {
  auto *tempBB = &begin;
  while (tempBB != &end) {
    if (tempBB->GetSucc().size() != 1) {
      return true;
    }
    for (auto &stmt : tempBB->GetMeStmts()) {
      if (stmt.GetLHS() != nullptr && stmt.GetLHS()->GetOstIdx() == opnd.GetOstIdx()) {
        return !CanIgnoreTheDefPoint(stmt, end, opnd);
      }
      if (stmt.GetChiList() != nullptr) {
        for (auto &chi : *stmt.GetChiList()) {
          if (chi.first == opnd.GetOstIdx()) {
            return true;
          }
        }
      }
      if (stmt.GetAssignedLHS() != nullptr && stmt.GetAssignedLHS()->GetOstIdx() == opnd.GetOstIdx()) {
        return true;
      }
    }
    tempBB = tempBB->GetSucc(0);
  }
  return false;
}

// If expr a2 is assigned with b2 in pred bb, the valueRange of expr (a2 - b2) in pred bb is (0, kEqual).
//   pred1  pred(a2 = b2)
//        \  /
//       succ0(a3 = phi(a1,a2), b3 = phi(b1, b2))
//         |
//       succ1
//         |
//       bb if (a3 - b3 > constant)
std::unique_ptr<ValueRange> ValueRangePropagation::GetValueRangeOfLHS(const BB &pred, const BB &bb,
                                                                      MeExpr &expr, const BB &condGoto) {
  if (expr.GetMeOp() != kMeOpOp || expr.GetOp() != OP_sub) {
    return nullptr;
  }
  auto &opMeExpr = static_cast<OpMeExpr&>(expr);
  auto *opnd0 = opMeExpr.GetOpnd(0);
  auto *opnd1 = opMeExpr.GetOpnd(1);
  auto &versionOpnd0InPred = GetVersionOfOpndInPred(pred, bb, *opnd0, condGoto);
  auto &versionOpnd1InPred = GetVersionOfOpndInPred(pred, bb, *opnd1, condGoto);
  // If the vr of versionOpnd0InPred is equal to the vr of versionOpnd1InPred in pred.
  if (FindPairOfExprs(versionOpnd0InPred, versionOpnd1InPred, pred)) {
    return CreateValueRangeOfEqualZero(PTY_u1);
  }
  auto vrpOfOpnd0 = FindValueRangeInCurrBBOrDominateBBs(pred, versionOpnd0InPred);
  auto vrpOfOpnd1 = FindValueRangeInCurrBBOrDominateBBs(pred, versionOpnd1InPred);
  if (vrpOfOpnd0 == nullptr || vrpOfOpnd1 == nullptr ||
      // If two ranges are not a certain constant value, it cannot be determined whether they are equal.
      !vrpOfOpnd0->IsKEqualAndConstantRange() || !vrpOfOpnd1->IsKEqualAndConstantRange()) {
    return nullptr;
  }
  if (vrpOfOpnd0->IsEqual(vrpOfOpnd1.get())) {
    return CreateValueRangeOfEqualZero(PTY_u1);
  }
  return nullptr;
}

bool ValueRangePropagation::TowCompareOperandsAreInSameIterOfLoop(const MeExpr &lhs, const MeExpr &rhs) const {
  if (!lhs.IsScalar() || !rhs.IsScalar()) {
    return false;
  }
  auto &lhsScalar = static_cast<const ScalarMeExpr&>(lhs);
  auto &rhsScalar = static_cast<const ScalarMeExpr&>(rhs);
  MeStmt *defStmt = nullptr;
  auto *lhsDefBB = lhsScalar.GetDefByBBMeStmt(*func.GetCfg(), defStmt);
  auto *rhsDefBB = rhsScalar.GetDefByBBMeStmt(*func.GetCfg(), defStmt);
  auto *loopOfLHS = loops->GetBBLoopParent(lhsDefBB->GetBBId());
  auto *loopOfRHS = loops->GetBBLoopParent(rhsDefBB->GetBBId());
  if (loopOfLHS != nullptr && loopOfLHS == loopOfRHS) {
    // Two compare operands(tmp and var) are not in same iter of loop, so can not opt bb4 connected with bb4.
    // bb0: do
    // bb1:   tmp = a + b
    // bb2:   if tmp < var
    // bb3:     ...
    // bb4:   var = tmp
    // bb5: while ()
    return false;
  }
  return true;
}

bool ValueRangePropagation::AnalysisValueRangeInPredsOfCondGotoBB(
    BB &bb, MeExpr *opnd0, MeExpr &currOpnd, ValueRange *rightRange,
    BB &falseBranch, BB &trueBranch, PrimType opndType, Opcode op, BB &condGoto) {
  bool opt = false;
  /* Records the currOpnd of pred */
  MeExpr *predOpnd = nullptr;
  bool thePhiIsInBB = false;
  MapleVector<ScalarMeExpr*> phiOpnds(mpAllocator.Adapter());
  MePhiNode *phi = nullptr;
  MeExpr *realCurrOpnd = &currOpnd;
  // Deal with the case like:
  // if cvt(i64 u32(opnd)) != 1
  // If the value ranges of opnd before cvt and after cvt are equal to each other, use the opnd to opt the if stmt.
  if (realCurrOpnd->GetOp() == OP_cvt) {
    auto *opMeExpr = static_cast<OpMeExpr*>(realCurrOpnd);
    auto *opnd = opMeExpr->GetOpnd(0);
    auto *valueRangeOfOpnd = FindValueRangeAndInitNumOfRecursion(bb, *opnd);
    if (valueRangeOfOpnd != nullptr && valueRangeOfOpnd->IsConstantLowerAndUpper()) {
      auto toType = opMeExpr->GetPrimType();
      auto fromType = opMeExpr->GetOpndType();
      if (valueRangeOfOpnd->IsEqualAfterCVT(fromType, toType)) {
        realCurrOpnd = opnd;
      }
    }
  }
  // If the value ranges of opnd before zext and after zext are equal to each other, use the opnd to opt the if stmt.
  // The case like:
  // VAR:%_160__levVar_116_0{offset:0}<0>[idx:5] mx341 = MEPHI{mx340,mx339}
  // ||MEIR|| brfalse
  //     cond:  @@s44OP ne u1 u32 mx343
  //       opnd[0] = OP zext u32 kPtyInvalid mx342
  //         opnd[0] = VAR %_160__levVar_116_0{offset:0}<0>[idx:5] (field)0 mx341
  //     opnd[1] = CONST 0 mx96
  // The realCurrOpnd is mx341 not mx342.
  if (realCurrOpnd->GetOp() == OP_zext) {
    auto *opMeExpr = static_cast<OpMeExpr*>(realCurrOpnd);
    auto *opnd = opMeExpr->GetOpnd(0);
    auto *valueRangeOfOpnd = FindValueRangeAndInitNumOfRecursion(bb, *opnd);
    auto *valueRangeOfOpMeExpr = FindValueRangeAndInitNumOfRecursion(bb, *opMeExpr);
    if (valueRangeOfOpnd != nullptr && valueRangeOfOpMeExpr != nullptr &&
        valueRangeOfOpnd->IsEqual(valueRangeOfOpMeExpr)) {
      realCurrOpnd = opnd;
    }
  }
  /* find the rhs of currOpnd, which is used as the currOpnd of pred */
  ReplaceOpndByDef(bb, *realCurrOpnd, predOpnd, phi, thePhiIsInBB);
  if (phi != nullptr) {
    phiOpnds = static_cast<ScalarMeExpr&>(*predOpnd).GetDefPhi().GetOpnds();
  }
  size_t indexOfOpnd = 0;
  ScalarMeExpr *exprOnlyUsedByCondMeStmt = nullptr;
  std::map<OStIdx, std::set<BB*>> ssaupdateCandsForCondExpr;
  if (!thePhiIsInBB) {
    auto *useListOfPredOpnd = useInfo->GetUseSitesOfExpr(predOpnd);
    if (useListOfPredOpnd != nullptr && useListOfPredOpnd->size() == 1 && useListOfPredOpnd->front().IsUseByStmt()) {
      auto *useStmt = useListOfPredOpnd->front().GetStmt();
      ASSERT_NOT_NULL(condGoto.GetLastMe());
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
      if (predOpnd->GetMeOp() == kMeOpVar &&
          static_cast<VarMeExpr*>(predOpnd)->GetOst()->GetMIRSymbol()->GetAttr(ATTR_shortcc) &&
          useListOfPredOpnd != nullptr && useListOfPredOpnd->size() == 1) {
        if (useListOfPredOpnd->front().IsUseByPhi() && useListOfPredOpnd->front().GetPhi() == phi) {
          auto *useListOfPhi = useInfo->GetUseSitesOfExpr(phi->GetLHS());
          if (useListOfPhi != nullptr && useListOfPhi->size() == 1 && useListOfPhi->front().IsUseByStmt() &&
              !useListOfPhi->front().GetStmt()->GetBB()->GetAttributes(kBBAttrIsInLoop)) {
            auto *useStmt = useListOfPhi->front().GetStmt();
            if (condGoto.GetKind() == kBBCondGoto && condGoto.GetLastMe() != nullptr &&
                condGoto.GetLastMe()->IsCondBr() && condGoto.GetLastMe() == useStmt && predOpnd->IsScalar()) {
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
    if (bb.GetBBId() < dom.GetIterDomFrontierSize() && !dom.Dominate(*pred, bb)) {
      auto &frontier = dom.GetIterDomFrontier(bb.GetID());
      if (frontier.find(pred->GetID()) != frontier.end()) {
        ++i;
        continue;
      }
    }
    auto *valueRangeInPred = FindValueRangeAndInitNumOfRecursion(*pred, *predOpnd);
    std::unique_ptr<ValueRange> valueRangeInPredPtr = nullptr;
    auto dummyRightRange = rightRange;
    if (valueRangeInPred == nullptr && !defByStmtInUDChain) {
      valueRangeInPredPtr = GetValueRangeOfLHS(*pred, bb, *predOpnd, condGoto);
      valueRangeInPred = valueRangeInPredPtr.get();
    }
    if (dummyRightRange == nullptr || valueRangeInPred == nullptr) {
      // If the vr of versionOpnd0InPred is equal to the vr of versionOpnd1InPred in pred.
      if (opnd0 != nullptr && FindPairOfExprs(*predOpnd, *opnd0, *pred) &&
          TowCompareOperandsAreInSameIterOfLoop(currOpnd, *opnd0)) {
        valueRangeInPredPtr = CreateValueRangeOfEqualZero(predOpnd->GetPrimType());
        valueRangeInPred = valueRangeInPredPtr.get();
        dummyRightRange = valueRangeInPredPtr.get();
      }
    }
    if (ConditionEdgeCanBeDeleted(*pred, bb, valueRangeInPred, dummyRightRange, falseBranch, trueBranch, opndType, op,
        updateSSAExceptTheScalarExpr, ssaupdateCandsForCondExpr)) {
      if (updateSSAExceptTheScalarExpr != nullptr && updateSSAExceptTheScalarExpr->GetOst()->IsLocal() &&
          phi != nullptr) {
        if (updateSSAExceptTheScalarExpr->GetDefBy() == kDefByStmt) {
          // PredOpnd is only used by condGoto stmt and phi, if the condGoto stmt can be deleted, need not update ssa
          // of predOpnd and the def point of predOpnd can be deleted.
          phiOpnds[indexOfOpnd - 1] = nullptr;
          bool opndIsRemovedFromPhi =
              (std::find(phiOpnds.begin(), phiOpnds.end(), updateSSAExceptTheScalarExpr) == phiOpnds.end());
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
      if (((pred->GetKind() == kBBFallthru || pred->GetKind() == kBBGoto) && pred->GetSucc().size() == 1) &&
           pred->GetBBId() != falseBranch.GetBBId() && pred->GetBBId() != trueBranch.GetBBId() &&
           (opnd0 == nullptr || TowCompareOperandsAreInSameIterOfLoop(currOpnd, *opnd0))) {
        opt |= AnalysisValueRangeInPredsOfCondGotoBB(
            *pred, opnd0, *predOpnd, dummyRightRange, falseBranch, trueBranch, opndType, op, condGoto);
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
      ASSERT_NOT_NULL(condGoto.GetLastMe());
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
    ReplaceUsePoints(phi);
  }
  ssaupdateCandsForCondExpr.clear();
  return opt;
}

// If the opnds of phi are the same, delete the phi node and replace use points of phi node with phi opnds.
// mx13 = phi(mx12, mx12)
// if mx13
// ==>
// if mx12
void ValueRangePropagation::ReplaceUsePoints(MePhiNode *phi) {
  if (phi == nullptr || phi->GetDefBB() == nullptr ||
      phi->GetDefBB()->GetMePhiList().empty() || phi->GetOpnds().empty()) {
    return;
  }
  auto *opnd = phi->GetOpnd(0);
  for (size_t i = 1; i < phi->GetOpnds().size(); ++i) {
    if (phi->GetOpnd(i) != opnd) {
      return;
    }
  }
  auto *lhs = phi->GetLHS();
  phi->GetDefBB()->GetMePhiList().erase(lhs->GetOstIdx());
  auto *replaceMeExpr = static_cast<ScalarMeExpr*>(opnd);
  auto *useListOfPredOpnd = useInfo->GetUseSitesOfExpr(lhs);
  ASSERT_NOT_NULL(useListOfPredOpnd);
  for (auto &useItem : *useListOfPredOpnd) {
    if (useItem.IsUseByPhi()) {
      auto *usePhi = useItem.GetPhi();
      for (size_t i = 0; i < useItem.GetPhi()->GetOpnds().size(); ++i) {
        if (usePhi->GetOpnd(i) != lhs) {
          continue;
        }
        usePhi->SetOpnd(i, replaceMeExpr);
      }
    } else {
      auto *stmt = useItem.GetStmt();
      irMap.ReplaceMeExprStmt(*stmt, *lhs, *replaceMeExpr);
    }
  }
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

Opcode ValueRangePropagation::GetOpAfterSwapThePositionsOfTwoOperands(Opcode op) const {
  if (op == OP_eq) {
    return OP_eq;
  } else if (op == OP_ne) {
    return OP_ne;
  } else if (op == OP_lt) {
    return OP_gt;
  } else if (op == OP_le) {
    return OP_ge;
  } else if (op == OP_ge) {
    return OP_le;
  } else if (op == OP_gt) {
    return OP_lt;
  } else {
    CHECK_FATAL(false, "can not support");
  }
}

bool ValueRangePropagation::ConditionEdgeCanBeDeleted(BB &bb, MeExpr &opnd0, ValueRange *rightRange,
    PrimType opndType, Opcode op) {
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
  if (bb.GetKind() != kBBCondGoto) {
    return false;
  }
  defByStmtInUDChain = false;
  ASSERT_NOT_NULL(bb.GetLastMe());
  auto *opMeExpr = static_cast<CondGotoMeStmt*>(bb.GetLastMe())->GetOpnd();
  auto *opnd1 = (opMeExpr->GetNumOpnds() == kNumOperands) ? opMeExpr->GetOpnd(1) : nullptr;
  BB *trueBranch = nullptr;
  BB *falseBranch = nullptr;
  GetTrueAndFalseBranch(static_cast<CondGotoMeStmt*>(bb.GetLastMe())->GetOp(), bb, trueBranch, falseBranch);
  bool opt = AnalysisValueRangeInPredsOfCondGotoBB(
      bb, nullptr, *currOpnd, rightRange, *falseBranch, *trueBranch, opndType, op, bb);
  if (!opt && bb.GetKind() == kBBCondGoto &&
      static_cast<CondGotoMeStmt*>(bb.GetLastMe())->GetOpnd()->GetNumOpnds() == kNumOperands) {
    // Swap the positions of two operands like :
    // ||MEIR|| brfalse
    //     cond:  @@m25OP lt u1 i32 mx275
    //          opnd[0] = CONST 0 mx1
    //          opnd[1] = REGINDX:3 %3 mx92
    // ==>
    // ||MEIR|| brfalse
    //     cond:  @@m25OP gt u1 i32 mx275
    //          opnd[0] = REGINDX:3 %3 mx92
    //          opnd[1] = CONST 0 mx1

    auto *valueRangeOfOpnd0 = FindValueRangeAndInitNumOfRecursion(bb, opnd0);
    opt = AnalysisValueRangeInPredsOfCondGotoBB(bb, currOpnd, *opnd1, valueRangeOfOpnd0, *falseBranch,
        *trueBranch, opndType, GetOpAfterSwapThePositionsOfTwoOperands(op), bb);
  }
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
    MergeNotEqualRanges(opnd, leftRange, rightRange, falseBranch);
  } else if (op == OP_ne) {
    CreateValueRangeForNeOrEq(opnd, leftRange, rightRange, falseBranch, trueBranch);
    MergeNotEqualRanges(opnd, leftRange, rightRange, trueBranch);
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
  bool isAccurate = dealWithPhi && (rightRange.GetRangeType() == kEqual || rightRange.IsAccurate());
  if (op == OP_lt || op == OP_le) {
    if (leftRange != nullptr && leftRange->GetRangeType() == kNotEqual) {
      CreateValueRangeForLeOrLt(
          opnd, nullptr, newRightUpper, newRightLower, trueBranch, falseBranch, isAccurate);
    } else {
      CreateValueRangeForLeOrLt(opnd, leftRange, newRightUpper, newRightLower, trueBranch, falseBranch, isAccurate);
    }
  } else if (op == OP_gt || op == OP_ge) {
    if (leftRange != nullptr && leftRange->GetRangeType() == kNotEqual) {
      CreateValueRangeForGeOrGt(
          opnd, nullptr, newRightUpper, newRightLower, trueBranch, falseBranch, isAccurate);
    } else {
      CreateValueRangeForGeOrGt(opnd, leftRange, newRightUpper, newRightLower, trueBranch, falseBranch, isAccurate);
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
    ConditionEdgeCanBeDeleted(bb, *opnd0, &rightRange, primType, op);
  }
  CreateValueRangeForCondGoto(*opnd0, op, leftRange, rightRange, *trueBranch, *falseBranch);
}

bool ValueRangePropagation::GetValueRangeOfCondGotoOpnd(const BB &bb, const OpMeExpr &opMeExpr, MeExpr &opnd,
    ValueRange *&valueRange, std::unique_ptr<ValueRange> &rightRangePtr) {
  valueRange = FindValueRangeAndInitNumOfRecursion(bb, opnd);
  if (valueRange == nullptr) {
    if (opnd.GetMeOp() == kMeOpConst && static_cast<ConstMeExpr&>(opnd).GetConstVal()->GetKind() == kConstInt) {
      rightRangePtr = std::make_unique<ValueRange>(Bound(static_cast<ConstMeExpr&>(opnd).GetExtIntValue(),
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
      auto *valueRangeOfLHS = FindValueRangeAndInitNumOfRecursion(bb, *lhs);
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
      Bound rhsBound = Bound(static_cast<ConstMeExpr*>(rhs)->GetExtIntValue(), rhsPrimType);
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
    BB &bb, const OpMeExpr &opMeExpr, Opcode opOfBrStmt, ValueRange *valueRangeOfLeft) {
  auto opnd0 = opMeExpr.GetOpnd(0);
  auto opnd1 = opMeExpr.GetOpnd(1);
  auto conditionalOp = opMeExpr.GetOp();
  PrimType prim = opMeExpr.GetOpndType();
  if (!IsNeededPrimType(prim)) {
    return;
  }
  BB *trueBranch = nullptr;
  BB *falseBranch = nullptr;
  GetTrueAndFalseBranch(opOfBrStmt, bb, trueBranch, falseBranch);
  switch (conditionalOp) {
    case OP_ne: {
      (void)Insert2Caches(trueBranch->GetBBId(), opnd0->GetExprID(),
          std::make_unique<ValueRange>(Bound(opnd1, prim), kNotEqual));
      (void)Insert2Caches(falseBranch->GetBBId(), opnd0->GetExprID(),
          std::make_unique<ValueRange>(Bound(opnd1, prim), kEqual));
      break;
    }
    case OP_eq: {
      (void)Insert2Caches(trueBranch->GetBBId(), opnd0->GetExprID(),
          std::make_unique<ValueRange>(Bound(opnd1, prim), kEqual));
      (void)Insert2Caches(falseBranch->GetBBId(), opnd0->GetExprID(),
          std::make_unique<ValueRange>(Bound(opnd1, prim), kNotEqual));
      break;
    }
    case OP_le: {
      if (valueRangeOfLeft != nullptr && valueRangeOfLeft->GetRangeType() == kOnlyHasLowerBound) {
        (void)Insert2Caches(trueBranch->GetBBId(), opnd0->GetExprID(),
                            std::make_unique<ValueRange>(valueRangeOfLeft->GetLower(),
                                                         Bound(opnd1, prim), kLowerAndUpper, dealWithPhi));
      } else {
        (void)Insert2Caches(trueBranch->GetBBId(), opnd0->GetExprID(),
                            std::make_unique<ValueRange>(Bound(GetMinNumber(prim), prim),
                                                         Bound(opnd1, prim), kLowerAndUpper));
      }
      (void)Insert2Caches(falseBranch->GetBBId(), opnd0->GetExprID(),
          std::make_unique<ValueRange>(Bound(opnd1, 1, prim),
              Bound(GetMaxNumber(prim), prim), kLowerAndUpper));
      break;
    }
    case OP_lt: {
      if (valueRangeOfLeft != nullptr && valueRangeOfLeft->GetRangeType() == kOnlyHasLowerBound) {
        (void)Insert2Caches(trueBranch->GetBBId(), opnd0->GetExprID(),
                            std::make_unique<ValueRange>(valueRangeOfLeft->GetLower(),
                                                         Bound(opnd1, -1, prim), kLowerAndUpper, dealWithPhi));
      } else {
        (void)Insert2Caches(trueBranch->GetBBId(), opnd0->GetExprID(),
                            std::make_unique<ValueRange>(Bound(GetMinNumber(prim), prim),
                                                         Bound(opnd1, -1, prim), kLowerAndUpper));
      }
      (void)Insert2Caches(falseBranch->GetBBId(), opnd0->GetExprID(),
          std::make_unique<ValueRange>(Bound(opnd1, prim), Bound(GetMaxNumber(prim), prim), kLowerAndUpper));
      break;
    }
    case OP_ge: {
      if (valueRangeOfLeft != nullptr && valueRangeOfLeft->GetRangeType() == kOnlyHasUpperBound) {
        (void)Insert2Caches(trueBranch->GetBBId(), opnd0->GetExprID(), std::make_unique<ValueRange>(
            Bound(opnd1, prim), valueRangeOfLeft->GetBound(), kLowerAndUpper, dealWithPhi));
      } else {
        (void)Insert2Caches(trueBranch->GetBBId(), opnd0->GetExprID(),
            std::make_unique<ValueRange>(Bound(opnd1, prim),
                Bound(GetMaxNumber(prim), prim), kLowerAndUpper));
      }
      (void)Insert2Caches(falseBranch->GetBBId(), opnd0->GetExprID(),
          std::make_unique<ValueRange>(Bound(GetMinNumber(prim), prim),
              Bound(opnd1, -1, prim), kLowerAndUpper));
      break;
    }
    case OP_gt: {
      if (valueRangeOfLeft != nullptr && valueRangeOfLeft->GetRangeType() == kOnlyHasUpperBound) {
        (void)Insert2Caches(trueBranch->GetBBId(), opnd0->GetExprID(), std::make_unique<ValueRange>(
            Bound(opnd1, 1, prim), valueRangeOfLeft->GetBound(), kLowerAndUpper, dealWithPhi));
      } else {
        (void)Insert2Caches(trueBranch->GetBBId(), opnd0->GetExprID(),
            std::make_unique<ValueRange>(Bound(opnd1, 1, prim), Bound(GetMaxNumber(prim), prim), kLowerAndUpper));
      }
      (void)Insert2Caches(falseBranch->GetBBId(), opnd0->GetExprID(),
          std::make_unique<ValueRange>(Bound(GetMinNumber(prim), prim),
              Bound(opnd1, prim), kLowerAndUpper));
      break;
    }
    default:
      break;
  }
}

void ValueRangePropagation::GetValueRangeForUnsignedInt(const BB &bb, const OpMeExpr &opMeExpr, const MeExpr &opnd,
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
bool ValueRangePropagation::DealWithSpecialCondGoto(const BB &bb, const OpMeExpr &opMeExpr, const ValueRange &leftRange,
                                                    ValueRange &rightRange, CondGotoMeStmt &brMeStmt) {
  if (onlyPropVR) {
    return false;
  }
  if (opMeExpr.GetOp() != OP_gt && opMeExpr.GetOp() != OP_lt) {
    return false;
  }
  if (opMeExpr.GetNumOpnds() != kNumOperands) {
    return false;
  }
  if (rightRange.GetRangeType() != kEqual || !rightRange.IsConstant()) {
    return false;
  }
  if (leftRange.GetRangeType() != kLowerAndUpper) {
    return false;
  }
  if (leftRange.GetLower().GetVar() != nullptr || leftRange.GetUpper().GetVar() != nullptr ||
      leftRange.GetLower().IsGreaterThanOrEqualTo(leftRange.GetUpper(), opMeExpr.GetOpndType())) {
    return false;
  }
  if (opMeExpr.GetOp() == OP_gt) {
    if (!leftRange.GetLower().IsEqual(rightRange.GetBound(), opMeExpr.GetOpndType())) {
      return false;
    }
    auto *newExpr = irMap.CreateMeExprCompare(OP_ne, opMeExpr.GetPrimType(),
                                              opMeExpr.GetOpndType(), *opMeExpr.GetOpnd(0), *opMeExpr.GetOpnd(1));
    (void)irMap.ReplaceMeExprStmt(brMeStmt, opMeExpr, *newExpr);
    return true;
  }
  auto *loop = loops->GetBBLoopParent(bb.GetBBId());
  if (loop != nullptr && IsLoopVariable(*loop, *opMeExpr.GetOpnd(0))) {
    return false;
  }
  // Deal with op lt like this case:
  // if opnd0 < 1
  // vr(opnd0) : 0, 255
  // ==>
  // if opnd0 == 0
  if (!leftRange.GetUpper().IsGreaterThanOrEqualTo(rightRange.GetBound(), opMeExpr.GetOpndType())) {
    return false;
  }
  int64 res = 0;
  if (AddOrSubWithConstant(leftRange.GetLower().GetPrimType(), OP_add, leftRange.GetLower().GetConstant(), 1, res)) {
    if (!Bound(res, opMeExpr.GetOpndType()).IsEqual(rightRange.GetBound(), opMeExpr.GetOpndType())) {
      return false;
    }
  } else {
    return false;
  }
  auto *newConstExpr = irMap.CreateIntConstMeExpr(leftRange.GetLower().GetConstant(), opMeExpr.GetOpndType());
  auto *newExpr = irMap.CreateMeExprCompare(OP_eq, opMeExpr.GetPrimType(),
                                            opMeExpr.GetOpndType(), *opMeExpr.GetOpnd(0), *newConstExpr);
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
    std::unique_ptr<ValueRange> leftRangePtr = std::make_unique<ValueRange>(
        Bound(static_cast<ConstMeExpr &>(opnd).GetExtIntValue(), opnd.GetPrimType()), kEqual);
    DealWithCondGoto(bb, op, leftRangePtr.get(), *rightRangePtr.get(), stmt);
  } else {
    ValueRange *leftRange = FindValueRangeAndInitNumOfRecursion(bb, opnd);
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
  if (opnd1->IsScalar() && static_cast<ScalarMeExpr*>(opnd1)->GetDefBy() == kDefByPhi) {
    auto &opnds = static_cast<ScalarMeExpr*>(opnd1)->GetDefPhi().GetOpnds();
    if (opnds.size() == kNumOperands &&
        opnds[0]->GetDefBy() == kDefByStmt &&
        opnds[1]->GetDefBy() == kDefByStmt &&
        opnds[0]->GetDefStmt()->GetRHS()->GetMeOp() == kMeOpConst &&
        opnds[1]->GetDefStmt()->GetRHS()->GetMeOp() == kMeOpConst &&
        static_cast<ConstMeExpr*>(opnds[0]->GetDefStmt()->GetRHS())->GetConstVal()->GetKind() == kConstInt &&
        static_cast<ConstMeExpr*>(opnds[1]->GetDefStmt()->GetRHS())->GetConstVal()->GetKind() == kConstInt &&
        static_cast<ConstMeExpr*>(opnds[0]->GetDefStmt()->GetRHS())->GetIntValue() == 0 &&
        static_cast<ConstMeExpr*>(opnds[1]->GetDefStmt()->GetRHS())->GetIntValue() == 1) {
      // If the condition is shortcircuit, it will be opt later, need not push the opnds to pairOfExprs.
      return;
    }
  }
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
bool ValueRangePropagation::AnalysisUnreachableForGeOrGt(BB &bb, const CondGotoMeStmt &brMeStmt,
                                                         const ValueRange &leftRange) {
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
bool ValueRangePropagation::AnalysisUnreachableForEqOrNe(BB &bb, const CondGotoMeStmt &brMeStmt,
                                                         const ValueRange &leftRange) {
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
  auto primType = static_cast<OpMeExpr*>(brMeStmt.GetOpnd())->GetOpndType();
  MeExpr *opnd1 = static_cast<OpMeExpr*>(brMeStmt.GetOpnd())->GetOpnd(1);
  // Example1: deal with like if (mx1 > mx2), when the valuerange of mx1 is [mx2+1, max(mx2_type)]
  // Example2: deal with like if (mx1 >= mx2), when the valuerange of mx1 is [mx2, max(mx2_type)]
  if (leftRange.GetLower().GetVar() == opnd1 && leftRange.GetUpper().GetVar() == nullptr &&
      leftRange.UpperIsMax(primType)) {
    return AnalysisUnreachableForGeOrGt(bb, brMeStmt, leftRange);
  // Example1: deal with like if (mx1 < mx2), when the valuerange of mx1 is [min(mx2_type), mx2-1]
  // Example2: deal with like if (mx1 <= mx2), when the valuerange of mx1 is [min(mx2_type), mx2]
  } else if (leftRange.GetLower().GetVar() == nullptr && leftRange.GetUpper().GetVar() == opnd1 &&
      leftRange.LowerIsMin(primType)) {
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
  if (rightRange == nullptr || rightRange->IsUniversalSet()) {
    if (!ConditionEdgeCanBeDeleted(bb, *opnd0, rightRange, opMeExpr->GetOpndType(), opMeExpr->GetOp())) {
      DealWithCondGotoWhenRightRangeIsNotExist(bb, *opMeExpr, brMeStmt.GetOp(), leftRange);
    }
    return;
  }
  if (leftRange == nullptr && rightRange->GetRangeType() != kEqual) {
    return;
  }
  if (leftRange == nullptr) {
    GetValueRangeForUnsignedInt(bb, *opMeExpr, *opnd0, leftRange, leftRangePtr);
  }
  if (leftRange != nullptr) {
    if (opMeExpr->GetOp() == OP_gt && DealWithSpecialCondGoto(bb, *opMeExpr, *leftRange, *rightRange, brMeStmt)) {
      return DealWithCondGoto(bb, stmt);
    }
    if (opMeExpr->GetOp() == OP_lt && DealWithSpecialCondGoto(bb, *opMeExpr, *leftRange, *rightRange, brMeStmt)) {
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
  ValueRangePropagation::isDebug = DEBUGFUNC_NEWPM(f);
  f.vrpRuns++;
  auto *irMap = GET_ANALYSIS(MEIRMapBuild, f);
  CHECK_FATAL(irMap != nullptr, "irMap phase has problem");
  auto *dom = EXEC_ANALYSIS(MEDominance, f)->GetDomResult();
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
  ValueRangePropagation valueRangePropagation(f, *irMap, *dom, meLoop, *valueRangeMemPool, cands, sa, true);

  SafetyCheck safetyCheck; // dummy
  valueRangePropagation.SetSafetyBoundaryCheck(safetyCheck);
  valueRangePropagation.SetSafetyNonnullCheck(safetyCheck);

  valueRangePropagation.Execute();
  (void)f.GetCfg()->UnreachCodeAnalysis(true);
  f.GetCfg()->WontExitAnalysis();
  // split critical edges
  bool split = MeSplitCEdge(false).SplitCriticalEdgeForMeFunc(f);
  if (split || valueRangePropagation.IsCFGChange()) {
    GetAnalysisInfoHook()->ForceEraseAnalysisPhase(f.GetUniqueID(), &MEDominance::id);
    dom = FORCE_EXEC(MEDominance)->GetDomResult();
  }
  if (valueRangePropagation.IsCFGChange()) {
    if (ValueRangePropagation::isDebug) {
      f.GetCfg()->DumpToFile("valuerange-after" + std::to_string(f.vrpRuns));
    }
    if (valueRangePropagation.NeedUpdateSSA()) {
      MeSSAUpdate ssaUpdate(f, *f.GetMeSSATab(), *dom, cands);
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
    GetAnalysisInfoHook()->ForceRunTransFormPhase<MeFuncOptTy, MeFunction>(&MELoopCanon::id, f);
    // update cfg frequency
    if (f.GetCfg()->UpdateCFGFreq()) {
      if (f.GetCfg()->DumpIRProfileFile()) {
        f.GetCfg()->DumpToFile("after-valuerange" + std::to_string(f.vrpRuns), false, true);
      }
    }
  }
  // Run vrp twice when need check boundary and nullable pointer.
  if (MeOption::boundaryCheckMode != kNoCheck || MeOption::npeCheckMode != kNoCheck) {
    auto *hook = GetAnalysisInfoHook();
    dom = static_cast<MEDominance*>(
        hook->ForceRunAnalysisPhase<MapleFunctionPhase<MeFunction>>(&MEDominance::id, f))->GetDomResult();
    meLoop = static_cast<MELoopAnalysis*>(
        hook->ForceRunAnalysisPhase<MapleFunctionPhase<MeFunction>>(&MELoopAnalysis::id, f))->GetResult();
    ValueRangePropagation valueRangePropagationWithOPTAssert(
        f, *irMap, *dom, meLoop, *valueRangeMemPool, cands, sa, true, true);
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
    if (ValueRangePropagation::isDebug) {
      LogInfo::MapleLogger() << "***************after check***************" << "\n";
      f.Dump(false);
      f.GetCfg()->DumpToFile("valuerange-after-safe" + std::to_string(f.vrpRuns));
    }
  }
  return true;
}
}  // namespace maple
