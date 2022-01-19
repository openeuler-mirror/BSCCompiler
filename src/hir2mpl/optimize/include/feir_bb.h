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
#ifndef HIR2MPL_FEIR_BB_H
#define HIR2MPL_FEIR_BB_H
#include <vector>
#include "types_def.h"
#include "mempool_allocator.h"
#include "safe_ptr.h"
#include "fe_utils.h"
#include "feir_stmt.h"

namespace maple {
enum FEIRBBKind : uint8 {
  kBBKindDefault,
  kBBKindPesudoHead,
  kBBKindPesudoTail,
  kBBKindExt
};

class FEIRBB : public FELinkListNode {
 public:
  FEIRBB();
  explicit FEIRBB(uint8 argKind);
  virtual ~FEIRBB();

  uint8 GetBBKind() const {
    return kind;
  }

  std::string GetBBKindName() const;

  uint32 GetID() const {
    return id;
  }

  void SetID(uint32 arg) {
    id = arg;
  }

  FEIRStmt *GetStmtHead() const {
    return stmtHead;
  }

  void SetStmtHead(FEIRStmt *stmtHeadIn) {
    stmtHead = stmtHeadIn;
  }

  FEIRStmt *GetStmtTail() const {
    return stmtTail;
  }

  void SetStmtTail(FEIRStmt *stmtTailIn) {
    stmtTail = stmtTailIn;
  }

  void InsertAndUpdateNewHead(FEIRStmt *newHead) {
    stmtHead->InsertBefore(newHead);
    stmtHead = newHead;
  }

  void InsertAndUpdateNewTail(FEIRStmt *newTail) {
    stmtTail->InsertAfter(newTail);
    stmtTail = newTail;
  }

  FEIRStmt *GetStmtNoAuxHead() const {
    return stmtNoAuxHead;
  }

  FEIRStmt *GetStmtNoAuxTail() const {
    return stmtNoAuxTail;
  }

  void AddPredBB(FEIRBB *bb) {
    if (predBBs.find(bb) == predBBs.end()) {
      predBBs.insert(bb);
    }
  }

  void AddSuccBB(FEIRBB *bb) {
    if (succBBs.find(bb) == succBBs.end()) {
      succBBs.insert(bb);
    }
  }

  const std::set<FEIRBB*> &GetPredBBs() const {
    return predBBs;
  }

  const std::set<FEIRBB*> &GetSuccBBs() const {
    return succBBs;
  }

  bool IsPredBB(FEIRBB *bb) const {
    return predBBs.find(bb) != predBBs.end();
  }

  bool IsSuccBB(FEIRBB *bb) const {
    return succBBs.find(bb) != succBBs.end();
  }

  bool IsDead() const {
    return predBBs.empty();
  }

  bool IsFallThru() const {
    return stmtNoAuxTail->IsFallThru();
  }

  bool IsBranch() const {
    return stmtNoAuxTail->IsBranch();
  }

  void SetCheckPointIn(std::unique_ptr<FEIRStmtCheckPoint> argCheckPointIn) {
    checkPointIn = std::move(argCheckPointIn);
  }

  FEIRStmtCheckPoint &GetCheckPointIn() const {
    return *(checkPointIn.get());
  }

  void SetCheckPointOut(std::unique_ptr<FEIRStmtCheckPoint> argCheckPointOut) {
    checkPointOut = std::move(argCheckPointOut);
  }

  FEIRStmtCheckPoint &GetCheckPointOut() const {
    return *(checkPointOut.get());;
  }

  void AddCheckPoint(std::unique_ptr<FEIRStmtCheckPoint> checkPoint) {
    checkPoints.emplace_back(std::move(checkPoint));
  }

  FEIRStmtCheckPoint *GetLatestCheckPoint() const {
    if (checkPoints.empty()) {
      return nullptr;
    }
    return checkPoints.back().get();
  }

  const std::vector<std::unique_ptr<FEIRStmtCheckPoint>> &GetCheckPoints() const {
    return checkPoints;
  }

  void AppendStmt(FEIRStmt *stmt);
  void AddStmtAuxPre(FEIRStmt *stmt);
  void AddStmtAuxPost(FEIRStmt *stmt);
  bool IsPredBB(uint32 bbID);
  bool IsSuccBB(uint32 bbID);
  virtual void Dump() const;

 protected:
  uint8 kind;
  uint32 id;
  FEIRStmt *stmtHead;
  FEIRStmt *stmtTail;
  FEIRStmt *stmtNoAuxHead;
  FEIRStmt *stmtNoAuxTail;
  std::set<FEIRBB*> predBBs;
  std::set<FEIRBB*> succBBs;

 private:
  std::unique_ptr<FEIRStmtCheckPoint> checkPointIn;
  std::vector<std::unique_ptr<FEIRStmtCheckPoint>> checkPoints;
  std::unique_ptr<FEIRStmtCheckPoint> checkPointOut;
  std::map<const FEIRStmt*, FEIRStmtCheckPoint*> feirStmtCheckPointMap;
};
}  // namespace maple
#endif  // HIR2MPL_FEIR_BB_H