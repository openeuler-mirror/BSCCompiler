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
#include "me_irmap.h"
#include "me_abco.h"
#include "me_ssa_update.h"
namespace maple {
bool ValueRangePropagation::isDebug = false;
constexpr size_t kNumOperands = 2;
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
      switch (stmt.GetOp()) {
        case OP_dassign:
        case OP_maydassign: {
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
        case OP_igoto:
        default:
          break;
      }
    }
  }
  DeleteUnreachableBBs();
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

void ValueRangePropagation::DeleteUnreachableBBs() {
  if (unreachableBBs.empty()) {
    return;
  }
  isCFGChange = true;
  for (BB *bb : unreachableBBs) {
    for (auto &meStmt : bb->GetMeStmts()) {
      if (meStmt.GetVarLHS() != nullptr) {
        InsertCandsForSSAUpdate(meStmt.GetVarLHS()->GetOstIdx(), *bb);
      }
      if (meStmt.GetChiList() != nullptr) {
        for (auto &chi : *meStmt.GetChiList()) {
          auto *lhs = chi.second->GetLHS();
          const OStIdx &ostIdx = lhs->GetOstIdx();
          InsertCandsForSSAUpdate(ostIdx, *bb);
        }
      }
      if (meStmt.GetMustDefList() != nullptr) {
        for (auto &mustDefNode : *meStmt.GetMustDefList()) {
          const ScalarMeExpr *lhs = static_cast<const ScalarMeExpr*>(mustDefNode.GetLHS());
          if (lhs->GetMeOp() != kMeOpReg && lhs->GetMeOp() != kMeOpVar) {
            CHECK_FATAL(false, "unexpected opcode");
          }
          InsertCandsForSSAUpdate(lhs->GetOstIdx(), *bb);
        }
      }
    }
    bb->RemoveAllPred();
    bb->RemoveAllSucc();
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
      if (GetPrimTypeSize(primType) == 4) { // 32 bit
        return std::numeric_limits<uint32_t>::min();
      } else { // 64 bit
        CHECK_FATAL(GetPrimTypeSize(primType) == 8, "must be 64 bit");
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
      if (GetPrimTypeSize(primType) == 4) { // 32 bit
        return std::numeric_limits<uint32_t>::max();
      } else { // 64 bit
        CHECK_FATAL(GetPrimTypeSize(primType) == 8, "must be 64 bit");
        return std::numeric_limits<int64_t>::max();
      }
      break;
    case PTY_u64:
    case PTY_a64:
      return std::numeric_limits<int64_t>::max();
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
bool ValueRangePropagation::IsConstant(const BB &bb, MeExpr &expr, int64 &value) {
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
    value = valueRange->GetBound().GetConstant();
    return true;
  }
  return false;
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
void ValueRangePropagation::DealWithAddOrSub(const BB &bb, const MeExpr &lhsVar, OpMeExpr &opMeExpr) {
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
      return;
    }
    newValueRange = AddOrSubWithValueRange(opMeExpr.GetOp(), *valueRange, rhsConstant);
  } else if (lhsIsConstant && opMeExpr.GetOp() == OP_add) {
    auto *valueRange = FindValueRangeInCaches(bb.GetBBId(), opnd1->GetExprID());
    if (valueRange == nullptr) {
      return;
    }
    newValueRange = AddOrSubWithValueRange(opMeExpr.GetOp(), *valueRange, lhsConstant);
  }
  if (newValueRange != nullptr) {
    auto *valueRangePtr = newValueRange.get();
    if (Insert2Caches(bb.GetBBId(), lhsVar.GetExprID(), std::move(newValueRange))) {
      (void)Insert2Caches(bb.GetBBId(), opMeExpr.GetExprID(), CopyValueRange(*valueRangePtr));
    }
  }
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
      if (GetPrimTypeSize(primType) == 4) { // 32 bit
        return static_cast<uint32>(value);
      } else { // 64 bit
        CHECK_FATAL(GetPrimTypeSize(primType) == 8, "must be 64 bit");
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
      if (primType == PTY_begin) {
        return std::make_unique<ValueRange>(valueRange.GetBound(), kEqual);
      } else {
        Bound bound = Bound(valueRange.GetBound().GetVar(),
                            GetRealValue(valueRange.GetBound().GetConstant(), primType), primType);
        return std::make_unique<ValueRange>(bound, kEqual);
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

    case kNotEqual:
      CHECK_FATAL(false, "can not be here");
      break;
  }
}

// Create new value range when deal with assign.
void ValueRangePropagation::DealWithAssign(const BB &bb, const MeStmt &stmt) {
  auto *lhs = stmt.GetLHS();
  auto *rhs = stmt.GetRHS();
  auto *existValueRange = FindValueRangeInCaches(bb.GetBBId(), rhs->GetExprID());
  if (existValueRange != nullptr) {
    (void)Insert2Caches(bb.GetBBId(), lhs->GetExprID(), CopyValueRange(*existValueRange));
    return;
  }
  if (rhs->GetMeOp() == kMeOpOp) {
    auto *opMeExpr = static_cast<OpMeExpr*>(rhs);
    switch (rhs->GetOp()) {
      case OP_add:
      case OP_sub: {
        DealWithAddOrSub(bb, *lhs, *opMeExpr);
        break;
      }
      case OP_gcmallocjarray: {
        DealWithArrayLength(bb, *lhs, *opMeExpr->GetOpnd(0));
        break;
      }
      default:
        break;
    }
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
  return kCanNotCompute;
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
  if (lengthSet.find(&opnd1) != lengthSet.end()) {
    upperBound = Bound(&opnd1, GetRealValue(rightConstant, opnd1.GetPrimType()), opnd1.GetPrimType());
    return std::make_unique<ValueRange>(initBound, upperBound, kLowerAndUpper);
  } else if (IsConstant(bb, opnd1, constantValue)) {
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
    return std::make_unique<ValueRange>(initBound, upperBound, kLowerAndUpper);
  } else {
    return std::make_unique<ValueRange>(initBound, Bound(&opnd1, GetRealValue(
        rightConstant, initBound.GetPrimType()), initBound.GetPrimType()), kSpecialUpperForLoop);
  }
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
  if (lengthSet.find(&opnd1) != lengthSet.end()) {
    lowerBound = Bound(&opnd1, GetRealValue(rightConstant, opnd1.GetPrimType()), opnd1.GetPrimType());
    return std::make_unique<ValueRange>(lowerBound, initBound, kLowerAndUpper);
  } else if (IsConstant(bb, opnd1, constantValue)) {
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
    return std::make_unique<ValueRange>(lowerBound, initBound, kLowerAndUpper);
  } else {
    return std::make_unique<ValueRange>(Bound(&opnd1, GetRealValue(rightConstant, opnd1.GetPrimType()),
                                              opnd1.GetPrimType()), initBound, kSpecialLowerForLoop);
  }
}

// Create new value range when deal with phinode.
std::unique_ptr<ValueRange> ValueRangePropagation::CreateValueRangeForPhi(LoopDesc &loop,
    BB &bb, ScalarMeExpr &init, ScalarMeExpr &backedge, ScalarMeExpr &lhsOfPhi) {
  Bound initBound;
  ValueRange *valueRangeOfInit = FindValueRangeInCaches(bb.GetBBId(), init.GetExprID());
  if (valueRangeOfInit != nullptr && valueRangeOfInit->IsConstant()) {
    initBound = Bound(GetRealValue(valueRangeOfInit->GetBound().GetConstant(),
        valueRangeOfInit->GetBound().GetPrimType()), valueRangeOfInit->GetBound().GetPrimType());
  } else if (lengthSet.find(&init) != lengthSet.end()) {
    initBound = Bound(&init, init.GetPrimType());
  } else {
    return nullptr;
  }
  int stride = 0;
  if (!CanComputeLoopIndVar(lhsOfPhi, backedge, stride)) {
    return nullptr;
  }
  for (auto &it : loop.inloopBB2exitBBs) {
    auto *exitBB = func.GetCfg()->GetBBFromID(it.first);
    if (exitBB->GetKind() != kBBCondGoto) {
      continue;
    }
    const BB *trueBranch = nullptr;
    const BB *falseBranch = nullptr;
    auto *brMeStmt = static_cast<CondGotoMeStmt*>(exitBB->GetLastMe());
    if (brMeStmt->GetOp() == OP_brtrue) {
      trueBranch = exitBB->GetSucc(1);
      falseBranch = exitBB->GetSucc(0);
    } else {
      trueBranch = exitBB->GetSucc(0);
      falseBranch = exitBB->GetSucc(1);
    }
    auto *opMeExpr = static_cast<OpMeExpr*>(brMeStmt->GetOpnd());
    MeExpr *opnd0 = opMeExpr->GetOpnd(0);
    MeExpr *opnd1 = opMeExpr->GetOpnd(1);
    if (opnd0 == &backedge || (loop.head == exitBB && opnd0 == &lhsOfPhi)) {
      if (initBound.GetVar() != nullptr && lengthSet.find(initBound.GetVar()) == lengthSet.end()) {
        auto *initRange = FindValueRangeInCaches(bb.GetPred(0)->GetBBId(), initBound.GetVar()->GetExprID());
        if (initRange != nullptr) {
          initBound = (stride > 0) ? Bound(initRange->GetLower()) : Bound(initRange->GetUpper());
        }
      }
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
  return nullptr;
}

void ValueRangePropagation::DealWithPhi(BB &bb, MePhiNode &mePhiNode) {
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
  newBB->SetKindReturn();
  newBB->SetAttributes(kBBAttrArtificial);
  bb.AddSucc(*newBB);
  func.GetCfg()->GetCommonExitBB()->AddExit(*newBB);
  for (auto id : loop.loopBBs) {
    auto *bb = func.GetCfg()->GetBBFromID(id);
    bb->SetAttributes(kBBAttrWontExit);
  }
}

// when determine remove the condgoto stmt, need analysis which bbs need be deleted.
void ValueRangePropagation::AnalysisUnreachableBBOrEdge(BB &bb, BB &unreachableBB, BB &succBB) {
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
bool ValueRangePropagation::BrStmtInRange(BB &bb, ValueRange &leftRange, ValueRange &rightRange, Opcode op) const {
  if (leftRange.GetLower().GetVar() != leftRange.GetUpper().GetVar() ||
      leftRange.GetUpper().GetVar() != rightRange.GetUpper().GetVar() ||
      rightRange.GetUpper().GetVar() != rightRange.GetLower().GetVar()) {
    return false;
  }
  if (leftRange.GetLower().GetVar() != nullptr) {
    auto *valueRange = FindValueRangeInCaches(bb.GetBBId(), leftRange.GetLower().GetVar()->GetExprID());
    if (valueRange == nullptr || !valueRange->IsConstant()) {
      return false;
    }
  }
  if (leftRange.GetLower().GetConstant() > leftRange.GetUpper().GetConstant()) {
    return false;
  }
  if (rightRange.GetLower().GetConstant() > rightRange.GetUpper().GetConstant()) {
    return false;
  }
  if (op == OP_ge) {
    return leftRange.GetLower().GetConstant() >= rightRange.GetUpper().GetConstant();
  } else if (op == OP_gt) {
    return leftRange.GetLower().GetConstant() > rightRange.GetUpper().GetConstant();
  } else if (op == OP_lt) {
    return leftRange.GetUpper().GetConstant() < rightRange.GetLower().GetConstant();
  } else if (op == OP_le) {
    return leftRange.GetUpper().GetConstant() <= rightRange.GetLower().GetConstant();
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

void ValueRangePropagation::CreateValueRangeForNeOrEq(
    MeExpr &opnd, ValueRange &rightRange, BB &trueBranch) {
  if (rightRange.GetRangeType() != kEqual) {
    return;
  }
  std::unique_ptr<ValueRange> newTrueBranchRange =
      std::make_unique<ValueRange>(rightRange.GetBound(), kEqual);
  (void)Insert2Caches(trueBranch.GetBBId(), opnd.GetExprID(), std::move(newTrueBranchRange));
}

void ValueRangePropagation::DealWithOPNeOrEq(
    Opcode op, BB &bb, ValueRange *leftRange, ValueRange &rightRange, const CondGotoMeStmt &brMeStmt) {
  BB *trueBranch = nullptr;
  BB *falseBranch = nullptr;
  GetTrueAndFalseBranch(brMeStmt.GetOp(), bb, trueBranch, falseBranch);
  MeExpr *opnd0 = static_cast<OpMeExpr*>(brMeStmt.GetOpnd())->GetOpnd(0);
  if (leftRange == nullptr) {
    if (op == OP_eq) {
      CreateValueRangeForNeOrEq(*opnd0, rightRange, *trueBranch);
    } else if (op == OP_ne) {
      CreateValueRangeForNeOrEq(*opnd0, rightRange, *falseBranch);
    }
  } else {
    if (op == OP_eq) {
      if (leftRange->GetRangeType() == kEqual && rightRange.GetRangeType() == kEqual &&
          leftRange->GetBound().GetVar() == rightRange.GetBound().GetVar()) {
        if (leftRange->GetBound().GetConstant() == rightRange.GetBound().GetConstant()) {
          AnalysisUnreachableBBOrEdge(bb, *falseBranch, *trueBranch);
        } else {
          AnalysisUnreachableBBOrEdge(bb, *trueBranch, *falseBranch);
        }
      } else {
        CreateValueRangeForNeOrEq(*opnd0, rightRange, *trueBranch);
      }
    } else {
      if (leftRange->GetRangeType() == kEqual && rightRange.GetRangeType() == kEqual &&
          leftRange->GetBound().GetVar() == rightRange.GetBound().GetVar()) {
        if (leftRange->GetBound().GetConstant() == rightRange.GetBound().GetConstant()) {
          AnalysisUnreachableBBOrEdge(bb, *trueBranch, *falseBranch);
        } else {
          AnalysisUnreachableBBOrEdge(bb, *falseBranch, *trueBranch);
        }
      } else {
        CreateValueRangeForNeOrEq(*opnd0, rightRange, *falseBranch);
      }
    }
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
    BB &bb, MeExpr &opMeExpr, ValueRange *leftRange, ValueRange &rightRange, const CondGotoMeStmt &brMeStmt) {
  auto newRightUpper = rightRange.GetUpper();
  auto newRightLower = rightRange.GetLower();
  CHECK_FATAL(IsEqualPrimType(newRightUpper.GetPrimType(), newRightLower.GetPrimType()), "must be equal");
  if (opMeExpr.GetOp() == OP_ne || opMeExpr.GetOp() == OP_eq) {
    DealWithOPNeOrEq(opMeExpr.GetOp(), bb, leftRange, rightRange, brMeStmt);
    return;
  }
  Opcode antiOp = GetTheOppositeOp(opMeExpr.GetOp());
  if (leftRange != nullptr && leftRange->GetRangeType() != kSpecialLowerForLoop &&
      leftRange->GetRangeType() != kSpecialUpperForLoop) {
    BB *trueBranch = nullptr;
    BB *falseBranch = nullptr;
    GetTrueAndFalseBranch(brMeStmt.GetOp(), bb, trueBranch, falseBranch);
    if (BrStmtInRange(bb, *leftRange, rightRange, opMeExpr.GetOp())) {
      AnalysisUnreachableBBOrEdge(bb, *falseBranch, *trueBranch);
      return;
    } else if (BrStmtInRange(bb, *leftRange, rightRange, antiOp)) {
      AnalysisUnreachableBBOrEdge(bb, *trueBranch, *falseBranch);
      return;
    }
  }
  if ((opMeExpr.GetOp() == OP_lt) ||
      (opMeExpr.GetOp() == OP_ge)) {
    int64 constant = 0;
    if (!AddOrSubWithConstant(newRightUpper.GetPrimType(), OP_add, newRightUpper.GetConstant(), -1, constant)) {
      return;
    }
    newRightUpper = Bound(newRightUpper.GetVar(),
        GetRealValue(constant, newRightUpper.GetPrimType()), newRightUpper.GetPrimType());
  }
  if ((opMeExpr.GetOp() == OP_le) ||
      (opMeExpr.GetOp() == OP_gt)) {
    int64 constant = 0;
    if (!AddOrSubWithConstant(newRightUpper.GetPrimType(), OP_add, newRightLower.GetConstant(), 1, constant)) {
      return;
    }
    newRightLower = Bound(newRightLower.GetVar(),
                          GetRealValue(constant, newRightUpper.GetPrimType()), newRightUpper.GetPrimType());
  }
  if (opMeExpr.GetOp() == OP_lt || opMeExpr.GetOp() == OP_le) {
    DealWithOPLeOrLt(bb, leftRange, newRightUpper, newRightLower, brMeStmt);
  } else if (opMeExpr.GetOp() == OP_gt || opMeExpr.GetOp() == OP_ge) {
    DealWithOPGeOrGt(bb, leftRange, newRightUpper, newRightLower, brMeStmt);
  }
}

bool ValueRangePropagation::GetValueRangeOfCondGotoOpnd(BB &bb, OpMeExpr &opMeExpr, MeExpr &opnd,
    ValueRange *&valueRange, std::unique_ptr<ValueRange> &rightRangePtr) {
  valueRange = FindValueRangeInCaches(bb.GetBBId(), opnd.GetExprID());
  if (valueRange == nullptr) {
    if (opnd.GetMeOp() == kMeOpConst && static_cast<ConstMeExpr&>(opnd).GetConstVal()->GetKind() == kConstInt) {
      std::unique_ptr<ValueRange> valueRangeTemp = std::make_unique<ValueRange>(Bound(GetRealValue(
          static_cast<ConstMeExpr&>(opnd).GetIntValue(), opnd.GetPrimType()), opnd.GetPrimType()), kEqual);
      valueRange = valueRangeTemp.get();
      if (!Insert2Caches(bb.GetBBId(), opnd.GetExprID(), std::move(valueRangeTemp))) {
        valueRange = nullptr;
        return false;
      }
    }
  }
  if (valueRange != nullptr && valueRange->GetLower().GetPrimType() != opMeExpr.GetOpndType()) {
    rightRangePtr = CopyValueRange(*valueRange, opMeExpr.GetOpndType());
    valueRange = rightRangePtr.get();
    if (IsBiggerThanMaxInt64(*valueRange)) {
      valueRange = nullptr;
      return false;
    }
  }
  return true;
}

void ValueRangePropagation::DealWithCondGoto(BB &bb, MeStmt &stmt) {
  CondGotoMeStmt &brMeStmt = static_cast<CondGotoMeStmt&>(stmt);
  const BB *brTarget = bb.GetSucc(1);
  CHECK_FATAL(brMeStmt.GetOffset() == brTarget->GetBBLabel(), "must be");
  auto *opMeExpr = static_cast<OpMeExpr*>(brMeStmt.GetOpnd());
  if (opMeExpr->GetNumOpnds() != kNumOperands) {
    return;
  }
  if (opMeExpr->GetOp() != OP_le && opMeExpr->GetOp() != OP_lt &&
      opMeExpr->GetOp() != OP_ge && opMeExpr->GetOp() != OP_gt &&
      opMeExpr->GetOp() != OP_ne && opMeExpr->GetOp() != OP_eq) {
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
  if (!GetValueRangeOfCondGotoOpnd(bb, *opMeExpr, *opnd1, rightRange, rightRangePtr) || rightRange == nullptr) {
    return;
  }
  if (!GetValueRangeOfCondGotoOpnd(bb, *opMeExpr, *opnd0, leftRange, leftRangePtr)) {
    return;
  }

  if (leftRange == nullptr && rightRange->GetRangeType() != kEqual) {
    return;
  }
  if (leftRange != nullptr && leftRange->GetRangeType() == kOnlyHasLowerBound &&
      rightRange->GetRangeType() == kOnlyHasUpperBound) { // deal with special case
  }
  DealWithCondGoto(bb, *opMeExpr, leftRange, *rightRange, brMeStmt);
}

AnalysisResult *MeDoValueRangePropagation::Run(MeFunction *func, MeFuncResultMgr *frm, ModuleResultMgr*) {
  CHECK_FATAL(frm != nullptr, "frm is nullptr");
  auto *dom = static_cast<Dominance*>(frm->GetAnalysisResult(MeFuncPhase_DOMINANCE, func, !MeOption::quiet));
  CHECK_FATAL(dom != nullptr, "dominance phase has problem");
  auto *irMap = static_cast<MeIRMap*>(frm->GetAnalysisResult(MeFuncPhase_IRMAPBUILD, func, !MeOption::quiet));
  CHECK_FATAL(irMap != nullptr, "irMap phase has problem");
  frm->InvalidAnalysisResult(MeFuncPhase_MELOOP, func);
  IdentifyLoops *meLoop = static_cast<IdentifyLoops*>(frm->GetAnalysisResult(MeFuncPhase_MELOOP, func, !MeOption::quiet));
  if (ValueRangePropagation::isDebug) {
    LogInfo::MapleLogger() << func->GetName() << "\n";
    func->Dump(false);
    func->GetCfg()->DumpToFile("valuerange-before");
  }
  auto *valueRangeMemPool = NewMemPool();
  MapleAllocator valueRangeAlloc = MapleAllocator(valueRangeMemPool);
  MapleMap<OStIdx, MapleSet<BBId>*> cands((std::less<OStIdx>(), valueRangeAlloc.Adapter()));
  ValueRangePropagation valueRangePropagation(*func, *irMap, *dom, meLoop, *valueRangeMemPool, cands);
  valueRangePropagation.Execute();
  if (valueRangePropagation.IsCFGChange()) {
    if (ValueRangePropagation::isDebug) {
      func->GetCfg()->DumpToFile("valuerange-after");
    }
    frm->InvalidAnalysisResult(MeFuncPhase_DOMINANCE, func);
    auto dom = static_cast<Dominance*>(frm->GetAnalysisResult(MeFuncPhase_DOMINANCE, func));
    if (valueRangePropagation.NeedUpdateSSA()) {
      MeSSAUpdate ssaUpdate(*func, *func->GetMeSSATab(), *dom, cands, *valueRangeMemPool);
      ssaUpdate.Run();
    }
    frm->InvalidAnalysisResult(MeFuncPhase_MELOOP, func);
  }
  if (ValueRangePropagation::isDebug) {
    LogInfo::MapleLogger() << "***************after value range prop***************" << "\n";
    func->Dump(false);
    func->GetCfg()->DumpToFile("valuerange-after");
  }
  if (DEBUGFUNC(func)) {
    LogInfo::MapleLogger() << "\n============== After boundary check optimization  =============" << "\n";
    irMap->Dump();
  }
  return nullptr;
}
}  // namespace maple
