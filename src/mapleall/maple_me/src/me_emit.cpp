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
      // initialize is_deleted field to true; will reset when emitting Maple IR
      for (size_t k = 1; k < mirFunction->GetSymTab()->GetSymbolTableSize(); ++k) {
        MIRSymbol *sym = mirFunction->GetSymTab()->GetSymbolFromStIdx(k);
        if (sym->GetSKind() == kStVar) {
          sym->SetIsDeleted();
        }
      }
      for (BB *bb : layoutBBs) {
        ASSERT(bb != nullptr, "Check bblayout phase");
        bb->EmitBB(*f.GetMeSSATab(), *mirFunction->GetBody(), false);
      }
    } else {
      // emit from mir function body
      f.EmitBeforeHSSA((*(f.GetMirFunc())), f.GetLaidOutBBs());
    }
    if (!DEBUGFUNC_NEWPM(f) && f.GetIRMap()) {
      // constantfolding does not update BB's stmtNodeList, which breaks MirCFG::DumpToFile();
      // constantfolding also cannot work on SSANode's
      ConstantFold cf(f.GetMIRModule());
      cf.Simplify(f.GetMirFunc()->GetBody());
    }
    if (DEBUGFUNC_NEWPM(f)) {
      LogInfo::MapleLogger() << "\n==============after meemit =============" << '\n';
      f.GetMirFunc()->Dump();
    }
    if (DEBUGFUNC_NEWPM(f)) {
      f.GetCfg()->DumpToFile("meemit", true);
    }
  }
  return false;
}
}  // namespace maple
