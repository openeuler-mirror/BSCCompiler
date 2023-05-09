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
#ifndef MAPLE_ME_INCLUDE_ME_IRMAP_H
#define MAPLE_ME_INCLUDE_ME_IRMAP_H
#include "me_option.h"
#include "ssa_tab.h"
#include "me_function.h"
#include "irmap.h"
#include "me_cfg.h"

namespace maple {
class MeIRMap : public IRMap {
 public:
  static const uint32 kHmapHashLength = 5107;
  MeIRMap(MeFunction &f, MemPool &memPool)
      : IRMap(*f.GetMeSSATab(), memPool, kHmapHashLength), func(f), cfg(f.GetCfg()) {
    SetDumpStmtNum(MeOption::stmtNum);
  }

  ~MeIRMap() override = default;

  BB *GetBB(BBId id) override {
    return cfg->GetBBFromID(id);
  }

  BB *GetBBForLabIdx(LabelIdx lidx, PUIdx) override {
    return cfg->GetLabelBBAt(lidx);
  }

  void Dump() override;

  MeFunction &GetFunc() const {
    return func;
  }

 private:
  MeFunction &func;
  MeCFG *cfg;
};
}  // namespace maple
#endif  // MAPLE_ME_INCLUDE_ME_IRMAP_H
