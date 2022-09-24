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
// This phase performs alias analysis based on union-based alias analysis algorithm and
// represent the resulting alias relationships in the Maple IR representation
bool MeAliasClass::HasWriteToStaticFinal() const {
  for (auto bIt = cfg->valid_begin(); bIt != cfg->valid_end(); ++bIt) {
    for (const auto &stmt : (*bIt)->GetStmtNodes()) {
      if (stmt.GetOpCode() == OP_dassign) {
        const auto &dassignNode = static_cast<const DassignNode&>(stmt);
        if (dassignNode.GetStIdx().IsGlobal()) {
          const MIRSymbol *sym = mirModule.CurFunction()->GetLocalOrGlobalSymbol(dassignNode.GetStIdx());
          ASSERT_NOT_NULL(sym);
          if (sym->IsFinal()) {
            return true;
          }
        }
      }
    }
  }
  return false;
}

void MeAliasClass::PerformTBAAForC() {
  if (!mirModule.IsCModule() || !MeOption::tbaa) {
    return;
  }
  for (auto *ost : func.GetMeSSATab()->GetOriginalStTable()) {
    auto *aliasSet = GetAliasSet(*ost);
    if (aliasSet == nullptr) {
      continue;
    }
    auto *oldAliasSet = aliasSet;
    AliasSet *newAliasSet = nullptr;
    for (auto aliasedOstIdx : *oldAliasSet) {
      if (aliasedOstIdx == ost->GetIndex()) {
        continue;
      }
      bool alias = false;
      if (ost->GetIndex() < aliasedOstIdx) {
        auto *aliasedOst = func.GetMeSSATab()->GetOriginalStFromID(OStIdx(aliasedOstIdx));
        alias = TypeBasedAliasAnalysis::MayAliasTBAAForC(ost, aliasedOst);
      } else {
        auto aliasSetOfAliasedOst = GetAliasSet(OStIdx(aliasedOstIdx));
        alias = aliasSetOfAliasedOst == nullptr ||
                (aliasSetOfAliasedOst->find(ost->GetIndex()) != aliasSetOfAliasedOst->end());
      }
      if (alias) {
        continue;
      }
      if (newAliasSet == nullptr) {
        newAliasSet = GetMapleAllocator().GetMemPool()->New<AliasSet>(GetMapleAllocator().Adapter());
        newAliasSet->insert(oldAliasSet->cbegin(), oldAliasSet->cend());
      }
      newAliasSet->erase(aliasedOstIdx);
    }
    if (newAliasSet != nullptr) {
      SetAliasSet(ost->GetIndex(), newAliasSet);
    }
  }
}

void MeAliasClass::PerformDemandDrivenAliasAnalysis() {
  if (!MeOption::ddaa) {
    return;
  }
  if (!mirModule.IsCModule()) {
    return;
  }

  DemandDrivenAliasAnalysis ddAlias(&func, func.GetMeSSATab(), localMemPool, enabledDebug);
  for (auto *ost : func.GetMeSSATab()->GetOriginalStTable()) {
    auto *aliasSet = GetAliasSet(*ost);
    if (aliasSet != nullptr) {
      auto *oldAliasSet = aliasSet;
      AliasSet *newAliasSet = nullptr;
      for (auto aliasedOstIdx : *aliasSet) {
        if (aliasedOstIdx == ost->GetIndex()) {
          continue;
        }

        bool alias = false;
        if (ost->GetIndex() < aliasedOstIdx) {
          auto *aliasedOst = func.GetMeSSATab()->GetOriginalStFromID(OStIdx(aliasedOstIdx));
          alias = ddAlias.MayAlias(ost, aliasedOst);
        } else {
          auto aliasSetOfAliasedOst = GetAliasSet(OStIdx(aliasedOstIdx));
          alias = aliasSetOfAliasedOst == nullptr ||
                  (aliasSetOfAliasedOst->find(ost->GetIndex()) != aliasSetOfAliasedOst->end());
        }
        if (!alias) {
          if (newAliasSet == nullptr) {
            newAliasSet =
                GetMapleAllocator().GetMemPool()->New<AliasSet>(GetMapleAllocator().Adapter());
            newAliasSet->insert(oldAliasSet->cbegin(), oldAliasSet->cend());
          }
          newAliasSet->erase(aliasedOstIdx);
        }
      }
      if (newAliasSet != nullptr) {
        SetAliasSet(ost->GetIndex(), newAliasSet);
      }
    }
  }
}

void MeAliasClass::DoAliasAnalysis() {
  // pass 1 through the program statements
  for (auto bIt = cfg->valid_begin(); bIt != cfg->valid_end(); ++bIt) {
    const auto &phiList = (*bIt)->GetPhiList();
    for (const auto &ost2phi : phiList) {
      ApplyUnionForPhi(ost2phi.second);
    }

    for (auto &stmt : (*bIt)->GetStmtNodes()) {
      ApplyUnionForCopies(stmt);
    }
  }
  ApplyUnionForFieldsInCopiedAgg();
  CreateAssignSets();
  PropagateTypeUnsafe();
  if (enabledDebug) {
    DumpAssignSets();
  }
  ReinitUnionFind();
  if (!MeOption::unionBasedAA) {
    UnionAllPointedTos();
  } else {
    ApplyUnionForPointedTos();
    if (mirModule.IsJavaModule()) {
      UnionForNotAllDefsSeen();
    } else {
      UnionForNotAllDefsSeenCLang();
    }
  }
  // perform TBAA for Java
  if (MeOption::tbaa && mirModule.IsJavaModule()) {
    ReconstructAliasGroups();
  }
  CreateClassSets();
  if (enabledDebug) {
    LogInfo::MapleLogger() << "\n============ Alias Info After UBAA ============\n" << std::endl;
    DumpClassSets();
  }
  PerformTBAAForC();
  if (enabledDebug) {
    LogInfo::MapleLogger() << "\n============ Alias Info After TBAA ============\n" << std::endl;
    DumpClassSets(false); // every element has its own aliasSet
  }
  PerformDemandDrivenAliasAnalysis();
  if (enabledDebug) {
    if (MeOption::ddaa) {
      ReinitUnionFind();
    }
    LogInfo::MapleLogger() << "\n============ Alias Info After DDAA ============\n" << std::endl;
    DumpClassSets(false); // every element has its own aliasSet
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

  SetAnalyzedOstNum(func.GetMeSSATab()->GetOriginalStTableSize());
  TypeBasedAliasAnalysis::ClearOstTypeUnsafeInfo();
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
