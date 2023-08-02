/*
 * Copyright (c) [2019-2021] Huawei Technologies Co.,Ltd.All rights reserved.
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
#ifndef MPL2MPL_INCLUDE_INLINE_H
#define MPL2MPL_INCLUDE_INLINE_H
#include "inline_transformer.h"

namespace maple {

enum FuncCostResultType {
  kNotAllowedNode,
  kFuncBodyTooBig,
  kSmallFuncBody
};

enum ThresholdType {
  kSmallFuncThreshold,
  kHotFuncThreshold,
  kRecursiveFuncThreshold,
  kHotAndRecursiveFuncThreshold
};

class MInline {
 public:
  MInline(MIRModule &mod, MemPool *memPool, CallGraph *cg = nullptr, bool onlyAlwaysInline = false)
      : alloc(memPool),
        module(mod),
        builder(*mod.GetMIRBuilder()),
        funcToCostMap(alloc.Adapter()),
        cg(cg),
        onlyForAlwaysInline(onlyAlwaysInline),
        performEarlyInline(Options::enableGInline && module.GetSrcLang() == kSrcLangC) {
    Init();
  };

  virtual ~MInline() {
    CleanupInline();
    cg = nullptr;
  }

  void Inline();

 protected:
  MapleAllocator alloc;
  MIRModule &module;
  MIRBuilder &builder;
  // save the cost of calculated func to reduce the amount of calculation
  MapleMap<MIRFunction*, uint32> funcToCostMap;
  CallGraph *cg;

 private:
  void Init();
  void InitParams();
  void InitProfile() const;
  void CleanupInline();
  FuncCostResultType GetFuncCost(const MIRFunction &func, const BaseNode &baseNode, uint32 &cost,
      uint32 threshold) const;
  bool IsHotCallSite(const MIRFunction &caller, const MIRFunction &callee, const CallNode &callStmt) const;
  InlineResult AnalyzeCallee(const MIRFunction &caller, MIRFunction &callee, const CallNode &callStmt);
  void AdjustInlineThreshold(const MIRFunction &caller, MIRFunction &callee, const CallNode &callStmt,
      uint32 &threshold, uint32 &thresholdType);
  bool IsSmallCalleeForEarlyInline(MIRFunction &callee, int32 *outInsns) const;
  virtual bool CanInline(CGNode*, std::unordered_map<MIRFunction*, bool>&) {
    return false;
  }

  bool CheckCalleeAndInline(MIRFunction*, BlockNode *enclosingBlk, CallNode*, MIRFunction*);
  bool SuitableForTailCallOpt(BaseNode &enclosingBlk, const StmtNode &stmtNode, CallNode &callStmt);
  bool CalleeReturnValueCheck(StmtNode &stmtNode, CallNode &callStmt) const;
  void InlineCalls(CGNode &node);
  void PostInline(MIRFunction &caller);
  void InlineCallsBlock(MIRFunction &func, BlockNode &enclosingBlk, BaseNode &baseNode, bool &changed,
      BaseNode &prevStmt);
  void InlineCallsBlockInternal(MIRFunction &func, BaseNode &baseNode, bool &changed, BaseNode &prevStmt);
  void AlwaysInlineCallsBlockInternal(MIRFunction &func, BaseNode &baseNode, bool &changed);
  void EndsInliningWithFailure(const std::string &failReason, const MIRFunction &caller,
    const MIRFunction &callee, const CallNode &callStmt) const;
  GotoNode *UpdateReturnStmts(const MIRFunction&, BlockNode&, LabelIdx, const CallReturnVector&, int&) const;
  void ComputeTotalSize();
  void MarkSymbolUsed(const StIdx &symbolIdx) const;
  void MarkUsedSymbols(const BaseNode *baseNode) const;
  void MarkFunctionUsed(MIRFunction *func, bool inlined = false) const;
  void MarkUnInlinableFunction() const;
  bool HasAccessStatic(const BaseNode &baseNode) const;
  uint64 totalSize = 0;
  bool dumpDetail = false;
  std::string dumpFunc = "";
  uint32 smallFuncThreshold = 0;
  uint32 hotFuncThreshold = 0;
  uint32 recursiveFuncThreshold = 0;
  bool inlineWithProfile = false;
  const bool onlyForAlwaysInline;
  const bool performEarlyInline;  // open it only if ginline is enabled and srcLang is C
  uint32 maxRecursiveLevel = 4;  // for recursive function, allow inline 4 levels at most.
  uint32 currInlineDepth = 0;
  bool isCurrCallerTooBig = false;  // If the caller is too big, we wont't inline callsites except the ones that
                                    // must be inlined
};

MAPLE_MODULE_PHASE_DECLARE(M2MInline)
}  // namespace maple
#endif  // MPL2MPL_INCLUDE_INLINE_H
