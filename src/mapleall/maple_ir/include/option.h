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
#ifndef MAPLE_IR_INCLUDE_OPTION_H
#define MAPLE_IR_INCLUDE_OPTION_H
#include <string>
#include <vector>
#include "mempool.h"
#include "mempool_allocator.h"
#include "parser_opt.h"
#include "types_def.h"
#include "driver_option_common.h"
#include "option_descriptor.h"

namespace maple {
class Options : public MapleDriverOptionBase {
 public:
  static Options &GetInstance();

  Options();

  bool ParseOptions(int argc, char **argv, std::string &fileName) const;

  bool SolveOptions(const std::deque<mapleOption::Option> &opts, bool isDebug) const;
  ~Options() = default;

  void DumpOptions() const;
  const std::vector<std::string> &GetSequence() const {
    return phaseSeq;
  }

  std::string LastPhaseName() const {
    return phaseSeq.empty() ? "noopt" : phaseSeq[phaseSeq.size() - 1];
  }

  enum Level {
    kMpl2MplLevelZero = 0,
    kMpl2MplLevelOne = 1,
    kMpl2MplLevelTwo = 2
  };
  enum DecoupleLevel {
    kNoDecouple = 0,
    kConservativeDecouple = 1,
    kAggressiveDecouple = 2,
    kDecoupleAndLazy = 3
  };

  static bool DumpPhase(const std::string &phase) {
    if (phase == "") {
      return false;
    }
    return dumpPhase == "*" || dumpPhase == phase;
  }

  static bool IsSkipPhase(const std::string &phaseName) {
    return skipPhase == phaseName;
  }

  static bool DumpFunc() {
    return dumpFunc != "*" && dumpFunc != "";
  }
  static bool dumpBefore;
  static bool dumpAfter;
  static std::string dumpPhase;
  static std::string skipPhase;
  static std::string skipFrom;
  static std::string skipAfter;
  static std::string dumpFunc;
  static bool quiet;
  static bool regNativeFunc;
  static bool regNativeDynamicOnly;
  static bool nativeWrapper;
  static bool inlineWithProfile;
  static bool useInline;
  static std::string noInlineFuncList;
  static std::string importFileList;
  static bool useCrossModuleInline;
  static uint32 inlineSmallFunctionThreshold;
  static uint32 inlineHotFunctionThreshold;
  static uint32 inlineRecursiveFunctionThreshold;
  static uint32 inlineDepth;
  static uint32 inlineModuleGrowth;
  static uint32 inlineColdFunctionThreshold;
  static uint32 profileHotCount;
  static uint32 profileColdCount;
  static bool profileHotCountSeted;
  static bool profileColdCountSeted;
  static uint32 profileHotRate;
  static uint32 profileColdRate;
  static std::string staticBindingList;
  static bool usePreg;
  static bool mapleLinker;
  static bool dumpMuidFile;
  static bool emitVtableImpl;
#if MIR_JAVA
  static bool skipVirtualMethod;
#endif
  // Ready to be deleted.
  static bool noRC;
  static bool analyzeCtor;
  static bool strictNaiveRC;
  static bool gcOnly;
  static bool rcOpt1;
  static std::string classMetaProFile;
  static std::string methodMetaProfile;
  static std::string fieldMetaProFile;
  static std::string reflectStringProFile;
  static bool nativeOpt;
  static bool O2;
  static bool noDot;
  static bool decoupleStatic;
  static std::string criticalNativeFile;
  static std::string fastNativeFile;
  static bool barrier;
  static std::string nativeFuncPropertyFile;
  static bool mapleLinkerTransformLocal;
  static uint32 buildApp;
  static bool partialAot;
  static uint32 decoupleInit;
  static std::string sourceMuid;
  static bool decoupleSuper;
  static bool deferredVisit;
  static bool deferredVisit2;
  static bool genVtabAndItabForDecouple;
  static bool profileFunc;
  static uint32 parserOpt;
  static std::string dumpDevirtualList;
  static std::string readDevirtualList;
  static bool usePreloadedClass;
  static std::string profile;
  static std::string appPackageName;
  static std::string proFileData;
  static std::string proFileFuncData;
  static std::string proFileClassData;
  static bool profileStaticFields;
  static bool genIRProfile;
  static bool profileTest;
  static std::string classLoaderInvocationList;
  static bool dumpClassLoaderInvocation;
  static unsigned int warningLevel;
  static bool lazyBinding;
  static bool hotFix;
  static bool compactMeta;
  static bool genPGOReport;
  static bool verify;
  static uint32 inlineCache;
  static bool checkArrayStore;
  static bool noComment;
  static bool rmNoUseFunc;
  static bool sideEffect;
 private:
  void DecideMpl2MplRealLevel(const std::deque<mapleOption::Option> &inputOptions) const;
  std::vector<std::string> phaseSeq;
};
}  // namespace maple
#ifndef TRACE_PHASE
#define TRACE_PHASE (Options::dumpPhase.compare(PhaseName()) == 0)
#endif

#ifndef TRACE_MAPLE_PHASE
#define TRACE_MAPLE_PHASE (Options::dumpPhase.compare(PhaseName()) == 0)
#endif
#endif  // MAPLE_IR_INCLUDE_OPTION_H
