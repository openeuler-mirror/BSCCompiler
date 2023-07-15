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
#ifndef MAPLE_ME_INCLUDE_ME_COMBINEEXPR_H
#define MAPLE_ME_INCLUDE_ME_COMBINEEXPR_H

#include "bit_value.h"
#include "me_dominance.h"
#include "me_function.h"
#include "me_ir.h"
#include "me_irmap.h"
#include "meexpr_use_info.h"

namespace maple {
class ExprCombiner {
 public:
  ExprCombiner(MeIRMap &argIRMap, MeExprUseInfo &argUseInfo, bool isDebug)
      : irMap(argIRMap), useInfo(argUseInfo), debug(isDebug) {}
  void VisitStmt(MeStmt &stmt);

 private:
  MeExpr *VisitMeExpr(MeExpr &expr);
  MeExpr *VisitOpExpr(OpMeExpr &expr);
  MeExpr *VisitBior(OpMeExpr &expr);
  MeExpr *VisitBxor(OpMeExpr &expr);
  MeExpr *VisitBand(OpMeExpr &expr);
  MeExpr *VisitCmp(OpMeExpr &expr);
  MeExpr *SimplifyOpWithConst(OpMeExpr &expr);
  MeExpr *FoldCmpWithConst(OpMeExpr &expr);
  MeExpr *FoldCmpBinOpWithConst(OpMeExpr &cmpExpr, const MeExpr &binOpExpr, const IntVal &c1);
  MeExpr *SimplifyByDemandedBits(OpMeExpr &expr);
  MeExpr *SimplifyDemandedUseBits(MeExpr &expr, const IntVal &demanded, BitValue &known, uint32 depth);
  MeExpr *SimplifyIntrinsicByDUB(const MeExpr &expr, const IntVal &demanded);
  bool HasOneUse(const MeExpr &expr);
  void ReplaceExprInExpr(MeExpr &parentExpr, MeExpr &newExpr, size_t opndIdx);
  void ReplaceExprInStmt(MeStmt &parentStmt, MeExpr &newExpr, size_t opndIdx);
  MeIRMap &irMap;
  MeExprUseInfo &useInfo;
  bool debug = false;
};

class CombineExpr {
 public:
  CombineExpr(MeFunction &meFunc, MeIRMap &argIRMap, Dominance &argDom, bool isDebug)
      : func(meFunc),
        irMap(argIRMap),
        dom(argDom),
        debug(isDebug) {}
  ~CombineExpr() = default;
  void Execute();

 private:
  MeFunction &func;
  MeIRMap &irMap;
  Dominance &dom;
  bool debug;
};

MAPLE_FUNC_PHASE_DECLARE(MECombineExpr, MeFunction)
}  // namespace maple
#endif