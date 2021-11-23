/*
 * Copyright (c) [2021] Futurewei Technologies Co., Ltd. All rights reserved.
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
#ifndef MPL2MPL_INCLUDE_EXTCONSTANTFOLD_H
#define MPL2MPL_INCLUDE_EXTCONSTANTFOLD_H
#include "mir_nodes.h"
#include "phase_impl.h"

namespace maple {
class ExtConstantFold {
 public:
  explicit ExtConstantFold(MIRModule *mod) : mirModule(mod) {}
  StmtNode *ExtSimplify(StmtNode *node);
  BaseNode *Fold(BaseNode *node);
  BaseNode *FoldIor(BinaryNode *node);
  StmtNode *ExtSimplifyBlock(BlockNode *node);
  StmtNode *ExtSimplifyIf(IfStmtNode *node);
  StmtNode *ExtSimplifyDassign(DassignNode *node);
  StmtNode *ExtSimplifyIassign(IassignNode *node);
  StmtNode *ExtSimplifyWhile(WhileStmtNode *node);
  BaseNode* DispatchFold(BaseNode *node);

  MIRModule *mirModule;
};
}  // namespace maple
#endif  // MPL2MPL_INCLUDE_EXTCONSTANTFOLD_H
