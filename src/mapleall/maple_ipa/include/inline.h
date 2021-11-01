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
#ifndef MAPLE_IPA_INCLUDE_INLINE_H
#define MAPLE_IPA_INCLUDE_INLINE_H
#include "mir_parser.h"
#include "mir_function.h"
#include "opcode_info.h"
#include "mir_builder.h"
#include "mempool.h"
#include "mempool_allocator.h"
#include "call_graph.h"
#include "string_utils.h"
#include "me_option.h"
#include "maple_phase_manager.h"

namespace maple {
constexpr char kSpaceTabStr[] = " \t";
constexpr char kCommentsignStr[] = "#";
constexpr char kHyphenStr[] = "-";
constexpr char kAppointStr[] = "->";
constexpr char kUnderlineStr[] = "_";
constexpr char kVerticalLineStr[] = "__";
constexpr char kNumberZeroStr[] = "0";
constexpr char kReturnlocStr[] = "return_loc_";
constexpr char kThisStr[] = "_this";
constexpr char kDalvikSystemStr[] = "Ldalvik_2Fsystem_2FVMStack_3B_7C";
constexpr char kJavaLangThreadStr[] = "Ljava_2Flang_2FThread_3B_7C";
constexpr char kReflectionClassStr[] = "Ljava_2Flang_2Freflect";
constexpr char kJavaLangClassesStr[] = "Ljava_2Flang_2FClass_3B_7C";
constexpr char kJavaLangReferenceStr[] = "Ljava_2Flang_2Fref_2FReference_3B_7C";
constexpr char kInlineBeginComment[] = "inlining begin: FUNC ";
constexpr char kInlineEndComment[] = "inlining end: FUNC ";
constexpr char kSecondInlineBeginComment[] = "second inlining begin: FUNC ";
constexpr char kSecondInlineEndComment[] = "second inlining end: FUNC ";

struct InlineResult {
  bool canInline;
  std::string reason;

  InlineResult(bool canInline, const std::string &reason) : canInline(canInline), reason(reason) {}
  ~InlineResult() = default;
};

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
  MInline(MIRModule &mod, MemPool *memPool, CallGraph *cg = nullptr)
      : alloc(memPool),
        module(mod),
        builder(*mod.GetMIRBuilder()),
        inlineTimesMap(std::less<MIRFunction*>(), alloc.Adapter()),
        recursiveFuncToInlineLevel(alloc.Adapter()),
        funcToCostMap(alloc.Adapter()),
        cg(cg),
        excludedCaller(alloc.Adapter()),
        excludedCallee(alloc.Adapter()),
        hardCodedCallee(alloc.Adapter()),
        rcWhiteList(alloc.Adapter()),
        inlineListCallee(alloc.Adapter()),
        noInlineListCallee(alloc.Adapter()) {
    Init();
  };
  bool PerformInline(MIRFunction&, BlockNode &enclosingBlk, CallNode&, MIRFunction&);
  virtual void Inline();
  void CleanupInline();
  virtual ~MInline() = default;

 protected:
  MapleAllocator alloc;
  MIRModule &module;
  MIRBuilder &builder;
  MapleMap<MIRFunction*, uint32> inlineTimesMap;
  // For recursive inline, will dup the current function body before first recursive inline.
  BlockNode *currFuncBody = nullptr;
  // store recursive function's inlining levels, and allow inline 4 levels at most.
  MapleMap<MIRFunction*, uint32> recursiveFuncToInlineLevel;
  MapleMap<MIRFunction*, uint32> funcToCostMap; // save the cost of calculated func to reduce the amount of calculation
  CallGraph *cg;

 private:
  void Init();
  void InitParams();
  void InitProfile() const;
  void InitExcludedCaller();
  void InitExcludedCallee();
  void InitRCWhiteList();
  void ApplyInlineListInfo(const std::string &list, MapleMap<GStrIdx, MapleSet<GStrIdx>*> &listCallee);
  uint32 RenameSymbols(MIRFunction&, const MIRFunction&, uint32) const;
  void ReplaceSymbols(BaseNode*, uint32, const std::vector<uint32>*) const;
  uint32 RenameLabels(MIRFunction&, const MIRFunction&, uint32) const;
  void ReplaceLabels(BaseNode&, uint32) const;
  uint32 RenamePregs(const MIRFunction&, const MIRFunction&, std::unordered_map<PregIdx, PregIdx>&) const;
  void ReplacePregs(BaseNode*, std::unordered_map<PregIdx, PregIdx>&) const;
  LabelIdx CreateReturnLabel(MIRFunction&, const MIRFunction&, uint32) const;
  FuncCostResultType GetFuncCost(const MIRFunction&, const BaseNode&, uint32&, uint32) const;
  bool FuncInlinable(const MIRFunction&) const;
  bool IsSafeToInline(const MIRFunction*, const CallNode&) const;
  bool IsHotCallSite(const MIRFunction &caller, const MIRFunction &callee, const CallNode &callStmt) const;
  InlineResult AnalyzeCallsite(const MIRFunction &caller, MIRFunction &callee, const CallNode &callStmt);
  InlineResult AnalyzeCallee(const MIRFunction &caller, MIRFunction &callee, const CallNode &callStmt);
  virtual bool CanInline(CGNode*, std::unordered_map<MIRFunction*, bool>&) {
    return false;
  }

  void ConvertPStaticToFStatic(MIRFunction &func) const;
  bool CheckCalleeAndInline(MIRFunction*, BlockNode *enclosingBlk, CallNode*, MIRFunction*);
  bool SuitableForTailCallOpt(BaseNode &enclosingBlk, StmtNode &stmtNode, CallNode &callStmt);
  bool CalleeReturnValueCheck(StmtNode &stmtNode, CallNode &callStmt);
  void InlineCalls(const CGNode&);
  void InlineCallsBlock(MIRFunction&, BlockNode&, BaseNode&, bool&, BaseNode&);
  void InlineCallsBlockInternal(MIRFunction&, BlockNode&, BaseNode&, bool&);
  GotoNode *UpdateReturnStmts(const MIRFunction&, BlockNode&, LabelIdx, const CallReturnVector&, int&) const;
  void CollectMustInlineFuncs();
  void ComputeTotalSize();
  void MarkSymbolUsed(const StIdx&) const;
  void MarkUsedSymbols(const BaseNode*) const;
  void MarkUnInlinableFunction() const;
  bool ResolveNestedTryBlock(BlockNode&, TryNode&, const StmtNode*) const;
  void RecordRealCaller(MIRFunction&, const MIRFunction&);
  void SearchCallee(const MIRFunction&, const BaseNode&, std::set<GStrIdx>&) const;
  uint64 totalSize = 0;
  bool dumpDetail = false;
  std::string dumpFunc = "";
  MapleSet<GStrIdx> excludedCaller;
  MapleSet<GStrIdx> excludedCallee;
  MapleSet<GStrIdx> hardCodedCallee;
  MapleSet<GStrIdx> rcWhiteList;
  MapleMap<GStrIdx, MapleSet<GStrIdx>*> inlineListCallee;
  MapleMap<GStrIdx, MapleSet<GStrIdx>*> noInlineListCallee;
  uint32 smallFuncThreshold = 0;
  uint32 hotFuncThreshold = 0;
  uint32 recursiveFuncThreshold = 0;
  bool inlineWithProfile = false;
  std::string inlineFuncList;
  std::string noInlineFuncList;
  uint32 maxInlineLevel = 4; // for recursive function, allow inline 4 levels at most.
};

MAPLE_MODULE_PHASE_DECLARE(M2MInline)
}  // namespace maple
#endif  // MAPLE_IPA_INCLUDE_INLINE_H
