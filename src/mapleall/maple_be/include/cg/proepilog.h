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
#ifndef MAPLEBE_INCLUDE_CG_PROEPILOG_H
#define MAPLEBE_INCLUDE_CG_PROEPILOG_H
#include "cg_phase.h"
#include "cgfunc.h"
#include "insn.h"
#include "cg_dominance.h"
#include "loop.h"

namespace maplebe {
struct ProEpilogSaveInfo {
  ProEpilogSaveInfo(MapleAllocator &alloc) : prologBBs(alloc.Adapter()), epilogBBs(alloc.Adapter()) {}

  ~ProEpilogSaveInfo() = default;

  void Dump() {
    LogInfo::MapleLogger() << "prolog will saved at BB ";
    for (auto bbId : prologBBs) {
      LogInfo::MapleLogger() << bbId << " ";
    }
    LogInfo::MapleLogger() << "entry!\n";

    LogInfo::MapleLogger() << "epilog will saved at BB ";
    for (auto bbId : epilogBBs) {
      LogInfo::MapleLogger() << bbId << " ";
    }
    LogInfo::MapleLogger() << "exit!\n";
  }

  void Clear() {
    prologBBs.clear();
    epilogBBs.clear();
  }

  MapleSet<BBID> prologBBs;   // will be inserted prolog at entry
  MapleSet<BBID> epilogBBs;   // will be inserted epilog at exit
};

class ProEpilogAnalysis {
 public:
  ProEpilogAnalysis(CGFunc &func, MemPool &pool, DomAnalysis &dom, PostDomAnalysis &pdom, LoopAnalysis &loop)
    : cgFunc(func), memPool(pool), alloc(&pool), domInfo(dom), pdomInfo(pdom), loopInfo(loop), saveInfo(alloc) {}

  virtual ~ProEpilogAnalysis() = default;

  std::string PhaseName() const {
    return "proepiloganalysis";
  }

  virtual bool NeedProEpilog() {
    return true;
  }

  void Analysis();

  const ProEpilogSaveInfo &GetProEpilogSaveInfo() const {
    return saveInfo;
  }
 protected:
  CGFunc &cgFunc;
  MemPool &memPool;
  MapleAllocator alloc;
  DomAnalysis &domInfo;
  PostDomAnalysis &pdomInfo;
  LoopAnalysis &loopInfo;
  ProEpilogSaveInfo saveInfo;

  bool PrologBBHoist(const MapleSet<BBID> &saveBBs);
};

MAPLE_FUNC_PHASE_DECLARE_BEGIN(CgProEpilogAnalysis, maplebe::CGFunc);
const ProEpilogSaveInfo *GetResult() const {
  if (proepilogAnalysis == nullptr) {
    return nullptr;
  }
  return &proepilogAnalysis->GetProEpilogSaveInfo();
}
ProEpilogAnalysis *proepilogAnalysis = nullptr;
OVERRIDE_DEPENDENCE
MAPLE_FUNC_PHASE_DECLARE_END

class GenProEpilog {
 public:
  explicit GenProEpilog(CGFunc &func) : cgFunc(func) {}

  virtual ~GenProEpilog() = default;

  virtual void Run() {}

  std::string PhaseName() const {
    return "generateproepilog";
  }

  virtual bool NeedProEpilog() {
    return true;
  }

  /* CFI related routines */
  int64 GetOffsetFromCFA() const {
    return offsetFromCfa;
  }

  /* add increment (can be negative) and return the new value */
  int64 AddtoOffsetFromCFA(int64 delta) {
    offsetFromCfa += delta;
    return offsetFromCfa;
  }

 protected:

  CGFunc &cgFunc;
  int64 offsetFromCfa = 0; /* SP offset from Call Frame Address */
};
}  /* namespace maplebe */

#endif  /* MAPLEBE_INCLUDE_CG_PROEPILOG_H */
