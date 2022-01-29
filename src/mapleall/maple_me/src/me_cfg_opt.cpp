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

#include "me_cfg_opt.h"
#include "me_bb_layout.h"
#include "me_irmap_build.h"

// We find the following pattern in JAVA code:
// Examples: if (empty) { a = 1; } else { a = 3; }
//
// It will generate the following IR in mpl file:
// brfalse @label1 (dread u1 %Reg4_Z)
// dassign %Reg1_I 0 (cvt i32 i8 (constval i8 1))
// goto @label2
// @label1   dassign %Reg1_I 0 (cvt i32 i8 (constval i8 3))
//
// The mecfgopt phase overwrite the upper pattern with following code:
// regassign i32 %26 (select i32 (
//    select u1 (
//      ne u1 i32 (regread i32 %1, constval i32 0),
//      constval u1 0,
//      constval u1 1),
//    constval i32 1,
//    constval i32 3))
namespace maple {
bool MeCfgOpt::IsExpensiveOp(Opcode op) {
  switch (op) {
    case OP_abs:
    case OP_bnot:
    case OP_lnot:
    case OP_sqrt:
    case OP_neg:
      return false;
    case OP_recip:
    case OP_div:
    case OP_rem:
    case OP_alloca:
    case OP_malloc:
    case OP_gcmalloc:
    case OP_gcmallocjarray:
      return true;
    case OP_ceil:
    case OP_cvt:
    case OP_floor:
    case OP_retype:
    case OP_round:
    case OP_trunc:
      return false;
    case OP_sext:
    case OP_zext:
    case OP_extractbits:
      return false;
    case OP_iaddrof:
    case OP_iread:
      return true;
    case OP_ireadoff:
      return true;
    case OP_ireadfpoff:
      return true;
    case OP_add:
    case OP_ashr:
    case OP_band:
    case OP_bior:
    case OP_bxor:
    case OP_cand:
    case OP_cior:
    case OP_land:
    case OP_lior:
    case OP_lshr:
    case OP_max:
    case OP_min:
    case OP_mul:
    case OP_shl:
    case OP_sub:
      return false;
    case OP_CG_array_elem_add:
      return true;
    case OP_eq:
    case OP_ne:
    case OP_ge:
    case OP_gt:
    case OP_le:
    case OP_lt:
    case OP_cmp:
    case OP_cmpl:
    case OP_cmpg:
      return false;
    case OP_depositbits:
      return true;
    case OP_select:
      return false;
    case OP_intrinsicop:
    case OP_intrinsicopwithtype:
      return true;
    case OP_constval:
    case OP_conststr:
    case OP_conststr16:
      return false;
    case OP_sizeoftype:
    case OP_fieldsdist:
      return false;
    case OP_array:
      return true;
    case OP_addrof:
    case OP_dread:
    case OP_regread:
    case OP_addroffunc:
    case OP_addroflabel:
      return false;
    // statement nodes start here
    default:
      CHECK_FATAL(false, "should be an expression or NYI");
  }
}

const MeStmt *MeCfgOpt::GetCondBrStmtFromBB(const BB &bb) const {
  CHECK_FATAL(bb.GetKind() == kBBCondGoto, "must be cond goto");
  const MeStmt *meStmt = to_ptr(bb.GetMeStmts().rbegin());
  if (meStmt->IsCondBr()) {
    return meStmt;
  }
  CHECK_FATAL(meStmt->GetOp() == OP_comment, "NYI");
  return nullptr;
}

// if the bb contains only one stmt other than comment, return that stmt
// otherwise return nullptr
MeStmt *MeCfgOpt::GetTheOnlyMeStmtFromBB(BB &bb) const {
  MeStmt *noCommentStmt = nullptr;
  for (auto &meStmt : bb.GetMeStmts()) {
    if (meStmt.GetOp() == OP_comment) {
      continue;
    }
    if (noCommentStmt != nullptr) {
      return nullptr;
    }
    noCommentStmt = &meStmt;
  }
  return noCommentStmt;
}

// if the bb contains only one stmt other than comment and goto, return that stmt
// otherwise return nullptr
MeStmt *MeCfgOpt::GetTheOnlyMeStmtWithGotoFromBB(BB &bb) const {
  constexpr uint32 stmtCount = 2;
  MeStmt *arrayStmt[stmtCount];
  uint32 index = 0;
  for (auto &meStmt : bb.GetMeStmts()) {
    if (meStmt.GetOp() == OP_comment) {
      continue;
    }
    if (index >= stmtCount) {
      return nullptr;
    }
    arrayStmt[index++] = &meStmt;
  }
  if (index == stmtCount && arrayStmt[0]->GetOp() != OP_comment && arrayStmt[1]->GetOp() == OP_goto) {
    return arrayStmt[0];
  }
  return nullptr;
}

bool MeCfgOpt::IsOk2Select(const MeExpr &expr0, const MeExpr &expr1) const {
  if (expr0.GetExprID() == expr1.GetExprID()) {
    return true;
  }
  std::set<int32> expensiveOps1;
  std::set<int32> expensiveOps2;
  if (CollectExpensiveOps(expr0, expensiveOps1)) {
    return false;
  }
  if (CollectExpensiveOps(expr1, expensiveOps2)) {
    return false;
  }
  if (expensiveOps1.size() != expensiveOps2.size()) {
    return false;
  }
  for (auto it = expensiveOps1.begin(); it != expensiveOps1.end(); ++it) {
    int32 expr1Id = *it;
    if (expensiveOps2.find(expr1Id) == expensiveOps2.end()) {
      return false;
    }
  }
  return true;
}

bool MeCfgOpt::HasFloatCmp(const MeExpr &meExpr) const {
  Opcode op = meExpr.GetOp();
  if ((op == OP_cmpl || op == OP_cmpg || op == OP_cmp) &&
      IsPrimitiveFloat(static_cast<const OpMeExpr&>(meExpr).GetOpndType())) {
    return true;
  }
  for (size_t i = 0; i < meExpr.GetNumOpnds(); ++i) {
    if (HasFloatCmp(*meExpr.GetOpnd(i))) {
      return true;
    }
  }
  return false;
}

bool MeCfgOpt::PreCheck(const MeCFG &cfg) const {
  auto eIt = cfg.valid_end();
  for (auto bIt = cfg.valid_begin(); bIt != eIt; ++bIt) {
    auto *bb = *bIt;
    if (bb->GetAttributes(kBBAttrIsTry) && bb->GetBBLabel() != 0) {
      for (size_t i = 0; i < bb->GetPred().size(); ++i) {
        BB *predBB = bb->GetPred(i);
        if (!predBB->GetAttributes(kBBAttrIsTry)) {
          // comes from outside of try. give up folding select.
          return false;
        }
      }
    }
    for (auto &stmt : bb->GetMeStmts()) {
      for (size_t i = 0; i < stmt.NumMeStmtOpnds(); ++i) {
        MeExpr *meExpr = stmt.GetOpnd(i);
        if (HasFloatCmp(*meExpr)) {
          return false;
        }
      }
    }
  }
  return true;
}

bool MeCfgOpt::Run(MeCFG &cfg) {
  if (!PreCheck(cfg)) {
    return false;
  }
  bool isCfgChanged = false;
  auto eIt = cfg.valid_end();
  for (auto bIt = cfg.valid_begin(); bIt != eIt; ++bIt) {
    auto *bb = *bIt;
    constexpr uint32 numOfSuccs = 2;
    if (bb->GetKind() == kBBCondGoto && bb->GetSucc().size() == numOfSuccs) {
      const MeStmt *condMeStmt = GetCondBrStmtFromBB(*bb);
      if (condMeStmt == nullptr || !condMeStmt->IsCondBr()) {
        continue;
      }
      BB *bbLeft = bb->GetSucc(0);
      BB *bbRight = bb->GetSucc(1);
      if (bbLeft->GetSucc().size() != 1 || bbLeft->GetPred().size() != 1 ||
          bbRight->GetPred().size() != 1 || bbRight->GetSucc().size() != 1) {
        continue;
      }
      const auto *condGotoMeStmt = static_cast<const CondGotoMeStmt*>(condMeStmt);
      if (bbLeft->GetBBLabel() && bbLeft->GetBBLabel() == condGotoMeStmt->GetOffset()) {
        BB *tmpBB = bbLeft;
        bbLeft = bbRight;
        bbRight = tmpBB;
      }
      if (bbLeft->GetKind() != kBBGoto || bbLeft->GetPred(0)->GetBBId() != bb->GetBBId() ||
          bbRight->GetKind() != kBBFallthru || bbRight->GetPred(0)->GetBBId() != bb->GetBBId() ||
          bbLeft->GetSucc(0) != bbRight->GetSucc(0)) {
        continue;
      }
      MeStmt *assStmtLeft = GetTheOnlyMeStmtWithGotoFromBB(*bbLeft);
      if (!(assStmtLeft != nullptr && (assStmtLeft->GetOp() == OP_iassign || assStmtLeft->GetOp() == OP_dassign))) {
        continue;
      }
      MeStmt *assStmtRight = GetTheOnlyMeStmtFromBB(*bbRight);
      if (!(assStmtRight != nullptr && (assStmtRight->GetOp() == OP_iassign || assStmtRight->GetOp() == OP_dassign) &&
            assStmtRight->GetOp() == assStmtLeft->GetOp())) {
        continue;
      }
      bool isTrueBr = (condMeStmt->GetOp() == OP_brtrue);
      bool isIassign = (assStmtLeft->GetOp() == OP_iassign);
      MeExpr *selectOpLeft = nullptr;
      MeExpr *selectOpRight = nullptr;
      if (isIassign) {
        auto *iassLeft = static_cast<IassignMeStmt*>(assStmtLeft);
        auto *iassRight = static_cast<IassignMeStmt*>(assStmtRight);
        if (iassLeft->GetLHSVal()->GetBase() != iassRight->GetLHSVal()->GetBase() ||
            iassLeft->GetLHSVal()->GetFieldID() != iassRight->GetLHSVal()->GetFieldID()) {
          continue;
        }
        // patter match
        selectOpLeft = iassLeft->GetRHS();
        selectOpRight = iassRight->GetRHS();
      } else {
        auto *dassLeft = static_cast<DassignMeStmt*>(assStmtLeft);
        auto *dassRight = static_cast<DassignMeStmt*>(assStmtRight);
        if (!dassLeft->GetLHS()->IsUseSameSymbol(*dassRight->GetLHS())) {
          continue;
        }
        selectOpLeft = dassLeft->GetRHS();
        selectOpRight = dassRight->GetRHS();
      }
      if (!IsOk2Select(*selectOpLeft, *selectOpRight) ||
          (selectOpLeft->GetPrimType() == PTY_ref && MeOption::rcLowering)) {
        continue;
      }
      MeExpr *selectMeExpr = meIrMap->CreateMeExprSelect(
          selectOpLeft->GetPrimType(), *condGotoMeStmt->GetOpnd(),
          *(isTrueBr ? selectOpRight : selectOpLeft),
          *(isTrueBr ? selectOpLeft : selectOpRight));
      if (isIassign) {
        // use bb as the new bb and put new iassign there
        static_cast<IassignMeStmt*>(assStmtLeft)->SetRHS(selectMeExpr);
      } else {
        static_cast<DassignMeStmt*>(assStmtLeft)->SetRHS(selectMeExpr);
      }
      bb->ReplaceMeStmt(condMeStmt, assStmtLeft);
      // update the preds and succs
      bb->RemoveAllSucc();
      BB *succBB = bbLeft->GetSucc(0);
      succBB->RemovePred(*bbLeft);
      if (bbRight->IsPredBB(*succBB)) {
        succBB->RemovePred(*bbRight);
      }
      bb->AddSucc(*succBB);
      bb->SetKind(kBBFallthru);
      bb->GetSuccFreq().push_back(bb->GetFrequency());
      cfg.DeleteBasicBlock(*bbLeft);
      cfg.DeleteBasicBlock(*bbRight);
      isCfgChanged = true;
    }
  }
  return isCfgChanged;
}

bool CheckAnalysisResult(const MeFunction &func) {
  if (func.GetCfg()->NumBBs() > 0) {
    return func.HasLaidOut();
  }
  return false;
}

void EmitMapleIr(MeFunction &func) {
  CHECK_FATAL(func.HasLaidOut(), "Check bb layout phase.");
  auto layoutBBs = func.GetLaidOutBBs();
  CHECK_NULL_FATAL(func.GetIRMap());
  MIRFunction *mirFunction = func.GetMirFunc();

  mirFunction->ReleaseCodeMemory();
  mirFunction->SetMemPool(new ThreadLocalMemPool(memPoolCtrler, "IR from IRMap::Emit()"));
  mirFunction->SetBody(mirFunction->GetCodeMemPool()->New<BlockNode>());

  for (size_t k = 1; k < mirFunction->GetSymTab()->GetSymbolTableSize(); ++k) {
    MIRSymbol *sym = mirFunction->GetSymTab()->GetSymbolFromStIdx(k);
    if (sym->GetSKind() == kStVar) {
      sym->SetIsDeleted();
    }
  }
  func.GetIRMap()->SetNeedAnotherPass(true);
  for (BB *bb : layoutBBs) {
    ASSERT(bb != nullptr, "bb should not be nullptr");
    bb->EmitBB(*func.GetMeSSATab(), *func.GetMirFunc()->GetBody(), true);
  }
  func.ClearLayout();
}

bool FindLocalRefVarUses(const MeIRMap &irMap, const MeExpr &expr,
                         const MeStmt &meStmt, const VarMeExpr &var) {
  if (expr.GetMeOp() == kMeOpVar) {
    const auto &varMeExpr = static_cast<const VarMeExpr&>(expr);
    if (varMeExpr.GetOstIdx() == var.GetOstIdx()) {
      return true;
    }
  }
  for (size_t i = 0; i < expr.GetNumOpnds(); ++i) {
    if (FindLocalRefVarUses(irMap, *expr.GetOpnd(i), meStmt, var)) {
      return true;
    }
  }
  return false;
}

bool MePostCfgOpt(const MeFunction &func, const MeIRMap &irMap) {
  // check all basic blocks searching for split case
  for (BB *bb : func.GetCfg()->GetAllBBs()) {
    if (bb == nullptr || !(bb->GetAttributes(kBBAttrIsTry))) {
      continue;
    }
    // use and def in the same bb.
    for (MeStmt *stmt = to_ptr(bb->GetMeStmts().begin()); stmt != nullptr; stmt = stmt->GetNextMeStmt()) {
      // use and def the same var in one callassign statement
      if (stmt->GetOp() == OP_dassign) {  // skip identity assignments
        auto *dass = static_cast<DassignMeStmt*>(stmt);
        if (dass->GetLHS()->IsUseSameSymbol(*dass->GetRHS())) {
          continue;
        }
      }
      VarMeExpr *theLHS = nullptr;
      if (stmt->GetOp() == OP_dassign || kOpcodeInfo.IsCallAssigned(stmt->GetOp())) {
        theLHS = static_cast<VarMeExpr*>(stmt->GetLHSRef(false));
      }
      if (theLHS == nullptr) {
        continue;
      }
      MeStmt *nextStmt = stmt->GetNextMeStmt();
      for (MeStmt *stmtNew = to_ptr(bb->GetMeStmts().begin()); stmtNew != nullptr && stmtNew != nextStmt;
           stmtNew = stmtNew->GetNextMeStmt()) {
        // look for use occurrence of localrefvars
        for (size_t i = 0; i < stmtNew->NumMeStmtOpnds(); ++i) {
          ASSERT(stmtNew->GetOpnd(i) != nullptr, "null ptr check");
          if (FindLocalRefVarUses(irMap, *stmtNew->GetOpnd(i), *stmtNew, *theLHS)) {
            return true;
          }
        }
      }
    }
  }
  return false;
}

void MECfgOpt::GetAnalysisDependence(maple::AnalysisDep &aDep) const {
  aDep.AddRequired<MEIRMapBuild>();
}

/* change CFG */
bool MECfgOpt::PhaseRun(maple::MeFunction &f) {
  auto *irMap = GET_ANALYSIS(MEIRMapBuild, f);
  ASSERT_NOT_NULL(irMap);
  MeCfgOpt cfgOpt(irMap);
  auto cfgoptFunc = [](MeFunction &func)->void{
    if (CheckAnalysisResult(func)) {
      EmitMapleIr(func);
    }
    func.SetMeSSATab(nullptr);
    func.SetIRMap(nullptr);
  };
  if (cfgOpt.Run(*f.GetCfg())) {
    cfgoptFunc(f);
    return true;
  }

  if (MePostCfgOpt(f, *irMap)) {
    cfgoptFunc(f);
    return true;
  }
  return true;
}
}  // namespace maple
