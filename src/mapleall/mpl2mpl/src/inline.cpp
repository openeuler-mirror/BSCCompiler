/*
 * Copyright (c) [2019-2022] Huawei Technologies Co.,Ltd.All rights reserved.
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

#include <fstream>
#include <iostream>

#include "global_tables.h"
#include "inline_analyzer.h"
#include "inline_summary.h"
#include "mpl_logging.h"

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
static constexpr uint32 kRelaxThresholdForInlineHint = 3;
static constexpr uint32 kRelaxThresholdForCalledOnce = 5;
static constexpr uint32 kRestrictThresholdForColdCallsite = 2;
static constexpr uint32 kInlineSmallFunctionThresholdForJava = 15;
static constexpr uint32 kInlineHotFunctionThresholdForJava = 30;
static constexpr uint32 kThresholdForOs = 10;

static uint32 GetNumStmtsOfFunc(const MIRFunction &func) {
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
  if (Options::optForSize) {
    smallFuncThreshold = kThresholdForOs;
  } else {
    smallFuncThreshold = Options::inlineSmallFunctionThreshold;
  }
  hotFuncThreshold = Options::inlineHotFunctionThreshold;
  recursiveFuncThreshold = Options::inlineRecursiveFunctionThreshold;
  inlineWithProfile = Options::inlineWithProfile;
}

void MInline::InitProfile() const {
  // gcov use different profile data, return
  if (Options::profileUse) {
    return;
  }
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
#define DEF_MIR_INTRINSIC(X, NAME, NUM_INSN, INTRN_CLASS, RETURN_TYPE, ...) NAME,
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
      switch (node.GetIntrinsic()) {
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
        case INTRN_C___builtin_expect:
          break;  // builtin_expect has no cost
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

bool MInline::HasAccessStatic(const BaseNode &baseNode) const {
  Opcode op = baseNode.GetOpCode();
  switch (op) {
    case OP_block: {
      const BlockNode &blk = static_cast<const BlockNode&>(baseNode);
      for (auto &stmt : blk.GetStmtNodes()) {
        bool ret = HasAccessStatic(stmt);
        if (ret) {
          return true;
        }
      }
      break;
    }
    case OP_addrof:
    case OP_dread: {
      const DreadNode &dread = static_cast<const DreadNode&>(baseNode);
      bool isLocal = dread.GetStIdx().Islocal();
      if (!isLocal) {
        MIRSymbol *symbol = GlobalTables::GetGsymTable().GetSymbolFromStidx(dread.GetStIdx().Idx());
        return symbol->IsStatic();
      }
      break;
    }
    case OP_dassign: {
      const DassignNode &dassign = static_cast<const DassignNode&>(baseNode);
      bool isLocal = dassign.GetStIdx().Islocal();
      if (!isLocal) {
        MIRSymbol *symbol = GlobalTables::GetGsymTable().GetSymbolFromStidx(dassign.GetStIdx().Idx());
        return symbol->IsStatic();
      }
      break;
    }
    default: {
      break;
    }
  }
  for (size_t i = 0; i < baseNode.NumOpnds(); ++i) {
    bool ret = HasAccessStatic(*(baseNode.Opnd(i)));
    if (ret) {
      return true;
    }
  }
  return false;
}

static void MarkParent(const CGNode &node) {
  for (auto &pair : node.GetCaller()) {
    CGNode *parent = pair.first;
    parent->SetMustNotBeInlined();
  }
}

bool MInline::IsHotCallSite(const MIRFunction &caller, const MIRFunction &callee, const CallNode &callStmt) const {
  if (dumpDetail) {
    LogInfo::MapleLogger() << "[CHECK_HOT] " << callee.GetName() << " to " << caller.GetName() << " op "
                           << callStmt.GetOpCode() << '\n';
  }
  // use gcov profile
  if (Options::profileUse) {
    return caller.GetFuncProfData()->IsHotCallSite(callStmt.GetStmtID());
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

void MInline::InlineCalls(CGNode &node) {
  MIRFunction *func = node.GetMIRFunction();
  if (func == nullptr || func->GetBody() == nullptr || func->IsFromMpltInline()) {
    return;
  }
  // The caller is big enough, we don't inline any callees more
  if (GetNumStmtsOfFunc(*func) > kBigFuncNumStmts) {
    return;
  }
  bool changed = false;
  uint32 currInlineDepth = 0;
  do {
    changed = false;
    // We use bfs to solve all the callsites.
    // For recursive func, we use the original body in one level, even though the real body has changed.
    // However, in order to make the recursive inline stop quickly, we will update the original body in next level.
    node.SetOriginBody(nullptr);
    InlineCallsBlock(*func, *(func->GetBody()), *(func->GetBody()), changed, *(func->GetBody()));
    ++currInlineDepth;
  } while (changed && currInlineDepth < Options::inlineDepth);
}
bool MInline::CalleeReturnValueCheck(StmtNode &stmtNode, CallNode &callStmt) {
  NaryStmtNode &returnNode = static_cast<NaryStmtNode&>(stmtNode);
  if (!kOpcodeInfo.IsCallAssigned(callStmt.GetOpCode()) && returnNode.NumOpnds() == 0) {
    return true;
  }
  if (returnNode.NumOpnds() != 1 || returnNode.NumOpnds() != callStmt.GetCallReturnVector()->size()) {
    return false;
  } else {
    if (returnNode.Opnd(0)->GetOpCode() != OP_dread) {
      return false;
    }
    AddrofNode *retNode = static_cast<AddrofNode*>(returnNode.Opnd(0));
    RegFieldPair &tmp = callStmt.GetCallReturnVector()->at(0).second;
    if (retNode->GetFieldID() != 0 || tmp.IsReg() || tmp.GetFieldID() != 0) {
      return false;
    }
    if (static_cast<AddrofNode*>(retNode)->GetStIdx() == callStmt.GetCallReturnVector()->at(0).first) {
      return true;
    }
  }
  return false;
}
bool MInline::SuitableForTailCallOpt(BaseNode &enclosingBlk, const StmtNode &stmtNode, CallNode &callStmt) {
  // if call stmt have arguments, it may be better to be inlined
  if (!module.IsCModule() || callStmt.GetNumOpnds() != 0) {
    return false;
  }
  if (stmtNode.GetRealNext() != nullptr && stmtNode.GetRealNext()->GetOpCode() == OP_return) {
    return CalleeReturnValueCheck(static_cast<StmtNode&>(*stmtNode.GetRealNext()), callStmt);
  }
  if (enclosingBlk.GetOpCode() == OP_if) {
    IfStmtNode &ifStmt = static_cast<IfStmtNode&>(enclosingBlk);
    if (ifStmt.GetRealNext() != nullptr && ifStmt.GetRealNext()->GetOpCode() == OP_return &&
        stmtNode.GetRealNext() == nullptr) {
      return CalleeReturnValueCheck(static_cast<StmtNode&>(*ifStmt.GetRealNext()), callStmt);
    }
  }
  return false;
}
void MInline::InlineCallsBlock(MIRFunction &func, BlockNode &enclosingBlk, BaseNode &baseNode, bool &changed,
                               BaseNode &prevStmt) {
  if (baseNode.GetOpCode() == OP_block) {
    BlockNode &blk = static_cast<BlockNode&>(baseNode);
    for (auto &stmt : blk.GetStmtNodes()) {
      InlineCallsBlock(func, blk, stmt, changed, prevStmt);
    }
  } else if (baseNode.GetOpCode() == OP_callassigned || baseNode.GetOpCode() == OP_call ||
             baseNode.GetOpCode() == OP_virtualcallassigned || baseNode.GetOpCode() == OP_superclasscallassigned ||
             baseNode.GetOpCode() == OP_interfacecallassigned) {
    CallNode &callStmt = static_cast<CallNode&>(baseNode);
    if (SuitableForTailCallOpt(prevStmt, static_cast<StmtNode&>(baseNode), callStmt)) {
      return;
    }
    InlineCallsBlockInternal(func, enclosingBlk, baseNode, changed);
  } else if (baseNode.GetOpCode() == OP_doloop) {
    BlockNode *blk = static_cast<DoloopNode&>(baseNode).GetDoBody();
    InlineCallsBlock(func, enclosingBlk, *blk, changed, baseNode);
  } else if (baseNode.GetOpCode() == OP_foreachelem) {
    BlockNode *blk = static_cast<ForeachelemNode&>(baseNode).GetLoopBody();
    InlineCallsBlock(func, enclosingBlk, *blk, changed, baseNode);
  } else if (baseNode.GetOpCode() == OP_dowhile || baseNode.GetOpCode() == OP_while) {
    BlockNode *blk = static_cast<WhileStmtNode&>(baseNode).GetBody();
    InlineCallsBlock(func, enclosingBlk, *blk, changed, baseNode);
  } else if (baseNode.GetOpCode() == OP_if) {
    BlockNode *blk = static_cast<IfStmtNode&>(baseNode).GetThenPart();
    InlineCallsBlock(func, enclosingBlk, *blk, changed, baseNode);
    blk = static_cast<IfStmtNode&>(baseNode).GetElsePart();
    if (blk != nullptr) {
      InlineCallsBlock(func, enclosingBlk, *blk, changed, baseNode);
    }
  }
}

static inline bool IsExternInlineFunc(const MIRFunction &func) {
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
  if (callee.GetAttr(FUNCATTR_noinline)) {
    return InlineResult(false, "ATTR_NOINLINE");
  }
  // When callee is unsafe but callStmt is in safe region
  if (MeOption::safeRegionMode && (callStmt.IsInSafeRegion() || caller.IsSafe()) && (callee.IsUnSafe())) {
    return InlineResult(false, "UNSAFE_INLINE");
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
    if ((formalPrimType == PTY_agg) ^ (realArgPrimType == PTY_agg)) {
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
    if (callerList->find(callerStrIdx) != callerList->end()) {
      return InlineResult(true, "LIST_INLINE_CALLSITE");
    }
  }
  CGNode *node = cg->GetCGNode(&callee);
  if (node == nullptr) {
    return InlineResult(false, "NODE_NULL");
  }
  if (&caller == &callee && node->GetRecursiveLevel() >= maxRecursiveLevel) {
    return InlineResult(false, "RECURSIVE_LEVEL_TOO_MUCH");
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
        return InlineResult(true, "AUTO_INLINE_RECURSIVE_FUNCTION");
      case kHotAndRecursiveFuncThreshold:
        return InlineResult(true, "AUTO_INLINE_HOT_RECURSIVE_FUNCTION");
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

void MInline::AdjustInlineThreshold(const MIRFunction &caller, const MIRFunction &callee, const CallNode &callStmt,
    uint32 &threshold, uint32 &thresholdType) {
  // Update threshold if this callsite is hot, or dealing with recursive function
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
    threshold <<= kRelaxThresholdForInlineHint;
  }
  // We don't always inline called_once callee to avoid super big caller
  if (module.GetSrcLang() == kSrcLangC &&
      callee.GetAttr(FUNCATTR_called_once) &&
      callee.GetAttr(FUNCATTR_static)) {
    threshold <<= kRelaxThresholdForCalledOnce;
  }
}

InlineResult MInline::AnalyzeCallee(const MIRFunction &caller, MIRFunction &callee, const CallNode &callStmt) {
  if (!IsSafeToInline(&callee, callStmt)) {
    return InlineResult(false, "UNSAFE_TO_INLINE");
  }
  uint32 thresholdType = kSmallFuncThreshold;
  uint32 threshold = smallFuncThreshold;
  AdjustInlineThreshold(caller, callee, callStmt, threshold, thresholdType);
  uint32 cost = 0;
  BlockNode *calleeBody = callee.GetBody();
  auto *calleeNode = cg->GetCGNode(&callee);
  CHECK_NULL_FATAL(calleeNode);
  if (&caller == &callee && calleeNode->GetOriginBody() != nullptr) {
    // This is self recursive inline
    calleeBody = calleeNode->GetOriginBody();
  }
  if (Options::enableInlineSummary && callee.GetInlineSummary()->IsTrustworthy()) {
    if (caller.GetInlineSummary()->IsColdCallsite(callStmt.GetStmtID())) {
      threshold >>= kRestrictThresholdForColdCallsite;  // less tolerance for cold callsite
    }
    int32 condCost = GetCondInlineCost(caller, callee, callStmt);
    CHECK_FATAL(condCost >= 0, "neg cost haven been supported");
    cost = static_cast<uint32>(condCost);
  } else {
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
  }
  auto ret = GetInlineResult(threshold, thresholdType, cost);
  return ret;
}

void MInline::PostInline(MIRFunction &caller) {
  auto it = funcToCostMap.find(&caller);
  if (it != funcToCostMap.end()) {
    (void)funcToCostMap.erase(it);
  }
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
    auto *transformer = alloc.New<InlineTransformer>(func, *callee, callStmt, dumpDetail, cg);
    bool inlined = transformer->PerformInline(enclosingBlk);
    if (inlined) {
      PostInline(func);
    }
    // A inlined callee is never regarded as called_once
    callee->UnSetAttr(FUNCATTR_called_once);
    if (dumpDetail && dumpFunc == func.GetName()) {
      LogInfo::MapleLogger() << "[Dump after inline ] " << func.GetName() << '\n';
      func.Dump(false);
    }
    changed = (inlined ? true : changed);
    if (inlined && callee->IsFromMpltInline()) {
      LogInfo::MapleLogger() << "[CROSS_MODULE] [" << result.reason << "] " << callee->GetName() << " to "
                             << func.GetName() << '\n';
    }
    if (dumpDetail && inlined) {
      if (callee->IsFromMpltInline()) {
        LogInfo::MapleLogger() << "[CROSS_MODULE] [" << result.reason << "] " << callee->GetName() << " to "
                               << func.GetName() << '\n';
      } else {
        LogInfo::MapleLogger() << "[" << result.reason << "] " << callee->GetName() << " to " << func.GetName() << '\n';
      }
    }
  } else {
    if (dumpDetail) {
      LogInfo::MapleLogger() << "[INLINE_FAILED] "
                             << "[" << result.reason << "] " << callee->GetName() << " to " << func.GetName() << '\n';
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
  const MapleVector<SCCNode<CGNode>*> &topVec = cg->GetSCCTopVec();
  for (auto it = topVec.rbegin(); it != topVec.rend(); ++it) {
    for (CGNode *node : (*it)->GetNodes()) {
      MIRFunction *func = node->GetMIRFunction();
      if (!func->IsEmpty() && func->IsFromMpltInline() &&
          (node->IsMustNotBeInlined() || HasAccessStatic(*func->GetBody()))) {
        node->SetMustNotBeInlined();
        if (func->IsStatic() && node->HasCaller()) {
          MarkParent(*node);
        }
        continue;
      }

      const std::string &name = func->GetName();
      if (node->IsMustNotBeInlined() || StringUtils::StartsWith(name, kDalvikSystemStr) ||
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
  const MapleVector<SCCNode<CGNode>*> &topVec = cg->GetSCCTopVec();
  for (MapleVector<SCCNode<CGNode>*>::const_reverse_iterator it = topVec.rbegin(); it != topVec.rend(); ++it) {
    for (CGNode *node : (*it)->GetNodes()) {
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
  const MapleVector<SCCNode<CGNode>*> &topVec = cg->GetSCCTopVec();
  for (MapleVector<SCCNode<CGNode>*>::const_reverse_iterator it = topVec.rbegin(); it != topVec.rend(); ++it) {
    for (CGNode *node : (*it)->GetNodes()) {
      MIRFunction *func = node->GetMIRFunction();
      if (func != nullptr && func->IsFromMpltInline()) {
        // visit all the func which has been inlined, mark the static symbol, string symbol and function symbol as used.
        if (node->GetInlinedTimes() > 0) {
          MarkFunctionUsed(func, true);
        }
        if (!func->IsStatic()) {
          func->SetBody(nullptr);
          func->ReleaseCodeMemory();
          func->ReleaseMemory();
        }
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
    MIRSymbol *symbol = GlobalTables::GetGsymTable().GetSymbolFromStidx(static_cast<uint32>(i));
    if (symbol != nullptr && symbol->IsTmpUnused()) {
      symbol->SetStorageClass(kScUnused);
      if (dumpDetail) {
        LogInfo::MapleLogger() << "[INLINE_UNUSED_SYMBOL] " << symbol->GetName() << '\n';
      }
    }
  }
  if (dumpDetail) {
    LogInfo::MapleLogger() << "[INLINE_SUMMARY] " << module.GetFileName() << '\n';
    for (auto &it : cg->GetNodesMap()) {
      auto times = it.second->GetInlinedTimes();
      if (times == 0) {
        continue;
      }
      LogInfo::MapleLogger() << "[INLINE_SUMMARY] " << it.first->GetName() << " => " << times << '\n';
    }
    LogInfo::MapleLogger() << "[INLINE_SUMMARY] " << module.GetFileName() << '\n';
  }
  return;
}

void MInline::MarkSymbolUsed(const StIdx &symbolIdx) const {
  MIRSymbol *symbol = GlobalTables::GetGsymTable().GetSymbolFromStidx(symbolIdx.Idx());
  symbol->SetIsTmpUnused(false);
  if (symbol->IsConst()) {
    auto *konst = symbol->GetKonst();
    switch (konst->GetKind()) {
      case kConstAddrof: {
        auto *addrofKonst = static_cast<MIRAddrofConst*>(konst);
        MIRSymbol *sym = GlobalTables::GetGsymTable().GetSymbolFromStidx(addrofKonst->GetSymbolIndex().Idx());
        MarkSymbolUsed(sym->GetStIdx());
        break;
      }
      case kConstAddrofFunc: {
        auto *addrofFuncKonst = static_cast<MIRAddroffuncConst*>(konst);
        auto *func = GlobalTables::GetFunctionTable().GetFunctionFromPuidx(addrofFuncKonst->GetValue());
        MarkFunctionUsed(func);
        break;
      }
      case kConstAggConst: {
        auto &constVec = static_cast<MIRAggConst*>(konst)->GetConstVec();
        for (auto *cst : constVec) {
          if (cst == nullptr) {
            continue;
          }
          if (cst->GetKind() == kConstAddrofFunc) {
            auto *addrofFuncKonst = static_cast<MIRAddroffuncConst*>(cst);
            auto *func = GlobalTables::GetFunctionTable().GetFunctionFromPuidx(addrofFuncKonst->GetValue());
            MarkFunctionUsed(func);
          }
        }
        break;
      }
      default: {
        break;
      }
    }
  }
  std::string syName = symbol->GetName();
  // when _PTR_C_STR_XXXX is used, mark _C_STR_XXXX as used too.
  if (StringUtils::StartsWith(syName, namemangler::kPtrPrefixStr)) {
    GStrIdx gStrIdx = GlobalTables::GetStrTable().GetStrIdxFromName(syName.substr(strlen(namemangler::kPtrPrefixStr)));
    MIRSymbol *anotherSymbol = GlobalTables::GetGsymTable().GetSymbolFromStrIdx(gStrIdx);
    anotherSymbol->SetIsTmpUnused(false);
  }
}

void MInline::MarkFunctionUsed(MIRFunction *func, bool inlined) const {
  if (func == nullptr) {
    return;
  }
  if (func->IsVisited()) {
    return;
  }
  func->SetIsVisited();
  ASSERT_NOT_NULL(func->GetFuncSymbol());
  func->GetFuncSymbol()->SetIsTmpUnused(false);
  // We should visit the body if the function is static or it has been inlined for at least one time.
  if (func->IsStatic() || inlined) {
    MarkUsedSymbols(func->GetBody());
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
    case OP_addroffunc: {
      auto *addroffunc = static_cast<const AddroffuncNode*>(baseNode);
      MIRFunction *func = GlobalTables::GetFunctionTable().GetFunctionFromPuidx(addroffunc->GetPUIdx());
      MarkFunctionUsed(func);
      break;
    }
    case OP_call:
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
      MarkFunctionUsed(callee);
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
  MemPool *memPool = ApplyTempMemPool();
  CallGraph *cg = GET_ANALYSIS(M2MCallGraph, m);
  CHECK_FATAL(cg != nullptr, "Expecting a valid CallGraph, found nullptr");
  // Reset inlining threshold for other srcLang, especially for srcLangJava. Because those methods related to
  // reflection in Java cannot be inlined safely.
  if (m.GetSrcLang() != kSrcLangC) {
    Options::inlineSmallFunctionThreshold = kInlineSmallFunctionThresholdForJava;
    Options::inlineHotFunctionThreshold = kInlineHotFunctionThresholdForJava;
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
