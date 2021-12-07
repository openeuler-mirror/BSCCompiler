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
#ifndef HIR2MPL_INCLUDE_COMMON_GENERAL_BB_H
#define HIR2MPL_INCLUDE_COMMON_GENERAL_BB_H
#include <vector>
#include "types_def.h"
#include "mempool_allocator.h"
#include "safe_ptr.h"
#include "fe_utils.h"
#include "general_stmt.h"

namespace maple {
enum GeneralBBKind : uint8 {
  kBBKindDefault,
  kBBKindPesudoHead,
  kBBKindPesudoTail,
  kBBKindExt
};

class GeneralBB : public FELinkListNode {
 public:
  GeneralBB();
  explicit GeneralBB(uint8 argKind);
  virtual ~GeneralBB();
  void AppendStmt(GeneralStmt &stmt);
  void AddStmtAuxPre(GeneralStmt &stmt);
  void AddStmtAuxPost(GeneralStmt &stmt);
  bool IsPredBB(uint32 bbID);
  bool IsSuccBB(uint32 bbID);
  uint8 GetBBKind() const {
    return kind;
  }

  GeneralStmt *GetStmtHead() const {
    return stmtHead;
  }

  void SetStmtHead(GeneralStmt &stmtHeadIn) {
    stmtHead = &stmtHeadIn;
  }

  void InsertAndUpdateNewHead(GeneralStmt &newHead) {
    stmtHead->InsertBefore(&newHead);
    stmtHead = &newHead;
  }

  GeneralStmt *GetStmtTail() const {
    return stmtTail;
  }

  void SetStmtTail(GeneralStmt &stmtTailIn) {
    stmtTail = &stmtTailIn;
  }

  void InsertAndUpdateNewTail(GeneralStmt &newTail) {
    stmtTail->InsertAfter(&newTail);
    stmtTail = &newTail;
  }

  GeneralStmt *GetStmtNoAuxHead() const {
    return stmtNoAuxHead;
  }

  GeneralStmt *GetStmtNoAuxTail() const {
    return stmtNoAuxTail;
  }

  void AddPredBB(GeneralBB &bb) {
    if (predBBs.find(&bb) == predBBs.end()) {
      CHECK_FATAL(predBBs.insert(&bb).second, "predBBs insert failed");
    }
  }

  void AddSuccBB(GeneralBB &bb) {
    if (succBBs.find(&bb) == succBBs.end()) {
      CHECK_FATAL(succBBs.insert(&bb).second, "succBBs insert failed");
    }
  }

  const std::set<GeneralBB*> &GetPredBBs() const {
    return predBBs;
  }

  const std::set<GeneralBB*> &GetSuccBBs() const {
    return succBBs;
  }

  uint32 GetID() const {
    return id;
  }

  void SetID(uint32 arg) {
    id = arg;
  }

  bool IsPredBB(GeneralBB &bb) {
    return predBBs.find(&bb) != predBBs.end();
  }

  bool IsSuccBB(GeneralBB &bb) {
    return succBBs.find(&bb) != succBBs.end();
  }

  bool IsDead() {
    return IsDeadImpl();
  }

  void Dump() const {
    return DumpImpl();
  }

  std::string GetBBKindName() const {
    return GetBBKindNameImpl();
  }

 protected:
  virtual bool IsDeadImpl();
  virtual void DumpImpl() const;
  virtual std::string GetBBKindNameImpl() const;

  uint8 kind;
  GeneralStmt *stmtHead;
  GeneralStmt *stmtTail;
  GeneralStmt *stmtNoAuxHead;
  GeneralStmt *stmtNoAuxTail;
  std::set<GeneralBB*> predBBs;
  std::set<GeneralBB*> succBBs;
  uint32 id;
};
}  // namespace maple
#endif  // HIR2MPL_INCLUDE_COMMON_GENERAL_BB_H