/*
 * Copyright (c) [2019-2021] Huawei Technologies Co.,Ltd.All rights reserved.
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
#ifndef MAPLE_IR_INCLUDE_JAVA_EH_LOWER_H
#define MAPLE_IR_INCLUDE_JAVA_EH_LOWER_H
#include "phase_impl.h"
#include "class_hierarchy.h"
#include "maple_phase_manager.h"

namespace maple {
class JavaEHLowerer : public FuncOptimizeImpl {
 public:
  JavaEHLowerer(MIRModule &mod, KlassHierarchy *kh, bool dump) : FuncOptimizeImpl(mod, kh, dump) {}
  ~JavaEHLowerer() = default;

  FuncOptimizeImpl *Clone() override {
    return new JavaEHLowerer(*this);
  }

  void ProcessFunc(MIRFunction *func) override;

 private:
  BlockNode *DoLowerBlock(BlockNode &block);
  BaseNode *DoLowerExpr(BaseNode &expr, BlockNode &curblk);
  BaseNode *DoLowerDiv(BinaryNode &expr, BlockNode &blknode);
  void DoLowerBoundaryCheck(IntrinsiccallNode &intrincall, BlockNode &newblk);
  BaseNode *DoLowerRem(BinaryNode &expr, BlockNode &blkNode) {
    return DoLowerDiv(expr, blkNode);
  }

  uint32 divSTIndex = 0;                 // The index of divide operand and result.
  bool useRegTmp = Options::usePreg;     // Use register to save temp variable or not.
};

MAPLE_MODULE_PHASE_DECLARE(M2MJavaEHLowerer)
}  // namespace maple
#endif  // MAPLE_IR_INCLUDE_JAVA_EH_LOWER_H
