/*
 * Copyright (c) [2022] Huawei Technologies Co.,Ltd.All rights reserved.
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
#ifndef MAPLEBE_INCLUDE_CG_REG_ALLOC_H
#define MAPLEBE_INCLUDE_CG_REG_ALLOC_H

#include "cgfunc.h"
#include "maple_phase_manager.h"

namespace maplebe {
class RATimerManager {
 public:
  RATimerManager(const RATimerManager&) = delete;
  RATimerManager& operator=(const RATimerManager&) = delete;

  static MPLTimerManager &GetInstance() {
    static RATimerManager raTimerM{};
    return raTimerM.timerM;
  }

  void PrintAllTimerAndClear(const std::string &funcName) {
    LogInfo::MapleLogger() << "Func[" << funcName << "] Reg Alloc Time:\n";
    LogInfo::MapleLogger() << timerM.ConvertAllTimer2Str() << std::endl;
    timerM.Clear();
  }
 private:
  RATimerManager() = default;
  ~RATimerManager() = default;

  MPLTimerManager timerM;
};

// RA time statistics marco. If defined, RA time consumed will print.
#ifdef REG_ALLOC_TIME_STATISTICS
#define RA_TIMER_REGISTER(timerName, str) MPLTimerRegister timerName##Timer(RATimerManager::GetInstance(), str)
#define RA_TIMER_STOP(timerName) timerName##Timer.Stop()
#define RA_TIMER_PRINT(funcName) RATimerManager::GetInstance().PrintAllTimerAndClear(funcName)
#else
#define RA_TIMER_REGISTER(name, str)
#define RA_TIMER_STOP(name)
#define RA_TIMER_PRINT(funcName)
#endif

class RegAllocator {
 public:
  RegAllocator(CGFunc &tempCGFunc, MemPool &memPool)
      : cgFunc(&tempCGFunc),
        memPool(&memPool),
        alloc(&memPool),
        regInfo(tempCGFunc.GetTargetRegInfo()) {
    regInfo->Init();
  }

  virtual ~RegAllocator() = default;

  virtual bool AllocateRegisters() = 0;

  bool IsYieldPointReg(regno_t regNO) const;
  bool IsUntouchableReg(regno_t regNO) const;

  virtual std::string PhaseName() const {
    return "regalloc";
  }

 protected:
  CGFunc *cgFunc = nullptr;
  MemPool *memPool = nullptr;
  MapleAllocator alloc;
  RegisterInfo *regInfo = nullptr;
};

MAPLE_FUNC_PHASE_DECLARE_BEGIN(CgRegAlloc, maplebe::CGFunc)
MAPLE_FUNC_PHASE_DECLARE_END
}  /* namespace maplebe */

#endif  /* MAPLEBE_INCLUDE_CG_REG_ALLOC_H */
