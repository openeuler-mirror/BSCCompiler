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
#include "driver_runner.h"
#include <iostream>
#include <dirent.h>
#include "compiler.h"
#include "mpl_timer.h"
#include "mir_function.h"
#include "mir_parser.h"
#include "file_utils.h"
#include "constantfold.h"
#include "lower.h"
#include "me_phase_manager.h"
#include "ipa_phase_manager.h"
#include "lfo_loop_vec.h"
#include "seqvec.h"

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
    if ((pointer) != nullptr) { \
      delete (pointer);         \
      (pointer) = nullptr;      \
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
  (void)outputFile.append(GetPostfix());
  if (mpl2mplOptions != nullptr || meOptions != nullptr) {
    std::string vtableImplFile = originBaseName;
    std::string postFix = "";
    if (theModule->GetSrcLang() == kSrcLangC) {
      postFix = ".me";
    } else {
      postFix = ".VtableImpl";
    }
    (void)vtableImplFile.append(postFix + ".mpl");
    (void)originBaseName.append(postFix);
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

void DriverRunner::SolveCrossModuleInJava(MIRParser &parser) const {
  if (MeOption::optLevel < kLevelO2 || Options::lazyBinding ||
      Options::skipPhase == "inline" || Options::buildApp != 0 ||
      !Options::useInline || !Options::importInlineMplt) {
    return;
  }
  std::string originBaseName = baseName;
  // read in optimized mpl routines
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

static std::string GetSingleFileName(const std::string input) {
  size_t pos = input.find_last_of('/');
  if (pos == std::string::npos) {
    return input;
  }
  return input.substr(pos + 1);
}

static void GetFiles(const std::string &path, std::set<std::string> &fileSet) {
  auto dir = opendir(path.c_str());
  if (!dir) {
    LogInfo::MapleLogger(kLlErr) << "Error: Cannot open inline mplt dir " << Options::inlineMpltDir << '\n';
    CHECK_FATAL_FALSE("open dir failed!");
  }
  auto file = readdir(dir);
  while (file) {
    if (strcmp(file->d_name, ".") == 0 || strcmp(file->d_name, "..") == 0) {
      file = readdir(dir);
      continue;
    }

    auto fileName = path + kFileSeperatorChar + file->d_name;

    if (file->d_type == DT_DIR) {
      GetFiles(fileName, fileSet);
    }

    std::string keyWords = ".mplt_inline";
    if (file->d_type == DT_REG && fileName.substr(fileName.size() - keyWords.size()) == keyWords) {
      (void)fileSet.emplace(fileName);
    }

    file = readdir(dir);
  }
  if (closedir(dir) != 0) {
    CHECK_FATAL_FALSE("close dir failed!");
  }
}

static std::string GetPureName(const std::string &pathName) {
  size_t pos = pathName.find_last_of('/');
  std::string name = "";
  if (pos != std::string::npos) {
    name = pathName.substr(pos + 1);
  } else {
    name = pathName;
  }
  size_t posDot = name.find_last_of('.');
  if (posDot != std::string::npos) {
    return name.substr(0, posDot);
  } else {
    return name;
  }
}

void DriverRunner::SolveCrossModuleInC(MIRParser &parser) const {
  if (MeOption::optLevel < kLevelO2 || !Options::useInline ||
      !Options::importInlineMplt || Options::skipPhase == "inline" ||
      Options::inlineMpltDir.empty()) {
    return;
  }

  // find all inline mplt files from dir
  std::set<std::string> files;
  GetFiles(Options::inlineMpltDir, files);
  std::string curPath = FileUtils::GetCurDirPath();
  auto identPart = FileUtils::GetFileNameHashStr(curPath + kFileSeperatorChar + GetPureName(theModule->GetFileName()));

  LogInfo::MapleLogger() << "[CROSS_MODULE] read inline mplt files from: " << Options::inlineMpltDir << '\n';
  for (auto file : files) {
    TrimString(file);
    // skip the mplt_inline file of this mirmodule to avoid duplicate definition.
    if (file.empty() || GetSingleFileName(file).find(identPart) != std::string::npos) {
      continue;
    }
    std::ifstream optFile(file);
    if (!optFile.is_open()) {
      abort();
    }
    LogInfo::MapleLogger() << "Starting parse " << file << '\n';
    bool parsed = parser.ParseInlineFuncBody(optFile);
    if (!parsed) {
      parser.EmitError(actualInput);
    }
    optFile.close();
  }
}

ErrorCode DriverRunner::ParseInput() const {
  CHECK_MODULE(kErrorExit);
  if (opts::debug) {
    LogInfo::MapleLogger() << "Starting parse input" << '\n';
  }
  MPLTimer timer;
  timer.Start();
  MIRParser parser(*theModule);
  ErrorCode ret = kErrorNoError;
  if (!fileParsed) {
    if (inputFileType != InputFileType::kFileTypeBpl &&
        inputFileType != InputFileType::kFileTypeMbc &&
        inputFileType != InputFileType::kFileTypeLmbc) {
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
  if (CLANG) {
    SolveCrossModuleInC(parser);
  }
  timer.Stop();
  if (opts::debug) {
    LogInfo::MapleLogger() << "Parse consumed " << timer.Elapsed() << "s" << '\n';
  }
  return ret;
}

ErrorCode DriverRunner::ParseSrcLang(MIRSrcLang &srcLang) const {
  ErrorCode ret = kErrorNoError;
  if (inputFileType != InputFileType::kFileTypeBpl &&
      inputFileType != InputFileType::kFileTypeMbc &&
      inputFileType != InputFileType::kFileTypeLmbc) {
    MIRParser parser(*theModule);
    bool parsed = parser.ParseSrcLang(srcLang);
    if (!parsed) {
      ret = kErrorExit;
      parser.EmitError(actualInput);
    }
  } else {
    BinaryMplImport binMplt(*theModule);
    std::string modid = theModule->GetFileName();
    bool imported = binMplt.ImportForSrcLang(modid, srcLang);
    if (!imported) {
      ret = kErrorExit;
      LogInfo::MapleLogger() << "Cannot open .bpl file: %s" << modid << '\n';
    }
  }
  return ret;
}

void DriverRunner::RunNewPM(const std::string &output, const std::string &vtableImplFile) {
  if (opts::debug) {
    LogInfo::MapleLogger() << "Processing maplecomb in new phasemanager" << '\n';
  }
  auto pmMemPool = std::make_unique<ThreadLocalMemPool>(memPoolCtrler, "PM module mempool");
  const MaplePhaseInfo *curPhase = MaplePhaseRegister::GetMaplePhaseRegister()->GetPhaseByID(&MEBETopLevelManager::id);
  auto *topLevelPhaseManager = static_cast<MEBETopLevelManager*>(curPhase->GetConstructor()(pmMemPool.get()));
  topLevelPhaseManager->SetRunMpl2Mpl(mpl2mplOptions != nullptr);
  topLevelPhaseManager->SetRunMe(meOptions != nullptr);
  topLevelPhaseManager->SetQuiet(Options::quiet);
  if (timePhases) {
    topLevelPhaseManager->InitTimeHandler();
  }
  MeFuncPM::genMeMpl = genMeMpl;
  MeFuncPM::genMapleBC = genMapleBC;
  MeFuncPM::genLMBC = genLMBC;
  MeFuncPM::timePhases = timePhases;
  IpaSccPM::timePhases = timePhases;
  MPLTimer timer;
  timer.Start();
  topLevelPhaseManager->DoPhasesPopulate(*theModule);
  topLevelPhaseManager->Run(*theModule);
  if (timePhases) {
    LogInfo::MapleLogger() << "================== TopLevelPM ==================";
    topLevelPhaseManager->DumpPhaseTime();
  }
  // emit after module phase
  if (printOutExe == kMpl2mpl || printOutExe == kMplMe) {
    theModule->Emit(output);
  } else if (genVtableImpl || Options::emitVtableImpl) {
    theModule->Emit(vtableImplFile);
  }
  pmMemPool.reset();
  timer.Stop();
  if (opts::debug) {
    LogInfo::MapleLogger() << "maplecomb consumed " << timer.Elapsed() << "s" << '\n';
    // dump vectorized loop counter here
    {
      LogInfo::MapleLogger() << "\n" << LoopVectorization::vectorizedLoop << " loop vectorized\n";
      LogInfo::MapleLogger() << "\n" << SeqVectorize::seqVecStores << " sequencestores vectorized\n";
      LogInfo::MapleLogger() << "\n" << LfoUnrollOneLoop::countOfLoopsUnrolled << " loops unrolled\n";
    }
  }
}

void DriverRunner::ProcessMpl2mplAndMePhases(const std::string &output, const std::string &vtableImplFile) {
  CHECK_MODULE();
  theMIRModule = theModule;
  if (mpl2mplOptions != nullptr || meOptions != nullptr) {
    // multi-thread is not supported for now.
    MeOption::threads = 1;
    // main entry of newpm for me&mpl2mpl
    RunNewPM(output, vtableImplFile);
  }
}

void DriverRunner::ProcessCGPhase(const std::string &output, const std::string &originBaseName) {
  CHECK_MODULE();
  theMIRModule = theModule;
  if (cgOptions == nullptr) {
    return;
  }
  if (opts::debug) {
    LogInfo::MapleLogger() << "Processing mplcg in new phaseManager" << '\n';
  }
  MPLTimer timer;
  timer.Start();
  theModule->SetBaseName(originBaseName);
  theModule->SetOutputFileName(output);
  cgOptions->SetDefaultOptions(*theModule);
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
  // reserve static symbol in O0 debugging + temp flag for GDB testsuite
  if (!((opts::o0 || opts::o0.IsEnabledByUser()) && opts::noOptO0)) {
    cgfuncPhaseManager->SweepUnusedStaticSymbol(*theModule);
  }
  if (withDwarf && !theModule->IsWithDbgInfo()) {
    theMIRModule->GetDbgInfo()->BuildDebugInfo();
#if defined(DEBUG) && DEBUG
    if (cgOptions) {
      cgOptions->SetOption(CGOptions::kVerboseAsm);
    }
#endif
  }
  (void) cgfuncPhaseManager->PhaseRun(*theModule);
  if (timePhases) {
    LogInfo::MapleLogger() << "==================  CGFuncPM  ==================";
    cgfuncPhaseManager->DumpPhaseTime();
  }
  timer.Stop();
  if (theMIRModule->GetDbgInfo() != nullptr) {
    theMIRModule->GetDbgInfo()->ClearDebugInfo();
  }
  theMIRModule->ReleasePragmaMemPool();
  if (opts::debug) {
    LogInfo::MapleLogger() << "Mplcg consumed " << timer.ElapsedMilliseconds() << "ms" << '\n';
  }
  Globals::GetInstance()->ClearMAD();
}

void DriverRunner::InitProfile() const {
  if (!cgOptions->IsProfileDataEmpty()) {
    uint32 dexNameIdx = theModule->GetFileinfo(GlobalTables::GetStrTable().GetOrCreateStrIdxFromName("INFO_filename"));
    const std::string &dexName = GlobalTables::GetStrTable().GetStringFromStrIdx(GStrIdx(dexNameIdx));
    bool deCompressSucc = theModule->GetProfile().DeCompress(CGOptions::GetProfileData(), dexName);
    if (!deCompressSucc) {
      LogInfo::MapleLogger() << "WARN: DeCompress() " << CGOptions::GetProfileData() << "failed in mplcg()\n";
    }
  }
}
}  // namespace maple
