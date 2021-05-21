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
#include <cstring>
#include "mpl_logging.h"
#include "option_parser.h"
#include "string_utils.h"

namespace maple {
using namespace mapleOption;

std::unordered_set<std::string> MeOption::dumpPhases = {};
bool MeOption::dumpAfter = false;
std::string MeOption::dumpFunc = "*";
unsigned long MeOption::range[kRangeArrayLen] = { 0, 0 };
bool MeOption::useRange = false;
bool MeOption::quiet = false;
bool MeOption::setCalleeHasSideEffect = false;
bool MeOption::noSteensgaard = false;
bool MeOption::noTBAA = false;
uint8 MeOption::aliasAnalysisLevel = 3;
bool MeOption::noDot = false;
bool MeOption::stmtNum = false;
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
uint32 MeOption::profileBBHotRate = 10;
uint32 MeOption::profileBBColdRate = 99;
bool MeOption::noDelegateRC = false;
bool MeOption::noCondBasedRC = false;
bool MeOption::clinitPre = true;
bool MeOption::dassignPre = true;
bool MeOption::nullCheckPre = false;
bool MeOption::assign2FinalPre = false;
bool MeOption::epreIncludeRef = false;
bool MeOption::epreLocalRefVar = true;
bool MeOption::epreLHSIvar = true;
bool MeOption::lpreSpeculate = false;
bool MeOption::spillAtCatch = false;
bool MeOption::rcLowering = true;
bool MeOption::optDirectCall = false;
bool MeOption::propAtPhi = true;
bool MeOption::propDuringBuild = true;
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
bool MeOption::strengthReduction = false;
std::string MeOption::inlineFuncList = "";
bool MeOption::meVerify = false;
#if MIR_JAVA
std::string MeOption::acquireFuncName = "Landroid/location/LocationManager;|requestLocationUpdates|";
std::string MeOption::releaseFuncName = "Landroid/location/LocationManager;|removeUpdates|";
unsigned int MeOption::warningLevel = 0;
bool MeOption::mplToolOnly = false;
bool MeOption::mplToolStrict = false;
bool MeOption::skipVirtualMethod = false;
#endif

enum OptionIndex {
  kMeHelp = kCommonOptionEnd + 1,
  kMeDumpPhases,
  kMeSkipPhases,
  kMeDumpFunc,
  kMeQuiet,
  kMeNoDot,
  kMeUseRc,
  kMeStrictNaiveRc,
  kMeStubJniFunc,
  kMeToolOnly,
  kMeToolStrict,
  kMeSkipVirtual,
  kMeNativeOpt,
  kMeSkipFrom,
  kMeSkipAfter,
  kSetCalleeHasSideEffect,
  kNoSteensgaard,
  kNoTBAA,
  kAliasAnalysisLevel,
  kStmtNum,
  kRcLower,
  kNoRcLower,
  kGCOnlyOpt,
  kNoGcbar,
  kUseGcbar,
  kWarnNativefunc,
  kMeDumpBefore,
  kMeDumpAfter,
  kRealCheckcast,
  kMeAcquireFunc,
  kMeReleaseFunc,
  kMeWarnLevel,
  kMeOptL1,
  kMeOptL2,
  kRefUsedCheck,
  kMeRange,
  kEpreLimit,
  kEprepuLimit,
  kStmtPrepuLimit,
  kLpreLimit,
  kLprepulLimit,
  kPregreNameLimit,
  kRename2pregLimit,
  kDelrcpuLimit,
  kProfileBBHotRate,
  kProfileBBColdRate,
  kIgnoreIpa,
  kAggressiveABCO,
  kCommonABCO,
  kConservativeABCO,
  kEpreIncludeRef,
  kNoEpreIncludeRef,
  kEpreLocalRefVar,
  kNoEpreLocalRefVar,
  kEprelhSivar,
  kDseKeepRef,
  kNoDseKeepRef,
  kPropBase,
  kNopropiLoadRef,
  kPropiLoadRef,
  kPropGloablRef,
  kPropfinalIloadRef,
  kPropIloadRefnonparm,
  kLessThrowAlias,
  kNodeLegateRc,
  kNocondBasedRc,
  kCheckCastOpt,
  kNoCheckCastOpt,
  kParmToptr,
  kNullcheckPre,
  kClinitPre,
  kDassignPre,
  kAssign2finalPre,
  kSubsumRC,
  kPerformFSAA,
  kStrengthReduction,
  kRegReadAtReturn,
  kProPatphi,
  kNoProPatphi,
  kPropDuringBuild,
  kOptInterfaceCall,
  kNoOptInterfaceCall,
  kOptVirtualCall,
  kOptDirectCall,
  kEnableEa,
  kLpreSpeculate,
  kNoLpreSpeculate,
  kSpillatCatch,
  kPlacementRC,
  kNoPlacementRC,
  kLazyDecouple,
  kEaTransRef,
  kEaTransAlloc,
  kMeInlineHint,
  kMeThreads,
  kMeIgnoreInferredRetType,
  kMeVerify,
};

const Descriptor kUsage[] = {
  { kMeHelp,
    0,
    "h",
    "help",
    kBuildTypeExperimental,
    kArgCheckPolicyOptional,
    "  -h --help                   \tPrint usage and exit.Available command names:\n"
    "                              \tme\n",
    "me",
    {} },
  { kMeOptL1,
    0,
    "",
    "O1",
    kBuildTypeProduct,
    kArgCheckPolicyOptional,
    "  -O1                         \tDo some optimization.\n",
    "me",
    {} },
  { kMeOptL2,
    0,
    "",
    "O2",
    kBuildTypeProduct,
    kArgCheckPolicyOptional,
    "  -O2                         \tDo some optimization.\n",
    "me",
    {} },
  { kRefUsedCheck,
    0,
    "",
    "refusedcheck",
    kBuildTypeProduct,
    kArgCheckPolicyRequired,
    "  --refusedcheck=FUNCNAME,...    \tEnable ref check used in func in the comma separated list, * means all func.\n",
    "me",
    {} },
  { kMeRange,
    0,
    "",
    "range",
    kBuildTypeDebug,
    kArgCheckPolicyRequired,
    "  --range                     \tOptimize only functions in the range [NUM0, NUM1]\n"
    "                              \t--range=NUM0,NUM1\n",
    "me",
    {} },
  { kMeDumpPhases,
    0,
    "",
    "dump-phases",
    kBuildTypeDebug,
    kArgCheckPolicyRequired,
    "  --dump-phases               \tEnable debug trace for specified phases in the comma separated list\n"
    "                              \t--dump-phases=PHASENAME,...\n",
    "me",
    {} },
  { kMeSkipPhases,
    0,
    "",
    "skip-phases",
    kBuildTypeDebug,
    kArgCheckPolicyRequired,
    "  --skip-phases               \tSkip the phases specified in the comma separated list\n"
    "                              \t--skip-phases=PHASENAME,...\n",
    "me",
    {} },
  { kMeDumpFunc,
    0,
    "",
    "dump-func",
    kBuildTypeDebug,
    kArgCheckPolicyRequired,
    "  --dump-func                 \tDump/trace only for functions whose names contain FUNCNAME as substring\n"
    "                              \t(can only specify once)\n"
    "                              \t--dump-func=FUNCNAME\n",
    "me",
    {} },
  { kMeQuiet,
    kEnable,
    "",
    "quiet",
    kBuildTypeProduct,
    kArgCheckPolicyBool,
    "  --quiet                     \tDisable brief trace messages with phase/function names\n"
    "  --no-quiet                  \tEnable brief trace messages with phase/function names\n",
    "me",
    {} },
  { kMeNoDot,
    kEnable,
    "",
    "nodot",
    kBuildTypeExperimental,
    kArgCheckPolicyBool,
    "  --nodot                     \tDisable dot file generation from cfg\n"
    "  --no-nodot                  \tEnable dot file generation from cfg\n",
    "me",
    {} },
  { kMeUseRc,
    kEnable,
    "",
    "userc",
    kBuildTypeExperimental,
    kArgCheckPolicyBool,
    "  --userc                     \tEnable reference counting [default]\n"
    "  --no-userc                  \tDisable reference counting [default]\n",
    "me",
    {} },
  { kMeStrictNaiveRc,
    kEnable,
    "",
    "strict-naiverc",
    kBuildTypeProduct,
    kArgCheckPolicyBool,
    "  --strict-naiverc            \tStrict Naive RC mode, assume no unsafe multi-thread read/write racing\n"
    "  --no-strict-naiverc         \tDisable Strict Naive RC mode, assume no unsafe multi-thread read/write racing\n",
    "me",
    {} },
  { kMeSkipFrom,
    0,
    "",
    "skip-from",
    kBuildTypeDebug,
    kArgCheckPolicyRequired,
    "  --skip-from                 \tSkip the rest phases from PHASENAME(included)\n"
    "                              \t--skip-from=PHASENAME\n",
    "me",
    {} },
  { kMeSkipAfter,
    0,
    "",
    "skip-after",
    kBuildTypeDebug,
    kArgCheckPolicyRequired,
    "  --skip-after                \tSkip the rest phases after PHASENAME(excluded)\n"
    "                              \t--skip-after=PHASENAME\n",
    "me",
    {} },
  { kSetCalleeHasSideEffect,
    kEnable,
    "",
    "setCalleeHasSideEffect",
    kBuildTypeExperimental,
    kArgCheckPolicyBool,
    "  --setCalleeHasSideEffect    \tSet all the callees have side effect\n"
    "  --no-setCalleeHasSideEffect \tNot set all the callees have side effect\n",
    "me",
    {} },
  { kNoSteensgaard,
    kEnable,
    "",
    "noSteensgaard",
    kBuildTypeExperimental,
    kArgCheckPolicyBool,
    "  --noSteensgaard             \tDisable Steensgaard-style alias analysis\n"
    "  --no-noSteensgaard          \tEnable Steensgaard-style alias analysis\n",
    "me",
    {} },
  { kNoTBAA,
    kEnable,
    "",
    "noTBAA",
    kBuildTypeExperimental,
    kArgCheckPolicyBool,
    "  --noTBAA                    \tDisable type-based alias analysis\n"
    "  --no-noTBAA                 \tEnable type-based alias analysis\n",
    "me",
    {} },
  { kAliasAnalysisLevel,
    0,
    "",
    "aliasAnalysisLevel",
    kBuildTypeExperimental,
    kArgCheckPolicyRequired,
    "  --aliasAnalysisLevel        \tSet level of alias analysis. \n"
    "                              \t0: most conservative;\n"
    "                              \t1: Steensgaard-style alias analysis; 2: type-based alias analysis;\n"
    "                              \t3: Steensgaard-style and type-based alias analysis\n"
    "                              \t--aliasAnalysisLevel=NUM\n",
    "me",
    {} },
  { kStmtNum,
    kEnable,
    "",
    "stmtnum",
    kBuildTypeExperimental,
    kArgCheckPolicyBool,
    "  --stmtnum                   \tPrint MeStmt index number in IR dump\n"
    "  --no-stmtnum                \tDon't print MeStmt index number in IR dump\n",
    "me",
    {} },
  { kRcLower,
    kEnable,
    "",
    "rclower",
    kBuildTypeExperimental,
    kArgCheckPolicyBool,
    "  --rclower                   \tEnable rc lowering\n"
    "  --no-rclower                \tDisable rc lowering\n",
    "me",
    {} },
  { kGCOnlyOpt,
    kEnable,
    "gconlyopt",
    "",
    kBuildTypeProduct,
    kArgCheckPolicyBool,
    "  --gconlyopt                     \tEnable write barrier optimization in gconly\n"
    "  --no-gconlyopt                  \tDisable write barrier optimization in gconly\n",
    "me",
    {} },
  { kUseGcbar,
    kEnable,
    "",
    "usegcbar",
    kBuildTypeExperimental,
    kArgCheckPolicyBool,
    "  --usegcbar                  \tEnable GC barriers\n"
    "  --no-usegcbar               \tDisable GC barriers\n",
    "me",
    {} },
  { kMeStubJniFunc,
    kEnable,
    "",
    "regnativefunc",
    kBuildTypeExperimental,
    kArgCheckPolicyBool,
    "  --regnativefunc             \tGenerate native stub function to support JNI registration and calling\n"
    "  --no-regnativefunc          \tDon't generate native stub function to support JNI registration and calling\n",
    "me",
    {} },
  { kWarnNativefunc,
    kEnable,
    "",
    "warnemptynative",
    kBuildTypeExperimental,
    kArgCheckPolicyBool,
    "  --warnemptynative           \tGenerate warning and abort unimplemented native function\n"
    "  --no-warnemptynative        \tDon't generate warning and abort unimplemented native function\n",
    "me",
    {} },
  { kMeDumpBefore,
    kEnable,
    "",
    "dump-before",
    kBuildTypeDebug,
    kArgCheckPolicyBool,
    "  --dump-before               \tDo extra IR dump before the specified phase in me\n"
    "  --no-dump-before            \tDon't extra IR dump before the specified phase in me\n",
    "me",
    {} },
  { kMeDumpAfter,
    kEnable,
    "",
    "dump-after",
    kBuildTypeDebug,
    kArgCheckPolicyBool,
    "  --dump-after                \tDo extra IR dump after the specified phase in me\n"
    "  --no-dump-after             \tDo not extra IR dump after the specified phase in me\n",
    "me",
    {} },
  { kRealCheckcast,
    kEnable,
    "",
    "realcheckcast",
    kBuildTypeExperimental,
    kArgCheckPolicyBool,
    "  --realcheckcast\n"
    "  --no-realcheckcast\n",
    "me",
    {} },
  { kEpreLimit,
    0,
    "",
    "eprelimit",
    kBuildTypeDebug,
    kArgCheckPolicyRequired,
    "  --eprelimit                 \tApply EPRE optimization only for the first NUM expressions\n"
    "                              \t--eprelimit=NUM\n",
    "me",
    {} },
  { kEprepuLimit,
    0,
    "",
    "eprepulimit",
    kBuildTypeDebug,
    kArgCheckPolicyRequired,
    "  --eprepulimit               \tApply EPRE optimization only for the first NUM PUs\n"
    "                              \t--eprepulimit=NUM\n",
    "me",
    {} },
  { kStmtPrepuLimit,
    0,
    "",
    "stmtprepulimit",
    kBuildTypeDebug,
    kArgCheckPolicyRequired,
    "  --stmtprepulimit            \tApply STMTPRE optimization only for the first NUM PUs\n"
    "                              \t--stmtprepulimit=NUM\n",
    "me",
    {} },
  { kLpreLimit,
    0,
    "",
    "lprelimit",
    kBuildTypeDebug,
    kArgCheckPolicyRequired,
    "  --lprelimit                 \tApply LPRE optimization only for the first NUM variables\n"
    "                              \t--lprelimit=NUM\n",
    "me",
    {} },
  { kLprepulLimit,
    0,
    "",
    "lprepulimit",
    kBuildTypeDebug,
    kArgCheckPolicyRequired,
    "  --lprepulimit               \tApply LPRE optimization only for the first NUM PUs\n"
    "                              \t--lprepulimit=NUM\n",
    "me",
    {} },
  { kPregreNameLimit,
    0,
    "",
    "pregrenamelimit",
    kBuildTypeExperimental,
    kArgCheckPolicyRequired,
    "  --pregrenamelimit           \tApply Preg Renaming optimization only up to NUM times\n"
    "                              \t--pregrenamelimit=NUM\n",
    "me",
    {} },
  { kRename2pregLimit,
    0,
    "",
    "rename2preglimit",
    kBuildTypeExperimental,
    kArgCheckPolicyRequired,
    "  --rename2preglimit          \tApply Rename-to-Preg optimization only up to NUM times\n"
    "                              \t--rename2preglimit=NUM\n",
    "me",
    {} },
  { kDelrcpuLimit,
    0,
    "",
    "delrcpulimit",
    kBuildTypeExperimental,
    kArgCheckPolicyRequired,
    "  --delrcpulimit              \tApply DELEGATERC optimization only for the first NUM PUs\n"
    "                              \t--delrcpulimit=NUM\n",
    "me",
    {} },
  { kProfileBBHotRate,
    0,
    "",
    "profile-bb-hot-rate",
    kBuildTypeAll,
    kArgCheckPolicyRequired,
    "  --profile-bb-hot-rate=10   \tA count is regarded as hot if it is in the largest 10%\n",
    "me",
    {} },
  { kProfileBBColdRate,
    0,
    "",
    "profile-bb-cold-rate",
    kBuildTypeAll,
    kArgCheckPolicyRequired,
    "  --profile-bb-cold-rate=99  \tA count is regarded as cold if it is in the smallest 1%\n",
    "me",
    {} },
  { kIgnoreIpa,
    kEnable,
    "",
    "ignoreipa",
    kBuildTypeProduct,
    kArgCheckPolicyBool,
    "  --ignoreipa                 \tIgnore information provided by interprocedural analysis\n"
    "  --no-ignoreipa              \tDon't ignore information provided by interprocedural analysis\n",
    "me",
    {} },
  { kAggressiveABCO,
    kEnable,
    "",
    "aggressiveABCO",
    kBuildTypeExperimental,
    kArgCheckPolicyBool,
    "  --aggressiveABCO                 \tEnable aggressive array boundary check optimization\n"
    "  --no-aggressiveABCO              \tDon't enable aggressive array boundary check optimization\n",
    "me",
    {} },
  { kCommonABCO,
    kEnable,
    "",
    "commonABCO",
    kBuildTypeExperimental,
    kArgCheckPolicyBool,
    "  --commonABCO                 \tEnable aggressive array boundary check optimization\n"
    "  --no-commonABCO              \tDon't enable aggressive array boundary check optimization\n",
    "me",
    {} },
  { kConservativeABCO,
    kEnable,
    "",
    "conservativeABCO",
    kBuildTypeExperimental,
    kArgCheckPolicyBool,
    "  --conservativeABCO                 \tEnable aggressive array boundary check optimization\n"
    "  --no-conservativeABCO              \tDon't enable aggressive array boundary check optimization\n",
    "me",
    {} },
  { kEpreIncludeRef,
    kEnable,
    "",
    "epreincluderef",
    kBuildTypeExperimental,
    kArgCheckPolicyBool,
    "  --epreincluderef            \tInclude ref-type expressions when performing epre optimization\n"
    "  --no-epreincluderef         \tDon't include ref-type expressions when performing epre optimization\n",
    "me",
    {} },
  { kEpreLocalRefVar,
    kEnable,
    "",
    "eprelocalrefvar",
    kBuildTypeExperimental,
    kArgCheckPolicyBool,
    "  --eprelocalrefvar           \tThe EPRE phase will create new localrefvars when appropriate\n"
    "  --no-eprelocalrefvar        \tDisable the EPRE phase create new localrefvars when appropriate\n",
    "me",
    {} },
  { kEprelhSivar,
    kEnable,
    "",
    "eprelhsivar",
    kBuildTypeExperimental,
    kArgCheckPolicyBool,
    "  --eprelhsivar               \tThe EPRE phase will consider iassigns when optimizing ireads\n"
    "  --no-eprelhsivar            \tDisable the EPRE phase consider iassigns when optimizing ireads\n",
    "me",
    {} },
  { kDseKeepRef,
    kEnable,
    "",
    "dsekeepref",
    kBuildTypeExperimental,
    kArgCheckPolicyBool,
    "  --dsekeepref                \tPreverse dassign of local var that are of ref type to anywhere\n"
    "  --no-dsekeepref             \tDon't preverse dassign of local var that are of ref type to anywhere\n",
    "me",
    {} },
  { kPropBase,
    kEnable,
    "",
    "propbase",
    kBuildTypeExperimental,
    kArgCheckPolicyBool,
    "  --propbase                  \tApply copy propagation that can change the base of indirect memory accesses\n"
    "  --no-propbase               \tDon't apply copy propagation\n",
    "me",
    {} },
  { kPropiLoadRef,
    kEnable,
    "",
    "propiloadref",
    kBuildTypeExperimental,
    kArgCheckPolicyBool,
    "  --propiloadref              \tAllow copy propagating iloads that are of ref type to anywhere\n"
    "  --no-propiloadref           \tDon't aAllow copy propagating iloads that are of ref type to anywhere\n",
    "me",
    {} },
  { kPropGloablRef,
    kEnable,
    "",
    "propglobalref",
    kBuildTypeExperimental,
    kArgCheckPolicyBool,
    "  --propglobalref             \tAllow copy propagating global that are of ref type to anywhere\n"
    "  --no-propglobalref          \tDon't allow copy propagating global that are of ref type to anywhere\n",
    "me",
    {} },
  { kPropfinalIloadRef,
    kEnable,
    "",
    "propfinaliloadref",
    kBuildTypeExperimental,
    kArgCheckPolicyBool,
    "  --propfinaliloadref         \tAllow copy propagating iloads of\n"
    "                              \tfinal fields that are of ref type to anywhere\n"
    "  --no-propfinaliloadref      \tDisable propfinaliloadref\n",
    "me",
    {} },
  { kPropIloadRefnonparm,
    kEnable,
    "",
    "propiloadrefnonparm",
    kBuildTypeExperimental,
    kArgCheckPolicyBool,
    "  --propiloadrefnonparm       \tAllow copy propagating iloads that are of ref type to\n"
    "                              \tanywhere except actual parameters\n"
    "  --no-propiloadrefnonparm    \tDisbale propiloadref\n",
    "me",
    {} },
  { kLessThrowAlias,
    kEnable,
    "",
    "lessthrowalias",
    kBuildTypeExperimental,
    kArgCheckPolicyBool,
    "  --lessthrowalias            \tHandle aliases at java throw statements more accurately\n"
    "  --no-lessthrowalias         \tDisable lessthrowalias\n",
    "me",
    {} },
  { kNodeLegateRc,
    kEnable,
    "",
    "nodelegaterc",
    kBuildTypeExperimental,
    kArgCheckPolicyBool,
    "  --nodelegateerc             \tDo not apply RC delegation to local object reference pointers\n"
    "  --no-nodelegateerc          \tDisable nodelegateerc\n",
    "me",
    {} },
  { kNocondBasedRc,
    kEnable,
    "",
    "nocondbasedrc",
    kBuildTypeExperimental,
    kArgCheckPolicyBool,
    "  --nocondbasedrc             \tDo not apply condition-based RC optimization to\n"
    "                              \tlocal object reference pointers\n"
    "  --no-nocondbasedrc          \tDisable nocondbasedrc\n",
    "me",
    {} },
  { kSubsumRC,
    kEnable,
    "",
    "subsumrc",
    kBuildTypeExperimental,
    kArgCheckPolicyBool,
    "  --subsumrc               \tDelete decrements for localrefvars whose live range is just in\n"
    "                           \tanother which point to the same obj\n"
    "  --no-subsumrc            \tDisable subsumrc\n",
    "me",
    {} },
  { kPerformFSAA,
    kEnable,
    "",
    "performFSAA",
    kBuildTypeExperimental,
    kArgCheckPolicyBool,
    "  --performFSAA            \tPerform flow sensitive alias analysis\n"
    "  --no-performFSAA         \tDisable flow sensitive alias analysis\n",
    "me",
    {} },
  { kStrengthReduction,
    kEnable,
    "",
    "strengthReduction",
    kBuildTypeExperimental,
    kArgCheckPolicyBool,
    "  --strengthReduction      \tPerform flow sensitive alias analysis\n"
    "  --no-strengthReduction   \tDisable flow sensitive alias analysis\n",
    "me",
    {} },
  { kCheckCastOpt,
    kEnable,
    "",
    "checkcastopt",
    kBuildTypeExperimental,
    kArgCheckPolicyBool,
    "  --checkcastopt             \tApply template--checkcast optimization \n"
    "  --no-checkcastopt          \tDisable checkcastopt \n",
    "me",
    {} },
  { kParmToptr,
    kEnable,
    "",
    "parmtoptr",
    kBuildTypeExperimental,
    kArgCheckPolicyBool,
    "  --parmtoptr                 \tAllow rcoptlocals to change actual parameters from ref to ptr type\n"
    "  --no-parmtoptr              \tDisable parmtoptr\n",
    "me",
    {} },
  { kNullcheckPre,
    kEnable,
    "",
    "nullcheckpre",
    kBuildTypeExperimental,
    kArgCheckPolicyBool,
    "  --nullcheckpre              \tTurn on partial redundancy elimination of null pointer checks\n"
    "  --no-nullcheckpre           \tDisable nullcheckpre\n",
    "me",
    {} },
  { kClinitPre,
    kEnable,
    "",
    "clinitpre",
    kBuildTypeExperimental,
    kArgCheckPolicyBool,
    "  --clinitpre                 \tTurn on partial redundancy elimination of class initialization checks\n"
    "  --no-clinitpre              \tDisable clinitpre\n",
    "me",
    {} },
  { kDassignPre,
    kEnable,
    "",
    "dassignpre",
    kBuildTypeExperimental,
    kArgCheckPolicyBool,
    "  --dassignpre                \tTurn on partial redundancy elimination of assignments to scalar variables\n"
    "  --no-dassignpre             \tDisable dassignpre\n",
    "me",
    {} },
  { kAssign2finalPre,
    kEnable,
    "",
    "assign2finalpre",
    kBuildTypeExperimental,
    kArgCheckPolicyBool,
    "  --assign2finalpre           \tTurn on partial redundancy elimination of assignments to final variables\n"
    "  --no-assign2finalpre        \tDisable assign2finalpre\n",
    "me",
    {} },
  { kRegReadAtReturn,
    kEnable,
    "",
    "regreadatreturn",
    kBuildTypeExperimental,
    kArgCheckPolicyBool,
    "  --regreadatreturn           \tAllow register promotion to promote the operand of return statements\n"
    "  --no-regreadatreturn        \tDisable regreadatreturn\n",
    "me",
    {} },
  { kProPatphi,
    kEnable,
    "",
    "propatphi",
    kBuildTypeExperimental,
    kArgCheckPolicyBool,
    "  --propatphi                 \tEnable copy propagation across phi nodes\n"
    "  --no-propatphi              \tDisable propatphi\n",
    "me",
    {} },
  { kPropDuringBuild,
    kEnable,
    "",
    "propduringbuild",
    kBuildTypeExperimental,
    kArgCheckPolicyBool,
    "  --propduringbuild           \tEnable copy propagation when building HSSA\n"
    "  --no-propduringbuild        \tDisable propduringbuild\n",
    "me",
    {} },
  { kMeNativeOpt,
    kEnable,
    "",
    "nativeopt",
    kBuildTypeProduct,
    kArgCheckPolicyBool,
    "  --nativeopt              \tEnable native opt\n"
    "  --no-nativeopt           \tDisable native opt\n",
    "me",
    {} },
  { kOptDirectCall,
    kEnable,
    "",
    "optdirectcall",
    kBuildTypeExperimental,
    kArgCheckPolicyBool,
    "  --optdirectcall             \tEnable redundancy elimination of directcalls\n"
    "  --no-optdirectcall          \tDisable optdirectcall\n",
    "me",
    {} },
  { kEnableEa,
    kEnable,
    "",
    "enable-ea",
    kBuildTypeProduct,
    kArgCheckPolicyBool,
    "  --enable-ea                 \tEnable escape analysis\n"
    "  --no-enable-ea              \tDisable escape analysis\n",
    "me",
    {} },
  { kLpreSpeculate,
    kEnable,
    "",
    "lprespeculate",
    kBuildTypeExperimental,
    kArgCheckPolicyBool,
    "  --lprespeculate             \tEnable speculative code motion in LPRE phase\n"
    "  --no-lprespeculate          \tDisable speculative code motion in LPRE phase\n",
    "me",
    {} },
  { kSpillatCatch,
    kEnable,
    "",
    "spillatcatch",
    kBuildTypeExperimental,
    kArgCheckPolicyBool,
    "  --spillatcatch              \tMinimize upward exposed preg usage in catch blocks\n"
    "  --no-spillatcatch           \tDisable spillatcatch\n",
    "me",
    {} },
  { kPlacementRC,
    kEnable,
    "",
    "placementrc",
    kBuildTypeExperimental,
    kArgCheckPolicyBool,
    "  --placementrc               \tInsert RC decrements for localrefvars using\n"
    "                              \tthe placement optimization approach\n"
    "  --no-placementrc            \tInsert RC decrements for localrefvars using\n",
    "me",
    {} },
  { kLazyDecouple,
    kEnable,
    "",
    "lazydecouple",
    kBuildTypeExperimental,
    kArgCheckPolicyBool,
    "  --lazydecouple              \tDo optimized for lazy Decouple\n"
    "  --no-lazydecouple           \tDo not optimized for lazy Decouple\n",
    "me",
    {} },
  { kMeInlineHint,
    0,
    "",
    "inlinefunclist",
    kBuildTypeProduct,
    kArgCheckPolicyRequired,
    "  --inlinefunclist            \tInlining related configuration\n"
    "                              \t--inlinefunclist=\n",
    "me",
    {} },
  { kMeThreads,
    0,
    "",
    "threads",
    kBuildTypeExperimental,
    kArgCheckPolicyNumeric,
    "  --threads=n                 \tOptimizing me functions using n threads\n"
    "                              \tIf n >= 2, ignore-inferred-return-type will be enabled default\n",
    "me",
    {} },
  { kMeIgnoreInferredRetType,
    kEnable,
    "",
    "ignore-inferred-ret-type",
    kBuildTypeExperimental,
    kArgCheckPolicyBool,
    "  --ignore-inferred-ret-type  \tIgnore func return type inferred by ssadevirt\n"
    "  --no-ignore-inferred-ret-type\tDo not ignore func return type inferred by ssadevirt\n",
    "me",
    {} },
  { kMeVerify,
    kEnable,
    "",
    "meverify",
    kBuildTypeProduct,
    kArgCheckPolicyNone,
    "  --meverify                       \tenable meverify features\n",
    "me",
    {}},
#if MIR_JAVA
  { kMeAcquireFunc,
    0,
    "",
    "acquire-func",
    kBuildTypeExperimental,
    kArgCheckPolicyRequired,
    "  --acquire-func              \t--acquire-func=FUNCNAME\n",
    "me",
    {} },
  { kMeReleaseFunc,
    0,
    "",
    "release-func",
    kBuildTypeExperimental,
    kArgCheckPolicyRequired,
    "  --release-func              \t--release-func=FUNCNAME\n",
    "me",
    {} },
  { kMeToolOnly,
    kEnable,
    "",
    "toolonly",
    kBuildTypeExperimental,
    kArgCheckPolicyBool,
    "  --toolonly\n"
    "  --no-toolonly\n",
    "me",
    {} },
  { kMeToolStrict,
    kEnable,
    "",
    "toolstrict",
    kBuildTypeExperimental,
    kArgCheckPolicyBool,
    "  --toolstrict\n"
    "  --no-toolstrict\n",
    "me",
    {} },
  { kMeSkipVirtual,
    kEnable,
    "",
    "skipvirtual",
    kBuildTypeExperimental,
    kArgCheckPolicyBool,
    "  --skipvirtual\n"
    "  --no-skipvirtual\n",
    "me",
    {} },
  { kMeWarnLevel,
    0,
    "",
    "warning",
    kBuildTypeExperimental,
    kArgCheckPolicyNumeric,
    "  --warning=level             \t--warning=level\n",
    "me",
    {} },
#endif
  { kUnknown,
    0,
    "",
    "",
    kBuildTypeAll,
    kArgCheckPolicyNone,
    "",
    "me",
    {} }
};

void MeOption::DecideMeRealLevel(const std::vector<mapleOption::Option> &inputOptions) const {
  int realLevel = -1;
  for (const mapleOption::Option &opt : inputOptions) {
    switch (opt.Index()) {
      case kMeOptL1:
        realLevel = kLevelOne;
        break;
      case kMeOptL2:
        realLevel = kLevelTwo;
        break;
      default:
        break;
    }
  }
  if (realLevel == kLevelOne) {
    optLevel = kLevelOne;
  } else if (realLevel == kLevelTwo) {
    optLevel = kLevelTwo;
    // Turn the followings ON only at O2
    optDirectCall = true;
    placementRC = true;
    subsumRC = true;
    epreIncludeRef = true;
  }
}

bool MeOption::SolveOptions(const std::vector<mapleOption::Option> &opts, bool isDebug) {
  DecideMeRealLevel(opts);
  if (isDebug) {
    LogInfo::MapleLogger() << "Real Me level:" << std::to_string(optLevel) << "\n";
  }
  bool result = true;
  for (const mapleOption::Option &opt : opts) {
    if (isDebug) {
      LogInfo::MapleLogger() << "Me options: " << opt.Index() << " " << opt.OptionKey() << " " << opt.Args() << '\n';
    }
    switch (opt.Index()) {
      case kMeSkipPhases:
        SplitSkipPhases(opt.Args());
        break;
      case kMeOptL1:
        // Already handled above in DecideMeRealLevel
        break;
      case kMeOptL2:
        // Already handled above in DecideMeRealLevel
        break;
      case kRefUsedCheck:
        SplitPhases(opt.Args(), checkRefUsedInFuncs);
        break;
      case kMeRange:
        useRange = true;
        result = GetRange(opt.Args());
        break;
      case kMeDumpBefore:
        dumpBefore = (opt.Type() == kEnable);
        break;
      case kMeDumpAfter:
        dumpAfter = (opt.Type() == kEnable);
        break;
      case kMeDumpFunc:
        dumpFunc = opt.Args();
        break;
      case kMeSkipFrom:
        skipFrom = opt.Args();
        break;
      case kMeSkipAfter:
        skipAfter = opt.Args();
        break;
      case kMeDumpPhases:
        SplitPhases(opt.Args(), dumpPhases);
        break;
      case kMeQuiet:
        quiet = (opt.Type() == kEnable);
        break;
      case kVerbose:
        quiet = (opt.Type() == kEnable) ? false : true;
        break;
      case kSetCalleeHasSideEffect:
        setCalleeHasSideEffect = (opt.Type() == kEnable);
        break;
      case kNoSteensgaard:
        noSteensgaard = (opt.Type() == kEnable);
        break;
      case kNoTBAA:
        noTBAA = (opt.Type() == kEnable);
        break;
      case kAliasAnalysisLevel:
        aliasAnalysisLevel = std::stoul(opt.Args(), nullptr);
        if (aliasAnalysisLevel > kLevelThree) {
          aliasAnalysisLevel = kLevelThree;
        }
        switch (aliasAnalysisLevel) {
          case kLevelThree:
            setCalleeHasSideEffect = false;
            noSteensgaard = false;
            noTBAA = false;
            break;
          case kLevelZero:
            setCalleeHasSideEffect = true;
            noSteensgaard = true;
            noTBAA = true;
            break;
          case kLevelOne:
            setCalleeHasSideEffect = false;
            noSteensgaard = false;
            noTBAA = true;
            break;
          case kLevelTwo:
            setCalleeHasSideEffect = false;
            noSteensgaard = true;
            noTBAA = false;
            break;
          default:
            break;
        }
        if (isDebug) {
          LogInfo::MapleLogger() << "--sub options: setCalleeHasSideEffect "
                                 << setCalleeHasSideEffect << '\n';
          LogInfo::MapleLogger() << "--sub options: noSteensgaard " << noSteensgaard << '\n';
          LogInfo::MapleLogger() << "--sub options: noTBAA " << noTBAA << '\n';
        }
        break;
      case kRcLower:
        rcLowering = (opt.Type() == kEnable);
        break;
      case kMeUseRc:
        noRC = (opt.Type() == kDisable);
        break;
      case kLazyDecouple:
        lazyDecouple = (opt.Type() == kEnable);
        break;
      case kMeStrictNaiveRc:
        strictNaiveRC = (opt.Type() == kEnable);
        break;
      case kGCOnly:
        gcOnly = (opt.Type() == kEnable);
        propIloadRef = (opt.Type() == kEnable);
        if (isDebug) {
          LogInfo::MapleLogger() << "--sub options: propIloadRef " << propIloadRef << '\n';
          LogInfo::MapleLogger() << "--sub options: propGlobalRef " << propGlobalRef << '\n';
        }
        break;
      case kGCOnlyOpt:
        gcOnlyOpt = (opt.Type() == kEnable);
        break;
      case kUseGcbar:
        noGCBar = (opt.Type() == kEnable) ? false : true;
        break;
      case kRealCheckcast:
        realCheckCast = (opt.Type() == kEnable);
        break;
      case kMeNoDot:
        noDot = (opt.Type() == kEnable);
        break;
      case kStmtNum:
        stmtNum = (opt.Type() == kEnable);
        break;
      case kMeStubJniFunc:
        regNativeFunc = (opt.Type() == kEnable);
        break;
      case kWarnNativefunc:
        warnNativeFunc = (opt.Type() == kEnable);
        break;
      case kEpreLimit:
        epreLimit = std::stoul(opt.Args(), nullptr);
        break;
      case kEprepuLimit:
        eprePULimit = std::stoul(opt.Args(), nullptr);
        break;
      case kStmtPrepuLimit:
        stmtprePULimit = std::stoul(opt.Args(), nullptr);
        break;
      case kLpreLimit:
        lpreLimit = std::stoul(opt.Args(), nullptr);
        break;
      case kLprepulLimit:
        lprePULimit = std::stoul(opt.Args(), nullptr);
        break;
      case kPregreNameLimit:
        pregRenameLimit = std::stoul(opt.Args(), nullptr);
        break;
      case kRename2pregLimit:
        rename2pregLimit = std::stoul(opt.Args(), nullptr);
        break;
      case kDelrcpuLimit:
        delRcPULimit = std::stoul(opt.Args(), nullptr);
        break;
      case kProfileBBHotRate:
        profileBBHotRate = std::stoul(opt.Args(), nullptr);
        break;
      case kProfileBBColdRate:
        profileBBColdRate = std::stoul(opt.Args(), nullptr);
        break;
      case kIgnoreIpa:
        ignoreIPA = (opt.Type() == kEnable);
        break;
      case kAggressiveABCO:
        aggressiveABCO = (opt.Type() == kEnable);
        break;
      case kCommonABCO:
        commonABCO = (opt.Type() == kEnable);
        break;
      case kConservativeABCO:
        conservativeABCO = (opt.Type() == kEnable);
        break;
      case kEpreIncludeRef:
        epreIncludeRef = (opt.Type() == kEnable);
        break;
      case kEpreLocalRefVar:
        epreLocalRefVar = (opt.Type() == kEnable);
        break;
      case kEprelhSivar:
        epreLHSIvar = (opt.Type() == kEnable);
        break;
      case kDseKeepRef:
        dseKeepRef = (opt.Type() == kEnable);
        break;
      case kLessThrowAlias:
        lessThrowAlias = (opt.Type() == kEnable);
        break;
      case kPropBase:
        propBase = (opt.Type() == kEnable);
        break;
      case kPropiLoadRef:
        propIloadRef = (opt.Type() == kEnable);
        if (opt.Type() == kEnable) {
          propIloadRefNonParm = false;  // to override previous -propIloadRefNonParm
        }
        if (isDebug) {
          LogInfo::MapleLogger() << "--sub options: propIloadRefNonParm " << propIloadRefNonParm << '\n';
        }
        break;
      case kPropGloablRef:
        propGlobalRef = (opt.Type() == kEnable);
        break;
      case kPropfinalIloadRef:
        propFinaliLoadRef = (opt.Type() == kEnable);
        break;
      case kPropIloadRefnonparm:
        propIloadRefNonParm = (opt.Type() == kEnable);
        propIloadRef = (opt.Type() == kEnable);
        if (isDebug) {
          LogInfo::MapleLogger() << "--sub options: propIloadRef " << propIloadRef << '\n';
        }
        break;
      case kNodeLegateRc:
        noDelegateRC = (opt.Type() == kEnable);
        break;
      case kNocondBasedRc:
        noCondBasedRC = (opt.Type() == kEnable);
        break;
      case kCheckCastOpt:
        checkCastOpt = (opt.Type() == kEnable);
        break;
      case kParmToptr:
        parmToPtr = (opt.Type() == kEnable);
        break;
      case kNullcheckPre:
        nullCheckPre = (opt.Type() == kEnable);
        break;
      case kClinitPre:
        clinitPre = (opt.Type() == kEnable);
        break;
      case kDassignPre:
        dassignPre = (opt.Type() == kEnable);
        break;
      case kAssign2finalPre:
        assign2FinalPre = (opt.Type() == kEnable);
        break;
      case kRegReadAtReturn:
        regreadAtReturn = (opt.Type() == kEnable);
        break;
      case kProPatphi:
        propAtPhi = (opt.Type() == kEnable);
        break;
      case kPropDuringBuild:
        propDuringBuild = (opt.Type() == kEnable);
        break;
      case kMeNativeOpt:
        nativeOpt = (opt.Type() == kEnable);
        break;
      case kOptDirectCall:
        optDirectCall = (opt.Type() == kEnable);
        break;
      case kEnableEa:
        enableEA = (opt.Type() == kEnable);
        break;
      case kLpreSpeculate:
        lpreSpeculate = (opt.Type() == kEnable);
        break;
      case kSpillatCatch:
        spillAtCatch = (opt.Type() == kEnable);
        break;
      case kPlacementRC:
        placementRC = (opt.Type() == kEnable);
        if (opt.Type() == kDisable) {
          subsumRC = false;
          epreIncludeRef = false;
          if (isDebug) {
            LogInfo::MapleLogger() << "--sub options: subsumRC " << subsumRC << '\n';
            LogInfo::MapleLogger() << "--sub options: epreIncludeRef " << epreIncludeRef << '\n';
          }
        }
        break;
      case kSubsumRC:
        subsumRC = (opt.Type() == kEnable);
        epreIncludeRef = (opt.Type() == kEnable);
        break;
      case kPerformFSAA:
        performFSAA = (opt.Type() == kEnable);
        break;
      case kStrengthReduction:
        strengthReduction = (opt.Type() == kEnable);
        break;
      case kMeInlineHint:
        inlineFuncList = opt.Args();
        break;
      case kDecoupleStatic:
        decoupleStatic = (opt.Type() == kEnable);
        break;
      case kMeThreads:
        threads = std::stoul(opt.Args(), nullptr);
        break;
      case kMeIgnoreInferredRetType:
        ignoreInferredRetType = (opt.Type() == kEnable);
        break;
      case kMeVerify:
        meVerify = (opt.Type() == kEnable);
        break;
#if MIR_JAVA
      case kMeAcquireFunc:
        acquireFuncName = opt.Args();
        break;
      case kMeReleaseFunc:
        releaseFuncName = opt.Args();
        break;
      case kMeWarnLevel:
        warningLevel = std::stoul(opt.Args());
        break;
      case kMeToolOnly:
        mplToolOnly = (opt.Type() == kEnable);
        break;
      case kMeToolStrict:
        mplToolStrict = (opt.Type() == kEnable);
        break;
      case kMeSkipVirtual:
        skipVirtualMethod = (opt.Type() == kEnable);
        break;
#endif
      default:
        WARN(kLncWarn, "input invalid key for me " + opt.OptionKey());
        break;
    }
  }
  return result;
}

MeOption &MeOption::GetInstance() {
  static MeOption instance;
  return instance;
}

MeOption::MeOption() {
  CreateUsages(kUsage);
}

void MeOption::ParseOptions(int argc, char **argv, std::string &fileName) {
  OptionParser optionParser;
  optionParser.RegisteUsages(DriverOptionCommon::GetInstance());
  optionParser.RegisteUsages(MeOption::GetInstance());
  int ret = optionParser.Parse(argc, argv, "me");
  CHECK_FATAL(ret == kErrorNoError, "option parser error");
  bool result = SolveOptions(optionParser.GetOptions(), false);
  if (!result) {
    return;
  }
  if (optionParser.GetNonOptionsCount() != 1) {
    LogInfo::MapleLogger() << "expecting one .mpl file as last argument, found: ";
    for (std::string optionArg : optionParser.GetNonOptions()) {
      LogInfo::MapleLogger() << optionArg << " ";
    }
    LogInfo::MapleLogger() << '\n';
    CHECK_FATAL(false, "option parser error");
  }
  fileName = optionParser.GetNonOptions().front();
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
} // namespace maple
