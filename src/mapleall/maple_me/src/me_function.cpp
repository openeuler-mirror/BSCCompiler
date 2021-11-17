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
#include "lfo_mir_lower.h"

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
  cf.Simplify(mirFunc->GetBody());
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
    MemPool* lfomp = memPoolCtrler.NewMemPool("lfo", true);
    SetLfoFunc(lfomp->New<LfoFunction>(lfomp, this));
    SetLfoMempool(lfomp);
    LFOMIRLower lfomirlowerer(mirModule, this);
    lfomirlowerer.LowerFunc(*CurFunction());
  } else {
    /* lower first */
    MIRLower mirLowerer(mirModule, CurFunction());
    mirLowerer.Init();
    mirLowerer.SetLowerME();
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
}  // namespace maple
