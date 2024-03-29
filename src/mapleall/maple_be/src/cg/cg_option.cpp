/*
 * Copyright (c) [2020-2022] Huawei Technologies Co.,Ltd.All rights reserved.
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
#include "cg_option.h"
#include <fstream>
#include <unordered_map>
#include "cg_options.h"
#include "driver_options.h"
#include "mpl_logging.h"
#include "parser_opt.h"
#include "mir_parser.h"
#include "string_utils.h"
#include "triple.h"

namespace maplebe {
using namespace maple;

const std::string kMplcgVersion = "";

std::string CGOptions::targetArch = "";
std::unordered_set<std::string> CGOptions::dumpPhases = {};
std::unordered_set<std::string> CGOptions::skipPhases = {};
std::unordered_map<std::string, std::vector<std::string>> CGOptions::cyclePatternMap = {};
std::string CGOptions::skipFrom = "";
std::string CGOptions::skipAfter = "";
std::string CGOptions::dumpFunc = "*";
std::string CGOptions::globalVarProfile = "";
std::string CGOptions::profileData = "";
std::string CGOptions::profileFuncData = "";
std::string CGOptions::profileClassData = "";
#ifdef TARGARM32
std::string CGOptions::duplicateAsmFile = "";
#else
std::string CGOptions::duplicateAsmFile = "maple/mrt/codetricks/arch/arm64/duplicateFunc.s";
#endif
Range CGOptions::range = Range();
std::string CGOptions::fastFuncsAsmFile = "";
Range CGOptions::spillRanges = Range();
uint8 CGOptions::fastAllocMode = 0;  /* 0: fast, 1: spill all */
bool CGOptions::fastAlloc = false;
uint64 CGOptions::lsraBBOptSize = 150000;
uint64 CGOptions::lsraInsnOptSize = 200000;
uint64 CGOptions::overlapNum = 28;
uint8 CGOptions::rematLevel = 2;
bool CGOptions::optForSize = false;
bool CGOptions::enableHotColdSplit = false;
uint32 CGOptions::alignMinBBSize = 16;
uint32 CGOptions::alignMaxBBSize = 96;
uint32 CGOptions::loopAlignPow = 4;
uint32 CGOptions::jumpAlignPow = 5;
uint32 CGOptions::funcAlignPow = 5;
uint32 CGOptions::coldPathThreshold = 10;
bool CGOptions::liteProfGen = false;
bool CGOptions::liteProfUse = false;
bool CGOptions::liteProfVerify = false;
std::string CGOptions::liteProfile = "";
std::string CGOptions::litePgoWhiteList = "";
std::string CGOptions::instrumentationOutPutPath = "";
std::string CGOptions::litePgoOutputFunction = "";
std::string CGOptions::functionProrityFile = "";
std::string CGOptions::functionReorderAlgorithm = "";
std::string CGOptions::functionReorderProfile = "";
std::string CGOptions::cpu = "cortex-a53";
bool CGOptions::doOptimizedFrameLayout = true;
bool CGOptions::doPgoCodeAlign = false;
// Select fraction of the maximal frequency of executions of basic block in function given basic block get alignment.
uint32 CGOptions::alignThreshold = 100;
// Loops iterating at least selected number of iterations will get loop alignment.
uint32 CGOptions::alignLoopIterations = 4;
// percentage of frequency of first bb, if the freq of edge to retbb >= dupFreqThreshold, retbb will be duplicated.
uint32 CGOptions::dupFreqThreshold = 100;

#if TARGAARCH64 || TARGRISCV64
bool CGOptions::useBarriersForVolatile = false;
#else
bool CGOptions::useBarriersForVolatile = true;
#endif
bool CGOptions::exclusiveEH = false;
bool CGOptions::doEBO = false;
bool CGOptions::doCGSSA = false;
bool CGOptions::doLayoutColdPath = false;
bool CGOptions::doGlobalSchedule = false;
bool CGOptions::doLocalSchedule = false;
bool CGOptions::doVerifySchedule = false;
bool CGOptions::calleeEnsureParam = true;
bool CGOptions::doIPARA = true;
bool CGOptions::doCFGO = false;
bool CGOptions::doICO = false;
bool CGOptions::doStoreLoadOpt = false;
bool CGOptions::doGlobalOpt = false;
bool CGOptions::doVregRename = false;
bool CGOptions::doMultiPassColorRA = true;
bool CGOptions::doPrePeephole = false;
bool CGOptions::doPeephole = false;
bool CGOptions::doPostRASink = false;
bool CGOptions::doRetMerge = false;
bool CGOptions::doSchedule = false;
bool CGOptions::doWriteRefFieldOpt = false;
bool CGOptions::dumpOptimizeCommonLog = false;
bool CGOptions::checkArrayStore = false;
/* Enable large mode for fpic and fpie by default */
CGOptions::PICMode CGOptions::picMode = kLargeMode;
CGOptions::PICMode CGOptions::pieMode = kClose;
bool CGOptions::noSemanticInterposition = false;
bool CGOptions::noDupBB = false;
bool CGOptions::noCalleeCFI = true;
bool CGOptions::emitCyclePattern = false;
bool CGOptions::insertYieldPoint = false;
bool CGOptions::mapleLinker = false;
bool CGOptions::printFunction = false;
bool CGOptions::nativeOpt = false;
bool CGOptions::lazyBinding = false;
bool CGOptions::hotFix = false;
bool CGOptions::debugSched = false;
bool CGOptions::bruteForceSched = false;
bool CGOptions::simulateSched = false;
CGOptions::ABIType CGOptions::abiType = kABIHard;
CGOptions::EmitFileType CGOptions::emitFileType = kAsm;
bool CGOptions::genLongCalls = false;
bool CGOptions::functionSections = false;
bool CGOptions::dataSections = false;
CGOptions::FramePointerType CGOptions::useFramePointer = kNoneFP;
bool CGOptions::gcOnly = false;
bool CGOptions::quiet = false;
bool CGOptions::doPatchLongBranch = false;
bool CGOptions::doPreSchedule = false;
bool CGOptions::emitBlockMarker = true;
bool CGOptions::inRange = false;
bool CGOptions::doPreRAOpt = false;
bool CGOptions::doLocalRefSpill = false;
bool CGOptions::doCalleeToSpill = false;
bool CGOptions::doRegSavesOpt = false;
bool CGOptions::doTailCallOpt = false;
bool CGOptions::useSsaPreSave = false;
bool CGOptions::useSsuPreRestore = false;
bool CGOptions::useNewCg = false;
bool CGOptions::replaceASM = false;
bool CGOptions::generalRegOnly = false;
bool CGOptions::fastMath = false;
bool CGOptions::doAlignAnalysis = false;
bool CGOptions::doCondBrAlign = false;
bool CGOptions::doLoopAlign = false;
bool CGOptions::cgBigEndian = false;
bool CGOptions::arm64ilp32 = false;
bool CGOptions::noCommon = false;
bool CGOptions::flavorLmbc = false;
bool CGOptions::doAggrOpt = false;
CGOptions::VisibilityType CGOptions::visibilityType = kDefaultVisibility;
CGOptions::TLSModel CGOptions::tlsModel = kDefaultTLSModel;
bool CGOptions::noplt = false;
bool CGOptions::doCGMemAlias = false;

CGOptions &CGOptions::GetInstance() {
  static CGOptions instance;
  return instance;
}

void CGOptions::DecideMplcgRealLevel(bool isDebug) {
  if (opts::cg::o0 || opts::o0.IsEnabledByUser()) {
    if (isDebug) {
      LogInfo::MapleLogger() << "Real Mplcg level: O0\n";
    }
    EnableO0();
  }

  if (opts::cg::o1 || opts::o1.IsEnabledByUser()) {
    if (isDebug) {
      LogInfo::MapleLogger() << "Real Mplcg level: O1\n";
    }
    EnableO1();
  }

  if ((opts::cg::o2 || opts::o2.IsEnabledByUser() || opts::o3.IsEnabledByUser()) ||
      (opts::cg::os || opts::os.IsEnabledByUser())) {
    if (opts::cg::os || opts::os.IsEnabledByUser()) {
      optForSize = true;
    }
    if (isDebug) {
      std::string oLog = ((opts::cg::os == true) || opts::os.IsEnabledByUser()) ? "Os" : "O2";
      LogInfo::MapleLogger() << "Real Mplcg level: " << oLog << "\n";
    }
    EnableO2();
  }
  if (opts::cg::os || opts::os.IsEnabledByUser()) {
    DisableAlignAnalysis();
    SetFuncAlignPow(0);
  }
  if (opts::cg::oLitecg) {
    if (isDebug) {
      LogInfo::MapleLogger() << "Real Mplcg level: LiteCG\n";
    }
    EnableLiteCG();
  }
}

bool CGOptions::SolveOptions(bool isDebug) {
  DecideMplcgRealLevel(isDebug);

  for (const auto &opt : cgCategory.GetEnabledOptions()) {
    std::string printOpt;
    if (isDebug) {
      for (const auto &val : opt->GetRawValues()) {
        if (opt->IsEnabledByUser()) {
          printOpt += opt->GetName() + " " + val + " ";
          LogInfo::MapleLogger() << "cg options: " << printOpt << '\n';
        }
      }
    }
  }

  if (opts::cg::quiet.IsEnabledByUser()) {
    SetQuiet(true);
  }

  if (opts::verbose.IsEnabledByUser()) {
    SetQuiet(false);
  }

  if (opts::fpie.IsEnabledByUser() || opts::fPIE.IsEnabledByUser()) {
    if (opts::fPIE && opts::fPIE.IsEnabledByUser()) {
      SetOption(CGOptions::kGenPie);
      SetOption(CGOptions::kGenPic);
    } else if (opts::fpie && opts::fpie.IsEnabledByUser()) {
      SetOption(CGOptions::kGenPie);
      SetOption(CGOptions::kGenPic);
    } else {
      ClearOption(CGOptions::kGenPie);
    }
  }

  if (opts::fpic.IsEnabledByUser() || opts::fPIC.IsEnabledByUser()) {
    /* To avoid fpie mode being modified twice, need to ensure fpie is not opened. */
    if (!opts::fpie && !opts::fpie.IsEnabledByUser() && !opts::fPIE.IsEnabledByUser() && !opts::fPIE) {
      if (opts::fPIC && opts::fPIC.IsEnabledByUser()) {
        SetOption(CGOptions::kGenPic);
        ClearOption(CGOptions::kGenPie);
      } else if (opts::fpic && opts::fpic.IsEnabledByUser()) {
        SetOption(CGOptions::kGenPic);
        ClearOption(CGOptions::kGenPie);
      } else {
        ClearOption(CGOptions::kGenPic);
      }
    }
  }

  if (opts::fNoPlt.IsEnabledByUser()) {
    EnableNoplt();
  }

  if (opts::cg::fnoSemanticInterposition.IsEnabledByUser()) {
    if (opts::cg::fnoSemanticInterposition && IsShlib()) {
      EnableNoSemanticInterposition();
    } else {
      DisableNoSemanticInterposition();
    }
  }

  if (opts::linkerTimeOpt.IsEnabledByUser() && IsShlib()) {
    EnableNoSemanticInterposition();
  }

  if (opts::ftlsModel.IsEnabledByUser()) {
    SetTLSModel(opts::ftlsModel);
  }

  if (opts::cg::verboseAsm.IsEnabledByUser()) {
    opts::cg::verboseAsm ? SetOption(CGOptions::kVerboseAsm) : ClearOption(CGOptions::kVerboseAsm);
  }

  if (opts::cg::verboseCg.IsEnabledByUser()) {
    opts::cg::verboseCg ? SetOption(CGOptions::kVerboseCG) : ClearOption(CGOptions::kVerboseCG);
  }

  if (opts::cg::maplelinker.IsEnabledByUser()) {
    opts::cg::maplelinker ? EnableMapleLinker() : DisableMapleLinker();
  }

  if (opts::cg::fastAlloc.IsEnabledByUser()) {
    EnableFastAlloc();
    SetFastAllocMode(opts::cg::fastAlloc);
  }

  if (opts::cg::useBarriersForVolatile.IsEnabledByUser()) {
    opts::cg::useBarriersForVolatile ? EnableBarriersForVolatile() : DisableBarriersForVolatile();
  }

  if (opts::cg::spillRange.IsEnabledByUser()) {
    SetRange(opts::cg::spillRange, "--pill-range", GetSpillRanges());
  }

  if (opts::cg::range.IsEnabledByUser()) {
    SetRange(opts::cg::range, "--range", GetRange());
  }

  if (opts::cg::dumpFunc.IsEnabledByUser()) {
    SetDumpFunc(opts::cg::dumpFunc);
  }

  if (opts::cg::duplicateAsmList.IsEnabledByUser()) {
    SetDuplicateAsmFile(opts::cg::duplicateAsmList);
  }

  if (opts::cg::duplicateAsmList2.IsEnabledByUser()) {
    SetFastFuncsAsmFile(opts::cg::duplicateAsmList2);
  }

  if (opts::stackProtectorStrong.IsEnabledByUser()) {
    SetOption(kUseStackProtectorStrong);
  }

  if (opts::stackProtectorAll.IsEnabledByUser()) {
    SetOption(kUseStackProtectorAll);
  }

  if (opts::cg::debug.IsEnabledByUser()) {
    SetOption(kDebugFriendly);
    SetOption(kWithLoc);
    ClearOption(kSuppressFileInfo);
  }

  if (opts::cg::gdwarf.IsEnabledByUser()) {
    SetOption(kDebugFriendly);
    SetOption(kWithLoc);
    SetOption(kWithDwarf);
    SetParserOption(kWithDbgInfo);
    ClearOption(kSuppressFileInfo);
  }

  if (opts::cg::gsrc.IsEnabledByUser()) {
    SetOption(kDebugFriendly);
    SetOption(kWithLoc);
    SetOption(kWithSrc);
    ClearOption(kWithMpl);
  }

  if (opts::cg::gmixedsrc.IsEnabledByUser()) {
    SetOption(kDebugFriendly);
    SetOption(kWithLoc);
    SetOption(kWithSrc);
    SetOption(kWithMpl);
  }

  if (opts::cg::gmixedasm.IsEnabledByUser()) {
    SetOption(kDebugFriendly);
    SetOption(kWithLoc);
    SetOption(kWithSrc);
    SetOption(kWithMpl);
    SetOption(kWithAsm);
  }

  if (opts::cg::profile.IsEnabledByUser()) {
    SetOption(kWithProfileCode);
    SetParserOption(kWithProfileInfo);
  }

  if (opts::cg::withRaLinearScan.IsEnabledByUser()) {
    SetOption(kDoLinearScanRegAlloc);
    ClearOption(kDoColorRegAlloc);
  }

  if (opts::cg::withRaGraphColor.IsEnabledByUser()) {
    SetOption(kDoColorRegAlloc);
    ClearOption(kDoLinearScanRegAlloc);
  }

  if (opts::cg::printFunc.IsEnabledByUser()) {
    opts::cg::printFunc ? EnablePrintFunction() : DisablePrintFunction();
  }

  if (opts::cg::addDebugTrace.IsEnabledByUser()) {
    SetOption(kAddDebugTrace);
  }

  if (opts::cg::suppressFileinfo.IsEnabledByUser()) {
    SetOption(kSuppressFileInfo);
  }

  if (opts::cg::patchLongBranch.IsEnabledByUser()) {
    SetOption(kPatchLongBranch);
  }

  if (opts::cg::constFold.IsEnabledByUser()) {
    opts::cg::constFold ? SetOption(kConstFold) : ClearOption(kConstFold);
  }

  if (opts::cg::dumpCfg.IsEnabledByUser()) {
    SetOption(kDumpCFG);
  }

  if (opts::cg::classListFile.IsEnabledByUser()) {
    SetClassListFile(opts::cg::classListFile);
  }

  if (opts::cg::genCMacroDef.IsEnabledByUser()) {
    SetOrClear(GetGenerateFlags(), CGOptions::kCMacroDef, opts::cg::genCMacroDef);
  }

  if (opts::cg::genGctibFile.IsEnabledByUser()) {
    SetOrClear(GetGenerateFlags(), CGOptions::kGctib, opts::cg::genGctibFile);
  }

  if (opts::cg::yieldpoint.IsEnabledByUser()) {
    SetOrClear(GetGenerateFlags(), CGOptions::kGenYieldPoint, opts::cg::yieldpoint);
  }

  if (opts::cg::localRc.IsEnabledByUser()) {
    SetOrClear(GetGenerateFlags(), CGOptions::kGenLocalRc, opts::cg::localRc);
  }

  if (opts::cg::ehExclusiveList.IsEnabledByUser()) {
    SetEHExclusiveFile(opts::cg::ehExclusiveList);
    EnableExclusiveEH();
    ParseExclusiveFunc(opts::cg::ehExclusiveList);
  }

  if (opts::cg::cyclePatternList.IsEnabledByUser()) {
    SetCyclePatternFile(opts::cg::cyclePatternList);
    EnableEmitCyclePattern();
    ParseCyclePattern(opts::cg::cyclePatternList);
  }

  if (opts::cg::cg.IsEnabledByUser()) {
    SetRunCGFlag(opts::cg::cg);
    opts::cg::cg ? SetOption(CGOptions::kDoCg) : ClearOption(CGOptions::kDoCg);
  }

  if (opts::cg::objmap.IsEnabledByUser()) {
    SetGenerateObjectMap(opts::cg::objmap);
  }

  if (opts::cg::replaceAsm.IsEnabledByUser()) {
    opts::cg::replaceAsm ? EnableReplaceASM() : DisableReplaceASM();
  }

  if (opts::cg::generalRegOnly.IsEnabledByUser()) {
    opts::cg::generalRegOnly ? EnableGeneralRegOnly() : DisableGeneralRegOnly();
  }

  if (opts::cg::lazyBinding.IsEnabledByUser()) {
    opts::cg::lazyBinding ? EnableLazyBinding() : DisableLazyBinding();
  }

  if (opts::cg::hotFix.IsEnabledByUser()) {
    opts::cg::hotFix ? EnableHotFix() : DisableHotFix();
  }

  if (opts::cg::soeCheck.IsEnabledByUser()) {
    SetOption(CGOptions::kSoeCheckInsert);
  }

  if (opts::cg::checkArraystore.IsEnabledByUser()) {
    opts::cg::checkArraystore ? EnableCheckArrayStore() : DisableCheckArrayStore();
  }

  if (opts::cg::ebo.IsEnabledByUser()) {
    opts::cg::ebo ? EnableEBO() : DisableEBO();
  }

  if (opts::cg::ipara.IsEnabledByUser()) {
    opts::cg::ipara ? EnableIPARA() : DisableIPARA();
  }

  if (opts::cg::cfgo.IsEnabledByUser()) {
    opts::cg::cfgo ? EnableCFGO() : DisableCFGO();
  }

  if (opts::cg::ico.IsEnabledByUser()) {
    opts::cg::ico ? EnableICO() : DisableICO();
  }

  if (opts::cg::storeloadopt.IsEnabledByUser()) {
    opts::cg::storeloadopt ? EnableStoreLoadOpt() : DisableStoreLoadOpt();
  }

  if (opts::cg::globalopt.IsEnabledByUser()) {
    opts::cg::globalopt ? EnableGlobalOpt() : DisableGlobalOpt();
  }

  // only on master
  if (opts::cg::hotcoldsplit.IsEnabledByUser()) {
    opts::cg::hotcoldsplit ? EnableHotColdSplit() : DisableHotColdSplit();
  }

  if (opts::cg::preraopt.IsEnabledByUser()) {
    opts::cg::preraopt ? EnablePreRAOpt() : DisablePreRAOpt();
  }

  if (opts::cg::lsraLvarspill.IsEnabledByUser()) {
    opts::cg::lsraLvarspill ? EnableLocalRefSpill() : DisableLocalRefSpill();
  }

  if (opts::cg::lsraOptcallee.IsEnabledByUser()) {
    opts::cg::lsraOptcallee ? EnableCalleeToSpill() : DisableCalleeToSpill();
  }

  if (opts::cg::prepeep.IsEnabledByUser()) {
    opts::cg::prepeep ? EnablePrePeephole() : DisablePrePeephole();
  }

  if (opts::cg::peep.IsEnabledByUser()) {
    opts::cg::peep ? EnablePeephole() : DisablePeephole();
  }

  if (opts::cg::retMerge.IsEnabledByUser()) {
    opts::cg::retMerge ? EnableRetMerge() : DisableRetMerge();
  }

  if (opts::cg::preschedule.IsEnabledByUser()) {
    opts::cg::preschedule ? EnablePreSchedule() : DisablePreSchedule();
  }

  if (opts::cg::schedule.IsEnabledByUser()) {
    opts::cg::schedule ? EnableSchedule() : DisableSchedule();
  }

  if (opts::cg::vregRename.IsEnabledByUser()) {
    opts::cg::vregRename ? EnableVregRename() : DisableVregRename();
  }

  if (opts::cg::fullcolor.IsEnabledByUser()) {
    opts::cg::fullcolor ? EnableMultiPassColorRA() : DisableMultiPassColorRA();
  }

  if (opts::cg::writefieldopt.IsEnabledByUser()) {
    opts::cg::writefieldopt ? EnableWriteRefFieldOpt() : DisableWriteRefFieldOpt();
  }

  if (opts::cg::dumpOlog.IsEnabledByUser()) {
    opts::cg::dumpOlog ? EnableDumpOptimizeCommonLog() : DisableDumpOptimizeCommonLog();
  }

  if (opts::cg::nativeopt.IsEnabledByUser()) {
    opts::cg::nativeopt ? EnableNativeOpt() : DisableNativeOpt();
  }

  if (opts::cg::dupBb.IsEnabledByUser()) {
    opts::cg::dupBb ? DisableNoDupBB() : EnableNoDupBB();
  }

  if (opts::cg::calleeCfi.IsEnabledByUser()) {
    opts::cg::calleeCfi ? DisableNoCalleeCFI() : EnableNoCalleeCFI();
  }

  if (opts::cg::proepilogue.IsEnabledByUser()) {
    opts::cg::proepilogue ? SetOption(CGOptions::kProEpilogueOpt)
                          : ClearOption(CGOptions::kProEpilogueOpt);
  }

  if (opts::tailcall.IsEnabledByUser()) {
    opts::tailcall ? EnableTailCallOpt() : DisableTailCallOpt();
  }

  if (opts::cg::calleeregsPlacement.IsEnabledByUser()) {
    opts::cg::calleeregsPlacement ? EnableRegSavesOpt() : DisableRegSavesOpt();
  }

  if (opts::cg::ssapreSave.IsEnabledByUser()) {
    opts::cg::ssapreSave ? EnableSsaPreSave() : DisableSsaPreSave();
  }

  if (opts::cg::ssupreRestore.IsEnabledByUser()) {
    opts::cg::ssupreRestore ? EnableSsuPreRestore() : DisableSsuPreRestore();
  }

  if (opts::cg::newCg.IsEnabledByUser()) {
    opts::cg::newCg ? EnableNewCg() : DisableNewCg();
  }

  if (opts::cg::lsraBb.IsEnabledByUser()) {
    SetLSRABBOptSize(opts::cg::lsraBb);
  }

  if (opts::cg::lsraInsn.IsEnabledByUser()) {
    SetLSRAInsnOptSize(opts::cg::lsraInsn);
  }

  if (opts::cg::lsraOverlap.IsEnabledByUser()) {
    SetOverlapNum(opts::cg::lsraOverlap);
  }

  if (opts::cg::remat.IsEnabledByUser()) {
    SetRematLevel(opts::cg::remat);
  }

  if (opts::cg::dumpPhases.IsEnabledByUser()) {
    SplitPhases(opts::cg::dumpPhases, GetDumpPhases());
  }

  if (opts::cg::target.IsEnabledByUser()) {
    SetTargetMachine(opts::cg::target);
  }

  if (opts::cg::skipPhases.IsEnabledByUser()) {
    SplitPhases(opts::cg::skipPhases, GetSkipPhases());
  }

  if (opts::cg::skipFrom.IsEnabledByUser()) {
    SetSkipFrom(opts::cg::skipFrom);
  }

  if (opts::cg::skipAfter.IsEnabledByUser()) {
    SetSkipAfter(opts::cg::skipAfter);
  }

  if (opts::cg::debugSchedule.IsEnabledByUser()) {
    opts::cg::debugSchedule ? EnableDebugSched() : DisableDebugSched();
  }

  if (opts::cg::bruteforceSchedule.IsEnabledByUser()) {
    opts::cg::bruteforceSchedule ? EnableDruteForceSched() : DisableDruteForceSched();
  }

  if (opts::cg::simulateSchedule.IsEnabledByUser()) {
    opts::cg::simulateSchedule ? EnableSimulateSched() : DisableSimulateSched();
  }

  if (opts::profile.IsEnabledByUser()) {
    SetProfileData(opts::profile);
  }

  if (opts::cg::unwindTables.IsEnabledByUser()) {
    if (opts::cg::unwindTables) {
      SetOption(kUseUnwindTables);
    } else {
      ClearOption(kUseUnwindTables);
    }
  }

  if (opts::cg::nativeopt.IsEnabledByUser()) {
    DisableNativeOpt();
  }

  if (opts::cg::floatAbi.IsEnabledByUser()) {
    SetABIType(opts::cg::floatAbi);
  }

  if (opts::cg::filetype.IsEnabledByUser()) {
    SetEmitFileType(opts::cg::filetype);
  }

  if (opts::cg::longCalls.IsEnabledByUser()) {
    opts::cg::longCalls ? EnableLongCalls() : DisableLongCalls();
  }

  if (opts::cg::functionSections.IsEnabledByUser()) {
    opts::cg::functionSections ? EnableFunctionSections() : DisableFunctionSections();
  }

  if (opts::cg::dataSections.IsEnabledByUser()) {
    opts::cg::dataSections ? EnableDataSections() : DisableDataSections();
  }

  if (opts::cg::omitLeafFramePointer.IsEnabledByUser()) {
    opts::cg::omitLeafFramePointer ? SetFramePointer(kNonLeafFP) : SetFramePointer(kAllFP);
  }

  if (opts::cg::omitFramePointer.IsEnabledByUser()) {
    opts::cg::omitFramePointer ? SetFramePointer(kNoneFP) :
        ((!opts::cg::omitLeafFramePointer.IsEnabledByUser() || opts::cg::omitLeafFramePointer) ?
            SetFramePointer(kNonLeafFP) : SetFramePointer(kAllFP));
  }

  if (opts::gcOnly.IsEnabledByUser()) {
    opts::gcOnly ? EnableGCOnly() : DisableGCOnly();
  }

  if (opts::cg::fastMath.IsEnabledByUser()) {
    opts::cg::fastMath ? EnableFastMath() : DisableFastMath();
  }

  if (opts::cg::alignAnalysis.IsEnabledByUser()) {
    opts::cg::alignAnalysis ? EnableAlignAnalysis() : DisableAlignAnalysis();
  }

  if (opts::cg::condbrAlign.IsEnabledByUser()) {
    opts::cg::condbrAlign ? EnableCondBrAlign() : DisableCondBrAlign();
  }

  if (opts::cg::loopAlign.IsEnabledByUser()) {
    opts::cg::loopAlign ? EnableLoopAlign() : DisableLoopAlign();
  }

  /* big endian can be set with several options: --target, -Be.
   * Triple takes to account all these options and allows to detect big endian with IsBigEndian() interface */
  Triple::GetTriple().IsBigEndian() ? EnableBigEndianInCG() : DisableBigEndianInCG();
  (maple::Triple::GetTriple().GetEnvironment() == Triple::kGnuIlp32) ? EnableArm64ilp32() : DisableArm64ilp32();

  if (opts::cg::cgSsa.IsEnabledByUser()) {
    opts::cg::cgSsa ? EnableCGSSA() : DisableCGSSA();
  }

  if (opts::cg::layoutColdPath.IsEnabledByUser()) {
    opts::cg::layoutColdPath ? EnableLayoutColdPath() : DisableLayoutColdPath();
  }

  if (opts::cg::coldPathThreshold.IsEnabledByUser()) {
    SetColdPathThreshold(opts::cg::coldPathThreshold);
  }

  if (opts::cg::globalSchedule.IsEnabledByUser()) {
    opts::cg::globalSchedule ? EnableGlobalSchedule() : DisableGlobalSchedule();
  }

  if (opts::cg::localSchedule.IsEnabledByUser()) {
    opts::cg::localSchedule ? EnableLocalSchedule() : DisableLocalSchedule();
  }

  if (opts::cg::common.IsEnabledByUser()) {
    opts::cg::common ? EnableCommon() : DisableCommon();
  }

  if (opts::cg::alignMinBbSize.IsEnabledByUser()) {
    SetAlignMinBBSize(opts::cg::alignMinBbSize);
  }

  if (opts::cg::alignMaxBbSize.IsEnabledByUser()) {
    SetAlignMaxBBSize(opts::cg::alignMaxBbSize);
  }

  if (opts::cg::loopAlignPow.IsEnabledByUser()) {
    SetLoopAlignPow(opts::cg::loopAlignPow);
  }

  if (opts::cg::jumpAlignPow.IsEnabledByUser()) {
    SetJumpAlignPow(opts::cg::jumpAlignPow);
  }

  if (opts::cg::funcAlignPow.IsEnabledByUser()) {
    SetFuncAlignPow(opts::cg::funcAlignPow);
  }

  if (opts::cg::litePgoGen.IsEnabledByUser()) {
    opts::cg::litePgoGen ? EnableLiteProfGen() : DisableLiteProfGen();
  }
  if (opts::cg::litePgoVerify.IsEnabledByUser()) {
    opts::cg::litePgoVerify ? EnableLiteProfVerify() : DisableLiteProfVerify();
  }

  if (opts::cg::litePgoOutputFunc.IsEnabledByUser()) {
    EnableLiteProfGen();
    if (!opts::cg::litePgoOutputFunc.GetValue().empty()) {
      SetLitePgoOutputFunction(opts::cg::litePgoOutputFunc);
    }
  }

  if (opts::cg::litePgoWhiteList.IsEnabledByUser()) {
    SetLitePgoWhiteList(opts::cg::litePgoWhiteList);
  }

  if (opts::cg::instrumentationDir.IsEnabledByUser()) {
    SetInstrumentationOutPutPath(opts::cg::instrumentationDir);
    if (!opts::cg::instrumentationDir.GetValue().empty()) {
      EnableLiteProfGen();
    }
  }

  if (opts::cg::litePgoFile.IsEnabledByUser()) {
    SetLiteProfile(opts::cg::litePgoFile);
    if (!opts::cg::litePgoFile.GetValue().empty()) {
      EnableLiteProfUse();
    }
  }

  if (opts::cg::functionPriority.IsEnabledByUser()) {
    SetFunctionPriority(opts::cg::functionPriority);
  }

  if (opts::functionReorderAlgorithm.IsEnabledByUser()) {
    SetFunctionReorderAlgorithm(opts::functionReorderAlgorithm);
  }

  if (opts::functionReorderProfile.IsEnabledByUser()) {
    SetFunctionReorderProfile(opts::functionReorderProfile);
  }

  if (opts::fVisibility.IsEnabledByUser()) {
    SetVisibilityType(opts::fVisibility);
  }

  if (opts::cg::optimizedFrameLayout.IsEnabledByUser()) {
    opts::cg::optimizedFrameLayout ? EnableOptimizedFrameLayout() : DisableOptimizedFrameLayout();
  }

  SetOption(kWithSrc);

  if (opts::cg::pgoCodeAlign.IsEnabledByUser()) {
    EnablePgoCodeAlign();
  }

  if (opts::cg::alignThreshold.IsEnabledByUser()) {
    SetAlignThreshold(opts::cg::alignThreshold);
  }

  if (opts::cg::alignLoopIterations.IsEnabledByUser()) {
    SetAlignLoopIterations(opts::cg::alignLoopIterations);
  }

  if (opts::cg::dupFreqThreshold.IsEnabledByUser()) {
    SetDupFreqThreshold(opts::cg::dupFreqThreshold);
  }

  /* override some options when loc, dwarf is generated */
  if (WithLoc()) {
    SetOption(kWithSrc);
  }
  if (WithDwarf()) {
    SetOption(kDebugFriendly);
    SetOption(kWithSrc);
    SetOption(kWithLoc);
    ClearOption(kSuppressFileInfo);
  }

  return true;
}

void CGOptions::ParseExclusiveFunc(const std::string &fileName) {
  std::ifstream file(fileName);
  if (!file.is_open()) {
    ERR(kLncErr, "%s open failed!", fileName.c_str());
    return;
  }
  std::string content;
  while (file >> content) {
    ehExclusiveFunctionName.push_back(content);
  }
}

void CGOptions::ParseCyclePattern(const std::string &fileName) const {
  std::ifstream file(fileName);
  if (!file.is_open()) {
    ERR(kLncErr, "%s open failed!", fileName.c_str());
    return;
  }
  std::string content;
  std::string classStr("class: ");
  while (getline(file, content)) {
    if (content.compare(0, classStr.length(), classStr) == 0) {
      std::vector<std::string> classPatternContent;
      std::string patternContent;
      while (getline(file, patternContent)) {
        if (patternContent.length() == 0) {
          break;
        }
        classPatternContent.push_back(patternContent);
      }
      std::string className = content.substr(classStr.length());
      CGOptions::cyclePatternMap[className] = std::move(classPatternContent);
    }
  }
}

void CGOptions::SetRange(const std::string &str, const std::string &cmd, Range &subRange) const {
  const std::string &tmpStr = str;
  size_t comma = tmpStr.find_first_of(",", 0);
  subRange.enable = true;

  if (comma != std::string::npos) {
    subRange.begin = std::stoul(tmpStr.substr(0, comma), nullptr);
    subRange.end = std::stoul(tmpStr.substr(comma + 1, std::string::npos - (comma + 1)), nullptr);
  }
  CHECK_FATAL(range.begin < range.end, "invalid values for %s=%lu,%lu", cmd.c_str(), subRange.begin, subRange.end);
}

/* Set default options according to different languages. */
void CGOptions::SetDefaultOptions(const maple::MIRModule &mod) {
  if (mod.IsJavaModule()) {
    generateFlag = generateFlag | kGenYieldPoint | kGenLocalRc | kGrootList | kPrimorList;
  }
  if (mod.GetFlavor() == MIRFlavor::kFlavorLmbc) {
    EnableFlavorLmbc();
  }
  insertYieldPoint = GenYieldPoint();
}

void CGOptions::EnableO0() {
  optimizeLevel = kLevel0;
  doEBO = false;
  doCGSSA = false;
  doGlobalSchedule = false;
  doLocalSchedule = false;
  doCFGO = false;
  doICO = false;
  doPrePeephole = false;
  doPeephole = false;
  doPostRASink = false;
  doStoreLoadOpt = false;
  doGlobalOpt = false;
  doPreRAOpt = false;
  doLocalRefSpill = false;
  doCalleeToSpill = false;
  doPreSchedule = false;
  doSchedule = false;
  doRegSavesOpt = false;
  useSsaPreSave = false;
  useSsuPreRestore = false;
  doWriteRefFieldOpt = false;
  doAlignAnalysis = false;
  doCondBrAlign = false;
  doLoopAlign = false;
  doAggrOpt = false;
  doCGMemAlias = false;
  SetOption(kUseUnwindTables);
  if (maple::Triple::GetTriple().GetEnvironment() == Triple::kGnuIlp32) {
    ClearOption(kUseStackProtectorStrong);
  } else {
    SetOption(kUseStackProtectorStrong);
  }
  ClearOption(kUseStackProtectorAll);
  ClearOption(kConstFold);
  ClearOption(kProEpilogueOpt);
}

void CGOptions::EnableO1() {
  optimizeLevel = kLevel1;
  doPreRAOpt = true;
  doCalleeToSpill = true;
  doTailCallOpt = true;
  SetOption(kConstFold);
  SetOption(kProEpilogueOpt);
  SetOption(kUseUnwindTables);
  ClearOption(kUseStackProtectorStrong);
  ClearOption(kUseStackProtectorAll);
}

void CGOptions::EnableO2() {
  optimizeLevel = kLevel2;
  doEBO = true;
  doCGSSA = true;
  doGlobalSchedule = true;
  doLocalSchedule = true;
  doCFGO = true;
  doICO = true;
  doPrePeephole = true;
  doPeephole = true;
  doPostRASink = true;
  doStoreLoadOpt = true;
  doGlobalOpt = true;
  doPreSchedule = true;
  doSchedule = true;
  doAlignAnalysis = true;
  doCondBrAlign = true;
  doLoopAlign = true;
  doRetMerge = true;
  doAggrOpt = true;
  doCGMemAlias = true;
  SetOption(kConstFold);
  SetOption(kUseUnwindTables);
  ClearOption(kUseStackProtectorStrong);
  ClearOption(kUseStackProtectorAll);
#if defined(TARGARM32) && TARGARM32
  doPreRAOpt = false;
  doLocalRefSpill = false;
  doCalleeToSpill = false;
  doWriteRefFieldOpt = false;
  doTailCallOpt = false;
  ClearOption(kProEpilogueOpt);
#else
  doPreRAOpt = true;
  doLocalRefSpill = true;
  doCalleeToSpill = true;
  doRegSavesOpt = true;
  useSsaPreSave = true;
  useSsuPreRestore = true;
  doWriteRefFieldOpt = true;
  doTailCallOpt = true;
  SetOption(kProEpilogueOpt);
#endif

  /* O2 performs legalizeNumericTypes optimization on mpl2mpl (O0 does it on codegen) */
  opts::legalizeNumericTypes.SetValue(false);
}

void CGOptions::EnableLiteCG() {
  optimizeLevel = kLevelLiteCG;
  doEBO = false;
  doCGSSA = false;
  doGlobalSchedule = false;
  doLocalSchedule = false;
  doCFGO = false;
  doICO = false;
  doPrePeephole = false;
  doPeephole = false;
  doStoreLoadOpt = false;
  doGlobalOpt = false;
  doPreRAOpt = false;
  doLocalRefSpill = false;
  doCalleeToSpill = false;
  doPreSchedule = false;
  doSchedule = false;
  doRegSavesOpt = false;
  doTailCallOpt = false;
  useSsaPreSave = false;
  useSsuPreRestore = false;
  doWriteRefFieldOpt = false;
  doAlignAnalysis = false;
  doCondBrAlign = false;
  doAggrOpt = false;
  doCGMemAlias = false;

  ClearOption(kUseStackProtectorStrong);
  ClearOption(kUseStackProtectorAll);
  ClearOption(kConstFold);
  ClearOption(kProEpilogueOpt);
}

void CGOptions::SetTargetMachine(const std::string &str) {
  if (str == "aarch64") {
    targetArch = "aarch64";
  } else if (str == "x86_64") {
    targetArch = "x86_64";
  }
  CHECK_FATAL(false, "unknown target. not implement yet");
}

void CGOptions::SplitPhases(const std::string &str, std::unordered_set<std::string> &set) const {
  const std::string& tmpStr{ str };
  if ((tmpStr.compare("*") == 0) || (tmpStr.compare("cgir") == 0)) {
    (void)set.insert(tmpStr);
    return;
  }
  StringUtils::Split(tmpStr, set, ',');
}

bool CGOptions::DumpPhase(const std::string &phase) {
  return (IS_STR_IN_SET(dumpPhases, "*") || IS_STR_IN_SET(dumpPhases, "cgir") || IS_STR_IN_SET(dumpPhases, phase));
}

/* match sub std::string of function name */
bool CGOptions::FuncFilter(const std::string &name) {
  return dumpFunc == "*" || dumpFunc == name;
}
}  /* namespace maplebe */
