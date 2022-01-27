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
#include "ssa.h"
#include <iostream>
#include "ssa_tab.h"
#include "ssa_mir_nodes.h"
#include "ver_symbol.h"
#include "dominance.h"

namespace maple {
void SSA::InitRenameStack(const OriginalStTable &oTable, size_t bbSize, const VersionStTable &verStTab) {
  vstStacks.clear();
  vstStacks.resize(oTable.Size());
  bbRenamed.clear();
  bbRenamed.resize(bbSize, false);
  for (size_t i = 1; i < oTable.Size(); ++i) {
    const OriginalSt *ost = oTable.GetOriginalStFromID(OStIdx(i));
    if (ost->GetIndirectLev() < 0) {
      continue;
    }
    MapleStack<VersionSt*> *vStack = ssaAlloc.GetMemPool()->New<MapleStack<VersionSt*>>(ssaAlloc.Adapter());
    VersionSt *temp = verStTab.GetVersionStVectorItem(ost->GetZeroVersionIndex());
    vStack->push(temp);
    vstStacks[i] = vStack;
  }
}

VersionSt *SSA::CreateNewVersion(VersionSt &vSym, BB &defBB) {
  if (!runRenameOnly) {
    CHECK_FATAL(vSym.GetVersion() == 0, "rename before?");
  }
  // volatile variables will keep zero version.
  const OriginalSt *oSt = vSym.GetOst();
  if (oSt->IsVolatile() || oSt->IsSpecialPreg()) {
    return &vSym;
  }
  VersionSt *newVersionSym = nullptr;
  if (!runRenameOnly) {
    ASSERT(vSym.GetVersion() == kInitVersion, "renamed before");
    newVersionSym = ssaTab->GetVersionStTable().CreateNextVersionSt(vSym.GetOst());
  } else {
    newVersionSym = &vSym;
  }
  vstStacks[vSym.GetOrigIdx()]->push(newVersionSym);
  newVersionSym->SetDefBB(&defBB);
  return newVersionSym;
}

void SSA::RenamePhi(BB &bb) {
  for (auto phiIt = bb.GetPhiList().begin(); phiIt != bb.GetPhiList().end(); ++phiIt) {
    VersionSt *vSym = (*phiIt).second.GetResult();

    VersionSt *newVersionSym = CreateNewVersion(*vSym, bb);
    (*phiIt).second.SetResult(*newVersionSym);
    newVersionSym->SetDefType(VersionSt::kPhi);
    newVersionSym->SetPhi(&(*phiIt).second);
  }
}

void SSA::RenameDefs(StmtNode &stmt, BB &defBB) {
  Opcode opcode = stmt.GetOpCode();
  AccessSSANodes *theSSAPart = ssaTab->GetStmtsSSAPart().SSAPartOf(stmt);
  if (kOpcodeInfo.AssignActualVar(opcode)) {
    VersionSt *newVersionSym = CreateNewVersion(*theSSAPart->GetSSAVar(), defBB);
    newVersionSym->SetDefType(VersionSt::kAssign);
    newVersionSym->SetAssignNode(&stmt);
    theSSAPart->SetSSAVar(*newVersionSym);
  }
  if (kOpcodeInfo.HasSSADef(opcode) && opcode != OP_regassign) {
    TypeOfMayDefList &mayDefList = theSSAPart->GetMayDefNodes();
    for (auto it = mayDefList.begin(); it != mayDefList.end(); ++it) {
      MayDefNode &mayDef = it->second;
      VersionSt *vSym = mayDef.GetResult();
      CHECK_FATAL(vSym->GetOrigIdx() < vstStacks.size(), "index out of range in SSA::RenameMayDefs");
      mayDef.SetOpnd(vstStacks[vSym->GetOrigIdx()]->top());
      VersionSt *newVersionSym = CreateNewVersion(*vSym, defBB);
      mayDef.SetResult(newVersionSym);
      newVersionSym->SetDefType(VersionSt::kMayDef);
      newVersionSym->SetMayDef(&mayDef);
      if (opcode == OP_iassign && mayDef.base != nullptr) {
        mayDef.base = vstStacks[mayDef.base->GetOrigIdx()]->top();
      }
    }
  }
}

void SSA::RenameMustDefs(const StmtNode &stmt, BB &defBB) {
  Opcode opcode = stmt.GetOpCode();
  if (kOpcodeInfo.IsCallAssigned(opcode)) {
    MapleVector<MustDefNode> &mustDefs = ssaTab->GetStmtsSSAPart().GetMustDefNodesOf(stmt);
    for (MustDefNode &mustDefNode : mustDefs) {
      VersionSt *newVersionSym = CreateNewVersion(*mustDefNode.GetResult(), defBB);
      mustDefNode.SetResult(newVersionSym);
      newVersionSym->SetDefType(VersionSt::kMustDef);
      newVersionSym->SetMustDef(&(mustDefNode));
    }
  }
}

void SSA::RenameMayUses(const BaseNode &node) {
  TypeOfMayUseList &mayUseList = ssaTab->GetStmtsSSAPart().GetMayUseNodesOf(static_cast<const StmtNode&>(node));
  auto it = mayUseList.begin();
  for (; it != mayUseList.end(); ++it) {
    MayUseNode &mayUse = it->second;
    VersionSt *vSym = mayUse.GetOpnd();
    CHECK_FATAL(vSym->GetOrigIdx() < vstStacks.size(), "index out of range in SSA::RenameMayUses");
    mayUse.SetOpnd(vstStacks.at(vSym->GetOrigIdx())->top());
  }
}

void SSA::RenameExpr(BaseNode &expr) {
  if (expr.IsSSANode()) {
    auto &ssaNode = static_cast<SSANode&>(expr);
    VersionSt *vSym = ssaNode.GetSSAVar();
    CHECK_FATAL(vSym->GetOrigIdx() < vstStacks.size(), "index out of range in SSA::RenameExpr");
    ssaNode.SetSSAVar(*vstStacks[vSym->GetOrigIdx()]->top());
  }
  for (size_t i = 0; i < expr.NumOpnds(); ++i) {
    RenameExpr(*expr.Opnd(i));
  }
}

void SSA::RenameUses(StmtNode &stmt) {
  if (kOpcodeInfo.HasSSAUse(stmt.GetOpCode())) {
    RenameMayUses(stmt);
  }
  for (size_t i = 0; i < stmt.NumOpnds(); ++i) {
    RenameExpr(*stmt.Opnd(i));
  }
}

void SSA::RenamePhiUseInSucc(const BB &bb) {
  for (BB *succBB : bb.GetSucc()) {
    // find index of bb in succ_bb->pred[]
    size_t index = 0;
    while (index < succBB->GetPred().size()) {
      if (succBB->GetPred(index) == &bb) {
        break;
      }
      index++;
    }
    CHECK_FATAL(index < succBB->GetPred().size(), "RenamePhiUseInSucc: cannot find corresponding pred");
    // rename the phiOpnds[index] in all the phis in succ_bb
    for (auto phiIt = succBB->GetPhiList().begin(); phiIt != succBB->GetPhiList().end(); ++phiIt) {
      PhiNode &phiNode = phiIt->second;
      CHECK_FATAL(phiNode.GetPhiOpnd(index)->GetOrigIdx() < vstStacks.size(),
                  "out of range SSA::RenamePhiUseInSucc");
      phiNode.SetPhiOpnd(index, *vstStacks.at(phiNode.GetPhiOpnd(index)->GetOrigIdx())->top());
    }
  }
}

void SSA::RenameBB(BB &bb) {
  if (GetBBRenamed(bb.GetBBId())) {
    return;
  }

  SetBBRenamed(bb.GetBBId(), true);

  // record stack size for variable versions before processing rename. It is used for stack pop up.
  std::vector<uint32> oriStackSize(GetVstStacks().size());
  for (size_t i = 1; i < GetVstStacks().size(); ++i) {
    if (GetVstStacks()[i] == nullptr) {
      continue;
    }
    oriStackSize[i] = static_cast<uint32>(GetVstStack(i)->size());
  }
  RenamePhi(bb);
  for (auto &stmt : bb.GetStmtNodes()) {
    RenameUses(stmt);
    RenameDefs(stmt, bb);
    RenameMustDefs(stmt, bb);
  }
  RenamePhiUseInSucc(bb);
  // Rename child in Dominator Tree.
  ASSERT(bb.GetBBId() < dom->GetDomChildrenSize(), "index out of range in MeSSA::RenameBB");
  const auto &children = dom->GetDomChildren(bb.GetBBId());
  for (const BBId &child : children) {
    RenameBB(*bbVec[child]);
  }
  for (size_t i = 1; i < GetVstStacks().size(); ++i) {
    if (GetVstStacks()[i] == nullptr) {
      continue;
    }
    while (GetVstStack(i)->size() > oriStackSize[i]) {
      PopVersionSt(i);
    }
  }
}

void PhiNode::Dump() {
  GetResult()->Dump();
  LogInfo::MapleLogger() << " = PHI(";
  for (size_t i = 0; i < GetPhiOpnds().size(); ++i) {
    GetPhiOpnd(i)->Dump();
    if (i < GetPhiOpnds().size() - 1) {
      LogInfo::MapleLogger() << ',';
    }
  }
  LogInfo::MapleLogger() << ")" << '\n';
}
}  // namespace maple
