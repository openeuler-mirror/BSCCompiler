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
#ifndef MAPLE_ME_INCLUDE_MEIDENTLOOPS_H
#define MAPLE_ME_INCLUDE_MEIDENTLOOPS_H
#include <algorithm>
#include "me_function.h"
#include "bb.h"
#include "dominance.h"
#include "me_cfg.h"
#include "maple_phase_manager.h"

namespace maple {
class IdentifyLoops;
// describes a specific loop, including the loop head, tail and sets of bb.
struct LoopDesc {
  MapleAllocator *alloc;
  BB *head;
  BB *tail;
  BB *preheader;
  BB *latch;
  BB *exitBB;
  MapleMap<BBId, MapleVector<BB*>*> inloopBB2exitBBs;
  MapleSet<BBId> loopBBs;
  LoopDesc *parent;  // points to its closest nesting loop
  uint32 nestDepth;  // the nesting depth
  bool hasTryBB = false;
  bool hasIgotoBB = false; // backedge is construted by igoto
  bool isCanonicalLoop = false;
  LoopDesc(MapleAllocator &mapleAllocator, BB *headBB, BB *tailBB)
      : alloc(&mapleAllocator), head(headBB), tail(tailBB), preheader(nullptr), latch(nullptr), exitBB(nullptr),
        inloopBB2exitBBs(alloc->Adapter()), loopBBs(alloc->Adapter()), parent(nullptr), nestDepth(0), hasTryBB(false),
        hasIgotoBB(false) {}

  bool Has(const BB &bb) const {
    return loopBBs.find(bb.GetBBId()) != loopBBs.end();
  }

  bool IsNormalizationLoop() const {
    if (!hasTryBB && !hasIgotoBB && preheader != nullptr && !inloopBB2exitBBs.empty()) {
      return true;
    }
    return false;
  }

  bool CheckBasicIV(MeExpr *solve, ScalarMeExpr *phiLhs, bool onlyStepOne = false) const;
  bool CheckStepOneIV(MeExpr *solve, ScalarMeExpr *phiLhs) const;
  bool IsFiniteLoop() const;

  void InsertInloopBB2exitBBs(const BB &bb, BB &value) {
    BBId key = bb.GetBBId();
    if (inloopBB2exitBBs.find(key) == inloopBB2exitBBs.end()) {
      inloopBB2exitBBs[key] = alloc->GetMemPool()->New<MapleVector<BB*>>(alloc->Adapter());
      inloopBB2exitBBs[key]->push_back(&value);
    } else {
      auto it = find(inloopBB2exitBBs[key]->begin(), inloopBB2exitBBs[key]->end(), &value);
      if (it == inloopBB2exitBBs[key]->end()) {
        inloopBB2exitBBs[key]->push_back(&value);
      }
    }
  }

  void ReplaceInloopBB2exitBBs(const BB &bb, BB &oldValue, BB &value) {
    BBId key = bb.GetBBId();
    CHECK_FATAL(inloopBB2exitBBs.find(key) != inloopBB2exitBBs.end(), "key must exits");
    auto mapIt = inloopBB2exitBBs[key];
    auto it = find(mapIt->begin(), mapIt->end(), &oldValue);
    CHECK_FATAL(it != inloopBB2exitBBs[key]->end(), "old Vvalue must exits");
    *it = &value;
    CHECK_FATAL(find(inloopBB2exitBBs[key]->begin(), inloopBB2exitBBs[key]->end(), &value) !=
                inloopBB2exitBBs[key]->end(), "replace fail");
    CHECK_FATAL(find(inloopBB2exitBBs[key]->begin(), inloopBB2exitBBs[key]->end(), &oldValue) ==
                inloopBB2exitBBs[key]->end(), "replace fail");
  }

  void SetHasTryBB(bool has) {
    hasTryBB = has;
  }

  bool HasTryBB() const {
    return hasTryBB;
  }

  void SetHasIGotoBB(bool has) {
    hasIgotoBB = has;
  }

  bool HasIGotoBB() const {
    return hasIgotoBB;
  }

  void SetIsCanonicalLoop(bool is) {
    isCanonicalLoop = is;
  }

  bool IsCanonicalLoop() const {
    return isCanonicalLoop;
  }

  bool IsCanonicalAndOnlyHasOneExitBBLoop() const {
    return IsCanonicalLoop() && inloopBB2exitBBs.size() == 1 && inloopBB2exitBBs.begin()->second != nullptr &&
        inloopBB2exitBBs.begin()->second->size() == 1;
  }

 private:
  bool DoCheckBasicIV(MeExpr *solve, ScalarMeExpr *phiLhs, int &meet, bool tryProp = false,
      bool onlyStepOne = false) const;
};

// IdentifyLoop records all the loops in a MeFunction.
class IdentifyLoops : public AnalysisResult {
 public:
  IdentifyLoops(MemPool *memPool, MeFunction &mf, Dominance *dm)
      : AnalysisResult(memPool),
        meLoopMemPool(memPool),
        meLoopAlloc(memPool),
        func(mf),
        cfg(mf.GetCfg()),
        dominance(dm),
        meLoops(meLoopAlloc.Adapter()),
        bbLoopParent(func.GetCfg()->GetAllBBs().size(), nullptr, meLoopAlloc.Adapter()) {}

  virtual ~IdentifyLoops() = default;

  const MapleVector<LoopDesc*> &GetMeLoops() const {
    return meLoops;
  }

  MapleVector<LoopDesc*> &GetMeLoops() {
    return meLoops;
  }

  LoopDesc *GetBBLoopParent(BBId bbID) const {
    if (bbID >= bbLoopParent.size()) {
      return nullptr;
    }
    return bbLoopParent.at(bbID);
  }

  LoopDesc *CreateLoopDesc(BB &hd, BB &tail);
  void SetLoopParent4BB(const BB &bb, LoopDesc &loopDesc);
  void SetExitBB(LoopDesc &loop);
  void ProcessBB(BB *bb);
  void Dump() const;
  void ProcessPreheaderAndLatch(LoopDesc &loop);

 private:
  MemPool *meLoopMemPool;
  MapleAllocator meLoopAlloc;
  MeFunction &func;
  MeCFG *cfg;
  Dominance *dominance;
  MapleVector<LoopDesc*> meLoops;
  MapleVector<LoopDesc*> bbLoopParent;  // gives closest nesting loop for each bb
};

MAPLE_FUNC_PHASE_DECLARE_BEGIN(MELoopAnalysis, MeFunction)
  IdentifyLoops *GetResult() {
    return identLoops;
  }
  IdentifyLoops *identLoops = nullptr;
OVERRIDE_DEPENDENCE
MAPLE_MODULE_PHASE_DECLARE_END
}  // namespace maple
#endif  // MAPLE_ME_INCLUDE_MEIDENTLOOPS_H
