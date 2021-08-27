/*
 * Copyright (c) [2020-2021] Huawei Technologies Co.,Ltd.All rights reserved.
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
#include "clone.h"
#include <iostream>
#include <algorithm>
#include "mir_symbol.h"

// For some funcs, when we can ignore their return-values, we clone a new func of
// them without return-values. We configure a list to save these funcs and clone
// at the very beginning so that clones can also enjoy the optimizations after.
// This mainly contains the clone of funcbody(include labels, symbols, arguments,
// etc.) and the update of the new func infomation.
namespace maple {
ReplaceRetIgnored::ReplaceRetIgnored(MemPool *memPool)
    : memPool(memPool), allocator(memPool), toBeClonedFuncNames(allocator.Adapter()) {
#define ORIFUNC(ORIGINAL, CLONED) toBeClonedFuncNames.insert(MapleString(#ORIGINAL, memPool));
#include "tobe_cloned_funcs.def"
#undef ORIFUNC
}

bool ReplaceRetIgnored::RealShouldReplaceWithVoidFunc(Opcode op, size_t nRetSize,
    const MIRFunction &calleeFunc) const {
  MIRType *returnType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(calleeFunc.GetReturnTyIdx());
  return nRetSize == 0 && (op == OP_virtualcallassigned || op == OP_callassigned || op == OP_superclasscallassigned) &&
         !calleeFunc.IsNative() && (returnType->GetKind() == kTypePointer) && (returnType->GetPrimType() == PTY_ref);
}

bool ReplaceRetIgnored::ShouldReplaceWithVoidFunc(const CallMeStmt &stmt, const MIRFunction &calleeFunc) const {
  return RealShouldReplaceWithVoidFunc(stmt.GetOp(), stmt.MustDefListSize(), calleeFunc);
}

std::string ReplaceRetIgnored::GenerateNewBaseName(const MIRFunction &originalFunc) const {
  return std::string(originalFunc.GetBaseFuncName()).append(kVoidRetSuffix);
}

std::string ReplaceRetIgnored::GenerateNewFullName(const MIRFunction &originalFunc) const {
  const std::string &oldSignature = originalFunc.GetSignature();
  auto retPos = oldSignature.find("_29");
  constexpr auto retPosIndex = 3;
  return std::string(originalFunc.GetBaseClassName()).append(namemangler::kNameSplitterStr).
      append(GenerateNewBaseName(originalFunc)).append(namemangler::kNameSplitterStr).
      append(oldSignature.substr(0, retPos + retPosIndex)).append("V");
}

MIRSymbol *Clone::CloneLocalSymbol(const MIRSymbol &oldSym, const MIRFunction &newFunc) {
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

void Clone::CloneSymbols(MIRFunction &newFunc, const MIRFunction &oldFunc) {
  size_t symTabSize = oldFunc.GetSymbolTabSize();
  for (size_t i = oldFunc.GetFormalCount() + 1; i < symTabSize; ++i) {
    MIRSymbol *sym = oldFunc.GetSymbolTabItem(i);
    if (sym == nullptr) {
      continue;
    }
    MIRSymbol *newSym = CloneLocalSymbol(*sym, newFunc);
    if (!newFunc.GetSymTab()->AddStOutside(newSym)) {
      CHECK_FATAL(false, "%s already existed in func %s", sym->GetName().c_str(), newFunc.GetName().c_str());
    }
  }
}

void Clone::CloneLabels(MIRFunction &newFunc, const MIRFunction &oldFunc) {
  size_t labelTabSize = oldFunc.GetLabelTab()->GetLabelTableSize();
  for (size_t i = 1; i < labelTabSize; ++i) {
    const std::string &labelName = oldFunc.GetLabelTabItem(i);
    GStrIdx strIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(labelName);
    (void)newFunc.GetLabelTab()->AddLabel(strIdx);
  }
}

// Clone a function
MIRFunction *Clone::CloneFunction(MIRFunction &originalFunction, const std::string &newBaseFuncName,
    MIRType *returnType) const {
  MapleAllocator cgAlloc(originalFunction.GetCodeMempool());
  ArgVector argument(cgAlloc.Adapter());
  CloneArgument(originalFunction, argument);
  MIRType *retType = returnType;
  if (retType == nullptr) {
    retType = originalFunction.GetReturnType();
  }
  std::string fullName = originalFunction.GetBaseClassName();
  const std::string &signature = originalFunction.GetSignature();
  fullName = fullName.append(
      namemangler::kNameSplitterStr).append(newBaseFuncName).append(namemangler::kNameSplitterStr).append(signature);
  MIRFunction *newFunc =
      mirBuilder.CreateFunction(fullName, *retType, argument, false, originalFunction.GetBody() != nullptr);
  CHECK_FATAL(newFunc != nullptr, "create cloned function failed");
  mirBuilder.GetMirModule().AddFunction(newFunc);
  Klass *klass = kh->GetKlassFromName(originalFunction.GetBaseClassName());
  CHECK_FATAL(klass != nullptr, "getklass failed");
  klass->AddMethod(newFunc);
  newFunc->SetClassTyIdx(originalFunction.GetClassTyIdx());
  MIRClassType *classType = klass->GetMIRClassType();
  classType->GetMethods().push_back(
      MethodPair(newFunc->GetStIdx(), TyidxFuncAttrPair(newFunc->GetFuncSymbol()->GetTyIdx(),
          originalFunction.GetFuncAttrs())));
  newFunc->SetFlag(originalFunction.GetFlag());
  newFunc->SetSrcPosition(originalFunction.GetSrcPosition());
  newFunc->SetFuncAttrs(originalFunction.GetFuncAttrs());
  newFunc->SetBaseClassFuncNames(GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(fullName));
  if (originalFunction.GetBody() != nullptr) {
    CopyFuncInfo(originalFunction, *newFunc);
    MIRFunction *originalCurrFunction = mirBuilder.GetCurrentFunctionNotNull();
    mirBuilder.SetCurrentFunction(*newFunc);
    newFunc->SetBody(
        originalFunction.GetBody()->CloneTree(originalFunction.GetModule()->GetCurFuncCodeMPAllocator()));
    CloneSymbols(*newFunc, originalFunction);
    CloneLabels(*newFunc, originalFunction);
    mirBuilder.SetCurrentFunction(*originalCurrFunction);
  }
  return newFunc;
}

void Clone::CloneArgument(MIRFunction &originalFunction, ArgVector &argument) const {
  for (size_t i = 0; i < originalFunction.GetFormalCount(); ++i) {
    auto &formalName = originalFunction.GetFormalName(i);
    argument.push_back(ArgPair(formalName, originalFunction.GetNthParamType(i)));
  }
}

void Clone::CopyFuncInfo(MIRFunction &originalFunction, MIRFunction &newFunc) const {
  auto funcNameIdx = newFunc.GetBaseFuncNameStrIdx();
  auto fullNameIdx = newFunc.GetNameStrIdx();
  auto classNameIdx = newFunc.GetBaseClassNameStrIdx();
  auto metaFullNameIdx = mirBuilder.GetOrCreateStringIndex(kFullNameStr);
  auto metaClassNameIdx = mirBuilder.GetOrCreateStringIndex(kClassNameStr);
  auto metaFuncNameIdx = mirBuilder.GetOrCreateStringIndex(kFuncNameStr);
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

void Clone::UpdateFuncInfo(MIRFunction &newFunc) {
  auto fullNameIdx = newFunc.GetNameStrIdx();
  auto metaFullNameIdx = mirBuilder.GetOrCreateStringIndex(kFullNameStr);
  size_t size = newFunc.GetInfoVector().size();
  for (size_t i = 0; i < size; ++i) {
    if (newFunc.GetInfoPair(i).first == metaFullNameIdx) {
      newFunc.SetMIRInfoNum(i, fullNameIdx);
      break;
    }
  }
}

// Clone all functions that would be invoked with their return value ignored
// @param original_function The original function to be cloned
// @param mirBuilder A helper object
// @return Pointer to the newly cloned function
MIRFunction *Clone::CloneFunctionNoReturn(MIRFunction &originalFunction) {
  const std::string oldSignature = originalFunction.GetSignature();
  const std::string kNewMethodBaseName = replaceRetIgnored->GenerateNewBaseName(originalFunction);
  MIRFunction *originalCurrFunction = mirBuilder.GetMirModule().CurFunction();
  MIRFunction *newFunction =
      CloneFunction(originalFunction, kNewMethodBaseName, GlobalTables::GetTypeTable().GetTypeFromTyIdx(1));

  // new stmt should be located in the newFunction->codemp, mirBuilder.CreateStmtReturn will use CurFunction().codemp
  // to assign space for the new stmt. So we set it correctly here.
  mirBuilder.GetMirModule().SetCurFunction(newFunction);
  if (originalFunction.GetBody() != nullptr) {
    auto *body = newFunction->GetBody();
    for (auto &stmt : body->GetStmtNodes()) {
      if (stmt.GetOpCode() == OP_return) {
        body->ReplaceStmt1WithStmt2(&stmt, mirBuilder.CreateStmtReturn(nullptr));
      }
    }
  }
  // setup new names for the newly cloned function
  std::string newFuncFullName = replaceRetIgnored->GenerateNewFullName(originalFunction);
  GStrIdx fullNameStrIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(newFuncFullName);
  newFunction->OverrideBaseClassFuncNames(fullNameStrIdx);
  MIRSymbol *funcSt = newFunction->GetFuncSymbol();
  GlobalTables::GetGsymTable().RemoveFromStringSymbolMap(*funcSt);
  funcSt->SetNameStrIdx(fullNameStrIdx);
  GlobalTables::GetGsymTable().AddToStringSymbolMap(*funcSt);
  UpdateFuncInfo(*newFunction);
  mirBuilder.GetMirModule().SetCurFunction(originalCurrFunction);
  return newFunction;
}

void Clone::UpdateReturnVoidIfPossible(CallMeStmt *callMeStmt, const MIRFunction &targetFunc) {
  if (callMeStmt == nullptr) {
    return;
  }
  if (replaceRetIgnored->ShouldReplaceWithVoidFunc(*callMeStmt, targetFunc) &&
      replaceRetIgnored->IsInCloneList(targetFunc.GetName())) {
    std::string funcNameReturnVoid = replaceRetIgnored->GenerateNewFullName(targetFunc);
    GStrIdx gStrIdx = GlobalTables::GetStrTable().GetStrIdxFromName(funcNameReturnVoid);
    MIRFunction *funcReturnVoid = GlobalTables::GetGsymTable().GetSymbolFromStrIdx(gStrIdx)->GetFunction();
    CHECK_FATAL(funcReturnVoid != nullptr, "target function not found at ssadevirtual");
    callMeStmt->SetPUIdx(funcReturnVoid->GetPuidx());
  }
}

void Clone::DoClone() {
  std::set<std::string> clonedNewFuncMap;
  for (const MapleString &funcName : *(replaceRetIgnored->GetTobeClonedFuncNames())) {
    GStrIdx gStrIdx = GlobalTables::GetStrTable().GetStrIdxFromName(std::string(funcName.c_str()));
    MIRSymbol *symbol = GlobalTables::GetGsymTable().GetSymbolFromStrIdx(gStrIdx);
    if (symbol != nullptr) {
      GStrIdx gStrIdxOfFunc = GlobalTables::GetStrTable().GetStrIdxFromName(std::string(funcName.c_str()));
      MIRFunction *oriFunc = GlobalTables::GetGsymTable().GetSymbolFromStrIdx(gStrIdxOfFunc)->GetFunction();
      mirModule->SetCurFunction(oriFunc);
      (void)clonedNewFuncMap.insert(CloneFunctionNoReturn(*oriFunc)->GetName());
    }
  }
}

void M2MClone::GetAnalysisDependence(AnalysisDep &aDep) const {
  aDep.AddRequired<M2MKlassHierarchy>();
}

bool M2MClone::PhaseRun(maple::MIRModule &m) {
  maple::MIRBuilder dexMirBuilder(&m);
  KlassHierarchy *kh = GET_ANALYSIS(M2MKlassHierarchy, m);
  cl = GetPhaseAllocator()->New<Clone>(&m, GetPhaseMemPool(), dexMirBuilder, kh);
  cl->DoClone();
  return true;
}
}  // namespace maple
