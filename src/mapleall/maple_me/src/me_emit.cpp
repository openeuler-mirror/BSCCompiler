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
#include "me_emit.h"
#include <mutex>
#include "mir_function.h"
#include "thread_env.h"
#include "me_bb_layout.h"
#include "me_irmap.h"
#include "me_cfg.h"
#include "constantfold.h"
#include "me_merge_stmts.h"

namespace maple {
void MEEmit::GetAnalysisDependence(maple::AnalysisDep &aDep) const {
  aDep.AddRequired<MEBBLayout>();
  aDep.SetPreservedAll();
}

static void ResetDependentedSymbolLive(MIRConst *mirConst) {
  if (mirConst == nullptr) {
    return;
  }

  if (mirConst->GetKind() == kConstAddrof) {
    auto stIdx = static_cast<MIRAddrofConst*>(mirConst)->GetSymbolIndex();
    auto *usedSymbol = theMIRModule->GetMIRBuilder()->GetSymbolFromEnclosingScope(stIdx);
    if (usedSymbol->IsDeleted()) {
      usedSymbol->ResetIsDeleted();
      if (usedSymbol->IsConst()) {
        auto preConst = usedSymbol->GetKonst();
        ResetDependentedSymbolLive(preConst);
      }
    }
  } else if (mirConst->GetKind() == kConstAggConst) {
    for (auto *fldCst : static_cast<MIRAggConst*>(mirConst)->GetConstVec()) {
      ResetDependentedSymbolLive(fldCst);
    }
  }
}

void ResetDependentedSymbolLive(MIRFunction *func) {
  for (size_t k = 1; k < func->GetSymTab()->GetSymbolTableSize(); ++k) {
    MIRSymbol *sym = func->GetSymTab()->GetSymbolFromStIdx(k);
    CHECK_FATAL(sym, "sym is nullptr!");
    if (!sym->IsConst()) {
      continue;
    }
    ResetDependentedSymbolLive(sym->GetKonst());
  }
}

// emit IR to specified file
bool MEEmit::PhaseRun(maple::MeFunction &f) {
  static std::mutex mtx;
  ParallelGuard guard(mtx, ThreadEnv::IsMeParallel());
  if (f.GetCfg()->NumBBs() > 0) {
    CHECK_FATAL(f.HasLaidOut(), "Check bb layout phase.");
    // each phase need to keep either irmap or mirfunction is valid
    if (f.GetIRMap()) {
      if (MeOption::mergeStmts) {
        MergeStmts mergeStmts(f);
        mergeStmts.MergeMeStmts();
      }
      // emit after hssa
      auto layoutBBs = f.GetLaidOutBBs();
      MIRFunction *mirFunction = f.GetMirFunc();
      mirFunction->ReleaseCodeMemory();
      mirFunction->SetMemPool(new ThreadLocalMemPool(memPoolCtrler, "IR from IRMap::Emit()"));
      mirFunction->SetBody(mirFunction->GetCodeMempool()->New<BlockNode>());
      if (Options::profileUse && mirFunction->GetFuncProfData()) {
        mirFunction->GetFuncProfData()->SetStmtFreq(mirFunction->GetBody()->GetStmtID(),
            mirFunction->GetFuncProfData()->entry_freq);
      }
      // initialize is_deleted field to true; will reset when emitting Maple IR
      for (size_t k = 1; k < mirFunction->GetSymTab()->GetSymbolTableSize(); ++k) {
        MIRSymbol *sym = mirFunction->GetSymTab()->GetSymbolFromStIdx(k);
        CHECK_FATAL(sym, "sym is nullptr!");
        if (sym->GetSKind() == kStVar) {
          sym->SetIsDeleted();
        }
      }
      // the formal symbol should be reserved for mplbc mode
      for (FormalDef formalDef : mirFunction->GetFormalDefVec()) {
        if (formalDef.formalSym != nullptr) {
          // in case of __built_va_start functions, whose formalSym are nullptr
          formalDef.formalSym->ResetIsDeleted();
        }
      }
      for (BB *bb : layoutBBs) {
        ASSERT(bb != nullptr, "Check bblayout phase");
        bb->EmitBB(*f.GetMeSSATab(), *mirFunction->GetBody(), false);
        if (bb->IsEmpty()) {
          continue;
        }
        auto &lastStmt = bb->GetStmtNodes().back();
        auto &firstStmt = bb->GetStmtNodes().front();
        f.GetMirFunc()->SetFirstFreqMap(firstStmt.GetStmtID(), bb->GetFrequency());
        f.GetMirFunc()->SetLastFreqMap(lastStmt.GetStmtID(), bb->GetFrequency());
      }
      ResetDependentedSymbolLive(mirFunction);

      bool dumpFreq = MeOption::dumpFunc == f.GetName();
      if (dumpFreq && f.GetMirFunc()->HasFreqMap()) {
        auto &freqMap = f.GetMirFunc()->GetLastFreqMap();
        LogInfo::MapleLogger() << "Dump freqMap:" << std::endl;
        for (BB *bb : layoutBBs) {
          if (!bb->GetStmtNodes().empty()) {
            auto &lastStmt = bb->GetStmtNodes().back();
            auto it = freqMap.find(lastStmt.GetStmtID());
            if (it != freqMap.end()) {
              LogInfo::MapleLogger() << "BB" << bb->GetBBId() << ", freq: " << it->second << std::endl;
            }
          }
        }
      }
    } else {
      // emit from mir function body
      f.EmitBeforeHSSA((*(f.GetMirFunc())), f.GetLaidOutBBs());
    }
    if (!DEBUGFUNC_NEWPM(f) && f.GetIRMap()) {
      // constantfolding does not update BB's stmtNodeList, which breaks MirCFG::DumpToFile();
      // constantfolding also cannot work on SSANode's
      ConstantFold cf(f.GetMIRModule());
      (void)cf.Simplify(f.GetMirFunc()->GetBody());
    }
    if (DEBUGFUNC_NEWPM(f)) {
      LogInfo::MapleLogger() << "\n==============after meemit =============" << '\n';
      f.GetMirFunc()->Dump();
    }
    if (DEBUGFUNC_NEWPM(f)) {
      f.GetCfg()->DumpToFile("meemit", true);
    }
  }
  if (Options::profileUse && f.GetMirFunc()->GetFuncProfData()) {
    f.GetMirFunc()->SetFuncProfData(nullptr);
  }
  return false;
}

void EmitForIPA::GetAnalysisDependence(maple::AnalysisDep &aDep) const {
  aDep.SetPreservedAll();
}

// emit IR to specified file
bool EmitForIPA::PhaseRun(maple::MeFunction &f) {
  static std::mutex mtx;
  ParallelGuard guard(mtx, ThreadEnv::IsMeParallel());
  if (f.GetCfg()->NumBBs() > 0) {
    if (f.GetIRMap()) {
      // emit after hssa
      MIRFunction *mirFunction = f.GetMirFunc();
      mirFunction->ReleaseCodeMemory();
      mirFunction->SetMemPool(new ThreadLocalMemPool(memPoolCtrler, "IR from IRMap::Emit()"));
      mirFunction->SetBody(mirFunction->GetCodeMempool()->New<BlockNode>());
      // initialize is_deleted field to true; will reset when emitting Maple IR
      for (size_t k = 1; k < mirFunction->GetSymTab()->GetSymbolTableSize(); ++k) {
        MIRSymbol *sym = mirFunction->GetSymTab()->GetSymbolFromStIdx(k);
        if (sym->GetSKind() == kStVar) {
          sym->SetIsDeleted();
        }
      }
      for (BB *bb : f.GetCfg()->GetAllBBs()) {
        if (bb == nullptr) {
          continue;
        }
        bb->EmitBB(*f.GetMeSSATab(), *mirFunction->GetBody(), false);
      }
      ResetDependentedSymbolLive(mirFunction);
    } else {
      // emit from mir function body
      f.EmitBeforeHSSA((*(f.GetMirFunc())), f.GetLaidOutBBs());
    }
  }
  return false;
}
}  // namespace maple
