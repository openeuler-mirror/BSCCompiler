/*
 * Copyright (c) [2020-2021] Huawei Technologies Co.,Ltd.All rights reserved.
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
#ifndef HIR2MPL_INCLUDE_JBC_BB_H
#define HIR2MPL_INCLUDE_JBC_BB_H
#include <memory>
#include "general_bb.h"
#include "jbc_stack_helper.h"

namespace maple {
class JBCBB : public GeneralBB {
 public:
  JBCBB(const jbc::JBCConstPool &argConstPool);
  JBCBB(uint8 argBBKind, const jbc::JBCConstPool &argConstPool);
  ~JBCBB() = default;
  bool InitForFuncHeader();
  bool InitForCatch();
  bool UpdateStack();
  bool UpdateStackByPredBB(const JBCBB &bb);
  bool UpdateStackByPredBBEnd() const;
  bool CheckStack();
  uint32 GetSwapSize() const;
  bool GetStackError() const {
    return stackError;
  }

  const JBCStackHelper &GetMinStackIn() const {
    return minStackIn;
  }

  const JBCStackHelper &GetMinStackOut() const {
    return minStackOut;
  }

 protected:
  void DumpImpl() const override;

 private:
  const jbc::JBCConstPool &constPool;
  bool stackError : 1;
  bool stackInUpdated : 1;
  bool stackOutUpdated : 1;
  bool updatePredEnd : 1;
  JBCStackHelper minStackIn;
  JBCStackHelper minStackOut;
};
}  // namespace maple
#endif  // HIR2MPL_INCLUDE_JBC_BB_H
