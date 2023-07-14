/*
 * Copyright (c) [2023] Huawei Technologies Co.,Ltd.All rights reserved.
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
#include "me_expr_utils.h"

#include "irmap.h"
namespace maple {

const MIRIntConst *GetIntConst(const MeExpr &expr) {
  if (expr.GetMeOp() != kMeOpConst) {
    return nullptr;
  }
  auto cval = static_cast<const ConstMeExpr &>(expr).GetConstVal();
  if (cval->GetKind() != kConstInt) {
    return nullptr;
  }
  return static_cast<const MIRIntConst *>(cval);
}

std::pair<const MIRIntConst *, uint8_t> GetIntConstOpndOfBinExpr(const MeExpr &expr) {
  if (auto res = GetIntConst(*expr.GetOpnd(1))) {
    return {res, 1};
  }
  if (auto res = GetIntConst(*expr.GetOpnd(0))) {
    return {res, 0};
  }
  return {nullptr, 0};
}

MeExpr *SimplifyBinOpByDemanded(IRMap &irMap, const MeExpr &expr, const IntVal &demanded, BitValue &known,
                                const BitValue &lhsKnown, const BitValue &rhsKnown) {
  ASSERT(!lhsKnown.HasConflict() && !rhsKnown.HasConflict(), "BitValue has conflict");
  auto op = expr.GetOp();
  auto type = expr.GetPrimType();
  switch (op) {
    case OP_band: {
      known = lhsKnown & rhsKnown;

      // all demanded bits are known as 0 or 1
      if (demanded.IsSubsetOf(known.zeroBits | known.oneBits)) {
        return irMap.CreateIntConstMeExpr(known.oneBits, type);
      }

      // all demanded bits are known as 1 on rhs, return lhs
      if (demanded.IsSubsetOf(lhsKnown.zeroBits | rhsKnown.oneBits)) {
        return expr.GetOpnd(0);
      }
      // all demanded bits are known as 1 on lhs, return rhs
      if (demanded.IsSubsetOf(rhsKnown.zeroBits | lhsKnown.oneBits)) {
        return expr.GetOpnd(1);
      }
      break;
    }
    case OP_bior: {
      known = lhsKnown | rhsKnown;

      // all demanded bits are known as 0 or 1
      if (demanded.IsSubsetOf(known.zeroBits | known.oneBits)) {
        return irMap.CreateIntConstMeExpr(known.oneBits, type);
      }

      // all demanded bits are known as 1 on lhs, return lhs
      if (demanded.IsSubsetOf(lhsKnown.oneBits | rhsKnown.zeroBits)) {
        return expr.GetOpnd(0);
      }
      // all demanded bits are known as 1 on rhs, return rhs
      if (demanded.IsSubsetOf(rhsKnown.oneBits | lhsKnown.zeroBits)) {
        return expr.GetOpnd(1);
      }
      break;
    }
    case OP_bxor: {
      known = lhsKnown ^ rhsKnown;

      // all demanded bits are known as 0 or 1
      if (demanded.IsSubsetOf(known.zeroBits | known.oneBits)) {
        return irMap.CreateIntConstMeExpr(known.oneBits, type);
      }

      // all demanded bits are known as 0 on rhs(means that bits are decided by lhs), return lhs
      if (demanded.IsSubsetOf(rhsKnown.zeroBits)) {
        return expr.GetOpnd(0);
      }
      // all demanded bits are known as 1 on lhs(means that bits are decided by rhs), return rhs
      if (demanded.IsSubsetOf(lhsKnown.zeroBits)) {
        return expr.GetOpnd(1);
      }
      break;
    }
    default:
      break;
  }
  return nullptr;
}

MeExpr *SimplifyMultiUseByDemanded(IRMap &irMap, const MeExpr &expr, const IntVal &demanded, BitValue &known,
                                   uint32 depth) {
  auto bitWidth = demanded.GetBitWidth();
  BitValue lhsKnown(bitWidth);
  BitValue rhsKnown(bitWidth);
  auto op = expr.GetOp();

  switch (op) {
    case OP_band:
    case OP_bior:
    case OP_bxor: {
      ComputeBitValueOfExpr(*expr.GetOpnd(0), demanded, lhsKnown, depth + 1);
      ComputeBitValueOfExpr(*expr.GetOpnd(1), demanded, rhsKnown, depth + 1);
      if (auto res = SimplifyBinOpByDemanded(irMap, expr, demanded, known, lhsKnown, rhsKnown)) {
        return res;
      }
      break;
    }
    default:
      break;
  }
  return nullptr;
}
MeExpr *SimplifyMultiUseByDemanded(IRMap &irMap, const MeExpr &expr) {
  auto bitWidth = GetPrimTypeBitSize(expr.GetPrimType());
  IntVal demanded(IntVal::kAllOnes, bitWidth, false);
  BitValue known(bitWidth);
  return SimplifyMultiUseByDemanded(irMap, expr, demanded, known, 0);
}
}  // namespace maple