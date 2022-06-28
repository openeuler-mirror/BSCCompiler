/*
 * Copyright (c) [2021] Futurewei Technologies Co., Ltd. All rights reserved.
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

#ifndef MAPLE_ME_INCLUDE_PME_EMIT_H
#define MAPLE_ME_INCLUDE_PME_EMIT_H
#include "mir_nodes.h"
#include "me_irmap_build.h"

namespace maple {
class PreMeEmitter : public AnalysisResult {
 public:
  PreMeEmitter(MeIRMap *hmap, PreMeFunction *f, MemPool *premp)
      : AnalysisResult(premp), meirmap(hmap),
        preMeFunc(f),
        mirFunc(f->meFunc->GetMirFunc()),
        codeMP(f->meFunc->GetMirFunc()->GetCodeMempool()),
        codeMPAlloc(&f->meFunc->GetMirFunc()->GetCodeMemPoolAllocator()),
        preMeMP(premp),
        preMeMPAlloc(preMeMP),
        PreMeStmtExtensionMap(preMeMPAlloc.Adapter()),
        PreMeExprExtensionMap(preMeMPAlloc.Adapter()),
        cfg(f->meFunc->GetCfg()) {}
  virtual ~PreMeEmitter() = default;
  uint32 EmitPreMeBB(uint32, BlockNode *);
  void SetPreMeStmtExtension(uint32_t stmtID, PreMeMIRExtension* pmeExt) {
    PreMeStmtExtensionMap[stmtID] = pmeExt;
  }
  void SetPreMeExprExtension(BaseNode *expr, PreMeMIRExtension* pmeExt) {
    PreMeExprExtensionMap[expr] = pmeExt;
  }
  PreMeMIRExtension* GetPreMeExprExtension(BaseNode *node) {
    return PreMeExprExtensionMap[node];
  }
  PreMeMIRExtension* GetPreMeStmtExtension(uint32_t stmtID) {
    return PreMeStmtExtensionMap[stmtID];
  }
  BaseNode *GetParent(BaseNode *node) {
    PreMeMIRExtension *pmeExt = PreMeExprExtensionMap[node];
    if (pmeExt != nullptr) {
      return pmeExt->parent;
    }
    return nullptr;
  }
  BaseNode *GetParent(uint32_t stmtID) {
    PreMeMIRExtension *pmeExt = PreMeStmtExtensionMap[stmtID];
    if (pmeExt != nullptr) {
      return pmeExt->parent;
    }
    return nullptr;
  }
  MeExpr *GetMexpr(BaseNode *node) {
    MapleMap<BaseNode *, PreMeMIRExtension *>::iterator it = PreMeExprExtensionMap.find(node);
    if (it == PreMeExprExtensionMap.end()) {
      return nullptr;
    }
    PreMeMIRExtension *pmeExt = it->second;
    ASSERT_NOT_NULL(pmeExt);
    return pmeExt->meexpr;
  }
  MeStmt *GetMeStmt(uint32_t stmtID) {
    PreMeMIRExtension *pmeExt = PreMeStmtExtensionMap[stmtID];
    return pmeExt->mestmt;
  }
  MIRFunction *GetMirFunction() { return mirFunc; }
  MeIRMap *GetMeIRMap() { return meirmap; }
  MemPool *GetCodeMP()  { return codeMP; }
  MapleAllocator* GetCodeMPAlloc() { return codeMPAlloc; }
  MapleMap<uint32_t, PreMeMIRExtension *> *GetPreMeStmtExtensionMap() { return &PreMeStmtExtensionMap; }
  MapleMap<BaseNode *, PreMeMIRExtension *> *GetPreMeExprExtensionMap() { return &PreMeExprExtensionMap; }
  GcovFuncInfo *GetFuncProfData() { return mirFunc->GetFuncProfData(); }

 private:
  ArrayNode *ConvertToArray(BaseNode *x, TyIdx ptrTyIdx);
  BaseNode *EmitPreMeExpr(MeExpr*, BaseNode *);
  StmtNode* EmitPreMeStmt(MeStmt *, BaseNode *);
  void EmitBB(BB *, BlockNode *);
  DoloopNode *EmitPreMeDoloop(BB *, BlockNode *, PreMeWhileInfo *);
  WhileStmtNode *EmitPreMeWhile(BB *, BlockNode *);
  uint32 Raise2PreMeWhile(uint32, BlockNode *);
  uint32 Raise2PreMeIf(uint32, BlockNode *);

  MeIRMap *meirmap;
  PreMeFunction *preMeFunc;
  MIRFunction *mirFunc;
  MemPool *codeMP;
  MapleAllocator *codeMPAlloc;
  MemPool *preMeMP;
  MapleAllocator preMeMPAlloc;
  MapleMap<uint32_t, PreMeMIRExtension*>  PreMeStmtExtensionMap; // key is stmtID
  MapleMap<BaseNode*, PreMeMIRExtension*> PreMeExprExtensionMap; // key is BaseNode*
  MeCFG *cfg;
};

/* emit ir to specified file */
MAPLE_FUNC_PHASE_DECLARE_BEGIN(MEPreMeEmission, MeFunction)
  PreMeEmitter *GetResult() {
    return emitter;
  }
  PreMeEmitter *emitter = nullptr;
OVERRIDE_DEPENDENCE
MAPLE_FUNC_PHASE_DECLARE_END
}  // namespace maple
#endif  // MAPLE_ME_INCLUDE_PME_EMIT_H
