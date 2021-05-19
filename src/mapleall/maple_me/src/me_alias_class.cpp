/*
 * Copyright (c) [2019-2020] Huawei Technologies Co.,Ltd.All rights reserved.
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
#include "me_alias_class.h"
#include "me_option.h"
#include "mpl_logging.h"
#include "ssa_mir_nodes.h"
#include "ssa_tab.h"
#include "me_function.h"
#include "mpl_timer.h"

namespace maple {
// This phase performs alias analysis based on Steensgaard's algorithm and
// represent the resulting alias relationships in the Maple IR representation
bool MeAliasClass::HasWriteToStaticFinal() const {
  for (auto bIt = func.valid_begin(); bIt != func.valid_end(); ++bIt) {
    for (const auto &stmt : (*bIt)->GetStmtNodes()) {
      if (stmt.GetOpCode() == OP_dassign) {
        const auto &dassignNode = static_cast<const DassignNode&>(stmt);
        if (dassignNode.GetStIdx().IsGlobal()) {
          const MIRSymbol *sym = mirModule.CurFunction()->GetLocalOrGlobalSymbol(dassignNode.GetStIdx());
          if (sym->IsFinal()) {
            return true;
          }
        }
      }
    }
  }
  return false;
}

void MeAliasClass::DoAliasAnalysis() {
  // pass 1 through the program statements
  for (auto bIt = func.valid_begin(); bIt != func.valid_end(); ++bIt) {
    for (auto &stmt : (*bIt)->GetStmtNodes()) {
      ApplyUnionForCopies(stmt);
    }
  }
  UnionAddrofOstOfUnionFields();
  CreateAssignSets();
  if (enabledDebug) {
    DumpAssignSets();
  }
  ReinitUnionFind();
  if (MeOption::noSteensgaard) {
    UnionAllPointedTos();
  } else {
    ApplyUnionForPointedTos();
    if (mirModule.IsJavaModule()) {
      UnionForNotAllDefsSeen();
    } else {
      UnionForNotAllDefsSeenCLang();
    }
    if (mirModule.IsCModule()) {
      ApplyUnionForStorageOverlaps();
    }
  }
  if (!mirModule.IsJavaModule()) {
    UnionForAggAndFields();
  }
  // TBAA
  if (!MeOption::noTBAA && mirModule.IsJavaModule()) {
    ReconstructAliasGroups();
  }
  UnionNextLevelOfAliasOst();
  CreateClassSets();
  if (enabledDebug) {
    DumpClassSets();
  }
  // pass 2 through the program statements
  if (enabledDebug) {
    LogInfo::MapleLogger() << "\n============ Alias Classification Pass 2 ============" << '\n';
  }

  for (auto bIt = func.valid_begin(); bIt != func.valid_end(); ++bIt) {
    auto *bb = *bIt;
    for (auto &stmt : bb->GetStmtNodes()) {
      GenericInsertMayDefUse(stmt, bb->GetBBId());
    }
  }
}

AnalysisResult *MeDoAliasClass::Run(MeFunction *func, MeFuncResultMgr *funcResMgr, ModuleResultMgr *moduleResMgr) {
  MPLTimer timer;
  timer.Start();
  (void)funcResMgr->GetAnalysisResult(MeFuncPhase_SSATAB, func);
  MemPool *aliasClassMp = NewMemPool();
  KlassHierarchy *kh = nullptr;
  if (func->GetMIRModule().IsJavaModule()) {
    kh = static_cast<KlassHierarchy*>(moduleResMgr->GetAnalysisResult(MoPhase_CHA, &func->GetMIRModule()));
  }
  MeAliasClass *aliasClass = aliasClassMp->New<MeAliasClass>(
      *aliasClassMp, func->GetMIRModule(), *func->GetMeSSATab(), *func, MeOption::lessThrowAlias,
      MeOption::ignoreIPA, DEBUGFUNC(func), MeOption::setCalleeHasSideEffect, kh);
  aliasClass->DoAliasAnalysis();
  timer.Stop();
  if (DEBUGFUNC(func)) {
    LogInfo::MapleLogger() << "ssaTab + aliasClass passes consume cumulatively " << timer.Elapsed() << "seconds "
                           << '\n';
  }
  return aliasClass;
}
}  // namespace maple
