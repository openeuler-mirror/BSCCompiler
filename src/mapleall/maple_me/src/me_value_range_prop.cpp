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
constexpr size_t kFourByte = 4;
constexpr size_t kEightByte = 8;
constexpr size_t kCodeSizeLimit = 2000;
constexpr std::uint64_t kInvaliedBound = 0xdeadbeef;

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
  // In reverse post order traversal, a bb is accessed before any of its successor bbs. So the range of def points would
  // be calculated before and need not calculate the range of use points repeatedly.
  for (auto *bb : dom.GetReversePostOrder()) {
    if (unreachableBBs.find(bb) != unreachableBBs.end()) {
      continue;
    }
    for (auto &it : bb->GetMePhiList()) {
      DealWithPhi(*bb, *it.second);
    }
    for (auto it = bb->GetMeStmts().begin(); it != bb->GetMeStmts().end();) {
      bool deleteStmt = false;
      ReplaceOpndWithConstMeExpr(*bb, *it);
      for (size_t i = 0; i < it->NumMeStmtOpnds(); ++i) {
        DealWithOperand(*bb, *it, *it->GetOpnd(i));
      }
      if (MeOption::safeRegionMode && (it->IsInSafeRegion() || func.IsSafe()) &&
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
          DealWithAssign(*bb, *it);
          break;
        }
        case OP_brfalse:
        case OP_brtrue: {
          DealWithCondGoto(*bb, *it);
          InsertValueRangeOfCondExpr2Caches(*bb, *it);
          break;
        }
        CASE_OP_ASSERT_NONNULL {
          // If the option safeRegion is open and the stmt is not in safe region, delete it.
          if (MeOption::safeRegionMode && !it->IsInSafeRegion() && func.IsUnSafe()) {
            deleteStmt = true;
          } else if (DealWithAssertNonnull(*bb, *it)) {
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
          if (MeOption::safeRegionMode && !it->IsInSafeRegion() && func.IsUnSafe()) {
            deleteStmt = true;
          } else if (DealWithBoundaryCheck(*bb, *it)) {
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
          DealWithCallassigned(*bb, *it);
          break;
        }
        case OP_switch: {
          DealWithSwitch(*it);
          break;
        }
        case OP_igoto:
        default:
          break;
      }
      if (deleteStmt) {
        bb->GetMeStmts().erase(it++);
      } else {
        ++it;
      }
    }
  }
  if (ValueRangePropagation::isDebug) {
    DumpCaches();
  }
  DeleteUnreachableBBs();
}

void ValueRangePropagation::DealWithCallassigned(BB &bb, MeStmt &stmt) {
  auto &callassign = static_cast<CallMeStmt&>(stmt);
  MIRFunction *callFunc = GlobalTables::GetFunctionTable().GetFunctionFromPuidx(callassign.GetPUIdx());
  if (callFunc->GetAttr(FUNCATTR_nonnull) && callassign.GetAssignedLHS() != nullptr) {
    Insert2Caches(bb.GetBBId(), callassign.GetAssignedLHS()->GetExprID(),
        std::make_unique<ValueRange>(Bound(nullptr, 0, callassign.GetAssignedLHS()->GetPrimType()), kNotEqual));
  }
}

void ValueRangePropagation::DealWithSwitch(MeStmt &stmt) {
  auto &switchMeStmt = static_cast<SwitchMeStmt&>(stmt);
  auto *opnd = switchMeStmt.GetOpnd();
  std::set<BBId> bbs;
  for (auto &pair : switchMeStmt.GetSwitchTable()) {
    auto *bb = func.GetCfg()->GetLabelBBAt(pair.second);
    // Can prop value range to target bb only when the pred size of target bb is one and
    // only one case can jump to the target bb.
    if (bbs.find(bb->GetBBId()) == bbs.end() && bb->GetPred().size() == 1) {
      Insert2Caches(bb->GetBBId(), opnd->GetExprID(),
                    std::make_unique<ValueRange>(Bound(nullptr, pair.first, PTY_i64), kEqual));
    } else {
      Insert2Caches(bb->GetBBId(), opnd->GetExprID(), nullptr);
    }
    bbs.insert(bb->GetBBId());
  }
}

void ValueRangePropagation::DeleteThePhiNodeWhichOnlyHasOneOpnd(BB &bb) {
  if (unreachableBBs.find(&bb) != unreachableBBs.end()) {
    return;
  }
  if (bb.GetMePhiList().empty()) {
    return;
  }
  if (bb.GetPred().size() == 1) {
    InsertOstOfPhi2CandsForSSAUpdate(bb, bb);
    bb.GetMePhiList().clear();
    needUpdateSSA = true;
  }
}

bool ValueRange::IsEqual(ValueRange *valueRangeRight) {
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
std::unique_ptr<ValueRange> ValueRangePropagation::ZeroIsInRange(ValueRange &valueRange) {
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

std::unique_ptr<ValueRange> ValueRangePropagation::FindValueRangeInCurrBBOrDominateBBs(BB &bb, MeExpr &opnd) {
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
    std::unique_ptr<ValueRange> &valueRangeOfIndex, std::unique_ptr<ValueRange> &valueRangOfOpnd, int64 constant) {
  return (valueRangeOfIndex == nullptr) ?
      std::move(valueRangOfOpnd) : AddOrSubWithValueRange(OP_add, *valueRangeOfIndex, constant);
}

// Get the valueRange of index, for example:
// assertge(p, p + 8) -> the valueRange of index is 8
// assertge(p, p + X * 4 - 8) -> the valueRange of index is X - 2
std::unique_ptr<ValueRange> ValueRangePropagation::GetTheValueRangeOfIndex(
    BB &bb, MeStmt &meStmt, CRAddNode &crAddNode, uint32 &byteSize, MeExpr *lengthExpr,
    ValueRange *valueRangeOfLengthPtr) {
  std::unique_ptr<ValueRange> valueRangeOfIndex = nullptr;
  for (size_t i = 1; i < crAddNode.GetOpndsSize(); ++i) {
    auto *opndOfCRNode = crAddNode.GetOpnd(i);
    if (opndOfCRNode->GetCRType() != kCRConstNode && opndOfCRNode->GetCRType() != kCRVarNode &&
        opndOfCRNode->GetCRType() != kCRMulNode) {
      return nullptr;
    }
    if (opndOfCRNode->SizeOfOperand() == 1) {
      // Deal with the operand which is a constant.
      if (opndOfCRNode->GetCRType() == kCRConstNode) {
        int64 constant = (byteSize == 0) ? static_cast<CRConstNode*>(opndOfCRNode)->GetConstValue() :
            static_cast<CRConstNode*>(opndOfCRNode)->GetConstValue() / byteSize;
        valueRangeOfIndex = (valueRangeOfIndex == nullptr) ?
            std::make_unique<ValueRange>(Bound(constant, meStmt.GetOpnd(0)->GetPrimType()), kEqual) :
            AddOrSubWithValueRange(OP_add, *valueRangeOfIndex, constant);
        continue;
      }
      // Deal with the operand which is a meexpr.
      if (opndOfCRNode->GetExpr() == nullptr) {
        return nullptr;
      }
      auto valueRangOfOpnd = FindValueRangeInCurrBBOrDominateBBs(bb, *opndOfCRNode->GetExpr());
      if (valueRangOfOpnd == nullptr || valueRangOfOpnd->GetRangeType() == kNotEqual ||
          valueRangOfOpnd->GetRangeType() == kOnlyHasUpperBound ||
          valueRangOfOpnd->GetRangeType() == kOnlyHasLowerBound) {
        return nullptr;
      }
      if (valueRangOfOpnd->IsConstant()) {
        int64 constant = (valueRangOfOpnd->GetRangeType() == kEqual) ? valueRangOfOpnd->GetBound().GetConstant() :
            valueRangOfOpnd->GetLower().GetConstant();
        constant = (byteSize == 0) ? constant : constant / byteSize;
        valueRangeOfIndex = (valueRangeOfIndex == nullptr) ? std::move(valueRangOfOpnd) :
            AddOrSubWithValueRange(OP_add, *valueRangeOfIndex, constant);
      } else {
        valueRangeOfIndex = (valueRangeOfIndex == nullptr) ? std::move(valueRangOfOpnd) :
            AddOrSubWithValueRange(OP_add, *valueRangeOfIndex, *valueRangOfOpnd);
      }
      if (valueRangeOfIndex == nullptr) {
        return nullptr;
      }
      continue;
    }
    // Deal with the operand which is a mul expr.
    if (opndOfCRNode->GetCRType() == kCRMulNode) {
      auto *crMulNode = static_cast<CRMulNode*>(opndOfCRNode);
      // The second of crMulNode must be the bytesize.
      if (crMulNode->SizeOfOperand() != kNumOperands || crMulNode->GetOpnd(1)->GetCRType() != kCRConstNode) {
        return nullptr;
      }
      if (crMulNode->GetOpnd(0)->GetExpr() == nullptr) {
        return nullptr;
      }
      auto valueRangOfOpndLeftOperand = FindValueRangeInCurrBBOrDominateBBs(bb, *crMulNode->GetOpnd(0)->GetExpr());
      if (valueRangOfOpndLeftOperand == nullptr) {
        if (lengthExpr != nullptr && crMulNode->GetOpnd(0)->GetExpr() == lengthExpr) {
          valueRangeOfIndex = (valueRangeOfIndex == nullptr) ?
              CopyValueRange(*valueRangeOfLengthPtr, valueRangeOfLengthPtr->GetLower().GetPrimType()) :
              AddOrSubWithValueRange(OP_add, *valueRangeOfIndex, *valueRangeOfLengthPtr);
          continue;
        } else {
          return nullptr;
        }
      }
      if (valueRangOfOpndLeftOperand->GetRangeType() == kNotEqual ||
          valueRangOfOpndLeftOperand->GetRangeType() == kOnlyHasUpperBound ||
          valueRangOfOpndLeftOperand->GetRangeType() == kOnlyHasLowerBound) {
        return nullptr;
      }
      byteSize = static_cast<CRConstNode*>(crMulNode->GetOpnd(1))->GetConstValue();
      valueRangeOfIndex = (valueRangeOfIndex == nullptr) ? std::move(valueRangOfOpndLeftOperand) :
          AddOrSubWithValueRange(OP_add, *valueRangeOfIndex, *valueRangOfOpndLeftOperand);
      if (valueRangeOfIndex == nullptr) {
        return nullptr;
      }
    }
  }
  return valueRangeOfIndex;
}

void SafetyCheckWithBoundaryError::HandleBoundaryCheck(
    const BB &bb, MeStmt &meStmt, MeExpr &indexOpnd, MeExpr &boundOpnd) {
  std::set<MePhiNode*> visitedPhi;
  std::vector<MeStmt*> stmts{ &meStmt };
  vrp.JudgeTheConsistencyOfDefPointsOfBoundaryCheck(bb, indexOpnd, visitedPhi, stmts);
  visitedPhi.clear();
  stmts.clear();
  stmts.push_back(&meStmt);
  vrp.JudgeTheConsistencyOfDefPointsOfBoundaryCheck(bb, boundOpnd, visitedPhi, stmts);
}

void SafetyCheckWithNonnullError::HandleAssertNonnull(const MeStmt &meStmt, const ValueRange &valueRangeOfIndex) {
  if (valueRangeOfIndex.IsEqualZero()) {
    Error(meStmt);
  }
}

void SafetyCheckWithBoundaryError::HandleAssertge(const MeStmt &meStmt, const ValueRange &valueRangeOfIndex) {
  if (valueRangeOfIndex.IsLessThanZero()) {
    Error(meStmt);
  }
}

void SafetyCheckWithBoundaryError::HandleAssertltOrAssertle(
    const MeStmt &meStmt, Opcode op, int64 indexValue, int64 lengthValue) {
  if (kOpcodeInfo.IsAssertLeBoundary((op))) {
    if (indexValue > lengthValue) {
      Error(meStmt);
    }
  } else {
    if (indexValue >= lengthValue) {
      Error(meStmt);
    }
  }
}

// Deal with lower boundary check.
bool ValueRangePropagation::DealWithAssertGe(BB &bb, MeStmt &meStmt, CRNode &indexCR, CRNode &boundCR) {
  // The cr of lower bound must only have base address.
  if (boundCR.SizeOfOperand() != 1) {
    return false;
  }
  // If the index equal to the lower bound, return true.
  if (indexCR.SizeOfOperand() == 1) {
    if (meStmt.GetOpnd(0) == meStmt.GetOpnd(1)) {
      return true;
    }
    if (boundCR.GetExpr() != nullptr && indexCR.GetExpr() != nullptr && boundCR.GetExpr() == indexCR.GetExpr()) {
      return true;
    }
  }
  // The cr of index must like this pattern: base address + offset.
  // For example:
  // int *p,
  // p[i] = c, --> lower bound check : assertge(p + i * 4, p)
  if (indexCR.GetCRType() != kCRAddNode) {
    return false;
  }
  auto *crADDNode = static_cast<CRAddNode*>(&indexCR);
  if (crADDNode->GetOpnd(0)->GetExpr() == nullptr || boundCR.GetExpr() == nullptr ||
      (crADDNode->GetOpnd(0)->GetExpr() != boundCR.GetExpr())) {
    // If the base address of index and lower bound are not equal, return false.
    return false;
  }
  uint32 byteSize = 0;
  std::unique_ptr<ValueRange> valueRangeOfIndex = GetTheValueRangeOfIndex(
      bb, meStmt, *crADDNode, byteSize, nullptr, nullptr);
  if (valueRangeOfIndex != nullptr) {
    if (valueRangeOfIndex->IsBiggerThanZero()) {
      return true;
    }
    safetyCheckBoundary->HandleAssertge(meStmt, *valueRangeOfIndex);
  }
  return false;
}

bool ValueRangePropagation::GetTheValueRangeOfArrayLength(BB &bb, MeStmt &meStmt, CRAddNode &crADDNodeOfBound,
    MeExpr *&lengthExpr, int64 &lengthValue, ValueRange *&valueRangeOfLengthPtr,
    std::unique_ptr<ValueRange> &valueRangeOfLength, uint32 &byteSize) {
  // Get the valueRange of upper bound.
  if (crADDNodeOfBound.GetOpnd(1)->GetCRType() == kCRMulNode) {
    // If the upper bound is a mul expr, the first operand of mul node must be the length and the second operand must
    // be the sizeType, for example:
    // assertlt(p, p + len * 4) --> the length is len and the byteSize is 4.
    auto *crMulNodeOfBound = static_cast<CRMulNode*>(crADDNodeOfBound.GetOpnd(1));
    if (crMulNodeOfBound->GetOpnd(1)->GetCRType() != kCRConstNode) {
      return false;
    }
    byteSize = static_cast<CRConstNode*>(crMulNodeOfBound->GetOpnd(1))->GetConstValue();
    CHECK_FATAL(byteSize > 0, "byte size must greater than zero");
    if (crMulNodeOfBound->GetOpnd(0)->GetCRType() != kCRVarNode || crMulNodeOfBound->GetOpnd(0)->GetExpr() == nullptr) {
      return false;
    }
    lengthExpr = crMulNodeOfBound->GetOpnd(0)->GetExpr();
  } else if (crADDNodeOfBound.GetOpnd(1)->GetCRType() == kCRConstNode) {
    lengthValue = static_cast<CRConstNode*>(crADDNodeOfBound.GetOpnd(1))->GetConstValue();
  } else if (crADDNodeOfBound.GetOpnd(1)->GetCRType() == kCRVarNode) {
    if (crADDNodeOfBound.GetOpnd(1)->GetExpr() == nullptr) {
      return false;
    }
    lengthExpr = crADDNodeOfBound.GetOpnd(1)->GetExpr();
  }
  if (lengthExpr != nullptr) {
    valueRangeOfLength = FindValueRangeInCurrBBOrDominateBBs(bb, *lengthExpr);
    valueRangeOfLengthPtr = valueRangeOfLength.get();
    if (valueRangeOfLength == nullptr ||
        (valueRangeOfLength->GetRangeType() != kEqual && valueRangeOfLength->GetRangeType() != kLowerAndUpper)) {
      valueRangeOfLength = std::make_unique<ValueRange>(Bound(lengthExpr, 0, lengthExpr->GetPrimType()),
                                                        kEqual);
      valueRangeOfLengthPtr = valueRangeOfLength.get();
    }
  } else {
    valueRangeOfLength = std::make_unique<ValueRange>(
        Bound(nullptr, lengthValue, meStmt.GetOpnd(1)->GetPrimType()), kEqual);
    valueRangeOfLengthPtr = valueRangeOfLength.get();
  }
  return true;
}

bool ValueRangePropagation::CompareIndexWithUpper(MeStmt &meStmt, MeExpr &baseAddress, ValueRange &valueRangeOfIndex,
    int64 lengthValue, ValueRange &valueRangeOfLengthPtr, uint32 byteSize, Opcode op) {
  // Opt array boundary check when the array length is a constant.
  if (valueRangeOfLengthPtr.IsConstant()) {
    if (!valueRangeOfIndex.IsConstantLowerAndUpper() ||
        valueRangeOfIndex.GetLower().GetConstant() > valueRangeOfIndex.GetUpper().GetConstant()) {
      return false;
    }
    if (valueRangeOfLengthPtr.GetBound().GetConstant() < 0) {
      // The value of length is overflow, need deal with this case later.
    }
    if (byteSize != 0) {
      lengthValue = valueRangeOfLengthPtr.GetBound().GetConstant() / byteSize;
    }
    safetyCheckBoundary->HandleAssertltOrAssertle(meStmt, op, valueRangeOfIndex.GetLower().GetConstant(), lengthValue);
    return (kOpcodeInfo.IsAssertLeBoundary((op))) ? valueRangeOfIndex.GetUpper().GetConstant() <= lengthValue :
        valueRangeOfIndex.GetUpper().GetConstant() < lengthValue;
  }
  // Opt array boundary check when the array length is a var.
  if (valueRangeOfIndex.GetRangeType() == kSpecialUpperForLoop) {
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
      sa.SortCROperand(*crNode, baseAddress);
      if (crNode->GetCRType() != kCRAddNode ||
          static_cast<CRAddNode*>(crNode)->GetOpndsSize() != kNumOperands ||
          static_cast<CRAddNode*>(crNode)->GetOpnd(0)->GetExpr() == nullptr ||
          static_cast<CRAddNode*>(crNode)->GetOpnd(0)->GetExpr() != valueRangeOfLengthPtr.GetBound().GetVar() ||
          static_cast<CRAddNode*>(crNode)->GetOpnd(1)->GetCRType() != kCRConstNode) {
        return false;
      }
      safetyCheckBoundary->HandleAssertltOrAssertle(meStmt, op,
          static_cast<CRConstNode*>(static_cast<CRAddNode*>(crNode)->GetOpnd(1))->GetConstValue(),
              valueRangeOfLengthPtr.GetBound().GetConstant());
      return (kOpcodeInfo.IsAssertLeBoundary(op) ?
          static_cast<CRConstNode*>(static_cast<CRAddNode*>(crNode)->GetOpnd(1))->GetConstValue() <=
              valueRangeOfLengthPtr.GetBound().GetConstant() :
          static_cast<CRConstNode*>(static_cast<CRAddNode*>(crNode)->GetOpnd(1))->GetConstValue() <
              valueRangeOfLengthPtr.GetBound().GetConstant());
    }
  } else if ((valueRangeOfIndex.GetLower().GetVar() != valueRangeOfIndex.GetUpper().GetVar()) ||
             valueRangeOfIndex.GetUpper().GetVar() != valueRangeOfLengthPtr.GetBound().GetVar() ||
             valueRangeOfIndex.GetLower().GetConstant() > valueRangeOfIndex.GetUpper().GetConstant()) {
    return false;
  }
  safetyCheckBoundary->HandleAssertltOrAssertle(meStmt, op, valueRangeOfIndex.GetUpper().GetConstant(),
      valueRangeOfLengthPtr.GetBound().GetConstant());
  return kOpcodeInfo.IsAssertLeBoundary(op) ?
      valueRangeOfIndex.GetUpper().GetConstant() <= valueRangeOfLengthPtr.GetBound().GetConstant() :
      valueRangeOfIndex.GetUpper().GetConstant() < valueRangeOfLengthPtr.GetBound().GetConstant();
}

// Deal with upper boundary check.
bool ValueRangePropagation::DealWithAssertLtOrLe(BB &bb, MeStmt &meStmt, CRNode &indexCR, CRNode &boundCR, Opcode op) {
  // The cr of upper bound must be like this: base address + len * byteSize.
  // For example:
  // int *p,
  // p[i] = c, --> upper bound check : assertge(p + i * 4, p + len * 4)
  if (boundCR.SizeOfOperand() != kNumOperands && boundCR.GetCRType() != kCRAddNode) {
    return false;
  }
  auto *crADDNodeOfBound = static_cast<CRAddNode*>(&boundCR);
  uint32 byteSize = 0;
  MeExpr *lengthExpr = nullptr;
  int64 lengthValue = 0;
  ValueRange *valueRangeOfLengthPtr = nullptr;
  std::unique_ptr<ValueRange> valueRangeOfLength;
  if (!GetTheValueRangeOfArrayLength(bb, meStmt, *crADDNodeOfBound, lengthExpr, lengthValue, valueRangeOfLengthPtr,
                                     valueRangeOfLength, byteSize)) {
    return false;
  }
  if (indexCR.SizeOfOperand() == 1) {
    if ((indexCR.GetExpr() == nullptr || crADDNodeOfBound->GetOpnd(0)->GetExpr() == nullptr ||
         indexCR.GetExpr() != crADDNodeOfBound->GetOpnd(0)->GetExpr())) {
      // If the base address of index and upper bound are not equal, return false.
      return false;
    }
    return valueRangeOfLengthPtr->IsBiggerThanZero();
  }
  if (indexCR.GetCRType() != kCRAddNode) {
    return false;
  }
  auto *crAddNodeOfIndex = static_cast<CRAddNode*>(&indexCR);
  if (crADDNodeOfBound->GetOpnd(0)->GetExpr() == nullptr || crAddNodeOfIndex->GetOpnd(0)->GetExpr() == nullptr ||
      (crADDNodeOfBound->GetOpnd(0)->GetExpr() != crAddNodeOfIndex->GetOpnd(0)->GetExpr())) {
    // If the base address of index and upper bound are not equal, return false.
    return false;
  }
  auto *baseAddress = crADDNodeOfBound->GetOpnd(0)->GetExpr();
  std::unique_ptr<ValueRange> valueRangeOfIndex = GetTheValueRangeOfIndex(
      bb, meStmt, *crAddNodeOfIndex, byteSize, lengthExpr, valueRangeOfLengthPtr);
  if (valueRangeOfIndex == nullptr) {
    return false;
  }
  return CompareIndexWithUpper(meStmt, *baseAddress, *valueRangeOfIndex, lengthValue, *valueRangeOfLengthPtr,
                               byteSize, op);
}

// If the bb of the boundary check which is analysised befor dominates the bb of the boundary check which is
// analysised now, then eliminate the current boundary check.
bool ValueRangePropagation::IfAnalysisedBefore(BB &bb, MeStmt &stmt) {
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
    const BB &bb, MeStmt *meStmt, MeExpr &boundOpnd, bool updateCaches) {
  if (boundOpnd.GetMeOp() == kMeOpConst &&
      static_cast<const ConstMeExpr&>(boundOpnd).GetConstVal()->GetKind() == kConstInt &&
      static_cast<const ConstMeExpr&>(boundOpnd).GetIntValue() == kInvaliedBound) {
    if (updateCaches) {
      Insert2AnalysisedArrayChecks(bb.GetBBId(), *meStmt->GetOpnd(1), *meStmt->GetOpnd(0), meStmt->GetOp());
    }
    return true;
  } else {
    auto *valueRange = FindValueRange(bb, boundOpnd);
    if (valueRange != nullptr && valueRange->GetRangeType() != kNotEqual &&
        valueRange->IsConstant() && valueRange->GetLower().GetConstant() == kInvaliedBound) {
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
    const BB &bb, MeExpr &expr, std::set<MePhiNode*> &visitedPhi, std::vector<MeStmt*> &stmts) {
  if (TheValueOfOpndIsInvaliedInABCO(bb, nullptr, expr, false)) {
    std::string errorLog = "error: pointer assigned from multibranch requires the bounds info for all branches.\n";
    for (size_t i = 0; i < stmts.size(); ++i) {
      auto &srcPosition = stmts[i]->GetSrcPosition();
      errorLog += func.GetMIRModule().GetFileNameFromFileNum(srcPosition.FileNum()) + ":" +
          std::to_string(srcPosition.LineNum()) + "\n";
    }
    FATAL(kLncFatal, "%s", errorLog.c_str());
  }
  if (!expr.IsScalar()) {
    for (size_t i = 0; i < expr.GetNumOpnds(); ++i) {
      JudgeTheConsistencyOfDefPointsOfBoundaryCheck(bb, *expr.GetOpnd(i), visitedPhi, stmts);
    }
    return;
  }
  ScalarMeExpr &scalar = static_cast<ScalarMeExpr&>(expr);
  if (scalar.GetDefBy() == kDefByStmt) {
    stmts.push_back(scalar.GetDefStmt());
    JudgeTheConsistencyOfDefPointsOfBoundaryCheck(bb, *scalar.GetDefStmt()->GetRHS(), visitedPhi, stmts);
    stmts.pop_back();
  } else if (scalar.GetDefBy() == kDefByPhi) {
    MePhiNode &phi = scalar.GetDefPhi();
    if (visitedPhi.find(&phi) != visitedPhi.end()) {
      return;
    }
    visitedPhi.insert(&phi);
    for (auto &opnd : phi.GetOpnds()) {
      JudgeTheConsistencyOfDefPointsOfBoundaryCheck(bb, *opnd, visitedPhi, stmts);
    }
  }
}

bool ValueRangePropagation::DealWithBoundaryCheck(BB &bb, MeStmt &meStmt) {
  CHECK_FATAL(meStmt.NumMeStmtOpnds() == kNumOperands, "must have two opnds");
  auto &naryMeStmt = static_cast<NaryMeStmt&>(meStmt);
  auto *boundOpnd = naryMeStmt.GetOpnd(1);
  if (TheValueOfOpndIsInvaliedInABCO(bb, &meStmt, *boundOpnd)) {
    return true;
  }
  auto *indexOpnd = naryMeStmt.GetOpnd(0);
  safetyCheckBoundary->HandleBoundaryCheck(bb, meStmt, *indexOpnd, *boundOpnd);
  CRNode *indexCR = sa.GetOrCreateCRNode(*indexOpnd);
  CRNode *boundCR = sa.GetOrCreateCRNode(*boundOpnd);
  if (indexCR == nullptr || boundCR == nullptr) {
    return false;
  }
  MeExpr *addrExpr = nullptr;
  if (boundOpnd->GetNumOpnds() == 0) {
    addrExpr = boundOpnd;
  } else {
    CHECK_FATAL(boundOpnd->GetNumOpnds() >= 1, "must have one operand at least");
    addrExpr = boundOpnd->GetOpnd(0);
  }
  sa.SortCROperand(*indexCR, *addrExpr);
  sa.SortCROperand(*boundCR, *addrExpr);
  if (ValueRangePropagation::isDebug) {
    meStmt.Dump(&irMap);
    sa.Dump(*indexCR);
    LogInfo::MapleLogger() << "\n";
    sa.Dump(*boundCR);
    LogInfo::MapleLogger() << "\n";
  }
  if (IfAnalysisedBefore(bb, meStmt)) {
    return true;
  }
  if (kOpcodeInfo.IsAssertLowerBoundary(meStmt.GetOp())) {
    if (DealWithAssertGe(bb, meStmt, *indexCR, *boundCR)) {
      return true;
    }
    Insert2AnalysisedArrayChecks(bb.GetBBId(), *meStmt.GetOpnd(1), *meStmt.GetOpnd(0), meStmt.GetOp());
  } else {
    if (DealWithAssertLtOrLe(bb, meStmt, *indexCR, *boundCR, meStmt.GetOp())) {
      return true;
    }
    Insert2AnalysisedArrayChecks(bb.GetBBId(), *meStmt.GetOpnd(1), *meStmt.GetOpnd(0), meStmt.GetOp());
  }
  return false;
}

void SafetyCheck::Error(const MeStmt &stmt) const {
  auto srcPosition = stmt.GetSrcPosition();
  switch (stmt.GetOp()) {
    case OP_assertnonnull: {
      FATAL(kLncFatal, "%s:%d error: Dereference of null pointer",
            func->GetMIRModule().GetFileNameFromFileNum(srcPosition.FileNum()).c_str(), srcPosition.LineNum());
      break;
    }
    case OP_returnassertnonnull: {
      FATAL(kLncFatal, "%s:%d error: %s return nonnull but got null pointer",
            func->GetMIRModule().GetFileNameFromFileNum(srcPosition.FileNum()).c_str(), srcPosition.LineNum(),
            func->GetName().c_str());
      break;
    }
    case OP_assignassertnonnull: {
      FATAL(kLncFatal, "%s:%d error: null assignment of nonnull pointer",
            func->GetMIRModule().GetFileNameFromFileNum(srcPosition.FileNum()).c_str(), srcPosition.LineNum());
      break;
    }
    case OP_callassertnonnull: {
      auto &callStmt = static_cast<const CallAssertNonnullMeStmt &>(stmt);
      FATAL(kLncFatal, "%s:%d error: null pointer passed to %s that requires nonnull for %s argument",
            func->GetMIRModule().GetFileNameFromFileNum(srcPosition.FileNum()).c_str(), srcPosition.LineNum(),
            callStmt.GetFuncName().c_str(), GetNthStr(callStmt.GetParamIndex()).c_str());
      break;
    }
    case OP_assertlt:
    case OP_assertge: {
      std::ostringstream oss;
      if (stmt.GetOp() == OP_assertlt) {
        oss << "%s:%d error: cant't prove the offset < the upper bounds";
      } else {
        oss << "%s:%d error: cant't prove the offset >= the lower bounds";
      }
      FATAL(kLncFatal, oss.str().c_str(), func->GetMIRModule().GetFileNameFromFileNum(srcPosition.FileNum()).c_str(),
            srcPosition.LineNum());
      break;
    }
    case OP_callassertle: {
      auto &callStmt = static_cast<const CallAssertBoundaryMeStmt&>(stmt);
      FATAL(kLncFatal,
            "%s:%d error: can't prove pointer's bounds match the function %s declaration for the %s argument",
            func->GetMIRModule().GetFileNameFromFileNum(srcPosition.FileNum()).c_str(), srcPosition.LineNum(),
            callStmt.GetFuncName().c_str(), GetNthStr(callStmt.GetParamIndex()).c_str());
      break;
    }
    case OP_returnassertle: {
      FATAL(kLncFatal, "%s:%d error: can't prove return value's bounds match the function declaration for %s",
            func->GetMIRModule().GetFileNameFromFileNum(srcPosition.FileNum()).c_str(), srcPosition.LineNum(),
            func->GetName().c_str());
      break;
    }
    case OP_assignassertle: { // support 2
      FATAL(kLncFatal, "%s:%d error: l-value boundary should not be larger than r-value boundary",
            func->GetMIRModule().GetFileNameFromFileNum(srcPosition.FileNum()).c_str(), srcPosition.LineNum());
      break;
    }
    default:
      CHECK_FATAL(false, "can not be here");
  }
}

bool ValueRangePropagation::DealWithAssertNonnull(BB &bb, MeStmt &meStmt) {
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
      auto *valueRange = FindValueRange(*pred, *opnd);
      if (valueRange != nullptr && valueRange->IsNotEqualZero()) {
        return true;
      }
    }
    Insert2Caches(bb.GetBBId(), opnd->GetExprID(),
                  std::make_unique<ValueRange>(Bound(nullptr, 0, opnd->GetPrimType()), kNotEqual));
    return false;
  }
  safetyCheckNonnull->HandleAssertNonnull(meStmt, *valueRange);
  auto newValueRange = ZeroIsInRange(*valueRange);
  if (newValueRange != nullptr) {
    Insert2Caches(bb.GetBBId(), opnd->GetExprID(), std::move(newValueRange));
  }
  return valueRange->IsNotEqualZero() ? true : false;
}

// When the valuerange of expr is constant, replace it with constMeExpr.
void ValueRangePropagation::ReplaceOpndWithConstMeExpr(BB &bb, MeStmt &stmt) {
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
  for (int32 i = 0; i < meExpr.GetNumOpnds(); ++i) {
    CollectMeExpr(bb, stmt, *(meExpr.GetOpnd(i)), expr2ConstantValue);
  }
}

void ValueRangePropagation::DealWithOperand(const BB &bb, MeStmt &stmt, MeExpr &meExpr) {
  switch (meExpr.GetMeOp()) {
    case kMeOpConst: {
      if (static_cast<ConstMeExpr&>(meExpr).GetConstVal()->GetKind() == kConstInt) {
        Insert2Caches(bb.GetBBId(), meExpr.GetExprID(), std::make_unique<ValueRange>(
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
        Insert2Caches(bb.GetBBId(), base->GetExprID(), CreateValueRangeOfNotEqualZero(base->GetPrimType()));
      } else {
        auto newValueRange = ZeroIsInRange(*valueRange);
        if (newValueRange != nullptr) {
          Insert2Caches(bb.GetBBId(), base->GetExprID(), std::move(newValueRange));
        }
      }
      // prop value range of field
      auto filedID = static_cast<IvarMeExpr&>(meExpr).GetFieldID();
      auto *ptrType = static_cast<MIRPtrType*>(GlobalTables::GetTypeTable().GetTypeFromTyIdx(ivarMeExpr.GetTyIdx()));
      if (ptrType->GetPointedType()->IsStructType() &&
          static_cast<MIRStructType*>(ptrType->GetPointedType())->GetFieldAttrs(filedID).GetAttr(FLDATTR_nonnull)) {
        Insert2Caches(bb.GetBBId(), meExpr.GetExprID(), CreateValueRangeOfNotEqualZero(meExpr.GetPrimType()));
      }
      break;
    }
    case kMeOpVar: {
      auto &varMeExpr = static_cast<VarMeExpr&>(meExpr);
      // get nonnull attr from symbol or fieldPair
      if (varMeExpr.GetOst()->GetMIRSymbol()->GetAttr(ATTR_nonnull)) {
        Insert2Caches(bb.GetBBId(), meExpr.GetExprID(), CreateValueRangeOfNotEqualZero(meExpr.GetPrimType()));
      } else if (varMeExpr.GetFieldID() != 0) {
        auto filedID = varMeExpr.GetFieldID();
        auto *ty = varMeExpr.GetOst()->GetMIRSymbol()->GetType();
        if (ty->IsStructType() && static_cast<MIRStructType*>(ty)->GetFieldAttrs(filedID).GetAttr(FLDATTR_nonnull)) {
          Insert2Caches(bb.GetBBId(), meExpr.GetExprID(), CreateValueRangeOfNotEqualZero(meExpr.GetPrimType()));
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
              Insert2Caches(bb.GetBBId(), opMeExpr.GetExprID(), CopyValueRange(*valueRange));
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
            Insert2Caches(bb.GetBBId(), opMeExpr.GetExprID(), CreateValueRangeOfNotEqualZero(meExpr.GetPrimType()));
          }
          break;
        }
        default:
          break;
      }
      break;
    }
    case kMeOpAddrof: {
      auto *valueRangeOfAddrof = FindValueRange(bb, meExpr);
      if (valueRangeOfAddrof == nullptr) {
        Insert2Caches(bb.GetBBId(), meExpr.GetExprID(), CreateValueRangeOfNotEqualZero(meExpr.GetPrimType()));
      }
      break;
    }
    default:
      break;
  }
  for (int32 i = 0; i < meExpr.GetNumOpnds(); ++i) {
    DealWithOperand(bb, stmt, *(meExpr.GetOpnd(i)));
  }
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
    Insert2Caches(bb.GetBBId(), opMeExpr.GetExprID(), std::make_unique<ValueRange>(
        Bound(nullptr, 0, fromType), Bound(GetMaxNumber(fromType), fromType), kLowerAndUpper));
  }
}

// case1: Need insert phi in oldSuccBB to newSuccBB with ssaupdate when the cfg changed like this :
//        pred                         pred
//         |                            |
//     oldSuccBB[condgoto]  ---->       |  oldSuccBB[condgoto]
//      /     \                         |   /    \
// newSuccBB  bb                     newSuccBB   bb
// case2: Need insert phi in pred and oldSuccBB to newSuccBB with ssaupdate when the cfg changed like this :
//    pred0 pred1                     pred0   pred1
//        \ /                           |      |
//        pred                          |     pred
//         |                            |      |
//     oldSuccBB[condgoto]  ---->       |  oldSuccBB[condgoto]
//      /     \                         |   /    \
// newSuccBB  bb                     newSuccBB   bb
// case3: Need insert phi in bb to needUpdateBB for ssaupdate when change the cfg like this :
//           preheader                    preheader
//              |                             |
//              bb[header]---             newCopyBB   bb[header]----
//            /   \          |                |       /   \         |
//   needUpdateBB  bb1       |  --->      needUpdateBB    bb1       |
//            \   /          |                        \   /         |
//             bb2 ----------                          bb2----------
void ValueRangePropagation::InsertOstOfPhi2CandsForSSAUpdate(const BB &oldSuccBB, const BB &newSuccBB) {
  for (auto &it : oldSuccBB.GetMePhiList()) {
    InsertCandsForSSAUpdate(it.first, newSuccBB);
  }
}

void ValueRangePropagation::InsertCandsForSSAUpdate(OStIdx ostIdx, const BB &bb) {
  MeSSAUpdate::InsertOstToSSACands(ostIdx, bb, &cands);
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

void ValueRangePropagation::InsertCandsForSSAUpdate(MeStmt &meStmt, BB &bb) {
  if (kOpcodeInfo.AssignActualVar(meStmt.GetOp()) && meStmt.GetLHS() != nullptr) {
    InsertCandsForSSAUpdate(meStmt.GetLHS()->GetOstIdx(), bb);
  }
  if (meStmt.GetChiList() != nullptr) {
    for (auto &chi : *meStmt.GetChiList()) {
      auto *lhs = chi.second->GetLHS();
      const OStIdx &ostIdx = lhs->GetOstIdx();
      InsertCandsForSSAUpdate(ostIdx, bb);
    }
  }
  if (meStmt.GetMustDefList() != nullptr) {
    for (auto &mustDefNode : *meStmt.GetMustDefList()) {
      const ScalarMeExpr *lhs = static_cast<const ScalarMeExpr*>(mustDefNode.GetLHS());
      if (lhs->GetMeOp() != kMeOpReg && lhs->GetMeOp() != kMeOpVar) {
        CHECK_FATAL(false, "unexpected opcode");
      }
      InsertCandsForSSAUpdate(lhs->GetOstIdx(), bb);
    }
  }
}

void ValueRangePropagation::InsertCandsForSSAUpdate(BB &bb, bool insertDefBBOfPhiOpnds2Cands) {
  for (auto &it : bb.GetMePhiList()) {
    InsertCandsForSSAUpdate(it.first, bb);
    if (insertDefBBOfPhiOpnds2Cands) {
      for (auto *opnd : it.second->GetOpnds()) {
        MeStmt *stmt = nullptr;
        auto *defBB = opnd->GetDefByBBMeStmt(dom, stmt);
        InsertCandsForSSAUpdate(it.first, *defBB);
      }
    }
  }
  for (auto &meStmt : bb.GetMeStmts()) {
    InsertCandsForSSAUpdate(meStmt, bb);
  }
}

void ValueRangePropagation::DeleteUnreachableBBs() {
  if (unreachableBBs.empty()) {
    return;
  }
  isCFGChange = true;
  for (BB *bb : unreachableBBs) {
    InsertCandsForSSAUpdate(*bb);
    auto succs = bb->GetSucc();
    bb->RemoveAllPred();
    bb->RemoveAllSucc();
    for (auto &succ : succs) {
      DeleteThePhiNodeWhichOnlyHasOneOpnd(*succ);
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

int64 GetMinInt64() {
  return std::numeric_limits<int64_t>::min();
}

int64 GetMaxInt64() {
  return std::numeric_limits<int64_t>::max();
}

bool IsNeededPrimType(PrimType prim) {
  return (prim == PTY_i8 || prim == PTY_i16 || prim == PTY_i32 || prim == PTY_i64 || prim == PTY_u8 ||
          prim == PTY_u16 || prim == PTY_u32 || prim == PTY_a32 || prim == PTY_ref || prim == PTY_ptr ||
          prim == PTY_u64 || prim == PTY_a64 || prim == PTY_u1);
}

int64 GetMinNumber(PrimType primType) {
  switch (primType) {
    case PTY_i8:
    case PTY_v8i8:
    case PTY_v16i8:
      return std::numeric_limits<int8_t>::min();
      break;
    case PTY_i16:
    case PTY_v4i16:
    case PTY_v8i16:
      return std::numeric_limits<int16_t>::min();
      break;
    case PTY_i32:
    case PTY_v2i32:
    case PTY_v4i32:
      return std::numeric_limits<int32_t>::min();
      break;
    case PTY_i64:
    case PTY_v2i64:
      return std::numeric_limits<int64_t>::min();
      break;
    case PTY_u8:
    case PTY_v8u8:
    case PTY_v16u8:
      return std::numeric_limits<uint8_t>::min();
      break;
    case PTY_u16:
    case PTY_v4u16:
    case PTY_v8u16:
      return std::numeric_limits<uint16_t>::min();
      break;
    case PTY_u32:
    case PTY_a32:
    case PTY_v4u32:
    case PTY_v2u32:
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
    case PTY_v2u64:
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
    case PTY_v8i8:
    case PTY_v16i8:
      return std::numeric_limits<int8_t>::max();
      break;
    case PTY_i16:
    case PTY_v4i16:
    case PTY_v8i16:
      return std::numeric_limits<int16_t>::max();
      break;
    case PTY_i32:
    case PTY_v2i32:
    case PTY_v4i32:
      return std::numeric_limits<int32_t>::max();
      break;
    case PTY_i64:
    case PTY_v2i64:
      return std::numeric_limits<int64_t>::max();
      break;
    case PTY_u8:
    case PTY_v8u8:
    case PTY_v16u8:
      return std::numeric_limits<uint8_t>::max();
      break;
    case PTY_u16:
    case PTY_v4u16:
    case PTY_v8u16:
      return std::numeric_limits<uint16_t>::max();
      break;
    case PTY_u32:
    case PTY_a32:
    case PTY_v4u32:
    case PTY_v2u32:
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
    case PTY_v2u64:
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

// Determine if the result is overflow or underflow when lhs add rhs.
bool ValueRangePropagation::OverflowOrUnderflow(PrimType primType, int64 lhs, int64 rhs) {
  if (lhs == 0) {
    if (rhs >= GetMinNumber(primType) && rhs <= GetMaxNumber(primType)) {
      return false;
    }
  }
  if (rhs == 0) {
    if (lhs >= GetMinNumber(primType) && lhs <= GetMaxNumber(primType)) {
      return false;
    }
  }

  if (lhs > 0 && rhs > 0 && rhs <= (GetMaxNumber(primType) - lhs)) {
    return false;
  }
  if (lhs < 0 && rhs < 0 && rhs >= (GetMinNumber(primType) - lhs)) {
    return false;
  }
  if (((lhs > 0 && rhs < 0) || (lhs < 0 && rhs > 0)) && (lhs + rhs >= GetMinNumber(primType))) {
    return false;
  }
  return true;
}

// If the result of operator add or sub is overflow or underflow, return false.
bool ValueRangePropagation::AddOrSubWithConstant(
    PrimType primType, Opcode op, int64 lhsConstant, int64 rhsConstant, int64 &res) {
  bool overflowOrUnderflow = false;
  if (op == OP_add) {
    overflowOrUnderflow = OverflowOrUnderflow(primType, lhsConstant, rhsConstant);
  } else {
    CHECK_FATAL(op == OP_sub, "must be sub");
    overflowOrUnderflow = OverflowOrUnderflow(primType, lhsConstant, -rhsConstant);
  }
  if (!overflowOrUnderflow) {
    res = (op == OP_add) ? (lhsConstant + rhsConstant) : (lhsConstant - rhsConstant);
    return true;
  }
  return false;
}

// Create new bound when old bound add or sub with a constant.
bool ValueRangePropagation::CreateNewBoundWhenAddOrSub(Opcode op, Bound bound, int64 rhsConstant, Bound &res) {
  int64 constant = 0;
  if (AddOrSubWithConstant(bound.GetPrimType(), op, bound.GetConstant(), rhsConstant, constant)) {
    res = Bound(bound.GetVar(), GetRealValue(constant, bound.GetPrimType()), bound.GetPrimType());
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
      Insert2Caches(static_cast<VarMeExpr&>(expr).GetDefStmt()->GetBB()->GetBBId(), expr.GetExprID(),
                    std::make_unique<ValueRange>(Bound(GetRealValue(value, expr.GetPrimType()),
                                                       expr.GetPrimType()), kEqual));
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
  if (!valueRangeLeft.IsConstantLowerAndUpper() || valueRangeRight.IsConstantLowerAndUpper()) {
    return nullptr;
  }
  int64 rhsLowerConst = valueRangeRight.GetLower().GetConstant();
  int64 rhsUpperConst = valueRangeRight.GetUpper().GetConstant();
  if (valueRangeLeft.GetRangeType() == kLowerAndUpper) {
    int64 res = 0;
    if (!AddOrSubWithConstant(valueRangeLeft.GetLower().GetPrimType(), op,
                              valueRangeLeft.GetLower().GetConstant(), rhsLowerConst, res)) {
      return nullptr;
    }
    Bound lower = Bound(valueRangeLeft.GetLower().GetVar(), GetRealValue(
        res, valueRangeLeft.GetLower().GetPrimType()), valueRangeLeft.GetLower().GetPrimType());
    res = 0;
    if (!AddOrSubWithConstant(valueRangeLeft.GetLower().GetPrimType(), op,
                              valueRangeLeft.GetUpper().GetConstant(), rhsUpperConst, res)) {
      return nullptr;
    }
    Bound upper = Bound(valueRangeLeft.GetUpper().GetVar(), GetRealValue(
        res, valueRangeLeft.GetUpper().GetPrimType()), valueRangeLeft.GetUpper().GetPrimType());
    return std::make_unique<ValueRange>(lower, upper, kLowerAndUpper);
  } else if (valueRangeLeft.GetRangeType() == kEqual) {
    int64 res = 0;
    if (!AddOrSubWithConstant(valueRangeLeft.GetBound().GetPrimType(), op,
                              valueRangeLeft.GetBound().GetConstant(), rhsLowerConst, res)) {
      return nullptr;
    }
    Bound bound = Bound(valueRangeLeft.GetBound().GetVar(), GetRealValue(
        res, valueRangeLeft.GetBound().GetPrimType()), valueRangeLeft.GetBound().GetPrimType());
    return std::make_unique<ValueRange>(bound, valueRangeLeft.GetRangeType());
  }
  return nullptr;
}

// Create new valueRange when old valueRange add or sub with a constant.
std::unique_ptr<ValueRange> ValueRangePropagation::AddOrSubWithValueRange(
    Opcode op, ValueRange &valueRange, int64 rhsConstant) {
  if (valueRange.GetRangeType() == kLowerAndUpper) {
    int64 res = 0;
    if (!AddOrSubWithConstant(valueRange.GetLower().GetPrimType(), op,
                              valueRange.GetLower().GetConstant(), rhsConstant, res)) {
      return nullptr;
    }
    Bound lower = Bound(valueRange.GetLower().GetVar(),
                        GetRealValue(res, valueRange.GetLower().GetPrimType()), valueRange.GetLower().GetPrimType());
    res = 0;
    if (!AddOrSubWithConstant(valueRange.GetLower().GetPrimType(), op,
                              valueRange.GetUpper().GetConstant(), rhsConstant, res)) {
      return nullptr;
    }
    Bound upper = Bound(valueRange.GetUpper().GetVar(),
                        GetRealValue(res, valueRange.GetUpper().GetPrimType()), valueRange.GetUpper().GetPrimType());
    return std::make_unique<ValueRange>(lower, upper, kLowerAndUpper);
  } else if (valueRange.GetRangeType() == kEqual) {
    int64 res = 0;
    if (!AddOrSubWithConstant(valueRange.GetBound().GetPrimType(), op,
                              valueRange.GetBound().GetConstant(), rhsConstant, res)) {
      return nullptr;
    }
    Bound bound = Bound(valueRange.GetBound().GetVar(),
                        GetRealValue(res, valueRange.GetBound().GetPrimType()), valueRange.GetBound().GetPrimType());
    return std::make_unique<ValueRange>(bound, valueRange.GetRangeType());
  }
  return nullptr;
}

// Create valueRange when deal with OP_add or OP_sub.
std::unique_ptr<ValueRange> ValueRangePropagation::DealWithAddOrSub(
    const BB &bb, const MeExpr &lhsVar, OpMeExpr &opMeExpr) {
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
      newValueRange = std::make_unique<ValueRange>(
          Bound(GetRealValue(res, opMeExpr.GetPrimType()), opMeExpr.GetPrimType()), kEqual);
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
    case PTY_i128:
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
    case PTY_u128:
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
        return std::make_unique<ValueRange>(valueRange.GetBound(), valueRange.GetRangeType());
      } else {
        Bound bound = Bound(valueRange.GetBound().GetVar(),
                            GetRealValue(valueRange.GetBound().GetConstant(), primType), primType);
        return std::make_unique<ValueRange>(bound, valueRange.GetRangeType());
      }
    case kLowerAndUpper:
    case kSpecialUpperForLoop:
    case kSpecialLowerForLoop:
      if (primType == PTY_begin) {
        return std::make_unique<ValueRange>(valueRange.GetLower(), valueRange.GetUpper(), valueRange.GetRangeType());
      } else {
        Bound lower = Bound(valueRange.GetLower().GetVar(),
                            GetRealValue(valueRange.GetLower().GetConstant(), primType), primType);
        Bound upper = Bound(valueRange.GetUpper().GetVar(),
                            GetRealValue(valueRange.GetUpper().GetConstant(), primType), primType);
        return std::make_unique<ValueRange>(lower, upper, valueRange.GetRangeType());
      }
    case kOnlyHasLowerBound:
      if (primType == PTY_begin) {
        return std::make_unique<ValueRange>(valueRange.GetLower(), valueRange.GetStride(), kOnlyHasLowerBound);
      } else {
        Bound lower = Bound(valueRange.GetLower().GetVar(),
                            GetRealValue(valueRange.GetLower().GetConstant(), primType), primType);
        return std::make_unique<ValueRange>(lower, valueRange.GetStride(), kOnlyHasLowerBound);
      }

    case kOnlyHasUpperBound:
      if (primType == PTY_begin) {
        return std::make_unique<ValueRange>(valueRange.GetUpper(), valueRange.GetStride(), kOnlyHasUpperBound);
      } else {
        Bound upper = Bound(valueRange.GetUpper().GetVar(),
                            GetRealValue(valueRange.GetUpper().GetConstant(), primType), primType);
        return std::make_unique<ValueRange>(upper, kOnlyHasUpperBound);
      }
    default:
      CHECK_FATAL(false, "can not be here");
      break;
  }
}

void ValueRangePropagation::DealWithMeOp(const BB &bb, MeStmt &stmt) {
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
    default:
      break;
  }
}

// Create new value range when deal with assign.
void ValueRangePropagation::DealWithAssign(BB &bb, MeStmt &stmt) {
  auto *lhs = stmt.GetLHS();
  auto *rhs = stmt.GetRHS();
  if (lhs == nullptr || rhs == nullptr) {
    return;
  }
  Insert2PairOfExprs(*lhs, *rhs, bb);
  Insert2PairOfExprs(*rhs, *lhs, bb);
  auto *existValueRange = FindValueRange(bb, *rhs);
  if (existValueRange != nullptr) {
    (void)Insert2Caches(bb.GetBBId(), lhs->GetExprID(), CopyValueRange(*existValueRange));
    return;
  }
  if (rhs->GetMeOp() == kMeOpOp) {
    DealWithMeOp(bb, stmt);
  } else if (rhs->GetMeOp() == kMeOpConst && static_cast<ConstMeExpr*>(rhs)->GetConstVal()->GetKind() == kConstInt) {
    if (FindValueRange(bb, *lhs) != nullptr) {
      return;
    }
    std::unique_ptr<ValueRange> valueRange = std::make_unique<ValueRange>(Bound(GetRealValue(
        static_cast<ConstMeExpr*>(rhs)->GetIntValue(), rhs->GetPrimType()), rhs->GetPrimType()), kEqual);
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
bool ValueRangePropagation::CanComputeLoopIndVar(MeExpr &phiLHS, MeExpr &expr, int &constant) {
  auto *curExpr = &expr;
  while (true) {
    if (curExpr->GetMeOp() != kMeOpVar) {
      break;
    }
    VarMeExpr *varMeExpr = static_cast<VarMeExpr*>(curExpr);
    if (varMeExpr->GetDefBy() != kDefByStmt) {
      break;
    }
    MeStmt *defStmt = varMeExpr->GetDefStmt();
    if (defStmt->GetRHS()->GetOp() != OP_add && defStmt->GetRHS()->GetOp() != OP_sub) {
      break;
    }
    OpMeExpr &opMeExpr = static_cast<OpMeExpr&>(*defStmt->GetRHS());
    if (opMeExpr.GetOpnd(1)->GetMeOp() == kMeOpConst &&
        static_cast<ConstMeExpr*>(opMeExpr.GetOpnd(1))->GetConstVal()->GetKind() == kConstInt) {
      ConstMeExpr *rhsExpr = static_cast<ConstMeExpr*>(opMeExpr.GetOpnd(1));
      int64 rhsConst = (defStmt->GetRHS()->GetOp() == OP_sub) ? -rhsExpr->GetIntValue() : rhsExpr->GetIntValue();
      int64 res = 0;
      if (AddOrSubWithConstant(opMeExpr.GetPrimType(), OP_add, constant, rhsConst, res)) {
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
    LoopDesc &loop, BB &exitBB, BB &bb, OpMeExpr &opMeExpr, MeExpr &opnd1, Bound &initBound) {
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
    rightConstant = -1;
  }
  Bound upperBound;
  int64 constantValue = 0;
  if (IsConstant(bb, opnd1, constantValue)) {
    int64 res = 0;
    if (AddOrSubWithConstant(opnd1.GetPrimType(), OP_add, constantValue, rightConstant, res)) {
      upperBound = Bound(GetRealValue(res, opnd1.GetPrimType()), opnd1.GetPrimType());
    } else {
      return nullptr;
    }
    if (initBound.GetVar() == upperBound.GetVar() &&
        GetRealValue(initBound.GetConstant(), opMeExpr.GetOpndType()) >
        GetRealValue(upperBound.GetConstant(), opMeExpr.GetOpndType())) {
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
        upperBound =
            Bound(valueRangeOfUpper->GetUpper().GetVar(), GetRealValue(res, opnd1.GetPrimType()), opnd1.GetPrimType());
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
    return std::make_unique<ValueRange>(initBound, Bound(&opnd1, GetRealValue(
        rightConstant, initBound.GetPrimType()), initBound.GetPrimType()), kSpecialUpperForLoop);
  }
  return nullptr;
}

// Create new value range when the loop induction var is monotonic decrease.
std::unique_ptr<ValueRange> ValueRangePropagation::CreateValueRangeForMonotonicDecreaseVar(
    LoopDesc &loop, BB &exitBB, BB &bb, OpMeExpr &opMeExpr, MeExpr &opnd1, Bound &initBound) {
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
    rightConstant = 1;
  }
  Bound lowerBound;
  int64 constantValue = 0;
  if (IsConstant(bb, opnd1, constantValue)) {
    int64 res = 0;
    if (AddOrSubWithConstant(opnd1.GetPrimType(), OP_add, constantValue, rightConstant, res)) {
      lowerBound = Bound(GetRealValue(res, opnd1.GetPrimType()), opnd1.GetPrimType());
    } else {
      return nullptr;
    }
    if (lowerBound.GetVar() == initBound.GetVar() && GetRealValue(lowerBound.GetConstant(), opMeExpr.GetOpndType()) >
                                                     GetRealValue(initBound.GetConstant(), opMeExpr.GetOpndType())) {
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
          Bound(&opnd1, GetRealValue(rightConstant, opnd1.GetPrimType()), opnd1.GetPrimType()), initBound,
          kSpecialLowerForLoop);
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
    int stride, Bound &initBound) {
  if (stride > 0) {
    return std::make_unique<ValueRange>(initBound, stride, kOnlyHasLowerBound);
  }
  if (stride < 0) {
    return std::make_unique<ValueRange>(initBound, stride, kOnlyHasUpperBound);
  }
  return nullptr;
}

// Create new value range when deal with phinode.
std::unique_ptr<ValueRange> ValueRangePropagation::CreateValueRangeForPhi(LoopDesc &loop,
    BB &bb, ScalarMeExpr &init, ScalarMeExpr &backedge, ScalarMeExpr &lhsOfPhi) {
  Bound initBound;
  ValueRange *valueRangeOfInit = FindValueRange(bb, init);
  bool initIsConstant = false;
  if (valueRangeOfInit != nullptr && valueRangeOfInit->IsConstant()) {
    initIsConstant = true;
    initBound = Bound(GetRealValue(valueRangeOfInit->GetBound().GetConstant(),
        valueRangeOfInit->GetBound().GetPrimType()), valueRangeOfInit->GetBound().GetPrimType());
  } else {
    initBound = Bound(&init, init.GetPrimType());
  }
  int stride = 0;
  if (!CanComputeLoopIndVar(lhsOfPhi, backedge, stride) || stride == 0) {
    if (!initIsConstant) {
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
      if ((opMeExpr->GetOp() == OP_ne && loop.Has(*trueBranch) && initRangeType == kNotEqual) ||
          (opMeExpr->GetOp() == OP_eq && loop.Has(*falseBranch) && initRangeType == kNotEqual)) {
        return std::make_unique<ValueRange>(Bound(constantValue, opMeExpr->GetOpndType()), kNotEqual);
      } else if ((opMeExpr->GetOp() == OP_ne && loop.Has(*falseBranch) && initRangeType == kEqual) ||
                 (opMeExpr->GetOp() == OP_eq && loop.Has(*trueBranch) && initRangeType == kEqual)) {
        return std::make_unique<ValueRange>(Bound(constantValue, opMeExpr->GetOpndType()), kEqual);
      }
    }
    return nullptr;
  }
  if (valueRangeOfInit != nullptr && valueRangeOfInit->GetRangeType() == kNotEqual) {
    initBound = Bound(&init, init.GetPrimType());
  }
  for (auto &it : loop.inloopBB2exitBBs) {
    auto *exitBB = func.GetCfg()->GetBBFromID(it.first);
    if (exitBB->GetKind() != kBBCondGoto) {
      continue;
    }
    BB *trueBranch = nullptr;
    BB *falseBranch = nullptr;
    auto *brMeStmt = static_cast<CondGotoMeStmt*>(exitBB->GetLastMe());
    GetTrueAndFalseBranch(brMeStmt->GetOp(), *exitBB, trueBranch, falseBranch);
    auto *opMeExpr = static_cast<OpMeExpr*>(brMeStmt->GetOpnd());
    MeExpr *opnd0 = opMeExpr->GetOpnd(0);
    MeExpr *opnd1 = opMeExpr->GetOpnd(1);
    if (opnd0 == &backedge) {
      if (stride > 0) {
        if ((loop.Has(*trueBranch) && (opMeExpr->GetOp() == OP_lt || opMeExpr->GetOp() == OP_le)) ||
            (loop.Has(*falseBranch) && (opMeExpr->GetOp() == OP_gt || opMeExpr->GetOp() == OP_ge)) ||
            (stride == 1 && ((loop.Has(*trueBranch) && opMeExpr->GetOp() == OP_ne) ||
                             (loop.Has(*falseBranch) && opMeExpr->GetOp() == OP_eq)))) {
          return CreateValueRangeForMonotonicIncreaseVar(loop, *exitBB, bb, *opMeExpr, *opnd1, initBound);
        } else {
          return std::make_unique<ValueRange>(initBound, stride, kOnlyHasLowerBound);
        }
      }
      if (stride < 0) {
        if ((loop.Has(*trueBranch) && (opMeExpr->GetOp() == OP_gt || opMeExpr->GetOp() == OP_ge)) ||
            (loop.Has(*falseBranch) && (opMeExpr->GetOp() == OP_lt || opMeExpr->GetOp() == OP_le)) ||
            (stride == -1 && ((loop.Has(*trueBranch) && opMeExpr->GetOp() == OP_ne) ||
                              (loop.Has(*falseBranch) && opMeExpr->GetOp() == OP_eq)))) {
          return CreateValueRangeForMonotonicDecreaseVar(loop, *exitBB, bb, *opMeExpr, *opnd1, initBound);
        } else {
          return std::make_unique<ValueRange>(initBound, stride, kOnlyHasUpperBound);
        }
      }
    }
  }
  return MakeMonotonicIncreaseOrDecreaseValueRangeForPhi(stride, initBound);
}

// Calculate the valuerange of def-operand according to the valuerange of each rhs operand.
std::unique_ptr<ValueRange> ValueRangePropagation::MergeValueRangeOfPhiOperands(const BB &bb, MePhiNode &mePhiNode) {
  std::unique_ptr<ValueRange> mergeRange = nullptr;
  auto *valueRangeOfOpnd0 = FindValueRange(*bb.GetPred(0), *mePhiNode.GetOpnd(0));
  if (valueRangeOfOpnd0 == nullptr) {
    return nullptr;
  }
  for (size_t i = 1; i < mePhiNode.GetOpnds().size(); ++i) {
    auto *operand = mePhiNode.GetOpnd(i);
    auto *valueRange = FindValueRange(*bb.GetPred(i), *operand);
    // If one valuerange is nullptr, the result is nullptr.
    if (valueRange == nullptr) {
      return nullptr;
    }
    if (!valueRangeOfOpnd0->IsEqual(valueRange)) {
      return nullptr;
    }
  }
  return CopyValueRange(*valueRangeOfOpnd0);
}

bool ValueRangePropagation::IsBiggerThanMaxInt64(ValueRange &valueRange) const {
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

bool ValueRangePropagation::Insert2Caches(BBId bbID, int32 exprID, std::unique_ptr<ValueRange> valueRange) {
  if (valueRange == nullptr) {
    caches.at(bbID)[exprID] = nullptr;
    return true;
  }
  if (IsBiggerThanMaxInt64(*valueRange)) {
    return false;
  }
  if (valueRange->IsConstant() && valueRange->GetRangeType() == kLowerAndUpper) {
    valueRange->SetRangeType(kEqual);
    valueRange->SetBound(valueRange->GetLower());
  }
  caches.at(bbID)[exprID] = std::move(valueRange);
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
  auto valueOfRHS = GetRealValue(vrOfRHS.GetBound().GetConstant(), primType);
  if (vrOfLHS.GetRangeType() == kEqual) {
    auto valueOfLHS = GetRealValue(vrOfLHS.GetBound().GetConstant(), primType);
    if (valueOfLHS == valueOfRHS) {
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
    auto valueOfLHS = GetRealValue(vrOfLHS.GetBound().GetConstant(), primType);
    if (valueOfLHS == valueOfRHS) {
      // dealwith this case:
      // a = (b == c)
      // valueRange(b): kNotEqual 1
      // valueRange(c): kEqual 1
      // ==>
      // valueRange(a): kEqual 0
      valueRangePtr = (op == OP_eq) ? CreateValueRangeOfEqualZero(resPType) : CreateValueRangeOfNotEqualZero(resPType);
    }
  } else if (vrOfLHS.GetRangeType() == kLowerAndUpper) {
    auto valueOfLHSLower = GetRealValue(vrOfLHS.GetLower().GetConstant(), primType);
    auto valueOfLHSUper = GetRealValue(vrOfLHS.GetUpper().GetConstant(), primType);
    if (valueOfLHSLower <= valueOfLHSUper && (valueOfRHS < valueOfLHSLower || valueOfRHS > valueOfLHSUper)) {
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

ValueRange *ValueRangePropagation::FindValueRangeWithCompareOp(const BB &bb, MeExpr &expr) {
  auto op = expr.GetOp();
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
    Insert2Caches(bb.GetBBId(), expr.GetExprID(), std::move(valueRangePtr));
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

// Calculate the valuerange of the operands according to the mePhiNode.
void ValueRangePropagation::DealWithPhi(BB &bb, MePhiNode &mePhiNode) {
  std::unique_ptr<ValueRange> valueRangeOfPhi = MergeValueRangeOfPhiOperands(bb, mePhiNode);
  if (valueRangeOfPhi != nullptr) {
    (void)Insert2Caches(bb.GetBBId(), mePhiNode.GetLHS()->GetExprID(), std::move(valueRangeOfPhi));
    return;
  }
  if (loops == nullptr) {
    return;
  }
  auto *loop = loops->GetBBLoopParent(bb.GetBBId());
  if (loop == nullptr) {
    return;
  }
  auto phiOperand = mePhiNode.GetOpnds();
  if (phiOperand.size() != kNumOperands) {
    return;
  }
  auto *operand0 = phiOperand.at(0);
  auto *operand1 = phiOperand.at(1);
  std::unique_ptr<ValueRange> valueRange = nullptr;
  MeStmt *defStmt0 = nullptr;
  BB *defBB0 = operand0->GetDefByBBMeStmt(dom, defStmt0);
  MeStmt *defStmt1 = nullptr;
  BB *defBB1 = operand1->GetDefByBBMeStmt(dom, defStmt1);
  if (loop->Has(*defBB0) && !loop->Has(*defBB1)) {
    valueRange = CreateValueRangeForPhi(*loop, bb, *operand1, *operand0, *mePhiNode.GetLHS());
  } else if (loop->Has(*defBB1) && !loop->Has(*defBB0)) {
    valueRange = CreateValueRangeForPhi(*loop, bb, *operand0, *operand1, *mePhiNode.GetLHS());
  }
  if (valueRange != nullptr) {
    (void)Insert2Caches(bb.GetBBId(), mePhiNode.GetLHS()->GetExprID(), std::move(valueRange));
  }
}

// Return the max of leftBound or rightBound.
Bound ValueRangePropagation::Max(Bound leftBound, Bound rightBound) {
  if (leftBound.GetVar() == rightBound.GetVar()) {
    return (leftBound.GetConstant() > rightBound.GetConstant()) ? leftBound : rightBound;
  } else {
    if (leftBound.GetVar() == nullptr && leftBound.GetConstant() == GetMinInt64() &&
        rightBound.GetConstant() < 1 && lengthSet.find(rightBound.GetVar()) != lengthSet.end()) {
      return rightBound;
    }
    if (rightBound.GetVar() == nullptr && rightBound.GetConstant() == GetMinInt64() &&
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
    if (leftBound.GetVar() == nullptr && leftBound.GetConstant() == GetMaxInt64() &&
        rightBound.GetConstant() < 1 && lengthSet.find(rightBound.GetVar()) != lengthSet.end()) {
      return rightBound;
    }
    if (rightBound.GetVar() == nullptr && rightBound.GetConstant() == GetMaxInt64() &&
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
InRangeType ValueRangePropagation::InRange(const BB &bb, ValueRange &rangeTemp, ValueRange &range, bool lowerIsZero) {
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
    ValueRange &leftRange, ValueRange &rightRange, bool merge) {
  if (merge) {
    return std::make_unique<ValueRange>(Min(leftRange.GetLower(), rightRange.GetLower()),
                                        Max(leftRange.GetUpper(), rightRange.GetUpper()), kLowerAndUpper);
  } else {
    return std::make_unique<ValueRange>(Max(leftRange.GetLower(), rightRange.GetLower()),
                                        Min(leftRange.GetUpper(), rightRange.GetUpper()), kLowerAndUpper);
  }
}

// When delete the exit bb of loop and delete the condgoto stmt,
// the loop would not exit, so need change bb to gotobb for the f.GetCfg()->WontExitAnalysis().
void ValueRangePropagation::ChangeLoop2Goto(LoopDesc &loop, BB &bb, BB &succBB, BB &unreachableBB) {
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
bool ValueRangePropagation::BrStmtInRange(const BB &bb, ValueRange &leftRange, ValueRange &rightRange, Opcode op,
                                          PrimType opndType, bool judgeNotInRange) {
  if (!IsNeededPrimType(opndType) ||
      leftRange.GetLower().GetVar() != leftRange.GetUpper().GetVar() ||
      leftRange.GetUpper().GetVar() != rightRange.GetUpper().GetVar() ||
      rightRange.GetUpper().GetVar() != rightRange.GetLower().GetVar() ||
      leftRange.GetRangeType() == kSpecialLowerForLoop ||
      leftRange.GetRangeType() == kSpecialUpperForLoop) {
    return false;
  }
  if (judgeNotInRange) {
    if (leftRange.GetLower().GetVar() != nullptr) {
      return false;
    }
  } else {
    if (leftRange.GetLower().GetVar() != nullptr) {
      auto *valueRange = FindValueRange(bb, *leftRange.GetLower().GetVar());
      if (valueRange == nullptr || !valueRange->IsConstant()) {
        return false;
      }
    }
  }
  int64 leftLower = GetRealValue(leftRange.GetLower().GetConstant(), opndType);
  int64 leftUpper = GetRealValue(leftRange.GetUpper().GetConstant(), opndType);
  int64 rightLower = GetRealValue(rightRange.GetLower().GetConstant(), opndType);
  int64 rightUpper = GetRealValue(rightRange.GetUpper().GetConstant(), opndType);
  // deal the difference between i32 and u32.
  if (leftLower > leftUpper || rightLower > rightUpper) {
    return false;
  }
  if ((op == OP_eq && !judgeNotInRange) || (op == OP_ne && judgeNotInRange)) {
    // If the range of leftOpnd equal to the range of rightOpnd, remove the falseBranch.
    return leftRange.GetRangeType() != kNotEqual && rightRange.GetRangeType() != kNotEqual &&
        leftLower == leftUpper && rightLower == rightUpper && leftLower == rightLower;
  } else if ((op == OP_ne && !judgeNotInRange) || (op == OP_eq && judgeNotInRange)) {
    // If the range type of leftOpnd is kLowerAndUpper and the rightRange is not in range of it,
    // remove the trueBranch, such as :
    // brstmt a == b
    // valueRange a: [1, max]
    // valueRange b: 0
    return (leftRange.GetRangeType() != kNotEqual && rightRange.GetRangeType() != kNotEqual &&
            (leftLower > rightUpper || leftUpper < rightLower)) ||
        (leftRange.GetRangeType() == kNotEqual && rightRange.GetRangeType() == kEqual && leftLower == rightLower) ||
        (leftRange.GetRangeType() == kEqual && rightRange.GetRangeType() == kNotEqual && leftLower == rightLower);
  } else if ((op == OP_lt && !judgeNotInRange) || (op == OP_ge && judgeNotInRange)) {
    return leftRange.GetRangeType() != kNotEqual && rightRange.GetRangeType() != kNotEqual && leftUpper < rightLower;
  } else if ((op == OP_ge && !judgeNotInRange) || (op == OP_lt && judgeNotInRange)) {
    return leftRange.GetRangeType() != kNotEqual && rightRange.GetRangeType() != kNotEqual && leftLower >= rightUpper;
  } else if ((op == OP_le && !judgeNotInRange) || (op == OP_gt && judgeNotInRange)) {
    return leftRange.GetRangeType() != kNotEqual && rightRange.GetRangeType() != kNotEqual && leftUpper <= rightLower;
  } else if ((op == OP_gt && !judgeNotInRange) || (op == OP_le && judgeNotInRange)) {
    return leftRange.GetRangeType() != kNotEqual && rightRange.GetRangeType() != kNotEqual && leftLower > rightUpper;
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
    MeExpr &opnd, ValueRange *leftRange, Bound newRightUpper, Bound newRightLower, BB &trueBranch, BB &falseBranch) {
  CHECK_FATAL(IsEqualPrimType(newRightUpper.GetPrimType(), newRightLower.GetPrimType()), "must be equal");
  if (leftRange == nullptr) {
    std::unique_ptr<ValueRange> newTrueBranchRange =
        std::make_unique<ValueRange>(ValueRange::MinBound(newRightUpper.GetPrimType()), newRightUpper, kLowerAndUpper);
    (void)Insert2Caches(trueBranch.GetBBId(), opnd.GetExprID(), std::move(newTrueBranchRange));
    std::unique_ptr<ValueRange> newFalseBranchRange =
        std::make_unique<ValueRange>(newRightLower, ValueRange::MaxBound(newRightUpper.GetPrimType()), kLowerAndUpper);
    (void)Insert2Caches(falseBranch.GetBBId(), opnd.GetExprID(), std::move(newFalseBranchRange));
  } else {
    std::unique_ptr<ValueRange> newRightRange =
        std::make_unique<ValueRange>(ValueRange::MinBound(newRightUpper.GetPrimType()), newRightUpper, kLowerAndUpper);
    (void)Insert2Caches(trueBranch.GetBBId(), opnd.GetExprID(),
                        CombineTwoValueRange(*leftRange, *newRightRange));
    newRightRange = std::make_unique<ValueRange>(
        newRightLower, ValueRange::MaxBound(newRightUpper.GetPrimType()), kLowerAndUpper);
    (void)Insert2Caches(falseBranch.GetBBId(), opnd.GetExprID(),
        CombineTwoValueRange(*leftRange, *newRightRange));
  }
}

void ValueRangePropagation::CreateValueRangeForGeOrGt(
    MeExpr &opnd, ValueRange *leftRange, Bound newRightUpper, Bound newRightLower, BB &trueBranch, BB &falseBranch) {
  CHECK_FATAL(IsEqualPrimType(newRightUpper.GetPrimType(), newRightLower.GetPrimType()), "must be equal");
  if (leftRange == nullptr) {
    std::unique_ptr<ValueRange> newTrueBranchRange =
        std::make_unique<ValueRange>(newRightLower, ValueRange::MaxBound(newRightUpper.GetPrimType()), kLowerAndUpper);
    (void)Insert2Caches(trueBranch.GetBBId(), opnd.GetExprID(), std::move(newTrueBranchRange));
    std::unique_ptr<ValueRange> newFalseBranchRange =
        std::make_unique<ValueRange>(ValueRange::MinBound(newRightUpper.GetPrimType()), newRightUpper, kLowerAndUpper);
    (void)Insert2Caches(falseBranch.GetBBId(), opnd.GetExprID(), std::move(newFalseBranchRange));
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
    ValueRange &leftRange, ValueRange &rightRange, bool isLower) {
  bool lowerOrUpperIsEqual = isLower ? leftRange.GetLower().GetConstant() == rightRange.GetBound().GetConstant() :
      leftRange.GetUpper().GetConstant() == rightRange.GetBound().GetConstant();
  return leftRange.GetRangeType() == kLowerAndUpper && leftRange.IsConstantLowerAndUpper() &&
         leftRange.GetLower().GetConstant() < leftRange.GetUpper().GetConstant() && lowerOrUpperIsEqual;
}

void ValueRangePropagation::CreateValueRangeForNeOrEq(
    MeExpr &opnd, ValueRange *leftRange, ValueRange &rightRange, BB &trueBranch, BB &falseBranch) {
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
void ValueRangePropagation::RemoveUnreachableBB(BB &condGotoBB, BB &trueBranch) {
  CHECK_FATAL(condGotoBB.GetSucc().size() == kNumOperands, "must have 2 succ");
  auto *succ0 = condGotoBB.GetSucc(0);
  auto *succ1 = condGotoBB.GetSucc(1);
  if (succ0 == &trueBranch) {
    if (OnlyHaveOneCondGotoPredBB(*succ1, condGotoBB)) {
      AnalysisUnreachableBBOrEdge(condGotoBB, *succ1, trueBranch);
    } else {
      condGotoBB.SetKind(kBBFallthru);
      condGotoBB.RemoveSucc(*succ1);
      DeleteThePhiNodeWhichOnlyHasOneOpnd(*succ1);
      condGotoBB.RemoveMeStmt(condGotoBB.GetLastMe());
    }
  } else {
    if (OnlyHaveOneCondGotoPredBB(*succ0, condGotoBB)) {
      AnalysisUnreachableBBOrEdge(condGotoBB, *succ0, trueBranch);
    } else {
      condGotoBB.SetKind(kBBFallthru);
      condGotoBB.RemoveSucc(*succ0);
      DeleteThePhiNodeWhichOnlyHasOneOpnd(*succ0);
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
}

size_t ValueRangePropagation::GetRealPredSize(const BB &bb) const {
  size_t unreachablePredSize = 0;
  for (auto &pred : bb.GetPred()) {
    if (unreachableBBs.find(pred) != unreachableBBs.end()) {
      unreachablePredSize++;
    }
  }
  auto res = bb.GetPred().size() - unreachablePredSize;
  CHECK_FATAL(res >= 0, "must be greater than zero");
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

void ValueRangePropagation::ComputeCodeSize(MeExpr &meExpr, uint32 &cost) {
  for (int32 i = 0; i < meExpr.GetNumOpnds(); ++i) {
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

bool ValueRangePropagation::ChangeTheSuccOfPred2TrueBranch(BB &pred, BB &bb, BB &trueBranch) {
  auto *exitCopyFallthru = GetNewCopyFallthruBB(trueBranch, bb);
  if (exitCopyFallthru != nullptr) {
    size_t index = FindBBInSuccs(pred, bb);
    pred.RemoveSucc(bb);
    InsertCandsForSSAUpdate(pred);
    InsertOstOfPhi2CandsForSSAUpdate(bb, *exitCopyFallthru);
    DeleteThePhiNodeWhichOnlyHasOneOpnd(bb);
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
  while (currBB->GetKind() != kBBCondGoto) {
    if (currBB->GetKind() != kBBFallthru && currBB->GetKind() != kBBGoto) {
      CHECK_FATAL(false, "must be fallthru or goto bb");
    }
    InsertOstOfPhi2CandsForSSAUpdate(*currBB, *mergeAllFallthruBBs);
    currBB = currBB->GetSucc(0);
    CopyMeStmts(*currBB, *mergeAllFallthruBBs);
  }
  CHECK_FATAL(currBB->GetKind() == kBBCondGoto, "must be condgoto bb");
  auto *gotoMeStmt = irMap.New<GotoMeStmt>(func.GetOrCreateBBLabel(trueBranch));
  mergeAllFallthruBBs->AddMeStmtLast(gotoMeStmt);
  size_t index = FindBBInSuccs(pred, bb);
  pred.RemoveSucc(bb);
  pred.AddSucc(*mergeAllFallthruBBs, index);
  mergeAllFallthruBBs->AddSucc(trueBranch);
  InsertCandsForSSAUpdate(pred);
  // When the currBB is not equal to bb, the cfg changed like case 2 in func InsertOstOfPhi2CandsForSSAUpdate.
  if (currBB != &bb) {
    InsertOstOfPhi2CandsForSSAUpdate(*currBB, trueBranch);
  }
  InsertOstOfPhi2CandsForSSAUpdate(bb, trueBranch);
  DeleteThePhiNodeWhichOnlyHasOneOpnd(bb);
  InsertOstOfPhi2CandsForSSAUpdate(*currBB, *mergeAllFallthruBBs);
  CreateLabelForTargetBB(pred, *mergeAllFallthruBBs);
  Insert2TrueOrFalseBranch2NewCopyFallthru(trueBranch, bb, *mergeAllFallthruBBs);
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
    BB &pred, BB &bb, BB &trueBranch) {
  /* case1: the number of branches reaching to condBB > 1 */
  auto *tmpPred = &pred;
  do {
    tmpPred = tmpPred->GetSucc(0);
    if (GetRealPredSize(*tmpPred) > 1) {
      return ChangeTheSuccOfPred2TrueBranch(pred, bb, trueBranch);
    }
  } while (tmpPred->GetKind() != kBBCondGoto);
  // step2
  CHECK_FATAL(GetRealPredSize(bb) == 1, "must have one succ");
  auto *currBB = &bb;
  while (currBB->GetKind() != kBBCondGoto) {
    currBB = currBB->GetSucc(0);
  }
  /* case2: the number of branches reaching to condBB == 1 */
  RemoveUnreachableBB(*currBB, trueBranch);
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
bool ValueRangePropagation::RemoveTheEdgeOfPredBB(BB &pred, BB &bb, BB &trueBranch) {
  CHECK_FATAL(bb.GetKind() == kBBCondGoto, "must be condgoto bb");
  if (GetRealPredSize(bb) >= kNumOperands) {
    if (OnlyHaveCondGotoStmt(bb)) {
      size_t index = FindBBInSuccs(pred, bb);
      for (auto *currPred : bb.GetPred()) {
        InsertOstOfPhi2CandsForSSAUpdate(bb, *currPred);
      }
      pred.RemoveSucc(bb);
      InsertCandsForSSAUpdate(pred);
      InsertOstOfPhi2CandsForSSAUpdate(bb, trueBranch);
      DeleteThePhiNodeWhichOnlyHasOneOpnd(bb);
      pred.AddSucc(trueBranch, index);
      CreateLabelForTargetBB(pred, trueBranch);
    } else {
      auto *exitCopyFallthru = GetNewCopyFallthruBB(trueBranch, bb);
      if (exitCopyFallthru != nullptr) {
        size_t index = FindBBInSuccs(pred, bb);
        pred.RemoveSucc(bb);
        InsertCandsForSSAUpdate(pred);
        InsertOstOfPhi2CandsForSSAUpdate(bb, *exitCopyFallthru);
        DeleteThePhiNodeWhichOnlyHasOneOpnd(bb);
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
      size_t index = FindBBInSuccs(pred, bb);
      pred.RemoveSucc(bb);
      pred.AddSucc(*newBB, index);
      newBB->AddSucc(trueBranch);
      InsertCandsForSSAUpdate(pred);
      InsertOstOfPhi2CandsForSSAUpdate(bb, trueBranch);
      InsertOstOfPhi2CandsForSSAUpdate(bb, *newBB);
      DeleteThePhiNodeWhichOnlyHasOneOpnd(bb);
      func.GetOrCreateBBLabel(trueBranch);
      CreateLabelForTargetBB(pred, *newBB);
      Insert2TrueOrFalseBranch2NewCopyFallthru(trueBranch, bb, *newBB);
    }
  } else {
    CHECK_FATAL(GetRealPredSize(bb) == 1, "must have one pred");
    RemoveUnreachableBB(bb, trueBranch);
  }
  return true;
}

// opnd, tmpPred, tmpBB, reachableBB
bool ValueRangePropagation::RemoveUnreachableEdge(MeExpr &opnd, BB &pred, BB &bb, BB &trueBranch) {
  if (bb.GetKind() == kBBFallthru || bb.GetKind() == kBBGoto) {
    if (!CopyFallthruBBAndRemoveUnreachableEdge(pred, bb, trueBranch)) {
      return false;
    }
  } else {
    if (!RemoveTheEdgeOfPredBB(pred, bb, trueBranch)) {
      return false;
    }
  }
  if (ValueRangePropagation::isDebug) {
    LogInfo::MapleLogger() << "===========delete edge " << pred.GetBBId() << " " << bb.GetBBId() << " " <<
        trueBranch.GetBBId() << "===========\n";
  }
  InsertCandsForSSAUpdate(pred, true);
  InsertOstOfPhi2CandsForSSAUpdate(bb, pred);
  if (trueBranch.GetPred().size() > 1) {
    (void)func.GetOrCreateBBLabel(trueBranch);
  }
  InsertCandsForSSAUpdate(bb, true);
  needUpdateSSA = true;
  isCFGChange = true;
  return true;
}

bool ValueRangePropagation::ConditionEdgeCanBeDeleted(MeExpr &opnd, BB &pred, BB &bb, ValueRange *leftRange,
    ValueRange &rightRange, BB &falseBranch, BB &trueBranch, PrimType opndType, Opcode op) {
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
    return RemoveUnreachableEdge(opnd, *tmpPred, *tmpBB, trueBranch);
  } else if (BrStmtInRange(bb, *leftRange, rightRange, antiOp, opndType)) {
    return RemoveUnreachableEdge(opnd, *tmpPred, *tmpBB, falseBranch);
  }
  return false;
}

bool ValueRangePropagation::MustBeFallthruOrGoto(const BB &defBB, const BB &bb) {
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
    MeExpr &opnd0, ValueRange &rightRange, BB &falseBranch, BB &trueBranch) {
  std::unique_ptr<ValueRange> trueBranchValueRange;
  std::unique_ptr<ValueRange> falseBranchValueRange;
  trueBranchValueRange = CopyValueRange(rightRange);
  falseBranchValueRange = AntiValueRange(rightRange);
  Insert2Caches(trueBranch.GetBBId(), opnd0.GetExprID(), std::move(trueBranchValueRange));
  Insert2Caches(falseBranch.GetBBId(), opnd0.GetExprID(), std::move(falseBranchValueRange));
}

// a: phi(b, c); d: a; e: d
// currOpnd: e
// predOpnd: a, rhs of currOpnd in this bb
// phiOpnds: (b, c), phi rhs of opnd in this bb
// predOpnd or phiOpnds is uesd to find the valuerange in the pred of bb
void ValueRangePropagation::ReplaceOpndByDef(BB &bb, MeExpr &currOpnd, MeExpr *&predOpnd,
    MapleVector<ScalarMeExpr*> &phiOpnds, bool &thePhiIsInBB) {
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
    phiOpnds = static_cast<ScalarMeExpr&>(*predOpnd).GetDefPhi().GetOpnds();
    thePhiIsInBB = true;
  }
}

bool ValueRangePropagation::AnalysisValueRangeInPredsOfCondGotoBB(
    BB &bb, MeExpr &opnd0, MeExpr &currOpnd, ValueRange &rightRange,
    BB &falseBranch, BB &trueBranch, PrimType opndType, Opcode op, BB *condGoto) {
  BB *curBB = (condGoto == nullptr) ? &bb : condGoto;
  bool opt = false;
  /* Records the currOpnd of pred */
  MeExpr *predOpnd = nullptr;
  bool thePhiIsInBB = false;
  MapleVector<ScalarMeExpr*> phiOpnds(mpAllocator.Adapter());
  /* find the rhs of currOpnd, which is used as the currOpnd of pred */
  ReplaceOpndByDef(bb, currOpnd, predOpnd, phiOpnds, thePhiIsInBB);
  size_t indexOfOpnd = 0;
  for (size_t i = 0; i < bb.GetPred().size();) {
    predOpnd = thePhiIsInBB ? phiOpnds.at(indexOfOpnd) : predOpnd;
    indexOfOpnd++;
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
      if(frontier.find(pred->GetBBId()) != frontier.end()) {
        ++i;
        continue;
      }
    }
    auto *valueRangeInPred = FindValueRange(*pred, *predOpnd);
    if (ConditionEdgeCanBeDeleted(opnd0, *pred, bb, valueRangeInPred, rightRange,
                                  falseBranch, trueBranch, opndType, op)) {
      opt = true;
    } else {
      /* avoid infinite loop, pred->GetKind() maybe kBBUnknown */
      if ((pred->GetKind() == kBBFallthru || pred->GetKind() == kBBGoto) &&
           pred->GetBBId() != falseBranch.GetBBId() && pred->GetBBId() != trueBranch.GetBBId()) {
        AnalysisValueRangeInPredsOfCondGotoBB(
            *pred, opnd0, *predOpnd, rightRange, falseBranch, trueBranch, opndType, op);
      }
    }
    if (bb.GetPred().size() == predSize) {
      ++i;
    } else if (bb.GetPred().size() != predSize - 1) {
      CHECK_FATAL(false, "immpossible");
    }
  }
  if (opt) {
    if (curBB->GetKind() == kBBCondGoto) {
      // vrp modified condgoto, branch probability is no longer trustworthy, so we invalidate it
      static_cast<CondGotoMeStmt*>(curBB->GetLastMe())->InvalidateBranchProb();
    }
  }
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
    BB &falseBranch, BB &trueBranch, PrimType opndType, Opcode op, BB *condGoto) {
  auto *currOpnd = &opnd0;
  if (opnd0.GetOp() == OP_zext) {
    auto &opMeExpr = static_cast<OpMeExpr&>(opnd0);
    auto *opnd = opMeExpr.GetOpnd(0);
    if (opMeExpr.GetBitsSize() >= GetPrimTypeBitSize(opnd->GetPrimType())) {
      currOpnd = opnd;
      opndType = currOpnd->GetPrimType();
    }
  }
  bool opt = AnalysisValueRangeInPredsOfCondGotoBB(
      bb, *currOpnd, *currOpnd, rightRange, falseBranch, trueBranch, opndType, op, condGoto);
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

void ValueRangePropagation::DealWithCondGoto(
    BB &bb, Opcode op, ValueRange *leftRange, ValueRange &rightRange, const CondGotoMeStmt &brMeStmt) {
  auto newRightUpper = rightRange.GetUpper();
  auto newRightLower = rightRange.GetLower();
  CHECK_FATAL(IsEqualPrimType(newRightUpper.GetPrimType(), newRightLower.GetPrimType()), "must be equal");
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
  if (op == OP_eq) {
    CreateValueRangeForNeOrEq(*opnd0, leftRange, rightRange, *trueBranch, *falseBranch);
  } else if (op == OP_ne) {
    CreateValueRangeForNeOrEq(*opnd0, leftRange, rightRange, *falseBranch, *trueBranch);
  }
  if ((op == OP_lt) || (op == OP_ge)) {
    int64 constant = 0;
    if (!AddOrSubWithConstant(newRightUpper.GetPrimType(), OP_add, newRightUpper.GetConstant(), -1, constant)) {
      return;
    }
    newRightUpper = Bound(newRightUpper.GetVar(),
        GetRealValue(constant, newRightUpper.GetPrimType()), newRightUpper.GetPrimType());
  } else if ((op == OP_le) || (op == OP_gt)) {
    int64 constant = 0;
    if (!AddOrSubWithConstant(newRightUpper.GetPrimType(), OP_add, newRightLower.GetConstant(), 1, constant)) {
      return;
    }
    newRightLower = Bound(newRightLower.GetVar(), GetRealValue(constant, newRightUpper.GetPrimType()), newRightUpper.GetPrimType());
  }
  if (op == OP_lt || op == OP_le) {
    if (leftRange != nullptr && leftRange->GetRangeType() == kNotEqual) {
      CreateValueRangeForLeOrLt(*opnd0, nullptr, newRightUpper, newRightLower, *trueBranch, *falseBranch);
    } else {
      CreateValueRangeForLeOrLt(*opnd0, leftRange, newRightUpper, newRightLower, *trueBranch, *falseBranch);
    }
  } else if (op == OP_gt || op == OP_ge) {
    if (leftRange != nullptr && leftRange->GetRangeType() == kNotEqual) {
      CreateValueRangeForGeOrGt(*opnd0, nullptr, newRightUpper, newRightLower, *trueBranch, *falseBranch);
    } else {
      CreateValueRangeForGeOrGt(*opnd0, leftRange, newRightUpper, newRightLower, *trueBranch, *falseBranch);
    }
  }
}

bool ValueRangePropagation::GetValueRangeOfCondGotoOpnd(BB &bb, OpMeExpr &opMeExpr, MeExpr &opnd,
    ValueRange *&valueRange, std::unique_ptr<ValueRange> &rightRangePtr) {
  valueRange = FindValueRange(bb, opnd);
  if (valueRange == nullptr) {
    if (opnd.GetMeOp() == kMeOpConst && static_cast<ConstMeExpr&>(opnd).GetConstVal()->GetKind() == kConstInt) {
      rightRangePtr = std::make_unique<ValueRange>(Bound(GetRealValue(
          static_cast<ConstMeExpr&>(opnd).GetIntValue(), opnd.GetPrimType()), opnd.GetPrimType()), kEqual);
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
      int64 lhsConstant = valueRangeOfLHS->GetBound().GetConstant();
      int64 rhsConstant = GetRealValue(static_cast<ConstMeExpr*>(rhs)->GetIntValue(), rhs->GetPrimType());
      // If the type of operands is OpMeExpr, need compute the valueRange of operand:
      // Example: if ((a != c) < b),
      // a: ValueRange(5, kEqual)
      // c: ValueRange(6, kEqual)
      // ===>
      // (a != c) : ValueRange(1, kEqual)
      if ((opnd.GetOp() == OP_ne && lhsConstant != rhsConstant && lhsRangeType == kEqual) ||
          (opnd.GetOp() == OP_eq && lhsConstant == rhsConstant && lhsRangeType == kEqual) ||
          (opnd.GetOp() == OP_ne && lhsConstant == rhsConstant && lhsRangeType == kNotEqual)) {
        rightRangePtr = std::make_unique<ValueRange>(Bound(1, PTY_u1), kEqual);
        valueRange = rightRangePtr.get();
        Insert2Caches(bb.GetBBId(), opnd.GetExprID(), std::move(rightRangePtr));
      } else if ((opnd.GetOp() == OP_ne && lhsConstant == rhsConstant && lhsRangeType == kEqual) ||
                 (opnd.GetOp() == OP_eq && lhsConstant != rhsConstant && lhsRangeType == kEqual) ||
                 (opnd.GetOp() == OP_eq && lhsConstant == rhsConstant && lhsRangeType == kNotEqual)) {
        rightRangePtr = std::make_unique<ValueRange>(Bound(nullptr, 0, PTY_u1), kEqual);
        valueRange = rightRangePtr.get();
        Insert2Caches(bb.GetBBId(), opnd.GetExprID(), std::move(rightRangePtr));
      }
    }
  }
  if (valueRange != nullptr && valueRange->GetLower().GetPrimType() != opMeExpr.GetOpndType() &&
      IsNeededPrimType(opMeExpr.GetOpndType())) {
    rightRangePtr = CopyValueRange(*valueRange, opMeExpr.GetOpndType());
    valueRange = rightRangePtr.get();
    if (IsBiggerThanMaxInt64(*valueRange)) {
      valueRange = nullptr;
      return false;
    }
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
    BB &bb, MeExpr &opnd0, MeExpr &opnd1, Opcode opOfBrStmt, Opcode conditionalOp) {
  PrimType prim = opnd1.GetPrimType();
  if (!IsNeededPrimType(prim)) {
    return;
  }
  BB *trueBranch = nullptr;
  BB *falseBranch = nullptr;
  GetTrueAndFalseBranch(opOfBrStmt, bb, trueBranch, falseBranch);
  switch (conditionalOp) {
    case OP_ne: {
      Insert2Caches(trueBranch->GetBBId(), opnd0.GetExprID(),
                    std::make_unique<ValueRange>(Bound(&opnd1, prim), kNotEqual));
      Insert2Caches(falseBranch->GetBBId(), opnd0.GetExprID(),
                    std::make_unique<ValueRange>(Bound(&opnd1, prim), kEqual));
      break;
    }
    case OP_eq: {
      Insert2Caches(trueBranch->GetBBId(), opnd0.GetExprID(),
                    std::make_unique<ValueRange>(Bound(&opnd1, prim), kEqual));
      Insert2Caches(falseBranch->GetBBId(), opnd0.GetExprID(),
                    std::make_unique<ValueRange>(Bound(&opnd1, prim), kNotEqual));
      break;
    }
    case OP_le: {
      Insert2Caches(trueBranch->GetBBId(), opnd0.GetExprID(),
                    std::make_unique<ValueRange>(Bound(GetMinNumber(prim), prim), Bound(&opnd1, prim), kLowerAndUpper));
      Insert2Caches(falseBranch->GetBBId(), opnd0.GetExprID(),
                    std::make_unique<ValueRange>(Bound(&opnd1, 1, prim),
                                                 Bound(GetMaxNumber(prim), prim), kLowerAndUpper));
      break;
    }
    case OP_lt: {
      Insert2Caches(trueBranch->GetBBId(), opnd0.GetExprID(),
                    std::make_unique<ValueRange>(Bound(GetMinNumber(prim), prim),
                                                 Bound(&opnd1, -1, prim), kLowerAndUpper));
      Insert2Caches(falseBranch->GetBBId(), opnd0.GetExprID(),
                    std::make_unique<ValueRange>(Bound(&opnd1, prim), Bound(GetMaxNumber(prim), prim), kLowerAndUpper));
      break;
    }
    case OP_ge: {
      Insert2Caches(trueBranch->GetBBId(), opnd0.GetExprID(),
                    std::make_unique<ValueRange>(Bound(&opnd1, prim), Bound(GetMaxNumber(prim), prim), kLowerAndUpper));
      Insert2Caches(falseBranch->GetBBId(), opnd0.GetExprID(),
                    std::make_unique<ValueRange>(Bound(GetMinNumber(prim), prim),
                                                 Bound(&opnd1, -1, prim), kLowerAndUpper));
      break;
    }
    case OP_gt: {
      Insert2Caches(trueBranch->GetBBId(), opnd0.GetExprID(),
                    std::make_unique<ValueRange>(Bound(&opnd1, 1, prim),
                                                 Bound(GetMaxNumber(prim), prim), kLowerAndUpper));
      Insert2Caches(falseBranch->GetBBId(), opnd0.GetExprID(),
                    std::make_unique<ValueRange>(Bound(GetMinNumber(prim), prim),
                                                 Bound(&opnd1, prim), kLowerAndUpper));
      break;
    }
    default:
      break;
  }
}

void ValueRangePropagation::GetValueRangeForUnsignedInt(BB &bb, OpMeExpr &opMeExpr, MeExpr &opnd,
    ValueRange *&valueRange, std::unique_ptr<ValueRange> &rightRangePtr) {
  PrimType prim = opMeExpr.GetOpndType();
  if (prim == PTY_u1 || prim == PTY_u8 || prim == PTY_u16 || prim == PTY_u32 || prim == PTY_a32 ||
      ((prim == PTY_ref || prim == PTY_ptr) && GetPrimTypeSize(prim) == kFourByte)) {
    rightRangePtr = std::make_unique<ValueRange>(
        Bound(nullptr, 0, prim), Bound(GetMaxNumber(prim), prim), kLowerAndUpper);
    valueRange = rightRangePtr.get();
    Insert2Caches(bb.GetBBId(), opnd.GetExprID(), std::move(rightRangePtr));
  }
}

// This function deals with the case like this:
// Example: if (a > 0)
// a: valuerange(0, Max)
// ==>
// Example: if (a != 0)
bool ValueRangePropagation::DealWithSpecialCondGoto(
    OpMeExpr &opMeExpr, ValueRange &leftRange, ValueRange &rightRange, CondGotoMeStmt &brMeStmt) {
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
  irMap.ReplaceMeExprStmt(brMeStmt, opMeExpr, *newExpr);
  return true;
}

// Deal with the special brstmt, which only has one opnd.
void ValueRangePropagation::DealWithBrStmtWithOneOpnd(BB &bb, CondGotoMeStmt &stmt, MeExpr &opnd, Opcode op) {
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
    std::unique_ptr<ValueRange> leftRangePtr = std::make_unique<ValueRange>(Bound(GetRealValue(
        static_cast<ConstMeExpr&>(opnd).GetIntValue(), opnd.GetPrimType()), opnd.GetPrimType()), kEqual);
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
  Insert2Caches(trueBranch->GetBBId(), brMeStmt.GetOpnd()->GetExprID(),
                CreateValueRangeOfNotEqualZero(brMeStmt.GetOpnd()->GetPrimType()));
  Insert2Caches(falseBranch->GetBBId(), brMeStmt.GetOpnd()->GetExprID(),
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
  if (!GetValueRangeOfCondGotoOpnd(bb, *opMeExpr, *opnd1, rightRange, rightRangePtr)) {
    return;
  }
  if (rightRange == nullptr) {
    DealWithCondGotoWhenRightRangeIsNotExist(bb, *opnd0, *opnd1, brMeStmt.GetOp(), opMeExpr->GetOp());
    return;
  }
  if (!GetValueRangeOfCondGotoOpnd(bb, *opMeExpr, *opnd0, leftRange, leftRangePtr)) {
    return;
  }
  if (leftRange == nullptr && rightRange->GetRangeType() != kEqual) {
    return;
  }
  if (leftRange == nullptr) {
    GetValueRangeForUnsignedInt(bb, *opMeExpr, *opnd0, leftRange, leftRangePtr);
  }
  if (leftRange != nullptr && leftRange->GetRangeType() == kOnlyHasLowerBound &&
      rightRange->GetRangeType() == kOnlyHasUpperBound) { // deal with special case
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
  for (int i = 0; i < caches.size(); ++i) {
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

        LogInfo::MapleLogger() << "mx" << bIt->first << " lower: " << lower << " upper: " << upper << "\n";
      } else if (bIt->second->GetRangeType() == kOnlyHasLowerBound) {
        std::string lower = (bIt->second->GetBound().GetVar() == nullptr) ?
            std::to_string(bIt->second->GetBound().GetConstant()) :
            "mx" + std::to_string(bIt->second->GetBound().GetVar()->GetExprID()) + " " +
            std::to_string(bIt->second->GetBound().GetConstant());
        LogInfo::MapleLogger() << "mx" << bIt->first << " lower: " << lower << " upper: max" << "\n";
      } else if (bIt->second->GetRangeType() == kEqual || bIt->second->GetRangeType() == kNotEqual) {
        std::string lower = (bIt->second->GetBound().GetVar() == nullptr) ?
            std::to_string(bIt->second->GetBound().GetConstant()) :
            "mx" + std::to_string(bIt->second->GetBound().GetVar()->GetExprID()) + " " +
            std::to_string(bIt->second->GetBound().GetConstant());
        LogInfo::MapleLogger() << "mx" << bIt->first << " lower and upper: " << lower << " ";
        if (bIt->second->GetRangeType() == kEqual) {
          LogInfo::MapleLogger() << "kEqual\n";
        } else {
          LogInfo::MapleLogger() << "kNotEqual\n";
        }
      }
    }
  }
  LogInfo::MapleLogger() << "================Dump value range===================" << "\n";
}

void MEValueRangePropagation::GetAnalysisDependence(maple::AnalysisDep &aDep) const {
  aDep.AddRequired<MEDominance>();
  aDep.AddRequired<MEIRMapBuild>();
  aDep.SetPreservedAll();
}

bool MEValueRangePropagation::PhaseRun(maple::MeFunction &f) {
  f.vrpRuns++;
  auto *dom = GET_ANALYSIS(MEDominance, f);
  CHECK_FATAL(dom != nullptr, "dominance phase has problem");
  auto *irMap = GET_ANALYSIS(MEIRMapBuild, f);
  CHECK_FATAL(irMap != nullptr, "irMap phase has problem");
  GetAnalysisInfoHook()->ForceEraseAnalysisPhase(f.GetUniqueID(), &MELoopAnalysis::id);
  auto *meLoop = FORCE_GET(MELoopAnalysis);
  if (ValueRangePropagation::isDebug) {
    LogInfo::MapleLogger() << f.GetName() << "\n";
    f.Dump(false);
    f.GetCfg()->DumpToFile("valuerange-before");
  }
  auto *valueRangeMemPool = GetPhaseMemPool();
  MapleAllocator valueRangeAlloc = MapleAllocator(valueRangeMemPool);
  std::map<OStIdx, std::unique_ptr<std::set<BBId>>> cands((std::less<OStIdx>()));
  LoopScalarAnalysisResult sa(*irMap, nullptr);
  sa.SetComputeTripCountForLoopUnroll(false);
  ValueRangePropagation valueRangePropagation(f, *irMap, *dom, meLoop, *valueRangeMemPool, cands, sa);

  SafetyCheck safetyCheck; // dummy
  SafetyCheckWithBoundaryError safetyCheckBoundaryError(f, valueRangePropagation);
  SafetyCheckWithNonnullError safetyCheckNonnullError(f);
  if (MeOption::boundaryCheckMode == kNoCheck) {
    valueRangePropagation.SetSafetyBoundaryCheck(safetyCheck);
  } else {
    valueRangePropagation.SetSafetyBoundaryCheck(safetyCheckBoundaryError);
  }
  if (MeOption::npeCheckMode == kNoCheck || f.vrpRuns == 1) {
    valueRangePropagation.SetSafetyNonnullCheck(safetyCheck);
  } else {
    valueRangePropagation.SetSafetyNonnullCheck(safetyCheckNonnullError);
  }

  valueRangePropagation.Execute();
  f.GetCfg()->UnreachCodeAnalysis(true);
  f.GetCfg()->WontExitAnalysis();
  // split critical edges
  MeSplitCEdge(false).SplitCriticalEdgeForMeFunc(f);
  if (valueRangePropagation.IsCFGChange()) {
    if (ValueRangePropagation::isDebug) {
      f.GetCfg()->DumpToFile("valuerange-after");
    }
    GetAnalysisInfoHook()->ForceEraseAnalysisPhase(f.GetUniqueID(), &MEDominance::id);
    auto *dom = FORCE_GET(MEDominance);
    if (valueRangePropagation.NeedUpdateSSA()) {
      MeSSAUpdate ssaUpdate(f, *f.GetMeSSATab(), *dom, cands, *valueRangeMemPool);
      ssaUpdate.Run();
    }
    GetAnalysisInfoHook()->ForceEraseAnalysisPhase(f.GetUniqueID(), &MELoopAnalysis::id);
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
