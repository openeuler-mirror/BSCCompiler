/*
 * Copyright (c) [2019-2021] Huawei Technologies Co.,Ltd.All rights reserved.
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
#include "me_ssa_tab.h"
#include "me_function.h"
#include "mpl_timer.h"
#include "demand_driven_alias_analysis.h"
#include "me_dominance.h"
#include "class_hierarchy_phase.h"
#include "me_phase_manager.h"

namespace maple {
// This phase performs alias analysis based on Steensgaard's algorithm and
// represent the resulting alias relationships in the Maple IR representation
bool MeAliasClass::HasWriteToStaticFinal() const {
  for (auto bIt = cfg->valid_begin(); bIt != cfg->valid_end(); ++bIt) {
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

void MeAliasClass::PerformDemandDrivenAliasAnalysis() {
  if (MeOption::noDDAA) {
    return;
  }
  if (!mirModule.IsCModule()) {
    return;
  }

  DemandDrivenAliasAnalysis ddAlias(&func, func.GetMeSSATab(), localMemPool, enabledDebug);
  for (auto ae : Id2AliasElem()) {
    if (ae->GetClassSet() != nullptr) {
      auto *oldAliasSet = ae->GetClassSet();
      MapleSet<unsigned int> *newAliasSet = nullptr;
      for (auto otherId : *ae->GetClassSet()) {
        auto aliasedAe = Id2AliasElem()[otherId];
        if (aliasedAe == ae) {
          continue;
        }
        bool alias = (ae->GetClassID() < aliasedAe->GetClassID())
            ? ddAlias.MayAlias(&ae->GetOriginalSt(), &Id2AliasElem()[otherId]->GetOriginalSt())
            : (aliasedAe->GetClassSet()->find(ae->GetClassID()) != aliasedAe->GetClassSet()->end());
        if (!alias) {
          if (newAliasSet == nullptr) {
            newAliasSet =
                GetMapleAllocator().GetMemPool()->New<MapleSet<unsigned int>>(GetMapleAllocator().Adapter());
            newAliasSet->insert(oldAliasSet->begin(), oldAliasSet->end());
          }
          newAliasSet->erase(otherId);
        }
      }
      if (newAliasSet != nullptr) {
        ae->SetClassSet(newAliasSet);
      }
    }
  }
}

void MeAliasClass::DoAliasAnalysis() {
  // pass 1 through the program statements
  for (auto bIt = cfg->valid_begin(); bIt != cfg->valid_end(); ++bIt) {
    for (auto &stmt : (*bIt)->GetStmtNodes()) {
      ApplyUnionForCopies(stmt);
    }
  }
  ApplyUnionForFieldsInCopiedAgg();
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
  }
  // TBAA
  if (!MeOption::noTBAA && mirModule.IsJavaModule()) {
    ReconstructAliasGroups();
  }
  CreateClassSets();
  PerformDemandDrivenAliasAnalysis();
  if (enabledDebug) {
    if (!MeOption::noDDAA) {
      ReinitUnionFind();
    }
    DumpClassSets();
  }
  // pass 2 through the program statements
  if (enabledDebug) {
    LogInfo::MapleLogger() << "\n============ Alias Classification Pass 2 ============" << '\n';
  }

  for (auto bIt = cfg->valid_begin(); bIt != cfg->valid_end(); ++bIt) {
    auto *bb = *bIt;
    for (auto &stmt : bb->GetStmtNodes()) {
      GenericInsertMayDefUse(stmt, bb->GetBBId());
    }
  }
}

bool MEAliasClass::PhaseRun(maple::MeFunction &f) {
  MPLTimer timer;
  timer.Start();
  KlassHierarchy *kh = nullptr;
  if (f.GetMIRModule().IsJavaModule()) {
    MaplePhase *it = GetAnalysisInfoHook()->GetTopLevelAnalyisData<M2MKlassHierarchy, MIRModule>(f.GetMIRModule());
    kh = static_cast<M2MKlassHierarchy *>(it)->GetResult();
  }
  aliasClass = GetPhaseAllocator()->New<MeAliasClass>(*GetPhaseMemPool(), *ApplyTempMemPool(), f.GetMIRModule(),
                                                      *f.GetMeSSATab(), f, MeOption::lessThrowAlias,
                                                      MeOption::ignoreIPA, DEBUGFUNC_NEWPM(f),
                                                      MeOption::setCalleeHasSideEffect, kh);
  aliasClass->DoAliasAnalysis();
  timer.Stop();
  if (DEBUGFUNC_NEWPM(f)) {
    LogInfo::MapleLogger() << "ssaTab + aliasClass passes consume cumulatively " << timer.Elapsed() << "seconds "
                           << '\n';
  }
  return true;
}

void MEAliasClass::GetAnalysisDependence(maple::AnalysisDep &aDep) const {
  aDep.AddRequired<MESSATab>();
  aDep.SetPreservedAll();
}
}  // namespace maple
