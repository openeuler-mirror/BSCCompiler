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
#ifndef MAPLE_ME_INCLUDE_PROP_H
#define MAPLE_ME_INCLUDE_PROP_H
#include <stack>
#include "me_ir.h"
#include "ver_symbol.h"
#include "bb.h"
#include "me_function.h"
#include "dominance.h"
#include "irmap.h"
#include "safe_ptr.h"
#include "me_ssa_update.h"

namespace maple {

enum Propagatability {
  kPropNo,
  kPropOnlyWithInverse,
  kPropYes,
};

class Prop {
 public:
  struct PropConfig {
    bool propagateBase;
    bool propagateIloadRef;
    bool propagateGlobalRef;
    bool propagateFinalIloadRef;
    bool propagateIloadRefNonParm;
    bool propagateAtPhi;
    bool propagateWithInverse;
  };

  Prop(IRMap&, Dominance&, MemPool&, uint32 bbvecsize, const PropConfig &config, uint32 limit = UINT32_MAX);
  virtual ~Prop() = default;

  MeExpr *CheckTruncation(MeExpr *lhs, MeExpr *rhs) const;
  MeExpr &PropVar(VarMeExpr &varmeExpr, bool atParm, bool checkPhi);
  MeExpr &PropReg(RegMeExpr &regmeExpr, bool atParm, bool checkPhi);
  MeExpr &PropIvar(IvarMeExpr &ivarMeExpr);
  void PropUpdateDef(MeExpr &meExpr);
  void PropUpdateChiListDef(const MapleMap<OStIdx, ChiMeNode*> &chiList);
  void PropUpdateMustDefList(MeStmt *mestmt);
  void TraversalBB(BB &bb);
  uint32 GetVstLiveStackVecSize() const {
    return static_cast<uint32>(vstLiveStackVec.size());
  }
  MapleStack<MeExpr *> *GetVstLiveStackVec(uint32 i) {
    return vstLiveStackVec[i];
  }
  void SetCurBB(BB *bb) {
    curBB = bb;
  }

  bool NoPropUnionAggField(const MeStmt *meStmt, const StmtNode *stmt /* for irmap */, const MeExpr *propedRHS) const;

  std::map<OStIdx, std::unique_ptr<std::set<BBId>>> &CandsForSSAUpdate() {
    return candsForSSAUpdate;
  }

 protected:
  virtual void UpdateCurFunction(BB&) const {
  }

  virtual bool LocalToDifferentPU(StIdx, const BB&) const {
    return false;
  }

  Dominance &dom;

  virtual MeExpr &PropMeExpr(MeExpr &meExpr, bool &isproped, bool atParm);

  virtual BB *GetBB(BBId) {
    return nullptr;
  }

  void PropEqualExpr(const MeExpr *replacedExpr, ConstMeExpr *constExpr, BB *fromBB);
  void PropConditionBranchStmt(MeStmt *condBranchStmt);
  virtual void TraversalMeStmt(MeStmt &meStmt);
  void CollectSubVarMeExpr(const MeExpr &expr, std::vector<const MeExpr*> &exprVec) const;
  bool IsVersionConsistent(const std::vector<const MeExpr*> &vstVec,
                           const MapleVector<MapleStack<MeExpr *> *> &vstLiveStack) const;
  bool IvarIsFinalField(const IvarMeExpr &ivarMeExpr) const;
  bool CanBeReplacedByConst(const MIRSymbol &symbol) const;
  int32 InvertibleOccurrences(ScalarMeExpr *scalar, MeExpr *x);
  bool IsFunctionOfCurVersion(ScalarMeExpr *scalar, const ScalarMeExpr *cur);
  Propagatability Propagatable(MeExpr *x, BB *fromBB, bool atParm, bool checkInverse = false,
                               ScalarMeExpr *propagatingScalar = nullptr);
  MeExpr *FormInverse(ScalarMeExpr *v, MeExpr *x, MeExpr *formingExp);
  MeExpr *RehashUsingInverse(MeExpr *x);

  void RecordSSAUpdateCandidate(const OStIdx &ostIdx, const BB &bb) {
    MeSSAUpdate::InsertOstToSSACands(ostIdx, bb, &candsForSSAUpdate);
  }

  IRMap &irMap;
  SSATab &ssaTab;
  MIRModule &mirModule;
  MapleAllocator propMapAlloc;
  MapleVector<MapleStack<MeExpr *> *> vstLiveStackVec;
  MapleVector<bool> bbVisited;  // needed because dominator tree is a DAG in wpo
  BB *curBB = nullptr;          // gives the bb of the traversal
  PropConfig config;
  std::map<OStIdx, std::unique_ptr<std::set<BBId>>> candsForSSAUpdate;
 public:
  uint32 propLimit;
  uint32 propsPerformed = 0;    // count number of copy propagations performed
  bool isLfo = false;           // true during LFO phases
};
}  // namespace maple
#endif  // MAPLE_ME_INCLUDE_PROP_H
