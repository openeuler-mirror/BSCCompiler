/*
 * Copyright (c) [2022] Futurewei Technologies, Inc. All rights reverved.
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
#include "cg_profile_use.h"
#include "cg_critical_edge.h"
#include "optimize_common.h"

namespace maplebe {
void CgProfUse::setupProf() {
  FuncProfInfo *funcProf = cgFunc->GetFunction().GetFuncProfData();
  if (funcProf == nullptr) {
    return;
  }

  // Initialize all BBs with freq of stmts within it
  FOR_ALL_BB(bb, cgFunc) {
    bb->InitEdgeProfFreq();
    if (bb->GetFirstStmt() &&
        (static_cast<int64_t>(funcProf->GetStmtFreq(bb->GetFirstStmt()->GetStmtID())) >= 0)) {
      bb->SetProfFreq(funcProf->GetStmtFreq(bb->GetFirstStmt()->GetStmtID()));
    } else if (bb->GetLastStmt() &&
               (static_cast<int64_t>(funcProf->GetStmtFreq(bb->GetLastStmt()->GetStmtID())) >= 0)) {
      bb->SetProfFreq(funcProf->GetStmtFreq(bb->GetLastStmt()->GetStmtID()));
    } else {
#if defined(DEBUG) && DEBUG
      if (!CGOptions::IsQuiet()) {
        LogInfo::MapleLogger() << "BB" << bb->GetId() << " : frequency undetermined\n";
      }
#endif
    }
  }

  // Propagate BB freq to edge freq
  FOR_ALL_BB(bb, cgFunc) {
    for (BB* succ : bb->GetSuccs()) {
      (void) CreateEdge(bb, succ);
    }
  }

  for (Edge *e : GetAllEdges()) {
    if (!(e->status) && (e->src->GetSuccs().size() == 1) && (e->src->GetProfFreq() != 0)) {
      e->frequency = e->src->GetProfFreq();
      e->status = true;
    } else if (!(e->status) && (e->dst->GetPreds().size() == 1) && (e->dst->GetProfFreq() != 0)) {
      e->frequency = e->dst->GetProfFreq();
      e->status = true;
    }
  }

  SetupBB2Edges();

  // Infer frequencies
  InferEdgeFreq();

  // Set freq on succs
  SetSuccsFreq();
}

bool CGProfUse::PhaseRun(maplebe::CGFunc &f) {
  CgProfUse cgprofuse(f, *GetPhaseMemPool());
  cgprofuse.setupProf();

#if defined(DEBUG) && DEBUG
  if (CGOptions::FuncFilter(f.GetName())) {
    DotGenerator::GenerateDot("after-CGProfUse", f, f.GetMirModule(), false, f.GetName());
  }
#endif
  return false;
}

void CGProfUse::GetAnalysisDependence(maple::AnalysisDep &aDep) const {
}

MAPLE_ANALYSIS_PHASE_REGISTER_CANSKIP(CGProfUse, cgprofuse)
}
