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
#include "simplifyCFG.h"
#include "me_phase_manager.h"

namespace maple {
static bool debug = false;
const char *funcName = nullptr;
#define LOG_BBID(BB) ((BB)->GetBBId().GetIdx())
#define DEBUG_LOG() if (debug)            \
LogInfo::MapleLogger() << "[SimplifyCFG] "

namespace {
// After currBB's succ is changed, we can update currBB's target
static void UpdateBranchTarget(BB &currBB, BB &oldTarget, BB &newTarget, MeFunction &func) {
  // update statement offset if succ is goto target
  if (currBB.IsGoto()) {
    ASSERT(currBB.GetSucc(0) == &newTarget, "[FUNC: %s]Goto's target BB is not newTarget", func.GetName().c_str());
    auto *gotoBr = static_cast<GotoMeStmt*>(currBB.GetLastMe());
    if (gotoBr->GetOffset() != newTarget.GetBBLabel()) {
      LabelIdx label = func.GetOrCreateBBLabel(newTarget);
      gotoBr->SetOffset(label);
    }
  } else if (currBB.GetKind() == kBBCondGoto) {
    auto *condBr = static_cast<CondGotoMeStmt*>(currBB.GetLastMe());
    BB *gotoBB = currBB.GetSucc().at(1);
    ASSERT(gotoBB == &newTarget || currBB.GetSucc(0) == &newTarget,
           "[FUNC: %s]newTarget is not one of CondGoto's succ BB", func.GetName().c_str());
    LabelIdx oldLabelIdx = condBr->GetOffset();
    if (oldLabelIdx != gotoBB->GetBBLabel() && oldLabelIdx == oldTarget.GetBBLabel()) {
      // original gotoBB is replaced by newBB
      LabelIdx label = func.GetOrCreateBBLabel(*gotoBB);
      condBr->SetOffset(label);
    }
  } else if (currBB.GetKind() == kBBSwitch) {
    auto *switchStmt = static_cast<SwitchMeStmt*>(currBB.GetLastMe());
    LabelIdx oldLabelIdx = oldTarget.GetBBLabel();
    LabelIdx label = func.GetOrCreateBBLabel(newTarget);
    if (switchStmt->GetDefaultLabel() == oldLabelIdx) {
      switchStmt->SetDefaultLabel(label);
    }
    for (size_t i = 0; i < switchStmt->GetSwitchTable().size(); ++i) {
      LabelIdx labelIdx = switchStmt->GetSwitchTable().at(i).second;
      if (labelIdx == oldLabelIdx) {
        switchStmt->SetCaseLabel(switchStmt->GetSwitchTable().at(i).first, label);
      }
    }
  }
}

// BreakCritecalEdge here is for me_func
// the implementation is like the same function in class MeSplitCEdge except for exception handling
// because SimplifyCFG is used for CLANG for the time being.
static void BreakCriticalEdge(MeCFG &cfg, BB &pred, BB &succ) {
  BB *newBB = nullptr;
  size_t index = succ.GetPred().size();
  if (&pred == cfg.GetCommonEntryBB()) {
    newBB = &cfg.InsertNewBasicBlock(*cfg.GetFirstBB());
    newBB->SetAttributes(kBBAttrIsEntry);
    succ.ClearAttributes(kBBAttrIsEntry);
    pred.RemoveEntry(succ);
    pred.AddEntry(*newBB);
  } else {
    newBB = cfg.NewBasicBlock();
    while (index > 0) {
      if (succ.GetPred(index - 1) == &pred) {
        break;
      }
      index--;
    }
    pred.ReplaceSucc(&succ, newBB, false); // num of succ's pred is not changed
  }
  // pred has been remove for pred vector of succ
  // means size reduced, so index reduced
  index--;
  succ.AddPred(*newBB, index);
  newBB->SetKind(kBBFallthru);  // default kind
  newBB->SetAttributes(kBBAttrArtificial);
  DEBUG_LOG() << "Insert Empty BB in critical Edge[BB" << LOG_BBID(&pred)
              << "->BB" << LOG_BBID(newBB) << "(added)->BB" << LOG_BBID(&succ) << "]\n";
  // update statement offset if succ is goto target
  UpdateBranchTarget(pred, succ, *newBB, const_cast<MeFunction&>(cfg.GetFunc()));
}

// return true if there is critical edge, return false otherwise
// Split critical edge aroud currBB, i.e. its predecessors and successors
static bool SplitCriticalEdgeForBB(MeCFG &cfg, BB &currBB) {
  bool split = false;
  if (currBB.GetSucc().size() > 1) {
    for (auto *succ : currBB.GetSucc()) {
      if (succ->GetPred().size() > 1) {
        // critical edge is found : currBB->succ
        BreakCriticalEdge(cfg, currBB, *succ);
        split = true;
      }
    }
  }
  if (currBB.GetPred().size() > 1) {
    for (auto *pred : currBB.GetPred()) {
      if (pred->GetSucc().size() > 1) {
        // critical edge is found : currBB->succ
        BreakCriticalEdge(cfg, *pred, currBB);
        split = true;
      }
    }
  }
  return split;
}

static bool IsCriticalEdgeBB(const BB &bb) {
  if (bb.GetPred().size() != 1 || bb.GetSucc().size() != 1) {
    return false;
  }
  if (bb.GetKind() != kBBGoto && bb.GetKind() != kBBFallthru) {
    return false;
  }
  return (bb.GetPred(0)->GetSucc().size() > 1) && (bb.GetSucc(0)->GetPred().size() > 1);
}
} // anonymous namespace

static bool RemoveUnreachableBB(maple::MeFunction &f) {
  return f.GetCfg()->UnreachCodeAnalysis(true);
}

static bool MergeEmptyReturnBB(maple::MeFunction &f) {
  // WORK TO BE DONE
  (void)f;
  return false;
}

// For BB Level simplification
class SimplifyCFG {
 public:
  SimplifyCFG(BB *bb, MeFunction &func, Dominance *dominance, MeIRMap *hmap,
              MapleMap<OStIdx, MapleSet<BBId>*> *candidates, MemPool *mp, MapleAllocator *ma)
      : currBB(bb), f(func), cfg(func.GetCfg()), dom(dominance), irmap(hmap), cands(candidates), MP(mp), MA(ma) {
    (void)dom; // to be use by some optimization, not used now
    (void)irmap;
  }
  // run on each BB(currBB) until no more change for currBB
  bool RunIterativelyOnBB();

 private:
  BB *currBB = nullptr;     // BB we currently perform simplification on
  MeFunction &f;            // function we currently perform simplification on
  MeCFG *cfg = nullptr;     // requiring cfg to find pattern to simplify
  Dominance *dom = nullptr; // some simplification need to check dominance
  MeIRMap *irmap = nullptr; // used to create new MeExpr/MeStmt
  MapleMap<OStIdx, MapleSet<BBId>*> *cands = nullptr; // candidates ost need to be updated ssa
  MemPool *MP = nullptr;    // for creating elements to cands
  MapleAllocator *MA = nullptr; // for creating elements to cands

  bool runOnSameBBAgain = false; // It will be always set false by RunIterativelyOnBB. If there is some optimization
                                 // opportunity for currBB after a/some simplification, we should set it true.
                                 // Otherwise it will stop after all simplification finished and continue to nextBB.
  enum BBErrStat {
    kBBNoErr,                 // BB is normal
    kBBErrNull,               // BB is nullptr
    kBBErrOOB,                // BB id is out-of-bound
    kBBErrCommonEntryOrExit,  // BB is CommoneEntry or CommonExit
    kBBErrDel                 // BB has been deleted before
  };

  BBErrStat bbErrStat = kBBNoErr;

  // run once time on bb, some common cfg opt and peephole cfg opt
  // will be performed on currBB
  bool RunOnceOnBB();
  // elminate BB that is unreachable:
  // 1.BB has no pred(expect then entry block)
  // 2.BB has itself as pred
  bool EliminateDeadBB();
  // chang condition branch to unconditon branch if possible
  // 1.condition is a constant
  // 2.all branches of condition branch is the same BB
  bool ChangeCondBr2UnCond();
  // eliminate PHI
  // 1.PHI has only one opnd
  // 2.PHI is duplicate(all opnds is the same as another one in the same BB)
  // return true if cfg is changed
  bool EliminatePHI();
  // disconnect predBB and currBB if predBB must cause error(e.g. null ptr deref)
  // If a expr is always cause error in predBB, predBB will never reach currBB
  bool DisconnectErrorIntroducingPredBB();
  // currBB has only one pred, and pred has only one succ
  // try to merge two BB, return true if merged, return false otherwise.
  bool MergeDistinctBBPair();
  // sink common code to their common succ BB, decrease the register pressure
  bool SinkCommonCode();
  // Following function check for BB pattern according to BB's Kind
  bool SimplifyCondBB();
  bool SimplifyUncondBB();
  bool SimplifyFallthruBB();
  bool SimplifyReturnBB();
  bool SimplifySwitchBB();

  // for sub-pattern in SimplifyCondBB
  bool FoldBranchToCommonDest();
  // for MergeDistinctBBPair
  BB *MergeDistinctBBPair(BB *pred, BB *succ);
  // merge two bb, if merged, return combinedBB, Otherwise return nullptr
  BB *CombineTwoBB(BB *pred, BB *succ);
  // for SimplifyUncondBB
  bool MergeGotoBBToPred(BB *gotoBB, BB *pred);

  // Check before every simplification to avoid error induced by other optimization on currBB
  // please use macro CHECK_CURR_BB instead
  bool CheckCurrBB();
  // Insert ost of philist in bb to cand
  void UpdateCand(BB *bb);
  // eliminate phi for specified BB
  bool EliminatePHIForBB(BB *bb);

  // Remove Pred and add succ's philist to cand
  void RemovePred(BB *succ, BB *pred, bool predNumChanged); // if predNumChanged, we should updatePhi
  // Remove succ and add succ's philist to cand
  void RemoveSucc(BB *pred, BB *succ, bool predNumChanged); // if predNumChanged, we should updatePhi
  // Delete BB and add its philist to cand
  void DeleteBB(BB *bb);

  void SetBBRunAgain() {
    runOnSameBBAgain = true;
  }
  void ResetBBRunAgain() {
    runOnSameBBAgain = false;
  }
#define CHECK_CURR_BB() \
if (!CheckCurrBB()) {   \
  return false;         \
}
};

bool SimplifyCFG::CheckCurrBB() {
  if (bbErrStat != kBBNoErr) { // has been checked before
    return false;
  }
  if  (currBB == nullptr) {
    bbErrStat = kBBErrNull;
    return false;
  }
  if (currBB->GetBBId() >= cfg->GetAllBBs().size()) {
    bbErrStat = kBBErrOOB;
    return false;
  }
  if (currBB == cfg->GetCommonEntryBB() || currBB == cfg->GetCommonExitBB()) {
    bbErrStat = kBBErrCommonEntryOrExit;
    return false;
  }
  if (cfg->GetBBFromID(currBB->GetBBId()) == nullptr) {
    // BB is deleted, it will be set nullptr in cfg's bbvec, but currBB has been set before(, so it is not null)
    bbErrStat = kBBErrDel;
    return false;
  }
  bbErrStat = kBBNoErr;
  return true;
}

void SimplifyCFG::UpdateCand(BB *bb) {
  if (bb == nullptr || bb->GetMePhiList().empty()) {
    return;
  }
  for (auto phi : bb->GetMePhiList()) {
    OStIdx ostIdx = phi.first;
    if (cands->find(ostIdx) == cands->end()) {
      MapleSet<BBId> *bbSet = MP->New<MapleSet<BBId>>(std::less<BBId>(), MA->Adapter());
      bbSet->insert(bb->GetBBId());
      (*cands)[ostIdx] = bbSet;
    } else {
      (*cands)[ostIdx]->insert(bb->GetBBId());
    }
  }
}

void SimplifyCFG::RemovePred(BB *succ, BB *pred, bool predNumChanged) {
  // if pred num is not changed, its phiOpnd num will be the same as before.
  // and we should not updatephi to remove phiOpnd
  ASSERT(succ->IsSuccBB(*pred), "[FUNC: %s]succ is not pred's successor");
  ASSERT(pred->IsPredBB(*succ), "[FUNC: %s]pred is not succ's predecessor");
  succ->RemovePred(*pred, predNumChanged);
  UpdateCand(succ);
}

void SimplifyCFG::RemoveSucc(BB *pred, BB *succ, bool predNumChanged) {
  // if pred num is not changed, its phiOpnd num will be the same as before.
  // and we should not updatephi to remove phiOpnd
  ASSERT(succ->IsSuccBB(*pred), "[FUNC: %s]succ is not pred's successor");
  ASSERT(pred->IsPredBB(*succ), "[FUNC: %s]pred is not succ's predecessor");
  pred->RemoveSucc(*succ, predNumChanged);
  UpdateCand(succ); // we update cand for succ, because its pred is changed
}

void SimplifyCFG::DeleteBB(BB *bb) {
  if (bb == nullptr) {
    return;
  }
  UpdateCand(bb);
  cfg->DeleteBasicBlock(*bb);
}

// eliminate dead BB :
// 1.BB has no pred(expect then entry block)
// 2.BB has only itself as pred
bool SimplifyCFG::EliminateDeadBB() {
  CHECK_CURR_BB();
  if (currBB->GetAttributes(kBBAttrIsEntry)) {
    return false;
  }
  if (currBB->GetPred().empty() || currBB->GetUniquePred() == currBB) {
    currBB->RemoveAllSucc();
    DeleteBB(currBB);
    return true;
  }
  return false;
}

// chang condition branch to unconditon branch if possible
// 1.condition is a constant
// 2.all branches of condition branch is the same BB
bool SimplifyCFG::ChangeCondBr2UnCond() {
  CHECK_CURR_BB();
  return false;
}

// eliminate PHI for specified BB
// 1.PHI has only one opnd
// 2.PHI is duplicate(all opnds is the same as another one in the same BB)
// return true if cfg is changed
bool SimplifyCFG::EliminatePHIForBB(BB *bb) {
  if (bb->GetMePhiList().empty()) {
    return false;
  }
  // 1.PHI has only one opnd
  if (bb->GetUniquePred() != nullptr) {
    UpdateCand(bb);
    bb->ClearMePhiList();
    DEBUG_LOG() << "Eliminate MEPHI list in BB" << LOG_BBID(bb) << "\n";
    return false; // cfg not changed
  }
  return false;
}

// eliminate PHI
// 1.PHI has only one opnd
// 2.PHI is duplicate(all opnds is the same as another one in the same BB)
// return true if cfg is changed
bool SimplifyCFG::EliminatePHI() {
  CHECK_CURR_BB();
  return false;
}

// disconnect predBB and currBB if predBB must cause error(e.g. null ptr deref)
// If a expr is always cause error in predBB, predBB will never reach currBB
bool SimplifyCFG::DisconnectErrorIntroducingPredBB() {
  CHECK_CURR_BB();
  return false;
}

// merge two bb, if merged, return combinedBB, Otherwise return nullptr
BB *SimplifyCFG::CombineTwoBB(BB *pred, BB *succ) {
  ASSERT(pred != cfg->GetCommonEntryBB(), "[FUNC: %s]Not allowed to merge BB to commonEntry", funcName);
  ASSERT(succ != cfg->GetCommonExitBB(), "[FUNC: %s]Not allowed to merge commonExit to pred", funcName);
  ASSERT(pred->GetUniqueSucc() == succ, "[FUNC: %s]Only allow pattern one pred and one succ", funcName);
  ASSERT(succ->GetUniquePred() == pred, "[FUNC: %s]Only allow pattern one pred and one succ", funcName);
  if (pred->GetKind() != kBBFallthru) {
    // remove last mestmt
    ASSERT(pred->IsGoto(), "[FUNC: %s]Only goto and fallthru BB is allowed", funcName);
    pred->RemoveLastMeStmt();
    pred->SetKind(kBBFallthru);
  }
  // merge succ to pred no matter whether pred is empty or not
  for (MeStmt *stmt = succ->GetFirstMe(); stmt != nullptr;) {
    MeStmt *next = stmt->GetNextMeStmt();
    succ->RemoveMeStmt(stmt);
    pred->AddMeStmtLast(stmt);
    stmt = next;
  }
  succ->MoveAllSuccToPred(pred, cfg->GetCommonExitBB());
  RemoveSucc(pred, succ, true);
  pred->SetAttributes(succ->GetAttributes());
  pred->SetKind(succ->GetKind());
  DEBUG_LOG() << "Merge successor BB" << LOG_BBID(succ) << " to predecessor BB"
              << LOG_BBID(pred) << ", and delete successor BB" << LOG_BBID(succ) << "\n";
  DeleteBB(succ);
  return pred;
}

BB *SimplifyCFG::MergeDistinctBBPair(BB *pred, BB *succ) {
  if (pred == nullptr || succ == nullptr || succ == pred) {
    return nullptr;
  }
  if (pred != succ->GetUniquePred() || pred == cfg->GetCommonEntryBB()) {
    return nullptr;
  }
  if (succ != pred->GetUniqueSucc() || succ == cfg->GetCommonExitBB()) {
    return nullptr;
  }
  // start merging currBB to predBB
  return CombineTwoBB(pred, succ);
}

// currBB has only one pred, and pred has only one succ
// try to merge two BB, return true if merged, return false otherwise.
bool SimplifyCFG::MergeDistinctBBPair() {
  CHECK_CURR_BB();
  return false;
  bool everChanged = false;
  BB *combineBB = MergeDistinctBBPair(currBB->GetUniquePred(), currBB);
  if (combineBB != nullptr) {
    currBB = combineBB;
    everChanged = true;
  }
  combineBB = MergeDistinctBBPair(currBB, currBB->GetUniqueSucc());
  if (combineBB != nullptr) {
    currBB = combineBB;
    everChanged = true;
  }
  if (everChanged) {
    SetBBRunAgain();
  }
  return everChanged;
}

// sink common code to their common succ BB, decrease the register pressure
bool SimplifyCFG::SinkCommonCode() {
  CHECK_CURR_BB();
  return false;
}

// CurrBB is Condition BB, we will look upward its predBB(s) to see if we can simplify
// 1. currBB is X == constVal, and predBB has checked for the same expr, the result is known for currBB's condition,
//    so we can make currBB to be an uncondBB.
// 2. predBB is CondBB, one of predBB's succBB is currBB, and another is one of currBB's successors(commonBB)
//    we can merge currBB to predBB if currBB is simple enough(has only one stmt).
// 3. currBB has only one stmt(conditional branch stmt), and the condition's value is calculated by all its predBB
//    we can hoist currBB's stmt to predBBs if it is profitable
bool SimplifyCFG::SimplifyCondBB() {
  CHECK_CURR_BB();
  MeStmt *stmt = currBB->GetLastMe();
  CHECK_FATAL(stmt != nullptr, "[FUNC: %s] CondBB has no stmt", f.GetName().c_str());
  CHECK_FATAL(kOpcodeInfo.IsCondBr(stmt->GetOp()), "[FUNC: %s] Opcode is error!", f.GetName().c_str());
  // 2.fold two continuous condBB to one condBB, use or/and to combine two condition
  if (FoldBranchToCommonDest()) {
    ResetBBRunAgain();
    return true;
  }
  return false;
}

bool SimplifyCFG::MergeGotoBBToPred(BB *succ, BB *pred) {
  if (pred == nullptr || succ == nullptr) {
    return false;
  }
  // If we merge succ to pred, a new BB will be create to split the same critical edge
  if (IsCriticalEdgeBB(*pred)) {
    return false;
  }
  if (succ == pred) {
    return false;
  }
  if (succ->GetAttributes(kBBAttrIsEntry) || !succ->IsGoto() || succ->IsMeStmtEmpty()) {
    return false;
  }
  // BB has only one stmt(i.e. unconditional goto stmt)
  if (succ->GetLastMe() != succ->GetFirstMe()) {
    return false;
  }
  BB *newTarget = succ->GetSucc(0); // succ must have only one succ, because it is uncondBB
  // newTarget has only one pred, skip, because MergeDistinctBBPair will deal with this case
  if (newTarget->GetPred().size() == 1) {
    return false;
  }
  int predIdx = succ->GetPredIndex(*pred);
  // pred is moved to newTarget
  if (pred->GetKind() == kBBFallthru) {
    GotoMeStmt *gotoMeStmt = irmap->New<GotoMeStmt>(newTarget->GetBBLabel());
    gotoMeStmt->SetSrcPos(succ->GetLastMe()->GetSrcPosition());
    gotoMeStmt->SetBB(pred);
    pred->AddMeStmtLast(gotoMeStmt);
    pred->SetKind(kBBGoto);
    DEBUG_LOG() << "Insert Uncond stmt to fallthru BB" << LOG_BBID(currBB) << ", and goto BB" << LOG_BBID(newTarget)
                << "\n";
  }
  pred->ReplaceSucc(succ, newTarget); // phi opnd is not removed from currBB's philist, we will remove it later

  DEBUG_LOG() << "Merge Uncond BB" << LOG_BBID(succ) << " to its pred BB" << LOG_BBID(pred)
              << ": BB" << LOG_BBID(pred) << "->BB" << LOG_BBID(succ) << "(merged)->BB" << LOG_BBID(newTarget) << "\n";
  UpdateBranchTarget(*pred, *succ, *newTarget, f);
  // update phi
  auto &newPhiList = newTarget->GetMePhiList();
  auto &gotoPhilist = succ->GetMePhiList();
  if (newPhiList.empty()) { // newTarget has only one pred(i.e. succ) before
    // we copy succ's philist to newTarget, but not with all phiOpnd
    for (auto &phiNode : gotoPhilist) {
      auto *phiMeNode = irmap->NewInPool<MePhiNode>();
      phiMeNode->SetDefBB(newTarget);
      newPhiList.emplace(phiNode.first, phiMeNode);
      phiMeNode->GetOpnds().push_back(phiNode.second->GetLHS()); // succ is the first pred of newTarget
      // new pred to newTarget
      phiMeNode->GetOpnds().push_back(phiNode.second->GetOpnd(predIdx));
      OStIdx ostIdx = phiNode.first;
      // create a new version for new phi
      phiMeNode->SetLHS(irmap->CreateRegOrVarMeExprVersion(ostIdx));
    }
  } else { // newTarget has other pred besides succ
    for (auto &phi : newPhiList) {
      OStIdx ostIdx = phi.first;
      auto it = gotoPhilist.find(ostIdx);
      if (it != gotoPhilist.end()) {
        // new pred to newTarget
        phi.second->GetOpnds().push_back(it->second->GetOpnd(predIdx));
      } else {
        int index = newTarget->GetPredIndex(*succ);
        ASSERT(index != -1, "[FUNC: %s]succ is not newTarget's pred", f.GetName().c_str());
        // new pred's phi opnd is the same as succ.
        phi.second->GetOpnds().push_back(phi.second->GetOpnd(index));
      }
    }
  }
  // remove redundant phiOpnd of succ's philist
  succ->RemovePhiOpnd(predIdx);
  UpdateCand(newTarget);
  // remove succ if succ has no pred
  if (succ->GetPred().empty()) {
    RemovePred(newTarget, succ, true);
    DEBUG_LOG() << "Delete Uncond BB" << LOG_BBID(succ) << " after merged to all its preds\n";
    DeleteBB(succ);
  }
  // split critical edges for newTarget
  (void)SplitCriticalEdgeForBB(*cfg, *newTarget);
  return true;
}

// if unconditional BB has only one uncondBr stmt, we try to merge it to its pred
// this is different from MergeDistinctBBPair, it is not required that current uncondBB has only one pred
bool SimplifyCFG::SimplifyUncondBB() {
  CHECK_CURR_BB();
  if (currBB->GetAttributes(kBBAttrIsEntry) || !currBB->IsGoto() || currBB->IsMeStmtEmpty()) {
    return false;
  }
  // BB has only one stmt(i.e. unconditional goto stmt)
  if (currBB->GetLastMe() != currBB->GetFirstMe()) {
    return false;
  }
  // jump to itself
  if (currBB->GetSucc(0) == currBB) {
    return false;
  }
  bool changed = false;
  // try to move pred from currBB to newTarget
  for (size_t i = 0; i < currBB->GetPred().size();) {
    BB *pred = currBB->GetPred(i);
    if (pred == nullptr || pred->GetLastMe() == nullptr) {
      ++i;
      continue;
    }
    if (pred->IsGoto() || (pred->GetLastMe()->IsCondBr() && currBB == pred->GetSucc(1))) {
      // pred is moved to newTarget
      bool merged = MergeGotoBBToPred(currBB, pred);
      if (merged) {
        changed = true;
      } else {
        ++i;
      }
    } else {
      ++i;
    }
  }
  return changed;
}

bool SimplifyCFG::SimplifyFallthruBB() {
  CHECK_CURR_BB();
  if (IsCriticalEdgeBB(*currBB)) {
    return false;
  }
  BB *succ = currBB->GetSucc(0);
  if (succ == nullptr || succ == currBB) {
    return false;
  }
  if (succ->GetAttributes(kBBAttrIsEntry) || !succ->IsGoto() || succ->IsMeStmtEmpty()) {
    return false;
  }
  // BB has only one pred/succ, skip, because MergeDistinctBBPair will deal with this case
  // note: gotoBB may have two succ, the second succ is inserted by cfg wont exit analysis
  if (succ->GetPred().size() == 1 || succ->GetSucc().size() == 1) {
    return false;
  }
  // BB has only one stmt(i.e. unconditional goto stmt)
  if (succ->GetLastMe() != succ->GetFirstMe()) {
    return false;
  }
  return MergeGotoBBToPred(succ, currBB);
}

bool SimplifyCFG::SimplifyReturnBB() {
  CHECK_CURR_BB();
  return false;
}

bool SimplifyCFG::SimplifySwitchBB() {
  CHECK_CURR_BB();
  return false;
}

bool SimplifyCFG::FoldBranchToCommonDest() {
  CHECK_CURR_BB();
  return false;
}

bool SimplifyCFG::RunOnceOnBB() {
  CHECK_CURR_BB();
  bool everChanged = false;
  // eliminate dead BB :
  // 1.BB has no pred(expect then entry block)
  // 2.BB has itself as pred
  everChanged |= EliminateDeadBB();

  // chang condition branch to unconditon branch if possible
  // 1.condition is a constant
  // 2.all branches of condition branch is the same BB
  everChanged |= ChangeCondBr2UnCond();

  // eliminate PHI
  // 1.PHI has only one opnd
  // 2.PHI is duplicate(all opnds is the same as another one in the same BB)
  // return true if cfg is changed
  everChanged |= EliminatePHI();

  // disconnect predBB and currBB if predBB must cause error(e.g. null ptr deref)
  // If a expr is always cause error in predBB, predBB will never reach currBB
  everChanged |= DisconnectErrorIntroducingPredBB();

  // merge currBB to predBB if currBB has only one predBB and predBB has only one succBB
  everChanged |= MergeDistinctBBPair();

  // sink common code to their common succ BB, decrease the register pressure
  everChanged |= SinkCommonCode();

  switch (currBB->GetKind()) {
    case kBBCondGoto: {
      everChanged |= SimplifyCondBB();
      break;
    }
    case kBBGoto: {
      everChanged |= SimplifyUncondBB();
      break;
    }
    case kBBFallthru: {
      everChanged |= SimplifyFallthruBB();
      break;
    }
    case kBBReturn: {
      everChanged |= SimplifyReturnBB();
      break;
    }
    case kBBSwitch: {
      everChanged |= SimplifySwitchBB();
      break;
    }
    default: {
      // do nothing for the time being
      break;
    }
  }
  return everChanged;
}

// run on each BB until no more change for currBB
bool SimplifyCFG::RunIterativelyOnBB() {
  bool changed = false;
  do {
    runOnSameBBAgain = false;
    // run only once on currBB, if we should run simplify on the same BB, runOnSameBBAgain will be set by optimization
    changed |= RunOnceOnBB();
  } while (runOnSameBBAgain);
  return changed;
}

// For function Level simplification
class SimplifyFuntionCFG {
 public:
  SimplifyFuntionCFG(maple::MeFunction &func, Dominance *dominance, MeIRMap *hmap,
                     MapleMap<OStIdx, MapleSet<BBId>*> *candidates, MemPool *mp, MapleAllocator *ma)
      : f(func), dom(dominance), irmap(hmap), cands(candidates), MP(mp), MA(ma) {}

  bool RunSimplifyOnFunc();

 private:
  MeFunction &f;
  Dominance *dom = nullptr;
  MeIRMap *irmap = nullptr;

  MapleMap<OStIdx, MapleSet<BBId>*> *cands = nullptr; // candidates ost need to update ssa
  MemPool *MP = nullptr;
  MapleAllocator *MA = nullptr;

  bool RepeatSimplifyFunctionCFG();
  bool SimplifyCFGForBB(BB *currBB);
};

bool SimplifyFuntionCFG::SimplifyCFGForBB(BB *currBB) {
  return SimplifyCFG(currBB, f, dom, irmap, cands, MP, MA)
      .RunIterativelyOnBB();
}
// run until no changes happened
bool SimplifyFuntionCFG::RepeatSimplifyFunctionCFG() {
  bool everChanged = false;
  bool changed = true;
  while (changed) {
    changed = false;
    // Since we may delete BB during traversal, we cannot use iterator here.
    auto &bbVec = f.GetCfg()->GetAllBBs();
    for (size_t idx = 0; idx < bbVec.size(); ++idx) {
      BB *currBB = bbVec[idx];
      if (currBB == nullptr) {
        continue;
      }
      changed |= SimplifyCFGForBB(currBB);
    }
    everChanged |= changed;
  }
  return everChanged;
}

bool SimplifyFuntionCFG::RunSimplifyOnFunc() {
  bool everChanged = RemoveUnreachableBB(f);
  everChanged |= MergeEmptyReturnBB(f);
  everChanged |= RepeatSimplifyFunctionCFG();
  if (!everChanged) {
    return false;
  }
  // RepeatSimplifyFunctionCFG may generate unreachable BB.
  // So RemoveUnreachableBB should be called to check for and
  // remove dead BB. And RemoveUnreachableBB may also generate
  // other optimize opportunity for RepeatSimplifyFunctionCFG.
  // Hench we should iterate between these two optimizations.
  // Here, we call RemoveUnreachableBB first to avoid running
  // RepeatSimplifyFunctionCFG for no changed situation.
  if (!RemoveUnreachableBB(f)) {
    return true;
  }
  do {
    everChanged = RepeatSimplifyFunctionCFG();
    everChanged |= RemoveUnreachableBB(f);
  } while (everChanged);
  return true;
}

static bool SkipSimplifyCFG(maple::MeFunction &f) {
  for (auto bbIt = f.GetCfg()->valid_begin(); bbIt != f.GetCfg()->valid_end(); ++bbIt) {
    if ((*bbIt)->GetKind() == kBBIgoto) {
      return true;
    }
  }
  return false;
}

void MESimplifyCFG::GetAnalysisDependence(maple::AnalysisDep &aDep) const {
  aDep.AddRequired<MEDominance>();
  aDep.AddRequired<MEIRMapBuild>();
  aDep.SetPreservedAll();
}

bool MESimplifyCFG::PhaseRun(maple::MeFunction &f) {
  auto *dom = GET_ANALYSIS(MEDominance, f);
  auto *irmap = GET_ANALYSIS(MEIRMapBuild, f);
  debug = DEBUGFUNC_NEWPM(f);
  funcName = f.GetName().c_str();
  if (SkipSimplifyCFG(f)) {
    DEBUG_LOG() << "Skip SimplifyCFG phase because of igotoBB\n";
    return false;
  }
  DEBUG_LOG() << "Start Simplifying Function : " << f.GetName() << "\n";
  if (debug) {
    f.GetCfg()->DumpToFile("Before_SimplifyCFG");
  }
  auto *MP = GetPhaseMemPool();
  MapleAllocator MA = MapleAllocator(MP);
  MapleMap<OStIdx, MapleSet<BBId>*> cands((std::less<OStIdx>(), MA.Adapter()));
  uint32 bbNumBefore = f.GetCfg()->ValidBBNum();
  // simplify entry
  bool change = SimplifyFuntionCFG(f, dom, irmap, &cands, MP, &MA).RunSimplifyOnFunc();
  if (change) {
    if (debug) {
      uint32 bbNumAfter = f.GetCfg()->ValidBBNum();
      if (bbNumBefore != bbNumAfter) {
        DEBUG_LOG() << "BBs' number " << (bbNumBefore > bbNumAfter ? "reduce" : "increase")
                    << " from " << bbNumBefore << " to " << bbNumAfter << "\n";
      }
      f.GetCfg()->DumpToFile("After_SimplifyCFG");
      f.Dump(false);
    }
    FORCE_INVALID(MEDominance, f);
    dom = FORCE_GET(MEDominance);
    if (!cands.empty()) {
      MeSSAUpdate ssaUpdate(f, *f.GetMeSSATab(), *dom, cands, *MP);
      ssaUpdate.Run();
    }
  }
  return change;
}
} // namespace maple

