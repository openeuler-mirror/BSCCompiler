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

#include "local_schedule.h"
#include "data_dep_base.h"
#include "aarch64_data_dep_base.h"
#include "cg.h"
#include "optimize_common.h"


namespace maplebe {
void LocalSchedule::Run() {
  FCDG *fcdg = cda.GetFCDG();
  CHECK_FATAL(fcdg != nullptr, "control dependence analysis failed");
  if (LOCAL_SCHEDULE_DUMP) {
    DotGenerator::GenerateDot("localsched", cgFunc, cgFunc.GetMirModule(), true, cgFunc.GetName());
  }
  InitInsnIdAndLocInsn();
  for (auto region : fcdg->GetAllRegions()) {
    if (region == nullptr || !CheckCondition(*region)) {
      continue;
    }
    CDGNode *cdgNode = region->GetRegionRoot();
    BB *bb = cdgNode->GetBB();
    ASSERT(bb != nullptr, "get bb from cdgNode failed");
    if (bb->IsAtomicBuiltInBB()) {
      continue;
    }
    interDDA.Run(*region);
    InitInRegion(*region);
    if (LOCAL_SCHEDULE_DUMP) {
      DumpRegionInfoBeforeSchedule(*region);
    }
    DoLocalSchedule(*cdgNode);
  }
}

bool LocalSchedule::CheckCondition(CDGRegion &region) const {
  CHECK_FATAL(region.GetRegionNodeSize() == 1 && region.GetRegionRoot() != nullptr,
              "invalid region in local scheduling");
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
  if (insnSum > kMaxInsnNum) {
    return false;
  }
  return true;
}

void LocalSchedule::DoLocalSchedule(CDGNode &cdgNode) {
  listScheduler = schedMP.New<ListScheduler>(schedMP, cgFunc, false, "localschedule");
  InitInCDGNode(cdgNode);
  listScheduler->SetCDGRegion(*cdgNode.GetRegion());
  listScheduler->SetCDGNode(cdgNode);
  listScheduler->DoListScheduling();
  FinishScheduling(cdgNode);
  if (LOCAL_SCHEDULE_DUMP) {
    DumpCDGNodeInfoAfterSchedule(cdgNode);
  }
}

void LocalSchedule::InitInCDGNode(CDGNode &cdgNode) {
  commonSchedInfo = schedMP.New<CommonScheduleInfo>(schedMP);
  for (auto depNode : cdgNode.GetAllDataNodes()) {
    commonSchedInfo->AddCandidates(depNode);
    depNode->SetState(kCandidate);
  }
  listScheduler->SetCommonSchedInfo(*commonSchedInfo);

  uint32 insnNum = 0;
  BB *curBB = cdgNode.GetBB();
  CHECK_FATAL(curBB != nullptr, "get bb from cdgNode failed");
  FOR_BB_INSNS_CONST(insn, curBB) {
    if (insn->IsMachineInstruction()) {
      insnNum++;
    }
  }
  cdgNode.SetInsnNum(insnNum);

  if (LOCAL_SCHEDULE_DUMP) {
    DumpCDGNodeInfoBeforeSchedule(cdgNode);
  }
}

bool CgLocalSchedule::PhaseRun(maplebe::CGFunc &f) {
  MemPool *memPool = GetPhaseMemPool();
  auto *cda = memPool->New<ControlDepAnalysis>(f, *memPool, "localschedule", true);
  cda->Run();
  MAD *mad = Globals::GetInstance()->GetMAD();
  auto *ddb = memPool->New<AArch64DataDepBase>(*memPool, f, *mad, false);
  auto *idda = memPool->New<InterDataDepAnalysis>(f, *memPool, *ddb);
  auto *localScheduler = f.GetCG()->CreateLocalSchedule(*memPool, f, *cda, *idda);
  localScheduler->Run();
  return true;
}
MAPLE_TRANSFORM_PHASE_REGISTER_CANSKIP(CgLocalSchedule, localschedule)
} /* namespace maplebe */
