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

namespace maple {
bool ValueRangePropagation::isDebug = false;
constexpr size_t kNumOperands = 2;
constexpr size_t kFourByte = 4;
constexpr size_t kEightByte = 8;
constexpr size_t kCodeSizeLimit = 2000;
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
    for (auto &stmt : bb->GetMeStmts()) {
      for (size_t i = 0; i < stmt.NumMeStmtOpnds(); ++i) {
        DealWithOperand(*bb, stmt, *stmt.GetOpnd(i));
      }
      switch (stmt.GetOp()) {
        case OP_dassign:
        case OP_maydassign:
        case OP_regassign: {
          if (stmt.GetLHS() != nullptr && stmt.GetLHS()->IsVolatile()) {
            continue;
          }
          DealWithAssign(*bb, stmt);
          break;
        }
        case OP_brfalse:
        case OP_brtrue: {
          DealWithCondGoto(*bb, stmt);
          break;
        }
        CASE_ASSERTNONNULL {
          DealWithAssertNonnull(*bb, stmt);
          break;
        }
        case OP_assertge:
        case OP_assertlt: {
          DealWithBoundaryCheck(*bb, stmt);
          break;
        }
        case OP_callassigned: {
          DealWithCallassigned(*bb, stmt);
          break;
        }
        case OP_switch: {
          DealWithSwitch(stmt);
          break;
        }
        case OP_igoto:
        default:
          break;
      }
    }
  }
  if (ValueRangePropagation::isDebug) {
    DumpCaches();
  }
  DeleteUnreachableBBs();
  DeleteAssertNonNull();
  DeleteBoundaryCheck();
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
  for (auto &pair : switchMeStmt.GetSwitchTable()) {
    Insert2Caches(func.GetCfg()->GetLabelBBAt(pair.second)->GetBBId(), opnd->GetExprID(),
                  std::make_unique<ValueRange>(Bound(nullptr, pair.first, PTY_i64), kEqual));
  }
}

void ValueRangePropagation::DeleteAssertNonNull() {
  if (ValueRangePropagation::isDebug) {
    LogInfo::MapleLogger() << "=========delete assert nonnull=========\n";
  }
  for (auto it : needDeletedAssertNonNull) {
    for (auto &stmt : it.second) {
      if (ValueRangePropagation::isDebug) {
        stmt->Dump(&irMap);
      }
      it.first->RemoveMeStmt(stmt);
    }
  }
  if (ValueRangePropagation::isDebug) {
    LogInfo::MapleLogger() << "=========delete assert nonnull=========\n";
  }
}

void ValueRangePropagation::DeleteBoundaryCheck() {
  if (ValueRangePropagation::isDebug) {
    LogInfo::MapleLogger() << "=========delete boundary check=========\n";
  }
  for (auto it : needDeletedBoundaryCheck) {
    for (auto &stmt : it.second) {
      LogInfo::MapleLogger() << it.first->GetBBId() << "\n";
      if (ValueRangePropagation::isDebug) {
        stmt->Dump(&irMap);
      }
      it.first->RemoveMeStmt(stmt);
    }
  }
  if (ValueRangePropagation::isDebug) {
    LogInfo::MapleLogger() << "=========delete boundary check=========\n";
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
    for (auto it : bb.GetMePhiList()) {
      InsertCandsForSSAUpdate(it.first, bb);
    }
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
      return range.pair.upper.IsEqual(valueRangeRight->GetUpper()) &&
             range.pair.lower.IsEqual(valueRangeRight->GetLower());
    case kOnlyHasLowerBound:
      return range.pair.lower.IsEqual(valueRangeRight->GetLower()) &&
             stride == valueRangeRight->GetStride();
    case kOnlyHasUpperBound:
      return range.pair.upper.IsEqual(valueRangeRight->GetUpper()) &&
             stride == valueRangeRight->GetStride();
    case kNotEqual:
    case kEqual:
      return range.bound.IsEqual(valueRangeRight->GetBound());
    default:
      CHECK_FATAL(false, "can not be here");
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
  auto *valueRangeOfOpnd = FindValueRangeInCaches(bb.GetBBId(), opnd.GetExprID());
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
  if (valueRangeOfIndex != nullptr && valueRangeOfIndex->IsBiggerThanZero()) {
    return true;
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

bool ValueRangePropagation::CompareIndexWithUpper(MeStmt &meStmt, ValueRange &valueRangeOfIndex,
    int64 lengthValue, ValueRange &valueRangeOfLengthPtr, uint32 byteSize) {
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
    return valueRangeOfIndex.GetUpper().GetConstant() < lengthValue;
  }
  // Opt array boundary check when the array length is a var.
  if (valueRangeOfIndex.GetRangeType() == kSpecialUpperForLoop) {
    // When the var of index is not equal to the var of length, need analysis if the var of index is related to the
    // var of length. For example:
    // i = len - i;
    // p[i];
    // assertlt(p + i * 4, p + len * 4)
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
      MeExpr *addrExpr = nullptr;
      if (meStmt.GetOpnd(1)->GetOpnd(0)->GetNumOpnds() == 0) {
        addrExpr = meStmt.GetOpnd(1)->GetOpnd(0);
      } else {
        CHECK_FATAL(meStmt.GetOpnd(1)->GetOpnd(0)->GetNumOpnds() >= 1, "must have one operand at least");
        addrExpr = meStmt.GetOpnd(1)->GetOpnd(0)->GetOpnd(0);
      }
      sa.SortCROperand(*crNode, *addrExpr);
      return crNode->GetCRType() == kCRAddNode &&
             static_cast<CRAddNode*>(crNode)->GetOpndsSize() == kNumOperands &&
             static_cast<CRAddNode*>(crNode)->GetOpnd(0)->GetExpr() != nullptr &&
             static_cast<CRAddNode*>(crNode)->GetOpnd(0)->GetExpr() == valueRangeOfLengthPtr.GetBound().GetVar() &&
             static_cast<CRAddNode*>(crNode)->GetOpnd(1)->GetCRType() == kCRConstNode &&
             static_cast<CRConstNode*>(static_cast<CRAddNode*>(crNode)->GetOpnd(1))->GetConstValue() <
             valueRangeOfLengthPtr.GetBound().GetConstant();
    }
  } else if ((valueRangeOfIndex.GetLower().GetVar() != valueRangeOfIndex.GetUpper().GetVar()) ||
             valueRangeOfIndex.GetUpper().GetVar() != valueRangeOfLengthPtr.GetBound().GetVar() ||
             valueRangeOfIndex.GetLower().GetConstant() > valueRangeOfIndex.GetUpper().GetConstant()) {
    return false;
  }
  if (valueRangeOfIndex.GetUpper().GetConstant() < valueRangeOfLengthPtr.GetBound().GetConstant()) {
    return true;
  }
  return false;
}

// Deal with upper boundary check.
bool ValueRangePropagation::DealWithAssertlt(BB &bb, MeStmt &meStmt, CRNode &indexCR, CRNode &boundCR) {
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
  std::unique_ptr<ValueRange> valueRangeOfIndex = GetTheValueRangeOfIndex(
      bb, meStmt, *crAddNodeOfIndex, byteSize, lengthExpr, valueRangeOfLengthPtr);
  if (valueRangeOfIndex == nullptr) {
    return false;
  }
  return CompareIndexWithUpper(meStmt, *valueRangeOfIndex, lengthValue, *valueRangeOfLengthPtr,
                               byteSize);
}

// If the bb of the boundary check which is analysised befor dominates the bb of the boundary check which is
// analysised now, then eliminate the current boundary check.
bool ValueRangePropagation::IfAnalysisedBefore(BB &bb, MeStmt &stmt) {
  MeExpr &index = *stmt.GetOpnd(0);
  MeExpr &bound = *stmt.GetOpnd(1);
  auto &analysisedArrayChecks = (stmt.GetOp() == OP_assertge) ? analysisedLowerBoundChecks : analysisedUpperBoundChecks;
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
      Insert2NeedDeleteBoundaryCheck(bb, stmt);
      return true;
    }
  }
  return false;
}

void ValueRangePropagation::DealWithBoundaryCheck(BB &bb, MeStmt &meStmt) {
  CHECK_FATAL(meStmt.NumMeStmtOpnds() == kNumOperands, "must have two opnds");
  auto &naryMeStmt = static_cast<NaryMeStmt&>(meStmt);
  auto *indexOpnd = naryMeStmt.GetOpnd(0);
  auto *boundOpnd = naryMeStmt.GetOpnd(1);
  CRNode *indexCR = sa.GetOrCreateCRNode(*indexOpnd);
  CRNode *boundCR = sa.GetOrCreateCRNode(*boundOpnd);
  if (indexCR == nullptr || boundCR == nullptr) {
    return;
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
    return;
  }
  if (meStmt.GetOp() == OP_assertge) {
    if (DealWithAssertGe(bb, meStmt, *indexCR, *boundCR)) {
      Insert2NeedDeleteBoundaryCheck(bb, meStmt);
    }
    Insert2AnalysisedArrayChecks(bb.GetBBId(), *meStmt.GetOpnd(1), *meStmt.GetOpnd(0), OP_assertge);
  } else {
    if (DealWithAssertlt(bb, meStmt, *indexCR, *boundCR)) {
      Insert2NeedDeleteBoundaryCheck(bb, meStmt);
    }
    Insert2AnalysisedArrayChecks(bb.GetBBId(), *meStmt.GetOpnd(1), *meStmt.GetOpnd(0), OP_assertlt);
  }
}

void ValueRangePropagation::DealWithAssertNonnull(BB &bb, MeStmt &meStmt) {
  auto *opnd = static_cast<const UnaryMeStmt&>(meStmt).GetOpnd();
  auto *valueRange = FindValueRangeInCaches(bb.GetBBId(), opnd->GetExprID());

  if (valueRange == nullptr) {
    Insert2Caches(bb.GetBBId(), opnd->GetExprID(),
                  std::make_unique<ValueRange>(Bound(nullptr, 0, opnd->GetPrimType()), kNotEqual));
    return;
  }
  auto newValueRange = ZeroIsInRange(*valueRange);
  if (newValueRange != nullptr) {
    Insert2Caches(bb.GetBBId(), opnd->GetExprID(), std::move(newValueRange));
  }
  if (valueRange->IsNotEqualZero()) {
    auto it = needDeletedAssertNonNull.find(&bb);
    if (it == needDeletedAssertNonNull.end()) {
      needDeletedAssertNonNull[&bb] = std::set<MeStmt*>{ &meStmt };
    } else {
      it->second.insert(&meStmt);
    }
  }
}

void ValueRangePropagation::DealWithOperand(const BB &bb, MeStmt &stmt, MeExpr &meExpr) {
  switch (meExpr.GetMeOp()) {
    case kMeOpIvar: {
      auto *base = static_cast<IvarMeExpr&>(meExpr).GetBase();
      auto *valueRange = FindValueRangeInCaches(bb.GetBBId(), base->GetExprID());
      if (valueRange == nullptr) {
        Insert2Caches(bb.GetBBId(), base->GetExprID(),
                      std::make_unique<ValueRange>(Bound(nullptr, 0, base->GetPrimType()), kNotEqual));
      } else {
        auto newValueRange = ZeroIsInRange(*valueRange);
        if (newValueRange != nullptr) {
          Insert2Caches(bb.GetBBId(), base->GetExprID(), std::move(newValueRange));
        }
      }
      break;
    }
    case kMeOpVar: {
      if (static_cast<ScalarMeExpr&>(meExpr).GetOst()->GetMIRSymbol()->GetAttr(ATTR_nonnull)) {
        Insert2Caches(bb.GetBBId(), meExpr.GetExprID(),
                      std::make_unique<ValueRange>(Bound(nullptr, 0, meExpr.GetPrimType()), kNotEqual));
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
            auto *valueRange = FindValueRangeInCaches(bb.GetBBId(), opnd->GetExprID());
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
          auto *valueRange = FindValueRangeInCaches(bb.GetBBId(), opnd->GetExprID());
          if (pointerType->GetPointedType()->GetBitOffsetFromBaseAddr(opMeExpr.GetFieldID()) != 0 ||
              (valueRange != nullptr && valueRange->IsNotEqualZero())) {
            Insert2Caches(bb.GetBBId(), opMeExpr.GetExprID(),
                          std::make_unique<ValueRange>(Bound(nullptr, 0, meExpr.GetPrimType()), kNotEqual));
          }
          break;
        }
        default:
          break;
      }
      break;
    }
    case kMeOpAddrof: {
      auto *valueRangeOfAddrof = FindValueRangeInCaches(bb.GetBBId(), meExpr.GetExprID());
      if (valueRangeOfAddrof == nullptr) {
        Insert2Caches(bb.GetBBId(), meExpr.GetExprID(),
                      std::make_unique<ValueRange>(Bound(nullptr, 0, meExpr.GetPrimType()), kNotEqual));
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
    auto *valueRange = FindValueRangeInCaches(bb.GetBBId(), opMeExpr.GetExprID());
    if (valueRange != nullptr) {
      return;
    }
    Insert2Caches(bb.GetBBId(), opMeExpr.GetExprID(), std::make_unique<ValueRange>(
        Bound(nullptr, 0, fromType), Bound(GetMaxNumber(fromType), fromType), kLowerAndUpper));
  }
}

void ValueRangePropagation::InsertCandsForSSAUpdate(OStIdx ostIdx, const BB &bb) {
  if (cands.find(ostIdx) == cands.end()) {
    MapleSet<BBId> *bbSet = memPool.New<MapleSet<BBId>>(std::less<BBId>(), mpAllocator.Adapter());
    bbSet->insert(bb.GetBBId());
    cands[ostIdx] = bbSet;
  } else {
    cands[ostIdx]->insert(bb.GetBBId());
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

void ValueRangePropagation::InsertCandsForSSAUpdate(MeStmt &meStmt, BB &bb) {
  if (meStmt.GetVarLHS() != nullptr) {
    InsertCandsForSSAUpdate(meStmt.GetVarLHS()->GetOstIdx(), bb);
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
  auto *valueRange = FindValueRangeInCaches(bb.GetBBId(), expr.GetExprID());
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
    auto *valueRange = FindValueRangeInCaches(bb.GetBBId(), opnd0->GetExprID());
    if (valueRange == nullptr) {
      return nullptr;
    }
    newValueRange = AddOrSubWithValueRange(opMeExpr.GetOp(), *valueRange, rhsConstant);
  } else if (lhsIsConstant && opMeExpr.GetOp() == OP_add) {
    auto *valueRange = FindValueRangeInCaches(bb.GetBBId(), opnd1->GetExprID());
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
    auto *valueRangeOfLength = FindValueRangeInCaches(bb.GetBBId(), rhs.GetExprID());
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
int64 ValueRangePropagation::GetRealValue(int64 value, PrimType primType) const {
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
void ValueRangePropagation::DealWithAssign(const BB &bb, MeStmt &stmt) {
  auto *lhs = stmt.GetLHS();
  auto *rhs = stmt.GetRHS();
  auto *existValueRange = FindValueRangeInCaches(bb.GetBBId(), rhs->GetExprID());
  if (existValueRange != nullptr) {
    (void)Insert2Caches(bb.GetBBId(), lhs->GetExprID(), CopyValueRange(*existValueRange));
    return;
  }
  if (rhs->GetMeOp() == kMeOpOp) {
    DealWithMeOp(bb, stmt);
  } else if (rhs->GetMeOp() == kMeOpConst && static_cast<ConstMeExpr*>(rhs)->GetConstVal()->GetKind() == kConstInt) {
    if (FindValueRangeInCaches(bb.GetBBId(), lhs->GetExprID()) != nullptr) {
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
  } else if (rhs->GetMeOp() == kMeOpIvar) {
    if (use2Defs.find(rhs->GetExprID()) == use2Defs.end()) {
      use2Defs[rhs->GetExprID()] = std::set<int32>{ lhs->GetExprID() };
    } else {
      use2Defs[rhs->GetExprID()].insert(lhs->GetExprID());
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
  ValueRange *valueRangeOfInit = FindValueRangeInCaches(bb.GetBBId(), init.GetExprID());
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
    return nullptr;
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

std::unique_ptr<ValueRange> ValueRangePropagation::MergeValueRangeOfPhiOperands(const BB &bb, MePhiNode &mePhiNode) {
  std::unique_ptr<ValueRange> mergeRange = nullptr;
  auto *valueRangeOfOpnd0 = FindValueRangeInCaches(bb.GetPred(0)->GetBBId(), mePhiNode.GetOpnd(0)->GetExprID());
  if (valueRangeOfOpnd0 == nullptr) {
    return nullptr;
  }
  for (size_t i = 1; i < mePhiNode.GetOpnds().size(); ++i) {
    auto *operand = mePhiNode.GetOpnd(i);
    auto *valueRange = FindValueRangeInCaches(bb.GetPred(i)->GetBBId(), operand->GetExprID());
    if (valueRange == nullptr) {
      return nullptr;
    }
    if (!valueRangeOfOpnd0->IsEqual(valueRange)) {
      return nullptr;
    }
  }
  return CopyValueRange(*valueRangeOfOpnd0);
}

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
      auto *lowerRangeValue = FindValueRangeInCaches(bb.GetBBId(), lowerVar->GetExprID());
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
  auto *upperRangeValue = FindValueRangeInCaches(bb.GetBBId(), upperVar->GetExprID());
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
// the loop would not exit, so need add wont exit bb for the loop.
void ValueRangePropagation::ChangeLoop2WontExit(LoopDesc &loop, BB &bb, BB &succBB, BB &unreachableBB) {
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
  // create artificial BB to transition to common_exit_bb
  BB *newBB = func.GetCfg()->NewBasicBlock();
  caches.resize(caches.size() + 1);
  newBB->SetKindReturn();
  newBB->SetAttributes(kBBAttrArtificial);
  bb.AddSucc(*newBB);
  func.GetCfg()->GetCommonExitBB()->AddExit(*newBB);
  for (auto id : loop.loopBBs) {
    auto *bb = func.GetCfg()->GetBBFromID(id);
    bb->SetAttributes(kBBAttrWontExit);
  }
}

void ValueRangePropagation::AnalysisUnreachableBBOrEdge(BB &unreachableBB) {
  std::set<BB*> traveledBB;
  std::list<BB*> bbList;
  bbList.push_back(&unreachableBB);
  while (!bbList.empty()) {
    BB *curr = bbList.front();
    bbList.pop_front();
    if (!dom.Dominate(unreachableBB, *curr)) {
      bool canBeDeleted = true;
      for (auto &pred : curr->GetPred()) {
        if (dom.Dominate(*curr, *pred)) { // Currbb is the head of curr loop and pred is the backedge of curr loop.
          continue;
        }
        if (unreachableBBs.find(pred) == unreachableBBs.end()) {
          canBeDeleted = false;
        }
      }
      if (!canBeDeleted) {
        needUpdateSSA = true;
        continue;
      }
    }
    if (traveledBB.find(curr) != traveledBB.end()) {
      continue;
    }
    unreachableBBs.insert(curr);
    if (ValueRangePropagation::isDebug) {
      LogInfo::MapleLogger() << curr->GetBBId() << " ";
    }
    for (BB *curSucc : curr->GetSucc()) {
      bbList.push_back(curSucc);
    }
    traveledBB.insert(curr);
  }
  if (ValueRangePropagation::isDebug) {
    LogInfo::MapleLogger() << "\n";
  }
}

// when determine remove the condgoto stmt, need analysis which bbs need be deleted.
void ValueRangePropagation::AnalysisUnreachableBBOrEdge(BB &bb, BB &unreachableBB, BB &succBB) {
  AnalysisUnreachableBBOrEdge(unreachableBB);
  bb.RemoveSucc(unreachableBB);
  bb.RemoveMeStmt(bb.GetLastMe());
  bb.SetKind(kBBFallthru);
  if (ValueRangePropagation::isDebug) {
    LogInfo::MapleLogger() << "=============delete bb=============" << "\n";
  }
  auto *loop = loops->GetBBLoopParent(bb.GetBBId());
  if (loop == nullptr) {
    return;
  }
  ChangeLoop2WontExit(*loop, bb, succBB, unreachableBB);
}

// Judge whether the value range is in range.
bool ValueRangePropagation::BrStmtInRange(BB &bb, ValueRange &leftRange, ValueRange &rightRange, Opcode op,
                                          bool judgeNotInRange) {
  if (leftRange.GetLower().GetVar() != leftRange.GetUpper().GetVar() ||
      leftRange.GetUpper().GetVar() != rightRange.GetUpper().GetVar() ||
      rightRange.GetUpper().GetVar() != rightRange.GetLower().GetVar()) {
    return false;
  }
  if (judgeNotInRange) {
    if (leftRange.GetLower().GetVar() != nullptr) {
      return false;
    }
  } else {
    if (leftRange.GetLower().GetVar() != nullptr) {
      auto *valueRange = FindValueRangeInCaches(bb.GetBBId(), leftRange.GetLower().GetVar()->GetExprID());
      if (valueRange == nullptr || !valueRange->IsConstant()) {
        return false;
      }
    }
  }
  if (leftRange.GetLower().GetConstant() > leftRange.GetUpper().GetConstant()) {
    return false;
  }
  if (rightRange.GetLower().GetConstant() > rightRange.GetUpper().GetConstant()) {
    return false;
  }
  if (op == OP_ge) {
    return judgeNotInRange ? leftRange.GetUpper().GetConstant() < rightRange.GetLower().GetConstant() :
        leftRange.GetLower().GetConstant() >= rightRange.GetUpper().GetConstant();
  } else if (op == OP_gt) {
    return judgeNotInRange ? leftRange.GetUpper().GetConstant() <= rightRange.GetLower().GetConstant() :
        leftRange.GetLower().GetConstant() > rightRange.GetUpper().GetConstant();
  } else if (op == OP_lt) {
    return judgeNotInRange ? leftRange.GetLower().GetConstant() >= rightRange.GetUpper().GetConstant() :
        leftRange.GetUpper().GetConstant() < rightRange.GetLower().GetConstant();
  } else if (op == OP_le) {
    return judgeNotInRange ? leftRange.GetLower().GetConstant() > rightRange.GetUpper().GetConstant() :
        leftRange.GetUpper().GetConstant() <= rightRange.GetLower().GetConstant();
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

void ValueRangePropagation::DealWithOPLeOrLt(
    BB &bb, ValueRange *leftRange, Bound newRightUpper, Bound newRightLower, const CondGotoMeStmt &brMeStmt) {
  CHECK_FATAL(IsEqualPrimType(newRightUpper.GetPrimType(), newRightLower.GetPrimType()), "must be equal");
  BB *trueBranch = nullptr;
  BB *falseBranch = nullptr;
  GetTrueAndFalseBranch(brMeStmt.GetOp(), bb, trueBranch, falseBranch);
  MeExpr *opnd0 = static_cast<OpMeExpr*>(brMeStmt.GetOpnd())->GetOpnd(0);
  if (leftRange == nullptr) {
    std::unique_ptr<ValueRange> newTrueBranchRange =
        std::make_unique<ValueRange>(ValueRange::MinBound(newRightUpper.GetPrimType()), newRightUpper, kLowerAndUpper);
    (void)Insert2Caches(trueBranch->GetBBId(), opnd0->GetExprID(), std::move(newTrueBranchRange));
    std::unique_ptr<ValueRange> newFalseBranchRange =
        std::make_unique<ValueRange>(newRightLower, ValueRange::MaxBound(newRightUpper.GetPrimType()), kLowerAndUpper);
    (void)Insert2Caches(falseBranch->GetBBId(), opnd0->GetExprID(), std::move(newFalseBranchRange));
  } else {
    std::unique_ptr<ValueRange> newRightRange =
        std::make_unique<ValueRange>(ValueRange::MinBound(newRightUpper.GetPrimType()), newRightUpper, kLowerAndUpper);
    (void)Insert2Caches(trueBranch->GetBBId(), opnd0->GetExprID(),
                        CombineTwoValueRange(*leftRange, *newRightRange));
    newRightRange = std::make_unique<ValueRange>(
        newRightLower, ValueRange::MaxBound(newRightUpper.GetPrimType()), kLowerAndUpper);
    (void)Insert2Caches(falseBranch->GetBBId(), opnd0->GetExprID(),
        CombineTwoValueRange(*leftRange, *newRightRange));
  }
}

void ValueRangePropagation::DealWithOPGeOrGt(
    BB &bb, ValueRange *leftRange, Bound newRightUpper, Bound newRightLower, const CondGotoMeStmt &brMeStmt) {
  CHECK_FATAL(IsEqualPrimType(newRightUpper.GetPrimType(), newRightLower.GetPrimType()), "must be equal");
  BB *trueBranch = nullptr;
  BB *falseBranch = nullptr;
  GetTrueAndFalseBranch(brMeStmt.GetOp(), bb, trueBranch, falseBranch);
  MeExpr *opnd0 = static_cast<OpMeExpr*>(brMeStmt.GetOpnd())->GetOpnd(0);
  if (leftRange == nullptr) {
    std::unique_ptr<ValueRange> newTrueBranchRange =
        std::make_unique<ValueRange>(newRightLower, ValueRange::MaxBound(newRightUpper.GetPrimType()), kLowerAndUpper);
    (void)Insert2Caches(trueBranch->GetBBId(), opnd0->GetExprID(), std::move(newTrueBranchRange));
    std::unique_ptr<ValueRange> newFalseBranchRange =
        std::make_unique<ValueRange>(ValueRange::MinBound(newRightUpper.GetPrimType()), newRightUpper, kLowerAndUpper);
    (void)Insert2Caches(falseBranch->GetBBId(), opnd0->GetExprID(), std::move(newFalseBranchRange));
  } else {
    std::unique_ptr<ValueRange> newRightRange =
        std::make_unique<ValueRange>(newRightLower, ValueRange::MaxBound(newRightUpper.GetPrimType()), kLowerAndUpper);
    (void)Insert2Caches(trueBranch->GetBBId(), opnd0->GetExprID(), CombineTwoValueRange(*leftRange, *newRightRange));
    newRightRange = std::make_unique<ValueRange>(
        ValueRange::MinBound(newRightUpper.GetPrimType()), newRightUpper, kLowerAndUpper);
    (void)Insert2Caches(falseBranch->GetBBId(), opnd0->GetExprID(),
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
    if (leftRange == nullptr || leftRange->GetRangeType() != kNotEqual ||
        leftRange->GetBound().GetVar() != rightRange.GetBound().GetVar() ||
        leftRange->GetBound().GetConstant() != rightRange.GetBound().GetConstant()) {
      std::unique_ptr<ValueRange> newFalseBranchRange =
          std::make_unique<ValueRange>(rightRange.GetBound(), kEqual);
      (void)Insert2Caches(falseBranch.GetBBId(), opnd.GetExprID(), std::move(newFalseBranchRange));
    }
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

BB *ValueRangePropagation::CreateNewBasicBlockWithoutCondGotoStmt(BB &bb) {
  if (CodeSizeIsOverflow(bb)) {
    return nullptr;
  }
  BB *newBB = func.GetCfg()->NewBasicBlock();
  caches.resize(caches.size() + 1);
  newBB->SetKind(kBBFallthru);
  CopyMeStmts(bb, *newBB, true);
  return newBB;
}

void ValueRangePropagation::CopyMeStmts(BB &fromBB, BB &toBB, bool copyWithoutCondGotoStmt) {
  if (copyWithoutCondGotoStmt) {
    if (fromBB.GetKind() != kBBCondGoto && fromBB.GetKind() != kBBGoto) {
      CHECK_FATAL(false, "must be condGoto or goto bb");
    }
  } else {
    CHECK_FATAL(fromBB.GetKind() == kBBFallthru, "must be fallthru or goto bb");
  }
  LoopUnrolling::CopyAndInsertStmt(
      irMap, memPool, mpAllocator, cands, toBB, fromBB, copyWithoutCondGotoStmt);
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

bool ValueRangePropagation::CopyFallthruBBAndRemoveUnreachableEdgeWhenDealWithShortCircuit(
    BB &pred, BB &bb, BB &trueBranch) {
  auto *exitCopyFallthru = GetNewCopyFallthruBB(trueBranch, bb);
  if (exitCopyFallthru != nullptr) {
    size_t index = FindBBInSuccs(pred, bb);
    pred.RemoveSucc(bb);
    DeleteThePhiNodeWhichOnlyHasOneOpnd(bb);
    pred.AddSucc(*exitCopyFallthru, index);
    CreateLabelForTargetBB(pred, *exitCopyFallthru);
    return true;
  }
  auto *currBB = &bb;
  BB *currPred = nullptr;
  auto *mergeAllFallthruBBs = func.GetCfg()->NewBasicBlock();
  if (CodeSizeIsOverflow(bb)) {
    return false;
  }
  bool hasGoto = false;
  while (currBB->GetKind() != kBBCondGoto) {
    if (currBB->GetKind() != kBBFallthru && currBB->GetKind() != kBBGoto) {
      CHECK_FATAL(false, "must be fallthru or goto bb");
    }
    // When the size of pred of currBB is more than one, need copy stmt, the cfg change like this:
    // pred0   currPred             pred0    currPred
    //     \  /                       |         |
    //      \/                        |         |
    //    currBB          ---->     currBB   newfallthru (copy stmt from currBB)
    //      |                         |         |
    //   condgoto                  condgoto  mergeAllFallthruBBs (copy stmt except condgoto mestmt from condgoto)
    //     / \                        |   \   /
    //    /   \                       |    \ /
    // false  true                  false  true
    if (GetRealPredSize(*currBB) > 1 && !hasGoto) {
      CHECK_FATAL(currBB != &bb, "must not equal");
      caches.resize(caches.size() + 1);
      if (currBB->GetKind() == kBBGoto) {
        CopyMeStmts(*currBB, *mergeAllFallthruBBs, true);
      } else {
        CopyMeStmts(*currBB, *mergeAllFallthruBBs);
      }
      CHECK_NULL_FATAL(currPred);
      currPred->RemoveSucc(*currBB);
      DeleteThePhiNodeWhichOnlyHasOneOpnd(*currBB);
      currPred->AddSucc(*mergeAllFallthruBBs);
      CreateLabelForTargetBB(*currPred, *mergeAllFallthruBBs);
      if (currPred->GetKind() == kBBGoto) {
        CHECK_FATAL(!currPred->GetMeStmts().empty(), "must not be empty");
        CHECK_FATAL(currPred->GetLastMe()->GetOp() == OP_goto, "must be goto");
        currPred->RemoveMeStmt(currPred->GetLastMe());
      }
      if (!currBB->GetMePhiList().empty()) {
        for (auto &it : currBB->GetMePhiList()) {
          InsertCandsForSSAUpdate(it.first, *mergeAllFallthruBBs);
        }
      }
      hasGoto = true;
    } else {
      if (hasGoto) {
        if (!currBB->GetMeStmts().empty()) {
          if (currBB->GetKind() == kBBGoto) {
            CopyMeStmts(*currBB, *mergeAllFallthruBBs, true);
          } else {
            CopyMeStmts(*currBB, *mergeAllFallthruBBs);
          }
        }
        if (!currBB->GetMePhiList().empty()) {
          for (auto &it : currBB->GetMePhiList()) {
            InsertCandsForSSAUpdate(it.first, *mergeAllFallthruBBs);
          }
        }
      }
    }
    currPred = currBB;
    currBB = currBB->GetSucc(0);
  }
  if (ValueRangePropagation::isDebug) {
    LogInfo::MapleLogger() << "old: " << pred.GetBBId() << " new: " << mergeAllFallthruBBs->GetBBId() << "\n";
  }
  mergeAllFallthruBBs->SetKind(kBBFallthru);
  caches.resize(caches.size() + 1);
  CopyMeStmts(*currBB, *mergeAllFallthruBBs, true);
  mergeAllFallthruBBs->AddSucc(trueBranch);
  thePredEdgeIsRemoved = true;
  Insert2TrueOrFalseBranch2NewCopyFallthru(trueBranch, bb, *mergeAllFallthruBBs);
  return true;
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

bool ValueRangePropagation::CodeSizeIsOverflow(const BB &bb) {
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
    DeleteThePhiNodeWhichOnlyHasOneOpnd(bb);
    pred.AddSucc(*exitCopyFallthru, index);
    CreateLabelForTargetBB(pred, *exitCopyFallthru);
    return true;
  }
  if (CodeSizeIsOverflow(bb)) {
    return false;
  }
  auto *mergeAllFallthruBBs = func.GetCfg()->NewBasicBlock();
  if (ValueRangePropagation::isDebug) {
    LogInfo::MapleLogger() << "old: " << pred.GetBBId() << " new: " << mergeAllFallthruBBs->GetBBId() << "\n";
  }
  mergeAllFallthruBBs->SetKind(kBBFallthru);
  caches.resize(caches.size() + 1);
  auto *currBB = &bb;
  while (currBB->GetKind() != kBBCondGoto) {
    if (currBB->GetKind() != kBBFallthru && currBB->GetKind() != kBBGoto) {
      CHECK_FATAL(false, "must be fallthru or goto bb");
    }
    if (!currBB->GetMeStmts().empty()) {
      if (currBB->GetKind() == kBBGoto) {
        CopyMeStmts(*currBB, *mergeAllFallthruBBs, true);
      } else {
        CopyMeStmts(*currBB, *mergeAllFallthruBBs);
      }
    }
    if (!currBB->GetMePhiList().empty()) {
      for (auto &it : currBB->GetMePhiList()) {
        InsertCandsForSSAUpdate(it.first, *mergeAllFallthruBBs);
      }
    }
    currBB = currBB->GetSucc(0);
  }
  CHECK_FATAL(currBB->GetKind() == kBBCondGoto, "must be condgoto bb");
  CopyMeStmts(*currBB, *mergeAllFallthruBBs, true);
  size_t index = FindBBInSuccs(pred, bb);
  pred.RemoveSucc(bb);
  DeleteThePhiNodeWhichOnlyHasOneOpnd(bb);
  pred.AddSucc(*mergeAllFallthruBBs, index);
  mergeAllFallthruBBs->AddSucc(trueBranch);
  CreateLabelForTargetBB(pred, *mergeAllFallthruBBs);
  thePredEdgeIsRemoved = true;
  Insert2TrueOrFalseBranch2NewCopyFallthru(trueBranch, bb, *mergeAllFallthruBBs);
  return true;
}

// If the valuerange of opnd in pred0 and pred1 is equal the valuerange in true branch:
//     pred0   pred1                        pred0    pred1                         pred0    pred1
//         \  /                               |        |                             |        |
//          \/                                |        |                             |        |
//          bb  [fallthru]                 bb(copy)    bb                         bb(copy)    bb
//          |                  step1          |        |            step2            |        |
//          bb1 [fallthru]    ------>      bb1(copy)   bb1        -------->       bb1(copy)   bb1
//          |                                 |        |                             |        |
//          bb2 [condgoto]                    |        bb2 [condgoto]                |        bb2 [fallthru]
//          /\                                 \     / |                             |        |
//         /  \                                 \   /  |                             |        |
//      true  false                             true false                         true     false
bool ValueRangePropagation::CopyFallthruBBAndRemoveUnreachableEdge(
    BB &pred, BB &bb, BB &trueBranch, bool dealWithShortCircuit) {
  // step1
  if (GetRealPredSize(bb) > 1) {
    return ChangeTheSuccOfPred2TrueBranch(pred, bb, trueBranch);
  } else {
    if (dealWithShortCircuit) {
      return CopyFallthruBBAndRemoveUnreachableEdgeWhenDealWithShortCircuit(pred, bb, trueBranch);
    }
    // step2
    CHECK_FATAL(GetRealPredSize(bb) == 1, "must have one succ");
    auto *currBB = &bb;
    while (currBB->GetKind() != kBBCondGoto) {
      currBB = currBB->GetSucc(0);
    }
    RemoveUnreachableBB(*currBB, trueBranch);
  }
  return true;
}

// If the valuerange of opnd in pred0 and pred1 is equal the valuerange in true branch:
//     pred0   pred1                        pred0    pred1                         pred0    pred1
//         \  /                               |        |                             |        |
//          \/                step1           |        |                step2        |        |
//          bb2 [condgoto]   ------->         |        bb2 [condgoto]  ------->      |        bb2 [fallthru]
//          /\                                 \     / |                             |        |
//         /  \                                 \   /  |                             |        |
//      true  false                             true false                         true     false
bool ValueRangePropagation::RemoveTheEdgeOfPredBB(BB &pred, BB &bb, BB &trueBranch) {
  CHECK_FATAL(bb.GetKind() == kBBCondGoto, "must be condgoto bb");
  // step1
  if (GetRealPredSize(bb) >= kNumOperands) {
    thePredEdgeIsRemoved = true;
    if (OnlyHaveCondGotoStmt(bb)) {
      size_t index = FindBBInSuccs(pred, bb);
      pred.RemoveSucc(bb);
      DeleteThePhiNodeWhichOnlyHasOneOpnd(bb);
      pred.AddSucc(trueBranch, index);
      CreateLabelForTargetBB(pred, trueBranch);
    } else {
      auto *exitCopyFallthru = GetNewCopyFallthruBB(trueBranch, bb);
      if (exitCopyFallthru != nullptr) {
        size_t index = FindBBInSuccs(pred, bb);
        pred.RemoveSucc(bb);
        DeleteThePhiNodeWhichOnlyHasOneOpnd(bb);
        pred.AddSucc(*exitCopyFallthru, index);
        CreateLabelForTargetBB(pred, *exitCopyFallthru);
        return true;
      }
      auto *newBB = CreateNewBasicBlockWithoutCondGotoStmt(bb);
      if (newBB == nullptr) {
        return false;
      }
      if (ValueRangePropagation::isDebug) {
        LogInfo::MapleLogger() << "old: " << pred.GetBBId() << " new: " << newBB->GetBBId() << "\n";
      }
      size_t index = FindBBInSuccs(pred, bb);
      pred.RemoveSucc(bb);
      DeleteThePhiNodeWhichOnlyHasOneOpnd(bb);
      pred.AddSucc(*newBB, index);
      newBB->AddSucc(trueBranch);
      func.GetOrCreateBBLabel(trueBranch);
      CreateLabelForTargetBB(pred, *newBB);
      Insert2TrueOrFalseBranch2NewCopyFallthru(trueBranch, bb, *newBB);
    }
  } else {
    // step2
    CHECK_FATAL(GetRealPredSize(bb) == 1, "must have one succ");
    RemoveUnreachableBB(bb, trueBranch);
    return true;
  }
  return true;
}

bool ValueRangePropagation::RemoveUnreachableEdge(MeExpr &opnd, BB &pred, BB &bb, BB &trueBranch,
    bool dealWithShortCircuit) {
  if (bb.GetKind() == kBBFallthru || bb.GetKind() == kBBGoto) {
    if (!CopyFallthruBBAndRemoveUnreachableEdge(pred, bb, trueBranch, dealWithShortCircuit)) {
      return false;
    }
  } else {
    if (!RemoveTheEdgeOfPredBB(pred, bb, trueBranch)) {
      return false;
    }
  }
  if (ValueRangePropagation::isDebug) {
    LogInfo::MapleLogger() << "=============delete edge " << pred.GetBBId() << " " << bb.GetBBId() << " " <<
        trueBranch.GetBBId() << "=============" << "\n";
  }
  InsertCandsForSSAUpdate(pred, true);
  for (auto &it : bb.GetMePhiList()) {
    InsertCandsForSSAUpdate(it.first, pred);
  }
  if (pred2NewSuccs.find(&trueBranch) == pred2NewSuccs.end()) {
    pred2NewSuccs[&trueBranch] = std::set<std::pair<BB*, MeExpr*>>{ std::make_pair(&pred, &opnd) };
  } else {
    pred2NewSuccs[&trueBranch].insert(std::make_pair(&pred, &opnd));
  }
  if (trueBranch.GetPred().size() > 1) {
    (void)func.GetOrCreateBBLabel(trueBranch);
  }
  InsertCandsForSSAUpdate(bb, thePredEdgeIsRemoved);
  needUpdateSSA = true;
  isCFGChange = true;
  return true;
}

bool ValueRangePropagation::ConditionEdgeCanBeDeletedAfterOPNeOrEq(MeExpr &opnd, BB &pred, BB &bb,
    ValueRange *leftRange, ValueRange &rightRange, BB &falseBranch, BB &trueBranch, PrimType opndType,
    bool dealWithShortCircuit) {
  if (leftRange == nullptr) {
    return false;
  }
  if ((leftRange->GetRangeType() == kEqual && rightRange.GetRangeType() == kEqual) &&
      leftRange->GetBound().GetVar() == rightRange.GetBound().GetVar() &&
      GetRealValue(leftRange->GetBound().GetConstant(), opndType) ==
      GetRealValue(rightRange.GetBound().GetConstant(), opndType)) {
    return RemoveUnreachableEdge(opnd, pred, bb, trueBranch, dealWithShortCircuit);
  } else if (((leftRange->GetRangeType() == kNotEqual && rightRange.GetRangeType() == kEqual) ||
              (leftRange->GetRangeType() == kEqual && rightRange.GetRangeType() == kNotEqual)) &&
             leftRange->GetBound().GetVar() == rightRange.GetBound().GetVar() &&
             GetRealValue(leftRange->GetBound().GetConstant(), opndType) ==
             GetRealValue(rightRange.GetBound().GetConstant(), opndType)) {
    return RemoveUnreachableEdge(opnd, pred, bb, falseBranch, dealWithShortCircuit);
  } else if ((leftRange->GetRangeType() == kEqual && rightRange.GetRangeType() == kEqual) &&
             leftRange->GetBound().GetVar() == rightRange.GetBound().GetVar() &&
             GetRealValue(leftRange->GetBound().GetConstant(), opndType) !=
             GetRealValue(rightRange.GetBound().GetConstant(), opndType)) {
    return RemoveUnreachableEdge(opnd, pred, bb, falseBranch, dealWithShortCircuit);
  }
  return false;
}

void ValueRangePropagation::MergeValueRangeOfPred(BB &bb, const MeExpr &opnd) {
  for (auto it : pred2NewSuccs) {
    if (unreachableBBs.find(&bb) == unreachableBBs.end() && it.first->IsSuccBB(bb) && it.first->GetPred().size() != 1) {
      continue;
    }
    if (it.second.empty()) {
      continue;
    }
    ValueRange *valueRangeFirst = nullptr;
    bool canNotPropValueRange = false;
    for (auto pair : it.second) {
      if (valueRangeFirst == nullptr) {
        valueRangeFirst = FindValueRangeInCaches(pair.first->GetBBId(), pair.second->GetExprID());
        CHECK_NULL_FATAL(valueRangeFirst);
        continue;
      }
      auto *valueRange = FindValueRangeInCaches(pair.first->GetBBId(), pair.second->GetExprID());
      CHECK_NULL_FATAL(valueRange);
      if (!valueRangeFirst->IsEqual(valueRange)) {
        canNotPropValueRange = true;
        break;
      }
    }
    if (canNotPropValueRange) {
      continue;
    }
    auto *valueRangeOfTrueBranchExsit = FindValueRangeInCaches(it.first->GetBBId(), opnd.GetExprID());
    if (valueRangeOfTrueBranchExsit == nullptr || valueRangeOfTrueBranchExsit->IsEqual(valueRangeFirst)) {
      Insert2Caches(it.first->GetBBId(), opnd.GetExprID(), CopyValueRange(*valueRangeFirst));
    }
  }
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
    if (ValueRangePropagation::isDebug) {
      LogInfo::MapleLogger() << "=============delete bb "<< curBB.GetBBId() << "=============\n";
    }
    // If the preds of curBB which is condgoto and analysised is empty, delete the curBB.
    unreachableBBs.insert(&curBB);
    // If the preds of falseBranch is empty, delete the falseBranch.
    if (falseBranch.GetPred().size() == 1 && falseBranch.GetPred(0) == &curBB) {
      AnalysisUnreachableBBOrEdge(falseBranch);
    }
    // If the preds of trueBranch is empty, delete the trueBranch.
    if (trueBranch.GetPred().size() == 1 && trueBranch.GetPred(0) == &curBB) {
      AnalysisUnreachableBBOrEdge(trueBranch);
    }
  }
}

void ValueRangePropagation::PropValueRangeOfNewPredOfTrue2FalseBranch(
    MeExpr &opnd, BB &condGoto, BB &bb, std::unique_ptr<ValueRange> valueRange) {
  if (!trueOrFalseBranch2NewPred[&bb].empty()) {
    trueOrFalseBranch2NewPred[&bb].push_back(std::make_pair(&condGoto, valueRange.get()));
    valueRanges.insert(std::move(valueRange));
    auto it = newMergeBB2Opnd.find(&bb);
    if (it != newMergeBB2Opnd.end()) {
      CHECK_FATAL(it->second == opnd.GetExprID(), "must be equal");
    } else {
      newMergeBB2Opnd[&bb] = opnd.GetExprID();
    }
  } else {
    Insert2Caches(bb.GetBBId(), opnd.GetExprID(), std::move(valueRange));
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

void ValueRangePropagation::PropValueRangeFromCondGotoToTrueAndFalseBranch(
    BB &curBB, MeExpr &opnd0, ValueRange &rightRange, BB &falseBranch, BB &trueBranch,
    std::unordered_map<BB*, ValueRange*> &pred2ValueRange) {
  DeleteUnreachableBBs(curBB, falseBranch, trueBranch);
  if (curBB.GetKind() == kBBCondGoto || trueBranch.GetPred().size() > 1 || falseBranch.GetPred().size() > 1) {
    for (auto pair : pred2NewSuccs) {
      for (auto it : pair.second) {
        std::pair<BB*, ValueRange*> currPair;
        auto itOfValueRange = pred2ValueRange.find(it.first);
        if (itOfValueRange == pred2ValueRange.end()) {
          continue;
        }
        currPair = std::make_pair(it.first, itOfValueRange->second);
        if (trueOrFalseBranch2NewPred.find(pair.first) == trueOrFalseBranch2NewPred.end()) {
          trueOrFalseBranch2NewPred[pair.first] = std::vector<std::pair<BB*, ValueRange*>>{ currPair };
          Insert2NewMergeBB2Opnd(*pair.first, opnd0);
        } else {
          trueOrFalseBranch2NewPred[pair.first].push_back(currPair);
          Insert2NewMergeBB2Opnd(*pair.first, opnd0);
        }
      }
    }
    if (curBB.GetKind() == kBBCondGoto) {
      PropValueRangeFromCondGotoToTrueAndFalseBranch(opnd0, rightRange, falseBranch, trueBranch);
    }
  } else {
    PropValueRangeFromCondGotoToTrueAndFalseBranch(opnd0, rightRange, falseBranch, trueBranch);
  }
  PropValueRangeFromCondGotoToTrueAndFalseBranch(opnd0, rightRange, falseBranch, trueBranch);
}

void ValueRangePropagation::IsTheOpndOfCondGotoStmtIsPhi(
    BB &bb, MeExpr &opnd0, BB *&defBB, MapleVector<ScalarMeExpr*> &opnds, bool &thePhiIsInBB) {
  if ((opnd0.GetMeOp() == kMeOpVar || opnd0.GetMeOp() == kMeOpReg) &&
      static_cast<ScalarMeExpr&>(opnd0).GetDefBy() == kDefByPhi) {
    defBB = static_cast<ScalarMeExpr&>(opnd0).GetDefPhi().GetDefBB();
    if (dom.Dominate(*defBB, bb) && MustBeFallthruOrGoto(*defBB, bb)) {
      opnds = static_cast<ScalarMeExpr&>(opnd0).GetDefPhi().GetOpnds();
      thePhiIsInBB = true;
    } else {
      defBB = nullptr;
    }
  }
}

// Deal with short circurt like this:
//   pred0  pred1                      pred1                        pred1
//     \     /                          /                            /
//      \   /                          /                            /
//    condgoto              pred0 condgoto                     condgoto
//      /  \                  \     /  \                         /  \
//     /    \     step1        \   /    \        step2          /    \
//  true   false  ---->        true    false     ---->        true  false
//     \    /                    \      /                       \    /
//      \  /                      \    /                         \  /
//    condgoto                   condgoto               pred0  condgoto
//      / \                        /  \                     \     /  \
//     /   \                      /    \                     \   /    \
//  true  false                 true   false                  true   false
void ValueRangePropagation::DealWithShortCricuit(MeExpr &opnd, ValueRange &rightRange, BB &falseBranch,
    BB &trueBranch, BB &newMerge, PrimType opndType, bool &opt, std::unordered_map<BB*, ValueRange*> &pred2ValueRange,
    bool hasCondGotoFromDomToPredBB) {
  if (hasCondGotoFromDomToPredBB) {
    return;
  }
  for (auto vecIt : trueOrFalseBranch2NewPred[&newMerge]) {
    pred2ValueRange[vecIt.first] = vecIt.second;
    CHECK_FATAL(vecIt.first->GetSucc().size() == 1, "must have one succ bb");
    if (ConditionEdgeCanBeDeletedAfterOPNeOrEq(
        opnd, *vecIt.first, *vecIt.first->GetSucc(0), vecIt.second, rightRange, falseBranch,
        trueBranch, opndType, true)) {
      if (ValueRangePropagation::isDebug) {
        LogInfo::MapleLogger() << "================================find short cricuit\n";
      }
      opt = true;
    }
  }
  if (newMerge.GetPred().size() == 1 && unreachableBBs.find(newMerge.GetPred(0)) == unreachableBBs.end() &&
      newMerge.GetPred(0)->GetKind() == kBBCondGoto) {
    auto *valueRange = FindValueRangeInCurrentBB(newMerge.GetBBId(), opnd.GetExprID());
    pred2ValueRange[&newMerge] = valueRange;
    if (ConditionEdgeCanBeDeletedAfterOPNeOrEq(
        opnd, newMerge, *newMerge.GetSucc(0), valueRange, rightRange, falseBranch, trueBranch, opndType)) {
      opt = true;
    }
  }
  return;
}

bool ValueRangePropagation::AnalysisValueRangeInPredsOfCondGotoBB(
    BB &bb, MeExpr &opnd0, ValueRange &rightRange, BB &falseBranch, BB &trueBranch, PrimType opndType, BB *condGoto) {
  BB *curBB = (condGoto == nullptr) ? &bb : condGoto;
  bool opt = false;
  bool thePhiIsInBB = false;
  MapleVector<ScalarMeExpr*> opnds(mpAllocator.Adapter());
  BB *defBB = nullptr;
  IsTheOpndOfCondGotoStmtIsPhi(bb, opnd0, defBB, opnds, thePhiIsInBB);
  size_t indexOfOpnd = 0;
  std::unordered_map<BB*, ValueRange*> pred2ValueRange;
  for (size_t i = 0; i < bb.GetPred().size();) {
    size_t predSize = bb.GetPred().size();
    auto *pred = bb.GetPred(i);
    if (thePhiIsInBB && defBB != &bb) {
      pred = defBB->GetPred(i);
    }
    if (pred->GetKind() == kBBIgoto || unreachableBBs.find(pred) != unreachableBBs.end()) {
      indexOfOpnd++;
      ++i;
      continue;
    }
    auto *opnd = thePhiIsInBB ? opnds.at(indexOfOpnd) : &opnd0;
    indexOfOpnd++;
    newMergeBB = nullptr;
    hasCondGotoFromDomToPredBB = false;
    auto *valueRangeInPred = FindValueRangeInCaches(pred->GetBBId(), opnd->GetExprID());
    thePredEdgeIsRemoved = false;
    if (newMergeBB == nullptr) {
      pred2ValueRange[pred] = valueRangeInPred;
    }
    if (newMergeBB != nullptr && newMergeBB->GetSucc().size() == 1) {
      DealWithShortCricuit(*opnd, rightRange, falseBranch, trueBranch, *newMergeBB, opndType, opt,
                           pred2ValueRange, hasCondGotoFromDomToPredBB);
    } else if (ConditionEdgeCanBeDeletedAfterOPNeOrEq(
        *opnd, *pred, bb, valueRangeInPred, rightRange, falseBranch, trueBranch, opndType)) {
      opt = true;
    }
    if (bb.GetPred().size() == predSize) {
      ++i;
    } else if (bb.GetPred().size() != predSize - 1) {
      CHECK_FATAL(false, "immpossible");
    }
  }
  if (opt) {
    PropValueRangeFromCondGotoToTrueAndFalseBranch(*curBB, opnd0, rightRange, falseBranch, trueBranch, pred2ValueRange);
    if (curBB->GetKind() == kBBCondGoto) {
      // vrp modified condgoto, branch probability is no longer trustworthy, so we invalidate it
      static_cast<CondGotoMeStmt*>(curBB->GetLastMe())->InvalidateBranchProb();
    }
  }
  pred2NewSuccs.clear();
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

bool ValueRangePropagation::ConditionEdgeCanBeDeletedAfterOPNeOrEq(
    BB &bb, MeExpr &opnd0, ValueRange &rightRange, BB &falseBranch, BB &trueBranch, PrimType opndType, BB *condGoto) {
  size_t unreachableBB = 0;
  BB *reachableBB = nullptr;
  GetSizeOfUnreachableBBsAndReachableBB(bb, unreachableBB, reachableBB);
  if (reachableBB != nullptr && bb.GetPred().size() - unreachableBB == 1 &&
      (reachableBB->GetKind() == kBBFallthru || reachableBB->GetKind() == kBBGoto)) {
    if (trueOrFalseBranch2NewPred.find(reachableBB) != trueOrFalseBranch2NewPred.end() &&
        !trueOrFalseBranch2NewPred[reachableBB].empty()) {
      bool opt = false;
      if (newMergeBB2Opnd.find(reachableBB) != newMergeBB2Opnd.end() &&
          newMergeBB2Opnd[reachableBB] == opnd0.GetExprID()) {
        std::unordered_map<BB*, ValueRange*> pred2ValueRange;
        DealWithShortCricuit(opnd0, rightRange, falseBranch, trueBranch, *reachableBB, opndType, opt,
                             pred2ValueRange, hasCondGotoFromDomToPredBB);
        if (opt) {
          PropValueRangeFromCondGotoToTrueAndFalseBranch(condGoto == nullptr ? bb : *condGoto, opnd0,
              rightRange, falseBranch, trueBranch, pred2ValueRange);
        }
      }
      return opt;
    }
    return ConditionEdgeCanBeDeletedAfterOPNeOrEq(*reachableBB, opnd0, rightRange, falseBranch,
                                                  trueBranch, opndType, condGoto == nullptr ? &bb : condGoto);
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
      bb, *currOpnd, rightRange, falseBranch, trueBranch, opndType, condGoto);
  bool canDeleteBB = false;
  for (size_t i = 0; i < bb.GetPred().size(); ++i) {
    if (unreachableBBs.find(bb.GetPred(i)) == unreachableBBs.end()) {
      return opt;
    }
    canDeleteBB = true;
  }
  if (canDeleteBB) {
    if (ValueRangePropagation::isDebug) {
      LogInfo::MapleLogger() << "=====================delete bb " << bb.GetBBId() << "========================\n";
    }
    unreachableBBs.insert(&bb);
    isCFGChange = true;
  }
  return opt;
}

void ValueRangePropagation::DealWithOPNeOrEq(
    BB &bb, ValueRange *leftRange, ValueRange &rightRange, const CondGotoMeStmt &brMeStmt) {
  BB *trueBranch = nullptr;
  BB *falseBranch = nullptr;
  auto *opMeExpr = static_cast<OpMeExpr*>(brMeStmt.GetOpnd());
  MeExpr *opnd0 = opMeExpr->GetOpnd(0);
  if (opMeExpr->GetOp() == OP_eq) {
    GetTrueAndFalseBranch(brMeStmt.GetOp(), bb, trueBranch, falseBranch);
  } else {
    CHECK_FATAL(opMeExpr->GetOp() == OP_ne, "must be OP_ne");
    GetTrueAndFalseBranch(brMeStmt.GetOp(), bb, falseBranch, trueBranch);
  }
  if (ConditionBBCanBeDeletedAfterOPNeOrEq(bb, *leftRange, rightRange, *falseBranch, *trueBranch)) {
    return;
  }
  if (ConditionEdgeCanBeDeletedAfterOPNeOrEq(bb, *opnd0, rightRange, *falseBranch, *trueBranch,
                                             opMeExpr->GetOpndType())) {
    return;
  }
  CreateValueRangeForNeOrEq(*opnd0, leftRange, rightRange, *trueBranch, *falseBranch);
}

void ValueRangePropagation::DealWithOPNeOrEq(
    Opcode op, BB &bb, ValueRange *leftRange, ValueRange &rightRange, const CondGotoMeStmt &brMeStmt) {
  BB *trueBranch = nullptr;
  BB *falseBranch = nullptr;
  GetTrueAndFalseBranch(brMeStmt.GetOp(), bb, trueBranch, falseBranch);
  OpMeExpr *opMeExpr = static_cast<OpMeExpr*>(brMeStmt.GetOpnd());
  MeExpr *opnd0 = opMeExpr->GetOpnd(0);
  if (leftRange == nullptr) {
    if (op == OP_eq) {
      if (ConditionEdgeCanBeDeletedAfterOPNeOrEq(bb, *opnd0, rightRange, *falseBranch, *trueBranch,
                                                 opMeExpr->GetOpndType())) {
        return;
      }
      CreateValueRangeForNeOrEq(*opnd0, leftRange, rightRange, *trueBranch, *falseBranch);
    } else if (op == OP_ne) {
      if (ConditionEdgeCanBeDeletedAfterOPNeOrEq(bb, *opnd0, rightRange, *trueBranch, *falseBranch,
                                                 opMeExpr->GetOpndType())) {
        return;
      }
      CreateValueRangeForNeOrEq(*opnd0, leftRange, rightRange, *falseBranch, *trueBranch);
    }
  } else {
    DealWithOPNeOrEq(bb, leftRange, rightRange, brMeStmt);
  }
}

Opcode ValueRangePropagation::GetTheOppositeOp(Opcode op) const {
  if (op == OP_lt) {
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
  if (op == OP_ne || op == OP_eq) {
    DealWithOPNeOrEq(op, bb, leftRange, rightRange, brMeStmt);
    return;
  }
  Opcode antiOp = GetTheOppositeOp(op);
  if (leftRange != nullptr && leftRange->GetRangeType() != kSpecialLowerForLoop && leftRange->GetRangeType() !=
      kSpecialUpperForLoop && leftRange->GetRangeType() != kNotEqual && rightRange.GetRangeType() != kNotEqual) {
    BB *trueBranch = nullptr;
    BB *falseBranch = nullptr;
    GetTrueAndFalseBranch(brMeStmt.GetOp(), bb, trueBranch, falseBranch);
    if (BrStmtInRange(bb, *leftRange, rightRange, op)) {
      AnalysisUnreachableBBOrEdge(bb, *falseBranch, *trueBranch);
      return;
    } else if (BrStmtInRange(bb, *leftRange, rightRange, antiOp)) {
      AnalysisUnreachableBBOrEdge(bb, *trueBranch, *falseBranch);
      return;
    }
  }
  if ((op == OP_lt) || (op == OP_ge)) {
    int64 constant = 0;
    if (!AddOrSubWithConstant(newRightUpper.GetPrimType(), OP_add, newRightUpper.GetConstant(), -1, constant)) {
      return;
    }
    newRightUpper = Bound(newRightUpper.GetVar(),
        GetRealValue(constant, newRightUpper.GetPrimType()), newRightUpper.GetPrimType());
  }
  if ((op == OP_le) || (op == OP_gt)) {
    int64 constant = 0;
    if (!AddOrSubWithConstant(newRightUpper.GetPrimType(), OP_add, newRightLower.GetConstant(), 1, constant)) {
      return;
    }
    newRightLower = Bound(newRightLower.GetVar(),
                          GetRealValue(constant, newRightUpper.GetPrimType()), newRightUpper.GetPrimType());
  }
  if (op == OP_lt || op == OP_le) {
    if (leftRange != nullptr && leftRange->GetRangeType() == kNotEqual) {
      DealWithOPLeOrLt(bb, nullptr, newRightUpper, newRightLower, brMeStmt);
    } else {
      DealWithOPLeOrLt(bb, leftRange, newRightUpper, newRightLower, brMeStmt);
    }
  } else if (op == OP_gt || op == OP_ge) {
    if (leftRange != nullptr && leftRange->GetRangeType() == kNotEqual) {
      DealWithOPGeOrGt(bb, nullptr, newRightUpper, newRightLower, brMeStmt);
    } else {
      DealWithOPGeOrGt(bb, leftRange, newRightUpper, newRightLower, brMeStmt);
    }
  }
}

bool ValueRangePropagation::GetValueRangeOfCondGotoOpnd(BB &bb, OpMeExpr &opMeExpr, MeExpr &opnd,
    ValueRange *&valueRange, std::unique_ptr<ValueRange> &rightRangePtr) {
  valueRange = FindValueRangeInCaches(bb.GetBBId(), opnd.GetExprID());
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
      auto *valueRangeOfLHS = FindValueRangeInCaches(bb.GetBBId(), lhs->GetExprID());
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
      // if ((a != c) < b),
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

void ValueRangePropagation::DealWithCondGotoWhenRightRangeIsNotExist(BB &bb, MeExpr &opnd0, MeExpr &opnd1, Opcode op) {
  PrimType prim = opnd1.GetPrimType();
  if (!IsNeededPrimType(prim)) {
    return;
  }
  BB *trueBranch = nullptr;
  BB *falseBranch = nullptr;
  GetTrueAndFalseBranch(op, bb, trueBranch, falseBranch);
  switch (op) {
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
// if (a > 0)
// a: valuerange(0, Max)
// ==>
// if (a != 0)
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
  ValueRange *leftRange = FindValueRangeInCaches(bb.GetBBId(), opnd.GetExprID());
  std::unique_ptr<ValueRange> rightRangePtr;
  rightRangePtr = (op == OP_eq) ?
      std::make_unique<ValueRange>(Bound(nullptr, 0, opnd.GetPrimType()), kNotEqual) :
      std::make_unique<ValueRange>(Bound(nullptr, 0, opnd.GetPrimType()), kEqual);
  DealWithCondGoto(bb, OP_eq, leftRange, *rightRangePtr.get(), stmt);
}

void ValueRangePropagation::DealWithCondGotoWithOneOpnd(BB &bb, CondGotoMeStmt &brMeStmt) {
  auto *opMeExpr = static_cast<OpMeExpr*>(brMeStmt.GetOpnd());
  if (opMeExpr->GetOpnd(0)->GetMeOp() == kMeOpVar || opMeExpr->GetOpnd(0)->GetMeOp() == kMeOpReg) {
    auto *scalarExpr = static_cast<ScalarMeExpr*>(opMeExpr->GetOpnd(0));
    MeStmt *defStmt = nullptr;
    (void)scalarExpr->GetDefByBBMeStmt(dom, defStmt);
    if (defStmt != nullptr && defStmt->GetRHS() != nullptr && IsConditionalOp(defStmt->GetRHS()->GetOp())) {
      opMeExpr = static_cast<OpMeExpr*>(defStmt->GetRHS());
    } else {
      return DealWithBrStmtWithOneOpnd(bb, brMeStmt, *opMeExpr->GetOpnd(0), opMeExpr->GetOp());
    }
  } else {
    return DealWithBrStmtWithOneOpnd(bb, brMeStmt, *opMeExpr->GetOpnd(0), opMeExpr->GetOp());
  }
}

void ValueRangePropagation::DealWithCondGoto(BB &bb, MeStmt &stmt) {
  CondGotoMeStmt &brMeStmt = static_cast<CondGotoMeStmt&>(stmt);
  const BB *brTarget = bb.GetSucc(1);
  CHECK_FATAL(brMeStmt.GetOffset() == brTarget->GetBBLabel(), "must be");
  auto *opMeExpr = static_cast<OpMeExpr*>(brMeStmt.GetOpnd());
  if (opMeExpr->GetNumOpnds() == 1) {
    return ValueRangePropagation::DealWithCondGotoWithOneOpnd(bb, brMeStmt);
  }
  if (opMeExpr->GetNumOpnds() != kNumOperands) {
    return;
  }
  if (!IsConditionalOp(opMeExpr->GetOp())) {
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
    DealWithCondGotoWhenRightRangeIsNotExist(bb, *opnd0, *opnd1, opMeExpr->GetOp());
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
  MapleMap<OStIdx, MapleSet<BBId>*> cands((std::less<OStIdx>(), valueRangeAlloc.Adapter()));
  LoopScalarAnalysisResult sa(*irMap, nullptr);
  sa.SetComputeTripCountForLoopUnroll(false);
  ValueRangePropagation valueRangePropagation(f, *irMap, *dom, meLoop, *valueRangeMemPool, cands, sa);
  valueRangePropagation.Execute();
  f.GetCfg()->UnreachCodeAnalysis(true);
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
