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
#include "compiler.h"
#include "driver_runner.h"
#include <iostream>
#include <sys/stat.h>
#include "mpl_timer.h"
#include "mir_function.h"
#include "mir_parser.h"
#include "file_utils.h"
#include "constantfold.h"
#include "lower.h"
#if TARGAARCH64 || TARGRISCV64
#include "aarch64/aarch64_emitter.h"
#elif TARGARM32
#include "arm32/arm32_cg.h"
#include "arm32/arm32_emitter.h"
#else
#error "Unsupported target"
#endif

using namespace maplebe;

#define JAVALANG (theModule->IsJavaModule())
#define CLANG (theModule->IsCModule())

#define CHECK_MODULE(errorCode...)                                              \
  do {                                                                          \
    if (theModule == nullptr) {                                                 \
      LogInfo::MapleLogger() << "Fatal error: the module is null" << '\n';      \
      return errorCode;                                                         \
    }                                                                           \
  } while (0)

#define RELEASE(pointer)      \
  do {                        \
    if (pointer != nullptr) { \
      delete pointer;         \
      pointer = nullptr;      \
    }                         \
  } while (0)

#define ADD_PHASE(name, condition)       \
  if ((condition)) {                     \
    phases.push_back(std::string(name)); \
  }

#define ADD_EXTRA_PHASE(name, timephases, timeStart)                                                       \
  if (timephases) {                                                                                        \
    auto duration = std::chrono::system_clock::now() - (timeStart);                                        \
    extraPhasesTime.emplace_back(std::chrono::duration_cast<std::chrono::microseconds>(duration).count()); \
    extraPhasesName.emplace_back(name);                                                                    \
  }

namespace maple {
const std::string kMplCg = "mplcg";
const std::string kMpl2mpl = "mpl2mpl";
const std::string kMplMe = "me";

enum OptLevel {
  kLevelO0,
  kLevelO1,
  kLevelO2
};

ErrorCode DriverRunner::Run() {
  CHECK_MODULE(kErrorExit);

  if (exeNames.empty()) {
    LogInfo::MapleLogger() << "Fatal error: no exe specified" << '\n';
    return kErrorExit;
  }
  std::string originBaseName = baseName;
  outputFile = baseName;
  outputFile.append(GetPostfix());
  if (mpl2mplOptions != nullptr || meOptions != nullptr) {
    std::string vtableImplFile = originBaseName;
    std::string postFix = "";
    if (theModule->GetSrcLang() == kSrcLangC) {
      postFix = ".me";
    } else {
      postFix = ".VtableImpl";
    }
    vtableImplFile.append(postFix + ".mpl");
    originBaseName.append(postFix);
    ProcessMpl2mplAndMePhases(outputFile, vtableImplFile);
  }
  return kErrorNoError;
}

std::string DriverRunner::GetPostfix() {
  if (printOutExe == kMplMe) {
    return ".me.mpl";
  }
  if (printOutExe == kMpl2mpl) {
    return ".VtableImpl.mpl";
  }
  if (printOutExe == kMplCg) {
    if (theModule->GetSrcLang() == kSrcLangC) {
      return ".s";
    } else {
      return ".VtableImpl.s";
    }
  }
  return "";
}

ErrorCode DriverRunner::ParseInput() const {
  CHECK_MODULE(kErrorExit);

  std::string originBaseName = baseName;
  LogInfo::MapleLogger() << "Starting parse input" << '\n';
  MPLTimer timer;
  timer.Start();
  MIRParser parser(*theModule);
  ErrorCode ret = kErrorNoError;
  if (!fileParsed) {
    if (inputFileType != kFileTypeBpl) {
      MPLTimer parseMirTimer;
      parseMirTimer.Start();
      bool parsed = parser.ParseMIR(0, 0, false, true);
      parseMirTimer.Stop();
      InterleavedManager::interleavedTimer.emplace_back(
          std::pair<std::string, time_t>("parseMpl", parseMirTimer.ElapsedMicroseconds()));
      if (!parsed) {
        ret = kErrorExit;
        parser.EmitError(actualInput);
      }
    } else {
      BinaryMplImport binMplt(*theModule);
      binMplt.SetImported(false);
      std::string modid = theModule->GetFileName();
      MPLTimer importMirTimer;
      importMirTimer.Start();
      bool imported = binMplt.Import(modid, true);
      importMirTimer.Stop();
      InterleavedManager::interleavedTimer.emplace_back(
          std::pair<std::string, time_t>("parseMpl", importMirTimer.ElapsedMicroseconds()));
      if (!imported) {
        ret = kErrorExit;
        LogInfo::MapleLogger() << "Cannot open .bpl file: %s" << modid << '\n';
      }
    }
  }
  // read in optimized mpl routines
  if (MeOption::optLevel == kLevelO2 && !Options::lazyBinding &&
      Options::skipPhase != "inline" && Options::buildApp == 0 &&
      Options::useInline && Options::useCrossModuleInline && JAVALANG) {
    const MapleVector<std::string> &inputMplt = theModule->GetImportedMplt();
    auto it = inputMplt.cbegin();
    for (++it; it != inputMplt.cend(); ++it) {
      const std::string &curStr = *it;
      auto lastDotInner = curStr.find_last_of(".");
      std::string tmp = (lastDotInner == std::string::npos) ? curStr : curStr.substr(0, lastDotInner);
      if (tmp.find("framework") != std::string::npos && originBaseName.find("framework") != std::string::npos) {
        continue;
      }
      // Skip the import file
      if (tmp.find(FileUtils::GetFileName(originBaseName, true)) != std::string::npos) {
        continue;
      }
      size_t index = curStr.rfind(".");
      CHECK_FATAL(index != std::string::npos, "can not find .");

      std::string inputInline = curStr.substr(0, index + 1) + "mplt_inline";
      std::ifstream optFile(inputInline);
      if (!optFile.is_open()) {
        continue;
      }

      LogInfo::MapleLogger() << "Starting parse " << inputInline << '\n';
      bool parsed = parser.ParseInlineFuncBody(optFile);
      if (!parsed) {
        parser.EmitError(actualInput);
      }
      optFile.close();
    }
  }
  timer.Stop();
  LogInfo::MapleLogger() << "Parse consumed " << timer.Elapsed() << "s" << '\n';
  return ret;
}

#ifdef NEW_PM
void DriverRunner::RunNewPM(const std::string &outputFile, const std::string &vtableImplFile) {
  auto PMMemPool = std::make_unique<ThreadLocalMemPool>(memPoolCtrler, "PM module mempool");
  const MaplePhaseInfo *curPhase = MaplePhaseRegister::GetMaplePhaseRegister()->GetPhaseByID(&MEBETopLevelManager::id);
  auto *topLevelPhaseManager = static_cast<MEBETopLevelManager*>(curPhase->GetConstructor()(PMMemPool.get()));
  MPLTimer timer;
  timer.Start();
  topLevelPhaseManager->Run(*theModule);

  // emit after module phase
  if (printOutExe == kMpl2mpl || printOutExe == kMplMe) {
    theModule->Emit(outputFile);
  } else if (genVtableImpl || Options::emitVtableImpl) {
    theModule->Emit(vtableImplFile);
  }
  PMMemPool.reset();
  timer.Stop();
  LogInfo::MapleLogger() << " Mpl2mpl&mplme consumed " << timer.Elapsed() << "s" << '\n';
}
#endif

void DriverRunner::ProcessMpl2mplAndMePhases(const std::string &outputFile, const std::string &vtableImplFile) {
  CHECK_MODULE();
  theMIRModule = theModule;
  if (withDwarf && !theModule->IsWithDbgInfo()) {
    std::cout << "set up debug info " << std::endl;
    theMIRModule->GetDbgInfo()->BuildDebugInfo();
  }
  if (mpl2mplOptions != nullptr || meOptions != nullptr) {
#ifdef NEW_PM
    if (MeOption::threads <= 1) {
      // main entry of newpm for me&mpl2mpl
      RunNewPM(outputFile, vtableImplFile);
      return;
    }
#endif
    LogInfo::MapleLogger() << "Processing maplecomb" << '\n';

    InterleavedManager mgr(optMp, theModule, meInput, timePhases);
    std::vector<std::string> phases;
#include "phases.def"
    InitPhases(mgr, phases);
    MPLTimer timer;
    timer.Start();
    mgr.Run();
    MPLTimer emitVtableMplTimer;
    emitVtableMplTimer.Start();
    // emit after module phase
    if (printOutExe == kMpl2mpl || printOutExe == kMplMe) {
      theModule->Emit(outputFile);
    } else if (genVtableImpl || Options::emitVtableImpl) {
      theModule->Emit(vtableImplFile);
    }
    emitVtableMplTimer.Stop();
    mgr.SetEmitVtableImplTime(emitVtableMplTimer.ElapsedMicroseconds());
    timer.Stop();
    LogInfo::MapleLogger() << " maplecomb consumed " << timer.Elapsed() << "s" << '\n';
  }
}

void DriverRunner::InitPhases(InterleavedManager &mgr, const std::vector<std::string> &phases) const {
  if (phases.empty()) {
    return;
  }

  const PhaseManager *curManager = nullptr;
  std::vector<std::string> curPhases;

  for (const std::string &phase : phases) {
    const PhaseManager *supportManager = mgr.GetSupportPhaseManager(phase);
    if (supportManager != nullptr) {
      if (curManager != nullptr && curManager != supportManager && !curPhases.empty()) {
        AddPhases(mgr, curPhases, *curManager);
        curPhases.clear();
      }

      if (curManager != supportManager) {
        curManager = supportManager;
      }
      AddPhase(curPhases, phase, *supportManager);
    }
  }

  if (curManager != nullptr && !curPhases.empty()) {
    AddPhases(mgr, curPhases, *curManager);
  }
}

void DriverRunner::AddPhases(InterleavedManager &mgr, const std::vector<std::string> &phases,
                             const PhaseManager &phaseManager) const {
  const auto &type = typeid(phaseManager);
  if (type == typeid(ModulePhaseManager)) {
    mgr.AddPhases(phases, true, timePhases);
  } else if (type == typeid(MeFuncPhaseManager)) {
    mgr.AddPhases(phases, false, timePhases, genMeMpl);
  } else {
    CHECK_FATAL(false, "Should not reach here, phases should be handled");
  }
}

void DriverRunner::AddPhase(std::vector<std::string> &phases, const std::string phase,
                            const PhaseManager &phaseManager) const {
  if (typeid(phaseManager) == typeid(ModulePhaseManager)) {
    if (mpl2mplOptions && Options::skipPhase.compare(phase) != 0) {
      phases.push_back(phase);
    }
  } else if (typeid(phaseManager) == typeid(MeFuncPhaseManager)) {
    if (meOptions && meOptions->GetSkipPhases().find(phase) == meOptions->GetSkipPhases().end()) {
      phases.push_back(phase);
    }
  } else {
    CHECK_FATAL(false, "Should not reach here, phase should be handled");
  }
}

void DriverRunner::ProcessCGPhase(const std::string &outputFile, const std::string &originBaseName) {
  CHECK_MODULE();
  theMIRModule = theModule;
  if (withDwarf && !theModule->IsWithDbgInfo()) {
    LogInfo::MapleLogger() << "set up debug info " << '\n';
    theMIRModule->GetDbgInfo()->BuildDebugInfo();
  }
  if (cgOptions == nullptr) {
    return;
  }
  LogInfo::MapleLogger() << "Processing mplcg in new phaseManager" << '\n';
  MPLTimer timer;
  timer.Start();
  theModule->SetBaseName(originBaseName);
  theModule->SetOutputFileName(outputFile);
  cgOptions->SetDefaultOptions(*theModule);
  if (timePhases) {
    CGOptions::EnableTimePhases();
  }
  Globals::GetInstance()->SetOptimLevel(cgOptions->GetOptimizeLevel());
  MAD mad;
  Globals::GetInstance()->SetMAD(mad);

  auto cgPhaseManager = std::make_unique<ThreadLocalMemPool>(memPoolCtrler, "cg function phasemanager");
  const MaplePhaseInfo *cgPMInfo = MaplePhaseRegister::GetMaplePhaseRegister()->GetPhaseByID(&CgFuncPM::id);
  auto *cgfuncNewPhaseManager = static_cast<CgFuncPM*>(cgPMInfo->GetConstructor()(cgPhaseManager.get()));
  /* It is a specifc work around  (need refactor) */
  cgfuncNewPhaseManager->SetCGOptions(cgOptions);
  (void) cgfuncNewPhaseManager->PhaseRun(*theModule);
  timer.Stop();
  LogInfo::MapleLogger() << "Mplcg consumed " << timer.ElapsedMilliseconds() << "ms" << '\n';
}

void DriverRunner::InitProfile() const {
  if (!cgOptions->IsProfileDataEmpty()) {
    uint32 dexNameIdx = theModule->GetFileinfo(GlobalTables::GetStrTable().GetOrCreateStrIdxFromName("INFO_filename"));
    const std::string &dexName = GlobalTables::GetStrTable().GetStringFromStrIdx(GStrIdx(dexNameIdx));
    bool deCompressSucc = theModule->GetProfile().DeCompress(cgOptions->GetProfileData(), dexName);
    if (!deCompressSucc) {
      LogInfo::MapleLogger() << "WARN: DeCompress() " << cgOptions->GetProfileData() << "failed in mplcg()\n";
    }
  }
}
}  // namespace maple
