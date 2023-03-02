
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
#include "optimizeCFG.h"

#include "bb.h"
#include "factory.h"
#include "global_tables.h"
#include "me_phase_manager.h"
#include "mir_symbol.h"
#include "mpl_logging.h"
#include "orig_symbol.h"
#include "types_def.h"

namespace maple {
namespace {
bool debug = false;
std::string funcName;
std::string phaseName;

#define LOG_BBID(BB) ((BB)->GetBBId().GetIdx())
#define DEBUG_LOG() \
  if (debug) LogInfo::MapleLogger() << "[" << phaseName << "] "

// return {trueBB, falseBB}
std::pair<BB*, BB*> GetTrueFalseBrPair(BB *bb) {
  if (bb == nullptr) {
    return {nullptr, nullptr};
  }
  if (bb->GetKind() == kBBCondGoto) {
    auto *condBr = static_cast<CondGotoMeStmt*>(bb->GetLastMe());
    CHECK_FATAL(condBr, "condBr is nullptr!");
    if (condBr->GetOp() == OP_brtrue) {
      return {bb->GetSucc(1), bb->GetSucc(0)};
    } else {
      ASSERT(condBr->GetOp() == OP_brfalse, "only support brtrue/brfalse");
      return {bb->GetSucc(0), bb->GetSucc(1)};
    }
  } else if (bb->GetKind() == kBBGoto) {
    return {bb->GetSucc(0), nullptr};
  }
  return {nullptr, nullptr};
}

// Only one fallthru is allowed for a bb at most , but a bb may have be more than one fallthru pred
// temporarily during optimizing process.
size_t GetFallthruPredNum(const BB &bb) {
  size_t num = 0;
  for (BB *pred : bb.GetPred()) {
    if (pred->GetKind() == kBBFallthru) {
      ++num;
    } else if (pred->GetKind() == kBBCondGoto && pred->GetSucc(0) == &bb) {
      ++num;
    }
  }
  return num;
}

// This interface is use to check for every bits of two floating point num, not just their value.
// example:
// The result of IsFloatingPointNumBitsSame<double >(16.1 * 100 + 0.9 * 100, 17.0 * 100) is false,
// althought their value is the same.
// A = 16.1 * 100 + 0.9 * 100 => bits : 0x409A 9000 0000 0001
// and
// B = 17.0 * 100             => bits : 0x409A 9000 0000 0000
template <class T, class = typename std::enable_if<std::is_floating_point<T>::value>::type>
bool IsFloatingPointNumBitsSame(T val1, T val2) {
  if (std::is_same<T, float>::value) {
    return *reinterpret_cast<uint32*>(&val1) == *reinterpret_cast<uint32*>(&val2);
  } else if (std::is_same<T, double>::value) {
    return *reinterpret_cast<uint64*>(&val1) == *reinterpret_cast<uint64*>(&val2);
  }
  return false;
}

// Collect expressions that may be unsafe from expr to exprSet
void CollectUnsafeExpr(MeExpr *expr, std::set<MeExpr*> &exprSet) {
  if (expr->GetMeOp() == kMeOpConst || expr->GetMeOp() == kMeOpVar) {
    return;
  }
  if (expr->GetMeOp() == kMeOpIvar) {  // do not use HasIvar here for efficiency reasons
    exprSet.insert(expr);
  }
  if (expr->GetOp() == OP_div || expr->GetOp() == OP_rem) {
    MeExpr *opnd1 = expr->GetOpnd(1);
    if (opnd1->GetMeOp() == kMeOpConst && !opnd1->IsZero()) {
      CollectUnsafeExpr(expr->GetOpnd(0), exprSet);
    }
    // we are not sure whether opnd1 may be zero
    exprSet.insert(expr);
  }
  for (size_t i = 0; i < expr->GetNumOpnds(); ++i) {
    CollectUnsafeExpr(expr->GetOpnd(i), exprSet);
  }
}

// expr not throw exception
bool IsSafeExpr(const MeExpr *expr) {
  if (expr->GetMeOp() == kMeOpIvar) {  // do not use HasIvar here for efficiency reasons
    return false;
  }
  if (expr->GetOp() == OP_div || expr->GetOp() == OP_rem) {
    MeExpr *opnd1 = expr->GetOpnd(1);
    if (opnd1->GetMeOp() == kMeOpConst && !opnd1->IsZero()) {
      return IsSafeExpr(expr->GetOpnd(0));
    }
    // we are not sure whether opnd1 may be zero
    return false;
  }
  for (size_t i = 0; i < expr->GetNumOpnds(); ++i) {
    if (!IsSafeExpr(expr->GetOpnd(i))) {
      return false;
    }
  }
  return true;
}

// simple imm : can be assign to an reg by only one insn
bool IsSimpleImm(uint64 imm) {
  return ((imm & (static_cast<uint64>(0xffff) << 48u)) == imm) ||
         ((imm & (static_cast<uint64>(0xffff) << 32u)) == imm) ||
         ((imm & (static_cast<uint64>(0xffff) << 16u)) == imm) || ((imm & (static_cast<uint64>(0xffff))) == imm) ||
         (((~imm) & (static_cast<uint64>(0xffff) << 48u)) == ~imm) ||
         (((~imm) & (static_cast<uint64>(0xffff) << 32u)) == ~imm) ||
         (((~imm) & (static_cast<uint64>(0xffff) << 16u)) == ~imm) ||
         (((~imm) & (static_cast<uint64>(0xffff))) == ~imm);
}

// this function can only check for expr itself, not iteratively check for opnds
// if non-simple imm exist, return it, otherwise return 0
int64 GetNonSimpleImm(MeExpr *expr) {
  if (expr->GetMeOp() == kMeOpConst && IsPrimitiveInteger(expr->GetPrimType())) {
    int64 imm = static_cast<MIRIntConst*>(static_cast<ConstMeExpr*>(expr)->GetConstVal())->GetExtValue();
    if (!IsSimpleImm(static_cast<uint64>(imm))) {
      return imm;
    }
  }
  return 0;  // 0 is a simple imm
}

// Check if ftBB has only one regassign stmt, if regassign exists, return it, otherwise return nullptr
AssignMeStmt *GetSingleAssign(BB *bb) {
  MeStmt *stmt = bb->GetFirstMe();
  // Skip comment stmt at the beginning of bb
  while (stmt != nullptr && stmt->GetOp() == OP_comment) {
    stmt = stmt->GetNextMeStmt();
  }
  if (stmt == nullptr) {  // empty bb or has only comment stmt
    return nullptr;
  }
  if (stmt->GetOp() == OP_regassign || stmt->GetOp() == OP_dassign) {
    auto *ass = static_cast<AssignMeStmt*>(stmt);
    // Skip comment stmt under this regassign
    stmt = stmt->GetNextMeStmt();
    while (stmt != nullptr && stmt->GetOp() == OP_comment) {
      stmt = stmt->GetNextMeStmt();
    }
    if (stmt == nullptr || stmt->GetOp() == OP_goto) {
      return ass;
    }
  }
  return nullptr;
}

bool IsAllOpndsNotDefByCurrBB(const MeExpr &expr, const BB &currBB, std::set<const ScalarMeExpr*> &infLoopCheck) {
  switch (expr.GetMeOp()) {
    case kMeOpConst:
    case kMeOpConststr:
    case kMeOpConststr16:
    case kMeOpAddrof:
    case kMeOpAddroflabel:
    case kMeOpAddroffunc:
    case kMeOpSizeoftype:
      return true;
    case kMeOpVar:
    case kMeOpReg: {
      auto &scalarExpr = static_cast<const ScalarMeExpr&>(expr);
      MeStmt *stmt = scalarExpr.GetDefByMeStmt();
      if (stmt == nullptr) {
        // not def by a stmt, may be def by a phinode.
        if (scalarExpr.IsDefByPhi()) {
          const MePhiNode &phiNode = scalarExpr.GetDefPhi();
          const BB *bb = phiNode.GetDefBB();
          if (bb == &currBB) {
            return true;
          } else {
            // If phinode is in a bb that has been deleted from cfg, and has only one opnd,
            // we should find its real def thru use-def chain
            // If use-def chain in a loop of phinodes, we can be sure that its real def is in its ancestors.
            auto result = infLoopCheck.emplace(&scalarExpr);
            if (!result.second) {  // element is in infLoopCheck, all the def are phinode
              return true;
            }
            if (bb->GetPred().empty() && bb->GetSucc().empty() && phiNode.GetOpnds().size() == 1) {
              ScalarMeExpr *phiOpnd = phiNode.GetOpnd(0);
              return IsAllOpndsNotDefByCurrBB(*phiOpnd, currBB, infLoopCheck);
            }
          }
        }
        return true;
      }
      return stmt->GetBB() != &currBB;
    }
    case kMeOpIvar: {
      auto &ivar = static_cast<const IvarMeExpr&>(expr);
      if (!IsAllOpndsNotDefByCurrBB(*ivar.GetBase(), currBB, infLoopCheck)) {
        return false;
      }
      for (auto *mu : ivar.GetMuList()) {
        if (!IsAllOpndsNotDefByCurrBB(*mu, currBB, infLoopCheck)) {
          return false;
        }
      }
      return true;
    }
    case kMeOpNary:
    case kMeOpOp: {
      for (size_t i = 0; i < expr.GetNumOpnds(); ++i) {
        if (!IsAllOpndsNotDefByCurrBB(*expr.GetOpnd(i), currBB, infLoopCheck)) {
          return false;
        }
      }
      return true;
    }
    default:
      return false;
  }
  // never reach here
  CHECK_FATAL(false, "[FUNC: %s] Should never reach here!", funcName.c_str());
  return false;
}

// opnds is defined by stmt not in currBB or defined by phiNode(no matter whether in currBB)
bool IsAllOpndsNotDefByCurrBB(const MeStmt &stmt) {
  const BB *currBB = stmt.GetBB();
  for (size_t i = 0; i < stmt.NumMeStmtOpnds(); ++i) {
    std::set<const ScalarMeExpr*> infLoopCheck;
    if (!IsAllOpndsNotDefByCurrBB(*stmt.GetOpnd(i), *currBB, infLoopCheck)) {
      return false;
    }
  }
  return true;
}

MeExpr *GetInvertCond(MeIRMap *irmap, MeExpr *cond) {
  if (IsCompareHasReverseOp(cond->GetOp())) {
    return irmap->CreateMeExprBinary(GetReverseCmpOp(cond->GetOp()), cond->GetPrimType(), *cond->GetOpnd(0),
                                     *cond->GetOpnd(1));
  }
  return irmap->CreateMeExprUnary(OP_lnot, cond->GetPrimType(), *cond);
}

// Is subSet a subset of superSet?
template <class T>
inline bool IsSubset(const std::set<T> &subSet, const std::set<T> &superSet) {
  return std::includes(superSet.begin(), superSet.end(), subSet.begin(), subSet.end());
}

// before : predCond ---> succCond
// Is it safe when we use logical operation to merge predCond and succCond together?
// If unsafe expr in succCond is included in preCond, return true; otherwise return false;
bool IsSafeToMergeCond(MeExpr *predCond, MeExpr *succCond) {
  // Make sure succCond is not dependent on predCond
  // e.g. if (ptr != nullptr) if (*ptr), the second condition depends on the first one
  if (predCond == succCond) {
    return true;  // same expr
  }
  if (!IsSafeExpr(succCond)) {
    std::set<MeExpr*> predSet;
    CollectUnsafeExpr(predCond, predSet);
    std::set<MeExpr*> succSet;
    CollectUnsafeExpr(succCond, succSet);
    if (!IsSubset(succSet, predSet)) {
      return false;
    }
  }
  return true;
}

bool IsProfitableToMergeCond(MeExpr *predCond, MeExpr *succCond) {
  if (predCond == succCond) {
    return true;
  }
  ASSERT(IsSafeToMergeCond(predCond, succCond), "please check for safety first");
  // no constraint for predCond
  // Only "cmpop (var/reg/const, var/reg/const)" are allowed for subCond
  if (IsCompareHasReverseOp(succCond->GetOp())) {
    MeExprOp opnd0Op = succCond->GetOpnd(0)->GetMeOp();
    MeExprOp opnd1Op = succCond->GetOpnd(1)->GetMeOp();
    if ((opnd0Op == kMeOpVar || opnd0Op == kMeOpReg || opnd0Op == kMeOpConst) &&
        (opnd1Op == kMeOpVar || opnd1Op == kMeOpReg || opnd1Op == kMeOpConst)) {
      return true;
    }
  }
  return false;
}

bool GetBoundOfOpnd(const MeExpr &expr, Bound &bound) {
  Opcode op = expr.GetOp();
  if (!IsCompareHasReverseOp(op)) {
    return false;
  }
  // only deal with "scalarExpr cmp constExpr"
  MeExpr *opnd0 = expr.GetOpnd(0);
  if (!opnd0->IsScalar()) {
    return false;
  }
  PrimType opndType = static_cast<const OpMeExpr&>(expr).GetOpndType();
  if (!IsPrimitiveInteger(opndType)) {
    return false;
  }
  bound.SetPrimType(opndType); // bound of expr's opnd1
  if (expr.GetOpnd(1)->GetMeOp() == kMeOpConst) {
    MIRConst *constVal = static_cast<ConstMeExpr*>(expr.GetOpnd(1))->GetConstVal();
    if (constVal->GetKind() != kConstInt) {
      return false;
    }
    int64 val = static_cast<MIRIntConst*>(constVal)->GetExtValue();
    bound.SetConstant(val);
  } else if (expr.GetOpnd(1)->IsScalar()) {
    bound.SetVar(expr.GetOpnd(1));
  } else {
    return false;
  }
  return true;
}

// simple cmp expr is "scalarExpr cmp constExpr/scalarExpr"
std::unique_ptr<ValueRange> GetVRForSimpleCmpExpr(const MeExpr &expr) {
  Opcode op = expr.GetOp();
  PrimType opndType = static_cast<const OpMeExpr&>(expr).GetOpndType();
  Bound bound;
  if (!GetBoundOfOpnd(expr, bound)) {
    return nullptr;
  }
  Bound maxBound = Bound::MaxBound(opndType);
  Bound minBound = Bound::MinBound(opndType);

  switch (op) {
    case OP_gt: {
      if (bound == Bound::MaxBound(bound.GetPrimType())) {
        return nullptr;
      }
      if (bound.IsConstantBound()) {
        ++bound;
      } else {
        bound.SetClosedInterval(false);
      }
      return std::make_unique<ValueRange>(bound, maxBound, kLowerAndUpper);
    }
    case OP_ge: {
      return std::make_unique<ValueRange>(bound, maxBound, kLowerAndUpper);
    }
    case OP_lt: {
      if (bound == Bound::MinBound(bound.GetPrimType())) {
        return nullptr;
      }
      if (bound.IsConstantBound()) {
        --bound;
      } else {
        bound.SetClosedInterval(false);
      }
      return std::make_unique<ValueRange>(minBound, bound, kLowerAndUpper);
    }
    case OP_le: {
      return std::make_unique<ValueRange>(minBound, bound, kLowerAndUpper);
    }
    case OP_eq: {
      return std::make_unique<ValueRange>(bound, kEqual);
    }
    case OP_ne: {
      return std::make_unique<ValueRange>(bound, kNotEqual);
    }
    default: {
      return nullptr;
    }
  }
}

// Check for pattern as below, return commonBB if exit, return nullptr otherwise.
//        predBB
//        /  \
//       /  succBB
//      /   /   \
//     commonBB  exitBB
// note: there may be some empty BB between predBB->commonBB, and succBB->commonBB, we should skip them
BB *GetCommonDest(BB *predBB, BB *succBB) {
  if (predBB == nullptr || succBB == nullptr || predBB == succBB) {
    return nullptr;
  }
  if (predBB->GetKind() != kBBCondGoto || succBB->GetKind() != kBBCondGoto) {
    return nullptr;
  }
  BB *psucc0 = FindFirstRealSucc(predBB->GetSucc(0));
  BB *psucc1 = FindFirstRealSucc(predBB->GetSucc(1));
  BB *ssucc0 = FindFirstRealSucc(succBB->GetSucc(0));
  BB *ssucc1 = FindFirstRealSucc(succBB->GetSucc(1));
  if (psucc0 == nullptr || psucc1 == nullptr || ssucc0 == nullptr || ssucc1 == nullptr) {
    return nullptr;
  }

  if (psucc0 != succBB && psucc1 != succBB) {
    return nullptr;  // predBB has no branch to succBB
  }
  BB *commonBB = (psucc0 == succBB) ? psucc1 : psucc0;
  if (commonBB == nullptr) {
    return nullptr;
  }
  if (ssucc0 == commonBB || ssucc1 == commonBB) {
    return commonBB;
  }
  return nullptr;
};

// Collect all var expr to varSet iteratively
bool DoesExprContainSubExpr(const MeExpr *expr, MeExpr *subExpr) {
  if (expr == subExpr) {
    return true;
  }
  for (size_t i = 0; i < expr->GetNumOpnds(); ++i) {
    if (DoesExprContainSubExpr(expr->GetOpnd(i), subExpr)) {
      return true;
    }
  }
  return false;
}

// a <- cond ? ftRHS : gtRHS
// We have tried to get ftRHS/gtRHS from stmt before, if it still nullptr, try to find it from phiNode
//    a <- mx1
//      cond
//     |    \
//     |    a <- mx2
//     |    /
//     a <- phi(mx1, mx2)
// we turn it to
// a <- cond ? mx1 : mx2
// so we should find oldVersion(mx1) from phi in jointBB
MeExpr *FindCond2SelRHSFromPhiNode(BB *condBB, const BB *ftOrGtBB, BB *jointBB, const OStIdx &ostIdx) {
  if (ftOrGtBB != jointBB) {
    return nullptr;
  }
  int predIdx = GetRealPredIdx(*jointBB, *condBB);
  ASSERT(predIdx != -1, "[FUNC: %s]ftBB is not a pred of jointBB", funcName.c_str());
  auto &phiList = jointBB->GetMePhiList();
  auto it = phiList.find(ostIdx);
  if (it == phiList.end()) {
    return nullptr;
  }
  MePhiNode *phi = it->second;
  ScalarMeExpr *ftOrGtRHS = phi->GetOpnd(static_cast<size_t>(predIdx));
  while (ftOrGtRHS->IsDefByPhi()) {
    MePhiNode &phiNode = ftOrGtRHS->GetDefPhi();
    BB *bb = phiNode.GetDefBB();
    // bb is a succ of condBB, find the real def thru use-def chain
    // when we find a version defined by condBB, or condBB's ancestor, we can stop.
    if (bb->GetBBId() == condBB->GetBBId() || phiNode.GetOpnds().size() != 1 || GetRealSuccIdx(*condBB, *bb) == -1) {
      break;
    }
    ftOrGtRHS = phiNode.GetOpnd(0);
  }
  return ftOrGtRHS;
}

// expr has deref nullptr or div/rem zero, return expr;
// if it is not sure whether the expr will throw exception, return nullptr
void MustThrowExceptionExpr(MeExpr *expr, std::set<MeExpr*> &exceptionExpr, bool &isDivOrRemException) {
  if (isDivOrRemException) {
    return;
  }
  if (expr->GetMeOp() == kMeOpIvar) {
    // deref nullptr
    if (static_cast<IvarMeExpr*>(expr)->GetBase()->IsZero()) {
      exceptionExpr.emplace(static_cast<IvarMeExpr*>(expr));
      return;
    }
  } else if ((expr->GetOp() == OP_div || expr->GetOp() == OP_rem) && expr->GetOpnd(1)->IsIntZero()) {
    // for float or double zero, this is legal.
    exceptionExpr.emplace(expr);
    isDivOrRemException = true;
    return;
  } else if (expr->GetOp() == OP_select) {
    MustThrowExceptionExpr(expr->GetOpnd(0), exceptionExpr, isDivOrRemException);
    // for select, if only one result will cause error, we are not sure whether
    // the actual result of this select expr will cause error
    std::set<MeExpr*> trueExpr;
    MustThrowExceptionExpr(expr->GetOpnd(1), trueExpr, isDivOrRemException);
    if (trueExpr.empty()) {
      return;
    }
    std::set<MeExpr*> falseExpr;
    MustThrowExceptionExpr(expr->GetOpnd(2), falseExpr, isDivOrRemException);
    if (falseExpr.empty()) {
      return;
    }
    exceptionExpr.emplace(expr);
    return;
  }
  for (size_t i = 0; i < expr->GetNumOpnds(); ++i) {
    MustThrowExceptionExpr(expr->GetOpnd(i), exceptionExpr, isDivOrRemException);
  }
}

// No return stmt
//  1. call no-return func
//  2. stmt with expr that must throw exception
MeStmt *GetNoReturnStmt(BB *bb) {
  // iterate all stmt
  for (auto *stmt = bb->GetFirstMe(); stmt != nullptr; stmt = stmt->GetNextMeStmt()) {
    std::set<MeExpr*> exceptionExpr;
    for (size_t i = 0; i < stmt->NumMeStmtOpnds(); ++i) {
      bool isDivOrRemException = false;
      MustThrowExceptionExpr(stmt->GetOpnd(i), exceptionExpr, isDivOrRemException);
      if (!exceptionExpr.empty()) {
        return stmt;
      }
    }
    if (stmt->GetOp() == OP_call) {
      PUIdx puIdx = static_cast<CallMeStmt*>(stmt)->GetPUIdx();
      MIRFunction *callee = GlobalTables::GetFunctionTable().GetFunctionFromPuidx(puIdx);
      if (callee->GetAttr(FUNCATTR_noreturn)) {
        return stmt;
      }
    }
  }
  return nullptr;
}

bool SkipOptimizeCFG(const maple::MeFunction &f) {
  for (auto bbIt = f.GetCfg()->valid_begin(); bbIt != f.GetCfg()->valid_end(); ++bbIt) {
    if ((*bbIt)->GetKind() == kBBIgoto || (*bbIt)->GetAttributes(kBBAttrIsTry)) {
      return true;
    }
  }
  return false;
}

bool UnreachBBAnalysis(const maple::MeFunction &f) {
  if (f.GetCfg()->UnreachCodeAnalysis(true)) {
    DEBUG_LOG() << "Remove unreachable BB\n";
    return true;
  }
  return false;
}
}  // anonymous namespace

// contains only one valid goto stmt
bool HasOnlyMeGotoStmt(BB &bb) {
  if (IsMeEmptyBB(bb) || !bb.IsGoto()) {
    return false;
  }
  MeStmt *stmt = bb.GetFirstMe();
  // Skip comment stmt
  while (stmt != nullptr && stmt->GetOp() == OP_comment) {
    stmt = stmt->GetNextMeStmt();
  }
  return (stmt->GetOp() == OP_goto);
}

// contains only one valid goto mpl stmt
bool HasOnlyMplGotoStmt(BB &bb) {
  if (IsMplEmptyBB(bb) || !bb.IsGoto()) {
    return false;
  }
  StmtNode *stmt = &bb.GetFirst();
  // Skip comment stmt
  if (stmt != nullptr && stmt->GetOpCode() == OP_comment) {
    stmt = stmt->GetRealNext();
  }
  return (stmt->GetOpCode() == OP_goto);
}

// contains only one valid condgoto stmt
bool HasOnlyMeCondGotoStmt(BB &bb) {
  if (IsMeEmptyBB(bb) || bb.GetKind() != kBBCondGoto) {
    return false;
  }
  MeStmt *stmt = bb.GetFirstMe();
  // Skip comment stmt
  while (stmt != nullptr && stmt->GetOp() == OP_comment) {
    stmt = stmt->GetNextMeStmt();
  }
  return (stmt != nullptr && kOpcodeInfo.IsCondBr(stmt->GetOp()));
}

// contains only one valid condgoto mpl stmt
bool HasOnlyMplCondGotoStmt(BB &bb) {
  if (IsMplEmptyBB(bb) || bb.GetKind() != kBBCondGoto) {
    return false;
  }
  StmtNode *stmt = &bb.GetFirst();
  // Skip comment stmt
  if (stmt != nullptr && stmt->GetOpCode() == OP_comment) {
    stmt = stmt->GetRealNext();
  }
  return (stmt != nullptr && kOpcodeInfo.IsCondBr(stmt->GetOpCode()));
}

bool IsMeEmptyBB(BB &bb) {
  if (bb.IsMeStmtEmpty()) {
    return true;
  }
  MeStmt *stmt = bb.GetFirstMe();
  // Skip comment stmt
  while (stmt != nullptr && stmt->GetOp() == OP_comment) {
    stmt = stmt->GetNextMeStmt();
  }
  return stmt == nullptr;
}

// contains no valid mpl stmt
bool IsMplEmptyBB(BB &bb) {
  if (bb.IsEmpty()) {
    return true;
  }
  StmtNode *stmt = &bb.GetFirst();
  // Skip comment stmt
  if (stmt != nullptr && stmt->GetOpCode() == OP_comment) {
    stmt = stmt->GetRealNext();
  }
  return stmt == nullptr;
}

// pred-connecting-succ
// connectingBB has only one pred and succ, and has no stmt (except a single gotoStmt) in it
bool IsConnectingBB(BB &bb) {
  return (bb.GetPred().size() == 1 && bb.GetSucc().size() == 1) &&
         (IsMeEmptyBB(bb) || HasOnlyMeGotoStmt(bb)) &&  // for meir, if no meir exist, it is empty
         (IsMplEmptyBB(bb) || HasOnlyMplGotoStmt(bb));  // for mplir, if no mplir exist, it is empty
}

// RealSucc is a non-connecting BB which is not empty (or has just a single gotoStmt).
// If we want to find the non-empty succ of currBB, we start from the succ (i.e. the argument)
// skip those connecting bb used to connect its pred and succ, like: pred -- connecting -- succ
// func will stop at first non-connecting BB or stopBB
BB *FindFirstRealSucc(BB *succ, const BB *stopBB) {
  while (succ != stopBB && IsConnectingBB(*succ)) {
    succ = succ->GetSucc(0);
  }
  return succ;
}

// RealPred is a non-connecting BB which is not empty (or has just a single gotoStmt).
// If we want to find the non-empty pred of currBB, we start from the pred (i.e. the argument)
// skip those connecting bb used to connect its pred and succ, like: pred -- connecting -- succ
// func will stop at first non-connecting BB or stopBB
BB *FindFirstRealPred(BB *pred, const BB *stopBB) {
  while (pred != stopBB && IsConnectingBB(*pred)) {
    pred = pred->GetPred(0);
  }
  return pred;
}

int GetRealPredIdx(BB &succ, const BB &realPred) {
  size_t i = 0;
  size_t predSize = succ.GetPred().size();
  while (i < predSize) {
    if (FindFirstRealPred(succ.GetPred(i), &realPred) == &realPred) {
      return static_cast<int>(i);
    }
    ++i;
  }
  // bb not in the vector
  return -1;
}

int GetRealSuccIdx(BB &pred, const BB &realSucc) {
  size_t i = 0;
  size_t succSize = pred.GetSucc().size();
  while (i < succSize) {
    if (FindFirstRealSucc(pred.GetSucc(i), &realSucc) == &realSucc) {
      return static_cast<int>(i);
    }
    ++i;
  }
  // bb not in the vector
  return -1;
}

// delete all empty bb used to connect its pred and succ, like: pred -- empty -- empty -- succ
// the result after this will be : pred -- succ
// if no empty exist, return;
// we will stop at stopBB(stopBB will not be deleted), if stopBB is nullptr, means no constraint
void EliminateEmptyConnectingBB(const BB *predBB, BB *emptyBB, const BB *stopBB, MeCFG &cfg) {
  if (emptyBB == stopBB && emptyBB != nullptr && predBB->IsPredBB(*stopBB)) {
    return;
  }
  // we can only eliminate those emptyBBs that have only one pred and succ
  while (emptyBB != nullptr && emptyBB != stopBB && IsConnectingBB(*emptyBB)) {
    BB *succ = emptyBB->GetSucc(0);
    BB *pred = emptyBB->GetPred(0);
    DEBUG_LOG() << "Delete empty connecting : BB" << LOG_BBID(pred) << "->BB" << LOG_BBID(emptyBB) << "(deleted)->BB"
                << LOG_BBID(succ) << "\n";
    if (pred->IsPredBB(*succ)) {
      pred->RemoveSucc(*emptyBB, true);
      emptyBB->RemoveSucc(*succ, true);
    } else {
      int predIdx = succ->GetPredIndex(*emptyBB);
      succ->SetPred(static_cast<size_t>(predIdx), pred);
      int succIdx = pred->GetSuccIndex(*emptyBB);
      pred->SetSucc(static_cast<size_t>(succIdx), succ);
    }
    if (emptyBB->GetAttributes(kBBAttrIsEntry)) {
      cfg.GetCommonEntryBB()->RemoveEntry(*emptyBB);
      cfg.GetCommonEntryBB()->AddEntry(*succ);
      succ->SetAttributes(kBBAttrIsEntry);
      emptyBB->ClearAttributes(kBBAttrIsEntry);
    }
    cfg.DeleteBasicBlock(*emptyBB);
    emptyBB = succ;
  }
}

bool HasFallthruPred(const BB &bb) {
  return GetFallthruPredNum(bb) != 0;
}

// For BB Level optimization
class OptimizeBB {
 public:
  OptimizeBB(BB *bb, MeFunction &func, std::map<OStIdx, std::unique_ptr<std::set<BBId>>> *candidates)
      : currBB(bb),
        f(func),
        cfg(func.GetCfg()),
        irmap(func.GetIRMap()),
        irBuilder(func.GetMIRModule().GetMIRBuilder()),
        isMeIR(irmap != nullptr),
        cands(candidates) {}
  // optimize each currBB until no change occur
  bool OptBBIteratively();
  // initial factory to create corresponding optimizer for currBB according to BBKind.
  void InitBBOptFactory();

 private:
  BB *currBB = nullptr;             // BB we currently perform optimization on
  MeFunction &f;                    // function we currently perform optimization on
  MeCFG *cfg = nullptr;             // requiring cfg to find pattern to optimize current BB
  MeIRMap *irmap = nullptr;         // used to create new MeExpr/MeStmt
  MIRBuilder *irBuilder = nullptr;  // used to create new BaseNode(expr)/StmtNode
  bool isMeIR = false;
  std::map<OStIdx, std::unique_ptr<std::set<BBId>>> *cands = nullptr;  // candidates ost need to be updated ssa
  std::map<BBId, std::set<OStIdx>> candsOstInBB;                       // bb with ost needed to be updated

  bool repeatOpt = false;  // It will be always set false by OptBBIteratively. If there is some optimization
                           // opportunity for currBB after a/some optimization, we should set it true.
                           // Otherwise it will stop after all optimizations finished and continue to nextBB.
  enum BBErrStat {
    kBBNoErr,                 // BB is normal
    kBBErrNull,               // BB is nullptr
    kBBErrOOB,                // BB id is out-of-bound
    kBBErrCommonEntryOrExit,  // BB is CommonEntry or CommonExit
    kBBErrDel                 // BB has been deleted before
  };

  // For BBChecker, if currBB has error state, do not opt it.
  BBErrStat bbErrStat = kBBNoErr;

  // Optimize once time on bb, some common cfg opt and peephole cfg opt will be performed on currBB
  bool OptBBOnce();
  // elminate BB that is unreachable:
  // 1.BB has no pred(expect then entry block)
  // 2.BB has itself as pred
  bool EliminateDeadBB();
  // chang condition branch to unconditon branch if possible
  // 1.condition is a constant
  // 2.all branches of condition branch is the same BB
  bool OptimizeCondBB2UnCond();
  // disconnect predBB and currBB if predBB must cause error(e.g. null ptr deref)
  // If a expr is always cause error in predBB, predBB will never reach currBB
  bool RemoveSuccFromNoReturnBB();
  // currBB has only one pred, and pred has only one succ
  // try to merge two BB, return true if merged, return false otherwise.
  bool MergeDistinctBBPair();
  // Eliminate redundant philist in bb which has only one pred
  bool EliminateRedundantPhiList();

  // Following function check for BB pattern according to BB's Kind
  bool OptimizeCondBB();
  bool OptimizeUncondBB();
  bool OptimizeFallthruBB();
  bool OptimizeReturnBB();
  bool OptimizeSwitchBB();

  // for sub-pattern in OptimizeCondBB
  MeExpr *TryToSimplifyCombinedCond(const MeExpr &expr);
  bool FoldBranchToCommonDest(BB *pred, BB *succ);
  bool FoldBranchToCommonDest();
  bool SkipRedundantCond();
  bool SkipRedundantCond(BB &pred, BB &succ);
  // for MergeDistinctBBPair
  BB *MergeDistinctBBPair(BB *pred, BB *succ);
  // merge two bb, if merged, return combinedBB, Otherwise return nullptr
  BB *MergeSuccIntoPred(BB *pred, BB *succ);
  bool CondBranchToSelect();
  bool FoldCondBranch();
  bool IsProfitableForCond2Sel(MeExpr *condExpr, MeExpr *trueExpr, MeExpr *falseExpr);
  // for OptimizeUncondBB
  bool MergeGotoBBToPred(BB *gotoBB, BB *pred);
  // after moving pred from curr to curr's successor (i.e. succ), update the phiList of curr and succ
  // a phiOpnd will be removed from curr's philist, and a phiOpnd will be inserted to succ's philist
  // note: when replace pred's succ (i.e. curr) with succ, please DO NOT remove phiOpnd immediately,
  // otherwise we cannot get phiOpnd in this step
  void UpdatePhiForMovingPred(int predIdxForCurr, const BB *pred, BB *curr, BB *succ);
  // for OptimizeCondBB2UnCond
  bool BranchBB2UncondBB(BB &bb);
  // Get first return BB
  BB *GetFirstReturnBB();
  bool EliminateRedundantPhiList(BB *bb);

  // Check if state of currBB is error.
  bool CheckCurrBB();
  // Insert ost of philist in bb to cand, and set ost start from newBB(newBB will be bb itself if not specified)
  void UpdateSSACandForBBPhiList(BB *bb, const BB *newBB = nullptr);
  void UpdateSSACandForOst(const OStIdx &ostIdx, const BB *bb);
  // replace oldBBID in cands with newBBID
  void UpdateBBIdInSSACand(const BBId &oldBBID, const BBId &newBBID);

  void DeleteBB(BB *bb);

  bool IsEmptyBB(BB &bb) {
    return isMeIR ? IsMeEmptyBB(bb) : IsMplEmptyBB(bb);
  }

  void SetBBRunAgain() {
    repeatOpt = true;
  }
  void ResetBBRunAgain() {
    repeatOpt = false;
  }
#define CHECK_CURR_BB() \
  if (!CheckCurrBB()) { \
    return false;       \
  }

#define ONLY_FOR_MEIR() \
  if (!isMeIR) {        \
    return false;       \
  }
};

using OptBBFatory = FunctionFactory<BBKind, bool, OptimizeBB*>;

bool OptimizeBB::CheckCurrBB() {
  if (bbErrStat != kBBNoErr) {  // has been checked before
    return false;
  }
  if (currBB == nullptr) {
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

void OptimizeBB::UpdateBBIdInSSACand(const BBId &oldBBID, const BBId &newBBID) {
  auto it = candsOstInBB.find(oldBBID);
  if (it == candsOstInBB.end()) {
    return;  // no item to update
  }
  auto &ostSet = candsOstInBB[newBBID];  // find or create
  ostSet.insert(it->second.begin(), it->second.end());
  candsOstInBB.erase(it);
  for (auto &cand : *cands) {
    auto item = cand.second->find(oldBBID);
    if (item != cand.second->end()) {
      cand.second->erase(item);
      cand.second->emplace(newBBID);
    }
  }
}

void OptimizeBB::UpdateSSACandForOst(const OStIdx &ostIdx, const BB *bb) {
  MeSSAUpdate::InsertOstToSSACands(ostIdx, *bb, cands);
}

void OptimizeBB::UpdateSSACandForBBPhiList(BB *bb, const BB *newBB) {
  if (!isMeIR || bb == nullptr || bb->GetMePhiList().empty()) {
    return;
  }
  if (newBB == nullptr) {  // if not specified, newBB is bb itself
    newBB = bb;
  }
  std::set<OStIdx> &ostSet = candsOstInBB[newBB->GetBBId()];
  for (auto phi : bb->GetMePhiList()) {
    OStIdx ostIdx = phi.first;
    UpdateSSACandForOst(ostIdx, newBB);
    ostSet.emplace(ostIdx);
  }
  if (bb != newBB) {
    auto it = candsOstInBB.find(bb->GetBBId());
    if (it != candsOstInBB.end()) {
      // ost in bb should be updated, make it updated with newBB
      for (auto ostIdx : it->second) {
        UpdateSSACandForOst(ostIdx, newBB);
        ostSet.emplace(ostIdx);
      }
    }
  }
}

void OptimizeBB::DeleteBB(BB *bb) {
  if (bb == nullptr) {
    return;
  }
  bb->GetSucc().clear();
  bb->GetPred().clear();
  cfg->DeleteBasicBlock(*bb);
  bb->GetSuccFreq().clear();
}

// eliminate dead BB
// 1.BB has no pred(expect then entry block)
// 2.BB has only itself as pred
bool OptimizeBB::EliminateDeadBB() {
  CHECK_CURR_BB();
  if (currBB->IsSuccBB(*cfg->GetCommonEntryBB())) {
    return false;
  }
  if (currBB->GetPred().empty() || currBB->GetUniquePred() == currBB) {
    DEBUG_LOG() << "EliminateDeadBB : Delete BB" << LOG_BBID(currBB) << "\n";
    currBB->RemoveAllSucc();
    if (currBB->IsPredBB(*cfg->GetCommonExitBB())) {
      cfg->GetCommonExitBB()->RemoveExit(*currBB);
    }
    DeleteBB(currBB);
    return true;
  }
  return false;
}

// Eliminate redundant philist in bb which has only one pred
bool OptimizeBB::EliminateRedundantPhiList() {
  CHECK_CURR_BB();
  ONLY_FOR_MEIR();
  return EliminateRedundantPhiList(currBB);
}

// Eliminate redundant philist in bb which has only one pred
bool OptimizeBB::EliminateRedundantPhiList(BB *bb) {
  CHECK_FATAL(bb, "bb is null!");
  if (bb->GetPred().size() != 1) {
    return false;
  }
  UpdateSSACandForBBPhiList(bb, bb->GetPred(0));
  bb->GetMePhiList().clear();
  return true;
}

// all branches of bb are the same if skipping all empty connecting BB
// turn it into an unconditional BB (if destBB has other pred, bb->gotoBB, otherwise, bb->fallthruBB)
//      bb                             bb
//   /  |  \                            |
//  A   B   C  (ABC are empty)  ==>     |
//   \  |  /                            |
//    destBB                          destBB
bool OptimizeBB::BranchBB2UncondBB(BB &bb) {
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

  DEBUG_LOG() << "BranchBB2UncondBB : " << (bb.GetKind() == kBBCondGoto ? "Conditional " : "Switch ") << "BB"
              << LOG_BBID(&bb) << " to unconditional BB\n";
  // delete all empty bb between bb and destBB
  // note : bb and destBB will be connected after empty BB is deleted
  for (int i = static_cast<int>(bb.GetSucc().size()) - 1; i >= 0; --i) {
    EliminateEmptyConnectingBB(&bb, bb.GetSucc(static_cast<size_t>(i)), destBB, *cfg);
  }
  while (bb.GetSucc().size() != 1) { // bb is an unconditional bb now, and its successor num should be 1
    ASSERT(bb.GetSucc().back() == destBB,
           "[FUNC: %s]Goto BB%d has different destination", funcName.c_str(), LOG_BBID(&bb));
    bb.RemoveSucc(*bb.GetSucc().back());
  }
  if (cfg->UpdateCFGFreq()) {
    ASSERT(bb.GetSuccFreq().size() == bb.GetSucc().size(), "sanity check");
    bb.SetSuccFreq(0, bb.GetFrequency());
  }
  // bb must be one of fallthru pred of destBB, if there is another one,
  // we should add gotoStmt to avoid duplicate fallthru pred
  if (GetFallthruPredNum(*destBB) > 1) {
    LabelIdx label = f.GetOrCreateBBLabel(*destBB);
    if (isMeIR) {
      ASSERT_NOT_NULL(bb.GetLastMe());
      auto *gotoStmt = irmap->CreateGotoMeStmt(label, &bb, &bb.GetLastMe()->GetSrcPosition());
      bb.RemoveLastMeStmt();
      bb.AddMeStmtLast(gotoStmt);
    } else {
      auto *gotoStmt = irBuilder->CreateStmtGoto(OP_goto, label);
      bb.RemoveLastStmt();
      bb.AddStmtNode(gotoStmt);
    }
    bb.SetKind(kBBGoto);
  } else {
    if (isMeIR) {
      bb.RemoveLastMeStmt();
    } else {
      bb.RemoveLastStmt();
    }
    bb.SetKind(kBBFallthru);
  }
  return true;
}

// chang condition branch to unconditon branch if possible
// 1.condition is a constant
// 2.all branches of condition branch is the same BB
bool OptimizeBB::OptimizeCondBB2UnCond() {
  CHECK_CURR_BB();
  if (currBB->GetKind() != kBBCondGoto) {
    return false;
  }
  // case 2
  if (FindFirstRealSucc(currBB->GetSucc(0)) == FindFirstRealSucc(currBB->GetSucc(1))) {
    if (BranchBB2UncondBB(*currBB)) {
      SetBBRunAgain();
      return true;
    }
  }

  // case 1
  BB *ftBB = currBB->GetSucc(0);
  BB *gtBB = currBB->GetSucc(1);
  FreqType removedSuccFreq = 0;
  if (isMeIR) {
    MeStmt *brStmt = currBB->GetLastMe();
    ASSERT_NOT_NULL(brStmt);
    MeExpr *condExpr = brStmt->GetOpnd(0);
    if (condExpr->GetMeOp() == kMeOpConst) {
      CHECK_FATAL(brStmt, "brStmt is nullptr!");
      bool isCondTrue = (!condExpr->IsZero());
      bool isBrtrue = (brStmt->GetOp() == OP_brtrue);
      if (isCondTrue != isBrtrue) {  // goto fallthru BB
        currBB->RemoveLastMeStmt();
        currBB->SetKind(kBBFallthru);
        if (cfg->UpdateCFGFreq()) {
          removedSuccFreq = currBB->GetSuccFreq()[1];
        }
        currBB->RemoveSucc(*gtBB, true);
        // update frequency
        if (cfg->UpdateCFGFreq()) {
          currBB->SetSuccFreq(0, currBB->GetFrequency());
          ftBB->SetFrequency(ftBB->GetFrequency() + removedSuccFreq);
          ftBB->UpdateEdgeFreqs(false);
        }
      } else {
        MeStmt *gotoStmt = irmap->CreateGotoMeStmt(f.GetOrCreateBBLabel(*gtBB), currBB, &brStmt->GetSrcPosition());
        currBB->ReplaceMeStmt(brStmt, gotoStmt);
        currBB->SetKind(kBBGoto);
        // update frequency
        if (cfg->UpdateCFGFreq()) {
          removedSuccFreq = currBB->GetSuccFreq()[0];
        }
        currBB->RemoveSucc(*ftBB, true);
        if (cfg->UpdateCFGFreq()) {
          currBB->SetSuccFreq(0, currBB->GetFrequency());
          gtBB->SetFrequency(gtBB->GetFrequency() + removedSuccFreq);
          gtBB->UpdateEdgeFreqs(false);
        }
      }
      SetBBRunAgain();
      return true;
    }
  } else {
    StmtNode &brStmt = currBB->GetLast();
    BaseNode *condExpr = brStmt.Opnd(0);
    if (condExpr->GetOpCode() == OP_constval) {
      MIRConst *constVal = static_cast<ConstvalNode*>(condExpr)->GetConstVal();
      bool isCondTrue = (!constVal->IsZero());
      bool isBrTrue = (brStmt.GetOpCode() == OP_brtrue);
      if (isCondTrue != isBrTrue) {
        currBB->RemoveLastStmt();
        currBB->SetKind(kBBFallthru);
        if (cfg->UpdateCFGFreq()) {
          removedSuccFreq = currBB->GetSuccFreq()[1];
        }
        currBB->RemoveSucc(*gtBB);
        if (cfg->UpdateCFGFreq()) {
          currBB->SetSuccFreq(0, currBB->GetFrequency());
          ftBB->SetFrequency(ftBB->GetFrequency() + removedSuccFreq);
          ftBB->UpdateEdgeFreqs(false);
        }
      } else {
        StmtNode *gotoStmt = irBuilder->CreateStmtGoto(OP_goto, f.GetOrCreateBBLabel(*gtBB));
        currBB->ReplaceStmt(&brStmt, gotoStmt);
        currBB->SetKind(kBBGoto);
        if (cfg->UpdateCFGFreq()) {
          removedSuccFreq = currBB->GetSuccFreq()[0];
        }
        currBB->RemoveSucc(*ftBB);
        if (cfg->UpdateCFGFreq()) {
          currBB->SetSuccFreq(0, currBB->GetFrequency());
          gtBB->SetFrequency(gtBB->GetFrequency() + removedSuccFreq);
          gtBB->UpdateEdgeFreqs(false);
        }
      }
      SetBBRunAgain();
      return true;
    }
  }
  return false;
}

// return first return bb
BB *OptimizeBB::GetFirstReturnBB() {
  for (auto *bb : cfg->GetAllBBs()) {
    if (bb == nullptr) {
      continue;
    }
    if (bb->GetKind() == kBBReturn) {
      return bb;
    }
  }
  ASSERT(false, "should never reach here");
  return nullptr;
}

// Disconnect no-return BB with all its successors. BB is a no-return BB if:
// case 1: BB has stmt that must throw exception(e.g. deref nullptr, div/rem zero)
// case 2: BB has call site of func with attribute FUNCATTR_noreturn
// If a stmt in currBB is no return, currBB will never reach its successors
bool OptimizeBB::RemoveSuccFromNoReturnBB() {
  CHECK_CURR_BB();
  ONLY_FOR_MEIR();
  if (currBB->IsMeStmtEmpty() || currBB->GetSucc().empty()) {
    return false;
  }
  MeStmt *exceptionStmt = GetNoReturnStmt(currBB);
  if (exceptionStmt != nullptr) {
    DEBUG_LOG() << "RemoveSuccFromNoReturnBB : Remove all successors from pred BB" << LOG_BBID(currBB)
                << ", and remove all stmts after noreturn stmt\n";
    if ((currBB->IsPredBB(*cfg->GetCommonExitBB()) && currBB->GetLastMe() == exceptionStmt) ||
        (currBB->GetKind() == kBBNoReturn)) {
      // it has been dealt with before or it has been connected to commonExit, do nothing
      return false;
    }
    if (!currBB->GetSucc().empty()) {
      currBB->RemoveAllSucc();
    }
    while (currBB->GetLastMe() != exceptionStmt) {
      CHECK_FATAL(currBB->GetLastMe(), "LastMe is nullptr!");
      currBB->GetLastMe()->SetIsLive(false);
      currBB->RemoveLastMeStmt();
    }
    std::set<MeExpr*> exceptionExprSet;
    bool divOrRemException = false;
    for (size_t i = 0; i < exceptionStmt->NumMeStmtOpnds(); ++i) {
      MustThrowExceptionExpr(exceptionStmt->GetOpnd(i), exceptionExprSet, divOrRemException);
    }
    // if exceptionStmt not a callsite of exit func, we replace it with a exception-throwing expr.
    if (f.GetMIRModule().IsCModule() && divOrRemException) {
      currBB->RemoveLastMeStmt();
      auto *newCallMeStmt = irmap->NewInPool<IntrinsiccallMeStmt>(
          OP_intrinsiccall, INTRN_C___builtin_division_exception);
      newCallMeStmt->CopyInfo(*exceptionStmt);
      for (auto *exceptionExpr : exceptionExprSet) {
        newCallMeStmt->PushBackOpnd(exceptionExpr);
      }
      currBB->AddMeStmtLast(newCallMeStmt);
    } else if (exceptionStmt->GetOp() != OP_call) {
      currBB->RemoveLastMeStmt();
      // we create a expr that must throw exception
      ASSERT(!exceptionExprSet.empty(), "Exception stmt must have an exception expr");
      for (auto *exceptionExpr : exceptionExprSet) {
        UnaryMeStmt *evalStmt = irmap->New<UnaryMeStmt>(OP_eval);
        evalStmt->SetOpnd(0, exceptionExpr);
        evalStmt->CopyInfo(*exceptionStmt);
        currBB->AddMeStmtLast(evalStmt);
      }
    }
    currBB->SetKind(kBBNoReturn);
    // connect to exit for postdom
    cfg->GetCommonExitBB()->AddExit(*currBB);
    currBB->SetAttributes(kBBAttrIsExit);
    ResetBBRunAgain();
    return true;
  }
  return false;
}

// merge two bb, if merged, return combinedBB, Otherwise return nullptr
BB *OptimizeBB::MergeSuccIntoPred(BB *pred, BB *succ) {
  if (pred == cfg->GetCommonEntryBB() || succ == cfg->GetCommonExitBB()) {
    return nullptr;
  }
  if (pred->GetUniqueSucc() != succ || succ->GetUniquePred() != pred) {
    return nullptr;
  }
  if (pred->GetKind() == kBBGoto && (!IsMeEmptyBB(*succ) || !IsMplEmptyBB(*succ))) {
    if (isMeIR) {
      // remove last mestmt
      ASSERT(pred->GetLastMe()->GetOp() == OP_goto,
             "[FUNC: %s]GotoBB has no goto stmt as its terminator", funcName.c_str());
      pred->RemoveLastMeStmt();
    } else {
      // remove last stmt
      ASSERT(pred->GetLast().GetOpCode() == OP_goto,
             "[FUNC: %s]GotoBB has no goto stmt as its terminator", funcName.c_str());
      pred->RemoveLastStmt();
    }
    pred->SetKind(kBBFallthru);
  }
  if (pred->GetKind() != kBBFallthru && pred->GetKind() != kBBGoto) {
    // Only goto and fallthru BB are allowed
    return nullptr;
  }
  bool isSuccEmpty = IsMeEmptyBB(*succ) && IsMplEmptyBB(*succ);
  if (!isSuccEmpty) {
    if (isMeIR) {
      MeStmt *stmt = succ->GetFirstMe();
      while (stmt != nullptr && (!succ->IsMeStmtEmpty())) {
        MeStmt *next = stmt->GetNextMeStmt();
        succ->RemoveMeStmt(stmt);
        pred->AddMeStmtLast(stmt);
        stmt = next;
      }
    } else {
      StmtNode *stmt = &succ->GetFirst();
      while (stmt != nullptr && (!succ->IsEmpty())) {
        StmtNode *next = stmt->GetNext();
        succ->RemoveStmtNode(stmt);
        pred->AddStmtNode(stmt);
        stmt = next;
      }
    }
  }
  // update succFreqs before update succs
  if (cfg->UpdateCFGFreq() && !succ->GetSuccFreq().empty()) {
    // copy succFreqs of succ to pred
    for (size_t i = 0; i < succ->GetSuccFreq().size(); i++) {
      pred->PushBackSuccFreq(succ->GetSuccFreq()[i]);
    }
  }
  succ->MoveAllSuccToPred(pred, cfg->GetCommonExitBB());
  pred->RemoveSucc(*succ, false);  // philist will not be associated with pred BB, no need to update phi here.
  if (!isSuccEmpty) {
    // only when we move stmt from succ to pred should we set attr and kind here
    pred->SetAttributes(succ->GetAttributes());
    pred->SetKind(succ->GetKind());
  } else if (pred->GetKind() == kBBGoto) {
    if (pred->IsPredBB(*cfg->GetCommonExitBB())) {
      if (isMeIR) {
        pred->RemoveLastMeStmt();
      } else {
        pred->RemoveLastStmt();
      }
      pred->SetKind(kBBFallthru);
    } else {
      // old target of goto is succ, and we merge succ into pred, so we should update it
      cfg->UpdateBranchTarget(*pred, *succ, *pred->GetSucc(0), f);
    }
  }
  if (pred->GetBBId().GetIdx() > succ->GetBBId().GetIdx()) {
    DEBUG_LOG() << "Swap BBID : BB" << LOG_BBID(pred) << " <-> BB" << LOG_BBID(succ) << "\n";
    cfg->SwapBBId(*pred, *succ);
    // update bbid : tell ssa-updater to update with new BBID
    UpdateBBIdInSSACand(succ->GetBBId(), pred->GetBBId());
  }
  DEBUG_LOG() << "Merge BB" << LOG_BBID(succ) << " to BB" << LOG_BBID(pred) << ", and delete BB" << LOG_BBID(succ)
              << "\n";
  UpdateSSACandForBBPhiList(succ, pred);
  DeleteBB(succ);
  succ->SetBBId(pred->GetBBId());  // succ has been merged to pred, and reference to succ should be set to pred too.
  return pred;
}

BB *OptimizeBB::MergeDistinctBBPair(BB *pred, BB *succ) {
  if (pred == nullptr || succ == nullptr || succ == pred) {
    return nullptr;
  }
  if (pred != succ->GetUniquePred() || pred == cfg->GetCommonEntryBB() || pred->IsPredBB(*cfg->GetCommonExitBB())) {
    return nullptr;
  }
  if (succ != pred->GetUniqueSucc() || succ == cfg->GetCommonExitBB() || succ->IsSuccBB(*cfg->GetCommonEntryBB())) {
    return nullptr;
  }
  auto &addrTaken = f.GetMirFunc()->GetLabelTab()->GetAddrTakenLabels();
  if (addrTaken.find(succ->GetBBLabel()) != addrTaken.end()) {
    // need keep address taken label to maintain jump address
    return nullptr;
  }
  // start merging currBB and predBB
  return MergeSuccIntoPred(pred, succ);
}

// currBB has only one pred, and pred has only one succ
// try to merge two BB, return true if merged, return false otherwise.
bool OptimizeBB::MergeDistinctBBPair() {
  CHECK_CURR_BB();
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

bool OptimizeBB::IsProfitableForCond2Sel(MeExpr *condExpr, MeExpr *trueExpr, MeExpr *falseExpr) {
  if (trueExpr == falseExpr) {
    return true;
  }
 /* Select for Float128 is not possible */
  if (condExpr->GetPrimType() == PTY_f128 || falseExpr->GetPrimType() == PTY_f128) {
    return false;
  }
  ASSERT(IsSafeExpr(trueExpr), "[FUNC: %s]Please check for safety first", funcName.c_str());
  ASSERT(IsSafeExpr(falseExpr), "[FUNC: %s]Please check for safety first", funcName.c_str());
  // try to simplify select expr
  MeExpr *selExpr = irmap->CreateMeExprSelect(trueExpr->GetPrimType(), *condExpr, *trueExpr, *falseExpr);
  MeExpr *simplifiedSel = irmap->SimplifyMeExpr(selExpr);
  if (simplifiedSel != selExpr) {
    return true;  // can be simplified
  }

  // We can check for every opnd of opndExpr, and calculate their cost according to cg's insn
  // but optimization in mplbe may change the insn and the result is not correct after that.
  // Therefore, to make this easier, only reg/const/var are allowed here
  MeExprOp trueOp = trueExpr->GetMeOp();
  MeExprOp falseOp = falseExpr->GetMeOp();
  if (trueOp == kMeOpVar && !DoesExprContainSubExpr(condExpr, trueExpr)) {
    return false;
  }
  if (falseOp == kMeOpVar && !DoesExprContainSubExpr(condExpr, falseExpr)) {
    return false;
  }
  if ((trueOp != kMeOpConst && trueOp != kMeOpReg && trueOp != kMeOpVar) ||
      (falseOp != kMeOpConst && falseOp != kMeOpReg && falseOp != kMeOpVar)) {
    return false;
  }
  // big integer
  if (GetNonSimpleImm(trueExpr) != 0 || GetNonSimpleImm(falseExpr) != 0) {
    return false;
  }
  return true;
}

// Ignoring connecting BB, two cases can be turn into select
//     condBB        condBB
//     /   \         /    \
 //   ftBB  gtBB     |   gtBB/ftBB
//   x<-   x<-      |     x<-
//     \   /         \     /
//     jointBB       jointBB
bool OptimizeBB::CondBranchToSelect() {
  CHECK_CURR_BB();
  if (currBB->GetKind() != kBBCondGoto) {
    return false;
  }
  BB *ftBB = FindFirstRealSucc(currBB->GetSucc(0));  // fallthruBB
  BB *gtBB = FindFirstRealSucc(currBB->GetSucc(1));  // gotoBB
  if (ftBB == gtBB) {
    (void)BranchBB2UncondBB(*currBB);
    return true;
  }
  // if ftBB or gtBB itself is jointBB, just return itself; otherwise return real succ
  BB *ftTargetBB =
      (ftBB->GetPred().size() == 1 && ftBB->GetSucc().size() == 1) ? FindFirstRealSucc(ftBB->GetSucc(0)) : ftBB;
  BB *gtTargetBB =
      (gtBB->GetPred().size() == 1 && gtBB->GetSucc().size() == 1) ? FindFirstRealSucc(gtBB->GetSucc(0)) : gtBB;
  if (ftTargetBB != gtTargetBB) {
    return false;
  }
  BB *jointBB = ftTargetBB;  // common succ
  // Check if ftBB has only one assign stmt, set ftStmt if a assign stmt exist
  AssignMeStmt *ftStmt = nullptr;
  if (ftBB != jointBB) {
    ftStmt = GetSingleAssign(ftBB);
    if (ftStmt == nullptr) {
      return false;
    }
  }
  // Check if gtBB has only one assign stmt, set gtStmt if a assign stmt exist
  AssignMeStmt *gtStmt = nullptr;
  if (gtBB != jointBB) {
    gtStmt = GetSingleAssign(gtBB);
    if (gtStmt == nullptr) {
      return false;
    }
  }
  if (ftStmt == nullptr && gtStmt == nullptr) {
    DEBUG_LOG() << "Abort cond2sel for BB" << LOG_BBID(currBB) << ", because no single assign stmt is found\n";
    return false;
  }

  // Here we found a pattern, collect select opnds and result reg
  ScalarMeExpr *ftLHS = nullptr;
  MeExpr *ftRHS = nullptr;
  std::set<ScalarMeExpr*> chiListCands;  // collect chilist if assign stmt has one
  if (ftStmt != nullptr) {
    ftLHS = static_cast<ScalarMeExpr*>(ftStmt->GetLHS());
    ftRHS = ftStmt->GetRHS();
    if (ftStmt->GetChiList() != nullptr) {
      for (auto &chiNode : *ftStmt->GetChiList()) {
        chiListCands.emplace(chiNode.second->GetRHS());
      }
    }
  }
  ScalarMeExpr *gtLHS = nullptr;
  MeExpr *gtRHS = nullptr;
  if (gtStmt != nullptr) {
    gtLHS = static_cast<ScalarMeExpr*>(gtStmt->GetLHS());
    gtRHS = gtStmt->GetRHS();
    if (gtStmt->GetChiList() != nullptr) {
      for (auto &chiNode : *gtStmt->GetChiList()) {
        chiListCands.emplace(chiNode.second->GetRHS());
      }
    }
  }
  // We have tried to get ftRHS/gtRHS from stmt before, if it still nullptr, try to find it from phiNode
  //    a <- mx1
  //      cond
  //     |    \
  //     |    a <- mx2
  //     |    /
  //     a <- phi(mx1, mx2)
  // we turn it to
  // a <- cond ? mx1 : mx2
  if (ftRHS == nullptr) {
    ftRHS = FindCond2SelRHSFromPhiNode(currBB, ftBB, jointBB, gtLHS->GetOstIdx());
    if (ftRHS == nullptr) {
      return false;
    }
    ftLHS = gtLHS;
  } else if (gtRHS == nullptr) {
    gtRHS = FindCond2SelRHSFromPhiNode(currBB, gtBB, jointBB, ftLHS->GetOstIdx());
    if (gtRHS == nullptr) {
      return false;
    }
    gtLHS = ftLHS;
  }
  // pattern not found
  if (gtLHS->GetOstIdx() != ftLHS->GetOstIdx()) {
    DEBUG_LOG() << "Abort cond2sel for BB" << LOG_BBID(currBB) << ", because two ost assigned are not the same\n";
    return false;
  }
  DEBUG_LOG() << "Candidate cond2sel BB" << LOG_BBID(currBB) << "(cond)->{BB" << LOG_BBID(currBB->GetSucc(0)) << ", BB"
              << LOG_BBID(currBB->GetSucc(1)) << "}->BB" << LOG_BBID(ftTargetBB) << "(jointBB)\n";
  if (ftRHS != gtRHS) {  // if ftRHS is the same as gtRHS, they can be simplified, and no need to check safety
    // black list
    if (!IsSafeExpr(ftRHS) || !IsSafeExpr(gtRHS)) {
      DEBUG_LOG() << "Abort cond2sel for BB" << LOG_BBID(currBB) << ", because trueExpr or falseExpr is not safe\n";
      return false;
    }
  }
  MeStmt *condStmt = currBB->GetLastMe();
  CHECK_FATAL(condStmt, "condStmt is nullptr!");
  MeExpr *trueExpr = (condStmt->GetOp() == OP_brtrue) ? gtRHS : ftRHS;
  MeExpr *falseExpr = (trueExpr == gtRHS) ? ftRHS : gtRHS;
  MeExpr *condExpr = condStmt->GetOpnd(0);
  if (!IsProfitableForCond2Sel(condExpr, trueExpr, falseExpr)) {
    DEBUG_LOG() << "Abort cond2sel for BB" << LOG_BBID(currBB) << ", because cond2sel is not profitable\n";
    return false;
  }
  DEBUG_LOG() << "Condition To Select : BB" << LOG_BBID(currBB) << "(cond)->[BB" << LOG_BBID(ftBB) << "(ft), BB"
              << LOG_BBID(gtBB) << "(gt)]->BB" << LOG_BBID(jointBB) << "(joint)\n";
  ScalarMeExpr *resLHS = nullptr;
  if (jointBB->GetPred().size() == 2) {  // if jointBB has only ftBB and gtBB as its pred.
    // use phinode lhs as result
    auto it = jointBB->GetMePhiList().find(ftLHS->GetOstIdx());
    if (it == jointBB->GetMePhiList().end()) {
      return false;  // volatile ost always has no phi node
    }
    resLHS = it->second->GetLHS();
  } else {
    // we should create a new version
    resLHS = irmap->CreateRegOrVarMeExprVersion(ftLHS->GetOstIdx());
  }
  // It is profitable for instruction selection if select opnd is a compare expression
  // select condExpr, trueExpr, falseExpr => select cmpExpr, trueExpr, falseExpr
  if (condExpr->IsScalar() && condExpr->GetPrimType() != PTY_u1) {
    auto *scalarExpr = static_cast<ScalarMeExpr*>(condExpr);
    if (scalarExpr->GetDefBy() == kDefByStmt) {
      MeStmt *defStmt = scalarExpr->GetDefStmt();
      MeExpr *rhs = defStmt->GetRHS();
      if (rhs != nullptr && kOpcodeInfo.IsCompare(rhs->GetOp())) {
        condExpr = rhs;
      }
    }
  }
  MeExpr *selExpr = irmap->CreateMeExprSelect(resLHS->GetPrimType(), *condExpr, *trueExpr, *falseExpr);
  MeExpr *simplifiedSel = irmap->SimplifyMeExpr(selExpr);
  AssignMeStmt *newAssStmt = irmap->CreateAssignMeStmt(*resLHS, *simplifiedSel, *currBB);
  CHECK_FATAL(currBB->GetLastMe(), "LastMe is nullptr!");
  newAssStmt->SetSrcPos(currBB->GetLastMe()->GetSrcPosition());
  // here we do not remove condStmt, because it will be delete in BranchBB2UncondBB
  currBB->InsertMeStmtBefore(condStmt, newAssStmt);
  // we remove assign stmt in ftBB and gtBB to make it an empty BB, so that BranchBB2UncondBB can simplify it.
  if (gtStmt != nullptr) {
    gtBB->RemoveMeStmt(gtStmt);
  }
  if (ftStmt != nullptr) {
    ftBB->RemoveMeStmt(ftStmt);
  }
  (void)BranchBB2UncondBB(*currBB);
  // update phi
  if (jointBB->GetPred().size() == 1) {  // jointBB has only currBB as its pred
    // just remove phinode
    jointBB->GetMePhiList().erase(resLHS->GetOstIdx());
  } else {
    // set phi opnd as resLHS
    MePhiNode *phiNode = jointBB->GetMePhiList()[resLHS->GetOstIdx()];
    int predIdx = GetRealPredIdx(*jointBB, *currBB);
    ASSERT(predIdx != -1, "[FUNC: %s]currBB is not a pred of jointBB", funcName.c_str());
    phiNode->SetOpnd(static_cast<size_t>(predIdx), resLHS);
  }
  // if newAssStmt is an dassign, copy old chilist to it
  if (!chiListCands.empty() && newAssStmt->GetOp() == OP_dassign) {
    MapleMap<OStIdx, ChiMeNode*> *chiList = newAssStmt->GetChiList();
    for (auto *rhs : chiListCands) {
      VarMeExpr *newLHS = irmap->CreateVarMeExprVersion(rhs->GetOst());
      UpdateSSACandForOst(rhs->GetOstIdx(), currBB);
      auto *newChiNode = irmap->New<ChiMeNode>(newAssStmt);
      newLHS->SetDefChi(*newChiNode);
      newLHS->SetDefBy(kDefByChi);
      newChiNode->SetLHS(newLHS);
      newChiNode->SetRHS(rhs);
      chiList->emplace(rhs->GetOstIdx(), newChiNode);
    }
  }
  return true;
}

static MeExpr *FoldCmpOfBitOps(IRMap &irmap, const MeExpr &cmp1, const MeExpr &cmp2) {
  if (cmp1.GetOp() != cmp2.GetOp() || cmp1.GetOpnd(1) != cmp2.GetOpnd(1)) {
    return nullptr;
  }
  auto opnd1 = cmp1.GetOpnd(1);
  if (!opnd1 || opnd1->GetMeOp() != kMeOpConst) {
    return nullptr;
  }
  if (!IsPrimitiveInteger(cmp1.GetOpnd(1)->GetPrimType())) {
    return nullptr;
  }
  auto val = static_cast<ConstMeExpr *>(cmp1.GetOpnd(1))->GetIntValue();
  if (!(cmp1.GetOp() == OP_ne && val == 0)) {
    return nullptr;
  }
  auto op0OfCmp1 = cmp1.GetOpnd(0);
  auto op0OfCmp2 = cmp2.GetOpnd(0);
  return ConstantFold::FoldOrOfAnds(irmap, *op0OfCmp1, *op0OfCmp2);
}

// fold 2 sequential condbranch if they are semantically logical and/or
// cond1                cond3
//   |   \                |\
// cond2  \      ->       | \
//   |   \ \          fallth br
// fallth \|
//   |    br
bool OptimizeBB::FoldCondBranch() {
  CHECK_CURR_BB();
  auto succBB = currBB->GetSucc(0);
  if (succBB->GetKind() != kBBCondGoto) {
    return false;
  }
  auto realBrOfCurr = FindFirstRealSucc(currBB->GetSucc(1));
  auto realBrOfSucc = FindFirstRealSucc(succBB->GetSucc(1));
  if (realBrOfCurr != realBrOfSucc) {
    return false;
  }
  auto stmt1 = static_cast<CondGotoMeStmt *>(currBB->GetLastMe());
  auto stmt2 = static_cast<CondGotoMeStmt *>(succBB->GetFirstMe());
  if (stmt1->GetOp() != stmt2->GetOp()) {
    return false;
  }
  MeExpr *foldExpr = nullptr;

  do {
    bool isAnd = false;
    if (stmt1->GetOp() == OP_brfalse) {
      isAnd = true;
    }

    if (!isAnd && (foldExpr = FoldCmpOfBitOps(*irmap, *stmt1->GetOpnd(), *stmt2->GetOpnd())) != nullptr) {
      break;
    }

    if ((foldExpr = ConstantFold::FoldCmpExpr(*irmap, *stmt1->GetOpnd(), *stmt2->GetOpnd(), isAnd)) != nullptr) {
      break;
    }
  } while (0);

  if (foldExpr) {
    stmt1->SetOpnd(0, foldExpr);
    stmt1->SetBranchProb(stmt2->GetBranchProb());
    succBB->RemoveLastMeStmt();
    succBB->SetKind(kBBFallthru);
    if (cfg->UpdateCFGFreq()) {
      FreqType freqToMove = succBB->GetSuccFreq()[1];
      currBB->SetSuccFreq(0, currBB->GetSuccFreq()[0] - freqToMove);
      succBB->SetFrequency(succBB->GetFrequency() - freqToMove);
      currBB->SetSuccFreq(1, currBB->GetSuccFreq()[1] + freqToMove);
      currBB->GetSucc(1)->SetFrequency(currBB->GetSucc(1)->GetFrequency() + freqToMove);
    }
    BB *succOfSuccBB = succBB->GetSucc(1);
    succBB->RemoveBBFromSucc(*succOfSuccBB);
    succOfSuccBB->RemoveBBFromPred(*succBB, true);
    return true;
  }
  return false;
}

bool IsExprSameLexicalally(MeExpr *expr1, MeExpr *expr2) {
  if (expr1 == expr2) {
    return true;
  }
  PrimType ptyp1 = expr1->GetPrimType();
  PrimType ptyp2 = expr2->GetPrimType();
  if (expr1->GetOp() != expr2->GetOp() || ptyp1 != ptyp2) {
    return false;
  }
  MeExprOp op1 = expr1->GetMeOp();
  switch (op1) {
    case kMeOpConst: {
      MIRConst *const1 = static_cast<ConstMeExpr*>(expr1)->GetConstVal();
      MIRConst *const2 = static_cast<ConstMeExpr*>(expr2)->GetConstVal();
      if (const1->GetKind() == kConstInt) {
        return static_cast<MIRIntConst*>(const1)->GetExtValue() == static_cast<MIRIntConst*>(const2)->GetExtValue();
      } else if (const1->GetKind() == kConstFloatConst) {
        return IsFloatingPointNumBitsSame<float>(static_cast<MIRFloatConst*>(const1)->GetValue(),
                                                 static_cast<MIRFloatConst*>(const2)->GetValue());
      } else if (const1->GetKind() == kConstDoubleConst) {
        return IsFloatingPointNumBitsSame<double>(static_cast<MIRDoubleConst*>(const1)->GetValue(),
                                                  static_cast<MIRDoubleConst*>(const2)->GetValue());
      }
      return false;
    }
    case kMeOpReg:
    case kMeOpVar: {
      return static_cast<ScalarMeExpr*>(expr1)->GetOstIdx() == static_cast<ScalarMeExpr*>(expr2)->GetOstIdx();
    }
    case kMeOpAddrof: {
      return static_cast<AddrofMeExpr*>(expr1)->GetOstIdx() == static_cast<AddrofMeExpr*>(expr2)->GetOstIdx();
    }
    case kMeOpOp: {
      auto *opExpr1 = static_cast<OpMeExpr*>(expr1);
      auto *opExpr2 = static_cast<OpMeExpr*>(expr2);
      if (opExpr1->GetOp() != opExpr2->GetOp() || opExpr1->GetTyIdx() != opExpr2->GetTyIdx() ||
          opExpr1->GetFieldID() != opExpr2->GetFieldID() || opExpr1->GetBitsOffSet() != opExpr2->GetBitsOffSet() ||
          opExpr1->GetBitsSize() != opExpr2->GetBitsSize() || opExpr1->GetNumOpnds() != opExpr2->GetNumOpnds()) {
        return false;
      }
      for (size_t i = 0; i < expr1->GetNumOpnds(); ++i) {
        if (!IsExprSameLexicalally(expr1->GetOpnd(i), expr2->GetOpnd(i))) {
          return false;
        }
      }
      return true;
    }
    default: {
      return false;
    }
  }
}

enum BranchResult {
  kBrFalse,
  kBrTrue,
  kBrUnknown
};

void SwapCmpOpnds(Opcode &op, MeExpr *&opnd0, MeExpr *&opnd1) {
  if (!kOpcodeInfo.IsCompare(op)) {
    return;
  }
  MeExpr *tmp = opnd0;
  opnd0 = opnd1;
  opnd1 = tmp;
  // keep the result of succCond the same
  // for cmp/cmpg/cmpl, only if the opnds are equal, the result is zero, otherwise the result is not zero(-1/+1)
  switch (op) {
    case OP_ge:
      op = OP_le;
      return;
    case OP_gt:
      op = OP_lt;
      return;
    case OP_le:
      op = OP_ge;
      return;
    case OP_lt:
      op = OP_gt;
      return;
    default:
      return;
  }
}

// Precondition : predCond branches to succCond
// isPredTrueBrSucc : predCond->succCond is true branch or false branch
BranchResult InferSuccCondBrFromPredCond(const MeExpr *predCond, const MeExpr *succCond, bool isPredTrueBrSucc) {
  if (!kOpcodeInfo.IsCompare(predCond->GetOp()) || !kOpcodeInfo.IsCompare(succCond->GetOp())) {
    return kBrUnknown;
  }
  if (predCond->ContainsVolatile() || succCond->ContainsVolatile()) {
    return kBrUnknown;
  }
  if (predCond == succCond) {
    return isPredTrueBrSucc ? kBrTrue : kBrFalse;
  }
  Opcode op1 = predCond->GetOp();
  Opcode op2 = succCond->GetOp();
  MeExpr *opnd10 = predCond->GetOpnd(0);
  MeExpr *opnd11 = predCond->GetOpnd(1);
  MeExpr *opnd20 = succCond->GetOpnd(0);
  MeExpr *opnd21 = succCond->GetOpnd(1);
  if (IsExprSameLexicalally(opnd10, opnd21) && IsExprSameLexicalally(opnd11, opnd20)) {
    // swap two opnds ptr of succCond
    SwapCmpOpnds(op2, opnd20, opnd21);
  }
  if (!IsExprSameLexicalally(opnd10, opnd20) || !IsExprSameLexicalally(opnd11, opnd21)) {
    return kBrUnknown;
  }
  if (op1 == op2) {
    return isPredTrueBrSucc ? kBrTrue : kBrFalse;
  }
  if (!isPredTrueBrSucc) {
    if (IsCompareHasReverseOp(op1)) {
      // if predCond false br to succCond, we invert its op and assume it true br to succCond
      op1 = GetReverseCmpOp(op1);
    }
  }
  switch (op1) {
    case OP_ge: {
      if (op2 == OP_lt) {
        return kBrFalse;
      }
      return kBrUnknown;
    }
    case OP_gt: {
      if (op2 == OP_ge || op2 == OP_ne || op2 == OP_cmp) {
        return kBrTrue;
      } else if (op2 == OP_le || op2 == OP_lt || op2 == OP_eq) {
        return kBrFalse;
      }
      return kBrUnknown;
    }
    case OP_eq: {
      if (op2 == OP_gt || op2 == OP_lt || op2 == OP_ne || op2 == OP_cmp) {
        return kBrFalse;
      }
      return kBrUnknown;
    }
    case OP_le: {
      if (op2 == OP_gt) {
        return kBrFalse;
      }
      return kBrUnknown;
    }
    case OP_lt: {
      if (op2 == OP_ge || op2 == OP_gt || op2 == OP_eq) {
        return kBrFalse;
      } else if (op2 == OP_le || op2 == OP_ne || op2 == OP_cmp) {
        return kBrTrue;
      }
      return kBrUnknown;
    }
    case OP_ne: {
      if (op2 == OP_eq) {
        return kBrFalse;
      } else if (op2 == OP_cmp) {
        return kBrTrue;
      }
      return kBrUnknown;
    }
    case OP_cmp: {
      if (op2 == OP_eq) {
        return isPredTrueBrSucc ? kBrFalse : kBrTrue;
      } else if (op2 == OP_ne) {
        return isPredTrueBrSucc ? kBrTrue : kBrFalse;
      }
      return kBrUnknown;
    }
    case OP_cmpg:
    case OP_cmpl: {
      return kBrUnknown;
    }
    default:
      return kBrUnknown;
  }
}

//    ...  pred
//      \  /  \
//      succ  ...
//      /  \
//    ftBB  gtBB
//
// If succ's cond can be inferred from pred's cond, pred can skip succ and branches to one of succ's successors directly
// Here we deal with two cases:
// 1. pred's cond is the same as succ's
// 2. pred's cond is opposite to succ's
bool OptimizeBB::SkipRedundantCond(BB &pred, BB &succ) {
  if (pred.GetKind() != kBBCondGoto || succ.GetKind() != kBBCondGoto || &pred == &succ) {
    return false;
  }
  // try to simplify succ first, if all successors of succ is the same, no need to check the condition
  if (BranchBB2UncondBB(succ)) {
    return true;
  }
  auto *predBr = static_cast<CondGotoMeStmt*>(pred.GetLastMe());
  auto *succBr = static_cast<CondGotoMeStmt*>(succ.GetLastMe());
  CHECK_FATAL(predBr, "predBr is nullptr!");
  CHECK_FATAL(succBr, "succBr is nullptr!");
  if (!IsAllOpndsNotDefByCurrBB(*succBr)) {
    return false;
  }
  MeExpr *predCond = predBr->GetOpnd(0);
  MeExpr *succCond = succBr->GetOpnd(0);
  auto ptfSucc = GetTrueFalseBrPair(&pred);  // pred true and false
  auto stfSucc = GetTrueFalseBrPair(&succ);  // succ true and false
  // Try to infer result of succCond from predCond
  bool isPredTrueBrSucc = (FindFirstRealSucc(ptfSucc.first) == &succ);
  BranchResult tfBranch = InferSuccCondBrFromPredCond(predCond, succCond, isPredTrueBrSucc);
  if (tfBranch == kBrUnknown) {  // succCond cannot be inferred from predCond
    return false;
  }
  // if succ's cond can be inferred from pred's cond, pred can skip succ and branches to newTarget directly
  BB *newTarget = (tfBranch == kBrTrue) ? FindFirstRealSucc(stfSucc.first) : FindFirstRealSucc(stfSucc.second);
  if (newTarget == nullptr || newTarget == &succ) {
    return false;
  }
  DEBUG_LOG() << "Condition in BB" << LOG_BBID(&succ) << " is redundant, since it has been checked in BB"
              << LOG_BBID(&pred) << ", BB" << LOG_BBID(&pred) << " can branch to BB" << LOG_BBID(newTarget) << "\n";
  // succ has only one pred, turn succ to an uncondBB(fallthru or gotoBB)
  //         pred
  //         /  \
  //      succ  ...
  //      /  \
  //    ftBB  gtBB
  if (succ.GetPred().size() == 1) {  // succ has only one pred
    // if newTarget is succ's gotoBB, and it has fallthru pred, we should add a goto stmt to succ's last
    // to replace condGoto stmt. Otherwise, newTarget will have two fallthru pred
    if (newTarget == FindFirstRealSucc(succ.GetSucc(1)) && HasFallthruPred(*newTarget)) {
      CHECK_FATAL(succ.GetLastMe(), "LastMe is nullptr!");
      auto *gotoStmt =
          irmap->CreateGotoMeStmt(f.GetOrCreateBBLabel(*newTarget), &succ, &succ.GetLastMe()->GetSrcPosition());
      succ.ReplaceMeStmt(succ.GetLastMe(), gotoStmt);
      succ.SetKind(kBBGoto);
      DEBUG_LOG() << "SkipRedundantCond : Replace condBr in BB" << LOG_BBID(&succ) << " with an uncond goto\n";
      EliminateEmptyConnectingBB(&succ, succ.GetSucc(1), newTarget, *cfg);
      ASSERT(succ.GetSucc(1) == newTarget, "[FUNC: %s] newTarget should be successor of succ!", funcName.c_str());
    } else {
      succ.RemoveLastMeStmt();
      succ.SetKind(kBBFallthru);
      DEBUG_LOG() << "SkipRedundantCond : Remove condBr in BB" << LOG_BBID(&succ) << ", turn it to fallthruBB\n";
    }
    BB *rmBB = (FindFirstRealSucc(succ.GetSucc(0)) == newTarget) ? succ.GetSucc(1) : succ.GetSucc(0);
    FreqType deletedSuccFreq = 0;
    if (cfg->UpdateCFGFreq()) {
      int idx = succ.GetSuccIndex(*rmBB);
      deletedSuccFreq = succ.GetSuccFreq()[static_cast<uint32>(idx)];
    }
    succ.RemoveSucc(*rmBB, true);
    if (cfg->UpdateCFGFreq()) {
      succ.SetSuccFreq(0, succ.GetFrequency());
      auto *succofSucc = succ.GetSucc(0);
      succofSucc->SetFrequency(succofSucc->GetFrequency() + deletedSuccFreq);
    }
    DEBUG_LOG() << "Remove succ BB" << LOG_BBID(rmBB) << " of pred BB" << LOG_BBID(&succ) << "\n";
    return true;
  } else {
    if (MeOption::optForSize) {
      return false;
    }
    BB *newBB = cfg->NewBasicBlock();
    // if succ has only last stmt, no need to copy
    if (!HasOnlyMeCondGotoStmt(succ)) {
      // succ has more than one pred, clone all stmts in succ (except last stmt) to a new BB
      DEBUG_LOG() << "Create a new BB" << LOG_BBID(newBB) << ", and copy stmts from BB" << LOG_BBID(&succ) << "\n";
      // this step will create new def version and collect it to ssa updater
      f.CloneBBMeStmts(succ, *newBB, cands, true);
      // we should update use version in newBB as phiopnds in succ
      // BB succ:
      //  ...    pred(v2 is def here or its pred)              ...               pred(v2 is def here or its pred)
      //    \      /                                            \                  /
      //      succ                                 ==>         succ             newBB
      //   v3 = phi(..., v2)                               v3 = phi(...)       newBB should use v2 instead of v3
      //
      // here we collect ost in philist of succ, and make it defBB as pred, then ssa updater will update it in newBB
      UpdateSSACandForBBPhiList(&succ, &pred);
    }
    newBB->SetAttributes(succ.GetAttributes());
    if (HasFallthruPred(*newTarget)) {
      // insert a gotostmt to avoid duplicate fallthru pred
      CHECK_FATAL(succ.GetLastMe(), "LastMe is nullptr!");
      auto *gotoStmt =
          irmap->CreateGotoMeStmt(f.GetOrCreateBBLabel(*newTarget), newBB, &succ.GetLastMe()->GetSrcPosition());
      newBB->AddMeStmtLast(gotoStmt);
      newBB->SetKind(kBBGoto);
    } else {
      newBB->SetKind(kBBFallthru);
    }
    BB *replacedSucc = isPredTrueBrSucc ? ptfSucc.first : ptfSucc.second;
    EliminateEmptyConnectingBB(&pred, replacedSucc, &succ, *cfg);
    ASSERT(pred.IsPredBB(succ),
           "[FUNC: %s]After eliminate connecting BB, pred must be predecessor of succ", funcName.c_str());
    int predPredIdx = succ.GetPredIndex(pred); // before replace succ, record predidx for UpdatePhiForMovingPred
    pred.ReplaceSucc(&succ, newBB, false); // do not update phi here, UpdatePhiForMovingPred will do it
    DEBUG_LOG() << "Replace succ BB" << LOG_BBID(replacedSucc) << " with BB" << LOG_BBID(newBB) << ": BB"
                << LOG_BBID(&pred) << "->...->BB" << LOG_BBID(&succ) << "(skipped)"
                << " => BB" << LOG_BBID(&pred) << "->BB" << LOG_BBID(newBB) << "(new)->BB" << LOG_BBID(newTarget)
                << "\n";
    if (pred.GetSucc().size() == 1) {
      pred.RemoveLastMeStmt();
      pred.SetKind(kBBFallthru);
    } else if (pred.GetSucc(1) == newBB) {
      cfg->UpdateBranchTarget(pred, succ, *newBB, f);
    }
    newTarget->AddPred(*newBB);
    UpdatePhiForMovingPred(predPredIdx, newBB, &succ, newTarget);
    // update newBB frequency : copy predBB succFreq as newBB frequency
    if (cfg->UpdateCFGFreq()) {
      int idx = pred.GetSuccIndex(*newBB);
      ASSERT(idx >= 0 && idx < pred.GetSucc().size(), "sanity check");
      FreqType freq = pred.GetEdgeFreq(idx);
      newBB->SetFrequency(freq);
      newBB->PushBackSuccFreq(freq);
      // update frequency of succ because one of its pred is removed
      // frequency of
      FreqType freqOfSucc = succ.GetFrequency();
      ASSERT(freqOfSucc >= freq, "sanity check");
      succ.SetFrequency(freqOfSucc - freq);
      // update edge frequency
      BB *affectedBB = (tfBranch == kBrTrue) ? stfSucc.first : stfSucc.second;
      idx = succ.GetSuccIndex(*affectedBB);
      ASSERT(idx >= 0 && idx < succ.GetSucc().size(), "sanity check");
      FreqType oldedgeFreq = succ.GetSuccFreq()[static_cast<uint32>(idx)];
      if (oldedgeFreq >= freq) {
        succ.SetSuccFreq(idx, oldedgeFreq - freq);
      } else {
        succ.SetSuccFreq(idx, 0);
      }
    }
    return true;
  }
}

bool OptimizeBB::SkipRedundantCond() {
  CHECK_CURR_BB();
  if (currBB->GetKind() != kBBCondGoto) {
    return false;
  }
  bool changed = false;
  // Check for currBB and its successors
  for (size_t i = 0; i < currBB->GetSucc().size(); ++i) {
    BB *realSucc = FindFirstRealSucc(currBB->GetSucc(i));
    changed |= SkipRedundantCond(*currBB, *realSucc);
  }
  return changed;
}

// CurrBB is Condition BB, we will look upward its predBB(s) to see if it can be optimized.
// 1. currBB is X == constVal, and predBB has checked for the same expr, the result is known for currBB's condition,
//    so we can make currBB to be an uncondBB.
// 2. currBB has only one stmt(conditional branch stmt), and the condition's value is calculated by all its predBB
//    we can hoist currBB's stmt to predBBs if it is profitable
// 3. predBB is CondBB, one of predBB's succBB is currBB, and another is one of currBB's successors(commonBB)
//    we can merge currBB to predBB if currBB is simple enough(has only one stmt).
// 4. condition branch to select
bool OptimizeBB::OptimizeCondBB() {
  CHECK_CURR_BB();
  ONLY_FOR_MEIR();
  MeStmt *stmt = currBB->GetLastMe();
  CHECK_FATAL(stmt != nullptr, "[FUNC: %s] CondBB has no stmt", f.GetName().c_str());
  CHECK_FATAL(kOpcodeInfo.IsCondBr(stmt->GetOp()), "[FUNC: %s] Opcode is error!", f.GetName().c_str());
  bool change = false;
  // 2. result of currBB's cond can be inferred from predBB's cond, change predBB's br target
  if (SkipRedundantCond()) {
    SetBBRunAgain();
    change = true;
  }
  // 3.fold two continuous condBB to one condBB, use or/and to combine two condition
  if (FoldBranchToCommonDest()) {
    ResetBBRunAgain();
    return true;
  }
  // 4. condition branch to select
  if (CondBranchToSelect()) {
    SetBBRunAgain();
    return true;
  }
  if (FoldCondBranch()) {
    SetBBRunAgain();
    return true;
  }
  return change;
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
void OptimizeBB::UpdatePhiForMovingPred(int predIdxForCurr, const BB *pred, BB *curr, BB *succ) {
  auto &succPhiList = succ->GetMePhiList();
  auto &currPhilist = curr->GetMePhiList();
  int predPredIdx = succ->GetPredIndex(*pred);
  if (succPhiList.empty()) {
    // succ has only one pred(i.e. curr) before
    // we copy curr's philist to succ, but not with all phiOpnd
    for (auto &phiNode : currPhilist) {
      auto *phiMeNode = irmap->NewInPool<MePhiNode>();
      phiMeNode->SetDefBB(succ);
      succPhiList.emplace(phiNode.first, phiMeNode);
      auto &phiOpnds = phiMeNode->GetOpnds();
      // curr is already pred of succ, so all phiOpnds (except for pred) are phiNode lhs in curr
      phiOpnds.insert(phiOpnds.end(), succ->GetPred().size(), phiNode.second->GetLHS());
      // pred is a new pred for succ, we copy its corresponding phiopnd in curr to succ
      phiMeNode->SetOpnd(static_cast<size_t>(predPredIdx),
                         phiNode.second->GetOpnd(static_cast<size_t>(predIdxForCurr)));
      OStIdx ostIdx = phiNode.first;
      // create a new version for new phi
      phiMeNode->SetLHS(irmap->CreateRegOrVarMeExprVersion(ostIdx));
    }
    UpdateSSACandForBBPhiList(succ);  // new philist has been created in succ
  } else {
    // succ has other pred besides curr
    for (auto &phi : succPhiList) {
      OStIdx ostIdx = phi.first;
      auto it = currPhilist.find(ostIdx);
      ASSERT(predPredIdx != -1,
             "[FUNC: %s]pred BB%d is not a predecessor of succ BB%d yet", funcName.c_str(),
             LOG_BBID(pred), LOG_BBID(succ));
      auto &phiOpnds = phi.second->GetOpnds();
      if (it != currPhilist.end()) {
        // curr has phiNode for this ost, we copy pred's corresponding phiOpnd in curr to succ
        phiOpnds.insert(phiOpnds.begin() + predPredIdx, it->second->GetOpnd(static_cast<size_t>(predIdxForCurr)));
      } else {
        // curr has no phiNode for this ost, pred's phiOpnd in succ will be the same as curr's phiOpnd in succ
        int index = GetRealPredIdx(*succ, *curr);
        ASSERT(index != -1, "[FUNC: %s]succ is not newTarget's real pred", f.GetName().c_str());
        // pred's phi opnd is the same as curr.
        phiOpnds.insert(phiOpnds.begin() + predPredIdx, phi.second->GetOpnd(static_cast<size_t>(index)));
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
        resPair.first->second = phiMeNode;  // replace nullptr inserted before
        auto &phiOpnds = phiMeNode->GetOpnds();
        // insert opnd into New phiNode : all phiOpnds (except for pred) are phiNode lhs in curr
        phiOpnds.insert(phiOpnds.end(), succ->GetPred().size(), phi.second->GetLHS());
        // pred is new pred for succ, we copy its corresponding phiopnd in curr to succ
        phiMeNode->SetOpnd(static_cast<size_t>(predPredIdx), phi.second->GetOpnd(static_cast<size_t>(predIdxForCurr)));
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
bool OptimizeBB::MergeGotoBBToPred(BB *succ, BB *pred) {
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
  if (!HasOnlyMeGotoStmt(*succ)) {
    return false;
  }
  BB *newTarget = succ->GetSucc(0);  // succ must have only one succ, because it is uncondBB
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
    CHECK_FATAL(succ->GetLastMe(), "LastMe is nullptr!");
    GotoMeStmt *gotoMeStmt =
        irmap->CreateGotoMeStmt(newTarget->GetBBLabel(), pred, &succ->GetLastMe()->GetSrcPosition());
    pred->AddMeStmtLast(gotoMeStmt);
    pred->SetKind(kBBGoto);
    DEBUG_LOG() << "Insert Uncond stmt to fallthru BB" << LOG_BBID(currBB) << ", and goto BB" << LOG_BBID(newTarget)
                << "\n";
  }
  int predIdx = succ->GetPredIndex(*pred);
  bool needUpdatePhi = false;
  FreqType removedFreq = 0;
  if (cfg->UpdateCFGFreq()) {
    int idx = pred->GetSuccIndex(*succ);
    removedFreq = pred->GetSuccFreq()[static_cast<uint32>(idx)];
  }
  if (pred->IsPredBB(*newTarget)) {
    pred->RemoveSucc(*succ, true);  // one of pred's succ has been newTarget, avoid duplicate succ here
    if (cfg->UpdateCFGFreq()) {
      int idx = pred->GetSuccIndex(*newTarget);
      pred->SetSuccFreq(idx, pred->GetSuccFreq()[static_cast<uint32>(idx)] + removedFreq);
    }
  } else {
    pred->ReplaceSucc(succ, newTarget);  // phi opnd is not removed from currBB's philist, we will remove it later
    needUpdatePhi = true;
  }
  if (cfg->UpdateCFGFreq()) {
    FreqType succFreq = succ->GetFrequency();
    if (succFreq >= removedFreq) {
      succ->SetFrequency(succFreq - removedFreq);
      succ->SetSuccFreq(0, succ->GetFrequency());
    }
  }
  DEBUG_LOG() << "Merge Uncond BB" << LOG_BBID(succ) << " to its pred BB" << LOG_BBID(pred) << ": BB" << LOG_BBID(pred)
              << "->BB" << LOG_BBID(succ) << "(merged)->BB" << LOG_BBID(newTarget) << "\n";
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
  (void)BranchBB2UncondBB(*pred);
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
bool OptimizeBB::OptimizeUncondBB() {
  CHECK_CURR_BB();
  ONLY_FOR_MEIR();
  if (currBB->GetAttributes(kBBAttrIsEntry) || currBB->IsMeStmtEmpty()) {
    return false;
  }
  // BB has only one stmt(i.e. unconditional goto stmt)
  if (!HasOnlyMeGotoStmt(*currBB)) {
    return false;
  }
  // jump to itself
  if (currBB->GetSucc(0) == currBB) {
    return false;
  }
  // wont exit BB and has an edge to commonExit, if we merge it to pred and delete it, the egde will be cut off
  if (currBB->GetSucc().size() == 2) { // 2 succ : first is gotoTarget, second is edge to commonExit
    ASSERT(currBB->GetAttributes(kBBAttrWontExit),
           "[FUNC: %s]GotoBB%d is not wontexitBB, but has two succ", funcName.c_str(), LOG_BBID(currBB));
    return false;
  }
  bool changed = false;
  // try to move pred from currBB to newTarget
  for (size_t i = 0; i < currBB->GetPred().size();) {
    BB *pred = currBB->GetPred(i);
    if (pred == nullptr || pred->GetLastMe() == nullptr || pred->GetPred().empty()) {
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

bool OptimizeBB::OptimizeFallthruBB() {
  CHECK_CURR_BB();
  ONLY_FOR_MEIR();
  if (MeSplitCEdge::IsCriticalEdgeBB(*currBB) || currBB->IsPredBB(*cfg->GetCommonExitBB())) {
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
  if (!HasOnlyMeGotoStmt(*succ)) {
    return false;
  }
  // BB has only one pred/succ, skip, because MergeDistinctBBPair will deal with this case
  // note: gotoBB may have two succ, the second succ is inserted by cfg wont exit analysis
  if (succ->GetPred().size() == 1 || succ->GetSucc().size() == 1) {
    return false;
  }
  return MergeGotoBBToPred(succ, currBB);
}

bool OptimizeBB::OptimizeSwitchBB() {
  CHECK_CURR_BB();
  ONLY_FOR_MEIR();
  auto *swStmt = static_cast<SwitchMeStmt*>(currBB->GetLastMe());
  CHECK_FATAL(swStmt, "swStmt is nullptr!");
  if (swStmt->GetOpnd()->GetMeOp() != kMeOpConst) {
    return false;
  }
  auto *swConstExpr = static_cast<ConstMeExpr*>(swStmt->GetOpnd());
  MIRConst *swConstVal = swConstExpr->GetConstVal();
  ASSERT(swConstVal->GetKind() == kConstInt, "[FUNC: %s]switch is only legal for integer val", funcName.c_str());
  int64 val = static_cast<MIRIntConst*>(swConstVal)->GetExtValue();
  BB *survivor = currBB->GetSucc(0);  // init as default BB
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

MeExpr *OptimizeBB::TryToSimplifyCombinedCond(const MeExpr &expr) {
  Opcode op = expr.GetOp();
  if (op != OP_land && op != OP_lior) {
    return nullptr;
  }
  MeExpr *expr1 = expr.GetOpnd(0);
  MeExpr *expr2 = expr.GetOpnd(1);
  // we can only deal with "and/or(cmpop1(sameExpr, constExpr1/scalarExpr), cmpop2(sameExpr, constExpr2/scalarExpr))"
  if (expr1->GetOpnd(0) != expr2->GetOpnd(0)) {
    return nullptr;
  }
  auto vr1 = GetVRForSimpleCmpExpr(*expr1);
  auto vr2 = GetVRForSimpleCmpExpr(*expr2);
  if (vr1.get() == nullptr || vr2.get() == nullptr) {
    return nullptr;
  }
  auto vrRes = MergeVR(*vr1, *vr2, op == OP_land);
  MeExpr *resExpr = GetCmpExprFromVR(vrRes.get(), *expr1->GetOpnd(0), irmap);
  if (resExpr != nullptr) {
    resExpr->SetPtyp(expr.GetPrimType());
    if (!IsCompareHasReverseOp(resExpr->GetOp())) {
      return nullptr;
    }
  }
  return resExpr;
}

// pattern is like:
//       pred(condBB)
//       /         \
//      /     succ(condBB)
//     /     /       \
//    commonBB       exitBB
// note: pred->commonBB and succ->commonBB are critical edge, they may be cut by an empty bb before
bool OptimizeBB::FoldBranchToCommonDest(BB *pred, BB *succ) {
  if (!HasOnlyMeCondGotoStmt(*succ)) {
    // not a simple condBB
    return false;
  }
  // try to simplify realSucc first, if all successors of realSucc is the same, no need to check the condition
  if (BranchBB2UncondBB(*succ)) {
    return true;
  }
  if (succ->GetPred().size() != 1) {
    return false;
  }
  // Check for pattern : predBB branches to succ, and one of succ's successor(common BB)
  BB *commonBB = GetCommonDest(pred, succ);
  if (commonBB == nullptr) {
    return false;
  }
  // we have found a pattern
  auto *succCondBr = static_cast<CondGotoMeStmt*>(succ->GetLastMe());
  CHECK_FATAL(succCondBr, "succCondBr is nullptr!");
  MeExpr *succCond = succCondBr->GetOpnd();
  auto *predCondBr = static_cast<CondGotoMeStmt*>(pred->GetLastMe());
  CHECK_FATAL(predCondBr, "predCondBr is nullptr!");
  MeExpr *predCond = predCondBr->GetOpnd();
  // Check for safety
  if (!IsSafeToMergeCond(predCond, succCond)) {
    DEBUG_LOG() << "Abort Merging Two CondBB : Condition in successor BB" << LOG_BBID(succ) << " is not safe\n";
    return false;
  }
  if (!IsProfitableToMergeCond(predCond, succCond)) {
    DEBUG_LOG() << "Abort Merging Two CondBB : Condition in successor BB" << LOG_BBID(succ)
                << " is not simple enough and not profitable\n";
    return false;
  }
  // Start trying to merge two condition together if possible
  auto ptfBrPair = GetTrueFalseBrPair(pred);  // pred's true and false branches
  auto stfBrPair = GetTrueFalseBrPair(succ);  // succ's true and false branches
  Opcode combinedCondOp = OP_undef;
  bool invertSuccCond = false;  // invert second condition, e.g. (cond1 && !cond2)
  // all cases are listed as follow:
  //  | case | predBB -> common | succBB -> common | invertSuccCond | or/and |
  //  | ---- | ---------------- | ---------------- | -------------- | ------ |
  //  | 1    | true             | true             |                | or     |
  //  | 2    | false            | false            |                | and    |
  //  | 3    | true             | false            | true           | or     |
  //  | 4    | false            | true             | true           | and    |
  // pred's false branch to succ, so true branch to common
  bool isPredTrueToCommon = (FindFirstRealSucc(ptfBrPair.second) == succ);
  bool isSuccTrueToCommon = (FindFirstRealSucc(stfBrPair.first) == commonBB);
  if (isPredTrueToCommon && isSuccTrueToCommon) {  // case 1
    invertSuccCond = false;
    combinedCondOp = OP_lior;
  } else if (!isPredTrueToCommon && !isSuccTrueToCommon) {  // case 2
    invertSuccCond = false;
    combinedCondOp = OP_land;
  } else if (isPredTrueToCommon && !isSuccTrueToCommon) {  // case 3
    invertSuccCond = true;
    combinedCondOp = OP_lior;
  } else if (!isPredTrueToCommon && isSuccTrueToCommon) {  // case 4
    invertSuccCond = true;
    combinedCondOp = OP_land;
  } else {
    CHECK_FATAL(false, "[FUNC: %s] pred and succ have no common dest", f.GetName().c_str());
  }
  if (invertSuccCond) {
    succCond = GetInvertCond(irmap, succCond);
  }
  MeExpr *combinedCond = irmap->CreateMeExprBinary(combinedCondOp, PTY_u1, *predCond, *succCond);
  // try to simplify combinedCond, if failed, not combine condition
  combinedCond = TryToSimplifyCombinedCond(*combinedCond);
  if (combinedCond == nullptr) {
    // work to do : after enhance instruction select, rm this restriction
    return false;
  }
  // eliminate empty bb between pred and succ.
  EliminateEmptyConnectingBB(pred, isPredTrueToCommon ? ptfBrPair.second : ptfBrPair.first, succ, *cfg);
  DEBUG_LOG() << "CondBB Group To Merge : \n  {\n"
              << "    BB" << LOG_BBID(pred) << " succ : (BB" << LOG_BBID(pred->GetSucc(0)) << ", BB"
              << LOG_BBID(pred->GetSucc(1)) << ")\n"
              << "    BB" << LOG_BBID(succ) << " succ : (BB" << LOG_BBID(succ->GetSucc(0)) << ", BB"
              << LOG_BBID(succ->GetSucc(1)) << ")\n  }\n";
  predCondBr->SetOpnd(0, combinedCond);
  // remove succ and update cfg
  BB *exitBB = isSuccTrueToCommon ? stfBrPair.second : stfBrPair.first;
  pred->ReplaceSucc(succ, exitBB);
  exitBB->RemovePred(*succ, false);  // not update phi here, because its phi opnd is the same as before.
  // we should eliminate edge from succ to commonBB
  BB *toEliminateBB = isSuccTrueToCommon ? stfBrPair.first : stfBrPair.second;
  EliminateEmptyConnectingBB(succ, toEliminateBB, commonBB /* stop here */, *cfg);
  succ->RemoveSucc(*commonBB);
  // Update target label
  if (predCondBr->GetOffset() != pred->GetSucc(1)->GetBBLabel()) {
    LabelIdx label = f.GetOrCreateBBLabel(*pred->GetSucc(1));
    predCondBr->SetOffset(label);
  }
  DEBUG_LOG() << "Delete CondBB BB" << LOG_BBID(succ) << " after merged to CondBB BB" << LOG_BBID(pred) << "\n";
  DeleteBB(succ);
  SetBBRunAgain();
  return true;
}

// pattern is like:
//       curr(condBB)
//       /         \
//      /     succ(condBB)
//     /     /       \
//    commonBB       exitBB
// note: curr->commonBB and succ->commonBB are critical edge, they may be cut by an empty bb before
bool OptimizeBB::FoldBranchToCommonDest() {
  CHECK_CURR_BB();
  if (currBB->GetKind() != kBBCondGoto) {
    return false;
  }
  bool change = false;
  for (size_t i = 0; i < currBB->GetSucc().size(); ++i) {
    BB *realSucc = FindFirstRealSucc(currBB->GetSucc(i));
    if (realSucc == nullptr) {
      continue;
    }
    change |= FoldBranchToCommonDest(currBB, realSucc);
  }
  return change;
}

bool OptimizeBB::OptBBOnce() {
  CHECK_CURR_BB();
  DEBUG_LOG() << "Try to optimize BB" << LOG_BBID(currBB) << "...\n";
  // bb frequency may be updated, make bb frequency and succs frequency consistent
  // skip deadBB
  if (cfg->UpdateCFGFreq() && (!currBB->GetPred().empty()) && (currBB->GetUniquePred() != currBB)) {
    currBB->UpdateEdgeFreqs(false);
  }
  bool everChanged = false;
  // eliminate dead BB :
  // 1.BB has no pred(expect then entry block)
  // 2.BB has only itself as pred
  if (EliminateDeadBB()) {
    return true;
  }

  // chang condition branch to unconditon branch if possible
  // 1.condition is a constant
  // 2.all branches of condition branch is the same BB
  everChanged |= OptimizeCondBB2UnCond();

  // disconnect predBB and currBB if predBB must cause error(e.g. null ptr deref)
  // If a expr is always cause error in predBB, predBB will never reach currBB
  if (RemoveSuccFromNoReturnBB()) {
    // all succ will be removed from currBB, no need to run on this BB.
    return true;
  }

  // merge currBB to predBB if currBB has only one predBB and predBB has only one succBB
  everChanged |= MergeDistinctBBPair();

  // Eliminate redundant philist in bb which has only one pred
  (void)EliminateRedundantPhiList();

  auto optimizer = CreateProductFunction<OptBBFatory>(currBB->GetKind());
  if (optimizer != nullptr) {
    everChanged |= optimizer(this);
  }
  return everChanged;
}

void OptimizeBB::InitBBOptFactory() {
  RegisterFactoryFunction<OptBBFatory>(kBBCondGoto, &OptimizeBB::OptimizeCondBB);
  RegisterFactoryFunction<OptBBFatory>(kBBGoto, &OptimizeBB::OptimizeUncondBB);
  RegisterFactoryFunction<OptBBFatory>(kBBFallthru, &OptimizeBB::OptimizeFallthruBB);
  RegisterFactoryFunction<OptBBFatory>(kBBSwitch, &OptimizeBB::OptimizeSwitchBB);
}

// run on each BB until no more change for currBB
bool OptimizeBB::OptBBIteratively() {
  bool changed = false;
  do {
    repeatOpt = false;
    // optimize currBB only once, if we should reopt on this BB, repeatOpt will be set by optimization
    changed |= OptBBOnce();
  } while (repeatOpt);
  return changed;
}

// For function Level optimization
class OptimizeFuntionCFG {
 public:
  OptimizeFuntionCFG(maple::MeFunction &func, std::map<OStIdx, std::unique_ptr<std::set<BBId>>> *candidates)
      : f(func),
        cands(candidates) {}

  bool OptimizeOnFunc();

 private:
  MeFunction &f;

  std::map<OStIdx, std::unique_ptr<std::set<BBId>>> *cands = nullptr;  // candidates ost need to update ssa
  bool OptimizeFuncCFGIteratively();
  bool OptimizeCFGForBB(BB *currBB);
};

bool OptimizeFuntionCFG::OptimizeCFGForBB(BB *currBB) {
  OptimizeBB bbOptimizer(currBB, f, cands);
  bbOptimizer.InitBBOptFactory();
  return bbOptimizer.OptBBIteratively();
}
// run until no changes happened
bool OptimizeFuntionCFG::OptimizeFuncCFGIteratively() {
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
      changed |= OptimizeCFGForBB(currBB);
    }
    everChanged |= changed;
  }
  return everChanged;
}

bool OptimizeFuntionCFG::OptimizeOnFunc() {
  bool everChanged = UnreachBBAnalysis(f);
  everChanged |= OptimizeFuncCFGIteratively();
  if (!everChanged) {
    return false;
  }
  // OptimizeFuncCFGIteratively may generate unreachable BB.
  // So UnreachBBAnalysis should be called to check for and
  // remove dead BB. And UnreachBBAnalysis may also generate
  // other optimize opportunity for OptimizeFuncCFGIteratively.
  // Hench we should iterate between these two optimizations.
  // Here, we call UnreachBBAnalysis first to avoid running
  // OptimizeFuncCFGIteratively for no changed situation.
  if (!UnreachBBAnalysis(f)) {
    return true;
  }
  do {
    everChanged = OptimizeFuncCFGIteratively();
    everChanged |= UnreachBBAnalysis(f);
  } while (everChanged);
  return true;
}

// An Interface to optimizing cfg for func with MeIR (INSTEAD OF MPLIR),
// so that if irmap is not built, this interface will do nothing.
// ssaCand, mp, ma are used to collect ost whose version need to be updated for ssa-updater
bool OptimizeMeFuncCFG(maple::MeFunction &f, std::map<OStIdx, std::unique_ptr<std::set<BBId>>> *ssaCand = nullptr) {
  funcName = f.GetName();
  if (SkipOptimizeCFG(f)) {
    DEBUG_LOG() << "Skip OptimizeBB phase because of igotoBB\n";
    return false;
  }
  DEBUG_LOG() << "Start Optimizing Function : " << f.GetName() << "\n";
  if (debug) {
    f.GetCfg()->DumpToFile("Before_" + phaseName);
  }
  uint32 bbNumBefore = f.GetCfg()->ValidBBNum();
  // optimization entry
  bool change = OptimizeFuntionCFG(f, ssaCand).OptimizeOnFunc();
  if (change) {
    f.GetCfg()->WontExitAnalysis();
    if (debug) {
      uint32 bbNumAfter = f.GetCfg()->ValidBBNum();
      if (bbNumBefore != bbNumAfter) {
        DEBUG_LOG() << "BBs' number " << (bbNumBefore > bbNumAfter ? "reduce" : "increase") << " from " << bbNumBefore
                    << " to " << bbNumAfter << "\n";
      } else {
        DEBUG_LOG() << "BBs' number keep the same as before (num = " << bbNumAfter << ")\n";
      }
      f.GetCfg()->DumpToFile("After_" + phaseName);
      f.Dump(f.GetIRMap() == nullptr);
    }
  }
  return change;
}

// phase for MPLIR, without SSA info
void MEOptimizeCFGNoSSA::GetAnalysisDependence(maple::AnalysisDep &aDep) const {
  aDep.AddRequired<MEMeCfg>();
  aDep.SetPreservedAll();
}

bool MEOptimizeCFGNoSSA::PhaseRun(MeFunction &f) {
  debug = DEBUGFUNC_NEWPM(f);
  phaseName = PhaseName();
  bool change = OptimizeMeFuncCFG(f, nullptr);
  if (change && f.GetCfg()->DumpIRProfileFile()) {
    f.GetCfg()->DumpToFile("after-OptimizeCFGNOSSA", false, f.GetCfg()->UpdateCFGFreq());
  }
  return change;
}

// phase for MEIR, should maintain ssa info and split critical edge
void MEOptimizeCFG::GetAnalysisDependence(maple::AnalysisDep &aDep) const {
  aDep.AddRequired<MEDominance>();
  aDep.AddRequired<MEIRMapBuild>();
  aDep.PreservedAllExcept<MELoopAnalysis>();
}

bool MEOptimizeCFG::PhaseRun(maple::MeFunction &f) {
  debug = DEBUGFUNC_NEWPM(f);
  std::map<OStIdx, std::unique_ptr<std::set<BBId>>> cands((std::less<OStIdx>()));
  phaseName = PhaseName();
  // optimization entry
  bool change = OptimizeMeFuncCFG(f, &cands);
  if (change) {
    // split critical edges
    (void)MeSplitCEdge(debug).SplitCriticalEdgeForMeFunc(f);
    FORCE_INVALID(MEDominance, f);
    Dominance *dom = FORCE_EXEC(MEDominance)->GetDomResult();
    if (!cands.empty()) {
      MeSSAUpdate ssaUpdate(f, *f.GetMeSSATab(), *dom, cands);
      ssaUpdate.Run();
    }
    if (f.GetCfg()->DumpIRProfileFile()) {
      f.GetCfg()->DumpToFile("after-OptimizeCFG", false, f.GetCfg()->UpdateCFGFreq());
    }
  }
  return change;
}
}  // namespace maple
