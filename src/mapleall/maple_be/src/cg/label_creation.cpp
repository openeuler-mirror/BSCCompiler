/*
 * Copyright (c) [2020] Huawei Technologies Co.,Ltd.All rights reserved.
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
#include "label_creation.h"
#include "cgfunc.h"
#include "cg.h"
#include "debug_info.h"

namespace maplebe {
using namespace maple;

void LabelCreation::Run() const {
  CreateStartEndLabel();
}

void LabelCreation::CreateStartEndLabel() const {
  ASSERT(cgFunc != nullptr, "expect a cgfunc before CreateStartEndLabel");
  MIRBuilder *mirBuilder = cgFunc->GetFunction().GetModule()->GetMIRBuilder();
  ASSERT(mirBuilder != nullptr, "get mirbuilder failed in CreateStartEndLabel");

  /* create start label */
  LabelIdx startLblIdx = cgFunc->CreateLabel();
  LabelNode *startLabel = mirBuilder->CreateStmtLabel(startLblIdx);
  cgFunc->SetStartLabel(*startLabel);
  cgFunc->GetFunction().GetBody()->InsertFirst(startLabel);

  /* creat return label */
  LabelIdx returnLblIdx = cgFunc->CreateLabel();
  LabelNode *returnLabel = mirBuilder->CreateStmtLabel(returnLblIdx);
  cgFunc->SetReturnLabel(*returnLabel);
  cgFunc->GetFunction().GetBody()->InsertLast(returnLabel);

  /* create end label */
  LabelIdx endLblIdx = cgFunc->CreateLabel();
  LabelNode *endLabel = mirBuilder->CreateStmtLabel(endLblIdx);
  cgFunc->SetEndLabel(*endLabel);
  cgFunc->GetFunction().GetBody()->InsertLast(endLabel);
  ASSERT(cgFunc->GetFunction().GetBody()->GetLast() == endLabel, "last stmt must be a endLabel");

  /* create function's low/high pc if dwarf enabled */
  MIRFunction *func = &cgFunc->GetFunction();
  CG *cg = cgFunc->GetCG();
  if (cg->GetCGOptions().WithDwarf() && cgFunc->GetWithSrc()) {
    DebugInfo *di = cg->GetMIRModule()->GetDbgInfo();
    DBGDie *fdie = di->GetFuncDie(func);
    fdie->SetAttr(DW_AT_low_pc, startLblIdx);
    fdie->SetAttr(DW_AT_high_pc, endLblIdx);
  }

  /* add start/end labels into the static map table in class cg */
  if (!CG::IsInFuncWrapLabels(func)) {
    CG::SetFuncWrapLabels(func, std::make_pair(startLblIdx, endLblIdx));
  }
}

bool CgCreateLabel::PhaseRun(maplebe::CGFunc &f) {
  MemPool *memPool = GetPhaseMemPool();
  LabelCreation *labelCreate = memPool->New<LabelCreation>(f);
  labelCreate->Run();
  return false;
}
} /* namespace maplebe */
