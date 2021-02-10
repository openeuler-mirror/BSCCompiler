/*
 * Copyright (c) [2020] Huawei Technologies Co.,Ltd.All rights reserved.
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
#include "general_bb.h"
#include "mpl_logging.h"
#include "fe_options.h"
#include "fe_manager.h"
#include "feir_var_name.h"
#include "feir_var_reg.h"
#include "mplfe_env.h"
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
      mirFunction(argMIRFunction) {
}

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
  // general stmt/bb
  genStmtHead = RegisterGeneralStmt(std::make_unique<GeneralStmt>(GeneralStmtKind::kStmtDummyBegin));
  genStmtTail = RegisterGeneralStmt(std::make_unique<GeneralStmt>(GeneralStmtKind::kStmtDummyEnd));
  genStmtHead->SetNext(genStmtTail);
  genStmtTail->SetPrev(genStmtHead);
  genBBHead = RegisterGeneralBB(std::make_unique<GeneralBB>(GeneralBBKind::kBBKindPesudoHead));
  genBBTail = RegisterGeneralBB(std::make_unique<GeneralBB>(GeneralBBKind::kBBKindPesudoTail));
  genBBHead->SetNext(genBBTail);
  genBBTail->SetPrev(genBBHead);
  // feir stmt/bb
  feirStmtHead = RegisterFEIRStmt(std::make_unique<FEIRStmt>(GeneralStmtKind::kStmtDummyBegin,
                                                             FEIRNodeKind::kStmtPesudoFuncStart));
  feirStmtTail = RegisterFEIRStmt(std::make_unique<FEIRStmt>(GeneralStmtKind::kStmtDummyEnd,
                                                             FEIRNodeKind::kStmtPesudoFuncEnd));
  feirStmtHead->SetNext(feirStmtTail);
  feirStmtTail->SetPrev(feirStmtHead);
  feirBBHead = RegisterFEIRBB(std::make_unique<FEIRBB>(GeneralBBKind::kBBKindPesudoHead));
  feirBBTail = RegisterFEIRBB(std::make_unique<FEIRBB>(GeneralBBKind::kBBKindPesudoTail));
  feirBBHead->SetNext(feirBBTail);
  feirBBTail->SetPrev(feirBBHead);
}

GeneralStmt *FEFunction::RegisterGeneralStmt(std::unique_ptr<GeneralStmt> stmt) {
  genStmtList.push_back(std::move(stmt));
  return genStmtList.back().get();
}

const std::unique_ptr<GeneralStmt> &FEFunction::RegisterGeneralStmtUniqueReturn(std::unique_ptr<GeneralStmt> stmt) {
  genStmtList.push_back(std::move(stmt));
  return genStmtList.back();
}

GeneralBB *FEFunction::RegisterGeneralBB(std::unique_ptr<GeneralBB> bb) {
  genBBList.push_back(std::move(bb));
  return genBBList.back().get();
}

FEIRStmt *FEFunction::RegisterFEIRStmt(UniqueFEIRStmt stmt) {
  feirStmtList.push_back(std::move(stmt));
  return feirStmtList.back().get();
}

FEIRBB *FEFunction::RegisterFEIRBB(std::unique_ptr<FEIRBB> bb) {
  feirBBList.push_back(std::move(bb));
  return feirBBList.back().get();
}

GeneralBB *FEFunction::NewGeneralBB() {
  return new GeneralBB();
}

GeneralBB *FEFunction::NewGeneralBB(uint8 argBBKind) {
  return new GeneralBB(argBBKind);
}

bool FEFunction::BuildGeneralBB(const std::string &phaseName) {
  phaseResult.RegisterPhaseNameAndStart(phaseName);
  bool success = phaseResult.IsSuccess();
  if (!success) {
    return phaseResult.Finish(success);
  }
  // Build BB
  FELinkListNode *nodeStmt = genStmtHead->GetNext();
  GeneralBB *currBB = nullptr;
  while (nodeStmt != nullptr && nodeStmt != genStmtTail) {
    GeneralStmt *stmt = static_cast<GeneralStmt*>(nodeStmt);
    if (stmt->IsAux() == false) {
      // check start of BB
      if (currBB == nullptr || stmt->GetGeneralStmtKind() == GeneralStmtKind::kStmtMultiIn) {
        currBB = NewGeneralBB();
        genBBTail->InsertBefore(currBB);
      }
      CHECK_FATAL(currBB != nullptr, "nullptr check for currBB");
      currBB->AppendStmt(*stmt);
      // check end of BB
      if (stmt->IsFallThru() == false || stmt->GetGeneralStmtKind() == GeneralStmtKind::kStmtMultiOut) {
        currBB = nullptr;
      }
    }
    nodeStmt = nodeStmt->GetNext();
  }
  // Add Aux Stmt
  FELinkListNode *nodeBB = genBBHead->GetNext();
  while (nodeBB != nullptr && nodeBB != genBBTail) {
    GeneralBB *bb = static_cast<GeneralBB*>(nodeBB);
    // add pre
    nodeStmt = bb->GetStmtHead()->GetPrev();
    while (nodeStmt != nullptr && nodeStmt != genStmtHead) {
      GeneralStmt *stmt = static_cast<GeneralStmt*>(nodeStmt);
      if (stmt->IsAuxPre()) {
        bb->AddStmtAuxPre(*stmt);
      } else {
        break;
      }
      nodeStmt = nodeStmt->GetPrev();
    }
    // add post
    nodeStmt = bb->GetStmtTail()->GetNext();
    while (nodeStmt != nullptr && nodeStmt != genStmtTail) {
      GeneralStmt *stmt = static_cast<GeneralStmt*>(nodeStmt);
      if (stmt->IsAuxPost()) {
        bb->AddStmtAuxPost(*stmt);
      } else {
        break;
      }
      nodeStmt = nodeStmt->GetNext();
    }
    nodeBB = nodeBB->GetNext();
  }
  return phaseResult.Finish(success);
}

bool FEFunction::BuildGeneralCFG(const std::string &phaseName) {
  phaseResult.RegisterPhaseNameAndStart(phaseName);
  bool success = phaseResult.IsSuccess();
  if (!success) {
    return phaseResult.Finish(success);
  }
  // build target map
  std::map<const GeneralStmt*, GeneralBB*> mapTargetStmtBB;
  FELinkListNode *nodeBB = genBBHead->GetNext();
  while (nodeBB != nullptr && nodeBB != genBBTail) {
    GeneralBB *bb = static_cast<GeneralBB*>(nodeBB);
    const GeneralStmt *stmtHead = bb->GetStmtNoAuxHead();
    if (stmtHead != nullptr && stmtHead->GetGeneralStmtKind() == GeneralStmtKind::kStmtMultiIn) {
      mapTargetStmtBB[stmtHead] = bb;
    }
    nodeBB = nodeBB->GetNext();
  }
  // link
  nodeBB = genBBHead->GetNext();
  bool firstBB = true;
  while (nodeBB != nullptr && nodeBB != genBBTail) {
    GeneralBB *bb = static_cast<GeneralBB*>(nodeBB);
    if (firstBB) {
      bb->AddPredBB(*genBBHead);
      genBBHead->AddSuccBB(*bb);
      firstBB = false;
    }
    const GeneralStmt *stmtTail = bb->GetStmtNoAuxTail();
    CHECK_FATAL(stmtTail != nullptr, "stmt tail is nullptr");
    if (stmtTail->IsFallThru()) {
      FELinkListNode *nodeBBNext = nodeBB->GetNext();
      if (nodeBBNext == nullptr || nodeBBNext == genBBTail) {
        ERR(kLncErr, "Method without return");
        return phaseResult.Finish(false);
      }
      GeneralBB *bbNext = static_cast<GeneralBB*>(nodeBBNext);
      bb->AddSuccBB(*bbNext);
      bbNext->AddPredBB(*bb);
    }
    if (stmtTail->GetGeneralStmtKind() == GeneralStmtKind::kStmtMultiOut) {
      for (GeneralStmt *stmt : stmtTail->GetSuccs()) {
        auto itBB = mapTargetStmtBB.find(stmt);
        CHECK_FATAL(itBB != mapTargetStmtBB.end(), "Target BB is not found");
        GeneralBB *bbNext = itBB->second;
        bb->AddSuccBB(*bbNext);
        bbNext->AddPredBB(*bb);
      }
    }
    nodeBB = nodeBB->GetNext();
  }
  return phaseResult.Finish(success);
}

bool FEFunction::CheckDeadBB(const std::string &phaseName) {
  phaseResult.RegisterPhaseNameAndStart(phaseName);
  bool success = phaseResult.IsSuccess();
  if (!success) {
    return phaseResult.Finish(success);
  }
  uint32 nDeadBB = 0;
  FELinkListNode *nodeBB = genBBHead->GetNext();
  while (nodeBB != nullptr && nodeBB != genBBTail) {
    GeneralBB *bb = static_cast<GeneralBB*>(nodeBB);
    ASSERT(bb != nullptr, "nullptr check");
    if (bb->IsDead()) {
      nDeadBB++;
    }
    nodeBB = nodeBB->GetNext();
  }
  if (nDeadBB > 0) {
    ERR(kLncErr, "Dead BB existed");
    success = false;
  }
  return phaseResult.Finish(success);
}

void FEFunction::LabelGenStmt() {
  FELinkListNode *nodeStmt = genStmtHead;
  uint32 idx = 0;
  while (nodeStmt != nullptr) {
    GeneralStmt *stmt = static_cast<GeneralStmt*>(nodeStmt);
    stmt->SetID(idx);
    idx++;
    nodeStmt = nodeStmt->GetNext();
  }
}

void FEFunction::LabelGenBB() {
  FELinkListNode *nodeBB = genBBHead;
  uint32 idx = 0;
  while (nodeBB != nullptr) {
    GeneralBB *bb = static_cast<GeneralBB*>(nodeBB);
    bb->SetID(idx);
    idx++;
    nodeBB = nodeBB->GetNext();
  }
}

bool FEFunction::HasDeadBB() {
  FELinkListNode *nodeBB = genBBHead->GetNext();
  while (nodeBB != nullptr && nodeBB != genBBTail) {
    GeneralBB *bb = static_cast<GeneralBB*>(nodeBB);
    if (bb->IsDead()) {
      return true;
    }
    nodeBB = nodeBB->GetNext();
  }
  return false;
}

void FEFunction::DumpGeneralStmts() {
  FELinkListNode *nodeStmt = genStmtHead;
  while (nodeStmt != nullptr) {
    GeneralStmt *stmt = static_cast<GeneralStmt*>(nodeStmt);
    stmt->Dump();
    nodeStmt = nodeStmt->GetNext();
  }
}

void FEFunction::DumpGeneralBBs() {
  FELinkListNode *nodeBB = genBBHead->GetNext();
  while (nodeBB != nullptr && nodeBB != genBBTail) {
    GeneralBB *bb = static_cast<GeneralBB*>(nodeBB);
    bb->Dump();
    nodeBB = nodeBB->GetNext();
  }
}

void FEFunction::DumpGeneralCFGGraph() {
  MPLFE_PARALLEL_FORBIDDEN();
  if (!FEOptions::GetInstance().IsDumpGeneralCFGGraph()) {
    return;
  }
  std::string fileName = FEOptions::GetInstance().GetJBCCFGGraphFileName();
  CHECK_FATAL(!fileName.empty(), "General CFG Graph FileName is empty");
  std::ofstream file(fileName);
  CHECK_FATAL(file.is_open(), "Failed to open General CFG Graph FileName: %s", fileName.c_str());
  file << "digraph {" << std::endl;
  file << "  # /* " << GetGeneralFuncName() << " */" << std::endl;
  FELinkListNode *nodeBB = genBBHead->GetNext();
  while (nodeBB != nullptr && nodeBB != genBBTail) {
    GeneralBB *bb = static_cast<GeneralBB*>(nodeBB);
    DumpGeneralCFGGraphForBB(file, *bb);
    nodeBB = nodeBB->GetNext();
  }
  DumpGeneralCFGGraphForCFGEdge(file);
  DumpGeneralCFGGraphForDFGEdge(file);
  file << "}" << std::endl;
  file.close();
}

void FEFunction::DumpGeneralCFGGraphForBB(std::ofstream &file, const GeneralBB &bb) {
  file << "  BB" << bb.GetID() << " [shape=record,label=\"{" << std::endl;
  const FELinkListNode *nodeStmt = bb.GetStmtHead();
  while (nodeStmt != nullptr) {
    const GeneralStmt *stmt = static_cast<const GeneralStmt*>(nodeStmt);
    file << "      " << stmt->DumpDotString();
    if (nodeStmt == bb.GetStmtTail()) {
      file << std::endl;
      break;
    } else {
      file << " |" << std::endl;
    }
    nodeStmt = nodeStmt->GetNext();
  }
  file << "    }\"];" << std::endl;
}

void FEFunction::DumpGeneralCFGGraphForCFGEdge(std::ofstream &file) {
  file << "  subgraph cfg_edges {" << std::endl;
  file << "    edge [color=\"#000000\",weight=0.3,len=3];" << std::endl;
  const FELinkListNode *nodeBB = genBBHead->GetNext();
  while (nodeBB != nullptr && nodeBB != genBBTail) {
    const GeneralBB *bb = static_cast<const GeneralBB*>(nodeBB);
    const GeneralStmt *stmtS = bb->GetStmtTail();
    for (GeneralBB *bbNext : bb->GetSuccBBs()) {
      const GeneralStmt *stmtE = bbNext->GetStmtHead();
      file << "    BB" << bb->GetID() << ":stmt" << stmtS->GetID() << " -> ";
      file << "BB" << bbNext->GetID() << ":stmt" << stmtE->GetID() << std::endl;
    }
    nodeBB = nodeBB->GetNext();
  }
  file << "  }" << std::endl;
}

void FEFunction::DumpGeneralCFGGraphForDFGEdge(std::ofstream &file) {
  file << "  subgraph cfg_edges {" << std::endl;
  file << "    edge [color=\"#00FF00\",weight=0.3,len=3];" << std::endl;
  file << "  }" << std::endl;
}

bool FEFunction::BuildGeneralStmtBBMap(const std::string &phaseName) {
  phaseResult.RegisterPhaseNameAndStart(phaseName);
  FELinkListNode *nodeBB = genBBHead->GetNext();
  while (nodeBB != nullptr && nodeBB != genBBTail) {
    GeneralBB *bb = static_cast<GeneralBB*>(nodeBB);
    const FELinkListNode *nodeStmt = bb->GetStmtHead();
    while (nodeStmt != nullptr) {
      const GeneralStmt *stmt = static_cast<const GeneralStmt*>(nodeStmt);
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
    GeneralStmt *stmt = static_cast<GeneralStmt*>(nodeStmt);
    stmt->SetID(idx);
    idx++;
    nodeStmt = nodeStmt->GetNext();
  }
  return phaseResult.Finish();
}

bool FEFunction::LabelGeneralBBs(const std::string &phaseName) {
  phaseResult.RegisterPhaseNameAndStart(phaseName);
  uint32 idx = 0;
  FELinkListNode *nodeBB = genBBHead->GetNext();
  while (nodeBB != nullptr && nodeBB != genBBTail) {
    GeneralBB *bb = static_cast<GeneralBB*>(nodeBB);
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
  MPLFE_PARALLEL_FORBIDDEN();
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
  if (FEOptions::GetInstance().GetTypeInferKind() == FEOptions::kRoahAlgorithm) {
    success = success && BuildFEIRBB("fe/build feir bb");
    success = success && BuildFEIRCFG("fe/build feir CFG");
    success = success && BuildFEIRDFG("fe/build feir DFG");
    success = success && BuildFEIRUDDU("fe/build feir UDDU");
    success = success && TypeInfer("fe/type infer");
  }
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
  GStrIdx thisNameIdx = GlobalTables::GetStrTable().GetStrIdxFromName("_this");
  uint32 nameIdx = static_cast<uint32>(thisNameIdx);
  CHECK_FATAL((nameIdx != 0), "StringIndex for \"_this\" should has been created before this phase");
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

bool FEFunction::BuildFEIRBB(const std::string &phaseName) {
  bool success = CheckPhaseResult(phaseName);
  if (!success) {
    return success;
  }
  LabelFEIRStmts();
  FEIRBB *currBB = nullptr;
  uint32 bbID = 1;
  FELinkListNode *node = feirStmtHead->GetNext();
  while (node != feirStmtTail) {
    FEIRStmt *currStmt = static_cast<FEIRStmt*>(node);
    if (ShouldNewBB(currBB, *currStmt)) {
      currBB = NewFEIRBB(bbID);
      FELinkListNode::InsertBefore(currBB, feirBBTail);
    }
    currBB->AppendStmt(*currStmt);
    (void)feirStmtBBMap.insert(std::make_pair(currStmt, currBB));
    if (IsBBEnd(*currStmt)) {
      currBB = nullptr;
    }
    node = node->GetNext();
  }
  return phaseResult.Finish(success);
}

bool FEFunction::BuildFEIRCFG(const std::string &phaseName) {
  bool success = CheckPhaseResult(phaseName);
  if (!success) {
    return success;
  }
  FELinkListNode *node = feirBBHead->GetNext();
  while (node != feirBBTail) {
    FEIRBB *currBB = static_cast<FEIRBB*>(node);
    LinkFallThroughBBAndItsNext(*currBB);
    LinkBranchBBAndItsTargets(*currBB);
    node = currBB->GetNext();
  }
  return phaseResult.Finish(success);
}

bool FEFunction::BuildFEIRDFG(const std::string &phaseName) {
  bool success = CheckPhaseResult(phaseName);
  if (!success) {
    return success;
  }
  ProcessCheckPoints();
  return phaseResult.Finish(success);
}

bool FEFunction::BuildFEIRUDDU(const std::string &phaseName) {
  bool success = CheckPhaseResult(phaseName);
  if (!success) {
    return success;
  }
  InitFirstVisibleStmtForCheckPoints();
  InitFEIRStmtCheckPointMap();
  RegisterDFGNodes2CheckPoints();
  success = success && CalculateDefs4AllUses();  // build Use-Def Chain
  FEIRDFG::CalculateDefUseByUseDef(defUseChain, useDefChain);  // build Def-Use Chain
  return phaseResult.Finish(success);
}

bool FEFunction::TypeInfer(const std::string &phaseName) {
  bool success = CheckPhaseResult(phaseName);
  if (!success) {
    return success;
  }
  InitTrans4AllVars();
  typeInfer = std::make_unique<FEIRTypeInfer>(srcLang, defUseChain);
  for (auto it : defUseChain) {
    UniqueFEIRVar *def = it.first;
    typeInfer->ProcessVarDef(*def);
  }
  SetUpDefVarTypeScatterStmtMap();
  if (defVarTypeScatterStmtMap.size() == 0) {
    return phaseResult.Finish(success);
  }
  for (auto it : defUseChain) {
    UniqueFEIRVar *def = it.first;
    InsertRetypeStmtsAfterDef(*def);
  }
  return phaseResult.Finish(success);
}

void FEFunction::OutputUseDefChain() {
  std::cout << "useDefChain : {" << std::endl;
  for (auto it : useDefChain) {
    UniqueFEIRVar *use = it.first;
    std::cout << "  use : " << (*use)->GetNameRaw() << "_" << GetPrimTypeName((*use)->GetType()->GetPrimType());
    std::cout << " defs : [";
    std::set<UniqueFEIRVar*> &defs = it.second;
    for (UniqueFEIRVar *def : defs) {
      std::cout << (*def)->GetNameRaw() << "_" << GetPrimTypeName((*def)->GetType()->GetPrimType()) << ", ";
    }
    if (defs.size() == 0) {
      std::cout << "empty defs";
    }
    std::cout << " ]" << std::endl;
  }
  std::cout << "}" << std::endl;
}

void FEFunction::OutputDefUseChain() {
  std::cout << "defUseChain : {" << std::endl;
  for (auto it : defUseChain) {
    UniqueFEIRVar *def = it.first;
    std::cout << "  def : " << (*def)->GetNameRaw() << "_" << GetPrimTypeName((*def)->GetType()->GetPrimType());
    std::cout << " uses : [";
    std::set<UniqueFEIRVar*> &uses = it.second;
    for (UniqueFEIRVar *use : uses) {
      std::cout << (*use)->GetNameRaw()  << "_" << GetPrimTypeName((*use)->GetType()->GetPrimType()) << ", ";
    }
    if (uses.size() == 0) {
      std::cout << "empty uses";
    }
    std::cout << " ]" << std::endl;
  }
  std::cout << "}" << std::endl;
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

FEIRBB *FEFunction::NewFEIRBB(uint32 &bbID) {
  std::unique_ptr<FEIRBB> uniqueFEIRBB = std::make_unique<FEIRBB>(GeneralBBKind::kBBKindDefault);
  uniqueFEIRBB->SetID(bbID);
  bbID++;
  return RegisterFEIRBB(std::move(uniqueFEIRBB));
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
  return (stmt.IsBranch() || !stmt.IsFallThrough());
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
  const GeneralStmt *generalStmtTail = bb.GetStmtNoAuxTail();
  const FEIRStmt *stmtTail = static_cast<const FEIRStmt*>(generalStmtTail);
  FEIRNodeKind nodeKind = stmtTail->GetKind();
  switch (nodeKind) {
    case FEIRNodeKind::kStmtCondGoto:
      // should fallthrough
      [[fallthrough]];
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
  predBB.AddSuccBB(succBB);
  succBB.AddPredBB(predBB);
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

void FEFunction::ProcessCheckPoints() {
  InsertCheckPointForBBs();
  InsertCheckPointForTrys();
  LabelFEIRStmts();
  LinkCheckPointBetweenBBs();
  LinkCheckPointInsideBBs();
  LinkCheckPointForTrys();
}

void FEFunction::InsertCheckPointForBBs() {
  FELinkListNode *node = feirBBHead->GetNext();
  while (node != feirBBTail) {
    FEIRBB *currBB = static_cast<FEIRBB*>(node);  // get currBB
    // create chekPointIn
    std::unique_ptr<FEIRStmtCheckPoint> chekPointIn = std::make_unique<FEIRStmtCheckPoint>();
    currBB->SetCheckPointIn(std::move(chekPointIn));  // set to currBB's checkPointIn
    FEIRStmtCheckPoint &cpIn = currBB->GetCheckPointIn();
    currBB->InsertAndUpdateNewHead(cpIn);  // insert and update new head to chekPointIn
    (void)feirStmtBBMap.insert(std::make_pair(&cpIn, currBB));  // add pair to feirStmtBBMap
    // create chekPointOut
    std::unique_ptr<FEIRStmtCheckPoint> chekPointOut = std::make_unique<FEIRStmtCheckPoint>();
    currBB->SetCheckPointOut(std::move(chekPointOut));  // set to currBB's checkPointOut
    FEIRStmtCheckPoint &cpOut = currBB->GetCheckPointOut();
    currBB->InsertAndUpdateNewTail(cpOut);  // insert and update new tail to chekPointOut
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
          currBB.SetStmtHead(*currStmt);
        }
        node = node->GetNext();
        continue;
      }
      std::unique_ptr<FEIRStmtCheckPoint> newCheckPoint = std::make_unique<FEIRStmtCheckPoint>();
      currBB.AddCheckPointInside(std::move(newCheckPoint));
      checkPointInTry = currBB.GetLatestCheckPointInside();
      CHECK_NULL_FATAL(checkPointInTry);
      FELinkListNode::InsertBefore(checkPointInTry, currStmt);
      (void)feirStmtBBMap.insert(std::make_pair(checkPointInTry, &currBB));
      (void)checkPointJavaTryMap.insert(std::make_pair(checkPointInTry, currTry));
      if (currStmt == currBB.GetStmtHead()) {
        currBB.SetStmtHead(*currStmt);
      }
    }
    if (currStmt->GetKind() == FEIRNodeKind::kStmtPesudoEndTry) {
      currTry = nullptr;
    }
    node = node->GetNext();
  }
}

void FEFunction::LinkCheckPointBetweenBBs() {
  FELinkListNode *node = feirBBHead->GetNext();
  while (node != feirBBTail) {
    FEIRBB *currBB = static_cast<FEIRBB*>(node);
    currBB->LinkSuccBBsCheckPoints();
    node = node->GetNext();
  }
}

void FEFunction::LinkCheckPointInsideBBs() {
  FELinkListNode *node = feirBBHead->GetNext();
  while (node != feirBBTail) {
    FEIRBB *currBB = static_cast<FEIRBB*>(node);
    currBB->LinkCheckPointsInside();
    node = node->GetNext();
  }
}

void FEFunction::LinkCheckPointForTrys() {
  FELinkListNode *node = feirBBHead->GetNext();
  while (node != feirBBTail) {
    FEIRBB *currBB = static_cast<FEIRBB*>(node);
    FEIRStmtCheckPoint &checkPointIn = currBB->GetCheckPointIn();
    if (checkPointJavaTryMap.find(&checkPointIn) != checkPointJavaTryMap.end()) {
      LinkCheckPointForTry(checkPointIn);
    }
    const std::vector<std::unique_ptr<FEIRStmtCheckPoint>> &checkPointsInside = currBB->GetCheckPointsInside();
    for (const std::unique_ptr<FEIRStmtCheckPoint> &currCheckPoint : checkPointsInside) {
      LinkCheckPointForTry(*(currCheckPoint.get()));
    }
    node = node->GetNext();
  }
}

void FEFunction::LinkCheckPointForTry(FEIRStmtCheckPoint &checkPoint) {
  FEIRStmtPesudoJavaTry2 &currTry = GetJavaTryByCheckPoint(checkPoint);
  const std::vector<FEIRStmtPesudoLabel2*> &catchTargets = currTry.GetCatchTargets();
  for (FEIRStmtPesudoLabel2 *label : catchTargets) {
    FEIRBB &targetBB = GetFEIRBBByStmt(*label);
    FEIRStmtCheckPoint &checkPointIn = targetBB.GetCheckPointIn();
    checkPointIn.AddPredCheckPoint(checkPoint);
  }
}

void FEFunction::InitFirstVisibleStmtForCheckPoints() {
  FELinkListNode *node = feirBBHead->GetNext();
  while (node != feirBBTail) {
    FEIRBB *currBB = static_cast<FEIRBB*>(node);
    currBB->InitFirstVisibleStmtForCheckPoints();
    node = node->GetNext();
  }
}

void FEFunction::InitFEIRStmtCheckPointMap() {
  FELinkListNode *node = feirBBHead->GetNext();
  while (node != feirBBTail) {
    FEIRBB *currBB = static_cast<FEIRBB*>(node);
    const std::map<const FEIRStmt*, FEIRStmtCheckPoint*> &stmtCheckPointMap = currBB->GetFEIRStmtCheckPointMap();
    feirStmtCheckPointMap.insert(stmtCheckPointMap.begin(), stmtCheckPointMap.end());
    node = node->GetNext();
  }
}

void FEFunction::RegisterDFGNodes2CheckPoints() {
  RegisterDFGNodesForFuncParameters();
  RegisterDFGNodesForStmts();
}

void FEFunction::RegisterDFGNodesForStmts() {
  FELinkListNode *node = feirBBHead->GetNext();
  while (node != feirBBTail) {
    FEIRBB *currBB = static_cast<FEIRBB*>(node);
    currBB->RegisterDFGNodes2CheckPoints();
    node = node->GetNext();
  }
}

void FEFunction::RegisterDFGNodesForFuncParameters() {
  if (argVarList.size() == 0) {
    return;
  }
  FELinkListNode *node = feirBBHead->GetNext();
  if (node == feirBBTail) {
    OutputStmts();
    CHECK_FATAL((feirStmtHead->GetNext() == feirStmtTail), "there is no bb in function");
  }
  FEIRBB *funcHeadBB = static_cast<FEIRBB*>(node);
  FEIRStmtCheckPoint &checkPointIn = funcHeadBB->GetCheckPointIn();
  for (std::unique_ptr<FEIRVar> &argVar : argVarList) {
    argVar->SetDef(true);
    checkPointIn.RegisterDFGNode(argVar);
  }
}

bool FEFunction::CalculateDefs4AllUses() {
  bool success = true;
  FELinkListNode *node = feirStmtHead->GetNext();
  while (node != feirStmtTail) {
    FEIRStmt *currStmt = static_cast<FEIRStmt*>(node);
    if (currStmt->GetKind() != FEIRNodeKind::kStmtCheckPoint) {
      FEIRStmtCheckPoint &checkPoint = GetCheckPointByFEIRStmt(*currStmt);
      success = success && currStmt->CalculateDefs4AllUses(checkPoint, useDefChain);
    }
    node = node->GetNext();
  }
  return success;
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

void FEFunction::EmitToMIRStmt() {
  FELinkListNode *nodeStmt = feirStmtHead->GetNext();
  while (nodeStmt != nullptr && nodeStmt != feirStmtTail) {
    FEIRStmt *stmt = static_cast<FEIRStmt*>(nodeStmt);
    std::list<StmtNode*> mirStmts = stmt->GenMIRStmts(FEManager::GetMIRBuilder());
#ifdef DEBUG
    // LOC info has been recorded in FEIRStmt already, this could be removed later.
    AddLocForStmt(*stmt, mirStmts);
#endif
    for (StmtNode *mirStmt : mirStmts) {
      FEManager::GetMIRBuilder().AddStmtInCurrentFunctionBody(*mirStmt);
    }
    nodeStmt = nodeStmt->GetNext();
  }
}

void FEFunction::AddLocForStmt(const FEIRStmt &stmt, std::list<StmtNode*> &mirStmts) const {
  const FEIRStmtPesudoLOC *pesudoLoc = GetLOCForStmt(stmt);
  if (pesudoLoc != nullptr) {
    mirStmts.front()->GetSrcPos().SetFileNum(static_cast<uint16>(pesudoLoc->GetSrcFileIdx()));
    mirStmts.front()->GetSrcPos().SetLineNum(pesudoLoc->GetLineNumber());
  }
}
}  // namespace maple
