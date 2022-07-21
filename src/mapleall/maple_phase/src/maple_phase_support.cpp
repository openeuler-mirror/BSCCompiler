/*
 * Copyright (c) [2021] Huawei Technologies Co.,Ltd.All rights reserved.
 *
 * OpenArkCompiler is licensed under the Mulan PSL v1.
 * You can use this software according to the terms and conditions of the Mulan PSL v1.
 * You may obtain a copy of Mulan PSL v1 at:
 *
 *     http://license.coscl.org.cn/MulanPSL
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR
 * FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 */
#include "maple_phase_support.h"

#include "cgfunc.h"
namespace maple {
void PhaseTimeHandler::RunBeforePhase(const MaplePhaseInfo &pi) {
  (void)pi;
  if (isMultithread) {
    static std::mutex mtx;
    ParallelGuard guard(mtx, true);
    std::thread::id tid = std::this_thread::get_id();
    if (multiTimers.count(tid) == 0) {
      multiTimers.emplace(std::make_pair(tid, allocator.New<MPLTimer>()));
    }
    multiTimers[tid]->Start();
  } else {
    timer.Start();
  }
}

void PhaseTimeHandler::RunAfterPhase(const MaplePhaseInfo &pi) {
  static std::mutex mtx;
  ParallelGuard guard(mtx, true);
  long usedTime = 0;
  if (isMultithread) {
    std::thread::id tid = std::this_thread::get_id();
    if (multiTimers.count(tid) > 0) {
      multiTimers[tid]->Stop();
      usedTime += multiTimers[tid]->ElapsedMicroseconds();
    } else {
      ASSERT(false, " phase time handler create failed");
    }
  } else {
    timer.Stop();
    usedTime = timer.ElapsedMicroseconds();
  }
  std::string phaseName = pi.PhaseName();
  if (phaseTimeRecord.count(phaseName) > 0) {
    phaseTimeRecord[phaseName] += usedTime;
  } else {
    auto ret = phaseTimeRecord.emplace(std::make_pair(phaseName, usedTime));
    if (ret.second) {
      originOrder.push_back(ret.first);
    }
  }
  phaseTotalTime += usedTime;
}

void PhaseTimeHandler::DumpPhasesTime() {
  auto timeLogger = [](const std::string &itemName, time_t itemTimeUs, time_t totalTimeUs) {
    LogInfo::MapleLogger() << std::left << std::setw(25) << std::setfill(' ') << itemName << std::setw(10)
                           << std::setfill(' ') << std::right << std::fixed << std::setprecision(2)
                           << (maplebe::kPercent * itemTimeUs / totalTimeUs) << "%" << std::setw(10)
                           << std::setfill(' ') << std::setprecision(0) << (itemTimeUs / maplebe::kMicroSecPerMilliSec)
                           << "ms\n";
  };

  LogInfo::MapleLogger() << "\n================== TIMEPHASES ==================\n";
  LogInfo::MapleLogger() << "================================================\n";
  for (auto phaseIt : std::as_const(originOrder)) {
    /*
     * output information by specified format, setw function parameter specifies show width
     * setprecision function parameter specifies precision
     */
    timeLogger(phaseIt->first, phaseIt->second, phaseTotalTime);
  }
  LogInfo::MapleLogger() << "================================================\n\n";
  LogInfo::MapleLogger().unsetf(std::ios::fixed);
}

const MapleVector<MaplePhaseID> &AnalysisDep::GetRequiredPhase() const {
  return required;
}
const MapleSet<MaplePhaseID> &AnalysisDep::GetPreservedPhase() const {
  return preserved;
}
const MapleSet<MaplePhaseID> &AnalysisDep::GetPreservedExceptPhase() const {
  return preservedExcept;
}
}  // namespace maple
