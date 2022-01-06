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
class MeSSAUpdate {
 public:
  MeSSAUpdate(MeFunction &f, SSATab &stab, Dominance &d,
              std::map<OStIdx, std::unique_ptr<std::set<BBId>>> &cands, MemPool &mp)
      : func(f),
        irMap(*f.GetIRMap()),
        ssaTab(stab),
        dom(d),
        ssaUpdateMp(mp),
        ssaUpdateAlloc(&mp),
        updateCands(cands),
        renameStacks(std::less<OStIdx>(), ssaUpdateAlloc.Adapter()) {}

  ~MeSSAUpdate() = default;

  void Run();
  // tool function - insert ost defined in defBB to ssaCands, if ssaCands is nullptr, do not insert.
  static void InsertOstToSSACands(OStIdx ostIdx, const BB &defBB,
                                  std::map<OStIdx, std::unique_ptr<std::set<BBId>>> *ssaCands = nullptr);
  // tool function - insert ost defined in defBB to ssaCands after traverse philist and all statements.
  static void InsertDefPointsOfBBToSSACands(BB &defBB, std::map<OStIdx, std::unique_ptr<std::set<BBId>>> &ssaCands);

 private:
  void InsertPhis();
  void RenamePhi(const BB &bb);
  MeExpr *RenameExpr(MeExpr &meExpr, bool &changed);
  void RenameStmts(BB &bb);
  void RenamePhiOpndsInSucc(const BB &bb);
  void RenameBB(BB &bb);
  MeFunction &func;
  IRMap &irMap;
  SSATab &ssaTab;
  Dominance &dom;
  MemPool &ssaUpdateMp;
  MapleAllocator ssaUpdateAlloc;
  std::map<OStIdx, std::unique_ptr<std::set<BBId>>> &updateCands;
  MapleMap<OStIdx, MapleStack<ScalarMeExpr*>*> renameStacks;
};

}  // namespace maple
#endif  // MAPLE_ME_INCLUDE_ME_SSA_UPDATE_H
