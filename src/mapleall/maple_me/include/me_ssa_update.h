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
#ifndef MAPLE_ME_INCLUDE_ME_SSA_UPDATE_H
#define MAPLE_ME_INCLUDE_ME_SSA_UPDATE_H
#include "me_function.h"
#include "me_dominance.h"
#include "me_irmap.h"

namespace maple {
// When the number of osts is greater than kOstLimitSize, use RenameWithVectorStack object to update ssa, otherwise,
// use RenameWithMapStack object to update ssa
constexpr size_t kOstLimitSize = 5000;

class VersionStacks {
 public:
  VersionStacks() = default;
  virtual ~VersionStacks() = default;

  virtual std::stack<ScalarMeExpr*> *GetRenameStack(OStIdx idx) {
    (void)idx;
    ASSERT(false, "can not be here");
    return nullptr;
  }

  virtual void InsertZeroVersion2RenameStack(SSATab &ssaTab, IRMap &irMap) {
    (void)ssaTab;
    (void)irMap;
    ASSERT(false, "can not be here");
  }

  virtual void InitRenameStack(OStIdx idx) {
    (void)idx;
    ASSERT(false, "can not be here");
  }

  virtual void RecordCurrentStackSize(std::vector<std::pair<uint32, OStIdx >> &origStackSize) {
    (void)origStackSize;
    ASSERT(false, "can not be here");
  }

  virtual void RecoverStackSize(std::vector<std::pair<uint32, OStIdx >> &origStackSize) {
    (void)origStackSize;
    ASSERT(false, "can not be here");
  }
};

class VectorVersionStacks : public VersionStacks {
 public:
  VectorVersionStacks() = default;
  ~VectorVersionStacks() override = default;

  std::stack<ScalarMeExpr*> *GetRenameStack(OStIdx idx) override;
  void InsertZeroVersion2RenameStack(SSATab &ssaTab, IRMap &irMap) override;
  void InitRenameStack(OStIdx idx) override;
  void RecordCurrentStackSize(std::vector<std::pair<uint32, OStIdx >> &origStackSize) override;
  void RecoverStackSize(std::vector<std::pair<uint32, OStIdx >> &origStackSize) override;

  void ResizeRenameStack(size_t size) {
    renameWithVectorStacks.resize(size);
  }

 private:
  std::vector<std::unique_ptr<std::stack<ScalarMeExpr*>>> renameWithVectorStacks;
};

class MapVersionStacks : public VersionStacks {
 public:
  MapVersionStacks()
      : renameWithMapStacks(std::less<OStIdx>()) {}
  ~MapVersionStacks() override = default;

  std::stack<ScalarMeExpr*> *GetRenameStack(OStIdx idx) override;
  void InsertZeroVersion2RenameStack(SSATab &ssaTab, IRMap &irMap) override;
  void InitRenameStack(OStIdx idx) override;
  void RecordCurrentStackSize(std::vector<std::pair<uint32, OStIdx >> &origStackSize) override;
  void RecoverStackSize(std::vector<std::pair<uint32, OStIdx >> &origStackSize) override;

 private:
  std::map<OStIdx, std::unique_ptr<std::stack<ScalarMeExpr*>>> renameWithMapStacks;
};

class MeSSAUpdate {
 public:
  MeSSAUpdate(MeFunction &f, SSATab &stab, Dominance &d,
              std::map<OStIdx, std::unique_ptr<std::set<BBId>>> &cands)
      : func(f),
        irMap(*f.GetIRMap()),
        ssaTab(stab),
        dom(d),
        updateCands(cands) {}

  ~MeSSAUpdate() = default;

  void Run();
  // tool function - insert ost defined in defBB to ssaCands, if ssaCands is nullptr, do not insert.
  static void InsertOstToSSACands(OStIdx ostIdx, const BB &defBB,
                                  std::map<OStIdx, std::unique_ptr<std::set<BBId>>> *ssaCands = nullptr);
  // tool function - insert ost defined in defBB to ssaCands after traverse philist and all statements.
  static void InsertDefPointsOfBBToSSACands(BB &defBB, std::map<OStIdx, std::unique_ptr<std::set<BBId>>> &ssaCands,
      const OStIdx updateSSAExceptTheOstIdx = OStIdx(0));

 private:
  void InsertPhis();
  void RenamePhi(const BB &bb) const;
  MeExpr *RenameExpr(MeExpr &meExpr, bool &changed);
  void RenameStmts(BB &bb);
  void RenamePhiOpndsInSucc(const BB &bb) const;
  void RenameBB(BB &bb);
  MeFunction &func;
  IRMap &irMap;
  SSATab &ssaTab;
  Dominance &dom;
  std::map<OStIdx, std::unique_ptr<std::set<BBId>>> &updateCands;
  VersionStacks *rename = nullptr;
};

}  // namespace maple
#endif  // MAPLE_ME_INCLUDE_ME_SSA_UPDATE_H
