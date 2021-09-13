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
#include "inline.h"
#include <iostream>
#include <fstream>
#include "mpl_logging.h"
#include "global_tables.h"

namespace maple {
// This phase replaces a function call site with the body of the called function.
//   Step 0: See if CALLEE have been inlined to CALLER once.
//   Step 1: Clone CALLEE's body.
//   Step 2: Rename symbols, labels, pregs.
//   Step 3: Replace symbols, labels, pregs.
//   Step 4: Null check 'this' and assign actuals to formals.
//   Step 5: Insert the callee'return jump dest label.
//   Step 6: Handle return values.
//   Step 7: Remove the successive goto statement and label statement in some circumstances.
//   Step 8: Replace the call-stmt with new CALLEE'body.
//   Step 9: Update inlined_times.
constexpr uint32 kHalfInsn = 1;
constexpr uint32 kOneInsn = 2;
constexpr uint32 kDoubleInsn = 4;
constexpr uint32 kPentupleInsn = 10;
constexpr uint32 kBigFuncNumStmts = 1000;

static int GetNumStmtsOfFunc(const MIRFunction &func) {
  if (func.GetBody() == nullptr) {
    return 0;
  }
  const auto &stmtNodes = func.GetBody()->GetStmtNodes();
  if (stmtNodes.empty()) {
    return 0;
  }
  return stmtNodes.back().GetStmtID() - stmtNodes.front().GetStmtID();
}

static bool IsFinalMethod(const MIRFunction *mirFunc) {
  if (mirFunc == nullptr) {
    return false;
  }
  const auto *cType = static_cast<const MIRClassType*>(mirFunc->GetClassType());
  // Return true if the method or its class is declared as final
  return (cType != nullptr && (mirFunc->IsFinal() || cType->IsFinal()));
}

void MInline::Init() {
  InitParams();
  if (inlineWithProfile) {
    InitProfile();
  }
  InitRCWhiteList();
  InitExcludedCaller();
  InitExcludedCallee();
}

void MInline::InitParams() {
  dumpDetail = (Options::dumpPhase == "inline");
  dumpFunc = Options::dumpFunc;
  inlineFuncList = MeOption::inlineFuncList;
  noInlineFuncList = Options::noInlineFuncList;
  smallFuncThreshold = Options::inlineSmallFunctionThreshold;
  hotFuncThreshold = Options::inlineHotFunctionThreshold;
  recursiveFuncThreshold = Options::inlineRecursiveFunctionThreshold;
  inlineWithProfile = Options::inlineWithProfile;
}

void MInline::InitProfile() const {
  uint32 dexNameIdx = module.GetFileinfo(GlobalTables::GetStrTable().GetOrCreateStrIdxFromName("INFO_filename"));
  const std::string &dexName = GlobalTables::GetStrTable().GetStringFromStrIdx(GStrIdx(dexNameIdx));
  bool deCompressSucc = module.GetProfile().DeCompress(Options::profile, dexName);
  LogInfo::MapleLogger() << "dexName: " << dexName << '\n';
  if (!deCompressSucc) {
    LogInfo::MapleLogger() << "WARN: DeCompress() failed in DoInline::Run()\n";
  }
}

void MInline::InitRCWhiteList() {
  std::set<std::string> whitelistFunc{
#include "rcwhitelist.def"
  };
  for (auto it = whitelistFunc.begin(); it != whitelistFunc.end(); ++it) {
    GStrIdx strIdx = GlobalTables::GetStrTable().GetStrIdxFromName(*it);
    (void)rcWhiteList.insert(strIdx);
  }
}

void MInline::InitExcludedCaller() {
  std::set<std::string> specialfunc = {
    std::string("Landroid_2Fcontent_2Fpm_2FFallbackCategoryProvider_3B_7CloadFallbacks_7C_28_29V"),
  };
  for (auto it = specialfunc.begin(); it != specialfunc.end(); ++it) {
    GStrIdx strIdx = GlobalTables::GetStrTable().GetStrIdxFromName(*it);
    (void)excludedCaller.insert(strIdx);
  }
}

void MInline::InitExcludedCallee() {
  std::set<std::string> excludedFunc{
#include "castcheck_whitelist.def"
#define PROPILOAD(funcname) #funcname,
#include "propiloadlist.def"
#undef PROPILOAD
#define DEF_MIR_INTRINSIC(X, NAME, INTRN_CLASS, RETURN_TYPE, ...) NAME,
#include "simplifyintrinsics.def"
#undef DEF_MIR_INTRINSIC
  };
  for (auto it = excludedFunc.begin(); it != excludedFunc.end(); ++it) {
    GStrIdx strIdx = GlobalTables::GetStrTable().GetStrIdxFromName(*it);
    (void)excludedCallee.insert(strIdx);
  }
  std::set<std::string> setArrayHotFunc = {
      std::string("Landroid_2Ficu_2Fimpl_2Fduration_2FBasicDurationFormat_3B_7CformatDuration_7C_28Ljava_2Flang_2FObject_3B_29Ljava_2Flang_2FString_3B"),
      std::string("Landroid_2Fapp_2FIActivityManager_24Stub_3B_7ConTransact_7C_28ILandroid_2Fos_2FParcel_3BLandroid_2Fos_2FParcel_3BI_29Z"),
      std::string("Landroid_2Fcontent_2Fpm_2FIPackageManager_24Stub_3B_7ConTransact_7C_28ILandroid_2Fos_2FParcel_3BLandroid_2Fos_2FParcel_3BI_29Z"),
      std::string("Landroid_2Fcontent_2FIContentService_24Stub_3B_7ConTransact_7C_28ILandroid_2Fos_2FParcel_3BLandroid_2Fos_2FParcel_3BI_29Z"),
      std::string("Lcom_2Fandroid_2Fserver_2Fam_2FHwActivityManagerService_3B_7ConTransact_7C_28ILandroid_2Fos_2FParcel_3BLandroid_2Fos_2FParcel_3BI_29Z"),
      std::string("Lcom_2Fandroid_2Fserver_2Fam_2FActivityManagerService_3B_7ConTransact_7C_28ILandroid_2Fos_2FParcel_3BLandroid_2Fos_2FParcel_3BI_29Z"),
      std::string("Ljava_2Flang_2Freflect_2FMethod_3B_7Cinvoke_7C_28Ljava_2Flang_2FObject_3BALjava_2Flang_2FObject_3B_29Ljava_2Flang_2FObject_3B"),
      std::string("Lcom_2Fandroid_2Fserver_2FSystemServer_3B_7Crun_7C_28_29V"),
      std::string("Lcom_2Fandroid_2Finternal_2Ftelephony_2FIPhoneStateListener_24Stub_24Proxy_3B_7ConMessageWaitingIndicatorChanged_7C_28Z_29V"),
      std::string("Landroid_2Fview_2Fanimation_2FTransformation_3B_7C_3Cinit_3E_7C_28_29V"),
      std::string("Lcom_2Fandroid_2Fserver_2FSystemServer_3B_7CstartOtherServices_7C_28_29V"),
      std::string("Lcom_2Fandroid_2Fserver_2Fpm_2FSettings_3B_7CreadLPw_7C_28Ljava_2Futil_2FList_3B_29Z"),
      std::string("Lcom_2Fandroid_2Fserver_2Fam_2FActivityManagerService_3B_7CupdateOomAdjLocked_7C_28_29V"),
      std::string("Lcom_2Fandroid_2Fserver_2Fpm_2FHwPackageManagerService_3B_7ConTransact_7C_28ILandroid_2Fos_2FParcel_3BLandroid_2Fos_2FParcel_3BI_29Z"),
      std::string("Lcom_2Fandroid_2Fserver_2Fpm_2FPackageManagerService_3B_7CgeneratePackageInfo_7C_28Lcom_2Fandroid_2Fserver_2Fpm_2FPackageSetting_3BII_29Landroid_2Fcontent_2Fpm_2FPackageInfo_3B"),
      std::string("Ljava_2Flang_2FThrowable_3B_7CprintStackTrace_7C_28Ljava_2Flang_2FThrowable_24PrintStreamOrWriter_3B_29V"),
      std::string("Lcom_2Fandroid_2Fserver_2FSystemServer_3B_7CstartBootstrapServices_7C_28_29V"),
      std::string("Ljava_2Flang_2FThrowable_3B_7CgetOurStackTrace_7C_28_29ALjava_2Flang_2FStackTraceElement_3B"),
      std::string("Ldalvik_2Fsystem_2FVMStack_3B_7CgetStackClass2_7C_28_29Ljava_2Flang_2FClass_3B"),
      std::string("Lcom_2Fandroid_2Fserver_2Fam_2FActivityManagerService_3B_7CattachApplicationLocked_7C_28Landroid_2Fapp_2FIApplicationThread_3BI_29Z"),
      std::string("Lcom_2Fandroid_2Fserver_2FInputMethodManagerService_3B_7ChideCurrentInputLocked_7C_28ILandroid_2Fos_2FResultReceiver_3B_29Z"),
  };
  for (auto it = setArrayHotFunc.begin(); it != setArrayHotFunc.end(); ++it) {
    GStrIdx strIdx = GlobalTables::GetStrTable().GetStrIdxFromName(*it);
    (void)excludedCallee.insert(strIdx);
  }
}

// trim both leading and trailing space and tab
static void TrimString(std::string &str) {
  size_t pos = str.find_first_not_of(kSpaceTabStr);
  if (pos != std::string::npos) {
    str = str.substr(pos);
  } else {
    str.clear();
  }
  pos = str.find_last_not_of(kSpaceTabStr);
  if (pos != std::string::npos) {
    str = str.substr(0, pos + 1);
  }
}

void MInline::ApplyInlineListInfo(const std::string &list, MapleMap<GStrIdx, MapleSet<GStrIdx>*> &listCallee) {
  if (list.empty()) {
    return;
  }
  std::ifstream infile(list);
  if (!infile.is_open()) {
    LogInfo::MapleLogger(kLlErr) << "Cannot open function list file " << list << '\n';
    return;
  }
  LogInfo::MapleLogger() << "[INLINE] read function from list: " << list << '\n';
  std::string str;
  GStrIdx calleeStrIdx;
  while (getline(infile, str)) {
    TrimString(str);
    if (str.empty() || str[0] == kCommentsignStr[0]) {
      continue;
    }
    if (str[0] != kHyphenStr[0]) {
      calleeStrIdx = GlobalTables::GetStrTable().GetStrIdxFromName(str);
      auto it = listCallee.find(calleeStrIdx);
      if (it == listCallee.end()) {
        auto callerList = alloc.GetMemPool()->New<MapleSet<GStrIdx>>(alloc.Adapter());
        (void)listCallee.insert(std::pair<GStrIdx, MapleSet<GStrIdx>*>(calleeStrIdx, callerList));
      }
    } else {
      size_t pos = str.find_first_not_of(kAppointStr);
      CHECK_FATAL(pos != std::string::npos, "cannot find '->' ");
      str = str.substr(pos);
      GStrIdx callerStrIdx = GlobalTables::GetStrTable().GetStrIdxFromName(str);
      auto it = listCallee.find(calleeStrIdx);
      CHECK_FATAL(it != listCallee.end(), "illegal configuration for inlineList");
      it->second->insert(callerStrIdx);
    }
  }
  infile.close();
}

// Common rename function
uint32 MInline::RenameSymbols(MIRFunction &caller, const MIRFunction &callee, uint32 inlinedTimes) const {
  size_t symTabSize = callee.GetSymbolTabSize();
  size_t stIdxOff = caller.GetSymTab()->GetSymbolTableSize() - 1;
  for (size_t i = 0; i < symTabSize; ++i) {
    const MIRSymbol *sym = callee.GetSymbolTabItem(i);
    if (sym == nullptr) {
      continue;
    }
    CHECK_FATAL(sym->GetStorageClass() != kScPstatic, "pstatic symbols should have been converted to fstatic ones");
    std::string syName(kUnderlineStr);
    // Use puIdx here instead of func name because our mangled func name can be
    // really long.
    syName.append(std::to_string(callee.GetPuidx()));
    syName.append(kVerticalLineStr);
    syName.append((sym->GetName() == "") ? std::to_string(i) : sym->GetName());
    syName.append(kUnderlineStr);
    if (!module.firstInline) {
      syName.append("SECOND_");
    }
    GStrIdx strIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(syName + std::to_string(inlinedTimes));
    if (sym->GetScopeIdx() == 0) {
      CHECK_FATAL(false, "sym->GetScopeIdx() should be != 0");
    }
    MIRSymbol *newSym = nullptr;
    newSym = caller.GetSymTab()->CloneLocalSymbol(*sym);
    newSym->SetNameStrIdx(strIdx);
    if (newSym->GetStorageClass() == kScFormal) {
      newSym->SetStorageClass(kScAuto);
    }
    newSym->SetIsTmp(true);
    newSym->ResetIsDeleted();
    if (!caller.GetSymTab()->AddStOutside(newSym)) {
      CHECK_FATAL(false, "Reduplicate names.");
    }
    if (newSym->GetStIndex() != (i + stIdxOff)) {
      CHECK_FATAL(false, "wrong symbol table index");
    }
    CHECK_FATAL(caller.GetSymTab()->IsValidIdx(newSym->GetStIndex()), "symbol table index out of range");
  }
  return stIdxOff;
}

static StIdx UpdateIdx(const StIdx &stIdx, uint32 stIdxOff, const std::vector<uint32> *oldStIdx2New) {
  // If the callee has pstatic symbols, we will save all symbol mapping info in the oldStIdx2New.
  // So if this oldStIdx2New is nullptr, we only use stIdxOff to update stIdx, otherwise we only use oldStIdx2New.
  StIdx newStIdx = stIdx;
  if (oldStIdx2New == nullptr) {
    newStIdx.SetIdx(newStIdx.Idx() + stIdxOff);
  } else {
    CHECK_FATAL(newStIdx.Idx() < oldStIdx2New->size(), "stIdx out of range");
    newStIdx.SetFullIdx((*oldStIdx2New)[newStIdx.Idx()]);
  }
  return newStIdx;
}

void MInline::ReplaceSymbols(BaseNode *baseNode, uint32 stIdxOff,
                             const std::vector<uint32> *oldStIdx2New) const {
  if (baseNode == nullptr) {
    return;
  }
  // IfStmtNode's `numOpnds` and actual operands number are different, so we treat it as a special case
  if (baseNode->GetOpCode() == OP_if) {
    IfStmtNode *ifStmtNode = static_cast<IfStmtNode*>(baseNode);
    ReplaceSymbols(baseNode->Opnd(0), stIdxOff, oldStIdx2New);
    if (ifStmtNode->GetThenPart() != nullptr) {
      ReplaceSymbols(ifStmtNode->GetThenPart(), stIdxOff, oldStIdx2New);
    }
    if (ifStmtNode->GetElsePart() != nullptr) {
      ReplaceSymbols(ifStmtNode->GetElsePart(), stIdxOff, oldStIdx2New);
    }
    return;
  }
  CallReturnVector *returnVector = baseNode->GetCallReturnVector();
  if (baseNode->GetOpCode() == OP_block) {
    BlockNode *blockNode = static_cast<BlockNode*>(baseNode);
    for (auto &stmt : blockNode->GetStmtNodes()) {
      ReplaceSymbols(&stmt, stIdxOff, oldStIdx2New);
    }
  } else if (baseNode->GetOpCode() == OP_dassign) {
    DassignNode *dassNode = static_cast<DassignNode*>(baseNode);
    // Skip globals.
    if (dassNode->GetStIdx().Islocal()) {
      dassNode->SetStIdx(UpdateIdx(dassNode->GetStIdx(), stIdxOff, oldStIdx2New));
    }
  } else if ((baseNode->GetOpCode() == OP_addrof || baseNode->GetOpCode() == OP_dread)) {
    AddrofNode *addrNode = static_cast<AddrofNode*>(baseNode);
    // Skip globals.
    if (addrNode->GetStIdx().Islocal()) {
      addrNode->SetStIdx(UpdateIdx(addrNode->GetStIdx(), stIdxOff, oldStIdx2New));
    }
  } else if (returnVector != nullptr) {
    if (returnVector->size() > 1) {
      CHECK_FATAL(false, "multiple return values are not supported");
    }
    // Skip globals.
    if (returnVector->size() == 1 && !(*returnVector).at(0).second.IsReg() && (*returnVector).at(0).first.Islocal()) {
      (*returnVector)[0].first = UpdateIdx((*returnVector).at(0).first, stIdxOff, oldStIdx2New);
    }
  } else if (baseNode->GetOpCode() == OP_foreachelem) {
    ForeachelemNode *forEachNode = static_cast<ForeachelemNode*>(baseNode);
    // Skip globals.
    if (forEachNode->GetElemStIdx().Idx() != 0) {
      forEachNode->SetElemStIdx(UpdateIdx(forEachNode->GetElemStIdx(), stIdxOff, oldStIdx2New));
    }
    if (forEachNode->GetArrayStIdx().Idx() != 0) {
      forEachNode->SetArrayStIdx(UpdateIdx(forEachNode->GetArrayStIdx(), stIdxOff, oldStIdx2New));
    }
  } else if (baseNode->GetOpCode() == OP_doloop) {
    DoloopNode *doLoopNode = static_cast<DoloopNode*>(baseNode);
    // Skip globals.
    if (!doLoopNode->IsPreg() && doLoopNode->GetDoVarStIdx().Idx()) {
      doLoopNode->SetDoVarStIdx(UpdateIdx(doLoopNode->GetDoVarStIdx(), stIdxOff, oldStIdx2New));
    }
  }
  // Search for nested dassign/dread/addrof node that may include a symbol index.
  for (size_t i = 0; i < baseNode->NumOpnds(); ++i) {
    ReplaceSymbols(baseNode->Opnd(i), stIdxOff, oldStIdx2New);
  }
}

uint32 MInline::RenameLabels(MIRFunction &caller, const MIRFunction &callee, uint32 inlinedTimes) const {
  size_t labelTabSize = callee.GetLabelTab()->GetLabelTableSize();
  size_t labIdxOff = caller.GetLabelTab()->GetLabelTableSize() - 1;
  // label table start at 1.
  for (size_t i = 1; i < labelTabSize; ++i) {
    std::string labelName = callee.GetLabelTabItem(static_cast<LabelIdx>(i));
    std::string newLableName(kUnderlineStr);
    newLableName.append(std::to_string(callee.GetPuidx()));
    newLableName.append(kVerticalLineStr);
    newLableName.append(labelName);
    newLableName.append(kUnderlineStr);
    newLableName.append(std::to_string(inlinedTimes));
    GStrIdx strIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(newLableName);
    LabelIdx labIdx = caller.GetLabelTab()->AddLabel(strIdx);
    CHECK_FATAL(labIdx == (i + labIdxOff), "wrong label index");
  }
  return labIdxOff;
}

#define MPLTOOL
void MInline::ReplaceLabels(BaseNode &baseNode, uint32 labIdxOff) const {
  // Now only mpltool would allow try in the callee.
#ifdef MPLTOOL
  if (baseNode.GetOpCode() == OP_try) {
    TryNode *tryNode = static_cast<TryNode*>(&baseNode);
    for (size_t i = 0; i < tryNode->GetOffsetsCount(); ++i) {
      tryNode->SetOffset(tryNode->GetOffset(i) + labIdxOff, i);
    }
  }
#else
  CHECK_FATAL(baseNode->op != OP_try, "Java `try` not allowed");
#endif
  if (baseNode.GetOpCode() == OP_block) {
    BlockNode *blockNode = static_cast<BlockNode*>(&baseNode);
    for (auto &stmt : blockNode->GetStmtNodes()) {
      ReplaceLabels(stmt, labIdxOff);
    }
  } else if (baseNode.IsCondBr()) {
    CondGotoNode *temp = static_cast<CondGotoNode*>(&baseNode);
    temp->SetOffset(temp->GetOffset() + labIdxOff);
  } else if (baseNode.GetOpCode() == OP_label) {
    static_cast<LabelNode*>(&baseNode)->SetLabelIdx(static_cast<LabelNode*>(&baseNode)->GetLabelIdx() + labIdxOff);
  } else if (baseNode.GetOpCode() == OP_addroflabel) {
    uint32 offset = static_cast<AddroflabelNode*>(&baseNode)->GetOffset();
    static_cast<AddroflabelNode*>(&baseNode)->SetOffset(offset + labIdxOff);
  } else if (baseNode.GetOpCode() == OP_goto) {
    static_cast<GotoNode*>(&baseNode)->SetOffset(static_cast<GotoNode*>(&baseNode)->GetOffset() + labIdxOff);
  } else if (baseNode.GetOpCode() == OP_multiway || baseNode.GetOpCode() == OP_rangegoto) {
    ASSERT(false, "MInline::ReplaceLabels: OP_multiway and OP_rangegoto are not supported");
  } else if (baseNode.GetOpCode() == OP_switch) {
    SwitchNode *switchNode = static_cast<SwitchNode*>(&baseNode);
    switchNode->SetDefaultLabel(switchNode->GetDefaultLabel() + labIdxOff);
    for (size_t i = 0; i < switchNode->GetSwitchTable().size(); ++i) {
      LabelIdx idx = switchNode->GetSwitchTable()[i].second + labIdxOff;
      switchNode->UpdateCaseLabelAt(i, idx);
    }
  } else if (baseNode.GetOpCode() == OP_doloop) {
    ReplaceLabels(*static_cast<DoloopNode&>(baseNode).GetDoBody(), labIdxOff);
  } else if (baseNode.GetOpCode() == OP_foreachelem) {
    ReplaceLabels(*static_cast<ForeachelemNode&>(baseNode).GetLoopBody(), labIdxOff);
  } else if (baseNode.GetOpCode() == OP_dowhile || baseNode.GetOpCode() == OP_while) {
    ReplaceLabels(*static_cast<WhileStmtNode&>(baseNode).GetBody(), labIdxOff);
  } else if (baseNode.GetOpCode() == OP_if) {
    ReplaceLabels(*static_cast<IfStmtNode&>(baseNode).GetThenPart(), labIdxOff);
    if (static_cast<IfStmtNode*>(&baseNode)->GetElsePart()) {
      ReplaceLabels(*static_cast<IfStmtNode&>(baseNode).GetElsePart(), labIdxOff);
    }
  }
}

uint32 MInline::RenamePregs(const MIRFunction &caller, const MIRFunction &callee,
                            std::unordered_map<PregIdx, PregIdx> &pregOld2new) const {
  const MapleVector<MIRPreg*> &tab = callee.GetPregTab()->GetPregTable();
  size_t tableSize = tab.size();
  size_t regIdxOff = caller.GetPregTab()->Size() - 1;
  for (size_t i = 1; i < tableSize; ++i) {
    MIRPreg *mirPreg = tab[i];
    PregIdx idx = 0;
    if (mirPreg->GetPrimType() == PTY_ptr || mirPreg->GetPrimType() == PTY_ref) {
      idx = caller.GetPregTab()->ClonePreg(*mirPreg);
    } else {
      idx = caller.GetPregTab()->CreatePreg(mirPreg->GetPrimType());
    }
    (void)pregOld2new.insert(std::pair<PregIdx, PregIdx>(i, idx));
  }
  return regIdxOff;
}

static PregIdx GetNewPregIdx(PregIdx regIdx, std::unordered_map<PregIdx, PregIdx> &pregOld2New) {
  if (regIdx < 0) {
    return regIdx;
  }
  auto it = pregOld2New.find(regIdx);
  if (it == pregOld2New.end()) {
    CHECK_FATAL(false, "Unable to find the regIdx to replace");
  }
  return it->second;
}

void MInline::ReplacePregs(BaseNode *baseNode, std::unordered_map<PregIdx, PregIdx> &pregOld2New) const {
  if (baseNode == nullptr) {
    return;
  }
  switch (baseNode->GetOpCode()) {
    case OP_block: {
      BlockNode *blockNode = static_cast<BlockNode*>(baseNode);
      for (auto &stmt : blockNode->GetStmtNodes()) {
        ReplacePregs(&stmt, pregOld2New);
      }
      break;
    }
    case OP_regassign: {
      RegassignNode *regassign = static_cast<RegassignNode*>(baseNode);
      regassign->SetRegIdx(GetNewPregIdx(regassign->GetRegIdx(), pregOld2New));
      break;
    }
    case OP_regread: {
      RegreadNode *regread = static_cast<RegreadNode*>(baseNode);
      regread->SetRegIdx(GetNewPregIdx(regread->GetRegIdx(), pregOld2New));
      break;
    }
    case OP_doloop: {
      DoloopNode *doloop = static_cast<DoloopNode*>(baseNode);
      if (doloop->IsPreg()) {
        PregIdx oldIdx = static_cast<PregIdx>(doloop->GetDoVarStIdx().FullIdx());
        doloop->SetDoVarStFullIdx(static_cast<uint32>(GetNewPregIdx(oldIdx, pregOld2New)));
      }
      break;
    }
    case OP_callassigned:
    case OP_virtualcallassigned:
    case OP_superclasscallassigned:
    case OP_interfacecallassigned:
    case OP_customcallassigned:
    case OP_polymorphiccallassigned:
    case OP_icallassigned:
    case OP_intrinsiccallassigned:
    case OP_xintrinsiccallassigned:
    case OP_intrinsiccallwithtypeassigned: {
      CallReturnVector *retVec = baseNode->GetCallReturnVector();
      CHECK_FATAL(retVec != nullptr, "retVec is nullptr in MInline::ReplacePregs");
      for (size_t i = 0; i < retVec->size(); ++i) {
        CallReturnPair &callPair = (*retVec).at(i);
        if (callPair.second.IsReg()) {
          PregIdx oldIdx = callPair.second.GetPregIdx();
          callPair.second.SetPregIdx(GetNewPregIdx(oldIdx, pregOld2New));
        }
      }
      break;
    }
    default:
      break;
  }
  for (size_t i = 0; i < baseNode->NumOpnds(); ++i) {
    ReplacePregs(baseNode->Opnd(i), pregOld2New);
  }
}

LabelIdx MInline::CreateReturnLabel(MIRFunction &caller, const MIRFunction &callee, uint32 inlinedTimes) const {
  std::string labelName(kUnderlineStr);
  labelName.append(std::to_string(callee.GetPuidx()));
  labelName.append(kVerticalLineStr);
  labelName.append(kReturnlocStr);
  labelName.append(std::to_string(inlinedTimes));
  GStrIdx strIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(labelName);
  LabelIdx labIdx = caller.GetLabelTab()->AddLabel(strIdx);
  return labIdx;
}

GotoNode *MInline::UpdateReturnStmts(const MIRFunction &caller, BlockNode &newBody, LabelIdx retLabIdx,
                                     const CallReturnVector &callReturnVector, int &retCount) const {
  // For callee: return f ==>
  //             rval = f; goto label x;
  GotoNode *lastGoto = nullptr;
  for (auto &stmt : newBody.GetStmtNodes()) {
    switch (stmt.GetOpCode()) {
      case OP_foreachelem:
        lastGoto = UpdateReturnStmts(caller, *static_cast<ForeachelemNode&>(stmt).GetLoopBody(), retLabIdx,
                                     callReturnVector, retCount);
        break;
      case OP_doloop:
        lastGoto = UpdateReturnStmts(caller, *static_cast<DoloopNode&>(stmt).GetDoBody(),
                                     retLabIdx, callReturnVector, retCount);
        break;
      case OP_dowhile:
      case OP_while:
        lastGoto = UpdateReturnStmts(caller, *static_cast<WhileStmtNode&>(stmt).GetBody(),
                                     retLabIdx, callReturnVector, retCount);
        break;
      case OP_if: {
        IfStmtNode &ifStmt = static_cast<IfStmtNode&>(stmt);
        lastGoto = UpdateReturnStmts(caller, *ifStmt.GetThenPart(), retLabIdx, callReturnVector, retCount);
        if (ifStmt.GetElsePart() != nullptr) {
          lastGoto = UpdateReturnStmts(caller, *ifStmt.GetElsePart(), retLabIdx, callReturnVector, retCount);
        }
        break;
      }
      case OP_return: {
        if (callReturnVector.size() > 1) {
          CHECK_FATAL(false, "multiple return values are not supported");
        }
        ++retCount;
        GotoNode *gotoNode = builder.CreateStmtGoto(OP_goto, retLabIdx);
        lastGoto = gotoNode;
        if (callReturnVector.size() == 1) {
          BaseNode *currBaseNode = static_cast<NaryStmtNode&>(stmt).Opnd(0);
          StmtNode *dStmt = nullptr;
          if (!callReturnVector.at(0).second.IsReg()) {
            dStmt = builder.CreateStmtDassign(
                callReturnVector.at(0).first, callReturnVector.at(0).second.GetFieldID(), currBaseNode);
          } else {
            PregIdx pregIdx = callReturnVector.at(0).second.GetPregIdx();
            const MIRPreg *mirPreg = caller.GetPregTab()->PregFromPregIdx(pregIdx);
            dStmt = builder.CreateStmtRegassign(mirPreg->GetPrimType(), pregIdx, currBaseNode);
          }
          newBody.ReplaceStmt1WithStmt2(&stmt, dStmt);
          newBody.InsertAfter(dStmt, gotoNode);
        } else {
          newBody.ReplaceStmt1WithStmt2(&stmt, gotoNode);
        }
        break;
      }
      default:
        break;
    }
  }
  return lastGoto;
}

void MInline::SearchCallee(const MIRFunction &func, const BaseNode &baseNode, std::set<GStrIdx> &ret) const {
  Opcode op = baseNode.GetOpCode();
  switch (op) {
    case OP_block: {
      const BlockNode &blk = static_cast<const BlockNode&>(baseNode);
      for (auto &stmt : blk.GetStmtNodes()) {
        SearchCallee(func, stmt, ret);
      }
      break;
    }
    case OP_call:
    case OP_callassigned: {
      const CallNode &call = static_cast<const CallNode&>(baseNode);
      MIRFunction *callee = GlobalTables::GetFunctionTable().GetFunctionFromPuidx(call.GetPUIdx());
      (void)ret.insert(callee->GetNameStrIdx());
      break;
    }
    case OP_dread: {
      const DreadNode &dread = static_cast<const DreadNode&>(baseNode);
      const MIRSymbol *mirSymbol = func.GetLocalOrGlobalSymbol(dread.GetStIdx());
      if (mirSymbol->IsStatic()) {
        (void)ret.insert(mirSymbol->GetNameStrIdx());
      }
      break;
    }
    case OP_dassign: {
      const DassignNode &dassign = static_cast<const DassignNode&>(baseNode);
      const MIRSymbol *mirSymbol = func.GetLocalOrGlobalSymbol(dassign.GetStIdx());
      if (mirSymbol->IsStatic()) {
        (void)ret.insert(mirSymbol->GetNameStrIdx());
      }
      break;
    }
    default:
      break;
  }
  for (size_t i = 0; i < baseNode.NumOpnds(); ++i) {
    CHECK_FATAL(baseNode.Opnd(i) != nullptr, "funcName: %s", func.GetName().c_str());
    SearchCallee(func, *(baseNode.Opnd(i)), ret);
  }
}

void MInline::RecordRealCaller(MIRFunction &caller, const MIRFunction &callee) {
  auto it = funcToCostMap.find(&caller);
  if (it != funcToCostMap.end()) {
    (void)funcToCostMap.erase(it);
  }
  auto &realCaller = module.GetRealCaller();
  std::set<GStrIdx> names;
  CHECK_FATAL(callee.GetBody() != nullptr, "funcName: %s", callee.GetName().c_str());
  SearchCallee(callee, *(callee.GetBody()), names);
  for (auto name : names) {
    auto pairOld = std::pair<GStrIdx, GStrIdx>(callee.GetNameStrIdx(), name);
    auto pairNew = std::pair<GStrIdx, GStrIdx>(caller.GetNameStrIdx(), name);
    if (realCaller.find(pairOld) != realCaller.end()) {
      (void)realCaller.insert(std::pair<std::pair<GStrIdx, GStrIdx>, GStrIdx>(pairNew, realCaller[pairOld]));
    } else {
      (void)realCaller.insert(std::pair<std::pair<GStrIdx, GStrIdx>,
                              GStrIdx>(pairNew, callee.GetBaseClassNameStrIdx()));
    }
  }
}

void MInline::ConvertPStaticToFStatic(MIRFunction &func) const {
  bool hasPStatic = false;
  for (int i = 0; i < func.GetSymbolTabSize(); ++i) {
    MIRSymbol *sym = func.GetSymbolTabItem(i);
    if (sym != nullptr && sym->GetStorageClass() == kScPstatic) {
      hasPStatic = true;
      break;
    }
  }
  if (!hasPStatic) {
    return;  // No pu-static symbols, just return
  }
  std::vector<MIRSymbol*> localSymbols;
  std::vector<uint32> oldStIdx2New(func.GetSymbolTabSize(), 0);
  int pstaticNum = 0;
  for (int i = 0; i < func.GetSymbolTabSize(); ++i) {
    MIRSymbol *sym = func.GetSymbolTabItem(i);
    if (sym == nullptr) {
      continue;
    }
    StIdx oldStIdx = sym->GetStIdx();
    if (sym->GetStorageClass() == kScPstatic) {
      ++pstaticNum;
      // convert pu-static to file-static
      // pstatic symbol name mangling example: "foo_bar" --> "__pstatic__125__foo_bar"
      const auto &symNameOrig = sym->GetName();
      std::string symNameMangling = "__pstatic__" + std::to_string(func.GetPuidx()) + kVerticalLineStr + symNameOrig;
      GStrIdx strIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(symNameMangling);
      MIRSymbol *newSym = GlobalTables::GetGsymTable().CreateSymbol(kScopeGlobal);
      newSym->SetNameStrIdx(strIdx);
      newSym->SetStorageClass(kScFstatic);
      newSym->SetTyIdx(sym->GetTyIdx());
      newSym->SetSKind(sym->GetSKind());
      newSym->SetAttrs(sym->GetAttrs());
      newSym->SetValue(sym->GetValue());
      // avoid pstatic vars to be replaced by const
      newSym->SetHasPotentialAssignment();
      bool success = GlobalTables::GetGsymTable().AddToStringSymbolMap(*newSym);
      CHECK_FATAL(success, "Found repeated global symbols!");
      oldStIdx2New[i] = newSym->GetStIdx().FullIdx();
      // If a pstatic symbol `foo` is initialized by address of another pstatic symbol `bar`, we need update the stIdx
      // of foo's initial value. Example code:
      //   static int bar = 42;
      //   static int *foo = &bar;
      if ((newSym->GetSKind() == kStConst || newSym->GetSKind() == kStVar) && newSym->GetKonst() != nullptr &&
        newSym->GetKonst()->GetKind() == kConstAddrof) {
        auto *addrofConst = static_cast<MIRAddrofConst*>(newSym->GetKonst());
        StIdx valueStIdx = addrofConst->GetSymbolIndex();
        if (!valueStIdx.IsGlobal()) {
          MIRSymbol *valueSym = func.GetSymbolTabItem(valueStIdx.Idx());
          if (valueSym->GetStorageClass() == kScPstatic) {
            valueStIdx.SetFullIdx(oldStIdx2New[valueStIdx.Idx()]);
            addrofConst->SetSymbolIndex(valueStIdx);
          }
        }
      }
    } else {
      StIdx newStIdx(oldStIdx);
      newStIdx.SetIdx(oldStIdx.Idx() - pstaticNum);
      oldStIdx2New[i] = newStIdx.FullIdx();
      sym->SetStIdx(newStIdx);
      localSymbols.push_back(sym);
    }
  }
  func.GetSymTab()->Clear();
  func.GetSymTab()->PushNullSymbol();
  for (MIRSymbol *sym : localSymbols) {
    func.GetSymTab()->AddStOutside(sym);
  }
  // The stIdxOff will be ignored, 0 is just a placeholder
  ReplaceSymbols(func.GetBody(), 0, &oldStIdx2New);
}

// Inline CALLEE into CALLER.
bool MInline::PerformInline(MIRFunction &caller, BlockNode &enclosingBlk, CallNode &callStmt, MIRFunction &callee) {
  if (callee.IsEmpty()) {
    enclosingBlk.RemoveStmt(&callStmt);
    return false;
  }
  // Record stmt right before and after the callStmt.
  StmtNode *stmtBeforeCall = callStmt.GetPrev();
  StmtNode *stmtAfterCall = callStmt.GetNext();
  // Step 0: See if CALLEE have been inlined to CALLER once.
  uint32 inlinedTimes = 0;
  if (inlineTimesMap.find(&callee) != inlineTimesMap.end()) {
    inlinedTimes = inlineTimesMap[&callee];
  } else {
    inlinedTimes = 0;
  }
  // If the callee has local static variables, We convert local pu-static symbols to global file-static symbol to avoid
  // multiple definition for these static symbols
  ConvertPStaticToFStatic(callee);
  // Step 1: Clone CALLEE's body.
  auto getBody = [callee, this] (BlockNode* funcBody) {
    if (callee.IsFromMpltInline()) {
      return funcBody->CloneTree(module.GetCurFuncCodeMPAllocator());
    }
    return funcBody->CloneTreeWithSrcPosition(module);
  };

  BlockNode *newBody = nullptr;
  if (recursiveFuncToInlineLevel.find(&callee) != recursiveFuncToInlineLevel.end() &&
      recursiveFuncToInlineLevel[&callee] >= maxInlineLevel) {
    return false;
  }
  if (&caller == &callee) {
    if (dumpDetail) {
      LogInfo::MapleLogger() << "[INLINE Recursive]: " << caller.GetName() << '\n';
      constexpr int32 dumpIndent = 4;
      callStmt.Dump(dumpIndent);
    }
    if (currFuncBody == nullptr) {
      // For Inline recursive, we save the original function body before first inline.
      currFuncBody = getBody(callee.GetBody());
      // update inlining levels
      if (recursiveFuncToInlineLevel.find(&callee) != recursiveFuncToInlineLevel.end()) {
        recursiveFuncToInlineLevel[&callee]++;
      } else {
        recursiveFuncToInlineLevel[&callee] = 1;
      }
    }
    newBody = getBody(currFuncBody);
  } else {
    newBody = getBody(callee.GetBody());
  }
  // Step 2: Rename symbols, labels, pregs
  uint32 stIdxOff = RenameSymbols(caller, callee, inlinedTimes);
  uint32 labIdxOff = RenameLabels(caller, callee, inlinedTimes);
  std::unordered_map<PregIdx, PregIdx> pregOld2New;
  uint32 regIdxOff = RenamePregs(caller, callee, pregOld2New);
  // Step 3: Replace symbols, labels, pregs
  CHECK_NULL_FATAL(newBody);
  // Callee has no pu-static symbols now, so we only use stIdxOff to update stIdx, set oldStIdx2New nullptr
  ReplaceSymbols(newBody, stIdxOff, nullptr);
  ReplaceLabels(*newBody, labIdxOff);
  ReplacePregs(newBody, pregOld2New);
  // Step 4: Null check 'this' and assign actuals to formals.
  if (static_cast<uint32>(callStmt.NumOpnds()) != callee.GetFormalCount()) {
    LogInfo::MapleLogger() << "warning: # formal arguments != # actual arguments in the function " <<
        callee.GetName() << ". [formal count] " << callee.GetFormalCount() << ", " <<
        "[argument count] " << callStmt.NumOpnds() << std::endl;
  }
  if (callee.GetFormalCount() > 0 && callee.GetFormal(0)->GetName() == kThisStr) {
    UnaryStmtNode *nullCheck = module.CurFuncCodeMemPool()->New<UnaryStmtNode>(OP_assertnonnull);
    nullCheck->SetOpnd(callStmt.Opnd(0), 0);
    newBody->InsertBefore(newBody->GetFirst(), nullCheck);
  }
  // The number of formals and realArg are not always equal
  size_t realArgNum = std::min(callStmt.NumOpnds(), callee.GetFormalCount());
  for (size_t i = 0; i < realArgNum; ++i) {
    BaseNode *currBaseNode = callStmt.Opnd(i);
    MIRSymbol *formal = callee.GetFormal(i);
    if (formal->IsPreg()) {
      PregIdx idx = callee.GetPregTab()->GetPregIdxFromPregno(formal->GetPreg()->GetPregNo());
      RegassignNode *regAssign = builder.CreateStmtRegassign(
          formal->GetPreg()->GetPrimType(), static_cast<uint32>(idx) + regIdxOff, currBaseNode);
      newBody->InsertBefore(newBody->GetFirst(), regAssign);
    } else {
      MIRSymbol *newFormal = caller.GetSymTab()->GetSymbolFromStIdx(formal->GetStIndex() + stIdxOff);
      PrimType formalPrimType = newFormal->GetType()->GetPrimType();
      PrimType realArgPrimType = currBaseNode->GetPrimType();
      // If realArg's type is different from formal's type, use cvt
      if (formalPrimType != realArgPrimType) {
        bool fpTrunc = (IsPrimitiveFloat(formalPrimType) && IsPrimitiveFloat(realArgPrimType) &&
            GetPrimTypeSize(realArgPrimType) > GetPrimTypeSize(formalPrimType));
        // fpTrunc always needs explicit cvt, for example: cvt f32 f64 (dread f32 xxx)
        if (fpTrunc) {
          currBaseNode = builder.CreateExprTypeCvt(OP_cvt, formalPrimType, realArgPrimType, *currBaseNode);
        } else if (MustBeAddress(formalPrimType) != MustBeAddress(realArgPrimType)) {
          bool intTrunc = (IsPrimitiveInteger(formalPrimType) && IsPrimitiveInteger(realArgPrimType) &&
              GetPrimTypeSize(realArgPrimType) > GetPrimTypeSize(formalPrimType));
          // For dassign, if rhs-expr is a primitive integer type, the assigned variable may be smaller, resulting in a
          // truncation. In this case, explicit cvt is not necessary
          if (!intTrunc) {
            currBaseNode = builder.CreateExprTypeCvt(OP_cvt, formalPrimType, realArgPrimType, *currBaseNode);
          }
        }
      }
      DassignNode *stmt = builder.CreateStmtDassign(*newFormal, 0, currBaseNode);
      newBody->InsertBefore(newBody->GetFirst(), stmt);
    }
  }
  // Step 5: Insert the callee'return jump dest label.
  // For caller: a = foo() ==>
  //             a = foo(), label x.
  LabelIdx retLabIdx;
  // record the created label
  StmtNode *labelStmt = nullptr;
  if (callStmt.GetNext() != nullptr && callStmt.GetNext()->GetOpCode() == OP_label) {
    // if the next stmt is a label, just reuse it
    LabelNode *nextLabel = static_cast<LabelNode*>(callStmt.GetNext());
    retLabIdx = nextLabel->GetLabelIdx();
  } else {
    retLabIdx = CreateReturnLabel(caller, callee, inlinedTimes);
    labelStmt = builder.CreateStmtLabel(retLabIdx);
    newBody->AddStatement(labelStmt);
  }
  // Step 6: Handle return values.
  // Find the rval of call-stmt
  // calcute number of return stmt in CALLEE'body
  int retCount = 0;
  // record the last return stmt in CALLEE'body
  GotoNode *lastGoto = nullptr;
  CallReturnVector currReturnVec(alloc.Adapter());
  if (callStmt.GetOpCode() == OP_callassigned || callStmt.GetOpCode() == OP_virtualcallassigned ||
      callStmt.GetOpCode() == OP_superclasscallassigned || callStmt.GetOpCode() == OP_interfacecallassigned) {
    currReturnVec = callStmt.GetReturnVec();
    if (currReturnVec.size() > 1) {
      CHECK_FATAL(false, "multiple return values are not supported");
    }
  }
  lastGoto = UpdateReturnStmts(caller, *newBody, retLabIdx, currReturnVec, retCount);
  // Step 6.5: remove the successive goto statement and label statement in some circumstances.
  // There is no return stmt in CALLEE'body, if we have create a new label in Step5, remove it.
  if (retCount == 0) {
    if (labelStmt != nullptr) {
      newBody->RemoveStmt(labelStmt);
    }
  } else {
    // There are one or more goto stmt, remove the successive goto stmt and label stmt,
    // if there is only one return stmt, we can remove the label created in Step5, too.
    CHECK_FATAL(lastGoto != nullptr, "there should be at least one goto statement");
    if (labelStmt != nullptr) {
      // if we have created a new label in Step5, then last_goto->next == label_stmt means they are successive.
      if (lastGoto->GetNext() == labelStmt) {
        newBody->RemoveStmt(lastGoto);
        if (retCount == 1) {
          newBody->RemoveStmt(labelStmt);
        }
      }
    } else {
      // if we haven't created a new label in Step5, then new_body->last == last_goto means they are successive.
      if (newBody->GetLast() == lastGoto) {
        newBody->RemoveStmt(lastGoto);
      }
    }
  }
  // Step 7: Replace the call-stmt with new CALLEE'body.
  // begin inlining function
  if (!Options::noComment) {
    MapleString beginCmt(module.CurFuncCodeMemPool());
    if (module.firstInline) {
      (void)beginCmt.append(kInlineBeginComment);
    } else {
      (void)beginCmt.append(kSecondInlineBeginComment);
    }
    beginCmt.append(callee.GetName());
    enclosingBlk.InsertBefore(&callStmt, builder.CreateStmtComment(beginCmt.c_str()));
    // end inlining function
    MapleString endCmt(module.CurFuncCodeMemPool());
    if (module.firstInline) {
      (void)endCmt.append(kInlineEndComment);
    } else {
      (void)endCmt.append(kSecondInlineEndComment);
    }
    endCmt.append(callee.GetName());
    if (enclosingBlk.GetLast() != nullptr && &callStmt != enclosingBlk.GetLast()) {
      CHECK_FATAL(callStmt.GetNext() != nullptr, "null ptr check");
    }
    enclosingBlk.InsertAfter(&callStmt, builder.CreateStmtComment(endCmt.c_str()));
    CHECK_FATAL(callStmt.GetNext() != nullptr, "null ptr check");
  }
  if (newBody->IsEmpty()) {
    enclosingBlk.RemoveStmt(&callStmt);
  } else {
    enclosingBlk.ReplaceStmtWithBlock(callStmt, *newBody);
  }
  RecordRealCaller(caller, callee);

  // Step 8: Update inlined_times.
  inlineTimesMap[&callee] = inlinedTimes + 1;
  // Step 9: After inlining, if there exists nested try-catch block, flatten them all.
  // Step 9.1 Find whether there is a javatry before callStmt.
  bool hasOuterTry = false;
  TryNode *outerTry = nullptr;
  StmtNode *outerEndTry = nullptr;
  for (StmtNode *stmt = stmtBeforeCall; stmt != nullptr; stmt = stmt->GetPrev()) {
    if (stmt->op == OP_endtry) {
      break;
    }
    if (stmt->op == OP_try) {
      hasOuterTry = true;
      outerTry = static_cast<TryNode*>(stmt);
      break;
    }
  }
  if (!hasOuterTry) {
    return true;
  }
  for (StmtNode *stmt = stmtAfterCall; stmt != nullptr; stmt = stmt->GetNext()) {
    if (stmt->op == OP_endtry) {
      outerEndTry = stmt;
      break;
    }
    if (stmt->op == OP_try) {
      ASSERT(false, "Impossible, caller: [%s] callee: [%s]",
          caller.GetName().c_str(), callee.GetName().c_str());
      break;
    }
  }
  ASSERT(outerTry != nullptr, "Impossible, caller: [%s] callee: [%s]",
         caller.GetName().c_str(), callee.GetName().c_str());
  ASSERT(outerEndTry != nullptr, "Impossible, caller: [%s] callee: [%s]",
         caller.GetName().c_str(), callee.GetName().c_str());

  // Step 9.2 We have found outerTry and outerEndTry node, resolve the nested try-catch blocks between them.
  bool hasInnerTry = ResolveNestedTryBlock(*caller.GetBody(), *outerTry, outerEndTry);
  if (hasOuterTry && hasInnerTry) {
    if (dumpDetail) {
      LogInfo::MapleLogger() << "[NESTED_TRY_CATCH]" << callee.GetName() << " to " << caller.GetName() << '\n';
    }
  }
  return true;
}

bool MInline::ResolveNestedTryBlock(BlockNode &body, TryNode &outerTryNode, const StmtNode *outerEndTryNode) const {
  StmtNode *stmt = outerTryNode.GetNext();
  StmtNode *next = nullptr;
  bool changed = false;
  while (stmt != outerEndTryNode) {
    next = stmt->GetNext();
    switch (stmt->op) {
      case OP_try: {
        // Find previous meaningful stmt.
        StmtNode *last = stmt->GetPrev();
        while (last->op == OP_comment || last->op == OP_label) {
          last = last->GetPrev();
        }
        ASSERT(last->op != OP_endtry, "Impossible");
        ASSERT(last->op != OP_try, "Impossible");
        StmtNode *newEnd = module.CurFuncCodeMemPool()->New<StmtNode>(OP_endtry);
        body.InsertAfter(last, newEnd);
        TryNode *tryNode = static_cast<TryNode*>(stmt);
        tryNode->OffsetsInsert(tryNode->GetOffsetsEnd(), outerTryNode.GetOffsetsBegin(), outerTryNode.GetOffsetsEnd());
        changed = true;
        break;
      }
      case OP_endtry: {
        // Find next meaningful stmt.
        StmtNode *first = stmt->GetNext();
        while (first->op == OP_comment || first->op == OP_label) {
          first = first->GetNext();
        }
        ASSERT(first != outerEndTryNode, "Impossible");
        next = first->GetNext();
        if (first->op == OP_try) {
          // In this case, there are no meaningful statements between last endtry and the next try,
          // we just solve the trynode and move the cursor to the statement right after the trynode.
          TryNode *tryNode = static_cast<TryNode*>(first);
          tryNode->OffsetsInsert(
              tryNode->GetOffsetsEnd(), outerTryNode.GetOffsetsBegin(), outerTryNode.GetOffsetsEnd());
          changed = true;
          break;
        } else {
          TryNode *node = outerTryNode.CloneTree(module.GetCurFuncCodeMPAllocator());
          CHECK_FATAL(node != nullptr, "Impossible");
          body.InsertBefore(first, node);
          changed = true;
          break;
        }
      }
      default: {
        break;
      }
    }
    stmt = next;
  }
  return changed;
}

// Here, one insn's cost is 2.
FuncCostResultType MInline::GetFuncCost(const MIRFunction &func, const BaseNode &baseNode, uint32 &cost,
                                        uint32 threshold) const {
  if (cost > threshold) {
    return kFuncBodyTooBig;
  }
  Opcode op = baseNode.GetOpCode();
  switch (op) {
    case OP_block: {
      const BlockNode &blk = static_cast<const BlockNode&>(baseNode);
      for (auto &stmt : blk.GetStmtNodes()) {
        FuncCostResultType funcCostResultType = GetFuncCost(func, stmt, cost, threshold);
        if (funcCostResultType != kSmallFuncBody) {
          return funcCostResultType;
        }
      }
      break;
    }
    case OP_switch: {
      const SwitchNode &switchNode = static_cast<const SwitchNode&>(baseNode);
      cost += static_cast<uint32>(switchNode.GetSwitchTable().size() + 1);
      break;
    }
    case OP_call:
    case OP_callassigned: {
      PUIdx puIdx = static_cast<const CallNode&>(baseNode).GetPUIdx();
      MIRFunction *callee = GlobalTables::GetFunctionTable().GetFunctionFromPuidx(puIdx);
      if (callee->GetName().find("setjmp") != std::string::npos) {
        cost += smallFuncThreshold;
        break;
      }
    }
    [[clang::fallthrough]];
    case OP_customcallassigned:
    case OP_polymorphiccallassigned:
    case OP_customcall:
    case OP_polymorphiccall:
    case OP_intrinsiccall:
    case OP_intrinsiccallwithtype:
    case OP_xintrinsiccall:
    case OP_intrinsiccallassigned:
    case OP_intrinsiccallwithtypeassigned:
    case OP_xintrinsiccallassigned:
    case OP_virtualcall:
    case OP_superclasscall:
    case OP_interfacecall:
    case OP_virtualcallassigned:
    case OP_superclasscallassigned:
    case OP_interfacecallassigned:
    case OP_throw: {
      cost += kPentupleInsn;
      break;
    }
    case OP_intrinsicop:
    case OP_intrinsicopwithtype: {
      const IntrinsicopNode &node = static_cast<const IntrinsicopNode&>(baseNode);
      switch(node.GetIntrinsic()) {
        case INTRN_JAVA_CONST_CLASS:
        case INTRN_JAVA_ARRAY_LENGTH:
          cost += kOneInsn;
          break;
        case INTRN_JAVA_MERGE:
          cost += kHalfInsn;
          break;
        case INTRN_JAVA_INSTANCE_OF:
          cost += kPentupleInsn;
          break;
        case INTRN_MPL_READ_OVTABLE_ENTRY:
          cost += kDoubleInsn;
          break;
        case INTRN_C_ctz32:
        case INTRN_C_clz32:
        case INTRN_C_clz64:
        case INTRN_C_constant_p:
          cost += kOneInsn;
          break;
        default:
          // Other intrinsics generate a call
          cost += kPentupleInsn;
          break;
      }
      break;
    }
    case OP_jstry:
    case OP_try:
    case OP_jscatch:
    case OP_catch:
    case OP_finally:
    case OP_cleanuptry:
    case OP_endtry:
    case OP_syncenter:
    case OP_syncexit: {
      return kNotAllowedNode;
    }
    case OP_membaracquire:
    case OP_membarrelease:
    case OP_membarstoreload:
    case OP_membarstorestore: {
      cost += kOneInsn;
      break;
    }
    case OP_comment:
    case OP_return:
    case OP_label:
      break;
    case OP_dread: {
      const DreadNode &dread = static_cast<const DreadNode&>(baseNode);
      bool isLocal = dread.GetStIdx().Islocal();
      if (!isLocal) {
        cost += kDoubleInsn;
      }
      break;
    }
    case OP_dassign: {
      const DassignNode &dassign = static_cast<const DassignNode&>(baseNode);
      bool isLocal = dassign.GetStIdx().Islocal();
      if (!isLocal) {
        cost += kDoubleInsn;
      }
      break;
    }
    case OP_cvt: {
      cost += kHalfInsn;
      break;
    }
    case OP_alloca: {
      cost += smallFuncThreshold;
      break;
    }
    default: {
      cost += kOneInsn;
      break;
    }
  }
  for (size_t i = 0; i < baseNode.NumOpnds(); ++i) {
    FuncCostResultType funcCostResultType = GetFuncCost(func, *(baseNode.Opnd(i)), cost, threshold);
    if (funcCostResultType != kSmallFuncBody) {
      return funcCostResultType;
    }
  }
  return kSmallFuncBody;
}

static void MarkParent(const CGNode &node) {
  for (auto it = node.CallerBegin(); it != node.CallerEnd(); ++it) {
    CGNode *parent = *it;
    parent->SetMustNotBeInlined();
  }
}

bool MInline::IsHotCallSite(const MIRFunction &caller, const MIRFunction &callee, const CallNode &callStmt) const {
  if (dumpDetail) {
    LogInfo::MapleLogger() << "[CHECK_HOT] " << callee.GetName() << " to " << caller.GetName() <<
        " op " << callStmt.GetOpCode() << '\n';
  }
  return module.GetProfile().CheckFuncHot(caller.GetName());
}

bool MInline::FuncInlinable(const MIRFunction &func) const {
  std::string name = func.GetName();
  if (StringUtils::StartsWith(name, kReflectionClassStr) ||
      StringUtils::StartsWith(name, kJavaLangClassesStr) ||
      StringUtils::StartsWith(name, kJavaLangReferenceStr)) {
    return false;
  }
  if (func.GetAttr(FUNCATTR_abstract) || func.GetAttr(FUNCATTR_const) || func.GetAttr(FUNCATTR_declared_synchronized) ||
      func.GetAttr(FUNCATTR_synchronized) || func.GetAttr(FUNCATTR_weak) || func.GetAttr(FUNCATTR_varargs) ||
      ((func.GetAttr(FUNCATTR_critical_native) || func.GetAttr(FUNCATTR_fast_native) ||
        func.GetAttr(FUNCATTR_native)) &&
       (func.GetBody() == nullptr || func.GetBody()->GetFirst() == nullptr))) {
    return false;
  }
  const BlockNode *body = func.GetBody();
  for (auto &stmt : body->GetStmtNodes()) {
    if (stmt.GetOpCode() == OP_iassign) {
      const IassignNode &assign = static_cast<const IassignNode&>(stmt);
      MIRPtrType *ptrType = static_cast<MIRPtrType*>(GlobalTables::GetTypeTable().GetTypeFromTyIdx(assign.GetTyIdx()));
      TyIdxFieldAttrPair fieldPair = ptrType->GetPointedTyIdxFldAttrPairWithFieldID(assign.GetFieldID());
      if (fieldPair.second.GetAttr(FLDATTR_final)) {
        return false;
      }
      if (assign.Opnd(1)->GetPrimType() == PTY_ref) {
        return false;
      }
    }
  }
  return true;
}

bool MInline::IsSafeToInline(const MIRFunction *callee, const CallNode &callStmt) const {
  Opcode op = callStmt.GetOpCode();
  if (op == OP_callassigned || op == OP_call) {
    return true;
  }
  if (IsFinalMethod(callee)) {
    return true;
  }
  return false;
}

void MInline::InlineCalls(const CGNode &node) {
  MIRFunction *func = node.GetMIRFunction();
  if (func == nullptr || func->GetBody() == nullptr || func->IsFromMpltInline()) {
    return;
  }
  // The caller is big enough, we don't inline any callees more
  if (GetNumStmtsOfFunc(*func) > kBigFuncNumStmts) {
    return;
  }
  bool changed = false;
  int currInlineDepth = 0;
  do {
    changed = false;
    currFuncBody = nullptr;
    InlineCallsBlock(*func, *(func->GetBody()), *(func->GetBody()), changed);
    ++currInlineDepth;
  } while (changed && currInlineDepth < Options::inlineDepth);
}

void MInline::InlineCallsBlock(MIRFunction &func, BlockNode &enclosingBlk, BaseNode &baseNode, bool &changed) {
  if (baseNode.GetOpCode() == OP_block) {
    BlockNode &blk = static_cast<BlockNode&>(baseNode);
    for (auto &stmt : blk.GetStmtNodes()) {
      InlineCallsBlock(func, blk, stmt, changed);
    }
  } else if (baseNode.GetOpCode() == OP_callassigned || baseNode.GetOpCode() == OP_call ||
             baseNode.GetOpCode() == OP_virtualcallassigned || baseNode.GetOpCode() == OP_superclasscallassigned ||
             baseNode.GetOpCode() == OP_interfacecallassigned) {
    InlineCallsBlockInternal(func, enclosingBlk, baseNode, changed);
  } else if (baseNode.GetOpCode() == OP_doloop) {
    BlockNode *blk = static_cast<DoloopNode&>(baseNode).GetDoBody();
    InlineCallsBlock(func, enclosingBlk, *blk, changed);
  } else if (baseNode.GetOpCode() == OP_foreachelem) {
    BlockNode *blk = static_cast<ForeachelemNode&>(baseNode).GetLoopBody();
    InlineCallsBlock(func, enclosingBlk, *blk, changed);
  } else if (baseNode.GetOpCode() == OP_dowhile || baseNode.GetOpCode() == OP_while) {
    BlockNode *blk = static_cast<WhileStmtNode&>(baseNode).GetBody();
    InlineCallsBlock(func, enclosingBlk, *blk, changed);
  } else if (baseNode.GetOpCode() == OP_if) {
    BlockNode *blk = static_cast<IfStmtNode&>(baseNode).GetThenPart();
    InlineCallsBlock(func, enclosingBlk, *blk, changed);
    blk = static_cast<IfStmtNode&>(baseNode).GetElsePart();
    if (blk != nullptr) {
      InlineCallsBlock(func, enclosingBlk, *blk, changed);
    }
  }
}

static inline bool IsExternInlineFunc(const MIRFunction& func) {
    return func.GetAttr(FUNCATTR_extern) && func.GetAttr(FUNCATTR_inline);
}

InlineResult MInline::AnalyzeCallsite(const MIRFunction &caller, MIRFunction &callee, const CallNode &callStmt) {
  GStrIdx callerStrIdx = caller.GetNameStrIdx();
  GStrIdx calleeStrIdx = callee.GetNameStrIdx();
  // For noInlineList function.
  if (noInlineListCallee.find(calleeStrIdx) != noInlineListCallee.end()) {
    auto callerList = noInlineListCallee[calleeStrIdx];
    if (callerList->empty()) {
      return InlineResult(false, "LIST_NOINLINE_FUNC");
    }
    if (callerList->find(callerStrIdx) != callerList->end()) {
      return InlineResult(false, "LIST_NOINLINE_CALLSITE");
    }
  }
  // For extern inline function, we check nothing
  if (IsExternInlineFunc(callee)) {
    return InlineResult(true, "EXTERN_INLINE");
  }
  // For hardCoded function, we check nothing.
  if (hardCodedCallee.find(calleeStrIdx) != hardCodedCallee.end()) {
    return InlineResult(true, "HARD_INLINE");
  }
  if (excludedCaller.find(callerStrIdx) != excludedCaller.end()) {
    return InlineResult(false, "EXCLUDED_CALLER");
  }
  if (excludedCallee.find(calleeStrIdx) != excludedCallee.end()) {
    return InlineResult(false, "EXCLUDED_CALLEE");
  }
  if (StringUtils::StartsWith(callee.GetName(), "MCC_")) {
    return InlineResult(false, "INTRINSIC");
  }
  auto itCaller = rcWhiteList.find(callerStrIdx);
  auto itCallee = rcWhiteList.find(calleeStrIdx);
  if (itCaller != rcWhiteList.end() && itCallee == rcWhiteList.end()) {
    return InlineResult(false, "RC_UNSAFE");
  }
  if (callee.GetBody() == nullptr) {
    return InlineResult(false, "EMPTY_CALLEE");
  }
  if (!FuncInlinable(callee)) {
    return InlineResult(false, "ATTR");
  }
  // Incompatible type conversion from arguments to formals
  size_t realArgNum = std::min(callStmt.NumOpnds(), callee.GetFormalCount());
  for (size_t i = 0; i < realArgNum; ++i) {
    PrimType formalPrimType = callee.GetFormal(i)->GetType()->GetPrimType();
    PrimType realArgPrimType = callStmt.Opnd(i)->GetPrimType();
    if (formalPrimType == PTY_agg ^ realArgPrimType == PTY_agg) {
      return InlineResult(false, "INCOMPATIBLE_TYPE_CVT_FORM_ARG_TO_FORMAL");
    }
  }
  if (!callee.GetLabelTab()->GetAddrTakenLabels().empty()) {
    return InlineResult(false, "ADDR_TAKEN_LABELS");
  }
  // For inlineList function.
  if (inlineListCallee.find(calleeStrIdx) != inlineListCallee.end()) {
    auto callerList = inlineListCallee[calleeStrIdx];
    if (callerList->empty()) {
      return InlineResult(true, "LIST_INLINE_FUNC");
    }
    if (callerList->find(calleeStrIdx) != callerList->end()) {
      return InlineResult(true, "LIST_INLINE_CALLSITE");
    }
  }
  CGNode *node = cg->GetCGNode(&callee);
  if (node == nullptr) {
    return InlineResult(false, "NODE_NULL");
  }
  if (node->IsMustNotBeInlined()) {
    return InlineResult(false, "VMStack");
  }
  return AnalyzeCallee(caller, callee, callStmt);
}

static InlineResult GetInlineResult(uint32 threshold, uint32 thresholdType, uint32 cost) {
  if (cost == UINT32_MAX) {
    return InlineResult(false, "NOT_ALLOWED_NODE");
  }

  if (cost <= threshold) {
    switch (thresholdType) {
      case kSmallFuncThreshold:
        return InlineResult(true, "AUTO_INLINE");
      case kHotFuncThreshold:
        return InlineResult(true, "AUTO_INLINE_HOT");
      case kRecursiveFuncThreshold:
        return InlineResult(true, "AUTO_INLINE_RECURSICE_FUNCTION");
      case kHotAndRecursiveFuncThreshold:
        return InlineResult(true, "AUTO_INLINE_HOT_RECURSICE_FUNCTION");
      default:
        break;
    }
  }
  switch (thresholdType) {
    case kSmallFuncThreshold:
      return InlineResult(false, "TOO_BIG " + std::to_string(cost));
    case kHotFuncThreshold:
      return InlineResult(false, "HOT_METHOD_TOO_BIG " + std::to_string(cost));
    case kRecursiveFuncThreshold:
      return InlineResult(false, "RECURSIVE_FUNCTION_TOO_BIG " + std::to_string(cost));
    case kHotAndRecursiveFuncThreshold:
      return InlineResult(false, "HOT_RECURSIVE_FUNCTION_TOO_BIG " + std::to_string(cost));
    default:
      break;
  }
  return InlineResult(false, "IMPOSSIBLE SITUATION!!!");
}

InlineResult MInline::AnalyzeCallee(const MIRFunction &caller, MIRFunction &callee, const CallNode &callStmt) {
  if (!IsSafeToInline(&callee, callStmt)) {
    return InlineResult(false, "UNSAFE_TO_INLINE");
  }
  uint32 thresholdType = kSmallFuncThreshold;
  // Update threshold if this callsite is hot, or dealing with recursive function
  uint32 threshold = smallFuncThreshold;
  if (inlineWithProfile && IsHotCallSite(caller, callee, callStmt)) {
    threshold = hotFuncThreshold;
    thresholdType = kHotFuncThreshold;
  }
  if (&caller == &callee) {
    threshold = (threshold > recursiveFuncThreshold) ? threshold : recursiveFuncThreshold;
    if (thresholdType != kHotFuncThreshold) {
      thresholdType = kRecursiveFuncThreshold;
    } else {
      thresholdType = kHotAndRecursiveFuncThreshold;
    }
  }
  // More tolerant of functions with inline attr
  if (callee.GetAttr(FUNCATTR_inline)) {
    threshold <<= 3;
  }
  // We don't always inline called_once callee to avoid super big caller
  if (module.GetSrcLang() == kSrcLangC &&
      callee.GetAttr(FUNCATTR_called_once) &&
      callee.GetAttr(FUNCATTR_static)) {
    threshold <<= 5;
  }
  uint32 cost = 0;
  BlockNode *calleeBody = callee.GetBody();
  if (&caller == &callee && currFuncBody != nullptr) {
    // This is self recursive inline
    calleeBody = currFuncBody;
  }
  if (funcToCostMap.find(&callee) != funcToCostMap.end()) {
    cost = funcToCostMap[&callee];
  } else {
    FuncCostResultType checkResult = GetFuncCost(callee, *calleeBody, cost, threshold);
    if (checkResult == kNotAllowedNode) {
      funcToCostMap[&callee] = UINT32_MAX;
      return InlineResult(false, "NOT_ALLOWED_NODE");
    } else {
      funcToCostMap[&callee] = cost;
    }
  }
  return GetInlineResult(threshold, thresholdType, cost);
}

void MInline::InlineCallsBlockInternal(MIRFunction &func, BlockNode &enclosingBlk, BaseNode &baseNode, bool &changed) {
  CallNode &callStmt = static_cast<CallNode&>(baseNode);
  MIRFunction *callee = GlobalTables::GetFunctionTable().GetFunctionFromPuidx(callStmt.GetPUIdx());
  InlineResult result = AnalyzeCallsite(func, *callee, callStmt);
  if (result.canInline) {
    module.SetCurFunction(&func);
    if (dumpDetail && dumpFunc == func.GetName()) {
      LogInfo::MapleLogger() << "[Dump before inline ] " << func.GetName() << '\n';
      func.Dump(false);
    }
    bool inlined = PerformInline(func, enclosingBlk, callStmt, *callee);
    // A inlined callee is never regarded as called_once
    callee->UnSetAttr(FUNCATTR_called_once);
    if (dumpDetail && dumpFunc == func.GetName()) {
      LogInfo::MapleLogger() << "[Dump after inline ] " << func.GetName() << '\n';
      func.Dump(false);
    }
    changed = (inlined ? true : changed);
    if (dumpDetail && inlined) {
      LogInfo::MapleLogger() << "[" << result.reason << "] " << callee->GetName() << " to " << func.GetName() << '\n';
    }
  } else {
    if (dumpDetail) {
      LogInfo::MapleLogger() << "[INLINE_FAILED] " << "[" << result.reason << "] " << callee->GetName() <<
          " to " << func.GetName() << '\n';
    }
  }
}

void MInline::ComputeTotalSize() {
  for (auto it = cg->Begin(); it != cg->End(); ++it) {
    CGNode *caller = it->second;
    totalSize += caller->GetNodeCount();
  }
}

void MInline::CollectMustInlineFuncs() {
  // Add here the function names that must be inlined.
  std::unordered_set<std::string> funcs = {};
  for (auto name : funcs) {
    GStrIdx nameStrIdx = GlobalTables::GetStrTable().GetStrIdxFromName(name);
    (void)hardCodedCallee.insert(nameStrIdx);
  }
}

void MInline::MarkUnInlinableFunction() const {
  const MapleVector<SCCNode*> &topVec = cg->GetSCCTopVec();
  for (MapleVector<SCCNode*>::const_reverse_iterator it = topVec.rbegin(); it != topVec.rend(); ++it) {
    for (CGNode *node : (*it)->GetCGNodes()) {
      std::string name = node->GetMIRFunction()->GetName();
      if (node->IsMustNotBeInlined() ||
          node->GetMIRFunction()->HasAsm() ||
          StringUtils::StartsWith(name, kDalvikSystemStr) ||
          StringUtils::StartsWith(name, kJavaLangThreadStr)) {
        node->SetMustNotBeInlined();
        if (node->HasCaller()) {
          MarkParent(*node);
        }
      }
    }
  }
}

void MInline::Inline() {
  ApplyInlineListInfo(inlineFuncList, inlineListCallee);
  ApplyInlineListInfo(noInlineFuncList, noInlineListCallee);
  CollectMustInlineFuncs();
  ComputeTotalSize();
  MarkUnInlinableFunction();
  const MapleVector<SCCNode*> &topVec = cg->GetSCCTopVec();
  for (MapleVector<SCCNode*>::const_reverse_iterator it = topVec.rbegin(); it != topVec.rend(); ++it) {
    for (CGNode *node : (*it)->GetCGNodes()) {
      // If a function is called only once by a single caller, we set the func called_once. Callee will be set first.
      if (node->NumberOfUses() == 1 && node->NumReferences() == 1 && !node->IsAddrTaken()) {
        node->GetMIRFunction()->SetAttr(FUNCATTR_called_once);
      }
      InlineCalls(*node);
    }
  }
  return;
}

void MInline::CleanupInline() {
  const MapleVector<SCCNode*> &topVec = cg->GetSCCTopVec();
  for (MapleVector<SCCNode*>::const_reverse_iterator it = topVec.rbegin(); it != topVec.rend(); ++it) {
    for (CGNode *node : (*it)->GetCGNodes()) {
      MIRFunction *func = node->GetMIRFunction();
      if (func != nullptr && func->IsFromMpltInline()) {
        // visit all the func which has been inlined, mark the static symbol, string symbol and function symbol as used.
        auto f = inlineTimesMap.find(func);
        if (f != inlineTimesMap.end() && inlineTimesMap[func] > 0) {
          MarkUsedSymbols(func->GetBody());
        }
        func->SetBody(nullptr);
      }
      if (func != nullptr && IsExternInlineFunc(*func)) {
        func->SetBody(nullptr);
        func->ReleaseCodeMemory();
        func->ReleaseMemory();
      }
    }
  }
  // after marking all the used symbols, set the other symbols as unused.
  for (size_t i = 1; i < GlobalTables::GetGsymTable().GetSymbolTableSize(); ++i) {
    MIRSymbol *symbol = GlobalTables::GetGsymTable().GetSymbolFromStidx(i);
    if (symbol != nullptr && symbol->IsTmpUnused()) {
      symbol->SetStorageClass(kScUnused);
      if (dumpDetail) {
        LogInfo::MapleLogger() << "[INLINE_UNUSED_SYMBOL] " << symbol->GetName() << '\n';
      }
    }
  }
  if (dumpDetail) {
    LogInfo::MapleLogger() << "[INLINE_SUMMARY] " << module.GetFileName() << '\n';
    for (auto it = inlineTimesMap.begin(); it != inlineTimesMap.end(); ++it) {
      LogInfo::MapleLogger() << "[INLINE_SUMMARY] " << it->first->GetName() << " => " << it->second << '\n';
    }
    LogInfo::MapleLogger() << "[INLINE_SUMMARY] " << module.GetFileName() << '\n';
  }

  if (dumpDetail) {
    auto &records = module.GetRealCaller();
    for (auto it : records) {
      std::string caller = GlobalTables::GetStrTable().GetStringFromStrIdx(it.first.first);
      std::string callee = GlobalTables::GetStrTable().GetStringFromStrIdx(it.first.second);
      std::string realCaller = GlobalTables::GetStrTable().GetStringFromStrIdx(it.second);
      LogInfo::MapleLogger() << "[REAL_CALLER] caller: " << caller << " callee: " << callee <<
          " realCaller: " << realCaller << '\n';
    }
  }
  return;
}

void MInline::MarkSymbolUsed(const StIdx &symbolIdx) const {
  MIRSymbol *symbol = GlobalTables::GetGsymTable().GetSymbolFromStidx(symbolIdx.Idx());
  symbol->SetIsTmpUnused(false);
  std::string syName = symbol->GetName();
  // when _PTR_C_STR_XXXX is used, mark _C_STR_XXXX as used too.
  if (StringUtils::StartsWith(syName, namemangler::kPtrPrefixStr)) {
    GStrIdx gStrIdx = GlobalTables::GetStrTable().GetStrIdxFromName(syName.substr(strlen(namemangler::kPtrPrefixStr)));
    MIRSymbol *anotherSymbol = GlobalTables::GetGsymTable().GetSymbolFromStrIdx(gStrIdx);
    anotherSymbol->SetIsTmpUnused(false);
  }
}

void MInline::MarkUsedSymbols(const BaseNode *baseNode) const {
  if (baseNode == nullptr) {
    return;
  }
  Opcode op = baseNode->GetOpCode();
  switch (op) {
    case OP_block: {
      const BlockNode *blk = static_cast<const BlockNode*>(baseNode);
      for (auto &stmt : blk->GetStmtNodes()) {
        MarkUsedSymbols(&stmt);
      }
      break;
    }
    case OP_dassign: {
      const DassignNode *dassignNode = static_cast<const DassignNode*>(baseNode);
      MarkSymbolUsed(dassignNode->GetStIdx());
      break;
    }
    case OP_addrof:
    case OP_dread: {
      const AddrofNode *dreadNode = static_cast<const AddrofNode*>(baseNode);
      MarkSymbolUsed(dreadNode->GetStIdx());
      break;
    }
    case OP_callassigned:
    case OP_virtualcallassigned:
    case OP_superclasscallassigned:
    case OP_interfacecallassigned:
    case OP_customcallassigned:
    case OP_polymorphiccallassigned:
    case OP_intrinsiccallassigned:
    case OP_xintrinsiccallassigned:
    case OP_intrinsiccallwithtypeassigned: {
      const CallNode *callStmt = static_cast<const CallNode*>(baseNode);
      MIRFunction *callee = GlobalTables::GetFunctionTable().GetFunctionFromPuidx(callStmt->GetPUIdx());
      MIRSymbol *symbol = callee->GetFuncSymbol();
      symbol->SetIsTmpUnused(false);
      break;
    }
    default: {
      break;
    }
  }
  for (size_t i = 0; i < baseNode->NumOpnds(); ++i) {
    MarkUsedSymbols(baseNode->Opnd(i));
  }
}

// Unified interface to run inline module phase.
bool M2MInline::PhaseRun(maple::MIRModule &m) {
  MemPool *memPool = GetPhaseMemPool();
  CallGraph *cg = GET_ANALYSIS(M2MCallGraph, m);
  CHECK_FATAL(cg != nullptr, "Expecting a valid CallGraph, found nullptr");
  // Reset inlining threshold for other srcLang, especially for srcLangJava. Because those methods related to
  // reflection in Java cannot be inlined safely.
  if (m.GetSrcLang() != kSrcLangC) {
    Options::inlineSmallFunctionThreshold = 15;
    Options::inlineHotFunctionThreshold = 30;
  }
  MInline mInline(m, memPool, cg);
  mInline.Inline();
  mInline.CleanupInline();
  if (m.firstInline) {
    m.firstInline = false;
  }
  return true;
}

void M2MInline::GetAnalysisDependence(AnalysisDep &aDep) const {
  aDep.AddRequired<M2MCallGraph>();
  aDep.PreservedAllExcept<M2MCallGraph>();
}
}  // namespace maple
