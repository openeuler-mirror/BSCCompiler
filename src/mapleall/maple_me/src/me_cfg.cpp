/*
 * Copyright (c) [2019-2021] Huawei Technologies Co.,Ltd.All rights reserved.
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
#include "me_cfg.h"
#include <iostream>
#include <algorithm>
#include "bb.h"
#include "ssa_mir_nodes.h"
#include "me_irmap.h"
#include "mir_builder.h"

namespace {
constexpr int kFuncNameLenLimit = 80;
static bool CaseValOfSwitchIsSuccInt(const maple::CaseVector &switchTable) {
  ASSERT(!switchTable.empty(), "switch table is empty");
  size_t caseNum = switchTable.size();
  int val = switchTable[0].first;
  for (size_t id = 1; id < caseNum; id++) {
    val++;
    if (val != switchTable[id].first) {
      return false;
    }
  }
  return true;
}
}

namespace maple {
#define MATCH_STMT(stmt, kOpCode) do {                                           \
  while ((stmt) != nullptr && (stmt)->GetOpCode() == OP_comment) {               \
    (stmt) = (stmt)->GetNext();                                                  \
  }                                                                              \
  if ((stmt) == nullptr || (stmt)->GetOpCode() != (kOpCode)) {                   \
    return false;                                                                \
  }                                                                              \
} while (0) // END define
// determine if need to be replaced by assertnonnull
bool MeCFG::IfReplaceWithAssertNonNull(const BB &bb) const {
  const StmtNode *stmt = bb.GetStmtNodes().begin().d();
  constexpr const char npeTypeName[] = "Ljava_2Flang_2FNullPointerException_3B";
  GStrIdx npeGStrIdx = GlobalTables::GetStrTable().GetStrIdxFromName(npeTypeName);
  TyIdx npeTypeIdx = func.GetMIRModule().GetTypeNameTab()->GetTyIdxFromGStrIdx(npeGStrIdx);
  // match first stmt
  MATCH_STMT(stmt, OP_intrinsiccallwithtype);

  auto *cNode = static_cast<const IntrinsiccallNode*>(stmt);
  if (cNode->GetTyIdx() != npeTypeIdx) {
    return false;
  }
  stmt = stmt->GetNext();
  // match second stmt
  MATCH_STMT(stmt, OP_dassign);

  auto *dassignNode = static_cast<const DassignNode*>(stmt);
  if (dassignNode->GetRHS()->GetOpCode() != OP_gcmalloc) {
    return false;
  }
  auto *gcMallocNode = static_cast<GCMallocNode*>(dassignNode->GetRHS());
  if (gcMallocNode->GetTyIdx() != npeTypeIdx) {
    return false;
  }
  stmt = stmt->GetNext();
  // match third stmt
  MATCH_STMT(stmt, OP_callassigned);

  auto *callNode = static_cast<const CallNode*>(stmt);
  if (GlobalTables::GetFunctionTable().GetFunctionFromPuidx(callNode->GetPUIdx())->GetName() !=
      "Ljava_2Flang_2FNullPointerException_3B_7C_3Cinit_3E_7C_28_29V") {
    return false;
  }
  stmt = stmt->GetNext();
  MATCH_STMT(stmt, OP_throw);

  return true;
}

void MeCFG::ReplaceSwitchContainsOneCaseBranchWithBrtrue(maple::BB &bb, MapleVector<BB*> &exitBlocks) {
  StmtNode &lastStmt = bb.GetStmtNodes().back();
  ASSERT(lastStmt.GetOpCode() == OP_switch, "runtime check error");
  auto &switchStmt = static_cast<SwitchNode&>(lastStmt);
  auto &swithcTable = switchStmt.GetSwitchTable();
  if (!CaseValOfSwitchIsSuccInt(swithcTable)) {
    return;
  }
  LabelIdx defaultLabelIdx = switchStmt.GetDefaultLabel();
  int32 minCaseVal = swithcTable.front().first;
  int32 maxCaseVal = swithcTable.back().first;
  auto *baseNode = switchStmt.Opnd(0);

  auto &mirBuilder = func.GetMIRModule().GetMIRBuilder();
  auto *minCaseNode = mirBuilder->CreateIntConst(minCaseVal, PTY_i32);
  auto *ltNode = mirBuilder->CreateExprCompare(OP_lt, GetTypeFromTyIdx(TyIdx(PTY_u1)),
                                               GetTypeFromTyIdx(TyIdx(PTY_i32)), baseNode, minCaseNode);
  auto *condGoto = mirBuilder->CreateStmtCondGoto(ltNode, OP_brtrue, defaultLabelIdx);
  bb.ReplaceStmt(&switchStmt, condGoto);
  bb.SetKind(kBBCondGoto);

  auto *newBB = func.NewBasicBlock();
  auto *maxCaseNode = mirBuilder->CreateIntConst(maxCaseVal, PTY_i32);
  auto *gtNode = mirBuilder->CreateExprCompare(OP_gt, GetTypeFromTyIdx(TyIdx(PTY_u1)),
                                               GetTypeFromTyIdx(TyIdx(PTY_i32)), baseNode, maxCaseNode);
  condGoto = mirBuilder->CreateStmtCondGoto(gtNode, OP_brtrue, defaultLabelIdx);
  newBB->GetStmtNodes().push_back(condGoto);
  newBB->SetKind(kBBCondGoto);

  BB *defaultBB = func.GetLabelBBAt(defaultLabelIdx);
  ASSERT(defaultBB != nullptr, "null ptr check");
  while (!bb.GetSucc().empty()) {
    bb.RemoveSucc(*bb.GetSucc(0));
  }
  bb.AddSucc(*newBB);
  bb.AddSucc(*defaultBB);

  BB *caseBB = func.GetLabelBBAt(switchStmt.GetSwitchTable().front().second);
  ASSERT(caseBB != nullptr, "null ptr check");
  newBB->AddSucc(*caseBB);
  newBB->AddSucc(*defaultBB);

  if (bb.GetAttributes(kBBAttrIsTry)) {
    newBB->SetAttributes(kBBAttrIsTry);
    func.SetBBTryNodeMap(*newBB, *func.GetBBTryNodeMap().at(&bb));
    AddCatchHandlerForTryBB(bb, exitBlocks);
    AddCatchHandlerForTryBB(*newBB, exitBlocks);
  }
}

void MeCFG::AddCatchHandlerForTryBB(BB &bb, MapleVector<BB*> &exitBlocks) {
  if (!bb.GetAttributes(kBBAttrIsTry)) {
    return;
  }
  auto it = func.GetBBTryNodeMap().find(&bb);
  CHECK_FATAL(it != func.GetBBTryNodeMap().end(), "try bb without try");
  StmtNode *currTry = it->second;
  const auto *tryNode = static_cast<const TryNode*>(currTry);
  bool hasFinallyHandler = false;
  // add exception handler bb
  for (size_t j = 0; j < tryNode->GetOffsetsCount(); ++j) {
    LabelIdx labelIdx = tryNode->GetOffset(j);
    ASSERT(func.GetLabelBBIdMap().find(labelIdx) != func.GetLabelBBIdMap().end(), "runtime check error");
    BB *meBB = func.GetLabelBBAt(labelIdx);
    CHECK_FATAL(meBB != nullptr, "null ptr check");
    ASSERT(meBB->GetAttributes(kBBAttrIsCatch), "runtime check error");
    if (meBB->GetAttributes(kBBAttrIsJSFinally) || meBB->GetAttributes(kBBAttrIsCatch)) {
      hasFinallyHandler = true;
    }
    // avoid redundant succ
    if (!meBB->IsSuccBB(bb)) {
      bb.AddSucc(*meBB);
    }
  }
  // if try block don't have finally catch handler, add common_exit_bb as its succ
  if (!hasFinallyHandler) {
    if (!bb.GetAttributes(kBBAttrIsExit)) {
      bb.SetAttributes(kBBAttrIsExit);  // may exit
      exitBlocks.push_back(&bb);
    }
  } else if (func.GetMIRModule().IsJavaModule() && bb.GetAttributes(kBBAttrIsExit)) {
    // deal with throw bb, if throw bb in a tryblock and has finallyhandler
    auto &stmtNodes = bb.GetStmtNodes();
    if (!stmtNodes.empty() && stmtNodes.back().GetOpCode() == OP_throw) {
      bb.ClearAttributes(kBBAttrIsExit);
      ASSERT(&bb == exitBlocks.back(), "runtime check error");
      exitBlocks.pop_back();
    }
  }
}

void MeCFG::BuildMirCFG() {
  MapleVector<BB*> entryBlocks(func.GetAlloc().Adapter());
  MapleVector<BB*> exitBlocks(func.GetAlloc().Adapter());
  std::vector<BB*> switchBBsWithOneCaseBranch;
  auto eIt = func.valid_end();
  for (auto bIt = func.valid_begin(); bIt != eIt; ++bIt) {
    if (bIt == func.common_entry() || bIt == func.common_exit()) {
      continue;
    }
    BB *bb = *bIt;
    CHECK_FATAL(bb != nullptr, "null ptr check ");

    if (bb->GetAttributes(kBBAttrIsEntry)) {
      entryBlocks.push_back(bb);
    }
    if (bb->GetAttributes(kBBAttrIsExit)) {
      exitBlocks.push_back(bb);
    }
    switch (bb->GetKind()) {
      case kBBGoto: {
        StmtNode &lastStmt = bb->GetStmtNodes().back();
        if (lastStmt.GetOpCode() == OP_throw) {
          break;
        }
        ASSERT(lastStmt.GetOpCode() == OP_goto, "runtime check error");
        auto &gotoStmt = static_cast<GotoNode&>(lastStmt);
        LabelIdx lblIdx = gotoStmt.GetOffset();
        BB *meBB = func.GetLabelBBAt(lblIdx);
        bb->AddSucc(*meBB);
        break;
      }
      case kBBCondGoto: {
        StmtNode &lastStmt = bb->GetStmtNodes().back();
        ASSERT(lastStmt.IsCondBr(), "runtime check error");
        // succ[0] is fallthru, succ[1] is gotoBB
        auto rightNextBB = bIt;
        ++rightNextBB;
        CHECK_FATAL(rightNextBB != eIt, "null ptr check ");
        bb->AddSucc(**rightNextBB);
        // link goto
        auto &gotoStmt = static_cast<CondGotoNode&>(lastStmt);
        LabelIdx lblIdx = gotoStmt.GetOffset();
        BB *meBB = func.GetLabelBBAt(lblIdx);
        if (*rightNextBB == meBB) {
          constexpr char tmpBool[] = "tmpBool";
          auto *mirBuilder = func.GetMIRModule().GetMIRBuilder();
          MIRSymbol *st = mirBuilder->GetOrCreateLocalDecl(tmpBool, *GlobalTables::GetTypeTable().GetUInt1());
          auto *dassign = mirBuilder->CreateStmtDassign(st->GetStIdx(), 0, lastStmt.Opnd(0));
          bb->ReplaceStmt(&lastStmt, dassign);
          bb->SetKind(kBBFallthru);
        } else {
          bb->AddSucc(*meBB);
        }
        // can the gotostmt be replaced by assertnonnull ?
        if (IfReplaceWithAssertNonNull(*meBB)) {
          patternSet.insert(lblIdx);
        }
        break;
      }
      case kBBSwitch: {
        StmtNode &lastStmt = bb->GetStmtNodes().back();
        ASSERT(lastStmt.GetOpCode() == OP_switch, "runtime check error");
        auto &switchStmt = static_cast<SwitchNode&>(lastStmt);
        LabelIdx lblIdx = switchStmt.GetDefaultLabel();
        BB *mirBB = func.GetLabelBBAt(lblIdx);
        bb->AddSucc(*mirBB);
        std::set<LabelIdx> caseLabels;
        for (size_t j = 0; j < switchStmt.GetSwitchTable().size(); ++j) {
          lblIdx = switchStmt.GetCasePair(j).second;
          BB *meBB = func.GetLabelBBAt(lblIdx);
          (void)caseLabels.insert(lblIdx);
          // Avoid duplicate succs.
          auto it = std::find(bb->GetSucc().begin(), bb->GetSucc().end(), meBB);
          if (it == bb->GetSucc().end()) {
            bb->AddSucc(*meBB);
          }
        }
        if (bb->GetSucc().size() == 1) {
          bb->RemoveLastStmt();
          bb->SetKind(kBBFallthru);
        } else if (caseLabels.size() == 1) {
          switchBBsWithOneCaseBranch.push_back(bb);
        }
        break;
      }
      case kBBIgoto: {
        for (LabelIdx lidx : func.GetMirFunc()->GetLabelTab()->GetAddrTakenLabels()) {
          BB *mebb = func.GetLabelBBAt(lidx);
          bb->AddSucc(*mebb);
        }
        break;
      }
      case kBBReturn:
        break;
      default: {
        // fall through?
        auto rightNextBB = bIt;
        ++rightNextBB;
        if (rightNextBB != eIt) {
          bb->AddSucc(**rightNextBB);
        }
        break;
      }
    }
    // deal try blocks, add catch handler to try's succ. SwitchBB dealed individually
    if (bb->GetAttributes(kBBAttrIsTry)) {
      AddCatchHandlerForTryBB(*bb, exitBlocks);
    }
  }

  for (BB *switchBB : switchBBsWithOneCaseBranch) {
    ReplaceSwitchContainsOneCaseBranchWithBrtrue(*switchBB, exitBlocks);
  }
  // merge all blocks in entryBlocks
  for (BB *bb : entryBlocks) {
    func.GetCommonEntryBB()->AddEntry(*bb);
  }
  // merge all blocks in exitBlocks
  for (BB *bb : exitBlocks) {
    func.GetCommonExitBB()->AddExit(*bb);
  }
}

bool MeCFG::FindExprUse(const BaseNode &expr, StIdx stIdx) const {
  if (expr.GetOpCode() == OP_addrof || expr.GetOpCode() == OP_dread) {
    const auto &addofNode = static_cast<const AddrofNode&>(expr);
    return addofNode.GetStIdx() == stIdx;
  } else if (expr.GetOpCode() == OP_iread) {
    return FindExprUse(*expr.Opnd(0), stIdx);
  } else {
    for (size_t i = 0; i < expr.NumOpnds(); ++i) {
      if (FindExprUse(*expr.Opnd(i), stIdx)) {
        return true;
      }
    }
  }
  return false;
}

bool MeCFG::FindUse(const StmtNode &stmt, StIdx stIdx) const {
  Opcode opcode = stmt.GetOpCode();
  switch (opcode) {
    case OP_call:
    case OP_virtualcall:
    case OP_virtualicall:
    case OP_superclasscall:
    case OP_interfacecall:
    case OP_interfaceicall:
    case OP_customcall:
    case OP_polymorphiccall:
    case OP_icall:
    case OP_intrinsiccall:
    case OP_xintrinsiccall:
    case OP_intrinsiccallwithtype:
    case OP_callassigned:
    case OP_virtualcallassigned:
    case OP_virtualicallassigned:
    case OP_superclasscallassigned:
    case OP_interfacecallassigned:
    case OP_interfaceicallassigned:
    case OP_customcallassigned:
    case OP_polymorphiccallassigned:
    case OP_icallassigned:
    case OP_intrinsiccallassigned:
    case OP_xintrinsiccallassigned:
    case OP_intrinsiccallwithtypeassigned:
    case OP_syncenter:
    case OP_syncexit: {
      for (size_t i = 0; i < stmt.NumOpnds(); ++i) {
        BaseNode *argExpr = stmt.Opnd(i);
        if (FindExprUse(*argExpr, stIdx)) {
          return true;
        }
      }
      break;
    }
    case OP_dassign: {
      const auto &dNode = static_cast<const DassignNode&>(stmt);
      return FindExprUse(*dNode.GetRHS(), stIdx);
    }
    case OP_regassign: {
      const auto &rNode = static_cast<const RegassignNode&>(stmt);
      if (rNode.GetRegIdx() < 0) {
        return false;
      }
      return FindExprUse(*rNode.Opnd(0), stIdx);
    }
    case OP_iassign: {
      const auto &iNode = static_cast<const IassignNode&>(stmt);
      if (FindExprUse(*iNode.Opnd(0), stIdx)) {
        return true;
      } else {
        return FindExprUse(*iNode.GetRHS(), stIdx);
      }
    }
    case OP_assertnonnull:
    case OP_eval:
    case OP_free:
    case OP_switch: {
      BaseNode *argExpr = stmt.Opnd(0);
      return FindExprUse(*argExpr, stIdx);
    }
    default:
      break;
  }
  return false;
}

bool MeCFG::FindDef(const StmtNode &stmt, StIdx stIdx) const {
  if (stmt.GetOpCode() != OP_dassign && !kOpcodeInfo.IsCallAssigned(stmt.GetOpCode())) {
    return false;
  }
  if (stmt.GetOpCode() == OP_dassign) {
    const auto &dassStmt = static_cast<const DassignNode&>(stmt);
    return dassStmt.GetStIdx() == stIdx;
  }
  const auto &cNode = static_cast<const CallNode&>(stmt);
  const CallReturnVector &nRets = cNode.GetReturnVec();
  if (!nRets.empty()) {
    ASSERT(nRets.size() == 1, "Single Ret value for now.");
    StIdx idx = nRets[0].first;
    RegFieldPair regFieldPair = nRets[0].second;
    if (!regFieldPair.IsReg()) {
      return idx == stIdx;
    }
  }
  return false;
}

// Return true if there is no use or def of sym betweent from to to.
bool MeCFG::HasNoOccBetween(StmtNode &from, const StmtNode &to, StIdx stIdx) const {
  for (StmtNode *stmt = &from; stmt && stmt != &to; stmt = stmt->GetNext()) {
    if (FindUse(*stmt, stIdx) || FindDef(*stmt, stIdx)) {
      return false;
    }
  }
  return true;
}

// Fix the initially created CFG
void MeCFG::FixMirCFG() {
  auto eIt = func.valid_end();
  for (auto bIt = func.valid_begin(); bIt != eIt; ++bIt) {
    if (bIt == func.common_entry() || bIt == func.common_exit()) {
      continue;
    }
    auto *bb = *bIt;
    // Split bb in try if there are use -- def the same ref var.
    if (bb->GetAttributes(kBBAttrIsTry)) {
      // 1. Split callassigned/dassign stmt to two insns if it redefine a symbole and also use
      //    the same symbol as its operand.
      for (StmtNode *stmt = to_ptr(bb->GetStmtNodes().begin()); stmt != nullptr; stmt = stmt->GetNext()) {
        const MIRSymbol *sym = nullptr;
        if (kOpcodeInfo.IsCallAssigned(stmt->GetOpCode())) {
          auto *cNode = static_cast<CallNode*>(stmt);
          sym = cNode->GetCallReturnSymbol(func.GetMIRModule());
        } else if (stmt->GetOpCode() == OP_dassign) {
          auto *dassStmt = static_cast<DassignNode*>(stmt);
          // exclude the case a = b;
          if (dassStmt->GetRHS()->GetOpCode() == OP_dread) {
            continue;
          }
          sym = func.GetMirFunc()->GetLocalOrGlobalSymbol(dassStmt->GetStIdx());
        }
        if (sym == nullptr || sym->GetType()->GetPrimType() != PTY_ref || !sym->IsLocal()) {
          continue;
        }
        if (kOpcodeInfo.IsCallAssigned(stmt->GetOpCode()) || FindUse(*stmt, sym->GetStIdx())) {
          func.GetMirFunc()->IncTempCount();
          std::string tempStr = std::string("tempRet").append(std::to_string(func.GetMirFunc()->GetTempCount()));
          MIRBuilder *builder = func.GetMirFunc()->GetModule()->GetMIRBuilder();
          MIRSymbol *targetSt = builder->GetOrCreateLocalDecl(tempStr, *sym->GetType());
          targetSt->ResetIsDeleted();
          if (stmt->GetOpCode() == OP_dassign) {
            auto *rhs = static_cast<DassignNode*>(stmt)->GetRHS();
            StmtNode *dassign = builder->CreateStmtDassign(*targetSt, 0, rhs);
            bb->ReplaceStmt(stmt, dassign);
            stmt = dassign;
          } else {
            auto *cNode = static_cast<CallNode*>(stmt);
            CallReturnPair retPair = cNode->GetReturnPair(0);
            retPair.first = targetSt->GetStIdx();
            cNode->SetReturnPair(retPair, 0);
          }
          StmtNode *dassign = builder->CreateStmtDassign(*sym, 0, builder->CreateExprDread(*targetSt));
          if (stmt->GetNext() != nullptr) {
            bb->InsertStmtBefore(stmt->GetNext(), dassign);
          } else {
            ASSERT(stmt == &(bb->GetStmtNodes().back()), "just check");
            stmt->SetNext(dassign);
            dassign->SetPrev(stmt);
            bb->GetStmtNodes().update_back(dassign);
          }
        }
      }
    }
  }

  // 2. Split bb to two bbs if there are use and def the same ref var in bb.
  for (auto bIt = func.valid_begin(); bIt != eIt; ++bIt) {
    if (bIt == func.common_entry() || bIt == func.common_exit()) {
      continue;
    }
    auto *bb = *bIt;
    if (bb == nullptr) {
      continue;
    }
    if (bb->GetAttributes(kBBAttrIsTry)) {
      for (auto &splitPoint : bb->GetStmtNodes()) {
        const MIRSymbol *sym = nullptr;
        StmtNode *nextStmt = splitPoint.GetNext();
        if (kOpcodeInfo.IsCallAssigned(splitPoint.GetOpCode())) {
          auto *cNode = static_cast<CallNode*>(&splitPoint);
          sym = cNode->GetCallReturnSymbol(func.GetMIRModule());
        } else {
          if (nextStmt == nullptr ||
              (nextStmt->GetOpCode() != OP_dassign && !kOpcodeInfo.IsCallAssigned(nextStmt->GetOpCode()))) {
            continue;
          }
          if (nextStmt->GetOpCode() == OP_dassign) {
            auto *dassignStmt = static_cast<DassignNode*>(nextStmt);
            const StIdx stIdx = dassignStmt->GetStIdx();
            sym = func.GetMirFunc()->GetLocalOrGlobalSymbol(stIdx);
          } else {
            auto *cNode = static_cast<CallNode*>(nextStmt);
            sym = cNode->GetCallReturnSymbol(func.GetMIRModule());
          }
        }
        if (sym == nullptr || sym->GetType()->GetPrimType() != PTY_ref || !sym->IsLocal()) {
          continue;
        }
        if (!kOpcodeInfo.IsCallAssigned(splitPoint.GetOpCode()) &&
            HasNoOccBetween(*bb->GetStmtNodes().begin().d(), *nextStmt, sym->GetStIdx())) {
          continue;
        }
        BB &newBB = func.SplitBB(*bb, splitPoint);
        // because SplitBB will insert a bb, we need update bIt & eIt
        auto newBBIt = std::find(func.cbegin(), func.cend(), bb);
        bIt = build_filter_iterator(
            newBBIt, std::bind(FilterNullPtr<MapleVector<BB*>::const_iterator>, std::placeholders::_1, func.end()));
        eIt = func.valid_end();
        for (size_t si = 0; si < newBB.GetSucc().size(); ++si) {
          BB *sucBB = newBB.GetSucc(si);
          if (sucBB->GetAttributes(kBBAttrIsCatch)) {
            bb->AddSucc(*sucBB);
          }
        }
        break;
      }
    }
    // removing outgoing exception edge from normal return bb
    if (bb->GetKind() != kBBReturn || !bb->GetAttributes(kBBAttrIsTry) || bb->IsEmpty()) {
      continue;
    }
    // make sure this is a normal return bb
    // throw, retsub and retgoto are not allowed here
    if (bb->GetStmtNodes().back().GetOpCode() != OP_return) {
      continue;
    }
    // fast path
    StmtNode *stmt = bb->GetTheOnlyStmtNode();
    if (stmt != nullptr) {
      // simplify the cfg removing all succs of this bb
      for (size_t si = 0; si < bb->GetSucc().size(); ++si) {
        BB *sucBB = bb->GetSucc(si);
        if (sucBB->GetAttributes(kBBAttrIsCatch)) {
          sucBB->RemovePred(*bb);
          --si;
        }
      }
      continue;
    }
    // split this non-trivial bb
    StmtNode *splitPoint = bb->GetStmtNodes().back().GetPrev();
    while (splitPoint != nullptr && splitPoint->GetOpCode() == OP_comment) {
      splitPoint = splitPoint->GetPrev();
    }
    CHECK_FATAL(splitPoint != nullptr, "null ptr check");
    BB &newBB = func.SplitBB(*bb, *splitPoint);
    // because SplitBB will insert a bb, we need update bIt & eIt
    auto newBBIt = std::find(func.cbegin(), func.cend(), bb);
    bIt = build_filter_iterator(
        newBBIt, std::bind(FilterNullPtr<MapleVector<BB*>::const_iterator>, std::placeholders::_1, func.end()));
    eIt = func.valid_end();
    // redirect all succs of new bb to bb
    for (size_t si = 0; si < newBB.GetSucc().size(); ++si) {
      BB *sucBB = newBB.GetSucc(si);
      if (sucBB->GetAttributes(kBBAttrIsCatch)) {
        sucBB->ReplacePred(&newBB, bb);
        --si;
      }
    }
    if (bIt == eIt) {
      break;
    }
  }
}


// replace "if() throw NPE()" with assertnonnull
void MeCFG::ReplaceWithAssertnonnull() {
  constexpr char rnnTypeName[] =
      "Ljava_2Futil_2FObjects_3B_7CrequireNonNull_7C_28Ljava_2Flang_2FObject_3B_29Ljava_2Flang_2FObject_3B";
  if (func.GetName() == rnnTypeName) {
    return;
  }
  for (LabelIdx lblIdx : patternSet) {
    BB *bb = func.GetLabelBBAt(lblIdx);
    // if BB->pred_.size()==0, it won't enter this function
    for (size_t i = 0; i < bb->GetPred().size(); ++i) {
      BB *innerBB = bb->GetPred(i);
      if (innerBB->GetKind() == kBBCondGoto) {
        StmtNode &stmt = innerBB->GetStmtNodes().back();
        Opcode stmtOp = stmt.GetOpCode();
        ASSERT(stmt.IsCondBr(), "CondGoto BB with no condGoto stmt");
        auto &condGotoNode = static_cast<CondGotoNode&>(stmt);
        if ((stmtOp == OP_brtrue && condGotoNode.Opnd(0)->GetOpCode() != OP_eq) ||
            (stmtOp == OP_brfalse && condGotoNode.Opnd(0)->GetOpCode() != OP_ne)) {
          continue;
        }
        auto *cmpNode = static_cast<CompareNode*>(condGotoNode.Opnd(0));
        BaseNode *opnd = nullptr;
        if (cmpNode->GetOpndType() != PTY_ref && cmpNode->GetOpndType() != PTY_ptr) {
          continue;
        }
        if (cmpNode->GetBOpnd(0)->GetOpCode() == OP_constval) {
          auto *constNode = static_cast<ConstvalNode*>(cmpNode->GetBOpnd(0));
          if (!constNode->GetConstVal()->IsZero()) {
            continue;
          }
          opnd = cmpNode->GetBOpnd(1);
        } else if (cmpNode->GetBOpnd(1)->GetOpCode() == OP_constval) {
          auto *constNode = static_cast<ConstvalNode*>(cmpNode->GetBOpnd(1));
          if (!constNode->GetConstVal()->IsZero()) {
            continue;
          }
          opnd = cmpNode->GetBOpnd(0);
        }
        ASSERT(opnd != nullptr, "Compare with non-zero");
        UnaryStmtNode *nullCheck = func.GetMIRModule().GetMIRBuilder()->CreateStmtUnary(OP_assertnonnull, opnd);
        innerBB->ReplaceStmt(&stmt, nullCheck);
        innerBB->SetKind(kBBFallthru);
        innerBB->RemoveSucc(*bb);
        --i;
      }
    }
  }
}

bool MeCFG::IsStartTryBB(maple::BB &meBB) const {
  if (!meBB.GetAttributes(kBBAttrIsTry) || meBB.GetAttributes(kBBAttrIsTryEnd)) {
    return false;
  }
  return (!meBB.GetStmtNodes().empty() && meBB.GetStmtNodes().front().GetOpCode() == OP_try);
}

void MeCFG::FixTryBB(maple::BB &startBB, maple::BB &nextBB) {
  startBB.RemoveAllPred();
  for (size_t i = 0; i < nextBB.GetPred().size(); ++i) {
    nextBB.GetPred(i)->ReplaceSucc(&nextBB, &startBB);
  }
  ASSERT(nextBB.GetPred().empty(), "pred of nextBB should be empty");
  startBB.RemoveAllSucc();
  startBB.AddSucc(nextBB);
}

// analyse the CFG to find the BBs that are not reachable from function entries
// and delete them
bool MeCFG::UnreachCodeAnalysis(bool updatePhi) {
  std::vector<bool> visitedBBs(func.NumBBs(), false);
  func.GetCommonEntryBB()->FindReachableBBs(visitedBBs);
  // delete the unreached bb
  bool cfgChanged = false;
  auto eIt = func.valid_end();
  for (auto bIt = func.valid_begin(); bIt != eIt; ++bIt) {
    if (bIt == func.common_exit()) {
      continue;
    }
    auto *bb = *bIt;
    if (bb->GetAttributes(kBBAttrIsEntry)) {
      continue;
    }
    BBId idx = bb->GetBBId();
    if (visitedBBs[idx]) {
      continue;
    }

    // if bb is StartTryBB, relationship between endtry and try should be maintained
    if (IsStartTryBB(*bb)) {
      bool needFixTryBB = false;
      size_t size = func.GetAllBBs().size();
      for (size_t nextIdx = idx + 1; nextIdx < size; ++nextIdx) {
        auto nextBB = func.GetBBFromID(BBId(nextIdx));
        if (nextBB == nullptr) {
          continue;
        }
        if (!visitedBBs[nextIdx] && nextBB->GetAttributes(kBBAttrIsTryEnd)) {
          break;
        }
        if (visitedBBs[nextIdx]) {
          needFixTryBB = true;
          visitedBBs[idx] = true;
          FixTryBB(*bb, *nextBB);
          cfgChanged = true;
          break;
        }
      }
      if (needFixTryBB) {
        continue;
      }
    }
    if (!MeOption::quiet) {
      LogInfo::MapleLogger() << "#### BB " << bb->GetBBId() << " deleted because unreachable\n";
    }
    if (bb->GetAttributes(kBBAttrIsTryEnd)) {
      // unreachable bb has try end info
      auto bbIt = std::find(func.rbegin(), func.rend(), bb);
      auto prevIt = ++bbIt;
      for (auto it = prevIt; it != func.rend(); ++it) {
        if (*it != nullptr) {
          // move entrytry tag to previous bb with try
          if ((*it)->GetAttributes(kBBAttrIsTry) && !(*it)->GetAttributes(kBBAttrIsTryEnd)) {
            (*it)->SetAttributes(kBBAttrIsTryEnd);
            func.SetTryBBByOtherEndTryBB(*it, bb);
          }
          break;
        }
      }
    }
    func.DeleteBasicBlock(*bb);
    cfgChanged = true;
    // remove the bb from its succ's pred_ list
    while (bb->GetSucc().size() > 0) {
      BB *sucBB = bb->GetSucc(0);
      sucBB->RemovePred(*bb, updatePhi);
      if (updatePhi) {
        if (sucBB->GetPred().empty()) {
          sucBB->ClearPhiList();
        } else if (sucBB->GetPred().size() == 1) {
          ConvertPhis2IdentityAssigns(*sucBB);
        }
      }
    }
    // remove the bb from common_exit_bb's pred list if it is there
    auto &predsOfCommonExit = func.GetCommonExitBB()->GetPred();
    auto it = std::find(predsOfCommonExit.begin(), predsOfCommonExit.end(), bb);
    if (it != predsOfCommonExit.end()) {
      func.GetCommonExitBB()->RemoveExit(*bb);
    }
  }
  return cfgChanged;
}

void MeCFG::ConvertPhiList2IdentityAssigns(BB &meBB) const {
  auto phiIt = meBB.GetPhiList().begin();
  while (phiIt != meBB.GetPhiList().end()) {
    // replace phi with identify assignment as it only has 1 opnd
    const OriginalSt *ost = func.GetMeSSATab()->GetOriginalStFromID(phiIt->first);
    if (ost->IsSymbolOst() && ost->GetIndirectLev() == 0) {
      const MIRSymbol *st = ost->GetMIRSymbol();
      MIRType *type = GlobalTables::GetTypeTable().GetTypeFromTyIdx(st->GetTyIdx());
      AddrofNode *dread = func.GetMIRModule().GetMIRBuilder()->CreateDread(*st, GetRegPrimType(type->GetPrimType()));
      auto *dread2 = func.GetMirFunc()->GetCodeMemPool()->New<AddrofSSANode>(*dread);
      dread2->SetSSAVar(*(*phiIt).second.GetPhiOpnd(0));
      DassignNode *dassign = func.GetMIRModule().GetMIRBuilder()->CreateStmtDassign(*st, 0, dread2);
      func.GetMeSSATab()->GetStmtsSSAPart().SetSSAPartOf(
          *dassign, func.GetMeSSATab()->GetStmtsSSAPart().GetSSAPartMp()->New<MayDefPartWithVersionSt>(
              &func.GetMeSSATab()->GetStmtsSSAPart().GetSSAPartAlloc()));
      auto *theSSAPart =
          static_cast<MayDefPartWithVersionSt*>(func.GetMeSSATab()->GetStmtsSSAPart().SSAPartOf(*dassign));
      theSSAPart->SetSSAVar(*((*phiIt).second.GetResult()));
      meBB.PrependStmtNode(dassign);
    }
    ++phiIt;
  }
  meBB.ClearPhiList();  // delete all the phis
}

void MeCFG::ConvertMePhiList2IdentityAssigns(BB &meBB) const {
  auto phiIt = meBB.GetMePhiList().begin();
  while (phiIt != meBB.GetMePhiList().end()) {
    // replace phi with identify assignment as it only has 1 opnd
    const OriginalSt *ost = func.GetMeSSATab()->GetOriginalStFromID(phiIt->first);
    if (ost->IsSymbolOst() && ost->GetIndirectLev() == 0) {
      MePhiNode *varPhi = phiIt->second;
      auto *dassign = func.GetIRMap()->NewInPool<DassignMeStmt>(
          static_cast<VarMeExpr*>(varPhi->GetLHS()), varPhi->GetOpnd(0));
      dassign->SetBB(varPhi->GetDefBB());
      dassign->SetIsLive(varPhi->GetIsLive());
      meBB.PrependMeStmt(dassign);
    } else if (ost->IsPregOst()) {
      MePhiNode *regPhi = phiIt->second;
      auto *regAss = func.GetIRMap()->New<AssignMeStmt>(
          OP_regassign, static_cast<RegMeExpr*>(regPhi->GetLHS()), regPhi->GetOpnd(0));
      regAss->SetBB(regPhi->GetDefBB());
      regAss->SetIsLive(regPhi->GetIsLive());
      meBB.PrependMeStmt(regAss);
    }
    ++phiIt;
  }
  meBB.GetMePhiList().clear();  // delete all the phis
}

// used only after DSE because it looks at live field of VersionSt
void MeCFG::ConvertPhis2IdentityAssigns(BB &meBB) const {
  if (meBB.IsEmpty()) {
    return;
  }
  ConvertPhiList2IdentityAssigns(meBB);
  ConvertMePhiList2IdentityAssigns(meBB);
}

// analyse the CFG to find the BBs that will not reach any function exit; these
// are BBs inside infinite loops; mark their wontExit flag and create
// artificial edges from them to common_exit_bb
void MeCFG::WontExitAnalysis() {
  if (func.NumBBs() == 0) {
    return;
  }
  std::vector<bool> visitedBBs(func.NumBBs(), false);
  func.GetCommonExitBB()->FindWillExitBBs(visitedBBs);
  auto eIt = func.valid_end();
  for (auto bIt = func.valid_begin(); bIt != eIt; ++bIt) {
    if (bIt == func.common_entry()) {
      continue;
    }
    auto *bb = *bIt;
    BBId idx = bb->GetBBId();
    if (visitedBBs[idx]) {
      continue;
    }
    bb->SetAttributes(kBBAttrWontExit);
    if (!MeOption::quiet) {
      LogInfo::MapleLogger() << "#### BB " << idx << " wont exit\n";
    }
    if (bb->GetKind() == kBBGoto) {
      // create artificial BB to transition to common_exit_bb
      BB *newBB = func.NewBasicBlock();
      // update bIt & eIt
      auto newBBIt = std::find(func.cbegin(), func.cend(), bb);
      bIt = build_filter_iterator(
          newBBIt, std::bind(FilterNullPtr<MapleVector<BB*>::const_iterator>, std::placeholders::_1, func.end()));
      eIt = func.valid_end();
      newBB->SetKindReturn();
      newBB->SetAttributes(kBBAttrArtificial);
      bb->AddSucc(*newBB);
      func.GetCommonExitBB()->AddExit(*newBB);
    }
  }
}

// CFG Verify
// Check bb-vec and bb-list are strictly consistent.
void MeCFG::Verify() const {
  // Check every bb in bb-list.
  auto eIt = func.valid_end();
  for (auto bIt = func.valid_begin(); bIt != eIt; ++bIt) {
    if (bIt == func.common_entry() || bIt == func.common_exit()) {
      continue;
    }
    auto *bb = *bIt;
    ASSERT(bb->GetBBId() < func.GetAllBBs().size(), "CFG Error!");
    ASSERT(func.GetBBFromID(bb->GetBBId()) == bb, "CFG Error!");
    if (bb->IsEmpty()) {
      continue;
    }
    ASSERT(bb->GetKind() != kBBUnknown, "runtime check error");
    // verify succ[1] is goto bb
    if (bb->GetKind() == kBBCondGoto) {
      if (!bb->GetAttributes(kBBAttrIsTry) && !bb->GetAttributes(kBBAttrWontExit)) {
        ASSERT(bb->GetStmtNodes().rbegin().base().d() != nullptr, "runtime check error");
        ASSERT(bb->GetSucc().size() == kBBVectorInitialSize, "runtime check error");
      }
      ASSERT(bb->GetSucc(1)->GetBBLabel() == static_cast<CondGotoNode&>(bb->GetStmtNodes().back()).GetOffset(),
             "runtime check error");
    } else if (bb->GetKind() == kBBGoto) {
      if (bb->GetStmtNodes().back().GetOpCode() == OP_throw) {
        continue;
      }
      if (!bb->GetAttributes(kBBAttrIsTry) && !bb->GetAttributes(kBBAttrWontExit)) {
        ASSERT(bb->GetStmtNodes().rbegin().base().d() != nullptr, "runtime check error");
        ASSERT(bb->GetSucc().size() == 1, "runtime check error");
      }
      ASSERT(bb->GetSucc(0)->GetBBLabel() == static_cast<GotoNode&>(bb->GetStmtNodes().back()).GetOffset(),
             "runtime check error");
    }
  }
}

// check that all the target labels in jump statements are defined
void MeCFG::VerifyLabels() const {
  auto eIt = func.valid_end();
  for (auto bIt = func.valid_begin(); bIt != eIt; ++bIt) {
    BB *mirBB = *bIt;
    auto &stmtNodes = mirBB->GetStmtNodes();
    if (stmtNodes.rbegin().base().d() == nullptr) {
      continue;
    }
    if (mirBB->GetKind() == kBBGoto) {
      if (stmtNodes.back().GetOpCode() == OP_throw) {
        continue;
      }
      ASSERT(
          func.GetLabelBBAt(static_cast<GotoNode&>(stmtNodes.back()).GetOffset())->GetBBLabel() ==
              static_cast<GotoNode&>(stmtNodes.back()).GetOffset(),
          "undefined label in goto");
    } else if (mirBB->GetKind() == kBBCondGoto) {
      ASSERT(
          func.GetLabelBBAt(static_cast<CondGotoNode&>(stmtNodes.back()).GetOffset())->GetBBLabel() ==
              static_cast<CondGotoNode&>(stmtNodes.back()).GetOffset(),
          "undefined label in conditional branch");
    } else if (mirBB->GetKind() == kBBSwitch) {
      auto &switchStmt = static_cast<SwitchNode&>(stmtNodes.back());
      LabelIdx targetLabIdx = switchStmt.GetDefaultLabel();
      BB *bb = func.GetLabelBBAt(targetLabIdx);
      ASSERT(bb->GetBBLabel() == targetLabIdx, "undefined label in switch");
      for (size_t j = 0; j < switchStmt.GetSwitchTable().size(); ++j) {
        targetLabIdx = switchStmt.GetCasePair(j).second;
        bb = func.GetLabelBBAt(targetLabIdx);
        ASSERT(bb->GetBBLabel() == targetLabIdx, "undefined switch target label");
      }
    }
  }
}

void MeCFG::Dump() const {
  // BSF Dump the cfg
  LogInfo::MapleLogger() << "####### CFG Dump: ";
  ASSERT(func.NumBBs() != 0, "size to be allocated is 0");
  auto *visitedMap = static_cast<bool*>(calloc(func.NumBBs(), sizeof(bool)));
  if (visitedMap != nullptr) {
    std::queue<BB*> qu;
    qu.push(func.GetFirstBB());
    while (!qu.empty()) {
      BB *bb = qu.front();
      qu.pop();
      if (bb == nullptr) {
        continue;
      }
      BBId id = bb->GetBBId();
      if (visitedMap[static_cast<long>(id)]) {
        continue;
      }
      LogInfo::MapleLogger() << id << " ";
      visitedMap[static_cast<long>(id)] = true;
      auto it = bb->GetSucc().begin();
      while (it != bb->GetSucc().end()) {
        BB *kidBB = *it;
        if (!visitedMap[static_cast<long>(kidBB->GetBBId())]) {
          qu.push(kidBB);
        }
        ++it;
      }
    }
    LogInfo::MapleLogger() << '\n';
    free(visitedMap);
  }
}

// replace special char in FunctionName for output file
static void ReplaceFilename(std::string &fileName) {
  for (char &c : fileName) {
    if (c == ';' || c == '/' || c == '|') {
      c = '_';
    }
  }
}

static bool ContainsConststr(const BaseNode &x) {
  if (x.GetOpCode() == OP_conststr || x.GetOpCode() == OP_conststr16) {
    return true;
  }
  for (size_t i = 0; i < x.NumOpnds(); ++i)
    if (ContainsConststr(*x.Opnd(i))) {
      return true;
    }
  return false;
}

std::string MeCFG::ConstructFileNameToDump(const std::string &prefix) const {
  std::string fileName;
  if (!prefix.empty()) {
    fileName.append(prefix);
    fileName.append("-");
  }
  // the func name length may exceed OS's file name limit, so truncate after 80 chars
  if (func.GetName().size() <= kFuncNameLenLimit) {
    fileName.append(func.GetName());
  } else {
    fileName.append(func.GetName().c_str(), kFuncNameLenLimit);
  }
  ReplaceFilename(fileName);
  fileName.append(".dot");
  return fileName;
}

void MeCFG::DumpToFileInStrs(std::ofstream &cfgFile) const {
  const auto &eIt = func.valid_end();
  for (auto bIt = func.valid_begin(); bIt != eIt; ++bIt) {
    auto *bb = *bIt;
    if (bb->GetKind() == kBBCondGoto) {
      cfgFile << "BB" << bb->GetBBId() << "[shape=diamond,label= \" BB" << bb->GetBBId() << ":\n{ ";
    } else {
      cfgFile << "BB" << bb->GetBBId() << "[shape=box,label= \" BB" << bb->GetBBId() << ":\n{ ";
    }
    if (bb->GetBBLabel() != 0) {
      cfgFile << "@" << func.GetMirFunc()->GetLabelName(bb->GetBBLabel()) << ":\n";
    }
    for (auto &phiPair : bb->GetPhiList()) {
      phiPair.second.Dump();
    }
    for (auto &stmt : bb->GetStmtNodes()) {
      // avoid printing content that may contain " as this needs to be quoted
      if (stmt.GetOpCode() == OP_comment) {
        continue;
      }
      if (ContainsConststr(stmt)) {
        continue;
      }
      stmt.Dump(1);
    }
    cfgFile << "}\"];\n";
  }
}

// generate dot file for cfg
void MeCFG::DumpToFile(const std::string &prefix, bool dumpInStrs, bool dumpEdgeFreq) const {
  if (MeOption::noDot) {
    return;
  }
  std::ofstream cfgFile;
  std::streambuf *coutBuf = LogInfo::MapleLogger().rdbuf(); // keep original cout buffer
  std::streambuf *buf = cfgFile.rdbuf();
  LogInfo::MapleLogger().rdbuf(buf);
  const std::string &fileName = ConstructFileNameToDump(prefix);
  cfgFile.open(fileName, std::ios::trunc);
  cfgFile << "digraph {\n";
  cfgFile << " # /*" << func.GetName() << " (red line is exception handler)*/\n";
  // dump edge
  auto eIt = func.valid_end();
  for (auto bIt = func.valid_begin(); bIt != eIt; ++bIt) {
    auto *bb = *bIt;
    if (bIt == func.common_exit()) {
      // specical case for common_exit_bb
      for (auto it = bb->GetPred().begin(); it != bb->GetPred().end(); ++it) {
        dumpEdgeFreq ? cfgFile << "BB" << (*it)->GetBBId() << "_freq_" << (*it)->GetFrequency() << " -> " <<
            "BB" << bb->GetBBId() << "_freq_" << bb->GetFrequency() << "[style=dotted];\n" :
            cfgFile << "BB" << (*it)->GetBBId()<< " -> " << "BB" << bb->GetBBId() << "[style=dotted];\n";
      }
      continue;
    }
    if (bb->GetAttributes(kBBAttrIsInstrument)) {
      cfgFile << "BB" << bb->GetBBId() << "[color=blue];\n";
    }

    for (auto it = bb->GetSucc().begin(); it != bb->GetSucc().end(); ++it) {
      dumpEdgeFreq ? cfgFile << "BB" << bb->GetBBId() << "_freq_" << bb->GetFrequency() << " -> " <<
          "BB" << (*it)->GetBBId() << "_freq_" << (*it)->GetFrequency() :
          cfgFile << "BB" << bb->GetBBId() << " -> " << "BB" << (*it)->GetBBId();
      if (bb == func.GetCommonEntryBB()) {
        cfgFile << "[style=dotted]";
        continue;
      }
      if ((*it)->GetAttributes(kBBAttrIsCatch)) {
        /* succ is exception handler */
        cfgFile << "[color=red]";
      }

      if (dumpEdgeFreq) {
        cfgFile << "[label=" << bb->GetEdgeFreq(*it) << "];\n";
      } else {
        cfgFile << ";\n";
      }
    }
  }
  // dump instruction in each BB
  if (dumpInStrs) {
    DumpToFileInStrs(cfgFile);
  }
  cfgFile << "}\n";
  cfgFile.flush();
  LogInfo::MapleLogger().rdbuf(coutBuf);
}
}  // namespace maple
