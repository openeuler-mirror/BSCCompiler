/*
* Copyright (c) [2023] Huawei Technologies Co.,Ltd.All rights reserved.
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

#include "global_schedule.h"
#include "optimize_common.h"
#include "aarch64_data_dep_base.h"
#include "cg.h"

namespace maplebe {
void GlobalSchedule::Run() {
  FCDG *fcdg = cda.GetFCDG();
  CHECK_FATAL(fcdg != nullptr, "control dependence analysis failed");
  if (GLOBAL_SCHEDULE_DUMP) {
    cda.GenerateSimplifiedCFGDot();
    cda.GenerateCFGDot();
    cda.GenerateFCDGDot();
    DotGenerator::GenerateDot("globalsched", cgFunc, cgFunc.GetMirModule(), true, cgFunc.GetName());
  }
  InitInsnIdAndLocInsn();
  for (auto region : fcdg->GetAllRegions()) {
    if (region == nullptr || !CheckCondition(*region)) {
      continue;
    }
    interDDA.Run(*region);
    if (GLOBAL_SCHEDULE_DUMP) {
      cda.GenerateCFGInRegionDot(*region);
      interDDA.GenerateDataDepGraphDotOfRegion(*region);
    }
    InitInRegion(*region);
    if (CGOptions::DoVerifySchedule()) {
      VerifyingSchedule(*region);
      continue;
    }
    DoGlobalSchedule(*region);
  }
}

bool GlobalSchedule::CheckCondition(CDGRegion &region) {
  uint32 insnSum = 0;
  for (auto cdgNode : region.GetRegionNodes()) {
    BB *bb = cdgNode->GetBB();
    CHECK_FATAL(bb != nullptr, "get bb from cdgNode failed");
    FOR_BB_INSNS_CONST(insn, bb) {
      if (!insn->IsMachineInstruction()) {
        continue;
      }
      insnSum++;
    }
  }
  return insnSum <= kMaxInsnNum;
}

/*
 * The entry of global scheduling
 */
void GlobalSchedule::DoGlobalSchedule(CDGRegion &region) {
  if (GLOBAL_SCHEDULE_DUMP) {
    DumpRegionInfoBeforeSchedule(region);
  }
  listScheduler = schedMP.New<ListScheduler>(schedMP, cgFunc, false, "globalschedule");
  /* Process nodes in a region by the topology sequence */
  for (auto cdgNode : region.GetRegionNodes()) {
    BB *bb = cdgNode->GetBB();
    ASSERT(bb != nullptr, "get bb from cdgNode failed");
    if (bb->IsAtomicBuiltInBB()) {
      for (auto depNode : cdgNode->GetAllDataNodes()) {
        for (auto succLink : depNode->GetSuccs()) {
          DepNode &succNode = succLink->GetTo();
          succNode.DecreaseValidPredsSize();
        }
      }
      continue;
    }

    MemPool *cdgNodeMp = memPoolCtrler.NewMemPool("global-scheduler cdgNode memPool", true);
    /* Collect candidate instructions of current cdgNode */
    InitInCDGNode(region, *cdgNode, cdgNodeMp);

    /* Execute list scheduling */
    listScheduler->SetCDGRegion(region);
    listScheduler->SetCDGNode(*cdgNode);
    listScheduler->DoListScheduling();

    /* Reorder instructions in the current BB based on the scheduling result */
    FinishScheduling(*cdgNode);

    if (GLOBAL_SCHEDULE_DUMP) {
      DumpCDGNodeInfoAfterSchedule(*cdgNode);
    }

    ClearCDGNodeInfo(region, *cdgNode, cdgNodeMp);
  }
}

void GlobalSchedule::ClearCDGNodeInfo(CDGRegion &region, CDGNode &cdgNode, MemPool *cdgNodeMp) {
  std::vector<CDGNode*> equivalentNodes;
  cda.GetEquivalentNodesInRegion(region, cdgNode, equivalentNodes);
  for (auto equivNode : equivalentNodes) {
    for (auto depNode : equivNode->GetAllDataNodes()) {
      ASSERT(depNode->GetState() != kScheduled, "update state of depNode failed in finishScheduling");
      depNode->SetState(kNormal);
    }
  }

  memPoolCtrler.DeleteMemPool(cdgNodeMp);
  commonSchedInfo = nullptr;
}

void CgGlobalSchedule::GetAnalysisDependence(maple::AnalysisDep &aDep) const {
  aDep.AddRequired<CgControlDepAnalysis>();
}

bool CgGlobalSchedule::PhaseRun(maplebe::CGFunc &f) {
  MemPool *memPool = GetPhaseMemPool();
  ControlDepAnalysis *cda = GET_ANALYSIS(CgControlDepAnalysis, f);
  MAD *mad = Globals::GetInstance()->GetMAD();
  auto *ddb = memPool->New<AArch64DataDepBase>(*memPool, f, *mad, false);
  auto *idda = memPool->New<InterDataDepAnalysis>(f, *memPool, *ddb);
  auto *globalScheduler = f.GetCG()->CreateGlobalSchedule(*memPool, f, *cda, *idda);
  globalScheduler->Run();
  return true;
}
MAPLE_TRANSFORM_PHASE_REGISTER_CANSKIP(CgGlobalSchedule, globalschedule)
} /* namespace maplebe */
