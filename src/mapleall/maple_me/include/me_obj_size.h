/*
 * Copyright (c) [2022] Huawei Technologies Co.,Ltd.All rights reserved.
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
#ifndef MAPLE_ME_INCLUDE_ME_OBJ_SIZE_H
#define MAPLE_ME_INCLUDE_ME_OBJ_SIZE_H

#include "me_ir.h"
#include "me_function.h"
#include "meexpr_use_info.h"

namespace maple {
enum ObjSizeType : std::uint8_t {
  kTypeZero,
  kTypeOne,
  kTypeTwo,
  kTypeThree
};

class OBJSize {
 public:
  OBJSize(MeFunction &meFunc, MeIRMap &map) : func(meFunc), irMap(map) {}
  ~OBJSize() = default;
  void Execute();

 private:
  size_t InvalidSize(bool getMaxSizeOfObjs) const {
    return getMaxSizeOfObjs ? 0 : -1;
  }

  size_t IsInvalidSize(size_t size) const {
    return size == static_cast<uint64>(-1);
  }

  bool IsConstIntExpr(const MeExpr &expr) const {
    return expr.GetOp() == OP_constval && static_cast<const ConstMeExpr&>(expr).GetConstVal()->GetKind() == kConstInt;
  }

  MeExpr *GetStrMeExpr(MeExpr &expr);
  void ComputeObjectSize(MeStmt &meStmt);
  void ERRWhenSizeTypeIsInvalid(const MeStmt &meStmt) const;
  void DealWithBuiltinObjectSize(BB &bb, MeStmt &meStmt);
  size_t ComputeObjectSizeWithType(MeExpr &opnd, int64 type) const;
  void ReplaceStmt(CallMeStmt &callMeStmt, const std::string &str);
  size_t DealWithSprintfAndVsprintf(const CallMeStmt &callMeStmt, const MIRFunction &calleeFunc);
  size_t DealWithArray(const MeExpr &opnd, int64 type) const;
  size_t DealWithAddrof(const MeExpr &opnd, bool getSizeOfWholeVar) const;
  size_t DealWithDread(const MeExpr &opnd, int64 type, bool getMaxSizeOfObjs) const;
  size_t DealWithIaddrof(const MeExpr &opnd, int64 type, bool getSizeOfWholeVar) const;

  MeFunction &func;
  MeIRMap &irMap;
};

MAPLE_FUNC_PHASE_DECLARE(MEOBJSize, MeFunction)
}  // namespace maple
#endif