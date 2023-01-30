/*
 * Copyright (c) [2022] Futurewei Technologies Co., Ltd. All rights reserved.
 *
 * OpenArkCompiler is licensed under the Mulan Permissive Software License v2.
 * You can use this software according to the terms and conditions of the MulanPSL - 2.0.
 * You may obtain a copy of MulanPSL - 2.0 at:
 *
 *   https://opensource.org/licenses/MulanPSL-2.0
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR
 * FIT FOR A PARTICULAR PURPOSE.
 * See the MulanPSL - 2.0 for more details.
 */

#ifndef MAPLEME_INCLUDE_LMBC_LOWER_H_
#define MAPLEME_INCLUDE_LMBC_LOWER_H_
#include "mir_builder.h"
#include "lmbc_memlayout.h"

namespace maple {

class LMBCLowerer {
 public:
  explicit LMBCLowerer(MIRModule *mod, maplebe::BECommon *becmmn, MIRFunction *f,
                       GlobalMemLayout *gmemlayout, LMBCMemLayout *lmemlayout)
      : mirModule(mod), func(f), becommon(becmmn), mirBuilder(mod->GetMIRBuilder()),
        globmemlayout(gmemlayout), memlayout(lmemlayout) {}

  PregIdx GetSpecialRegFromSt(const MIRSymbol *);
  BaseNode *LowerAddrof(AddrofNode *);
  BaseNode *LowerDread(const AddrofNode *);
  BaseNode *LowerDreadoff(DreadoffNode *);
  BaseNode *LowerIread(IreadNode *);
  BaseNode *LowerIaddrof(IreadNode *);
  BaseNode *LowerExpr(BaseNode *expr);
  void LowerAggDassign(const DassignNode *, MIRType *lhsty, int32 offset, BlockNode *);
  void LowerDassign(DassignNode *, BlockNode *);
  void LowerDassignoff(DassignoffNode *, BlockNode *);
  void LowerIassign(IassignNode *, BlockNode *);
  void LowerAggIassign(IassignNode *, MIRType *type, int32 offset, BlockNode *);
  void LowerReturn(NaryStmtNode &retNode, BlockNode *newblk);
  void LowerCall(NaryStmtNode *stmt, BlockNode *newblk);
  BlockNode *LowerBlock(BlockNode *);
  void FixPrototype4FirstArgReturn(IcallNode *icall);
  void LowerFunction();

  MIRModule *mirModule;
  MIRFunction *func;
  maplebe::BECommon *becommon;
  MIRBuilder *mirBuilder;
  GlobalMemLayout *globmemlayout;
  LMBCMemLayout *memlayout;
};

}  // namespace maple

#endif  // MAPLEME_INCLUDE_LMBC_LOWER_H_
