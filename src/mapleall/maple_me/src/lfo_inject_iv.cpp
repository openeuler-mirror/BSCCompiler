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

#include "lfo_inject_iv.h"
#include "me_dominance.h"
#include "me_loop_analysis.h"
#include "me_option.h"
#include "mir_builder.h"
#include <string>

namespace maple {
bool MELfoInjectIV::PhaseRun(MeFunction &f) {
  Dominance *dom = GET_ANALYSIS(MEDominance, f);
  CHECK_FATAL(dom, "dominance phase has problem");
  IdentifyLoops *identloops = GET_ANALYSIS(MELoopAnalysis, f);
  CHECK_FATAL(identloops != nullptr, "identloops has problem");

  uint32 ivCount = 0;
  MIRBuilder *mirbuilder = f.GetMIRModule().GetMIRBuilder();
  PreMeFunction *preMeFunc = f.GetPreMeFunc();

  for (LoopDesc *aloop : identloops->GetMeLoops()) {
    BB *headbb = aloop->head;
    // check if the label has associated PreMeWhileInfo
    if (headbb->GetBBLabel() == 0) {
      continue;
    }
    if (headbb->GetPred().size() != 2) {
      continue;  // won't insert IV for loops with > 1 tail bbs
    }
    MapleMap<LabelIdx, PreMeWhileInfo*>::iterator it = preMeFunc->label2WhileInfo.find(headbb->GetBBLabel());
    if (it == preMeFunc->label2WhileInfo.end()) {
      continue;
    }
    PreMeWhileInfo *whileInfo = it->second;
    // find the entry BB as the predecessor of headbb that dominates headbb
    MapleVector<BB*>::iterator predit = headbb->GetPred().begin();
    for (; predit != headbb->GetPred().end(); ++predit) {
      if (dom->Dominate(**predit, *headbb))
        break;
    }
    if (predit == headbb->GetPred().end()) {
      continue;  // cannot insert IV for this loop
    }
    BB *entrybb = *predit;

    // create the IV for this loop
    std::string ivName("injected.iv");
    ivName.append(std::to_string(++ivCount));
    GStrIdx strIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(ivName);
    MIRSymbol *st = mirbuilder->CreateSymbol((TyIdx)PTY_i64, strIdx, kStVar, kScAuto, f.GetMirFunc(), kScopeLocal);
    whileInfo->injectedIVSym = st;
    if (DEBUGFUNC_NEWPM(f)) {
      LogInfo::MapleLogger() << "****** Injected IV " << st->GetName() << " in while loop at label ";
      LogInfo::MapleLogger() << "@" << f.GetMirFunc()->GetLabelName(headbb->GetBBLabel()) << std::endl;
    }

    // initialize IV to 0 at loop entry
    DassignNode *dass = mirbuilder->CreateStmtDassign(st->GetStIdx(), 0, mirbuilder->CreateIntConst(0, PTY_i64));
    StmtNode *laststmt = entrybb->IsEmpty() ? nullptr : &entrybb->GetLast();
    if (laststmt &&
        (laststmt->op == OP_brfalse || laststmt->op == OP_brtrue || laststmt->op == OP_goto ||
         laststmt->op == OP_igoto || laststmt->op == OP_switch)) {
      entrybb->InsertStmtBefore(laststmt, dass);
    } else {
      entrybb->AddStmtNode(dass);
    }

    // insert IV increment at loop tail BB
    BB *tailbb = aloop->tail;
    AddrofNode *dread = mirbuilder->CreateExprDread(*GlobalTables::GetTypeTable().GetInt64(), *st);
    BinaryNode *addnode = mirbuilder->CreateExprBinary(OP_add, *GlobalTables::GetTypeTable().GetInt64(),
                                                       dread, mirbuilder->CreateIntConst(1, PTY_i64));
    dass = mirbuilder->CreateStmtDassign(*st, 0, addnode);
    laststmt = tailbb->IsEmpty() ? nullptr : &tailbb->GetLast();
    laststmt ? tailbb->InsertStmtBefore(laststmt, dass) : tailbb->AddStmtNode(dass);
  }
  return true;
}

void MELfoInjectIV::GetAnalysisDependence(maple::AnalysisDep &aDep) const {
  aDep.AddRequired<MEDominance>();
  aDep.AddRequired<MELoopAnalysis>();
  aDep.SetPreservedAll();
}
}  // namespace maple
