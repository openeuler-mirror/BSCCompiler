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
#include "me_cfg.h"

namespace maple {
bool IsLocalTopLevelOst(const OriginalSt &ost) {
  if (ost.IsVolatile()) {
    return false;
  }
  if (ost.IsSymbolOst()) {
    MIRSymbol *sym = ost.GetMIRSymbol();
    // global/local static ost can be modified whenever an assign to it appear.
    if (sym->GetStorageClass() == kScPstatic || sym->GetStorageClass() == kScFstatic) {
      return false;
    }
    // sym is agg, ost may alias with other ost
    if (sym->GetType()->GetPrimType() == PTY_agg) {
      return false;
    }
  } else {
    ASSERT(ost.IsPregOst(), "OriginalSt can only be symbol or preg");
    if (ost.GetPregIdx() < 0) {
      return false;
    }
  }
  return (ost.IsLocal() && !ost.IsAddressTaken() && ost.GetIndirectLev() == 0);
}

void SSA::InsertPhiForDefBB(size_t bbid, VersionSt *vst) {
  for (BBId dfBBId : dom->GetDomFrontier(bbid)) {
    BB *phiBB = bbVec[dfBBId];
    CHECK_FATAL(phiBB != nullptr, "MeSSA::InsertPhiNode: non-existent BB for definition");
    auto successTag = phiBB->InsertPhi(&ssaTab->GetVersAlloc(), vst);
    if (!successTag) {
      continue;
    }
    InsertPhiForDefBB(dfBBId, vst);
  }
}

void SSA::InsertPhiNode() {
  for (size_t i = 1; i < ssaTab->GetOriginalStTable().Size(); ++i) {
    OriginalSt *ost = ssaTab->GetOriginalStFromID(OStIdx(i));
    if (ost->GetIndirectLev() < 0) {
      continue;
    }
    if (!ssaTab->HasDefBB(ost->GetIndex())) {
      continue;
    }
    if (ost->IsVolatile()) { // volatile variables will not have ssa form.
      continue;
    }
    if (!ShouldProcessOst(*ost)) {
      continue;
    }
    VersionSt *vst = ssaTab->GetVersionStTable().GetZeroVersionSt(ost);
    for (BBId bbid : *ssaTab->GetDefBBs4Ost(ost->GetIndex())) {
      InsertPhiForDefBB(bbid, vst);
    }
  }
}

void SSA::InitRenameStack(const OriginalStTable &oTable, const VersionStTable &verStTab, MapleAllocator &alloc) {
  vstStacks = alloc.New<MapleVector<MapleStack<VersionSt*>*>>(alloc.Adapter());
  vstStacks->resize(oTable.Size());
  for (size_t i = 1; i < oTable.Size(); ++i) {
    const OriginalSt *ost = oTable.GetOriginalStFromID(OStIdx(i));
    if (ost->GetIndirectLev() < 0) {
      continue;
    }
    MapleStack<VersionSt*> *vStack = alloc.New<MapleStack<VersionSt*>>(alloc.Adapter());
    VersionSt *temp = verStTab.GetVersionStVectorItem(ost->GetZeroVersionIndex());
    vStack->push(temp);
    (*vstStacks)[i] = vStack;
  }
}

void SSA::PushToRenameStack(VersionSt *vSym) {
  if (vSym == nullptr || vSym->IsInitVersion()) {
    return;
  }
  GetVstStacks()[vSym->GetOrigIdx()]->push(vSym);
}

VersionSt *SSA::CreateNewVersion(VersionSt &vSym, BB &defBB) {
  // volatile variables will keep zero version.
  const OriginalSt *oSt = vSym.GetOst();
  if (oSt->IsVolatile() || oSt->IsSpecialPreg()) {
    return &vSym;
  }
  VersionSt *newVersionSym = nullptr;
  if (!runRenameOnly) {
    newVersionSym = ssaTab->GetVersionStTable().CreateNextVersionSt(vSym.GetOst());
  } else {
    newVersionSym = &vSym;
  }
  GetVstStacks()[vSym.GetOrigIdx()]->push(newVersionSym);
  newVersionSym->SetDefBB(&defBB);
  return newVersionSym;
}

void SSA::RenamePhi(BB &bb) {
  for (auto phiIt = bb.GetPhiList().begin(); phiIt != bb.GetPhiList().end(); ++phiIt) {
    VersionSt *vSym = phiIt->second.GetResult();
    if (!ShouldRenameVst(vSym)) {
      PushToRenameStack(vSym); // no need to create next version, just use the vst rename before.
      continue;
    }
    VersionSt *newVersionSym = CreateNewVersion(*vSym, bb);
    phiIt->second.SetResult(*newVersionSym);
    newVersionSym->SetDefType(VersionSt::kPhi);
    newVersionSym->SetPhi(&(*phiIt).second);
  }
}

void SSA::RenameDefs(StmtNode &stmt, BB &defBB) {
  Opcode opcode = stmt.GetOpCode();
  AccessSSANodes *theSSAPart = ssaTab->GetStmtsSSAPart().SSAPartOf(stmt);
  if (kOpcodeInfo.AssignActualVar(opcode)) {
    VersionSt *vSym = theSSAPart->GetSSAVar();
    if (ShouldRenameVst(vSym)) {
      VersionSt *newVersionSym = CreateNewVersion(*vSym, defBB);
      newVersionSym->SetDefType(VersionSt::kAssign);
      newVersionSym->SetAssignNode(&stmt);
      theSSAPart->SetSSAVar(*newVersionSym);
    } else {
      PushToRenameStack(vSym);
    }
  }
  if (kOpcodeInfo.HasSSADef(opcode) && opcode != OP_regassign) {
    TypeOfMayDefList &mayDefList = theSSAPart->GetMayDefNodes();
    for (auto it = mayDefList.begin(); it != mayDefList.end(); ++it) {
      MayDefNode &mayDef = it->second;
      VersionSt *vSym = mayDef.GetResult();
      CHECK_FATAL(vSym->GetOrigIdx() < GetVstStacks().size(), "index out of range in SSA::RenameMayDefs");
      if (!ShouldRenameVst(vSym)) { // maydassign will insert maydefNode, if it is top-level, we should process it.
        PushToRenameStack(vSym);
        continue;
      }
      mayDef.SetOpnd(GetVstStacks()[vSym->GetOrigIdx()]->top());
      VersionSt *newVersionSym = CreateNewVersion(*vSym, defBB);
      mayDef.SetResult(newVersionSym);
      newVersionSym->SetDefType(VersionSt::kMayDef);
      newVersionSym->SetMayDef(&mayDef);
      if (opcode == OP_iassign && mayDef.base != nullptr) {
        mayDef.base = GetVstStacks()[mayDef.base->GetOrigIdx()]->top();
      }
    }
  }
}

void SSA::RenameMustDefs(const StmtNode &stmt, BB &defBB) {
  Opcode opcode = stmt.GetOpCode();
  if (kOpcodeInfo.IsCallAssigned(opcode)) {
    MapleVector<MustDefNode> &mustDefs = ssaTab->GetStmtsSSAPart().GetMustDefNodesOf(stmt);
    for (MustDefNode &mustDefNode : mustDefs) {
      VersionSt *vSym = mustDefNode.GetResult();
      if (!ShouldRenameVst(vSym)) {
        PushToRenameStack(vSym);
        continue;
      }
      VersionSt *newVersionSym = CreateNewVersion(*vSym, defBB);
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
    CHECK_FATAL(vSym->GetOrigIdx() < GetVstStacks().size(), "index out of range in SSA::RenameMayUses");
    mayUse.SetOpnd(GetVstStacks().at(vSym->GetOrigIdx())->top());
  }
}

void SSA::RenameExpr(BaseNode &expr) {
  if (expr.IsSSANode()) {
    auto &ssaNode = static_cast<SSANode&>(expr);
    VersionSt *vSym = ssaNode.GetSSAVar();
    if (ShouldRenameVst(vSym)) {
      CHECK_FATAL(vSym->GetOrigIdx() < GetVstStacks().size(), "index out of range in SSA::RenameExpr");
      ssaNode.SetSSAVar(*GetVstStacks()[vSym->GetOrigIdx()]->top());
    }
  }
  for (size_t i = 0; i < expr.NumOpnds(); ++i) {
    RenameExpr(*expr.Opnd(i));
  }
}

void SSA::RenameUses(const StmtNode &stmt) {
  if (BuildSSAAddrTaken() && kOpcodeInfo.HasSSAUse(stmt.GetOpCode())) {
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
      VersionSt *vSym = phiNode.GetPhiOpnd(index);
      CHECK_FATAL(vSym->GetOrigIdx() < GetVstStacks().size(), "out of range SSA::RenamePhiUseInSucc");
      if (!ShouldRenameVst(vSym)) {
        continue;
      }
      phiNode.SetPhiOpnd(index, *GetVstStacks().at(phiNode.GetPhiOpnd(index)->GetOrigIdx())->top());
    }
  }
}

void SSA::RenameAllBBs(MeCFG *cfg) {
  // renameMP is a tmp mempool, will be release after rename.
  auto renameMP = std::make_unique<ThreadLocalMemPool>(memPoolCtrler, "ssa-rename-mempool");
  MapleAllocator renameAlloc(renameMP.get());
  InitRenameStack(ssaTab->GetOriginalStTable(), ssaTab->GetVersionStTable(), renameAlloc);
  // recurse down dominator tree in pre-order traversal
  auto *children = &dom->domChildren[cfg->GetCommonEntryBB()->GetBBId()];
  for (BBId child : *children) {
    RenameBB(*cfg->GetBBFromID(child));
  }
  vstStacks = nullptr;
}

void SSA::RenameBB(BB &bb) {
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

// Check if ost should be processed according to target ssa level set before
bool SSA::ShouldProcessOst(const OriginalSt &ost) const {
  // for memory ssa, all ost should be processed
  if (BuildSSAAllLevel()) {
    return true;
  }
  // for local ssa, check if ost is local top-level
  if (BuildSSATopLevel()) {
    return IsLocalTopLevelOst(ost);
  }
  // for addr-taken ssa, check if ost is non local top-level
  if (BuildSSAAddrTaken()) {
    return !IsLocalTopLevelOst(ost);
  }
  return false;
}

bool SSA::ShouldRenameVst(const VersionSt *vst) const {
  if (vst == nullptr) {
    return false;
  }
  if (runRenameOnly) {
    return true;
  }
  return ShouldProcessOst(*vst->GetOst());
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
