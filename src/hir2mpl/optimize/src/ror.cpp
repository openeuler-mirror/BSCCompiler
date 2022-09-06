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
#include "ror.h"
#include "feir_builder.h"

namespace maple {
UniqueFEIRExpr Ror::Emit2FEExpr() {
  if (op != OP_bior || !CheckBaseExpr()) {
    return nullptr;
  }

  auto lBinExpr = static_cast<FEIRExprBinary*>(lExpr.get());
  auto rBinExpr = static_cast<FEIRExprBinary*>(rExpr.get());
  bitWidth = GetPrimTypeBitSize(lBinExpr->GetOpnd0()->GetPrimType());

  if (lBinExpr->GetOp() == OP_shl && rBinExpr->GetOp() == OP_lshr) {
    if (!IsRorLshrOpnd(rBinExpr->GetOpnd1(), false) || !IsRorShlOpnd(lBinExpr->GetOpnd1())) {
      return nullptr;
    }
  } else if (lBinExpr->GetOp() == OP_lshr && rBinExpr->GetOp() == OP_shl) {
    if (!IsRorLshrOpnd(lBinExpr->GetOpnd1(), false) || !IsRorShlOpnd(rBinExpr->GetOpnd1())) {
      return nullptr;
    }
  }

  if (*rShiftBaseExpr != *lShiftBaseExpr) {
    return nullptr;
  }

  return FEIRBuilder::CreateExprBinary(OP_ror, lBinExpr->GetOpnd0()->Clone(), rShiftBaseExpr->Clone());
}

bool Ror::CheckBaseExpr() const {
  if (lExpr->GetKind() != kExprBinary || rExpr->GetKind() != kExprBinary) {
    return false;
  }

  auto lBinExpr = static_cast<FEIRExprBinary*>(lExpr.get());
  auto rBinExpr = static_cast<FEIRExprBinary*>(rExpr.get());
  if (!((lBinExpr->GetOp() == OP_shl && rBinExpr->GetOp() == OP_lshr) ||
      (lBinExpr->GetOp() == OP_lshr && rBinExpr->GetOp() == OP_shl))) {
    return false;
  }

  if (*(lBinExpr->GetOpnd0()) != *(rBinExpr->GetOpnd0()) ||
      !IsUnsignedInteger(lBinExpr->GetOpnd0()->GetPrimType())) {
    return false;
  }

  return true;
}

bool Ror::GetConstVal(const UniqueFEIRExpr &expr) {
  if (expr->GetKind() != kExprConst && expr->GetKind() != kExprUnary) {
    return false;
  }

  FEIRExprConst *constExpr;
  if (expr->GetKind() == kExprUnary) {
    UniqueFEIRExpr opndExpr = static_cast<FEIRExprUnary*>(expr.get())->GetOpnd()->Clone();
    if (opndExpr->GetKind() != kExprConst) {
      return false;
    }
    constExpr = static_cast<FEIRExprConst*>(opndExpr.get());
  } else {
    constExpr = static_cast<FEIRExprConst*>(expr.get());
  }

  constVal = constExpr->GetValue().u64;

  return true;
}

bool Ror::IsRorShlOpnd(const UniqueFEIRExpr &expr) {
  if (expr->GetKind() != kExprBinary) {
    return false;
  }

  auto binExpr = static_cast<FEIRExprBinary*>(expr.get());
  if (binExpr->GetOp() != OP_sub || !GetConstVal(binExpr->GetOpnd0()) || constVal != bitWidth) {
    return false;
  }

  if (!IsRorLshrOpnd(binExpr->GetOpnd1(), true)) {
    return false;
  }

  return true;
}

bool Ror::IsRorLshrOpnd(const UniqueFEIRExpr &expr, bool inShl) {
  if (expr->GetKind() != kExprBinary) {
    return false;
  }

  auto binExpr = static_cast<FEIRExprBinary*>(expr.get());
  if (binExpr->GetOp() != OP_band) {
    return false;
  }

  auto SetShiftBaseExpr = [inShl, this](const UniqueFEIRExpr &expr) {
    if (inShl) {
      lShiftBaseExpr = expr->Clone();
    } else {
      rShiftBaseExpr = expr->Clone();
    }
  };

  if (GetConstVal(binExpr->GetOpnd0())) {
    SetShiftBaseExpr(binExpr->GetOpnd1());
  } else if (GetConstVal(binExpr->GetOpnd1())) {
    SetShiftBaseExpr(binExpr->GetOpnd0());
  } else {
    return false;
  }

  if (constVal != bitWidth - 1) {
    return false;
  }

  return true;
}
}
