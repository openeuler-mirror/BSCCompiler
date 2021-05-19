/*
 * Copyright (c) [2019-2020] Huawei Technologies Co.,Ltd.All rights reserved.
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
#ifndef MAPLE_ME_INCLUDE_ME_CFG_H
#define MAPLE_ME_INCLUDE_ME_CFG_H
#include "me_function.h"
#include "me_phase.h"

namespace maple {
class MeCFG {
 public:
  explicit MeCFG(MeFunction &f) : patternSet(f.GetAlloc().Adapter()), func(f) {}

  ~MeCFG() = default;

  bool IfReplaceWithAssertNonNull(const BB &bb) const;
  void ReplaceWithAssertnonnull();
  void BuildMirCFG();
  void FixMirCFG();
  void ConvertPhis2IdentityAssigns(BB &meBB) const;
  bool UnreachCodeAnalysis(bool updatePhi = false);
  void WontExitAnalysis();
  void Verify() const;
  void VerifyLabels() const;
  void Dump() const;
  void DumpToFile(const std::string &prefix, bool dumpInStrs = false, bool dumpEdgeFreq = false) const;
  bool FindExprUse(const BaseNode &expr, StIdx stIdx) const;
  bool FindUse(const StmtNode &stmt, StIdx stIdx) const;
  bool FindDef(const StmtNode &stmt, StIdx stIdx) const;
  bool HasNoOccBetween(StmtNode &from, const StmtNode &to, StIdx stIdx) const;

  const MeFunction &GetFunc() const {
    return func;
  }

  bool GetHasDoWhile() const {
    return hasDoWhile;
  }

  void SetHasDoWhile(bool hdw) {
    hasDoWhile = hdw;
  }

 private:
  void ReplaceSwitchContainsOneCaseBranchWithBrtrue(BB &bb, MapleVector<BB*> &exitBlocks);
  void AddCatchHandlerForTryBB(BB &bb, MapleVector<BB*> &exitBlocks);
  std::string ConstructFileNameToDump(const std::string &prefix) const;
  void DumpToFileInStrs(std::ofstream &cfgFile) const;
  void ConvertPhiList2IdentityAssigns(BB &meBB) const;
  void ConvertMePhiList2IdentityAssigns(BB &meBB) const;
  bool IsStartTryBB(BB &meBB) const;
  void FixTryBB(BB &startBB, BB &nextBB);
  MapleSet<LabelIdx> patternSet;
  MeFunction &func;
  bool hasDoWhile = false;
};
}  // namespace maple
#endif  // MAPLE_ME_INCLUDE_ME_CFG_H
