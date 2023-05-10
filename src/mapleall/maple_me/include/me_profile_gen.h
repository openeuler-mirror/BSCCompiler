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
#ifndef MAPLE_ME_INCLUDE_PROFILEGEN_H
#define MAPLE_ME_INCLUDE_PROFILEGEN_H

#include "me_pgo_instrument.h"
#include "bb.h"
#include "me_irmap.h"
#include "maple_phase_manager.h"

namespace maple {
class BBEdge;
class MeProfGen : public PGOInstrument<BBEdge> {
 public:
  MeProfGen(MeFunction &func, MemPool &mp, MeIRMap &hMap, bool dump)
      : PGOInstrument(func, mp, dump), func(&func), hMap(&hMap) {
    Init();
  }
  virtual ~MeProfGen() = default;
  bool CanInstrument() const;
  void InstrumentFunc();
  static void DumpSummary();
  static void IncTotalFunc();
 private:
  void Init();
  void InstrumentBB(BB &bb);
  void SaveProfile() const;
  MeFunction *func;
  MeIRMap *hMap;
  static uint64 counterIdx;
  static uint64 totalBB;
  static uint64 instrumentBB;
  static uint64 totalFunc;
  static uint64 instrumentFunc;
  static MIRSymbol *bbCounterTabSym;
  static bool firstRun;
};

MAPLE_FUNC_PHASE_DECLARE(MEProfGen, MeFunction)
}  // namespace maple
#endif  // MAPLE_ME_INCLUDE_PROFILEGEN_H
