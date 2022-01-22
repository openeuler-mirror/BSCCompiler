/*
 * Copyright (c) [2021] Huawei Technologies Co.,Ltd.All rights reserved.
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
#include "me_ivopts.h"
#include "me_irmap.h"
#include "me_scalar_analysis.h"
#include "me_hdse.h"
#include "me_phase_manager.h"
#include "meexpr_use_info.h"
#include "me_ssa_update.h"

namespace maple {
constexpr uint32 kMaxUseCount = 250;
constexpr uint32 kMaxCandidatesPerGroup = 40;
constexpr uint64 kDefaultEstimatedLoopIterNum = 10;
constexpr uint32 kInfinityCost = 0xdeadbeef;
constexpr uint32 kRegCost = 1;
class IVGroup;
class IVUse;
class IVCand;

// the struct of IV
class IV {
 public:
  friend class IVGroup;
  friend class IVUse;
  friend class IVCand;
  friend class IVOptData;
  friend class IVOptimizer;

  IV(MeExpr *setExpr, MeExpr *setBase, MeExpr *setStep, bool setBasic)
      : expr(setExpr), base(setBase), step(setStep), isBasicIV(setBasic) {}
 private:
  MeExpr *expr;  // the expr that the iv holds
  MeExpr *base;  // the base value of the iv
  MeExpr *step;  // the step of the iv
  bool isBasicIV;  // true if the iv is not one of the linear version of other ivs
  bool usedInAddress = false;  // true if the iv is used in any address expr
};

enum IVUseType {
  kUseGeneral,  // use in a non-linear expr
  kUseAddress,  // use as a base of ivar
  kUseCompare   // use as a compare expr
};

// record the use message in the loop of iv
class IVUse {
 public:
  friend class IVOptData;
  friend class IVOptimizer;
  IVUse(MeStmt *s, IV *i)
      : stmt(s), iv(i) {}

 private:
  MeStmt *stmt;  // the stmts that the use belongs to
  IV *iv;  // the iv that the use holds
  IVGroup *group = nullptr;  // the group that the use belongs to
  MeExpr *comparedExpr = nullptr;  // another hand of the compare, used for kUseCompare type
  MeExpr *expr = nullptr;  // the expr that holds the iv
  bool hasField = false;  // use to record whether the address use include field computation
};

// collect uses that can be considered together
class IVGroup {
 public:
  friend class IVOptData;
  friend class IVOptimizer;
  using IVGroupID = utils::Index<IVGroup>;
  void SetID(uint32 idx) {
    id = IVGroupID(idx);
  }
  uint32 GetID() const {
    return static_cast<uint32>(id);
  }

 private:
  std::vector<IVUse*> uses;  // the uses that the group holds
  IVUseType type = kUseGeneral;  // the type of the group, same of all uses
  IVGroupID id = IVGroupID(-1);  // the id of the group
  std::set<uint32> relatedCands;  // record the IVCand.id of every related IVCand
  std::vector<uint32> candCosts;  // record the cost of cand, index is candID
};

enum IncPos {
  kBeforeExitTest,  // increase before exit test, normal case
  kAfterExitTest,   // increase after exit test
  kBeforeUse,       // increase before one of the uses
  kAfterUse,        // increase after one of the uses
  kOriginal         // the candidate is the original iv that we always prefer
};

// the template iv we produce for replacing
class IVCand {
 public:
  friend class IVOptData;
  friend class IVOptimizer;
  using IVCandID = utils::Index<IVCand>;
  IVCand(IV *i, MeExpr *inc) : iv(i), incVersion(inc) {}
  void SetID(uint32 idx) {
    id = IVCandID(idx);
  }
  uint32 GetID() const {
    return static_cast<uint32>(id);
  }

 private:
  IVCandID id = IVCandID(-1);  // the id of the candidate
  IV *iv = nullptr;  // the iv that the candidate holds
  IncPos incPos = kBeforeExitTest;  // where the iv increase
  MeExpr *incVersion = nullptr;  // record the ssa version after inc
  MeExpr *originTmp = nullptr;  // use tmp to record the before-inc version of iv
  uint32 cost = 0;  // the cost of the candidate ifself
  IVUse *inc_use = nullptr;  // the use for before/after increasing, only used for kBeforeUse/kAfterUse
  IV *origIV = nullptr;  // record the orig_iv if this candidate is the expand type of origIV
  bool important = false;  // true if every use should consider this cand.
                           // (some times we will skip considering all cands if there're too many)
};

class CandSet;
// record the data of the IVOptimizer
class IVOptData {
 public:
  friend class IVOptimizer;
  IVOptData(MemPool *m) : mp(m), alloc(m) {}
  void CreateIV(MeExpr *expr, MeExpr *base, MeExpr *step, bool isBasicIV);
  void CreateFakeGroup(MeStmt &stmt, IV &iv, IVUseType type, MeExpr *expr);
  void CreateGroup(MeStmt &stmt, IV &iv, IVUseType type, MeExpr *expr);
  IVCand *CreateCandidate(MeExpr *ivTmp, MeExpr *base, MeExpr *step, MeExpr *inc);
  IV *GetIV(const MeExpr &expr);
  bool IsLoopInvariant(const MeExpr &expr);
 private:
  MemPool *mp;
  MapleAllocator alloc;
  std::vector<IVGroup*> groups;  // record all groups in this loop, use IVGroup.id as index
  std::vector<IVGroup*> fakeGroups;  // record the groups that extracted by pre and no need to compute the cost
  std::vector<IVCand*> cands;  // record all candidates in this loop, use IVCand.id as index
  std::unordered_map<int32, std::vector<IVCand*>> candRecord;  // record candidates with same base,
                                                               // use base.exprID as key
  std::unordered_map<int32, IV*> ivs;  // record all ivs, use exprID as key
  LoopDesc *currLoop = nullptr;  // currently optimized loop
  uint64 iterNum = kDefaultEstimatedLoopIterNum;  // the iterations of current loop
  uint64 realIterNum = -1;  // record the real iternum if we can compute
  std::set<uint32> importantCands;  // record the IVCand.id of every important IVCand
  bool considerAll = false;  // true if we consider all candidates for every use
  CandSet *set = nullptr;  // used to record set
};

class CandSet {
 public:
  friend class IVOptData;
  friend class IVOptimizer;
  uint32 NumIVs();
 private:
  std::vector<IVCand*> chosenCands;  // record cands, use IVGroup.id as index
  std::vector<uint32> candCount;  // record used cand and its count
  uint32 cost = kInfinityCost;  // record set cost
};

class IVOptimizer {
 public:
  IVOptimizer(MemPool &memPool, MeFunction &f, bool enabledDebug, IdentifyLoops *meLoops, Dominance *d)
      : func(f),
        ivoptMP(&memPool),
        ivoptAlloc(&memPool),
        irMap(f.GetIRMap()),
        dumpDetail(enabledDebug),
        cfg(f.GetCfg()),
        loops(meLoops),
        dom(d) {}

  ~IVOptimizer() = default;
  void Run();
  void DumpIV(const IV &iv, int32 indent = 0);
  void DumpGroup(const IVGroup &group);
  void DumpCand(const IVCand &cand);
  void DumpSet(const CandSet &set);
  bool LoopOptimized() const;
  // step1: find basic iv (the minimal inc uint)
  MeExpr *ReplacePhiLhs(OpMeExpr *op, ScalarMeExpr *phiLhs, MeExpr *replace);
  MeExpr *ResolveBasicIV(ScalarMeExpr *backValue, ScalarMeExpr *phiLhs, MeExpr *replace);
  bool CheckBasicIV(MeExpr *solve, ScalarMeExpr *phiLhs);
  bool FindBasicIVs();
  // step2: find all ivs that is the affine form of basic iv, collect iv uses the same time
  bool CreateIVFromAdd(OpMeExpr &op, MeStmt &stmt);
  bool CreateIVFromMul(OpMeExpr &op, MeStmt &stmt);
  bool CreateIVFromSub(OpMeExpr &op, MeStmt &stmt);
  bool CreateIVFromCvt(OpMeExpr &op, MeStmt &stmt);
  bool CreateIVFromIaddrof(OpMeExpr &op, MeStmt &stmt);
  bool FindGeneralIVInExpr(MeStmt &stmt, MeExpr &expr, bool useInAddress = false);
  bool LHSEscape(ScalarMeExpr *lhs);
  void FindGeneralIVInStmt(MeStmt &stmt);
  void FindGeneralIVInPhi(MePhiNode &phi);
  void TraversalLoopBB(BB &bb, std::vector<bool> &bbVisited);
  // step3: create candidates used to replace ivs
  void CreateIVCandidateFromBasicIV(IV &iv);
  MeExpr *StripConstantPart(MeExpr &expr, int64 &offset);
  void CreateIVCandidateFromUse(IVUse &use);
  void CreateIVCandidate();
  // step4: estimate the costs of candidates in every use and its own costs
  void ComputeCandCost();
  int64 ComputeRatioOfStep(MeExpr &candStep, MeExpr &groupStep);
  MeExpr *ComputeExtraExprOfBase(MeExpr &candBase, MeExpr &groupBase, int64 ratio, bool &replaced);
  uint32 ComputeCandCostForGroup(IVCand &cand, IVGroup &group);
  void ComputeGroupCost();
  uint32 ComputeSetCost(CandSet &set);
  // step5: greedily find the best set of candidates in all uses
  void InitSet(bool originFirst);
  void TryOptimize(bool originFirst);
  void FindCandSet();
  void TryReplaceWithCand(CandSet &set, IVCand &cand, std::unordered_map<IVGroup*, IVCand*> &changehange);
  bool OptimizeSet();
  // step6: replace ivs with selected candidates
  bool IsReplaceSameOst(MeExpr *parent, ScalarMeExpr *target);
  MeStmt *GetIncPos();
  void UseReplace();
  // optimization entry
  void ApplyOptimize();

 private:
  MeFunction &func;
  MemPool *ivoptMP;
  MapleAllocator ivoptAlloc;
  MeIRMap *irMap;
  bool dumpDetail;  // dump the detail of the optimization
  MeCFG *cfg;
  IdentifyLoops *loops;
  Dominance *dom;
  MeExprUseInfo *useInfo = nullptr;
  bool optimized = false;
  IVOptData *data = nullptr;  // used to record the messages when processing the loop
  std::map<OStIdx, std::unique_ptr<std::set<BBId>>> ssaupdateCands;
};

void IVOptimizer::DumpIV(const IV &iv, int32 indent) {
  constexpr int32 kIndent8 = 8;
  PrintIndentation(indent);
  LogInfo::MapleLogger() << "IV:\n";
  PrintIndentation(indent);
  LogInfo::MapleLogger() << "  HSSA:\tmx" << iv.expr->GetExprID() << std::endl;
  PrintIndentation(indent);
  LogInfo::MapleLogger() << "  Type: " << GetPrimTypeName(iv.expr->GetPrimType()) << std::endl;
  PrintIndentation(indent);
  LogInfo::MapleLogger() << "  Base: ";
  iv.base->Dump(irMap, indent + kIndent8);
  LogInfo::MapleLogger() << std::endl;
  PrintIndentation(indent);
  LogInfo::MapleLogger() << "  Step: ";
  iv.step->Dump(irMap, indent + kIndent8);
  LogInfo::MapleLogger() << std::endl;
  PrintIndentation(indent);
  LogInfo::MapleLogger() << "  BasicIV: " << (iv.isBasicIV ? "Yes\n" : "No\n");
}

void IVOptimizer::DumpGroup(const IVGroup &group) {
  constexpr int32 kIndent4 = 4;
  LogInfo::MapleLogger() << "Group" << group.id << ":\n"
                         << "  Type: ";
  if (group.type == kUseGeneral) {
    LogInfo::MapleLogger() << "General\n";
  } else if (group.type == kUseAddress){
    LogInfo::MapleLogger() << "Address\n";
  } else {
    LogInfo::MapleLogger() << "Compare\n";
  }
  // dump uses
  for (uint32 i = 0; i < group.uses.size(); ++i) {
    auto *use = group.uses[i];
    LogInfo::MapleLogger() << "  Use" << i << ":\n"
                           << "    At Stmt:\n";
    use->stmt->Dump(irMap);
    DumpIV(*use->iv, kIndent4);
  }
}

void IVOptimizer::DumpCand(const IVCand &cand) {
  constexpr int32 kIndent2 = 2;
  LogInfo::MapleLogger() << "Candidate" << cand.id << ":\n"
                         << "  IV Name:\t%" << static_cast<RegMeExpr*>(cand.iv->expr)->GetRegIdx() << std::endl;
  DumpIV(*cand.iv, kIndent2);
  LogInfo::MapleLogger() << "  Inc Position: ";
  if (cand.incPos == kOriginal) {
    LogInfo::MapleLogger() << "Origin IV\n";
  } else if (cand.incPos == kBeforeExitTest) {
    LogInfo::MapleLogger() << "before exit test\n";
  } else if (cand.incPos == kAfterExitTest) {
    LogInfo::MapleLogger() << "after exit test\n";
  } else if (cand.incPos == kAfterUse) {
    LogInfo::MapleLogger() << "after use: ";
    cand.inc_use->stmt->Dump(irMap);
  } else if (cand.incPos == kBeforeUse) {
    LogInfo::MapleLogger() << "before use: ";
    cand.inc_use->stmt->Dump(irMap);
  }
}

void IVOptimizer::DumpSet(const CandSet &set) {
  LogInfo::MapleLogger() << "  cost: " << set.cost << std::endl
                         << "  cand cost: ";
  uint32 candCost = 0;
  for (uint32 k = 0; k < set.candCount.size(); ++k) {
    if (set.candCount[k] > 0) {
      candCost += data->cands[k]->cost;
    }
  }
  LogInfo::MapleLogger() << candCost << std::endl;
  LogInfo::MapleLogger() << "  group cost: ";
  uint32 groupCost = 0;
  for (uint32 i = 0; i < set.chosenCands.size(); ++i) {
    auto *cand = set.chosenCands[i];
    groupCost += data->groups[i]->candCosts[cand->GetID()];
  }
  LogInfo::MapleLogger() << groupCost << std::endl;
  LogInfo::MapleLogger() << "  chosen candidates: ";
  for (uint32 j = 0; j < set.candCount.size(); ++j) {
    if (set.candCount[j] > 0) {
      LogInfo::MapleLogger() << j << ", ";
    }
  }
  LogInfo::MapleLogger() << std::endl;
  for (uint32 t = 0; t < set.chosenCands.size(); ++t) {
    LogInfo::MapleLogger() << "     group" << data->groups[t]->GetID() << ": cand" << set.chosenCands[t]->GetID()
                           << ", cost:" << data->groups[t]->candCosts[set.chosenCands[t]->GetID()] << std::endl;
  }
}

bool IVOptimizer::LoopOptimized() const {
  return optimized;
}

void IVOptData::CreateIV(MeExpr *expr, MeExpr *base, MeExpr *step, bool isBasicIV) {
  if (ivs.find(expr->GetExprID()) != ivs.end()) {
    return;
  }
  IV *iv = mp->New<IV>(expr, base, step, isBasicIV);
  ivs.emplace(expr->GetExprID(), iv);
}

void IVOptData::CreateFakeGroup(MeStmt &stmt, IV &iv, IVUseType type, MeExpr *expr) {
  IVGroup *group = mp->New<IVGroup>();
  group->SetID(fakeGroups.size());
  IVUse *use = mp->New<IVUse>(&stmt, &iv);
  group->uses.emplace_back(use);
  group->type = type;
  use->group = group;
  use->expr = expr;
  fakeGroups.emplace_back(group);
}

void IVOptData::CreateGroup(MeStmt &stmt, IV &iv, IVUseType type, MeExpr *expr) {
  IVGroup *group = mp->New<IVGroup>();
  group->SetID(groups.size());
  IVUse *use = mp->New<IVUse>(&stmt, &iv);
  group->uses.emplace_back(use);
  group->type = type;
  use->group = group;
  use->expr = expr;
  groups.emplace_back(group);
}

IVCand *IVOptData::CreateCandidate(MeExpr *ivTmp, MeExpr *base, MeExpr *step, MeExpr *inc) {
  auto it = candRecord.find(base->GetExprID());
  if (it != candRecord.end()) {
    for (auto *cand : it->second) {
      if (cand->iv->step == step) {
        // already created
        return cand;
      }
    }
  }

  IV *iv = mp->New<IV>(ivTmp, base, step, false);
  auto *cand = mp->New<IVCand>(iv, inc);
  cand->SetID(cands.size());
  cands.emplace_back(cand);
  candRecord[base->GetExprID()].emplace_back(cand);
  return cand;
}

IV *IVOptData::GetIV(const MeExpr &expr) {
  auto iter = ivs.find(expr.GetExprID());
  return iter == ivs.end() ? nullptr : iter->second;
}

bool IVOptData::IsLoopInvariant(const MeExpr &expr) {
  switch (expr.GetMeOp()) {
    case kMeOpConst:
      return true;
    case kMeOpReg:
    case kMeOpVar: {
      auto *bb = static_cast<const ScalarMeExpr&>(expr).DefByBB();
      auto *phi = static_cast<const ScalarMeExpr&>(expr).GetMePhiDef();
      if (phi != nullptr && phi->GetDefBB() == currLoop->head) {
        for (auto *phiOpnd : phi->GetOpnds()) {
          if (phiOpnd == phi->GetLHS()) {
            return true;
          }
        }
      }
      if (bb != nullptr) {
        return currLoop->loopBBs.count(static_cast<const ScalarMeExpr&>(expr).DefByBB()->GetBBId()) == 0;
      }
      return true;
    }
    default:
      // we hope that other kinds of invariant have been moved outside by epre phase, so we don't care them here.
      return false;
  }
}

// replace the 'phiLHS' with 'replace' so we can the base & step from the inc expr
MeExpr *IVOptimizer::ReplacePhiLhs(OpMeExpr *op, ScalarMeExpr *phiLhs, MeExpr *replace) {
  for (uint32 i = 0; i < op->GetNumOpnds(); ++i) {
    auto *opnd = op->GetOpnd(i);
    switch (opnd->GetMeOp()) {
      case kMeOpConst:
        continue;
      case kMeOpReg:
      case kMeOpVar: {
        auto *scalar = static_cast<ScalarMeExpr*>(opnd);
        if (scalar == phiLhs) {
          return irMap->ReplaceMeExprExpr(*op, *scalar, *replace);
        }
        if (scalar->GetOstIdx() == phiLhs->GetOstIdx()) {
          ASSERT(scalar->GetDefBy() == kDefByStmt, "NYI");
          if (scalar->GetDefStmt()->GetRHS()->GetMeOp() != kMeOpOp) {
            return op;
          }
          auto *replaced = ReplacePhiLhs(static_cast<OpMeExpr*>(scalar->GetDefStmt()->GetRHS()), phiLhs, replace);
          return irMap->ReplaceMeExprExpr(*op, *scalar, *replaced);
        }
        continue;
      }
      case kMeOpOp: {
        auto *opOpnd = static_cast<OpMeExpr*>(opnd);
        auto *replaced = ReplacePhiLhs(opOpnd, phiLhs, replace);
        if (replaced == opOpnd) {
          continue;
        }
        return irMap->ReplaceMeExprExpr(*op, *opnd, *replaced);
      }
      default:
        CHECK_FATAL(false, "should not reach here");
    }
  }
  return op;
}

// try to find the base & step from inc expr
MeExpr *IVOptimizer::ResolveBasicIV(ScalarMeExpr *backValue, ScalarMeExpr *phiLhs, MeExpr *replace) {
  if (backValue->GetDefBy() != kDefByStmt) {
    return nullptr;
  }
  auto *backRhs = backValue->GetDefStmt()->GetRHS();
  if (backRhs->GetMeOp() != kMeOpOp) {
    return nullptr;
  }
  auto replaced = ReplacePhiLhs(static_cast<OpMeExpr*>(backRhs), phiLhs, replace);
  if (replaced == backRhs) {  // replace failed, can not get step
    return nullptr;
  }
  return replaced;
}

// find if there are basic ivs
bool IVOptimizer::CheckBasicIV(MeExpr *solve, ScalarMeExpr *phiLhs) {
  switch (solve->GetMeOp()) {
    case kMeOpVar:
    case kMeOpReg: {
      if (solve->IsVolatile()) {
        return false;
      }
      if (solve == phiLhs) {
        return true;
      }
      auto *scalar = static_cast<ScalarMeExpr*>(solve);
      if (phiLhs == nullptr || scalar->GetOstIdx() != phiLhs->GetOstIdx()) {
        if (scalar->IsDefByNo()) {  // parameter
          return true;
        }
        return data->currLoop->loopBBs.count(scalar->DefByBB()->GetBBId()) == 0;
      }
      if (scalar->GetDefBy() != kDefByStmt) {
        return false;
      }
      return CheckBasicIV(scalar->GetDefStmt()->GetRHS(), phiLhs);
    }
    case kMeOpConst:
      return IsPrimitiveInteger(solve->GetPrimType());
    case kMeOpOp: {
      if (solve->GetOp() == OP_add) {
        auto *opMeExpr = static_cast<OpMeExpr*>(solve);
        return CheckBasicIV(opMeExpr->GetOpnd(0), phiLhs) &&
               CheckBasicIV(opMeExpr->GetOpnd(1), phiLhs);
      }
      if (solve->GetOp() == OP_sub) {
        auto *opMeExpr = static_cast<OpMeExpr*>(solve);
        return CheckBasicIV(opMeExpr->GetOpnd(0), phiLhs) &&
               CheckBasicIV(opMeExpr->GetOpnd(1), nullptr);
      }
      return false;
    }
    default:
      break;
  }
  return false;
}

// check if there are basic ivs in loop, return true if found and created one
bool IVOptimizer::FindBasicIVs() {
  bool find = false;
  auto *loopHeader = data->currLoop->head;
  // basic iv always appears in head's phi list
  for (auto &phi : loopHeader->GetMePhiList()) {
    if (!phi.second->GetIsLive()) {
      continue;
    }
    OriginalSt *ost = func.GetMeSSATab()->GetOriginalStFromID(phi.first);
    if (!ost->IsIVCandidate()) {
      continue;
    }
    auto *phiLhs = phi.second->GetLHS();
    auto *initValue = phi.second->GetOpnd(0);
    auto *backValue = phi.second->GetOpnd(1);
    if (data->currLoop->loopBBs.count(loopHeader->GetPred(0)->GetBBId()) != 0) {
      initValue = phi.second->GetOpnd(1);
      backValue = phi.second->GetOpnd(0);
    }
    if (!CheckBasicIV(backValue, phiLhs)) {
      continue;
    }
    auto *step = ResolveBasicIV(backValue, phiLhs, irMap->CreateIntConstMeExpr(0, phiLhs->GetPrimType()));
    if (step == nullptr || (step->GetMeOp() == kMeOpConst && static_cast<ConstMeExpr*>(step)->IsZero())) {
      continue;
    }
    auto *simplified = irMap->SimplifyMeExpr(step);
    if (simplified != nullptr) { step = simplified; }
    // Create Basic IV
    auto *initDef = initValue->GetDefByMeStmt();
    MeExpr *init = initValue;
    if (initDef != nullptr && initDef->GetRHS() != nullptr && initDef->GetRHS()->GetMeOp() == kMeOpConst) {
      init = initDef->GetRHS();
    }
    data->CreateIV(phiLhs, init, step, true);
    // Create Basic IV for inc version
    auto *backInit = ResolveBasicIV(backValue, phiLhs, init);
    data->CreateIV(backValue, backInit, step, true);
    find = true;
  }
  return find;
}

bool IVOptimizer::CreateIVFromAdd(OpMeExpr &op, MeStmt &stmt) {
  auto *iv0 = data->GetIV(*op.GetOpnd(0));
  auto *iv1 = data->GetIV(*op.GetOpnd(1));
  if (iv0 != nullptr && iv1 != nullptr) {
    auto *initValue = irMap->CreateMeExprBinary(OP_add, op.GetPrimType(),
                                                *iv0->base, *iv1->base);
    auto *simplified = irMap->SimplifyMeExpr(initValue);
    if (simplified != nullptr) {
      initValue = simplified;
    }
    auto *step = irMap->CreateMeExprBinary(OP_add, op.GetPrimType(),
                                           *iv0->step, *iv1->step);
    simplified = irMap->SimplifyMeExpr(step);
    if (simplified != nullptr) {
      step = simplified;
    }
    data->CreateIV(&op, initValue, step, false);
    return true;
  } else if (iv0 != nullptr || iv1 != nullptr) {
    auto *realIV = iv0 != nullptr ? iv0 : iv1;
    auto *opnd = iv0 != nullptr ? op.GetOpnd(1) : op.GetOpnd(0);
    if (data->IsLoopInvariant(*opnd)) {
      auto *initValue = irMap->CreateMeExprBinary(OP_add, op.GetPrimType(),
                                                  *realIV->base, *opnd);
      auto *simplified = irMap->SimplifyMeExpr(initValue);
      if (simplified != nullptr) {
        initValue = simplified;
      }
      data->CreateIV(&op, initValue, realIV->step, false);
      return true;
    }
    // need to record a use of the realIV
    data->CreateGroup(stmt, *realIV, kUseGeneral, &op);
    return false;
  }
  return false;
}

bool IVOptimizer::CreateIVFromMul(OpMeExpr &op, MeStmt &stmt) {
  auto *iv0 = data->GetIV(*op.GetOpnd(0));
  auto *iv1 = data->GetIV(*op.GetOpnd(1));
  if (iv0 != nullptr && iv1 != nullptr) {
    // record use of both iv
    data->CreateGroup(stmt, *iv0, kUseGeneral, &op);
    data->CreateGroup(stmt, *iv1, kUseGeneral, &op);
    return false;
  } else if (iv0 != nullptr || iv1 != nullptr) {
    auto *realIV = iv0 != nullptr ? iv0 : iv1;
    auto *opnd = iv0 != nullptr ? op.GetOpnd(1) : op.GetOpnd(0);
    if (opnd->IsZero()) {
      return false;
    }
    if (data->IsLoopInvariant(*opnd)) {
      auto *initValue = irMap->CreateMeExprBinary(OP_mul, op.GetPrimType(),
                                                  *realIV->base, *opnd);
      auto *simplified = irMap->SimplifyMeExpr(initValue);
      if (simplified != nullptr) {
        initValue = simplified;
      }
      auto *step = irMap->CreateMeExprBinary(OP_mul, op.GetPrimType(),
                                             *realIV->step, *opnd);
      simplified = irMap->SimplifyMeExpr(step);
      if (simplified != nullptr) {
        step = simplified;
      }
      data->CreateIV(&op, initValue, step, false);
      return true;
    }
    // need to record a use of the realIV
    data->CreateGroup(stmt, *realIV, kUseGeneral, &op);
    return false;
  }
  return false;
}

bool IVOptimizer::CreateIVFromSub(OpMeExpr &op, MeStmt &stmt) {
  auto *iv0 = data->GetIV(*op.GetOpnd(0));
  auto *iv1 = data->GetIV(*op.GetOpnd(1));
  if (iv0 != nullptr && iv1 != nullptr) {
    auto *initValue = irMap->CreateMeExprBinary(OP_sub, op.GetPrimType(),
                                                *iv0->base, *iv1->base);
    auto *simplified = irMap->SimplifyMeExpr(initValue);
    if (simplified != nullptr) {
      initValue = simplified;
    }
    auto *step = irMap->CreateMeExprBinary(OP_sub, op.GetPrimType(),
                                           *iv0->step, *iv1->step);
    simplified = irMap->SimplifyMeExpr(step);
    if (simplified != nullptr) {
      step = simplified;
    }
    data->CreateIV(&op, initValue, step, false);
    return true;
  } else if (iv0 != nullptr || iv1 != nullptr) {
    auto *realIV = iv0 != nullptr ? iv0 : iv1;
    auto *opnd = iv0 != nullptr ? op.GetOpnd(1) : op.GetOpnd(0);
    if (data->IsLoopInvariant(*opnd)) {
      auto *initValue = irMap->CreateMeExprBinary(OP_sub, op.GetPrimType(),
                                                  iv0 != nullptr ? *realIV->base : *opnd,
                                                  iv0 != nullptr ? *opnd : *realIV->base);
      auto *simplified = irMap->SimplifyMeExpr(initValue);
      if (simplified != nullptr) {
        initValue = simplified;
      }
      auto *step = realIV->step;
      if (iv0 == nullptr) {
        step = irMap->CreateMeExprUnary(OP_neg, op.GetPrimType(), *step);
        static_cast<OpMeExpr*>(step)->SetOpndType(realIV->step->GetPrimType());
        simplified = irMap->SimplifyMeExpr(step);
        if (simplified != nullptr) {
          step = simplified;
        }
      }
      data->CreateIV(&op, initValue, step, false);
      return true;
    }
    // need to record a use of the realIV
    data->CreateGroup(stmt, *realIV, kUseGeneral, &op);
    return false;
  }
  return false;
}

bool IVOptimizer::CreateIVFromCvt(OpMeExpr &op, MeStmt &stmt) {
  auto *iv = data->GetIV(*op.GetOpnd(0));
  ASSERT_NOT_NULL(iv);
  if (!IsPrimitiveInteger(op.GetPrimType())) {
    data->CreateGroup(stmt, *iv, kUseGeneral, &op);
    return false;
  }
  auto *initValue = irMap->CreateMeExprTypeCvt(op.GetPrimType(), op.GetOpndType(), *iv->base);
  auto *simplified = irMap->SimplifyMeExpr(initValue);
  if (simplified != nullptr) {
    initValue = simplified;
  }
  auto *step = irMap->CreateMeExprTypeCvt(op.GetPrimType(), GetSignedPrimType(op.GetOpndType()), *iv->step);
  simplified = irMap->SimplifyMeExpr(step);
  if (simplified != nullptr) {
    step = simplified;
  }
  data->CreateIV(&op, initValue, step, false);
  return true;
}

bool IVOptimizer::CreateIVFromIaddrof(OpMeExpr &op, MeStmt &stmt) {
  auto *iv = data->GetIV(*op.GetOpnd(0));
  ASSERT_NOT_NULL(iv);
  if (!IsPrimitiveInteger(op.GetPrimType())) {
    data->CreateGroup(stmt, *iv, kUseGeneral, &op);
    return false;
  }
  auto *type = GlobalTables::GetTypeTable().GetTypeFromTyIdx(op.GetTyIdx());
  auto *pointedType = static_cast<MIRPtrType*>(type)->GetPointedType();
  CHECK_FATAL(pointedType != nullptr, "expect a pointed type of iaddrof");
  auto offset = pointedType->GetBitOffsetFromBaseAddr(op.GetFieldID()) / 8;
  auto *inc = irMap->CreateIntConstMeExpr(offset, op.GetPrimType());
  auto *initValue = irMap->CreateMeExprBinary(OP_add, op.GetPrimType(), *iv->base, *inc);
  auto *simplified = irMap->SimplifyMeExpr(initValue);
  if (simplified != nullptr) { initValue = simplified; }
  data->CreateIV(&op, initValue, iv->step, false);
  return true;
}

// find other ivs that is the affine transform of other ivs
bool IVOptimizer::FindGeneralIVInExpr(MeStmt &stmt, MeExpr &expr, bool useInAddress) {
  switch (expr.GetMeOp()) {
    case kMeOpOp: {
      if (data->GetIV(expr) != nullptr) {
        return true;
      }
      auto &op = static_cast<OpMeExpr&>(expr);
      bool opndIsIV = false;
      for (uint32 i = 0; i < op.GetNumOpnds(); ++i) {
        if (FindGeneralIVInExpr(stmt, *op.GetOpnd(i))) {
          opndIsIV = true;
        }
      }
      if (!opndIsIV) {
        return false;
      }
      if (op.GetOp() == OP_add) {
        return CreateIVFromAdd(op, stmt);
      }
      if (op.GetOp() == OP_mul) {
        return CreateIVFromMul(op, stmt);
      }
      if (op.GetOp() == OP_sub) {
        return CreateIVFromSub(op, stmt);
      }
      if (op.GetOp() == OP_cvt) {
        return CreateIVFromCvt(op, stmt);
      }
      if (op.GetOp() == OP_iaddrof) {
        return CreateIVFromIaddrof(op, stmt);
      }
      if (IsCompareHasReverseOp(op.GetOp())) {  // record compare use
        auto *iv = data->GetIV(*op.GetOpnd(0));
        if (iv != nullptr && data->IsLoopInvariant(*op.GetOpnd(1))) {
          data->CreateGroup(stmt, *iv, kUseCompare, &op);
          (*data->groups.rbegin())->uses[0]->comparedExpr = op.GetOpnd(1);
          return false;
        }
        iv = data->GetIV(*op.GetOpnd(1));
        if (iv != nullptr && data->IsLoopInvariant(*op.GetOpnd(0))) {
          data->CreateGroup(stmt, *iv, kUseCompare, &op);
          (*data->groups.rbegin())->uses[0]->comparedExpr = op.GetOpnd(0);
          return false;
        }
        iv = iv == nullptr ? data->GetIV(*op.GetOpnd(0)) : iv;
        data->CreateGroup(stmt, *iv, kUseGeneral, &op);
        return false;
      }
      for (int j = 0; j < op.GetNumOpnds(); ++j) {
        auto *iv = data->GetIV(*op.GetOpnd(j));
        if (iv != nullptr) {
          // create use of the iv
          data->CreateGroup(stmt, *iv, kUseGeneral, &op);
        }
      }
      return false;
    }
    case kMeOpVar:
    case kMeOpReg: {
      auto *iv = data->GetIV(expr);
      if (iv != nullptr) {
        iv->usedInAddress = useInAddress;
        return true;
      }
      return false;
    }
    case kMeOpIvar: {
      auto &ivar = static_cast<IvarMeExpr&>(expr);
      auto *base = ivar.GetBase();
      if (FindGeneralIVInExpr(stmt, *base, true)) {
        auto *iv = data->GetIV(*base);
        data->CreateGroup(stmt, *iv, kUseAddress, &ivar);
        (*(*data->groups.rbegin())->uses.rbegin())->hasField = ivar.GetFieldID() != 0;
      }
      return false;
    }
    case kMeOpNary: {
      auto &nary = static_cast<NaryMeExpr&>(expr);
      for (auto *opnd : nary.GetOpnds()) {
        if (FindGeneralIVInExpr(stmt, *opnd)) {
          auto *iv = data->GetIV(*opnd);
          data->CreateGroup(stmt, *iv, kUseGeneral, &nary);
        }
      }
      return false;
    }
    default:
      return false;
  }
  return false;
}

void IVOptimizer::FindGeneralIVInPhi(MePhiNode &phi) {
  if (!phi.GetIsLive()) {
    return;
  }

  bool allSameIV = true;
  auto *firstOpnd = phi.GetOpnd(0);
  auto firstIV = data->GetIV(*firstOpnd);
  if (firstIV == nullptr) {
    return;
  }
  for (auto *phiOpnd : phi.GetOpnds()) {
    auto *iv = data->GetIV(*phiOpnd);
    if (iv == nullptr) {
      return;
    }
    if (iv->base != firstIV->base || iv->step != firstIV->step) {
      allSameIV = false;
      break;
    }
  }

  if (allSameIV) {
    data->CreateIV(phi.GetLHS(), firstIV->base, firstIV->step, false);
  }
}

// check if the assign escape the loop, if so, we need to keep the value
bool IVOptimizer::LHSEscape(ScalarMeExpr *lhs) {
  if (lhs->IsVolatile()) {  // volatile var needs to keep the value, same as escaping the loop
    return true;
  }
  auto *useList = useInfo->GetUseSitesOfExpr(lhs);
  if (useList == nullptr || useList->empty()) {
    return false;
  }
  // find in all uses to check if the lhs escape the loop
  for (auto &useSite : *useList) {
    if (useSite.IsUseByStmt()) {
      auto *useBB = useSite.GetStmt()->GetBB();
      if (data->currLoop->loopBBs.count(useBB->GetBBId()) == 0) {
        return true;
      }
    }
    if (useSite.IsUseByPhi()) {
      auto *useBB = useSite.GetPhi()->GetDefBB();
      auto *iv = data->GetIV(*lhs);
      if (useBB != data->currLoop->head || !iv->isBasicIV) {
        return true;
      }
    }
  }
  return false;
}

void IVOptimizer::FindGeneralIVInStmt(MeStmt &stmt) {
  for (uint32 i = 0; i < stmt.NumMeStmtOpnds(); ++i) {
    auto *opnd = stmt.GetOpnd(i);
    bool isUsedInAddr = (stmt.GetOp() == OP_iassign || stmt.GetOp() == OP_iassignoff) && i == 0;
    isUsedInAddr |= stmt.GetOp() == OP_assertnonnull;
    if (FindGeneralIVInExpr(stmt, *opnd, isUsedInAddr)) {
      auto *iv = data->GetIV(*opnd);
      if (stmt.GetOp() == OP_regassign || stmt.GetOp() == OP_dassign) {
        // record lhs scalar same (base, step) iv as rhs
        data->CreateIV(stmt.GetLHS(), iv->base, iv->step, false);
        // skip ivs that wont escape the loop
        auto *lhs = static_cast<AssignMeStmt&>(stmt).GetLHS();
        if (!LHSEscape(lhs)) {
          data->CreateFakeGroup(stmt, *iv, kUseGeneral, opnd);
          continue;
        }
      }
      if (stmt.GetOp() == OP_assertnonnull) {
        data->CreateGroup(stmt, *iv, kUseAddress, opnd);
        continue;
      }
      if (isUsedInAddr) {
        data->CreateGroup(stmt, *iv, kUseAddress, static_cast<IassignMeStmt&>(stmt).GetLHSVal());
        (*(*data->groups.rbegin())->uses.rbegin())->hasField =
            static_cast<IassignMeStmt&>(stmt).GetLHSVal()->GetFieldID() != 0;
        continue;
      }
      // record use of iv that is root expr
      data->CreateGroup(stmt, *iv, kUseGeneral, opnd);
    }
  }
}

void IVOptimizer::TraversalLoopBB(BB &bb, std::vector<bool> &bbVisited) {
  if (bbVisited[bb.GetBBId()] || data->currLoop->loopBBs.count(bb.GetBBId()) == 0) {
    return;
  }
  bbVisited[bb.GetBBId()] = true;

  if (&bb != data->currLoop->head) {
    for (auto &phiMap : bb.GetMePhiList()) {
      FindGeneralIVInPhi(*phiMap.second);
    }
  }

  for (auto &stmt : bb.GetMeStmts()) {
    FindGeneralIVInStmt(stmt);
  }

  for (auto childID : dom->GetDomChildren(bb.GetBBId())) {
    TraversalLoopBB(*cfg->GetBBFromID(childID), bbVisited);
  }
}

void IVOptimizer::CreateIVCandidateFromBasicIV(IV &iv) {
  ScalarMeExpr *tmp = nullptr;
  ScalarMeExpr *inc = nullptr;
  MeExpr *base = nullptr;
  MeExpr *step = nullptr;
  IVCand *cand = nullptr;
  // create origin version
  if (iv.expr->GetMeOp() == kMeOpReg || iv.expr->GetMeOp() == kMeOpVar) {
    if (static_cast<ScalarMeExpr*>(iv.expr)->IsDefByPhi()) {
      inc = data->currLoop->preheader == data->currLoop->head->GetPred(0) ?
          static_cast<ScalarMeExpr*>(iv.expr)->GetDefPhi().GetOpnd(1) :
          static_cast<ScalarMeExpr*>(iv.expr)->GetDefPhi().GetOpnd(0);
      cand = data->CreateCandidate(iv.expr, iv.base, iv.step, inc);
      cand->incPos = kOriginal;
      if (cand->iv->expr != iv.expr) {
        // already created a iv with same base & step, use origin one to replace it
        cand->iv->expr = iv.expr;
        cand->incVersion = inc;
      }
      cand->important = true;
    }
  }

  // create unsigned version
  auto type = GetUnsignedPrimType(iv.expr->GetPrimType());
  tmp = irMap->CreateRegMeExpr(type);
  base = irMap->CreateMeExprTypeCvt(type, iv.base->GetPrimType(), *iv.base);
  auto *simplified = irMap->SimplifyMeExpr(base);
  if (simplified != nullptr) {
    base = simplified;
  }
  step = irMap->CreateMeExprTypeCvt(type, iv.step->GetPrimType(), *iv.step);
  simplified = irMap->SimplifyMeExpr(step);
  if (simplified != nullptr) {
    step = simplified;
  }
  inc = irMap->CreateRegMeExprVersion(*tmp);
  cand = data->CreateCandidate(tmp, base, step, inc);
  cand->important = true;

  // create zero-based version
  tmp = irMap->CreateRegMeExpr(type);
  base = irMap->CreateIntConstMeExpr(0, type);
  inc = irMap->CreateRegMeExprVersion(*tmp);
  cand = data->CreateCandidate(tmp, base, step, inc);
  cand->important = true;

  // create address version
  auto addressPTY = GetPrimTypeSize(PTY_ptr) == GetPrimTypeSize(PTY_u64) ? PTY_u64 : PTY_u32;
  tmp = irMap->CreateRegMeExpr(addressPTY);
  base = irMap->CreateMeExprTypeCvt(addressPTY, GetSignedPrimType(iv.base->GetPrimType()), *iv.base);
  simplified = irMap->SimplifyMeExpr(base);
  if (simplified != nullptr) {
    base = simplified;
  }
  step = irMap->CreateMeExprTypeCvt(addressPTY, GetSignedPrimType(iv.step->GetPrimType()), *iv.step);
  simplified = irMap->SimplifyMeExpr(step);
  if (simplified != nullptr) {
    step = simplified;
  }
  inc = irMap->CreateRegMeExprVersion(*tmp);
  cand = data->CreateCandidate(tmp, base, step, inc);
  cand->important = true;
  cand->origIV = &iv;
}

MeExpr *IVOptimizer::StripConstantPart(MeExpr &expr, int64 &offset) {
  int64 offset0 = 0;
  int64 offset1 = 0;
  switch (expr.GetOp()) {
    case OP_add:
    case OP_sub: {
      auto *opnd0 = static_cast<OpMeExpr&>(expr).GetOpnd(0);
      auto *opnd1 = static_cast<OpMeExpr&>(expr).GetOpnd(1);
      auto *op0 = StripConstantPart(*opnd0, offset0);
      auto *op1 = StripConstantPart(*opnd1, offset1);
      offset = expr.GetOp() == OP_add ? (offset0 + offset1) : (offset0 - offset1);
      if (op0 != nullptr && op1 != nullptr) {
        return irMap->CreateMeExprBinary(expr.GetOp(), expr.GetPrimType(), *op0, *op1);
      }
      if (op0 != nullptr) {
        return op0;
      }
      if (op1 != nullptr) {
        return expr.GetOp() == OP_add ? op1 : irMap->CreateMeExprUnary(OP_neg, expr.GetPrimType(), *op1);
      }
      return nullptr;
    }
    case OP_mul: {
      auto *opnd0 = static_cast<OpMeExpr&>(expr).GetOpnd(0);
      auto *opnd1 = static_cast<OpMeExpr&>(expr).GetOpnd(1);
      auto *op0 = StripConstantPart(*opnd0, offset0);
      auto *op1 = StripConstantPart(*opnd1, offset1);
      if (op0 != nullptr && op1 != nullptr) {
        return &expr;
      }
      if (op0 != nullptr) {
        if (op0 == opnd0) {
          return &expr;
        }
        offset = offset1 * offset0;
        return irMap->CreateMeExprBinary(OP_mul, expr.GetPrimType(), *op0,
                                         *irMap->CreateIntConstMeExpr(offset1, expr.GetPrimType()));
      }
      if (op1 != nullptr) {
        if (op1 == opnd1) {
          return &expr;
        }
        offset = offset1 * offset0;
        return irMap->CreateMeExprBinary(OP_mul, expr.GetPrimType(), *op1,
                                         *irMap->CreateIntConstMeExpr(offset0, expr.GetPrimType()));
      }
      offset = offset1 * offset0;
      return nullptr;
    }
    case OP_constval: {
      offset = static_cast<ConstMeExpr&>(expr).GetIntValue();
      return nullptr;
    }
    default: {
      return &expr;
    }
  }
}

void IVOptimizer::CreateIVCandidateFromUse(IVUse &use) {
  auto *tmp = irMap->CreateRegMeExpr(GetUnsignedPrimType(use.iv->expr->GetPrimType()));
  auto *inc = irMap->CreateRegMeExprVersion(*tmp);
  auto *cand = data->CreateCandidate(tmp, use.iv->base, use.iv->step, inc);
  use.group->relatedCands.emplace(cand->GetID());

  // strip constant part of base and create a candidate
  int64 offset = 0;
  auto *newBase = StripConstantPart(*use.iv->base, offset);
  if (newBase == nullptr) {
    return;
  }
  auto simplified = irMap->SimplifyMeExpr(newBase);
  newBase = simplified == nullptr ? newBase : simplified;
  if (newBase != use.iv->base) {
    tmp = irMap->CreateRegMeExpr(GetUnsignedPrimType(use.iv->expr->GetPrimType()));
    inc = irMap->CreateRegMeExprVersion(*tmp);
    cand = data->CreateCandidate(tmp, use.iv->base, use.iv->step, inc);
    use.group->relatedCands.emplace(cand->GetID());
  }
}

void IVOptimizer::CreateIVCandidate() {
  // create (0, +1) iv
  auto *tmp = irMap->CreateRegMeExpr(PTY_u32);
  auto *base = irMap->CreateIntConstMeExpr(0, PTY_u32);
  auto *inc = irMap->CreateRegMeExprVersion(*tmp);
  auto *step = irMap->CreateIntConstMeExpr(1, PTY_u32);
  auto *newCand = data->CreateCandidate(tmp, base, step, inc);
  newCand->important = true;

  tmp = irMap->CreateRegMeExpr(PTY_u64);
  base = irMap->CreateIntConstMeExpr(0, PTY_u64);
  inc = irMap->CreateRegMeExprVersion(*tmp);
  step = irMap->CreateIntConstMeExpr(1, PTY_u64);
  newCand = data->CreateCandidate(tmp, base, step, inc);
  newCand->important = true;
  // create candidate from basic IV
  for (auto &itIV : data->ivs) {
    auto *iv = itIV.second;
    if (iv->isBasicIV) {
      CreateIVCandidateFromBasicIV(*iv);
    }
  }

  // add all candidate created from basic iv to all groups
  for (auto *group : data->groups) {
    for (auto *cand : data->cands) {
      group->relatedCands.emplace(cand->GetID());
    }
  }

  std::map<MeExpr*, std::vector<IVUse*>> offsetCount;
  // create candidate from use
  for (auto *group : data->groups) {
    // just need to consider the first use
    CreateIVCandidateFromUse(*group->uses[0]);

    // apart base
    auto ivExpr = group->uses[0]->iv->expr;
    if (ivExpr->GetOp() == OP_add) {
      auto *opExpr = static_cast<OpMeExpr*>(ivExpr);
      MeExpr *aparted = nullptr;
      if (data->IsLoopInvariant(*opExpr->GetOpnd(0))) {
        aparted = opExpr->GetOpnd(1);
      } else if (data->IsLoopInvariant(*opExpr->GetOpnd(1))) {
        aparted = opExpr->GetOpnd(0);
      }
      if (aparted != nullptr) {
        offsetCount.try_emplace(aparted, std::vector<IVUse*>());
        offsetCount[aparted].emplace_back(group->uses[0]);
      }
    }
  }

  // create candidate from common offset
  for (auto &it : offsetCount) {
    if (it.second.size() > 1) {
      tmp = irMap->CreateRegMeExpr(it.first->GetPrimType());
      auto *iv = data->GetIV(*it.first);
      CHECK_FATAL(iv != nullptr, "common offset must be a iv");
      inc = irMap->CreateRegMeExprVersion(*tmp);
      auto *cand = data->CreateCandidate(tmp, iv->base, iv->step, inc);
      for (auto *use : it.second) {
        use->group->relatedCands.emplace(cand->GetID());
      }
    }
  }

  // create candidate from fakeGroup
  for (auto *group : data->fakeGroups) {
    // just need to consider the first use
    CreateIVCandidateFromUse(*group->uses[0]);
  }
}

static uint32 ComputeExprCost(MeExpr &expr, MeExpr *parent = nullptr) {
#ifndef TARGAARCH64
  return 0;
#endif
  // initialize cost
  static uint32 regCost = 0;
  static uint32 constCost = 4;
  static uint32 cvtCost = 4;
  static uint32 addCost = 4;
  static uint32 addressCost = 5;
  static uint32 mulCost = 5;
  static uint32 symbolCost = 9;
  static uint32 defaultCost = 16;

  switch (expr.GetMeOp()) {
    case kMeOpReg:
      return regCost;
    case kMeOpVar: {
      if (static_cast<VarMeExpr&>(expr).HasAddressValue()) {
        return addressCost;
      }
      return symbolCost;
    }
    case kMeOpConst:
      return constCost;
    case kMeOpOp: {
      uint32 op0Cost = 0;
      uint32 op1Cost = 0;
      auto &op = static_cast<OpMeExpr&>(expr);
      if (op.GetOp() == OP_mul) {
        op0Cost = ComputeExprCost(*op.GetOpnd(0), &op);
        op1Cost = ComputeExprCost(*op.GetOpnd(1), &op);
        if (parent != nullptr) {
          if (parent->GetOp() == OP_sub || parent->GetOp() == OP_add) {
            return op0Cost + op1Cost + 1;
          }
        }
        return op0Cost + op1Cost + mulCost;
      }
      if (op.GetOp() == OP_add || op.GetOp() == OP_sub) {
        op0Cost = ComputeExprCost(*op.GetOpnd(0), &op);
        op1Cost = ComputeExprCost(*op.GetOpnd(1), &op);
        return op0Cost + op1Cost + addCost;
      }
      if (op.GetOp() == OP_cvt || op.GetOp() == OP_retype) {
        op0Cost = ComputeExprCost(*op.GetOpnd(0), &op);
        if (GetPrimTypeSize(op.GetOpndType()) <= GetPrimTypeSize(op.GetPrimType())) {
          return op0Cost;
        }
        return op0Cost + cvtCost;
      }
    }
    default:
      return defaultCost;
  }
}

static void CountElement(MeExpr &expr, std::vector<uint32> &record) {
  switch (expr.GetMeOp()) {
    case kMeOpVar: {
      if (record[0] == 0) { ++record[0]; }
      return;
    }
    case kMeOpReg: {
      if (record[1] == 0) { ++record[1]; }
      return;
    }
    case kMeOpConst: {
      if (record[2] == 0) { ++record[2]; }
      return;
    }
    case kMeOpOp: {
      auto &op = static_cast<OpMeExpr&>(expr);
      auto opop = op.GetOp();
      if (opop == OP_add || opop == OP_mul || opop == OP_sub) {
        CountElement(*op.GetOpnd(0), record);
        CountElement(*op.GetOpnd(1), record);
      }
      return;
    }
    default:
      return;
  }
}

static uint32 ComputeAddressCost(MeExpr *expr, int64 ratio, bool hasField) {
  // cost model
  //  index costs 1
  //  var + index costs 2
  //  reg + index costs 1
  //  var + reg + index costs 6
  //  cst + index costs 1
  //  var + cst + index costs 5
  //  reg + cst + index costs 4
  //  var + reg + cst + index costs 9
  //
  //  rat * index costs 4
  //  var + rat * index costs 2
  //  reg + rat * index costs 1
  //  var + reg + rat * index costs 6
  //  cst + rat * index costs 12
  //  var + cst + rat * index costs 9
  //  reg + cst + rat * index costs 8
  //  var + reg + cst + rat * index costs 13
  // matrix[var][reg][cst][rat]
  static const uint32 addressCostMatrix[2][2][2][2] = {
      {{{1, 4}, {1, 12}}, {{1, 1}, {4, 8}}}, {{{2, 2}, {5, 9}},{{6, 6}, {9, 13}}}
  };
  bool ratioCombine = ratio == 1 || ratio == 2 || ratio == 4 || ratio == 8;
  std::vector<uint32> count = {0, 0, 0};
  if (hasField) {
    count[2] = 1;
  }
  if (expr != nullptr) {
    CountElement(*expr, count);
  }
  return addressCostMatrix[count[0]][count[1]][count[2]][ratio ? 0 : 1] + (ratioCombine ? 0 : 1);
}

void IVOptimizer::ComputeCandCost() {
  for (auto *cand : data->cands) {
    uint32 baseCost = ComputeExprCost(*cand->iv->base);
    if (baseCost == 0) {
      // reg generally need a copy
      baseCost = 1;
    }
#ifdef TARGAARCH64
    uint32 stepCost = 4;
#else
    uint32 stepCost = 0;
#endif
    uint32 cost = baseCost / data->iterNum + stepCost;  // average baseCost to per loop iter and add iv inc cost
    if (cand->incPos != kOriginal) {
      // we prefer original iv because it is closer to the source code, but we wont give too much weight for this.
      ++cost;
    }
    if (cand->incPos == kAfterExitTest && data->currLoop->latch->IsMeStmtEmpty()) {
      // do not insert stmt to empty bb, or we need to insert an extra jump stmt
      ++cost;
    }
    cand->cost = cost;
  }
}

static void FindScalarFactor(MeExpr &expr, std::unordered_map<int32, std::pair<MeExpr*, int64>> &record,
                             int64 multiplier) {
  switch (expr.GetMeOp()) {
    case kMeOpConst: {
      int64 constVal = static_cast<ConstMeExpr&>(expr).GetIntValue();
      auto it = record.find(kInvalidExprID);
      if (it == record.end()) {
        record.emplace(kInvalidExprID, std::make_pair(nullptr, multiplier * constVal));
      } else {
        it->second.second += (multiplier * constVal);
      }
      return;
    }
    case kMeOpReg:
    case kMeOpVar: {
      auto it = record.find(expr.GetExprID());
      if (it == record.end()) {
        record.emplace(expr.GetExprID(), std::make_pair(&expr, multiplier));
      } else {
        it->second.second += multiplier;
      }
      return;
    }
    case kMeOpOp: {
      auto &op = static_cast<OpMeExpr&>(expr);
      if (op.GetOp() == OP_add) {
        FindScalarFactor(*op.GetOpnd(0), record, multiplier);
        FindScalarFactor(*op.GetOpnd(1), record, multiplier);
      } else if (op.GetOp() == OP_sub) {
        FindScalarFactor(*op.GetOpnd(0), record, multiplier);
        FindScalarFactor(*op.GetOpnd(1), record, -multiplier);
      } else if (op.GetOp() == OP_mul) {
        auto *opnd0 = op.GetOpnd(0);
        auto *opnd1 = op.GetOpnd(1);
        if (opnd0->GetMeOp() != kMeOpConst && opnd1->GetMeOp() != kMeOpConst) {
          record.emplace(expr.GetExprID(), std::make_pair(&expr, multiplier));
          return;
        }
        if (opnd0->GetMeOp() == kMeOpConst) {
          FindScalarFactor(*op.GetOpnd(1), record, multiplier * static_cast<ConstMeExpr*>(opnd0)->GetIntValue());
        } else {
          FindScalarFactor(*op.GetOpnd(0), record, multiplier * static_cast<ConstMeExpr*>(opnd1)->GetIntValue());
        }
      } else if (op.GetOp() == OP_cvt) {
        FindScalarFactor(*op.GetOpnd(0), record, multiplier);
      }
      return;
    }
    default:
      return;
  }
}

int64 IVOptimizer::ComputeRatioOfStep(MeExpr &candStep, MeExpr &groupStep) {
  if (candStep.GetMeOp() == kMeOpConst || groupStep.GetMeOp() == kMeOpConst) {
    if (groupStep.GetMeOp() != kMeOpConst || candStep.GetMeOp() != kMeOpConst) {
      // can not compute
      return 0;
    }
    int64 candConst = static_cast<ConstMeExpr&>(candStep).GetIntValue();
    int64 groupConst = static_cast<ConstMeExpr&>(groupStep).GetIntValue();
    if (candStep.GetPrimType() == PTY_u32 || groupStep.GetPrimType() == PTY_u32) {
      candConst = static_cast<int64>(static_cast<int32>(candConst));
      groupConst = static_cast<int64>(static_cast<int32>(groupConst));
    }
    if (candConst == 0) {
      return 0;
    }
    int64 remainder = groupConst % candConst;
    return remainder == 0 ? groupConst / candConst : 0;
  }

  std::unordered_map<int32, std::pair<MeExpr*, int64>> candMap;
  std::unordered_map<int32, std::pair<MeExpr*, int64>> groupMap;
  FindScalarFactor(candStep, candMap, 1);
  FindScalarFactor(groupStep, groupMap, 1);
  int64 commonRatio = 0;
  for (auto &itGroup : groupMap) {
    auto itCand = candMap.find(itGroup.first);
    if (itCand == candMap.end()) {
      return 0;
    }
  }
  for (auto &itCand : candMap) {
    auto itGroup = groupMap.find(itCand.first);
    if (itGroup == groupMap.end()) {
      return 0;
    }
    if (itGroup->second.second == 0 && itCand.second.second == 0) {
      continue;
    } else if (itCand.second.second == 0) {
      return kInfinityCost;
    }
    int64 remainder = itGroup->second.second % itCand.second.second;
    if (remainder != 0) {
      return 0;
    }
    int64 ratio = itGroup->second.second / itCand.second.second;
    if (ratio == 0) {
      return 0;
    }
    if (commonRatio == 0) {
      commonRatio = ratio;
    }
    if (ratio != commonRatio) {
      return 0;
    }
  }
  return commonRatio;
}

MeExpr *IVOptimizer::ComputeExtraExprOfBase(MeExpr &candBase, MeExpr &groupBase, int64 ratio, bool &replaced) {
  std::unordered_map<int32, std::pair<MeExpr*, int64>> candMap;
  std::unordered_map<int32, std::pair<MeExpr*, int64>> groupMap;
  FindScalarFactor(candBase, candMap, 1);
  FindScalarFactor(groupBase, groupMap, 1);
  MeExpr *extraExpr = nullptr;
  int64 candConst = 0;
  int64 groupConst = 0;
  for (auto &itGroup : groupMap) {
    auto itCand = candMap.find(itGroup.first);
    if (itGroup.first == kInvalidExprID) {
      candConst = itCand == candMap.end() ? 0 : itCand->second.second * ratio;
      groupConst = itGroup.second.second;
      continue;
    }
    if (itCand == candMap.end()) {
      MeExpr *constExpr = nullptr;
      MeExpr *expr = itGroup.second.first;
      if (GetPrimTypeSize(expr->GetPrimType()) != GetPrimTypeSize(groupBase.GetPrimType()) ||
          IsSignedInteger(expr->GetPrimType()) != IsSignedInteger(groupBase.GetPrimType())) {
        expr = irMap->CreateMeExprTypeCvt(groupBase.GetPrimType(), expr->GetPrimType(), *expr);
      }
      if (itGroup.second.second != 1) {
        constExpr = irMap->CreateIntConstMeExpr(itGroup.second.second, groupBase.GetPrimType());
        expr = irMap->CreateMeExprBinary(OP_mul, groupBase.GetPrimType(), *expr, *constExpr);
      }
      extraExpr = extraExpr == nullptr ? expr
                                       : irMap->CreateMeExprBinary(OP_add, groupBase.GetPrimType(), *extraExpr, *expr);
    } else {
      int64 newMultiplier = itGroup.second.second - (itCand->second.second * ratio);
      if (newMultiplier == 0) {
        continue;
      }
      MeExpr *constExpr = nullptr;
      MeExpr *expr = itGroup.second.first;
      if (GetPrimTypeSize(expr->GetPrimType()) != GetPrimTypeSize(groupBase.GetPrimType()) ||
          IsSignedInteger(expr->GetPrimType()) != IsSignedInteger(groupBase.GetPrimType())) {
        expr = irMap->CreateMeExprTypeCvt(groupBase.GetPrimType(), expr->GetPrimType(), *expr);
      }
      if (newMultiplier != 1) {
        constExpr = irMap->CreateIntConstMeExpr(newMultiplier, groupBase.GetPrimType());
        expr = irMap->CreateMeExprBinary(OP_mul, groupBase.GetPrimType(), *expr, *constExpr);
      }
      extraExpr = extraExpr == nullptr ? expr
                                       : irMap->CreateMeExprBinary(OP_add, groupBase.GetPrimType(), *extraExpr, *expr);
    }
  }
  auto ptyp = groupBase.GetPrimType();
  if (GetPrimTypeSize(candBase.GetPrimType()) > GetPrimTypeSize(groupBase.GetPrimType())) {
    ptyp = candBase.GetPrimType();
  }
  for (auto &itCand : candMap) {
    auto itGroup = groupMap.find(itCand.first);
    if (itCand.first == kInvalidExprID) {
      candConst = static_cast<uint64>(itCand.second.second) * ratio;
      groupConst = itGroup == groupMap.end() ? 0 : itGroup->second.second;
      continue;
    }
    if (itGroup == groupMap.end()) {
      if (itCand.second.first->GetPrimType() == PTY_ptr ||
          itCand.second.first->GetPrimType() == PTY_a64 ||
          itCand.second.first->GetPrimType() == PTY_a32) {  // it's not good to use one obj to form others
        replaced = false;
        return nullptr;
      }
      auto *constExpr = irMap->CreateIntConstMeExpr(-(itCand.second.second * ratio), ptyp);
      auto *expr = itCand.second.first;
      if (extraExpr != nullptr) {
        if (GetPrimTypeSize(extraExpr->GetPrimType()) != GetPrimTypeSize(ptyp) ||
            IsSignedInteger(extraExpr->GetPrimType()) != IsSignedInteger(ptyp)) {
          extraExpr = irMap->CreateMeExprTypeCvt(ptyp, extraExpr->GetPrimType(), *extraExpr);
        }
      }
      expr = irMap->CreateMeExprBinary(OP_mul, ptyp, *expr, *constExpr);
      extraExpr = extraExpr == nullptr ? expr
                                       : irMap->CreateMeExprBinary(OP_add, ptyp, *extraExpr, *expr);
    }
  }
  if (groupConst - candConst == 0) {
    return extraExpr;
  }
  auto *constExpr = irMap->CreateIntConstMeExpr(groupConst - candConst, ptyp);
  extraExpr = extraExpr == nullptr ? constExpr
                                   : irMap->CreateMeExprBinary(OP_add, ptyp, *extraExpr, *constExpr);
  return extraExpr;
}

static bool CheckOverflow(MeExpr *opnd0, MeExpr *opnd1, Opcode op, PrimType ptyp) {
  // can be extended to scalar later
  if (opnd0->GetMeOp() != kMeOpConst || opnd1->GetMeOp() != kMeOpConst) {
    return true;
  }
  int64 const0 = static_cast<ConstMeExpr*>(opnd0)->GetIntValue();
  int64 const1 = static_cast<ConstMeExpr*>(opnd1)->GetIntValue();
  if (op == OP_add) {
    int64 res = static_cast<int64>(static_cast<uint64>(const0) + static_cast<uint64>(const1));
    if (IsUnsignedInteger(ptyp)) {
      auto shiftNum = 64 - GetPrimTypeBitSize(ptyp);
      return (static_cast<uint64>(res) << shiftNum) >> shiftNum <
             (static_cast<uint64>(const0) << shiftNum) >> shiftNum;
    }
    auto rightShiftNumToGetSignFlag = GetPrimTypeBitSize(ptyp) - 1;
    return (static_cast<uint64>(res) >> rightShiftNumToGetSignFlag !=
            static_cast<uint64>(const0) >> rightShiftNumToGetSignFlag) &&
           (static_cast<uint64>(res) >> rightShiftNumToGetSignFlag !=
            static_cast<uint64>(const1) >> rightShiftNumToGetSignFlag );
  }
  if (op == OP_sub) {
    if (IsUnsignedInteger(ptyp)) {
      return const0 < const1;
    }
    int64 res = static_cast<int64>(static_cast<uint64>(const0) - static_cast<uint64>(const1));
    auto rightShiftNumToGetSignFlag = GetPrimTypeBitSize(ptyp) - 1;
    return (static_cast<uint64>(const0) >> rightShiftNumToGetSignFlag !=
            static_cast<uint64>(const1) >> rightShiftNumToGetSignFlag) &&
           (static_cast<uint64>(res) >> rightShiftNumToGetSignFlag !=
            static_cast<uint64>(const0) >> rightShiftNumToGetSignFlag);
  }
  return true;
}

uint32 IVOptimizer::ComputeCandCostForGroup(IVCand &cand, IVGroup &group) {
  if (GetPrimTypeSize(cand.iv->expr->GetPrimType()) < GetPrimTypeSize(group.uses[0]->iv->expr->GetPrimType())) {
    return kInfinityCost;
  }
  int64 ratio = ComputeRatioOfStep(*cand.iv->step, *group.uses[0]->iv->step);
  bool replaced = true;
  if (ratio == 0) {
    // if the cand is bigger than cmp group, we can also replace it by increasing the other hand of cmp
    if (group.type == kUseCompare) {
      ratio = ComputeRatioOfStep(*group.uses[0]->iv->step, *cand.iv->step);
      if (ratio == 0) {
        return kInfinityCost;
      }
      auto *cmp = static_cast<OpMeExpr*>(group.uses[0]->expr);
      auto isEqNe = cmp->GetOp() == OP_eq || cmp->GetOp() == OP_ne;
      if (!isEqNe && ratio != 1 && ratio != -1) {
        return kInfinityCost;
      }
      MeExpr *extraExpr = ComputeExtraExprOfBase(*group.uses[0]->iv->base, *cand.iv->base, ratio, replaced);
      if (!replaced) {
        return kInfinityCost;
      }
      if (extraExpr == nullptr) {
        return 0;
      }
      return ComputeExprCost(*extraExpr) / data->iterNum;
    }
    return kInfinityCost;
  }

  MeExpr *extraExpr = ComputeExtraExprOfBase(*cand.iv->base, *group.uses[0]->iv->base, ratio, replaced);
  if (!replaced) {
    return kInfinityCost;
  }
  uint32 mulCost = 5;
  if (group.type == kUseGeneral) {
    if (extraExpr == nullptr) {
      return (ratio == 1 ? 0 : mulCost + kRegCost);
    }
    if (extraExpr == cand.iv->step && ratio == 1) {
      return 0;
    }
    uint32 cost = ComputeExprCost(*extraExpr);
    if (extraExpr->GetMeOp() == kMeOpConst) {
      return 4;
    }
    return (cost / data->iterNum) + (ratio == 1 ? mulCost - 1 + kRegCost : mulCost + kRegCost * 2);
  } else if (group.type == kUseAddress) {
    if (GetPrimTypeSize(cand.iv->expr->GetPrimType()) > GetPrimTypeSize(PTY_ptr)) {
      // keep address type
      return kInfinityCost;
    }
    uint32 addressCost = ComputeAddressCost(extraExpr, ratio, group.uses[0]->hasField);
    if (extraExpr == nullptr) {
      return addressCost;
    }
    uint32 cost = ComputeExprCost(*extraExpr);
    return (cost / data->iterNum) + (extraExpr->IsLeaf() ? 0 : kRegCost) + addressCost;
  } else if (group.type == kUseCompare) {
    if (extraExpr == nullptr) {
      return ((ratio == 1 || ratio == -1) ? 0 : mulCost + kRegCost);
    }
    uint32 cost = ComputeExprCost(*extraExpr);
    auto isEqNe = group.uses[0]->expr->GetOp() == OP_eq || group.uses[0]->expr->GetOp() == OP_ne;
    return (cost / data->iterNum) + ((ratio == 1 || (ratio == -1 && isEqNe)) ? 0 : mulCost + kRegCost * 2);
  }
  return kInfinityCost;
}

void IVOptimizer::ComputeGroupCost() {
  for (auto *group : data->groups) {
    for (auto *cand : data->cands) {
      uint32 cost = ComputeCandCostForGroup(*cand, *group);
      group->candCosts.emplace_back(cost);
    }
  }
}

uint32 IVOptimizer::ComputeSetCost(CandSet &set) {
  uint32 cost = 0;
  for (uint32 k = 0; k < set.candCount.size(); ++k) {
    if (set.candCount[k] > 0) {
      cost += (data->cands[k]->cost + kRegCost);
    }
  }
  for (uint32 i = 0; i < set.chosenCands.size(); ++i) {
    auto *cand = set.chosenCands[i];
    cost += data->groups[i]->candCosts[cand->GetID()];
  }
  return cost;
}

void IVOptimizer::InitSet(bool originFirst) {
  auto *init = data->mp->New<CandSet>();
  init->chosenCands.resize(data->groups.size(), nullptr);
  init->candCount.resize(data->cands.size(), 0);
  for (auto *group : data->groups) {
    for (auto *cand : data->cands) {
      if (group->candCosts[cand->GetID()] == kInfinityCost) {
        continue;
      }
      if (init->chosenCands[group->GetID()] == nullptr) {
        init->chosenCands[group->GetID()] = cand;
        ++init->candCount[cand->GetID()];
        continue;
      }
      if (originFirst) {
        if (cand->incPos == kOriginal) {
          if (init->chosenCands[group->GetID()]->incPos != kOriginal) {
            ++init->candCount[cand->GetID()];
            --init->candCount[init->chosenCands[group->GetID()]->GetID()];
            init->chosenCands[group->GetID()] = cand;
            continue;
          }
        } else if (cand->origIV != nullptr) {
          if (init->chosenCands[group->GetID()]->incPos == kOriginal) {
            continue;
          } else if (init->chosenCands[group->GetID()]->origIV != nullptr) {
          } else {
            ++init->candCount[cand->GetID()];
            --init->candCount[init->chosenCands[group->GetID()]->GetID()];
            init->chosenCands[group->GetID()] = cand;
            continue;
          }
        } else {
          if (init->chosenCands[group->GetID()]->incPos == kOriginal ||
              init->chosenCands[group->GetID()]->origIV != nullptr) {
            continue;
          }
        }
      }
      uint32 curCost = group->candCosts[init->chosenCands[group->GetID()]->GetID()];
      uint32 candCost = group->candCosts[cand->GetID()];
      if (candCost < curCost) {
        ++init->candCount[cand->GetID()];
        --init->candCount[init->chosenCands[group->GetID()]->GetID()];
        init->chosenCands[group->GetID()] = cand;
      }
    }
    if (init->chosenCands[group->GetID()] == nullptr ||
        group->candCosts[init->chosenCands[group->GetID()]->GetID()] == kInfinityCost) {
      if (dumpDetail) {
        LogInfo::MapleLogger() << "Can not initial" << (originFirst ? " original set" : "") << "!!!!\n";
      }
      return;
    }
  }
  // try eliminate cand by replacing it with other used cands
  for (auto *cand : data->cands) {
    if (init->candCount[cand->GetID()] >= 1) {
      auto tmpSet = *init;
      std::unordered_map<IVGroup*, IVCand*> tmpChange;
      TryReplaceWithCand(tmpSet, *cand, tmpChange);
      if (tmpSet.cost < init->cost) {
        for (auto &it : tmpChange) {
          ++init->candCount[it.second->GetID()];
          --init->candCount[init->chosenCands[it.first->GetID()]->GetID()];
          init->chosenCands[it.first->GetID()] = it.second;
        }
      }
    }
  }
  init->cost = ComputeSetCost(*init);
  data->set = init;
}

uint32 CandSet::NumIVs() {
  uint32 num = 0;
  for (auto count : candCount) {
    if (count > 0) {
      ++num;
    }
  }
  return num;
}

// try to replace more with the 'cand' and find if there're opportunities
void IVOptimizer::TryReplaceWithCand(CandSet &set, IVCand &cand, std::unordered_map<IVGroup*, IVCand*> &change) {
  std::vector<bool> visitedCands(set.candCount.size(), false);
  for (uint32 i = 0; i < set.chosenCands.size(); ++i) {
    auto *group = data->groups[i];
    auto *chosenCand = set.chosenCands[i];
    if (chosenCand == &cand || visitedCands[chosenCand->GetID()]) {
      continue;
    }
    visitedCands[chosenCand->GetID()] = true;
    if (set.candCount[chosenCand->GetID()] == 1) {
      if (group->candCosts[cand.GetID()] <= group->candCosts[chosenCand->GetID()] + kRegCost + chosenCand->cost) {
        change.insert_or_assign(group, &cand);
        set.chosenCands[i] = &cand;
        set.cost = set.cost - group->candCosts[chosenCand->GetID()] - kRegCost + group->candCosts[cand.GetID()] -
                   chosenCand->cost;
      }
      continue;
    }
    uint32 tmpCost = set.cost;
    std::unordered_map<IVGroup*, IVCand*> tmpChange;
    bool allReplaced = true;
    for (uint32 j = i; j < set.chosenCands.size(); ++j) {
      auto *anotherGroup = data->groups[j];
      auto *anotherCand = set.chosenCands[j];
      if (anotherCand != chosenCand) {
        continue;
      }
      if (anotherGroup->candCosts[cand.GetID()] == kInfinityCost) {
        allReplaced = false;
        continue;
      }
      tmpChange.emplace(anotherGroup, &cand);
      tmpCost = tmpCost - anotherGroup->candCosts[chosenCand->GetID()] + anotherGroup->candCosts[cand.GetID()];
    }
    tmpCost = allReplaced ? tmpCost - kRegCost - chosenCand->cost : tmpCost;
    if (tmpCost <= set.cost) {
      for (auto &it : tmpChange) {
        set.chosenCands[it.first->GetID()] = &cand;
        change.insert_or_assign(it.first, it.second);
      }
      set.cost = tmpCost;
    }
  }
}

bool IVOptimizer::OptimizeSet() {
  auto *set = data->set;
  uint32 bestCost = set->cost;
  uint32 numIVs = set->NumIVs();
  std::unordered_map<IVGroup*, IVCand*> bestChange;
  for (auto *cand : data->cands) {
    if (set->candCount[cand->GetID()] != 0) {
      continue;
    }
    uint32 curCost = set->cost;
    std::unordered_map<IVGroup*, IVCand*> curChange;
    std::vector<uint32> tmpCandCount = set->candCount;
    bool replaced = false;
    for (auto *group : data->groups) {
      auto chosenCand = set->chosenCands[group->GetID()];
      if (group->candCosts[cand->GetID()] <= group->candCosts[chosenCand->GetID()]) {
        curCost = curCost - group->candCosts[chosenCand->GetID()] + group->candCosts[cand->GetID()];
        if (--tmpCandCount[chosenCand->GetID()] == 0) {
          curCost = curCost - kRegCost - chosenCand->cost;
        }
        if (++tmpCandCount[cand->GetID()] == 1) {
          curCost = curCost + kRegCost + cand->cost;
        }
        curChange.emplace(group, cand);
        replaced = true;
      }
    }
    if (replaced && numIVs < 10) {
      auto tmpSet = *set;
      tmpSet.cost = curCost;
      for (auto &it : curChange) {
        tmpSet.chosenCands[it.first->GetID()] = it.second;
      }
      tmpSet.candCount = tmpCandCount;
      TryReplaceWithCand(tmpSet, *cand, curChange);
      curCost = tmpSet.cost;
    }
    if (curCost < bestCost) {
      bestCost = curCost;
      bestChange = curChange;
    }
  }
  if (bestCost == set->cost) {
    return false;
  }
  set->cost = bestCost;
  for (auto &it : bestChange) {
    ++set->candCount[it.second->GetID()];
    --set->candCount[set->chosenCands[it.first->GetID()]->GetID()];
    set->chosenCands[it.first->GetID()] = it.second;
  }
  return true;
}

void IVOptimizer::TryOptimize(bool originFirst) {
  InitSet(originFirst);
  if (data->set != nullptr) {
    if (dumpDetail) {
      LogInfo::MapleLogger() << std::endl << "||||Initial Set||||" << std::endl;
      DumpSet(*data->set);
    }
    while (OptimizeSet()) {
      if (dumpDetail) {
        LogInfo::MapleLogger() << std::endl << "||||Optimized Set||||" << std::endl;
        DumpSet(*data->set);
      }
    }
    // try eliminate cand by replacing it with other used cands
    for (auto *cand : data->cands) {
      if (data->set->candCount[cand->GetID()] >= 1) {
        auto tmpSet = *data->set;
        std::unordered_map<IVGroup*, IVCand*> tmpChange;
        TryReplaceWithCand(tmpSet, *cand, tmpChange);
        if (tmpSet.cost < data->set->cost) {
          for (auto &it : tmpChange) {
            ++data->set->candCount[it.second->GetID()];
            --data->set->candCount[data->set->chosenCands[it.first->GetID()]->GetID()];
            data->set->chosenCands[it.first->GetID()] = it.second;
          }
        }
      }
    }
    if (dumpDetail) {
      LogInfo::MapleLogger() << std::endl << "||||Optimized Set||||" << std::endl;
      DumpSet(*data->set);
    }
  }
}

void IVOptimizer::FindCandSet() {
  // first time start from origin iv
  TryOptimize(true);
  auto *firstSet = data->set;

  // second time start from chosen iv
  TryOptimize(false);
  auto *secondSet = data->set;

  if (firstSet == nullptr && secondSet == nullptr) {
  } else if (firstSet == nullptr) {
    data->set = secondSet;
  } else if (secondSet == nullptr) {
    data->set = firstSet;
  } else {
    if (firstSet->cost > secondSet->cost) {
      data->set = secondSet;
    } else {
      data->set = firstSet;
    }
  }
}

bool IVOptimizer::IsReplaceSameOst(MeExpr *parent, ScalarMeExpr *target) {
  switch (parent->GetMeOp()) {
    case kMeOpReg:
    case kMeOpVar:
      return static_cast<ScalarMeExpr*>(parent)->GetOstIdx() == target->GetOstIdx();
    default: {
      for (uint32 i = 0; i < parent->GetNumOpnds(); ++i) {
        auto *opnd = parent->GetOpnd(i);
        if (IsReplaceSameOst(opnd, target)) {
          return true;
        }
      }
      return false;
    }
  }
}

MeStmt *IVOptimizer::GetIncPos() {
  auto *latchBB = data->currLoop->latch;
  MeStmt *incPos = latchBB->GetLastMe();
  while (incPos != nullptr && (incPos->GetOp() == OP_comment || incPos->GetOp() == OP_goto)) {
    incPos = incPos->GetPrev();
  }
  bool headQuicklyExit = data->currLoop->inloopBB2exitBBs.find(data->currLoop->head->GetBBId()) !=
                         data->currLoop->inloopBB2exitBBs.end();
  if (headQuicklyExit) {
    auto *headFirst = data->currLoop->head->GetFirstMe();
    while (headFirst != nullptr && headFirst->GetOp() == OP_comment) {
      headFirst = headFirst->GetNext();
    }
    if (headFirst == nullptr || !headFirst->IsCondBr()) {
      headQuicklyExit = false;
    }
  }
  // cross the empty and try to find the real pos
  if (incPos == nullptr && latchBB->GetPred().size() == 1 && !headQuicklyExit) {
    auto *pred = latchBB->GetPred(0);
    MeStmt *lastMe = nullptr;
    while (pred != nullptr) {
      bool predIsExit = data->currLoop->inloopBB2exitBBs.find(pred->GetBBId()) !=
                        data->currLoop->inloopBB2exitBBs.end();
      if (pred->GetSucc().size() > 1 && !predIsExit) {
        break;
      }
      lastMe = pred->GetLastMe();
      while (lastMe != nullptr && (lastMe->GetOp() == OP_comment || lastMe->GetOp() == OP_goto)) {
        lastMe = lastMe->GetPrev();
      }
      if (lastMe == nullptr) {
        if (pred->GetPred().size() > 1 || pred->GetSucc().size() > 1) {
          break;
        }
        pred = pred->GetPred(0);
        continue;
      }
      break;
    }
    incPos = lastMe;
  }
  return incPos;
}

void IVOptimizer::UseReplace() {
  std::vector<bool> initedCand(data->cands.size(), false);
  std::unordered_map<int32, MeExpr*> invariables;
  bool replaced = true;
  auto *preheaderLast = data->currLoop->preheader->GetLastMe();
  while (preheaderLast != nullptr && (preheaderLast->GetOp() == OP_comment || preheaderLast->GetOp() == OP_goto)) {
    preheaderLast = preheaderLast->GetPrev();
  }
  auto *latchBB = data->currLoop->latch;
  auto *incPos = GetIncPos();

  for (int i = 0; i < data->groups.size(); ++i) {
    auto *group = data->groups[i];
    auto *cand = data->set->chosenCands[i];
    if (cand->incPos == kOriginal && cand->originTmp == nullptr) {
      cand->originTmp = irMap->CreateRegMeExpr(cand->iv->expr->GetPrimType());
      auto *tmpAssign = irMap->CreateAssignMeStmt(static_cast<ScalarMeExpr&>(*cand->originTmp),
                                                  *cand->iv->expr, *data->currLoop->head);
      data->currLoop->head->AddMeStmtFirst(tmpAssign);
    }
    // replace use
    for (auto *use : group->uses) {
      MeExpr *replace = cand->iv->expr;
      if (cand->incPos == kOriginal) {
        if (IsReplaceSameOst(use->expr, static_cast<ScalarMeExpr*>(cand->iv->expr)) &&
            use->iv->step == cand->iv->step) {
          // no need to replace original iv itself
          continue;
        }
        replace = cand->originTmp;
      }
      bool replaceCompare = false;
      int64 ratio = 0;
      MeExpr *extraExpr = nullptr;
      MeExpr *simplified = nullptr;
      if (group->type == kUseCompare) {
        if (IsSignedInteger(static_cast<OpMeExpr*>(use->expr)->GetOpndType())) {
          static_cast<OpMeExpr*>(use->expr)->SetOpndType(GetSignedPrimType(cand->iv->expr->GetPrimType()));
        }
        ratio = ComputeRatioOfStep(*cand->iv->step, *use->iv->step);
        if (ratio == 0) {
          ratio = ComputeRatioOfStep(*use->iv->step, *cand->iv->step);
          // swap comparison if ratio is negative
          if (ratio < 0) {
            OpMeExpr newOpExpr(static_cast<OpMeExpr&>(*use->expr), kInvalidExprID);
            auto op = newOpExpr.GetOp();
            CHECK_FATAL(IsCompareHasReverseOp(op), "should be known op!");
            auto newOp = op == OP_ge ? OP_le
                                     : op == OP_le ? OP_ge
                                                   : op == OP_lt ? OP_gt
                                                                 : op == OP_gt ? OP_lt
                                                                               : op;
            newOpExpr.SetOp(newOp);
            auto *hashed = irMap->HashMeExpr(newOpExpr);
            (void)irMap->ReplaceMeExprStmt(*use->stmt, *use->expr, *hashed);
            use->expr = hashed;
          }
          if (incPos != nullptr && incPos->IsCondBr() && use->stmt == incPos) {
            // use inc version to replace
            auto *newBase = irMap->CreateMeExprBinary(OP_add, cand->iv->base->GetPrimType(),
                                                      *cand->iv->base, *cand->iv->step);
            extraExpr = ComputeExtraExprOfBase(*use->iv->base, *newBase, ratio, replaced);
            auto *tmp = irMap->ReplaceMeExprExpr(*use->expr, *use->iv->expr, *cand->incVersion);
            (void)irMap->ReplaceMeExprStmt(*use->stmt, *use->expr, *tmp);
            use->expr = tmp;
          } else {
            extraExpr = ComputeExtraExprOfBase(*use->iv->base, *cand->iv->base, ratio, replaced);
            auto *tmp = irMap->ReplaceMeExprExpr(*use->expr, *use->iv->expr, *replace);
            (void)irMap->ReplaceMeExprStmt(*use->stmt, *use->expr, *tmp);
            use->expr = tmp;
          }
          replace = use->comparedExpr;
          replaceCompare = true;
        } else {
          bool mayOverflow = true;
          if (incPos != nullptr && incPos->IsCondBr() && use->stmt == incPos) {
            // use inc version to replace
            auto *newBase = irMap->CreateMeExprBinary(OP_add, cand->iv->base->GetPrimType(),
                                                      *cand->iv->base, *cand->iv->step);
            extraExpr = ComputeExtraExprOfBase(*newBase, *use->iv->base, ratio, replaced);
            replace = cand->incVersion;
          } else {
            extraExpr = ComputeExtraExprOfBase(*cand->iv->base, *use->iv->base, ratio, replaced);
          }
          // move extra computation to comparedExpr
          if (extraExpr != nullptr) {
            if (data->realIterNum == -1) {
              mayOverflow = true;
            } else {
              mayOverflow = CheckOverflow(use->comparedExpr, extraExpr, OP_sub,
                                          extraExpr->GetPrimType());
              if (!mayOverflow) {
                auto *candBase = cand->iv->base;
                auto *candStep = cand->iv->step;
                auto *tmp = irMap->CreateMeExprBinary(OP_mul, candStep->GetPrimType(), *candStep,
                    *irMap->CreateIntConstMeExpr(data->realIterNum, candStep->GetPrimType()));
                simplified = irMap->SimplifyMeExpr(tmp);
                tmp = simplified == nullptr ? tmp : simplified;
                mayOverflow = CheckOverflow(candBase, tmp, OP_add, tmp->GetPrimType()) ||
                              CheckOverflow(candBase, extraExpr, OP_add, extraExpr->GetPrimType()) ||
                              CheckOverflow(tmp, extraExpr, OP_add, extraExpr->GetPrimType());
              }
            }
          }
          if (static_cast<OpMeExpr*>(use->expr)->GetOp() == OP_eq ||
              static_cast<OpMeExpr*>(use->expr)->GetOp() == OP_ne ||
              !mayOverflow) {
            if (extraExpr != nullptr) {
              auto *comparedExpr = use->comparedExpr;
              if (GetPrimTypeSize(extraExpr->GetPrimType()) != GetPrimTypeSize(comparedExpr->GetPrimType()) ||
                  IsSignedInteger(extraExpr->GetPrimType()) != IsSignedInteger(comparedExpr->GetPrimType())) {
                comparedExpr = irMap->CreateMeExprTypeCvt(extraExpr->GetPrimType(),
                                                          comparedExpr->GetPrimType(), *comparedExpr);
              }
              extraExpr = irMap->CreateMeExprBinary(OP_sub, extraExpr->GetPrimType(),
                                                    *comparedExpr, *extraExpr);
              if (extraExpr->GetPrimType() != static_cast<OpMeExpr*>(use->expr)->GetOpndType()) {
                extraExpr = irMap->CreateMeExprTypeCvt(static_cast<OpMeExpr*>(use->expr)->GetOpndType(),
                                                       extraExpr->GetPrimType(), *extraExpr);
              }
              simplified = irMap->SimplifyMeExpr(extraExpr);
              if (simplified != nullptr) { extraExpr = simplified; }
              if (invariables.find(extraExpr->GetExprID()) == invariables.end()) {
                if (!extraExpr->IsLeaf()) {
                  auto *extraReg = irMap->CreateRegMeExpr(extraExpr->GetPrimType());
                  auto *assign = irMap->CreateAssignMeStmt(*extraReg, *extraExpr, *data->currLoop->preheader);
                  if (preheaderLast == nullptr) {
                    data->currLoop->preheader->AddMeStmtFirst(assign);
                  } else {
                    preheaderLast->GetBB()->InsertMeStmtAfter(preheaderLast, assign);
                  }
                  preheaderLast = assign;
                  invariables.emplace(extraExpr->GetExprID(), extraReg);
                  extraExpr = extraReg;
                }
              } else {
                extraExpr = invariables[extraExpr->GetExprID()];
              }
              if (ratio == -1) {
                extraExpr = irMap->CreateMeExprBinary(OP_mul, extraExpr->GetPrimType(), *extraExpr,
                                                      *irMap->CreateIntConstMeExpr(-1, extraExpr->GetPrimType()));
                ratio = 1;
                simplified = irMap->SimplifyMeExpr(extraExpr);
                if (simplified != nullptr) { extraExpr = simplified; }
              }
              auto *tmp = irMap->ReplaceMeExprExpr(*use->expr, *use->comparedExpr, *extraExpr);
              (void)irMap->ReplaceMeExprStmt(*use->stmt, *use->expr, *tmp);
              use->expr = tmp;
              use->comparedExpr = extraExpr;
              extraExpr = nullptr;
            }
          }
          if (use->comparedExpr->GetPrimType() != static_cast<OpMeExpr*>(use->expr)->GetOpndType()) {
            auto *cvt = irMap->CreateMeExprTypeCvt(static_cast<OpMeExpr*>(use->expr)->GetOpndType(),
                                                   use->comparedExpr->GetPrimType(), *use->comparedExpr);
            if (invariables.find(cvt->GetExprID()) == invariables.end()) {
              auto *cvtReg = irMap->CreateRegMeExpr(cvt->GetPrimType());
              auto *assign = irMap->CreateAssignMeStmt(*cvtReg, *cvt, *data->currLoop->preheader);
              if (preheaderLast == nullptr) {
                data->currLoop->preheader->AddMeStmtFirst(assign);
              } else {
                preheaderLast->GetBB()->InsertMeStmtAfter(preheaderLast, assign);
              }
              preheaderLast = assign;
              invariables.emplace(cvt->GetExprID(), cvtReg);
              cvt = cvtReg;
            } else {
              cvt = invariables[cvt->GetExprID()];
            }
            auto *tmp = irMap->ReplaceMeExprExpr(*use->expr, *use->comparedExpr, *cvt);
            (void)irMap->ReplaceMeExprStmt(*use->stmt, *use->expr, *tmp);
            use->expr = tmp;
            use->comparedExpr = cvt;
          }
        }
      } else {
        ratio = ComputeRatioOfStep(*cand->iv->step, *use->iv->step);
        extraExpr = ComputeExtraExprOfBase(*cand->iv->base, *use->iv->base, ratio, replaced);
        if (incPos != nullptr && incPos->IsCondBr() && use->stmt == incPos) {
          if (extraExpr == nullptr || (extraExpr->IsLeaf() && extraExpr->GetMeOp() != kMeOpConst)) {
            auto *tmpExpr = irMap->CreateRegMeExpr(use->expr->GetPrimType());
            auto *assignStmt = irMap->CreateAssignMeStmt(*tmpExpr, *use->expr, *incPos->GetBB());
            incPos->GetBB()->InsertMeStmtBefore(incPos, assignStmt);
            (void)irMap->ReplaceMeExprStmt(*use->stmt, *use->expr, *tmpExpr);
            use->stmt = assignStmt;
          } else {
            // use inc version to replace
            auto *newBase = irMap->CreateMeExprBinary(OP_add, cand->iv->base->GetPrimType(),
                                                      *cand->iv->base, *cand->iv->step);
            extraExpr = ComputeExtraExprOfBase(*newBase, *use->iv->base, ratio, replaced);
            replace = cand->incVersion;
          }
        }
      }
      ASSERT(replaced, "should have been replaced");
      auto realUseType = replace->GetPrimType();
      if ((IsCompareHasReverseOp(use->expr->GetOp()) || use->expr->GetOp() == OP_retype) &&
          static_cast<OpMeExpr*>(use->expr)->GetOpndType() != kPtyInvalid) {
        realUseType = static_cast<OpMeExpr*>(use->expr)->GetOpndType();
      }
      if (extraExpr != nullptr) {
        if (GetPrimTypeSize(extraExpr->GetPrimType()) != GetPrimTypeSize(realUseType) ||
            IsSignedInteger(extraExpr->GetPrimType()) != IsSignedInteger(realUseType)) {
          extraExpr = irMap->CreateMeExprTypeCvt(realUseType, extraExpr->GetPrimType(), *extraExpr);
          simplified = irMap->SimplifyMeExpr(extraExpr);
          if (simplified != nullptr) {
            extraExpr = simplified;
          }
        }
        if (invariables.find(extraExpr->GetExprID()) == invariables.end()) {
          auto *extraReg = irMap->CreateRegMeExpr(extraExpr->GetPrimType());
          auto *assign = irMap->CreateAssignMeStmt(*extraReg, *extraExpr, *data->currLoop->preheader);
          if (preheaderLast == nullptr) {
            data->currLoop->preheader->AddMeStmtFirst(assign);
          } else {
            preheaderLast->GetBB()->InsertMeStmtAfter(preheaderLast, assign);
          }
          preheaderLast = assign;
          invariables.emplace(extraExpr->GetExprID(), extraReg);
          extraExpr = extraReg;
        } else {
          extraExpr = invariables[extraExpr->GetExprID()];
        }
      }
      if (GetPrimTypeSize(replace->GetPrimType()) != GetPrimTypeSize(realUseType) ||
          IsSignedInteger(replace->GetPrimType()) != IsSignedInteger(realUseType)) {
        replace = irMap->CreateMeExprTypeCvt(realUseType, replace->GetPrimType(), *replace);
        simplified = irMap->SimplifyMeExpr(replace);
        if (simplified != nullptr) {
          replace = simplified;
        }
      }
      if (ratio == 1 && extraExpr == nullptr) {
      } else if (ratio == 1) {
        replace = irMap->CreateMeExprBinary(OP_add, realUseType, *extraExpr, *replace);
        simplified = irMap->SimplifyMeExpr(replace);
        if (simplified != nullptr) {
          replace = simplified;
        }
      } else if (extraExpr == nullptr) {
        replace = irMap->CreateMeExprBinary(OP_mul, realUseType,
                                            *irMap->CreateIntConstMeExpr(ratio, replace->GetPrimType()), *replace);
      } else {
        replace = irMap->CreateMeExprBinary(OP_mul, realUseType, *replace,
                                            *irMap->CreateIntConstMeExpr(ratio, replace->GetPrimType()));
        if (replaceCompare) {
          auto *regExtra = static_cast<RegMeExpr*>(extraExpr);
          auto *tmpReplace = irMap->CreateMeExprBinary(OP_add, replace->GetPrimType(),
                                                       *regExtra->GetDefStmt()->GetRHS(), *replace);
          simplified = irMap->SimplifyMeExpr(tmpReplace);
          if (simplified != nullptr) { tmpReplace = simplified; }
          if (tmpReplace->GetDepth() <= regExtra->GetDefStmt()->GetRHS()->GetDepth()) {
            replace = tmpReplace;
          } else {
            replace = irMap->CreateMeExprBinary(OP_add, replace->GetPrimType(), *extraExpr, *replace);
            simplified = irMap->SimplifyMeExpr(replace);
            if (simplified != nullptr) { replace = simplified; }
          }
        } else {
          replace = irMap->CreateMeExprBinary(OP_add, replace->GetPrimType(), *extraExpr, *replace);
          simplified = irMap->SimplifyMeExpr(replace);
          if (simplified != nullptr) { replace = simplified; }
        }
      }
      if (replaceCompare) {
        if (invariables.find(replace->GetExprID()) == invariables.end()) {
          auto *replaceReg = irMap->CreateRegMeExpr(replace->GetPrimType());
          auto *assign = irMap->CreateAssignMeStmt(*replaceReg, *replace, *data->currLoop->preheader);
          if (preheaderLast == nullptr) {
            data->currLoop->preheader->AddMeStmtFirst(assign);
          } else {
            preheaderLast->GetBB()->InsertMeStmtAfter(preheaderLast, assign);
          }
          preheaderLast = assign;
          invariables.emplace(replace->GetExprID(), replaceReg);
          replace = replaceReg;
        } else {
          replace = invariables[replace->GetExprID()];
        }
        auto *tmp = irMap->ReplaceMeExprExpr(*use->expr, *use->comparedExpr, *replace);
        (void)irMap->ReplaceMeExprStmt(*use->stmt, *use->expr, *tmp);
      } else {
        MeExpr *tmp = nullptr;
        if (use->expr == use->iv->expr) {
          tmp = replace;
        } else {
          tmp = irMap->ReplaceMeExprExpr(*use->expr, *use->iv->expr, *replace);
        }
        (void)irMap->ReplaceMeExprStmt(*use->stmt, *use->expr, *tmp);
      }
    }
  }

  for (auto *cand : data->cands) {
    if (data->set->candCount[cand->GetID()] == 0 || cand->incPos == kOriginal) {
      continue;
    }
    // add init
    auto *initVersion = irMap->CreateRegMeExprVersion(static_cast<RegMeExpr &>(*cand->iv->expr));
    auto *initStmt = irMap->CreateAssignMeStmt(static_cast<ScalarMeExpr &>(*initVersion), *cand->iv->base,
                                               *data->currLoop->preheader);
    if (preheaderLast == nullptr) {
      data->currLoop->preheader->AddMeStmtLast(initStmt);
    } else {
      preheaderLast->GetBB()->InsertMeStmtAfter(preheaderLast, initStmt);
    }
    preheaderLast = initStmt;

    // add inc
    auto *latchVersion = static_cast<ScalarMeExpr*>(cand->incVersion);
    auto *incStmt = irMap->CreateAssignMeStmt(*latchVersion,
                                              *irMap->CreateMeExprBinary(OP_add, latchVersion->GetPrimType(),
                                                                         *cand->iv->expr, *cand->iv->step),
                                              *data->currLoop->preheader);
    if (incPos == nullptr) {
      latchBB->AddMeStmtFirst(incStmt);
      incPos = incStmt;
    } else if (incPos->IsCondBr()) {
      incPos->GetBB()->InsertMeStmtBefore(incPos, incStmt);
    } else {
      incPos->GetBB()->InsertMeStmtAfter(incPos, incStmt);
      incPos = incStmt;
    }
    auto *bb = incStmt->GetBB();
    auto ostID = latchVersion->GetOstIdx();
    MeSSAUpdate::InsertOstToSSACands(ostID, *bb, &ssaupdateCands);

    // add phi
    auto *headPhi = irMap->CreateMePhi(static_cast<ScalarMeExpr&>(*cand->iv->expr));
    if (data->currLoop->preheader == data->currLoop->head->GetPred(0)) {
      headPhi->GetOpnds().emplace_back(initVersion);
      headPhi->GetOpnds().emplace_back(latchVersion);
    } else {
      headPhi->GetOpnds().emplace_back(latchVersion);
      headPhi->GetOpnds().emplace_back(initVersion);
    }
    data->currLoop->head->GetMePhiList().emplace(initVersion->GetOstIdx(), headPhi);
    headPhi->SetDefBB(data->currLoop->head);
  }
}

void IVOptimizer::ApplyOptimize() {
  if (dumpDetail) {
    LogInfo::MapleLogger() << "IVOpts's details of func: " << func.GetName() << std::endl
                           << "Processing loop with preheader BB id: " << data->currLoop->preheader->GetBBId()
                           << std::endl;
    if (data->currLoop->inloopBB2exitBBs.size() == 1) {
      auto *testStmt = cfg->GetBBFromID(data->currLoop->inloopBB2exitBBs.begin()->first)->GetLastMe();
      while (testStmt != nullptr && !testStmt->IsCondBr()) {
        testStmt = testStmt->GetPrev();
      }
      if (testStmt != nullptr) {
        LogInfo::MapleLogger() << "    loop has single exit with cond ";
        testStmt->Dump(irMap);
        LogInfo::MapleLogger() << std::endl << std::endl;
      }
    }
  }

  // if there's call in loop, we need to consider the register pressure later,
  // if the pressure is high, choose minimal ivs
  if (!FindBasicIVs()) {
    return;
  }
  // find general ivs
  std::vector<bool> bbVisited(cfg->GetAllBBs().size(), false);
  TraversalLoopBB(*data->currLoop->head, bbVisited);
  if (data->groups.empty()) {
    return;
  }
  // can merge groups with same base obj later
  // too much uses in loop causes little opportunity but huge cost to optimize, just skip it
  if (data->groups.size() >= kMaxUseCount) {
    return;
  }
  if (dumpDetail) {
    LogInfo::MapleLogger() << "||||Induction Variables||||" << std::endl;
    for (auto &itIV : data->ivs) {
      DumpIV(*itIV.second);
    }
    LogInfo::MapleLogger() << std::endl;

    LogInfo::MapleLogger() << "||||Groups||||" << std::endl;
    for (auto *group : data->groups) {
      DumpGroup(*group);
    }
    LogInfo::MapleLogger() << std::endl;
  }
  // create candidates used to replace uses's ivs
  CreateIVCandidate();
  data->considerAll = data->cands.size() <= kMaxCandidatesPerGroup;
  if (dumpDetail) {
    LogInfo::MapleLogger() << "||||Candidates||||" << std::endl;
    for (auto *cand : data->cands) {
      DumpCand(*cand);
    }
    LogInfo::MapleLogger() << std::endl;
    LogInfo::MapleLogger() << "Important Candidate:  ";
    for (auto *cand : data->cands) {
      if (cand->important) {
        LogInfo::MapleLogger() << cand->GetID() << ", ";
      }
    }
    LogInfo::MapleLogger() << std::endl << std::endl;
    LogInfo::MapleLogger() << "Group , Related Cand :\n";
    for (auto *group : data->groups) {
      LogInfo::MapleLogger() << "  Group " << group->GetID() << ":\t";
      for (auto candID : group->relatedCands) {
        LogInfo::MapleLogger() << candID << ", ";
      }
      LogInfo::MapleLogger() << std::endl;
    }
    LogInfo::MapleLogger() << std::endl;
  }

  // compute cost model
  ComputeCandCost();
  ComputeGroupCost();
  if (dumpDetail) {
    LogInfo::MapleLogger() << "||||Cand Cost||||" << std::endl;
    LogInfo::MapleLogger() << "  cand  cost" << std::endl;
    for (auto *cand : data->cands) {
      LogInfo::MapleLogger() << "  " << cand->GetID() << "    " << cand->cost << std::endl;
    }
    LogInfo::MapleLogger() << std::endl;
    LogInfo::MapleLogger() << "||||Group-Candidate Cost||||" << std::endl;
    for (auto *group : data->groups) {
      LogInfo::MapleLogger() << "Group" << group->GetID() << ":\n";
      LogInfo::MapleLogger() << "  cand  cost" << std::endl;
      for (uint32 i = 0; i < group->candCosts.size(); ++i) {
        if (group->candCosts[i] != kInfinityCost) {
          LogInfo::MapleLogger() << "  " << i << "    " << group->candCosts[i] << std::endl;
        }
      }
    }
  }

  FindCandSet();
  if (data->set == nullptr) {
    return;
  }

  optimized = true;
  // replace uses with chosen ivs
  UseReplace();
}

void IVOptimizer::Run() {
  // prepare
  useInfo = &irMap->GetExprUseInfo();
  if (!useInfo->UseInfoOfScalarIsValid()) { // sink only depends on use info of scalar
    useInfo->CollectUseInfoInFunc(irMap, dom, kUseInfoOfScalar);
  }
  for (int32 i = loops->GetMeLoops().size() - 1; i >= 0; i--) {
    auto *loop = loops->GetMeLoops()[i];
    if (loop->head == nullptr || loop->preheader == nullptr || loop->latch == nullptr) {
      // not canonicalized
      continue;
    }
    // remove redundant phi
    for (auto &phi : loop->head->GetMePhiList()) {
      for (uint32 k = 0; k < phi.second->GetOpnds().size(); ++k) {
        auto *phiOpnd = phi.second->GetOpnd(k);
        if (phiOpnd == phi.second->GetLHS()) {
          // replace it by another opnd
          (void)useInfo->ReplaceScalar(irMap, phiOpnd, phi.second->GetOpnd(1 - k));
          phi.second->SetIsLive(false);
        }
      }
    }
  }
  for (int32 i = loops->GetMeLoops().size() - 1; i >= 0; i--) {
    auto *loop = loops->GetMeLoops()[i];
    if (loop->head == nullptr || loop->preheader == nullptr || loop->latch == nullptr) {
      // not canonicalized
      continue;
    }
    if (loop->loopBBs.size() > 60 && loop->nestDepth > 0) {
      // just skip now because we can hardly get register pressure
      continue;
    }
    data = new IVOptData(ivoptMP);
    data->currLoop = loop;
    LoopScalarAnalysisResult sa(*irMap, data->currLoop);
    uint64 tripCount = 0;
    CRNode *conditionCRNode = nullptr;
    CR *itCR = nullptr;
    TripCountType type = sa.ComputeTripCount(func, tripCount, conditionCRNode, itCR);
    if (type == kConstCR) {
      data->iterNum = tripCount;
      data->realIterNum = tripCount;
    }
    if ((loops->GetMeLoops().size() - i) > MeOption::ivoptsLimit) {
      break;
    }
    ApplyOptimize();
  }
  useInfo->InvalidUseInfo();
  MeSSAUpdate ssaUpdate(func, *func.GetMeSSATab(), *dom, ssaupdateCands, *ivoptMP);
  ssaUpdate.Run();
}

void MEIVOpts::GetAnalysisDependence(maple::AnalysisDep &aDep) const {
  aDep.AddRequired<MEMeCfg>();
  aDep.AddRequired<MELoopAnalysis>();
  aDep.AddRequired<MEDominance>();
  aDep.SetPreservedAll();
}

bool MEIVOpts::PhaseRun(maple::MeFunction &f) {
  if (!MeOption::ivopts) {
    return false;
  }
  MeCFG *cfg = GET_ANALYSIS(MEMeCfg, f);
  IdentifyLoops *meLoop = GET_ANALYSIS(MELoopAnalysis, f);
  auto *dom = GET_ANALYSIS(MEDominance, f);
  auto *ivOptimizer = GetPhaseAllocator()->New<IVOptimizer>(*GetPhaseMemPool(), f, DEBUGFUNC_NEWPM(f), meLoop, dom);
  if (DEBUGFUNC_NEWPM(f)) {
    cfg->DumpToFile("beforeIVOpts", false);
    f.Dump();
  }
  ivOptimizer->Run();
  if (ivOptimizer->LoopOptimized()) {
    // run hdse to remove unused exprs
    auto *aliasClass = FORCE_GET(MEAliasClass);
    MeHDSE hdse(f, *dom, *f.GetIRMap(), aliasClass, DEBUGFUNC_NEWPM(f));
    if (!MeOption::quiet) {
      LogInfo::MapleLogger() << "  == " << PhaseName() << " invokes [ " << hdse.PhaseName() << " ] ==\n";
    }
    hdse.DoHDSE();
  }
  return true;
}
}  // namespace maple
