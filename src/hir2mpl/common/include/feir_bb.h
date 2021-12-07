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
#ifndef HIR2MPL_INCLUDE_COMMON_FEIR_BB_H
#define HIR2MPL_INCLUDE_COMMON_FEIR_BB_H
#include <vector>
#include <memory>
#include "general_bb.h"
#include "feir_stmt.h"

namespace maple {
class FEIRBB : public GeneralBB {
 public:
  FEIRBB(uint8 argKind) : GeneralBB(argKind) {}
  virtual ~FEIRBB() = default;
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

  void AddCheckPointInside(std::unique_ptr<FEIRStmtCheckPoint> checkPoint) {
    checkPointsInside.push_back(std::move(checkPoint));
  }

  FEIRStmtCheckPoint *GetLatestCheckPointInside() const {
    if (checkPointsInside.size() == 0) {
      return nullptr;
    }
    return checkPointsInside.back().get();
  }

  const std::vector<std::unique_ptr<FEIRStmtCheckPoint>> &GetCheckPointsInside() const {
    return checkPointsInside;
  }

  bool IsFallThru() const {
    return stmtNoAuxTail->IsFallThru();
  }

  bool IsBranch() const {
    return stmtNoAuxTail->IsBranch();
  }

  const std::map<const FEIRStmt*, FEIRStmtCheckPoint*> &GetFEIRStmtCheckPointMap() const {
    return feirStmtCheckPointMap;
  }

  void LinkSuccBBsCheckPoints() {
    for (GeneralBB *generalBB : succBBs) {
      FEIRBB *succBB = static_cast<FEIRBB*>(generalBB);
      FEIRStmtCheckPoint &cpIn = succBB->GetCheckPointIn();
      cpIn.AddPredCheckPoint(GetCheckPointOut());
    }
  }

  void LinkCheckPointsInside() {
    FEIRStmtCheckPoint *lastCheckPoint = checkPointIn.get();
    for (std::unique_ptr<FEIRStmtCheckPoint> &cp : checkPointsInside) {
      cp->AddPredCheckPoint(*lastCheckPoint);
      lastCheckPoint = cp.get();
    }
    checkPointOut->AddPredCheckPoint(*lastCheckPoint);
  }

  void InitFirstVisibleStmtForCheckPoints() {
    FELinkListNode *node = nullptr;
    FEIRStmt *currStmt = nullptr;
    FEIRStmtCheckPoint *currCheckPoint = checkPointOut.get();
    node = currCheckPoint->GetPrev();
    while (node != checkPointIn.get()) {
      currStmt = static_cast<FEIRStmt*>(node);
      if (currStmt->GetKind() == FEIRNodeKind::kStmtCheckPoint) {
        currCheckPoint = static_cast<FEIRStmtCheckPoint*>(currStmt);
      } else {
        currCheckPoint->SetFirstVisibleStmt(*currStmt);
        (void)feirStmtCheckPointMap.insert(std::make_pair(currStmt, currCheckPoint));
      }
      node = node->GetPrev();
    }
  }

  void RegisterDFGNodes2CheckPoints() {
    for (std::unique_ptr<FEIRStmtCheckPoint> &cp : checkPointsInside) {
      cp->RegisterDFGNodeFromAllVisibleStmts();
    }
    checkPointOut->RegisterDFGNodeFromAllVisibleStmts();
  }

 private:
  std::unique_ptr<FEIRStmtCheckPoint> checkPointIn;
  std::vector<std::unique_ptr<FEIRStmtCheckPoint>> checkPointsInside;
  std::unique_ptr<FEIRStmtCheckPoint> checkPointOut;
  std::map<const FEIRStmt*, FEIRStmtCheckPoint*> feirStmtCheckPointMap;
};  // class FEIRBB
}  // namespace maple
#endif  // HIR2MPL_INCLUDE_COMMON_FEIR_BB_H