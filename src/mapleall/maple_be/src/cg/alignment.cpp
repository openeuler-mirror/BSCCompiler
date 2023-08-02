/*
 * Copyright (c) [2021] Huawei Technologies Co.,Ltd.All rights reserved.
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
#include "alignment.h"
#include "optimize_common.h"
#include "cgfunc.h"
#include "cg.h"
#include "cg_option.h"

namespace maplebe {
#define ALIGN_ANALYZE_DUMP_NEWPW CG_DEBUG_FUNC(func)

void AlignAnalysis::AnalysisAlignment() {
  // If pgo is enabled, we analysis alignment on specific by frequency.
  if (cgFunc.HasLaidOutByPgoUse() && CGOptions::DoPgoCodeAlign()) {
    LiteProfile::BBInfo *bbInfo = cgFunc.GetFunction().GetModule()->GetLiteProfile().GetFuncBBProf(cgFunc.GetName());
    if (bbInfo == nullptr) {
      LogInfo::MapleLogger() << cgFunc.GetName() << " is not in white list in pgo use\n";
      return;
    }
    FindLoopHeaderByFrequency();
    FindJumpTargetByFrequency();
  } else {
    FindLoopHeaderByDefault();
    FindJumpTargetByDefault();
  }

  ComputeLoopAlign();
  ComputeJumpAlign();

  if (CGOptions::DoCondBrAlign()) {
    ComputeCondBranchAlign();
  }
  if (CGOptions::DoLoopAlign() && !loopHeaderBBs.empty()) {
    AddNopForLoop();
  }
}

void AlignAnalysis::Dump() {
  MIRSymbol *funcSt = GlobalTables::GetGsymTable().GetSymbolFromStidx(cgFunc.GetFunction().GetStIdx().Idx());
  ASSERT(funcSt != nullptr, "null ptr check");
  LogInfo::MapleLogger() << "\n********* alignment for " << funcSt->GetName() << " *********\n";
  LogInfo::MapleLogger() << "------ jumpTargetBBs: " << jumpTargetBBs.size() << " total ------\n";
  for (auto *jumpLabel : jumpTargetBBs) {
    LogInfo::MapleLogger() << " === BB_" << jumpLabel->GetId() << " (" << std::hex << jumpLabel << ")"
                           << std::dec << " <" << jumpLabel->GetKindName();
    if (jumpLabel->GetLabIdx() != MIRLabelTable::GetDummyLabel()) {
      LogInfo::MapleLogger() << "[labeled with " << jumpLabel->GetLabIdx() << "]> ===\n";
    }
    if (!jumpLabel->GetPreds().empty()) {
      LogInfo::MapleLogger() << "\tpreds: [ ";
      for (auto *pred : jumpLabel->GetPreds()) {
        LogInfo::MapleLogger() << "BB_" << pred->GetId();
        if (pred->GetLabIdx() != MIRLabelTable::GetDummyLabel()) {
          LogInfo::MapleLogger() << "<labeled with " << jumpLabel->GetLabIdx() << ">";
        }
        LogInfo::MapleLogger() << " (" << std::hex << pred << ") " << std::dec << " ";
      }
      LogInfo::MapleLogger() << "]\n";
    }
    if (jumpLabel->GetPrev() != nullptr) {
      LogInfo::MapleLogger() << "\tprev: [ ";
      LogInfo::MapleLogger() << "BB_" << jumpLabel->GetPrev()->GetId();
      if (jumpLabel->GetPrev()->GetLabIdx() != MIRLabelTable::GetDummyLabel()) {
        LogInfo::MapleLogger() << "<labeled with " << jumpLabel->GetLabIdx() << ">";
      }
      LogInfo::MapleLogger() << " (" << std::hex << jumpLabel->GetPrev() << ") " << std::dec << " ";
      LogInfo::MapleLogger() << "]\n";
    }
    FOR_BB_INSNS_CONST(insn, jumpLabel) {
      insn->Dump();
    }
  }
  LogInfo::MapleLogger() << "\n------ loopHeaderBBs: " << loopHeaderBBs.size() << " total ------\n";
  for (auto *loopHeader : loopHeaderBBs) {
    LogInfo::MapleLogger() << " === BB_" << loopHeader->GetId() << " (" << std::hex << loopHeader << ")"
                           << std::dec << " <" << loopHeader->GetKindName();
    if (loopHeader->GetLabIdx() != MIRLabelTable::GetDummyLabel()) {
      LogInfo::MapleLogger() << "[labeled with " << loopHeader->GetLabIdx() << "]> ===\n";
    }
    auto *loop = loopInfo.GetBBLoopParent(loopHeader->GetId());
    LogInfo::MapleLogger() << "\tLoop Level: " << loop->GetNestDepth() << "\n";
    FOR_BB_INSNS_CONST(insn, loopHeader) {
      insn->Dump();
    }
  }
  LogInfo::MapleLogger() << "\n------ alignInfos: " << alignInfos.size() << " total ------\n";
  MapleUnorderedMap<BB*, uint32>::iterator iter;
  for (iter = alignInfos.begin(); iter != alignInfos.end(); ++iter) {
    BB *bb = iter->first;
    LogInfo::MapleLogger() << " === BB_" << bb->GetId() << " (" << std::hex << bb << ")"
                           << std::dec << " <" << bb->GetKindName();
    if (bb->GetLabIdx() != MIRLabelTable::GetDummyLabel()) {
      LogInfo::MapleLogger() << "[labeled with " << bb->GetLabIdx() << "]> ===\n";
    }
    LogInfo::MapleLogger() << "\talignPower: " << iter->second << "\n";
  }
}

void CgAlignAnalysis::GetAnalysisDependence(AnalysisDep &aDep) const {
  aDep.AddRequired<CgLoopAnalysis>();
  aDep.SetPreservedAll();
}

bool CgAlignAnalysis::PhaseRun(maplebe::CGFunc &func) {
  if (ALIGN_ANALYZE_DUMP_NEWPW) {
    DotGenerator::GenerateDot("alignanalysis", func, func.GetMirModule(), true, func.GetName());
  }
  auto *loopInfo = GET_ANALYSIS(CgLoopAnalysis, func);
  MemPool *alignMemPool = GetPhaseMemPool();
  AlignAnalysis *alignAnalysis = func.GetCG()->CreateAlignAnalysis(*alignMemPool, func, *loopInfo);

  CHECK_FATAL(alignAnalysis != nullptr, "AlignAnalysis instance create failure");
  alignAnalysis->AnalysisAlignment();
  if (ALIGN_ANALYZE_DUMP_NEWPW) {
    alignAnalysis->Dump();
  }
  return true;
}
} /* namespace maplebe */
