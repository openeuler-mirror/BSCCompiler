/*
 * Copyright (c) [2020-2022] Huawei Technologies Co.,Ltd.All rights reserved.
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
#ifndef HIR2MPL_FEIR_CFG_H
#define HIR2MPL_FEIR_CFG_H
#include <memory>
#include <list>
#include "feir_bb.h"

namespace maple {
class FEIRCFG {
 public:
  FEIRCFG(FEIRStmt *argStmtHead, FEIRStmt *argStmtTail);

  virtual ~FEIRCFG() {
    currBBNode = nullptr;
    stmtHead = nullptr;
    stmtTail = nullptr;
  }

  void Init();
  void BuildBB();
  bool BuildCFG();
  const FEIRBB *GetHeadBB();
  const FEIRBB *GetNextBB();

  FEIRBB *GetDummyHead() const {
    return bbHead.get();
  }

  FEIRBB *GetDummyTail() const {
    return bbTail.get();
  }

  std::unique_ptr<FEIRBB> NewFEIRBB() const {
    return std::make_unique<FEIRBB>();
  }

 private:
  void AppendAuxStmt();
  FEIRBB *NewBBAppend();

  FEIRStmt *stmtHead;
  FEIRStmt *stmtTail;
  FELinkListNode *currBBNode = nullptr;
  std::list<std::unique_ptr<FEIRBB>> listBB;
  std::unique_ptr<FEIRBB> bbHead;
  std::unique_ptr<FEIRBB> bbTail;
};
}  // namespace maple
#endif  // HIR2MPL_FEIR_CFG_H