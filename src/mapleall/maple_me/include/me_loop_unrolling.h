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
#ifndef MAPLE_ME_INCLUDE_LOOP_UNROLLING_H
#define MAPLE_ME_INCLUDE_LOOP_UNROLLING_H
#include "me_scalar_analysis.h"
#include "me_function.h"
#include "me_irmap.h"
#include "me_phase.h"
#include "me_ir.h"
#include "me_ssa_update.h"
#include "me_dominance.h"
#include "me_loop_analysis.h"
#include "profile.h"
#include "me_cfg.h"

namespace maple {
constexpr uint32 kMaxCost = 100;
constexpr uint8 unrollTimes[3] = { 8, 4, 2 }; // unrollTimes
class LoopUnrolling {
 public:
  LoopUnrolling(MeFunction &f, MeFuncResultMgr &m, LoopDesc &l, MeIRMap &map, MemPool &pool, Dominance *d)
      : func(&f), cfg(f.GetCfg()), mgr(&m), loop(&l), irMap(&map), memPool(&pool), mpAllocator(&pool), dom(d),
        cands((std::less<OStIdx>(), mpAllocator.Adapter())), lastNew2OldBB(mpAllocator.Adapter()),
        profValid(func->IsIRProfValid()) {}
  ~LoopUnrolling() = default;
  bool LoopFullyUnroll(int64 tripCount);
  bool LoopPartialUnrollWithConst(uint32 tripCount);
  void LoopPartialUnrollWithVar(CR &cr, CRNode &varNode, uint32 i);

 private:
  VarMeExpr *CreateIndVarOrTripCountWithName(const std::string &name);
  void RemoveCondGoto();
  void BuildChiList(const BB &bb, MeStmt &newStmt, const MapleMap<OStIdx, ChiMeNode*> &oldChilist,
                    MapleMap<OStIdx, ChiMeNode*> &newChiList);
  void BuildMustDefList(const BB &bb, MeStmt &newStmt, const MapleVector<MustDefMeNode> &oldMustDef,
                        MapleVector<MustDefMeNode> &newMustDef);
  void CopyDassignStmt(const MeStmt &stmt, BB &bb);
  void CopyIassignStmt(MeStmt &stmt, BB &bb);
  void CopyIntrinsiccallStmt(MeStmt &stmt, BB &bb);
  void CopyCallStmt(MeStmt &stmt, BB &bb);
  void SetLabelWithCondGotoOrGotoBB(BB &bb, std::unordered_map<BB*, BB*> &old2NewBB, const BB &exitBB,
                                    LabelIdx oldlabIdx);
  void ResetOldLabel2NewLabel(std::unordered_map<BB*, BB*> &old2NewBB, BB &bb,
                              const BB &exitBB, BB &newHeadBB);
  void ResetOldLabel2NewLabel2(std::unordered_map<BB*, BB*> &old2NewBB, BB &bb,
                               const BB &exitBB, BB &newHeadBB);
  void CopyLoopBody(BB &newHeadBB, std::unordered_map<BB*, BB*> &old2NewBB,
                    std::set<BB*> &labelBBs, const BB &exitBB, bool copyAllLoop);
  void CopyLoopBodyForProfile(BB &newHeadBB, std::unordered_map<BB*, BB*> &old2NewBB,
                              std::set<BB*> &labelBBs, const BB &exitBB, bool copyAllLoop);
  void UpdateCondGotoBB(BB &bb, VarMeExpr &indVar, MeExpr &tripCount, MeExpr &unrollTimeExpr);
  void UpdateCondGotoStmt(BB &bb, VarMeExpr &indVar, MeExpr &tripCount, MeExpr &unrollTimeExpr, uint32 offset);
  void CreateIndVarAndCondGotoStmt(CR &cr, CRNode &varNode, BB &preCondGoto, uint32 unrollTime, uint32 i);
  void CopyLoopForPartial(BB &partialCondGoto, BB &exitedBB, BB &exitingBB);
  void CopyLoopForPartial(CR &cr, CRNode &varNode, uint32 j, uint32 unrollTime);
  void AddPreHeader(BB *oldPreHeader, BB *head);
  MeExpr *CreateExprWithCRNode(CRNode &crNode);
  void InsertCondGotoBB();
  void ResetFrequency(BB &bb);
  void ResetFrequency(BB &newCondGotoBB, BB &exitingBB, const BB &exitedBB, uint32 headFreq);
  void ResetFrequency();
  void ResetFrequency(const BB &curBB, const BB &succ, const BB &exitBB, BB &curCopyBB, bool copyAllLoop);
  BB *CopyBB(BB &bb, bool isInLoop);
  void CopyLoopForPartialAndPre(BB *&newHead, BB *&newExiting);
  void CopyAndInsertBB(bool isPartial);
  void CopyAndInsertStmt(BB &bb, std::vector<MeStmt*> &meStmts);
  void InsertCandsForSSAUpdate(OStIdx ostIdx, const BB &bb);
  void ComputeCodeSize(const MeStmt &meStmt, uint32 &cost);
  bool DetermineUnrollTimes(uint32 &index, bool isConst);
  void CreateLableAndInsertLabelBB(BB &newHeadBB, std::set<BB*> &labelBBs);
  void AddEdgeForExitBBLastNew2OldBBEmpty(BB &exitBB, std::unordered_map<BB*, BB*> &old2NewBB, BB &newHeadBB);
  void AddEdgeForExitBB(BB &exitBB, std::unordered_map<BB*, BB*> &old2NewBB, BB &newHeadBB);
  void ExchangeSucc(BB &partialExit);

  bool canUnroll = true;
  MeFunction *func;
  MeCFG      *cfg;
  MeFuncResultMgr *mgr;
  LoopDesc *loop;
  MeIRMap *irMap;
  MemPool *memPool;
  MapleAllocator mpAllocator;
  Dominance *dom;
  MapleMap<OStIdx, MapleSet<BBId>*> cands;
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
};

class MeDoLoopUnrolling : public MeFuncPhase {
 public:
  static bool enableDebug;
  static bool enableDump;
  explicit MeDoLoopUnrolling(MePhaseID id) : MeFuncPhase(id) {}
  ~MeDoLoopUnrolling() = default;

  AnalysisResult *Run(MeFunction *func, MeFuncResultMgr *m, ModuleResultMgr*) override;
  std::string PhaseName() const override {
    return "loopunrolling";
  }

 private:
  std::map<LoopDesc*, std::set<LoopDesc*>> parentLoop;
  void SetNestedLoop(const IdentifyLoops &meloop);
  void ExecuteLoopUnrolling(MeFunction &func, MeFuncResultMgr &m, MeIRMap &irMap);
  void VerifyCondGotoBB(BB &exitBB) const;
  bool IsDoWhileLoop(MeFunction &func, LoopDesc &loop) const;
  bool PredIsOutOfLoopBB(MeFunction &func, LoopDesc &loop) const;
  bool IsCanonicalAndOnlyOneExitLoop(MeFunction &func, LoopDesc &loop) const;
  void ExcuteLoopUnrollingWithConst(uint32 tripCount, MeFunction &func, MeIRMap &irMap,
                                    LoopUnrolling &loopUnrolling);
};
}  // namespace maple
#endif  // MAPLE_ME_INCLUDE_LOOP_UNROLLING_H
