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
#include "driver_option_common.h"

namespace {
using namespace maple;
using namespace mapleOption;
const mapleOption::Descriptor kUsages[] = {
  // index, type, shortOption, longOption, enableBuildType, checkPolicy, help, exeName, extra bins
  { kUnknown,
    0,
    "",
    "",
    kBuildTypeAll,
    kArgCheckPolicyNone,
    "USAGE: maple [options]\n\n"
    "  Example 1: <Maple bin path>/maple --run=me:mpl2mpl:mplcg --option=\"[MEOPT]:[MPL2MPLOPT]:[MPLCGOPT]\"\n"
    "                                    --mplt=MPLTPATH inputFile.mpl\n"
    "  Example 2: <Maple bin path>/maple -O2 --mplt=mpltPath inputFile.dex\n\n"
    "==============================\n"
    "  Options:\n",
    "driver",
    {} },
  { kHelp,
    0,
    "h",
    "help",
    kBuildTypeProduct,
    kArgCheckPolicyOptional,
    "  -h --help [COMPILERNAME]    \tPrint usage and exit.\n"
    "                              \tTo print the help info of specified compiler,\n"
    "                              \tyou can use jbc2mpl, me, mpl2mpl, mplcg... as the COMPILERNAME\n"
    "                              \teg: --help=mpl2mpl\n",
    "driver",
    {} },
  { kVersion,
    0,
    "",
    "version",
    kBuildTypeProduct,
    kArgCheckPolicyOptional,
    "  --version [command]         \tPrint version and exit.\n",
    "driver",
    {} },
  { kInFile,
    0,
    "",
    "infile",
    kBuildTypeProduct,
    kArgCheckPolicyRequired,
    "  --infile file1,file2,file3  \tInput files.\n",
    "driver",
    {} },
  { kInMplt,
    0,
    "",
    "mplt",
    kBuildTypeProduct,
    kArgCheckPolicyRequired,
    "  --mplt=file1,file2,file3    \tImport mplt files.\n",
    "driver",
    { "jbc2mpl",
      "dex2mpl"
    } },
  { kOptimization0,
    0,
    "O0",
    "",
    kBuildTypeProduct,
    kArgCheckPolicyNone,
    "  -O0                         \tNo optimization.\n",
    "driver",
    {} },
  { kOptimization1,
    0,
    "O1",
    "",
    kBuildTypeExperimental,
    kArgCheckPolicyNone,
    "  -O1                         \tDo some optimization.\n",
    "driver",
    {} },
  { kOptimization2,
    0,
    "O2",
    "",
    kBuildTypeProduct,
    kArgCheckPolicyNone,
    "  -O2                         \tDo more optimization. (Default)\n",
    "driver",
    {"hir2mpl"} },
  { kOptimizationS,
      0,
      "Os",
      "",
      kBuildTypeProduct,
      kArgCheckPolicyNone,
      "  -Os                         \tOptimize for size, based on O2.\n",
      "driver",
      {"hir2mpl"} },
  { kPartO2,
    0,
    "",
    "partO2",
    kBuildTypeProduct,
    kArgCheckPolicyNone,
    "  --partO2               \tSet func list for O2\n",
    "driver",
    {} },
  { kWithIpa,
    kEnable,
    "",
    "with-ipa",
    kBuildTypeExperimental,
    kArgCheckPolicyBool,
    "  --with-ipa                  \tRun IPA when building\n"
    "  --no-with-ipa               \n",
    "driver",
    {} },
  { kVerify,
    kEnable,
    "",
    "verify",
    kBuildTypeExperimental,
    kArgCheckPolicyNone,
    "  --verify                    \tVerify mpl file\n",
    "driver",
    { "dex2mpl", "mpl2mpl" } },
  { kJbc2mplOpt,
    0,
    "",
    "jbc2mpl-opt",
    kBuildTypeProduct,
    kArgCheckPolicyNone,
    "  --jbc2mpl-opt               \tSet options for jbc2mpl\n",
    "driver",
    {} },
  { kCpp2mplOpt,
    0,
    "",
    "hir2mpl-opt",
    kBuildTypeProduct,
    kArgCheckPolicyNone,
    "  --hir2mpl-opt               \tSet options for hir2mpl\n",
    "driver",
    {} },
  { kAsOpt,
    0,
    "",
    "as-opt",
    kBuildTypeProduct,
    kArgCheckPolicyNone,
    "--as-opt                      \tSet options for as\n",
    "driver",
    {} },
  { kLdOpt,
    0,
    "",
    "ld-opt",
    kBuildTypeProduct,
    kArgCheckPolicyNone,
    "--ld-opt                      \tSet options for ld\n",
    "driver",
    {} },
  { kDex2mplOpt,
    0,
    "",
    "dex2mpl-opt",
    kBuildTypeProduct,
    kArgCheckPolicyNone,
    "  --dex2mpl-opt               \tSet options for dex2mpl\n",
    "driver",
    {} },
  { kMplipaOpt,
    0,
    "",
    "mplipa-opt",
    kBuildTypeProduct,
    kArgCheckPolicyNone,
    "  --mplipa-opt                \tSet options for mplipa\n",
    "driver",
    {} },
  { kMplcgOpt,
    0,
    "",
    "mplcg-opt",
    kBuildTypeProduct,
    kArgCheckPolicyNone,
    "  --mplcg-opt                  \tSet options for mplcg\n",
    "driver",
    {} },
  { kDecoupleStatic,
    kEnable,
    "",
    "decouple-static",
    kBuildTypeProduct,
    kArgCheckPolicyBool,
    "  --decouple-static           \tdecouple the static method and field\n"
    "  --no-decouple-static        \tDon't decouple the static method and field\n",
    "driver",
    { "dex2mpl", "me", "mpl2mpl" } },
  { kProfilePath,
    0,
    "",
    "profile",
    kBuildTypeProduct,
    kArgCheckPolicyRequired,
    "  --profile                   \tFor PGO optimization\n"
    "                              \t--profile=list_file\n",
    "driver",
    { "dex2mpl", "mpl2mpl", "mplcg" } },
  { kProfileGen,
    kEnable,
    "",
    "profileGen",
    kBuildTypeProduct,
    kArgCheckPolicyBool,
    "  --profileGen                \tGenerate profile data for static languages\n",
    "driver",
    { "me", "mpl2mpl", "mplcg" } },
  { kProfileUse,
    kEnable,
    "",
    "profileUse",
    kBuildTypeProduct,
    kArgCheckPolicyBool,
    "  --profileUse                \tOptimize static languages with profile data\n",
    "driver",
    { "mpl2mpl" } },
  { kGCOnly,
    kEnable,
    "",
    "gconly",
    kBuildTypeProduct,
    kArgCheckPolicyBool,
    "  --gconly                     \tMake gconly is enable\n"
    "  --no-gconly                  \tDon't make gconly is enable\n",
    "driver",
    {
      "dex2mpl", "me", "mpl2mpl",
      "mplcg"
    } },
  { kBigEndian,
    kEnable,
    "Be",
    "BigEndian",
    kBuildTypeProduct,
    kArgCheckPolicyBool,
    "  --BigEndian/--Be                     \tUsing BigEndian\n"
    "  --no-BigEndian/ --no-Be              \tUsing LittleEndian\n",
    "driver",
    {
      "me", "mpl2mpl",
      "mplcg"
    } },
  { kMeOpt,
    0,
    "",
    "me-opt",
    kBuildTypeProduct,
    kArgCheckPolicyNone,
    "  --me-opt                    \tSet options for me\n",
    "driver",
    {} },
  { kMpl2MplOpt,
    0,
    "",
    "mpl2mpl-opt",
    kBuildTypeProduct,
    kArgCheckPolicyNone,
    "  --mpl2mpl-opt               \tSet options for mpl2mpl\n",
    "driver",
    {} },
  { kSaveTemps,
    0,
    "",
    "save-temps",
    kBuildTypeProduct,
    kArgCheckPolicyNone,
    "  --save-temps                \tDo not delete intermediate files.\n"
    "                              \t--save-temps Save all intermediate files.\n"
    "                              \t--save-temps=file1,file2,file3 Save the\n"
    "                              \ttarget files.\n",
    "driver",
    {} },
  { kRun,
    0,
    "",
    "run",
    kBuildTypeProduct,
    kArgCheckPolicyRequired,
    "  --run=cmd1:cmd2             \tThe name of executables that are going\n"
    "                              \tto execute. IN SEQUENCE.\n"
    "                              \tSeparated by \":\".Available exe names:\n"
    "                              \tjbc2mpl, me, mpl2mpl, mplcg\n"
    "                              \tInput file must match the tool can\n"
    "                              \thandle\n",
    "driver",
    {} },
  { kOption,
    0,
    "",
    "option",
    kBuildTypeProduct,
    kArgCheckPolicyRequired,
    "  --option=\"opt1:opt2\"      \tOptions for each executable,\n"
    "                              \tseparated by \":\".\n"
    "                              \tThe sequence must match the sequence in\n"
    "                              \t--run.\n",
    "driver",
    {} },
  { kTimePhases,
    0,
    "time-phases",
    "",
    kBuildTypeExperimental,
    kArgCheckPolicyNone,
    "  -time-phases                \tTiming phases and print percentages\n",
    "driver",
    {} },
  { kGenMeMpl,
    0,
    "",
    "genmempl",
    kBuildTypeProduct,
    kArgCheckPolicyNone,
    "  --genmempl                  \tGenerate me.mpl file\n",
    "driver",
    {} },
  { kGenMapleBC,
    0,
    "",
    "genmaplebc",
    kBuildTypeProduct,
    kArgCheckPolicyNone,
    "  --genmaplebc                \tGenerate .mbc file\n",
    "driver",
    {} },
  { kGenLMBC,
    0,
    "",
    "genlmbc",
    kBuildTypeProduct,
    kArgCheckPolicyNone,
    "  --genlmbc                   \tGenerate .lmbc file\n",
    "driver",
    {} },
  { kGenObj,
    kEnable,
    "c",
    "",
    kBuildTypeProduct,
    kArgCheckPolicyNone,
    "  -c                          \tCompile or assemble the source files, but do not link.\n",
    "driver",
    {}  },
  { kGenVtableImpl,
    0,
    "",
    "genVtableImpl",
    kBuildTypeProduct,
    kArgCheckPolicyNone,
    "  --genVtableImpl             \tGenerate VtableImpl.mpl file\n",
    "driver",
    {} },
  { kVerbose,
    kEnable,
    "",
    "verbose",
    kBuildTypeDebug,
    kArgCheckPolicyBool,
    "  -verbose                    \t: print informations\n",
    "driver",
    { "jbc2mpl", "hir2mpl", "me", "mpl2mpl", "mplcg" } },
  { kAllDebug,
    0,
    "",
    "debug",
    kBuildTypeDebug,
    kArgCheckPolicyNone,
    "  --debug                     \tPrint debug info.\n",
    "driver",
    {} },
  { kWithDwarf,
    0,
    "g",
    "",
    kBuildTypeDebug,
    kArgCheckPolicyNone,
    "  -g                          \t: with dwarf\n",
    "driver",
    {} },
  { kHelpLevel,
    0,
    "",
    "level",
    kBuildTypeProduct,
    kArgCheckPolicyNumeric,
    "  --level=NUM                 \tPrint the help info of specified level.\n"
    "                              \tNUM=0: All options (Default)\n"
    "                              \tNUM=1: Product options\n"
    "                              \tNUM=2: Experimental options\n"
    "                              \tNUM=3: Debug options\n",
    "driver",
    {} },
  { kNpeNoCheck,
    0,
    "",
    "no-npe-check",
    kBuildTypeProduct,
    kArgCheckPolicyNone,
    "  --no-npe-check              \tDisable null pointer check (Default)\n",
    "driver",
    {} },
  { kNpeStaticCheck,
    0,
    "",
    "npe-check-static",
    kBuildTypeProduct,
    kArgCheckPolicyNone,
    "  --npe-check-static          \tEnable null pointer static check only\n",
    "driver",
    {} },
  { kNpeDynamicCheck,
    0,
    "",
    "npe-check-dynamic",
    kBuildTypeProduct,
    kArgCheckPolicyNone,
    "  --npe-check-dynamic         \tEnable null pointer dynamic check with static warning\n",
    "driver",
    {} },
  { kNpeDynamicCheckSilent,
    0,
    "",
    "npe-check-dynamic-silent",
    kBuildTypeProduct,
    kArgCheckPolicyNone,
    "  --npe-check-dynamic-silent  \tEnable null pointer dynamic without static warning\n",
    "driver",
    {} },
  { kNpeDynamicCheckAll,
    0,
    "",
    "npe-check-dynamic-all",
    kBuildTypeProduct,
    kArgCheckPolicyNone,
    "  --npe-check-dynamic-all     \tKeep dynamic check before dereference, used with --npe-check-dynamic* options\n",
    "driver",
    {} },
  { kBoundaryNoCheck,
    0,
    "",
    "no-boundary-check",
    kBuildTypeProduct,
    kArgCheckPolicyNone,
    "  --no-boundary-check         \tDisable boundary check (Default)\n",
    "driver",
    {} },
  { kBoundaryStaticCheck,
    0,
    "",
    "boundary-check-static",
    kBuildTypeProduct,
    kArgCheckPolicyNone,
    "  --boundary-check-static     \tEnable boundary static check\n",
    "driver",
    {} },
  { kBoundaryDynamicCheck,
    0,
    "",
    "boundary-check-dynamic",
    kBuildTypeProduct,
    kArgCheckPolicyNone,
    "  --boundary-check-dynamic    \tEnable boundary dynamic check with static warning\n",
    "driver",
    {} },
  { kBoundaryDynamicCheckSilent,
    0,
    "",
    "boundary-check-dynamic-silent",
    kBuildTypeProduct,
    kArgCheckPolicyNone,
    "  --boundary-check-dynamic-silent  \tEnable boundary dynamic check without static warning\n",
    "driver",
    {} },
  { kSafeRegionOption,
      0,
      "",
      "safe-region",
      kBuildTypeProduct,
      kArgCheckPolicyNone,
      "  --safe-region \tEnable safe region\n",
      "driver",
      {} },
  { kMaplePrintPhases,
    0,
    "maple-print-phases",
    "maple-print-phases",
    kBuildTypeAll,
    kArgCheckPolicyOptional,
    "  -maple-print-phases         \tPrint Driver Phases.\n  --maple-print-phases\n",
    "driver",
    {} },
  { kLdStatic,
    0,
    "static",
    "static",
    kBuildTypeAll,
    kArgCheckPolicyNone,
    "  -static                     \tForce the linker to link a program statically\n",
    "driver",
    {"ld"} },
  { kLdLib,
    0,
    "l",
    "",
    kBuildTypeAll,
    kArgCheckPolicyRequired,
    "  -l <lib>                    \tLinks with a library file\n",
    "driver",
    {"ld"} },
  { kLdLibPath,
    0,
    "L",
    "",
    kBuildTypeAll,
    kArgCheckPolicyRequired,
    "  -L <libpath>                \tAdd directory to library search path\n",
    "driver",
    {"ld"} },
  { kClangMacro,
    0,
    "D",
    "",
    kBuildTypeAll,
    kArgCheckPolicyRequired,
    "  -D <macro>=<value>          \tDefine <macro> to <value> (or 1 if <value> omitted)\n",
    "driver",
    {"clang"} },
  { kClangMacro,
    0,
    "U",
    "",
    kBuildTypeAll,
    kArgCheckPolicyRequired,
    "  -U <macro>                  \tUndefine macro <macro>\n",
    "driver",
    {"clang"} },
  { kClangInclude,
    0,
    "I",
    "",
    kBuildTypeAll,
    kArgCheckPolicyRequired,
    "  -I <dir>                    \tAdd directory to include search path\n",
    "driver",
    {"clang"} },
  { kClangISystem,
    0,
    "isystem",
    "",
    kBuildTypeAll,
    kArgCheckPolicyRequired,
    "  -isystem <dir>              \tAdd directory to SYSTEM include search path\n",
    "driver",
    {"clang"} },
  { kMaplePhaseOnly,
    kEnable,
    "",
    "maple-phase",
    kBuildTypeAll,
    kArgCheckPolicyBool,
    "  --maple-phase               \tRun maple phase only\n  --no-maple-phase\n",
    "driver",
    {} },
  { kMapleOut,
    0,
    "o",
    "",
    kBuildTypeAll,
    kArgCheckPolicyRequired,
    "  -o <outfile>                \tPlace the output into <file>\n",
    "driver",
    {} },
  { kUnknown,
    0,
    "",
    "",
    kBuildTypeAll,
    kArgCheckPolicyNone,
    "",
    "driver",
    {} }
};
}  // namepsace

namespace maple {
using namespace mapleOption;
DriverOptionCommon &DriverOptionCommon::GetInstance() {
  static DriverOptionCommon instance;
  return instance;
}

DriverOptionCommon::DriverOptionCommon() {
  CreateUsages(kUsages);
}
}  // namespace maple
