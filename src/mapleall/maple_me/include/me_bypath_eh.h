/*
 * Copyright (c) [2020] Huawei Technologies Co.,Ltd.All rights reserved.
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
#ifndef MAPLEME_INCLUDE_MEBYPATHEH_H
#define MAPLEME_INCLUDE_MEBYPATHEH_H

#include "me_phase.h"
#include "bb.h"
#include "class_hierarchy.h"
#include "mir_builder.h"

namespace maple {
class MeBypathEH {
 public:
  void BypathException(MeFunction &func, const KlassHierarchy &kh) const;
 private:
  StmtNode *IsSyncExit(BB &syncBB, MeFunction &func, LabelIdx secondLabel) const;
  bool DoBypathException(BB *tryBB, BB *catchBB, const Klass *catchClass, const StIdx &stIdx,
                         const KlassHierarchy &kh, MeFunction &func, const StmtNode *syncExitStmt) const;
};

class MeDoBypathEH : public MeFuncPhase {
 public:
  explicit MeDoBypathEH(MePhaseID id) : MeFuncPhase(id) {}
  ~MeDoBypathEH() = default;

  AnalysisResult *Run(MeFunction *func, MeFuncResultMgr *m, ModuleResultMgr *mrm) override;
  std::string PhaseName() const override {
    return "bypatheh";
  }
};
}  // namespace maple
#endif
