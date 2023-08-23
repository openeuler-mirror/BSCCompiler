/*
 * Copyright (c) [2022] Huawei Technologies Co.,Ltd.All rights reserved.
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
#ifndef MPL2MPL_INCLUDE_INLINE_TRANSFORMER_H
#define MPL2MPL_INCLUDE_INLINE_TRANSFORMER_H
#include "call_graph.h"
#include "maple_phase_manager.h"
#include "me_option.h"
#include "mempool.h"
#include "mempool_allocator.h"
#include "mir_builder.h"
#include "mir_function.h"
#include "mir_parser.h"
#include "opcode_info.h"
#include "string_utils.h"

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
constexpr char kPstatic[] = "__pstatic__";
constexpr char kDalvikSystemStr[] = "Ldalvik_2Fsystem_2FVMStack_3B_7C";
constexpr char kJavaLangThreadStr[] = "Ljava_2Flang_2FThread_3B_7C";
constexpr char kReflectionClassStr[] = "Ljava_2Flang_2Freflect";
constexpr char kJavaLangClassesStr[] = "Ljava_2Flang_2FClass_3B_7C";
constexpr char kJavaLangReferenceStr[] = "Ljava_2Flang_2Fref_2FReference_3B_7C";
constexpr char kInlineBeginComment[] = "inlining begin: FUNC ";
constexpr char kInlineEndComment[] = "inlining end: FUNC ";
constexpr char kSecondInlineSymbolSuffix[] = "SECOND_";
constexpr char kSecondInlineBeginComment[] = "second inlining begin: FUNC ";
constexpr char kSecondInlineEndComment[] = "second inlining end: FUNC ";

void UpdateEnclosingBlockCallback(const BlockNode &oldBlock, BlockNode &newBlock, const StmtNode &oldStmt,
    StmtNode &newStmt, CallBackData *data);

enum class RealArgPropCandKind {
  kUnknown,
  kConst,
  kVar,
  kPreg,
  kExpr,
};

// The candidate of real argument that can be propagated to callee's formal.
// It is either a mirConst or a CONST symbol (both local and global are OK),
// see RealArgPropCand::Parse for details.
struct RealArgPropCand {
  RealArgPropCandKind kind = RealArgPropCandKind::kUnknown;
  union {
    MIRSymbol *symbol = nullptr;
    MIRConst *mirConst;
    BaseNode *expr;
  } data;

  void Parse(MIRFunction &caller, bool calleeIsLikeMacro, BaseNode &argExpr);

  PrimType GetPrimType() const {
    if (kind == RealArgPropCandKind::kConst) {
      CHECK_NULL_FATAL(data.mirConst);
      return data.mirConst->GetType().GetPrimType();
    } else if (kind == RealArgPropCandKind::kVar || kind == RealArgPropCandKind::kPreg) {
      ASSERT_NOT_NULL(data.symbol);
      return data.symbol->GetType()->GetPrimType();
    } else if (kind == RealArgPropCandKind::kExpr) {
      ASSERT_NOT_NULL(data.expr);
      return data.expr->GetPrimType();
    }
    return PTY_unknown;
  }
};

enum InlineStage {
  kEarlyInline,
  kGreedyInline
};

class InlineTransformer {
 public:
  InlineTransformer(InlineStage inlineStage, MIRFunction &caller, MIRFunction &callee,
                    CallNode &callStmt, bool dumpDetail, CallGraph *cg)
      : builder(*theMIRModule->GetMIRBuilder()),
        caller(caller),
        callee(callee),
        callStmt(callStmt),
        enclosingBlk(*callStmt.GetEnclosingBlock()),
        stage(inlineStage),
        dumpDetail(dumpDetail),
        cg(cg),
        updateFreq(Options::profileUse && caller.GetFuncProfData() && callee.GetFuncProfData()) {
    theMIRModule->SetCurFunction(&caller);
  }

  bool PerformInline(BlockNode &enclosingBlk);
  bool PerformInline(CallInfo *callInfo = nullptr, std::vector<CallInfo*> *newCallInfo = nullptr);
  static void ReplaceSymbols(BaseNode *baseNode, uint32 stIdxOff, const std::vector<uint32> *oldStIdx2New);
  static void ConvertPStaticToFStatic(MIRFunction &func);
  void SetDumpDetail(bool value) {
    dumpDetail = value;
  }
  ~InlineTransformer() = default;

 private:
  BlockNode *CloneFuncBody(BlockNode &funcBody, bool recursiveFirstClone);
  BlockNode *GetClonedCalleeBody();
  uint32 RenameSymbols(uint32 inlinedTimes) const;
  uint32 RenameLabels(uint32 inlinedTimes) const;
  uint32 RenamePregs(std::unordered_map<PregIdx, PregIdx> &pregOld2new) const;
  void ReplaceLabels(BaseNode &baseNode, uint32 labIdxOff) const;
  void ReplacePregs(BaseNode *baseNode, std::unordered_map<PregIdx, PregIdx> &pregOld2New) const;
  void UpdateEnclosingBlock(BlockNode &body) const;
  void CollectNewCallInfo(BaseNode *node, std::vector<CallInfo*> *newCallInfo,
                          const std::map<uint32, uint32> &reverseCallIdMap) const;
  void AssignActualsToFormals(BlockNode &newBody, uint32 stIdxOff, uint32 regIdxOff);
  void AssignActualToFormal(BlockNode& newBody, uint32 stIdxOff, uint32 regIdxOff,
      BaseNode &oldActual, const MIRSymbol &formal);
  void PropConstFormalInBlock(BlockNode &newBody, MIRSymbol &formal, const RealArgPropCand &realArg,
      uint32 stIdxOff, uint32 regIdxOff);
  void PropConstFormalInNode(BaseNode &baseNode, MIRSymbol &formal, const RealArgPropCand &realArg,
      uint32 stIdxOff, uint32 regIdxOff);
  void TryReplaceConstFormalWithRealArg(BaseNode &parent, uint32 opndIdx, MIRSymbol &formal,
      const RealArgPropCand &realArg, const std::pair<uint32, uint32> &offsetPair);
  void GenReturnLabel(BlockNode &newBody, uint32 inlinedTimes);
  void HandleReturn(BlockNode &newBody);
  void ReplaceCalleeBody(BlockNode &enclosingBlock, BlockNode &newBody);
  LabelIdx CreateReturnLabel(uint32 inlinedTimes) const;
  GotoNode *UpdateReturnStmts(BlockNode &newBody, const CallReturnVector *callReturnVector, int &retCount) const;
  GotoNode *DoUpdateReturnStmts(BlockNode &newBody, StmtNode &stmt, const CallReturnVector *callReturnVector,
      int &retCount) const;
  void HandleTryBlock(StmtNode *stmtBeforeCall, StmtNode *stmtAfterCall);
  bool ResolveNestedTryBlock(BlockNode &body, TryNode &outerTryNode, const StmtNode *outerEndTryNode) const;

  MIRBuilder &builder;
  MIRFunction &caller;
  MIRFunction &callee;
  CallNode &callStmt;
  BlockNode &enclosingBlk;
  const InlineStage stage;
  bool dumpDetail = false;
  LabelIdx returnLabelIdx = 0;    // The lableidx where the code will jump when the callee returns.
  StmtNode *labelStmt = nullptr;  // The LabelNode we created for the callee, if needed.
  CallGraph *cg = nullptr;
  bool updateFreq = false;
};
}  // namespace maple
#endif  // MPL2MPL_INCLUDE_INLINE_TRANSFORMER_H
