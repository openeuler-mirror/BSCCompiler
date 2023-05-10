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
#ifndef MAPLE_ME_INCLUDE_LOOP_UNROLLING_H
#define MAPLE_ME_INCLUDE_LOOP_UNROLLING_H
#include "me_cfg.h"
#include "me_dominance.h"
#include "me_function.h"
#include "me_ir.h"
#include "me_irmap_build.h"
#include "me_loop_analysis.h"
#include "me_scalar_analysis.h"
#include "me_ssa_update.h"
#include "profile.h"

namespace maple {
constexpr uint32 kMaxCost = 100;
constexpr uint8 unrollTimes[3] = {8, 4, 2};  // unrollTimes
class LoopUnrolling {
 public:
  enum ReturnKindOfFullyUnroll {
    kCanFullyUnroll,
    kCanNotSplitCondGoto,
    kCanNotFullyUnroll,
  };

  LoopUnrolling(MeFunction &f, LoopDesc &l, MeIRMap &map, const MapleAllocator &alloc,
                std::map<OStIdx, std::unique_ptr<std::set<BBId>>> &candsTemp)
      : func(&f),
        cfg(f.GetCfg()),
        loop(&l),
        irMap(&map),
        mpAllocator(alloc),
        cands(candsTemp),
        lastNew2OldBB(mpAllocator.Adapter()),
        profValid(func->IsIRProfValid()) {}
  ~LoopUnrolling() = default;
  ReturnKindOfFullyUnroll LoopFullyUnroll(uint64 tripCount);
  bool LoopPartialUnrollWithConst(uint64 tripCount);
  bool LoopPartialUnrollWithVar(CR &cr, CRNode &varNode, uint32 j);
  bool LoopUnrollingWithConst(uint64 tripCount, bool onlyFully = false);
  void SetInstrumentProf(bool useInstrument) {
    instrumentProf = useInstrument;
    profValid = useInstrument;
  }
  bool GetInstrumentProf() const {
    return instrumentProf;
  }

 private:
  bool SplitCondGotoBB() const;
  VarMeExpr *CreateIndVarOrTripCountWithName(const std::string &name);
  void RemoveCondGoto();

  void SetLabelWithCondGotoOrGotoBB(BB &bb, std::unordered_map<BB*, BB*> &old2NewBB, const BB &exitBB,
                                    LabelIdx oldlabIdx);
  void ResetOldLabel2NewLabel(std::unordered_map<BB*, BB*> &old2NewBB, BB &bb, const BB &exitBB, BB &newHeadBB);
  void ResetOldLabel2NewLabel2(std::unordered_map<BB*, BB*> &old2NewBB, BB &bb, const BB &exitBB, BB &newHeadBB);
  void CopyLoopBody(BB &newHeadBB, std::unordered_map<BB*, BB*> &old2NewBB, std::set<BB*> &labelBBs,
                    const BB &exitBB, bool copyAllLoop);
  void CopyLoopBodyForProfile(BB &newHeadBB, std::unordered_map<BB*, BB*> &old2NewBB, std::set<BB*> &labelBBs,
                              const BB &exitBB, bool copyAllLoop);
  void UpdateCondGotoBB(BB &bb, VarMeExpr &indVar, MeExpr &tripCount, MeExpr &unrollTimeExpr);
  void UpdateCondGotoStmt(BB &bb, VarMeExpr &indVar, MeExpr &tripCount, MeExpr &unrollTimeExpr, uint32 offset);
  void CreateIndVarAndCondGotoStmt(CR &cr, CRNode &varNode, BB &preCondGoto, uint32 unrollTime, uint32 i);
  void CopyLoopForPartial(BB &partialCondGoto, BB &exitedBB, BB &exitingBB);
  void CopyLoopForPartial(CR &cr, CRNode &varNode, uint32 j, uint32 unrollTime);
  void AddPreHeader(BB *oldPreHeader, BB *head);
  MeExpr *CreateExprWithCRNode(CRNode &crNode);
  void InsertCondGotoBB();
  void ResetFrequency(BB &bb);
  void ResetFrequency(BB &newCondGotoBB, BB &exitingBB, const BB &exitedBB, FreqType headFreq);
  void ResetFrequency();
  void ResetFrequency(const BB &curBB, const BB &succ, const BB &exitBB, BB &curCopyBB, bool copyAllLoop);
  BB *CopyBB(BB &bb, bool isInLoop);
  void CopyLoopForPartialAndPre(BB *&newHead, BB *&newExiting);
  void CopyAndInsertBB(bool isPartial);
  void ComputeCodeSize(const MeStmt &meStmt, uint32 &cost);
  bool DetermineUnrollTimes(uint32 &index, bool isConst);
  void CreateLableAndInsertLabelBB(BB &newHeadBB, std::set<BB*> &labelBBs);
  void AddEdgeForExitBBLastNew2OldBBEmpty(BB &exitBB, std::unordered_map<BB*, BB*> &old2NewBB, BB &newHeadBB);
  void AddEdgeForExitBB(BB &exitBB, std::unordered_map<BB*, BB*> &old2NewBB, BB &newHeadBB);
  void ExchangeSucc(BB &partialExit);

  bool canUnroll = true;
  MeFunction *func;
  MeCFG *cfg;
  LoopDesc *loop;
  MeIRMap *irMap;
  MapleAllocator mpAllocator;
  std::map<OStIdx, std::unique_ptr<std::set<BBId>>> &cands;
  MapleMap<BB*, BB*> lastNew2OldBB;
  BB *partialSuccHead = nullptr;
  uint64 replicatedLoopNum = 0;
  uint64 partialCount = 0;
  bool needUpdateInitLoopFreq = true;
  bool profValid;
  bool resetFreqForAfterInsertGoto = false;
  bool firstResetForAfterInsertGoto = true;
  bool resetFreqForUnrollWithVar = false;
  bool isUnrollWithVar = false;
  bool instrumentProf = false;  // use instrumented profiling
};

class LoopUnrollingExecutor {
 public:
  explicit LoopUnrollingExecutor(MemPool &mp) : innerMp(&mp) {}
  virtual ~LoopUnrollingExecutor() = default;
  static bool enableDebug;
  static bool enableDump;

  void ExecuteLoopUnrolling(MeFunction &func, MeIRMap &irMap, std::map<OStIdx, std::unique_ptr<std::set<BBId>>> &cands,
                            IdentifyLoops &meLoop, const MapleAllocator &alloc);
  bool IsCFGChange() const {
    return isCFGChange;
  }

 private:
  void SetNestedLoop(const IdentifyLoops &meLoop, std::unordered_map<LoopDesc*, std::set<LoopDesc*>> &parentLoop) const;
  void VerifyCondGotoBB(BB &exitBB) const;
  bool IsDoWhileLoop(const MeFunction &func, LoopDesc &loop) const;
  bool PredIsOutOfLoopBB(const MeFunction &func, LoopDesc &loop) const;
  bool IsCanonicalAndOnlyOneExitLoop(const MeFunction &func, LoopDesc &loop,
                                     const std::unordered_map<LoopDesc*, std::set<LoopDesc*>> &parentLoop) const;
  MemPool *innerMp;
  bool isCFGChange = false;
};

MAPLE_FUNC_PHASE_DECLARE(MELoopUnrolling, MeFunction)
}  // namespace maple
#endif  // MAPLE_ME_INCLUDE_LOOP_UNROLLING_H
