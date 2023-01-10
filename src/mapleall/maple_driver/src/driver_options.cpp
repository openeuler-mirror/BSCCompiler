/*
 * Copyright (c) [2022] Huawei Technologies Co.,Ltd.All rights reserved.
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

#include "driver_options.h"
#include "cl_option.h"

namespace opts {

/* ##################### BOOL Options ############################################################### */

maplecl::Option<bool> version({"--version", "-v"},
                         "  --version [command]         \tPrint version and exit.\n",
                         {driverCategory});

maplecl::Option<bool> ignoreUnkOpt({"--ignore-unknown-options"},
                              "  --ignore-unknown-options    \tIgnore unknown compilation options\n",
                              {driverCategory});

maplecl::Option<bool> o0({"--O0", "-O0"},
                    "  -O0                         \tNo optimization. (Default)\n",
                    {driverCategory});

maplecl::Option<bool> o1({"--O1", "-O1"},
                    "  -O1                         \tDo some optimization.\n",
                    {driverCategory});

maplecl::Option<bool> o2({"--O2", "-O2"},
                    "  -O2                         \tDo more optimization.\n",
                    {driverCategory});

maplecl::Option<bool> os({"--Os", "-Os"},
                    "  -Os                         \tOptimize for size, based on O2.\n",
                    {driverCategory, hir2mplCategory});

maplecl::Option<bool> verify({"--verify"},
                        "  --verify                    \tVerify mpl file\n",
                        {driverCategory, dex2mplCategory, mpl2mplCategory});

maplecl::Option<bool> decoupleStatic({"--decouple-static", "-decouple-static"},
                                "  --decouple-static           \tDecouple the static method and field\n"
                                "  --no-decouple-static        \tDon't decouple the static method and field\n",
                                {driverCategory, dex2mplCategory, meCategory, mpl2mplCategory},
                                maplecl::DisableWith("--no-decouple-static"));

maplecl::Option<bool> gcOnly({"--gconly", "-gconly"},
                        "  --gconly                     \tMake gconly is enable\n"
                        "  --no-gconly                  \tDon't make gconly is enable\n",
                        {driverCategory, dex2mplCategory, meCategory,
                         mpl2mplCategory, cgCategory},
                        maplecl::DisableWith("--no-gconly"));

maplecl::Option<bool> timePhase({"-time-phases"},
                           "  -time-phases                \tTiming phases and print percentages\n",
                           {driverCategory});

maplecl::Option<bool> genMeMpl({"--genmempl"},
                          "  --genmempl                  \tGenerate me.mpl file\n",
                          {driverCategory});

maplecl::Option<bool> compileWOLink({"-c"},
                               "  -c                          \tCompile the source files without linking\n",
                               {driverCategory});

maplecl::Option<bool> genVtable({"--genVtableImpl"},
                           "  --genVtableImpl             \tGenerate VtableImpl.mpl file\n",
                           {driverCategory});

maplecl::Option<bool> verbose({"-verbose"},
                         "  -verbose                    \tPrint informations\n",
                         {driverCategory, jbc2mplCategory, hir2mplCategory,
                          meCategory, mpl2mplCategory, cgCategory});

maplecl::Option<bool> debug({"--debug"},
                       "  --debug                     \tPrint debug info.\n",
                       {driverCategory});

maplecl::Option<bool> withDwarf({"-g"},
                           "  --debug                     \tPrint debug info.\n",
                           {driverCategory});

maplecl::Option<bool> withIpa({"--with-ipa"},
                          "  --with-ipa                  \tRun IPA when building\n"
                          "  --no-with-ipa               \n",
                          {driverCategory},
                          maplecl::DisableWith("--no-with-ipa"));

maplecl::Option<bool> npeNoCheck({"--no-npe-check"},
                            "  --no-npe-check              \tDisable null pointer check (Default)\n",
                            {driverCategory});

maplecl::Option<bool> npeStaticCheck({"--npe-check-static"},
                                "  --npe-check-static          \tEnable null pointer static check only\n",
                                {driverCategory});

maplecl::Option<bool> npeDynamicCheck({"--npe-check-dynamic"},
                                 "  --npe-check-dynamic         \tEnable null "
                                 "pointer dynamic check with static warning\n",
                                 {driverCategory});

maplecl::Option<bool> npeDynamicCheckSilent({"--npe-check-dynamic-silent"},
                                       "  --npe-check-dynamic-silent  \tEnable null pointer dynamic "
                                       "without static warning\n",
                                       {driverCategory});

maplecl::Option<bool> npeDynamicCheckAll({"--npe-check-dynamic-all"},
                                    "  --npe-check-dynamic-all     \tKeep dynamic check before dereference, "
                                    "used with --npe-check-dynamic* options\n",
                                    {driverCategory});

maplecl::Option<bool> boundaryNoCheck({"--no-boundary-check"},
                                 "  --no-boundary-check         \tDisable boundary check (Default)\n",
                                 {driverCategory});

maplecl::Option<bool> boundaryStaticCheck({"--boundary-check-static"},
                                     "  --boundary-check-static     \tEnable boundary static check\n",
                                     {driverCategory});

maplecl::Option<bool> boundaryDynamicCheck({"--boundary-check-dynamic"},
                                      "  --boundary-check-dynamic    \tEnable boundary dynamic check "
                                      "with static warning\n",
                                      {driverCategory});

maplecl::Option<bool> boundaryDynamicCheckSilent({"--boundary-check-dynamic-silent"},
                                            "  --boundary-check-dynamic-silent  \tEnable boundary dynamic "
                                            "check without static warning\n",
                                            {driverCategory});

maplecl::Option<bool> safeRegionOption({"--safe-region"},
                                  "  --safe-region               \tEnable safe region\n",
                                  {driverCategory});

maplecl::Option<bool> enableArithCheck({"--boundary-arith-check"},
                                  "  --boundary-arith-check       \tEnable pointer arithmetic check\n",
                                  {driverCategory});

maplecl::Option<bool> enableCallFflush({"--boudary-dynamic-call-fflush"},
                                  "  --boudary-dynamic-call-fflush    \tEnable call fflush function to flush "
                                  "boundary-dynamic-check error message to the STDOUT\n",
                                  {driverCategory});

maplecl::Option<bool> onlyCompile({"-S"}, "Only run preprocess and compilation steps", {driverCategory});

maplecl::Option<bool> printDriverPhases({"--print-driver-phases"},
                                   "  --print-driver-phases       \tPrint Driver Phases\n",
                                   {driverCategory});

maplecl::Option<bool> ldStatic({"-static", "--static"},
                          "  -static                     \tForce the linker to link a program statically\n",
                          {driverCategory, ldCategory});

maplecl::Option<bool> maplePhase({"--maple-phase"},
                            "  --maple-phase               \tRun maple phase only\n  --no-maple-phase\n",
                            {driverCategory},
                            maplecl::DisableWith("--maple-toolchain"),
                            maplecl::Init(true));

maplecl::Option<bool> genMapleBC({"--genmaplebc"},
                            "  --genmaplebc                \tGenerate .mbc file\n",
                            {driverCategory});

maplecl::Option<bool> genLMBC({"--genlmbc"},
                         "  --genlmbc                \tGenerate .lmbc file\n",
                         {driverCategory, mpl2mplCategory});

maplecl::Option<bool> profileGen({"--profileGen"},
                            "  --profileGen                \tGenerate profile data for static languages\n",
                            {driverCategory, meCategory, mpl2mplCategory, cgCategory});

maplecl::Option<bool> profileUse({"--profileUse"},
                            "  --profileUse                \tOptimize static languages with profile data\n",
                            {driverCategory, mpl2mplCategory});

maplecl::Option<bool> missingProfDataIsError({"--missing-profdata-is-error"},
                            "  --missing-profdata-is-error \tTreat missing profile data file as error\n"
                            "  --no-missing-profdata-is-error \tOnly warn on missing profile data file\n",
                          {driverCategory},
                          maplecl::DisableWith("--no-missing-profdata-is-error"),
                          maplecl::Init(true));
maplecl::Option<bool> stackProtectorStrong({"--stack-protector-strong", "-fstack-protector-strong"},
                                           "  -fstack-protector-strong        \tadd stack guard for some function\n",
                                           {driverCategory, meCategory, cgCategory});

maplecl::Option<bool> stackProtectorAll({"--stack-protector-all"},
                                        "  --stack-protector-all        \tadd stack guard for all functions\n",
                                        {driverCategory, meCategory, cgCategory});

maplecl::Option<bool> inlineAsWeak({"-inline-as-weak", "--inline-as-weak"},
                                   "  --inline-as-weak              \tSet inline functions as weak symbols"
                                   " as it's in C++\n", {driverCategory, hir2mplCategory});
maplecl::Option<bool> expand128Floats({"--expand128floats"},
                                      "  --expand128floats        \tEnable expand128floats pass\n",
                                      {driverCategory},
                                      maplecl::DisableWith("--no-expand128floats"),
                                      maplecl::hide, maplecl::Init(true));

maplecl::Option<bool> MD({"-MD"},
                    "  -MD                         \tWrite a depfile containing user and system headers\n",
                    {driverCategory, unSupCategory});

maplecl::Option<bool> unUsePlt({"-fno-plt"},
                    "  -fno-plt                \tDo not use the PLT to make function calls\n",
                    {driverCategory, unSupCategory});

maplecl::Option<bool> usePipe({"-pipe"},
                    "  -pipe                   \tUse pipes between commands, when possible\n",
                    {driverCategory, unSupCategory});

maplecl::Option<bool> fDataSections({"-fdata-sections"},
                    "  -fdata-sections         \tPlace each data in its own section (ELF Only)\n",
                    {driverCategory, unSupCategory});

maplecl::Option<bool> fRegStructReturn({"-freg-struct-return"},
                    "  -freg-struct-return     \tOverride the default ABI to return small structs in registers\n",
                    {driverCategory});

maplecl::Option<bool> fTreeVectorize({"-ftree-vectorize"},
                    "  -ftree-vectorize    \tEnable vectorization on trees\n",
                    {driverCategory});

maplecl::Option<bool> fNoFatLtoObjects({"-fno-fat-lto-objects"},
                    "  -fno-fat-lto-objects    \tSpeeding up lto compilation\n",
                    {driverCategory, unSupCategory});

maplecl::Option<bool> gcSections({"--gc-sections"},
                    "  -gc-sections    \tDiscard all sections that are not accessed in the final elf\n",
                    {driverCategory, ldCategory});

maplecl::Option<bool> copyDtNeededEntries({"--copy-dt-needed-entries"},
                    "  --copy-dt-needed-entries    \tGenerate a DT_NEEDED entry for each lib that is present in"
                    " the link command.\n",
                    {driverCategory, ldCategory});

maplecl::Option<bool> sOpt({"-s"},
                    "  -s    \t\n",
                    {driverCategory, ldCategory});

maplecl::Option<bool> noStdinc({"-nostdinc"},
                    "  -s    \tDo not search standard system include directories"
                    "(those specified with -isystem will still be used).\n",
                    {driverCategory, clangCategory});

maplecl::Option<bool> pie({"-pie"},
                    "  -pie    \tCreate a position independent executable.\n",
                    {driverCategory, ldCategory});

maplecl::Option<bool> fStrongEvalOrder({"-fstrong-eval-order"},
                            "  -fstrong-eval-order    \tFollow the C++17 evaluation order requirements"
                            "for assignment expressions, shift, member function calls, etc.\n",
                            {driverCategory, unSupCategory});

maplecl::Option<bool> linkerTimeOpt({"-flto"},
                            "  -flto                   \tEnable LTO in 'full' mode\n",
                            {driverCategory, unSupCategory});

maplecl::Option<bool> usesignedchar({"-fsigned-char"},
                               "  -fsigned-char          \tuse signed char",
                               {driverCategory, hir2mplCategory});

/* ##################### STRING Options ############################################################### */
maplecl::Option<std::string> help({"--help", "-h"},
                             "  --help                   \tPrint help\n",
                             {driverCategory},
                             maplecl::optionalValue);

maplecl::Option<std::string> infile({"--infile"},
                               "  --infile file1,file2,file3  \tInput files.\n",
                               {driverCategory});

maplecl::Option<std::string> mplt({"--mplt", "-mplt"},
                             "  --mplt=file1,file2,file3    \tImport mplt files.\n",
                             {driverCategory, dex2mplCategory, jbc2mplCategory});

maplecl::Option<std::string> partO2({"--partO2"},
                               "  --partO2               \tSet func list for O2\n",
                               {driverCategory});

maplecl::List<std::string> jbc2mplOpt({"--jbc2mpl-opt"},
                                 "  --jbc2mpl-opt               \tSet options for jbc2mpl\n",
                                 {driverCategory});

maplecl::List<std::string> hir2mplOpt({"--hir2mpl-opt"},
                                 "  --hir2mpl-opt               \tSet options for hir2mpl\n",
                                 {driverCategory});

maplecl::List<std::string> clangOpt({"--clang-opt"},
                               "  --clang-opt               \tSet options for clang as AST generator\n",
                               {driverCategory});

maplecl::List<std::string> asOpt({"--as-opt"},
                            "  --as-opt                  \tSet options for as\n",
                            {driverCategory});

maplecl::List<std::string> ldOpt({"--ld-opt"},
                            "  --ld-opt                  \tSet options for ld\n",
                            {driverCategory});

maplecl::List<std::string> dex2mplOpt({"--dex2mpl-opt"},
                                 "  --dex2mpl-opt             \tSet options for dex2mpl\n",
                                 {driverCategory});

maplecl::List<std::string> mplipaOpt({"--mplipa-opt"},
                                "  --mplipa-opt              \tSet options for mplipa\n",
                                {driverCategory});

maplecl::List<std::string> mplcgOpt({"--mplcg-opt"},
                               "  --mplcg-opt               \tSet options for mplcg\n",
                               {driverCategory});

maplecl::List<std::string> meOpt({"--me-opt"},
                            "  --me-opt                  \tSet options for me\n",
                            {driverCategory});

maplecl::List<std::string> mpl2mplOpt({"--mpl2mpl-opt"},
                                 "  --mpl2mpl-opt             \tSet options for mpl2mpl\n",
                                 {driverCategory});

maplecl::Option<std::string> profile({"--profile"},
                                "  --profile                   \tFor PGO optimization\n"
                                "                              \t--profile=list_file\n",
                                {driverCategory, dex2mplCategory, mpl2mplCategory, cgCategory});

maplecl::Option<std::string> run({"--run"},
                            "  --run=cmd1:cmd2             \tThe name of executables that are going\n"
                            "                              \tto execute. IN SEQUENCE.\n"
                            "                              \tSeparated by \":\".Available exe names:\n"
                            "                              \tjbc2mpl, me, mpl2mpl, mplcg\n"
                            "                              \tInput file must match the tool can\n"
                            "                              \thandle\n",
                            {driverCategory});

maplecl::Option<std::string> optionOpt({"--option"},
                                  "  --option=\"opt1:opt2\"      \tOptions for each executable,\n"
                                  "                              \tseparated by \":\".\n"
                                  "                              \tThe sequence must match the sequence in\n"
                                  "                              \t--run.\n",
                                  {driverCategory});

maplecl::List<std::string> ldLib({"-l"},
                            "  -l <lib>                    \tLinks with a library file\n",
                            {driverCategory, ldCategory},
                            maplecl::joinedValue);

maplecl::List<std::string> ldLibPath({"-L"},
                                "  -L <libpath>                \tAdd directory to library search path\n",
                                {driverCategory, ldCategory},
                                maplecl::joinedValue);

maplecl::List<std::string> enableMacro({"-D"},
                                  "  -D <macro>=<value>          \tDefine <macro> to <value> "
                                  "(or 1 if <value> omitted)\n",
                                  {driverCategory, clangCategory},
                                  maplecl::joinedValue);

maplecl::List<std::string> disableMacro({"-U"},
                                   "  -U <macro>                  \tUndefine macro <macro>\n",
                                   {driverCategory, clangCategory},
                                   maplecl::joinedValue);

maplecl::List<std::string> includeDir({"-I"},
                                 "  -I <dir>                    \tAdd directory to include search path\n",
                                 {driverCategory, clangCategory},
                                 maplecl::joinedValue);

maplecl::List<std::string> includeSystem({"-isystem"},
                                    "  -isystem <dir>              \tAdd directory to SYSTEM include search path\n",
                                    {driverCategory, clangCategory},
                                    maplecl::joinedValue);

maplecl::Option<std::string> output({"-o"},
                               "  -o <outfile>                \tPlace the output into <file>\n",
                               {driverCategory},
                               maplecl::Init("a.out"));

maplecl::Option<std::string> saveTempOpt({"--save-temps"},
                                    "  --save-temps                \tDo not delete intermediate files.\n"
                                    "                              \t--save-temps Save all intermediate files.\n"
                                    "                              \t--save-temps=file1,file2,file3 Save the\n"
                                    "                              \ttarget files.\n",
                                    {driverCategory},
                                    maplecl::optionalValue);

maplecl::Option<std::string> target({"--target", "-target"},
                               "  --target=<arch><abi>        \tDescribe target platform\n"
                               "  \t\t\t\tExample: --target=aarch64-gnu or --target=aarch64_be-gnuilp32\n",
                               {driverCategory, hir2mplCategory, dex2mplCategory, ipaCategory});

maplecl::Option<std::string> MT({"-MT"},
                           "  -MT<args>                   \tSpecify name of main file output in depfile\n",
                           {driverCategory, unSupCategory}, maplecl::joinedValue);

maplecl::Option<std::string> MF({"-MF"},
                           "  -MF<file>                   \tWrite depfile output from -MD, -M to <file>\n",
                           {driverCategory, clangCategory}, maplecl::joinedValue);


maplecl::Option<std::string> std({"-std"},
                            "  -std \t\n",
                            {driverCategory, clangCategory, ldCategory, unSupCategory});

maplecl::Option<std::string> Wl({"-Wl"},
                            "  -Wl,<arg>               \tPass the comma separated arguments in <arg> to the linker\n",
                            {driverCategory, ldCategory}, maplecl::joinedValue);

maplecl::Option<std::string> linkerTimeOptE({"-flto="},
                            "  -flto=<value>           \tSet LTO mode to either 'full' or 'thin'\n",
                            {driverCategory, unSupCategory});

maplecl::Option<std::string> setDefSymVisi({"-fvisibility"},
                            "  -fvisibility=<value>    \tSet the default symbol visibility for all global declarationse",
                            {driverCategory, unSupCategory});

maplecl::Option<std::string> fStrongEvalOrderE({"-fstrong-eval-order="},
                            "  -fstrong-eval-order    \tFollow the C++17 evaluation order requirements"
                            "for assignment expressions, shift, member function calls, etc.\n",
                            {driverCategory, unSupCategory});

maplecl::Option<std::string> march({"-march"},
                            "  -march=    \tGenerate code for given CPU.\n",
                            {driverCategory, clangCategory});

maplecl::Option<std::string> sysRoot({"--sysroot"},
                            "  --sysroot <value>    \tSet the root directory of the target platform.\n"
                            "  --sysroot=<value>    \tSet the root directory of the target platform.\n",
                            {driverCategory, clangCategory});

/* ##################### DIGITAL Options ############################################################### */

maplecl::Option<uint32_t> helpLevel({"--level"},
                               "  --level=NUM                 \tPrint the help info of specified level.\n"
                               "                              \tNUM=0: All options (Default)\n"
                               "                              \tNUM=1: Product options\n"
                               "                              \tNUM=2: Experimental options\n"
                               "                              \tNUM=3: Debug options\n",
                               {driverCategory});

/* ##################### Warnings Options ############################################################### */

maplecl::Option<bool> wUnusedMacro({"-Wunused-macros"},
                              "  -Wunused-macros             \twarning: macro is not used\n",
                              {driverCategory, clangCategory});

maplecl::Option<bool> wBadFunctionCast({"-Wbad-function-cast"},
                                  "  -Wbad-function-cast         \twarning: "
                                  "cast from function call of type A to non-matching type B\n",
                                  {driverCategory, clangCategory});

maplecl::Option<bool> wStrictPrototypes({"-Wstrict-prototypes"},
                                   "  -Wstrict-prototypes         \twarning: "
                                   "Warn if a function is declared or defined without specifying the argument types\n",
                                   {driverCategory, clangCategory});

maplecl::Option<bool> wUndef({"-Wundef"},
                        "  -Wundef                     \twarning: "
                        "Warn if an undefined identifier is evaluated in an #if directive. "
                        "Such identifiers are replaced with zero\n",
                        {driverCategory, clangCategory});

maplecl::Option<bool> wCastQual({"-Wcast-qual"},
                           "  -Wcast-qual                 \twarning: "
                           "Warn whenever a pointer is cast so as to remove a type qualifier from the target type. "
                           "For example, warn if a const char * is cast to an ordinary char *\n",
                           {driverCategory, clangCategory});

maplecl::Option<bool> wMissingFieldInitializers({"-Wmissing-field-initializers"},
                                           "  -Wmissing-field-initializers\twarning: "
                                           "Warn if a structure’s initializer has some fields missing\n",
                                           {driverCategory, clangCategory},
                                           maplecl::DisableWith("-Wno-missing-field-initializers"));

maplecl::Option<bool> wUnusedParameter({"-Wunused-parameter"},
                                  "  -Wunused-parameter       \twarning: "
                                  "Warn whenever a function parameter is unused aside from its declaration\n",
                                  {driverCategory, clangCategory},
                                  maplecl::DisableWith("-Wno-unused-parameter"));

maplecl::Option<bool> wAll({"-Wall"},
                      "  -Wall                    \tThis enables all the warnings about constructions "
                      "that some users consider questionable\n",
                      {driverCategory, clangCategory});

maplecl::Option<bool> wExtra({"-Wextra"},
                        "  -Wextra                  \tEnable some extra warning flags that are not enabled by -Wall\n",
                        {driverCategory, clangCategory});

maplecl::Option<bool> wWriteStrings({"-Wwrite-strings"},
                               "  -Wwrite-strings          \tWhen compiling C, give string constants the type "
                               "const char[length] so that copying the address of one into "
                               "a non-const char * pointer produces a warning\n",
                               {driverCategory, clangCategory});

maplecl::Option<bool> wVla({"-Wvla"},
                      "  -Wvla                    \tWarn if a variable-length array is used in the code\n",
                      {driverCategory, clangCategory});

maplecl::Option<bool> wFormatSecurity({"-Wformat-security"},
                                 "  -Wformat-security                    \tWwarn about uses of format "
                                 "functions that represent possible security problems\n",
                                 {driverCategory, clangCategory});

maplecl::Option<bool> wShadow({"-Wshadow"},
                         "  -Wshadow                             \tWarn whenever a local variable "
                         "or type declaration shadows another variable\n",
                         {driverCategory, clangCategory});

maplecl::Option<bool> wTypeLimits({"-Wtype-limits"},
                             "  -Wtype-limits                         \tWarn if a comparison is always true or always "
                             "false due to the limited range of the data type\n",
                             {driverCategory, clangCategory});

maplecl::Option<bool> wSignCompare({"-Wsign-compare"},
                              "  -Wsign-compare                         \tWarn when a comparison between signed and "
                              " unsigned values could produce an incorrect result when the signed value is converted "
                              "to unsigned\n",
                              {driverCategory, clangCategory});

maplecl::Option<bool> wShiftNegativeValue({"-Wshift-negative-value"},
                                     "  -Wshift-negative-value                 \tWarn if left "
                                     "shifting a negative value\n",
                                     {driverCategory, clangCategory});

maplecl::Option<bool> wPointerArith({"-Wpointer-arith"},
                                "  -Wpointer-arith                        \tWarn about anything that depends on the "
                                "“size of” a function type or of void\n",
                               {driverCategory, clangCategory});

maplecl::Option<bool> wIgnoredQualifiers({"-Wignored-qualifiers"},
                                    "  -Wignored-qualifiers                   \tWarn if the return type of a "
                                    "function has a type qualifier such as const\n",
                                    {driverCategory, clangCategory});

maplecl::Option<bool> wFormat({"-Wformat"},
                         "  -Wformat                     \tCheck calls to printf and scanf, etc., "
                         "to make sure that the arguments supplied have types appropriate "
                         "to the format string specified\n",
                         {driverCategory, clangCategory});

maplecl::Option<bool> wFloatEqual({"-Wfloat-equal"},
                             "  -Wfloat-equal                \tWarn if floating-point values are used "
                             "in equality comparisons\n",
                             {driverCategory, clangCategory});

maplecl::Option<bool> wDateTime({"-Wdate-time"},
                           "  -Wdate-time                  \tWarn when macros __TIME__, __DATE__ or __TIMESTAMP__ "
                           "are encountered as they might prevent bit-wise-identical reproducible compilations\n",
                           {driverCategory, clangCategory});

maplecl::Option<bool> wImplicitFallthrough({"-Wimplicit-fallthrough"},
                                      "  -Wimplicit-fallthrough       \tWarn when a switch case falls through\n",
                                      {driverCategory, clangCategory});

maplecl::Option<bool> wShiftOverflow({"-Wshift-overflow"},
                                "  -Wshift-overflow             \tWarn about left shift overflows\n",
                                {driverCategory, clangCategory},
                                maplecl::DisableWith("-Wno-shift-overflow"));

/* #################################################################################################### */

} /* namespace opts */
