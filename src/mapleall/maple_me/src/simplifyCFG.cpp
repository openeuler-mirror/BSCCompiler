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
#define LOG_BBID(BB) ((BB)->GetBBId().GetIdx())
#define DEBUG_LOG() if (debug)            \
LogInfo::MapleLogger() << "[SimplifyCFG] "

namespace {
static bool debug = false;
const char *funcName = nullptr;

// pred-connecting-succ
// connectingBB has only one pred and succ, and has no stmt (except a single gotoStmt) in it
bool IsConnectingBB(BB &bb) {
  return (bb.GetPred().size() == 1 && bb.GetSucc().size() == 1) &&
         (IsEmptyBB(bb) || HasOnlyGotoStmt(bb));
}

// RealSucc is a non-connecting BB which is not empty (or has just a single gotoStmt).
// If we want to find the non-empty succ of currBB, we start from the succ (i.e. the argument)
// skip those connecting bb used to connect its pred and succ, like: pred -- connecting -- succ
BB *FindFirstRealSucc(BB *succ) {
  while (succ != nullptr && IsConnectingBB(*succ)) {
    succ = succ->GetSucc(0);
  }
  return succ;
}

// delete all empty bb used to connect its pred and succ, like: pred -- empty -- empty -- succ
// the result after this will be : pred -- succ
// if no empty exist, return;
// we will stop at stopBB(stopBB will not be deleted), if stopBB is nullptr, means no constraint
void EliminateEmptyConnectingBB(BB *predBB, BB *emptyBB, BB *stopBB, MeCFG &cfg) {
  if (emptyBB == stopBB && emptyBB != nullptr && predBB->IsPredBB(*stopBB)) {
    return;
  }
  // we can only eliminate those emptyBBs that have only one pred and succ
  while (emptyBB != nullptr && emptyBB != stopBB && IsConnectingBB(*emptyBB)) {
    BB *succ = emptyBB->GetSucc(0);
    BB *pred = emptyBB->GetPred(0);
    DEBUG_LOG() << "Delete empty connecting : BB" << LOG_BBID(pred) << "->BB" << LOG_BBID(emptyBB)
                << "(deleted)->BB" << LOG_BBID(succ) << "\n";
    succ->ReplacePred(emptyBB, pred);
    emptyBB->RemoveAllPred();
    cfg.DeleteBasicBlock(*emptyBB);
    emptyBB = succ;
  }
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
  BB *MergeSuccIntoPred(BB *pred, BB *succ);
  bool CondBranchToSelect();
  bool IsProfitableForCond2Sel(MeExpr *condExpr, MeExpr *trueExpr, MeExpr *falseExpr);
  void CombineCond2SelPattern(BB *condBB, BB *ftBB, BB *gtBB, BB *jointBB);
  // for SimplifyUncondBB
  bool MergeGotoBBToPred(BB *gotoBB, BB *pred);
  // after moving pred from curr to curr's successor (i.e. succ), update the phiList of curr and succ
  // a phiOpnd will be removed from curr's philist, and a phiOpnd will be inserted to succ's philist
  // note: when replace pred's succ (i.e. curr) with succ, please DO NOT remove phiOpnd immediately,
  // otherwise we cannot get phiOpnd in this step
  void UpdatePhiForMovingPred(int predIdxForCurr, BB *pred, BB *curr, BB *succ);
  // for ChangeCondBr2UnCond
  bool SimplifyBranchBBToUncondBB(BB &bb);

  // Check before every simplification to avoid error induced by other optimization on currBB
  // please use macro CHECK_CURR_BB instead
  bool CheckCurrBB();
  // Insert ost of philist in bb to cand, and set ost start from newBB(newBB will be bb itself if not specified)
  void UpdateSSACandForBBPhiList(BB *bb, BB *newBB = nullptr);
  void UpdateSSACandForOst(OStIdx ostIdx, BB *bb);

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

void SimplifyCFG::UpdateSSACandForOst(OStIdx ostIdx, BB *bb) {
  if (cands->find(ostIdx) == cands->end()) {
    MapleSet<BBId> *bbSet = MP->New<MapleSet<BBId>>(std::less<BBId>(), MA->Adapter());
    bbSet->insert(bb->GetBBId());
    (*cands)[ostIdx] = bbSet;
  } else {
    (*cands)[ostIdx]->insert(bb->GetBBId());
  }
}

void SimplifyCFG::UpdateSSACandForBBPhiList(BB *bb, BB *newBB) {
  if (bb == nullptr || bb->GetMePhiList().empty()) {
    return;
  }
  if (newBB == nullptr) { // if not specified, is bb itself
    newBB = bb;
  }
  for (auto phi : bb->GetMePhiList()) {
    OStIdx ostIdx = phi.first;
    UpdateSSACandForOst(ostIdx, newBB);
  }
}

void SimplifyCFG::DeleteBB(BB *bb) {
  if (bb == nullptr) {
    return;
  }
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

// all branches of bb are the same if skipping all empty connecting BB
// turn it into an unconditional BB (if destBB has other pred, bb->gotoBB, otherwise, bb->fallthruBB)
//      bb                             bb
//   /  |  \                            |
//  A   B   C  (ABC are empty)  ==>     |
//   \  |  /                            |
//    destBB                          destBB
bool SimplifyCFG::SimplifyBranchBBToUncondBB(BB &bb) {
  if (bb.GetKind() != kBBCondGoto && bb.GetKind() != kBBSwitch) {
    return false;
  }
  // check if all successors of bb branch to the same destination (ignoring all empty bb between bb and destination)
  BB *destBB = FindFirstRealSucc(bb.GetSucc(0));
  for (size_t i = 1; i < bb.GetSucc().size(); ++i) {
    if (FindFirstRealSucc(bb.GetSucc(i)) != destBB) {
      return false;
    }
  }

  DEBUG_LOG() << "SimplifyBranchBBToUncondBB : " << (bb.GetKind() == kBBCondGoto ? "Conditional " : "Switch ")
              << "BB" << LOG_BBID(&bb) << " to unconditional BB\n";
  // delete all empty bb between bb and destBB
  // note : bb and destBB will be connected after empty BB is deleted
  for (int i = bb.GetSucc().size() - 1; i >= 0; --i) {
    EliminateEmptyConnectingBB(&bb, bb.GetSucc(i), destBB, *cfg);
  }
  while (bb.GetSucc().size() != 1) { // bb is an unconditional bb now, and its successor num should be 1
    ASSERT(bb.GetSucc().back() == destBB, "[FUNC: %s]Goto BB%d has different destination", funcName, LOG_BBID(&bb));
    bb.RemoveSucc(*bb.GetSucc().back());
  }
  if (destBB->GetPred().size() > 1) {
    LabelIdx label = f.GetOrCreateBBLabel(*destBB);
    auto *gotoStmt = irmap->CreateGotoMeStmt(label, &bb, &bb.GetLastMe()->GetSrcPosition());
    bb.RemoveLastMeStmt();
    bb.AddMeStmtLast(gotoStmt);
    bb.SetKind(kBBGoto);
  } else {
    bb.RemoveLastMeStmt();
    bb.SetKind(kBBFallthru);
  }
  return true;
}

// chang condition branch to unconditon branch if possible
// 1.condition is a constant
// 2.all branches of condition branch is the same BB
bool SimplifyCFG::ChangeCondBr2UnCond() {
  CHECK_CURR_BB();
  if (currBB->GetKind() != kBBCondGoto) {
    return false;
  }
  // this step is to check if other opt does not maintain the BBKind and its stmt
  if (currBB->IsMeStmtEmpty() || !kOpcodeInfo.IsCondBr(currBB->GetLastMe()->GetOp())) {
    return false;
  }
  // case 2
  if (currBB->GetSucc(0) == currBB->GetSucc(1)) {
    if (SimplifyBranchBBToUncondBB(*currBB)) {
      SetBBRunAgain();
      return true;
    }
  }
  MeExpr *condExpr = currBB->GetLastMe()->GetOpnd(0);
  // case 1
  if (condExpr->GetMeOp() == kMeOpConst) {
    // work to be done later
    return false;
  }
  // case 1 : work to be done - condExpr is not a const, but all its opnd is const but has not been constfolded
  return false;
}

// disconnect predBB and currBB if predBB must cause error(e.g. null ptr deref)
// If a expr is always cause error in predBB, predBB will never reach currBB
bool SimplifyCFG::DisconnectErrorIntroducingPredBB() {
  CHECK_CURR_BB();
  return false;
}

// merge two bb, if merged, return combinedBB, Otherwise return nullptr
BB *SimplifyCFG::MergeSuccIntoPred(BB *pred, BB *succ) {
  ASSERT(pred != cfg->GetCommonEntryBB(), "[FUNC: %s]Not allowed to merge BB to commonEntry", funcName);
  ASSERT(succ != cfg->GetCommonExitBB(), "[FUNC: %s]Not allowed to merge commonExit to pred", funcName);
  ASSERT(pred->GetUniqueSucc() == succ, "[FUNC: %s]Only allow pattern one pred and one succ", funcName);
  ASSERT(succ->GetUniquePred() == pred, "[FUNC: %s]Only allow pattern one pred and one succ", funcName);
  if (pred->GetKind() == kBBGoto) {
    // remove last mestmt
    ASSERT(pred->GetLastMe()->GetOp() == OP_goto, "[FUNC: %s]GotoBB has no goto stmt as its terminator", funcName);
    pred->RemoveLastMeStmt();
    pred->SetKind(kBBFallthru);
  }
  if (pred->GetKind() != kBBFallthru) {
    // Only goto and fallthru BB is allowed
    return nullptr;
  }
  // merge succ to pred no matter whether pred is empty or not
  for (MeStmt *stmt = succ->GetFirstMe(); stmt != nullptr;) {
    MeStmt *next = stmt->GetNextMeStmt();
    succ->RemoveMeStmt(stmt);
    pred->AddMeStmtLast(stmt);
    stmt = next;
  }
  succ->MoveAllSuccToPred(pred, cfg->GetCommonExitBB());
  pred->RemoveSucc(*succ, true);
  pred->SetAttributes(succ->GetAttributes());
  pred->SetKind(succ->GetKind());
  DEBUG_LOG() << "Merge successor BB" << LOG_BBID(succ) << " to predecessor BB"
              << LOG_BBID(pred) << ", and delete successor BB" << LOG_BBID(succ) << "\n";
  UpdateSSACandForBBPhiList(succ, pred);
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
  return MergeSuccIntoPred(pred, succ);
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

static bool IsSafeForCond2Sel(MeExpr *expr) {
  if (expr->GetMeOp() == kMeOpIvar) { // do not use HasIvar here for efficiency reasons
    return false;
  }
  if (expr->GetOp() == OP_div || expr->GetOp() == OP_rem) {
    MeExpr *opnd1 = expr->GetOpnd(1);
    if (opnd1->GetMeOp() == kMeOpConst && !opnd1->IsZero()) {
      return true;
    }
    // we are not sure whether opnd1 may be zero
    return false;
  }
  for (size_t i = 0; i < expr->GetNumOpnds(); ++i) {
    if (!IsSafeForCond2Sel(expr->GetOpnd(i))) {
      return false;
    }
  }
  return true;
}

static bool IsSimpleImm(uint64 imm) {
  return ((imm & (static_cast<uint64>(0xffff) << 48u)) == imm) ||
         ((imm & (static_cast<uint64>(0xffff) << 32u)) == imm) ||
         ((imm & (static_cast<uint64>(0xffff) << 16u)) == imm) ||
         ((imm & (static_cast<uint64>(0xffff))) == imm) ||
         (((~imm) & (static_cast<uint64>(0xffff) << 48u)) == ~imm) ||
         (((~imm) & (static_cast<uint64>(0xffff) << 32u)) == ~imm) ||
         (((~imm) & (static_cast<uint64>(0xffff) << 16u)) == ~imm) ||
         (((~imm) & (static_cast<uint64>(0xffff))) == ~imm);
}

// this function can only check for expr itself, not iteratively check for opnds
// if non-simple imm exist, return it, otherwise return 0
static int64 GetNonSimpleImm(MeExpr *expr) {
  if (expr->GetMeOp() == kMeOpConst && IsPrimitiveInteger(expr->GetPrimType())) {
    int64 imm = static_cast<MIRIntConst *>(static_cast<ConstMeExpr *>(expr)->GetConstVal())->GetValue();
    if (!IsSimpleImm(imm)) {
      return imm;
    }
  }
  return 0; // 0 is a simple imm
}

bool SimplifyCFG::IsProfitableForCond2Sel(MeExpr *condExpr, MeExpr *trueExpr, MeExpr *falseExpr) {
  if (trueExpr == falseExpr) {
    return true;
  }
  ASSERT(IsSafeForCond2Sel(trueExpr), "[FUNC: %s]Please check for safety first", funcName) ;
  ASSERT(IsSafeForCond2Sel(falseExpr), "[FUNC: %s]Please check for safety first", funcName) ;
  // try to simplify
  MeExpr *selExpr = irmap->CreateMeExprSelect(trueExpr->GetPrimType(), *condExpr, *trueExpr, *falseExpr);
  MeExpr *simplifiedSel = irmap->SimplifyMeExpr(selExpr);
  if (simplifiedSel != selExpr) {
    return true; // can be simplified
  }
  // big integer
  if (GetNonSimpleImm(trueExpr) != 0 || GetNonSimpleImm(falseExpr) != 0) {
    return false;
  }

  // We can check for every opnd of opndExpr, and calculate their cost according to cg's insn
  // but optimization in mplbe may change the insn and the result is not correct after that.
  // Therefore, to make this easier, only reg and const are allowed here
  MeExprOp trueOp = trueExpr->GetMeOp();
  MeExprOp falseOp = falseExpr->GetMeOp();
  if ((trueOp != kMeOpConst && trueOp != kMeOpReg) || (falseOp != kMeOpConst && falseOp != kMeOpReg)) {
    return false;
  }
  // some special case
  // lt (sign num, 0) ==> testbit sign bit
  // ge (sign num, 0) ==> testbit sign bit
  if (condExpr->GetOp() == OP_lt || condExpr->GetOp() == OP_ge) {
    if (IsSignedInteger(condExpr->GetOpnd(0)->GetPrimType()) && condExpr->GetOpnd(1)->IsZero()) {
      return false;
    }
  }
  return true;
}

// Check if ftBB has only one or zero regassign stmt, set ftStmt if a regassign stmt exist
// return IsPatternMatch (i.e. bb has only zero/one regassign)
// if regassign exists, set ass as it, otherwise set ass as nullptr
static bool GetIndividualRegassign(BB *bb, AssignMeStmt *&ass) {
  MeStmt *stmt = bb->GetFirstMe();
  // Skip comment stmt at the beginning of bb
  while (stmt != nullptr && stmt->GetOp() == OP_comment) {
    stmt = stmt->GetNextMeStmt();
  }
  if (stmt == nullptr) { // empty bb or has only comment stmt
    ass = nullptr;
    return true;
  }
  if (stmt->GetOp() == OP_regassign) {
    ass = static_cast<AssignMeStmt*>(stmt);
    // Skip comment stmt under this regassign
    stmt = stmt->GetNextMeStmt();
    while (stmt != nullptr && stmt->GetOp() == OP_comment) {
      stmt = stmt->GetNextMeStmt();
    }
    if (stmt == nullptr || stmt->GetOp() == OP_goto) {
      return true;
    }
  }
  return false;
}

void SimplifyCFG::CombineCond2SelPattern(BB *condBB, BB *ftBB, BB *gtBB, BB *jointBB) {
  DEBUG_LOG() << "Condition To Fallthru : BB" << LOG_BBID(condBB) << "(cond->fallthru)->[BB" << LOG_BBID(ftBB)
              << "(removed), BB" << LOG_BBID(gtBB) << "(removed)]->BB" << LOG_BBID(jointBB) << "(joint)\n";
  if (condBB->GetLastMe()->IsCondBr()) {
    condBB->RemoveLastMeStmt();
  }
  condBB->SetKind(kBBFallthru);
  jointBB->RemovePred(*gtBB, true);
  jointBB->ReplacePred(ftBB, condBB);
  condBB->RemoveSucc(*ftBB, false); // we will delete ftBB, so no need to updatephi
  condBB->RemoveSucc(*gtBB, false);
  if (ftBB->IsGoto()) {
    // move LastMeStmt of ftBB(i.e. a goto stmt) to condBB
    MeStmt *ftLast = ftBB->GetLastMe();
    ftBB->RemoveLastMeStmt();
    condBB->AddMeStmtLast(ftLast);
    condBB->SetKind(kBBGoto);
  }
  DeleteBB(ftBB);
  DeleteBB(gtBB);
  if (jointBB->GetPred().size() == 1) {
    MergeDistinctBBPair(condBB, jointBB);
    UpdateSSACandForBBPhiList(jointBB, condBB);
  }
}
/*
 *  condBB
 *  /   \
 * ftBB gtBB
 *  \   /
 *  jointBB
 */
bool SimplifyCFG::CondBranchToSelect() {
  CHECK_CURR_BB();
  BB *ftBB = currBB->GetSucc(0); // fallthruBB
  BB *gtBB = currBB->GetSucc(1); // gotoBB
  // check for pattern
  // ftBB and gtBB has only one pred(condBB)
  if (ftBB->GetPred().size() != 1 || gtBB->GetPred().size() != 1) {
    return false;
  }
  if ((ftBB->GetKind() != kBBFallthru && ftBB->GetKind() != kBBGoto) ||
      (gtBB->GetKind() != kBBFallthru && gtBB->GetKind() != kBBGoto)) {
    return false;
  }
  if (ftBB->GetSucc(0) != gtBB->GetSucc(0)) {
    return false;
  }
  BB *jointBB = ftBB->GetSucc(0); // common succ
  // jointBB has only two preds (ftBB and gtBB)
  if (jointBB->GetPred().size() != 2) {
    return false;
  }
  // Check if ftBB has only one or zero regassign stmt, set ftStmt if a regassign stmt exist
  AssignMeStmt *ftStmt = nullptr;
  if (!GetIndividualRegassign(ftBB, ftStmt)) {
    return false;
  }
  AssignMeStmt *gtStmt = nullptr;
  if (!GetIndividualRegassign(gtBB, gtStmt)) {
    return false;
  }
  // ftBB and gtBB is an empty BB, we can remove them
  if (ftStmt == nullptr && gtStmt == nullptr) {
    CombineCond2SelPattern(currBB, ftBB, gtBB, jointBB);
    return true;
  }

  // Here we found a pattern, collect select opnds and result reg
  RegMeExpr *ftReg = nullptr;
  MeExpr *ftRHS = nullptr;
  if (ftStmt != nullptr) {
    ftReg = static_cast<RegMeExpr*>(ftStmt->GetLHS());
    ftRHS = ftStmt->GetRHS();
  }
  RegMeExpr *gtReg = nullptr;
  MeExpr *gtRHS = nullptr;
  if (gtStmt != nullptr) {
    gtReg = static_cast<RegMeExpr*>(gtStmt->GetLHS());
    gtRHS = gtStmt->GetRHS();
  }
  // fix it if one of ftRHS/gtRHS is nullptr
  //    a <- mx1
  //      cond
  //     |    \
  //     |    a <- mx2
  //     |    /
  //     a <- phi(mx1, mx2)
  // we turn it to
  // a <- cond ? mx1 : mx2
  // so we should find oldVersion(mx1) from phi in jointBB
  if (ftRHS == nullptr) {
    int predIdx = jointBB->GetPredIndex(*ftBB);
    ASSERT(predIdx != -1, "[FUNC: %s]ftBB is not a pred of jointBB", funcName);
    ftRHS = jointBB->GetMePhiList()[gtReg->GetOstIdx()]->GetOpnd(predIdx);
    ftReg = gtReg;
  } else if (gtRHS == nullptr) {
    int predIdx = jointBB->GetPredIndex(*gtBB);
    ASSERT(predIdx != -1, "[FUNC: %s]gtBB is not a pred of jointBB", funcName);
    gtRHS = jointBB->GetMePhiList()[ftReg->GetOstIdx()]->GetOpnd(predIdx);
    gtReg = ftReg;
  }
  // pattern not found
  if (gtReg->GetRegIdx() != ftReg->GetRegIdx()) {
    return false;
  }
  if (ftRHS != gtRHS) { // if ftRHS is the same as gtRHS, they can be simplified, and no need to check safety
    // black list
    if (!IsSafeForCond2Sel(ftRHS) || !IsSafeForCond2Sel(gtRHS)) {
      return false;
    }
  }
  MeStmt *condStmt = currBB->GetLastMe();
  MeExpr *trueExpr = (condStmt->GetOp() == OP_brtrue) ? gtRHS : ftRHS;
  MeExpr *falseExpr = (trueExpr == gtRHS) ? ftRHS : gtRHS;
  // use phinode lhs as result
  ScalarMeExpr *resReg = jointBB->GetMePhiList()[ftReg->GetOstIdx()]->GetLHS();

  MeExpr *condExpr = condStmt->GetOpnd(0);
  if (!IsProfitableForCond2Sel(condExpr, trueExpr, falseExpr)) {
    return false;
  }
  DEBUG_LOG() << "Condition To Select : BB" << LOG_BBID(currBB) << "(cond)->[BB" << LOG_BBID(ftBB) << "(ft), BB"
              << LOG_BBID(gtBB) << "(gt)]->BB" << LOG_BBID(jointBB) << "(joint)\n";
  MeExpr *selExpr = irmap->CreateMeExprSelect(resReg->GetPrimType(), *condExpr, *trueExpr, *falseExpr);
  MeExpr *simplifiedSel = irmap->SimplifyMeExpr(selExpr);
  AssignMeStmt *regAss = irmap->CreateAssignMeStmt(*resReg, *simplifiedSel, *currBB);
  jointBB->GetMePhiList().erase(resReg->GetOstIdx());
  regAss->SetSrcPos(currBB->GetLastMe()->GetSrcPosition());
  // here we do not remove condStmt, because it will be delete in CombineCond2SelPattern
  currBB->InsertMeStmtBefore(condStmt, regAss);
  CombineCond2SelPattern(currBB, ftBB, gtBB, jointBB);
  return true;
}
// CurrBB is Condition BB, we will look upward its predBB(s) to see if we can simplify
// 1. currBB is X == constVal, and predBB has checked for the same expr, the result is known for currBB's condition,
//    so we can make currBB to be an uncondBB.
// 2. predBB is CondBB, one of predBB's succBB is currBB, and another is one of currBB's successors(commonBB)
//    we can merge currBB to predBB if currBB is simple enough(has only one stmt).
// 3. currBB has only one stmt(conditional branch stmt), and the condition's value is calculated by all its predBB
//    we can hoist currBB's stmt to predBBs if it is profitable
// 4. condition branch to select
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
  // 4. condition branch to select
  if (CondBranchToSelect()) {
    SetBBRunAgain();
    return true;
  }
  return false;
}

// after moving pred from curr to curr's successor (i.e. succ), update the phiList of curr and succ
// a phiOpnd will be removed from curr's philist, and another phiOpnd will be inserted to succ's philist
//
//    ...  pred           ...     pred
//      \  /  \            \       / \
//      curr  ...   ==>   curr    /  ...
//      /  \   ...         /  \  / ...
//     /    \  /          /    \/ /
//   ...    succ         ...  succ
//
// parameter predIdxForCurr is the index of pred in the predVector of curr
// note:
// 1.when replace pred's succ (i.e. curr) with succ, please DO NOT remove phiOpnd immediately,
// otherwise we cannot get phiOpnd in this step
// 2.predIdxForCurr should be get before disconnecting pred and curr
void SimplifyCFG::UpdatePhiForMovingPred(int predIdxForCurr, BB *pred, BB *curr, BB *succ) {
  auto &succPhiList = succ->GetMePhiList();
  auto &currPhilist = curr->GetMePhiList();
  if (succPhiList.empty()) {
    // succ has only one pred(i.e. curr) before
    // we copy curr's philist to succ, but not with all phiOpnd
    for (auto &phiNode : currPhilist) {
      auto *phiMeNode = irmap->NewInPool<MePhiNode>();
      phiMeNode->SetDefBB(succ);
      succPhiList.emplace(phiNode.first, phiMeNode);
      auto &phiOpnds = phiMeNode->GetOpnds();
      phiOpnds.resize(2); // succ has 2 pred now : curr and pred
      int currPredIdx = succ->GetPredIndex(*curr);
      int predPredIdx = succ->GetPredIndex(*pred);
      // curr is already pred of succ, so we add its phiLHS as succ's phiOpnd
      phiMeNode->SetOpnd(currPredIdx, phiNode.second->GetLHS());
      // pred is a new pred for succ, we copy its corresponding phiopnd in curr to succ
      phiMeNode->SetOpnd(predPredIdx, phiNode.second->GetOpnd(predIdxForCurr));
      OStIdx ostIdx = phiNode.first;
      // create a new version for new phi
      phiMeNode->SetLHS(irmap->CreateRegOrVarMeExprVersion(ostIdx));
    }
    UpdateSSACandForBBPhiList(succ); // new philist has been created in succ
  } else {
    // succ has other pred besides curr
    for (auto &phi : succPhiList) {
      OStIdx ostIdx = phi.first;
      auto it = currPhilist.find(ostIdx);
      int predPredIdx = succ->GetPredIndex(*pred);
      auto &phiOpnds = phi.second->GetOpnds();
      if (it != currPhilist.end()) {
        // curr has phiNode for this ost, we copy pred's corresponding phiOpnd in curr to succ
        phiOpnds.insert(phiOpnds.begin() + predPredIdx, it->second->GetOpnd(predIdxForCurr));
      } else {
        // curr has no phiNode for this ost, pred's phiOpnd in succ will be the same as curr's phiOpnd in succ
        int index = succ->GetPredIndex(*curr);
        ASSERT(index != -1, "[FUNC: %s]succ is not newTarget's pred", f.GetName().c_str());
        // pred's phi opnd is the same as curr.
        phiOpnds.insert(phiOpnds.begin() + predPredIdx, phi.second->GetOpnd(index));
      }
    }
    // search philist in curr for phinode that is not in succ yet
    for (auto &phi : currPhilist) {
      OStIdx ostIdx = phi.first;
      auto resPair = succPhiList.emplace(ostIdx, nullptr);
      if (!resPair.second) {
        // phinode is in succ, last step has updated it
        continue;
      } else {
        auto *phiMeNode = irmap->NewInPool<MePhiNode>();
        phiMeNode->SetDefBB(succ);
        resPair.first->second = phiMeNode; // replace nullptr inserted before
        auto &phiOpnds = phiMeNode->GetOpnds();
        // insert opnd into New phiNode : all phiOpnds (except for pred) are phiNode lhs in curr
        phiOpnds.insert(phiOpnds.end(), succ->GetPred().size() - 1, phi.second->GetLHS());
        // pred is new pred for succ, we copy its corresponding phiopnd in curr to succ
        int predPredIdx = succ->GetPredIndex(*pred);
        phiOpnds.insert(phiOpnds.begin() + predPredIdx, phi.second->GetOpnd(predIdxForCurr));
        // create a new version for new phinode
        phiMeNode->SetLHS(irmap->CreateRegOrVarMeExprVersion(ostIdx));
        UpdateSSACandForOst(ostIdx, succ);
      }
    }
  }
  // remove pred's corresponding phiOpnd from curr's philist
  curr->RemovePhiOpnd(predIdxForCurr);
}

// succ must have only one goto statement
// pred must be three of below:
// 1. fallthrough BB
// 2. have a goto stmt
// 3. conditional branch, and succ must be pred's goto target in this case
bool SimplifyCFG::MergeGotoBBToPred(BB *succ, BB *pred) {
  if (pred == nullptr || succ == nullptr) {
    return false;
  }
  // If we merge succ to pred, a new BB will be create to split the same critical edge
  if (MeSplitCEdge::IsCriticalEdgeBB(*pred)) {
    return false;
  }
  if (succ == pred) {
    return false;
  }
  if (succ->GetAttributes(kBBAttrIsEntry) || succ->IsMeStmtEmpty()) {
    return false;
  }
  // BB has only one stmt(i.e. unconditional goto stmt)
  if (!HasOnlyGotoStmt(*succ)) {
    return false;
  }
  BB *newTarget = succ->GetSucc(0); // succ must have only one succ, because it is uncondBB
  if (newTarget == succ) {
    // succ goto itself, no need to update goto target to newTarget
    return false;
  }
  // newTarget has only one pred, skip, because MergeDistinctBBPair will deal with this case
  if (newTarget->GetPred().size() == 1) {
    return false;
  }
  // pred is moved to newTarget
  if (pred->GetKind() == kBBFallthru) {
    GotoMeStmt *gotoMeStmt = irmap->CreateGotoMeStmt(newTarget->GetBBLabel(), pred,
                                                     &succ->GetLastMe()->GetSrcPosition());
    pred->AddMeStmtLast(gotoMeStmt);
    pred->SetKind(kBBGoto);
    DEBUG_LOG() << "Insert Uncond stmt to fallthru BB" << LOG_BBID(currBB) << ", and goto BB" << LOG_BBID(newTarget)
                << "\n";
  }
  int predIdx = succ->GetPredIndex(*pred);
  bool needUpdatePhi = false;
  if (pred->IsPredBB(*newTarget)) {
    pred->RemoveSucc(*succ, true); // one of pred's succ has been newTarget, avoid duplicate succ here
  } else {
    pred->ReplaceSucc(succ, newTarget); // phi opnd is not removed from currBB's philist, we will remove it later
    needUpdatePhi = true;
  }

  DEBUG_LOG() << "Merge Uncond BB" << LOG_BBID(succ) << " to its pred BB" << LOG_BBID(pred)
              << ": BB" << LOG_BBID(pred) << "->BB" << LOG_BBID(succ) << "(merged)->BB" << LOG_BBID(newTarget) << "\n";
  cfg->UpdateBranchTarget(*pred, *succ, *newTarget, f);
  if (needUpdatePhi) {
    // remove phiOpnd in succ, and add phiOpnd to newTarget
    UpdatePhiForMovingPred(predIdx, pred, succ, newTarget);
  }
  // remove succ if succ has no pred
  if (succ->GetPred().empty()) {
    newTarget->RemovePred(*succ, true);
    DEBUG_LOG() << "Delete Uncond BB" << LOG_BBID(succ) << " after merged to all its preds\n";
    UpdateSSACandForBBPhiList(succ, newTarget);
    DeleteBB(succ);
  }
  // if all branch destination of pred (condBB or switchBB) are the same, we should turn pred into uncond
  (void) SimplifyBranchBBToUncondBB(*pred);
  return true;
}

// if unconditional BB has only one uncondBr stmt, we try to merge it to its pred
// this is different from MergeDistinctBBPair, it is not required that current uncondBB has only one pred
//   pred1  pred2  pred3
//        \   |   /
//         \  |  /
//           succ
//            |     other pred
//            |    /
//         newTarget
bool SimplifyCFG::SimplifyUncondBB() {
  CHECK_CURR_BB();
  if (currBB->GetAttributes(kBBAttrIsEntry) || currBB->IsMeStmtEmpty()) {
    return false;
  }
  // BB has only one stmt(i.e. unconditional goto stmt)
  if (!HasOnlyGotoStmt(*currBB)) {
    return false;
  }
  // jump to itself
  if (currBB->GetSucc(0) == currBB) {
    return false;
  }
  // wont exit BB and has an edge to commonExit, if we merge it to pred and delete it, the egde will be cut off
  if (currBB->GetSucc().size() == 2) { // 2 succ : first is gotoTarget, second is edge to commonExit
    ASSERT(currBB->GetAttributes(kBBAttrWontExit), "[FUNC: %s]GotoBB%d is not wontexitBB, but has two succ", funcName,
           LOG_BBID(currBB));
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
    if (pred->IsGoto() || pred->GetKind() == kBBSwitch ||
        (pred->GetLastMe()->IsCondBr() && currBB == pred->GetSucc(1))) {
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
  if (MeSplitCEdge::IsCriticalEdgeBB(*currBB)) {
    return false;
  }
  BB *succ = currBB->GetSucc(0);
  if (succ == nullptr || succ == currBB) {
    return false;
  }
  if (succ->GetAttributes(kBBAttrIsEntry) || succ->IsMeStmtEmpty()) {
    return false;
  }
  // BB has only one stmt(i.e. unconditional goto stmt)
  if (!HasOnlyGotoStmt(*succ)) {
    return false;
  }
  // BB has only one pred/succ, skip, because MergeDistinctBBPair will deal with this case
  // note: gotoBB may have two succ, the second succ is inserted by cfg wont exit analysis
  if (succ->GetPred().size() == 1 || succ->GetSucc().size() == 1) {
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
  auto *swStmt = static_cast<SwitchMeStmt*>(currBB->GetLastMe());
  if (swStmt->GetOpnd()->GetMeOp() != kMeOpConst) {
    return false;
  }
  auto *swConstExpr = static_cast<ConstMeExpr *>(swStmt->GetOpnd());
  MIRConst *swConstVal = swConstExpr->GetConstVal();
  ASSERT(swConstVal->GetKind() == kConstInt, "[FUNC: %s]switch is only legal for integer val", funcName);
  int64 val = static_cast<MIRIntConst*>(swConstVal)->GetValue();
  BB *survivor = currBB->GetSucc(0); // init as default BB
  for (size_t i = 0; i < swStmt->GetSwitchTable().size(); ++i) {
    int64 caseVal = swStmt->GetSwitchTable().at(i).first;
    if (caseVal == val) {
      LabelIdx label = swStmt->GetSwitchTable().at(i).second;
      survivor = cfg->GetLabelBBAt(label);
    }
  }
  // remove all succ expect for survivor
  for (size_t i = 0; i < currBB->GetSucc().size();) {
    BB *succ = currBB->GetSucc(i);
    if (succ != survivor) {
      succ->RemovePred(*currBB, true);
      continue;
    }
    ++i;
  }
  DEBUG_LOG() << "Constant switchBB to Uncond BB : BB" << LOG_BBID(currBB) << "->BB" << LOG_BBID(survivor) << "\n";
  // replace switch stmt with a goto stmt
  LabelIdx label = f.GetOrCreateBBLabel(*survivor);
  auto *gotoStmt = irmap->New<GotoMeStmt>(label);
  gotoStmt->SetSrcPos(swStmt->GetSrcPosition());
  gotoStmt->SetBB(currBB);
  currBB->RemoveLastMeStmt();
  currBB->AddMeStmtLast(gotoStmt);
  currBB->SetKind(kBBGoto);
  SetBBRunAgain();
  return true;
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
  aDep.PreservedAllExcept<MELoopAnalysis>();
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
    // split critical edges
    MeSplitCEdge(debug).SplitCriticalEdgeForMeFunc(f);
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

