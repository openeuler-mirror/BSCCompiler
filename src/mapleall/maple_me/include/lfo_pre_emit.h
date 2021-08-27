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
#include "mir_nodes.h"
#include "me_irmap_build.h"

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
  MapleMap<uint32_t, LfoPart*>  lfoStmtParts; // map lfoinfo for StmtNode, key is stmtID
  MapleMap<BaseNode*, LfoPart*> lfoExprParts; // map lfoinfor for exprNode, key is mirnode
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
        lfoStmtParts(lfoMPAlloc.Adapter()),
        lfoExprParts(lfoMPAlloc.Adapter()),
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
    lfoExprParts[funcbody] = rootlfo;
  }
  void SetLfoStmtPart(uint32_t stmtID, LfoPart* lfoInfo) {
    lfoStmtParts[stmtID] = lfoInfo;
  }
  void SetLfoExprPart(BaseNode *expr, LfoPart* lfoInfo) {
    lfoExprParts[expr] = lfoInfo;
  }
  LfoPart* GetLfoExprPart(BaseNode *node) {
    return lfoExprParts[node];
  }
  LfoPart* GetLfoStmtPart(uint32_t stmtID) {
    return lfoStmtParts[stmtID];
  }
  BaseNode *GetParent(BaseNode *node) {
    LfoPart *lfopart = lfoExprParts[node];
    if (lfopart != nullptr) {
      return lfopart->parent;
    }
    return nullptr;
  }
  BaseNode *GetParent(uint32_t stmtID) {
    LfoPart *lfopart = lfoStmtParts[stmtID];
    if (lfopart != nullptr) {
      return lfopart->parent;
    }
    return nullptr;
  }
  MeExpr *GetMexpr(BaseNode *node) {
    LfoPart *lfopart = lfoExprParts[node];
    return lfopart->meexpr;
  }
  MeStmt *GetMeStmt(uint32_t stmtID) {
    LfoPart *lfopart = lfoStmtParts[stmtID];
    return lfopart->mestmt;
  }
  MIRFunction *GetMirFunction() { return mirFunc; }
  MeIRMap *GetMeIRMap() { return meirmap; }
  MemPool *GetCodeMP()  { return codeMP; }
  MapleAllocator* GetCodeMPAlloc() { return codeMPAlloc; }
  MapleMap<uint32_t, LfoPart *> *GetLfoStmtMap() { return &lfoStmtParts; }
  MapleMap<BaseNode *, LfoPart *> *GetLfoExprMap() { return &lfoExprParts; }
};

/* emit ir to specified file */
MAPLE_FUNC_PHASE_DECLARE_BEGIN(MELfoPreEmission, MeFunction)
  LfoPreEmitter *GetResult() {
    return emitter;
  }
  LfoPreEmitter *emitter = nullptr;
OVERRIDE_DEPENDENCE
MAPLE_FUNC_PHASE_DECLARE_END
}  // namespace maple
#endif  // MAPLE_ME_INCLUDE_LFO_PRE_EMIT_H
