/*
 * Copyright (c) [2022] Huawei Technologies Co.,Ltd.All rights reserved.
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
#include "ipa_clone.h"
#include "clone.h"
#include "mir_symbol.h"
#include "func_desc.h"
#include "inline.h"

// For some funcs, when we can ignore their return-values, we clone a new func of
// them without return-values. We configure a list to save these funcs and clone
// at the very beginning so that clones can also enjoy the optimizations after.
// This mainly contains the clone of funcbody(include labels, symbols, arguments,
// etc.) and the update of the new func infomation.
namespace maple {
void IpaClone::InitParams() {
  // the option for the clone parameter
  if (Options::optForSize) {
    numOfCloneVersions = 2;
    numOfImpExprLowBound = 2;
    numOfImpExprHighBound = 5;
    numOfCallSiteLowBound = 2;
    numOfCallSiteUpBound = 10;
    numOfConstpropValue = 2;
  } else {
    numOfCloneVersions = Options::numOfCloneVersions;
    numOfImpExprLowBound = Options::numOfImpExprLowBound;
    numOfImpExprHighBound = Options::numOfImpExprHighBound;
    numOfCallSiteLowBound = Options::numOfCallSiteLowBound;
    numOfCallSiteUpBound = Options::numOfCallSiteUpBound;
    numOfConstpropValue = Options::numOfConstpropValue;
  }
}

bool IpaClone::IsBrCondOrIf(Opcode op) const {
  return op == OP_brfalse || op == OP_brtrue || op == OP_if;
}

MIRSymbol *IpaClone::IpaCloneLocalSymbol(const MIRSymbol &oldSym, const MIRFunction &newFunc) {
  MemPool *newMP = newFunc.GetDataMemPool();
  MIRSymbol *newSym = newMP->New<MIRSymbol>(oldSym);
  if (oldSym.GetSKind() == kStConst) {
    newSym->SetKonst(oldSym.GetKonst()->Clone(*newMP));
  } else if (oldSym.GetSKind() == kStPreg) {
    newSym->SetPreg(newMP->New<MIRPreg>(*oldSym.GetPreg()));
  } else if (oldSym.GetSKind() == kStFunc) {
    CHECK_FATAL(false, "%s has unexpected local func symbol", oldSym.GetName().c_str());
  }
  return newSym;
}

void IpaClone::IpaCloneSymbols(MIRFunction &newFunc, const MIRFunction &oldFunc) {
  size_t symTabSize = oldFunc.GetSymbolTabSize();
  for (size_t i = oldFunc.GetFormalCount() + 1; i < symTabSize; ++i) {
    MIRSymbol *sym = oldFunc.GetSymbolTabItem(static_cast<uint32>(i));
    if (sym == nullptr) {
      continue;
    }
    MIRSymbol *newSym = IpaCloneLocalSymbol(*sym, newFunc);
    if (!newFunc.GetSymTab()->AddStOutside(newSym)) {
      CHECK_FATAL(false, "%s already existed in func %s", sym->GetName().c_str(), newFunc.GetName().c_str());
    }
  }
}

void IpaClone::IpaCloneLabels(MIRFunction &newFunc, const MIRFunction &oldFunc) {
  size_t labelTabSize = oldFunc.GetLabelTab()->GetLabelTableSize();
  for (size_t i = 1; i < labelTabSize; ++i) {
    GStrIdx strIdx = oldFunc.GetLabelTab()->GetSymbolFromStIdx(static_cast<uint32>(i));
    (void)newFunc.GetLabelTab()->AddLabel(strIdx);
  }
}

void IpaClone::IpaClonePregTable(MIRFunction &newFunc, const MIRFunction &oldFunc) {
  newFunc.AllocPregTab();
  size_t pregTableSize = oldFunc.GetPregTab()->Size();
  MIRPregTable *newPregTable = newFunc.GetPregTab();
  for (size_t i = 0; i < pregTableSize; ++i) {
    MIRPreg *temp = const_cast<MIRPreg*>(oldFunc.GetPregTab()->GetPregTableItem(static_cast<uint32>(i)));
    if (temp != nullptr) {
      PregIdx id = newPregTable->CreatePreg(temp->GetPrimType(), temp->GetMIRType());
      MIRPreg *newPreg = newPregTable->PregFromPregIdx(id);
      if (newPreg == nullptr || newPreg->GetPregNo() != temp->GetPregNo()) {
        ASSERT(false, "The cloned pregNo isn't consistent");
      }
    }
  }
}

// IpaClone a function
MIRFunction *IpaClone::IpaCloneFunction(MIRFunction &originalFunction, const std::string &fullName) const {
  MapleAllocator cgAlloc(originalFunction.GetDataMemPool());
  ArgVector argument(cgAlloc.Adapter());
  IpaCloneArgument(originalFunction, argument);
  MIRType *retType = originalFunction.GetReturnType();
  MIRFunction *newFunc =
      mirBuilder.CreateFunction(fullName, *retType, argument, false, originalFunction.GetBody() != nullptr);
  CHECK_FATAL(newFunc != nullptr, "create cloned function failed");
  mirBuilder.GetMirModule().AddFunction(newFunc);
  newFunc->SetFlag(originalFunction.GetFlag());
  newFunc->SetSrcPosition(originalFunction.GetSrcPosition());
  newFunc->SetFuncAttrs(originalFunction.GetFuncAttrs());
  newFunc->SetBaseClassFuncNames(GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(fullName));
  newFunc->GetFuncSymbol()->SetAppearsInCode(true);
  newFunc->SetPuidxOrigin(newFunc->GetPuidx());
  if (originalFunction.GetBody() != nullptr) {
    CopyFuncInfo(originalFunction, *newFunc);
    newFunc->SetBody(
        originalFunction.GetBody()->CloneTree(newFunc->GetCodeMempoolAllocator()));
    IpaCloneSymbols(*newFunc, originalFunction);
    IpaCloneLabels(*newFunc, originalFunction);
    IpaClonePregTable(*newFunc, originalFunction);
  }
  newFunc->SetFuncDesc(originalFunction.GetFuncDesc());
  // All the cloned functions cannot be accessed from other transform unit.
  newFunc->SetAttr(FUNCATTR_static);
  return newFunc;
}

MIRFunction *IpaClone::IpaCloneFunctionWithFreq(MIRFunction &originalFunction,
                                                const std::string &fullName, uint64_t callSiteFreq) const {
  MapleAllocator cgAlloc(originalFunction.GetDataMemPool());
  ArgVector argument(cgAlloc.Adapter());
  IpaCloneArgument(originalFunction, argument);
  MIRType *retType = originalFunction.GetReturnType();
  MIRFunction *newFunc =
      mirBuilder.CreateFunction(fullName, *retType, argument, false, originalFunction.GetBody() != nullptr);
  CHECK_FATAL(newFunc != nullptr, "create cloned function failed");
  mirBuilder.GetMirModule().AddFunction(newFunc);
  newFunc->SetFlag(originalFunction.GetFlag());
  newFunc->SetSrcPosition(originalFunction.GetSrcPosition());
  newFunc->SetFuncAttrs(originalFunction.GetFuncAttrs());
  newFunc->SetBaseClassFuncNames(GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(fullName));
  newFunc->GetFuncSymbol()->SetAppearsInCode(true);
  newFunc->SetPuidxOrigin(newFunc->GetPuidx());
  FuncProfInfo *origProfData = originalFunction.GetFuncProfData();
  auto *moduleMp = mirBuilder.GetMirModule().GetMemPool();
  FuncProfInfo *newProfData = moduleMp->New<FuncProfInfo>(&mirBuilder.GetMirModule().GetMPAllocator(),
                                  newFunc->GetPuidx(), 0, 0); // skip checksum information
  newFunc->SetFuncProfData(newProfData);
  newProfData->SetFuncFrequency(callSiteFreq);
  newProfData->SetFuncRealFrequency(callSiteFreq);
  // original function need to update frequency by real entry value
  // update real left frequency
  origProfData->SetFuncRealFrequency(origProfData->GetFuncRealFrequency() - callSiteFreq);
  if (originalFunction.GetBody() != nullptr) {
    CopyFuncInfo(originalFunction, *newFunc);
    BlockNode *newbody = originalFunction.GetBody()->CloneTreeWithFreqs(newFunc->GetCodeMempoolAllocator(),
        newProfData->GetStmtFreqs(), origProfData->GetStmtFreqs(),
        callSiteFreq, /* numer */
        origProfData->GetFuncFrequency(), /* denom */
        static_cast<uint32>(kKeepOrigFreq) | static_cast<uint32>(kUpdateFreqbyScale));
    newFunc->SetBody(newbody);
    IpaCloneSymbols(*newFunc, originalFunction);
    IpaCloneLabels(*newFunc, originalFunction);
    IpaClonePregTable(*newFunc, originalFunction);
  }
  newFunc->SetFuncDesc(originalFunction.GetFuncDesc());
  // All the cloned functions cannot be accessed from other transform unit.
  newFunc->SetAttr(FUNCATTR_static);
  return newFunc;
}

void IpaClone::IpaCloneArgument(MIRFunction &originalFunction, ArgVector &argument) const {
  for (size_t i = 0; i < originalFunction.GetFormalCount(); ++i) {
    auto &formalName = originalFunction.GetFormalName(i);
    argument.emplace_back(ArgPair(formalName, originalFunction.GetNthParamType(i)));
  }
}

void IpaClone::CopyFuncInfo(MIRFunction &originalFunction, MIRFunction &newFunc) const {
  const auto &funcNameIdx = newFunc.GetBaseFuncNameStrIdx();
  const auto &fullNameIdx = newFunc.GetNameStrIdx();
  const auto &classNameIdx = newFunc.GetBaseClassNameStrIdx();
  const static auto &metaFullNameIdx = mirBuilder.GetOrCreateStringIndex(kFullNameStr);
  const static auto &metaClassNameIdx = mirBuilder.GetOrCreateStringIndex(kClassNameStr);
  const static auto &metaFuncNameIdx = mirBuilder.GetOrCreateStringIndex(kFuncNameStr);
  MIRInfoVector &fnInfo = originalFunction.GetInfoVector();
  const MapleVector<bool> &infoIsString = originalFunction.InfoIsString();
  size_t size = fnInfo.size();
  for (size_t i = 0; i < size; ++i) {
    if (fnInfo[i].first == metaFullNameIdx) {
      newFunc.PushbackMIRInfo(std::pair<GStrIdx, uint32>(fnInfo[i].first, fullNameIdx));
    } else if (fnInfo[i].first == metaFuncNameIdx) {
      newFunc.PushbackMIRInfo(std::pair<GStrIdx, uint32>(fnInfo[i].first, funcNameIdx));
    } else if (fnInfo[i].first == metaClassNameIdx) {
      newFunc.PushbackMIRInfo(std::pair<GStrIdx, uint32>(fnInfo[i].first, classNameIdx));
    } else {
      newFunc.PushbackMIRInfo(std::pair<GStrIdx, uint32>(fnInfo[i].first, fnInfo[i].second));
    }
    newFunc.PushbackIsString(infoIsString[i]);
  }
}

bool IpaClone::CheckCostModel(uint32 paramIndex, std::vector<int64_t> &calleeValue,
                              std::vector<ImpExpr> &result) const {
  uint32 impSize = 0;
  for (auto impExpr : result) {
    if (curFunc->GetStmtNodeFromMeId(impExpr.GetStmtId()) == nullptr) {
      continue;
    }
    if (curFunc->GetStmtNodeFromMeId(impExpr.GetStmtId())->IsCondBr() ||
        curFunc->GetStmtNodeFromMeId(impExpr.GetStmtId())->GetOpCode() == OP_if) {
      ++impSize;
    }
  }
  if (impSize >= numOfImpExprHighBound) {
    return true;
  }
  auto &calleeInfo = mirModule->GetCalleeParamAboutInt();
  CalleePair keyPair(curFunc->GetPuidx(), paramIndex);
  uint32 callSiteSize = 0;
  for (auto &value : calleeValue) {
    callSiteSize += static_cast<uint32>(calleeInfo[keyPair][value].size());
  }
  if (callSiteSize >= numOfCallSiteUpBound) {
    return true;
  }
  if (callSiteSize < numOfCallSiteLowBound || impSize < numOfImpExprLowBound) {
    return false;
  }
  // Later: we will consider the body size
  return true;
}

void IpaClone::ReplaceIfCondtion(MIRFunction *newFunc, std::vector<ImpExpr> &result, uint64_t res) const {
  ASSERT(newFunc != nullptr, "null ptr check");
  MemPool *currentFunMp = newFunc->GetCodeMempool();
  auto elemPrimType = PTY_u8;
  MIRType *type = GlobalTables::GetTypeTable().GetPrimType(elemPrimType);
  MIRConst *constVal = nullptr;
  for (int32 index = static_cast<int32>(result.size()) - 1; index >= 0; --index) {
    uint32 stmtId = result[static_cast<uint32>(index)].GetStmtId();
    StmtNode *newReplace = newFunc->GetStmtNodeFromMeId(stmtId);
    ASSERT(newReplace != nullptr, "null ptr check");
    if (newReplace->GetOpCode() == OP_switch) {
      continue;
    }
    if (newReplace->GetOpCode() != OP_if && newReplace->GetOpCode() != OP_brtrue &&
        newReplace->GetOpCode() != OP_brfalse) {
      ASSERT(false, "ERROR: cann't find the replace statement");
    }
    IfStmtNode *ifStmtNode = static_cast<IfStmtNode*>(newReplace);
    constVal = GlobalTables::GetIntConstTable().GetOrCreateIntConst(static_cast<int64>(res & 0x1), *type);
    res >>= 1;
    ConstvalNode *constNode = currentFunMp->New<ConstvalNode>(constVal->GetType().GetPrimType(), constVal);
    ifStmtNode->SetOpnd(constNode, 0);
  }
  return;
}

void IpaClone::RemoveSwitchCase(MIRFunction &newFunc, SwitchNode &switchStmt, std::vector<int64_t> &calleeValue) const {
  auto iter = switchStmt.GetSwitchTable().begin();
  while (iter < switchStmt.GetSwitchTable().end()) {
    bool isNeed = false;
    for (size_t j = 0; j < calleeValue.size(); ++j) {
      int64_t value = calleeValue[j];
      if (switchStmt.GetSwitchOpnd()->GetOpCode() == OP_neg) {
        value = -value;
      }
      if (value == iter->first) {
        isNeed = true;
        break;
      }
    }
    if (!isNeed) {
      iter = switchStmt.GetSwitchTable().erase(iter);
    } else {
      ++iter;
    }
  }
  if (switchStmt.GetSwitchTable().size() == calleeValue.size()) {
    StmtNode *stmt = newFunc.GetBody()->GetFirst();
    for (; stmt != newFunc.GetBody()->GetLast(); stmt = stmt->GetNext()) {
      if (stmt->GetOpCode() == OP_label &&
          static_cast<LabelNode*>(stmt)->GetLabelIdx() == switchStmt.GetDefaultLabel()) {
        switchStmt.SetDefaultLabel(0);
        newFunc.GetBody()->RemoveStmt(stmt);
        break;
      }
    }
  }
}

void IpaClone::RemoveUnneedSwitchCase(MIRFunction &newFunc, std::vector<ImpExpr> &result,
                                      std::vector<int64_t> &calleeValue) const {
  for (size_t i = 0; i < result.size(); ++i) {
    uint32 stmtId = result[i].GetStmtId();
    StmtNode *newSwitch = newFunc.GetStmtNodeFromMeId(stmtId);
    if (newSwitch->GetOpCode() != OP_switch) {
      continue;
    }
    SwitchNode *switchStmtNode = static_cast<SwitchNode*>(newSwitch);
    RemoveSwitchCase(newFunc, *switchStmtNode, calleeValue);
  }
}

void IpaClone::ModifyParameterSideEffect(MIRFunction *newFunc, uint32 paramIndex) const {
  ASSERT(newFunc != nullptr, "null ptr check");
  auto &desc = newFunc->GetFuncDesc();
  if (paramIndex >= kMaxParamCount) {
    return;
  }
  for (size_t idx = paramIndex; idx < kMaxParamCount - 1; ++idx) {
    desc.SetParamInfo(idx, desc.GetParamInfo(idx + 1));
  }
  desc.SetParamInfo(kMaxParamCount - 1, PI::kUnknown);
  return;
}

void IpaClone::RemoveUnneedParameter(MIRFunction *newFunc, uint32 paramIndex, int64_t value) const {
  ASSERT(newFunc != nullptr, "null ptr check");
  if (newFunc->GetBody() != nullptr) {
    MemPool *newFuncMP = newFunc->GetCodeMempool();
    // Create the const value
    MIRType *type = GlobalTables::GetTypeTable().GetPrimType(PTY_i64);
    MIRIntConst *constVal = GlobalTables::GetIntConstTable().GetOrCreateIntConst(static_cast<uint64_t>(value), *type);
    ConstvalNode *constNode = newFuncMP->New<ConstvalNode>(constVal->GetType().GetPrimType(), constVal);
    // Create the dassign statement.
    DassignNode *dass = newFuncMP->New<DassignNode>();
    MIRSymbol *sym = newFunc->GetFormal(paramIndex);
    dass->SetStIdx(sym->GetStIdx());
    dass->SetOpnd(constNode, 0);
    dass->SetFieldID(0);
    // Insert this dassign statment to the body.
    newFunc->GetBody()->InsertFirst(dass);
    // Remove the unneed function parameter.
    auto &formalVec = newFunc->GetFormalDefVec();
    for (size_t i = paramIndex; i < newFunc->GetFormalCount() - 1; ++i) {
      formalVec[i] = formalVec[i + 1];
    }
    formalVec.resize(formalVec.size() - 1);
    sym->SetStorageClass(kScAuto);
    // fix the paramTypelist && paramTypeAttrs.
    MIRFuncType *funcType = newFunc->GetMIRFuncType();
    std::vector<TyIdx> paramTypeList;
    std::vector<TypeAttrs> paramTypeAttrsList;
    for (size_t i = 0; i < newFunc->GetParamTypes().size(); i++) {
      if (i != paramIndex) {
        paramTypeList.push_back(funcType->GetParamTypeList()[i]);
        paramTypeAttrsList.push_back(funcType->GetParamAttrsList()[i]);
      }
    }
    MIRSymbol *funcSymbol = newFunc->GetFuncSymbol();
    ASSERT(funcSymbol != nullptr, "null ptr check");
    funcSymbol->SetTyIdx(GlobalTables::GetTypeTable().GetOrCreateFunctionType(funcType->GetRetTyIdx(), paramTypeList,
        paramTypeAttrsList, funcType->IsVarargs(), funcType->GetRetAttrs())->GetTypeIndex());
    auto *newFuncType = static_cast<MIRFuncType*>(funcSymbol->GetType());
    newFunc->SetMIRFuncType(newFuncType);
    // Modify the parameter sideeffect
    ModifyParameterSideEffect(newFunc, paramIndex);
  }
  return;
}

// Clone Function steps:
// 1. clone Function && replace the condtion
// 2. modify the callsite and update the call_graph
void IpaClone::DecideCloneFunction(std::vector<ImpExpr> &result, uint32 paramIndex,
                                   std::map<uint32, std::vector<int64_t>> &evalMap) const {
  uint32 puidx = curFunc->GetPuidx();
  CalleePair keyPair(puidx, paramIndex);
  auto &calleeInfo = mirModule->GetCalleeParamAboutInt();
  uint32 index = 0;
  for (auto &eval : std::as_const(evalMap)) {
    uint64_t evalValue = eval.first;
    std::vector<int64_t> calleeValue = eval.second;
    if (!CheckCostModel(paramIndex, calleeValue, result)) {
      continue;
    }
    if (index > numOfCloneVersions) {
      break;
    }
    std::string newFuncName = curFunc->GetName() + ".clone." + std::to_string(index++);
    InlineTransformer::ConvertPStaticToFStatic(*curFunc);
    MIRFunction *newFunc = nullptr;
    if (Options::profileUse && curFunc->GetFuncProfData()) {
      uint64_t clonedSiteFreqs = 0;
      for (auto &value: calleeValue) {
        for (auto &callSite : calleeInfo[keyPair][value]) {
          MIRFunction *callerFunc = GlobalTables::GetFunctionTable().GetFunctionFromPuidx(callSite.GetPuidx());
          uint32 stmtId = callSite.GetStmtId();
          CallNode *oldCallNode = static_cast<CallNode*>(callerFunc->GetStmtNodeFromMeId(stmtId));
          if (oldCallNode == nullptr) {
            continue;
          }
          if (callerFunc->GetFuncProfData() == nullptr) {
            continue;
          }
          uint64_t callsiteFreq = callerFunc->GetFuncProfData()->GetStmtFreq(stmtId);
          clonedSiteFreqs += callsiteFreq;
        }
      }
      newFunc = IpaCloneFunctionWithFreq(*curFunc, newFuncName, clonedSiteFreqs);
    } else {
      newFunc = IpaCloneFunction(*curFunc, newFuncName);
    }
    ReplaceIfCondtion(newFunc, result, evalValue);
    RemoveUnneedSwitchCase(*newFunc, result, calleeValue);
    for (auto &value: calleeValue) {
      bool optCallerParam = false;
      if (calleeValue.size() == 1) {
        optCallerParam = true;
        // If the callleeValue just have one value, it means we can add a dassign stmt.
        RemoveUnneedParameter(newFunc, paramIndex, value);
      }
      for (auto &callSite : calleeInfo[keyPair][value]) {
        MIRFunction *callerFunc = GlobalTables::GetFunctionTable().GetFunctionFromPuidx(callSite.GetPuidx());
        if (callerFunc == nullptr) {
          CHECK_FATAL(callSite.GetPuidx() != 0, "something wrong in calleeInfo?");
          continue;  // func has been removed from funcTable by CallGraph, see RemoveFileStaticRootNodes for details
        }
        uint32 stmtId = callSite.GetStmtId();
        CallNode *oldCallNode = static_cast<CallNode*>(callerFunc->GetStmtNodeFromMeId(stmtId));
        if (oldCallNode == nullptr) {
          continue;
        }
        oldCallNode->SetPUIdx(newFunc->GetPuidx());
        if (optCallerParam) {
          for (size_t i = paramIndex; i < oldCallNode->GetNopndSize() - 1; ++i) {
            oldCallNode->SetNOpndAt(i, oldCallNode->GetNopndAt(i + 1));
          }
          oldCallNode->GetNopnd().resize(static_cast<uint8>(oldCallNode->GetNumOpnds() - 1));
          oldCallNode->SetNumOpnds(static_cast<uint8>(oldCallNode->GetNumOpnds() - 1));
        }
      }
    }
  }
}

void IpaClone::ComupteValue(const IntVal& value, const IntVal& paramValue,
                            const CompareNode &cond, uint64_t &bitRes) const {
  if (cond.GetOpCode() == OP_gt) {
    bitRes = static_cast<uint64_t>(value > paramValue) | (bitRes << 1);
  } else if (cond.GetOpCode() == OP_eq) {
    bitRes = static_cast<uint64_t>(value == paramValue) | (bitRes << 1);
  } else if (cond.GetOpCode() == OP_lt) {
    bitRes = static_cast<uint64_t>(value < paramValue) | (bitRes << 1);
  } else if (cond.GetOpCode() == OP_ge) {
    bitRes = static_cast<uint64_t>(value >= paramValue) | (bitRes << 1);
  } else if (cond.GetOpCode() == OP_le) {
    bitRes = static_cast<uint64_t>(value <= paramValue) | (bitRes << 1);
  } else if (cond.GetOpCode() == OP_ne) {
    bitRes = static_cast<uint64_t>(value != paramValue) | (bitRes << 1);
  }
}

void IpaClone::EvalCompareResult(std::vector<ImpExpr> &result, std::map<uint32, std::vector<int64_t>> &evalMap,
                                 std::map<int64_t, std::vector<CallerSummary>> &summary, uint32 index) const {
  for (auto &it: std::as_const(summary)) {
    int64 value = it.first;
    uint64_t bitRes = 0;
    bool runFlag = false;
    for (auto &expr : result) {
      StmtNode *stmt  = curFunc->GetStmtNodeFromMeId(expr.GetStmtId());
      if (stmt == nullptr || expr.GetParamIndex() != index) {
        continue;
      }
      runFlag = true;
      if (stmt->GetOpCode() == OP_switch) {
        continue;
      }
      IfStmtNode* ifStmt = static_cast<IfStmtNode*>(stmt);
      CompareNode *cond = static_cast<CompareNode*>(ifStmt->Opnd(0));
      if (cond->Opnd(0)->GetOpCode() == OP_intrinsicop &&
          static_cast<IntrinsicopNode*>(cond->Opnd(0))->GetIntrinsic() == INTRN_C___builtin_expect) {
        cond = static_cast<CompareNode*>(static_cast<IntrinsicopNode*>(cond->Opnd(0))->Opnd(0));
      }
      PrimType primType = cond->GetOpndType();
      BaseNode *opnd1 = cond->Opnd(0)->GetOpCode() == OP_constval ? cond->Opnd(0) : cond->Opnd(1);
      ConstvalNode *constNode = static_cast<ConstvalNode*>(opnd1);
      MIRIntConst *constVal = safe_cast<MIRIntConst>(constNode->GetConstVal());
      ASSERT(constVal, "invalid const type");
      if (primType != PTY_i64 && primType != PTY_u64 && primType != PTY_i32 && primType != PTY_u32 &&
          primType != PTY_i16 && primType != PTY_u16 && primType != PTY_i8 && primType != PTY_u8) {
        runFlag = false;
        break;
      }
      IntVal paramValue = { constVal->GetValue(), primType };
      IntVal newValue = { static_cast<uint64>(value), primType };
      ComupteValue(newValue, paramValue, *cond, bitRes);
    }
    if (runFlag) {
      (void)evalMap[bitRes].emplace_back(value);
    }
  }
  return;
}

void IpaClone::EvalImportantExpression(MIRFunction *func, std::vector<ImpExpr> &result) {
  int paramSize = static_cast<int>(func->GetFormalCount());
  uint32 puidx = func->GetPuidx();
  auto &calleeInfo = mirModule->GetCalleeParamAboutInt();
  for (int index = 0; index < paramSize; ++index) {
    CalleePair keyPair(puidx, index);
    if (calleeInfo.find(keyPair) == calleeInfo.end()) {
      continue;
    }
    std::map<uint32, std::vector<int64_t > > evalMap;
    EvalCompareResult(result, evalMap, calleeInfo[keyPair], static_cast<uint32>(index));
    // Later: Now we just the consider one parameter important expression
    std::vector<ImpExpr> filterRes;
    if (!evalMap.empty()) {
      bool hasBrExpr = false;
      for (auto &expr : result) {
        if (expr.GetParamIndex() == static_cast<uint32>(index) &&
            func->GetStmtNodeFromMeId(expr.GetStmtId()) != nullptr) {
          hasBrExpr = IsBrCondOrIf(func->GetStmtNodeFromMeId(expr.GetStmtId())->GetOpCode()) || hasBrExpr;
          (void)filterRes.emplace_back(expr);
          // Resolve most numOfImpExprUpper important expression
          if (filterRes.size() > kNumOfImpExprUpper) {
            break;
          }
        }
      }
      if (hasBrExpr) {
        DecideCloneFunction(filterRes, static_cast<uint32>(index), evalMap);
        return;
      }
    }
  }
}

void IpaClone::CloneNoImportantExpressFunction(MIRFunction *func, uint32 paramIndex) const {
  uint32 puidx = curFunc->GetPuidx();
  CalleePair keyPair(puidx, paramIndex);
  auto &calleeInfo = mirModule->GetCalleeParamAboutInt();
  std::string newFuncName = func->GetName() + ".constprop." + std::to_string(paramIndex);
  InlineTransformer::ConvertPStaticToFStatic(*func);
  MIRFunction *newFunc = nullptr;
  if (Options::profileUse && func->GetFuncProfData()) {
    uint64_t clonedSiteFreqs = 0;
    int64_t value = calleeInfo[keyPair].cbegin()->first;
    for (auto &callSite : std::as_const(calleeInfo[keyPair][value])) {
      MIRFunction *callerFunc = GlobalTables::GetFunctionTable().GetFunctionFromPuidx(callSite.GetPuidx());
      uint32 stmtId = callSite.GetStmtId();
      CallNode *oldCallNode = static_cast<CallNode*>(callerFunc->GetStmtNodeFromMeId(stmtId));
      if (oldCallNode == nullptr) {
        continue;
      }
      if (callerFunc->GetFuncProfData() == nullptr) {
        continue;
      }
      uint64_t callsiteFreq = callerFunc->GetFuncProfData()->GetStmtFreq(stmtId);
      clonedSiteFreqs += callsiteFreq;
    }
    newFunc = IpaCloneFunctionWithFreq(*func, newFuncName, clonedSiteFreqs);
  } else {
    newFunc = IpaCloneFunction(*func, newFuncName);
  }
  int64_t value = calleeInfo[keyPair].begin()->first;
  RemoveUnneedParameter(newFunc, paramIndex, value);
  for (auto &callSite : calleeInfo[keyPair][value]) {
    MIRFunction *callerFunc = GlobalTables::GetFunctionTable().GetFunctionFromPuidx(callSite.GetPuidx());
    if (callerFunc == nullptr) {
      CHECK_FATAL(callSite.GetPuidx() != 0, "something wrong in calleeInfo?");
      continue;  // func has been removed from funcTable by CallGraph, see RemoveFileStaticRootNodes for details
    }
    uint32 stmtId = callSite.GetStmtId();
    CallNode *oldCallNode = static_cast<CallNode*>(callerFunc->GetStmtNodeFromMeId(stmtId));
    if (oldCallNode == nullptr) {
      continue;
    }
    oldCallNode->SetPUIdx(newFunc->GetPuidx());
    for (size_t i = paramIndex; i < oldCallNode->GetNopndSize() - 1; ++i) {
      oldCallNode->SetNOpndAt(i, oldCallNode->GetNopndAt(i + 1));
    }
    oldCallNode->GetNopnd().resize(static_cast<uint8>(oldCallNode->GetNumOpnds() - 1));
    oldCallNode->SetNumOpnds(static_cast<uint8>(oldCallNode->GetNumOpnds() - 1));
  }
}

bool IpaClone::CheckImportantExprHasBr(const std::vector<ImpExpr> &exprVec) const {
  for (auto expr : exprVec) {
    if (curFunc->GetStmtNodeFromMeId(expr.GetStmtId()) != nullptr &&
        IsBrCondOrIf(curFunc->GetStmtNodeFromMeId(expr.GetStmtId())->GetOpCode())) {
      return true;
    }
  }
  return false;
}

void IpaClone::DoIpaClone() {
  InitParams();
  for (uint32 i = 0; i < GlobalTables::GetFunctionTable().GetFuncTable().size(); ++i) {
    MIRFunction *func = GlobalTables::GetFunctionTable().GetFunctionFromPuidx(i);
    if (func == nullptr) {
      continue;
    }
    if (Options::stackProtectorStrong && func->GetMayWriteToAddrofStack()) {
      continue;
    }
    curFunc = func;
    std::map<PUIdx, std::vector<ImpExpr>> &funcImportantExpr = mirModule->GetFuncImportantExpr();
    if (funcImportantExpr.find(func->GetPuidx()) != funcImportantExpr.end() &&
        CheckImportantExprHasBr(funcImportantExpr[func->GetPuidx()])) {
      EvalImportantExpression(func, funcImportantExpr[func->GetPuidx()]);
    } else {
      auto &calleeInfo = mirModule->GetCalleeParamAboutInt();
      for (uint index = 0; index < func->GetFormalCount(); ++index) {
        CalleePair keyPair(func->GetPuidx(), index);
        if (calleeInfo.find(keyPair) != calleeInfo.end() && calleeInfo[keyPair].size() == 1 &&
            (calleeInfo[keyPair].begin())->second.size() > numOfConstpropValue) {
          CloneNoImportantExpressFunction(func, index);
          break;
        }
      }
    }
  }
}

void M2MIpaClone::GetAnalysisDependence(AnalysisDep &aDep) const {
  aDep.AddRequired<M2MCallGraph>();
  aDep.PreservedAllExcept<M2MCallGraph>();
}

bool M2MIpaClone::PhaseRun(maple::MIRModule &m) {
  maple::MIRBuilder dexMirBuilder(&m);
  cl = GetPhaseAllocator()->New<IpaClone>(&m, GetPhaseMemPool(), dexMirBuilder);
  cl->DoIpaClone();
  GetAnalysisInfoHook()->ForceEraseAnalysisPhase(m.GetUniqueID(), &M2MCallGraph::id);
  (void)GetAnalysisInfoHook()->ForceRunAnalysisPhase<MapleModulePhase, MIRModule>(&M2MCallGraph::id, m);
  return true;
}
}  // namespace maple
