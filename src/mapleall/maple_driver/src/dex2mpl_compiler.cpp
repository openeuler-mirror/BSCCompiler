/*
 * Copyright (c) [2019-2020] Huawei Technologies Co.,Ltd.All rights reserved.
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
#include "compiler.h"
#include "default_options.def"
#ifdef INTERGRATE_DRIVER
#include "dex2mpl_runner.h"
#include "mir_function.h"
#endif

namespace maple {
const std::string &Dex2MplCompiler::GetBinName() const {
  return kBinNameDex2mpl;
}

DefaultOption Dex2MplCompiler::GetDefaultOptions(const MplOptions &options, const Action &) const {
  DefaultOption defaultOptions = { nullptr, 0 };
  if (options.GetOptimizationLevel() == kO0 && options.HasSetDefaultLevel()) {
    defaultOptions.mplOptions = kDex2mplDefaultOptionsO0;
    defaultOptions.length = sizeof(kDex2mplDefaultOptionsO0) / sizeof(MplOption);
  } else if (options.GetOptimizationLevel() == kO2 && options.HasSetDefaultLevel()) {
    defaultOptions.mplOptions = kDex2mplDefaultOptionsO2;
    defaultOptions.length = sizeof(kDex2mplDefaultOptionsO2) / sizeof(MplOption);
  }
  for (unsigned int i = 0; i < defaultOptions.length; ++i) {
    defaultOptions.mplOptions[i].SetValue(
        FileUtils::AppendMapleRootIfNeeded(defaultOptions.mplOptions[i].GetNeedRootPath(),
                                           defaultOptions.mplOptions[i].GetValue(),
                                           options.GetExeFolder()));
  }
  return defaultOptions;
}

void Dex2MplCompiler::GetTmpFilesToDelete(const MplOptions &, const Action &action,
                                          std::vector<std::string> &tempFiles) const {
  tempFiles.push_back(action.GetFullOutputName() + ".mpl");
  tempFiles.push_back(action.GetFullOutputName() + ".mplt");
}

std::unordered_set<std::string> Dex2MplCompiler::GetFinalOutputs(const MplOptions &,
                                                                 const Action &action) const {
  auto finalOutputs = std::unordered_set<std::string>();
  (void)finalOutputs.insert(action.GetFullOutputName() + ".mpl");
  (void)finalOutputs.insert(action.GetFullOutputName() + ".mplt");
  return finalOutputs;
}

#ifdef INTERGRATE_DRIVER
void Dex2MplCompiler::PostDex2Mpl(std::unique_ptr<MIRModule> &theModule) const {
  // for each function
  for (auto *func : theModule->GetFunctionList()) {
    if (func == nullptr) {
      continue;
    }

    MIRSymbolTable *symTab = func->GetSymTab();
    // for each symbol
    for (size_t i = 0; i != symTab->GetSymbolTableSize(); ++i) {
      MIRSymbol *currSymbol = symTab->GetSymbolFromStIdx(i);
      if (currSymbol == nullptr) {
        continue;
      }
      // (1) replace void ptr with void ref
      if (theModule->IsJavaModule() && currSymbol->GetType() == GlobalTables::GetTypeTable().GetVoidPtr()) {
        MIRType *voidRef = GlobalTables::GetTypeTable().GetOrCreatePointerType(
            *GlobalTables::GetTypeTable().GetVoid(), PTY_ref);
        currSymbol->SetTyIdx(voidRef->GetTypeIndex());
      }
      // (2) replace String ref with String ptr if symbol's name starts with "L_STR"
      if (currSymbol->GetType()->GetKind() == kTypePointer && currSymbol->GetName().find("L_STR") == 0) {
        MIRType *ty = GlobalTables::GetTypeTable().GetTypeFromTyIdx(currSymbol->GetTyIdx());
        auto *ptrTy = static_cast<MIRPtrType *>(ty->CopyMIRTypeNode());
        ASSERT(ptrTy != nullptr, "null ptr check");
        ptrTy->SetPrimType(PTY_ptr);
        TyIdx newTyIdx = GlobalTables::GetTypeTable().GetOrCreateMIRType(ptrTy);
        delete ptrTy;
        currSymbol->SetTyIdx(newTyIdx);
      }
    }

    // (3) reset pregIndex of pregTab if function has body
    if (func->GetBody() != nullptr) {
      uint32 maxPregNo = 0;
      for (uint32 i = 0; i < func->GetFormalCount(); ++i) {
        MIRSymbol *formalSt = func->GetFormal(i);
        if (formalSt->IsPreg()) {
          // no special register appears in the formals
          uint32 pRegNo = static_cast<uint32>(formalSt->GetPreg()->GetPregNo());
          if (pRegNo > maxPregNo) {
            maxPregNo = pRegNo;
          }
        }
      }
      if (func->GetPregTab() == nullptr) {
        continue;
      }
      func->GetPregTab()->SetIndex(maxPregNo + 1);
    }
  }

  // (4) fix unmatched MIRConst type of global symbols
  for (size_t i = 0; i != GlobalTables::GetGsymTable().GetSymbolTableSize(); ++i) {
    MIRSymbol *symbol = GlobalTables::GetGsymTable().GetSymbolFromStidx(i);
    if (symbol == nullptr || !symbol->IsConst()) {
      continue;
    }
    TyIdx stTyIdx = symbol->GetTyIdx();
    if (stTyIdx == 0) {
      continue;
    }
    MIRType *stType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(stTyIdx);
    MIRConst *mirConst = symbol->GetKonst();

    if (mirConst == nullptr || mirConst->GetKind() != kConstInt) {
      continue;
    }
    if (static_cast<MIRIntConst*>(mirConst)->GetValue() != 0) {
      continue;
    }
    MIRType &valueType = mirConst->GetType();
    if (valueType.GetTypeIndex() != stTyIdx) {
      auto *newIntConst = theModule->GetMemPool()->New<MIRIntConst>(0, *stType);
      symbol->SetValue({newIntConst});
    }
  }

  // (5) remove type attr `rcunowned` of local symbol in rclocalunowned function specified by dex2mpl
  for (auto *func : theModule->GetFunctionList()) {
    if (func == nullptr || !func->GetAttr(FUNCATTR_rclocalunowned)) {
      continue;
    }
    MIRSymbolTable *symTab = func->GetSymTab();
    for (size_t i = 0; i != symTab->GetSymbolTableSize(); ++i) {
      MIRSymbol *symbol = symTab->GetSymbolFromStIdx(i);
      if (symbol == nullptr) {
        continue;
      }
      if (symbol->GetAttr(ATTR_rcunowned)) {
        symbol->ResetAttr(ATTR_rcunowned);
      }
    }
  }

  // 1: MIRStructType::isImported has different meaning for dex2mpl and binary mplt importer.
  // for dex2mpl, `isImported` means whether a type is imported from mplt file instead of dex file, so all types from
  // mplt are marked imported. But for binary mplt importer, `isImported` means whether a type is loaded successfully,
  // so only complete types are marked imported.
  // The workaround is to reset `isImported` according to the completeness of a type.
  for (MIRType *type : GlobalTables::GetTypeTable().GetTypeTable()) {
    if (type == nullptr) {
      continue;
    }
    MIRTypeKind typeKind = type->GetKind();
    if (typeKind == kTypeStructIncomplete || typeKind == kTypeClassIncomplete || typeKind == kTypeInterfaceIncomplete) {
      auto *structType = static_cast<MIRStructType*>(type);
      structType->SetIsImported(false);
    } else if (typeKind == kTypeClass || typeKind == kTypeInterface) {
      auto *structType = static_cast<MIRStructType*>(type);
      structType->SetIsImported(true);
    }
  }
}
#endif

void Dex2MplCompiler::PrintCommand(const MplOptions &options, const Action &action) const {
  std::string runStr = "--run=";
  std::string optionStr = "--option=\"";
  std::string connectSym = "";
  if (options.GetExeOptions().find(kBinNameDex2mpl) != options.GetExeOptions().end()) {
    runStr += "dex2mpl";
    auto inputDex2mplOptions = options.GetExeOptions().find(kBinNameDex2mpl);
    for (auto &opt : inputDex2mplOptions->second) {
      connectSym = opt.Args() != "" ? "=" : "";
      optionStr += " --" + opt.OptionKey() + connectSym + opt.Args();
    }
  }
  optionStr += "\"";
  LogInfo::MapleLogger() << "Starting:" << options.GetExeFolder() << "maple " << runStr << " " << optionStr
                         << " --infile " << GetInputFileName(options, action) << '\n';
}

#ifdef INTERGRATE_DRIVER
bool Dex2MplCompiler::MakeDex2mplOptions(const MplOptions &options) {
  Dex2mplOptions &dex2mplOptions = Dex2mplOptions::GetInstance();
  dex2mplOptions.LoadDefault();
  auto it = options.GetExeOptions().find(kBinNameDex2mpl);
  if (it == options.GetExeOptions().end()) {
    LogInfo::MapleLogger() << "no dex2mpl input options\n";
    return false;
  }
  bool result = dex2mplOptions.SolveOptions(it->second, options.HasSetDebugFlag());
  if (result == false) {
    LogInfo::MapleLogger() << "Meet error dex2mpl options\n";
    return false;
  }
  return true;
}

ErrorCode Dex2MplCompiler::Compile(MplOptions &options, const Action &action,
                                   std::unique_ptr<MIRModule> &theModule) {
  Dex2mplOptions &dex2mplOptions = Dex2mplOptions::GetInstance();
  bool result = MakeDex2mplOptions(options);
  if (!result) {
    return ErrorCode::kErrorCompileFail;
  }
  // .dex
  std::string dexFileName = action.GetInputFile();
  theModule = std::make_unique<MIRModule>(dexFileName);

  const auto &runningExes = options.GetRunningExes();
  bool isDex2mplFinalExe = (runningExes[runningExes.size() - 1] == kBinNameDex2mpl);
  std::unique_ptr<Dex2mplRunner> dex2mpl = std::make_unique<Dex2mplRunner>(dexFileName, dex2mplOptions,
      std::move(theModule), options.HasSetSaveTmps(), isDex2mplFinalExe);
  if (dex2mpl == nullptr) {
    ERR(kLncErr, "new Dex2mplRunner failed.");
    return ErrorCode::kErrorCompileFail;
  }
  LogInfo::MapleLogger() << "Starting dex2mpl" << '\n';
  int ret = dex2mpl->Init();
  if (ret != 0) {
    return ErrorCode::kErrorCompileFail;
  }
  ret = dex2mpl->Run();
  if (ret != 0) {
    ERR(kLncErr, "(ToIDEUser)dex2mpl failed.");
    return ErrorCode::kErrorCompileFail;
  }
  // Check that whether the kBinNameDex2mpl is the final compiler
  // If not, we need to call PostDex2Mpl() to deal with some differences in theModule to
  // adapt to the needs of maplecomb
  if (runningExes[runningExes.size() - 1] != kBinNameDex2mpl) {
    dex2mpl->MoveMirModule(theModule);
    PostDex2Mpl(theModule);
  }
  PrintCommand(options, action);
  return ErrorCode::kErrorNoError;
}
#endif
}  // namespace maple
