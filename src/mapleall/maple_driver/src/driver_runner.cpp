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
#include "me_phase_manager.h"
#include "lfo_loop_vec.h"
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
      bool parsed = parser.ParseMIR(0, 0, false, true);
      if (!parsed) {
        ret = kErrorExit;
        parser.EmitError(actualInput);
      }
    } else {
      BinaryMplImport binMplt(*theModule);
      binMplt.SetImported(false);
      std::string modid = theModule->GetFileName();
      bool imported = binMplt.Import(modid, true);
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

void DriverRunner::RunNewPM(const std::string &outputFile, const std::string &vtableImplFile) {
  LogInfo::MapleLogger() << "Processing maplecomb in new phasemanager" << '\n';
  auto PMMemPool = std::make_unique<ThreadLocalMemPool>(memPoolCtrler, "PM module mempool");
  const MaplePhaseInfo *curPhase = MaplePhaseRegister::GetMaplePhaseRegister()->GetPhaseByID(&MEBETopLevelManager::id);
  auto *topLevelPhaseManager = static_cast<MEBETopLevelManager*>(curPhase->GetConstructor()(PMMemPool.get()));
  topLevelPhaseManager->SetRunMpl2Mpl(mpl2mplOptions != nullptr);
  topLevelPhaseManager->SetRunMe(meOptions != nullptr);
  topLevelPhaseManager->SetQuiet(Options::quiet);
  if (timePhases) {
    topLevelPhaseManager->InitTimeHandler();
  }
  MeFuncPM::genMeMpl = genMeMpl;
  MeFuncPM::timePhases = timePhases;
  MPLTimer timer;
  timer.Start();
  topLevelPhaseManager->DoPhasesPopulate(*theModule);
  topLevelPhaseManager->Run(*theModule);
  if (timePhases) {
    topLevelPhaseManager->DumpPhaseTime();
  }
  // emit after module phase
  if (printOutExe == kMpl2mpl || printOutExe == kMplMe) {
    theModule->Emit(outputFile);
  } else if (genVtableImpl || Options::emitVtableImpl) {
    theModule->Emit(vtableImplFile);
  }
  PMMemPool.reset();
  timer.Stop();
  LogInfo::MapleLogger() << "maplecomb consumed " << timer.Elapsed() << "s" << '\n';
  // dump vectorized loop counter here
  {
    LogInfo::MapleLogger() << "\n" << LoopVectorization::vectorizedLoop << " loop vectorized\n";
  }
}

void DriverRunner::ProcessMpl2mplAndMePhases(const std::string &outputFile, const std::string &vtableImplFile) {
  CHECK_MODULE();
  theMIRModule = theModule;
  if (withDwarf && !theModule->IsWithDbgInfo()) {
    std::cout << "set up debug info " << std::endl;
    theMIRModule->GetDbgInfo()->BuildDebugInfo();
  }
  if (mpl2mplOptions != nullptr || meOptions != nullptr) {
    // multi-thread is not supported for now.
    MeOption::threads = 1;
    // main entry of newpm for me&mpl2mpl
    RunNewPM(outputFile, vtableImplFile);
    return;
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
  auto *cgfuncPhaseManager = static_cast<CgFuncPM*>(cgPMInfo->GetConstructor()(cgPhaseManager.get()));
  cgfuncPhaseManager->SetQuiet(CGOptions::IsQuiet());
  if (timePhases) {
    cgfuncPhaseManager->InitTimeHandler();
  }
  /* It is a specifc work around  (need refactor) */
  cgfuncPhaseManager->SetCGOptions(cgOptions);
  (void) cgfuncPhaseManager->PhaseRun(*theModule);
  if (timePhases) {
    cgfuncPhaseManager->DumpPhaseTime();
  }
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
