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
#ifndef MPL2MPL_INCLUDE_GINLINE_H
#define MPL2MPL_INCLUDE_GINLINE_H
#include <queue>
#include <vector>

#include "call_graph.h"
#include "inline_analyzer.h"
#include "inline_transformer.h"
#include "maple_phase_manager.h"
#include "me_option.h"
#include "mempool.h"
#include "mempool_allocator.h"
#include "mir_builder.h"
#include "mir_function.h"
#include "mir_parser.h"
#include "opcode_info.h"
#include "string_utils.h"
#include "inline_summary.h"

namespace maple {
class CallSiteNode {
 public:
  CallSiteNode(CallInfo *info, BadnessInfo &badnessInfo, uint32 inlineDepth)
      : callInfo(info), badInfo(badnessInfo), depth(inlineDepth) {}

  auto *GetCallInfo() const {
    return callInfo;
  }

  const BadnessInfo &GetBadInfo() const {
    return badInfo;
  }

  double GetBadness() const {
    return badInfo.badness;
  }

  uint32 GetDepth() const {
    return depth;
  }

  int32 GetGrowth() const {
    return badInfo.growth;
  }

  double GetCallFreqPercent() const {
    return badInfo.freqPercent;
  }

  void Dump() const {
    LogInfo::MapleLogger() << "[" << depth << "] ";
    callInfo->Dump();
    LogInfo::MapleLogger() << " ";
    badInfo.Dump();
  }

  void DumpBrief() const {
    // lineno col caller callee callerId calleeId badInfo
    const auto &srcPos = callInfo->GetCallStmt()->GetSrcPos();
    auto lineno = srcPos.LineNum();
    auto col = srcPos.Column();
    auto *caller = callInfo->GetCaller();
    auto *callee = callInfo->GetCallee();
    LogInfo::MapleLogger() << callInfo->GetID() << "\t" << lineno << "\t" << col << "\t" << caller->GetName() << "\t"
        << callee->GetName() << "\t" << caller->GetPuidx() << "\t" << callee->GetPuidx() << "\t";
    badInfo.DumpWithoutKey();
  }

 private:
  CallInfo *callInfo;
  const BadnessInfo &badInfo;
  uint32 depth = 0;  // ginline callsite depth, default depth is 0
};

struct CallSiteNodeCmp {
  bool operator()(const CallSiteNode *lhs, const CallSiteNode *rhs) const {
    if (std::equal_to<double>()(lhs->GetBadness(), rhs->GetBadness())) {
      return lhs->GetCallInfo()->GetID() < rhs->GetCallInfo()->GetID();
    }
    return lhs->GetBadness() < rhs->GetBadness();
  }
};

class GInline {
 public:
  GInline(MIRModule &mod, MemPool *memPool, CallGraph *cg = nullptr)
      : alloc(memPool), module(mod), builder(*mod.GetMIRBuilder()), callSiteSet(alloc.Adapter()), cg(cg),
        funcsToBeRemoved(alloc.Adapter()), inlineStat(kIFC_End, 0, alloc.Adapter()) {
    Init();
  };

  ~GInline() {
    CleanupInline();
    cg = nullptr;
  }

  void Inline();

 protected:
  MapleAllocator alloc;
  MIRModule &module;
  MIRBuilder &builder;
  MapleSet<CallSiteNode*, CallSiteNodeCmp> callSiteSet;
  CallGraph *cg;

 private:
  void Init();
  void CleanupInline();
  void InitParams();
  void PrepareCallsiteSet();
  void AfterInlineSuccess(const CallSiteNode &heapNode, const std::vector<CallInfo*> &newCallInfo, bool canBeRemoved,
      int64 lastSize);
  void InlineCallsiteSet();
  bool FuncInlinable(const MIRFunction&) const;
  bool IsSafeToInline(const MIRFunction*, const CallNode&) const;

  void TryInlineToAllCallers();
  void TryInlineToAllCallers(const CGNode &calleeCgNode);
  bool ConsiderInlineCallsite(CallInfo &info, uint32 depth);
  void AddNewEdge(const std::vector<CallInfo*> &newCallInfo, uint32 depth);
  MapleSet<CallSiteNode*, CallSiteNodeCmp>::iterator FindCallsiteInSet(uint32 callId) const;
  void UpdateCaller(CGNode &callerNode);
  int64 ComputeTotalSize() const;
  int64 ComputeMaxSize(bool enableMoreGrowthForSmallModule) const;
  int64 GetFunctionStaticInsns(MIRFunction &func) const;
  BadnessInfo *CalcBadness(CallInfo &info);
  void InsertNewCallSite(CallInfo &info, uint32 depth);
  void UpdateCallSite(CallInfo &info);
  bool CanIgnoreGrowthLimit(const CallSiteNode &callSiteNode);
  void PrintGInlineReport() const;

  MapleSet<MIRFunction*> funcsToBeRemoved;
  MapleVector<uint32> inlineStat;  // index is InlineFailedCode, value is count
  uint32 totalSuccessCnt = 0;
  int64 initSize = 0;  // the initial size of the module.
  int64 curSize = 0;   // current size of the module.
  int64 finalSize = 0; // the final size after ginline.
  int64 maxSize = 0;   // the maximal size that this module can reach when doing inline.
  bool dumpDetail = false;
  std::string dumpFunc = "";
};

MAPLE_MODULE_PHASE_DECLARE(M2MGInline)
}  // namespace maple
#endif  // MPL2MPL_INCLUDE_GINLINE_H
