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

#include "ra_opt.h"
#include "cgfunc.h"
#include "live.h"
#include "loop.h"
#include "cg.h"

namespace maplebe {
void LRSplitForSink::SetSplitRegCrossCall(const BB &bb, regno_t regno, bool afterCall) {
  if (afterCall) {
    auto *refsInfo = GetOrCreateSplitRegRefsInfo(regno);
    if (refsInfo == nullptr) {
      return;
    }
    refsInfo->afterCallBBs.insert(bb.GetId());
  }
}

void LRSplitForSink::SetSplitRegRef(Insn &insn, regno_t regno, bool isDef, bool isUse, bool afterCall) {
  auto *refsInfo = GetOrCreateSplitRegRefsInfo(regno);
  if (refsInfo == nullptr) {
    return;
  }
  if (isDef) {
    refsInfo->defInsns.push_back(&insn);
  }
  if (isUse) {
    refsInfo->useInsns.push_back(&insn);
  }
  SetSplitRegCrossCall(*insn.GetBB(), regno, afterCall);
}

void LRSplitForSink::ColletSplitRegRefsWithInsn(Insn &insn, bool afterCall) {
  const InsnDesc *md = insn.GetDesc();
  for (uint32 i = 0; i < insn.GetOperandSize(); ++i) {
    auto &opnd = insn.GetOperand(i);
    const auto *opndDesc = md->GetOpndDes(i);
    if (opnd.IsRegister()) {
      auto regno = static_cast<RegOperand&>(opnd).GetRegisterNumber();
      SetSplitRegRef(insn, regno, opndDesc->IsDef(), opndDesc->IsUse(), afterCall);
    } else if (opnd.IsMemoryAccessOperand()) {
      auto &memOpnd = static_cast<MemOperand&>(opnd);
      if (memOpnd.GetBaseRegister()) {
        SetSplitRegRef(insn, memOpnd.GetBaseRegister()->GetRegisterNumber(), false, true, afterCall);
      }
      if (memOpnd.GetIndexRegister()) {
        SetSplitRegRef(insn, memOpnd.GetIndexRegister()->GetRegisterNumber(), false, true, afterCall);
      }
    } else if (opnd.IsList()) {
      auto &listOpnd = static_cast<ListOperand&>(opnd);
      for (auto *regOpnd : std::as_const(listOpnd.GetOperands())) {
        SetSplitRegRef(insn, regOpnd->GetRegisterNumber(), opndDesc->IsDef(), opndDesc->IsUse(), afterCall);
      }
    }
  }
}

void LRSplitForSink::ColletSplitRegRefs() {
  FOR_ALL_BB(bb, &cgFunc) {
    bool afterCall = afterCallBBs[bb->GetId()];
    FOR_BB_INSNS(insn, bb) {
      if (insn->IsImmaterialInsn() || !insn->IsMachineInstruction()) {
        continue;
      }
      if (insn->IsCall()) {
        afterCall = true;
      }
      ColletSplitRegRefsWithInsn(*insn, afterCall);
    }
    if (afterCall) {
      for (auto regno : bb->GetLiveOutRegNO()) {
        SetSplitRegCrossCall(*bb, regno, afterCall);
      }
    }
  }
  if (dumpInfo) {
    for (const auto &[regno, refsInfo] : splitRegRefs) {
      LogInfo::MapleLogger() << "R" << regno << " : D" << refsInfo->defInsns.size() << "U" <<
          refsInfo->useInsns.size() << " After Call BB :";
      for (auto bb : refsInfo->afterCallBBs) {
        LogInfo::MapleLogger() << bb << " ";
      }
      LogInfo::MapleLogger() << "\n";
    }
  }
}

void LRSplitForSink::ColletAfterCallBBs(BB &bb, MapleBitVector &visited, bool isAfterCallBB) {
  if ((!isAfterCallBB && visited[bb.GetId()]) || afterCallBBs[bb.GetId()]) {
    return;
  }
  if (isAfterCallBB) {
    afterCallBBs[bb.GetId()] = true;
  } else {
    FOR_BB_INSNS(insn, &bb) {
      if (insn->IsImmaterialInsn() || !insn->IsMachineInstruction()) {
        continue;
      }
      if (insn->IsCall()) {
        isAfterCallBB = true;
        break;
      }
    }
  }
  visited[bb.GetId()] = true;
  for (auto *succBB : bb.GetSuccs()) {
    ColletAfterCallBBs(*succBB, visited, isAfterCallBB);
  }
  for (auto *succEhBB : bb.GetEhSuccs()) {
    ColletAfterCallBBs(*succEhBB, visited, isAfterCallBB);
  }
}

void LRSplitForSink::ColletAfterCallBBs() {
  MapleBitVector visited(cgFunc.NumBBs(), false, alloc.Adapter());
  FOR_ALL_BB(bb, &cgFunc) {
    if (bb->GetPreds().size() == 0) {
      ColletAfterCallBBs(*bb, visited, false);
    }
  }
}

BB *LRSplitForSink::SearchSplitBB(const RefsInfo &refsInfo) {
  BB *spiltBB = nullptr;
  for (auto bbId : refsInfo.afterCallBBs) {
    auto *bb = cgFunc.GetBBFromID(bbId);
    if (spiltBB == nullptr) {
      spiltBB = bb;
    } else {
      spiltBB = domInfo.GetCommonDom(*spiltBB, *bb);
    }
  }
  if (spiltBB == nullptr) {
    return nullptr;
  }
  auto *loop = loopInfo.GetBBLoopParent(spiltBB->GetId());
  while (loop != nullptr) {
    spiltBB = domInfo.GetDom(spiltBB->GetId());
    loop = loopInfo.GetBBLoopParent(spiltBB->GetId());
  }
  return spiltBB;
}

void LRSplitForSink::TryToSplitLiveRanges() {
  ColletSplitRegRefs();
  for (auto &[regno, refsInfo] : splitRegRefs) {
    // def point more than 1 or not cross call, there's no need to split it
    if (refsInfo->defInsns.size() > 1 || refsInfo->afterCallBBs.size() <= 0) {
      continue;
    }
    if (dumpInfo) {
      LogInfo::MapleLogger() << "try to split R" << regno << "\n";
    }

    auto *splitBB = SearchSplitBB(*refsInfo);
    // when splitBB is firstBB, there is no benefit from split live range.
    if (splitBB == nullptr || splitBB == cgFunc.GetFirstBB()) {
      return;
    }
    if (dumpInfo) {
      LogInfo::MapleLogger() << "R" << regno << " will split at BB " << splitBB->GetId() << "\n";
    }

    // create newreg and replace oldregs that is dominated by splitBB
    auto *destReg = splitRegs[regno];
    ASSERT_NOT_NULL(destReg);
    uint32 regSize = (destReg->GetSize() == k64BitSize) ? k8ByteSize : k4ByteSize;
    auto *newReg = &cgFunc.CreateVirtualRegisterOperand(cgFunc.NewVReg(destReg->GetRegisterType(), regSize));
    for (auto *insn : refsInfo->useInsns) {
      if (insn->GetBB() == splitBB || domInfo.Dominate(*splitBB, *insn->GetBB())) {
        cgFunc.ReplaceOpndInInsn(*destReg, *newReg, *insn, destReg->GetRegisterNumber());
      }
    }
    auto &movInsn = GenMovInsn(*newReg, *destReg);
    splitBB->InsertInsnBegin(movInsn);
    if (dumpInfo) {
      LogInfo::MapleLogger() << "split R" << regno << " to R" << newReg->GetRegisterNumber() << "\n";
    }
  }
}

void LRSplitForSink::Run() {
  CollectSplitRegs();
  if (splitRegs.empty()) {  // no need split reg
    return;
  }
  ColletAfterCallBBs();
  TryToSplitLiveRanges();
}

bool CgRaOpt::PhaseRun(maplebe::CGFunc &f) {
  auto *live = GET_ANALYSIS(CgLiveAnalysis, f);
  CHECK_FATAL(live != nullptr, "null ptr check");
  live->ResetLiveSet();

  auto *dom = GET_ANALYSIS(CgDomAnalysis, f);
  CHECK_FATAL(dom != nullptr, "null ptr check");
  auto *loop = GET_ANALYSIS(CgLoopAnalysis, f);
  CHECK_FATAL(loop != nullptr, "null ptr check");
  auto raOpt = f.GetCG()->CreateRaOptimizer(*GetPhaseMemPool(), f, *dom, *loop);
  CHECK_FATAL(raOpt != nullptr, "null ptr check");
  raOpt->InitializePatterns();
  raOpt->Run();
  // the live range info may changed, so invalid the info.
  if (live != nullptr) {
    live->ClearInOutDataInfo();
  }
  return false;
}

void CgRaOpt::GetAnalysisDependence(maple::AnalysisDep &aDep) const {
  aDep.AddRequired<CgLiveAnalysis>();
  aDep.AddRequired<CgDomAnalysis>();
  aDep.AddRequired<CgPostDomAnalysis>();
  aDep.AddRequired<CgLoopAnalysis>();
  aDep.PreservedAllExcept<CgLiveAnalysis>();
}
MAPLE_TRANSFORM_PHASE_REGISTER_CANSKIP(CgRaOpt, raopt)
}  /* namespace maplebe */
