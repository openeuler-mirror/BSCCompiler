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
#include "ipa_collect.h"

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
        preMeStmtExtensionMap(preMeMPAlloc.Adapter()),
        preMeExprExtensionMap(preMeMPAlloc.Adapter()),
        cfg(f->meFunc->GetCfg()) {}
  ~PreMeEmitter() override {
    meirmap = nullptr;
    preMeFunc = nullptr;
    mirFunc = nullptr;
    codeMP = nullptr;
    codeMPAlloc = nullptr;
    preMeMP = nullptr;
    cfg = nullptr;
    ipaInfo = nullptr;
  }
  uint32 EmitPreMeBB(uint32 curJ, BlockNode *curBlk);
  void SetPreMeStmtExtension(uint32_t stmtID, PreMeMIRExtension* pmeExt) {
    preMeStmtExtensionMap[stmtID] = pmeExt;
  }
  void SetPreMeExprExtension(BaseNode* const expr, PreMeMIRExtension* pmeExt) {
    preMeExprExtensionMap[expr] = pmeExt;
  }
  PreMeMIRExtension* GetPreMeExprExtension(BaseNode *node) {
    return preMeExprExtensionMap[node];
  }
  PreMeMIRExtension* GetPreMeStmtExtension(uint32_t stmtID) {
    return preMeStmtExtensionMap[stmtID];
  }
  BaseNode *GetParent(BaseNode *node) {
    PreMeMIRExtension *pmeExt = preMeExprExtensionMap[node];
    if (pmeExt != nullptr) {
      return pmeExt->parent;
    }
    return nullptr;
  }
  BaseNode *GetParent(uint32_t stmtID) {
    PreMeMIRExtension *pmeExt = preMeStmtExtensionMap[stmtID];
    if (pmeExt != nullptr) {
      return pmeExt->parent;
    }
    return nullptr;
  }
  MeExpr *GetMexpr(BaseNode *node) {
    MapleMap<BaseNode *, PreMeMIRExtension *>::const_iterator it = preMeExprExtensionMap.find(node);
    if (it == preMeExprExtensionMap.end()) {
      return nullptr;
    }
    PreMeMIRExtension *pmeExt = it->second;
    ASSERT_NOT_NULL(pmeExt);
    return pmeExt->meexpr;
  }
  MeStmt *GetMeStmt(uint32_t stmtID) {
    PreMeMIRExtension *pmeExt = preMeStmtExtensionMap[stmtID];
    return pmeExt->mestmt;
  }
  MIRFunction &GetMirFunction() {
    return *mirFunc;
  }
  MeIRMap &GetMeIRMap() {
    return *meirmap;
  }
  MemPool &GetCodeMP() {
    return *codeMP;
  }
  MapleAllocator &GetCodeMPAlloc() {
    return *codeMPAlloc;
  }
  MapleMap<uint32_t, PreMeMIRExtension *> *GetPreMeStmtExtensionMap() {
    return &preMeStmtExtensionMap;
  }
  MapleMap<BaseNode *, PreMeMIRExtension *> *GetPreMeExprExtensionMap() {
    return &preMeExprExtensionMap;
  }
  FuncProfInfo *GetFuncProfData() const {
    return mirFunc->GetFuncProfData();
  }
  void SetIpaInfo(CollectIpaInfo *info) { ipaInfo = info; }
  void UpdateStmtInfo(const MeStmt &meStmt, StmtNode &stmt, BlockNode &currBlock, FreqType frequency) const;
  void UpdateStmtInfoForLabelNode(LabelNode &label, BB &bb) const;
 private:
  ArrayNode *ConvertToArray(BaseNode &x, const TyIdx &ptrTyIdx);
  BaseNode *EmitPreMeExpr(MeExpr &meExpr, BaseNode *parent);
  StmtNode* EmitPreMeStmt(MeStmt &meStmt, BaseNode *parent);
  template <typename Stmt, typename Node>
  void SetAssertBoundaryNode(const MeStmt &meStmt, const Stmt *assertBoundaryStmt,
                             Node *assertBoundaryNode, PreMeMIRExtension *pmeExt);
  void EmitBB(BB &bb, BlockNode &curBlk);
  DoloopNode *EmitPreMeDoloop(BB &meWhileBB, BlockNode &curBlk, PreMeWhileInfo &whileInfo);
  WhileStmtNode *EmitPreMeWhile(BB &meWhileBB, BlockNode &curBlk);
  uint32 Raise2PreMeWhile(uint32 curJ, BlockNode *curBlk);
  uint32 Raise2PreMeIf(uint32 curJ, BlockNode &curBlk);

  MeIRMap *meirmap = nullptr;
  PreMeFunction *preMeFunc = nullptr;
  MIRFunction *mirFunc = nullptr;
  MemPool *codeMP = nullptr;
  MapleAllocator *codeMPAlloc = nullptr;
  MemPool *preMeMP = nullptr;
  MapleAllocator preMeMPAlloc;
  MapleMap<uint32_t, PreMeMIRExtension*> preMeStmtExtensionMap; // key is stmtID
  MapleMap<BaseNode*, PreMeMIRExtension*> preMeExprExtensionMap; // key is BaseNode*
  MeCFG *cfg = nullptr;
  CollectIpaInfo *ipaInfo = nullptr;
};

/* emit ir to specified file */
MAPLE_FUNC_PHASE_DECLARE_BEGIN(MEPreMeEmission, MeFunction)
  void SetIpaInfo(MIRFunction &mirFunc);
  PreMeEmitter *GetResult() {
    return emitter;
  }
  PreMeEmitter *emitter = nullptr;
OVERRIDE_DEPENDENCE
MAPLE_FUNC_PHASE_DECLARE_END
}  // namespace maple
#endif  // MAPLE_ME_INCLUDE_PME_EMIT_H
