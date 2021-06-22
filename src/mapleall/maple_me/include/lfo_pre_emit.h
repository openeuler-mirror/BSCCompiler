/*
 * Copyright (c) [2021] Huawei Technologies Co., Ltd. All rights reserved.
 *
 * OpenArkCompiler is licensed under the Mulan Permissive Software License v2.
 * You can use this software according to the terms and conditions of the MulanPSL - 2.0.
 * You may obtain a copy of MulanPSL - 2.0 at:
 *
 *   https://opensource.org/licenses/MulanPSL-2.0
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR
 * FIT FOR A PARTICULAR PURPOSE.
 * See the MulanPSL - 2.0 for more details.
 */

#ifndef MAPLE_ME_INCLUDE_LFO_PRE_EMIT_H
#define MAPLE_ME_INCLUDE_LFO_PRE_EMIT_H
#include "me_irmap.h"
#include "me_phase.h"

namespace maple {
class LfoPreEmitter : public AnalysisResult {
 private:
  MeIRMap *meirmap;
  LfoFunction *lfoFunc;
  MIRFunction *mirFunc;
  MemPool *codeMP;
  MapleAllocator *codeMPAlloc;
  MemPool *lfoMP;
  MapleAllocator lfoMPAlloc;
  MapleMap<BaseNode*, LfoPart*> lfoParts; // key is mirnode
  MeCFG *cfg;

 public:
  LfoPreEmitter(MeIRMap *hmap, LfoFunction *f, MemPool *lfomp)
      : AnalysisResult(lfomp), meirmap(hmap),
        lfoFunc(f),
        mirFunc(f->meFunc->GetMirFunc()),
        codeMP(f->meFunc->GetMirFunc()->GetCodeMempool()),
        codeMPAlloc(&f->meFunc->GetMirFunc()->GetCodeMemPoolAllocator()),
        lfoMP(lfomp),
        lfoMPAlloc(lfoMP),
        lfoParts(lfoMPAlloc.Adapter()),
        cfg(f->meFunc->GetCfg()) {}

 private:
  BaseNode *EmitLfoExpr(MeExpr*, BaseNode *);
  StmtNode* EmitLfoStmt(MeStmt *, BaseNode *);
  void EmitBB(BB *, BlockNode *);
  DoloopNode *EmitLfoDoloop(BB *, BlockNode *, LfoWhileInfo *);
  WhileStmtNode *EmitLfoWhile(BB *, BlockNode *);
  uint32 Raise2LfoWhile(uint32, BlockNode *);
  uint32 Raise2LfoIf(uint32, BlockNode *);
 public:
  uint32 EmitLfoBB(uint32, BlockNode *);
  void InitFuncBodyLfoPart(BaseNode *funcbody) {
    LfoPart *rootlfo = lfoMP->New<LfoPart>(nullptr);
    lfoParts[funcbody] = rootlfo;
  }
  void SetLfoPart(BaseNode* node, LfoPart* lfoInfo) {
     lfoParts[node] = lfoInfo;
  }
  LfoPart* GetLfoPart(BaseNode *node) {
    return lfoParts[node];
  }
  BaseNode *GetParent(BaseNode *node) {
    LfoPart *lfopart = lfoParts[node];
    if (lfopart != nullptr) {
      return lfopart->parent;
    }
    return nullptr;
  }
  MeExpr *GetMexpr(BaseNode *node) {
    LfoPart *lfopart = lfoParts[node];
    return lfopart->meexpr;
  }
  MeStmt *GetMeStmt(BaseNode *node) {
    LfoPart *lfopart = lfoParts[node];
    return lfopart->mestmt;
  }
};

/* emit ir to specified file */
class DoLfoPreEmission : public MeFuncPhase {
 public:
  DoLfoPreEmission(MePhaseID id) : MeFuncPhase(id) {}

  AnalysisResult *Run(MeFunction *func, MeFuncResultMgr *m, ModuleResultMgr *mrm) override;
  std::string PhaseName() const override { return "lfopreemit"; }
};
}  // namespace maple
#endif  // MAPLE_ME_INCLUDE_LFO_PRE_EMIT_H
