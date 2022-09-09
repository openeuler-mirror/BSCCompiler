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
#include "me_bb_analyze.h"

namespace maple {
  void BBAnalyze::SetHotAndColdBBCountThreshold() {
    std::vector<uint32> times;
    auto eIt = cfg->valid_end();
    for (auto bIt = cfg->valid_begin(); bIt != eIt; ++bIt) {
      auto *bb = *bIt;
      times.push_back((bb->GetFrequency()));
    }
    std::sort(times.begin(), times.end(), std::greater<uint32>());
    uint32 hotIndex =
        static_cast<uint32>(static_cast<double>(times.size()) / kPrecision * (MeOption::profileBBHotRate));
    uint32 coldIndex =
        static_cast<uint32>(static_cast<double>(times.size()) / kPrecision * (MeOption::profileBBColdRate));
    hotBBCountThreshold = times.at(hotIndex);
    coldBBCountThreshold = times.at(coldIndex);
  }

  bool BBAnalyze::CheckBBHot(const BBId bbId) const {
    BB *bb = cfg->GetBBFromID(bbId);
    return bb->GetFrequency() >= hotBBCountThreshold;
  }

  bool BBAnalyze::CheckBBCold(const BBId bbId) const {
    BB *bb = cfg->GetBBFromID(bbId);
    return bb->GetFrequency() <= coldBBCountThreshold;
  }

  uint32 BBAnalyze::getHotBBCountThreshold() const {
    return hotBBCountThreshold;
  }

  uint32 BBAnalyze::getColdBBCountThreshold() const {
    return coldBBCountThreshold;
  }

#ifdef NOT_USED
  AnalysisResult *MeDoBBAnalyze::Run(MeFunction *func, MeFuncResultMgr *, ModuleResultMgr*) {
    MemPool *meBBAnalyze = NewMemPool();
    BBAnalyze *bbAnalyze = meBBAnalyze->New<BBAnalyze>(*meBBAnalyze, *func);
    if (func->IsIRProfValid()) {
      bbAnalyze->SetHotAndColdBBCountThreshold();
      return bbAnalyze;
    }
    return nullptr;
  }
#endif
}  // namespace maple
