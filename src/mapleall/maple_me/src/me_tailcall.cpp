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

#include "me_tailcall.h"

#include "me_cfg.h"

namespace maple {

static constexpr int kUnvisited = 0;
static constexpr int kUnEscaped = 1;
static constexpr int kEscaped = 2;

TailcallOpt::TailcallOpt(MeFunction &f, MemPool &mempool)
    : AnalysisResult(&mempool),
     func(f), tailcallAlloc(&mempool),
     callCands(tailcallAlloc.Adapter()),
     escapedPoints(f.GetCfg()->NumBBs(), kUnvisited, tailcallAlloc.Adapter()) {}

void TailcallOpt::Walk() {
  auto cfg = func.GetCfg();
  auto entryBB = cfg->GetFirstBB();
  std::vector<BB *> worklist{entryBB};

  // all of the BBs are marked as unvisited at the begining, while found address token in
  // one stmt, marked it's parent BB escaped.
  while (!worklist.empty()) {
    auto currentBB = worklist.back();
    worklist.pop_back();
    const auto &escaped = escapedPoints[currentBB->GetBBId()];
    if (escaped == kUnvisited) {
      WalkTroughBB(*currentBB);
    }

    for (auto bb : currentBB->GetSucc()) {
      auto &escapedNext = escapedPoints[bb->GetBBId()];
      // all unvisited succ BBs are pushed into worklist. And if current BB is escaped, we
      // push the unescaped succs and prop them;
      if (escapedNext < escaped) {
        if (escaped == kEscaped) {
          escapedNext = kEscaped;
        }
        worklist.push_back(bb);
      }
    }
  }

  for (auto call : callCands) {
    // if the parent BB of call has stack address escaped, don't mark it as tailcall
    if (escapedPoints[call->GetBB()->GetBBId()] == kEscaped) {
      continue;
    }
    call->SetMayTailcall();
  }
}

void TailcallOpt::WalkTroughBB(BB &bb) {
  for (auto &stmt : bb.GetMeStmts()) {
    for (size_t opndId = 0; opndId < stmt.NumMeStmtOpnds(); ++opndId) {
      auto opnd = stmt.GetOpnd(opndId);
      // stack memory segment would be token by alloca as well
      if (opnd->GetOp() == OP_alloca) {
        escapedPoints[bb.GetBBId()] = kEscaped;
        return;
      }
      if (opnd->GetOp() == OP_addrof) {
        auto addrExpr = static_cast<AddrofMeExpr *>(opnd);
        auto symbolStorageClass = addrExpr->GetOst()->GetMIRSymbol()->GetStorageClass();
        if (symbolStorageClass == kScAuto || symbolStorageClass == kScFormal) {
          escapedPoints[bb.GetBBId()] = kEscaped;
          return;
        }
      }
    }
    if (kOpcodeInfo.IsCall(stmt.GetOp()) || kOpcodeInfo.IsCallAssigned(stmt.GetOp()) ||
        kOpcodeInfo.IsICall(stmt.GetOp())) {
      callCands.push_back(&stmt);
    }
  }
  // no stack address token, mark bb as unescaped
  escapedPoints[bb.GetBBId()] = kUnEscaped;
}

void METailcall::GetAnalysisDependence(maple::AnalysisDep &aDep) const {
  (void)aDep;
}

bool METailcall::PhaseRun(MeFunction &f) {
  auto opt = GetPhaseAllocator()->New<TailcallOpt>(f, *GetPhaseMemPool());
  opt->Walk();
  return true;
}

}  // namespace maple
