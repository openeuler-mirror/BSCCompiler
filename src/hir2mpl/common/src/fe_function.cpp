/*
 * Copyright (c) [2020-2022] Huawei Technologies Co.,Ltd.All rights reserved.
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
#include "fe_function.h"
#include "feir_bb.h"
#include "mpl_logging.h"
#include "fe_options.h"
#include "fe_manager.h"
#include "feir_var_name.h"
#include "feir_var_reg.h"
#include "hir2mpl_env.h"
#include "feir_builder.h"
#include "feir_dfg.h"
#include "feir_type_helper.h"
#include "feir_var_type_scatter.h"
#include "fe_options.h"

namespace maple {
FEFunction::FEFunction(MIRFunction &argMIRFunction, const std::unique_ptr<FEFunctionPhaseResult> &argPhaseResultTotal)
    : genStmtHead(nullptr),
      genStmtTail(nullptr),
      genBBHead(nullptr),
      genBBTail(nullptr),
      feirStmtHead(nullptr),
      feirStmtTail(nullptr),
      feirBBHead(nullptr),
      feirBBTail(nullptr),
      phaseResult(FEOptions::GetInstance().IsDumpPhaseTimeDetail() || FEOptions::GetInstance().IsDumpPhaseTime()),
      phaseResultTotal(argPhaseResultTotal),
      mirFunction(argMIRFunction) {}

FEFunction::~FEFunction() {
  genStmtHead = nullptr;
  genStmtTail = nullptr;
  genBBHead = nullptr;
  genBBTail = nullptr;
  feirStmtHead = nullptr;
  feirStmtTail = nullptr;
  feirBBHead = nullptr;
  feirBBTail = nullptr;
}

void FEFunction::InitImpl() {
  // feir stmt/bb
  feirStmtHead = RegisterFEIRStmt(std::make_unique<FEIRStmt>(FEIRNodeKind::kStmtPesudoFuncStart));
  feirStmtTail = RegisterFEIRStmt(std::make_unique<FEIRStmt>(FEIRNodeKind::kStmtPesudoFuncEnd));
  feirStmtHead->SetNext(feirStmtTail);
  feirStmtTail->SetPrev(feirStmtHead);
  feirBBHead = RegisterFEIRBB(std::make_unique<FEIRBB>(FEIRBBKind::kBBKindPesudoHead));
  feirBBTail = RegisterFEIRBB(std::make_unique<FEIRBB>(FEIRBBKind::kBBKindPesudoTail));
  feirBBHead->SetNext(feirBBTail);
  feirBBTail->SetPrev(feirBBHead);
}

void FEFunction::AppendFEIRStmts(std::list<UniqueFEIRStmt> &stmts) {
  ASSERT_NOT_NULL(feirStmtTail);
  InsertFEIRStmtsBefore(*feirStmtTail, stmts);
}

void FEFunction::InsertFEIRStmtsBefore(FEIRStmt &pos, std::list<UniqueFEIRStmt> &stmts) {
  while (stmts.size() > 0) {
    FEIRStmt *ptrFEIRStmt = RegisterFEIRStmt(std::move(stmts.front()));
    stmts.pop_front();
    pos.InsertBefore(ptrFEIRStmt);
  }
}

FEIRStmt *FEFunction::RegisterGeneralStmt(std::unique_ptr<FEIRStmt> stmt) {
  genStmtList.push_back(std::move(stmt));
  return genStmtList.back().get();
}

const std::unique_ptr<FEIRStmt> &FEFunction::RegisterGeneralStmtUniqueReturn(std::unique_ptr<FEIRStmt> stmt) {
  genStmtList.push_back(std::move(stmt));
  return genStmtList.back();
}

FEIRStmt *FEFunction::RegisterFEIRStmt(UniqueFEIRStmt stmt) {
  feirStmtList.push_back(std::move(stmt));
  return feirStmtList.back().get();
}

FEIRBB *FEFunction::RegisterFEIRBB(std::unique_ptr<FEIRBB> bb) {
  feirBBList.push_back(std::move(bb));
  return feirBBList.back().get();
}

void FEFunction::DumpGeneralStmts() {
  FELinkListNode *nodeStmt = genStmtHead;
  while (nodeStmt != nullptr) {
    FEIRStmt *stmt = static_cast<FEIRStmt*>(nodeStmt);
    stmt->Dump();
    nodeStmt = nodeStmt->GetNext();
  }
}

bool FEFunction::LowerFunc(const std::string &phaseName) {
  phaseResult.RegisterPhaseNameAndStart(phaseName);
  if (feirLower == nullptr) {
    feirLower = std::make_unique<FEIRLower>(*this);
    feirLower->LowerFunc();
    feirStmtHead = feirLower->GetlowerStmtHead();
    feirStmtTail = feirLower->GetlowerStmtTail();
  }
  return phaseResult.Finish();
}

bool FEFunction::DumpFEIRBBs(const std::string &phaseName) {
  HIR2MPL_PARALLEL_FORBIDDEN();
  phaseResult.RegisterPhaseNameAndStart(phaseName);
  if (feirCFG == nullptr) {
    feirCFG = std::make_unique<FEIRCFG>(feirStmtHead, feirStmtTail);
    feirCFG->GenerateCFG();
  }
  std::cout << "****** CFG built by FEIR for " << GetGeneralFuncName() << " *******\n";
  feirCFG->DumpBBs();
  std::cout << "****** END CFG built for " << GetGeneralFuncName() << " *******\n\n";
  return phaseResult.Finish();
}

bool FEFunction::DumpFEIRCFGGraph(const std::string &phaseName) {
  HIR2MPL_PARALLEL_FORBIDDEN();
  phaseResult.RegisterPhaseNameAndStart(phaseName);
  std::string outName = FEManager::GetModule().GetFileName();
  size_t lastDot = outName.find_last_of(".");
  if (lastDot != std::string::npos) {
    outName = outName.substr(0, lastDot);
  }
  CHECK_FATAL(!outName.empty(), "General CFG Graph FileName is empty");
  std::string fileName = outName + "." + GetGeneralFuncName() + ".dot";
  std::ofstream file(fileName);
  CHECK_FATAL(file.is_open(), "Failed to open General CFG Graph FileName: %s", fileName.c_str());
  if (feirCFG == nullptr) {
    feirCFG = std::make_unique<FEIRCFG>(feirStmtHead, feirStmtTail);
    feirCFG->GenerateCFG();
  }
  file << "digraph {" << std::endl;
  file << "  label=\"" << GetGeneralFuncName() << "\"\n";
  file << "  labelloc=t\n";
  feirCFG->DumpCFGGraph(file);
  file.close();
  return phaseResult.Finish();
}

void FEFunction::DumpFEIRCFGGraphForDFGEdge(std::ofstream &file) {
  file << "  subgraph cfg_edges {" << std::endl;
  file << "    edge [color=\"#00FF00\",weight=0.3,len=3];" << std::endl;
  file << "  }" << std::endl;
}

bool FEFunction::BuildGeneralStmtBBMap(const std::string &phaseName) {
  phaseResult.RegisterPhaseNameAndStart(phaseName);
  FELinkListNode *nodeBB = genBBHead->GetNext();
  while (nodeBB != nullptr && nodeBB != genBBTail) {
    FEIRBB *bb = static_cast<FEIRBB*>(nodeBB);
    const FELinkListNode *nodeStmt = bb->GetStmtHead();
    while (nodeStmt != nullptr) {
      const FEIRStmt *stmt = static_cast<const FEIRStmt*>(nodeStmt);
      genStmtBBMap[stmt] = bb;
      if (nodeStmt == bb->GetStmtTail()) {
        break;
      }
      nodeStmt = nodeStmt->GetNext();
    }
    nodeBB = nodeBB->GetNext();
  }
  return phaseResult.Finish();
}

bool FEFunction::LabelGeneralStmts(const std::string &phaseName) {
  phaseResult.RegisterPhaseNameAndStart(phaseName);
  uint32 idx = 0;
  FELinkListNode *nodeStmt = genStmtHead;
  while (nodeStmt != nullptr) {
    FEIRStmt *stmt = static_cast<FEIRStmt*>(nodeStmt);
    stmt->SetID(idx);
    idx++;
    nodeStmt = nodeStmt->GetNext();
  }
  return phaseResult.Finish();
}

bool FEFunction::LabelFEIRBBs(const std::string &phaseName) {
  phaseResult.RegisterPhaseNameAndStart(phaseName);
  uint32 idx = 0;
  FELinkListNode *nodeBB = genBBHead->GetNext();
  while (nodeBB != nullptr && nodeBB != genBBTail) {
    FEIRBB *bb = static_cast<FEIRBB*>(nodeBB);
    bb->SetID(idx);
    idx++;
    nodeBB = nodeBB->GetNext();
  }
  return phaseResult.Finish();
}

std::string FEFunction::GetGeneralFuncName() const {
  return mirFunction.GetName();
}

void FEFunction::PhaseTimerStart(FETimerNS &timer) {
  if (!FEOptions::GetInstance().IsDumpPhaseTime()) {
    return;
  }
  timer.Start();
}

void FEFunction::PhaseTimerStopAndDump(FETimerNS &timer, const std::string &label) {
  if (!FEOptions::GetInstance().IsDumpPhaseTime()) {
    return;
  }
  timer.Stop();
  INFO(kLncInfo, "[PhaseTime]   %s: %lld ns", label.c_str(), timer.GetTimeNS());
}

bool FEFunction::UpdateFormal(const std::string &phaseName) {
  phaseResult.RegisterPhaseNameAndStart(phaseName);
  HIR2MPL_PARALLEL_FORBIDDEN();
  uint32 idx = 0;
  mirFunction.ClearFormals();
  FEManager::GetMIRBuilder().SetCurrentFunction(mirFunction);
  for (const std::unique_ptr<FEIRVar> &argVar : argVarList) {
    MIRSymbol *sym = argVar->GenerateMIRSymbol(FEManager::GetMIRBuilder());
    sym->SetStorageClass(kScFormal);
    mirFunction.AddArgument(sym);
    idx++;
  }
  return phaseResult.Finish();
}

std::string FEFunction::GetDescription() {
  std::stringstream ss;
  std::string oriFuncName = GetGeneralFuncName();
  std::string mplFuncName = namemangler::EncodeName(oriFuncName);
  ss << "ori function name: " << oriFuncName << std::endl;
  ss << "mpl function name: " << mplFuncName << std::endl;
  ss << "parameter list:" << "(";
  for (const std::unique_ptr<FEIRVar> &argVar : argVarList) {
    ss << argVar->GetNameRaw() << ", ";
  }
  ss << ") {" << std::endl;
  FELinkListNode *node = feirStmtHead->GetNext();
  while (node != feirStmtTail) {
    FEIRStmt *currStmt = static_cast<FEIRStmt*>(node);
    ss << currStmt->DumpDotString() << std::endl;
    node = node->GetNext();
  }
  ss << "}" << std::endl;
  return ss.str();
}

bool FEFunction::EmitToMIR(const std::string &phaseName) {
  phaseResult.RegisterPhaseNameAndStart(phaseName);
  mirFunction.NewBody();
  FEManager::GetMIRBuilder().SetCurrentFunction(mirFunction);
  FEManager::SetCurrentFEFunction(*this);
  BuildMapLabelIdx();
  EmitToMIRStmt();
  return phaseResult.Finish();
}

const FEIRStmtPesudoLOC *FEFunction::GetLOCForStmt(const FEIRStmt &feIRStmt) const {
  if (!feIRStmt.ShouldHaveLOC()) {
    return nullptr;
  }
  FELinkListNode *prevNode = static_cast<FELinkListNode*>(feIRStmt.GetPrev());
  while (prevNode != nullptr) {
    if ((*static_cast<FEIRStmt*>(prevNode)).ShouldHaveLOC()) {
      return nullptr;
    }
    FEIRStmt *stmt = static_cast<FEIRStmt*>(prevNode);
    if (stmt->GetKind() == kStmtPesudoLOC) {
      FEIRStmtPesudoLOC *loc = static_cast<FEIRStmtPesudoLOC*>(stmt);
      return loc;
    }
    prevNode = prevNode->GetPrev();
  }
  return nullptr;
}

void FEFunction::BuildMapLabelIdx() {
  FELinkListNode *nodeStmt = feirStmtHead->GetNext();
  while (nodeStmt != nullptr && nodeStmt != feirStmtTail) {
    FEIRStmt *stmt = static_cast<FEIRStmt*>(nodeStmt);
    if (stmt->GetKind() == FEIRNodeKind::kStmtPesudoLabel) {
      FEIRStmtPesudoLabel *stmtLabel = static_cast<FEIRStmtPesudoLabel*>(stmt);
      stmtLabel->GenerateLabelIdx(FEManager::GetMIRBuilder());
      mapLabelIdx[stmtLabel->GetLabelIdx()] = stmtLabel->GetMIRLabelIdx();
    }
    nodeStmt = nodeStmt->GetNext();
  }
}

bool FEFunction::CheckPhaseResult(const std::string &phaseName) {
  phaseResult.RegisterPhaseNameAndStart(phaseName);
  bool success = phaseResult.IsSuccess();
  return phaseResult.Finish(success);
}

bool FEFunction::ProcessFEIRFunction() {
  bool success = true;
  success = success && BuildMapLabelStmt("fe/build map label stmt");
  success = success && SetupFEIRStmtJavaTry("fe/setup stmt javatry");
  success = success && SetupFEIRStmtBranch("fe/setup stmt branch");
  success = success && UpdateRegNum2This("fe/update reg num to this pointer");
  return success;
}

bool FEFunction::BuildMapLabelStmt(const std::string &phaseName) {
  phaseResult.RegisterPhaseNameAndStart(phaseName);
  FELinkListNode *nodeStmt = feirStmtHead->GetNext();
  while (nodeStmt != nullptr && nodeStmt != feirStmtTail) {
    FEIRStmt *stmt = static_cast<FEIRStmt*>(nodeStmt);
    FEIRNodeKind kind = stmt->GetKind();
    switch (kind) {
      case FEIRNodeKind::kStmtPesudoLabel:
      case FEIRNodeKind::kStmtPesudoJavaCatch: {
        FEIRStmtPesudoLabel *stmtLabel = static_cast<FEIRStmtPesudoLabel*>(stmt);
        mapLabelStmt[stmtLabel->GetLabelIdx()] = stmtLabel;
        break;
      }
      default:
        break;
    }
    nodeStmt = nodeStmt->GetNext();
  }
  return phaseResult.Finish();
}

bool FEFunction::SetupFEIRStmtJavaTry(const std::string &phaseName) {
  phaseResult.RegisterPhaseNameAndStart(phaseName);
  FELinkListNode *nodeStmt = feirStmtHead->GetNext();
  while (nodeStmt != nullptr && nodeStmt != feirStmtTail) {
    FEIRStmt *stmt = static_cast<FEIRStmt*>(nodeStmt);
    if (stmt->GetKind() == FEIRNodeKind::kStmtPesudoJavaTry) {
      FEIRStmtPesudoJavaTry *stmtJavaTry = static_cast<FEIRStmtPesudoJavaTry*>(stmt);
      for (uint32 labelIdx : stmtJavaTry->GetCatchLabelIdxVec()) {
        auto it = mapLabelStmt.find(labelIdx);
        CHECK_FATAL(it != mapLabelStmt.end(), "label is not found");
        stmtJavaTry->AddCatchTarget(*(it->second));
      }
    }
    nodeStmt = nodeStmt->GetNext();
  }
  return phaseResult.Finish();
}

bool FEFunction::SetupFEIRStmtBranch(const std::string &phaseName) {
  phaseResult.RegisterPhaseNameAndStart(phaseName);
  bool success = true;
  FELinkListNode *nodeStmt = feirStmtHead->GetNext();
  while (nodeStmt != nullptr && nodeStmt != feirStmtTail) {
    FEIRStmt *stmt = static_cast<FEIRStmt*>(nodeStmt);
    FEIRNodeKind kind = stmt->GetKind();
    switch (kind) {
      case FEIRNodeKind::kStmtGoto:
      case FEIRNodeKind::kStmtCondGoto:
        success = success && SetupFEIRStmtGoto(*(static_cast<FEIRStmtGoto*>(stmt)));
        break;
      case FEIRNodeKind::kStmtSwitch:
        success = success && SetupFEIRStmtSwitch(*(static_cast<FEIRStmtSwitch*>(stmt)));
        break;
      default:
        break;
    }
    nodeStmt = nodeStmt->GetNext();
  }
  return phaseResult.Finish(success);
}

bool FEFunction::SetupFEIRStmtGoto(FEIRStmtGoto &stmt) {
  auto it = mapLabelStmt.find(stmt.GetLabelIdx());
  if (it == mapLabelStmt.end()) {
    ERR(kLncErr, "target not found for stmt goto");
    return false;
  }
  stmt.SetStmtTarget(*(it->second));
  return true;
}

bool FEFunction::SetupFEIRStmtSwitch(FEIRStmtSwitch &stmt) {
  // default target
  auto itDefault = mapLabelStmt.find(stmt.GetDefaultLabelIdx());
  if (itDefault == mapLabelStmt.end()) {
    ERR(kLncErr, "target not found for stmt goto");
    return false;
  }
  stmt.SetDefaultTarget(*(itDefault->second));

  // value targets
  for (const auto &itItem : stmt.GetMapValueLabelIdx()) {
    auto itTarget = mapLabelStmt.find(itItem.second);
    if (itTarget == mapLabelStmt.end()) {
      ERR(kLncErr, "target not found for stmt goto");
      return false;
    }
    stmt.AddTarget(itItem.first, *(itTarget->second));
  }
  return true;
}

bool FEFunction::UpdateRegNum2This(const std::string &phaseName) {
  bool success = CheckPhaseResult(phaseName);
  if (!success) {
    return success;
  }
  if (!HasThis()) {
    return success;
  }
  const std::unique_ptr<FEIRVar> &firstArg = argVarList.front();
  std::unique_ptr<FEIRVar> varReg = firstArg->Clone();
  GStrIdx thisNameIdx = FEUtils::GetThisIdx();
  std::unique_ptr<FEIRVar> varThisAsParam = std::make_unique<FEIRVarName>(thisNameIdx, varReg->GetType()->Clone());
  if (!IsNative()) {
    std::unique_ptr<FEIRVar> varThisAsLocalVar = std::make_unique<FEIRVarName>(thisNameIdx, varReg->GetType()->Clone());
    std::unique_ptr<FEIRExpr> dReadThis = std::make_unique<FEIRExprDRead>(std::move(varThisAsLocalVar));
    std::unique_ptr<FEIRStmt> daStmt = std::make_unique<FEIRStmtDAssign>(std::move(varReg), std::move(dReadThis));
    FEIRStmt *stmt = RegisterFEIRStmt(std::move(daStmt));
    FELinkListNode::InsertAfter(stmt, feirStmtHead);
  }
  argVarList[0].reset(varThisAsParam.release());
  return success;
}

void FEFunction::OutputStmts() {
  FELinkListNode *node = feirStmtHead->GetNext();
  while (node != feirStmtTail) {
    FEIRStmt *currStmt = static_cast<FEIRStmt*>(node);
    LogInfo::MapleLogger() << currStmt->DumpDotString() <<  "\n";
    node = node->GetNext();
  }
}

void FEFunction::LabelFEIRStmts() {
  // stmt idx start from 1
  FELinkListNode *node = feirStmtHead->GetNext();
  uint32 id = 1;
  while (node != feirStmtTail) {
    FEIRStmt *currStmt = static_cast<FEIRStmt*>(node);
    currStmt->SetID(id++);
    node = node->GetNext();
  }
  stmtCount = --id;
}

bool FEFunction::ShouldNewBB(const FEIRBB *currBB, const FEIRStmt &currStmt) const {
  if (currBB == nullptr) {
    return true;
  }
  if (currStmt.IsTarget()) {
    if (currBB->GetStmtNoAuxTail() != nullptr) {
      return true;
    }
  }
  return false;
}

bool FEFunction::IsBBEnd(const FEIRStmt &stmt) const {
  bool currStmtMayBeBBEnd = MayBeBBEnd(stmt);
  if (currStmtMayBeBBEnd) {
    FELinkListNode *node = stmt.GetNext();
    FEIRStmt *nextStmt = static_cast<FEIRStmt*>(node);
    if (nextStmt->IsAuxPost()) {  // if curr stmt my be BB end, but next stmt is AuxPost
      return false;  // curr stmt should not be BB end
    }
    return true;
  }
  if (stmt.IsAuxPost()) {
    FELinkListNode *node = stmt.GetPrev();
    FEIRStmt *prevStmt = static_cast<FEIRStmt*>(node);
    bool prevStmtMayBeBBEnd = MayBeBBEnd(*prevStmt);  // if curr stmt is AuxPost, and prev stmt my be BB end
    return prevStmtMayBeBBEnd;  // return prev stmt my be BB end as result
  }
  return currStmtMayBeBBEnd;
}

bool FEFunction::MayBeBBEnd(const FEIRStmt &stmt) const {
  return (stmt.IsBranch() || !stmt.IsFallThru());
}

void FEFunction::LinkFallThroughBBAndItsNext(FEIRBB &bb) {
  if (!CheckBBsStmtNoAuxTail(bb)) {
    return;
  }
  if (!bb.IsFallThru()) {
    return;
  }
  FELinkListNode *node = bb.GetNext();
  FEIRBB *nextBB = static_cast<FEIRBB*>(node);
  if (nextBB != feirBBTail) {
    LinkBB(bb, *nextBB);
  }
}

void FEFunction::LinkBranchBBAndItsTargets(FEIRBB &bb) {
  if (!CheckBBsStmtNoAuxTail(bb)) {
    return;
  }
  if (!bb.IsBranch()) {
    return;
  }
  const FEIRStmt *stmtTail = bb.GetStmtNoAuxTail();
  FEIRNodeKind nodeKind = stmtTail->GetKind();
  switch (nodeKind) {
    case FEIRNodeKind::kStmtCondGoto:
    case FEIRNodeKind::kStmtGoto: {
      LinkGotoBBAndItsTarget(bb, *stmtTail);
      break;
    }
    case FEIRNodeKind::kStmtSwitch: {
      LinkSwitchBBAndItsTargets(bb, *stmtTail);
      break;
    }
    default: {
      CHECK_FATAL(false, "nodeKind %u is not branch", nodeKind);
      break;
    }
  }
}

void FEFunction::LinkGotoBBAndItsTarget(FEIRBB &bb, const FEIRStmt &stmtTail) {
  const FEIRStmtGoto2 &gotoStmt = static_cast<const FEIRStmtGoto2&>(stmtTail);
  const FEIRStmtPesudoLabel2 &targetStmt = gotoStmt.GetStmtTargetRef();
  FEIRBB &targetBB = GetFEIRBBByStmt(targetStmt);
  LinkBB(bb, targetBB);
}

void FEFunction::LinkSwitchBBAndItsTargets(FEIRBB &bb, const FEIRStmt &stmtTail) {
  const FEIRStmtSwitch2 &switchStmt = static_cast<const FEIRStmtSwitch2&>(stmtTail);
  const std::map<int32, FEIRStmtPesudoLabel2*> &mapValueTargets = switchStmt.GetMapValueTargets();
  for (auto it : mapValueTargets) {
    FEIRStmtPesudoLabel2 *pesudoLabel = it.second;
    FEIRBB &targetBB = GetFEIRBBByStmt(*pesudoLabel);
    LinkBB(bb, targetBB);
  }
  FEIRBB &targetBB = GetFEIRBBByStmt(switchStmt.GetDefaultTarget());
  LinkBB(bb, targetBB);
}

void FEFunction::LinkBB(FEIRBB &predBB, FEIRBB &succBB) {
  predBB.AddSuccBB(&succBB);
  succBB.AddPredBB(&predBB);
}

FEIRBB &FEFunction::GetFEIRBBByStmt(const FEIRStmt &stmt) {
  auto it = feirStmtBBMap.find(&stmt);
  return *(it->second);
}

bool FEFunction::CheckBBsStmtNoAuxTail(const FEIRBB &bb) {
  bool bbHasStmtNoAuxTail = (bb.GetStmtNoAuxTail() != nullptr);
  CHECK_FATAL(bbHasStmtNoAuxTail, "Error accured in BuildFEIRBB phase, bb.GetStmtNoAuxTail() should not be nullptr");
  return true;
}

void FEFunction::InsertCheckPointForBBs() {
  FELinkListNode *node = feirBBHead->GetNext();
  while (node != feirBBTail) {
    FEIRBB *currBB = static_cast<FEIRBB*>(node);  // get currBB
    // create chekPointIn
    std::unique_ptr<FEIRStmtCheckPoint> chekPointIn = std::make_unique<FEIRStmtCheckPoint>();
    currBB->SetCheckPointIn(std::move(chekPointIn));  // set to currBB's checkPointIn
    FEIRStmtCheckPoint &cpIn = currBB->GetCheckPointIn();
    currBB->InsertAndUpdateNewHead(&cpIn);  // insert and update new head to chekPointIn
    (void)feirStmtBBMap.insert(std::make_pair(&cpIn, currBB));  // add pair to feirStmtBBMap
    // create chekPointOut
    std::unique_ptr<FEIRStmtCheckPoint> chekPointOut = std::make_unique<FEIRStmtCheckPoint>();
    currBB->SetCheckPointOut(std::move(chekPointOut));  // set to currBB's checkPointOut
    FEIRStmtCheckPoint &cpOut = currBB->GetCheckPointOut();
    currBB->InsertAndUpdateNewTail(&cpOut);  // insert and update new tail to chekPointOut
    (void)feirStmtBBMap.insert(std::make_pair(&cpOut, currBB));  // add pair to feirStmtBBMap
    // get next BB
    node = node->GetNext();
  }
}

void FEFunction::InsertCheckPointForTrys() {
  FEIRStmtPesudoJavaTry2 *currTry = nullptr;
  FEIRStmtCheckPoint *checkPointInTry = nullptr;
  FELinkListNode *node = feirStmtHead->GetNext();
  while (node != feirStmtTail) {
    FEIRStmt *currStmt = static_cast<FEIRStmt*>(node);
    if (currStmt->GetKind() == FEIRNodeKind::kStmtPesudoJavaTry) {
      currTry = static_cast<FEIRStmtPesudoJavaTry2*>(currStmt);
      checkPointInTry = nullptr;
    }
    if ((currTry != nullptr) &&
        (currStmt->IsThrowable()) &&
        ((checkPointInTry == nullptr) || currStmt->HasDef())) {
      FEIRBB &currBB = GetFEIRBBByStmt(*currStmt);
      if (currStmt == currBB.GetStmtNoAuxHead()) {
        checkPointInTry = &(currBB.GetCheckPointIn());
        (void)checkPointJavaTryMap.insert(std::make_pair(checkPointInTry, currTry));
        if (currStmt == currBB.GetStmtHead()) {
          currBB.SetStmtHead(currStmt);
        }
        node = node->GetNext();
        continue;
      }
      std::unique_ptr<FEIRStmtCheckPoint> newCheckPoint = std::make_unique<FEIRStmtCheckPoint>();
      currBB.AddCheckPoint(std::move(newCheckPoint));
      checkPointInTry = currBB.GetLatestCheckPoint();
      CHECK_NULL_FATAL(checkPointInTry);
      FELinkListNode::InsertBefore(checkPointInTry, currStmt);
      (void)feirStmtBBMap.insert(std::make_pair(checkPointInTry, &currBB));
      (void)checkPointJavaTryMap.insert(std::make_pair(checkPointInTry, currTry));
      if (currStmt == currBB.GetStmtHead()) {
        currBB.SetStmtHead(currStmt);
      }
    }
    if (currStmt->GetKind() == FEIRNodeKind::kStmtPesudoEndTry) {
      currTry = nullptr;
    }
    node = node->GetNext();
  }
}

void FEFunction::InitTrans4AllVars() {
  FELinkListNode *node = feirStmtHead->GetNext();
  while (node != feirStmtTail) {
    FEIRStmt *currStmt = static_cast<FEIRStmt*>(node);
    currStmt->InitTrans4AllVars();
    node = node->GetNext();
  }
}

FEIRStmtPesudoJavaTry2 &FEFunction::GetJavaTryByCheckPoint(FEIRStmtCheckPoint &checkPoint) {
  auto it = checkPointJavaTryMap.find(&checkPoint);
  return *(it->second);
}

FEIRStmtCheckPoint &FEFunction::GetCheckPointByFEIRStmt(const FEIRStmt &stmt) {
  auto it = feirStmtCheckPointMap.find(&stmt);
  return *(it->second);
}

void FEFunction::SetUpDefVarTypeScatterStmtMap() {
  FELinkListNode *node = feirStmtHead->GetNext();
  while (node != feirStmtTail) {
    FEIRStmt *currStmt = static_cast<FEIRStmt*>(node);
    FEIRVarTypeScatter *defVarTypeScatter = currStmt->GetTypeScatterDefVar();
    if (defVarTypeScatter != nullptr) {
      (void)defVarTypeScatterStmtMap.insert(std::make_pair(defVarTypeScatter, currStmt));
    }
    node = node->GetNext();
  }
}

void FEFunction::InsertRetypeStmtsAfterDef(const UniqueFEIRVar& def) {
  bool defIsTypeScatter = (def->GetKind() == kFEIRVarTypeScatter);
  if (!defIsTypeScatter) {
    return;
  }
  FEIRVarTypeScatter &fromVar = *(static_cast<FEIRVarTypeScatter*>(def.get()));
  const std::unordered_set<FEIRTypeKey, FEIRTypeKeyHash> &scatterTypes = fromVar.GetScatterTypes();
  for (auto &it : scatterTypes) {
    const maple::FEIRTypeKey &typeKey = it;
    FEIRType &toType = *(typeKey.GetType());
    FEIRType &fromType = *(fromVar.GetType());
    Opcode opcode = FEIRTypeCvtHelper::ChooseCvtOpcodeByFromTypeAndToType(fromType, toType);
    if (opcode == OP_retype) {
      InsertRetypeStmt(fromVar, toType);
    } else if (opcode == OP_cvt) {
      InsertCvtStmt(fromVar, toType);
    } else {
      InsertJavaMergeStmt(fromVar, toType);
    }
  }
}

void FEFunction::InsertRetypeStmt(const FEIRVarTypeScatter &fromVar, const FEIRType &toType) {
  // create DRead Expr
  std::unique_ptr<FEIRExprDRead> exprDRead = std::make_unique<FEIRExprDRead>(fromVar.GetVar()->Clone());
  std::unique_ptr<FEIRType> typeDst = FEIRTypeHelper::CreatePointerType(toType.Clone(), toType.GetPrimType());
  // create expr for retype
  std::unique_ptr<FEIRExprTypeCvt> expr = std::make_unique<FEIRExprTypeCvt>(std::move(typeDst), OP_retype,
                                                                            std::move(exprDRead));
  // after expr created, insert dassign stmt
  InsertDAssignStmt4TypeCvt(fromVar, toType, std::move(expr));
}

void FEFunction::InsertCvtStmt(const FEIRVarTypeScatter &fromVar, const FEIRType &toType) {
  // create DRead Expr
  std::unique_ptr<FEIRExprDRead> exprDRead = std::make_unique<FEIRExprDRead>(fromVar.GetVar()->Clone());
  // create expr for type cvt
  std::unique_ptr<FEIRExprTypeCvt> expr = std::make_unique<FEIRExprTypeCvt>(OP_cvt, std::move(exprDRead));
  expr->GetType()->SetPrimType(toType.GetPrimType());
  // after expr created, insert dassign stmt
  InsertDAssignStmt4TypeCvt(fromVar, toType, std::move(expr));
}

void FEFunction::InsertJavaMergeStmt(const FEIRVarTypeScatter &fromVar, const FEIRType &toType) {
  // create DRead Expr
  std::unique_ptr<FEIRExprDRead> exprDRead = std::make_unique<FEIRExprDRead>(fromVar.GetVar()->Clone());
  // create expr for java merge
  std::vector<std::unique_ptr<FEIRExpr>> argOpnds;
  argOpnds.push_back(std::move(exprDRead));
  std::unique_ptr<FEIRExprJavaMerge> javaMergeExpr = std::make_unique<FEIRExprJavaMerge>(toType.Clone(), argOpnds);
  // after expr created, insert dassign stmt
  InsertDAssignStmt4TypeCvt(fromVar, toType, std::move(javaMergeExpr));
}

void FEFunction::InsertDAssignStmt4TypeCvt(const FEIRVarTypeScatter &fromVar, const FEIRType &toType,
                                           UniqueFEIRExpr expr) {
  FEIRVar *var = fromVar.GetVar().get();
  CHECK_FATAL((var->GetKind() == FEIRVarKind::kFEIRVarReg), "fromVar's inner var must be var reg kind");
  FEIRVarReg *varReg = static_cast<FEIRVarReg*>(var);
  uint32 regNum = varReg->GetRegNum();
  UniqueFEIRVar toVar = FEIRBuilder::CreateVarReg(regNum, toType.Clone());
  std::unique_ptr<FEIRStmt> daStmt = std::make_unique<FEIRStmtDAssign>(std::move(toVar), std::move(expr));
  FEIRStmt *insertedStmt = RegisterFEIRStmt(std::move(daStmt));
  FEIRStmt &stmt = GetStmtByDefVarTypeScatter(fromVar);
  FELinkListNode::InsertAfter(insertedStmt, &stmt);
}

FEIRStmt &FEFunction::GetStmtByDefVarTypeScatter(const FEIRVarTypeScatter &varTypeScatter) {
  auto it = defVarTypeScatterStmtMap.find(&varTypeScatter);
  return *(it->second);
}

bool FEFunction::WithFinalFieldsNeedBarrier(MIRClassType *classType, bool isStatic) const {
  // final field
  if (isStatic) {
    // final static fields with non-primitive types
    // the one with primitive types are all inlined
    for (auto it : classType->GetStaticFields()) {
      MIRType *type = GlobalTables::GetTypeTable().GetTypeFromTyIdx(it.second.first);
      if (it.second.second.GetAttr(FLDATTR_final) && type->GetPrimType() == PTY_ref) {
        return true;
      }
    }
  } else {
    for (auto it : classType->GetFields()) {
      if (it.second.second.GetAttr(FLDATTR_final)) {
        return true;
      }
    }
  }
  return false;
}

bool FEFunction::IsNeedInsertBarrier() {
  if (mirFunction.GetAttr(FUNCATTR_constructor) ||
      mirFunction.GetName().find("_7Cclone_7C") != std::string::npos ||
      mirFunction.GetName().find("_7CcopyOf_7C") != std::string::npos) {
    const std::string &className = mirFunction.GetBaseClassName();
    MIRType *type = FEManager::GetTypeManager().GetClassOrInterfaceType(className);
    if (type->GetKind() == kTypeClass) {
      MIRClassType *currClass = static_cast<MIRClassType*>(type);
      if (!mirFunction.GetAttr(FUNCATTR_constructor) ||
          WithFinalFieldsNeedBarrier(currClass, mirFunction.GetAttr(FUNCATTR_static))) {
        return true;
      }
    }
  }
  return false;
}

void FEFunction::EmitToMIRStmt() {
  MIRBuilder &builder = FEManager::GetMIRBuilder();
  FELinkListNode *nodeStmt = feirStmtHead->GetNext();
  while (nodeStmt != nullptr && nodeStmt != feirStmtTail) {
    FEIRStmt *stmt = static_cast<FEIRStmt*>(nodeStmt);
    std::list<StmtNode*> mirStmts = stmt->GenMIRStmts(builder);
#ifdef DEBUG
    // LOC info has been recorded in FEIRStmt already, this could be removed later.
    AddLocForStmt(*stmt, mirStmts);
#endif
    for (StmtNode *mirStmt : mirStmts) {
      builder.AddStmtInCurrentFunctionBody(*mirStmt);
    }
    nodeStmt = nodeStmt->GetNext();
  }
}

void FEFunction::AddLocForStmt(const FEIRStmt &stmt, std::list<StmtNode*> &mirStmts) const {
  const FEIRStmtPesudoLOC *pesudoLoc = GetLOCForStmt(stmt);
  if (pesudoLoc != nullptr) {
    mirStmts.front()->GetSrcPos().SetFileNum(static_cast<uint16>(pesudoLoc->GetSrcFileIdx()));
    mirStmts.front()->GetSrcPos().SetLineNum(pesudoLoc->GetSrcFileLineNum());
  }
}

void FEFunction::PushFuncScope(const SrcPosition &startOfScope, const SrcPosition &endOfScope) {
  UniqueFEIRScope feirScope = std::make_unique<FEIRScope>(scopeID++);
  if (FEOptions::GetInstance().IsDbgFriendly()) {
    MIRScope *mirScope = mirFunction.GetScope();
    mirScope->SetRange(startOfScope, endOfScope);
    feirScope->SetMIRScope(mirScope);
  }
  scopeStack.push_front(std::move(feirScope));
}

void FEFunction::PushStmtScope(const SrcPosition &startOfScope, const SrcPosition &endOfScope, bool isControllScope) {
  UniqueFEIRScope feirScope = std::make_unique<FEIRScope>(scopeID++);
  if (FEOptions::GetInstance().IsDbgFriendly()) {
    MIRScope *parentMIRScope = GetTopMIRScope();
    CHECK_NULL_FATAL(parentMIRScope);
    MIRScope *mirScope = mirFunction.GetModule()->GetMemPool()->New<MIRScope>(mirFunction.GetModule());
    mirScope->SetRange(startOfScope, endOfScope);
    (void)parentMIRScope->AddScope(mirScope);
    feirScope->SetMIRScope(mirScope);
  }
  feirScope->SetIsControllScope(isControllScope);
  scopeStack.push_front(std::move(feirScope));
}

void FEFunction::PushStmtScope(bool isControllScope) {
  UniqueFEIRScope feirScope = std::make_unique<FEIRScope>(scopeID++, isControllScope);
  scopeStack.push_front(std::move(feirScope));
}

FEIRScope *FEFunction::GetTopFEIRScopePtr() const {
  if (!scopeStack.empty()) {
    return scopeStack.front().get();
  }
  CHECK_FATAL(false, "scope stack is empty");
  return nullptr;
}

MIRScope *FEFunction::GetTopMIRScope() const {
  if (scopeStack.empty()) {
    CHECK_FATAL(false, "scope stack is empty");
    return nullptr;
  }
  for (const auto &feirScope : scopeStack) {
    if (feirScope->GetMIRScope() != nullptr) {
      return feirScope->GetMIRScope();
    }
  }
  return nullptr;
}

UniqueFEIRScope FEFunction::PopTopScope() {
  if (!scopeStack.empty()) {
    UniqueFEIRScope scope = std::move(scopeStack.front());
    scopeStack.pop_front();
    return scope;
  }
  CHECK_FATAL(false, "scope stack is empty");
  return nullptr;
}

void FEFunction::AddAliasInMIRScope(MIRScope *scope, const std::string &srcVarName, const MIRSymbol *symbol,
                                    const GStrIdx &typeNameIdx, const MIRType *sourceType) {
  GStrIdx nameIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(srcVarName);
  MIRAliasVars aliasVar;
  aliasVar.mplStrIdx = symbol->GetNameStrIdx();
  aliasVar.isLocal = symbol->IsLocal();
  if (sourceType != nullptr) {
    aliasVar.atk = ATK_type;
    aliasVar.index = sourceType->GetTypeIndex().GetIdx();
  } else if (typeNameIdx != 0) {
    aliasVar.atk = ATK_string;
    aliasVar.index = typeNameIdx.GetIdx();
  } else {
    aliasVar.atk = ATK_type;
    aliasVar.index = symbol->GetTyIdx().GetIdx();
  }
  scope->SetAliasVarMap(nameIdx, aliasVar);
};

void FEFunction::AddVLACleanupStmts(std::list<UniqueFEIRStmt> &stmts) {
  (void)stmts;
  CHECK_FATAL(false, "AddVLACleanupStmts only support astfunction");
};
}  // namespace maple
