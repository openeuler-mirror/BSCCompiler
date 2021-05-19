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
#ifndef MAPLE_ME_INCLUDE_MEIDENTLOOPS_H
#define MAPLE_ME_INCLUDE_MEIDENTLOOPS_H
#include "me_function.h"
#include "bb.h"
#include "me_phase.h"
#include "dominance.h"
#include <algorithm>
#include "me_cfg.h"

namespace maple {
class IdentifyLoops;
// describes a specific loop, including the loop head, tail and sets of bb.
struct LoopDesc {
  MapleAllocator *alloc;
  BB *head;
  BB *tail;
  BB *preheader;
  BB *latch;
  MapleMap<BBId, MapleVector<BB*>*> inloopBB2exitBBs;
  MapleSet<BBId> loopBBs;
  LoopDesc *parent;  // points to its closest nesting loop
  uint32 nestDepth;  // the nesting depth
  bool hasTryBB = false;
  bool hasIgotoBB = false; // backedge is construted by igoto
  bool isCanonicalLoop = false;
  LoopDesc(MapleAllocator &mapleAllocator, BB *headBB, BB *tailBB)
      : alloc(&mapleAllocator), head(headBB), tail(tailBB), preheader(nullptr), latch(nullptr),
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

  void SetMeLoop(size_t i, LoopDesc &desc) {
    meLoops[i] = &desc;
  }

  LoopDesc *CreateLoopDesc(BB &hd, BB &tail);
  void SetLoopParent4BB(const BB &bb, LoopDesc &loopDesc);
  void InsertExitBB(LoopDesc &loop);
  void ProcessBB(BB *bb);
  void MarkBB();
  void Dump() const;
  bool ProcessPreheaderAndLatch(LoopDesc &loop);
  void SetTryBB();
  void SetIGotoBB(); // check loop is constructed by igoto

 private:
  MemPool *meLoopMemPool;
  MapleAllocator meLoopAlloc;
  MeFunction &func;
  MeCFG *cfg;
  Dominance *dominance;
  MapleVector<LoopDesc*> meLoops;
  MapleVector<LoopDesc*> bbLoopParent;  // gives closest nesting loop for each bb
};

class MeDoMeLoop : public MeFuncPhase {
 public:
  explicit MeDoMeLoop(MePhaseID id) : MeFuncPhase(id) {}
  virtual ~MeDoMeLoop() = default;
  AnalysisResult *Run(MeFunction *func, MeFuncResultMgr *m, ModuleResultMgr*) override;
  std::string PhaseName() const override {
    return "identloops";
  }
};
}  // namespace maple
#endif  // MAPLE_ME_INCLUDE_MEIDENTLOOPS_H
