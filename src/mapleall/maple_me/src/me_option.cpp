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
#include "me_option.h"
#include <iostream>
#include "driver_options.h"
#include "me_options.h"
#include "mpl_logging.h"
#include "string_utils.h"
#include "triple.h"

namespace maple {

std::unordered_set<std::string> MeOption::dumpPhases = {};
std::unordered_set<std::string> MeOption::skipPhases = {};
bool MeOption::isBigEndian = false;
bool MeOption::dumpAfter = false;
std::string MeOption::dumpFunc = "*";
unsigned long MeOption::range[kRangeArrayLen] = { 0, 0 };
bool MeOption::useRange = false;
bool MeOption::quiet = false;
bool MeOption::setCalleeHasSideEffect = false;
bool MeOption::unionBasedAA = true;
bool MeOption::tbaa = true;
bool MeOption::ddaa = true;
uint8 MeOption::aliasAnalysisLevel = 3;
bool MeOption::noDot = false;
bool MeOption::stmtNum = false;
bool MeOption::optForSize = false;
bool MeOption::enableHotColdSplit = false;
uint8 MeOption::optLevel = 0;
bool MeOption::ignoreIPA = true;
bool MeOption::aggressiveABCO = false;
bool MeOption::commonABCO = true;
bool MeOption::conservativeABCO = false;
bool MeOption::lessThrowAlias = true;
bool MeOption::regreadAtReturn = true;
bool MeOption::propBase = true;
bool MeOption::propIloadRef = false;
bool MeOption::propGlobalRef = false;
bool MeOption::propFinaliLoadRef = true;
bool MeOption::propIloadRefNonParm = false;
uint32 MeOption::delRcPULimit = UINT32_MAX;
uint32 MeOption::stmtprePULimit = UINT32_MAX;
uint32 MeOption::epreLimit = UINT32_MAX;
uint32 MeOption::eprePULimit = UINT32_MAX;
uint32 MeOption::lpreLimit = UINT32_MAX;
uint32 MeOption::lprePULimit = UINT32_MAX;
uint32 MeOption::pregRenameLimit = UINT32_MAX;
uint32 MeOption::rename2pregLimit = UINT32_MAX;
uint32 MeOption::propLimit = UINT32_MAX;
uint32 MeOption::copyPropLimit = UINT32_MAX;
uint32 MeOption::vecLoopLimit = UINT32_MAX;
uint32 MeOption::ivoptsLimit = UINT32_MAX;
uint32 MeOption::profileBBHotRate = 10;
uint32 MeOption::profileBBColdRate = 99;
bool MeOption::noDelegateRC = false;
bool MeOption::noCondBasedRC = false;
bool MeOption::clinitPre = true;
bool MeOption::dassignPre = false;
bool MeOption::mergeStmts = true;
bool MeOption::generalRegOnly = false;
bool MeOption::nullCheckPre = false;
bool MeOption::assign2FinalPre = false;
bool MeOption::epreIncludeRef = false;
bool MeOption::epreLocalRefVar = true;
bool MeOption::epreLHSIvar = true;
bool MeOption::lpreSpeculate = false;
bool MeOption::lpre4Address = true;
bool MeOption::lpre4LargeInt = true;
bool MeOption::spillAtCatch = false;
bool MeOption::rcLowering = true;
bool MeOption::optDirectCall = false;
bool MeOption::propAtPhi = true;
bool MeOption::propDuringBuild = true;
bool MeOption::propWithInverse = false;
bool MeOption::dseKeepRef = false;
bool MeOption::decoupleStatic = false;
bool MeOption::dumpBefore = false;
std::string MeOption::skipFrom = "";
std::string MeOption::skipAfter = "";
bool MeOption::noRC = false;
bool MeOption::lazyDecouple = false;
bool MeOption::strictNaiveRC = false;
std::unordered_set<std::string> MeOption::checkRefUsedInFuncs = {};
bool MeOption::gcOnly = false;
bool MeOption::gcOnlyOpt = false;
bool MeOption::noGCBar = true;
bool MeOption::realCheckCast = false;
bool MeOption::regNativeFunc = false;
bool MeOption::warnNativeFunc = false;
uint32 MeOption::parserOpt = 0;
uint32 MeOption::threads = 1;  // set default threads number
bool MeOption::ignoreInferredRetType = false;
bool MeOption::checkCastOpt = false;
bool MeOption::parmToPtr = false;
bool MeOption::nativeOpt = true;
bool MeOption::enableEA = false;
bool MeOption::placementRC = false;
bool MeOption::subsumRC = false;
bool MeOption::performFSAA = true;
bool MeOption::strengthReduction = true;
bool MeOption::srForAdd = false;
bool MeOption::doLFTR = true;
bool MeOption::ivopts = true;
std::string MeOption::inlineFuncList = "";
bool MeOption::meVerify = false;
uint32 MeOption::dseRunsLimit = 2;    // dse phase run at most 2 times each PU
uint32 MeOption::hdseRunsLimit = 3;   // hdse phase run at most 3 times each PU
uint32 MeOption::hpropRunsLimit = 2;  // hprop phase run at most 2 times each PU
uint32 MeOption::sinkLimit = UINT32_MAX;
uint32 MeOption::sinkPULimit = UINT32_MAX;
bool MeOption::loopVec = true;
bool MeOption::seqVec = true;
bool MeOption::enableLFO = true;
uint8 MeOption::rematLevel = 2;
bool MeOption::layoutWithPredict = true;  // optimize output layout using branch prediction
SafetyCheckMode MeOption::npeCheckMode = SafetyCheckMode::kNoCheck;
bool MeOption::isNpeCheckAll = false;
SafetyCheckMode MeOption::boundaryCheckMode = SafetyCheckMode::kNoCheck;
bool MeOption::safeRegionMode = false;
bool MeOption::unifyRets = false;
bool MeOption::dumpCfgOfPhases = false;
#if MIR_JAVA
std::string MeOption::acquireFuncName = "Landroid/location/LocationManager;|requestLocationUpdates|";
std::string MeOption::releaseFuncName = "Landroid/location/LocationManager;|removeUpdates|";
unsigned int MeOption::warningLevel = 0;
bool MeOption::mplToolOnly = false;
bool MeOption::mplToolStrict = false;
bool MeOption::skipVirtualMethod = false;
#endif

void MeOption::DecideMeRealLevel() const {
  if (opts::me::o1) {
    optLevel = kLevelOne;
  } else if (opts::me::o2 || opts::me::os) {
    if (opts::me::os) {
      optForSize = true;
    }
    optLevel = kLevelTwo;
    // Turn the followings ON only at O2
    optDirectCall = true;
    placementRC = true;
    subsumRC = true;
    epreIncludeRef = true;
  } else if (opts::me::o3) {
    optForSize = false;
    optLevel = kLevelThree;
    // turn on as O2
    optDirectCall = true;
    placementRC = true;
    subsumRC = true;
    epreIncludeRef = true;
  }
}

bool MeOption::SolveOptions(bool isDebug) {
  DecideMeRealLevel();
  if (isDebug) {
    LogInfo::MapleLogger() << "Real Me level:" << std::to_string(optLevel) << "\n";

    for (const auto &opt : meCategory.GetEnabledOptions()) {
      std::string printOpt;
      for (const auto &val : opt->GetRawValues()) {
        printOpt += opt->GetName() + " " + val + " ";
      }
      LogInfo::MapleLogger() << "me options: " << printOpt << '\n';
    }
  }

  if (opts::me::skipPhases.IsEnabledByUser()) {
    SplitSkipPhases(opts::me::skipPhases);
  }

  if (opts::me::refusedcheck.IsEnabledByUser()) {
    SplitPhases(opts::me::refusedcheck, checkRefUsedInFuncs);
  }

  if (opts::me::range.IsEnabledByUser()) {
    useRange = true;
    bool ret = GetRange(opts::me::range);
    if (!ret) {
      return ret;
    }
  }

  maplecl::CopyIfEnabled(dumpBefore, opts::me::dumpBefore);
  maplecl::CopyIfEnabled(dumpAfter, opts::me::dumpAfter);

  /* big endian can be set with several options: --target, -Be.
   * Triple takes to account all these options and allows to detect big endian with IsBigEndian() interface */
  if (Triple::GetTriple().IsBigEndian()) {
    isBigEndian = true;
  }

  maplecl::CopyIfEnabled(dumpFunc, opts::me::dumpFunc);
  maplecl::CopyIfEnabled(skipFrom, opts::me::skipFrom);
  maplecl::CopyIfEnabled(skipAfter, opts::me::skipAfter);

  if (opts::me::dumpPhases.IsEnabledByUser()) {
    SplitPhases(opts::me::dumpPhases, dumpPhases);
  }

  maplecl::CopyIfEnabled(quiet, opts::me::quiet);
  maplecl::CopyIfEnabled(quiet, !opts::verbose, opts::verbose);

  if (opts::profileGen.IsEnabledByUser()) {
    if (optLevel != kLevelZero) {
      WARN(kLncWarn, "profileGen requires no optimization");
      return false;
    }
  }

  maplecl::CopyIfEnabled(setCalleeHasSideEffect, opts::me::calleeHasSideEffect);
  maplecl::CopyIfEnabled(unionBasedAA, opts::me::ubaa);
  maplecl::CopyIfEnabled(tbaa, opts::me::tbaa);
  maplecl::CopyIfEnabled(ddaa, opts::me::ddaa);

  if (opts::me::aliasAnalysisLevel.IsEnabledByUser()) {
    aliasAnalysisLevel = opts::me::aliasAnalysisLevel;
    if (aliasAnalysisLevel > kLevelThree) {
      aliasAnalysisLevel = kLevelThree;
    }

    switch (aliasAnalysisLevel) {
      case kLevelThree: {
        setCalleeHasSideEffect = false;
        unionBasedAA = true;
        tbaa = true;
        break;
      }
      case kLevelZero: {
        setCalleeHasSideEffect = true;
        unionBasedAA = false;
        tbaa = false;
        break;
      }
      case kLevelOne: {
        setCalleeHasSideEffect = false;
        unionBasedAA = true;
        tbaa = false;
        break;
      }
      case kLevelTwo: {
        setCalleeHasSideEffect = false;
        unionBasedAA = false;
        tbaa = true;
        break;
      }
      default:
        break;
    }

    if (isDebug) {
      LogInfo::MapleLogger() << "--sub options: setCalleeHasSideEffect "
                             << setCalleeHasSideEffect << '\n';
      LogInfo::MapleLogger() << "--sub options: ubaa " << unionBasedAA << '\n';
      LogInfo::MapleLogger() << "--sub options: tbaa " << tbaa << '\n';
    }
  }

  maplecl::CopyIfEnabled(rcLowering, opts::me::rclower);
  maplecl::CopyIfEnabled(noRC, !opts::me::userc, opts::me::userc);
  maplecl::CopyIfEnabled(lazyDecouple, opts::me::lazydecouple);
  maplecl::CopyIfEnabled(strictNaiveRC, opts::me::strictNaiverc);

  if (opts::gcOnly.IsEnabledByUser()) {
    gcOnly = opts::gcOnly;
    propIloadRef = opts::gcOnly;
    if (isDebug) {
      LogInfo::MapleLogger() << "--sub options: propIloadRef " << propIloadRef << '\n';
      LogInfo::MapleLogger() << "--sub options: propGlobalRef " << propGlobalRef << '\n';
    }
  }

  maplecl::CopyIfEnabled(gcOnlyOpt, opts::me::gconlyopt);
  maplecl::CopyIfEnabled(noGCBar, !opts::me::usegcbar, opts::me::usegcbar);
  maplecl::CopyIfEnabled(realCheckCast, opts::me::realcheckcast);
  maplecl::CopyIfEnabled(noDot, opts::me::nodot);
  maplecl::CopyIfEnabled(stmtNum, opts::me::stmtnum);
  maplecl::CopyIfEnabled(regNativeFunc, opts::me::regnativefunc);
  maplecl::CopyIfEnabled(warnNativeFunc, opts::me::warnemptynative);
  maplecl::CopyIfEnabled(epreLimit, opts::me::eprelimit);
  maplecl::CopyIfEnabled(eprePULimit, opts::me::eprepulimit);
  maplecl::CopyIfEnabled(stmtprePULimit, opts::me::stmtprepulimit);
  maplecl::CopyIfEnabled(lpreLimit, opts::me::lprelimit);
  maplecl::CopyIfEnabled(lprePULimit, opts::me::lprepulimit);
  maplecl::CopyIfEnabled(pregRenameLimit, opts::me::pregrenamelimit);
  maplecl::CopyIfEnabled(rename2pregLimit, opts::me::rename2preglimit);
  maplecl::CopyIfEnabled(propLimit, opts::me::proplimit);
  maplecl::CopyIfEnabled(copyPropLimit, opts::me::copyproplimit);
  maplecl::CopyIfEnabled(delRcPULimit, opts::me::delrcpulimit);
  maplecl::CopyIfEnabled(profileBBHotRate, opts::me::profileBbHotRate);
  maplecl::CopyIfEnabled(profileBBColdRate, opts::me::profileBbColdRate);
  maplecl::CopyIfEnabled(ignoreIPA, opts::me::ignoreipa);
  maplecl::CopyIfEnabled(enableHotColdSplit, opts::me::enableHotColdSplit);
  maplecl::CopyIfEnabled(aggressiveABCO, opts::me::aggressiveABCO);
  maplecl::CopyIfEnabled(commonABCO, opts::me::commonABCO);
  maplecl::CopyIfEnabled(conservativeABCO, opts::me::conservativeABCO);
  maplecl::CopyIfEnabled(epreIncludeRef, opts::me::epreincluderef);
  maplecl::CopyIfEnabled(epreLocalRefVar, opts::me::eprelocalrefvar);
  maplecl::CopyIfEnabled(epreLHSIvar, opts::me::eprelhsivar);
  maplecl::CopyIfEnabled(dseKeepRef, opts::me::dsekeepref);
  maplecl::CopyIfEnabled(lessThrowAlias, opts::me::lessthrowalias);
  maplecl::CopyIfEnabled(propBase, opts::me::propbase);
  maplecl::CopyIfEnabled(dumpCfgOfPhases, opts::me::dumpCfgOfPhases);

  if (opts::me::propiloadref.IsEnabledByUser()) {
    propIloadRef = opts::me::propiloadref;
    if (opts::me::propiloadref) {
      propIloadRefNonParm = false;  // to override previous -propIloadRefNonParm
    }

    if (isDebug) {
      LogInfo::MapleLogger() << "--sub options: propIloadRefNonParm " << propIloadRefNonParm << '\n';
    }
  }

  maplecl::CopyIfEnabled(propGlobalRef, opts::me::propglobalref);
  maplecl::CopyIfEnabled(propFinaliLoadRef, opts::me::propfinaliloadref);

  if (opts::me::propiloadrefnonparm.IsEnabledByUser()) {
    propIloadRefNonParm = opts::me::propiloadrefnonparm;
    propIloadRef = propIloadRefNonParm;

    if (isDebug) {
      LogInfo::MapleLogger() << "--sub options: propIloadRef " << propIloadRef << '\n';
    }
  }

  maplecl::CopyIfEnabled(noDelegateRC, opts::me::nodelegaterc);
  maplecl::CopyIfEnabled(noCondBasedRC, opts::me::nocondbasedrc);
  maplecl::CopyIfEnabled(checkCastOpt, opts::me::checkcastopt);
  maplecl::CopyIfEnabled(parmToPtr, opts::me::parmtoptr);
  maplecl::CopyIfEnabled(nullCheckPre, opts::me::nullcheckpre);
  maplecl::CopyIfEnabled(clinitPre, opts::me::clinitpre);
  maplecl::CopyIfEnabled(dassignPre, opts::me::dassignpre);
  maplecl::CopyIfEnabled(mergeStmts, opts::me::mergestmts);
  maplecl::CopyIfEnabled(generalRegOnly, opts::me::generalRegOnly);
  maplecl::CopyIfEnabled(assign2FinalPre, opts::me::assign2finalpre);
  maplecl::CopyIfEnabled(regreadAtReturn, opts::me::regreadatreturn);
  maplecl::CopyIfEnabled(propAtPhi, opts::me::propatphi);
  maplecl::CopyIfEnabled(propDuringBuild, opts::me::propduringbuild);
  maplecl::CopyIfEnabled(propWithInverse, opts::me::propwithinverse);
  maplecl::CopyIfEnabled(nativeOpt, opts::me::nativeopt);
  maplecl::CopyIfEnabled(optDirectCall, opts::me::optdirectcall);
  maplecl::CopyIfEnabled(enableEA, opts::me::enableEa);
  maplecl::CopyIfEnabled(lpreSpeculate, opts::me::lprespeculate);
  maplecl::CopyIfEnabled(lpre4Address, opts::me::lpre4address);
  maplecl::CopyIfEnabled(lpre4LargeInt, opts::me::lpre4largeint);
  maplecl::CopyIfEnabled(spillAtCatch, opts::me::spillatcatch);

  if (opts::me::placementrc.IsEnabledByUser()) {
    placementRC = opts::me::placementrc;
    if (!placementRC) {
      subsumRC = false;
      epreIncludeRef = false;
      if (isDebug) {
        LogInfo::MapleLogger() << "--sub options: subsumRC " << subsumRC << '\n';
        LogInfo::MapleLogger() << "--sub options: epreIncludeRef " << epreIncludeRef << '\n';
      }
    }
  }

  if (opts::me::subsumrc.IsEnabledByUser()) {
    subsumRC = opts::me::subsumrc;
    epreIncludeRef = opts::me::subsumrc;
  }

  maplecl::CopyIfEnabled(performFSAA, opts::me::performFSAA);
  maplecl::CopyIfEnabled(strengthReduction, opts::me::strengthreduction);
  maplecl::CopyIfEnabled(srForAdd, opts::me::sradd);
  maplecl::CopyIfEnabled(doLFTR, opts::me::lftr);
  maplecl::CopyIfEnabled(ivopts, opts::me::ivopts);

  // must have processed opts::strengthReduction
  if (ivopts) {
    // disable strengthReduction when ivopts is on
    strengthReduction = false;
  }

  maplecl::CopyIfEnabled(inlineFuncList, opts::me::inlinefunclist);
  maplecl::CopyIfEnabled(decoupleStatic, opts::decoupleStatic);
  maplecl::CopyIfEnabled(threads, opts::me::threads);
  maplecl::CopyIfEnabled(ignoreInferredRetType, opts::me::ignoreInferredRetType);
  maplecl::CopyIfEnabled(meVerify, opts::me::meverify);
  maplecl::CopyIfEnabled(dseRunsLimit, opts::me::dserunslimit);
  maplecl::CopyIfEnabled(hdseRunsLimit, opts::me::hdserunslimit);
  maplecl::CopyIfEnabled(hpropRunsLimit, opts::me::hproprunslimit);
  maplecl::CopyIfEnabled(sinkLimit, opts::me::sinklimit);
  maplecl::CopyIfEnabled(sinkPULimit, opts::me::sinkPUlimit);
  maplecl::CopyIfEnabled(loopVec, opts::me::loopvec);
  maplecl::CopyIfEnabled(seqVec, opts::me::seqvec);
  maplecl::CopyIfEnabled(enableLFO, opts::me::lfo);
  maplecl::CopyIfEnabled(rematLevel, opts::me::remat);
  maplecl::CopyIfEnabled(layoutWithPredict, opts::me::layoutwithpredict);
  maplecl::CopyIfEnabled(vecLoopLimit, opts::me::veclooplimit);
  maplecl::CopyIfEnabled(ivoptsLimit, opts::me::ivoptslimit);
  maplecl::CopyIfEnabled(unifyRets, opts::me::unifyrets);

#if MIR_JAVA
  maplecl::CopyIfEnabled(acquireFuncName, opts::me::acquireFunc);
  maplecl::CopyIfEnabled(releaseFuncName, opts::me::releaseFunc);
  maplecl::CopyIfEnabled(warningLevel, opts::me::warning);
  maplecl::CopyIfEnabled(mplToolOnly, opts::me::toolonly);
  maplecl::CopyIfEnabled(mplToolStrict, opts::me::toolstrict);
  maplecl::CopyIfEnabled(skipVirtualMethod, opts::me::skipvirtual);
#endif

  return true;
}

MeOption &MeOption::GetInstance() {
  static MeOption instance;
  return instance;
}

void MeOption::ParseOptions(int argc, char **argv, std::string &fileName) {
  maplecl::CommandLine::GetCommandLine().Parse(argc, argv, meCategory);
  bool result = SolveOptions(false);
  if (!result) {
    return;
  }

  auto &badArgs = maplecl::CommandLine::GetCommandLine().badCLArgs;
  int inputFileCount = 0;
  for (auto &arg : badArgs) {
    if (FileUtils::IsFileExists(arg.first)) {
      inputFileCount++;
      fileName = arg.first;
    } else {
      LogInfo::MapleLogger() << "Unknown Option: " << arg.first;
      CHECK_FATAL(false, "Unknown Option");
    }
  }

  /* only 1 input file should be set */
  if (inputFileCount != 1) {
    LogInfo::MapleLogger() << "expecting one .mpl file as last argument, found: ";
    for (const auto &optionArg : badArgs) {
      LogInfo::MapleLogger() << optionArg.first << " ";
    }
    LogInfo::MapleLogger() << '\n';
    CHECK_FATAL(false, "option parser error");
  }

#ifdef DEBUG_OPTION
  LogInfo::MapleLogger() << "mpl file : " << fileName << "\t";
#endif
}

void MeOption::SplitPhases(const std::string &str, std::unordered_set<std::string> &set) const {
  std::string s{ str };

  if (s.compare("*") == 0) {
    set.insert(s);
    return;
  }
  StringUtils::Split(s, set, ',');
}

bool MeOption::GetRange(const std::string &str) const {
  std::string s{ str };
  size_t comma = s.find_first_of(",", 0);
  if (comma != std::string::npos) {
    range[0] = std::stoul(s.substr(0, comma), nullptr);
    range[1] = std::stoul(s.substr(comma + 1, std::string::npos - (comma + 1)), nullptr);
  }
  if (range[0] > range[1]) {
    LogInfo::MapleLogger(kLlErr) << "invalid values for --range=" << range[0] << "," << range[1] << '\n';
    return false;
  }
  return true;
}

bool MeOption::DumpPhase(const std::string &phase) {
  if (phase == "") {
    return false;
  }
  return ((dumpPhases.find(phase) != dumpPhases.end()) || (dumpPhases.find("*") != dumpPhases.end()));
}

bool MeOption::DumpFunc(const std::string &func) {
  if (func == "") {
    return false;
  }
  return dumpFunc == "*" || dumpFunc == func;
}
} // namespace maple
