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
#include "cg_option.h"
#include <fstream>
#include <string>
#include <unordered_map>
#include "mpl_logging.h"
#include "parser_opt.h"
#include "mir_parser.h"
#include "option_parser.h"
#include "string_utils.h"

namespace maplebe {
using namespace maple;
using namespace mapleOption;

const std::string kMplcgVersion = "";

bool CGOptions::dumpBefore = false;
bool CGOptions::dumpAfter = false;
bool CGOptions::timePhases = false;
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
uint8 CGOptions::rematLevel = 0;
#if TARGAARCH64 || TARGRISCV64
bool CGOptions::useBarriersForVolatile = false;
#else
bool CGOptions::useBarriersForVolatile = true;
#endif
bool CGOptions::exclusiveEH = false;
bool CGOptions::doEBO = false;
bool CGOptions::doCFGO = false;
bool CGOptions::doICO = false;
bool CGOptions::doStoreLoadOpt = false;
bool CGOptions::doGlobalOpt = false;
bool CGOptions::doVregRename = false;
bool CGOptions::doMultiPassColorRA = true;
bool CGOptions::doPrePeephole = false;
bool CGOptions::doPeephole = false;
bool CGOptions::doSchedule = false;
bool CGOptions::doWriteRefFieldOpt = false;
bool CGOptions::dumpOptimizeCommonLog = false;
bool CGOptions::checkArrayStore = false;
bool CGOptions::doPIC = false;
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
bool CGOptions::useFramePointer = false;
bool CGOptions::gcOnly = false;
bool CGOptions::quiet = false;
bool CGOptions::doPatchLongBranch = false;
bool CGOptions::doPreSchedule = false;
bool CGOptions::emitBlockMarker = true;
bool CGOptions::inRange = false;
bool CGOptions::doPreLSRAOpt = false;
bool CGOptions::doLocalRefSpill = false;
bool CGOptions::doCalleeToSpill = false;
bool CGOptions::replaceASM = false;
bool CGOptions::generalRegOnly = false;
bool CGOptions::fastMath = false;

enum OptionIndex : uint64 {
  kCGQuiet = kCommonOptionEnd + 1,
  kPie,
  kPic,
  kCGVerbose,
  kCGVerboseCG,
  kCGMapleLinker,
  kCgen,
  kEbo,
  kCfgo,
  kIco,
  kSlo,
  kGo,
  kPreLSRAOpt,
  kLocalrefSpill,
  kOptCallee,
  kPrepeep,
  kPeep,
  kPreSchedule,
  kSchedule,
  kVregRename,
  kMultiPassRA,
  kWriteRefFieldOpt,
  kDumpOlog,
  kCGNativeOpt,
  kInsertCall,
  kTrace,
  kCGClassList,
  kGenDef,
  kGenGctib,
  kCGBarrier,
  kGenPrimorList,
  kRaLinear,
  kRaColor,
  kPatchBranch,
  kConstFoldOpt,
  kSuppressFinfo,
  kEhList,
  kObjMap,
  kCGDumpcfg,
  kCGDumpBefore,
  kCGDumpAfter,
  kCGTimePhases,
  kCGDumpFunc,
  kDebuggingInfo,
  kStackGuard,
  kDebugGenDwarf,
  kDebugUseSrc,
  kDebugUseMix,
  kDebugAsmMix,
  kProfilingInfo,
  kProfileEnable,
  kLSRABB,
  kLSRAInsn,
  kLSRAOverlap,
  kRaRemat,
  kCGO0,
  kCGO1,
  kCGO2,
  kProepilogue,
  kYieldPoing,
  kLocalRc,
  kCGRange,
  kFastAlloc,
  kSpillRange,
  kDuplicateBB,
  kCalleeCFI,
  kCyclePatternList,
  kDuplicateToDelPlt,
  kDuplicateToDelPlt2,
  kReplaceAsm,
  kUseGeneralRegOnly,
  kEmitBlockMarker,
  kInsertSoe,
  kCheckArrayStore,
  kPrintFunction,
  kCGDumpPhases,
  kCGSkipPhases,
  kCGSkipFrom,
  kCGSkipAfter,
  kCGLazyBinding,
  kCGHotFix,
  kDebugSched,
  kBruteForceSched,
  kSimulateSched,
  kCrossLoc,
  kABIType,
  kEmitFileType,
  kLongCalls,
  kFunctionSections,
  kOmitFramePointer,
  kFastMath,
  kTailCall,
};

const Descriptor kUsage[] = {
  { kPie,
    kEnable,
    "",
    "pie",
    kBuildTypeProduct,
    kArgCheckPolicyBool,
    "  --pie                       \tGenerate position-independent executable\n"
    "  --no-pie\n",
    "mplcg",
    {} },
  { kPic,
    kEnable,
    "",
    "fpic",
    kBuildTypeProduct,
    kArgCheckPolicyBool,
    "  --fpic                      \tGenerate position-independent shared library\n"
    "  --no-fpic\n",
    "mplcg",
    {} },
  { kCGVerbose,
    kEnable,
    "",
    "verbose-asm",
    kBuildTypeProduct,
    kArgCheckPolicyBool,
    "  --verbose-asm               \tAdd comments to asm output\n"
    "  --no-verbose-asm\n",
    "mplcg",
    {} },
  { kCGVerboseCG,
    kEnable,
    "",
    "verbose-cg",
    kBuildTypeProduct,
    kArgCheckPolicyBool,
    "  --verbose-cg               \tAdd comments to cg output\n"
    "  --no-verbose-cg\n",
    "mplcg",
    {} },
  { kCGMapleLinker,
    kEnable,
    "",
    "maplelinker",
    kBuildTypeProduct,
    kArgCheckPolicyBool,
    "  --maplelinker               \tGenerate the MapleLinker .s format\n"
    "  --no-maplelinker\n",
    "mplcg",
    {} },
  { kCGQuiet,
    kEnable,
    "",
    "quiet",
    kBuildTypeExperimental,
    kArgCheckPolicyBool,
    "  --quiet                     \tBe quiet (don't output debug messages)\n"
    "  --no-quiet\n",
    "mplcg",
    {} },
  { kCgen,
    kEnable,
    "",
    "cg",
    kBuildTypeExperimental,
    kArgCheckPolicyBool,
    "  --cg                        \tGenerate the output .s file\n"
    "  --no-cg\n",
    "mplcg",
    {} },
  { kReplaceAsm,
    kEnable,
    "",
    "replaceasm",
    kBuildTypeProduct,
    kArgCheckPolicyBool,
    "  --replaceasm                \tReplace the the assembly code\n"
    "  --no-replaceasm\n",
    "mplcg",
    {} },
  { kUseGeneralRegOnly,
    kEnable,
    "",
    "general-reg-only",
    kBuildTypeProduct,
    kArgCheckPolicyBool,
    " --general-reg-only           \tdisable floating-point or Advanced SIMD registers\n"
    " --no-general-reg-only\n",
    "mplcg",
    {} },
  { kCGLazyBinding,
    kEnable,
    "",
    "lazy-binding",
    kBuildTypeProduct,
    kArgCheckPolicyBool,
    "  --lazy-binding              \tBind class symbols lazily[default off]\n",
    "mplcg",
    {} },
  { kCGHotFix,
    kEnable,
    "",
    "hot-fix",
    kBuildTypeExperimental,
    kArgCheckPolicyBool,
    "  --hot-fix                   \tOpen for App hot fix[default off]\n"
    "  --no-hot-fix\n",
    "mplcg",
    {} },
  { kEbo,
    kEnable,
    "",
    "ebo",
    kBuildTypeExperimental,
    kArgCheckPolicyBool,
    "  --ebo                       \tPerform Extend block optimization\n"
    "  --no-ebo\n",
    "mplcg",
    {} },
  { kCfgo,
    kEnable,
    "",
    "cfgo",
    kBuildTypeExperimental,
    kArgCheckPolicyBool,
    "  --cfgo                      \tPerform control flow optimization\n"
    "  --no-cfgo\n",
    "mplcg",
    {} },
  { kIco,
    kEnable,
    "",
    "ico",
    kBuildTypeExperimental,
    kArgCheckPolicyBool,
    "  --ico                       \tPerform if-conversion optimization\n"
    "  --no-ico\n",
    "mplcg",
    {} },
  { kSlo,
    kEnable,
    "",
    "storeloadopt",
    kBuildTypeExperimental,
    kArgCheckPolicyBool,
    "  --storeloadopt              \tPerform global store-load optimization\n"
    "  --no-storeloadopt\n",
    "mplcg",
    {} },
  { kGo,
    kEnable,
    "",
    "globalopt",
    kBuildTypeExperimental,
    kArgCheckPolicyBool,
    "  --globalopt                 \tPerform global optimization\n"
    "  --no-globalopt\n",
    "mplcg",
    {} },
  { kPreLSRAOpt,
    kEnable,
    "",
    "prelsra",
    kBuildTypeExperimental,
    kArgCheckPolicyBool,
    "  --prelsra                   \tPerform live interval simplification in LSRA\n"
    "  --no-prelsra\n",
    "mplcg",
    {} },
  { kLocalrefSpill,
    kEnable,
    "",
    "lsra-lvarspill",
    kBuildTypeExperimental,
    kArgCheckPolicyBool,
    "  --lsra-lvarspill            \tPerform LSRA spill using local ref var stack locations\n"
    "  --no-lsra-lvarspill\n",
    "mplcg",
    {} },
  { kOptCallee,
    kEnable,
    "",
    "lsra-optcallee",
    kBuildTypeExperimental,
    kArgCheckPolicyBool,
    "  --lsra-optcallee            \tSpill callee if only one def to use\n"
    "  --no-lsra-optcallee\n",
    "mplcg",
    {} },
  { kPrepeep,
    kEnable,
    "",
    "prepeep",
    kBuildTypeExperimental,
    kArgCheckPolicyBool,
    "  --prepeep                   \tPerform peephole optimization before RA\n"
    "  --no-prepeep\n",
    "mplcg",
    {} },
  { kPeep,
    kEnable,
    "",
    "peep",
    kBuildTypeExperimental,
    kArgCheckPolicyBool,
    "  --peep                      \tPerform peephole optimization after RA\n"
    "  --no-peep\n",
    "mplcg",
    {} },
  { kPreSchedule,
    kEnable,
    "",
    "preschedule",
    kBuildTypeExperimental,
    kArgCheckPolicyBool,
    "  --preschedule               \tPerform prescheduling\n"
    "  --no-preschedule\n",
    "mplcg",
    {} },
  { kSchedule,
    kEnable,
    "",
    "schedule",
    kBuildTypeExperimental,
    kArgCheckPolicyBool,
    "  --schedule                  \tPerform scheduling\n"
    "  --no-schedule\n",
    "mplcg",
    {} },
  { kVregRename,
    kEnable,
    "",
    "vreg-rename",
    kBuildTypeExperimental,
    kArgCheckPolicyBool,
    "  --vreg-rename                  \tPerform rename of long live range around loops in coloring RA\n"
    "  --no-vreg-rename\n",
    "mplcg",
    {} },
  { kMultiPassRA,
    kEnable,
    "",
    "fullcolor",
    kBuildTypeExperimental,
    kArgCheckPolicyBool,
    "  --fullcolor                  \tPerform multi-pass coloring RA\n"
    "  --no-fullcolor\n",
    "mplcg",
    {} },
  { kWriteRefFieldOpt,
    kEnable,
    "",
    "writefieldopt",
    kBuildTypeExperimental,
    kArgCheckPolicyBool,
    "  --writefieldopt                  \tPerform WriteRefFieldOpt\n"
    "  --no-writefieldopt\n",
    "mplcg",
    {} },
  { kDumpOlog,
    kEnable,
    "",
    "dump-olog",
    kBuildTypeExperimental,
    kArgCheckPolicyBool,
    "  --dump-olog                 \tDump CFGO and ICO debug information\n"
    "  --no-dump-olog\n",
    "mplcg",
    {} },
  { kCGNativeOpt,
    kEnable,
    "",
    "nativeopt",
    kBuildTypeProduct,
    kArgCheckPolicyBool,
    "  --nativeopt                 \tEnable native opt\n"
    "  --no-nativeopt\n",
    "mplcg",
    {} },
  { kObjMap,
    kEnable,
    "",
    "objmap",
    kBuildTypeExperimental,
    kArgCheckPolicyBool,
    "  --objmap                    \tCreate object maps (GCTIBs) inside the main output (.s) file\n"
    "  --no-objmap\n",
    "mplcg",
    {} },
  { kYieldPoing,
    kEnable,
    "",
    "yieldpoint",
    kBuildTypeExperimental,
    kArgCheckPolicyBool,
    "  --yieldpoint                \tGenerate yieldpoints [default]\n"
    "  --no-yieldpoint\n",
    "mplcg",
    {} },
  { kProepilogue,
    kEnable,
    "",
    "proepilogue",
    kBuildTypeExperimental,
    kArgCheckPolicyBool,
    "  --proepilogue               \tDo tail call optimization and eliminate unnecessary prologue and epilogue.\n"
    "  --no-proepilogue\n",
    "mplcg",
    {} },
  { kLocalRc,
    kEnable,
    "",
    "local-rc",
    kBuildTypeExperimental,
    kArgCheckPolicyBool,
    "  --local-rc                  \tHandle Local Stack RC [default]\n"
    "  --no-local-rc\n",
    "mplcg",
    {} },
  { kInsertCall,
    0,
    "",
    "insert-call",
    kBuildTypeExperimental,
    kArgCheckPolicyRequired,
    "  --insert-call=name          \tInsert a call to the named function\n",
    "mplcg",
    {} },
  { kTrace,
    0,
    "",
    "add-debug-trace",
    kBuildTypeProduct,
    kArgCheckPolicyNone,
    "  --add-debug-trace           \tInstrument the output .s file to print call traces at runtime\n",
    "mplcg",
    {} },
  { kProfileEnable,
    0,
    "",
    "add-func-profile",
    kBuildTypeExperimental,
    kArgCheckPolicyNone,
    "  --add-func-profile          \tInstrument the output .s file to record func at runtime\n",
    "mplcg",
    {} },
  { kCGClassList,
    0,
    "",
    "class-list-file",
    kBuildTypeExperimental,
    kArgCheckPolicyRequired,
    "  --class-list-file           \tSet the class list file for the following generation options,\n"
    "                              \tif not given, generate for all visible classes\n"
    "                              \t--class-list-file=class_list_file\n",
    "mplcg",
    {} },
  { kGenDef,
    kEnable,
    "",
    "gen-c-macro-def",
    kBuildTypeProduct,
    kArgCheckPolicyBool,
    "  --gen-c-macro-def           \tGenerate a .def file that contains extra type metadata, including the\n"
    "                              \tclass instance sizes and field offsets (default)\n"
    "  --no-gen-c-macro-def\n",
    "mplcg",
    {} },
  { kGenGctib,
    kEnable,
    "",
    "gen-gctib-file",
    kBuildTypeExperimental,
    kArgCheckPolicyBool,
    "  --gen-gctib-file            \tGenerate a separate .s file for GCTIBs. Usually used together with\n"
    "                              \t--no-objmap (not implemented yet)\n"
    "  --no-gen-gctib-file\n",
    "mplcg",
    {} },
  { kStackGuard,
    kEnable,
    "",
    "stackguard",
    kBuildTypeExperimental,
    kArgCheckPolicyBool,
    "  -stackguard                 \tadd stack guard\n"
    "  -no-stackguard\n",
    "mplcg",
    {} },
  { kDebuggingInfo,
    0,
    "g",
    "",
    kBuildTypeExperimental,
    kArgCheckPolicyNone,
    "  -g                          \tGenerate debug information\n",
    "mplcg",
    {} },
  { kDebugGenDwarf,
    0,
    "",
    "gdwarf",
    kBuildTypeProduct,
    kArgCheckPolicyNone,
    "  --gdwarf                    \tGenerate dwarf infomation\n",
    "mplcg",
    {} },
  { kDebugUseSrc,
    0,
    "",
    "gsrc",
    kBuildTypeProduct,
    kArgCheckPolicyNone,
    "  --gsrc                      \tUse original source file instead of mpl file for debugging\n",
    "mplcg",
    {} },
  { kDebugUseMix,
    0,
    "",
    "gmixedsrc",
    kBuildTypeProduct,
    kArgCheckPolicyNone,
    "  --gmixedsrc                 \tUse both original source file and mpl file for debugging\n",
    "mplcg",
    {} },
  { kDebugAsmMix,
    0,
    "",
    "gmixedasm",
    kBuildTypeExperimental,
    kArgCheckPolicyNone,
    "  --gmixedasm                 \tComment out both original source file and mpl file for debugging\n",
    "mplcg",
    {} },
  { kProfilingInfo,
    0,
    "p",
    "",
    kBuildTypeExperimental,
    kArgCheckPolicyNone,
    "  -p                          \tGenerate profiling infomation\n",
    "mplcg",
    {} },
  { kRaLinear,
    0,
    "",
    "with-ra-linear-scan",
    kBuildTypeExperimental,
    kArgCheckPolicyNone,
    "  --with-ra-linear-scan       \tDo linear-scan register allocation\n",
    "mplcg",
    {} },
  { kRaColor,
    0,
    "",
    "with-ra-graph-color",
    kBuildTypeExperimental,
    kArgCheckPolicyNone,
    "  --with-ra-graph-color       \tDo coloring-based register allocation\n",
    "mplcg",
    {} },
  { kPatchBranch,
    0,
    "",
    "patch-long-branch",
    kBuildTypeExperimental,
    kArgCheckPolicyNone,
    "  --patch-long-branch         \tEnable patching long distance branch with jumping pad\n",
    "mplcg",
    {} },
  { kConstFoldOpt,
    0,
    "",
    "const-fold",
    kBuildTypeExperimental,
    kArgCheckPolicyNone,
    "  --const-fold                \tEnable constant folding\n",
    "mplcg",
    {} },
  { kEhList,
    0,
    "",
    "eh-exclusive-list",
    kBuildTypeExperimental,
    kArgCheckPolicyRequired,
    "  --eh-exclusive-list         \tFor generating gold files in unit testing\n"
    "                              \t--eh-exclusive-list=list_file\n",
    "mplcg",
    {} },
  { kCGO0,
    0,
    "",
    "O0",
    kBuildTypeExperimental,
    kArgCheckPolicyNone,
    "  -O0                         \tNo optimization.\n",
    "mplcg",
    {} },
  { kCGO1,
    0,
    "",
    "O1",
    kBuildTypeExperimental,
    kArgCheckPolicyOptional,
    "  -O1                         \tDo some optimization.\n",
    "mplcg",
    {} },
  { kCGO2,
    0,
    "",
    "O2",
    kBuildTypeProduct,
    kArgCheckPolicyOptional,
    "  -O2                          \tDo some optimization.\n",
    "mplcg",
    {} },
  { kLSRABB,
    0,
    "",
    "lsra-bb",
    kBuildTypeExperimental,
    kArgCheckPolicyRequired,
    "  --lsra-bb=NUM               \tSwitch to spill mode if number of bb in function exceeds NUM\n",
    "mplcg",
    {} },
  { kLSRAInsn,
    0,
    "",
    "lsra-insn",
    kBuildTypeExperimental,
    kArgCheckPolicyRequired,
    "  --lsra-insn=NUM             \tSwitch to spill mode if number of instructons in function exceeds NUM\n",
    "mplcg",
    {} },
  { kLSRAOverlap,
    0,
    "",
    "lsra-overlap",
    kBuildTypeExperimental,
    kArgCheckPolicyRequired,
    "  --lsra-overlap=NUM          \toverlap NUM to decide pre spill in lsra\n",
    "mplcg",
    {} },
  { kRaRemat,
    0,
    "",
    "remat",
    kBuildTypeExperimental,
    kArgCheckPolicyRequired,
    "  --remat                     \tEnable rematerialization during register allocation\n"
    "                              \t     0: no rematerialization (default)\n"
    "                              \t  >= 1: rematerialize constants\n"
    "                              \t  >= 2: rematerialize addresses\n"
    "                              \t  >= 3: rematerialize local dreads\n"
    "                              \t  >= 4: rematerialize global dreads\n",
    "mplcg",
    {} },
  { kSuppressFinfo,
    0,
    "",
    "suppress-fileinfo",
    kBuildTypeExperimental,
    kArgCheckPolicyNone,
    "  --suppress-fileinfo         \tFor generating gold files in unit testing\n",
    "mplcg",
    {} },
  { kCGDumpcfg,
    0,
    "",
    "dump-cfg",
    kBuildTypeExperimental,
    kArgCheckPolicyNone,
    "  --dump-cfg\n",
    "mplcg",
    {} },
  { kCGDumpPhases,
    0,
    "",
    "dump-phases",
    kBuildTypeExperimental,
    kArgCheckPolicyRequired,
    "  --dump-phases=PHASENAME,... \tEnable debug trace for specified phases in the comma separated list\n",
    "mplcg",
    {} },
  { kCGSkipPhases,
    0,
    "",
    "skip-phases",
    kBuildTypeExperimental,
    kArgCheckPolicyRequired,
    "  --skip-phases=PHASENAME,... \tSkip the phases specified in the comma separated list\n",
    "mplcg",
    {} },
  { kCGSkipFrom,
    0,
    "",
    "skip-from",
    kBuildTypeExperimental,
    kArgCheckPolicyRequired,
    "  --skip-from=PHASENAME       \tSkip the rest phases from PHASENAME(included)\n",
    "mplcg",
    {} },
  { kCGSkipAfter,
    0,
    "",
    "skip-after",
    kBuildTypeExperimental,
    kArgCheckPolicyRequired,
    "  --skip-after=PHASENAME      \tSkip the rest phases after PHASENAME(excluded)\n",
    "mplcg",
    {} },
  { kCGDumpFunc,
    0,
    "",
    "dump-func",
    kBuildTypeExperimental,
    kArgCheckPolicyRequired,
    "  --dump-func=FUNCNAME        \tDump/trace only for functions whose names contain FUNCNAME as substring\n"
    "                              \t(can only specify once)\n",
    "mplcg",
    {} },
  { kCGDumpBefore,
    kEnable,
    "",
    "dump-before",
    kBuildTypeExperimental,
    kArgCheckPolicyBool,
    "  --dump-before               \tDo extra IR dump before the specified phase\n"
    "  --no-dump-before            \tDon't extra IR dump before the specified phase\n",
    "mplcg",
    {} },
  { kCGDumpAfter,
    kEnable,
    "",
    "dump-after",
    kBuildTypeExperimental,
    kArgCheckPolicyBool,
    "  --dump-after                \tDo extra IR dump after the specified phase\n"
    "  --no-dump-after             \tDon't extra IR dump after the specified phase\n",
    "mplcg",
    {} },
  { kCGTimePhases,
    kEnable,
    "",
    "time-phases",
    kBuildTypeExperimental,
    kArgCheckPolicyBool,
    "  --time-phases               \tCollect compilation time stats for each phase\n"
    "  --no-time-phases            \tDon't Collect compilation time stats for each phase\n",
    "mplcg",
    {} },
  { kCGBarrier,
    kEnable,
    "",
    "use-barriers-for-volatile",
    kBuildTypeExperimental,
    kArgCheckPolicyBool,
    "  --use-barriers-for-volatile \tOptimize volatile load/str\n"
    "  --no-use-barriers-for-volatile\n",
    "mplcg",
    {} },
  { kCGRange,
    0,
    "",
    "range",
    kBuildTypeExperimental,
    kArgCheckPolicyRequired,
    "  --range=NUM0,NUM1           \tOptimize only functions in the range [NUM0, NUM1]\n",
    "mplcg",
    {} },
  { kFastAlloc,
    0,
    "",
    "fast-alloc",
    kBuildTypeExperimental,
    kArgCheckPolicyRequired,
    "  --fast-alloc=[0/1]          \tO2 RA fast mode, set to 1 to spill all registers\n",
    "mplcg",
    {} },
  { kSpillRange,
    0,
    "",
    "spill_range",
    kBuildTypeExperimental,
    kArgCheckPolicyRequired,
    "  --spill_range=NUM0,NUM1     \tO2 RA spill registers in the range [NUM0, NUM1]\n",
    "mplcg",
    {} },
  { kDuplicateBB,
    kEnable,
    "",
    "dup-bb",
    kBuildTypeExperimental,
    kArgCheckPolicyBool,
    "  --dup-bb                 \tAllow cfg optimizer to duplicate bb\n"
    "  --no-dup-bb              \tDon't allow cfg optimizer to duplicate bb\n",
    "mplcg",
    {} },
  { kCalleeCFI,
    kEnable,
    "",
    "callee-cfi",
    kBuildTypeExperimental,
    kArgCheckPolicyBool,
    "  --callee-cfi                \tcallee cfi message will be generated\n"
    "  --no-callee-cfi             \tcallee cfi message will not be generated\n",
    "mplcg",
    {} },
  { kPrintFunction,
    kEnable,
    "",
    "print-func",
    kBuildTypeExperimental,
    kArgCheckPolicyBool,
    "  --print-func\n"
    "  --no-print-func\n",
    "mplcg",
    {} },
  { kCyclePatternList,
    0,
    "",
    "cycle-pattern-list",
    kBuildTypeExperimental,
    kArgCheckPolicyRequired,
    "  --cycle-pattern-list        \tFor generating cycle pattern meta\n"
    "                              \t--cycle-pattern-list=list_file\n",
    "mplcg",
    {} },
  { kDuplicateToDelPlt,
    0,
    "",
    "duplicate_asm_list",
    kBuildTypeProduct,
    kArgCheckPolicyRequired,
    "  --duplicate_asm_list        \tDuplicate asm functions to delete plt call\n"
    "                              \t--duplicate_asm_list=list_file\n",
    "mplcg",
    {} },
  { kDuplicateToDelPlt2,
    0,
    "",
    "duplicate_asm_list2",
    kBuildTypeProduct,
    kArgCheckPolicyRequired,
    "  --duplicate_asm_list2       \tDuplicate more asm functions to delete plt call\n"
    "                              \t--duplicate_asm_list2=list_file\n",
    "mplcg",
    {} },
  { kEmitBlockMarker,
    0,
    "",
    "block-marker",
    kBuildTypeExperimental,
    kArgCheckPolicyRequired,
    "  --block-marker              \tEmit block marker symbols in emitted assembly files\n",
    "mplcg",
    {} },
  { kInsertSoe,
    0,
    "",
    "soe-check",
    kBuildTypeExperimental,
    kArgCheckPolicyNone,
    "  --soe-check                 \tInsert a soe check instruction[default off]\n",
    "mplcg",
    {} },
  { kCheckArrayStore,
    kEnable,
    "",
    "check-arraystore",
    kBuildTypeExperimental,
    kArgCheckPolicyBool,
    "  --check-arraystore          \tcheck arraystore exception[default off]\n"
    "  --no-check-arraystore\n",
    "mplcg",
    {} },
  { kDebugSched,
    kEnable,
    "",
    "debug-schedule",
    kBuildTypeExperimental,
    kArgCheckPolicyBool,
    "  --debug-schedule            \tdump scheduling information\n"
    "  --no-debug-schedule\n",
    "mplcg",
    {} },
  { kBruteForceSched,
    kEnable,
    "",
    "bruteforce-schedule",
    kBuildTypeExperimental,
    kArgCheckPolicyBool,
    "  --bruteforce-schedule       \tdo brute force schedule\n"
    "  --no-bruteforce-schedule\n",
    "mplcg",
    {} },
  { kSimulateSched,
    kEnable,
    "",
    "simulate-schedule",
    kBuildTypeExperimental,
    kArgCheckPolicyBool,
    "  --simulate-schedule         \tdo simulate schedule\n"
    "  --no-simulate-schedule\n",
    "mplcg",
    {} },
  { kCrossLoc,
    kEnable,
    "",
    "cross-loc",
    kBuildTypeExperimental,
    kArgCheckPolicyBool,
    "  --cross-loc                 \tcross loc insn schedule\n"
    "  --no-cross-loc\n",
    "mplcg",
    {} },
  { kABIType,
    0,
    "",
    "float-abi",
    kBuildTypeExperimental,
    kArgCheckPolicyRequired,
    "  --float-abi=name            \tPrint the abi type.\n"
    "                              \tname=hard: abi-hard (Default)\n"
    "                              \tname=soft: abi-soft\n"
    "                              \tname=softfp: abi-softfp\n",
    "mplcg",
    {} },
  { kEmitFileType,
    0,
    "",
    "filetype",
    kBuildTypeExperimental,
    kArgCheckPolicyRequired,
    "  --filetype=name             \tChoose a file type.\n"
    "                              \tname=asm: Emit an assembly file (Default)\n"
    "                              \tname=obj: Emit an object file\n"
    "                              \tname=null: not support yet\n",
    "mplcg",
    {} },
  { kLongCalls,
    kEnable,
    "",
    "long-calls",
    kBuildTypeExperimental,
    kArgCheckPolicyBool,
    "  --long-calls                \tgenerate long call\n"
    "  --no-long-calls\n",
    "mplcg",
    {} },
  { kFunctionSections,
    kEnable,
    "",
    "function-sections",
    kBuildTypeProduct,
    kArgCheckPolicyBool,
    " --function-sections           \t \n"
    "  --no-function-sections\n",
    "mplcg",
    {} },
  { kOmitFramePointer,
    kEnable,
    "",
    "omit-frame-pointer",
    kBuildTypeProduct,
    kArgCheckPolicyBool,
    " --omit-frame-pointer          \t do not use frame pointer \n"
    " --no-omit-frame-pointer\n",
    "mplcg",
    {} },
  { kFastMath,
    kEnable,
    "",
    "fast-math",
    kBuildTypeExperimental,
    kArgCheckPolicyBool,
    "  --fast-math                  \tPerform fast math\n"
    "  --no-fast-math\n",
    "mplcg",
    {} },
  { kTailCall,
    kEnable,
    "",
    "tailcall",
    kBuildTypeExperimental,
    kArgCheckPolicyBool,
    "  --tailcall                   \tDo tail call optimization\n"
    "  --no-tailcall\n",
    "mplcg",
    {} },

// End
  { kUnknown,
    0,
    "",
    "",
    kBuildTypeAll,
    kArgCheckPolicyNone,
    "",
    "mplcg",
    {} }
};

CGOptions &CGOptions::GetInstance() {
  static CGOptions instance;
  return instance;
}

CGOptions::CGOptions() {
  CreateUsages(kUsage);
}

void CGOptions::DecideMplcgRealLevel(const std::vector<mapleOption::Option> &inputOptions, bool isDebug) {
  int realLevel = -1;
  for (const mapleOption::Option &opt : inputOptions) {
    switch (opt.Index()) {
      case kCGO0:
        realLevel = CGOptions::kLevel0;
        break;
      case kCGO1:
        realLevel = CGOptions::kLevel1;
        break;
      case kCGO2:
        realLevel = CGOptions::kLevel2;
        break;
      default:
        break;
    }
  }
  if (isDebug) {
    LogInfo::MapleLogger() << "Real Mplcg level:" << std::to_string(realLevel) << "\n";
  }
  if (realLevel ==  CGOptions::kLevel0) {
    EnableO0();
  } else if (realLevel ==  CGOptions::kLevel1) {
    EnableO1();
  } else if (realLevel ==  CGOptions::kLevel2) {
    EnableO2();
  }
}

bool CGOptions::SolveOptions(const std::vector<Option> &opts, bool isDebug) {
  DecideMplcgRealLevel(opts, isDebug);
  for (const mapleOption::Option &opt : opts) {
    if (isDebug) {
      LogInfo::MapleLogger() << "mplcg options: "  << opt.Index() << " " << opt.OptionKey() << " " <<
                                opt.Args() << '\n';
    }
    switch (opt.Index()) {
      case kCGQuiet:
        SetQuiet((opt.Type() == kEnable));
        break;
      case kVerbose:
        SetQuiet((opt.Type() == kEnable) ? false : true);
        break;
      case kPie:
        (opt.Type() == kEnable) ? SetOption(CGOptions::kGenPie)
                                : ClearOption(CGOptions::kGenPie);
        break;
      case kPic: {
        if (opt.Type() == kEnable) {
          EnablePIC();
          SetOption(CGOptions::kGenPic);
        } else {
          DisablePIC();
          ClearOption(CGOptions::kGenPic);
        }
        break;
      }
      case kCGVerbose:
        (opt.Type() == kEnable) ? SetOption(CGOptions::kVerboseAsm)
                                : ClearOption(CGOptions::kVerboseAsm);
        break;
      case kCGVerboseCG:
        (opt.Type() == kEnable) ? SetOption(CGOptions::kVerboseCG)
                                : ClearOption(CGOptions::kVerboseCG);
        break;
      case kCGMapleLinker:
        (opt.Type() == kEnable) ? EnableMapleLinker() : DisableMapleLinker();
        break;
      case kFastAlloc:
        EnableFastAlloc();
        SetFastAllocMode(std::stoul(opt.Args(), nullptr));
        break;
      case kCGBarrier:
        (opt.Type() == kEnable) ? EnableBarriersForVolatile() : DisableBarriersForVolatile();
        break;
      case kSpillRange:
        SetRange(opt.Args(), "--pill-range", GetSpillRanges());
        break;
      case kCGRange:
        SetRange(opt.Args(), "--range", GetRange());
        break;
      case kCGDumpBefore:
        (opt.Type() == kEnable) ? EnableDumpBefore() : DisableDumpBefore();
        break;
      case kCGDumpAfter:
        (opt.Type() == kEnable) ? EnableDumpAfter() : DisableDumpAfter();
        break;
      case kCGTimePhases:
        (opt.Type() == kEnable) ? EnableTimePhases() : DisableTimePhases();
        break;
      case kCGDumpFunc:
        SetDumpFunc(opt.Args());
        break;
      case kDuplicateToDelPlt:
        SetDuplicateAsmFile(opt.Args());
        break;
      case kDuplicateToDelPlt2:
        SetFastFuncsAsmFile(opt.Args());
        break;
      case kInsertCall:
        SetInstrumentationFunction(opt.Args());
        SetInsertCall(true);
        break;
      case kStackGuard:
        SetOption(kUseStackGuard);
        break;
      case kDebuggingInfo:
        SetOption(kDebugFriendly);
        SetOption(kWithLoc);
        ClearOption(kSuppressFileInfo);
        break;
      case kDebugGenDwarf:
        SetOption(kDebugFriendly);
        SetOption(kWithLoc);
        SetOption(kWithDwarf);
        SetParserOption(kWithDbgInfo);
        ClearOption(kSuppressFileInfo);
        break;
      case kDebugUseSrc:
        SetOption(kDebugFriendly);
        SetOption(kWithLoc);
        SetOption(kWithSrc);
        ClearOption(kWithMpl);
        break;
      case kDebugUseMix:
        SetOption(kDebugFriendly);
        SetOption(kWithLoc);
        SetOption(kWithSrc);
        SetOption(kWithMpl);
        break;
      case kDebugAsmMix:
        SetOption(kDebugFriendly);
        SetOption(kWithLoc);
        SetOption(kWithSrc);
        SetOption(kWithMpl);
        SetOption(kWithAsm);
        break;
      case kProfilingInfo:
        SetOption(kWithProfileCode);
        SetParserOption(kWithProfileInfo);
        break;
      case kRaLinear:
        SetOption(kDoLinearScanRegAlloc);
        ClearOption(kDoColorRegAlloc);
        break;
      case kRaColor:
        SetOption(kDoColorRegAlloc);
        ClearOption(kDoLinearScanRegAlloc);
        break;
      case kPrintFunction:
        (opt.Type() == kEnable) ? EnablePrintFunction() : DisablePrintFunction();
        break;
      case kTrace:
        SetOption(kAddDebugTrace);
        break;
      case kProfileEnable:
        SetOption(kAddFuncProfile);
        break;
      case kSuppressFinfo:
        SetOption(kSuppressFileInfo);
        break;
      case kPatchBranch:
        SetOption(kPatchLongBranch);
        break;
      case kConstFoldOpt:
        SetOption(kConstFold);
        break;
      case kCGDumpcfg:
        SetOption(kDumpCFG);
        break;
      case kCGClassList:
        SetClassListFile(opt.Args());
        break;
      case kGenDef:
        SetOrClear(GetGenerateFlags(), CGOptions::kCMacroDef, static_cast<bool>(opt.Type()));
        break;
      case kGenGctib:
        SetOrClear(GetGenerateFlags(), CGOptions::kGctib, static_cast<bool>(opt.Type()));
        break;
      case kGenPrimorList:
        SetOrClear(GetGenerateFlags(), CGOptions::kPrimorList, static_cast<bool>(opt.Type()));
        break;
      case kYieldPoing:
        SetOrClear(GetGenerateFlags(), CGOptions::kGenYieldPoint, static_cast<bool>(opt.Type()));
        break;
      case kLocalRc:
        SetOrClear(GetGenerateFlags(), CGOptions::kGenLocalRc, static_cast<bool>(opt.Type()));
        break;
      case kEhList: {
        const std::string &ehList = opt.Args();
        SetEHExclusiveFile(ehList);
        EnableExclusiveEH();
        ParseExclusiveFunc(ehList);
        break;
      }
      case kCyclePatternList: {
        const std::string &patternList = opt.Args();
        SetCyclePatternFile(patternList);
        EnableEmitCyclePattern();
        ParseCyclePattern(patternList);
        break;
      }
      case kCgen: {
        bool cgFlag = (opt.Type() == kEnable);
        SetRunCGFlag(cgFlag);
        cgFlag ? SetOption(CGOptions::kDoCg) : ClearOption(CGOptions::kDoCg);
        break;
      }
      case kObjMap:
        SetGenerateObjectMap(opt.Type() == kEnable);
        break;
      case kReplaceAsm:
        (opt.Type() == kEnable) ? EnableReplaceASM() : DisableReplaceASM();
        break;
      case kUseGeneralRegOnly:
        (opt.Type() == kEnable) ? EnableGeneralRegOnly() : DisableGeneralRegOnly();
        break;
      case kCGLazyBinding:
        (opt.Type() == kEnable) ? EnableLazyBinding() : DisableLazyBinding();
        break;
      case kCGHotFix:
        (opt.Type() == kEnable) ? EnableHotFix() : DisableHotFix();
        break;
      case kInsertSoe:
        SetOption(CGOptions::kSoeCheckInsert);
        break;
      case kCheckArrayStore:
        (opt.Type() == kEnable) ? EnableCheckArrayStore() : DisableCheckArrayStore();
        break;

      case kEbo:
        (opt.Type() == kEnable) ? EnableEBO() : DisableEBO();
        break;

      case kCfgo:
        (opt.Type() == kEnable) ? EnableCFGO() : DisableCFGO();
        break;
      case kIco:
        (opt.Type() == kEnable) ? EnableICO() : DisableICO();
        break;
      case kSlo:
        (opt.Type() == kEnable) ? EnableStoreLoadOpt() : DisableStoreLoadOpt();
        break;
      case kGo:
        (opt.Type() == kEnable) ? EnableGlobalOpt() : DisableGlobalOpt();
        break;
      case kPreLSRAOpt:
        (opt.Type() == kEnable) ? EnablePreLSRAOpt() : DisablePreLSRAOpt();
        break;
      case kLocalrefSpill:
        (opt.Type() == kEnable) ? EnableLocalRefSpill() : DisableLocalRefSpill();
        break;
      case kOptCallee:
        (opt.Type() == kEnable) ? EnableCalleeToSpill() : DisableCalleeToSpill();
        break;
      case kPrepeep:
        (opt.Type() == kEnable) ? EnablePrePeephole() : DisablePrePeephole();
        break;
      case kPeep:
        (opt.Type() == kEnable) ? EnablePeephole() : DisablePeephole();
        break;
      case kPreSchedule:
        (opt.Type() == kEnable) ? EnablePreSchedule() : DisablePreSchedule();
        break;
      case kSchedule:
        (opt.Type() == kEnable) ? EnableSchedule() : DisableSchedule();
        break;
      case kVregRename:
        (opt.Type() == kEnable) ? EnableVregRename() : DisableVregRename();
        break;
      case kMultiPassRA:
        (opt.Type() == kEnable) ? EnableMultiPassColorRA() : DisableMultiPassColorRA();
        break;
      case kWriteRefFieldOpt:
        (opt.Type() == kEnable) ? EnableWriteRefFieldOpt() : DisableWriteRefFieldOpt();
        break;
      case kDumpOlog:
        (opt.Type() == kEnable) ? EnableDumpOptimizeCommonLog() : DisableDumpOptimizeCommonLog();
        break;
      case kCGNativeOpt:
        DisableNativeOpt();
        break;
      case kDuplicateBB:
        (opt.Type() == kEnable) ? DisableNoDupBB() : EnableNoDupBB();
        break;
      case kCalleeCFI:
        (opt.Type() == kEnable) ? DisableNoCalleeCFI() : EnableNoCalleeCFI();
        break;
      case kProepilogue:
        (opt.Type() == kEnable) ? SetOption(CGOptions::kProEpilogueOpt)
                                : ClearOption(CGOptions::kProEpilogueOpt);
        break;
      case kTailCall:
        (opt.Type() == kEnable) ? SetOption(CGOptions::kTailCallOpt)
                                : ClearOption(CGOptions::kTailCallOpt);
        break;
      case kLSRABB:
        SetLSRABBOptSize(std::stoul(opt.Args(), nullptr));
        break;
      case kLSRAInsn:
        SetLSRAInsnOptSize(std::stoul(opt.Args(), nullptr));
        break;
      case kLSRAOverlap:
        SetOverlapNum(std::stoul(opt.Args(), nullptr));
        break;
      case kRaRemat:
        SetRematLevel(std::stoul(opt.Args(), nullptr));
        break;
      case kCGO0:
        // Already handled above in DecideMplcgRealLevel
        break;
      case kCGO1:
        // Already handled above in DecideMplcgRealLevel
        break;
      case kCGO2:
        // Already handled above in DecideMplcgRealLevel
        break;
      case kCGDumpPhases:
        SplitPhases(opt.Args(), GetDumpPhases());
        break;
      case kCGSkipPhases:
        SplitPhases(opt.Args(), GetSkipPhases());
        break;
      case kCGSkipFrom:
        SetSkipFrom(opt.Args());
        break;
      case kCGSkipAfter:
        SetSkipAfter(opt.Args());
        break;
      case kDebugSched:
        (opt.Type() == kEnable) ? EnableDebugSched() : DisableDebugSched();
        break;
      case kBruteForceSched:
        (opt.Type() == kEnable) ? EnableDruteForceSched() : DisableDruteForceSched();
        break;
      case kSimulateSched:
        (opt.Type() == kEnable) ? EnableSimulateSched() : DisableSimulateSched();
        break;
      case kProfilePath:
        SetProfileData(opt.Args());
        break;
      case kABIType:
        SetABIType(opt.Args());
        break;
      case kEmitFileType:
        SetEmitFileType(opt.Args());
        break;
      case kLongCalls:
        (opt.Type() == kEnable) ? EnableLongCalls() : DisableLongCalls();
        break;
      case kFunctionSections:
        (opt.Type() == kEnable) ? EnableFunctionSections() : DisableFunctionSections();
        break;
      case kOmitFramePointer:
        (opt.Type() == kEnable) ?  DisableFramePointer() : EnableFramePointer();
        break;
      case kGCOnly:
        (opt.Type() == kEnable) ? EnableGCOnly() : DisableGCOnly();
        break;
      case kFastMath:
        (opt.Type() == kEnable) ? EnableFastMath() : DisableFastMath();
        break;
      default:
        WARN(kLncWarn, "input invalid key for mplcg " + opt.OptionKey());
        break;
    }
  }
  /* override some options when loc, dwarf is generated */
  if (WithLoc()) {
    DisableSchedule();
    SetOption(kWithSrc);
  }
  if (WithDwarf()) {
    DisableEBO();
    DisableCFGO();
    DisableICO();
    DisableSchedule();
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

void CGOptions::ParseCyclePattern(const std::string &fileName) {
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
      CGOptions::cyclePatternMap[className] = move(classPatternContent);
    }
  }
}

void CGOptions::SetRange(const std::string &str, const std::string &cmd, Range &subRange) {
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
  insertYieldPoint = GenYieldPoint();
}

void CGOptions::EnableO0() {
  optimizeLevel = kLevel0;
  doEBO = false;
  doCFGO = false;
  doICO = false;
  doPrePeephole = false;
  doPeephole = false;
  doStoreLoadOpt = false;
  doGlobalOpt = false;
  doPreLSRAOpt = false;
  doLocalRefSpill = false;
  doCalleeToSpill = false;
  doSchedule = false;
  doWriteRefFieldOpt = false;
  SetOption(kUseStackGuard);
  ClearOption(kConstFold);
  ClearOption(kProEpilogueOpt);
  ClearOption(kTailCallOpt);
}

void CGOptions::EnableO1() {
  optimizeLevel = kLevel1;
  doPreLSRAOpt = true;
  doCalleeToSpill = true;
  SetOption(kConstFold);
  SetOption(kProEpilogueOpt);
  SetOption(kTailCallOpt);
  ClearOption(kUseStackGuard);
}

void CGOptions::EnableO2() {
  optimizeLevel = kLevel2;
  doEBO = true;
  doCFGO = true;
  doICO = true;
  doPrePeephole = true;
  doPeephole = true;
  doStoreLoadOpt = true;
  doGlobalOpt = true;
  doPreSchedule = false;
  doSchedule = true;
  SetOption(kConstFold);
  ClearOption(kUseStackGuard);
#if TARGARM32
  doPreLSRAOpt = false;
  doLocalRefSpill = false;
  doCalleeToSpill = false;
  doWriteRefFieldOpt = false;
  ClearOption(kProEpilogueOpt);
  ClearOption(kTailCallOpt);
#else
  doPreLSRAOpt = true;
  doLocalRefSpill = true;
  doCalleeToSpill = true;
  doWriteRefFieldOpt = true;
  SetOption(kProEpilogueOpt);
  SetOption(kTailCallOpt);
#endif
}

void CGOptions::SplitPhases(const std::string &str, std::unordered_set<std::string> &set) {
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
