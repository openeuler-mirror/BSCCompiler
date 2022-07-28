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

#ifndef MAPLE_ME_INCLUDE_LFO_IV_CANON_H
#define MAPLE_ME_INCLUDE_LFO_IV_CANON_H

#include "pme_function.h"
#include "me_loop_analysis.h"
#include "me_irmap_build.h"
#include "maple_phase.h"

namespace maple {
// describe characteristics of one IV
class IVDesc {
 public:
  OriginalSt *ost;
  PrimType primType;
  MeExpr *initExpr = nullptr;
  int32 stepValue = 0;
  bool canBePrimary = true;

  explicit IVDesc(OriginalSt *o) : ost(o) {
    MIRType *mirType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(ost->GetTyIdx());
    primType = mirType->GetPrimType();
  }
  virtual ~IVDesc() = default;
};

// this is for processing a single loop
class IVCanon {
 public:
  MemPool *mp;
  MapleAllocator alloc;
  MeFunction *func;
  Dominance *dominance;
  SSATab *ssatab;
  LoopDesc *aloop;
  uint32 loopID;
  PreMeWhileInfo *whileInfo;
  MapleVector<IVDesc *> ivvec;
  int32 idxPrimaryIV = -1;      // the index in ivvec of the primary IV
  MeExpr *tripCount = nullptr;

  IVCanon(MemPool *m, MeFunction *f, Dominance *dom, LoopDesc *ldesc, uint32 id, PreMeWhileInfo *winfo)
      : mp(m), alloc(m), func(f), dominance(dom), ssatab(f->GetMeSSATab()),
        aloop(ldesc), loopID(id), whileInfo(winfo), ivvec(alloc.Adapter()) {}
  virtual ~IVCanon() = default;
  bool ResolveExprValue(MeExpr *x, ScalarMeExpr *phiLHS);
  int32 ComputeIncrAmt(MeExpr *x, ScalarMeExpr *phiLHS, int32 *appearances, bool &canBePrimary);
  void CharacterizeIV(ScalarMeExpr *initversion, ScalarMeExpr *loopbackversion, ScalarMeExpr *philhs);
  void FindPrimaryIV();
  bool IsLoopInvariant(MeExpr *x);
  void CanonEntryValues();
  bool CheckPostIncDecFixUp(CondGotoMeStmt *condbr);
  void ComputeTripCount();
  void CanonExitValues();
  void ReplaceSecondaryIVPhis();
  void PerformIVCanon();
  std::string PhaseName() const { return "ivcanon"; }
};

MAPLE_FUNC_PHASE_DECLARE(MELfoIVCanon, MeFunction)
}  // namespace maple
#endif  // MAPLE_ME_INCLUDE_LFO_IV_CANON_H
