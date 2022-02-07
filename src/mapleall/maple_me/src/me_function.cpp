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
#include "pme_mir_lower.h"
#include "me_ssa_update.h"

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
  (void)cf.Simplify(mirFunc->GetBody());
  if (mirModule.IsJavaModule() && (!mirFunc->GetInfoVector().empty())) {
    std::string string("INFO_registers");
    GStrIdx strIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(string);
    regNum = mirFunc->GetInfo(strIdx);
    std::string tryNum("INFO_tries_size");
    strIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(tryNum);
    uint32 num = mirFunc->GetInfo(strIdx);
    hasEH = (num != 0);
  }
}

void MeFunction::DumpFunction() const {
  if (theCFG == nullptr) {
    mirFunc->Dump(false);
    return;
  }
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
  if (isLfo) {
    return;
  }
  if (theCFG == nullptr) {
    LogInfo::MapleLogger()  << "Warning: no cfg analysis, just dump ir" << '\n';
    mirFunc->Dump();
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

void MeFunction::Prepare() {
  if (MeOption::optLevel >= 3) {
    MemPool* pmemp = memPoolCtrler.NewMemPool("lfo", true);
    SetPreMeFunc(pmemp->New<PreMeFunction>(pmemp, this));
    SetLfoMempool(pmemp);
    PreMeMIRLower pmemirlowerer(mirModule, this);
    pmemirlowerer.LowerFunc(*CurFunction());
  } else {
    /* lower first */
    MIRLower mirLowerer(mirModule, CurFunction());
    mirLowerer.Init();
    mirLowerer.SetLowerME();
    mirLowerer.SetOptLevel(MeOption::optLevel);
    if (mirModule.IsJavaModule()) {
      mirLowerer.SetLowerExpandArray();
    }
    ASSERT(CurFunction() != nullptr, "nullptr check");
    mirLowerer.LowerFunc(*CurFunction());
  }
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

namespace {
// Clone ChiList from srcStmt to destStmt
void CloneChiListOfStmt(MeStmt &srcStmt, MeStmt &destStmt, MeIRMap &irmap,
                        std::map<OStIdx, std::unique_ptr<std::set<BBId>>> *ssaCands = nullptr) {
  const MapleMap<OStIdx, ChiMeNode*> *srcChiList = srcStmt.GetChiList();
  MapleMap<OStIdx, ChiMeNode*> *destChiList = destStmt.GetChiList();
  if (srcChiList == nullptr || destChiList == nullptr || !destChiList->empty()) {
    return;
  }
  const BB &newBB = *destStmt.GetBB();
  for (auto &chiNode : *srcChiList) {
    CHECK_FATAL(chiNode.first == chiNode.second->GetLHS()->GetOstIdx(), "must be");
    VarMeExpr *newMul = irmap.CreateVarMeExprVersion(chiNode.second->GetLHS()->GetOst());
    MeSSAUpdate::InsertOstToSSACands(newMul->GetOstIdx(), newBB, ssaCands);
    auto *newChiNode = irmap.New<ChiMeNode>(&destStmt);
    newMul->SetDefChi(*newChiNode);
    newMul->SetDefBy(kDefByChi);
    newChiNode->SetLHS(newMul);
    newChiNode->SetRHS(chiNode.second->GetRHS());
    destChiList->emplace(chiNode.first, newChiNode);
  }
}

// Clone MustDefList from srcStmt to destStmt
void CloneMustDefListOfStmt(MeStmt &srcStmt, MeStmt &destStmt, MeIRMap &irmap,
                            std::map<OStIdx, std::unique_ptr<std::set<BBId>>> *ssaCands = nullptr) {
  const MapleVector<MustDefMeNode> *srcMustDef = srcStmt.GetMustDefList();
  MapleVector<MustDefMeNode> *destMustDef = destStmt.GetMustDefList();
  if (srcMustDef == nullptr || destMustDef == nullptr || !destMustDef->empty()) {
    return;
  }
  const BB &newBB = *destStmt.GetBB();
  for (auto &mustDefNode : *srcMustDef) {
    const ScalarMeExpr *oldLHS = mustDefNode.GetLHS();
    ScalarMeExpr *newLHS = irmap.CreateRegOrVarMeExprVersion(oldLHS->GetOstIdx());
    newLHS->SetDefBy(kDefByMustDef);
    MeSSAUpdate::InsertOstToSSACands(newLHS->GetOstIdx(), newBB, ssaCands);
    auto *mustDef = irmap.New<MustDefMeNode>(newLHS, &destStmt);
    newLHS->SetDefMustDef(*mustDef);
    destMustDef->push_back(*mustDef);
  }
}
} // anonymous namespace

// clone MeStmts from srcBB to destBB, and collect new version to ssaCands which is used for ssa-updater
// if ssaCands is nullptr, no new version will be collected.
void MeFunction::CloneBBMeStmts(BB &srcBB, BB &destBB, std::map<OStIdx, std::unique_ptr<std::set<BBId>>> *ssaCands,
                                bool copyWithoutLastMe) {
  if (irmap == nullptr) {
    return;
  }
  for (auto &stmt : srcBB.GetMeStmts()) {
    if (copyWithoutLastMe && &stmt == srcBB.GetLastMe()) {
      break;
    }
    MeStmt *newStmt = nullptr;
    switch (stmt.GetOp()) {
      case OP_dassign:
      case OP_regassign: {
        ScalarMeExpr *scalarLHS = stmt.GetLHS();
        ScalarMeExpr *newVerLHS = irmap->CreateRegOrVarMeExprVersion(scalarLHS->GetOstIdx());
        MeSSAUpdate::InsertOstToSSACands(newVerLHS->GetOstIdx(), destBB, ssaCands);
        newStmt = irmap->CreateAssignMeStmt(*newVerLHS, *stmt.GetRHS(), destBB);
        destBB.AddMeStmtLast(newStmt);
        break;
      }
      case OP_iassign: {
        auto *iassStmt = static_cast<IassignMeStmt*>(&stmt);
        IvarMeExpr *ivar = irmap->BuildLHSIvarFromIassMeStmt(*iassStmt);
        newStmt = irmap->NewInPool<IassignMeStmt>(
            iassStmt->GetTyIdx(), *static_cast<IvarMeExpr*>(ivar), *iassStmt->GetRHS());
        destBB.AddMeStmtLast(newStmt);
        break;
      }
      case OP_maydassign: {
        auto &maydassStmt = static_cast<MaydassignMeStmt&>(stmt);
        newStmt = irmap->NewInPool<MaydassignMeStmt>(maydassStmt);
        destBB.AddMeStmtLast(newStmt);
        break;
      }
      case OP_goto: {
        auto &gotoStmt = static_cast<GotoMeStmt&>(stmt);
        newStmt = irmap->New<GotoMeStmt>(gotoStmt);
        destBB.AddMeStmtLast(newStmt);
        break;
      }
      case OP_brfalse:
      case OP_brtrue: {
        auto &condGotoStmt = static_cast<CondGotoMeStmt&>(stmt);
        newStmt = irmap->New<CondGotoMeStmt>(condGotoStmt);
        destBB.AddMeStmtLast(newStmt);
        break;
      }
      case OP_intrinsiccall:
      case OP_intrinsiccallassigned:
      case OP_intrinsiccallwithtype: {
        auto *intrnStmt = static_cast<IntrinsiccallMeStmt*>(&stmt);
        newStmt =
            irmap->NewInPool<IntrinsiccallMeStmt>(static_cast<const NaryMeStmt*>(intrnStmt), intrnStmt->GetIntrinsic(),
                                                  intrnStmt->GetTyIdx(), intrnStmt->GetReturnPrimType());
        for (auto &mu : *intrnStmt->GetMuList()) {
          newStmt->GetMuList()->emplace(mu.first, mu.second);
        }
        destBB.AddMeStmtLast(newStmt);
        break;
      }
      case OP_icall:
      case OP_icallassigned: {
        auto *icallStmt = static_cast<IcallMeStmt*>(&stmt);
        newStmt = irmap->NewInPool<IcallMeStmt>(static_cast<NaryMeStmt*>(icallStmt),
                                                icallStmt->GetRetTyIdx(), icallStmt->GetStmtID());
        for (auto &mu : *icallStmt->GetMuList()) {
          newStmt->GetMuList()->emplace(mu.first, mu.second);
        }
        destBB.AddMeStmtLast(newStmt);
        break;
      }
      case OP_asm:{
        auto *asmStmt = static_cast<AsmMeStmt*>(&stmt);
        newStmt = irmap->NewInPool<AsmMeStmt>(asmStmt);
        for (auto &mu : *asmStmt->GetMuList()) {
          newStmt->GetMuList()->emplace(mu.first, mu.second);
        }
        destBB.AddMeStmtLast(newStmt);
        break;
      }
      case OP_call:
      case OP_callassigned:
      case OP_virtualcallassigned:
      case OP_virtualicallassigned:
      case OP_interfaceicallassigned: {
        auto *callStmt = static_cast<CallMeStmt*>(&stmt);
        newStmt =
            irmap->NewInPool<CallMeStmt>(static_cast<NaryMeStmt*>(callStmt), callStmt->GetPUIdx());
        for (auto &mu : *callStmt->GetMuList()) {
          newStmt->GetMuList()->emplace(mu.first, mu.second);
        }
        destBB.AddMeStmtLast(newStmt);
        break;
      }
      case OP_returnassertnonnull:
      case OP_assertnonnull:
      case OP_assignassertnonnull: {
        auto &unaryStmt = static_cast<AssertNonnullMeStmt&>(stmt);
        newStmt = irmap->New<AssertNonnullMeStmt>(unaryStmt);
        destBB.AddMeStmtLast(newStmt);
        break;
      }
      case OP_callassertnonnull: {
        auto &callAssertStmt = static_cast<CallAssertNonnullMeStmt&>(stmt);
        newStmt = irmap->New<CallAssertNonnullMeStmt>(callAssertStmt);
        destBB.AddMeStmtLast(newStmt);
        break;
      }
      case OP_membaracquire:
      case OP_membarrelease:
      case OP_membarstoreload:
      case OP_membarstorestore: {
        newStmt = irmap->New<MeStmt>(stmt.GetOp());
        destBB.AddMeStmtLast(newStmt);
        break;
      }
      case OP_eval: {
        newStmt = irmap->New<UnaryMeStmt>(static_cast<UnaryMeStmt&>(stmt));
        destBB.AddMeStmtLast(newStmt);
        break;
      }
      case OP_comment: {
        break;
      }
      case OP_callassertle:{
        auto &oldStmt = static_cast<CallAssertBoundaryMeStmt&>(stmt);
        newStmt = irmap->New<CallAssertBoundaryMeStmt>(oldStmt);
        destBB.AddMeStmtLast(newStmt);
        break;
      }
      case OP_calcassertge:
      case OP_calcassertlt:
      case OP_assignassertle:
      case OP_assertlt:
      case OP_assertge:
      case OP_returnassertle: {
        auto &oldStmt = static_cast<AssertBoundaryMeStmt&>(stmt);
        newStmt = irmap->New<AssertBoundaryMeStmt>(oldStmt);
        destBB.AddMeStmtLast(newStmt);
        break;
      }
      default:
        LogInfo::MapleLogger() << "stmt with op :"<< stmt.GetOp() << " is not implemented yet\n";
        CHECK_FATAL(false, "NYI");
        break;
    }
    if (stmt.GetChiList() != nullptr) {
      CloneChiListOfStmt(stmt, *newStmt, *irmap, ssaCands);
    }
    if (stmt.GetMustDefList() != nullptr) {
      CloneMustDefListOfStmt(stmt, *newStmt, *irmap, ssaCands);
    }
    if (newStmt != nullptr) {
      newStmt->CopyInfo(stmt);
    }
  }
}
}  // namespace maple
