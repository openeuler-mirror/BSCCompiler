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
#include "me_ssa.h"
#include <iostream>
#include "bb.h"
#include "me_option.h"
#include "ssa_mir_nodes.h"
#include "ver_symbol.h"
#include "dominance.h"
#include "me_function.h"
#include "mir_builder.h"
#include "me_ssa_tab.h"
#include "me_phase_manager.h"

// This phase builds the SSA form of a function. Before this we have got the dominator tree
// and each bb's dominance frontiers. Then the algorithm follows this outline:
//
// Step 1: Inserting phi-node.
// With dominance frontiers, we can determine more
// precisely where phi-node might be needed. The basic idea is simple.
// A definition of x in block b forces a phi-node at every node in b's
// dominance frontiers. Since that phi-node is a new definition of x,
// it may, in turn, force the insertion of additional phi-node.
//
// Step 2: Renaming.
// Renames both definitions and uses of each symbol in
// a preorder walk over the dominator tree. In each block, we first
// rename the values defined by phi-node at the head of the block, then
// visit each stmt in the block: we rewrite each uses of a symbol with current
// SSA names(top of the stack which holds the current SSA version of the corresponding symbol),
// then it creates a new SSA name for the result of the stmt.
// This latter act makes the new name current. After all the stmts in the
// block have been rewritten, we rewrite the appropriate phi-node's
// parameters in each cfg successor of the block, using the current SSA names.
// Finally, it recurs on any children of the block in the dominator tree. When it
// returns from those recursive calls, we restores the stack of current SSA names to
// the state that existed before the current block was visited.
namespace maple {
void MeSSA::InsertPhiNode() {
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
    std::set<BBId> phibbs;
    for (BBId bbid : *ssaTab->GetDefBBs4Ost(ost->GetIndex())) {
      phibbs.insert(dom->iterDomFrontier[bbid].begin(), dom->iterDomFrontier[bbid].end());
    }
    VersionSt *vst = ssaTab->GetVersionStTable().GetZeroVersionSt(ost);
    for (BBId bbid : phibbs) {
      BB *phiBB = bbVec[bbid];
      CHECK_FATAL(phiBB != nullptr, "MeSSA::InsertPhiNode: non-existent BB for definition");
      phiBB->InsertPhi(&func->GetMeSSATab()->GetVersAlloc(), vst);
    }
  }
}

void MeSSA::VerifySSAOpnd(const BaseNode &node) const {
  Opcode op = node.GetOpCode();
  size_t vtableSize = func->GetMeSSATab()->GetVersionStTable().GetVersionStVectorSize();
  if (op == OP_dread || op == OP_addrof) {
    const auto &addrOfSSANode = static_cast<const AddrofSSANode&>(node);
    const VersionSt *verSt = addrOfSSANode.GetSSAVar();
    CHECK_FATAL(verSt->GetIndex() < vtableSize, "runtime check error");
    return;
  }
  if (op == OP_regread) {
    const auto &regNode = static_cast<const RegreadSSANode&>(node);
    const VersionSt *verSt = regNode.GetSSAVar();
    CHECK_FATAL(verSt->GetIndex() < vtableSize, "runtime check error");
    return;
  }

  for (size_t i = 0; i < node.NumOpnds(); ++i) {
    VerifySSAOpnd(*node.Opnd(i));
  }
}

void MeSSA::VerifySSA() const {
  size_t vtableSize = func->GetMeSSATab()->GetVersionStTable().GetVersionStVectorSize();
  auto cfg = func->GetCfg();
  // to prevent valid_end from being called repeatedly, don't modify the definition of eIt
  for (auto bIt = cfg->valid_begin(), eIt = cfg->valid_end(); bIt != eIt; ++bIt) {
    auto *bb = *bIt;
    Opcode opcode;
    for (auto &stmt : bb->GetStmtNodes()) {
      opcode = stmt.GetOpCode();
      if (opcode == OP_dassign || opcode == OP_regassign) {
        VersionSt *verSt = func->GetMeSSATab()->GetStmtsSSAPart().GetAssignedVarOf(stmt);
        CHECK_FATAL(verSt != nullptr && verSt->GetIndex() < vtableSize, "runtime check error");
      }
      for (size_t i = 0; i < stmt.NumOpnds(); ++i) {
        VerifySSAOpnd(*stmt.Opnd(i));
      }
    }
  }
}

void MeSSA::InsertIdentifyAssignments(IdentifyLoops *identloops) {
  MIRBuilder *mirbuilder = func->GetMIRModule().GetMIRBuilder();
  LfoFunction *lfoFunc = func->GetLfoFunc();
  SSATab *ssatab = func->GetMeSSATab();

  for (LoopDesc *aloop : identloops->GetMeLoops()) {
    BB *headbb = aloop->head;
    // check if the label has associated LfoWhileInfo
    if (headbb->GetBBLabel() == 0) {
      continue;
    }
    if (aloop->exitBB == nullptr) {
      continue;
    }
    MapleMap<LabelIdx, LfoWhileInfo*>::iterator it = lfoFunc->label2WhileInfo.find(headbb->GetBBLabel());
    if (it == lfoFunc->label2WhileInfo.end()) {
      continue;
    }
    if (headbb->GetPred().size() != 2) {
      continue;
    }
    // collect the symbols for inserting identity assignments
    std::set<OriginalSt *> ostSet;
    for (auto& mapEntry: (headbb->GetPhiList())) {
      OriginalSt *ost = func->GetMeSSATab()->GetOriginalStFromID(mapEntry.first);
      if (ost->IsIVCandidate()) {
        ostSet.insert(ost);
      }
    }
    if (ostSet.empty()) {
      continue;
    }
    // for the exitBB, insert identify assignment for any var that has phi at
    // headbb
    for (OriginalSt *ost : ostSet) {
      MIRType *mirtype = GlobalTables::GetTypeTable().GetTypeFromTyIdx(ost->GetTyIdx());
      if (ost->IsSymbolOst()) {
        AddrofNode *dread = func->GetMirFunc()->GetCodeMempool()->New<AddrofNode>(OP_dread,
            mirtype->GetPrimType(), ost->GetMIRSymbol()->GetStIdx(), ost->GetFieldID());
        AddrofSSANode *ssadread = func->GetMirFunc()->GetCodeMempool()->New<AddrofSSANode>(*dread);
        ssadread->SetSSAVar(*ssatab->GetVersionStTable().GetZeroVersionSt(ost));

        DassignNode *dass = mirbuilder->CreateStmtDassign(*ost->GetMIRSymbol(), ost->GetFieldID(), ssadread);
        aloop->exitBB->PrependStmtNode(dass);

        MayDefPartWithVersionSt *thessapart = ssatab->GetStmtsSSAPart().GetSSAPartMp()->New<MayDefPartWithVersionSt>(
            &ssatab->GetStmtsSSAPart().GetSSAPartAlloc());
        ssatab->GetStmtsSSAPart().SetSSAPartOf(*dass, thessapart);
        thessapart->SetSSAVar(*ssatab->GetVersionStTable().GetZeroVersionSt(ost));
      } else {
        RegreadNode *regread = func->GetMirFunc()->GetCodeMempool()->New<RegreadNode>(ost->GetPregIdx());
        MIRPreg *preg = func->GetMirFunc()->GetPregTab()->PregFromPregIdx(ost->GetPregIdx());
        regread->SetPrimType(preg->GetPrimType());
        RegreadSSANode *ssaregread = func->GetMirFunc()->GetCodeMempool()->New<RegreadSSANode>(*regread);
        ssaregread->SetSSAVar(*ssatab->GetVersionStTable().GetZeroVersionSt(ost));

        RegassignNode *rass = mirbuilder->CreateStmtRegassign(mirtype->GetPrimType(), ost->GetPregIdx(), ssaregread);
        aloop->exitBB->PrependStmtNode(rass);

        VersionSt *vst = ssatab->GetVersionStTable().GetZeroVersionSt(ost);
        ssatab->GetStmtsSSAPart().SetSSAPartOf(*rass, vst);
      }
      ssatab->AddDefBB4Ost(ost->GetIndex(), aloop->exitBB->GetBBId());
    }
    if (eDebug) {
      LogInfo::MapleLogger() << "****** Identity assignments inserted at loop exit BB "
                             << aloop->exitBB->GetBBId() << "\n";
    }
  }
}

void MESSA::GetAnalysisDependence(maple::AnalysisDep &aDep) const {
  aDep.AddRequired<MEDominance>();
  aDep.AddRequired<MESSATab>();
  aDep.SetPreservedAll();
}

bool MESSA::PhaseRun(maple::MeFunction &f) {
  auto *dom = GET_ANALYSIS(MEDominance, f);
  CHECK_FATAL(dom != nullptr, "dominance phase has problem");
  auto *ssaTab = GET_ANALYSIS(MESSATab, f);
  CHECK_FATAL(ssaTab != nullptr, "ssaTab phase has problem");
  ssa = GetPhaseAllocator()->New<MeSSA>(f, ssaTab, *dom, *GetPhaseMemPool(), DEBUGFUNC_NEWPM(f));
  auto cfg = f.GetCfg();
  ssa->InsertPhiNode();
  if (f.IsLfo()) {
    IdentifyLoops *identloops = FORCE_GET(MELoopAnalysis);
    CHECK_FATAL(identloops != nullptr, "identloops has problem");
    ssa->InsertIdentifyAssignments(identloops);
  }

  ssa->InitRenameStack(ssaTab->GetOriginalStTable(), cfg->GetAllBBs().size(),
                       ssaTab->GetVersionStTable());
  // recurse down dominator tree in pre-order traversal
  MapleSet<BBId> *children = &dom->domChildren[cfg->GetCommonEntryBB()->GetBBId()];
  for (BBId child : *children) {
    ssa->RenameBB(*cfg->GetBBFromID(child));
  }
  ssa->VerifySSA();
  if (DEBUGFUNC_NEWPM(f)) {
    ssaTab->GetVersionStTable().Dump(&ssaTab->GetModule());
  }
  if (DEBUGFUNC_NEWPM(f)) {
    f.DumpFunction();
  }  return true;
}
}  // namespace maple
