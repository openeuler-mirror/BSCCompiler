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
#ifndef MAPLEBE_CG_INCLUDE_CG_PGO_GEN_H
#define MAPLEBE_CG_INCLUDE_CG_PGO_GEN_H

#include "cgfunc.h"
#include "instrument.h"
namespace maplebe {
class CGProfGen {
 public:
  CGProfGen(CGFunc &curF, MemPool &mp)
      : f(&curF),
        instrumenter(mp) {}
  virtual ~CGProfGen() {
    f = nullptr;
  }

  void InstrumentFunction();
  void CreateProfileCalls();
  virtual void CreateCallForDump(BB &bb, const MIRSymbol &dumpCall) = 0;
  virtual void CreateCallForAbort(BB &bb) = 0;
  virtual void InstrumentBB(BB &bb, MIRSymbol &countTab, uint32 offset) = 0;

  static void CreateProfInitExitFunc(MIRModule &m);
  static void CreateProfFileSym(MIRModule &m, const std::string &outputPath, const std::string &symName);
  static void CreateChildTimeSym(MIRModule &m, const std::string &symName);
 protected:
  CGFunc *f;
 private:
  static uint64 counterIdx;
  PGOInstrumentTemplate<maplebe::BB, maple::BBEdge<maplebe::BB>> instrumenter;
};

MAPLE_FUNC_PHASE_DECLARE(CgPgoGen, maplebe::CGFunc)
}
#endif // MAPLEBE_CG_INCLUDE_CG_PGO_GEN_H
