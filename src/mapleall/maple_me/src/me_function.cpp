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
#include "me_function.h"
#include <iostream>
#include <functional>
#include "ssa_mir_nodes.h"
#include "me_cfg.h"
#include "mir_lower.h"
#include "mir_builder.h"
#include "constantfold.h"
#include "me_irmap.h"
#include "me_phase.h"

namespace maple {
#if DEBUG
MIRModule *globalMIRModule = nullptr;
MeFunction *globalFunc = nullptr;
MeIRMap *globalIRMap = nullptr;
SSATab *globalSSATab = nullptr;
#endif
void MeFunction::PartialInit(bool isSecondPass) {
  theCFG = nullptr;
  irmap = nullptr;
  regNum = 0;
  hasEH = false;
  secondPass = isSecondPass;
  ConstantFold cf(mirModule);
  cf.Simplify(mirModule.CurFunction()->GetBody());
  if (mirModule.IsJavaModule() && (!mirModule.CurFunction()->GetInfoVector().empty())) {
    std::string string("INFO_registers");
    GStrIdx strIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(string);
    regNum = mirModule.CurFunction()->GetInfo(strIdx);
    std::string tryNum("INFO_tries_size");
    strIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(tryNum);
    uint32 num = mirModule.CurFunction()->GetInfo(strIdx);
    hasEH = (num != 0);
  }
}

void MeFunction::DumpFunction() const {
  if (meSSATab == nullptr) {
    LogInfo::MapleLogger() << "no ssa info, just dump simpfunction\n";
    DumpFunctionNoSSA();
    return;
  }
  auto eIt = valid_end();
  for (auto bIt = valid_begin(); bIt != eIt; ++bIt) {
    auto *bb = *bIt;
    bb->DumpHeader(&mirModule);
    for (auto &phiPair : bb->GetPhiList()) {
      phiPair.second.Dump();
    }
    for (auto &stmt : bb->GetStmtNodes()) {
      GenericSSAPrint(mirModule, stmt, 1, meSSATab->GetStmtsSSAPart());
    }
  }
}

void MeFunction::DumpFunctionNoSSA() const {
  auto eIt = valid_end();
  for (auto bIt = valid_begin(); bIt != eIt; ++bIt) {
    auto *bb = *bIt;
    bb->DumpHeader(&mirModule);
    for (auto &phiPair : bb->GetPhiList()) {
      phiPair.second.Dump();
    }
    for (auto &stmt : bb->GetStmtNodes()) {
      stmt.Dump(1);
    }
  }
}

void MeFunction::DumpMayDUFunction() const {
  auto eIt = valid_end();
  for (auto bIt = valid_begin(); bIt != eIt; ++bIt) {
    auto *bb = *bIt;
    bb->DumpHeader(&mirModule);
    bool skipStmt = false;
    CHECK_FATAL(meSSATab != nullptr, "meSSATab is null");
    for (auto &stmt : bb->GetStmtNodes()) {
      if (meSSATab->GetStmtsSSAPart().HasMayDef(stmt) || HasMayUseOpnd(stmt, *meSSATab) ||
          kOpcodeInfo.NotPure(stmt.GetOpCode())) {
        if (skipStmt) {
          mirModule.GetOut() << "......\n";
        }
        GenericSSAPrint(mirModule, stmt, 1, meSSATab->GetStmtsSSAPart());
        skipStmt = false;
      } else {
        skipStmt = true;
      }
    }
    if (skipStmt) {
      mirModule.GetOut() << "......\n";
    }
  }
}

void MeFunction::Dump(bool DumpSimpIr) const {
  LogInfo::MapleLogger() << ">>>>> Dump IR for Function " << mirFunc->GetName() << "<<<<<\n";
  if (irmap == nullptr || DumpSimpIr) {
    LogInfo::MapleLogger() << "no ssa or irmap info, just dump simp function\n";
    DumpFunction();
    return;
  }
  auto eIt = valid_end();
  for (auto bIt = valid_begin(); bIt != eIt; ++bIt) {
    auto *bb = *bIt;
    bb->DumpHeader(&mirModule);
    bb->DumpMePhiList(irmap);
    for (auto &meStmt : bb->GetMeStmts()) {
      meStmt.Dump(irmap);
    }
  }
}

void MeFunction::SetTryBlockInfo(const StmtNode *nextStmt, StmtNode *tryStmt, BB *lastTryBB, BB *curBB, BB *newBB) {
  if (nextStmt->GetOpCode() == OP_endtry) {
    curBB->SetAttributes(kBBAttrIsTryEnd);
    ASSERT(lastTryBB != nullptr, "null ptr check");
    endTryBB2TryBB[curBB] = lastTryBB;
  } else {
    newBB->SetAttributes(kBBAttrIsTry);
    bbTryNodeMap[newBB] = tryStmt;
  }
}

void MeFunction::CreateBasicBlocks() {
  if (CurFunction()->IsEmpty()) {
    if (!MeOption::quiet) {
      LogInfo::MapleLogger() << "function is empty, cfg is nullptr\n";
    }
    return;
  }
  // create common_entry/exit bb first as bbVec[0] and bb_vec[1]
  auto *commonEntryBB = NewBasicBlock();
  commonEntryBB->SetAttributes(kBBAttrIsEntry);
  auto *commonExitBB = NewBasicBlock();
  commonExitBB->SetAttributes(kBBAttrIsExit);
  auto *firstBB = NewBasicBlock();
  firstBB->SetAttributes(kBBAttrIsEntry);
  StmtNode *nextStmt = CurFunction()->GetBody()->GetFirst();
  ASSERT(nextStmt != nullptr, "function has no statement");
  BB *curBB = firstBB;
  StmtNode *tryStmt = nullptr;  // record current try stmt for map<bb, try_stmt>
  BB *lastTryBB = nullptr;      // bb containing try_stmt
  do {
    StmtNode *stmt = nextStmt;
    nextStmt = stmt->GetNext();
    switch (stmt->GetOpCode()) {
      case OP_goto: {
        if (curBB->IsEmpty()) {
          curBB->SetFirst(stmt);
        }
        curBB->SetLast(stmt);
        curBB->SetKind(kBBGoto);
        if (nextStmt != nullptr) {
          BB *newBB = NewBasicBlock();
          if (tryStmt != nullptr) {
            SetTryBlockInfo(nextStmt, tryStmt, lastTryBB, curBB, newBB);
          }
          curBB = newBB;
        }
        break;
      }
      case OP_igoto: {
        if (curBB->IsEmpty()) {
          curBB->SetFirst(stmt);
        }
        curBB->SetLast(stmt);
        curBB->SetKind(kBBIgoto);
        if (nextStmt != nullptr) {
          curBB = NewBasicBlock();
        }
        break;
      }
      case OP_dassign: {
        if (curBB->IsEmpty()) {
          curBB->SetFirst(stmt);
        }
        if (static_cast<DassignNode*>(stmt)->GetRHS()->MayThrowException()) {
          stmt->SetOpCode(OP_maydassign);
          if (tryStmt != nullptr) {
            // breaks new BB only inside try blocks
            curBB->SetLast(stmt);
            curBB->SetKind(kBBFallthru);
            BB *newBB = NewBasicBlock();
            SetTryBlockInfo(nextStmt, tryStmt, lastTryBB, curBB, newBB);
            curBB = newBB;
            break;
          }
        }
        if ((nextStmt == nullptr) && to_ptr(curBB->GetStmtNodes().rbegin()) == nullptr) {
          curBB->SetLast(stmt);
        }
        break;
      }
      case OP_brfalse:
      case OP_brtrue: {
        if (curBB->IsEmpty()) {
          curBB->SetFirst(stmt);
        }
        curBB->SetLast(stmt);
        curBB->SetKind(kBBCondGoto);
        BB *newBB = NewBasicBlock();
        if (tryStmt != nullptr) {
          SetTryBlockInfo(nextStmt, tryStmt, lastTryBB, curBB, newBB);
        }
        curBB = newBB;
        break;
      }
      case OP_if:
      case OP_doloop:
      case OP_dowhile:
      case OP_while: {
        ASSERT(false, "not yet implemented");
        break;
      }
      case OP_throw:
        if (tryStmt != nullptr) {
          // handle as goto
          if (curBB->IsEmpty()) {
            curBB->SetFirst(stmt);
          }
          curBB->SetLast(stmt);
          curBB->SetKind(kBBGoto);
          if (nextStmt != nullptr) {
            BB *newBB = NewBasicBlock();
            SetTryBlockInfo(nextStmt, tryStmt, lastTryBB, curBB, newBB);
            curBB = newBB;
          }
          break;
        }
      // fall thru to handle as return
      [[clang::fallthrough]];
      case OP_gosub:
      case OP_retsub:
      case OP_return: {
        if (curBB->IsEmpty()) {
          curBB->SetFirst(stmt);
        }
        curBB->SetLast(stmt);
        curBB->SetKindReturn();
        if (nextStmt != nullptr) {
          BB *newBB = NewBasicBlock();
          if (tryStmt != nullptr) {
            SetTryBlockInfo(nextStmt, tryStmt, lastTryBB, curBB, newBB);
          }
          curBB = newBB;
          if (stmt->GetOpCode() == OP_gosub) {
            curBB->SetAttributes(kBBAttrIsEntry);
          }
        }
        break;
      }
      case OP_endtry:
        if (mirModule.IsJavaModule()) {
          if (tryStmt == nullptr) {
            break;
          }
          /* skip OP_entry and generate it in emit phase */
          ASSERT(lastTryBB != nullptr, "null ptr check");
          tryStmt = nullptr;  // reset intryblocks
          if (!curBB->IsEmpty()) {
            StmtNode *lastStmt = stmt->GetPrev();
            ASSERT(curBB->GetStmtNodes().rbegin().base().d() == nullptr ||
                   curBB->GetStmtNodes().rbegin().base().d() == lastStmt,
                   "something wrong building BB");
            curBB->SetLast(lastStmt);
            if (curBB->GetKind() == kBBUnknown) {
              curBB->SetKind(kBBFallthru);
            }
            curBB->SetAttributes(kBBAttrIsTryEnd);
            endTryBB2TryBB[curBB] = lastTryBB;
            curBB = NewBasicBlock();
          } else if (curBB->GetBBLabel() != 0) {
            // create the empty BB
            curBB->SetKind(kBBFallthru);
            curBB->SetAttributes(kBBAttrIsTryEnd);
            endTryBB2TryBB[curBB] = lastTryBB;
            curBB = NewBasicBlock();
          } else {
          }  // endtry has already been processed in SetTryBlockInfo()
          lastTryBB = nullptr;
        } else {
          if (curBB->IsEmpty()) {
            curBB->SetFirst(stmt);
          }
          if ((nextStmt == nullptr) && (curBB->GetStmtNodes().rbegin().base().d() == nullptr)) {
            curBB->SetLast(stmt);
          }
        }
        break;
      case OP_try: {
        // start a new bb or with a label
        if (!curBB->IsEmpty()) {
          // prepare a new bb
          StmtNode *lastStmt = stmt->GetPrev();
          ASSERT(curBB->GetStmtNodes().rbegin().base().d() == nullptr ||
                 curBB->GetStmtNodes().rbegin().base().d() == lastStmt,
                 "something wrong building BB");
          curBB->SetLast(lastStmt);
          if (curBB->GetKind() == kBBUnknown) {
            curBB->SetKind(kBBFallthru);
          }
          BB *newBB = NewBasicBlock();
          // assume no nested try, so no need to call SetTryBlockInfo()
          curBB = newBB;
        }
        curBB->SetFirst(stmt);
        tryStmt = stmt;
        lastTryBB = curBB;
        curBB->SetAttributes(kBBAttrIsTry);
        bbTryNodeMap[curBB] = tryStmt;
        // prepare a new bb that contains only a OP_try. It is needed for
        // dse to work correctly: assignments in a try block should not affect
        // assignments before the try block as exceptions might occur.
        curBB->SetLast(stmt);
        curBB->SetKind(kBBFallthru);
        BB *newBB = NewBasicBlock();
        SetTryBlockInfo(nextStmt, tryStmt, lastTryBB, curBB, newBB);
        curBB = newBB;
        break;
      }
      case OP_catch: {
        // start a new bb or with a label
        if (!curBB->IsEmpty()) {
          // prepare a new bb
          StmtNode *lastStmt = stmt->GetPrev();
          ASSERT(curBB->GetStmtNodes().rbegin().base().d() == nullptr ||
                 curBB->GetStmtNodes().rbegin().base().d() == lastStmt,
                 "something wrong building BB");
          curBB->SetLast(lastStmt);
          if (curBB->GetKind() == kBBUnknown) {
            curBB->SetKind(kBBFallthru);
          }
          BB *newBB = NewBasicBlock();
          if (tryStmt != nullptr) {
            SetTryBlockInfo(nextStmt, tryStmt, lastTryBB, curBB, newBB);
          }
          curBB = newBB;
        }
        curBB->SetFirst(stmt);
        curBB->SetAttributes(kBBAttrIsCatch);
        auto *catchNode = static_cast<CatchNode*>(stmt);
        const MapleVector<TyIdx> &exceptionTyIdxVec = catchNode->GetExceptionTyIdxVec();

        for (TyIdx exceptIdx : exceptionTyIdxVec) {
          MIRType *eType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(exceptIdx);
          ASSERT(eType != nullptr && (eType->GetPrimType() == PTY_ptr || eType->GetPrimType() == PTY_ref),
                 "wrong exception type");
          auto *exceptType = static_cast<MIRPtrType*>(eType);
          MIRType *pointType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(exceptType->GetPointedTyIdx());
          const std::string &eName = GlobalTables::GetStrTable().GetStringFromStrIdx(pointType->GetNameStrIdx());
          if ((pointType->GetPrimType() == PTY_void) || (eName == "Ljava/lang/Throwable;") ||
              (eName == "Ljava/lang/Exception;")) {
            // "Ljava/lang/Exception;" is risk to set isJavaFinally because it
            // only deal with "throw exception". if throw error,  it's wrong
            curBB->SetAttributes(kBBAttrIsJavaFinally);  // this is a start of finally handler
          }
        }
        break;
      }
      case OP_label: {
        auto *labelNode = static_cast<LabelNode*>(stmt);
        LabelIdx labelIdx = labelNode->GetLabelIdx();
        if (!curBB->IsEmpty() || curBB->GetBBLabel() != 0) {
          // prepare a new bb
          StmtNode *lastStmt = stmt->GetPrev();
          ASSERT((curBB->GetStmtNodes().rbegin().base().d() == nullptr ||
                  curBB->GetStmtNodes().rbegin().base().d() == lastStmt),
                 "something wrong building BB");
          if (curBB->GetStmtNodes().rbegin().base().d() == nullptr && (lastStmt->GetOpCode() != OP_label)) {
            if (mirModule.IsJavaModule() && lastStmt->GetOpCode() == OP_endtry) {
              if (curBB->GetStmtNodes().empty()) {
                curBB->SetLast(nullptr);
              } else {
                // find a valid stmt which is not label or endtry
                StmtNode *p = lastStmt->GetPrev();
                ASSERT(p != nullptr, "null ptr check");
                ASSERT(p->GetOpCode() != OP_label, "runtime check error");
                ASSERT(p->GetOpCode() != OP_endtry, "runtime check error");
                curBB->SetLast(p);
              }
            } else {
              curBB->SetLast(lastStmt);
            }
          }
          if (curBB->GetKind() == kBBUnknown) {
            curBB->SetKind(kBBFallthru);
          }
          BB *newBB = NewBasicBlock();
          if (tryStmt != nullptr) {
            newBB->SetAttributes(kBBAttrIsTry);
            bbTryNodeMap[newBB] = tryStmt;
          }
          curBB = newBB;
        }
        labelBBIdMap[labelIdx] = curBB;
        curBB->SetBBLabel(labelIdx);
        break;
      }
      case OP_jscatch: {
        if (curBB->IsEmpty()) {
          curBB->SetFirst(stmt);
        }
        curBB->SetAttributes(kBBAttrIsEntry);
        curBB->SetAttributes(kBBAttrIsJSCatch);
        break;
      }
      case OP_finally: {
        ASSERT(curBB->GetStmtNodes().empty(), "runtime check error");
        curBB->SetFirst(stmt);
        curBB->SetAttributes(kBBAttrIsEntry);
        curBB->SetAttributes(kBBAttrIsJSFinally);
        break;
      }
      case OP_switch: {
        if (curBB->IsEmpty()) {
          curBB->SetFirst(stmt);
        }
        curBB->SetLast(stmt);
        curBB->SetKind(kBBSwitch);
        BB *newBB = NewBasicBlock();
        if (tryStmt != nullptr) {
          SetTryBlockInfo(nextStmt, tryStmt, lastTryBB, curBB, newBB);
        }
        curBB = newBB;
        break;
      }
      default: {
        if (curBB->IsEmpty()) {
          curBB->SetFirst(stmt);
        }
        if ((nextStmt == nullptr) && (curBB->GetStmtNodes().rbegin().base().d() == nullptr)) {
          curBB->SetLast(stmt);
        }
        break;
      }
    }
  } while (nextStmt != nullptr);
  ASSERT(tryStmt == nullptr, "unclosed try");    // tryandendtry should be one-one mapping
  ASSERT(lastTryBB == nullptr, "unclosed tryBB");  // tryandendtry should be one-one mapping
  auto *lastBB = curBB;
  if (lastBB->IsEmpty()) {
    // insert a return statement
    lastBB->SetFirst(mirModule.GetMIRBuilder()->CreateStmtReturn(nullptr));
    lastBB->SetLast(lastBB->GetStmtNodes().begin().d());
    lastBB->SetKindReturn();
  } else if (lastBB->GetKind() == kBBUnknown) {
    lastBB->SetKindReturn();
    lastBB->SetAttributes(kBBAttrIsExit);
  }
}

void MeFunction::Prepare(unsigned long rangeNum) {
  if (!MeOption::quiet) {
    LogInfo::MapleLogger() << "---Preparing Function  < " << CurFunction()->GetName() << " > [" << rangeNum
                           << "] ---\n";
  }
  /* lower first */
  MIRLower mirLowerer(mirModule, CurFunction());
  mirLowerer.Init();
  mirLowerer.SetLowerME();
  mirLowerer.SetLowerExpandArray();
  ASSERT(CurFunction() != nullptr, "nullptr check");
  mirLowerer.LowerFunc(*CurFunction());
  CreateBasicBlocks();
  if (NumBBs() == 0) {
    /* there's no basicblock generated */
    return;
  }
  theCFG = memPool->New<MeCFG>(*this);
  theCFG->BuildMirCFG();
  if (MeOption::optLevel > mapleOption::kLevelZero) {
    theCFG->FixMirCFG();
  }
  theCFG->ReplaceWithAssertnonnull();
  theCFG->VerifyLabels();
  theCFG->UnreachCodeAnalysis();
  theCFG->WontExitAnalysis();
  theCFG->Verify();
}

void MeFunction::Verify() const {
  CHECK_FATAL(theCFG != nullptr, "theCFG is null");
  theCFG->Verify();
  theCFG->VerifyLabels();
}

BB *MeFunction::NewBasicBlock() {
  BB *newBB = memPool->New<BB>(&alloc, &versAlloc, BBId(nextBBId++));
  bbVec.push_back(newBB);
  return newBB;
}

// new a basic block and insert before position or after position
BB &MeFunction::InsertNewBasicBlock(const BB &position, bool isInsertBefore) {
  BB *newBB = memPool->New<BB>(&alloc, &versAlloc, BBId(nextBBId++));

  auto bIt = std::find(begin(), end(), &position);
  auto idx = position.GetBBId();
  if (!isInsertBefore) {
    ++bIt;
    ++idx;
  }
  auto newIt = bbVec.insert(bIt, newBB);
  auto eIt = end();
  // update bb's idx
  for (auto it = newIt; it != eIt; ++it) {
    if ((*it) != nullptr) {
      (*it)->SetBBId(BBId(idx));
    }
    ++idx;
  }
  return *newBB;
}

void MeFunction::DeleteBasicBlock(const BB &bb) {
  ASSERT(bbVec[bb.GetBBId()] == &bb, "runtime check error");
  /* update first_bb_ and last_bb if needed */
  bbVec.at(bb.GetBBId()) = nullptr;
}

/* get next bb in bbVec */
BB *MeFunction::NextBB(const BB *bb) {
  auto bbIt = std::find(begin(), end(), bb);
  CHECK_FATAL(bbIt != end(), "bb must be inside bb_vec");
  for (auto it = ++bbIt; it != end(); ++it) {
    if (*it != nullptr) {
      return *it;
    }
  }
  return nullptr;
}

/* get prev bb in bbVec */
BB *MeFunction::PrevBB(const BB *bb) {
  auto bbIt = std::find(rbegin(), rend(), bb);
  CHECK_FATAL(bbIt != rend(), "bb must be inside bb_vec");
  for (auto it = ++bbIt; it != rend(); ++it) {
    if (*it != nullptr) {
      return *it;
    }
  }
  return nullptr;
}

/* clone stmtnode in orig bb to newBB */
void MeFunction::CloneBasicBlock(BB &newBB, const BB &orig) {
  if (orig.IsEmpty()) {
    return;
  }
  for (const auto &stmt : orig.GetStmtNodes()) {
    StmtNode *newStmt = stmt.CloneTree(mirModule.GetCurFuncCodeMPAllocator());
    newStmt->SetNext(nullptr);
    newStmt->SetPrev(nullptr);
    newBB.AddStmtNode(newStmt);
    if (meSSATab != nullptr) {
      meSSATab->CreateSSAStmt(*newStmt, &newBB);
    }
  }
}

void MeFunction::SplitBBPhysically(BB &bb, StmtNode &splitPoint, BB &newBB) {
  StmtNode *newBBStart = splitPoint.GetNext();
  // Fix Stmt in BB.
  if (newBBStart != nullptr) {
    newBBStart->SetPrev(nullptr);
    for (StmtNode *stmt = newBBStart; stmt != nullptr;) {
      StmtNode *nextStmt = stmt->GetNext();
      newBB.AddStmtNode(stmt);
      if (meSSATab != nullptr) {
        meSSATab->CreateSSAStmt(*stmt, &newBB);
      }
      stmt = nextStmt;
    }
  }
  bb.SetLast(&splitPoint);
  splitPoint.SetNext(nullptr);
}

/* Split BB at split_point */
BB &MeFunction::SplitBB(BB &bb, StmtNode &splitPoint, BB *newBB) {
  if (newBB == nullptr) {
    newBB = memPool->New<BB>(&alloc, &versAlloc, BBId(nextBBId++));
  }
  SplitBBPhysically(bb, splitPoint, *newBB);
  // Fix BB in CFG
  newBB->SetKind(bb.GetKind());
  bb.SetKind(kBBFallthru);
  auto bIt = std::find(begin(), end(), &bb);
  auto idx = bb.GetBBId();
  auto newIt = bbVec.insert(++bIt, newBB);
  auto eIt = end();
  // update bb's idx
  for (auto it = newIt; it != eIt; ++it) {
    ++idx;
    if ((*it) != nullptr) {
      (*it)->SetBBId(BBId(idx));
    }
  }
  // Special Case: commonExitBB is orig bb's succ
  auto *commonExitBB = *common_exit();
  if (bb.IsPredBB(*commonExitBB)) {
    commonExitBB->RemoveExit(bb);
    commonExitBB->AddExit(*newBB);
  }
  while (bb.GetSucc().size() > 0) {
    BB *succ = bb.GetSucc(0);
    succ->ReplacePred(&bb, newBB);
  }
  bb.AddSucc(*newBB);
  // Setup flags
  newBB->CopyFlagsAfterSplit(bb);
  if (newBB->GetAttributes(kBBAttrIsTryEnd)) {
    endTryBB2TryBB[newBB] = endTryBB2TryBB[&bb];
    endTryBB2TryBB[&bb] = nullptr;
    bb.ClearAttributes(kBBAttrIsTryEnd);
  }
  bb.ClearAttributes(kBBAttrIsExit);
  return *newBB;
}

/* create label for bb */
LabelIdx MeFunction::GetOrCreateBBLabel(BB &bb) {
  LabelIdx label = bb.GetBBLabel();
  if (label != 0) {
    return label;
  }
  label = mirModule.CurFunction()->GetLabelTab()->CreateLabelWithPrefix('m');
  mirModule.CurFunction()->GetLabelTab()->AddToStringLabelMap(label);
  bb.SetBBLabel(label);
  labelBBIdMap.insert(std::make_pair(label, &bb));
  return label;
}

void MeFunction::BuildSCCDFS(BB &bb, uint32 &visitIndex, std::vector<SCCOfBBs*> &sccNodes,
                             std::vector<uint32> &visitedOrder, std::vector<uint32> &lowestOrder,
                             std::vector<bool> &inStack, std::stack<uint32> &visitStack) {
  uint32 id = bb.UintID();
  visitedOrder[id] = visitIndex;
  lowestOrder[id] = visitIndex;
  ++visitIndex;
  visitStack.push(id);
  inStack[id] = true;

  for (BB *succ : bb.GetSucc()){
    if (succ == nullptr) {
      continue;
    }
    uint32 succId = succ->UintID();
    if (!visitedOrder[succId]) {
      BuildSCCDFS(*succ, visitIndex, sccNodes, visitedOrder, lowestOrder, inStack, visitStack);
      if (lowestOrder[succId] < lowestOrder[id]) {
        lowestOrder[id] = lowestOrder[succId];
      }
    } else if (inStack[succId]) {
      backEdges.insert(std::pair<uint32, uint32>(id, succId));
      if (visitedOrder[succId] < lowestOrder[id]) {
        lowestOrder[id] = visitedOrder[succId];
      }
    }
  }

  if (visitedOrder.at(id) == lowestOrder.at(id)) {
    auto *sccNode = alloc.GetMemPool()->New<SCCOfBBs>(numOfSCCs++, &bb, &alloc);
    uint32 stackTopId;
    do {
      stackTopId = visitStack.top();
      visitStack.pop();
      inStack[stackTopId] = false;
      auto *topBB = static_cast<BB*>(GetAllBBs()[stackTopId]);
      sccNode->AddBBNode(topBB);
      sccOfBB[stackTopId] = sccNode;
    } while (stackTopId != id);

    sccNodes.push_back(sccNode);
  }
}

void MeFunction::VerifySCC() {
  for (BB *bb : GetAllBBs()) {
    if (bb == nullptr || bb == GetCommonExitBB()) {
      continue;
    }
    SCCOfBBs *scc = sccOfBB.at(bb->UintID());
    CHECK_FATAL(scc != nullptr, "bb should belong to a scc");
  }
}

void MeFunction::BuildSCC() {
  sccTopologicalVec.clear();
  sccOfBB.clear();
  sccOfBB.assign(GetAllBBs().size(), nullptr);
  std::vector<uint32> visitedOrder(GetAllBBs().size(), 0);
  std::vector<uint32> lowestOrder(GetAllBBs().size(), 0);
  std::vector<bool> inStack(GetAllBBs().size(), false);
  std::vector<SCCOfBBs*> sccNodes;
  uint32 visitIndex = 1;
  std::stack<uint32> visitStack;

  // Starting from common entry bb for DFS
  BuildSCCDFS(*GetCommonEntryBB(), visitIndex, sccNodes, visitedOrder, lowestOrder, inStack, visitStack);

  for (SCCOfBBs *scc : sccNodes) {
    scc->Verify(sccOfBB);
    scc->SetUp(sccOfBB);
  }

  VerifySCC();
  SCCTopologicalSort(sccNodes);
}

void MeFunction::SCCTopologicalSort(std::vector<SCCOfBBs*> &sccNodes) {
  std::set<SCCOfBBs*> inQueue;
  for (SCCOfBBs *node : sccNodes) {
    if (!node->HasPred()) {
      sccTopologicalVec.push_back(node);
      (void)inQueue.insert(node);
    }
  }

  // Top-down iterates all nodes
  for (size_t i = 0; i < sccTopologicalVec.size(); ++i) {
    SCCOfBBs *sccBB = sccTopologicalVec[i];
    for (SCCOfBBs *succ : sccBB->GetSucc()) {
      if (inQueue.find(succ) == inQueue.end()) {
        // successor has not been visited
        bool predAllVisited = true;
        // check whether all predecessors of the current successor have been visited
        for (SCCOfBBs *pred : succ->GetPred()) {
          if (inQueue.find(pred) == inQueue.end()) {
            predAllVisited = false;
            break;
          }
        }
        if (predAllVisited) {
          sccTopologicalVec.push_back(succ);
          (void)inQueue.insert(succ);
        }
      }
    }
  }
}

void MeFunction::BBTopologicalSort(SCCOfBBs &scc) {
  std::set<BB*> inQueue;
  std::vector<BB*> bbs;
  for (BB *bb : scc.GetBBs()) {
    bbs.push_back(bb);
  }
  scc.Clear();
  scc.AddBBNode(scc.GetEntry());
  (void)inQueue.insert(scc.GetEntry());

  for (size_t i = 0; i < scc.GetBBs().size(); ++i) {
    BB *bb = scc.GetBBs()[i];
    for (BB *succ : bb->GetSucc()) {
      if (succ == nullptr) {
        continue;
      }
      if (inQueue.find(succ) != inQueue.end() ||
          std::find(bbs.begin(), bbs.end(), succ) == bbs.end()) {
        continue;
      }
      bool predAllVisited = true;
      for (BB *pred : succ->GetPred()) {
        if (pred == nullptr) {
          continue;
        }
        if (std::find(bbs.begin(), bbs.end(), pred) == bbs.end()) {
          continue;
        }
        if (backEdges.find(std::pair<uint32, uint32>(pred->UintID(), succ->UintID())) != backEdges.end()) {
          continue;
        }
        if (inQueue.find(pred) == inQueue.end()) {
          predAllVisited = false;
          break;
        }
      }
      if (predAllVisited) {
        scc.AddBBNode(succ);
        (void)inQueue.insert(succ);
      }
    }
  }
}
}  // namespace maple
