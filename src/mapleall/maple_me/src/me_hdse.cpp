/*
 * Copyright (c) [2020-2021] Huawei Technologies Co.,Ltd.All rights reserved.
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
#include "me_hdse.h"
#include <iostream>
#include "ssa_mir_nodes.h"
#include "ver_symbol.h"
#include "me_ssa.h"
#include "hdse.h"

// The hdse phase performs dead store elimination using the well-known algorithm
// based on SSA.  The algorithm works as follows:
// 0. Initially, assume all stmts and expressions are not needed.
// 1. In a pass over the program, mark stmts that cannot be deleted as needed.
//    These include return/eh/call statements and assignments to volatile fields
//    and variables.  Expressions used by needed statements are put into a
//    worklist.
// 2. Process each node in the worklist. For any variable used in expression,
//    mark the def stmt for the variable as needed and push expressions used
//    by the def stmt to the worklist.
// 3. When the worklist becomes empty, perform a pass over the program to delete
//    statements that have not been marked as needed.
//
// The backward substitution optimization is also performed in this phase by
// piggy-backing on Step 3.  In the pass over the program in Step 3, it counts
// the number of uses for each variable version.  It also create a list of
// backward substitution candidates, which are dassign statements of the form:
//              x_i = y_j
// where x is a local variable with no alias, and y_j is defined via the return
// value of a call which is in the same BB.  Then,
// 4. For each backward substitution candidate x_i = y_j, if the use count of y_j
//    is 1, then replace the return value definition of y_j by x_i, and delete
//    the statement x_i = y_j.
// Before the hdse phase finishes, it performs 2 additional optimizations:
// 5. When earlier deletion caused a try block to become empty, delete the empty
//    try block while fixing up the CFG.
// 6. Perform unreacble code analysis, delete the BBs found unreachable and fix
//    up the CFG.

namespace maple {
// mark initExpr in whileinfo live
void MeHDSE::ProcessWhileInfos() {
  PreMeFunction *preMeFunc = func.GetPreMeFunc();
  if (preMeFunc == nullptr) {
    return;
  }
  MapleMap<LabelIdx, PreMeWhileInfo *>::iterator it = preMeFunc->label2WhileInfo.begin();
  for (; it != preMeFunc->label2WhileInfo.end(); it++) {
    if (it->second->initExpr != nullptr &&
        (it->second->initExpr->GetMeOp() == maple::kMeOpVar || it->second->initExpr->GetMeOp() == maple::kMeOpReg)) {
      workList.push_front(it->second->initExpr);
    }
  }
}

void MeHDSE::BackwardSubstitution() {
  for (DassignMeStmt *dass : backSubsCands) {
    ScalarMeExpr *rhsscalar = static_cast<ScalarMeExpr *>(dass->GetRHS());
    if (verstUseCounts[rhsscalar->GetVstIdx()] != 1) {
      continue;
    }
    ScalarMeExpr *lhsscalar = dass->GetLHS();
    // check that lhsscalar has no use after rhsscalar's definition
    CHECK_FATAL(rhsscalar->GetDefBy() == kDefByMustDef, "MeHDSE::BackwardSubstitution: rhs not defined by mustDef");
    MustDefMeNode *mustDef = &rhsscalar->GetDefMustDef();
    MeStmt *defStmt = mustDef->GetBase();
    bool hasAppearance = false;
    MeStmt *curstmt = dass->GetPrev();
    while (curstmt != defStmt && !hasAppearance) {
      for (uint32 i = 0; i < curstmt->NumMeStmtOpnds(); ++i) {
        if (curstmt->GetOpnd(i)->SymAppears(lhsscalar->GetOst()->GetIndex())) {
          hasAppearance = true;
        }
      }
      curstmt = curstmt->GetPrev();
    }
    if (hasAppearance) {
      continue;
    }
    // perform the backward substitution
    if (hdseDebug) {
      LogInfo::MapleLogger() << "------ hdse backward substitution deletes this stmt: ";
      dass->Dump(&irMap);
    }
    mustDef->UpdateLHS(*lhsscalar);
    dass->GetBB()->RemoveMeStmt(dass);
  }
}

void MakeEmptyTrysUnreachable(MeFunction &func) {
  auto cfg = func.GetCfg();
  auto eIt = cfg->valid_end();
  for (auto bIt = cfg->valid_begin(); bIt != eIt; ++bIt) {
    BB *tryBB = *bIt;
    // get next valid bb
    auto endTryIt = bIt;
    if (++endTryIt == eIt) {
      break;
    }
    BB *endTry = *endTryIt;
    auto &meStmts = tryBB->GetMeStmts();
    if (tryBB->GetAttributes(kBBAttrIsTry) && !meStmts.empty() &&
        meStmts.front().GetOp() == OP_try && tryBB->GetMePhiList().empty() &&
        endTry->GetAttributes(kBBAttrIsTryEnd) && endTry->IsMeStmtEmpty()) {
      // we found a try BB followed by an empty endtry BB
      BB *targetBB = endTry->GetSucc(0);
      while (!tryBB->GetPred().empty()) {
        auto *tryPred = tryBB->GetPred(0);
        // update targetbb's predecessors
        if (!tryPred->IsPredBB(*targetBB)) {
          ASSERT(endTry->IsPredBB(*targetBB), "MakeEmptyTrysUnreachable: processing error");
          for (size_t k = 0; k < targetBB->GetPred().size(); ++k) {
            if (targetBB->GetPred(k) == endTry) {
              // push additional phi operand for each phi at targetbb
              auto phiIter = targetBB->GetMePhiList().begin();
              for (; phiIter != targetBB->GetMePhiList().end(); ++phiIter) {
                MePhiNode *meVarPhi = phiIter->second;
                meVarPhi->GetOpnds().push_back(meVarPhi->GetOpnds()[k]);
              }
            }
          }
        }
        // replace tryBB in the pred's succ list by targetbb
        tryPred->ReplaceSucc(tryBB, targetBB);
        // if needed, update branch label
        MeStmt *br = to_ptr(tryPred->GetMeStmts().rbegin());
        if (br != nullptr) {
          if (br->IsCondBr()) {
            if (static_cast<CondGotoMeStmt*>(br)->GetOffset() == tryBB->GetBBLabel()) {
              LabelIdx label = func.GetOrCreateBBLabel(*targetBB);
              static_cast<CondGotoMeStmt*>(br)->SetOffset(label);
            }
          } else if (br->GetOp() == OP_goto) {
            LabelIdx label = func.GetOrCreateBBLabel(*targetBB);
            ASSERT(static_cast<GotoMeStmt*>(br)->GetOffset() == tryBB->GetBBLabel(), "Wrong label");
            static_cast<GotoMeStmt*>(br)->SetOffset(label);
          } else if (br->GetOp() == OP_multiway || br->GetOp() == OP_rangegoto) {
            CHECK_FATAL(false, "OP_multiway and OP_rangegoto are not supported");
          } else if (br->GetOp() == OP_switch) {
            LabelIdx label = func.GetOrCreateBBLabel(*targetBB);
            auto *switchNode = static_cast<SwitchMeStmt*>(br);
            if (switchNode->GetDefaultLabel() == tryBB->GetBBLabel()) {
              switchNode->SetDefaultLabel(label);
            }
            for (size_t m = 0; m < switchNode->GetSwitchTable().size(); ++m) {
              if (switchNode->GetSwitchTable()[m].second == tryBB->GetBBLabel()) {
                switchNode->SetCaseLabel(m, label);
              }
            }
          }
        }
      }
    }
  }
}

void MEHdse::GetAnalysisDependence(maple::AnalysisDep &aDep) const {
  aDep.AddRequired<MEDominance>();
  aDep.AddRequired<MEAliasClass>();
  aDep.AddRequired<MEIRMapBuild>();
  aDep.PreservedAllExcept<MEDominance>();
  aDep.PreservedAllExcept<MELoopAnalysis>();
}

bool MEHdse::PhaseRun(maple::MeFunction &f) {
  if (f.hdseRuns >= MeOption::hdseRunsLimit) {
    if (!MeOption::quiet) {
      LogInfo::MapleLogger() << "  == " << PhaseName() << " skipped\n";
    }
    return false;
  }
  f.hdseRuns++;
  auto *dom = GET_ANALYSIS(MEDominance, f);
  CHECK_NULL_FATAL(dom);
  auto *aliasClass = GET_ANALYSIS(MEAliasClass, f);
  CHECK_NULL_FATAL(aliasClass);
  auto *hMap = GET_ANALYSIS(MEIRMapBuild, f);
  CHECK_NULL_FATAL(hMap);

  MemPool *memPool = GetPhaseMemPool();
  auto *hdse = memPool->New<MeHDSE>(f, *dom, *hMap, aliasClass, DEBUGFUNC_NEWPM(f));
  hdse->hdseKeepRef = MeOption::dseKeepRef;
  hdse->DoHDSE();
  hdse->BackwardSubstitution();
  MakeEmptyTrysUnreachable(f);
  (void)f.GetCfg()->UnreachCodeAnalysis(/* update_phi = */ true);
  f.GetCfg()->WontExitAnalysis();
  if (DEBUGFUNC_NEWPM(f)) {
    LogInfo::MapleLogger() << "\n============== HDSE =============" << '\n';
    f.Dump(false);
  }
  return false;
}
}  // namespace maple
