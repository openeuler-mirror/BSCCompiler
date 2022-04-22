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
#include <iostream>
#include <algorithm>
#include "mir_symbol.h"
#include "func_desc.h"

// For some funcs, when we can ignore their return-values, we clone a new func of
// them without return-values. We configure a list to save these funcs and clone
// at the very beginning so that clones can also enjoy the optimizations after.
// This mainly contains the clone of funcbody(include labels, symbols, arguments,
// etc.) and the update of the new func infomation.
namespace maple {
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
    MIRPreg *temp = const_cast<MIRPreg*>(oldFunc.GetPregTab()->GetPregTableItem(i));
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
  if (originalFunction.GetBody() != nullptr) {
    CopyFuncInfo(originalFunction, *newFunc);
    newFunc->SetBody(
        originalFunction.GetBody()->CloneTree(newFunc->GetCodeMempoolAllocator()));
    IpaCloneSymbols(*newFunc, originalFunction);
    IpaCloneLabels(*newFunc, originalFunction);
    IpaClonePregTable(*newFunc, originalFunction);
  }
  newFunc->SetFuncDesc(originalFunction.GetFuncDesc());
  return newFunc;
}

void IpaClone::IpaCloneArgument(MIRFunction &originalFunction, ArgVector &argument) const {
  for (size_t i = 0; i < originalFunction.GetFormalCount(); ++i) {
    auto &formalName = originalFunction.GetFormalName(i);
    argument.push_back(ArgPair(formalName, originalFunction.GetNthParamType(i)));
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

bool IpaClone::CheckCostModel(MIRFunction *newFunc, uint32 paramIndex, std::vector<int64_t> &calleeValue,
                              uint32 impSize) {
  if (impSize >= kNumOfImpExprHighBound) {
    return true;
  }
  auto &calleeInfo = mirModule->GetCalleeParamAboutInt();
  CalleePair keyPair(curFunc->GetPuidx(), paramIndex);
  int callSiteSize = 0;
  for (auto &value : calleeValue) {
    callSiteSize += calleeInfo[keyPair][value].size();
  }
  if (callSiteSize >= kNumOfCallSiteUpBound) {
    return true;
  }
  if (callSiteSize < kNumOfCallSiteLowBound || impSize < kNumOfImpExprLowBound) {
    return false;
  }
  // Later: we will consider the body size
  return true;
}

void IpaClone::ReplaceIfCondtion(MIRFunction *newFunc, std::vector<ImpExpr> &result, uint64_t res) {
  MemPool *currentFunMp = newFunc->GetCodeMempool();
  auto elemPrimType = PTY_u8;
  MIRType *type = GlobalTables::GetTypeTable().GetPrimType(elemPrimType);
  MIRConst *constVal = nullptr;
  for (int32 index = result.size() - 1; index >= 0; index--) {
    uint32 stmtId = result[index].GetStmtId();
    StmtNode *newReplace = newFunc->GetStmtNodeFromMeId(stmtId);
    if (newReplace->GetOpCode() != OP_if && newReplace->GetOpCode() != OP_brtrue &&
        newReplace->GetOpCode() != OP_brfalse) {
      ASSERT(false, "ERROR: cann't find the replace statement");
    }
    IfStmtNode *IfstmtNode = static_cast<IfStmtNode*>(newReplace);
    constVal = GlobalTables::GetIntConstTable().GetOrCreateIntConst(res & 0x1, *type);
    res >>= 1;
    ConstvalNode *constNode = currentFunMp->New<ConstvalNode>(constVal->GetType().GetPrimType(), constVal);
    IfstmtNode->SetOpnd(constNode, 0);
  }
  return;
}

void IpaClone::ModifyParameterSideEffect(MIRFunction *newFunc, uint32 paramIndex) {
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

void IpaClone::RemoveUnneedParameter(MIRFunction *newFunc, uint32 paramIndex, int64_t value) {
  if (newFunc->GetBody() != nullptr) {
    MemPool *newFuncMP = newFunc->GetCodeMempool();
    // Create the const value
    MIRType *type = GlobalTables::GetTypeTable().GetPrimType(PTY_i64);
    MIRIntConst *constVal = GlobalTables::GetIntConstTable().GetOrCreateIntConst(value, *type);
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
                                   std::map<uint32, std::vector<int64_t>> &evalMap) {
  uint32 puidx = curFunc->GetPuidx();
  CalleePair keyPair(puidx, paramIndex);
  auto &calleeInfo = mirModule->GetCalleeParamAboutInt();
  int index = 0;
  for (auto &eval : evalMap) {
    uint64_t evalValue = eval.first;
    std::vector<int64_t> calleeValue = eval.second;
    if (!CheckCostModel(curFunc, paramIndex, calleeValue, result.size())) {
      continue;
    }
    if (index > kNumOfCloneVersions) {
      break;
    }
    std::string newFuncName = curFunc->GetName() + ".clone." + std::to_string(index++);
    MIRFunction *newFunc = IpaCloneFunction(*curFunc, newFuncName);
    ReplaceIfCondtion(newFunc, result, evalValue);
    for (auto &value: calleeValue) {
      bool optCallerParam = false;
      if (calleeValue.size() == 1) {
        optCallerParam = true;
        //If the callleeValue just have one value, it means we can add a dassign stmt.
        RemoveUnneedParameter(newFunc, paramIndex, value);
      }
      for (auto &callSite : calleeInfo[keyPair][value]) {
        MIRFunction *callerFunc = GlobalTables::GetFunctionTable().GetFunctionFromPuidx(callSite.GetPuidx());
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
          oldCallNode->GetNopnd().resize(oldCallNode->GetNumOpnds() - 1);
          oldCallNode->SetNumOpnds(oldCallNode->GetNumOpnds() - 1);
        }
      }
    }
  }
}

template<typename dataT>
void IpaClone::ComupteValue(dataT value, dataT paramValue, CompareNode *cond, uint64_t &bitRes) {
  if (cond->GetOpCode() == OP_gt) {
    bitRes = (value > paramValue) | (bitRes << 1);
  } else if (cond->GetOpCode() == OP_eq) {
    bitRes = (value == paramValue) | (bitRes << 1);
  } else if (cond->GetOpCode() == OP_lt) {
    bitRes = (value < paramValue) | (bitRes << 1);
  } else if (cond->GetOpCode() == OP_ge) {
    bitRes = (value >= paramValue) | (bitRes << 1);
  } else if (cond->GetOpCode() == OP_le) {
    bitRes = (value <= paramValue) | (bitRes << 1);
  } else if (cond->GetOpCode() == OP_ne) {
    bitRes = (value != paramValue) | (bitRes << 1);
  }
}

void IpaClone::EvalCompareResult(std::vector<ImpExpr> &result, std::map<uint32, std::vector<int64_t > > &evalMap,
                                 std::map<int64_t, std::vector<CallerSummary>> &summary, uint32 index) {
  for (auto &it: summary) {
    int64_t value = it.first;
    uint64_t bitRes = 0;
    bool runFlag = false;
    for (auto &expr : result) {
      StmtNode *stmt  = curFunc->GetStmtNodeFromMeId(expr.GetStmtId());
      if (stmt == nullptr || expr.GetParamIndex() != index) {
        continue;
      }
      runFlag = true;
      IfStmtNode* ifStmt = static_cast<IfStmtNode*>(stmt);
      CompareNode *cond = static_cast<CompareNode*>(ifStmt->Opnd(0));
      PrimType primType = cond->GetOpndType();
      BaseNode *opnd1 = cond->Opnd(1);
      ConstvalNode *constNode = static_cast<ConstvalNode*>(opnd1);
      int64_t paramValue = static_cast<MIRIntConst*>(constNode->GetConstVal())->GetValue();
      if (primType == PTY_i64) {
        ComupteValue(value, paramValue, cond, bitRes);
      } else if (primType == PTY_u64) {
        ComupteValue(static_cast<uint64_t>(value), static_cast<uint64_t>(paramValue), cond, bitRes);
      } else if (primType == PTY_u32) {
        ComupteValue(static_cast<uint32_t>(value), static_cast<uint32_t>(paramValue), cond, bitRes);
      } else if (primType == PTY_i32) {
        ComupteValue(static_cast<int32_t>(value), static_cast<int32_t>(paramValue), cond, bitRes);
      } else if (primType == PTY_u16) {
        ComupteValue(static_cast<uint16_t>(value), static_cast<uint16_t>(paramValue), cond, bitRes);
      } else if (primType == PTY_i16) {
        ComupteValue(static_cast<int16_t>(value), static_cast<int16_t>(paramValue), cond, bitRes);
      } else if (primType == PTY_u8) {
        ComupteValue(static_cast<uint8_t>(value), static_cast<uint8_t>(paramValue), cond, bitRes);
      } else if (primType == PTY_i8) {
        ComupteValue(static_cast<int8_t>(value), static_cast<int8_t>(paramValue), cond, bitRes);
      } else {
        runFlag = false;
        break;
      }
    }
    if (runFlag) {
      evalMap[bitRes].emplace_back(value);
    }
  }
  return;
}

void IpaClone::EvalImportantExpression(MIRFunction *func, std::vector<ImpExpr> &result) {
  int paramSize = func->GetFormalCount();
  uint32 puidx = func->GetPuidx();
  auto &calleeInfo = mirModule->GetCalleeParamAboutInt();
  for (int index = 0; index < paramSize; ++index) {
    CalleePair keyPair(puidx, index);
    if (calleeInfo.find(keyPair) == calleeInfo.end()) {
      continue;
    }
    std::map<uint32, std::vector<int64_t > > evalMap;
    EvalCompareResult(result, evalMap ,calleeInfo[keyPair], index);
    // Later: Now we just the consider one parameter important expression
    std::vector<ImpExpr> filterRes;
    if (!evalMap.empty()) {
      for (auto &expr : result) {
        if (expr.GetParamIndex() == index && func->GetStmtNodeFromMeId(expr.GetStmtId()) != nullptr) {
          filterRes.emplace_back(expr);
          // Resolve most kNumOfImpExprUpper important expression
          if (filterRes.size() > kNumOfImpExprUpper) {
            break;
          }
        }
      }
      DecideCloneFunction(filterRes, index, evalMap);
      return;
    }
  }
}

void IpaClone::CloneNoImportantExpressFunction(MIRFunction *func, uint32 paramIndex) {
  uint32 puidx = curFunc->GetPuidx();
  CalleePair keyPair(puidx, paramIndex);
  auto &calleeInfo = mirModule->GetCalleeParamAboutInt();
  std::string newFuncName = func->GetName() + ".constprop." + std::to_string(paramIndex);
  MIRFunction *newFunc = IpaCloneFunction(*func, newFuncName);
  int64_t value = calleeInfo[keyPair].begin()->first;
  RemoveUnneedParameter(newFunc, paramIndex, value);
  for (auto &callSite : calleeInfo[keyPair][value]) {
    MIRFunction *callerFunc = GlobalTables::GetFunctionTable().GetFunctionFromPuidx(callSite.GetPuidx());
    uint32 stmtId = callSite.GetStmtId();
    CallNode *oldCallNode = static_cast<CallNode*>(callerFunc->GetStmtNodeFromMeId(stmtId));
    if (oldCallNode == nullptr) {
      continue;
    }
    oldCallNode->SetPUIdx(newFunc->GetPuidx());
    for (size_t i = paramIndex; i < oldCallNode->GetNopndSize() - 1; ++i) {
      oldCallNode->SetNOpndAt(i, oldCallNode->GetNopndAt(i + 1));
    }
    oldCallNode->GetNopnd().resize(oldCallNode->GetNumOpnds() - 1);
    oldCallNode->SetNumOpnds(oldCallNode->GetNumOpnds() - 1);
  }
}

void IpaClone::DoIpaClone() {
  for (uint32 i = 0; i < GlobalTables::GetFunctionTable().GetFuncTable().size(); ++i) {
    MIRFunction *func = GlobalTables::GetFunctionTable().GetFunctionFromPuidx(i);
    if (func == nullptr) {
      continue;
    }
    curFunc = func;
    std::map<PUIdx, std::vector<ImpExpr>> &funcImportantExpr = mirModule->GetFuncImportantExpr();
    if (funcImportantExpr.find(func->GetPuidx()) != funcImportantExpr.end()) {
      EvalImportantExpression(func, funcImportantExpr[func->GetPuidx()]);
    } else {
      auto &calleeInfo = mirModule->GetCalleeParamAboutInt();
      for (uint index = 0; index < func->GetFormalCount(); ++index) {
        CalleePair keyPair(func->GetPuidx(), index);
        if (calleeInfo.find(keyPair) != calleeInfo.end() && calleeInfo[keyPair].size() == 1 &&
            (calleeInfo[keyPair].begin())->second.size() > kNumOfConstpropValue) {
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