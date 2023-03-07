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
constexpr uint32 kRegCost2 = 2;
constexpr uint32 kCost3 = 3;
constexpr uint32 kCost5 = 5;
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
  std::vector<std::unique_ptr<IVUse>> uses;  // the uses that the group holds
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
  IVCand(std::unique_ptr<IV> i, MeExpr *inc) : iv(std::move(i)), incVersion(inc) {}
  void SetID(uint32 idx) {
    id = IVCandID(idx);
  }
  uint32 GetID() const {
    return static_cast<uint32>(id);
  }

 private:
  IVCandID id = IVCandID(-1);  // the id of the candidate
  std::unique_ptr<IV> iv = nullptr;  // the iv that the candidate holds
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
  IVOptData() = default;
  void CreateIV(MeExpr *expr, MeExpr *base, MeExpr *step, bool isBasicIV);
  void CreateFakeGroup(MeStmt &stmt, IV &iv, IVUseType type, MeExpr *expr);
  void CreateGroup(MeStmt &stmt, IV &iv, IVUseType type, MeExpr *expr);
  IVCand *CreateCandidate(MeExpr *ivTmp, MeExpr *base, MeExpr *step, MeExpr *inc);
  IV *GetIV(const MeExpr &expr);
  bool IsLoopInvariant(const MeExpr &expr);
 private:
  std::vector<std::unique_ptr<IVGroup>> groups;  // record all groups in this loop, use IVGroup.id as index
  std::vector<std::unique_ptr<IVGroup>> fakeGroups;  // record the groups that extracted by pre
                                                     // and no need to compute the cost
  std::vector<std::unique_ptr<IVCand>> cands;  // record all candidates in this loop, use IVCand.id as index
  std::unordered_map<int32, std::vector<IVCand*>> candRecord;  // record candidates with same base,
                                                               // use base.exprID as key
  std::unordered_map<int32, std::unique_ptr<IV>> ivs;  // record all ivs, use exprID as key
  LoopDesc *currLoop = nullptr;  // currently optimized loop
  uint64 iterNum = kDefaultEstimatedLoopIterNum;  // the iterations of current loop
  uint64 realIterNum = -1;  // record the real iternum if we can compute
  bool considerAll = false;  // true if we consider all candidates for every use
  std::unique_ptr<CandSet> set = nullptr;  // used to record set
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
  IVOptimizer(MeFunction &f, bool enabledDebug, IdentifyLoops *meLoops, Dominance *d)
      : func(f),
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
  MeExpr *ResolveBasicIV(const ScalarMeExpr *backValue, ScalarMeExpr *phiLhs, MeExpr *replace);
  bool FindBasicIVs();
  // step2: find all ivs that is the affine form of basic iv, collect iv uses the same time
  bool CreateIVFromAdd(OpMeExpr &op, MeStmt &stmt);
  bool CreateIVFromMul(OpMeExpr &op, MeStmt &stmt);
  bool CreateIVFromSub(OpMeExpr &op, MeStmt &stmt);
  bool CreateIVFromCvt(OpMeExpr &op, MeStmt &stmt);
  bool CreateIVFromIaddrof(OpMeExpr &op, MeStmt &stmt);
  OpMeExpr *TryCvtCmp(OpMeExpr &op, MeStmt &stmt);
  bool FindGeneralIVInExpr(MeStmt &stmt, MeExpr &expr, bool useInAddress = false);
  MeExpr *OptimizeInvariable(MeExpr *expr);
  bool LHSEscape(const ScalarMeExpr *lhs);
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
  MeExpr *ComputeExtraExprOfBase(MeExpr &candBase, MeExpr &groupBase, uint64 ratio, bool &replaced, bool analysis);
  uint32 ComputeCandCostForGroup(const IVCand &cand, IVGroup &group);
  void ComputeGroupCost();
  uint32 ComputeSetCost(CandSet &set);
  // step5: greedily find the best set of candidates in all uses
  void InitSet(bool originFirst);
  void TryOptimize(bool originFirst);
  void FindCandSet();
  void TryReplaceWithCand(CandSet &set, IVCand &cand, std::unordered_map<IVGroup*, IVCand*> &changehange);
  bool OptimizeSet();
  // step6: replace ivs with selected candidates
  bool IsReplaceSameOst(const MeExpr *parent, ScalarMeExpr *target);
  MeStmt *GetIncPos();
  MeExpr *GetInvariant(MeExpr *expr);
  MeExpr *ReplaceCompareOpnd(const OpMeExpr &cmp, MeExpr *compared, MeExpr *replace);
  bool PrepareCompareUse(int64 &ratio, IVUse *use, IVCand *cand, MeStmt *incPos,
                         MeExpr *&extraExpr, MeExpr *&replace);
  MeExpr *GenerateRealReplace(int64 ratio, MeExpr *extraExpr, MeExpr *replace,
                              PrimType realUseType, bool replaceCompare);
  void UseReplace();
  // optimization entry
  void ApplyOptimize();

 private:
  MeFunction &func;
  MeIRMap *irMap;
  bool dumpDetail;  // dump the detail of the optimization
  MeCFG *cfg;
  IdentifyLoops *loops;
  Dominance *dom;
  MeExprUseInfo *useInfo = nullptr;
  bool optimized = false;
  std::unique_ptr<IVOptData> data = nullptr;  // used to record the messages when processing the loop
  std::map<OStIdx, std::unique_ptr<std::set<BBId>>> ssaupdateCands;
  std::unordered_map<int32, MeExpr*> invariables;  // used to record the newly added invariables
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
  } else if (group.type == kUseAddress) {
    LogInfo::MapleLogger() << "Address\n";
  } else {
    LogInfo::MapleLogger() << "Compare\n";
  }
  // dump uses
  for (uint32 i = 0; i < group.uses.size(); ++i) {
    auto *use = group.uses[i].get();
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
  ivs.emplace(expr->GetExprID(), std::make_unique<IV>(expr, base, step, isBasicIV));
}

void IVOptData::CreateFakeGroup(MeStmt &stmt, IV &iv, IVUseType type, MeExpr *expr) {
  auto group = std::make_unique<IVGroup>();
  group->SetID(static_cast<uint32>(fakeGroups.size()));
  auto use = std::make_unique<IVUse>(&stmt, &iv);
  use->group = group.get();
  use->expr = expr;
  group->uses.emplace_back(std::move(use));
  group->type = type;
  fakeGroups.emplace_back(std::move(group));
}

void IVOptData::CreateGroup(MeStmt &stmt, IV &iv, IVUseType type, MeExpr *expr) {
  auto group = std::make_unique<IVGroup>();
  group->SetID(static_cast<uint32>(groups.size()));
  auto use = std::make_unique<IVUse>(&stmt, &iv);
  use->group = group.get();
  use->expr = expr;
  group->uses.emplace_back(std::move(use));
  group->type = type;
  groups.emplace_back(std::move(group));
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

  auto iv = std::make_unique<IV>(ivTmp, base, step, false);
  auto cand = std::make_unique<IVCand>(std::move(iv), inc);
  cand->SetID(static_cast<uint32>(cands.size()));
  auto canPtr = cand.get();
  cands.emplace_back(std::move(cand));
  candRecord[base->GetExprID()].emplace_back(canPtr);
  return canPtr;
}

IV *IVOptData::GetIV(const MeExpr &expr) {
  auto iter = ivs.find(expr.GetExprID());
  return iter == ivs.end() ? nullptr : iter->second.get();
}

bool IVOptData::IsLoopInvariant(const MeExpr &expr) {
  switch (expr.GetMeOp()) {
    case kMeOpConst:
      return true;
    case kMeOpReg:
    case kMeOpVar: {
      if (expr.IsVolatile()) {
        return false;
      }
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
    case kMeOpOp: {
      auto &op = static_cast<const OpMeExpr&>(expr);
      for (size_t i = 0; i < op.GetNumOpnds(); ++i) {
        if (!IsLoopInvariant(*op.GetOpnd(i))) {
          return false;
        }
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
MeExpr *IVOptimizer::ResolveBasicIV(const ScalarMeExpr *backValue, ScalarMeExpr *phiLhs, MeExpr *replace) {
  if (backValue->GetDefBy() != kDefByStmt) {
    return nullptr;
  }
  auto *backRhs = backValue->GetDefStmt()->GetRHS();
  while (backRhs->IsScalar()) {
    if (static_cast<ScalarMeExpr*>(backRhs)->GetDefBy() != kDefByStmt) {
      return nullptr;
    }
    backRhs = static_cast<ScalarMeExpr*>(backRhs)->GetDefStmt()->GetRHS();
  }
  if (backRhs->GetMeOp() != kMeOpOp) {
    return nullptr;
  }
  auto replaced = ReplacePhiLhs(static_cast<OpMeExpr*>(backRhs), phiLhs, replace);
  if (replaced == backRhs) {  // replace failed, can not get step
    return nullptr;
  }
  return replaced;
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
    CHECK_FATAL(ost, "ost is nullptr!");
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
    if (!data->currLoop->CheckBasicIV(backValue, phiLhs)) {
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
  if (IsUnsignedInteger(op.GetOpndType()) && GetPrimTypeSize(op.GetOpndType()) < GetPrimTypeSize(op.GetPrimType())) {
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

// try to cvt condition OP in condition stmt STMT to ne because it's better for optimization
OpMeExpr *IVOptimizer::TryCvtCmp(OpMeExpr &op, MeStmt &stmt) {
  if (data->currLoop->inloopBB2exitBBs.size() != 1 ||  // do not handle multi-exit loop
      (stmt.GetOp() != OP_brfalse && stmt.GetOp() != OP_brtrue) ||
      cfg->GetBBFromID(data->currLoop->inloopBB2exitBBs.begin()->first)->GetLastMe() != &stmt) {
    return &op;
  }
  auto *bb = stmt.GetBB();
  if (!dom->Dominate(*bb, *data->currLoop->latch)) {
    return &op;
  }
  auto &condbr = static_cast<CondGotoMeStmt&>(stmt);
  bool fallthruInLoop = data->currLoop->loopBBs.count(bb->GetSucc(0)->GetBBId()) != 0;
  bool targetInLoop = data->currLoop->loopBBs.count(bb->GetSucc(1)->GetBBId()) != 0;
  CHECK_FATAL(fallthruInLoop != targetInLoop, "TryCvtCmp: check loop form.");
  IV *iv = nullptr;
  MeExpr *opnd = nullptr;
  auto *iv0 = data->GetIV(*op.GetOpnd(0));
  auto *iv1 = data->GetIV(*op.GetOpnd(1));
  auto *opnd0 = op.GetOpnd(0);
  auto *opnd1 = op.GetOpnd(1);
  OpMeExpr newOp(op, kInvalidExprID);
  // prefer iv in left, if not, swap them
  if (iv0 != nullptr && data->IsLoopInvariant(*opnd1)) {
    iv = iv0;
    opnd = opnd1;
  } else if (iv1 != nullptr && data->IsLoopInvariant(*opnd0)) {
    auto condop = newOp.GetOp();
    condop = condop == OP_ge ? OP_le
                             : condop == OP_le ? OP_ge
                                               : condop == OP_lt ? OP_gt
                                                                 : condop == OP_gt ? OP_lt : condop;
    newOp.SetOp(condop);
    newOp.SetOpnd(0, opnd1);
    newOp.SetOpnd(1, opnd0);
    iv = iv1;
    opnd = opnd0;
  } else {
    return &op;
  }
  CHECK_NULL_FATAL(iv);
  if (iv->base->GetMeOp() != kMeOpConst || iv->step->GetMeOp() != kMeOpConst) {
    // do not handle range-unknown iv
    return &op;
  }
  if (opnd->GetMeOp() != kMeOpConst) {
    return &op;
  }
  int64 cmpConst = static_cast<ConstMeExpr*>(opnd)->GetExtIntValue();
  if (GetPrimTypeSize(opnd->GetPrimType()) < kEightByte) {
    cmpConst = static_cast<int64>(static_cast<int32>(cmpConst));
  }
  // prefer target in loop, if not, swap them
  if (fallthruInLoop) {
    condbr.SetOp(condbr.GetOp() == OP_brtrue ? OP_brfalse : OP_brtrue);
    condbr.SetOffset(func.GetOrCreateBBLabel(*bb->GetSucc(0)));
    auto *tmp = bb->GetSucc(0);
    bb->SetSucc(0, bb->GetSucc(1));
    bb->SetSucc(1, tmp);
  }

  int64 baseConst = static_cast<ConstMeExpr*>(iv->base)->GetExtIntValue();
  if (GetPrimTypeSize(iv->base->GetPrimType()) < kEightByte) {
    baseConst = static_cast<int64>(static_cast<int32>(baseConst));
  }
  int64 stepConst = static_cast<ConstMeExpr*>(iv->step)->GetExtIntValue();
  if (GetPrimTypeSize(iv->step->GetPrimType()) < kEightByte) {
    stepConst = static_cast<int64>(static_cast<int32>(stepConst));
  }

  if ((newOp.GetOp() == OP_lt && condbr.GetOp() == OP_brtrue && stepConst == 1) ||
      (newOp.GetOp() == OP_ge && condbr.GetOp() == OP_brfalse && stepConst == 1)) {
    // case1: i < 8, i++  ===>  i != 8, i++
    bool firstCheck = IsSignedInteger(newOp.GetOpndType()) ? baseConst <= cmpConst :
        static_cast<uint64>(baseConst) <= static_cast<uint64>(cmpConst);
    if (!firstCheck) {
      return &op;
    }
  } else if ((newOp.GetOp() == OP_gt && condbr.GetOp() == OP_brtrue && stepConst == -1) ||
             (newOp.GetOp() == OP_le && condbr.GetOp() == OP_brfalse && stepConst == -1)) {
    // case2: i > 8, i--  ===>  i != 8, i--
    bool firstCheck = IsSignedInteger(newOp.GetOpndType()) ? baseConst >= cmpConst :
        static_cast<uint64>(baseConst) >= static_cast<uint64>(cmpConst);
    if (!firstCheck) {
      return &op;
    }
  } else if ((newOp.GetOp() == OP_le && condbr.GetOp() == OP_brtrue && stepConst == 1) ||
             (newOp.GetOp() == OP_gt && condbr.GetOp() == OP_brfalse && stepConst == 1)) {
    // case3: i <= 8, i++  ===>  i < 9, i++  ===>  i != 9, i++
    bool overflow = IsSignedInteger(newOp.GetOpndType()) ? cmpConst == INT64_MAX : cmpConst == UINT64_MAX;
    if (overflow) {
      return &op;
    }
    cmpConst = cmpConst + stepConst;
    bool firstCheck = IsSignedInteger(newOp.GetOpndType()) ? baseConst <= cmpConst :
        static_cast<uint64>(baseConst) <= static_cast<uint64>(cmpConst);
    if (!firstCheck) {
      return &op;
    }
    newOp.SetOpnd(1, irMap->CreateIntConstMeExpr(cmpConst, opnd->GetPrimType()));
  } else if ((newOp.GetOp() == OP_ge && condbr.GetOp() == OP_brtrue && stepConst == -1) ||
             (newOp.GetOp() == OP_lt && condbr.GetOp() == OP_brfalse && stepConst == -1)) {
    // case4: i >= 8, i--  ===>  i > 7, i--  ===>  i != 7, i--
    bool underflow = IsSignedInteger(newOp.GetOpndType()) ? cmpConst == INT64_MIN : cmpConst == 0;
    if (underflow) {
      return &op;
    }
    cmpConst = cmpConst + stepConst;
    bool firstCheck = IsSignedInteger(newOp.GetOpndType()) ? baseConst >= cmpConst :
        static_cast<uint64>(baseConst) >= static_cast<uint64>(cmpConst);
    if (!firstCheck) {
      return &op;
    }
    newOp.SetOpnd(1, irMap->CreateIntConstMeExpr(cmpConst, opnd->GetPrimType()));
  } else {
    return &op;
  }
  newOp.SetOp(condbr.GetOp() == OP_brtrue ? OP_ne : OP_eq);
  auto *hashed = irMap->HashMeExpr(newOp);
  irMap->ReplaceMeExprStmt(stmt, op, *hashed);
  return static_cast<OpMeExpr*>(hashed);
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
        auto *cmp = TryCvtCmp(op, stmt);
        auto *iv = data->GetIV(*cmp->GetOpnd(0));
        if (iv != nullptr && data->IsLoopInvariant(*cmp->GetOpnd(1))) {
          data->CreateGroup(stmt, *iv, kUseCompare, cmp);
          (*data->groups.rbegin())->uses[0]->comparedExpr = cmp->GetOpnd(1);
          return false;
        }
        iv = data->GetIV(*cmp->GetOpnd(1));
        if (iv != nullptr && data->IsLoopInvariant(*cmp->GetOpnd(0))) {
          data->CreateGroup(stmt, *iv, kUseCompare, cmp);
          (*data->groups.rbegin())->uses[0]->comparedExpr = cmp->GetOpnd(0);
          return false;
        }
        iv = iv == nullptr ? data->GetIV(*cmp->GetOpnd(0)) : iv;
        data->CreateGroup(stmt, *iv, kUseGeneral, cmp);
        return false;
      }
      for (uint8 j = 0; j < op.GetNumOpnds(); ++j) {
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
        (*(*data->groups.rbegin())->uses.rbegin())->hasField = ivar.GetFieldID() != 0 || ivar.GetOffset() != 0;
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
bool IVOptimizer::LHSEscape(const ScalarMeExpr *lhs) {
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
      CHECK_FATAL(iv, "iv is nullptr");
      if (useBB != data->currLoop->head || !iv->isBasicIV) {
        return true;
      }
    }
  }
  return false;
}

MeExpr *IVOptimizer::OptimizeInvariable(MeExpr *expr) {
  for (size_t i = 0; i < expr->GetNumOpnds(); ++i) {
    auto *opnd = expr->GetOpnd(i);
    if (opnd->GetMeOp() == kMeOpOp && opnd->GetOp() != OP_div && opnd->GetOp() != OP_rem &&
        data->IsLoopInvariant(*opnd)) {
      // move loop invariant out of current loop
      auto found = invariables.find(opnd->GetExprID());
      RegMeExpr *outValue = nullptr;
      if (found == invariables.end()) {
        outValue = irMap->CreateRegMeExpr(*opnd);
        invariables.emplace(opnd->GetExprID(), outValue);
        auto *outStmt = irMap->CreateAssignMeStmt(static_cast<ScalarMeExpr &>(*outValue), *opnd,
                                                  *data->currLoop->preheader);
        data->currLoop->preheader->InsertMeStmtLastBr(outStmt);
      } else {
        outValue = static_cast<RegMeExpr *>(found->second);
      }
      expr = irMap->ReplaceMeExprExpr(*expr, *opnd, *outValue);
      continue;
    }
    auto *res = OptimizeInvariable(opnd);
    expr = irMap->ReplaceMeExprExpr(*expr, *opnd, *res);
  }
  if (expr->GetMeOp() == kMeOpOp && expr->GetOp() != OP_div && expr->GetOp() != OP_rem &&
      data->IsLoopInvariant(*expr)) {
    // move loop invariant out of current loop
    auto found = invariables.find(expr->GetExprID());
    RegMeExpr *outValue = nullptr;
    if (found == invariables.end()) {
      outValue = irMap->CreateRegMeExpr(*expr);
      invariables.emplace(expr->GetExprID(), outValue);
      auto *outStmt = irMap->CreateAssignMeStmt(static_cast<ScalarMeExpr &>(*outValue), *expr,
                                                *data->currLoop->preheader);
      data->currLoop->preheader->InsertMeStmtLastBr(outStmt);
    } else {
      outValue = static_cast<RegMeExpr*>(found->second);
    }
    return outValue;
  }
  return expr;
}

void IVOptimizer::FindGeneralIVInStmt(MeStmt &stmt) {
  for (uint32 i = 0; i < stmt.NumMeStmtOpnds(); ++i) {
    auto *opnd = stmt.GetOpnd(i);
    auto *opted = OptimizeInvariable(opnd);
    if (opted != opnd) {
      irMap->ReplaceMeExprStmt(stmt, *opnd, *opted);
      opnd = opted;
    }
    bool isUsedInAddr = (stmt.GetOp() == OP_iassign || stmt.GetOp() == OP_iassignoff) && i == 0;
    isUsedInAddr |= stmt.GetOp() == OP_assertnonnull;
    if (FindGeneralIVInExpr(stmt, *opnd, isUsedInAddr)) {
      auto *iv = data->GetIV(*opnd);
      CHECK_FATAL(iv, "iv is nullptr!");
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
            static_cast<IassignMeStmt&>(stmt).GetLHSVal()->GetFieldID() != 0 ||
            static_cast<IassignMeStmt&>(stmt).GetLHSVal()->GetOffset() != 0;
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

  for (auto childID : dom->GetDomChildren(bb.GetID())) {
    TraversalLoopBB(*cfg->GetBBFromID(BBId(childID)), bbVisited);
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

  tmp = irMap->CreateRegMeExpr(addressPTY);
  base = irMap->CreateMeExprTypeCvt(addressPTY, GetUnsignedPrimType(iv.base->GetPrimType()), *iv.base);
  simplified = irMap->SimplifyMeExpr(base);
  if (simplified != nullptr) {
    base = simplified;
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
      offset = static_cast<ConstMeExpr&>(expr).GetExtIntValue();
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

  MeExpr *simplified = nullptr;
  // try to find simpler candidate
  if (use.iv->base->GetOp() == OP_add) {
    for (uint32 i = 0; i < 2 ; ++i) { // add/sub has 2 opnds
      if (use.iv->base->GetOpnd(i) == use.iv->step) {
        tmp = irMap->CreateRegMeExpr(GetUnsignedPrimType(use.iv->expr->GetPrimType()));
        inc = irMap->CreateRegMeExprVersion(*tmp);
        cand = data->CreateCandidate(tmp, use.iv->base->GetOpnd(1 - i), use.iv->step, inc);
        use.group->relatedCands.emplace(cand->GetID());
      }
    }
  } else if (use.iv->base->GetOp() == OP_sub) {
    if (use.iv->base->GetOpnd(1) == use.iv->step) {
      tmp = irMap->CreateRegMeExpr(GetUnsignedPrimType(use.iv->expr->GetPrimType()));
      inc = irMap->CreateRegMeExprVersion(*tmp);
      cand = data->CreateCandidate(tmp, use.iv->base->GetOpnd(0), use.iv->step, inc);
      use.group->relatedCands.emplace(cand->GetID());
    }
  }
  // strip constant part of base and create a candidate
  int64 offset = 0;
  auto *newBase = StripConstantPart(*use.iv->base, offset);
  if (newBase == nullptr) {
    return;
  }
  simplified = irMap->SimplifyMeExpr(newBase);
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
    auto *iv = itIV.second.get();
    if (iv->isBasicIV) {
      CreateIVCandidateFromBasicIV(*iv);
    }
  }

  // add all candidate created from basic iv to all groups
  for (auto &group : data->groups) {
    for (auto &cand : data->cands) {
      group->relatedCands.emplace(cand->GetID());
    }
  }

  std::map<MeExpr*, std::vector<IVUse*>> offsetCount;
  // create candidate from use
  for (auto &group : data->groups) {
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
        offsetCount[aparted].emplace_back(group->uses[0].get());
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
  for (auto &group : data->fakeGroups) {
    // just need to consider the first use
    CreateIVCandidateFromUse(*group->uses[0]);
  }
}

static uint32 ComputeExprCost(MeExpr &expr, const MeExpr *parent = nullptr) {
#ifndef TARGAARCH64
  return 0;
#endif
  // initialize cost
  constexpr uint32 regCost = 0;
  constexpr uint32 constCost = 4;
  constexpr uint32 cvtCost = 4;
  constexpr uint32 addCost = 4;
  constexpr uint32 addressCost = 5;
  constexpr uint32 mulCost = 5;
  constexpr uint32 symbolCost = 9;
  constexpr uint32 defaultCost = 16;

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
      return defaultCost;
    }
    default:
      return defaultCost;
  }
}

static uint32 ComputeAddressCost(MeExpr *expr, int64 ratio, bool hasField) {
  bool ratioCombine = ratio == 1 || ratio == 2 || ratio == 4 || ratio == 8;
  uint32 cost = 0;
  if (expr != nullptr) {
    cost += 1;
  }
  if (!ratioCombine) {
    cost += kCost5;
  }
  if (hasField && (!expr || expr->GetMeOp() != kMeOpConst)) {
    cost += kCost3;
  }
  return cost;
}

void IVOptimizer::ComputeCandCost() {
  for (auto &cand : data->cands) {
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

struct ScalarPeel {
  ScalarPeel(MeExpr *e, int64 m, PrimType t) {
    expr = e;
    multiplier = m;
    expandType = t;
  }

  MeExpr *expr = nullptr;
  int64 multiplier = 0;
  PrimType expandType = PTY_unknown;
};

void FindScalarFactor(MeExpr &expr, OpMeExpr *parentCvt, std::unordered_map<int32, ScalarPeel> &record,
    int64 multiplier, bool needSext, bool analysis) {
  switch (expr.GetMeOp()) {
    case kMeOpConst: {
      int64 constVal = needSext ? static_cast<ConstMeExpr&>(expr).GetSXTIntValue() :
                                  static_cast<ConstMeExpr&>(expr).GetExtIntValue();
      auto it = record.find(kInvalidExprID);
      if (it == record.end()) {
        record.emplace(kInvalidExprID, ScalarPeel(&expr, multiplier * constVal, PTY_unknown));
      } else {
        it->second.multiplier += (multiplier * constVal);
      }
      return;
    }
    case kMeOpReg:
    case kMeOpVar: {
      PrimType expandType = parentCvt ? parentCvt->GetOpndType() : expr.GetPrimType();
      auto it = record.find(expr.GetExprID());
      if (it == record.end()) {
        record.emplace(expr.GetExprID(), ScalarPeel(&expr, multiplier, expandType));
      } else {
        it->second.multiplier += multiplier;
      }
      return;
    }
    case kMeOpOp: {
      auto &op = static_cast<OpMeExpr&>(expr);
      if (op.GetOp() == OP_add) {
        FindScalarFactor(*op.GetOpnd(0), parentCvt, record, multiplier, needSext, analysis);
        FindScalarFactor(*op.GetOpnd(1), parentCvt, record, multiplier, needSext, analysis);
      } else if (op.GetOp() == OP_sub) {
        FindScalarFactor(*op.GetOpnd(0), parentCvt, record, multiplier, needSext, analysis);
        FindScalarFactor(*op.GetOpnd(1), parentCvt, record, -multiplier, needSext, analysis);
      } else if (op.GetOp() == OP_mul) {
        auto *opnd0 = op.GetOpnd(0);
        auto *opnd1 = op.GetOpnd(1);
        if (opnd0->GetMeOp() != kMeOpConst && opnd1->GetMeOp() != kMeOpConst) {
          PrimType expandType = parentCvt ? parentCvt->GetOpndType() : expr.GetPrimType();
          record.emplace(expr.GetExprID(), ScalarPeel(&expr, multiplier, expandType));
          return;
        }
        if (opnd0->GetMeOp() == kMeOpConst) {
          FindScalarFactor(*op.GetOpnd(1), parentCvt,
              record, multiplier * static_cast<ConstMeExpr*>(opnd0)->GetExtIntValue(), needSext, analysis);
        } else {
          FindScalarFactor(*op.GetOpnd(0), parentCvt,
              record, multiplier * static_cast<ConstMeExpr*>(opnd1)->GetExtIntValue(), needSext, analysis);
        }
      } else if (op.GetOp() == OP_cvt) {
        if (GetPrimTypeSize(op.GetOpndType()) < GetPrimTypeSize(op.GetPrimType())) {
          if (!analysis && IsUnsignedInteger(op.GetOpndType())) {
            PrimType expandType = parentCvt ? parentCvt->GetOpndType() : expr.GetPrimType();
            record.emplace(expr.GetExprID(), ScalarPeel(&expr, multiplier, expandType));
            return;
          }
          parentCvt = &op;
        }
        FindScalarFactor(*op.GetOpnd(0), parentCvt, record, multiplier, IsSignedInteger(op.GetOpndType()), analysis);
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
    int64 candConst = static_cast<ConstMeExpr&>(candStep).GetExtIntValue();
    int64 groupConst = static_cast<ConstMeExpr&>(groupStep).GetExtIntValue();
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

  std::unordered_map<int32, ScalarPeel> candMap;
  std::unordered_map<int32, ScalarPeel> groupMap;
  FindScalarFactor(candStep, nullptr, candMap, 1, false, true);
  FindScalarFactor(groupStep, nullptr, groupMap, 1, false, true);
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
    if (itGroup->second.multiplier == 0 && itCand.second.multiplier == 0) {
      continue;
    } else if (itCand.second.multiplier == 0) {
      return kInfinityCost;
    }
    int64 remainder = itGroup->second.multiplier % itCand.second.multiplier;
    if (remainder != 0) {
      return 0;
    }
    int64 ratio = itGroup->second.multiplier / itCand.second.multiplier;
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

MeExpr *IVOptimizer::ComputeExtraExprOfBase(MeExpr &candBase, MeExpr &groupBase, uint64 ratio, bool &replaced,
    bool analysis) {
  std::unordered_map<int32, ScalarPeel> candMap;
  std::unordered_map<int32, ScalarPeel> groupMap;
  FindScalarFactor(candBase, nullptr, candMap, 1, false, analysis);
  FindScalarFactor(groupBase, nullptr, groupMap, 1, false, analysis);
  MeExpr *extraExpr = nullptr;
  uint64 candConst = 0;
  int64 groupConst = 0;
  for (auto &itGroup : groupMap) {
    auto itCand = candMap.find(itGroup.first);
    if (itGroup.first == kInvalidExprID) {
      candConst = itCand == candMap.end() ?
          0 : itCand->second.multiplier * ratio;
      groupConst = itGroup.second.multiplier;
      continue;
    }
    bool addCvt = (itCand == candMap.end() || itGroup.second.expandType != itCand->second.expandType);
    if (itCand == candMap.end() || addCvt) {
      MeExpr *constExpr = nullptr;
      MeExpr *expr = itGroup.second.expr;
      if (NeedCvtOrRetype(itGroup.second.expandType, groupBase.GetPrimType())) {
        expr = irMap->CreateMeExprTypeCvt(groupBase.GetPrimType(), itGroup.second.expandType, *expr);
      }
      if (itGroup.second.multiplier != 1) {
        constExpr = irMap->CreateIntConstMeExpr(itGroup.second.multiplier, groupBase.GetPrimType());
        expr = irMap->CreateMeExprBinary(OP_mul, groupBase.GetPrimType(), *expr, *constExpr);
      }
      extraExpr = extraExpr == nullptr ? expr
                                       : irMap->CreateMeExprBinary(OP_add, groupBase.GetPrimType(), *extraExpr, *expr);
    } else {
      int64 newMultiplier = static_cast<int64>(static_cast<uint64>(itGroup.second.multiplier) -
          (static_cast<uint64>(itCand->second.multiplier) * ratio));
      if (newMultiplier == 0) {
        continue;
      }
      MeExpr *constExpr = nullptr;
      MeExpr *expr = itGroup.second.expr;
      if (NeedCvtOrRetype(itGroup.second.expandType, groupBase.GetPrimType())) {
        expr = irMap->CreateMeExprTypeCvt(groupBase.GetPrimType(), itGroup.second.expandType, *expr);
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
      candConst = static_cast<uint64>(itCand.second.multiplier) * ratio;
      groupConst = itGroup == groupMap.end() ? 0 : itGroup->second.multiplier;
      continue;
    }
    bool addCvt = (itGroup == groupMap.end() || itGroup->second.expandType != itCand.second.expandType);
    if (itGroup == groupMap.end() || addCvt) {
      if ((itCand.second.expr->GetPrimType() == PTY_ptr ||
          itCand.second.expr->GetPrimType() == PTY_a64 ||
          itCand.second.expr->GetPrimType() == PTY_a32) && itCand.second.expr->GetOp() != OP_cvt) {
        // it's not good to use one obj to form others
        replaced = false;
        return nullptr;
      }
      int64 multiplier = -(itCand.second.multiplier * static_cast<int64>(ratio));
      auto *constExpr = irMap->CreateIntConstMeExpr(multiplier, ptyp);
      auto *expr = itCand.second.expr;
      if (extraExpr != nullptr) {
        if (NeedCvtOrRetype(extraExpr->GetPrimType(), ptyp)) {
          extraExpr = irMap->CreateMeExprTypeCvt(ptyp, extraExpr->GetPrimType(), *extraExpr);
        }
      }
      if (NeedCvtOrRetype(itCand.second.expandType, ptyp)) {
        expr = irMap->CreateMeExprTypeCvt(ptyp, itCand.second.expandType, *expr);
      }
      expr = irMap->CreateMeExprBinary(OP_mul, ptyp, *expr, *constExpr);
      extraExpr = extraExpr == nullptr ? expr
                                       : irMap->CreateMeExprBinary(OP_add, ptyp, *extraExpr, *expr);
    }
  }
  if (static_cast<uint64>(groupConst) - candConst == 0) {
    return extraExpr;
  }
  auto *constExpr = irMap->CreateIntConstMeExpr(static_cast<uint64>(groupConst) - candConst, ptyp);
  extraExpr = extraExpr == nullptr ? constExpr
                                   : irMap->CreateMeExprBinary(OP_add, ptyp, *extraExpr, *constExpr);
  return extraExpr;
}

static bool CheckOverflow(const MeExpr *opnd0, const MeExpr *opnd1, Opcode op, PrimType ptyp) {
  // can be extended to scalar later
  if (opnd0->GetMeOp() != kMeOpConst || opnd1->GetMeOp() != kMeOpConst) {
    return true;
  }
  int64 const0 = static_cast<const ConstMeExpr*>(opnd0)->GetExtIntValue();
  int64 const1 = static_cast<const ConstMeExpr*>(opnd1)->GetExtIntValue();
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
            static_cast<uint64>(const1) >> rightShiftNumToGetSignFlag);
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

uint32 IVOptimizer::ComputeCandCostForGroup(const IVCand &cand, IVGroup &group) {
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
      MeExpr *extraExpr = ComputeExtraExprOfBase(*group.uses[0]->iv->base, *cand.iv->base,
                                                 static_cast<uint64>(ratio), replaced, true);
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

  MeExpr *extraExpr = ComputeExtraExprOfBase(*cand.iv->base, *group.uses[0]->iv->base,
                                             static_cast<uint64>(ratio), replaced, true);
  if (!replaced) {
    return kInfinityCost;
  }
  uint32 mulCost = 5;
  uint8 extraConstCost = 4;
  if (group.type == kUseGeneral) {
    if (extraExpr == nullptr) {
      return (ratio == 1 ? 0 : mulCost + kRegCost);
    }
    if (extraExpr == cand.iv->step && ratio == 1) {
      return 0;
    }
    uint32 cost = ComputeExprCost(*extraExpr);
    if (extraExpr->GetMeOp() == kMeOpConst) {
      return extraConstCost;
    }
    return (cost / data->iterNum) + (ratio == 1 ? mulCost - 1 + kRegCost : mulCost + kRegCost2);
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
    return (cost / data->iterNum) + ((ratio == 1 || (ratio == -1 && isEqNe)) ? 0 : mulCost + kRegCost2);
  }
  return kInfinityCost;
}

void IVOptimizer::ComputeGroupCost() {
  for (auto &group : data->groups) {
    for (auto &cand : data->cands) {
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
  auto init = std::make_unique<CandSet>();
  init->chosenCands.resize(data->groups.size(), nullptr);
  init->candCount.resize(data->cands.size(), 0);
  for (auto &group : data->groups) {
    for (auto &cand : data->cands) {
      if (group->candCosts[cand->GetID()] == kInfinityCost) {
        continue;
      }
      if (init->chosenCands[group->GetID()] == nullptr) {
        init->chosenCands[group->GetID()] = cand.get();
        ++init->candCount[cand->GetID()];
        continue;
      }
      if (originFirst) {
        if (cand->incPos == kOriginal) {
          if (init->chosenCands[group->GetID()]->incPos != kOriginal) {
            ++init->candCount[cand->GetID()];
            --init->candCount[init->chosenCands[group->GetID()]->GetID()];
            init->chosenCands[group->GetID()] = cand.get();
            continue;
          }
        } else if (cand->origIV != nullptr) {
          if (init->chosenCands[group->GetID()]->incPos == kOriginal) {
            continue;
          } else if (init->chosenCands[group->GetID()]->origIV != nullptr) {
          } else {
            ++init->candCount[cand->GetID()];
            --init->candCount[init->chosenCands[group->GetID()]->GetID()];
            init->chosenCands[group->GetID()] = cand.get();
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
        init->chosenCands[group->GetID()] = cand.get();
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
  for (auto &cand : data->cands) {
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
  data->set = std::move(init);
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
    auto *group = data->groups[i].get();
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
      auto *anotherGroup = data->groups[j].get();
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
  auto *set = data->set.get();
  uint32 bestCost = set->cost;
  uint32 numIVs = set->NumIVs();
  std::unordered_map<IVGroup*, IVCand*> bestChange;
  uint32 bestInitDepth = 0;
  for (auto &cand : data->cands) {
    if (set->candCount[cand->GetID()] != 0) {
      bestInitDepth += cand->iv->base->GetDepth();
    }
  }
  for (auto &cand : data->cands) {
    if (set->candCount[cand->GetID()] != 0) {
      continue;
    }
    uint32 curCost = set->cost;
    std::unordered_map<IVGroup*, IVCand*> curChange;
    uint32 curInitDepth = bestInitDepth;
    std::vector<uint32> tmpCandCount = set->candCount;
    bool replaced = false;
    for (auto &group : data->groups) {
      auto chosenCand = set->chosenCands[group->GetID()];
      if (group->candCosts[cand->GetID()] <= group->candCosts[chosenCand->GetID()]) {
        curCost = curCost - group->candCosts[chosenCand->GetID()] + group->candCosts[cand->GetID()];
        if (--tmpCandCount[chosenCand->GetID()] == 0) {
          curCost = curCost - kRegCost - chosenCand->cost;
          curInitDepth = curInitDepth - chosenCand->iv->base->GetDepth();
        }
        if (++tmpCandCount[cand->GetID()] == 1) {
          curCost = curCost + kRegCost + cand->cost;
          curInitDepth = curInitDepth + cand->iv->base->GetDepth();
        }
        curChange.emplace(group.get(), cand.get());
        replaced = true;
      }
    }
    constexpr uint8 kMaxNumIvs = 10;
    if (replaced && numIVs < kMaxNumIvs) {
      auto tmpSet = *set;
      tmpSet.cost = curCost;
      for (auto &it : curChange) {
        tmpSet.chosenCands[it.first->GetID()] = it.second;
      }
      tmpSet.candCount = tmpCandCount;
      TryReplaceWithCand(tmpSet, *cand, curChange);
      curCost = tmpSet.cost;
      curInitDepth = 0;
      for (auto &tmpCand : data->cands) {
        if (tmpSet.candCount[tmpCand->GetID()] != 0) {
          bestInitDepth += tmpCand->iv->base->GetDepth();
        }
      }
    }
    if (curCost < bestCost || (curCost == bestCost && curInitDepth < bestInitDepth)) {
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
    for (auto &cand : data->cands) {
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
  auto firstSet = std::move(data->set);

  // second time start from chosen iv
  TryOptimize(false);
  auto secondSet = std::move(data->set);
  if (firstSet == nullptr && secondSet == nullptr) {
  } else if (firstSet == nullptr) {
    data->set = std::move(secondSet);
  } else if (secondSet == nullptr) {
    data->set = std::move(firstSet);
  } else {
    if (firstSet->cost > secondSet->cost) {
      data->set = std::move(secondSet);
    } else {
      data->set = std::move(firstSet);
    }
  }
}

bool IVOptimizer::IsReplaceSameOst(const MeExpr *parent, ScalarMeExpr *target) {
  switch (parent->GetMeOp()) {
    case kMeOpReg:
    case kMeOpVar:
      return static_cast<const ScalarMeExpr*>(parent)->GetOstIdx() == target->GetOstIdx();
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

MeExpr *IVOptimizer::GetInvariant(MeExpr *expr) {
  auto *preheaderLast = data->currLoop->preheader->GetLastMe();
  while (preheaderLast != nullptr && (preheaderLast->GetOp() == OP_comment || preheaderLast->GetOp() == OP_goto)) {
    preheaderLast = preheaderLast->GetPrev();
  }
  if (invariables.find(expr->GetExprID()) == invariables.end()) {
    if (expr->GetMeOp() != kMeOpConst) {
      auto *extraReg = irMap->CreateRegMeExpr(expr->GetPrimType());
      auto *assign = irMap->CreateAssignMeStmt(*extraReg, *expr, *data->currLoop->preheader);
      if (preheaderLast == nullptr) {
        data->currLoop->preheader->AddMeStmtFirst(assign);
      } else {
        preheaderLast->GetBB()->InsertMeStmtAfter(preheaderLast, assign);
      }
      invariables.emplace(expr->GetExprID(), extraReg);
      return extraReg;
    }
  } else {
    return invariables[expr->GetExprID()];
  }
  return expr;
}

MeExpr *IVOptimizer::ReplaceCompareOpnd(const OpMeExpr &cmp, MeExpr *compared, MeExpr *replace) {
  OpMeExpr newOpExpr(cmp, kInvalidExprID);
  for (size_t i = 0; i < newOpExpr.GetNumOpnds(); i++) {
    if (newOpExpr.GetOpnd(i) == compared) {
      newOpExpr.SetOpnd(i, replace);
    }
  }
  return irMap->HashMeExpr(newOpExpr);
}

bool IVOptimizer::PrepareCompareUse(int64 &ratio, IVUse *use, IVCand *cand, MeStmt *incPos,
                                    MeExpr *&extraExpr, MeExpr *&replace) {
  bool replaced = true;
  bool replaceCompare = false;
  MeExpr *simplified = nullptr;
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
      auto newOp = GetSwapCmpOp(op);
      newOpExpr.SetOp(newOp);
      auto *hashed = irMap->HashMeExpr(newOpExpr);
      (void)irMap->ReplaceMeExprStmt(*use->stmt, *use->expr, *hashed);
      use->expr = hashed;
    }
    // compute extra expr after replaced by new iv
    if (incPos != nullptr && incPos->IsCondBr() && use->stmt == incPos) {
      // use inc version to replace
      auto *newBase = irMap->CreateMeExprBinary(OP_add, cand->iv->base->GetPrimType(),
                                                *cand->iv->base, *cand->iv->step);
      extraExpr = ComputeExtraExprOfBase(*use->iv->base, *newBase, static_cast<uint64>(ratio), replaced, false);
      auto *tmp = irMap->ReplaceMeExprExpr(*use->expr, *use->iv->expr, *cand->incVersion);
      (void)irMap->ReplaceMeExprStmt(*use->stmt, *use->expr, *tmp);
      use->expr = tmp;
    } else {
      extraExpr = ComputeExtraExprOfBase(*use->iv->base, *cand->iv->base, static_cast<uint64>(ratio), replaced, false);
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
      extraExpr = ComputeExtraExprOfBase(*newBase, *use->iv->base, static_cast<uint64>(ratio), replaced, false);
      replace = cand->incVersion;
    } else {
      extraExpr = ComputeExtraExprOfBase(*cand->iv->base, *use->iv->base, static_cast<uint64>(ratio), replaced, false);
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
              *irMap->CreateIntConstMeExpr(static_cast<int64>(data->realIterNum), candStep->GetPrimType()));
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
        if (NeedCvtOrRetype(extraExpr->GetPrimType(), comparedExpr->GetPrimType())) {
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
        extraExpr = GetInvariant(extraExpr);
        if (ratio == -1) {
          // swap comparison
          OpMeExpr newOpExpr(static_cast<OpMeExpr&>(*use->expr), kInvalidExprID);
          auto op = newOpExpr.GetOp();
          CHECK_FATAL(IsCompareHasReverseOp(op), "should be known op!");
          auto newOp = GetSwapCmpOp(op);
          newOpExpr.SetOp(newOp);
          auto *hashed = irMap->HashMeExpr(newOpExpr);
          (void)irMap->ReplaceMeExprStmt(*use->stmt, *use->expr, *hashed);
          use->expr = hashed;
          extraExpr = irMap->CreateMeExprBinary(OP_mul, extraExpr->GetPrimType(), *extraExpr,
                                                *irMap->CreateIntConstMeExpr(-1, extraExpr->GetPrimType()));
          ratio = 1;
          simplified = irMap->SimplifyMeExpr(extraExpr);
          if (simplified != nullptr) { extraExpr = simplified; }
        }
        extraExpr = GetInvariant(extraExpr);
        auto *newCmp = ReplaceCompareOpnd(static_cast<OpMeExpr&>(*use->expr), use->comparedExpr, extraExpr);
        (void)irMap->ReplaceMeExprStmt(*use->stmt, *use->expr, *newCmp);
        use->expr = newCmp;
        use->comparedExpr = extraExpr;
        extraExpr = nullptr;
      }
    }
    if (use->comparedExpr->GetPrimType() != static_cast<OpMeExpr*>(use->expr)->GetOpndType()) {
      auto *cvt = irMap->CreateMeExprTypeCvt(static_cast<OpMeExpr*>(use->expr)->GetOpndType(),
                                             use->comparedExpr->GetPrimType(), *use->comparedExpr);
      cvt = GetInvariant(cvt);
      auto *newCmp = ReplaceCompareOpnd(static_cast<OpMeExpr&>(*use->expr), use->comparedExpr, cvt);
      (void)irMap->ReplaceMeExprStmt(*use->stmt, *use->expr, *newCmp);
      use->expr = newCmp;
      use->comparedExpr = cvt;
    }
  }
  CHECK_FATAL(replaced, "use should be able to be replaced");
  return replaceCompare;
}

MeExpr *IVOptimizer::GenerateRealReplace(int64 ratio, MeExpr *extraExpr, MeExpr *replace, PrimType realUseType,
                                         bool replaceCompare) {
  MeExpr *simplified = nullptr;
  if (ratio == 1 && extraExpr == nullptr) {
  } else if (ratio == 1) {
    replace = irMap->CreateMeExprBinary(OP_add, realUseType, *extraExpr, *replace);
    simplified = irMap->SimplifyMeExpr(replace);
    if (simplified != nullptr) { replace = simplified; }
  } else if (extraExpr == nullptr) {
    replace = irMap->CreateMeExprBinary(OP_mul, realUseType,
                                        *irMap->CreateIntConstMeExpr(ratio, replace->GetPrimType()), *replace);
  } else {
    replace = irMap->CreateMeExprBinary(OP_mul, realUseType, *replace,
                                        *irMap->CreateIntConstMeExpr(ratio, replace->GetPrimType()));
    if (replaceCompare && extraExpr->GetMeOp() == kMeOpReg) {
      auto *regExtra = static_cast<RegMeExpr*>(extraExpr);
      auto *def = regExtra->GetDefStmt();
      if (def != nullptr && invariables.find(def->GetRHS()->GetExprID()) != invariables.end()) {
        auto *tmpReplace = irMap->CreateMeExprBinary(OP_add, replace->GetPrimType(),
                                                     *regExtra->GetDefStmt()->GetRHS(), *replace);
        simplified = irMap->SimplifyMeExpr(tmpReplace);
        if (simplified != nullptr) { tmpReplace = simplified; }
        if (tmpReplace->GetDepth() <= regExtra->GetDefStmt()->GetRHS()->GetDepth()) {
          return tmpReplace;
        }
      }
    }
    replace = irMap->CreateMeExprBinary(OP_add, replace->GetPrimType(), *extraExpr, *replace);
    simplified = irMap->SimplifyMeExpr(replace);
    if (simplified != nullptr) { replace = simplified; }
  }
  return replace;
}

void IVOptimizer::UseReplace() {
  auto *latchBB = data->currLoop->latch;
  auto *incPos = GetIncPos();

  for (size_t i = 0; i < data->groups.size(); ++i) {
    auto *group = data->groups[i].get();
    auto *cand = data->set->chosenCands[i];
    if (cand->incPos == kOriginal && cand->originTmp == nullptr) {
      cand->originTmp = irMap->CreateRegMeExpr(cand->iv->expr->GetPrimType());
      auto *tmpAssign = irMap->CreateAssignMeStmt(static_cast<ScalarMeExpr&>(*cand->originTmp),
                                                  *cand->iv->expr, *data->currLoop->head);
      data->currLoop->head->AddMeStmtFirst(tmpAssign);
    }
    // replace use
    for (auto &use : group->uses) {
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
      // preprocess use
      if (group->type == kUseCompare) {
        replaceCompare = PrepareCompareUse(ratio, use.get(), cand, incPos, extraExpr, replace);
      } else {
        bool replaced = true;
        ratio = ComputeRatioOfStep(*cand->iv->step, *use->iv->step);
        extraExpr = ComputeExtraExprOfBase(*cand->iv->base, *use->iv->base, static_cast<uint64>(ratio), replaced, false);
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
            extraExpr = ComputeExtraExprOfBase(*newBase, *use->iv->base, static_cast<uint64>(ratio), replaced, false);
            replace = cand->incVersion;
          }
        }
        CHECK_FATAL(replaced, "should have been replaced");
      }
      // confirm type consistent
      MeExpr *simplified = nullptr;
      auto realUseType = replace->GetPrimType();
      if ((IsCompareHasReverseOp(use->expr->GetOp()) || use->expr->GetOp() == OP_retype) &&
          static_cast<OpMeExpr*>(use->expr)->GetOpndType() != kPtyInvalid) {
        realUseType = static_cast<OpMeExpr*>(use->expr)->GetOpndType();
      } else if (GetPrimTypeSize(realUseType) > GetPrimTypeSize(use->iv->expr->GetPrimType())) {
        realUseType = use->iv->expr->GetPrimType();
      }
      // Update real use type for vector intrinsicop
      if (use->expr->GetOp() == OP_intrinsicop) {
        auto *naryExpr = static_cast<NaryMeExpr*>(use->expr);
        MIRIntrinsicID intrnID = naryExpr->GetIntrinsic();
        IntrinDesc &intrinDesc = IntrinDesc::intrinTable[intrnID];
        if (intrinDesc.IsVectorOp()) {
          realUseType = use->iv->expr->GetPrimType();
        }
      }
      if (extraExpr != nullptr) {
        if (NeedCvtOrRetype(extraExpr->GetPrimType(), realUseType)) {
          extraExpr = irMap->CreateMeExprTypeCvt(realUseType, extraExpr->GetPrimType(), *extraExpr);
          simplified = irMap->SimplifyMeExpr(extraExpr);
          if (simplified != nullptr) { extraExpr = simplified; }
        }
        extraExpr = GetInvariant(extraExpr);
      }
      if (NeedCvtOrRetype(replace->GetPrimType(), realUseType)) {
        replace = irMap->CreateMeExprTypeCvt(realUseType, replace->GetPrimType(), *replace);
        simplified = irMap->SimplifyMeExpr(replace);
        if (simplified != nullptr) { replace = simplified; }
      }
      // compute real replace
      replace = GenerateRealReplace(ratio, extraExpr, replace, realUseType, replaceCompare);
      // do replace
      if (replaceCompare) {
        replace = GetInvariant(replace);
        auto *newCmp = ReplaceCompareOpnd(static_cast<OpMeExpr&>(*use->expr), use->comparedExpr, replace);
        (void)irMap->ReplaceMeExprStmt(*use->stmt, *use->expr, *newCmp);
      } else {
        replace = use->expr == use->iv->expr ? replace :
            irMap->ReplaceMeExprExpr(*use->expr, *use->iv->expr, *replace);
        (void)irMap->ReplaceMeExprStmt(*use->stmt, *use->expr, *replace);
      }
    }
  }

  auto *preheaderLast = data->currLoop->preheader->GetLastMe();
  while (preheaderLast != nullptr && (preheaderLast->GetOp() == OP_comment || preheaderLast->GetOp() == OP_goto)) {
    preheaderLast = preheaderLast->GetPrev();
  }

  for (auto &cand : data->cands) {
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
  // clean up the invariables
  invariables.clear();
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
  invariables.clear();
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
    for (auto &group : data->groups) {
      DumpGroup(*group);
    }
    LogInfo::MapleLogger() << std::endl;
  }
  // create candidates used to replace uses's ivs
  CreateIVCandidate();
  data->considerAll = data->cands.size() <= kMaxCandidatesPerGroup;
  if (dumpDetail) {
    LogInfo::MapleLogger() << "||||Candidates||||" << std::endl;
    for (auto &cand : data->cands) {
      DumpCand(*cand);
    }
    LogInfo::MapleLogger() << std::endl;
    LogInfo::MapleLogger() << "Important Candidate:  ";
    for (auto &cand : data->cands) {
      if (cand->important) {
        LogInfo::MapleLogger() << cand->GetID() << ", ";
      }
    }
    LogInfo::MapleLogger() << std::endl << std::endl;
    LogInfo::MapleLogger() << "Group , Related Cand :\n";
    for (auto &group : data->groups) {
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
    for (auto &cand : data->cands) {
      LogInfo::MapleLogger() << "  " << cand->GetID() << "    " << cand->cost << std::endl;
    }
    LogInfo::MapleLogger() << std::endl;
    LogInfo::MapleLogger() << "||||Group-Candidate Cost||||" << std::endl;
    for (auto &group : data->groups) {
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
  for (int32 i = static_cast<int32>(loops->GetMeLoops().size()) - 1; i >= 0; i--) {
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
  for (int32 i = static_cast<int32>(loops->GetMeLoops().size()) - 1; i >= 0; --i) {
    auto *loop = loops->GetMeLoops()[i];
    if (loop->head == nullptr || loop->preheader == nullptr || loop->latch == nullptr) {
      // not canonicalized
      continue;
    }
    constexpr uint32 kMaxLoopBBSize = 60;
    if (loop->loopBBs.size() > kMaxLoopBBSize && loop->nestDepth > 0) {
      // just skip now because we can hardly get register pressure
      continue;
    }
    data = std::make_unique<IVOptData>();
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
    if ((loops->GetMeLoops().size() - static_cast<size_t>(i)) > MeOption::ivoptsLimit) {
      break;
    }
    ApplyOptimize();
  }
  useInfo->InvalidUseInfo();
  MeSSAUpdate ssaUpdate(func, *func.GetMeSSATab(), *dom, ssaupdateCands);
  ssaUpdate.Run();
}

void MEIVOpts::GetAnalysisDependence(maple::AnalysisDep &aDep) const {
  aDep.AddRequired<MEMeCfg>();
  aDep.AddRequired<MELoopCanon>();
  aDep.AddRequired<MELoopAnalysis>();
  aDep.AddRequired<MEDominance>();
  aDep.AddRequired<MEAliasClass>();
  aDep.SetPreservedAll();
}

bool MEIVOpts::PhaseRun(maple::MeFunction &f) {
  if (!MeOption::ivopts) {
    return false;
  }
  MeCFG *cfg = GET_ANALYSIS(MEMeCfg, f);
  IdentifyLoops *meLoop = GET_ANALYSIS(MELoopAnalysis, f);
  auto *dom = EXEC_ANALYSIS(MEDominance, f)->GetDomResult();
  CHECK_NULL_FATAL(dom);
  auto *pdom = EXEC_ANALYSIS(MEDominance, f)->GetPdomResult();
  CHECK_NULL_FATAL(pdom);
  auto *aliasClass = GET_ANALYSIS(MEAliasClass, f);
  MeHDSE hdse(f, *dom, *pdom, *f.GetIRMap(), aliasClass, DEBUGFUNC_NEWPM(f));
  // invoke hdse to update isLive only
  hdse.InvokeHDSEUpdateLive();

  IVOptimizer ivOptimizer(f, DEBUGFUNC_NEWPM(f), meLoop, dom);
  if (DEBUGFUNC_NEWPM(f)) {
    cfg->DumpToFile("beforeIVOpts", false);
    f.Dump();
  }
  ivOptimizer.Run();
  if (ivOptimizer.LoopOptimized()) {
    // run hdse to remove unused exprs
    auto *aliasClass0 = FORCE_GET(MEAliasClass);
    MeHDSE hdse0(f, *dom, *pdom, *f.GetIRMap(), aliasClass0, DEBUGFUNC_NEWPM(f));
    if (!MeOption::quiet) {
      LogInfo::MapleLogger() << "  == " << PhaseName() << " invokes [ " << hdse0.PhaseName() << " ] ==\n";
    }
    hdse0.DoHDSE();
    if (hdse0.NeedUNClean()) {
      f.GetCfg()->UnreachCodeAnalysis(true);
    }
  }
  return true;
}
}  // namespace maple
