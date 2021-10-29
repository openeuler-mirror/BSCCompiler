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
#ifndef OPTIMIZE_INCLUDE_ROR_H
#define OPTIMIZE_INCLUDE_ROR_H
#include "feir_stmt.h"
#include "opcodes.h"

namespace maple {
class Ror {
 public:
  Ror(Opcode opIn, const UniqueFEIRExpr &left, const UniqueFEIRExpr &right)
      : op(opIn), lExpr(left), rExpr(right) {}
  ~Ror() = default;
  UniqueFEIRExpr Emit2FEExpr();

 private:
  bool CheckBaseExpr() const;
  bool GetConstVal(const UniqueFEIRExpr &expr);
  bool IsRorShlOpnd(const UniqueFEIRExpr &expr);
  bool IsRorLshrOpnd(const UniqueFEIRExpr &expr, bool inShl);

  Opcode op;
  const UniqueFEIRExpr &lExpr;
  const UniqueFEIRExpr &rExpr;
  UniqueFEIRExpr rShiftBaseExpr;
  UniqueFEIRExpr lShiftBaseExpr;
  uint32 constVal = 0;
  uint32 bitWidth = 0;
};
} // namespace maple
#endif // OPTIMIZE_INCLUDE_ROR_H