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
#ifndef MAPLE_ME_INCLUDE_ME_OPTION_H
#define MAPLE_ME_INCLUDE_ME_OPTION_H
#include <vector>
#include <string>
#include "mempool.h"
#include "mempool_allocator.h"
#include "types_def.h"
#include "driver_option_common.h"
#include "option_parser.h"

namespace maple {
class MeOption : public MapleDriverOptionBase {
 public:
  static MeOption &GetInstance();

  MeOption();

  ~MeOption() = default;

  bool SolveOptions(const std::vector<mapleOption::Option> &opts, bool isDebug);

  void ParseOptions(int argc, char **argv, std::string &fileName);

  void SplitPhases(const std::string &str, std::unordered_set<std::string> &set) const;
  void SplitSkipPhases(const std::string &str) {
    SplitPhases(str, skipPhases);
  }
  bool GetRange(const std::string &str) const;

  const std::unordered_set<std::string> &GetSkipPhases() const {
    return skipPhases;
  }

  static bool IsSkipFromPhase(const std::string &phaseName) {
    return skipFrom.compare(phaseName) == 0;
  }

  static const std::string GetSkipFromPhase() {
    return skipFrom;
  }

  static void SetSkipFrom(const std::string &phaseName) {
    skipFrom = phaseName;
  }

  static bool IsSkipAfterPhase(const std::string &phaseName) {
    return skipAfter.compare(phaseName) == 0;
  }

  static const std::string GetSkipAfterPhase() {
    return skipAfter;
  }

  static void SetSkipAfter(const std::string &phaseName) {
    skipAfter = phaseName;
  }

  static bool IsSkipPhase(const std::string &phaseName) {
    return !(skipPhases.find(phaseName) == skipPhases.end());
  }

  static bool DumpPhase(const std::string &phase);
  static bool DumpFunc(const std::string &func);
  static std::unordered_set<std::string> dumpPhases;
  static std::unordered_set<std::string> skipPhases;

  static bool dumpBefore;
  static bool dumpAfter;
  static constexpr int kRangeArrayLen = 2;
  static unsigned long range[kRangeArrayLen];
  static bool useRange;
  static std::string dumpFunc;
  static std::string skipFrom;
  static std::string skipAfter;
  static bool quiet;
  static bool setCalleeHasSideEffect;
  static bool noSteensgaard;
  static bool noTBAA;
  static bool noDDAA;
  static uint8 aliasAnalysisLevel;
  static bool noDot;
  static bool stmtNum;
  static bool rcLowering;
  static bool noRC;
  static bool strictNaiveRC;
  static std::unordered_set<std::string> checkRefUsedInFuncs;
  static bool gcOnly;
  static bool gcOnlyOpt;
  static bool noGCBar;
  static bool noReflection;
  static bool realCheckCast;
  static bool regNativeFunc;
  static bool warnNativeFunc;
  static bool lazyDecouple;
  static uint8 optLevel;
  static uint32 stmtprePULimit;
  static uint32 epreLimit;
  static uint32 eprePULimit;
  static uint32 lpreLimit;
  static uint32 lprePULimit;
  static uint32 parserOpt;
  static uint32 threads;
  static bool ignoreInferredRetType;
  static uint32 pregRenameLimit;
  static uint32 rename2pregLimit;
  static uint32 copyPropLimit;
  static uint32 delRcPULimit;
  static uint32 profileBBHotRate;
  static uint32 profileBBColdRate;
  static bool ignoreIPA;
  static bool aggressiveABCO;
  static bool commonABCO;
  static bool conservativeABCO;
  static bool epreIncludeRef;
  static bool epreLocalRefVar;
  static bool epreLHSIvar;
  static bool dseKeepRef;
  static bool lessThrowAlias;
  static bool noDelegateRC;
  static bool noCondBasedRC;
  static bool enableEA;
  static bool checkCastOpt;
  static bool parmToPtr;
  static bool nullCheckPre;
  static bool assign2FinalPre;
  static bool clinitPre;
  static bool dassignPre;
  static bool mergeStmts;
  static bool generalRegOnly;
  static bool regreadAtReturn;
  static bool propBase;
  static bool propIloadRef;
  static bool propGlobalRef;
  static bool propFinaliLoadRef;
  static bool propIloadRefNonParm;
  static bool propAtPhi;
  static bool propDuringBuild;
  static bool propWithInverse;
  static bool lpreSpeculate;
  static bool lpre4Address;
  static bool lpre4LargeInt;
  static bool spillAtCatch;
  static bool optDirectCall;
  static bool decoupleStatic;
  static bool nativeOpt;
  static bool placementRC;
  static bool subsumRC;
  static bool performFSAA;
  static bool strengthReduction;
  static bool srForAdd;
  static bool doLFTR;
  static std::string inlineFuncList;
  static bool meVerify;
  static uint32 dseRunsLimit;
  static uint32 hdseRunsLimit;
  static uint32 hpropRunsLimit;
  static uint32 sinkLimit;
  static uint32 sinkPULimit;
  static bool loopVec;
  static bool seqVec;
  static uint8 rematLevel;
  static bool layoutWithPredict;
#if MIR_JAVA
  static std::string acquireFuncName;
  static std::string releaseFuncName;
  static unsigned int warningLevel;
  static bool mplToolOnly;
  static bool mplToolStrict;
  static bool skipVirtualMethod;
#endif
 private:
  void DecideMeRealLevel(const std::vector<mapleOption::Option> &inputOptions) const;
};

#ifndef DEBUGFUNC
#define DEBUGFUNC(f)                                                       \
  (MeOption::dumpPhases.find(PhaseName()) != MeOption::dumpPhases.end() && \
   (MeOption::dumpFunc.compare("*") == 0 || (f)->GetName() == MeOption::dumpFunc))
#endif

#ifndef DEBUGFUNC_NEWPM
#define DEBUGFUNC_NEWPM(f)                                                                                  \
  (!MeOption::dumpPhases.empty() && MeOption::dumpPhases.find(PhaseName()) != MeOption::dumpPhases.end() && \
   (MeOption::dumpFunc.compare("*") == 0 || (f).GetName() == MeOption::dumpFunc))
#endif
}  // namespace maple
#endif  // MAPLE_ME_INCLUDE_ME_OPTION_H
