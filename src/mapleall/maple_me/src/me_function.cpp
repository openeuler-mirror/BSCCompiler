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
void MeFunction::PartialInit() {
  theCFG = nullptr;
  irmap = nullptr;
  regNum = 0;
  hasEH = false;
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
  auto eIt = theCFG->valid_end();
  for (auto bIt = theCFG->valid_begin(); bIt != eIt; ++bIt) {
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
  auto eIt = theCFG->valid_end();
  for (auto bIt = theCFG->valid_begin(); bIt != eIt; ++bIt) {
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
  auto eIt = theCFG->valid_end();
  for (auto bIt = theCFG->valid_begin(); bIt != eIt; ++bIt) {
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
  auto eIt = theCFG->valid_end();
  for (auto bIt = theCFG->valid_begin(); bIt != eIt; ++bIt) {
    auto *bb = *bIt;
    bb->DumpHeader(&mirModule);
    bb->DumpMePhiList(irmap);
    for (auto &meStmt : bb->GetMeStmts()) {
      meStmt.Dump(irmap);
    }
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
}

void MeFunction::Verify() const {
  CHECK_FATAL(theCFG != nullptr, "theCFG is null");
  theCFG->Verify();
  theCFG->VerifyLabels();
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
  theCFG->SetLabelBBAt(label, &bb);
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
      auto *topBB = static_cast<BB*>(theCFG->GetAllBBs()[stackTopId]);
      sccNode->AddBBNode(topBB);
      sccOfBB[stackTopId] = sccNode;
    } while (stackTopId != id);

    sccNodes.push_back(sccNode);
  }
}

void MeFunction::VerifySCC() {
  for (BB *bb : theCFG->GetAllBBs()) {
    if (bb == nullptr || bb == theCFG->GetCommonExitBB()) {
      continue;
    }
    SCCOfBBs *scc = sccOfBB.at(bb->UintID());
    CHECK_FATAL(scc != nullptr, "bb should belong to a scc");
  }
}

void MeFunction::BuildSCC() {
  uint32_t  n = theCFG->GetAllBBs().size();
  sccTopologicalVec.clear();
  sccOfBB.clear();
  sccOfBB.assign(n, nullptr);
  std::vector<uint32> visitedOrder(n, 0);
  std::vector<uint32> lowestOrder(n, 0);
  std::vector<bool> inStack(n, false);
  std::vector<SCCOfBBs*> sccNodes;
  uint32 visitIndex = 1;
  std::stack<uint32> visitStack;

  // Starting from common entry bb for DFS
  BuildSCCDFS(*theCFG->GetCommonEntryBB(), visitIndex, sccNodes, visitedOrder, lowestOrder, inStack, visitStack);

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
