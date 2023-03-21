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
                    {driverCategory, hir2mplCategory});

maplecl::Option<bool> o3({"--O3", "-O3"},
                    "  -O3                         \tDo more optimization.\n",
                    {driverCategory});

maplecl::Option<bool> os({"--Os", "-Os"},
                    "  -Os                         \tOptimize for size, based on O2.\n",
                    {driverCategory});

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
                           {driverCategory, hir2mplCategory});

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

maplecl::Option<bool> npeDynamicCheck({"--npe-check-dynamic", "-npe-check-dynamic"},
                                 "  --npe-check-dynamic         \tEnable null "
                                 "pointer dynamic check with static warning\n",
                                 {driverCategory, hir2mplCategory});

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

maplecl::Option<bool> boundaryDynamicCheck({"--boundary-check-dynamic", "-boundary-check-dynamic"},
                                      "  --boundary-check-dynamic    \tEnable boundary dynamic check "
                                      "with static warning\n",
                                      {driverCategory, hir2mplCategory});

maplecl::Option<bool> boundaryDynamicCheckSilent({"--boundary-check-dynamic-silent"},
                                            "  --boundary-check-dynamic-silent  \tEnable boundary dynamic "
                                            "check without static warning\n",
                                            {driverCategory});

maplecl::Option<bool> safeRegionOption({"--safe-region", "-safe-region"},
                                  "  --safe-region               \tEnable safe region\n",
                                  {driverCategory, hir2mplCategory});

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

maplecl::Option<bool> fNoPlt({"-fno-plt"},
                    "  -fno-plt                \tDo not use the PLT to make function calls\n",
                    {driverCategory});

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

maplecl::Option<bool> gcSections({"--gc-sections"},
                    "  -gc-sections    \tDiscard all sections that are not accessed in the final elf\n",
                    {driverCategory, ldCategory});

maplecl::Option<bool> copyDtNeededEntries({"--copy-dt-needed-entries"},
                    "  --copy-dt-needed-entries    \tGenerate a DT_NEEDED entry for each lib that is present in"
                    " the link command.\n",
                    {driverCategory, ldCategory});

maplecl::Option<bool> sOpt({"-s"},
                    "  -s    \tRemove all symbol table and relocation information from the executable\n",
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

maplecl::Option<bool> shared({"-shared"},
                               "  -shared          \tCreate a shared library.\n",
                               {driverCategory, ldCategory});

maplecl::Option<bool> rdynamic({"-rdynamic"},
                               "  -rdynamic          \tPass the flag `-export-dynamic' to the ELF linker,"
                               "on targets that support it. This instructs the linker to add all symbols,"
                               "not only used ones, to the dynamic symbol table.\n",
                               {driverCategory, ldCategory});

maplecl::Option<bool> dndebug({"-DNDEBUG"},
                               "  -DNDEBUG          \t\n",
                               {driverCategory, ldCategory});

maplecl::Option<bool> usesignedchar({"-fsigned-char", "-usesignedchar", "--usesignedchar"},
                               "  -fsigned-char         \tuse signed char\n",
                               {driverCategory, clangCategory, hir2mplCategory},
                               maplecl::DisableWith("-funsigned-char"));

maplecl::Option<bool> suppressWarnings({"-w"},
                               "  -w         \tSuppress all warnings.\n",
                               {driverCategory, clangCategory, asCategory, ldCategory});

maplecl::Option<bool> pthread({"-pthread"},
                               "  -pthread         \tDefine additional macros required for using"
                               "the POSIX threads library.\n",
                               {driverCategory, clangCategory, asCategory, ldCategory});

maplecl::Option<bool> passO2ToClang({"-pO2ToCl"},
                               "  -pthread         \ttmp for option -D_FORTIFY_SOURCE=1\n",
                               {clangCategory});

maplecl::Option<bool> defaultSafe({"-defaultSafe", "--defaultSafe"},
                                  "  --defaultSafe     : treat unmarked function or blocks as safe region by default",
                                  {driverCategory, hir2mplCategory});

maplecl::Option<bool> onlyPreprocess({"-E"},
                                  "  -E    \tPreprocess only; do not compile, assemble or link.\n",
                                  {driverCategory, clangCategory});

maplecl::Option<bool> tailcall({"--tailcall", "-foptimize-sibling-calls"},
                               "  --tailcall/-foptimize-sibling-calls                   \tDo tail call optimization\n"
                               "  --no-tailcall/-fno-optimize-sibling-calls\n",
                               {cgCategory, driverCategory},
                               maplecl::DisableEvery({"-fno-optimize-sibling-calls", "--no-tailcall"}));

maplecl::Option<bool> noStdLib({"-nostdlib"},
                                  "  -nostdlib    \tDo not look for object files in standard path.\n",
                                  {driverCategory, ldCategory});

maplecl::Option<bool> r({"-r"},
                                  "  -r    \tProduce a relocatable object as output. This is also"
                                  " known as partial linking.\n",
                                  {driverCategory, ldCategory});

maplecl::Option<bool> wCCompat({"-Wc++-compat"},
                            "  -Wc++-compat    \tWarn about C constructs that are not in the "
                            "common subset of C and C++ .\n",
                            {driverCategory, asCategory, ldCategory});

maplecl::Option<bool> wpaa({"-wpaa", "--wpaa"},
                      "  -dump-cfg funcname1,funcname2\n"               \
                      "  -wpaa                  : enable whole program ailas analysis",
                      {driverCategory, hir2mplCategory});

maplecl::Option<bool> dumpTime({"--dump-time", "-dump-time"},
                          "  -dump-time             : dump time",
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
                               {driverCategory, clangCategory, hir2mplCategory, dex2mplCategory, ipaCategory});

maplecl::Option<std::string> MT({"-MT"},
                           "  -MT<args>                   \tSpecify name of main file output in depfile\n",
                           {driverCategory, unSupCategory}, maplecl::joinedValue);

maplecl::Option<std::string> MF({"-MF"},
                           "  -MF<file>                   \tWrite depfile output from -MD, -M to <file>\n",
                           {driverCategory, clangCategory}, maplecl::joinedValue);

maplecl::Option<std::string> Wl({"-Wl"},
                            "  -Wl,<arg>               \tPass the comma separated arguments in <arg> to the linker\n",
                            {driverCategory, ldCategory}, maplecl::joinedValue);

maplecl::Option<std::string> linkerTimeOptE({"-flto="},
                            "  -flto=<value>           \tSet LTO mode to either 'full' or 'thin'\n",
                            {driverCategory, unSupCategory});

maplecl::Option<std::string> fVisibility({"-fvisibility"},
                            "  -fvisibility=[default|hidden|protected|internal]    \tSet the default symbol visibility"
                            " for every global declaration unless overridden within the code\n",
                            {driverCategory});

maplecl::Option<std::string> fStrongEvalOrderE({"-fstrong-eval-order="},
                            "  -fstrong-eval-order    \tFollow the C++17 evaluation order requirements"
                            "for assignment expressions, shift, member function calls, etc.\n",
                            {driverCategory, unSupCategory});

maplecl::Option<std::string> marchE({"-march="},
                            "  -march=    \tGenerate code for given CPU.\n",
                            {driverCategory, clangCategory, asCategory, ldCategory, unSupCategory});

maplecl::Option<std::string> sysRoot({"--sysroot"},
                            "  --sysroot <value>    \tSet the root directory of the target platform.\n"
                            "  --sysroot=<value>    \tSet the root directory of the target platform.\n",
                            {driverCategory, clangCategory, ldCategory});

maplecl::Option<std::string> specs({"-specs"},
                            "  -specs <value>    \tOverride built-in specs with the contents of <file>.\n",
                            {driverCategory, ldCategory});

maplecl::Option<std::string> folder({"-tmp-folder"},
                            "  -p <value>    \tsave former folder when generating multiple output.\n",
                            {driverCategory});

maplecl::Option<std::string> imacros({"-imacros", "--imacros"},
                            "  -imacros    \tExactly like `-include', except that any output produced by"
                            " scanning FILE is thrown away.\n",
                            {driverCategory, clangCategory});

maplecl::Option<std::string> fdiagnosticsColor({"-fdiagnostics-color"},
                            "  -fdiagnostics-color    \tUse color in diagnostics.  WHEN is 'never',"
                            " 'always', or 'auto'.\n",
                            {driverCategory, clangCategory, asCategory, ldCategory});

maplecl::Option<std::string> mtlsSize({"-mtls-size"},
                            "  -mtls-size      \tSpecify bit size of immediate TLS offsets. Valid values are 12, "
                            "24, 32, 48. This option requires binutils 2.26 or newer.\n",
                            {driverCategory, asCategory, ldCategory});

/* ##################### DIGITAL Options ############################################################### */

maplecl::Option<uint32_t> helpLevel({"--level"},
                               "  --level=NUM                 \tPrint the help info of specified level.\n"
                               "                              \tNUM=0: All options (Default)\n"
                               "                              \tNUM=1: Product options\n"
                               "                              \tNUM=2: Experimental options\n"
                               "                              \tNUM=3: Debug options\n",
                               {driverCategory});

maplecl::Option<uint32_t> funcInliceSize({"-func-inline-size", "--func-inline-size"},
                                    "  -func-inline-size      : set func inline size",
                                    {driverCategory, hir2mplCategory});

/* ##################### Warnings Options ############################################################### */

maplecl::Option<bool> wUnusedMacro({"-Wunused-macros"},
                              "  -Wunused-macros             \twarning: macro is not used\n",
                              {driverCategory, clangCategory});

maplecl::Option<bool> wBadFunctionCast({"-Wbad-function-cast"},
                                  "  -Wbad-function-cast         \twarning: "
                                  "cast from function call of type A to non-matching type B\n",
                                  {driverCategory, clangCategory},
                                  maplecl::DisableWith("-Wno-bad-function-cast"));

maplecl::Option<bool> wStrictPrototypes({"-Wstrict-prototypes"},
                                   "  -Wstrict-prototypes         \twarning: "
                                   "Warn if a function is declared or defined without specifying the argument types\n",
                                   {driverCategory, clangCategory},
                                   maplecl::DisableWith("-Wno-strict-prototypes"));

maplecl::Option<bool> wUndef({"-Wundef"},
                        "  -Wundef                     \twarning: "
                        "Warn if an undefined identifier is evaluated in an #if directive. "
                        "Such identifiers are replaced with zero\n",
                        {driverCategory, clangCategory},
                        maplecl::DisableWith("-Wno-undef"));

maplecl::Option<bool> wCastQual({"-Wcast-qual"},
                           "  -Wcast-qual                 \twarning: "
                           "Warn whenever a pointer is cast so as to remove a type qualifier from the target type. "
                           "For example, warn if a const char * is cast to an ordinary char *\n",
                           {driverCategory, clangCategory},
                           maplecl::DisableWith("-Wno-cast-qual"));

maplecl::Option<bool> wMissingFieldInitializers({"-Wmissing-field-initializers"},
                                           "  -Wmissing-field-initializers\twarning: "
                                           "Warn if a structure's initializer has some fields missing\n",
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
                      {driverCategory, clangCategory},
                      maplecl::DisableWith("-Wno-all"));

maplecl::Option<bool> wExtra({"-Wextra"},
                        "  -Wextra                  \tEnable some extra warning flags that are not enabled by -Wall\n",
                        {driverCategory, clangCategory},
                        maplecl::DisableWith("-Wno-extra"));

maplecl::Option<bool> wWriteStrings({"-Wwrite-strings"},
                               "  -Wwrite-strings          \tWhen compiling C, give string constants the type "
                               "const char[length] so that copying the address of one into "
                               "a non-const char * pointer produces a warning\n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-write-strings"));

maplecl::Option<bool> wVla({"-Wvla"},
                      "  -Wvla                    \tWarn if a variable-length array is used in the code\n",
                      {driverCategory, clangCategory},
                      maplecl::DisableWith("-Wno-vla"));

maplecl::Option<bool> wFormatSecurity({"-Wformat-security"},
                                 "  -Wformat-security                    \tWwarn about uses of format "
                                 "functions that represent possible security problems\n",
                                 {driverCategory, clangCategory},
                                 maplecl::DisableWith("-Wno-format-security"));

maplecl::Option<bool> wShadow({"-Wshadow"},
                         "  -Wshadow                             \tWarn whenever a local variable "
                         "or type declaration shadows another variable\n",
                         {driverCategory, clangCategory},
                         maplecl::DisableWith("-Wno-shadow"));

maplecl::Option<bool> wTypeLimits({"-Wtype-limits"},
                             "  -Wtype-limits                         \tWarn if a comparison is always true or always "
                             "false due to the limited range of the data type\n",
                             {driverCategory, clangCategory},
                             maplecl::DisableWith("-Wno-type-limits"));

maplecl::Option<bool> wSignCompare({"-Wsign-compare"},
                              "  -Wsign-compare                         \tWarn when a comparison between signed and "
                              " unsigned values could produce an incorrect result when the signed value is converted "
                              "to unsigned\n",
                              {driverCategory, clangCategory},
                              maplecl::DisableWith("-Wno-sign-compare"));

maplecl::Option<bool> wShiftNegativeValue({"-Wshift-negative-value"},
                                     "  -Wshift-negative-value                 \tWarn if left "
                                     "shifting a negative value\n",
                                     {driverCategory, clangCategory},
                                      maplecl::DisableWith("-Wno-shift-negative-value"));

maplecl::Option<bool> wPointerArith({"-Wpointer-arith"},
                                "  -Wpointer-arith                        \tWarn about anything that depends on the "
                                "“size of” a function type or of void\n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-pointer-arith"));

maplecl::Option<bool> wIgnoredQualifiers({"-Wignored-qualifiers"},
                                    "  -Wignored-qualifiers                   \tWarn if the return type of a "
                                    "function has a type qualifier such as const\n",
                                    {driverCategory, clangCategory},
                                    maplecl::DisableWith("-Wno-ignored-qualifiers"));

maplecl::Option<bool> wFormat({"-Wformat"},
                         "  -Wformat                     \tCheck calls to printf and scanf, etc., "
                         "to make sure that the arguments supplied have types appropriate "
                         "to the format string specified\n",
                         {driverCategory, clangCategory},
                         maplecl::DisableWith("-Wno-format"));

maplecl::Option<bool> wFloatEqual({"-Wfloat-equal"},
                             "  -Wfloat-equal                \tWarn if floating-point values are used "
                             "in equality comparisons\n",
                             {driverCategory, clangCategory},
                             maplecl::DisableWith("-Wno-float-equal"));

maplecl::Option<bool> wDateTime({"-Wdate-time"},
                           "  -Wdate-time                  \tWarn when macros __TIME__, __DATE__ or __TIMESTAMP__ "
                           "are encountered as they might prevent bit-wise-identical reproducible compilations\n",
                           {driverCategory, clangCategory},
                           maplecl::DisableWith("-Wno-date-time"));

maplecl::Option<bool> wImplicitFallthrough({"-Wimplicit-fallthrough"},
                                      "  -Wimplicit-fallthrough       \tWarn when a switch case falls through\n",
                                      {driverCategory, clangCategory},
                                      maplecl::DisableWith("-Wno-implicit-fallthrough"));

maplecl::Option<bool> wShiftOverflow({"-Wshift-overflow"},
                                "  -Wshift-overflow             \tWarn about left shift overflows\n",
                                {driverCategory, clangCategory},
                                maplecl::DisableWith("-Wno-shift-overflow"));

maplecl::Option<bool> Wnounusedcommandlineargument({"-Wno-unused-command-line-argument"},
                                "  -Wno-unused-command-line-argument             \tno unused command line argument\n",
                                {driverCategory, clangCategory});

maplecl::Option<bool> Wnoconstantconversion({"-Wno-constant-conversion"},
                                "  -Wno-constant-conversion             \tno constant conversion\n",
                                {driverCategory, clangCategory});

maplecl::Option<bool> Wnounknownwarningoption({"-Wno-unknown-warning-option"},
                                "  -Wno-unknown-warning-option             \tno unknown warning option\n",
                                {driverCategory, clangCategory});

maplecl::Option<bool> W({"-W"},
                               "  -W             \tThis switch is deprecated; use -Wextra instead.\n",
                               {driverCategory, clangCategory});

maplecl::Option<bool> Wabi({"-Wabi"},
                               "  -Wabi             \tWarn about things that will change when compiling with an "
                               "ABI-compliant compiler.\n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-abi"));

maplecl::Option<bool> WabiTag({"-Wabi-tag"},
                               "  -Wabi-tag             \tWarn if a subobject has an abi_tag attribute that the "
                               "complete object type does not have.\n",
                               {driverCategory, clangCategory});

maplecl::Option<bool> WaddrSpaceConvert({"-Waddr-space-convert"},
                               "  -Waddr-space-convert             \tWarn about conversions between address spaces in"
                               " the case where the resulting address space is not contained in the incoming address "
                               "space.\n",
                               {driverCategory, clangCategory});

maplecl::Option<bool> Waddress({"-Waddress"},
                               "  -Waddress             \tWarn about suspicious uses of memory addresses.\n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-address"));

maplecl::Option<bool> WaggregateReturn({"-Waggregate-return"},
                               "  -Waggregate-return             \tWarn about returning structures\n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-aggregate-return"));

maplecl::Option<bool> WaggressiveLoopOptimizations({"-Waggressive-loop-optimizations"},
                               "  -Waggressive-loop-optimizations             \tWarn if a loop with constant number "
                               "of iterations triggers undefined behavior.\n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-aggressive-loop-optimizations"));

maplecl::Option<bool> WalignedNew({"-Waligned-new"},
                               "  -Waligned-new             \tWarn about 'new' of type with extended alignment "
                               "without -faligned-new.\n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-aligned-new"));

maplecl::Option<bool> WallocZero({"-Walloc-zero"},
                               "  -Walloc-zero             \t-Walloc-zero Warn for calls to allocation functions that"
                               " specify zero bytes.\n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-alloc-zero"));

maplecl::Option<bool> Walloca({"-Walloca"},
                               "  -Walloca             \tWarn on any use of alloca.\n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-alloca"));

maplecl::Option<bool> WarrayBounds({"-Warray-bounds"},
                               "  -Warray-bounds             \tWarn if an array is accessed out of bounds.\n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-array-bounds"));

maplecl::Option<bool> WassignIntercept({"-Wassign-intercept"},
                               "  -Wassign-intercept             \tWarn whenever an Objective-C assignment is being"
                               " intercepted by the garbage collector.\n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-assign-intercept"));

maplecl::Option<bool> Wattributes({"-Wattributes"},
                               "  -Wattributes             \tWarn about inappropriate attribute usage.\n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-attributes"));

maplecl::Option<bool> WboolCompare({"-Wbool-compare"},
                               "  -Wbool-compare             \tWarn about boolean expression compared with an "
                               "integer value different from true/false.\n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-bool-compare"));

maplecl::Option<bool> WboolOperation({"-Wbool-operation"},
                               "  -Wbool-operation             \tWarn about certain operations on boolean"
                               " expressions.\n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-bool-operation"));

maplecl::Option<bool> WbuiltinDeclarationMismatch({"-Wbuiltin-declaration-mismatch"},
                               "  -Wbuiltin-declaration-mismatch             \tWarn when a built-in function is"
                               " declared with the wrong signature.\n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-builtin-declaration-mismatch"));

maplecl::Option<bool> WbuiltinMacroRedefined({"-Wbuiltin-macro-redefined"},
                               "  -Wbuiltin-macro-redefined             \tWarn when a built-in preprocessor macro "
                               "is undefined or redefined.\n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-builtin-macro-redefined"));

maplecl::Option<bool> W11Compat({"-Wc++11-compat"},
                               "  -Wc++11-compat             \tWarn about C++ constructs whose meaning differs between"
                               " ISO C++ 1998 and ISO C++ 2011.\n",
                               {driverCategory, clangCategory});

maplecl::Option<bool> W14Compat({"-Wc++14-compat"},
                               "  -Wc++14-compat             \tWarn about C++ constructs whose meaning differs between"
                               " ISO C++ 2011 and ISO C++ 2014.\n",
                               {driverCategory, clangCategory});

maplecl::Option<bool> W1zCompat({"-Wc++1z-compat"},
                               "  -Wc++1z-compat             \tWarn about C++ constructs whose meaning differs between"
                               " ISO C++ 2014 and (forthcoming) ISO C++ 201z(7?).\n",
                               {driverCategory, clangCategory});

maplecl::Option<bool> Wc90C99Compat({"-Wc90-c99-compat"},
                               "  -Wc90-c99-compat             \tWarn about features not present in ISO C90\n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-c90-c99-compat"));

maplecl::Option<bool> Wc99C11Compat({"-Wc99-c11-compat"},
                               "  -Wc99-c11-compat             \tWarn about features not present in ISO C99\n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-c99-c11-compat"));

maplecl::Option<bool> WcastAlign({"-Wcast-align"},
                               "  -Wcast-align             \tWarn about pointer casts which increase alignment.\n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-cast-align"));

maplecl::Option<bool> WcharSubscripts({"-Wchar-subscripts"},
                               "  -Wchar-subscripts             \tWarn about subscripts whose type is \"char\".\n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-char-subscripts"));

maplecl::Option<bool> Wchkp({"-Wchkp"},
                               "  -Wchkp             \tWarn about memory access errors found by Pointer Bounds "
                               "Checker.\n",
                               {driverCategory, clangCategory});

maplecl::Option<bool> Wclobbered({"-Wclobbered"},
                               "  -Wclobbered             \tWarn about variables that might be changed "
                               "by \"longjmp\" or \"vfork\".\n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-clobbered"));

maplecl::Option<bool> Wcomment({"-Wcomment"},
                               "  -Wcomment             \tWarn about possibly nested block comments\n",
                               {driverCategory, clangCategory});

maplecl::Option<bool> Wcomments({"-Wcomments"},
                               "  -Wcomments             \tSynonym for -Wcomment.\n",
                               {driverCategory, clangCategory});

maplecl::Option<bool> WconditionallySupported({"-Wconditionally-supported"},
                               "  -Wconditionally-supported             \tWarn for conditionally-supported"
                               " constructs.\n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-conditionally-supported"));

maplecl::Option<bool> Wconversion({"-Wconversion"},
                               "  -Wconversion             \tWarn for implicit type conversions that may "
                               "change a value.\n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-conversion"));

maplecl::Option<bool> WconversionNull({"-Wconversion-null"},
                               "  -Wconversion-null             \tWarn for converting NULL from/to"
                               " a non-pointer type.\n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-conversion-null"));

maplecl::Option<bool> WctorDtorPrivacy({"-Wctor-dtor-privacy"},
                               "  -Wctor-dtor-privacy             \tWarn when all constructors and destructors are"
                               " private.\n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-ctor-dtor-privacy"));

maplecl::Option<bool> WdanglingElse({"-Wdangling-else"},
                               "  -Wdangling-else             \tWarn about dangling else.\n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-dangling-else"));

maplecl::Option<bool> WdeclarationAfterStatement({"-Wdeclaration-after-statement"},
                               "  -Wdeclaration-after-statement             \tWarn when a declaration is found after"
                               " a statement.\n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-declaration-after-statement"));

maplecl::Option<bool> WdeleteIncomplete({"-Wdelete-incomplete"},
                               "  -Wdelete-incomplete             \tWarn when deleting a pointer to incomplete type.\n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-delete-incomplete"));

maplecl::Option<bool> WdeleteNonVirtualDtor({"-Wdelete-non-virtual-dtor"},
                               "  -Wdelete-non-virtual-dtor             \tWarn about deleting polymorphic objects "
                               "with non-virtual destructors.\n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-delete-non-virtual-dtor"));

maplecl::Option<bool> Wdeprecated({"-Wdeprecated"},
                               "  -Wdeprecated             \tWarn if a deprecated compiler feature\n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-deprecated"));

maplecl::Option<bool> WdeprecatedDeclarations({"-Wdeprecated-declarations"},
                               "  -Wdeprecated-declarations             \tWarn about uses of "
                               "__attribute__((deprecated)) declarations.\n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-deprecated-declarations"));

maplecl::Option<bool> WdisabledOptimization({"-Wdisabled-optimization"},
                               "  -Wdisabled-optimization             \tWarn when an optimization pass is disabled.\n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-disabled-optimization"));

maplecl::Option<bool> WdiscardedArrayQualifiers({"-Wdiscarded-array-qualifiers"},
                               "  -Wdiscarded-array-qualifiers             \tWarn if qualifiers on arrays which "
                               "are pointer targets are discarded.\n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-discarded-array-qualifiers"));

maplecl::Option<bool> WdiscardedQualifiers({"-Wdiscarded-qualifiers"},
                               "  -Wdiscarded-qualifiers             \tWarn if type qualifiers on pointers are"
                               " discarded.\n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-discarded-qualifiers"));

maplecl::Option<bool> WdivByZero({"-Wdiv-by-zero"},
                               "  -Wdiv-by-zero             \tWarn about compile-time integer division by zero.\n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-div-by-zero"));

maplecl::Option<bool> WdoublePromotion({"-Wdouble-promotion"},
                               "  -Wdouble-promotion             \tWarn about implicit conversions from "
                               "\"float\" to \"double\".\n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-double-promotion"));

maplecl::Option<bool> WduplicateDeclSpecifier({"-Wduplicate-decl-specifier"},
                               "  -Wduplicate-decl-specifier             \tWarn when a declaration has "
                               "duplicate const\n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-duplicate-decl-specifier"));

maplecl::Option<bool> WduplicatedBranches({"-Wduplicated-branches"},
                               "  -Wduplicated-branches             \tWarn about duplicated branches in "
                               "if-else statements.\n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-duplicated-branches"));

maplecl::Option<bool> WduplicatedCond({"-Wduplicated-cond"},
                               "  -Wduplicated-cond             \tWarn about duplicated conditions in an "
                               "if-else-if chain.\n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-duplicated-cond"));

maplecl::Option<bool> Weak_reference_mismatches({"-weak_reference_mismatches"},
                               "  -weak_reference_mismatches             \t\n",
                               {driverCategory, clangCategory});

maplecl::Option<bool> Weffc({"-Weffc++"},
                               "  -Weffc++             \tWarn about violations of Effective C++ style rules.\n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-effc++"));

maplecl::Option<bool> WemptyBody({"-Wempty-body"},
                               "  -Wempty-body             \tWarn about an empty body in an if or else statement.\n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-empty-body"));

maplecl::Option<bool> WendifLabels({"-Wendif-labels"},
                               "  -Wendif-labels             \tWarn about stray tokens after #else and #endif.\n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-endif-labels"));

maplecl::Option<bool> WenumCompare({"-Wenum-compare"},
                               "  -Wenum-compare             \tWarn about comparison of different enum types.\n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-enum-compare"));

maplecl::Option<bool> Werror({"-Werror"},
                               "  -Werror             \tTreat all warnings as errors.\n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-error"));

maplecl::Option<std::string> WerrorE({"-Werror="},
                               "  -Werror=             \tTreat specified warning as error.\n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-error="));

maplecl::Option<bool> WexpansionToDefined({"-Wexpansion-to-defined"},
                               "  -Wexpansion-to-defined             \tWarn if \"defined\" is used outside #if.\n",
                               {driverCategory, clangCategory});

maplecl::Option<bool> WfatalErrors({"-Wfatal-errors"},
                               "  -Wfatal-errors             \tExit on the first error occurred.\n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-fatal-errors"));

maplecl::Option<bool> WfloatConversion({"-Wfloat-conversion"},
                               "  -Wfloat-conversion             \tWarn for implicit type conversions that cause "
                               "loss of floating point precision.\n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-float-conversion"));

maplecl::Option<bool> WformatContainsNul({"-Wformat-contains-nul"},
                               "  -Wformat-contains-nul             \tWarn about format strings that contain NUL"
                               " bytes.\n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-format-contains-nul"));

maplecl::Option<bool> WformatExtraArgs({"-Wformat-extra-args"},
                               "  -Wformat-extra-args             \tWarn if passing too many arguments to a function"
                               " for its format string.\n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-format-extra-args"));

maplecl::Option<bool> WformatNonliteral({"-Wformat-nonliteral"},
                               "  -Wformat-nonliteral             \tWarn about format strings that are not literals.\n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-format-nonliteral"));

maplecl::Option<bool> WformatOverflow({"-Wformat-overflow"},
                               "  -Wformat-overflow             \tWarn about function calls with format strings "
                               "that write past the end of the destination region.  Same as -Wformat-overflow=1.\n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-format-overflow"));

maplecl::Option<bool> WformatSignedness({"-Wformat-signedness"},
                               "  -Wformat-signedness             \tWarn about sign differences with format "
                               "functions.\n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-format-signedness"));

maplecl::Option<bool> WformatTruncation({"-Wformat-truncation"},
                               "  -Wformat-truncation             \tWarn about calls to snprintf and similar "
                               "functions that truncate output. Same as -Wformat-truncation=1.\n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-format-truncation"));

maplecl::Option<bool> WformatY2k({"-Wformat-y2k"},
                               "  -Wformat-y2k             \tWarn about strftime formats yielding 2-digit years.\n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-format-y2k"));

maplecl::Option<bool> WformatZeroLength({"-Wformat-zero-length"},
                               "  -Wformat-zero-length             \tWarn about zero-length formats.\n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-format-zero-length"));

maplecl::Option<bool> WframeAddress({"-Wframe-address"},
                               "  -Wframe-address             \tWarn when __builtin_frame_address or"
                               " __builtin_return_address is used unsafely.\n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-frame-address"));

maplecl::Option<bool> WframeLargerThan({"-Wframe-larger-than"},
                               "  -Wframe-larger-than             \t\n",
                               {driverCategory, clangCategory});

maplecl::Option<bool> WfreeNonheapObject({"-Wfree-nonheap-object"},
                               "  -Wfree-nonheap-object             \tWarn when attempting to free a non-heap"
                               " object.\n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-free-nonheap-object"));

maplecl::Option<bool> WignoredAttributes({"-Wignored-attributes"},
                               "  -Wignored-attributes             \tWarn whenever attributes are ignored.\n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-ignored-attributes"));

maplecl::Option<bool> Wimplicit({"-Wimplicit"},
                               "  -Wimplicit             \tWarn about implicit declarations.\n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-implicit"));

maplecl::Option<bool> WimplicitFunctionDeclaration({"-Wimplicit-function-declaration"},
                               "  -Wimplicit-function-declaration             \tWarn about implicit function "
                               "declarations.\n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-implicit-function-declaration"));

maplecl::Option<bool> WimplicitInt({"-Wimplicit-int"},
                               "  -Wimplicit-int             \tWarn when a declaration does not specify a type.\n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-implicit-int"));

maplecl::Option<bool> WincompatiblePointerTypes({"-Wincompatible-pointer-types"},
                               "  -Wincompatible-pointer-types             \tWarn when there is a conversion "
                               "between pointers that have incompatible types.\n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-incompatible-pointer-types"));

maplecl::Option<bool> WinheritedVariadicCtor({"-Winherited-variadic-ctor"},
                               "  -Winherited-variadic-ctor             \tWarn about C++11 inheriting constructors "
                               "when the base has a variadic constructor.\n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-inherited-variadic-ctor"));

maplecl::Option<bool> WinitSelf({"-Winit-self"},
                               "  -Winit-self             \tWarn about variables which are initialized to "
                               "themselves.\n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-init-self"));

maplecl::Option<bool> Winline({"-Winline"},
                               "  -Winline             \tWarn when an inlined function cannot be inlined.\n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-inline"));

maplecl::Option<bool> WintConversion({"-Wint-conversion"},
                               "  -Wint-conversion             \tWarn about incompatible integer to pointer and"
                               " pointer to integer conversions.\n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-int-conversion"));

maplecl::Option<bool> WintInBoolContext({"-Wint-in-bool-context"},
                               "  -Wint-in-bool-context             \tWarn for suspicious integer expressions in"
                               " boolean context.\n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-int-in-bool-context"));

maplecl::Option<bool> WintToPointerCast({"-Wint-to-pointer-cast"},
                               "  -Wint-to-pointer-cast             \tWarn when there is a cast to a pointer from"
                               " an integer of a different size.\n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-int-to-pointer-cast"));

maplecl::Option<bool> WinvalidMemoryModel({"-Winvalid-memory-model"},
                               "  -Winvalid-memory-model             \tWarn when an atomic memory model parameter"
                               " is known to be outside the valid range.\n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-invalid-memory-model"));

maplecl::Option<bool> WinvalidOffsetof({"-Winvalid-offsetof"},
                               "  -Winvalid-offsetof             \tWarn about invalid uses of "
                               "the \"offsetof\" macro.\n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-invalid-offsetof"));

maplecl::Option<bool> WLiteralSuffix({"-Wliteral-suffix"},
                               "  -Wliteral-suffix             \tWarn when a string or character literal is "
                               "followed by a ud-suffix which does not begin with an underscore.\n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-literal-suffix"));

maplecl::Option<bool> WLogicalNotParentheses({"-Wlogical-not-parentheses"},
                               "  -Wlogical-not-parentheses             \tWarn about logical not used on the left"
                               " hand side operand of a comparison.\n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-logical-not-parentheses"));

maplecl::Option<bool> WinvalidPch({"-Winvalid-pch"},
                               "  -Winvalid-pch             \tWarn about PCH files that are found but not used.\n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-invalid-pch"));

maplecl::Option<bool> WjumpMissesInit({"-Wjump-misses-init"},
                               "  -Wjump-misses-init             \tWarn when a jump misses a variable "
                               "initialization.\n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-jump-misses-init"));

maplecl::Option<bool> WLogicalOp({"-Wlogical-op"},
                               "  -Wlogical-op             \tWarn about suspicious uses of logical "
                               "operators in expressions. \n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-logical-op"));

maplecl::Option<bool> WLongLong({"-Wlong-long"},
                               "  -Wlong-long             \tWarn if long long type is used.\n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-long-long"));

maplecl::Option<bool> Wmain({"-Wmain"},
                               "  -Wmain             \tWarn about suspicious declarations of \"main\".\n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-main"));

maplecl::Option<bool> WmaybeUninitialized({"-Wmaybe-uninitialized"},
                               "  -Wmaybe-uninitialized             \tWarn about maybe uninitialized automatic"
                               " variables.\n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-maybe-uninitialized"));

maplecl::Option<bool> WmemsetEltSize({"-Wmemset-elt-size"},
                               "  -Wmemset-elt-size             \tWarn about suspicious calls to memset where "
                               "the third argument contains the number of elements not multiplied by the element "
                               "size.\n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-memset-elt-size"));

maplecl::Option<bool> WmemsetTransposedArgs({"-Wmemset-transposed-args"},
                               "  -Wmemset-transposed-args             \tWarn about suspicious calls to memset where"
                               " the third argument is constant literal zero and the second is not.\n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-memset-transposed-args"));

maplecl::Option<bool> WmisleadingIndentation({"-Wmisleading-indentation"},
                               "  -Wmisleading-indentation             \tWarn when the indentation of the code "
                               "does not reflect the block structure.\n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-misleading-indentatio"));

maplecl::Option<bool> WmissingBraces({"-Wmissing-braces"},
                               "  -Wmissing-braces             \tWarn about possibly missing braces around"
                               " initializers.\n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-missing-bracesh"));

maplecl::Option<bool> WmissingDeclarations({"-Wmissing-declarations"},
                               "  -Wmissing-declarations             \tWarn about global functions without previous"
                               " declarations.\n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-missing-declarations"));

maplecl::Option<bool> WmissingFormatAttribute({"-Wmissing-format-attribute"},
                               "  -Wmissing-format-attribute             \t\n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-missing-format-attribute"));

maplecl::Option<bool> WmissingIncludeDirs({"-Wmissing-include-dirs"},
                               "  -Wmissing-include-dirs             \tWarn about user-specified include "
                               "directories that do not exist.\n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-missing-include-dirs"));

maplecl::Option<bool> WmissingParameterType({"-Wmissing-parameter-type"},
                               "  -Wmissing-parameter-type             \tWarn about function parameters declared"
                               " without a type specifier in K&R-style functions.\n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-missing-parameter-type"));

maplecl::Option<bool> WmissingPrototypes({"-Wmissing-prototypes"},
                               "  -Wmissing-prototypes             \tWarn about global functions without prototypes.\n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-missing-prototypes"));

maplecl::Option<bool> Wmultichar({"-Wmultichar"},
                               "  -Wmultichar             \tWarn about use of multi-character character constants.\n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-multichar"));

maplecl::Option<bool> WmultipleInheritance({"-Wmultiple-inheritance"},
                               "  -Wmultiple-inheritance             \tWarn on direct multiple inheritance.\n",
                               {driverCategory, clangCategory});

maplecl::Option<bool> Wnamespaces({"-Wnamespaces"},
                               "  -Wnamespaces             \tWarn on namespace definition.\n",
                               {driverCategory, clangCategory});

maplecl::Option<bool> Wnarrowing({"-Wnarrowing"},
                               "  -Wnarrowing             \tWarn about narrowing conversions within { } "
                               "that are ill-formed in C++11.\n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-narrowing"));

maplecl::Option<bool> WnestedExterns({"-Wnested-externs"},
                               "  -Wnested-externs             \tWarn about \"extern\" declarations not at file"
                               " scope.\n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-nested-externs"));

maplecl::Option<bool> Wnoexcept({"-Wnoexcept"},
                               "  -Wnoexcept             \tWarn when a noexcept expression evaluates to false even"
                               " though the expression can't actually throw.\n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-noexcept"));

maplecl::Option<bool> WnoexceptType({"-Wnoexcept-type"},
                               "  -Wnoexcept-type             \tWarn if C++1z noexcept function type will change "
                               "the mangled name of a symbol.\n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-noexcept-type"));

maplecl::Option<bool> WnonTemplateFriend({"-Wnon-template-friend"},
                               "  -Wnon-template-friend             \tWarn when non-templatized friend functions"
                               " are declared within a template.\n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-non-template-friend"));

maplecl::Option<bool> WnonVirtualDtor({"-Wnon-virtual-dtor"},
                               "  -Wnon-virtual-dtor             \tWarn about non-virtual destructors.\n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-non-virtual-dtor"));

maplecl::Option<bool> Wnonnull({"-Wnonnull"},
                               "  -Wnonnull             \tWarn about NULL being passed to argument slots marked as"
                               " requiring non-NULL.\n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-nonnull"));

maplecl::Option<bool> WnonnullCompare({"-Wnonnull-compare"},
                               "  -Wnonnull-compare             \tWarn if comparing pointer parameter with nonnull"
                               " attribute with NULL.\n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-nonnull-compare"));

maplecl::Option<bool> Wnormalized({"-Wnormalized"},
                               "  -Wnormalized             \tWarn about non-normalized Unicode strings.\n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-normalized"));

maplecl::Option<std::string> WnormalizedE({"-Wnormalized="},
                               "  -Wnormalized=             \t-Wnormalized=[none|id|nfc|nfkc]   Warn about "
                               "non-normalized Unicode strings.\n",
                               {driverCategory, clangCategory});

maplecl::Option<bool> WnullDereference({"-Wnull-dereference"},
                               "  -Wnull-dereference             \tWarn if dereferencing a NULL pointer may lead "
                               "to erroneous or undefined behavior.\n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-null-dereference"));

maplecl::Option<bool> Wodr({"-Wodr"},
                               "  -Wodr             \tWarn about some C++ One Definition Rule violations during "
                               "link time optimization.\n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-odr"));

maplecl::Option<bool> WoldStyleCast({"-Wold-style-cast"},
                               "  -Wold-style-cast             \tWarn if a C-style cast is used in a program.\n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-old-style-cast"));

maplecl::Option<bool> WoldStyleDeclaration({"-Wold-style-declaration"},
                               "  -Wold-style-declaration             \tWarn for obsolescent usage in a declaration.\n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-old-style-declaration"));

maplecl::Option<bool> WoldStyleDefinition({"-Wold-style-definition"},
                               "  -Wold-style-definition             \tWarn if an old-style parameter definition"
                               " is used.\n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-old-style-definition"));

maplecl::Option<bool> WopenmSimd({"-Wopenm-simd"},
                               "  -Wopenm-simd             \tWarn if the vectorizer cost model overrides the OpenMP"
                               " or the Cilk Plus simd directive set by user. \n",
                               {driverCategory, clangCategory});

maplecl::Option<bool> Woverflow({"-Woverflow"},
                               "  -Woverflow             \tWarn about overflow in arithmetic expressions.\n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-overflow"));

maplecl::Option<bool> WoverlengthStrings({"-Woverlength-strings"},
                               "  -Woverlength-strings             \tWarn if a string is longer than the maximum "
                               "portable length specified by the standard.\n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-overlength-strings"));

maplecl::Option<bool> WoverloadedVirtual({"-Woverloaded-virtual"},
                               "  -Woverloaded-virtual             \tWarn about overloaded virtual function names.\n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-overloaded-virtual"));

maplecl::Option<bool> WoverrideInit({"-Woverride-init"},
                               "  -Woverride-init             \tWarn about overriding initializers without side"
                               " effects.\n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-override-init"));

maplecl::Option<bool> WoverrideInitSideEffects({"-Woverride-init-side-effects"},
                               "  -Woverride-init-side-effects             \tWarn about overriding initializers with"
                               " side effects.\n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-override-init-side-effects"));

maplecl::Option<bool> Wpacked({"-Wpacked"},
                               "  -Wpacked             \tWarn when the packed attribute has no effect on struct "
                               "layout.\n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-packed"));

maplecl::Option<bool> WpackedBitfieldCompat({"-Wpacked-bitfield-compat"},
                               "  -Wpacked-bitfield-compat             \tWarn about packed bit-fields whose offset"
                               " changed in GCC 4.4.\n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-packed-bitfield-compat"));

maplecl::Option<bool> Wpadded({"-Wpadded"},
                               "  -Wpadded             \tWarn when padding is required to align structure members.\n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-padded"));

maplecl::Option<bool> Wparentheses({"-Wparentheses"},
                               "  -Wparentheses             \tWarn about possibly missing parentheses.\n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-parentheses"));

maplecl::Option<bool> Wpedantic({"-Wpedantic"},
                               "  -Wpedantic             \tIssue warnings needed for strict compliance to the "
                               "standard.\n",
                               {driverCategory, clangCategory});

maplecl::Option<bool> WpedanticMsFormat({"-Wpedantic-ms-format"},
                               "  -Wpedantic-ms-format             \t\n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-pedantic-ms-format"));

maplecl::Option<bool> WplacementNew({"-Wplacement-new"},
                               "  -Wplacement-new             \tWarn for placement new expressions with undefined "
                               "behavior.\n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-placement-new"));

maplecl::Option<std::string> WplacementNewE({"-Wplacement-new="},
                               "  -Wplacement-new=             \tWarn for placement new expressions with undefined "
                               "behavior.\n",
                               {driverCategory, clangCategory});

maplecl::Option<bool> WpmfConversions({"-Wpmf-conversions"},
                               "  -Wpmf-conversions             \tWarn when converting the type of pointers to "
                               "member functions.\n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-pmf-conversions"));

maplecl::Option<bool> WpointerCompare({"-Wpointer-compare"},
                               "  -Wpointer-compare             \tWarn when a pointer is compared with a zero "
                               "character constant.\n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-pointer-compare"));

maplecl::Option<bool> WpointerSign({"-Wpointer-sign"},
                               "  -Wpointer-sign             \tWarn when a pointer differs in signedness in an "
                               "assignment.\n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-pointer-sign"));

maplecl::Option<bool> WpointerToIntCast({"-Wpointer-to-int-cast"},
                               "  -Wpointer-to-int-cast             \tWarn when a pointer is cast to an integer of "
                               "a different size.\n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-pointer-to-int-cast"));

maplecl::Option<bool> Wpragmas({"-Wpragmas"},
                               "  -Wpragmas             \tWarn about misuses of pragmas.\n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-pragmas"));

maplecl::Option<bool> Wprotocol({"-Wprotocol"},
                               "  -Wprotocol             \tWarn if inherited methods are unimplemented.\n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-protocol"));

maplecl::Option<bool> WredundantDecls({"-Wredundant-decls"},
                               "  -Wredundant-decls             \tWarn about multiple declarations of the same"
                               " object.\n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-redundant-decls"));

maplecl::Option<bool> Wregister({"-Wregister"},
                               "  -Wregister             \tWarn about uses of register storage specifier.\n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-register"));

maplecl::Option<bool> Wreorder({"-Wreorder"},
                               "  -Wreorder             \tWarn when the compiler reorders code.\n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-reorder"));

maplecl::Option<bool> Wrestrict({"-Wrestrict"},
                               "  -Wrestrict             \tWarn when an argument passed to a restrict-qualified "
                               "parameter aliases with another argument.\n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-restrict"));

maplecl::Option<bool> WreturnLocalAddr({"-Wreturn-local-addr"},
                               "  -Wreturn-local-addr             \tWarn about returning a pointer/reference to a"
                               " local or temporary variable.\n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-return-local-addr"));

maplecl::Option<bool> WreturnType({"-Wreturn-type"},
                               "  -Wreturn-type             \tWarn whenever a function's return type defaults to"
                               " \"int\" (C)\n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-return-type"));

maplecl::Option<bool> Wselector({"-Wselector"},
                               "  -Wselector             \tWarn if a selector has multiple methods.\n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-selector"));

maplecl::Option<bool> WsequencePoint({"-Wsequence-point"},
                               "  -Wsequence-point             \tWarn about possible violations of sequence point "
                               "rules.\n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-sequence-point"));

maplecl::Option<bool> WshadowIvar({"-Wshadow-ivar"},
                               "  -Wshadow-ivar             \tWarn if a local declaration hides an instance "
                               "variable.\n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-shadow-ivar"));

maplecl::Option<bool> WshiftCountNegative({"-Wshift-count-negative"},
                               "  -Wshift-count-negative             \tWarn if shift count is negative.\n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-shift-count-negative"));

maplecl::Option<bool> WshiftCountOverflow({"-Wshift-count-overflow"},
                               "  -Wshift-count-overflow             \tWarn if shift count >= width of type.\n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-shift-count-overflow"));

maplecl::Option<bool> WsignConversion({"-Wsign-conversion"},
                               "  -Wsign-conversion             \tWarn for implicit type conversions between signed "
                               "and unsigned integers.\n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-sign-conversion"));

maplecl::Option<bool> WsignPromo({"-Wsign-promo"},
                               "  -Wsign-promo             \tWarn when overload promotes from unsigned to signed.\n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-sign-promo"));

maplecl::Option<bool> WsizedDeallocation({"-Wsized-deallocation"},
                               "  -Wsized-deallocation             \tWarn about missing sized deallocation "
                               "functions.\n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-sized-deallocation"));

maplecl::Option<bool> WsizeofArrayArgument({"-Wsizeof-array-argument"},
                               "  -Wsizeof-array-argument             \tWarn when sizeof is applied on a parameter "
                               "declared as an array.\n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-sizeof-array-argument"));

maplecl::Option<bool> WsizeofPointerMemaccess({"-Wsizeof-pointer-memaccess"},
                               "  -Wsizeof-pointer-memaccess             \tWarn about suspicious length parameters to"
                               " certain string functions if the argument uses sizeof.\n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-sizeof-pointer-memaccess"));

maplecl::Option<bool> WstackProtector({"-Wstack-protector"},
                               "  -Wstack-protector             \tWarn when not issuing stack smashing protection "
                               "for some reason.\n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-stack-protector"));

maplecl::Option<std::string> WstackUsage({"-Wstack-usage"},
                               "  -Wstack-usage             \tWarn if stack usage might be larger than "
                               "specified amount.\n",
                               {driverCategory, clangCategory});

maplecl::Option<bool> WstrictAliasing({"-Wstrict-aliasing"},
                               "  -Wstrict-aliasing             \tWarn about code which might break strict aliasing "
                               "rules.\n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-strict-aliasing"));

maplecl::Option<std::string> WstrictAliasingE({"-Wstrict-aliasing="},
                               "  -Wstrict-aliasing=             \tWarn about code which might break strict aliasing "
                               "rules.\n",
                               {driverCategory, clangCategory});

maplecl::Option<bool> WstrictNullSentinel({"-Wstrict-null-sentinel"},
                               "  -Wstrict-null-sentinel             \tWarn about uncasted NULL used as sentinel.\n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-strict-null-sentinel"));

maplecl::Option<bool> WstrictOverflow({"-Wstrict-overflow"},
                               "  -Wstrict-overflow             \tWarn about optimizations that assume that signed "
                               "overflow is undefined.\n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-strict-overflow"));

maplecl::Option<bool> WstrictSelectorMatch({"-Wstrict-selector-match"},
                               "  -Wstrict-selector-match             \tWarn if type signatures of candidate methods "
                               "do not match exactly.\n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-strict-selector-match"));

maplecl::Option<bool> WstringopOverflow({"-Wstringop-overflow"},
                               "  -Wstringop-overflow             \tWarn about buffer overflow in string manipulation"
                               " functions like memcpy and strcpy.\n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-stringop-overflow"));

maplecl::Option<bool> WsubobjectLinkage({"-Wsubobject-linkage"},
                               "  -Wsubobject-linkage             \tWarn if a class type has a base or a field whose"
                               " type uses the anonymous namespace or depends on a type with no linkage.\n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-subobject-linkage"));

maplecl::Option<bool> WsuggestAttributeConst({"-Wsuggest-attribute=const"},
                               "  -Wsuggest-attribute=const             \tWarn about functions which might be "
                               "candidates for __attribute__((const)).\n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-suggest-attribute=const"));

maplecl::Option<bool> WsuggestAttributeFormat({"-Wsuggest-attribute=format"},
                               "  -Wsuggest-attribute=format             \tWarn about functions which might be "
                               "candidates for format attributes.\n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-suggest-attribute=format"));

maplecl::Option<bool> WsuggestAttributeNoreturn({"-Wsuggest-attribute=noreturn"},
                               "  -Wsuggest-attribute=noreturn             \tWarn about functions which might be"
                               " candidates for __attribute__((noreturn)).\n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-suggest-attribute=noreturn"));

maplecl::Option<bool> WsuggestAttributePure({"-Wsuggest-attribute=pure"},
                               "  -Wsuggest-attribute=pure             \tWarn about functions which might be "
                               "candidates for __attribute__((pure)).\n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-suggest-attribute=pure"));

maplecl::Option<bool> WsuggestFinalMethods({"-Wsuggest-final-methods"},
                               "  -Wsuggest-final-methods             \tWarn about C++ virtual methods where adding"
                               " final keyword would improve code quality.\n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-suggest-final-methods"));

maplecl::Option<bool> WsuggestFinalTypes({"-Wsuggest-final-types"},
                               "  -Wsuggest-final-types             \tWarn about C++ polymorphic types where adding"
                               " final keyword would improve code quality.\n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-suggest-final-types"));

maplecl::Option<bool> Wswitch({"-Wswitch"},
                               "  -Wswitch             \tWarn about enumerated switches\n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-switch"));

maplecl::Option<bool> WswitchBool({"-Wswitch-bool"},
                               "  -Wswitch-bool             \tWarn about switches with boolean controlling "
                               "expression.\n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-switch-bool"));

maplecl::Option<bool> WswitchDefault({"-Wswitch-default"},
                               "  -Wswitch-default             \tWarn about enumerated switches missing a \"default:\""
                               " statement.\n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-switch-default"));

maplecl::Option<bool> WswitchEnum({"-Wswitch-enum"},
                               "  -Wswitch-enum             \tWarn about all enumerated switches missing a specific "
                               "case.\n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-switch-enum"));

maplecl::Option<bool> WswitchUnreachable({"-Wswitch-unreachable"},
                               "  -Wswitch-unreachable             \tWarn about statements between switch's "
                               "controlling expression and the first case.\n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-switch-unreachable"));

maplecl::Option<bool> WsyncNand({"-Wsync-nand"},
                               "  -Wsync-nand             \tWarn when __sync_fetch_and_nand and __sync_nand_and_fetch "
                               "built-in functions are used.\n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-sync-nand"));

maplecl::Option<bool> WsystemHeaders({"-Wsystem-headers"},
                               "  -Wsystem-headers             \tDo not suppress warnings from system headers.\n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-system-headers"));

maplecl::Option<bool> WtautologicalCompare({"-Wtautological-compare"},
                               "  -Wtautological-compare             \tWarn if a comparison always evaluates to true"
                               " or false.\n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-tautological-compare"));

maplecl::Option<bool> Wtemplates({"-Wtemplates"},
                               "  -Wtemplates             \tWarn on primary template declaration.\n",
                               {driverCategory, clangCategory});

maplecl::Option<bool> Wterminate({"-Wterminate"},
                               "  -Wterminate             \tWarn if a throw expression will always result in a call "
                               "to terminate(). \n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-terminate"));

maplecl::Option<bool> Wtraditional({"-Wtraditional"},
                               "  -Wtraditional             \tWarn about features not present in traditional C. \n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-traditional"));

maplecl::Option<bool> WtraditionalConversion({"-Wtraditional-conversion"},
                               "  -Wtraditional-conversion             \tWarn of prototypes causing type conversions "
                               "different from what would happen in the absence of prototype. \n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-traditional-conversion"));

maplecl::Option<bool> Wtrampolines({"-Wtrampolines"},
                               "  -Wtrampolines             \tWarn whenever a trampoline is generated. \n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-trampolines"));

maplecl::Option<bool> Wtrigraphs({"-Wtrigraphs"},
                               "  -Wtrigraphs             \tWarn if trigraphs are encountered that might affect the "
                               "meaning of the program. \n",
                               {driverCategory, clangCategory});

maplecl::Option<bool> WundeclaredSelector({"-Wundeclared-selector"},
                               "  -Wundeclared-selector             \tWarn about @selector()s without previously "
                               "declared methods. \n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-undeclared-selector"));

maplecl::Option<bool> Wuninitialized({"-Wuninitialized"},
                               "  -Wuninitialized             \tWarn about uninitialized automatic variables. \n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-uninitialized"));

maplecl::Option<bool> WunknownPragmas({"-Wunknown-pragmas"},
                               "  -Wunknown-pragmas             \tWarn about unrecognized pragmas. \n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-unknown-pragmas"));

maplecl::Option<bool> WunsafeLoopOptimizations({"-Wunsafe-loop-optimizations"},
                               "  -Wunsafe-loop-optimizations             \tWarn if the loop cannot be optimized due"
                               " to nontrivial assumptions. \n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-unsafe-loop-optimizations"));

maplecl::Option<bool> WunsuffixedFloatConstants({"-Wunsuffixed-float-constants"},
                               "  -Wunsuffixed-float-constants             \tWarn about unsuffixed float constants. \n",
                               {driverCategory, clangCategory});

maplecl::Option<bool> Wunused({"-Wunused"},
                               "  -Wunused             \tEnable all -Wunused- warnings. \n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-unused"));

maplecl::Option<bool> WunusedButSetParameter({"-Wunused-but-set-parameter"},
                               "  -Wunused-but-set-parameter             \tWarn when a function parameter is only set"
                               ", otherwise unused. \n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-unused-but-set-parameter"));

maplecl::Option<bool> WunusedButSetVariable({"-Wunused-but-set-variable"},
                               "  -Wunused-but-set-variable             \tWarn when a variable is only set,"
                               " otherwise unused. \n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-unused-but-set-variable"));

maplecl::Option<bool> WunusedConstVariable({"-Wunused-const-variable"},
                               "  -Wunused-const-variable             \tWarn when a const variable is unused. \n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-unused-const-variable"));

maplecl::Option<bool> WunusedFunction({"-Wunused-function"},
                               "  -Wunused-function             \tWarn when a function is unused. \n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-unused-function"));

maplecl::Option<bool> WunusedLabel({"-Wunused-label"},
                               "  -Wunused-label             \tWarn when a label is unused. \n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-unused-label"));

maplecl::Option<bool> WunusedLocalTypedefs({"-Wunused-local-typedefs"},
                               "  -Wunused-local-typedefs             \tWarn when typedefs locally defined in a"
                               " function are not used. \n",
                               {driverCategory, clangCategory});

maplecl::Option<bool> WunusedResult({"-Wunused-result"},
                               "  -Wunused-result             \tWarn if a caller of a function, marked with attribute "
                               "warn_unused_result, does not use its return value. \n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-unused-result"));

maplecl::Option<bool> WunusedValue({"-Wunused-value"},
                               "  -Wunused-value             \tWarn when an expression value is unused. \n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-unused-value"));

maplecl::Option<bool> WunusedVariable({"-Wunused-variable"},
                               "  -Wunused-variable             \tWarn when a variable is unused. \n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-unused-variable"));

maplecl::Option<bool> WuselessCast({"-Wuseless-cast"},
                               "  -Wuseless-cast             \tWarn about useless casts. \n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-useless-cast"));

maplecl::Option<bool> Wvarargs({"-Wvarargs"},
                               "  -Wvarargs             \tWarn about questionable usage of the macros used to "
                               "retrieve variable arguments. \n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-varargs"));

maplecl::Option<bool> WvariadicMacros({"-Wvariadic-macros"},
                               "  -Wvariadic-macros             \tWarn about using variadic macros. \n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-variadic-macros"));

maplecl::Option<bool> WvectorOperationPerformance({"-Wvector-operation-performance"},
                               "  -Wvector-operation-performance             \tWarn when a vector operation is "
                               "compiled outside the SIMD. \n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-vector-operation-performance"));

maplecl::Option<bool> WvirtualInheritance({"-Wvirtual-inheritance"},
                               "  -Wvirtual-inheritance             \tWarn on direct virtual inheritance.\n",
                               {driverCategory, clangCategory});

maplecl::Option<bool> WvirtualMoveAssign({"-Wvirtual-move-assign"},
                               "  -Wvirtual-move-assign             \tWarn if a virtual base has a non-trivial move "
                               "assignment operator. \n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-virtual-move-assign"));

maplecl::Option<bool> WvolatileRegisterVar({"-Wvolatile-register-var"},
                               "  -Wvolatile-register-var             \tWarn when a register variable is declared "
                               "volatile. \n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-volatile-register-var"));

maplecl::Option<bool> WzeroAsNullPointerConstant({"-Wzero-as-null-pointer-constant"},
                               "  -Wzero-as-null-pointer-constant             \tWarn when a literal '0' is used as "
                               "null pointer. \n",
                               {driverCategory, clangCategory},
                               maplecl::DisableWith("-Wno-zero-as-null-pointer-constant"));

maplecl::Option<bool> WnoScalarStorageOrder({"-Wno-scalar-storage-order"},
                                "  -Wno-scalar-storage-order              \tDo not warn on suspicious constructs "
                                "involving reverse scalar storage order.\n",
                                {driverCategory, unSupCategory},
                                maplecl::DisableWith("-Wscalar-storage-order"));

maplecl::Option<std::string> FmaxErrors({"-fmax-errors"},
                                "  -fmax-errors             \tLimits the maximum number of "
                                "error messages to n, If n is 0 (the default), there is no limit on the number of "
                                "error messages produced. If -Wfatal-errors is also specified, then -Wfatal-errors "
                                "takes precedence over this option.\n",
                                {driverCategory, unSupCategory});

maplecl::Option<bool> StaticLibasan({"-static-libasan"},
                                "  -static-libasan             \tThe -static-libasan option directs the MAPLE driver"
                                " to link libasan statically, without necessarily linking other libraries "
                                "statically.\n",
                                {driverCategory, ldCategory});

maplecl::Option<bool> StaticLiblsan({"-static-liblsan"},
                                "  -static-liblsan             \tThe -static-liblsan option directs the MAPLE driver"
                                " to link liblsan statically, without necessarily linking other libraries "
                                "statically.\n",
                                {driverCategory, ldCategory});

maplecl::Option<bool> StaticLibtsan({"-static-libtsan"},
                                "  -static-libtsan             \tThe -static-libtsan option directs the MAPLE driver"
                                " to link libtsan statically, without necessarily linking other libraries "
                                "statically.\n",
                                {driverCategory, ldCategory});

maplecl::Option<bool> StaticLibubsan({"-static-libubsan"},
                                "  -static-libubsan             \tThe -static-libubsan option directs the MAPLE"
                                " driver to link libubsan statically, without necessarily linking other libraries "
                                "statically.\n",
                                {driverCategory, ldCategory});

maplecl::Option<bool> StaticLibmpx({"-static-libmpx"},
                                "  -static-libmpx             \tThe -static-libmpx option directs the MAPLE driver to "
                                "link libmpx statically, without necessarily linking other libraries statically.\n",
                                {driverCategory, ldCategory});

maplecl::Option<bool> StaticLibmpxwrappers({"-static-libmpxwrappers"},
                                "  -static-libmpxwrappers             \tThe -static-libmpxwrappers option directs the"
                                " MAPLE driver to link libmpxwrappers statically, without necessarily linking other"
                                " libraries statically.\n",
                                {driverCategory, ldCategory});

maplecl::Option<bool> Symbolic({"-symbolic"},
                                "  -symbolic             \tWarn about any unresolved references (unless overridden "
                                "by the link editor option -Xlinker -z -Xlinker defs).\n",
                                {driverCategory, ldCategory});

/* #################################################################################################### */

maplecl::Option<bool> FipaBitCp({"-fipa-bit-cp"},
                               "  -fipa-bit-cp             \tWhen enabled, perform interprocedural bitwise constant"
                               " propagation. This flag is enabled by default at -O2. It requires that -fipa-cp "
                               "is enabled.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> FipaVrp({"-fipa-vrp"},
                               "  -fipa-vrp             \tWhen enabled, perform interprocedural propagation of value"
                               " ranges. This flag is enabled by default at -O2. It requires that -fipa-cp is "
                               "enabled.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MindirectBranchRegister({"-mindirect-branch-register"},
                               "  -mindirect-branch-register            \tForce indirect call and jump via register.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MlowPrecisionDiv({"-mlow-precision-div"},
                               "  -mlow-precision-div             \tEnable the division approximation. "
                               "Enabling this reduces precision of division results to about 16 bits for "
                               "single precision and to 32 bits for double precision.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-mno-low-precision-div"));

maplecl::Option<bool> MlowPrecisionSqrt({"-mlow-precision-sqrt"},
                               "  -mlow-precision-sqrt             \tEnable the reciprocal square root approximation."
                               " Enabling this reduces precision of reciprocal square root results to about 16 bits "
                               "for single precision and to 32 bits for double precision.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-mno-low-precision-sqrt"));

maplecl::Option<bool> M80387({"-m80387"},
                               "  -m80387             \tGenerate output containing 80387 instructions for "
                               "floating point.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-mno-80387"));

maplecl::Option<bool> Allowable_client({"-allowable_client"},
                               "  -allowable_client             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> All_load({"-all_load"},
                               "  -all_load             \tLoads all members of static archive libraries.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Ansi({"-ansi", "--ansi"},
                               "  -ansi             \tIn C mode, this is equivalent to -std=c90. "
                               "In C++ mode, it is equivalent to -std=c++98.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Arch_errors_fatal({"-arch_errors_fatal"},
                               "  -arch_errors_fatal             \tCause the errors having to do with files "
                               "that have the wrong architecture to be fatal.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> AuxInfo({"-aux-info"},
                               "  -aux-info             \tEmit declaration information into <file>.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Bdynamic({"-Bdynamic"},
                               "  -Bdynamic             \tDefined for compatibility with Diab.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Bind_at_load({"-bind_at_load"},
                               "  -bind_at_load             \tCauses the output file to be marked such that the dynamic"
                               " linker will bind all undefined references when the file is loaded or launched.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Bstatic({"-Bstatic"},
                               "  -Bstatic             \tdefined for compatibility with Diab.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Bundle({"-bundle"},
                               "  -bundle             \tProduce a Mach-o bundle format file. \n",
                               {driverCategory, unSupCategory});

maplecl::Option<std::string> Bundle_loader({"-bundle_loader"},
                               "  -bundle_loader             \tThis option specifies the executable that will load "
                               "the build output file being linked.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> C({"-C"},
                               "  -C             \tDo not discard comments. \n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> CC({"-CC"},
                               "  -CC             \tDo not discard comments in macro expansions.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Client_name({"-client_name"},
                               "  -client_name             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Compatibility_version({"-compatibility_version"},
                               "  -compatibility_version             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Coverage({"-coverage"},
                               "  -coverage             \tThe option is a synonym for -fprofile-arcs -ftest-coverage"
                               " (when compiling) and -lgcov (when linking). \n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Current_version({"-current_version"},
                               "  -current_version             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Da({"-da"},
                               "  -da             \tProduce all the dumps listed above.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> DA({"-dA"},
                               "  -dA             \tAnnotate the assembler output with miscellaneous debugging "
                               "information.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> DD({"-dD"},
                               "  -dD             \tDump all macro definitions, at the end of preprocessing, "
                               "in addition to normal output.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Dead_strip({"-dead_strip"},
                               "  -dead_strip             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> DependencyFile({"-dependency-file"},
                               "  -dependency-file             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> DH({"-dH"},
                               "  -dH             \tProduce a core dump whenever an error occurs.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> DI({"-dI"},
                               "  -dI             \tOutput '#include' directives in addition to the result of"
                               " preprocessing.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> DM({"-dM"},
                               "  -dM             \tInstead of the normal output, generate a list of '#define' "
                               "directives for all the macros defined during the execution of the preprocessor, "
                               "including predefined macros. \n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> DN({"-dN"},
                               "  -dN             \t Like -dD, but emit only the macro names, not their expansions.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Dp({"-dp"},
                               "  -dp             \tAnnotate the assembler output with a comment indicating which "
                               "pattern and alternative is used. The length of each instruction is also printed.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> DP({"-dP"},
                               "  -dP             \tDump the RTL in the assembler output as a comment before each"
                               " instruction. Also turns on -dp annotation.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> DU({"-dU"},
                               "  -dU             \tLike -dD except that only macros that are expanded.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Dumpfullversion({"-dumpfullversion"},
                               "  -dumpfullversion             \tPrint the full compiler version, always 3 numbers "
                               "separated by dots, major, minor and patchlevel version.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Dumpmachine({"-dumpmachine"},
                               "  -dumpmachine             \tPrint the compiler's target machine (for example, "
                               "'i686-pc-linux-gnu')—and don't do anything else.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Dumpspecs({"-dumpspecs"},
                               "  -dumpspecs             \tPrint the compiler's built-in specs—and don't do anything "
                               "else. (This is used when MAPLE itself is being built.)\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Dumpversion({"-dumpversion"},
                               "  -dumpversion             \tPrint the compiler version "
                               "(for example, 3.0, 6.3.0 or 7)—and don't do anything else.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Dx({"-dx"},
                               "  -dx             \tJust generate RTL for a function instead of compiling it."
                               " Usually used with -fdump-rtl-expand.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Dylib_file({"-dylib_file"},
                               "  -dylib_file             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Dylinker_install_name({"-dylinker_install_name"},
                               "  -dylinker_install_name             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Dynamic({"-dynamic"},
                               "  -dynamic             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Dynamiclib({"-dynamiclib"},
                               "  -dynamiclib             \tWhen passed this option, GCC produces a dynamic library "
                               "instead of an executable when linking, using the Darwin libtool command.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> EB({"-EB"},
                               "  -EB             \tCompile code for big-endian targets.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> EL({"-EL"},
                               "  -EL             \tCompile code for little-endian targets. \n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Exported_symbols_list({"-exported_symbols_list"},
                               "  -exported_symbols_list             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<std::string> FabiCompatVersion({"-fabi-compat-version="},
                               "  -fabi-compat-version=             \tThe version of the C++ ABI in use.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<std::string> FabiVersion({"-fabi-version="},
                               "  -fabi-version=             \tUse version n of the C++ ABI. The default is "
                               "version 0.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<std::string> FadaSpecParent({"-fada-spec-parent="},
                               "  -fada-spec-parent=             \tIn conjunction with -fdump-ada-spec[-slim] above, "
                               "generate Ada specs as child units of parent unit.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> FaggressiveLoopOptimizations({"-faggressive-loop-optimizations"},
                               "  -faggressive-loop-optimizations             \tThis option tells the loop optimizer "
                               "to use language constraints to derive bounds for the number of iterations of a loop.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-aggressive-loop-optimizations"));

maplecl::Option<bool> FchkpFlexibleStructTrailingArrays({"-fchkp-flexible-struct-trailing-arrays"},
                               "  -fchkp-flexible-struct-trailing-arrays             \tForces Pointer Bounds Checker "
                               "to treat all trailing arrays in structures as possibly flexible. \n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-chkp-flexible-struct-trailing-arrays"));

maplecl::Option<bool> FchkpInstrumentCalls({"-fchkp-instrument-calls"},
                               "  -fchkp-instrument-calls             \tInstructs Pointer Bounds Checker to pass "
                               "pointer bounds to calls. Enabled by default.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-chkp-instrument-calls"));

maplecl::Option<bool> FchkpInstrumentMarkedOnly({"-fchkp-instrument-marked-only"},
                               "  -fchkp-instrument-marked-only             \tInstructs Pointer Bounds Checker to "
                               "instrument only functions marked with the bnd_instrument attribute \n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-chkp-instrument-marked-only"));

maplecl::Option<bool> FchkpNarrowBounds({"-fchkp-narrow-bounds"},
                               "  -fchkp-narrow-bounds             \tControls bounds used by Pointer Bounds Checker"
                               " for pointers to object fields. \n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-chkp-narrow-bounds"));

maplecl::Option<bool> FchkpNarrowToInnermostArray({"-fchkp-narrow-to-innermost-array"},
                               "  -fchkp-narrow-to-innermost-array             \tForces Pointer Bounds Checker to use "
                               "bounds of the innermost arrays in case of nested static array access.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-chkp-narrow-to-innermost-array"));

maplecl::Option<bool> FchkpOptimize({"-fchkp-optimize"},
                               "  -fchkp-optimize             \tEnables Pointer Bounds Checker optimizations.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-chkp-optimize"));

maplecl::Option<bool> FchkpStoreBounds({"-fchkp-store-bounds"},
                               "  -fchkp-store-bounds             \tInstructs Pointer Bounds Checker to generate bounds"
                               " stores for pointer writes.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-chkp-store-bounds"));

maplecl::Option<bool> FchkpTreatZeroDynamicSizeAsInfinite({"-fchkp-treat-zero-dynamic-size-as-infinite"},
                               "  -fchkp-treat-zero-dynamic-size-as-infinite             \tWith this option, objects "
                               "with incomplete type whose dynamically-obtained size is zero are treated as having "
                               "infinite size instead by Pointer Bounds Checker. \n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-chkp-treat-zero-dynamic-size-as-infinite"));

maplecl::Option<bool> FchkpUseFastStringFunctions({"-fchkp-use-fast-string-functions"},
                               "  -fchkp-use-fast-string-functions             \tEnables use of *_nobnd versions of "
                               "string functions (not copying bounds) by Pointer Bounds Checker. \n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-chkp-use-fast-string-functions"));

maplecl::Option<bool> FchkpUseNochkStringFunctions({"-fchkp-use-nochk-string-functions"},
                               "  -fchkp-use-nochk-string-functions             \tEnables use of *_nochk versions of "
                               "string functions (not checking bounds) by Pointer Bounds Checker. \n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-chkp-use-nochk-string-functions"));

maplecl::Option<bool> FchkpUseStaticBounds({"-fchkp-use-static-bounds"},
                               "  -fchkp-use-static-bounds             \tAllow Pointer Bounds Checker to generate "
                               "static bounds holding bounds of static variables. \n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-chkp-use-static-bounds"));

maplecl::Option<bool> FchkpUseStaticConstBounds({"-fchkp-use-static-const-bounds"},
                               "  -fchkp-use-static-const-bounds             \tUse statically-initialized bounds for "
                               "constant bounds instead of generating them each time they are required.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-chkp-use-static-const-bounds"));

maplecl::Option<bool> FchkpUseWrappers({"-fchkp-use-wrappers"},
                               "  -fchkp-use-wrappers             \tAllows Pointer Bounds Checker to replace calls to "
                               "built-in functions with calls to wrapper functions. \n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-chkp-use-wrappers"));

maplecl::Option<bool> Fcilkplus({"-fcilkplus"},
                               "  -fcilkplus             \tEnable Cilk Plus.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-cilkplus"));

maplecl::Option<bool> FcodeHoisting({"-fcode-hoisting"},
                               "  -fcode-hoisting             \tPerform code hoisting. \n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-code-hoisting"));

maplecl::Option<bool> FcombineStackAdjustments({"-fcombine-stack-adjustments"},
                               "  -fcombine-stack-adjustments             \tTracks stack adjustments (pushes and pops)"
                               " and stack memory references and then tries to find ways to combine them.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-combine-stack-adjustments"));

maplecl::Option<bool> FcompareDebug({"-fcompare-debug"},
                               "  -fcompare-debug             \tCompile with and without e.g. -gtoggle, and compare "
                               "the final-insns dump.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-compare-debug"));

maplecl::Option<std::string> FcompareDebugE({"-fcompare-debug="},
                               "  -fcompare-debug=             \tCompile with and without e.g. -gtoggle, and compare "
                               "the final-insns dump.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> FcompareDebugSecond({"-fcompare-debug-second"},
                               "  -fcompare-debug-second             \tRun only the second compilation of "
                               "-fcompare-debug.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> FcompareElim({"-fcompare-elim"},
                               "  -fcompare-elim             \tPerform comparison elimination after register "
                               "allocation has finished.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-compare-elim"));

maplecl::Option<bool> Fconcepts({"-fconcepts"},
                               "  -fconcepts             \tEnable support for C++ concepts.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-concepts"));

maplecl::Option<bool> FcondMismatch({"-fcond-mismatch"},
                               "  -fcond-mismatch             \tAllow the arguments of the '?' operator to have "
                               "different types.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-cond-mismatch"));

maplecl::Option<bool> FconserveStack({"-fconserve-stack"},
                               "  -fconserve-stack             \tDo not perform optimizations increasing noticeably"
                               " stack usage.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-conserve-stack"));

maplecl::Option<std::string> FconstantStringClass({"-fconstant-string-class="},
                               "  -fconstant-string-class=             \tUse class <name> for constant strings."
                                "no class name specified with %qs\n",
                               {driverCategory, unSupCategory});

maplecl::Option<std::string> FconstexprDepth({"-fconstexpr-depth="},
                               "  -fconstexpr-depth=             \tpecify maximum constexpr recursion depth.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<std::string> FconstexprLoopLimit({"-fconstexpr-loop-limit="},
                               "  -fconstexpr-loop-limit=             \tSpecify maximum constexpr loop iteration "
                               "count.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> FcpropRegisters({"-fcprop-registers"},
                               "  -fcprop-registers             \tPerform a register copy-propagation optimization"
                               " pass.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-cprop-registers"));

maplecl::Option<bool> Fcrossjumping({"-fcrossjumping"},
                               "  -fcrossjumping             \tPerform cross-jumping transformation. This "
                               "transformation unifies equivalent code and saves code size. \n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-crossjumping"));

maplecl::Option<bool> FcseFollowJumps({"-fcse-follow-jumps"},
                               "  -fcse-follow-jumps             \tWhen running CSE, follow jumps to their targets.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-cse-follow-jumps"));

maplecl::Option<bool> FcseSkipBlocks({"-fcse-skip-blocks"},
                               "  -fcse-skip-blocks             \tDoes nothing.  Preserved for backward "
                               "compatibility.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-cse-skip-blocks"));

maplecl::Option<bool> FcxFortranRules({"-fcx-fortran-rules"},
                               "  -fcx-fortran-rules             \tComplex multiplication and division follow "
                               "Fortran rules.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-cx-fortran-rules"));

maplecl::Option<bool> FcxLimitedRange({"-fcx-limited-range"},
                               "  -fcx-limited-range             \tOmit range reduction step when performing complex "
                               "division.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-cx-limited-range"));

maplecl::Option<bool> FdbgCnt({"-fdbg-cnt"},
                               "  -fdbg-cnt             \tPlace data items into their own section.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-dbg-cnt"));

maplecl::Option<bool> FdbgCntList({"-fdbg-cnt-list"},
                               "  -fdbg-cnt-list             \tList all available debugging counters with their "
                               "limits and counts.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-dbg-cnt-list"));

maplecl::Option<bool> Fdce({"-fdce"},
                               "  -fdce             \tUse the RTL dead code elimination pass.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-dce"));

maplecl::Option<bool> FdebugCpp({"-fdebug-cpp"},
                               "  -fdebug-cpp             \tEmit debug annotations during preprocessing.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-debug-cpp"));

maplecl::Option<bool> FdebugPrefixMap({"-fdebug-prefix-map"},
                               "  -fdebug-prefix-map             \tMap one directory name to another in debug "
                               "information.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-debug-prefix-map"));

maplecl::Option<bool> FdebugTypesSection({"-fdebug-types-section"},
                               "  -fdebug-types-section             \tOutput .debug_types section when using DWARF "
                               "v4 debuginfo.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-debug-types-section"));

maplecl::Option<bool> FdecloneCtorDtor({"-fdeclone-ctor-dtor"},
                               "  -fdeclone-ctor-dtor             \tFactor complex constructors and destructors to "
                               "favor space over speed.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-declone-ctor-dtor"));

maplecl::Option<bool> FdeduceInitList({"-fdeduce-init-list"},
                               "  -fdeduce-init-list             \tenable deduction of std::initializer_list for a "
                               "template type parameter from a brace-enclosed initializer-list.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-deduce-init-list"));

maplecl::Option<bool> FdelayedBranch({"-fdelayed-branch"},
                               "  -fdelayed-branch             \tAttempt to fill delay slots of branch instructions.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-delayed-branch"));

maplecl::Option<bool> FdeleteDeadExceptions({"-fdelete-dead-exceptions"},
                               "  -fdelete-dead-exceptions             \tDelete dead instructions that may throw "
                               "exceptions.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-delete-dead-exceptions"));

maplecl::Option<bool> FdeleteNullPointerChecks({"-fdelete-null-pointer-checks"},
                               "  -fdelete-null-pointer-checks             \tDelete useless null pointer checks.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-delete-null-pointer-checks"));

maplecl::Option<bool> Fdevirtualize({"-fdevirtualize"},
                               "  -fdevirtualize             \tTry to convert virtual calls to direct ones.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-devirtualize"));

maplecl::Option<bool> FdevirtualizeAtLtrans({"-fdevirtualize-at-ltrans"},
                               "  -fdevirtualize-at-ltrans             \tStream extra data to support more aggressive "
                               "devirtualization in LTO local transformation mode.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-devirtualize-at-ltrans"));

maplecl::Option<bool> FdevirtualizeSpeculatively({"-fdevirtualize-speculatively"},
                               "  -fdevirtualize-speculatively             \tPerform speculative devirtualization.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-devirtualize-speculatively"));

maplecl::Option<bool> FdiagnosticsGeneratePatch({"-fdiagnostics-generate-patch"},
                               "  -fdiagnostics-generate-patch             \tPrint fix-it hints to stderr in unified "
                               "diff format.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-diagnostics-generate-patch"));

maplecl::Option<bool> FdiagnosticsParseableFixits({"-fdiagnostics-parseable-fixits"},
                               "  -fdiagnostics-parseable-fixits             \tPrint fixit hints in machine-readable "
                               "form.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-diagnostics-parseable-fixits"));

maplecl::Option<bool> FdiagnosticsShowCaret({"-fdiagnostics-show-caret"},
                               "  -fdiagnostics-show-caret             \tShow the source line with a caret indicating "
                               "the column.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-diagnostics-show-caret"));

maplecl::Option<std::string> FdiagnosticsShowLocation({"-fdiagnostics-show-location"},
                               "  -fdiagnostics-show-location             \tHow often to emit source location at the "
                               "beginning of line-wrapped diagnostics.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> FdiagnosticsShowOption({"-fdiagnostics-show-option"},
                               "  -fdiagnostics-show-option             \tAmend appropriate diagnostic messages with "
                               "the command line option that controls them.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--fno-diagnostics-show-option"));

maplecl::Option<bool> FdirectivesOnly({"-fdirectives-only"},
                               "  -fdirectives-only             \tPreprocess directives only.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-fdirectives-only"));

maplecl::Option<std::string> Fdisable({"-fdisable-"},
                               "  -fdisable-             \t-fdisable-[tree|rtl|ipa]-<pass>=range1+range2 "
                               "disables an optimization pass.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> FdollarsInIdentifiers({"-fdollars-in-identifiers"},
                               "  -fdollars-in-identifiers             \tPermit '$' as an identifier character.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-dollars-in-identifiers"));

maplecl::Option<bool> Fdse({"-fdse"},
                               "  -fdse             \tUse the RTL dead store elimination pass.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-dse"));

maplecl::Option<bool> FdumpAdaSpec({"-fdump-ada-spec"},
                               "  -fdump-ada-spec             \tWrite all declarations as Ada code transitively.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> FdumpClassHierarchy({"-fdump-class-hierarchy"},
                               "  -fdump-class-hierarchy             \tC++ only)\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-dump-class-hierarchy"));

maplecl::Option<std::string> FdumpFinalInsns({"-fdump-final-insns"},
                               "  -fdump-final-insns             \tDump the final internal representation (RTL) to "
                               "file.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<std::string> FdumpGoSpec({"-fdump-go-spec="},
                               "  -fdump-go-spec             \tWrite all declarations to file as Go code.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> FdumpIpa({"-fdump-ipa"},
                               "  -fdump-ipa             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> FdumpNoaddr({"-fdump-noaddr"},
                               "  -fdump-noaddr             \tSuppress output of addresses in debugging dumps.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-dump-noaddr"));

maplecl::Option<bool> FdumpPasses({"-fdump-passes"},
                               "  -fdump-passes             \tDump optimization passes.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-dump-passes"));

maplecl::Option<bool> FdumpRtlAlignments({"-fdump-rtl-alignments"},
                               "  -fdump-rtl-alignments             \tDump after branch alignments have been "
                               "computed.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> FdumpRtlAll({"-fdump-rtl-all"},
                               "  -fdump-rtl-all             \tProduce all the dumps listed above.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-dump-rtl-all"));

maplecl::Option<bool> FdumpRtlAsmcons({"-fdump-rtl-asmcons"},
                               "  -fdump-rtl-asmcons             \tDump after fixing rtl statements that have "
                               "unsatisfied in/out constraints.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-dump-rtl-asmcons"));

maplecl::Option<bool> FdumpRtlAuto_inc_dec({"-fdump-rtl-auto_inc_dec"},
                               "  -fdump-rtl-auto_inc_dec             \tDump after auto-inc-dec discovery. \n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-dump-rtl-auto_inc_dec"));

maplecl::Option<bool> FdumpRtlBarriers({"-fdump-rtl-barriers"},
                               "  -fdump-rtl-barriers             \tDump after cleaning up the barrier instructions.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-dump-rtl-barriers"));

maplecl::Option<bool> FdumpRtlBbpart({"-fdump-rtl-bbpart"},
                               "  -fdump-rtl-bbpart             \tDump after partitioning hot and cold basic blocks.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-dump-rtl-bbpart"));

maplecl::Option<bool> FdumpRtlBbro({"-fdump-rtl-bbro"},
                               "  -fdump-rtl-bbro             \tDump after block reordering.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-dump-rtl-bbro"));

maplecl::Option<bool> FdumpRtlBtl2({"-fdump-rtl-btl2"},
                               "  -fdump-rtl-btl2             \t-fdump-rtl-btl1 and -fdump-rtl-btl2 enable dumping "
                               "after the two branch target load optimization passes.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-dump-rtl-btl2"));

maplecl::Option<bool> FdumpRtlBypass({"-fdump-rtl-bypass"},
                               "  -fdump-rtl-bypass             \tDump after jump bypassing and control flow "
                               "optimizations.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-dump-rtl-bypass"));

maplecl::Option<bool> FdumpRtlCe1({"-fdump-rtl-ce1"},
                               "  -fdump-rtl-ce1             \tEnable dumping after the three if conversion passes.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-dump-rtl-ce1"));

maplecl::Option<bool> FdumpRtlCe2({"-fdump-rtl-ce2"},
                               "  -fdump-rtl-ce2             \tEnable dumping after the three if conversion passes.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-dump-rtl-ce2"));

maplecl::Option<bool> FdumpRtlCe3({"-fdump-rtl-ce3"},
                               "  -fdump-rtl-ce3             \tEnable dumping after the three if conversion passes.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-dump-rtl-ce3"));

maplecl::Option<bool> FdumpRtlCombine({"-fdump-rtl-combine"},
                               "  -fdump-rtl-combine             \tDump after the RTL instruction combination pass.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-dump-rtl-combine"));

maplecl::Option<bool> FdumpRtlCompgotos({"-fdump-rtl-compgotos"},
                               "  -fdump-rtl-compgotos             \tDump after duplicating the computed gotos.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-dump-rtl-compgotos"));

maplecl::Option<bool> FdumpRtlCprop_hardreg({"-fdump-rtl-cprop_hardreg"},
                               "  -fdump-rtl-cprop_hardreg             \tDump after hard register copy propagation.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-dump-rtl-cprop_hardreg"));

maplecl::Option<bool> FdumpRtlCsa({"-fdump-rtl-csa"},
                               "  -fdump-rtl-csa             \tDump after combining stack adjustments.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-dump-rtl-csa"));

maplecl::Option<bool> FdumpRtlCse1({"-fdump-rtl-cse1"},
                               "  -fdump-rtl-cse1             \tEnable dumping after the two common subexpression "
                               "elimination passes.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-dump-rtl-cse1"));

maplecl::Option<bool> FdumpRtlCse2({"-fdump-rtl-cse2"},
                               "  -fdump-rtl-cse2             \tEnable dumping after the two common subexpression "
                               "elimination passes.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-dump-rtl-cse2"));

maplecl::Option<bool> FdumpRtlDbr({"-fdump-rtl-dbr"},
                               "  -fdump-rtl-dbr             \tDump after delayed branch scheduling.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-dump-rtl-dbr"));

maplecl::Option<bool> FdumpRtlDce({"-fdump-rtl-dce"},
                               "  -fdump-rtl-dce             \tDump after the standalone dead code elimination "
                               "passes.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-fdump-rtl-dce"));

maplecl::Option<bool> FdumpRtlDce1({"-fdump-rtl-dce1"},
                               "  -fdump-rtl-dce1             \tenable dumping after the two dead store elimination "
                               "passes.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-dump-rtl-dce1"));

maplecl::Option<bool> FdumpRtlDce2({"-fdump-rtl-dce2"},
                               "  -fdump-rtl-dce2             \tenable dumping after the two dead store elimination "
                               "passes.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-dump-rtl-dce2"));

maplecl::Option<bool> FdumpRtlDfinish({"-fdump-rtl-dfinish"},
                               "  -fdump-rtl-dfinish             \tThis dump is defined but always produce empty "
                               "files.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-dump-rtl-dfinish"));

maplecl::Option<bool> FdumpRtlDfinit({"-fdump-rtl-dfinit"},
                               "  -fdump-rtl-dfinit             \tThis dump is defined but always produce empty "
                               "files.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-dump-rtl-dfinit"));

maplecl::Option<bool> FdumpRtlEh({"-fdump-rtl-eh"},
                               "  -fdump-rtl-eh             \tDump after finalization of EH handling code.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-dump-rtl-eh"));

maplecl::Option<bool> FdumpRtlEh_ranges({"-fdump-rtl-eh_ranges"},
                               "  -fdump-rtl-eh_ranges             \tDump after conversion of EH handling range "
                               "regions.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-dump-rtl-eh_ranges"));

maplecl::Option<bool> FdumpRtlExpand({"-fdump-rtl-expand"},
                               "  -fdump-rtl-expand             \tDump after RTL generation.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-dump-rtl-expand"));

maplecl::Option<bool> FdumpRtlFwprop1({"-fdump-rtl-fwprop1"},
                               "  -fdump-rtl-fwprop1             \tenable dumping after the two forward propagation "
                               "passes.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-dump-rtl-fwprop1"));

maplecl::Option<bool> FdumpRtlFwprop2({"-fdump-rtl-fwprop2"},
                               "  -fdump-rtl-fwprop2             \tenable dumping after the two forward propagation "
                               "passes.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-dump-rtl-fwprop2"));

maplecl::Option<bool> FdumpRtlGcse1({"-fdump-rtl-gcse1"},
                               "  -fdump-rtl-gcse1             \tenable dumping after global common subexpression "
                               "elimination.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-dump-rtl-gcse1"));

maplecl::Option<bool> FdumpRtlGcse2({"-fdump-rtl-gcse2"},
                               "  -fdump-rtl-gcse2             \tenable dumping after global common subexpression "
                               "elimination.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-dump-rtl-gcse2"));

maplecl::Option<bool> FdumpRtlInitRegs({"-fdump-rtl-init-regs"},
                               "  -fdump-rtl-init-regs             \tDump after the initialization of the registers.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-tedump-rtl-init-regsst"));

maplecl::Option<bool> FdumpRtlInitvals({"-fdump-rtl-initvals"},
                               "  -fdump-rtl-initvals             \tDump after the computation of the initial "
                               "value sets.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-dump-rtl-initvals"));

maplecl::Option<bool> FdumpRtlInto_cfglayout({"-fdump-rtl-into_cfglayout"},
                               "  -fdump-rtl-into_cfglayout             \tDump after converting to cfglayout mode.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-dump-rtl-into_cfglayout"));

maplecl::Option<bool> FdumpRtlIra({"-fdump-rtl-ira"},
                               "  -fdump-rtl-ira             \tDump after iterated register allocation.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-dump-rtl-ira"));

maplecl::Option<bool> FdumpRtlJump({"-fdump-rtl-jump"},
                               "  -fdump-rtl-jump             \tDump after the second jump optimization.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-dump-rtl-jump"));

maplecl::Option<bool> FdumpRtlLoop2({"-fdump-rtl-loop2"},
                               "  -fdump-rtl-loop2             \tenables dumping after the rtl loop optimization "
                               "passes.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-dump-rtl-loop2"));

maplecl::Option<bool> FdumpRtlMach({"-fdump-rtl-mach"},
                               "  -fdump-rtl-mach             \tDump after performing the machine dependent "
                               "reorganization pass, if that pass exists.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-dump-rtl-mach"));

maplecl::Option<bool> FdumpRtlMode_sw({"-fdump-rtl-mode_sw"},
                               "  -fdump-rtl-mode_sw             \tDump after removing redundant mode switches.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-dump-rtl-mode_sw"));

maplecl::Option<bool> FdumpRtlOutof_cfglayout({"-fdump-rtl-outof_cfglayout"},
                               "  -fdump-rtl-outof_cfglayout             \tDump after converting from cfglayout "
                               "mode.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-dump-rtl-outof_cfglayout"));

maplecl::Option<std::string> FdumpRtlPass({"-fdump-rtl-pass"},
                               "  -fdump-rtl-pass             \tSays to make debugging dumps during compilation at "
                               "times specified by letters.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> FdumpRtlPeephole2({"-fdump-rtl-peephole2"},
                               "  -fdump-rtl-peephole2             \tDump after the peephole pass.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-dump-rtl-peephole2"));

maplecl::Option<bool> FdumpRtlPostreload({"-fdump-rtl-postreload"},
                               "  -fdump-rtl-postreload             \tDump after post-reload optimizations.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-dump-rtl-postreload"));

maplecl::Option<bool> FdumpRtlPro_and_epilogue({"-fdump-rtl-pro_and_epilogue"},
                               "  -fdump-rtl-pro_and_epilogue             \tDump after generating the function "
                               "prologues and epilogues.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-dump-rtl-pro_and_epilogue"));

maplecl::Option<bool> FdumpRtlRee({"-fdump-rtl-ree"},
                               "  -fdump-rtl-ree             \tDump after sign/zero extension elimination.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-dump-rtl-ree"));

maplecl::Option<bool> FdumpRtlRegclass({"-fdump-rtl-regclass"},
                               "  -fdump-rtl-regclass             \tThis dump is defined but always produce "
                               "empty files.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-dump-rtl-regclass"));

maplecl::Option<bool> FdumpRtlRnreg({"-fdump-rtl-rnreg"},
                               "  -fdump-rtl-rnreg             \tDump after register renumbering.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-dump-rtl-rnreg"));

maplecl::Option<bool> FdumpRtlSched1({"-fdump-rtl-sched1"},
                               "  -fdump-rtl-sched1             \tnable dumping after the basic block scheduling "
                               "passes.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-dump-rtl-sched1"));

maplecl::Option<bool> FdumpRtlSched2({"-fdump-rtl-sched2"},
                               "  -fdump-rtl-sched2             \tnable dumping after the basic block scheduling "
                               "passes.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-dump-rtl-sched2"));

maplecl::Option<bool> FdumpRtlSeqabstr({"-fdump-rtl-seqabstr"},
                               "  -fdump-rtl-seqabstr             \tDump after common sequence discovery.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-dump-rtl-seqabstr"));

maplecl::Option<bool> FdumpRtlShorten({"-fdump-rtl-shorten"},
                               "  -fdump-rtl-shorten             \tDump after shortening branches.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-dump-rtl-shorten"));

maplecl::Option<bool> FdumpRtlSibling({"-fdump-rtl-sibling"},
                               "  -fdump-rtl-sibling             \tDump after sibling call optimizations.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-dump-rtl-sibling"));

maplecl::Option<bool> FdumpRtlSms({"-fdump-rtl-sms"},
                               "  -fdump-rtl-sms             \tDump after modulo scheduling. This pass is only "
                               "run on some architectures.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-dump-rtl-sms"));

maplecl::Option<bool> FdumpRtlSplit1({"-fdump-rtl-split1"},
                               "  -fdump-rtl-split1             \tThis option enable dumping after five rounds of "
                               "instruction splitting.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-dump-rtl-split1"));

maplecl::Option<bool> FdumpRtlSplit2({"-fdump-rtl-split2"},
                               "  -fdump-rtl-split2             \tThis option enable dumping after five rounds of "
                               "instruction splitting.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-dump-rtl-split2"));

maplecl::Option<bool> FdumpRtlSplit3({"-fdump-rtl-split3"},
                               "  -fdump-rtl-split3             \tThis option enable dumping after five rounds of "
                               "instruction splitting.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-dump-rtl-split3"));

maplecl::Option<bool> FdumpRtlSplit4({"-fdump-rtl-split4"},
                               "  -fdump-rtl-split4             \tThis option enable dumping after five rounds of "
                               "instruction splitting.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-dump-rtl-split4"));

maplecl::Option<bool> FdumpRtlSplit5({"-fdump-rtl-split5"},
                               "  -fdump-rtl-split5             \tThis option enable dumping after five rounds of "
                               "instruction splitting.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-dump-rtl-split5"));

maplecl::Option<bool> FdumpRtlStack({"-fdump-rtl-stack"},
                               "  -fdump-rtl-stack             \tDump after conversion from GCC's "
                               "\"flat register file\" registers to the x87's stack-like registers. "
                               "This pass is only run on x86 variants.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-dump-rtl-stack"));

maplecl::Option<bool> FdumpRtlSubreg1({"-fdump-rtl-subreg1"},
                               "  -fdump-rtl-subreg1             \tenable dumping after the two subreg expansion "
                               "passes.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-dump-rtl-subreg1"));

maplecl::Option<bool> FdumpRtlSubreg2({"-fdump-rtl-subreg2"},
                               "  -fdump-rtl-subreg2             \tenable dumping after the two subreg expansion "
                               "passes.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-dump-rtl-subreg2"));

maplecl::Option<bool> FdumpRtlSubregs_of_mode_finish({"-fdump-rtl-subregs_of_mode_finish"},
                               "  -fdump-rtl-subregs_of_mode_finish             \tThis dump is defined but always "
                               "produce empty files.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-dump-rtl-subregs_of_mode_finish"));

maplecl::Option<bool> FdumpRtlSubregs_of_mode_init({"-fdump-rtl-subregs_of_mode_init"},
                               "  -fdump-rtl-subregs_of_mode_init             \tThis dump is defined but always "
                               "produce empty files.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-dump-rtl-subregs_of_mode_init"));

maplecl::Option<bool> FdumpRtlUnshare({"-fdump-rtl-unshare"},
                               "  -fdump-rtl-unshare             \tDump after all rtl has been unshared.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-dump-rtl-unshare"));

maplecl::Option<bool> FdumpRtlVartrack({"-fdump-rtl-vartrack"},
                               "  -fdump-rtl-vartrack             \tDump after variable tracking.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-dump-rtl-vartrack"));

maplecl::Option<bool> FdumpRtlVregs({"-fdump-rtl-vregs"},
                               "  -fdump-rtl-vregs             \tDump after converting virtual registers to "
                               "hard registers.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-dump-rtl-vregs"));

maplecl::Option<bool> FdumpRtlWeb({"-fdump-rtl-web"},
                               "  -fdump-rtl-web             \tDump after live range splitting.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-dump-rtl-web"));

maplecl::Option<bool> FdumpStatistics({"-fdump-statistics"},
                               "  -fdump-statistics             \tEnable and control dumping of pass statistics "
                               "in a separate file.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-dump-statistics"));

maplecl::Option<bool> FdumpTranslationUnit({"-fdump-translation-unit"},
                               "  -fdump-translation-unit             \tC++ only\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-dump-translation-unit"));

maplecl::Option<bool> FdumpTree({"-fdump-tree"},
                               "  -fdump-tree             \tControl the dumping at various stages of processing the "
                               "intermediate language tree to a file.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-dump-tree"));

maplecl::Option<bool> FdumpTreeAll({"-fdump-tree-all"},
                               "  -fdump-tree-all             \tControl the dumping at various stages of processing "
                               "the intermediate language tree to a file.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-dump-tree-all"));

maplecl::Option<bool> FdumpUnnumbered({"-fdump-unnumbered"},
                               "  -fdump-unnumbered             \tWhen doing debugging dumps, suppress instruction "
                               "numbers and address output. \n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-dump-unnumbered"));

maplecl::Option<bool> FdumpUnnumberedLinks({"-fdump-unnumbered-links"},
                               "  -fdump-unnumbered-links             \tWhen doing debugging dumps (see -d option "
                               "above), suppress instruction numbers for the links to the previous and next "
                               "instructions in a sequence.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-dump-unnumbered-links"));

maplecl::Option<bool> Fdwarf2CfiAsm({"-fdwarf2-cfi-asm"},
                               "  -fdwarf2-cfi-asm             \tEnable CFI tables via GAS assembler directives.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-dwarf2-cfi-asm"));

maplecl::Option<bool> FearlyInlining({"-fearly-inlining"},
                               "  -fearly-inlining             \tPerform early inlining.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-early-inlining"));

maplecl::Option<bool> FeliminateDwarf2Dups({"-feliminate-dwarf2-dups"},
                               "  -feliminate-dwarf2-dups             \tPerform DWARF duplicate elimination.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-eliminate-dwarf2-dups"));

maplecl::Option<bool> FeliminateUnusedDebugSymbols({"-feliminate-unused-debug-symbols"},
                               "  -feliminate-unused-debug-symbols             \tPerform unused symbol elimination "
                               "in debug info.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-feliminate-unused-debug-symbols"));

maplecl::Option<bool> FeliminateUnusedDebugTypes({"-feliminate-unused-debug-types"},
                               "  -feliminate-unused-debug-types             \tPerform unused type elimination in "
                               "debug info.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-eliminate-unused-debug-types"));

maplecl::Option<bool> FemitClassDebugAlways({"-femit-class-debug-always"},
                               "  -femit-class-debug-always             \tDo not suppress C++ class debug "
                               "information.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-emit-class-debug-always"));

maplecl::Option<bool> FemitStructDebugBaseonly({"-femit-struct-debug-baseonly"},
                               "  -femit-struct-debug-baseonly             \tAggressive reduced debug info for "
                               "structs.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-emit-struct-debug-baseonly"));

maplecl::Option<std::string> FemitStructDebugDetailedE({"-femit-struct-debug-detailed="},
                               "  -femit-struct-debug-detailed             \t-femit-struct-debug-detailed=<spec-list> "
                               "Detailed reduced debug info for structs.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> FemitStructDebugReduced({"-femit-struct-debug-reduced"},
                               "  -femit-struct-debug-reduced             \tConservative reduced debug info for "
                               "structs.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-emit-struct-debug-reduced"));

maplecl::Option<std::string> Fenable({"-fenable-"},
                               "  -fenable-             \t-fenable-[tree|rtl|ipa]-<pass>=range1+range2 enables an "
                               "optimization pass.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Fexceptions({"-fexceptions"},
                               "  -fexceptions             \tEnable exception handling.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-exceptions"));

maplecl::Option<std::string> FexcessPrecision({"-fexcess-precision="},
                               "  -fexcess-precision=             \tSpecify handling of excess floating-point "
                               "precision.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<std::string> FexecCharset({"-fexec-charset="},
                               "  -fexec-charset=             \tConvert all strings and character constants to "
                               "character set <cset>.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> FexpensiveOptimizations({"-fexpensive-optimizations"},
                               "  -fexpensive-optimizations             \tPerform a number of minor, expensive "
                               "optimizations.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-expensive-optimizations"));

maplecl::Option<bool> FextNumericLiterals({"-fext-numeric-literals"},
                               "  -fext-numeric-literals             \tInterpret imaginary, fixed-point, or other "
                               "gnu number suffix as the corresponding number literal rather than a user-defined "
                               "number literal.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-ext-numeric-literals"));

maplecl::Option<bool> FextendedIdentifiers({"-fextended-identifiers"},
                               "  -fextended-identifiers             \tPermit universal character names (\\u and \\U) "
                               "in identifiers.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-extended-identifiers"));

maplecl::Option<bool> FexternTlsInit({"-fextern-tls-init"},
                               "  -fextern-tls-init             \tSupport dynamic initialization of thread-local "
                               "variables in a different translation unit.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-extern-tls-init"));

maplecl::Option<bool> FfastMath({"-ffast-math"},
                               "  -ffast-math             \tThis option causes the preprocessor macro __FAST_MATH__ "
                               "to be defined.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-fast-math"));

maplecl::Option<bool> FfatLtoObjects({"-ffat-lto-objects"},
                               "  -ffat-lto-objects             \tOutput lto objects containing both the intermediate "
                               "language and binary output.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-fat-lto-objects"));

maplecl::Option<bool> FfiniteMathOnly({"-ffinite-math-only"},
                               "  -ffinite-math-only             \tAssume no NaNs or infinities are generated.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-finite-math-only"));

maplecl::Option<bool> FfixAndContinue({"-ffix-and-continue"},
                               "  -ffix-and-continue             \tGenerate code suitable for fast turnaround "
                               "development, such as to allow GDB to dynamically load .o files into already-running "
                               "programs.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-fix-and-continue"));

maplecl::Option<std::string> Ffixed({"-ffixed-"},
                               "  -ffixed-             \t-ffixed-<register>	Mark <register> as being unavailable to "
                               "the compiler.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> FfloatStore({"-ffloat-store"},
                               "  -ffloat-store             \ton't allocate floats and doubles in extended-precision "
                               "registers.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-float-store"));

maplecl::Option<bool> FforScope({"-ffor-scope"},
                               "  -ffor-scope             \tScope of for-init-statement variables is local to the "
                               "loop.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-for-scope"));

maplecl::Option<bool> FforwardPropagate({"-fforward-propagate"},
                               "  -fforward-propagate             \tPerform a forward propagation pass on RTL.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<std::string> FfpContract ({"-ffp-contract="},
                               "  -ffp-contract=             \tPerform floating-point expression contraction.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Ffreestanding({"-ffreestanding"},
                               "  -ffreestanding             \tDo not assume that standard C libraries and \"main\" "
                               "exist.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-freestanding"));

maplecl::Option<bool> FfriendInjection({"-ffriend-injection"},
                               "  -ffriend-injection             \tInject friend functions into enclosing namespace.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-friend-injection"));

maplecl::Option<bool> Fgcse({"-fgcse"},
                               "  -fgcse             \tPerform global common subexpression elimination.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-gcse"));

maplecl::Option<bool> FgcseAfterReload({"-fgcse-after-reload"},
                               "  -fgcse-after-reload             \t-fgcse-after-reload\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-gcse-after-reload"));

maplecl::Option<bool> FgcseLas({"-fgcse-las"},
                               "  -fgcse-las             \tPerform redundant load after store elimination in global "
                               "common subexpression elimination.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-gcse-las"));

maplecl::Option<bool> FgcseLm({"-fgcse-lm"},
                               "  -fgcse-lm             \tPerform enhanced load motion during global common "
                               "subexpression elimination.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-gcse-lm"));

maplecl::Option<bool> FgcseSm({"-fgcse-sm"},
                               "  -fgcse-sm             \tPerform store motion after global common subexpression "
                               "elimination.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-gcse-sm"));

maplecl::Option<bool> Fgimple({"-fgimple"},
                               "  -fgimple             \tEnable parsing GIMPLE.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-gimple"));

maplecl::Option<bool> FgnuRuntime({"-fgnu-runtime"},
                               "  -fgnu-runtime             \tGenerate code for GNU runtime environment.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-gnu-runtime"));

maplecl::Option<bool> FgnuTm({"-fgnu-tm"},
                               "  -fgnu-tm             \tEnable support for GNU transactional memory.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-gnu-tm"));

maplecl::Option<bool> Fgnu89Inline({"-fgnu89-inline"},
                               "  -fgnu89-inline             \tUse traditional GNU semantics for inline functions.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-gnu89-inline"));

maplecl::Option<bool> FgraphiteIdentity({"-fgraphite-identity"},
                               "  -fgraphite-identity             \tEnable Graphite Identity transformation.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-graphite-identity"));

maplecl::Option<bool> FhoistAdjacentLoads({"-fhoist-adjacent-loads"},
                               "  -fhoist-adjacent-loads             \tEnable hoisting adjacent loads to encourage "
                               "generating conditional move instructions.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-hoist-adjacent-loads"));

maplecl::Option<bool> Fhosted({"-fhosted"},
                               "  -fhosted             \tAssume normal C execution environment.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-hosted"));

maplecl::Option<bool> FifConversion({"-fif-conversion"},
                               "  -fif-conversion             \tPerform conversion of conditional jumps to "
                               "branchless equivalents.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-if-conversion"));

maplecl::Option<bool> FifConversion2({"-fif-conversion2"},
                               "  -fif-conversion2             \tPerform conversion of conditional "
                               "jumps to conditional execution.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-if-conversion2"));

maplecl::Option<bool> Filelist({"-filelist"},
                               "  -filelist             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> FindirectData({"-findirect-data"},
                               "  -findirect-data             \tGenerate code suitable for fast turnaround "
                               "development, such as to allow GDB to dynamically load .o files into "
                               "already-running programs\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-indirect-data"));

maplecl::Option<bool> FindirectInlining({"-findirect-inlining"},
                               "  -findirect-inlining             \tPerform indirect inlining.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-indirect-inlining"));

maplecl::Option<bool> FinhibitSizeDirective({"-finhibit-size-directive"},
                               "  -finhibit-size-directive             \tDo not generate .size directives.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-inhibit-size-directive"));

maplecl::Option<bool> FinlineFunctions({"-finline-functions"},
                               "  -finline-functions             \tIntegrate functions not declared \"inline\" "
                               "into their callers when profitable.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-inline-functions"));

maplecl::Option<bool> FinlineFunctionsCalledOnce({"-finline-functions-called-once"},
                               "  -finline-functions-called-once             \tIntegrate functions only required "
                               "by their single caller.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-inline-functions-called-once"));

maplecl::Option<std::string> FinlineLimit({"-finline-limit-"},
                               "  -finline-limit-             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<std::string> FinlineLimitE({"-finline-limit="},
                               "  -finline-limit=             \tLimit the size of inlined functions to <number>.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<std::string> FinlineMatmulLimitE({"-finline-matmul-limit="},
                               "  -finline-matmul-limit=             \tecify the size of the largest matrix for "
                               "which matmul will be inlined.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> FinlineSmallFunctions({"-finline-small-functions"},
                               "  -finline-small-functions             \tIntegrate functions into their callers"
                               " when code size is known not to grow.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-inline-small-functions"));

maplecl::Option<std::string> FinputCharset({"-finput-charset="},
                               "  -finput-charset=             \tSpecify the default character set for source files.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> FinstrumentFunctions({"-finstrument-functions"},
                               "  -finstrument-functions             \tInstrument function entry and exit with "
                               "profiling calls.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-instrument-functions"));

maplecl::Option<std::string> FinstrumentFunctionsExcludeFileList({"-finstrument-functions-exclude-file-list="},
                               "  -finstrument-functions-exclude-file-list=             \t Do not instrument functions "
                               "listed in files.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<std::string> FinstrumentFunctionsExcludeFunctionList({"-finstrument-functions-exclude-function-list="},
                               "  -finstrument-functions-exclude-function-list=             \tDo not instrument "
                               "listed functions.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> FipaCp({"-fipa-cp"},
                               "  -fipa-cp             \tPerform interprocedural constant propagation.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-ipa-cp"));

maplecl::Option<bool> FipaCpClone({"-fipa-cp-clone"},
                               "  -fipa-cp-clone             \tPerform cloning to make Interprocedural constant "
                               "propagation stronger.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-ipa-cp-clone"));

maplecl::Option<bool> FipaIcf({"-fipa-icf"},
                               "  -fipa-icf             \tPerform Identical Code Folding for functions and "
                               "read-only variables.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-ipa-icf"));

maplecl::Option<bool> FipaProfile({"-fipa-profile"},
                               "  -fipa-profile             \tPerform interprocedural profile propagation.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-ipa-profile"));

maplecl::Option<bool> FipaPta({"-fipa-pta"},
                               "  -fipa-pta             \tPerform interprocedural points-to analysis.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-ipa-pta"));

maplecl::Option<bool> FipaPureConst({"-fipa-pure-const"},
                               "  -fipa-pure-const             \tDiscover pure and const functions.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-ipa-pure-const"));

maplecl::Option<bool> FipaRa({"-fipa-ra"},
                               "  -fipa-ra             \tUse caller save register across calls if possible.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-ipa-ra"));

maplecl::Option<bool> FipaReference({"-fipa-reference"},
                               "  -fipa-reference             \tDiscover readonly and non addressable static "
                               "variables.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-ipa-reference"));

maplecl::Option<bool> FipaSra({"-fipa-sra"},
                               "  -fipa-sra             \tPerform interprocedural reduction of aggregates.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-ipa-sra"));

maplecl::Option<std::string> FiraAlgorithmE({"-fira-algorithm="},
                               "  -fira-algorithm=             \tSet the used IRA algorithm.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> FiraHoistPressure({"-fira-hoist-pressure"},
                               "  -fira-hoist-pressure             \tUse IRA based register pressure calculation in "
                               "RTL hoist optimizations.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-ira-hoist-pressure"));

maplecl::Option<bool> FiraLoopPressure({"-fira-loop-pressure"},
                               "  -fira-loop-pressure             \tUse IRA based register pressure calculation in "
                               "RTL loop optimizations.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-ira-loop-pressure"));

maplecl::Option<std::string> FiraRegion({"-fira-region="},
                               "  -fira-region=             \tSet regions for IRA.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<std::string> FiraVerbose({"-fira-verbose="},
                               "  -fira-verbose=             \t	Control IRA's level of diagnostic messages.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> FisolateErroneousPathsAttribute({"-fisolate-erroneous-paths-attribute"},
                               "  -fisolate-erroneous-paths-attribute             \tDetect paths that trigger "
                               "erroneous or undefined behavior due to a null value being used in a way forbidden "
                               "by a returns_nonnull or nonnull attribute.  Isolate those paths from the main control "
                               "flow and turn the statement with erroneous or undefined behavior into a trap.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-isolate-erroneous-paths-attribute"));

maplecl::Option<bool> FisolateErroneousPathsDereference({"-fisolate-erroneous-paths-dereference"},
                               "  -fisolate-erroneous-paths-dereference             \tDetect paths that trigger "
                              "erroneous or undefined behavior due to dereferencing a null pointer.  Isolate those "
                              "paths from the main control flow and turn the statement with erroneous or undefined "
                              "behavior into a trap.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-isolate-erroneous-paths-dereference"));

maplecl::Option<std::string> FivarVisibility({"-fivar-visibility="},
                               "  -fivar-visibility=             \tSet the default symbol visibility.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Fivopts({"-fivopts"},
                               "  -fivopts             \tOptimize induction variables on trees.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-ivopts"));

maplecl::Option<bool> FkeepInlineFunctions({"-fkeep-inline-functions"},
                               "  -fkeep-inline-functions             \tGenerate code for functions even if they "
                               "are fully inlined.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-keep-inline-functions"));

maplecl::Option<bool> FkeepStaticConsts({"-fkeep-static-consts"},
                               "  -fkeep-static-consts             \tEmit static const variables even if they are"
                               " not used.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-keep-static-consts"));

maplecl::Option<bool> FkeepStaticFunctions({"-fkeep-static-functions"},
                               "  -fkeep-static-functions             \tGenerate code for static functions even "
                               "if they are never called.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-keep-static-functions"));

maplecl::Option<bool> Flat_namespace({"-flat_namespace"},
                               "  -flat_namespace             \t\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-lat_namespace"));

maplecl::Option<bool> FlaxVectorConversions({"-flax-vector-conversions"},
                               "  -flax-vector-conversions             \tAllow implicit conversions between vectors "
                               "with differing numbers of subparts and/or differing element types.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-lax-vector-conversions"));

maplecl::Option<bool> FleadingUnderscore({"-fleading-underscore"},
                               "  -fleading-underscore             \tGive external symbols a leading underscore.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-leading-underscore"));

maplecl::Option<std::string> FliveRangeShrinkage({"-flive-range-shrinkage"},
                               "  -flive-range-shrinkage             \tRelief of register pressure through live range "
                               "shrinkage\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-live-range-shrinkage"));

maplecl::Option<bool> FlocalIvars({"-flocal-ivars"},
                               "  -flocal-ivars             \tAllow access to instance variables as if they were local"
                               " declarations within instance method implementations.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-local-ivars"));

maplecl::Option<bool> FloopBlock({"-floop-block"},
                               "  -floop-block             \tEnable loop nest transforms.  Same as "
                               "-floop-nest-optimize.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-loop-block"));

maplecl::Option<bool> FloopInterchange({"-floop-interchange"},
                               "  -floop-interchange             \tEnable loop nest transforms.  Same as "
                               "-floop-nest-optimize.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-loop-interchange"));

maplecl::Option<bool> FloopNestOptimize({"-floop-nest-optimize"},
                               "  -floop-nest-optimize             \tEnable the loop nest optimizer.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-loop-nest-optimize"));

maplecl::Option<bool> FloopParallelizeAll({"-floop-parallelize-all"},
                               "  -floop-parallelize-all             \tMark all loops as parallel.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-loop-parallelize-all"));

maplecl::Option<bool> FloopStripMine({"-floop-strip-mine"},
                               "  -floop-strip-mine             \tEnable loop nest transforms. "
                               "Same as -floop-nest-optimize.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-loop-strip-mine"));

maplecl::Option<bool> FloopUnrollAndJam({"-floop-unroll-and-jam"},
                               "  -floop-unroll-and-jam             \tEnable loop nest transforms. "
                               "Same as -floop-nest-optimize.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-loop-unroll-and-jam"));

maplecl::Option<bool> FlraRemat({"-flra-remat"},
                               "  -flra-remat             \tDo CFG-sensitive rematerialization in LRA.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-lra-remat"));

maplecl::Option<std::string> FltoCompressionLevel({"-flto-compression-level="},
                               "  -flto-compression-level=             \tUse zlib compression level <number> for IL.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> FltoOdrTypeMerging({"-flto-odr-type-merging"},
                               "  -flto-odr-type-merging             \tMerge C++ types using One Definition Rule.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-lto-odr-type-merging"));

maplecl::Option<std::string> FltoPartition({"-flto-partition="},
                               "  -flto-partition=             \tSpecify the algorithm to partition symbols and "
                               "vars at linktime.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> FltoReport({"-flto-report"},
                               "  -flto-report             \tReport various link-time optimization statistics.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-lto-report"));

maplecl::Option<bool> FltoReportWpa({"-flto-report-wpa"},
                               "  -flto-report-wpa             \tReport various link-time optimization statistics "
                               "for WPA only.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-lto-report-wpa"));

maplecl::Option<bool> FmemReport({"-fmem-report"},
                               "  -fmem-report             \tReport on permanent memory allocation.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-mem-report"));

maplecl::Option<bool> FmemReportWpa({"-fmem-report-wpa"},
                               "  -fmem-report-wpa             \tReport on permanent memory allocation in WPA only.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-mem-report-wpa"));

maplecl::Option<bool> FmergeAllConstants({"-fmerge-all-constants"},
                               "  -fmerge-all-constants             \tAttempt to merge identical constants and "
                               "constantvariables.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-merge-all-constants"));

maplecl::Option<bool> FmergeConstants({"-fmerge-constants"},
                               "  -fmerge-constants             \tAttempt to merge identical constants across "
                               "compilation units.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-merge-constants"));

maplecl::Option<bool> FmergeDebugStrings({"-fmerge-debug-strings"},
                               "  -fmerge-debug-strings             \tAttempt to merge identical debug strings "
                               "across compilation units.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-merge-debug-strings"));

maplecl::Option<std::string> FmessageLength({"-fmessage-length="},
                               "  -fmessage-length=             \t-fmessage-length=<number>     Limit diagnostics to "
                               "<number> characters per line.  0 suppresses line-wrapping.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> FmoduloSched({"-fmodulo-sched"},
                               "  -fmodulo-sched             \tPerform SMS based modulo scheduling before the first "
                               "scheduling pass.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-modulo-sched"));

maplecl::Option<bool> FmoduloSchedAllowRegmoves({"-fmodulo-sched-allow-regmoves"},
                               "  -fmodulo-sched-allow-regmoves             \tPerform SMS based modulo scheduling with "
                               "register moves allowed.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-modulo-sched-allow-regmoves"));

maplecl::Option<bool> FmoveLoopInvariants({"-fmove-loop-invariants"},
                               "  -fmove-loop-invariants             \tMove loop invariant computations "
                               "out of loops.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-move-loop-invariants"));

maplecl::Option<bool> FmsExtensions({"-fms-extensions"},
                               "  -fms-extensions             \tDon't warn about uses of Microsoft extensions.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-ms-extensions"));

maplecl::Option<bool> FnewInheritingCtors({"-fnew-inheriting-ctors"},
                               "  -fnew-inheriting-ctors             \tImplement C++17 inheriting constructor "
                               "semantics.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-new-inheriting-ctors"));

maplecl::Option<bool> FnewTtpMatching({"-fnew-ttp-matching"},
                               "  -fnew-ttp-matching             \tImplement resolution of DR 150 for matching of "
                               "template template arguments.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-new-ttp-matching"));

maplecl::Option<bool> FnextRuntime({"-fnext-runtime"},
                               "  -fnext-runtime             \tGenerate code for NeXT (Apple Mac OS X) runtime "
                               "environment.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> FnoAccessControl({"-fno-access-control"},
                               "  -fno-access-control             \tTurn off all access checking.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> FnoAsm({"-fno-asm"},
                               "  -fno-asm             \tDo not recognize asm, inline or typeof as a keyword, "
                               "so that code can use these words as identifiers. \n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> FnoBranchCountReg({"-fno-branch-count-reg"},
                               "  -fno-branch-count-reg             \tAvoid running a pass scanning for opportunities"
                               " to use “decrement and branch” instructions on a count register instead of generating "
                               "sequences of instructions that decrement a register, compare it against zero, and then"
                               " branch based upon the result.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> FnoBuiltin({"-fno-builtin", "-fno-builtin-function"},
                               "  -fno-builtin             \tDon't recognize built-in functions that do not begin "
                               "with '__builtin_' as prefix.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> FnoCanonicalSystemHeaders({"-fno-canonical-system-headers"},
                               "  -fno-canonical-system-headers             \tWhen preprocessing, do not shorten "
                               "system header paths with canonicalization.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> FCheckPointerBounds({"-fcheck-pointer-bounds"},
                               "  -fcheck-pointer-bounds             \tEnable Pointer Bounds Checker "
                               "instrumentation.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-check-pointer-bounds"));

maplecl::Option<bool> FChecking({"-fchecking"},
                               "  -fchecking             \tPerform internal consistency checkings.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-checking"));

maplecl::Option<std::string> FCheckingE({"-fchecking="},
                               "  -fchecking=             \tPerform internal consistency checkings.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> FChkpCheckIncompleteType({"-fchkp-check-incomplete-type"},
                               "  -fchkp-check-incomplete-type             \tGenerate pointer bounds checks for "
                               "variables with incomplete type.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-chkp-check-incomplete-type"));

maplecl::Option<bool> FChkpCheckRead({"-fchkp-check-read"},
                               "  -fchkp-check-read             \tGenerate checks for all read accesses to memory.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-chkp-check-read"));

maplecl::Option<bool> FChkpCheckWrite({"-fchkp-check-write"},
                               "  -fchkp-check-write             \tGenerate checks for all write accesses to memory.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-chkp-check-write"));

maplecl::Option<bool> FChkpFirstFieldHasOwnBounds({"-fchkp-first-field-has-own-bounds"},
                               "  -fchkp-first-field-has-own-bounds             \tForces Pointer Bounds Checker to "
                               "use narrowed bounds for address of the first field in the structure.  By default "
                               "pointer to the first field has the same bounds as pointer to the whole structure.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-chkp-first-field-has-own-bounds"));

maplecl::Option<bool> FDefaultInline({"-fdefault-inline"},
                               "  -fdefault-inline             \tDoes nothing.  Preserved for backward "
                               "compatibility.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-default-inline"));

maplecl::Option<bool> FdefaultInteger8({"-fdefault-integer-8"},
                               "  -fdefault-integer-8             \tSet the default integer kind to an 8 byte "
                               "wide type.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-default-integer-8"));

maplecl::Option<bool> FdefaultReal8({"-fdefault-real-8"},
                               "  -fdefault-real-8             \tSet the default real kind to an 8 byte wide type.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-default-real-8"));

maplecl::Option<bool> FDeferPop({"-fdefer-pop"},
                               "  -fdefer-pop             \tDefer popping functions args from stack until later.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-defer-pop"));

maplecl::Option<bool> FElideConstructors({"-felide-constructors"},
                               "  -felide-constructors             \t\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("fno-elide-constructors"));

maplecl::Option<bool> FEnforceEhSpecs({"-fenforce-eh-specs"},
                               "  -fenforce-eh-specs             \tGenerate code to check exception "
                               "specifications.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-enforce-eh-specs"));

maplecl::Option<bool> FFpIntBuiltinInexact({"-ffp-int-builtin-inexact"},
                               "  -ffp-int-builtin-inexact             \tAllow built-in functions ceil, floor, "
                               "round, trunc to raise \"inexact\" exceptions.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-fp-int-builtin-inexact"));

maplecl::Option<bool> FFunctionCse({"-ffunction-cse"},
                               "  -ffunction-cse             \tAllow function addresses to be held in registers.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-function-cse"));

maplecl::Option<bool> FGnuKeywords({"-fgnu-keywords"},
                               "  -fgnu-keywords             \tRecognize GNU-defined keywords.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-gnu-keywords"));

maplecl::Option<bool> FGnuUnique({"-fgnu-unique"},
                               "  -fgnu-unique             \tUse STB_GNU_UNIQUE if supported by the assembler.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-gnu-unique"));

maplecl::Option<bool> FGuessBranchProbability({"-fguess-branch-probability"},
                               "  -fguess-branch-probability             \tEnable guessing of branch probabilities.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-guess-branch-probability"));

maplecl::Option<bool> FIdent({"-fident"},
                               "  -fident             \tProcess #ident directives.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-ident"));

maplecl::Option<bool> FImplementInlines({"-fimplement-inlines"},
                               "  -fimplement-inlines             \tExport functions even if they can be inlined.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-implement-inlines"));

maplecl::Option<bool> FImplicitInlineTemplates({"-fimplicit-inline-templates"},
                               "  -fimplicit-inline-templates             \tEmit implicit instantiations of inline "
                               "templates.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-implicit-inline-templates"));

maplecl::Option<bool> FImplicitTemplates({"-fimplicit-templates"},
                               "  -fimplicit-templates             \tEmit implicit instantiations of templates.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("no-implicit-templates"));

maplecl::Option<bool> FIraShareSaveSlots({"-fira-share-save-slots"},
                               "  -fira-share-save-slots             \tShare slots for saving different hard "
                               "registers.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-ira-share-save-slots"));

maplecl::Option<bool> FIraShareSpillSlots({"-fira-share-spill-slots"},
                               "  -fira-share-spill-slots             \tShare stack slots for spilled "
                               "pseudo-registers.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-ira-share-spill-slots"));

maplecl::Option<bool> FJumpTables({"-fjump-tables"},
                               "  -fjump-tables             \tUse jump tables for sufficiently large "
                               "switch statements.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-jump-tables"));

maplecl::Option<bool> FKeepInlineDllexport({"-fkeep-inline-dllexport"},
                               "  -fkeep-inline-dllexport             \tDon't emit dllexported inline functions "
                               "unless needed.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-keep-inline-dllexport"));

maplecl::Option<bool> FLifetimeDse({"-flifetime-dse"},
                               "  -flifetime-dse             \tTell DSE that the storage for a C++ object is "
                               "dead when the constructor starts and when the destructor finishes.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-lifetime-dse"));

maplecl::Option<bool> FMathErrno({"-fmath-errno"},
                               "  -fmath-errno             \tSet errno after built-in math functions.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-math-errno"));

maplecl::Option<bool> FNilReceivers({"-fnil-receivers"},
                               "  -fnil-receivers             \tAssume that receivers of Objective-C "
                               "messages may be nil.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-nil-receivers"));

maplecl::Option<bool> FNonansiBuiltins({"-fnonansi-builtins"},
                               "  -fnonansi-builtins             \t\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-nonansi-builtins"));

maplecl::Option<bool> FOperatorNames({"-foperator-names"},
                               "  -foperator-names             \tRecognize C++ keywords like \"compl\" and \"xor\".\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-operator-names"));

maplecl::Option<bool> FOptionalDiags({"-foptional-diags"},
                               "  -foptional-diags             \t\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-optional-diags"));

maplecl::Option<bool> FPeephole({"-fpeephole"},
                               "  -fpeephole             \tEnable machine specific peephole optimizations.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-peephole"));

maplecl::Option<bool> FPeephole2({"-fpeephole2"},
                               "  -fpeephole2             \tEnable an RTL peephole pass before sched2.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-peephole2"));

maplecl::Option<bool> FPrettyTemplates({"-fpretty-templates"},
                               "  -fpretty-templates             \tpretty-print template specializations as "
                               "the template signature followed by the arguments.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-pretty-templates"));

maplecl::Option<bool> FPrintfReturnValue({"-fprintf-return-value"},
                               "  -fprintf-return-value             \tTreat known sprintf return values as "
                               "constants.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-printf-return-value"));

maplecl::Option<bool> FRtti({"-frtti"},
                               "  -frtti             \tGenerate run time type descriptor information.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-rtti"));

maplecl::Option<bool> FnoSanitizeAll({"-fno-sanitize=all"},
                               "  -fno-sanitize=all             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> FSchedInterblock({"-fsched-interblock"},
                               "  -fsched-interblock             \tEnable scheduling across basic blocks.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-sched-interblock"));

maplecl::Option<bool> FSchedSpec({"-fsched-spec"},
                               "  -fsched-spec             \tAllow speculative motion of non-loads.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-sched-spec"));

maplecl::Option<bool> FnoSetStackExecutable({"-fno-set-stack-executable"},
                               "  -fno-set-stack-executable             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> FShowColumn({"-fshow-column"},
                               "  -fshow-column             \tShow column numbers in diagnostics, "
                               "when available.  Default on.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-show-column"));

maplecl::Option<bool> FSignedZeros({"-fsigned-zeros"},
                               "  -fsigned-zeros             \tDisable floating point optimizations that "
                               "ignore the IEEE signedness of zero.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-signed-zeros"));

maplecl::Option<bool> FStackLimit({"-fstack-limit"},
                               "  -fstack-limit             \t\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-stack-limit"));

maplecl::Option<bool> FThreadsafeStatics({"-fthreadsafe-statics"},
                               "  -fthreadsafe-statics             \tDo not generate thread-safe code for "
                               "initializing local statics.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-threadsafe-statics"));

maplecl::Option<bool> FToplevelReorder({"-ftoplevel-reorder"},
                               "  -ftoplevel-reorder             \tReorder top level functions, variables, and asms.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-toplevel-reorder"));

maplecl::Option<bool> FTrappingMath({"-ftrapping-math"},
                               "  -ftrapping-math             \tAssume floating-point operations can trap.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-trapping-math"));

maplecl::Option<bool> FUseCxaGetExceptionPtr({"-fuse-cxa-get-exception-ptr"},
                               "  -fuse-cxa-get-exception-ptr             \tUse __cxa_get_exception_ptr in "
                               "exception handling.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-use-cxa-get-exception-ptr"));

maplecl::Option<bool> FWeak({"-fweak"},
                               "  -fweak             \tEmit common-like symbols as weak symbols.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-weak"));

maplecl::Option<bool> FnoWritableRelocatedRdata({"-fno-writable-relocated-rdata"},
                               "  -fno-writable-relocated-rdata             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> FZeroInitializedInBss({"-fzero-initialized-in-bss"},
                               "  -fzero-initialized-in-bss             \tPut zero initialized data in the"
                               " bss section.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-zero-initialized-in-bss"));

maplecl::Option<bool> FnonCallExceptions({"-fnon-call-exceptions"},
                               "  -fnon-call-exceptions             \tSupport synchronous non-call exceptions.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-non-call-exceptions"));

maplecl::Option<bool> FnothrowOpt({"-fnothrow-opt"},
                               "  -fnothrow-opt             \tTreat a throw() exception specification as noexcept to "
                               "improve code size.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-nothrow-opt"));

maplecl::Option<std::string> FobjcAbiVersion({"-fobjc-abi-version="},
                               "  -fobjc-abi-version=             \tSpecify which ABI to use for Objective-C "
                               "family code and meta-data generation.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> FobjcCallCxxCdtors({"-fobjc-call-cxx-cdtors"},
                               "  -fobjc-call-cxx-cdtors             \tGenerate special Objective-C methods to "
                               "initialize/destroy non-POD C++ ivars\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-objc-call-cxx-cdtors"));

maplecl::Option<bool> FobjcDirectDispatch({"-fobjc-direct-dispatch"},
                               "  -fobjc-direct-dispatch             \tAllow fast jumps to the message dispatcher.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-objc-direct-dispatch"));

maplecl::Option<bool> FobjcExceptions({"-fobjc-exceptions"},
                               "  -fobjc-exceptions             \tEnable Objective-C exception and synchronization "
                               "syntax.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-objc-exceptions"));

maplecl::Option<bool> FobjcGc({"-fobjc-gc"},
                               "  -fobjc-gc             \tEnable garbage collection (GC) in Objective-C/Objective-C++ "
                               "programs.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-objc-gc"));

maplecl::Option<bool> FobjcNilcheck({"-fobjc-nilcheck"},
                               "  -fobjc-nilcheck             \tEnable inline checks for nil receivers with the NeXT "
                               "runtime and ABI version 2.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-fobjc-nilcheck"));

maplecl::Option<bool> FobjcSjljExceptions({"-fobjc-sjlj-exceptions"},
                               "  -fobjc-sjlj-exceptions             \tEnable Objective-C setjmp exception "
                               "handling runtime.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-objc-sjlj-exceptions"));

maplecl::Option<bool> FobjcStd({"-fobjc-std=objc1"},
                               "  -fobjc-std             \tConform to the Objective-C 1.0 language as "
                               "implemented in GCC 4.0.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<std::string> FoffloadAbi({"-foffload-abi="},
                               "  -foffload-abi=             \tSet the ABI to use in an offload compiler.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<std::string> Foffload({"-foffload="},
                               "  -foffload=             \tSpecify offloading targets and options for them.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Fopenacc({"-fopenacc"},
                               "  -fopenacc             \tEnable OpenACC.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-openacc"));

maplecl::Option<std::string> FopenaccDim({"-fopenacc-dim="},
                               "  -fopenacc-dim=             \tSpecify default OpenACC compute dimensions.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Fopenmp({"-fopenmp"},
                               "  -fopenmp             \tEnable OpenMP (implies -frecursive in Fortran).\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-openmp"));

maplecl::Option<bool> FopenmpSimd({"-fopenmp-simd"},
                               "  -fopenmp-simd             \tEnable OpenMP's SIMD directives.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-openmp-simd"));

maplecl::Option<bool> FoptInfo({"-fopt-info"},
                               "  -fopt-info             \tEnable all optimization info dumps on stderr.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-opt-info"));

maplecl::Option<bool> FoptimizeStrlen({"-foptimize-strlen"},
                               "  -foptimize-strlen             \tEnable string length optimizations on trees.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-foptimize-strlen"));

maplecl::Option<bool> Force_cpusubtype_ALL({"-force_cpusubtype_ALL"},
                               "  -force_cpusubtype_ALL             \tThis causes GCC's output file to have the "
                               "'ALL' subtype, instead of one controlled by the -mcpu or -march option.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Force_flat_namespace({"-force_flat_namespace"},
                               "  -force_flat_namespace             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> FpackStruct({"-fpack-struct"},
                               "  -fpack-struct             \tPack structure members together without holes.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-pack-struct"));

maplecl::Option<bool> FpartialInlining({"-fpartial-inlining"},
                               "  -fpartial-inlining             \tPerform partial inlining.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-partial-inlining"));

maplecl::Option<bool> FpccStructReturn({"-fpcc-struct-return"},
                               "  -fpcc-struct-return             \tReturn small aggregates in memory\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-pcc-struct-return"));

maplecl::Option<bool> FpchDeps({"-fpch-deps"},
                               "  -fpch-deps             \tWhen using precompiled headers (see Precompiled Headers), "
                               "this flag causes the dependency-output flags to also list the files from the "
                               "precompiled header's dependencies.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-pch-deps"));

maplecl::Option<bool> FpchPreprocess({"-fpch-preprocess"},
                               "  -fpch-preprocess             \tLook for and use PCH files even when preprocessing.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-pch-preprocess"));

maplecl::Option<bool> FpeelLoops({"-fpeel-loops"},
                               "  -fpeel-loops             \tPerform loop peeling.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-peel-loops"));

maplecl::Option<bool> Fpermissive({"-fpermissive"},
                               "  -fpermissive             \tDowngrade conformance errors to warnings.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-permissive"));

maplecl::Option<std::string> FpermittedFltEvalMethods({"-fpermitted-flt-eval-methods="},
                               "  -fpermitted-flt-eval-methods=             \tSpecify which values of FLT_EVAL_METHOD"
                               " are permitted.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Fplan9Extensions({"-fplan9-extensions"},
                               "  -fplan9-extensions             \tEnable Plan 9 language extensions.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-fplan9-extensions"));

maplecl::Option<std::string> Fplugin({"-fplugin="},
                               "  -fplugin=             \tSpecify a plugin to load.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<std::string> FpluginArg({"-fplugin-arg-"},
                               "  -fplugin-arg-             \tSpecify argument <key>=<value> for plugin <name>.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> FpostIpaMemReport({"-fpost-ipa-mem-report"},
                               "  -fpost-ipa-mem-report             \tReport on memory allocation before "
                               "interprocedural optimization.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-post-ipa-mem-report"));

maplecl::Option<bool> FpreIpaMemReport({"-fpre-ipa-mem-report"},
                               "  -fpre-ipa-mem-report             \tReport on memory allocation before "
                               "interprocedural optimization.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-pre-ipa-mem-report"));

maplecl::Option<bool> FpredictiveCommoning({"-fpredictive-commoning"},
                               "  -fpredictive-commoning             \tRun predictive commoning optimization.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-fpredictive-commoning"));

maplecl::Option<bool> FprefetchLoopArrays({"-fprefetch-loop-arrays"},
                               "  -fprefetch-loop-arrays             \tGenerate prefetch instructions\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-prefetch-loop-arrays"));

maplecl::Option<bool> Fpreprocessed({"-fpreprocessed"},
                               "  -fpreprocessed             \tTreat the input file as already preprocessed.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-preprocessed"));

maplecl::Option<bool> FprofileArcs({"-fprofile-arcs"},
                               "  -fprofile-arcs             \tInsert arc-based program profiling code.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-profile-arcs"));

maplecl::Option<bool> FprofileCorrection({"-fprofile-correction"},
                               "  -fprofile-correction             \tEnable correction of flow inconsistent profile "
                               "data input.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-profile-correction"));

maplecl::Option<std::string> FprofileDir({"-fprofile-dir="},
                               "  -fprofile-dir=             \tSet the top-level directory for storing the profile "
                               "data. The default is 'pwd'.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> FprofileGenerate({"-fprofile-generate"},
                               "  -fprofile-generate             \tEnable common options for generating profile "
                               "info for profile feedback directed optimizations.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-profile-generate"));

maplecl::Option<bool> FprofileReorderFunctions({"-fprofile-reorder-functions"},
                               "  -fprofile-reorder-functions             \tEnable function reordering that "
                               "improves code placement.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-profile-reorder-functions"));

maplecl::Option<bool> FprofileReport({"-fprofile-report"},
                               "  -fprofile-report             \tReport on consistency of profile.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-profile-report"));

maplecl::Option<std::string> FprofileUpdate({"-fprofile-update="},
                               "  -fprofile-update=             \tSet the profile update method.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> FprofileUse({"-fprofile-use"},
                               "  -fprofile-use             \tEnable common options for performing profile feedback "
                               "directed optimizations.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-profile-use"));

maplecl::Option<std::string> FprofileUseE({"-fprofile-use="},
                               "  -fprofile-use=             \tEnable common options for performing profile feedback "
                               "directed optimizations, and set -fprofile-dir=.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> FprofileValues({"-fprofile-values"},
                               "  -fprofile-values             \tInsert code to profile values of expressions.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-profile-values"));

maplecl::Option<bool> Fpu({"-fpu"},
                               "  -fpu             \tEnables (-fpu) or disables (-nofpu) the use of RX "
                               "floating-point hardware. \n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-nofpu"));

maplecl::Option<bool> FrandomSeed({"-frandom-seed"},
                               "  -frandom-seed             \tMake compile reproducible using <string>.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-random-seed"));

maplecl::Option<std::string> FrandomSeedE({"-frandom-seed="},
                               "  -frandom-seed=             \tMake compile reproducible using <string>.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> FreciprocalMath({"-freciprocal-math"},
                               "  -freciprocal-math             \tSame as -fassociative-math for expressions which "
                               "include division.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-reciprocal-math"));

maplecl::Option<bool> FrecordGccSwitches({"-frecord-gcc-switches"},
                               "  -frecord-gcc-switches             \tRecord gcc command line switches in the "
                               "object file.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-record-gcc-switches"));

maplecl::Option<bool> Free({"-free"},
                               "  -free             \tTurn on Redundant Extensions Elimination pass.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-ree"));

maplecl::Option<bool> FrenameRegisters({"-frename-registers"},
                               "  -frename-registers             \tPerform a register renaming optimization pass.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-rename-registers"));

maplecl::Option<bool> FreorderBlocks({"-freorder-blocks"},
                               "  -freorder-blocks             \tReorder basic blocks to improve code placement.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-reorder-blocks"));

maplecl::Option<std::string> FreorderBlocksAlgorithm({"-freorder-blocks-algorithm="},
                               "  -freorder-blocks-algorithm=             \tSet the used basic block reordering "
                               "algorithm.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> FreorderBlocksAndPartition({"-freorder-blocks-and-partition"},
                               "  -freorder-blocks-and-partition             \tReorder basic blocks and partition into "
                               "hot and cold sections.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-reorder-blocks-and-partition"));

maplecl::Option<bool> FreorderFunctions({"-freorder-functions"},
                               "  -freorder-functions             \tReorder functions to improve code placement.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-reorder-functions"));

maplecl::Option<bool> FreplaceObjcClasses({"-freplace-objc-classes"},
                               "  -freplace-objc-classes             \tUsed in Fix-and-Continue mode to indicate that"
                               " object files may be swapped in at runtime.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-replace-objc-classes"));

maplecl::Option<bool> Frepo({"-frepo"},
                               "  -frepo             \tEnable automatic template instantiation.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-repo"));

maplecl::Option<bool> FreportBug({"-freport-bug"},
                               "  -freport-bug             \tCollect and dump debug information into temporary file "
                               "if ICE in C/C++ compiler occurred.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-report-bug"));

maplecl::Option<bool> FrerunCseAfterLoop({"-frerun-cse-after-loop"},
                               "  -frerun-cse-after-loop             \tAdd a common subexpression elimination pass "
                               "after loop optimizations.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-rerun-cse-after-loop"));

maplecl::Option<bool> FrescheduleModuloScheduledLoops({"-freschedule-modulo-scheduled-loops"},
                               "  -freschedule-modulo-scheduled-loops             \tEnable/Disable the traditional "
                               "scheduling in loops that already passed modulo scheduling.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-reschedule-modulo-scheduled-loops"));

maplecl::Option<bool> FroundingMath({"-frounding-math"},
                               "  -frounding-math             \tDisable optimizations that assume default FP "
                               "rounding behavior.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-rounding-math"));

maplecl::Option<bool> FsanitizeAddressUseAfterScope({"-fsanitize-address-use-after-scope"},
                               "  -fsanitize-address-use-after-scope             \tEnable sanitization of local "
                               "variables to detect use-after-scope bugs. \n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-sanitize-address-use-after-scope"));

maplecl::Option<bool> FsanitizeCoverageTracePc({"-fsanitize-coverage=trace-pc"},
                               "  -fsanitize-coverage=trace-pc             \tEnable coverage-guided fuzzing code i"
                               "nstrumentation. Inserts call to __sanitizer_cov_trace_pc into every basic block.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-sanitize-coverage=trace-pc"));

maplecl::Option<bool> FsanitizeRecover({"-fsanitize-recover"},
                               "  -fsanitize-recover             \tAfter diagnosing undefined behavior attempt to "
                               "continue execution.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-sanitize-recover"));

maplecl::Option<std::string> FsanitizeRecoverE({"-fsanitize-recover="},
                               "  -fsanitize-recover=             \tAfter diagnosing undefined behavior attempt to "
                               "continue execution.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<std::string> FsanitizeSections({"-fsanitize-sections="},
                               "  -fsanitize-sections=             \tSanitize global variables in user-defined "
                               "sections.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> FsanitizeUndefinedTrapOnError({"-fsanitize-undefined-trap-on-error"},
                               "  -fsanitize-undefined-trap-on-error             \tUse trap instead of a library "
                               "function for undefined behavior sanitization.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-sanitize-undefined-trap-on-error"));

maplecl::Option<std::string> Fsanitize({"-fsanitize"},
                               "  -fsanitize             \tSelect what to sanitize.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-sanitize"));

maplecl::Option<bool> FschedCriticalPathHeuristic({"-fsched-critical-path-heuristic"},
                               "  -fsched-critical-path-heuristic             \tEnable the critical path heuristic "
                               "in the scheduler.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-sched-critical-path-heuristic"));

maplecl::Option<bool> FschedDepCountHeuristic({"-fsched-dep-count-heuristic"},
                               "  -fsched-dep-count-heuristic             \tEnable the dependent count heuristic in "
                               "the scheduler.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-sched-dep-count-heuristic"));

maplecl::Option<bool> FschedGroupHeuristic({"-fsched-group-heuristic"},
                               "  -fsched-group-heuristic             \tEnable the group heuristic in the scheduler.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-sched-group-heuristic"));

maplecl::Option<bool> FschedLastInsnHeuristic({"-fsched-last-insn-heuristic"},
                               "  -fsched-last-insn-heuristic             \tEnable the last instruction heuristic "
                               "in the scheduler.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-sched-last-insn-heuristic"));

maplecl::Option<bool> FschedPressure({"-fsched-pressure"},
                               "  -fsched-pressure             \tEnable register pressure sensitive insn scheduling.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-sched-pressure"));

maplecl::Option<bool> FschedRankHeuristic({"-fsched-rank-heuristic"},
                               "  -fsched-rank-heuristic             \tEnable the rank heuristic in the scheduler.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-sched-rank-heuristic"));

maplecl::Option<bool> FschedSpecInsnHeuristic({"-fsched-spec-insn-heuristic"},
                               "  -fsched-spec-insn-heuristic             \tEnable the speculative instruction "
                               "heuristic in the scheduler.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-sched-spec-insn-heuristic"));

maplecl::Option<bool> FschedSpecLoad({"-fsched-spec-load"},
                               "  -fsched-spec-load             \tAllow speculative motion of some loads.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-sched-spec-load"));

maplecl::Option<bool> FschedSpecLoadDangerous({"-fsched-spec-load-dangerous"},
                               "  -fsched-spec-load-dangerous             \tAllow speculative motion of more loads.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-sched-spec-load-dangerous"));

maplecl::Option<bool> FschedStalledInsns({"-fsched-stalled-insns"},
                               "  -fsched-stalled-insns             \tAllow premature scheduling of queued insns.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-sched-stalled-insns"));

maplecl::Option<bool> FschedStalledInsnsDep({"-fsched-stalled-insns-dep"},
                               "  -fsched-stalled-insns-dep             \tSet dependence distance checking in "
                               "premature scheduling of queued insns.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-sched-stalled-insns-dep"));

maplecl::Option<bool> FschedVerbose({"-fsched-verbose"},
                               "  -fsched-verbose             \tSet the verbosity level of the scheduler.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-sched-verbose"));

maplecl::Option<bool> Fsched2UseSuperblocks({"-fsched2-use-superblocks"},
                               "  -fsched2-use-superblocks             \tIf scheduling post reload\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-sched2-use-superblocks"));

maplecl::Option<bool> FscheduleFusion({"-fschedule-fusion"},
                               "  -fschedule-fusion             \tPerform a target dependent instruction fusion"
                               " optimization pass.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-schedule-fusion"));

maplecl::Option<bool> FscheduleInsns({"-fschedule-insns"},
                               "  -fschedule-insns             \tReschedule instructions before register allocation.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-schedule-insns"));

maplecl::Option<bool> FscheduleInsns2({"-fschedule-insns2"},
                               "  -fschedule-insns2             \tReschedule instructions after register allocation.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-schedule-insns2"));

maplecl::Option<bool> FsectionAnchors({"-fsection-anchors"},
                               "  -fsection-anchors             \tAccess data in the same section from shared "
                               "anchor points.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-fsection-anchors"));

maplecl::Option<bool> FselSchedPipelining({"-fsel-sched-pipelining"},
                               "  -fsel-sched-pipelining             \tPerform software pipelining of inner "
                               "loops during selective scheduling.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-sel-sched-pipelining"));

maplecl::Option<bool> FselSchedPipeliningOuterLoops({"-fsel-sched-pipelining-outer-loops"},
                               "  -fsel-sched-pipelining-outer-loops             \tPerform software pipelining of "
                               "outer loops during selective scheduling.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-sel-sched-pipelining-outer-loops"));

maplecl::Option<bool> FselectiveScheduling({"-fselective-scheduling"},
                               "  -fselective-scheduling             \tSchedule instructions using selective "
                               "scheduling algorithm.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-selective-scheduling"));

maplecl::Option<bool> FselectiveScheduling2({"-fselective-scheduling2"},
                               "  -fselective-scheduling2             \tRun selective scheduling after reload.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-selective-scheduling2"));

maplecl::Option<bool> FshortEnums({"-fshort-enums"},
                               "  -fshort-enums             \tUse the narrowest integer type possible for "
                               "enumeration types.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-short-enums"));

maplecl::Option<bool> FshortWchar({"-fshort-wchar"},
                               "  -fshort-wchar             \tForce the underlying type for \"wchar_t\" to "
                               "be \"unsigned short\".\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-short-wchar"));

maplecl::Option<bool> FshrinkWrap({"-fshrink-wrap"},
                               "  -fshrink-wrap             \tEmit function prologues only before parts of the "
                               "function that need it\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-shrink-wrap"));

maplecl::Option<bool> FshrinkWrapSeparate({"-fshrink-wrap-separate"},
                               "  -fshrink-wrap-separate             \tShrink-wrap parts of the prologue and "
                               "epilogue separately.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-shrink-wrap-separate"));

maplecl::Option<bool> FsignalingNans({"-fsignaling-nans"},
                               "  -fsignaling-nans             \tDisable optimizations observable by IEEE "
                               "signaling NaNs.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-signaling-nans"));

maplecl::Option<bool> FsignedBitfields({"-fsigned-bitfields"},
                               "  -fsigned-bitfields             \tWhen \"signed\" or \"unsigned\" is not given "
                               "make the bitfield signed.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-signed-bitfields"));

maplecl::Option<bool> FsimdCostModel({"-fsimd-cost-model"},
                               "  -fsimd-cost-model             \t\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-simd-cost-model"));

maplecl::Option<bool> FsinglePrecisionConstant({"-fsingle-precision-constant"},
                               "  -fsingle-precision-constant             \tConvert floating point constants to "
                               "single precision constants.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-single-precision-constant"));

maplecl::Option<bool> FsizedDeallocation({"-fsized-deallocation"},
                               "  -fsized-deallocation             \tEnable C++14 sized deallocation support.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-sized-deallocation"));

maplecl::Option<bool> FsplitIvsInUnroller({"-fsplit-ivs-in-unroller"},
                               "  -fsplit-ivs-in-unroller             \tSplit lifetimes of induction variables "
                               "when loops are unrolled.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-split-ivs-in-unroller"));

maplecl::Option<bool> FsplitLoops({"-fsplit-loops"},
                               "  -fsplit-loops             \tPerform loop splitting.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-split-loops"));

maplecl::Option<bool> FsplitPaths({"-fsplit-paths"},
                               "  -fsplit-paths             \tSplit paths leading to loop backedges.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-split-paths"));

maplecl::Option<bool> FsplitStack({"-fsplit-stack"},
                               "  -fsplit-stack             \tGenerate discontiguous stack frames.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-split-stack"));

maplecl::Option<bool> FsplitWideTypes({"-fsplit-wide-types"},
                               "  -fsplit-wide-types             \tSplit wide types into independent registers.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-split-wide-types"));

maplecl::Option<bool> FssaBackprop({"-fssa-backprop"},
                               "  -fssa-backprop             \tEnable backward propagation of use properties at "
                               "the SSA level.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-ssa-backprop"));

maplecl::Option<bool> FssaPhiopt({"-fssa-phiopt"},
                               "  -fssa-phiopt             \tOptimize conditional patterns using SSA PHI nodes.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-ssa-phiopt"));

maplecl::Option<bool> FssoStruct({"-fsso-struct"},
                               "  -fsso-struct             \tSet the default scalar storage order.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-sso-struct"));

maplecl::Option<bool> FstackCheck({"-fstack-check"},
                               "  -fstack-check             \tInsert stack checking code into the program.  Same "
                               "as -fstack-check=specific.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-stack-check"));

maplecl::Option<std::string> FstackCheckE({"-fstack-check="},
                               "  -fstack-check=             \t-fstack-check=[no|generic|specific]	Insert stack "
                               "checking code into the program.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<std::string> FstackLimitRegister({"-fstack-limit-register="},
                               "  -fstack-limit-register=             \tTrap if the stack goes past <register>\n",
                               {driverCategory, unSupCategory});

maplecl::Option<std::string> FstackLimitSymbol({"-fstack-limit-symbol="},
                               "  -fstack-limit-symbol=             \tTrap if the stack goes past symbol <name>.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> FstackProtector({"-fstack-protector"},
                               "  -fstack-protector             \tUse propolice as a stack protection method.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-stack-protector"));

maplecl::Option<bool> FstackProtectorAll({"-fstack-protector-all"},
                               "  -fstack-protector-all             \tUse a stack protection method for "
                               "every function.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-stack-protector-all"));

maplecl::Option<bool> FstackProtectorExplicit({"-fstack-protector-explicit"},
                               "  -fstack-protector-explicit             \tUse stack protection method only for"
                               " functions with the stack_protect attribute.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-stack-protector-explicit"));

maplecl::Option<bool> FstackUsage({"-fstack-usage"},
                               "  -fstack-usage             \tOutput stack usage information on a per-function "
                               "basis.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-stack-usage"));

maplecl::Option<std::string> Fstack_reuse({"-fstack-reuse="},
                               "  -fstack_reuse=             \tSet stack reuse level for local variables.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Fstats({"-fstats"},
                               "  -fstats             \tDisplay statistics accumulated during compilation.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-stats"));

maplecl::Option<bool> FstdargOpt({"-fstdarg-opt"},
                               "  -fstdarg-opt             \tOptimize amount of stdarg registers saved to stack at "
                               "start of function.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-stdarg-opt"));

maplecl::Option<bool> FstoreMerging({"-fstore-merging"},
                               "  -fstore-merging             \tMerge adjacent stores.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-store-merging"));

maplecl::Option<bool> FstrictAliasing({"-fstrict-aliasing"},
                               "  -fstrict-aliasing             \tAssume strict aliasing rules apply.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> FstrictEnums({"-fstrict-enums"},
                               "  -fstrict-enums             \tAssume that values of enumeration type are always "
                               "within the minimum range of that type.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-strict-enums"));

maplecl::Option<bool> FstrictOverflow({"-fstrict-overflow"},
                               "  -fstrict-overflow             \tTreat signed overflow as undefined.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-strict-overflow"));

maplecl::Option<bool> FstrictVolatileBitfields({"-fstrict-volatile-bitfields"},
                               "  -fstrict-volatile-bitfields             \tForce bitfield accesses to match their "
                               "type width.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-strict-volatile-bitfields"));

maplecl::Option<bool> FsyncLibcalls({"-fsync-libcalls"},
                               "  -fsync-libcalls             \tImplement __atomic operations via libcalls to "
                               "legacy __sync functions.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-sync-libcalls"));

maplecl::Option<bool> FsyntaxOnly({"-fsyntax-only"},
                               "  -fsyntax-only             \tCheck for syntax errors\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-syntax-only"));

maplecl::Option<std::string> Ftabstop({"-ftabstop="},
                               "  -ftabstop=             \tSet the distance between tab stops.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<std::string> FtemplateBacktraceLimit({"-ftemplate-backtrace-limit="},
                               "  -ftemplate-backtrace-limit=             \tSet the maximum number of template "
                               "instantiation notes for a single warning or error to n.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<std::string> FtemplateDepth({"-ftemplate-depth-"},
                               "  -ftemplate-depth-             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<std::string> FtemplateDepthE({"-ftemplate-depth="},
                               "  -ftemplate-depth=             \tSpecify maximum template instantiation depth.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> FtestCoverage({"-ftest-coverage"},
                               "  -ftest-coverage             \tCreate data files needed by \"gcov\".\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-test-coverage"));

maplecl::Option<bool> FthreadJumps({"-fthread-jumps"},
                               "  -fthread-jumps             \tPerform jump threading optimizations.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-thread-jumps"));

maplecl::Option<bool> FtimeReport({"-ftime-report"},
                               "  -ftime-report             \tReport the time taken by each compiler pass.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-time-report"));

maplecl::Option<bool> FtimeReportDetails({"-ftime-report-details"},
                               "  -ftime-report-details             \tRecord times taken by sub-phases separately.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-time-report-details"));

maplecl::Option<std::string> FtlsModel({"-ftls-model="},
                               "  -ftls-model=             \tAlter the thread-local storage model to "
                               "be used (see Thread-Local).\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Ftracer({"-ftracer"},
                               "  -ftracer             \tPerform superblock formation via tail duplication.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-tracer"));

maplecl::Option<bool> FtrackMacroExpansion({"-ftrack-macro-expansion"},
                               "  -ftrack-macro-expansion             \tTrack locations of tokens across "
                               "macro expansions.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-track-macro-expansion"));

maplecl::Option<std::string> FtrackMacroExpansionE({"-ftrack-macro-expansion="},
                               "  -ftrack-macro-expansion=             \tTrack locations of tokens across "
                               "macro expansions.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Ftrampolines({"-ftrampolines"},
                               "  -ftrampolines             \tFor targets that normally need trampolines for "
                               "nested functions\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-trampolines"));

maplecl::Option<bool> Ftrapv({"-ftrapv"},
                               "  -ftrapv             \tTrap for signed overflow in addition\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-trapv"));

maplecl::Option<bool> FtreeBitCcp({"-ftree-bit-ccp"},
                               "  -ftree-bit-ccp             \tEnable SSA-BIT-CCP optimization on trees.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-tree-bit-ccp"));

maplecl::Option<bool> FtreeBuiltinCallDce({"-ftree-builtin-call-dce"},
                               "  -ftree-builtin-call-dce             \tEnable conditional dead code elimination for"
                               " builtin calls.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-tree-builtin-call-dce"));

maplecl::Option<bool> FtreeCcp({"-ftree-ccp"},
                               "  -ftree-ccp             \tEnable SSA-CCP optimization on trees.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-tree-ccp"));

maplecl::Option<bool> FtreeCh({"-ftree-ch"},
                               "  -ftree-ch             \tEnable loop header copying on trees.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-tree-ch"));

maplecl::Option<bool> FtreeCoalesceVars({"-ftree-coalesce-vars"},
                               "  -ftree-coalesce-vars             \tEnable SSA coalescing of user variables.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-tree-coalesce-vars"));

maplecl::Option<bool> FtreeCopyProp({"-ftree-copy-prop"},
                               "  -ftree-copy-prop             \tEnable copy propagation on trees.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-tree-copy-prop"));

maplecl::Option<bool> FtreeDce({"-ftree-dce"},
                               "  -ftree-dce             \tEnable SSA dead code elimination optimization on trees.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-tree-dce"));

maplecl::Option<bool> FtreeDominatorOpts({"-ftree-dominator-opts"},
                               "  -ftree-dominator-opts             \tEnable dominator optimizations.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-tree-dominator-opts"));

maplecl::Option<bool> FtreeDse({"-ftree-dse"},
                               "  -ftree-dse             \tEnable dead store elimination.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-tree-dse"));

maplecl::Option<bool> FtreeForwprop({"-ftree-forwprop"},
                               "  -ftree-forwprop             \tEnable forward propagation on trees.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-tree-forwprop"));

maplecl::Option<bool> FtreeFre({"-ftree-fre"},
                               "  -ftree-fre             \tEnable Full Redundancy Elimination (FRE) on trees.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-tree-fre"));

maplecl::Option<bool> FtreeLoopDistributePatterns({"-ftree-loop-distribute-patterns"},
                               "  -ftree-loop-distribute-patterns             \tEnable loop distribution for "
                               "patterns transformed into a library call.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-tree-loop-distribute-patterns"));

maplecl::Option<bool> FtreeLoopDistribution({"-ftree-loop-distribution"},
                               "  -ftree-loop-distribution             \tEnable loop distribution on trees.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-tree-loop-distribution"));

maplecl::Option<bool> FtreeLoopIfConvert({"-ftree-loop-if-convert"},
                               "  -ftree-loop-if-convert             \tConvert conditional jumps in innermost loops "
                               "to branchless equivalents.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-tree-loop-if-convert"));

maplecl::Option<bool> FtreeLoopIm({"-ftree-loop-im"},
                               "  -ftree-loop-im             \tEnable loop invariant motion on trees.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-tree-loop-im"));

maplecl::Option<bool> FtreeLoopIvcanon({"-ftree-loop-ivcanon"},
                               "  -ftree-loop-ivcanon             \tCreate canonical induction variables in loops.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-tree-loop-ivcanon"));

maplecl::Option<bool> FtreeLoopLinear({"-ftree-loop-linear"},
                               "  -ftree-loop-linear             \tEnable loop nest transforms.  Same as "
                               "-floop-nest-optimize.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-tree-loop-linear"));

maplecl::Option<bool> FtreeLoopOptimize({"-ftree-loop-optimize"},
                               "  -ftree-loop-optimize             \tEnable loop optimizations on tree level.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-tree-loop-optimize"));

maplecl::Option<bool> FtreeLoopVectorize({"-ftree-loop-vectorize"},
                               "  -ftree-loop-vectorize             \tEnable loop vectorization on trees.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-tree-loop-vectorize"));

maplecl::Option<bool> FtreeParallelizeLoops({"-ftree-parallelize-loops"},
                               "  -ftree-parallelize-loops             \t\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-tree-parallelize-loops"));

maplecl::Option<bool> FtreePartialPre({"-ftree-partial-pre"},
                               "  -ftree-partial-pre             \tIn SSA-PRE optimization on trees\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-tree-partial-pre"));

maplecl::Option<bool> FtreePhiprop({"-ftree-phiprop"},
                               "  -ftree-phiprop             \tEnable hoisting loads from conditional pointers.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-tree-phiprop"));

maplecl::Option<bool> FtreePre({"-ftree-pre"},
                               "  -ftree-pre             \tEnable SSA-PRE optimization on trees.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-tree-pre"));

maplecl::Option<bool> FtreePta({"-ftree-pta"},
                               "  -ftree-pta             \tPerform function-local points-to analysis on trees.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-tree-pta"));

maplecl::Option<bool> FtreeReassoc({"-ftree-reassoc"},
                               "  -ftree-reassoc             \tEnable reassociation on tree level.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-tree-reassoc"));

maplecl::Option<bool> FtreeSink({"-ftree-sink"},
                               "  -ftree-sink             \tEnable SSA code sinking on trees.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-tree-sink"));

maplecl::Option<bool> FtreeSlpVectorize({"-ftree-slp-vectorize"},
                               "  -ftree-slp-vectorize             \tEnable basic block vectorization (SLP) "
                               "on trees.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-tree-slp-vectorize"));

maplecl::Option<bool> FtreeSlsr({"-ftree-slsr"},
                               "  -ftree-slsr             \tPerform straight-line strength reduction.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-tree-slsr"));

maplecl::Option<bool> FtreeSra({"-ftree-sra"},
                               "  -ftree-sra             \tPerform scalar replacement of aggregates.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-tree-sra"));

maplecl::Option<bool> FtreeSwitchConversion({"-ftree-switch-conversion"},
                               "  -ftree-switch-conversion             \tPerform conversions of switch "
                               "initializations.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-tree-switch-conversion"));

maplecl::Option<bool> FtreeTailMerge({"-ftree-tail-merge"},
                               "  -ftree-tail-merge             \tEnable tail merging on trees.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-tree-tail-merge"));

maplecl::Option<bool> FtreeTer({"-ftree-ter"},
                               "  -ftree-ter             \tReplace temporary expressions in the SSA->normal pass.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-tree-ter"));

maplecl::Option<bool> FtreeVrp({"-ftree-vrp"},
                               "  -ftree-vrp             \tPerform Value Range Propagation on trees.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-tree-vrp"));

maplecl::Option<bool> FunconstrainedCommons({"-funconstrained-commons"},
                               "  -funconstrained-commons             \tAssume common declarations may be "
                               "overridden with ones with a larger trailing array.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-unconstrained-commons"));

maplecl::Option<bool> FunitAtATime({"-funit-at-a-time"},
                               "  -funit-at-a-time             \tCompile whole compilation unit at a time.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-unit-at-a-time"));

maplecl::Option<bool> FunrollAllLoops({"-funroll-all-loops"},
                               "  -funroll-all-loops             \tPerform loop unrolling for all loops.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-unroll-all-loops"));

maplecl::Option<bool> FunrollLoops({"-funroll-loops"},
                               "  -funroll-loops             \tPerform loop unrolling when iteration count is known.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-unroll-loops"));

maplecl::Option<bool> FunsafeMathOptimizations({"-funsafe-math-optimizations"},
                               "  -funsafe-math-optimizations             \tAllow math optimizations that may "
                               "violate IEEE or ISO standards.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-unsafe-math-optimizations"));

maplecl::Option<bool> FunsignedBitfields({"-funsigned-bitfields"},
                               "  -funsigned-bitfields             \tWhen \"signed\" or \"unsigned\" is not given "
                               "make the bitfield unsigned.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-unsigned-bitfields"));

maplecl::Option<bool> FunswitchLoops({"-funswitch-loops"},
                               "  -funswitch-loops             \tPerform loop unswitching.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-unswitch-loops"));

maplecl::Option<bool> FunwindTables({"-funwind-tables"},
                               "  -funwind-tables             \tJust generate unwind tables for exception handling.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-unwind-tables"));

maplecl::Option<bool> FuseCxaAtexit({"-fuse-cxa-atexit"},
                               "  -fuse-cxa-atexit             \tUse __cxa_atexit to register destructors.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-use-cxa-atexit"));

maplecl::Option<bool> FuseLdBfd({"-fuse-ld=bfd"},
                               "  -fuse-ld=bfd             \tUse the bfd linker instead of the default linker.\n",
                               {driverCategory, ldCategory});

maplecl::Option<bool> FuseLdGold({"-fuse-ld=gold"},
                               "  -fuse-ld=gold             \tUse the gold linker instead of the default linker.\n",
                               {driverCategory, ldCategory});

maplecl::Option<bool> FuseLinkerPlugin({"-fuse-linker-plugin"},
                               "  -fuse-linker-plugin             \tEnables the use of a linker plugin during "
                               "link-time optimization.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-use-linker-plugin"));

maplecl::Option<bool> FvarTracking({"-fvar-tracking"},
                               "  -fvar-tracking             \tPerform variable tracking.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-var-tracking"));

maplecl::Option<bool> FvarTrackingAssignments({"-fvar-tracking-assignments"},
                               "  -fvar-tracking-assignments             \tPerform variable tracking by "
                               "annotating assignments.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-var-tracking-assignments"));

maplecl::Option<bool> FvarTrackingAssignmentsToggle({"-fvar-tracking-assignments-toggle"},
                               "  -fvar-tracking-assignments-toggle             \tToggle -fvar-tracking-assignments.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-var-tracking-assignments-toggle"));

maplecl::Option<bool> FvariableExpansionInUnroller({"-fvariable-expansion-in-unroller"},
                               "  -fvariable-expansion-in-unroller             \tApply variable expansion when "
                               "loops are unrolled.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-variable-expansion-in-unroller"));

maplecl::Option<bool> FvectCostModel({"-fvect-cost-model"},
                               "  -fvect-cost-model             \tEnables the dynamic vectorizer cost model. "
                               "Preserved for backward compatibility.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-vect-cost-model"));

maplecl::Option<bool> FverboseAsm({"-fverbose-asm"},
                               "  -fverbose-asm             \tAdd extra commentary to assembler output.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-verbose-asm"));

maplecl::Option<bool> FvisibilityInlinesHidden({"-fvisibility-inlines-hidden"},
                               "  -fvisibility-inlines-hidden             \tMarks all inlined functions and methods"
                               " as having hidden visibility.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-visibility-inlines-hidden"));

maplecl::Option<bool> FvisibilityMsCompat({"-fvisibility-ms-compat"},
                               "  -fvisibility-ms-compat             \tChanges visibility to match Microsoft Visual"
                               " Studio by default.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-visibility-ms-compat"));

maplecl::Option<bool> Fvpt({"-fvpt"},
                               "  -fvpt             \tUse expression value profiles in optimizations.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-vpt"));

maplecl::Option<bool> FvtableVerify({"-fvtable-verify"},
                               "  -fvtable-verify             \t\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-vtable-verify"));

maplecl::Option<bool> FvtvCounts({"-fvtv-counts"},
                               "  -fvtv-counts             \tOutput vtable verification counters.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-vtv-counts"));

maplecl::Option<bool> FvtvDebug({"-fvtv-debug"},
                               "  -fvtv-debug             \tOutput vtable verification pointer sets information.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-vtv-debug"));

maplecl::Option<bool> Fweb({"-fweb"},
                               "  -fweb             \tConstruct webs and split unrelated uses of single variable.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-web"));

maplecl::Option<bool> FwholeProgram({"-fwhole-program"},
                               "  -fwhole-program             \tPerform whole program optimizations.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-whole-program"));

maplecl::Option<std::string> FwideExecCharset({"-fwide-exec-charset="},
                               "  -fwide-exec-charset=             \tConvert all wide strings and character "
                               "constants to character set <cset>.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> FworkingDirectory({"-fworking-directory"},
                               "  -fworking-directory             \tGenerate a #line directive pointing at the current "
                               "working directory.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-working-directory"));

maplecl::Option<bool> Fwrapv({"-fwrapv"},
                               "  -fwrapv             \tAssume signed arithmetic overflow wraps around.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-fwrapv"));

maplecl::Option<bool> FzeroLink({"-fzero-link"},
                               "  -fzero-link             \tGenerate lazy class lookup (via objc_getClass()) for use "
                               "inZero-Link mode.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-fno-zero-link"));

maplecl::Option<std::string> G({"-G"},
                               "  -G             \tOn embedded PowerPC systems, put global and static items less than"
                               " or equal to num bytes into the small data or BSS sections instead of the normal data "
                               "or BSS section. \n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Gcoff({"-gcoff"},
                               "  -gcoff             \tGenerate debug information in COFF format.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> GcolumnInfo({"-gcolumn-info"},
                               "  -gcolumn-info             \tRecord DW_AT_decl_column and DW_AT_call_column "
                               "in DWARF.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Gdwarf({"-gdwarf"},
                               "  -gdwarf             \tGenerate debug information in default version of DWARF "
                               "format.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> GenDecls({"-gen-decls"},
                               "  -gen-decls             \tDump declarations to a .decl file.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Gfull({"-gfull"},
                               "  -gfull             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Ggdb({"-ggdb"},
                               "  -ggdb             \tGenerate debug information in default extended format.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> GgnuPubnames({"-ggnu-pubnames"},
                               "  -ggnu-pubnames             \tGenerate DWARF pubnames and pubtypes sections with "
                               "GNU extensions.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> GnoColumnInfo({"-gno-column-info"},
                               "  -gno-column-info             \tDon't record DW_AT_decl_column and DW_AT_call_column"
                               " in DWARF.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> GnoRecordGccSwitches({"-gno-record-gcc-switches"},
                               "  -gno-record-gcc-switches             \tDon't record gcc command line switches "
                               "in DWARF DW_AT_producer.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> GnoStrictDwarf({"-gno-strict-dwarf"},
                               "  -gno-strict-dwarf             \tEmit DWARF additions beyond selected version.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Gpubnames({"-gpubnames"},
                               "  -gpubnames             \tGenerate DWARF pubnames and pubtypes sections.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> GrecordGccSwitches({"-grecord-gcc-switches"},
                               "  -grecord-gcc-switches             \tRecord gcc command line switches in "
                               "DWARF DW_AT_producer.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> GsplitDwarf({"-gsplit-dwarf"},
                               "  -gsplit-dwarf             \tGenerate debug information in separate .dwo files.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Gstabs({"-gstabs"},
                               "  -gstabs             \tGenerate debug information in STABS format.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> GstabsA({"-gstabs+"},
                               "  -gstabs+             \tGenerate debug information in extended STABS format.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> GstrictDwarf({"-gstrict-dwarf"},
                               "  -gstrict-dwarf             \tDon't emit DWARF additions beyond selected version.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Gtoggle({"-gtoggle"},
                               "  -gtoggle             \tToggle debug information generation.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Gused({"-gused"},
                               "  -gused             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Gvms({"-gvms"},
                               "  -gvms             \tGenerate debug information in VMS format.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Gxcoff({"-gxcoff"},
                               "  -gxcoff             \tGenerate debug information in XCOFF format.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> GxcoffA({"-gxcoff+"},
                               "  -gxcoff+             \tGenerate debug information in extended XCOFF format.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Gz({"-gz"},
                               "  -gz             \tGenerate compressed debug sections.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> H({"-H"},
                               "  -H             \tPrint the name of header files as they are used.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Headerpad_max_install_names({"-headerpad_max_install_names"},
                               "  -headerpad_max_install_names             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> I({"-I-"},
                               "  -I-             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Idirafter({"-idirafter"},
                               "  -idirafter             \t-idirafter <dir>     Add <dir> to the end of the system "
                               "include path.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<std::string> Iframework({"-iframework"},
                               "  -iframework             \tLike -F except the directory is a treated as a system "
                               "directory. \n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Image_base({"-image_base"},
                               "  -image_base             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<std::string> Imultilib({"-imultilib"},
                               "  -imultilib             \t-imultilib <dir>     Set <dir> to be the multilib include"
                               " subdirectory.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<std::string> Include({"-include"},
                               "  -include             \t-include <file>        Include the contents of <file> "
                               "before other files.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Init({"-init"},
                               "  -init             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Install_name({"-install_name"},
                               "  -install_name             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<std::string> Iplugindir({"-iplugindir="},
                               "  -iplugindir=             \t-iplugindir=<dir>  Set <dir> to be the default plugin "
                               "directory.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<std::string> Iprefix({"-iprefix"},
                               "  -iprefix             \t-iprefix <path>        Specify <path> as a prefix for next "
                               "two options.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<std::string> Iquote({"-iquote"},
                               "  -iquote             \t-iquote <dir>   Add <dir> to the end of the quote include "
                               "path.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<std::string> Isysroot({"-isysroot"},
                               "  -isysroot             \t-isysroot <dir>       Set <dir> to be the system root "
                               "directory.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<std::string> Iwithprefix({"-iwithprefix"},
                               "  -iwithprefix             \t-iwithprefix <dir> Add <dir> to the end of the system "
                               "include path.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<std::string> Iwithprefixbefore({"-iwithprefixbefore"},
                               "  -iwithprefixbefore             \t-iwithprefixbefore <dir>     Add <dir> to the end "
                               "ofthe main include path.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Keep_private_externs({"-keep_private_externs"},
                               "  -keep_private_externs             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> M({"-M"},
                               "  -M             \tGenerate make dependencies.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> M1({"-m1"},
                               "  -m1             \tGenerate code for the SH1.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> M10({"-m10"},
                               "  -m10             \tGenerate code for a PDP-11/10.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> M128bitLongDouble({"-m128bit-long-double"},
                               "  -m128bit-long-double             \tControl the size of long double type.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> M16({"-m16"},
                               "  -m16             \tGenerate code for a 16-bit.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> M16Bit({"-m16-bit"},
                               "  -m16-bit             \tArrange for stack frame, writable data and "
                               "constants to all be 16-bit.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-mno-16-bit"));

maplecl::Option<bool> M2({"-m2"},
                               "  -m2             \tGenerate code for the SH2.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> M210({"-m210"},
                               "  -m210             \tGenerate code for the 210 processor.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> M2a({"-m2a"},
                               "  -m2a             \tGenerate code for the SH2a-FPU assuming the floating-point"
                               " unit is in double-precision mode by default.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> M2e({"-m2e"},
                               "  -m2e             \tGenerate code for the SH2e.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> M2aNofpu({"-m2a-nofpu"},
                               "  -m2a-nofpu             \tGenerate code for the SH2a without FPU, or for a"
                               " SH2a-FPU in such a way that the floating-point unit is not used.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> M2aSingle({"-m2a-single"},
                               "  -m2a-single             \tGenerate code for the SH2a-FPU assuming the "
                               "floating-point unit is in single-precision mode by default.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> M2aSingleOnly({"-m2a-single-only"},
                               "  -m2a-single-only             \tGenerate code for the SH2a-FPU, in such a way"
                               " that no double-precision floating-point operations are used.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> M3({"-m3"},
                               "  -m3             \tGenerate code for the SH3.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> M31({"-m31"},
                               "  -m31             \tWhen -m31 is specified, generate code compliant to"
                               " the GNU/Linux for S/390 ABI. \n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> M32({"-m32"},
                               "  -m32             \tGenerate code for 32-bit ABI.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> M32Bit({"-m32-bit"},
                               "  -m32-bit             \tArrange for stack frame, writable data and "
                               "constants to all be 32-bit.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> M32bitDoubles({"-m32bit-doubles"},
                               "  -m32bit-doubles             \tMake the double data type  32 bits "
                               "(-m32bit-doubles) in size.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> M32r({"-m32r"},
                               "  -m32r             \tGenerate code for the M32R.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> M32r2({"-m32r2"},
                               "  -m32r2             \tGenerate code for the M32R/2.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> M32rx({"-m32rx"},
                               "  -m32rx             \tGenerate code for the M32R/X.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> M340({"-m340"},
                               "  -m340             \tGenerate code for the 210 processor.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> M3dnow({"-m3dnow"},
                               "  -m3dnow             \tThese switches enable the use of instructions "
                               "in the m3dnow.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> M3dnowa({"-m3dnowa"},
                               "  -m3dnowa             \tThese switches enable the use of instructions "
                               "in the m3dnowa.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> M3e({"-m3e"},
                               "  -m3e             \tGenerate code for the SH3e.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> M4({"-m4"},
                               "  -m4             \tGenerate code for the SH4.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> M4100({"-m4-100"},
                               "  -m4-100             \tGenerate code for SH4-100.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> M4100Nofpu({"-m4-100-nofpu"},
                               "  -m4-100-nofpu             \tGenerate code for SH4-100 in such a way that the "
                               "floating-point unit is not used.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> M4100Single({"-m4-100-single"},
                               "  -m4-100-single             \tGenerate code for SH4-100 assuming the floating-point "
                               "unit is in single-precision mode by default.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> M4100SingleOnly({"-m4-100-single-only"},
                               "  -m4-100-single-only             \tGenerate code for SH4-100 in such a way that no "
                               "double-precision floating-point operations are used.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> M4200({"-m4-200"},
                               "  -m4-200             \tGenerate code for SH4-200.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> M4200Nofpu({"-m4-200-nofpu"},
                               "  -m4-200-nofpu             \tGenerate code for SH4-200 without in such a way that"
                               " the floating-point unit is not used.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> M4200Single({"-m4-200-single"},
                               "  -m4-200-single             \tGenerate code for SH4-200 assuming the floating-point "
                               "unit is in single-precision mode by default.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> M4200SingleOnly({"-m4-200-single-only"},
                               "  -m4-200-single-only             \tGenerate code for SH4-200 in such a way that no "
                               "double-precision floating-point operations are used.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> M4300({"-m4-300"},
                               "  -m4-300             \tGenerate code for SH4-300.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> M4300Nofpu({"-m4-300-nofpu"},
                               "  -m4-300-nofpu             \tGenerate code for SH4-300 without in such a way that"
                               " the floating-point unit is not used.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> M4300Single({"-m4-300-single"},
                               "  -m4-300-single             \tGenerate code for SH4-300 in such a way that no "
                               "double-precision floating-point operations are used.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> M4300SingleOnly({"-m4-300-single-only"},
                               "  -m4-300-single-only             \tGenerate code for SH4-300 in such a way that "
                               "no double-precision floating-point operations are used.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> M4340({"-m4-340"},
                               "  -m4-340             \tGenerate code for SH4-340 (no MMU, no FPU).\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> M4500({"-m4-500"},
                               "  -m4-500             \tGenerate code for SH4-500 (no FPU). Passes "
                               "-isa=sh4-nofpu to the assembler.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> M4Nofpu({"-m4-nofpu"},
                               "  -m4-nofpu             \tGenerate code for the SH4al-dsp, or for a "
                               "SH4a in such a way that the floating-point unit is not used.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> M4Single({"-m4-single"},
                               "  -m4-single             \tGenerate code for the SH4a assuming the floating-point "
                               "unit is in single-precision mode by default.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> M4SingleOnly({"-m4-single-only"},
                               "  -m4-single-only             \tGenerate code for the SH4a, in such a way that "
                               "no double-precision floating-point operations are used.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> M40({"-m40"},
                               "  -m40             \tGenerate code for a PDP-11/40.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> M45({"-m45"},
                               "  -m45             \tGenerate code for a PDP-11/45. This is the default.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> M4a({"-m4a"},
                               "  -m4a             \tGenerate code for the SH4a.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> M4aNofpu({"-m4a-nofpu"},
                               "  -m4a-nofpu             \tGenerate code for the SH4al-dsp, or for a SH4a in such "
                               "a way that the floating-point unit is not used.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> M4aSingle({"-m4a-single"},
                               "  -m4a-single             \tGenerate code for the SH4a assuming the floating-point "
                               "unit is in single-precision mode by default.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> M4aSingleOnly({"-m4a-single-only"},
                               "  -m4a-single-only             \tGenerate code for the SH4a, in such a way that no"
                               " double-precision floating-point operations are used.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> M4al({"-m4al"},
                               "  -m4al             \tSame as -m4a-nofpu, except that it implicitly passes -dsp"
                               " to the assembler. GCC doesn't generate any DSP instructions at the moment.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> M4byteFunctions({"-m4byte-functions"},
                               "  -m4byte-functions             \tForce all functions to be aligned to a 4-byte"
                               " boundary.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-mno-4byte-functions"));

maplecl::Option<bool> M5200({"-m5200"},
                               "  -m5200             \tGenerate output for a 520X ColdFire CPU.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> M5206e({"-m5206e"},
                               "  -m5206e             \tGenerate output for a 5206e ColdFire CPU. \n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> M528x({"-m528x"},
                               "  -m528x             \tGenerate output for a member of the ColdFire 528X family. \n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> M5307({"-m5307"},
                               "  -m5307             \tGenerate output for a ColdFire 5307 CPU.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> M5407({"-m5407"},
                               "  -m5407             \tGenerate output for a ColdFire 5407 CPU.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> M64({"-m64"},
                               "  -m64             \tGenerate code for 64-bit ABI.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> M64bitDoubles({"-m64bit-doubles"},
                               "  -m64bit-doubles             \tMake the double data type be 64 bits (-m64bit-doubles)"
                               " in size.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> M68000({"-m68000"},
                               "  -m68000             \tGenerate output for a 68000.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> M68010({"-m68010"},
                               "  -m68010             \tGenerate output for a 68010.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> M68020({"-m68020"},
                               "  -m68020             \tGenerate output for a 68020. \n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> M6802040({"-m68020-40"},
                               "  -m68020-40             \tGenerate output for a 68040, without using any of "
                               "the new instructions. \n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> M6802060({"-m68020-60"},
                               "  -m68020-60             \tGenerate output for a 68060, without using any of "
                               "the new instructions. \n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> M68030({"-m68030"},
                               "  -m68030             \tGenerate output for a 68030. \n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> M68040({"-m68040"},
                               "  -m68040             \tGenerate output for a 68040.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> M68060({"-m68060"},
                               "  -m68060             \tGenerate output for a 68060.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> M68881({"-m68881"},
                               "  -m68881             \tGenerate floating-point instructions. \n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> M8Bit({"-m8-bit"},
                               "  -m8-bit             \tArrange for stack frame, writable data and constants to "
                               "all 8-bit aligned\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> M8bitIdiv({"-m8bit-idiv"},
                               "  -m8bit-idiv             \tThis option generates a run-time check\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> M8byteAlign({"-m8byte-align"},
                               "  -m8byte-align             \tEnables support for double and long long types to be "
                               "aligned on 8-byte boundaries.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-mno-8byte-align"));

maplecl::Option<bool> M96bitLongDouble({"-m96bit-long-double"},
                               "  -m96bit-long-double             \tControl the size of long double type\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MA6({"-mA6"},
                               "  -mA6             \tCompile for ARC600.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MA7({"-mA7"},
                               "  -mA7             \tCompile for ARC700.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mabicalls({"-mabicalls"},
                               "  -mabicalls             \tGenerate (do not generate) code that is suitable for "
                               "SVR4-style dynamic objects. -mabicalls is the default for SVR4-based systems.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-mno-abicalls"));

maplecl::Option<bool> Mabm({"-mabm"},
                               "  -mabm             \tThese switches enable the use of instructions in the mabm.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MabortOnNoreturn({"-mabort-on-noreturn"},
                               "  -mabort-on-noreturn             \tGenerate a call to the function abort at "
                               "the end of a noreturn function.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mabs2008({"-mabs=2008"},
                               "  -mabs=2008             \tThe option selects the IEEE 754-2008 treatment\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MabsLegacy({"-mabs=legacy"},
                               "  -mabs=legacy             \tThe legacy treatment is selected\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mabsdata({"-mabsdata"},
                               "  -mabsdata             \tAssume that all data in static storage can be "
                               "accessed by LDS / STS instructions.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mabsdiff({"-mabsdiff"},
                               "  -mabsdiff             \tEnables the abs instruction, which is the absolute "
                               "difference between two registers.n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mabshi({"-mabshi"},
                               "  -mabshi             \tUse abshi2 pattern. This is the default.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-mno-abshi"));

maplecl::Option<bool> Mac0({"-mac0"},
                               "  -mac0             \tReturn floating-point results in ac0 "
                               "(fr0 in Unix assembler syntax).\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-mno-ac0"));

maplecl::Option<bool> Macc4({"-macc-4"},
                               "  -macc-4             \tUse only the first four media accumulator registers.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Macc8({"-macc-8"},
                               "  -macc-8             \tUse all eight media accumulator registers.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MaccumulateArgs({"-maccumulate-args"},
                               "  -maccumulate-args             \tAccumulate outgoing function arguments and "
                               "acquire/release the needed stack space for outgoing function arguments once in"
                               " function prologue/epilogue.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MaccumulateOutgoingArgs({"-maccumulate-outgoing-args"},
                               "  -maccumulate-outgoing-args             \tReserve space once for outgoing arguments "
                               "in the function prologue rather than around each call. Generally beneficial for "
                               "performance and size\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MaddressModeLong({"-maddress-mode=long"},
                               "  -maddress-mode=long             \tGenerate code for long address mode.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MaddressModeShort({"-maddress-mode=short"},
                               "  -maddress-mode=short             \tGenerate code for short address mode.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MaddressSpaceConversion({"-maddress-space-conversion"},
                               "  -maddress-space-conversion             \tAllow/disallow treating the __ea address "
                               "space as superset of the generic address space. \n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-mno-address-space-conversion"));

maplecl::Option<bool> Mads({"-mads"},
                               "  -mads             \tOn embedded PowerPC systems, assume that the startup module "
                               "is called crt0.o and the standard C libraries are libads.a and libc.a.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Maes({"-maes"},
                               "  -maes             \tThese switches enable the use of instructions in the maes.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MaixStructReturn({"-maix-struct-return"},
                               "  -maix-struct-return             \tReturn all structures in memory "
                               "(as specified by the AIX ABI).\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Maix32({"-maix32"},
                               "  -maix32             \tEnable 64-bit AIX ABI and calling convention: 32-bit "
                               "pointers, 32-bit long type, and the infrastructure needed to support them.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Maix64({"-maix64"},
                               "  -maix64             \tEnable 64-bit AIX ABI and calling convention: 64-bit "
                               "pointers, 64-bit long type, and the infrastructure needed to support them.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Malign300({"-malign-300"},
                               "  -malign-300             \tOn the H8/300H and H8S, use the same alignment "
                               "rules as for the H8/300\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MalignCall({"-malign-call"},
                               "  -malign-call             \tDo alignment optimizations for call instructions.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<std::string> MalignData({"-malign-data"},
                               "  -malign-data             \tControl how GCC aligns variables. \n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MalignDouble({"-malign-double"},
                               "  -malign-double             \tControl whether GCC aligns double, long double, and "
                               "long long variables on a two-word boundary or a one-word boundary.\n",
                               {driverCategory, unSupCategory},
                                maplecl::DisableWith("-mno-align-double"));

maplecl::Option<bool> MalignInt({"-malign-int"},
                               "  -malign-int             \tAligning variables on 32-bit boundaries produces code that"
                               " runs somewhat faster on processors with 32-bit busses at the expense of more "
                               "memory.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-mno-align-int"));

maplecl::Option<bool> MalignLabels({"-malign-labels"},
                               "  -malign-labels             \tTry to align labels to an 8-byte boundary by inserting"
                               " NOPs into the previous packet. \n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MalignLoops({"-malign-loops"},
                               "  -malign-loops             \tAlign all loops to a 32-byte boundary.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-mno-align-loops"));

maplecl::Option<bool> MalignNatural({"-malign-natural"},
                               "  -malign-natural             \tThe option -malign-natural overrides the ABI-defined "
                               "alignment of larger types\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MalignPower({"-malign-power"},
                               "  -malign-power             \tThe option -malign-power instructs Maple to follow "
                               "the ABI-specified alignment rules\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MallOpts({"-mall-opts"},
                               "  -mall-opts             \tEnables all the optional instructions—average, multiply, "
                               "divide, bit operations, leading zero, absolute difference, min/max, clip, "
                               "and saturation.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-opts"));

maplecl::Option<bool> MallocCc({"-malloc-cc"},
                               "  -malloc-cc             \tDynamically allocate condition code registers.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MallowStringInsns({"-mallow-string-insns"},
                               "  -mallow-string-insns             \tEnables or disables the use of the string "
                               "manipulation instructions SMOVF, SCMPU, SMOVB, SMOVU, SUNTIL SWHILE and also the "
                               "RMPA instruction.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-mno-allow-string-insns"));

maplecl::Option<bool> Mallregs({"-mallregs"},
                               "  -mallregs             \tAllow the compiler to use all of the available registers.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Maltivec({"-maltivec"},
                               "  -maltivec             \tGenerate code that uses (does not use) AltiVec instructions, "
                               "and also enable the use of built-in functions that allow more direct access to the "
                               "AltiVec instruction set. \n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-mno-altivec"));

maplecl::Option<bool> MaltivecBe({"-maltivec=be"},
                               "  -maltivec=be             \tGenerate AltiVec instructions using big-endian element "
                               "order, regardless of whether the target is big- or little-endian. \n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MaltivecLe({"-maltivec=le"},
                               "  -maltivec=le             \tGenerate AltiVec instructions using little-endian element"
                               " order, regardless of whether the target is big- or little-endian.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mam33({"-mam33"},
                               "  -mam33             \tGenerate code using features specific to the AM33 processor.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-mno-am33"));

maplecl::Option<bool> Mam332({"-mam33-2"},
                               "  -mam33-2             \tGenerate code using features specific to the "
                               "AM33/2.0 processor.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mam34({"-mam34"},
                               "  -mam34             \tGenerate code using features specific to the AM34 processor.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mandroid({"-mandroid"},
                               "  -mandroid             \tCompile code compatible with Android platform.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MannotateAlign({"-mannotate-align"},
                               "  -mannotate-align             \tExplain what alignment considerations lead to the "
                               "decision to make an instruction short or long.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mapcs({"-mapcs"},
                               "  -mapcs             \tThis is a synonym for -mapcs-frame and is deprecated.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MapcsFrame({"-mapcs-frame"},
                               "  -mapcs-frame             \tGenerate a stack frame that is compliant with the ARM "
                               "Procedure Call Standard for all functions, even if this is not strictly necessary "
                               "for correct execution of the code. \n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MappRegs({"-mapp-regs"},
                               "  -mapp-regs             \tSpecify -mapp-regs to generate output using the global "
                               "registers 2 through 4, which the SPARC SVR4 ABI reserves for applications. \n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-mno-app-regs"));

maplecl::Option<bool> MARC600({"-mARC600"},
                               "  -mARC600             \tCompile for ARC600.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MARC601({"-mARC601"},
                               "  -mARC601             \tCompile for ARC601.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MARC700({"-mARC700"},
                               "  -mARC700             \tCompile for ARC700.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Marclinux({"-marclinux"},
                               "  -marclinux             \tPassed through to the linker, to specify use of the "
                               "arclinux emulation.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Marclinux_prof({"-marclinux_prof"},
                               "  -marclinux_prof             \tPassed through to the linker, to specify use of "
                               "the arclinux_prof emulation. \n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Margonaut({"-margonaut"},
                               "  -margonaut             \tObsolete FPX.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Marm({"-marm"},
                               "  -marm             \tSelect between generating code that executes in ARM and "
                               "Thumb states. \n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mas100Syntax({"-mas100-syntax"},
                               "  -mas100-syntax             \tWhen generating assembler output use a syntax that "
                               "is compatible with Renesas’s AS100 assembler. \n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-mno-as100-syntax"));

maplecl::Option<bool> MasmHex({"-masm-hex"},
                               "  -masm-hex             \tForce assembly output to always use hex constants.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MasmSyntaxUnified({"-masm-syntax-unified"},
                               "  -masm-syntax-unified             \tAssume inline assembler is using unified "
                               "asm syntax. \n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MasmDialect({"-masm=dialect"},
                               "  -masm=dialect             \tOutput assembly instructions using selected dialect. \n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Matomic({"-matomic"},
                               "  -matomic             \tThis enables use of the locked load/store conditional "
                               "extension to implement atomic memory built-in functions.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<std::string> MatomicModel({"-matomic-model"},
                               "  -matomic-model             \tSets the model of atomic operations and additional "
                               "parameters as a comma separated list. \n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MatomicUpdates({"-matomic-updates"},
                               "  -matomic-updates             \tThis option controls the version of libgcc that the "
                               "compiler links to an executable and selects whether atomic updates to the "
                               "software-managed cache of PPU-side variables are used. \n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-mno-atomic-updates"));

maplecl::Option<bool> MautoLitpools({"-mauto-litpools"},
                               "  -mauto-litpools             \tControl the treatment of literal pools.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-mno-auto-litpools"));

maplecl::Option<bool> MautoModifyReg({"-mauto-modify-reg"},
                               "  -mauto-modify-reg             \tEnable the use of pre/post modify with "
                               "register displacement.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MautoPic({"-mauto-pic"},
                               "  -mauto-pic             \tGenerate code that is self-relocatable. \n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Maverage({"-maverage"},
                               "  -maverage             \tEnables the ave instruction, which computes the "
                               "average of two registers.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MavoidIndexedAddresses({"-mavoid-indexed-addresses"},
                               "  -mavoid-indexed-addresses             \tGenerate code that tries to avoid "
                               "(not avoid) the use of indexed load or store instructions.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-mno-avoid-indexed-addresses"));

maplecl::Option<bool> Mavx({"-mavx"},
                               "  -mavx             \tMaple depresses SSEx instructions when -mavx is used. \n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mavx2({"-mavx2"},
                               "  -mavx2             \tEnable the use of instructions in the mavx2.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mavx256SplitUnalignedLoad({"-mavx256-split-unaligned-load"},
                               "  -mavx256-split-unaligned-load             \tSplit 32-byte AVX unaligned load .\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mavx256SplitUnalignedStore({"-mavx256-split-unaligned-store"},
                               "  -mavx256-split-unaligned-store             \tSplit 32-byte AVX unaligned store.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mavx512bw({"-mavx512bw"},
                               "  -mavx512bw             \tEnable the use of instructions in the mavx512bw.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mavx512cd({"-mavx512cd"},
                               "  -mavx512cd             \tEnable the use of instructions in the mavx512cd.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mavx512dq({"-mavx512dq"},
                               "  -mavx512dq             \tEnable the use of instructions in the mavx512dq.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mavx512er({"-mavx512er"},
                               "  -mavx512er             \tEnable the use of instructions in the mavx512er.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mavx512f({"-mavx512f"},
                               "  -mavx512f             \tEnable the use of instructions in the mavx512f.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mavx512ifma({"-mavx512ifma"},
                               "  -mavx512ifma             \tEnable the use of instructions in the mavx512ifma.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mavx512pf({"-mavx512pf"},
                               "  -mavx512pf             \tEnable the use of instructions in the mavx512pf.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mavx512vbmi({"-mavx512vbmi"},
                               "  -mavx512vbmi             \tEnable the use of instructions in the mavx512vbmi.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mavx512vl({"-mavx512vl"},
                               "  -mavx512vl             \tEnable the use of instructions in the mavx512vl.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<std::string> MaxVectAlign({"-max-vect-align"},
                               "  -max-vect-align             \tThe maximum alignment for SIMD vector mode types.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mb({"-mb"},
                               "  -mb             \tCompile code for the processor in big-endian mode.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mbackchain({"-mbackchain"},
                               "  -mbackchain             \tStore (do not store) the address of the caller's frame as"
                               " backchain pointer into the callee's stack frame.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-mno-backchain"));

maplecl::Option<bool> MbarrelShiftEnabled({"-mbarrel-shift-enabled"},
                               "  -mbarrel-shift-enabled             \tEnable barrel-shift instructions.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MbarrelShifter({"-mbarrel-shifter"},
                               "  -mbarrel-shifter             \tGenerate instructions supported by barrel shifter. \n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mbarrel_shifter({"-mbarrel_shifter"},
                               "  -mbarrel_shifter             \tReplaced by -mbarrel-shifter.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MbaseAddresses({"-mbase-addresses"},
                               "  -mbase-addresses             \tGenerate code that uses base addresses. \n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-base-addresses"));

maplecl::Option<std::string> Mbased({"-mbased"},
                               "  -mbased             \tVariables of size n bytes or smaller are placed in the .based "
                               "section by default\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MbbitPeephole({"-mbbit-peephole"},
                               "  -mbbit-peephole             \tEnable bbit peephole2.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mbcopy({"-mbcopy"},
                               "  -mbcopy             \tDo not use inline movmemhi patterns for copying memory.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MbcopyBuiltin({"-mbcopy-builtin"},
                               "  -mbcopy-builtin             \tUse inline movmemhi patterns for copying memory. \n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mbig({"-mbig"},
                               "  -mbig             \tCompile code for big-endian targets.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MbigEndianData({"-mbig-endian-data"},
                               "  -mbig-endian-data             \tStore data (but not code) in the big-endian "
                               "format.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MbigSwitch({"-mbig-switch"},
                               "  -mbig-switch             \tGenerate code suitable for big switch tables.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mbigtable({"-mbigtable"},
                               "  -mbigtable             \tUse 32-bit offsets in switch tables. \n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mbionic({"-mbionic"},
                               "  -mbionic             \tUse Bionic C library.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MbitAlign({"-mbit-align"},
                               "  -mbit-align             \tOn System V.4 and embedded PowerPC systems do not force "
                               "structures and unions that contain bit-fields to be aligned to the base type of the "
                               "bit-field.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-bit-align"));

maplecl::Option<bool> MbitOps({"-mbit-ops"},
                               "  -mbit-ops             \tGenerates sbit/cbit instructions for bit manipulations.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mbitfield({"-mbitfield"},
                               "  -mbitfield             \tDo use the bit-field instructions.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-bitfield"));

maplecl::Option<bool> Mbitops({"-mbitops"},
                               "  -mbitops             \tEnables the bit operation instructions—bit test (btstm), "
                               "set (bsetm), clear (bclrm), invert (bnotm), and test-and-set (tas).\n",
                               {driverCategory, unSupCategory});

maplecl::Option<std::string> MblockMoveInlineLimit({"-mblock-move-inline-limit"},
                               "  -mblock-move-inline-limit             \tInline all block moves (such as calls "
                               "to memcpy or structure copies) less than or equal to num bytes. \n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mbmi({"-mbmi"},
                               "  -mbmi             \tThese switches enable the use of instructions in the mbmi.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MbranchCheap({"-mbranch-cheap"},
                               "  -mbranch-cheap             \tDo not pretend that branches are expensive. \n",
                               {driverCategory, unSupCategory});

maplecl::Option<std::string> MbranchCost({"-mbranch-cost"},
                               "  -mbranch-cost             \tSet the cost of branches to roughly num “simple” "
                               "instructions.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MbranchExpensive({"-mbranch-expensive"},
                               "  -mbranch-expensive             \tPretend that branches are expensive.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MbranchHints({"-mbranch-hints"},
                               "  -mbranch-hints             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MbranchLikely({"-mbranch-likely"},
                               "  -mbranch-likely             \tEnable or disable use of Branch Likely instructions, "
                               "regardless of the default for the selected architecture.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-branch-likely"));

maplecl::Option<bool> MbranchPredict({"-mbranch-predict"},
                               "  -mbranch-predict             \tUse the probable-branch instructions, when static "
                               "branch prediction indicates a probable branch.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-branch-predict"));

maplecl::Option<bool> MbssPlt({"-mbss-plt"},
                               "  -mbss-plt             \tGenerate code that uses a BSS .plt section that ld.so "
                               "fills in, and requires .plt and .got sections that are both writable and executable.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MbuildConstants({"-mbuild-constants"},
                               "  -mbuild-constants             \tConstruct all integer constants using code, even "
                               "if it takes more instructions (the maximum is six).\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mbwx({"-mbwx"},
                               "  -mbwx             \tGenerate code to use the optional BWX instruction sets.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-bwx"));

maplecl::Option<bool> MbypassCache({"-mbypass-cache"},
                               "  -mbypass-cache             \t\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-bypass-cache"));

maplecl::Option<bool> Mc68000({"-mc68000"},
                               "  -mc68000             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mc68020({"-mc68020"},
                               "  -mc68020             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<std::string> Mc({"-mc"},
                               "  -mc             \tSelects which section constant data is placed in. name may "
                               "be 'tiny', 'near', or 'far'.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<std::string> McacheBlockSize({"-mcache-block-size"},
                               "  -mcache-block-size             \tSpecify the size of each cache block, which must "
                               "be a power of 2 between 4 and 512.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<std::string> McacheSize({"-mcache-size"},
                               "  -mcache-size             \tThis option controls the version of libgcc that the "
                               "compiler links to an executable and selects a software-managed cache for accessing "
                               "variables in the __ea address space with a particular cache size.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> McacheVolatile({"-mcache-volatile"},
                               "  -mcache-volatile             \t\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-cache-volatile"));

maplecl::Option<bool> McallEabi({"-mcall-eabi"},
                               "  -mcall-eabi             \tSpecify both -mcall-sysv and -meabi options.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> McallAixdesc({"-mcall-aixdesc"},
                               "  -mcall-aixdesc             \tOn System V.4 and embedded PowerPC systems "
                               "compile code for the AIX operating system.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> McallFreebsd({"-mcall-freebsd"},
                               "  -mcall-freebsd             \tOn System V.4 and embedded PowerPC systems compile "
                               "code for the FreeBSD operating system.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> McallLinux({"-mcall-linux"},
                               "  -mcall-linux             \tOn System V.4 and embedded PowerPC systems compile "
                               "code for the Linux-based GNU system.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> McallOpenbsd({"-mcall-openbsd"},
                               "  -mcall-openbsd             \tOn System V.4 and embedded PowerPC systems compile "
                               "code for the OpenBSD operating system.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> McallNetbsd({"-mcall-netbsd"},
                               "  -mcall-netbsd             \tOn System V.4 and embedded PowerPC systems compile"
                               " code for the NetBSD operating system.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> McallPrologues({"-mcall-prologues"},
                               "  -mcall-prologues             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> McallSysv({"-mcall-sysv"},
                               "  -mcall-sysv             \tOn System V.4 and embedded PowerPC systems compile code "
                               "using calling conventions that adhere to the March 1995 draft of the System V "
                               "Application Binary Interface, PowerPC processor supplement. \n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> McallSysvEabi({"-mcall-sysv-eabi"},
                               "  -mcall-sysv-eabi             \tSpecify both -mcall-sysv and -meabi options.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> McallSysvNoeabi({"-mcall-sysv-noeabi"},
                               "  -mcall-sysv-noeabi             \tSpecify both -mcall-sysv and -mno-eabi options.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> McalleeSuperInterworking({"-mcallee-super-interworking"},
                               "  -mcallee-super-interworking             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> McallerCopies({"-mcaller-copies"},
                               "  -mcaller-copies             \tThe caller copies function arguments "
                               "passed by hidden reference.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> McallerSuperInterworking({"-mcaller-super-interworking"},
                               "  -mcaller-super-interworking             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> McallgraphData({"-mcallgraph-data"},
                               "  -mcallgraph-data             \t\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-callgraph-data"));

maplecl::Option<bool> McaseVectorPcrel({"-mcase-vector-pcrel"},
                               "  -mcase-vector-pcrel             \tUse PC-relative switch case tables to enable "
                               "case table shortening. \n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mcbcond({"-mcbcond"},
                               "  -mcbcond             \tWith -mcbcond, Maple generates code that takes advantage of "
                               "the UltraSPARC Compare-and-Branch-on-Condition instructions. \n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-cbcond"));

maplecl::Option<bool> McbranchForceDelaySlot({"-mcbranch-force-delay-slot"},
                               "  -mcbranch-force-delay-slot             \tForce the usage of delay slots for "
                               "conditional branches, which stuffs the delay slot with a nop if a suitable "
                               "instruction cannot be found. \n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MccInit({"-mcc-init"},
                               "  -mcc-init             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mcfv4e({"-mcfv4e"},
                               "  -mcfv4e             \tGenerate output for a ColdFire V4e family CPU "
                               "(e.g. 547x/548x).\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> McheckZeroDivision({"-mcheck-zero-division"},
                               "  -mcheck-zero-division             \tTrap (do not trap) on integer division "
                               "by zero.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-check-zero-division"));

maplecl::Option<bool> Mcix({"-mcix"},
                               "  -mcix             \tGenerate code to use the optional CIX instruction sets.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-cix"));

maplecl::Option<bool> Mcld({"-mcld"},
                               "  -mcld             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MclearHwcap({"-mclear-hwcap"},
                               "  -mclear-hwcap             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mclflushopt({"-mclflushopt"},
                               "  -mclflushopt             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mclip({"-mclip"},
                               "  -mclip             \tEnables the clip instruction.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mclzero({"-mclzero"},
                               "  -mclzero             \tThese switches enable the use of instructions in the m"
                               "clzero.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<std::string> Mcmodel({"-mcmodel"},
                               "  -mcmodel             \tSpecify the code model.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mcmov({"-mcmov"},
                               "  -mcmov             \tGenerate conditional move instructions.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-cmov"));

maplecl::Option<bool> Mcmove({"-mcmove"},
                               "  -mcmove             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mcmpb({"-mcmpb"},
                               "  -mcmpb             \tSpecify which instructions are available on "
                               "the processor you are using. \n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-cmpb"));

maplecl::Option<bool> Mcmse({"-mcmse"},
                               "  -mcmse             \tGenerate secure code as per the ARMv8-M Security Extensions.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> McodeDensity({"-mcode-density"},
                               "  -mcode-density             \tEnable code density instructions for ARC EM. \n",
                               {driverCategory, unSupCategory});

maplecl::Option<std::string> McodeReadable({"-mcode-readable"},
                               "  -mcode-readable             \tSpecify whether Maple may generate code that reads "
                               "from executable sections.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> McodeRegion({"-mcode-region"},
                               "  -mcode-region             \tthe compiler where to place functions and data that "
                               "do not have one of the lower, upper, either or section attributes.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> McompactBranchesAlways({"-mcompact-branches=always"},
                               "  -mcompact-branches=always             \tThe -mcompact-branches=always option "
                               "ensures that a compact branch instruction will be generated if available. \n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> McompactBranchesNever({"-mcompact-branches=never"},
                               "  -mcompact-branches=never             \tThe -mcompact-branches=never option ensures "
                               "that compact branch instructions will never be generated.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> McompactBranchesOptimal({"-mcompact-branches=optimal"},
                               "  -mcompact-branches=optimal             \tThe -mcompact-branches=optimal option "
                               "will cause a delay slot branch to be used if one is available in the current ISA "
                               "and the delay slot is successfully filled.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> McompactCasesi({"-mcompact-casesi"},
                               "  -mcompact-casesi             \tEnable compact casesi pattern.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> McompatAlignParm({"-mcompat-align-parm"},
                               "  -mcompat-align-parm             \tGenerate code to pass structure parameters with a"
                               " maximum alignment of 64 bits, for compatibility with older versions of maple.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> McondExec({"-mcond-exec"},
                               "  -mcond-exec             \tEnable the use of conditional execution (default).\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-mno-cond-exec"));

maplecl::Option<bool> McondMove({"-mcond-move"},
                               "  -mcond-move             \tEnable the use of conditional-move "
                               "instructions (default).\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-cond-move"));

maplecl::Option<std::string> Mconfig({"-mconfig"},
                               "  -mconfig             \tSelects one of the built-in core configurations. \n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mconsole({"-mconsole"},
                               "  -mconsole             \tThis option specifies that a console application is to "
                               "be generated, by instructing the linker to set the PE header subsystem type required "
                               "for console applications.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MconstAlign({"-mconst-align"},
                               "  -mconst-align             \t\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-const-align"));

maplecl::Option<bool> Mconst16({"-mconst16"},
                               "  -mconst16             \t\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-const16"));

maplecl::Option<bool> MconstantGp({"-mconstant-gp"},
                               "  -mconstant-gp             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mcop({"-mcop"},
                               "  -mcop             \tEnables the coprocessor instructions. \n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mcop32({"-mcop32"},
                               "  -mcop32             \tEnables the 32-bit coprocessor's instructions.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mcop64({"-mcop64"},
                               "  -mcop64             \tEnables the 64-bit coprocessor's instructions.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mcorea({"-mcorea"},
                               "  -mcorea             \tBuild a standalone application for Core A of BF561 when "
                               "using the one-application-per-core programming model.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mcoreb({"-mcoreb"},
                               "  -mcoreb             \tBuild a standalone application for Core B of BF561 when "
                               "using the one-application-per-core programming model.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<std::string> Mcpu({"-mcpu"},
                               "  -mcpu             \tSpecify the name of the target processor, optionally suffixed "
                               "by one or more feature modifiers. \n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mcpu32({"-mcpu32"},
                               "  -mcpu32             \tGenerate output for a CPU32. \n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mcr16c({"-mcr16c"},
                               "  -mcr16c             \tGenerate code for CR16C architecture. \n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mcr16cplus({"-mcr16cplus"},
                               "  -mcr16cplus             \tGenerate code for CR16C+ architecture. \n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mcrc32({"-mcrc32"},
                               "  -mcrc32             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mcrypto({"-mcrypto"},
                               "  -mcrypto             \tEnable the use of the built-in functions that allow direct "
                               "access to the cryptographic instructions that were added in version 2.07 of "
                               "the PowerPC ISA.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-crypto"));

maplecl::Option<bool> McsyncAnomaly({"-mcsync-anomaly"},
                               "  -mcsync-anomaly             \tWhen enabled, the compiler ensures that the generated "
                               "code does not contain CSYNC or SSYNC instructions too soon after conditional"
                               " branches.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-csync-anomaly"));

maplecl::Option<bool> MctorDtor({"-mctor-dtor"},
                               "  -mctor-dtor             \tEnable constructor/destructor feature.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> McustomFpuCfg({"-mcustom-fpu-cfg"},
                               "  -mcustom-fpu-cfg             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> McustomInsn({"-mcustom-insn"},
                               "  -mcustom-insn             \t\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-custom-insn"));

maplecl::Option<bool> Mcx16({"-mcx16"},
                               "  -mcx16             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mdalign({"-mdalign"},
                               "  -mdalign             \tAlign doubles at 64-bit boundaries.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MdataAlign({"-mdata-align"},
                               "  -mdata-align             \t\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-data-align"));

maplecl::Option<bool> MdataModel({"-mdata-model"},
                               "  -mdata-model             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<std::string> MdataRegion({"-mdata-region"},
                               "  -mdata-region             \ttell the compiler where to place functions and data "
                               "that do not have one of the lower, upper, either or section attributes.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mdc({"-mdc"},
                               "  -mdc             \tCauses constant variables to be placed in the .near section.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mdebug({"-mdebug"},
                               "  -mdebug             \tPrint additional debug information when compiling. \n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-debug"));

maplecl::Option<bool> MdebugMainPrefix({"-mdebug-main=prefix"},
                               "  -mdebug-main=prefix             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MdecAsm({"-mdec-asm"},
                               "  -mdec-asm             \tUse DEC assembler syntax. \n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MdirectMove({"-mdirect-move"},
                               "  -mdirect-move             \tGenerate code that uses the instructions to move "
                               "data between the general purpose registers and the vector/scalar (VSX) registers "
                               "that were added in version 2.07 of the PowerPC ISA.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-direct-move"));

maplecl::Option<bool> MdisableCallt({"-mdisable-callt"},
                               "  -mdisable-callt             \tThis option suppresses generation of the "
                               "CALLT instruction for the v850e, v850e1, v850e2, v850e2v3 and v850e3v5 flavors "
                               "of the v850 architecture.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-disable-callt"));

maplecl::Option<bool> MdisableFpregs({"-mdisable-fpregs"},
                               "  -mdisable-fpregs             \tPrevent floating-point registers from being "
                               "used in any manner.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MdisableIndexing({"-mdisable-indexing"},
                               "  -mdisable-indexing             \tPrevent the compiler from using indexing address "
                               "modes.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mdiv({"-mdiv"},
                               "  -mdiv             \tEnables the div and divu instructions.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-div"));

maplecl::Option<bool> MdivRem({"-mdiv-rem"},
                               "  -mdiv-rem             \tEnable div and rem instructions for ARCv2 cores.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MdivStrategy({"-mdiv=strategy"},
                               "  -mdiv=strategy             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MdivideBreaks({"-mdivide-breaks"},
                               "  -mdivide-breaks             \tMIPS systems check for division by zero by generating "
                               "either a conditional trap or a break instruction.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MdivideEnabled({"-mdivide-enabled"},
                               "  -mdivide-enabled             \tEnable divide and modulus instructions.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MdivideTraps({"-mdivide-traps"},
                               "  -mdivide-traps             \tMIPS systems check for division by zero by generating "
                               "either a conditional trap or a break instruction.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<std::string> Mdivsi3_libfuncName({"-mdivsi3_libfunc"},
                               "  -mdivsi3_libfunc             \tSet the name of the library function "
                               "used for 32-bit signed division to name.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mdll({"-mdll"},
                               "  -mdll             \tThis option is available for Cygwin and MinGW targets.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mdlmzb({"-mdlmzb"},
                               "  -mdlmzb             \tGenerate code that uses the string-search 'dlmzb' "
                               "instruction on the IBM 405, 440, 464 and 476 processors. \n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-dlmzb"));

maplecl::Option<bool> Mdmx({"-mdmx"},
                               "  -mdmx             \tUse MIPS Digital Media Extension instructions. \n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-mdmx"));

maplecl::Option<bool> Mdouble({"-mdouble"},
                               "  -mdouble             \tUse floating-point double instructions.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-double"));

maplecl::Option<bool> MdoubleFloat({"-mdouble-float"},
                               "  -mdouble-float             \tGenerate code for double-precision floating-point "
                               "operations.",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mdpfp({"-mdpfp"},
                               "  -mdpfp             \tGenerate double-precision FPX instructions, tuned for the "
                               "compact implementation.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MdpfpCompact({"-mdpfp-compact"},
                               "  -mdpfp-compact             \tGenerate double-precision FPX instructions, tuned "
                               "for the compact implementation.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MdpfpFast({"-mdpfp-fast"},
                               "  -mdpfp-fast             \tGenerate double-precision FPX instructions, tuned "
                               "for the fast implementation.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mdpfp_compact({"-mdpfp_compact"},
                               "  -mdpfp_compact             \tReplaced by -mdpfp-compact.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mdpfp_fast({"-mdpfp_fast"},
                               "  -mdpfp_fast             \tReplaced by -mdpfp-fast.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mdsp({"-mdsp"},
                               "  -mdsp             \t\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-dsp"));

maplecl::Option<bool> MdspPacka({"-mdsp-packa"},
                               "  -mdsp-packa             \tPassed down to the assembler to enable the DSP Pack A "
                               "extensions. \n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mdspr2({"-mdspr2"},
                               "  -mdspr2             \tUse revision 2 of the MIPS DSP ASE.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-dspr2"));

maplecl::Option<bool> Mdsp_packa({"-mdsp_packa"},
                               "  -mdsp_packa             \tReplaced by -mdsp-packa.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MdualNops({"-mdual-nops"},
                               "  -mdual-nops             \tBy default, GCC inserts NOPs to increase dual issue when "
                               "it expects it to increase performance.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<std::string> MdualNopsE({"-mdual-nops="},
                               "  -mdual-nops=             \tBy default, GCC inserts NOPs to increase dual issue when "
                               "it expects it to increase performance.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MdumpTuneFeatures({"-mdump-tune-features"},
                               "  -mdump-tune-features             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mdvbf({"-mdvbf"},
                               "  -mdvbf             \tPassed down to the assembler to enable the dual Viterbi "
                               "butterfly extension.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mdwarf2Asm({"-mdwarf2-asm"},
                               "  -mdwarf2-asm             \t\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-dwarf2-asm"));

maplecl::Option<bool> Mdword({"-mdword"},
                               "  -mdword             \tChange ABI to use double word insns.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-dword"));

maplecl::Option<bool> MdynamicNoPic({"-mdynamic-no-pic"},
                               "  -mdynamic-no-pic             \tOn Darwin and Mac OS X systems, compile code so "
                               "that it is not relocatable, but that its external references are relocatable. \n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mea({"-mea"},
                               "  -mea             \tGenerate extended arithmetic instructions.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MEa({"-mEa"},
                               "  -mEa             \tReplaced by -mea.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mea32({"-mea32"},
                               "  -mea32             \tCompile code assuming that pointers to the PPU address space "
                               "accessed via the __ea named address space qualifier are either 32 bits wide. \n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mea64({"-mea64"},
                               "  -mea64             \tCompile code assuming that pointers to the PPU address space "
                               "accessed via the __ea named address space qualifier are either 64 bits wide. \n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Meabi({"-meabi"},
                               "  -meabi             \tOn System V.4 and embedded PowerPC systems adhere to the "
                               "Embedded Applications Binary Interface (EABI), which is a set of modifications to "
                               "the System V.4 specifications. \n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-eabi"));

maplecl::Option<bool> MearlyCbranchsi({"-mearly-cbranchsi"},
                               "  -mearly-cbranchsi             \tEnable pre-reload use of the cbranchsi pattern.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MearlyStopBits({"-mearly-stop-bits"},
                               "  -mearly-stop-bits             \t\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-early-stop-bits"));

maplecl::Option<bool> Meb({"-meb"},
                               "  -meb             \tGenerate big-endian code.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mel({"-mel"},
                               "  -mel             \tGenerate little-endian code.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Melf({"-melf"},
                               "  -melf             \tGenerate an executable in the ELF format, rather than the "
                               "default 'mmo' format used by the mmix simulator.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Memb({"-memb"},
                               "  -memb             \tOn embedded PowerPC systems, set the PPC_EMB bit in the ELF "
                               "flags header to indicate that 'eabi' extended relocations are used.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MembeddedData({"-membedded-data"},
                               "  -membedded-data             \tAllocate variables to the read-only data section "
                               "first if possible, then next in the small data section if possible, otherwise "
                               "in data.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-embedded-data"));

maplecl::Option<std::string> MemregsE({"-memregs="},
                               "  -memregs=             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mep({"-mep"},
                               "  -mep             \tDo not optimize basic blocks that use the same index pointer 4 "
                               "or more times to copy pointer into the ep register, and use the shorter sld and sst "
                               "instructions. \n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-ep"));

maplecl::Option<bool> Mepsilon({"-mepsilon"},
                               "  -mepsilon             \tGenerate floating-point comparison instructions that "
                               "compare with respect to the rE epsilon register.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-epsilon"));

maplecl::Option<bool> Mesa({"-mesa"},
                               "  -mesa             \tWhen -mesa is specified, generate code using the "
                               "instructions available on ESA/390.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Metrax100({"-metrax100"},
                               "  -metrax100             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Metrax4({"-metrax4"},
                               "  -metrax4             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Meva({"-meva"},
                               "  -meva             \tUse the MIPS Enhanced Virtual Addressing instructions.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-eva"));

maplecl::Option<bool> MexpandAdddi({"-mexpand-adddi"},
                               "  -mexpand-adddi             \tExpand adddi3 and subdi3 at RTL generation time "
                               "into add.f, adc etc.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MexplicitRelocs({"-mexplicit-relocs"},
                               "  -mexplicit-relocs             \tUse assembler relocation operators when dealing "
                               "with symbolic addresses.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-explicit-relocs"));

maplecl::Option<bool> Mexr({"-mexr"},
                               "  -mexr             \t\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-exr"));

maplecl::Option<bool> MexternSdata({"-mextern-sdata"},
                               "  -mextern-sdata             \tAssume (do not assume) that externally-defined data "
                               "is in a small data section if the size of that data is within the -G limit. "
                               "-mextern-sdata is the default for all configurations.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-extern-sdata"));

maplecl::Option<bool> Mf16c({"-mf16c"},
                               "  -mf16c             \tThese switches enable the use of instructions in the mf16c.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MfastFp({"-mfast-fp"},
                               "  -mfast-fp             \tLink with the fast floating-point library. \n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MfastIndirectCalls({"-mfast-indirect-calls"},
                               "  -mfast-indirect-calls             \tGenerate code that assumes calls never "
                               "cross space boundaries.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MfastSwDiv({"-mfast-sw-div"},
                               "  -mfast-sw-div             \t\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-fast-sw-div"));

maplecl::Option<bool> MfasterStructs({"-mfaster-structs"},
                               "  -mfaster-structs             \tWith -mfaster-structs, the compiler assumes that "
                               "structures should have 8-byte alignment.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-faster-structs"));

maplecl::Option<bool> Mfdiv({"-mfdiv"},
                               "  -mfdiv             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mfdpic({"-mfdpic"},
                               "  -mfdpic             \tSelect the FDPIC ABI, which uses function descriptors to "
                               "represent pointers to functions.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mfentry({"-mfentry"},
                               "  -mfentry             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mfix({"-mfix"},
                               "  -mfix             \tGenerate code to use the optional FIX instruction sets.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-fix"));

maplecl::Option<bool> Mfix24k({"-mfix-24k"},
                               "  -mfix-24k             \tWork around the 24K E48 (lost data on stores during refill) "
                               "errata. \n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-fix-24k"));

maplecl::Option<bool> MfixAndContinue({"-mfix-and-continue"},
                               "  -mfix-and-continue             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MfixAt697f({"-mfix-at697f"},
                               "  -mfix-at697f             \tEnable the documented workaround for the single erratum "
                               "of the Atmel AT697F processor (which corresponds to erratum #13 of the "
                               "AT697E processor).\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MfixCortexA53835769({"-mfix-cortex-a53-835769"},
                               "  -mfix-cortex-a53-835769             \tWorkaround for ARM Cortex-A53 Erratum "
                               "number 835769.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-fix-cortex-a53-835769"));

maplecl::Option<bool> MfixCortexA53843419({"-mfix-cortex-a53-843419"},
                               "  -mfix-cortex-a53-843419             \tWorkaround for ARM Cortex-A53 Erratum "
                               "number 843419.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-fix-cortex-a53-843419"));

maplecl::Option<bool> MfixCortexM3Ldrd({"-mfix-cortex-m3-ldrd"},
                               "  -mfix-cortex-m3-ldrd             \tSome Cortex-M3 cores can cause data corruption "
                               "when ldrd instructions with overlapping destination and base registers are used. \n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MfixGr712rc({"-mfix-gr712rc"},
                               "  -mfix-gr712rc             \tEnable the documented workaround for the back-to-back "
                               "store errata of the GR712RC processor.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MfixR10000({"-mfix-r10000"},
                               "  -mfix-r10000             \tWork around certain R10000 errata\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-fix-r10000"));

maplecl::Option<bool> MfixR4000({"-mfix-r4000"},
                               "  -mfix-r4000             \tWork around certain R4000 CPU errata\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-fix-r4000"));

maplecl::Option<bool> MfixR4400({"-mfix-r4400"},
                               "  -mfix-r4400             \tWork around certain R4400 CPU errata\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-fix-r4400"));

maplecl::Option<bool> MfixRm7000({"-mfix-rm7000"},
                               "  -mfix-rm7000             \tWork around the RM7000 dmult/dmultu errata.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-mno-fix-rm7000"));

maplecl::Option<bool> MfixSb1({"-mfix-sb1"},
                               "  -mfix-sb1             \tWork around certain SB-1 CPU core errata.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-mno-fix-sb1"));

maplecl::Option<bool> MfixUt699({"-mfix-ut699"},
                               "  -mfix-ut699             \tEnable the documented workarounds for the floating-point "
                               "errata and the data cache nullify errata of the UT699 processor.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MfixUt700({"-mfix-ut700"},
                               "  -mfix-ut700             \tEnable the documented workaround for the back-to-back "
                               "store errata of the UT699E/UT700 processor.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MfixVr4120({"-mfix-vr4120"},
                               "  -mfix-vr4120             \tWork around certain VR4120 errata\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-mno-fix-vr4120"));

maplecl::Option<bool> MfixVr4130({"-mfix-vr4130"},
                               "  -mfix-vr4130             \tWork around the VR4130 mflo/mfhi errata.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MfixedCc({"-mfixed-cc"},
                               "  -mfixed-cc             \tDo not try to dynamically allocate condition code "
                               "registers, only use icc0 and fcc0.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<std::string> MfixedRange({"-mfixed-range"},
                               "  -mfixed-range             \tGenerate code treating the given register range "
                               "as fixed registers.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mflat({"-mflat"},
                               "  -mflat             \tWith -mflat, the compiler does not generate save/restore "
                               "instructions and uses a “flat” or single register window model.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-flat"));

maplecl::Option<bool> MflipMips16({"-mflip-mips16"},
                               "  -mflip-mips16             \tGenerate MIPS16 code on alternating functions.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MfloatAbi({"-mfloat-abi"},
                               "  -mfloat-abi             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<std::string> MfloatGprs({"-mfloat-gprs"},
                               "  -mfloat-gprs             \tThis switch enables the generation of floating-point "
                               "operations on the general-purpose registers for architectures that support it.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MfloatIeee({"-mfloat-ieee"},
                               "  -mfloat-ieee             \tGenerate code that does not use VAX F and G "
                               "floating-point arithmetic instead of IEEE single and double precision.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MfloatVax({"-mfloat-vax"},
                               "  -mfloat-vax             \tGenerate code that uses  VAX F and G "
                               "floating-point arithmetic instead of IEEE single and double precision.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mfloat128({"-mfloat128"},
                               "  -mfloat128             \tEisable the __float128 keyword for IEEE 128-bit floating "
                               "point and use either software emulation for IEEE 128-bit floating point or hardware "
                               "instructions.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-float128"));

maplecl::Option<bool> Mfloat128Hardware({"-mfloat128-hardware"},
                               "  -mfloat128-hardware             \tEisable using ISA 3.0 hardware instructions to "
                               "support the __float128 data type.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-float128-hardware"));

maplecl::Option<bool> Mfloat32({"-mfloat32"},
                               "  -mfloat32             \tUse 32-bit float.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-float32"));

maplecl::Option<bool> Mfloat64({"-mfloat64"},
                               "  -mfloat64             \tUse 64-bit float.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-float64"));

maplecl::Option<std::string> MflushFunc({"-mflush-func"},
                               "  -mflush-func             \tSpecifies the function to call to flush the I and D "
                               "caches, or to not call any such function. \n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-flush-func"));

maplecl::Option<std::string> MflushTrap({"-mflush-trap"},
                               "  -mflush-trap             \t\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-flush-trap"));

maplecl::Option<bool> Mfma({"-mfma"},
                               "  -mfma             \tThese switches enable the use of instructions in the mfma.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-fmaf"));

maplecl::Option<bool> Mfma4({"-mfma4"},
                               "  -mfma4             \tThese switches enable the use of instructions in the mfma4.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mfmaf({"-mfmaf"},
                               "  -mfmaf             \tWith -mfmaf, Maple generates code that takes advantage of the "
                               "UltraSPARC Fused Multiply-Add Floating-point instructions.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mfmovd({"-mfmovd"},
                               "  -mfmovd             \tEnable the use of the instruction fmovd. \n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MforceNoPic({"-mforce-no-pic"},
                               "  -mforce-no-pic             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MfpExceptions({"-mfp-exceptions"},
                               "  -mfp-exceptions             \tSpecifies whether FP exceptions are enabled. \n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-mno-fp-exceptions"));

maplecl::Option<bool> MfpMode({"-mfp-mode"},
                               "  -mfp-mode             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<std::string> MfpRoundingMode({"-mfp-rounding-mode"},
                               "  -mfp-rounding-mode             \tSelects the IEEE rounding mode.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<std::string> MfpTrapMode({"-mfp-trap-mode"},
                               "  -mfp-trap-mode             \tThis option controls what floating-point "
                               "related traps are enabled.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mfp16Format({"-mfp16-format"},
                               "  -mfp16-format             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mfp32({"-mfp32"},
                               "  -mfp32             \tAssume that floating-point registers are 32 bits wide.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mfp64({"-mfp64"},
                               "  -mfp64             \tAssume that floating-point registers are 64 bits wide.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mfpmath({"-mfpmath"},
                               "  -mfpmath             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mfpr32({"-mfpr-32"},
                               "  -mfpr-32             \tUse only the first 32 floating-point registers.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mfpr64({"-mfpr-64"},
                               "  -mfpr-64             \tUse all 64 floating-point registers.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mfprnd({"-mfprnd"},
                               "  -mfprnd             \tSpecify which instructions are available on the "
                               "processor you are using. \n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-fprnd"));

maplecl::Option<std::string> Mfpu({"-mfpu"},
                               "  -mfpu             \tEnables support for specific floating-point hardware "
                               "extensions for ARCv2 cores.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-fpu"));

maplecl::Option<bool> Mfpxx({"-mfpxx"},
                               "  -mfpxx             \tDo not assume the width of floating-point registers.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MfractConvertTruncate({"-mfract-convert-truncate"},
                               "  -mfract-convert-truncate             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MframeHeaderOpt({"-mframe-header-opt"},
                               "  -mframe-header-opt             \tEnable (disable) frame header optimization "
                               "in the o32 ABI. \n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-mno-frame-header-opt"));

maplecl::Option<bool> Mfriz({"-mfriz"},
                               "  -mfriz             \tGenerate the friz instruction when the "
                               "-funsafe-math-optimizations option is used to optimize rounding of floating-point "
                               "values to 64-bit integer and back to floating point. \n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mfsca({"-mfsca"},
                               "  -mfsca             \tAllow or disallow the compiler to emit the fsca instruction "
                               "for sine and cosine approximations.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-fsca"));

maplecl::Option<bool> Mfsgsbase({"-mfsgsbase"},
                               "  -mfsgsbase             \tThese switches enable the use of instructions in the "
                               "mfsgsbase.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mfsmuld({"-mfsmuld"},
                               "  -mfsmuld             \tWith -mfsmuld, Maple generates code that takes advantage of "
                               "the Floating-point Multiply Single to Double (FsMULd) instruction. \n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-fsmuld"));

maplecl::Option<bool> Mfsrra({"-mfsrra"},
                               "  -mfsrra             \tAllow or disallow the compiler to emit the fsrra instruction"
                               " for reciprocal square root approximations.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-fsrra"));

maplecl::Option<bool> MfullRegs({"-mfull-regs"},
                               "  -mfull-regs             \tUse full-set registers for register allocation.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MfullToc({"-mfull-toc"},
                               "  -mfull-toc             \tModify generation of the TOC (Table Of Contents), "
                               "which is created for every executable file. \n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MfusedMadd({"-mfused-madd"},
                               "  -mfused-madd             \tGenerate code that uses the floating-point multiply "
                               "and accumulate instructions. \n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-fused-madd"));

maplecl::Option<bool> Mfxsr({"-mfxsr"},
                               "  -mfxsr             \tThese switches enable the use of instructions in the mfxsr.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MG({"-MG"},
                               "  -MG             \tTreat missing header files as generated files.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mg10({"-mg10"},
                               "  -mg10             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mg13({"-mg13"},
                               "  -mg13             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mg14({"-mg14"},
                               "  -mg14             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mgas({"-mgas"},
                               "  -mgas             \tEnable the use of assembler directives only GAS understands.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MgccAbi({"-mgcc-abi"},
                               "  -mgcc-abi             \tEnables support for the old GCC version of the V850 ABI.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MgenCellMicrocode({"-mgen-cell-microcode"},
                               "  -mgen-cell-microcode             \tGenerate Cell microcode instructions.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MgeneralRegsOnly({"-mgeneral-regs-only"},
                               "  -mgeneral-regs-only             \tGenerate code which uses only the general "
                               "registers.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mghs({"-mghs"},
                               "  -mghs             \tEnables support for the RH850 version of the V850 ABI.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mglibc({"-mglibc"},
                               "  -mglibc             \tUse GNU C library.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mgnu({"-mgnu"},
                               "  -mgnu             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MgnuAs({"-mgnu-as"},
                               "  -mgnu-as             \t\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-gnu-as"));

maplecl::Option<bool> MgnuAttribute({"-mgnu-attribute"},
                               "  -mgnu-attribute             \tEmit .gnu_attribute assembly directives to set "
                               "tag/value pairs in a .gnu.attributes section that specify ABI variations in "
                               "function parameters or return values.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-gnu-attribute"));

maplecl::Option<bool> MgnuLd({"-mgnu-ld"},
                               "  -mgnu-ld             \tUse options specific to GNU ld.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-gnu-ld"));

maplecl::Option<bool> Mgomp({"-mgomp"},
                               "  -mgomp             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mgotplt({"-mgotplt"},
                               "  -mgotplt             \t\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-gotplt"));

maplecl::Option<bool> Mgp32({"-mgp32"},
                               "  -mgp32             \tAssume that general-purpose registers are 32 bits wide.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mgp64({"-mgp64"},
                               "  -mgp64             \tAssume that general-purpose registers are 64 bits wide.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mgpopt({"-mgpopt"},
                               "  -mgpopt             \tUse GP-relative accesses for symbols that are known "
                               "to be in a small data section\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-gpopt"));

maplecl::Option<bool> Mgpr32({"-mgpr-32"},
                               "  -mgpr-32             \tOnly use the first 32 general-purpose registers.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mgpr64({"-mgpr-64"},
                               "  -mgpr-64             \tUse all 64 general-purpose registers.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MgprelRo({"-mgprel-ro"},
                               "  -mgprel-ro             \tEnable the use of GPREL relocations in the FDPIC ABI "
                               "for data that is known to be in read-only sections.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mh({"-mh"},
                               "  -mh             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mhal({"-mhal"},
                               "  -mhal             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MhalfRegFile({"-mhalf-reg-file"},
                               "  -mhalf-reg-file             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MhardDfp({"-mhard-dfp"},
                               "  -mhard-dfp             \tUse the hardware decimal-floating-point instructions "
                               "for decimal-floating-point operations. \n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-hard-dfp"));

maplecl::Option<bool> MhardFloat({"-mhard-float"},
                               "  -mhard-float             \tUse the hardware floating-point instructions and "
                               "registers for floating-point operations.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MhardQuadFloat({"-mhard-quad-float"},
                               "  -mhard-quad-float             \tGenerate output containing quad-word (long double) "
                               "floating-point instructions.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mhardlit({"-mhardlit"},
                               "  -mhardlit             \t\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-hardlit"));

maplecl::Option<std::string> MhintMaxDistance({"-mhint-max-distance"},
                               "  -mhint-max-distance             \tThe encoding of the branch hint instruction "
                               "limits the hint to be within 256 instructions of the branch it is affecting.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<std::string> MhintMaxNops({"-mhint-max-nops"},
                               "  -mhint-max-nops             \tMaximum number of NOPs to insert for a branch hint.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<std::string> Mhotpatch({"-mhotpatch"},
                               "  -mhotpatch             \tIf the hotpatch option is enabled, a “hot-patching” "
                               "function prologue is generated for all functions in the compilation unit. \n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MhpLd({"-mhp-ld"},
                               "  -mhp-ld             \tUse options specific to HP ld.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mhtm({"-mhtm"},
                               "  -mhtm             \tThe -mhtm option enables a set of builtins making use of "
                               "instructions available with the transactional execution facility introduced with "
                               "the IBM zEnterprise EC12 machine generation S/390 System z Built-in Functions.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-htm"));

maplecl::Option<bool> MhwDiv({"-mhw-div"},
                               "  -mhw-div             \t\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-hw-div"));

maplecl::Option<bool> MhwMul({"-mhw-mul"},
                               "  -mhw-mul             \t\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-hw-mul"));

maplecl::Option<bool> MhwMulx({"-mhw-mulx"},
                               "  -mhw-mulx             \t\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-hw-mulx"));

maplecl::Option<std::string> MhwmultE({"-mhwmult="},
                               "  -mhwmult=             \tDescribes the type of hardware multiply "
                               "supported by the target.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Miamcu({"-miamcu"},
                               "  -miamcu             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Micplb({"-micplb"},
                               "  -micplb             \tAssume that ICPLBs are enabled at run time.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MidSharedLibrary({"-mid-shared-library"},
                               "  -mid-shared-library             \tGenerate code that supports shared libraries "
                               "via the library ID method.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-id-shared-library"));

maplecl::Option<bool> Mieee({"-mieee"},
                               "  -mieee             \tThis option generates code fully IEEE-compliant code except"
                               " that the inexact-flag is not maintained (see below).\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-ieee"));

maplecl::Option<bool> MieeeConformant({"-mieee-conformant"},
                               "  -mieee-conformant             \tThis option marks the generated code as IEEE "
                               "conformant. \n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MieeeFp({"-mieee-fp"},
                               "  -mieee-fp             \t\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-ieee-fp"));

maplecl::Option<bool> MieeeWithInexact({"-mieee-with-inexact"},
                               "  -mieee-with-inexact             \tTurning on this option causes the generated "
                               "code to implement fully-compliant IEEE math. \n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Milp32({"-milp32"},
                               "  -milp32             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mimadd({"-mimadd"},
                               "  -mimadd             \tEnable (disable) use of the madd and msub integer "
                               "instructions.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-imadd"));

maplecl::Option<bool> MimpureText({"-mimpure-text"},
                               "  -mimpure-text             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MincomingStackBoundary({"-mincoming-stack-boundary"},
                               "  -mincoming-stack-boundary             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MindexedLoads({"-mindexed-loads"},
                               "  -mindexed-loads             \tEnable the use of indexed loads. \n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MinlineAllStringops({"-minline-all-stringops"},
                               "  -minline-all-stringops             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MinlineFloatDivideMaxThroughput({"-minline-float-divide-max-throughput"},
                               "  -minline-float-divide-max-throughput             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MinlineFloatDivideMinLatency({"-minline-float-divide-min-latency"},
                               "  -minline-float-divide-min-latency             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MinlineIc_invalidate({"-minline-ic_invalidate"},
                               "  -minline-ic_invalidate             \tInline code to invalidate instruction cache "
                               "entries after setting up nested function trampolines.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MinlineIntDivideMaxThroughput({"-minline-int-divide-max-throughput"},
                               "  -minline-int-divide-max-throughput             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MinlineIntDivideMinLatency({"-minline-int-divide-min-latency"},
                               "  -minline-int-divide-min-latency             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MinlinePlt({"-minline-plt"},
                               "  -minline-plt             \tEnable inlining of PLT entries in function calls to "
                               "functions that are not known to bind locally.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MinlineSqrtMaxThroughput({"-minline-sqrt-max-throughput"},
                               "  -minline-sqrt-max-throughput             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MinlineSqrtMinLatency({"-minline-sqrt-min-latency"},
                               "  -minline-sqrt-min-latency             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MinlineStringopsDynamically({"-minline-stringops-dynamically"},
                               "  -minline-stringops-dynamically             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Minrt({"-minrt"},
                               "  -minrt             \tEnable the use of a minimum runtime environment - no "
                               "static initializers or constructors.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<std::string> MinsertSchedNops({"-minsert-sched-nops"},
                               "  -minsert-sched-nops             \tThis option controls which NOP insertion "
                               "scheme is used during the second scheduling pass.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MintRegister({"-mint-register"},
                               "  -mint-register             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mint16({"-mint16"},
                               "  -mint16             \tUse 16-bit int.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-int16"));

maplecl::Option<bool> Mint32({"-mint32"},
                               "  -mint32             \tUse 32-bit int.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-int32"));

maplecl::Option<bool> Mint8({"-mint8"},
                               "  -mint8             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MinterlinkCompressed({"-minterlink-compressed"},
                               "  -minterlink-compressed             \tRequire that code using the standard "
                               "(uncompressed) MIPS ISA be link-compatible with MIPS16 and microMIPS code, "
                               "and vice versa.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-interlink-compressed"));

maplecl::Option<bool> MinterlinkMips16({"-minterlink-mips16"},
                               "  -minterlink-mips16             \tPredate the microMIPS ASE and are retained for "
                               "backwards compatibility.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-interlink-mips16"));

maplecl::Option<bool> MioVolatile({"-mio-volatile"},
                               "  -mio-volatile             \tTells the compiler that any variable marked with the "
                               "io attribute is to be considered volatile.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mips1({"-mips1"},
                               "  -mips1             \tEquivalent to -march=mips1.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mips16({"-mips16"},
                               "  -mips16             \tGenerate MIPS16 code.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-mips16"));

maplecl::Option<bool> Mips2({"-mips2"},
                               "  -mips2             \tEquivalent to -march=mips2.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mips3({"-mips3"},
                               "  -mips3             \tEquivalent to -march=mips3.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mips32({"-mips32"},
                               "  -mips32             \tEquivalent to -march=mips32.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mips32r3({"-mips32r3"},
                               "  -mips32r3             \tEquivalent to -march=mips32r3.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mips32r5({"-mips32r5"},
                               "  -mips32r5             \tEquivalent to -march=mips32r5.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mips32r6({"-mips32r6"},
                               "  -mips32r6             \tEquivalent to -march=mips32r6.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mips3d({"-mips3d"},
                               "  -mips3d             \tUse (do not use) the MIPS-3D ASE.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-mips3d"));

maplecl::Option<bool> Mips4({"-mips4"},
                               "  -mips4             \tEquivalent to -march=mips4.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mips64({"-mips64"},
                               "  -mips64             \tEquivalent to -march=mips64.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mips64r2({"-mips64r2"},
                               "  -mips64r2             \tEquivalent to -march=mips64r2.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mips64r3({"-mips64r3"},
                               "  -mips64r3             \tEquivalent to -march=mips64r3.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mips64r5({"-mips64r5"},
                               "  -mips64r5             \tEquivalent to -march=mips64r5.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mips64r6({"-mips64r6"},
                               "  -mips64r6             \tEquivalent to -march=mips64r6.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<std::string> MiselE({"-misel="},
                               "  -misel=             \tThis switch has been deprecated. "
                               "Use -misel and -mno-isel instead.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<std::string> Misel({"-misel"},
                               "  -misel             \tThis switch enables or disables the generation of "
                               "ISEL instructions.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-isel"));

maplecl::Option<bool> Misize({"-misize"},
                               "  -misize             \tAnnotate assembler instructions with estimated addresses.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<std::string> MisrVectorSize({"-misr-vector-size"},
                               "  -misr-vector-size             \tSpecify the size of each interrupt vector, which "
                               "must be 4 or 16.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MissueRateNumber({"-missue-rate=number"},
                               "  -missue-rate=number             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mivc2({"-mivc2"},
                               "  -mivc2             \tEnables IVC2 scheduling. IVC2 is a 64-bit VLIW coprocessor.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mjsr({"-mjsr"},
                               "  -mjsr             \t\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-jsr"));

maplecl::Option<bool> MjumpInDelay({"-mjump-in-delay"},
                               "  -mjump-in-delay             \tThis option is ignored and provided for compatibility "
                               "purposes only.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mkernel({"-mkernel"},
                               "  -mkernel             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mknuthdiv({"-mknuthdiv"},
                               "  -mknuthdiv             \tMake the result of a division yielding a remainder "
                               "have the same sign as the divisor. \n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-knuthdiv"));

maplecl::Option<bool> Ml({"-ml"},
                               "  -ml             \tCauses variables to be assigned to the .far section by default.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mlarge({"-mlarge"},
                               "  -mlarge             \tUse large-model addressing (20-bit pointers, 32-bit size_t).\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MlargeData({"-mlarge-data"},
                               "  -mlarge-data             \tWith this option the data area is limited to just "
                               "below 2GB.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MlargeDataThreshold({"-mlarge-data-threshold"},
                               "  -mlarge-data-threshold             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MlargeMem({"-mlarge-mem"},
                               "  -mlarge-mem             \tWith -mlarge-mem code is generated that assumes a "
                               "full 32-bit address.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MlargeText({"-mlarge-text"},
                               "  -mlarge-text             \tWhen -msmall-data is used, the compiler can assume that "
                               "all local symbols share the same $gp value, and thus reduce the number of instructions"
                               " required for a function call from 4 to 1.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mleadz({"-mleadz"},
                               "  -mleadz             \tnables the leadz (leading zero) instruction.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MleafIdSharedLibrary({"-mleaf-id-shared-library"},
                               "  -mleaf-id-shared-library             \tenerate code that supports shared libraries "
                               "via the library ID method, but assumes that this library or executable won't link "
                               "against any other ID shared libraries.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-leaf-id-shared-library"));

maplecl::Option<bool> Mlibfuncs({"-mlibfuncs"},
                               "  -mlibfuncs             \tSpecify that intrinsic library functions are being "
                               "compiled, passing all values in registers, no matter the size.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-libfuncs"));

maplecl::Option<bool> MlibraryPic({"-mlibrary-pic"},
                               "  -mlibrary-pic             \tGenerate position-independent EABI code.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MlinkedFp({"-mlinked-fp"},
                               "  -mlinked-fp             \tFollow the EABI requirement of always creating a "
                               "frame pointer whenever a stack frame is allocated.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MlinkerOpt({"-mlinker-opt"},
                               "  -mlinker-opt             \tEnable the optimization pass in the HP-UX linker.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mlinux({"-mlinux"},
                               "  -mlinux             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mlittle({"-mlittle"},
                               "  -mlittle             \tAssume target CPU is configured as little endian.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MlittleEndian({"-mlittle-endian"},
                               "  -mlittle-endian             \tAssume target CPU is configured as little endian.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MlittleEndianData({"-mlittle-endian-data"},
                               "  -mlittle-endian-data             \tStore data (but not code) in the big-endian "
                               "format.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mliw({"-mliw"},
                               "  -mliw             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mll64({"-mll64"},
                               "  -mll64             \tEnable double load/store operations for ARC HS cores.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mllsc({"-mllsc"},
                               "  -mllsc             \tUse (do not use) 'll', 'sc', and 'sync' instructions to "
                               "implement atomic memory built-in functions.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-llsc"));

maplecl::Option<bool> MloadStorePairs({"-mload-store-pairs"},
                               "  -mload-store-pairs             \tEnable (disable) an optimization that pairs "
                               "consecutive load or store instructions to enable load/store bonding. \n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-load-store-pairs"));

maplecl::Option<bool> MlocalSdata({"-mlocal-sdata"},
                               "  -mlocal-sdata             \tExtend (do not extend) the -G behavior to local data "
                               "too, such as to static variables in C. \n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-local-sdata"));

maplecl::Option<bool> Mlock({"-mlock"},
                               "  -mlock             \tPassed down to the assembler to enable the locked "
                               "load/store conditional extension. \n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MlongCalls({"-mlong-calls"},
                               "  -mlong-calls             \tGenerate calls as register indirect calls, thus "
                               "providing access to the full 32-bit address range.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-long-calls"));

maplecl::Option<bool> MlongDouble128({"-mlong-double-128"},
                               "  -mlong-double-128             \tControl the size of long double type. \n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MlongDouble64({"-mlong-double-64"},
                               "  -mlong-double-64             \tControl the size of long double type.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MlongDouble80({"-mlong-double-80"},
                               "  -mlong-double-80             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MlongJumpTableOffsets({"-mlong-jump-table-offsets"},
                               "  -mlong-jump-table-offsets             \tUse 32-bit offsets in switch tables. \n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MlongJumps({"-mlong-jumps"},
                               "  -mlong-jumps             \tDisable (or re-enable) the generation of PC-relative "
                               "jump instructions.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-long-jumps"));

maplecl::Option<bool> MlongLoadStore({"-mlong-load-store"},
                               "  -mlong-load-store             \tGenerate 3-instruction load and store "
                               "sequences as sometimes required by the HP-UX 10 linker.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mlong32({"-mlong32"},
                               "  -mlong32             \tForce long, int, and pointer types to be 32 bits wide.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mlong64({"-mlong64"},
                               "  -mlong64             \tForce long types to be 64 bits wide. \n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mlongcall({"-mlongcall"},
                               "  -mlongcall             \tBy default assume that all calls are far away so that a "
                               "longer and more expensive calling sequence is required. \n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-longcall"));

maplecl::Option<bool> Mlongcalls({"-mlongcalls"},
                               "  -mlongcalls             \t\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-longcalls"));

maplecl::Option<bool> Mloop({"-mloop"},
                               "  -mloop             \tEnables the use of the e3v5 LOOP instruction. \n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mlow64k({"-mlow-64k"},
                               "  -mlow-64k             \tWhen enabled, the compiler is free to take advantage of "
                               "the knowledge that the entire program fits into the low 64k of memory.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-low-64k"));

maplecl::Option<bool> MlowPrecisionRecipSqrt({"-mlow-precision-recip-sqrt"},
                               "  -mlow-precision-recip-sqrt             \tEnable the reciprocal square root "
                               "approximation.  Enabling this reduces precision of reciprocal square root results "
                               "to about 16 bits for single precision and to 32 bits for double precision.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-low-precision-recip-sqrt"));

maplecl::Option<bool> Mlp64({"-mlp64"},
                               "  -mlp64             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mlra({"-mlra"},
                               "  -mlra             \tEnable Local Register Allocation. By default the port uses "
                               "LRA. (i.e. -mno-lra).\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-lra"));

maplecl::Option<bool> MlraPriorityCompact({"-mlra-priority-compact"},
                               "  -mlra-priority-compact             \tIndicate target register priority for "
                               "r0..r3 / r12..r15.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MlraPriorityNoncompact({"-mlra-priority-noncompact"},
                               "  -mlra-priority-noncompact             \tReduce target register priority for "
                               "r0..r3 / r12..r15.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MlraPriorityNone({"-mlra-priority-none"},
                               "  -mlra-priority-none             \tDon't indicate any priority for target "
                               "registers.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mlwp({"-mlwp"},
                               "  -mlwp             \tThese switches enable the use of instructions in the mlwp.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mlxc1Sxc1({"-mlxc1-sxc1"},
                               "  -mlxc1-sxc1             \tWhen applicable, enable (disable) the "
                               "generation of lwxc1, swxc1, ldxc1, sdxc1 instructions.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mlzcnt({"-mlzcnt"},
                               "  -mlzcnt             \these switches enable the use of instructions in the mlzcnt\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MM({"-MM"},
                               "  -MM             \tLike -M but ignore system header files.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mm({"-Mm"},
                               "  -Mm             \tCauses variables to be assigned to the .near section by default.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mmac({"-mmac"},
                               "  -mmac             \tEnable the use of multiply-accumulate instructions.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mmac24({"-mmac-24"},
                               "  -mmac-24             \tPassed down to the assembler. \n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MmacD16({"-mmac-d16"},
                               "  -mmac-d16             \tPassed down to the assembler.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mmac_24({"-mmac_24"},
                               "  -mmac_24             \tReplaced by -mmac-24.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mmac_d16({"-mmac_d16"},
                               "  -mmac_d16             \tReplaced by -mmac-d16.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mmad({"-mmad"},
                               "  -mmad             \tEnable (disable) use of the mad, madu and mul instructions, "
                               "as provided by the R4650 ISA.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-mad"));

maplecl::Option<bool> Mmadd4({"-mmadd4"},
                               "  -mmadd4             \tWhen applicable, enable (disable) the generation of "
                               "4-operand madd.s, madd.d and related instructions.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mmainkernel({"-mmainkernel"},
                               "  -mmainkernel             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mmalloc64({"-mmalloc64"},
                               "  -mmalloc64             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mmax({"-mmax"},
                               "  -mmax             \tGenerate code to use MAX instruction sets.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-max"));

maplecl::Option<bool> MmaxConstantSize({"-mmax-constant-size"},
                               "  -mmax-constant-size             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MmaxStackFrame({"-mmax-stack-frame"},
                               "  -mmax-stack-frame             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MmcountRaAddress({"-mmcount-ra-address"},
                               "  -mmcount-ra-address             \tEmit (do not emit) code that allows _mcount "
                               "to modify the calling function’s return address.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-mcount-ra-address"));

maplecl::Option<bool> Mmcu({"-mmcu"},
                               "  -mmcu             \tUse the MIPS MCU ASE instructions.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-mcu"));

maplecl::Option<std::string> MmcuE({"-mmcu="},
                               "  -mmcu=             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MMD({"-MMD"},
                               "  -MMD             \tLike -MD but ignore system header files.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mmedia({"-mmedia"},
                               "  -mmedia             \tUse media instructions.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-media"));

maplecl::Option<bool> MmediumCalls({"-mmedium-calls"},
                               "  -mmedium-calls             \tDon’t use less than 25-bit addressing range for calls,"
                               " which is the offset available for an unconditional branch-and-link instruction. \n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mmemcpy({"-mmemcpy"},
                               "  -mmemcpy             \tForce (do not force) the use of memcpy for non-trivial "
                               "block moves.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-memcpy"));

maplecl::Option<bool> MmemcpyStrategyStrategy({"-mmemcpy-strategy=strategy"},
                               "  -mmemcpy-strategy=strategy             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<std::string> MmemoryLatency({"-mmemory-latency"},
                               "  -mmemory-latency             \tSets the latency the scheduler should assume for "
                               "typical memory references as seen by the application.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<std::string> MmemoryModel({"-mmemory-model"},
                               "  -mmemory-model             \tSet the memory model in force on the processor to "
                               "one of.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MmemsetStrategyStrategy({"-mmemset-strategy=strategy"},
                               "  -mmemset-strategy=strategy             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mmfcrf({"-mmfcrf"},
                               "  -mmfcrf             \tSpecify which instructions are available on "
                               "the processor you are using. \n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-mfcrf"));

maplecl::Option<bool> Mmfpgpr({"-mmfpgpr"},
                               "  -mmfpgpr             \tSpecify which instructions are available on the "
                               "processor you are using.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-mfpgpr"));

maplecl::Option<bool> Mmicromips({"-mmicromips"},
                               "  -mmicromips             \tGenerate microMIPS code.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-mmicromips"));

maplecl::Option<bool> MminimalToc({"-mminimal-toc"},
                               "  -mminimal-toc             \tModify generation of the TOC (Table Of Contents), which "
                               "is created for every executable file. \n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mminmax({"-mminmax"},
                               "  -mminmax             \tEnables the min and max instructions.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MmitigateRop({"-mmitigate-rop"},
                               "  -mmitigate-rop             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MmixedCode({"-mmixed-code"},
                               "  -mmixed-code             \tTweak register allocation to help 16-bit instruction"
                               " generation.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mmmx({"-mmmx"},
                               "  -mmmx             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MmodelLarge({"-mmodel=large"},
                               "  -mmodel=large             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MmodelMedium({"-mmodel=medium"},
                               "  -mmodel=medium             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MmodelSmall({"-mmodel=small"},
                               "  -mmodel=small             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mmovbe({"-mmovbe"},
                               "  -mmovbe             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mmpx({"-mmpx"},
                               "  -mmpx             \tThese switches enable the use of instructions in the mmpx.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MmpyOption({"-mmpy-option"},
                               "  -mmpy-option             \tCompile ARCv2 code with a multiplier design option. \n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MmsBitfields({"-mms-bitfields"},
                               "  -mms-bitfields             \t\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-ms-bitfields"));

maplecl::Option<bool> Mmt({"-mmt"},
                               "  -mmt             \tUse MT Multithreading instructions.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-mt"));

maplecl::Option<bool> Mmul({"-mmul"},
                               "  -mmul             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MmulBugWorkaround({"-mmul-bug-workaround"},
                               "  -mmul-bug-workaround             \t\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-mul-bug-workaround"));

maplecl::Option<bool> Mmulx({"-mmul.x"},
                               "  -mmul.x             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mmul32x16({"-mmul32x16"},
                               "  -mmul32x16             \tGenerate 32x16-bit multiply and multiply-accumulate "
                               "instructions.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mmul64({"-mmul64"},
                               "  -mmul64             \tGenerate mul64 and mulu64 instructions. "
                               "Only valid for -mcpu=ARC600.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mmuladd({"-mmuladd"},
                               "  -mmuladd             \tUse multiply and add/subtract instructions.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-muladd"));

maplecl::Option<bool> Mmulhw({"-mmulhw"},
                               "  -mmulhw             \tGenerate code that uses the half-word multiply and "
                               "multiply-accumulate instructions on the IBM 405, 440, 464 and 476 processors.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-mulhw"));

maplecl::Option<bool> Mmult({"-mmult"},
                               "  -mmult             \tEnables the multiplication and multiply-accumulate "
                               "instructions.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MmultBug({"-mmult-bug"},
                               "  -mmult-bug             \t\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-mult-bug"));

maplecl::Option<bool> Mmultcost({"-mmultcost"},
                               "  -mmultcost             \tCost to assume for a multiply instruction, "
                               "with '4' being equal to a normal instruction.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MmultiCondExec({"-mmulti-cond-exec"},
                               "  -mmulti-cond-exec             \tEnable optimization of && and || in conditional "
                               "execution (default).\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-multi-cond-exec"));

maplecl::Option<bool> Mmulticore({"-mmulticore"},
                               "  -mmulticore             \tBuild a standalone application for multicore "
                               "Blackfin processors. \n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mmultiple({"-mmultiple"},
                               "  -mmultiple             \tGenerate code that uses (does not use) the load multiple "
                               "word instructions and the store multiple word instructions. \n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-multiple"));

maplecl::Option<bool> Mmusl({"-mmusl"},
                               "  -mmusl             \tUse musl C library.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mmvcle({"-mmvcle"},
                               "  -mmvcle             \tGenerate code using the mvcle instruction to perform "
                               "block moves.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-mvcle"));

maplecl::Option<bool> Mmvme({"-mmvme"},
                               "  -mmvme             \tOn embedded PowerPC systems, assume that the startup module "
                               "is called crt0.o and the standard C libraries are libmvme.a and libc.a.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mmwaitx({"-mmwaitx"},
                               "  -mmwaitx             \tThese switches enable the use of instructions in "
                               "the mmwaitx.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mn({"-mn"},
                               "  -mn             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MnFlash({"-mn-flash"},
                               "  -mn-flash             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mnan2008({"-mnan=2008"},
                               "  -mnan=2008             \tControl the encoding of the special not-a-number "
                               "(NaN) IEEE 754 floating-point data.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MnanLegacy({"-mnan=legacy"},
                               "  -mnan=legacy             \tControl the encoding of the special not-a-number "
                               "(NaN) IEEE 754 floating-point data.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MneonFor64bits({"-mneon-for-64bits"},
                               "  -mneon-for-64bits             \tEnables using Neon to handle scalar 64-bits "
                               "operations.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MnestedCondExec({"-mnested-cond-exec"},
                               "  -mnested-cond-exec             \tEnable nested conditional execution "
                               "optimizations (default).\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-nested-cond-exec"));

maplecl::Option<bool> Mnhwloop({"-mnhwloop"},
                               "  -mnhwloop             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MnoAlignStringops({"-mno-align-stringops"},
                               "  -mno-align-stringops             \tDo not align the destination of inlined "
                               "string operations.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MnoBrcc({"-mno-brcc"},
                               "  -mno-brcc             \tThis option disables a target-specific pass in arc_reorg "
                               "to generate compare-and-branch (brcc) instructions. \n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MnoClearbss({"-mno-clearbss"},
                               "  -mno-clearbss             \tThis option is deprecated. \n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MnoCrt0({"-mno-crt0"},
                               "  -mno-crt0             \tDo not link in the C run-time initialization object file.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MnoDefault({"-mno-default"},
                               "  -mno-default             \tThis option instructs Maple to turn off all tunable "
                               "features.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MnoDpfpLrsr({"-mno-dpfp-lrsr"},
                               "  -mno-dpfp-lrsr             \tDisable lr and sr instructions from using FPX "
                               "extension aux registers.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MnoEflags({"-mno-eflags"},
                               "  -mno-eflags             \tDo not mark ABI switches in e_flags.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MnoFancyMath387({"-mno-fancy-math-387"},
                               "  -mno-fancy-math-387             \tSome 387 emulators do not support the sin, cos "
                               "and sqrt instructions for the 387. Specify this option to avoid generating those "
                               "instructions. \n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MnoFloat({"-mno-float"},
                               "  -mno-float             \tEquivalent to -msoft-float, but additionally asserts that "
                               "the program being compiled does not perform any floating-point operations. \n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MnoFpInToc({"-mno-fp-in-toc"},
                               "  -mno-fp-in-toc             \tModify generation of the TOC (Table Of Contents), "
                               "which is created for every executable file.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MFpReg({"-mfp-reg"},
                               "  -mfp-reg             \tGenerate code that uses (does not use) the "
                               "floating-point register set. \n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-fp-regs"));

maplecl::Option<bool> MnoFpRetIn387({"-mno-fp-ret-in-387"},
                               "  -mno-fp-ret-in-387             \tDo not use the FPU registers for return values of "
                               "functions.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MnoInlineFloatDivide({"-mno-inline-float-divide"},
                               "  -mno-inline-float-divide             \tDo not generate inline code for divides of "
                               "floating-point values.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MnoInlineIntDivide({"-mno-inline-int-divide"},
                               "  -mno-inline-int-divide             \tDo not generate inline code for divides of "
                               "integer values.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MnoInlineSqrt({"-mno-inline-sqrt"},
                               "  -mno-inline-sqrt             \tDo not generate inline code for sqrt.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MnoInterrupts({"-mno-interrupts"},
                               "  -mno-interrupts             \tGenerated code is not compatible with hardware "
                               "interrupts.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MnoLsim({"-mno-lsim"},
                               "  -mno-lsim             \tAssume that runtime support has been provided and so there "
                               "is no need to include the simulator library (libsim.a) on the linker command line.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MnoMillicode({"-mno-millicode"},
                               "  -mno-millicode             \tWhen optimizing for size (using -Os), prologues and "
                               "epilogues that have to save or restore a large number of registers are often "
                               "shortened by using call to a special function in libgcc\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MnoMpy({"-mno-mpy"},
                               "  -mno-mpy             \tDo not generate mpy-family instructions for ARC700. \n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MnoOpts({"-mno-opts"},
                               "  -mno-opts             \tDisables all the optional instructions enabled "
                               "by -mall-opts.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MnoPic({"-mno-pic"},
                               "  -mno-pic             \tGenerate code that does not use a global pointer register. \n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MnoPostinc({"-mno-postinc"},
                               "  -mno-postinc             \tCode generation tweaks that disable, respectively, "
                               "splitting of 32-bit loads, generation of post-increment addresses, and generation "
                               "of post-modify addresses.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MnoPostmodify({"-mno-postmodify"},
                               "  -mno-postmodify             \tCode generation tweaks that disable, respectively, "
                               "splitting of 32-bit loads, generation of post-increment addresses, and generation "
                               "of post-modify addresses. \n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MnoRedZone({"-mno-red-zone"},
                               "  -mno-red-zone             \tDo not use a so-called “red zone” for x86-64 code.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MnoRoundNearest({"-mno-round-nearest"},
                               "  -mno-round-nearest             \tMake the scheduler assume that the rounding "
                               "mode has been set to truncating.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MnoSchedProlog({"-mno-sched-prolog"},
                               "  -mno-sched-prolog             \tPrevent the reordering of instructions in the "
                               "function prologue, or the merging of those instruction with the instructions in the "
                               "function's body. \n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MnoSideEffects({"-mno-side-effects"},
                               "  -mno-side-effects             \tDo not emit instructions with side effects in "
                               "addressing modes other than post-increment.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MnoSoftCmpsf({"-mno-soft-cmpsf"},
                               "  -mno-soft-cmpsf             \tFor single-precision floating-point comparisons, emit"
                               " an fsub instruction and test the flags. \n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MnoSpaceRegs({"-mno-space-regs"},
                               "  -mno-space-regs             \tGenerate code that assumes the target has no "
                               "space registers.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MSpe({"-mspe"},
                               "  -mspe             \tThis switch enables the generation of SPE simd instructions.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-mno-spe"));

maplecl::Option<bool> MnoSumInToc({"-mno-sum-in-toc"},
                               "  -mno-sum-in-toc             \tModify generation of the TOC (Table Of Contents), "
                               "which is created for every executable file. \n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MnoVectDouble({"-mnovect-double"},
                               "  -mno-vect-double             \tChange the preferred SIMD mode to SImode. \n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mnobitfield({"-mnobitfield"},
                               "  -mnobitfield             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mnodiv({"-mnodiv"},
                               "  -mnodiv             \tDo not use div and mod instructions.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mnoliw({"-mnoliw"},
                               "  -mnoliw             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mnomacsave({"-mnomacsave"},
                               "  -mnomacsave             \tMark the MAC register as call-clobbered, even if "
                               "-mrenesas is given.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MnopFunDllimport({"-mnop-fun-dllimport"},
                               "  -mnop-fun-dllimport             \tThis option is available for Cygwin and "
                               "MinGW targets.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MnopMcount({"-mnop-mcount"},
                               "  -mnop-mcount             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mnops({"-mnops"},
                               "  -mnops             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mnorm({"-mnorm"},
                               "  -mnorm             \tGenerate norm instructions. \n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mnosetlb({"-mnosetlb"},
                               "  -mnosetlb             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MnosplitLohi({"-mnosplit-lohi"},
                               "  -mnosplit-lohi             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> ModdSpreg({"-modd-spreg"},
                               "  -modd-spreg             \tEnable the use of odd-numbered single-precision "
                               "floating-point registers for the o32 ABI.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-odd-spreg"));

maplecl::Option<bool> MomitLeafFramePointer({"-momit-leaf-frame-pointer"},
                               "  -momit-leaf-frame-pointer             \tOmit the frame pointer in leaf functions.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-omit-leaf-frame-pointer"));

maplecl::Option<bool> MoneByteBool({"-mone-byte-bool"},
                               "  -mone-byte-bool             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Moptimize({"-moptimize"},
                               "  -moptimize             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MoptimizeMembar({"-moptimize-membar"},
                               "  -moptimize-membar             \tThis switch removes redundant membar instructions "
                               "from the compiler-generated code. \n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-optimize-membar"));

maplecl::Option<std::string> MoverrideE({"-moverride="},
                               "  -moverride=             \tPower users only! Override CPU optimization parameters.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MP({"-MP"},
                               "  -MP             \tGenerate phony targets for all headers.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MpaRisc10({"-mpa-risc-1-0"},
                               "  -mpa-risc-1-0             \tSynonyms for -march=1.0 respectively.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MpaRisc11({"-mpa-risc-1-1"},
                               "  -mpa-risc-1-1             \tSynonyms for -march=1.1 respectively.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MpaRisc20({"-mpa-risc-2-0"},
                               "  -mpa-risc-2-0             \tSynonyms for -march=2.0 respectively.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mpack({"-mpack"},
                               "  -mpack             \tPack VLIW instructions.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-pack"));

maplecl::Option<bool> MpackedStack({"-mpacked-stack"},
                               "  -mpacked-stack             \tUse the packed stack layout. \n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-packed-stack"));

maplecl::Option<bool> Mpadstruct({"-mpadstruct"},
                               "  -mpadstruct             \tThis option is deprecated.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mpaired({"-mpaired"},
                               "  -mpaired             \tThis switch enables or disables the generation of PAIRED "
                               "simd instructions.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-paired"));

maplecl::Option<bool> MpairedSingle({"-mpaired-single"},
                               "  -mpaired-single             \tUse paired-single floating-point instructions.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-paired-single"));

maplecl::Option<bool> MpcRelativeLiteralLoads({"-mpc-relative-literal-loads"},
                               "  -mpc-relative-literal-loads             \tPC relative literal loads.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mpc32({"-mpc32"},
                               "  -mpc32             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mpc64({"-mpc64"},
                               "  -mpc64             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mpc80({"-mpc80"},
                               "  -mpc80             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mpclmul({"-mpclmul"},
                               "  -mpclmul             \tThese switches enable the use of instructions in "
                               "the mpclmul.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mpcrel({"-mpcrel"},
                               "  -mpcrel             \tUse the pc-relative addressing mode of the 68000 directly, "
                               "instead of using a global offset table.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mpdebug({"-mpdebug"},
                               "  -mpdebug             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mpe({"-mpe"},
                               "  -mpe             \tSupport IBM RS/6000 SP Parallel Environment (PE). \n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MpeAlignedCommons({"-mpe-aligned-commons"},
                               "  -mpe-aligned-commons             \tThis option is available for Cygwin and "
                               "MinGW targets.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MperfExt({"-mperf-ext"},
                               "  -mperf-ext             \tGenerate performance extension instructions.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-perf-ext"));

maplecl::Option<bool> MpicDataIsTextRelative({"-mpic-data-is-text-relative"},
                               "  -mpic-data-is-text-relative             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MpicRegister({"-mpic-register"},
                               "  -mpic-register             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mpid({"-mpid"},
                               "  -mpid             \t\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-pid"));

maplecl::Option<bool> Mpku({"-mpku"},
                               "  -mpku             \tThese switches enable the use of instructions in the mpku.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MpointerSizeSize({"-mpointer-size=size"},
                               "  -mpointer-size=size             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MpointersToNestedFunctions({"-mpointers-to-nested-functions"},
                               "  -mpointers-to-nested-functions             \tGenerate (do not generate) code to load"
                               " up the static chain register (r11) when calling through a pointer on AIX and 64-bit "
                               "Linux systems where a function pointer points to a 3-word descriptor giving the "
                               "function address, TOC value to be loaded in register r2, and static chain value to "
                               "be loaded in register r11. \n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MpokeFunctionName({"-mpoke-function-name"},
                               "  -mpoke-function-name             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mpopc({"-mpopc"},
                               "  -mpopc             \tWith -mpopc, Maple generates code that takes advantage of "
                               "the UltraSPARC Population Count instruction.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-popc"));

maplecl::Option<bool> Mpopcnt({"-mpopcnt"},
                               "  -mpopcnt             \tThese switches enable the use of instructions in the "
                               "mpopcnt.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mpopcntb({"-mpopcntb"},
                               "  -mpopcntb             \tSpecify which instructions are available on "
                               "the processor you are using. \n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-popcntb"));

maplecl::Option<bool> Mpopcntd({"-mpopcntd"},
                               "  -mpopcntd             \tSpecify which instructions are available on "
                               "the processor you are using. \n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-popcntd"));

maplecl::Option<bool> MportableRuntime({"-mportable-runtime"},
                               "  -mportable-runtime             \tUse the portable calling conventions proposed by "
                               "HP for ELF systems.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mpower8Fusion({"-mpower8-fusion"},
                               "  -mpower8-fusion             \tGenerate code that keeps some integer operations "
                               "adjacent so that the instructions can be fused together on power8 and later "
                               "processors.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-power8-fusion"));

maplecl::Option<bool> Mpower8Vector({"-mpower8-vector"},
                               "  -mpower8-vector             \tGenerate code that uses the vector and scalar "
                               "instructions that were added in version 2.07 of the PowerPC ISA.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-power8-vector"));

maplecl::Option<bool> MpowerpcGfxopt({"-mpowerpc-gfxopt"},
                               "  -mpowerpc-gfxopt             \tSpecify which instructions are available on the"
                               " processor you are using\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-powerpc-gfxopt"));

maplecl::Option<bool> MpowerpcGpopt({"-mpowerpc-gpopt"},
                               "  -mpowerpc-gpopt             \tSpecify which instructions are available on the"
                               " processor you are using\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-powerpc-gpopt"));

maplecl::Option<bool> Mpowerpc64({"-mpowerpc64"},
                               "  -mpowerpc64             \tSpecify which instructions are available on the"
                               " processor you are using\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-powerpc64"));

maplecl::Option<bool> MpreferAvx128({"-mprefer-avx128"},
                               "  -mprefer-avx128             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MpreferShortInsnRegs({"-mprefer-short-insn-regs"},
                               "  -mprefer-short-insn-regs             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mprefergot({"-mprefergot"},
                               "  -mprefergot             \tWhen generating position-independent code, "
                               "emit function calls using the Global Offset Table instead of the Procedure "
                               "Linkage Table.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MpreferredStackBoundary({"-mpreferred-stack-boundary"},
                               "  -mpreferred-stack-boundary             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mprefetchwt1({"-mprefetchwt1"},
                               "  -mprefetchwt1             \tThese switches enable the use of instructions in "
                               "the mprefetchwt1.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MpretendCmove({"-mpretend-cmove"},
                               "  -mpretend-cmove             \tPrefer zero-displacement conditional branches "
                               "for conditional move instruction patterns.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MprintTuneInfo({"-mprint-tune-info"},
                               "  -mprint-tune-info             \tPrint CPU tuning information as comment "
                               "in assembler file.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<std::string> MprioritizeRestrictedInsns({"-mprioritize-restricted-insns"},
                               "  -mprioritize-restricted-insns             \tThis option controls the priority that "
                               "is assigned to dispatch-slot restricted instructions during the second scheduling "
                               "pass. \n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MprologFunction({"-mprolog-function"},
                               "  -mprolog-function             \tDo use external functions to save and restore "
                               "registers at the prologue and epilogue of a function.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-prolog-function"));

maplecl::Option<bool> MprologueEpilogue({"-mprologue-epilogue"},
                               "  -mprologue-epilogue             \t\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-prologue-epilogue"));

maplecl::Option<bool> Mprototype({"-mprototype"},
                               "  -mprototype             \tOn System V.4 and embedded PowerPC systems assume "
                               "that all calls to variable argument functions are properly prototyped. \n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-prototype"));

maplecl::Option<bool> MpureCode({"-mpure-code"},
                               "  -mpure-code             \tDo not allow constant data to be placed in "
                               "code sections.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MpushArgs({"-mpush-args"},
                               "  -mpush-args             \t\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-push-args"));

maplecl::Option<bool> MQ({"-MQ"},
                               "  -MQ             \t-MQ <target>        Add a MAKE-quoted target.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MqClass({"-mq-class"},
                               "  -mq-class             \tEnable 'q' instruction alternatives.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MquadMemory({"-mquad-memory"},
                               "  -mquad-memory             \tGenerate code that uses the non-atomic quad word memory "
                               "instructions. \n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-quad-memory"));

maplecl::Option<bool> MquadMemoryAtomic({"-mquad-memory-atomic"},
                               "  -mquad-memory-atomic             \tGenerate code that uses the "
                               "atomic quad word memory instructions.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-quad-memory-atomic"));

maplecl::Option<bool> Mr10kCacheBarrier({"-mr10k-cache-barrier"},
                               "  -mr10k-cache-barrier             \tSpecify whether Maple should insert cache barriers"
                               " to avoid the side-effects of speculation on R10K processors.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MRcq({"-mRcq"},
                               "  -mRcq             \tEnable 'Rcq' constraint handling. \n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MRcw({"-mRcw"},
                               "  -mRcw             \tEnable 'Rcw' constraint handling. \n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mrdrnd({"-mrdrnd"},
                               "  -mrdrnd             \tThese switches enable the use of instructions in the mrdrnd.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MreadonlyInSdata({"-mreadonly-in-sdata"},
                               "  -mreadonly-in-sdata             \t\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-readonly-in-sdata"));

maplecl::Option<std::string> Mrecip({"-mrecip"},
                               "  -mrecip             \tThis option enables use of the reciprocal estimate and "
                               "reciprocal square root estimate instructions with additional Newton-Raphson steps "
                               "to increase precision instead of doing a divide or square root and divide for "
                               "floating-point arguments. \n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MrecipPrecision({"-mrecip-precision"},
                               "  -mrecip-precision             \tAssume (do not assume) that the reciprocal estimate "
                               "instructions provide higher-precision estimates than is mandated by the PowerPC"
                               " ABI. \n",
                               {driverCategory, unSupCategory});

maplecl::Option<std::string> MrecipE({"-mrecip="},
                               "  -mrecip=             \tThis option controls which reciprocal estimate instructions "
                               "may be used\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MrecordMcount({"-mrecord-mcount"},
                               "  -mrecord-mcount             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MreducedRegs({"-mreduced-regs"},
                               "  -mreduced-regs             \tUse reduced-set registers for register allocation.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MregisterNames({"-mregister-names"},
                               "  -mregister-names             \t\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-register-names"));

maplecl::Option<bool> Mregnames({"-mregnames"},
                               "  -mregnames             \tOn System V.4 and embedded PowerPC systems do emit "
                               "register names in the assembly language output using symbolic forms.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-regnames"));

maplecl::Option<bool> Mregparm({"-mregparm"},
                               "  -mregparm             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mrelax({"-mrelax"},
                               "  -mrelax             \tGuide linker to relax instructions.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-relax"));

maplecl::Option<bool> MrelaxImmediate({"-mrelax-immediate"},
                               "  -mrelax-immediate             \t\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-relax-immediate"));

maplecl::Option<bool> MrelaxPicCalls({"-mrelax-pic-calls"},
                               "  -mrelax-pic-calls             \tTry to turn PIC calls that are normally "
                               "dispatched via register $25 into direct calls.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mrelocatable({"-mrelocatable"},
                               "  -mrelocatable             \tGenerate code that allows a static executable to be "
                               "relocated to a different address at run time. \n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-relocatable"));

maplecl::Option<bool> MrelocatableLib({"-mrelocatable-lib"},
                               "  -mrelocatable-lib             \tGenerates a .fixup section to allow static "
                               "executables to be relocated at run time, but -mrelocatable-lib does not use the "
                               "smaller stack alignment of -mrelocatable.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-relocatable-lib"));

maplecl::Option<bool> Mrenesas({"-mrenesas"},
                               "  -mrenesas             \tComply with the calling conventions defined by Renesas.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-renesas"));

maplecl::Option<bool> Mrepeat({"-mrepeat"},
                               "  -mrepeat             \tEnables the repeat and erepeat instructions, "
                               "used for low-overhead looping.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MrestrictIt({"-mrestrict-it"},
                               "  -mrestrict-it             \tRestricts generation of IT blocks to "
                               "conform to the rules of ARMv8.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MreturnPointerOnD0({"-mreturn-pointer-on-d0"},
                               "  -mreturn-pointer-on-d0             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mrh850Abi({"-mrh850-abi"},
                               "  -mrh850-abi             \tEnables support for the RH850 version of the V850 ABI.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mrl78({"-mrl78"},
                               "  -mrl78             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mrmw({"-mrmw"},
                               "  -mrmw             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mrtd({"-mrtd"},
                               "  -mrtd             \tUse a different function-calling convention, in which "
                               "functions that take a fixed number of arguments return with the rtd instruction, "
                               "which pops their arguments while returning. \n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-rtd"));

maplecl::Option<bool> Mrtm({"-mrtm"},
                               "  -mrtm             \tThese switches enable the use of instructions in the mrtm.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mrtp({"-mrtp"},
                               "  -mrtp             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mrtsc({"-mrtsc"},
                               "  -mrtsc             \tPassed down to the assembler to enable the 64-bit time-stamp "
                               "counter extension instruction. \n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Ms({"-ms"},
                               "  -ms             \tCauses all variables to default to the .tiny section.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Ms2600({"-ms2600"},
                               "  -ms2600             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MsafeDma({"-msafe-dma"},
                               "  -msafe-dma             \ttell the compiler to treat the DMA instructions as "
                               "potentially affecting all memory.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-munsafe-dma"));

maplecl::Option<bool> MsafeHints({"-msafe-hints"},
                               "  -msafe-hints             \tWork around a hardware bug that causes the SPU to "
                               "stall indefinitely. \n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Msahf({"-msahf"},
                               "  -msahf             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Msatur({"-msatur"},
                               "  -msatur             \tEnables the saturation instructions. \n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MsaveAccInInterrupts({"-msave-acc-in-interrupts"},
                               "  -msave-acc-in-interrupts             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MsaveMducInInterrupts({"-msave-mduc-in-interrupts"},
                               "  -msave-mduc-in-interrupts             \t\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-save-mduc-in-interrupts"));

maplecl::Option<bool> MsaveRestore({"-msave-restore"},
                               "  -msave-restore             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MsaveTocIndirect({"-msave-toc-indirect"},
                               "  -msave-toc-indirect             \tGenerate code to save the TOC value in the "
                               "reserved stack location in the function prologue if the function calls through "
                               "a pointer on AIX and 64-bit Linux systems. I\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mscc({"-mscc"},
                               "  -mscc             \tEnable the use of conditional set instructions (default).\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-scc"));

maplecl::Option<bool> MschedArDataSpec({"-msched-ar-data-spec"},
                               "  -msched-ar-data-spec             \t\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-sched-ar-data-spec"));

maplecl::Option<bool> MschedArInDataSpec({"-msched-ar-in-data-spec"},
                               "  -msched-ar-in-data-spec             \t\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-sched-ar-in-data-spec"));

maplecl::Option<bool> MschedBrDataSpec({"-msched-br-data-spec"},
                               "  -msched-br-data-spec             \t\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-sched-br-data-spec"));

maplecl::Option<bool> MschedBrInDataSpec({"-msched-br-in-data-spec"},
                               "  -msched-br-in-data-spec             \t\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-sched-br-in-data-spec"));

maplecl::Option<bool> MschedControlSpec({"-msched-control-spec"},
                               "  -msched-control-spec             \t\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-sched-control-spec"));

maplecl::Option<std::string> MschedCostlyDep({"-msched-costly-dep"},
                               "  -msched-costly-dep             \tThis option controls which dependences are "
                               "considered costly by the target during instruction scheduling.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MschedCountSpecInCriticalPath({"-msched-count-spec-in-critical-path"},
                               "  -msched-count-spec-in-critical-path             \t\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-sched-count-spec-in-critical-path"));

maplecl::Option<bool> MschedFpMemDepsZeroCost({"-msched-fp-mem-deps-zero-cost"},
                               "  -msched-fp-mem-deps-zero-cost             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MschedInControlSpec({"-msched-in-control-spec"},
                               "  -msched-in-control-spec             \t\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-sched-in-control-spec"));

maplecl::Option<bool> MschedMaxMemoryInsns({"-msched-max-memory-insns"},
                               "  -msched-max-memory-insns             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MschedMaxMemoryInsnsHardLimit({"-msched-max-memory-insns-hard-limit"},
                               "  -msched-max-memory-insns-hard-limit             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MschedPreferNonControlSpecInsns({"-msched-prefer-non-control-spec-insns"},
                               "  -msched-prefer-non-control-spec-insns             \t\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-sched-prefer-non-control-spec-insns"));

maplecl::Option<bool> MschedPreferNonDataSpecInsns({"-msched-prefer-non-data-spec-insns"},
                               "  -msched-prefer-non-data-spec-insns             \t\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-sched-prefer-non-data-spec-insns"));

maplecl::Option<bool> MschedSpecLdc({"-msched-spec-ldc"},
                               "  -msched-spec-ldc             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MschedStopBitsAfterEveryCycle({"-msched-stop-bits-after-every-cycle"},
                               "  -msched-stop-bits-after-every-cycle             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<std::string> Mschedule({"-mschedule"},
                               "  -mschedule             \tSchedule code according to the constraints for "
                               "the machine type cpu-type. \n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mscore5({"-mscore5"},
                               "  -mscore5             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mscore5u({"-mscore5u"},
                               "  -mscore5u             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mscore7({"-mscore7"},
                               "  -mscore7             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mscore7d({"-mscore7d"},
                               "  -mscore7d             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<std::string> Msda({"-msda"},
                               "  -msda             \tPut static or global variables whose size is n bytes or less "
                               "into the small data area that register gp points to.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Msdata({"-msdata"},
                               "  -msdata             \tOn System V.4 and embedded PowerPC systems, if -meabi is "
                               "used, compile code the same as -msdata=eabi, otherwise compile code the same as "
                               "-msdata=sysv.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-sdata"));

maplecl::Option<bool> MsdataAll({"-msdata=all"},
                               "  -msdata=all             \tPut all data, not just small objects, into the sections "
                               "reserved for small data, and use addressing relative to the B14 register to "
                               "access them.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MsdataData({"-msdata=data"},
                               "  -msdata=data             \tOn System V.4 and embedded PowerPC systems, "
                               "put small global data in the .sdata section.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MsdataDefault({"-msdata=default"},
                               "  -msdata=default             \tOn System V.4 and embedded PowerPC systems, if -meabi "
                               "is used, compile code the same as -msdata=eabi, otherwise compile code the same as "
                               "-msdata=sysv.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MsdataEabi({"-msdata=eabi"},
                               "  -msdata=eabi             \tOn System V.4 and embedded PowerPC systems, put small "
                               "initialized const global and static data in the .sdata2 section, which is pointed to "
                               "by register r2. \n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MsdataNone({"-msdata=none"},
                               "  -msdata=none             \tOn embedded PowerPC systems, put all initialized global "
                               "and static data in the .data section, and all uninitialized data in the .bss"
                               " section.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MsdataSdata({"-msdata=sdata"},
                               "  -msdata=sdata             \tPut small global and static data in the small data "
                               "area, but do not generate special code to reference them.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MsdataSysv({"-msdata=sysv"},
                               "  -msdata=sysv             \tOn System V.4 and embedded PowerPC systems, put small "
                               "global and static data in the .sdata section, which is pointed to by register r13.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MsdataUse({"-msdata=use"},
                               "  -msdata=use             \tPut small global and static data in the small data area, "
                               "and generate special instructions to reference them.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Msdram({"-msdram"},
                               "  -msdram             \tLink the SDRAM-based runtime instead of the default "
                               "ROM-based runtime.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MsecurePlt({"-msecure-plt"},
                               "  -msecure-plt             \tenerate code that allows ld and ld.so to build "
                               "executables and shared libraries with non-executable .plt and .got sections. \n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MselSchedDontCheckControlSpec({"-msel-sched-dont-check-control-spec"},
                               "  -msel-sched-dont-check-control-spec             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MsepData({"-msep-data"},
                               "  -msep-data             \tGenerate code that allows the data segment to be "
                               "located in a different area of memory from the text segment.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-sep-data"));

maplecl::Option<bool> MserializeVolatile({"-mserialize-volatile"},
                               "  -mserialize-volatile             \t\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-serialize-volatile"));

maplecl::Option<bool> Msetlb({"-msetlb"},
                               "  -msetlb             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Msha({"-msha"},
                               "  -msha             \tThese switches enable the use of instructions in the msha.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<std::string> MsharedLibraryId({"-mshared-library-id"},
                               "  -mshared-library-id             \tSpecifies the identification number of the "
                               "ID-based shared library being compiled.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mshort({"-mshort"},
                               "  -mshort             \tConsider type int to be 16 bits wide, like short int.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-short"));

maplecl::Option<bool> MsignExtendEnabled({"-msign-extend-enabled"},
                               "  -msign-extend-enabled             \tEnable sign extend instructions.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<std::string> MsignReturnAddress({"-msign-return-address"},
                               "  -msign-return-address             \tSelect return address signing scope.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<std::string> MsiliconErrata({"-msilicon-errata"},
                               "  -msilicon-errata             \tThis option passes on a request to assembler to "
                               "enable the fixes for the named silicon errata.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<std::string> MsiliconErrataWarn({"-msilicon-errata-warn"},
                               "  -msilicon-errata-warn             \tThis option passes on a request to the "
                               "assembler to enable warning messages when a silicon errata might need to be applied.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Msim({"-msim"},
                               "  -msim             \tLink the simulator run-time libraries.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-sim"));

maplecl::Option<bool> Msimd({"-msimd"},
                               "  -msimd             \tEnable generation of ARC SIMD instructions via target-specific "
                               "builtins. Only valid for -mcpu=ARC700.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Msimnovec({"-msimnovec"},
                               "  -msimnovec             \tLink the simulator runtime libraries, excluding "
                               "built-in support for reset and exception vectors and tables.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MsimpleFpu({"-msimple-fpu"},
                               "  -msimple-fpu             \tDo not generate sqrt and div instructions for "
                               "hardware floating-point unit.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MsingleExit({"-msingle-exit"},
                               "  -msingle-exit             \tForce generated code to have a single exit "
                               "point in each function.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-single-exit"));

maplecl::Option<bool> MsingleFloat({"-msingle-float"},
                               "  -msingle-float             \tGenerate code for single-precision floating-point "
                               "operations. \n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MsinglePicBase({"-msingle-pic-base"},
                               "  -msingle-pic-base             \tTreat the register used for PIC addressing as "
                               "read-only, rather than loading it in the prologue for each function.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Msio({"-msio"},
                               "  -msio             \tGenerate the predefine, _SIO, for server IO. \n",
                               {driverCategory, unSupCategory});

maplecl::Option<std::string> MsizeLevel({"-msize-level"},
                               "  -msize-level             \tFine-tune size optimization with regards to "
                               "instruction lengths and alignment. \n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MskipRaxSetup({"-mskip-rax-setup"},
                               "  -mskip-rax-setup             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MslowBytes({"-mslow-bytes"},
                               "  -mslow-bytes             \t\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-slow-bytes"));

maplecl::Option<bool> MslowFlashData({"-mslow-flash-data"},
                               "  -mslow-flash-data             \tAssume loading data from flash is slower "
                               "than fetching instruction.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Msmall({"-msmall"},
                               "  -msmall             \tUse small-model addressing (16-bit pointers, 16-bit size_t).\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MsmallData({"-msmall-data"},
                               "  -msmall-data             \tWhen -msmall-data is used, objects 8 bytes long or "
                               "smaller are placed in a small data area (the .sdata and .sbss sections) and are "
                               "accessed via 16-bit relocations off of the $gp register. \n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MsmallDataLimit({"-msmall-data-limit"},
                               "  -msmall-data-limit             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MsmallDivides({"-msmall-divides"},
                               "  -msmall-divides             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MsmallExec({"-msmall-exec"},
                               "  -msmall-exec             \tGenerate code using the bras instruction to do "
                               "subroutine calls. \n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-small-exec"));

maplecl::Option<bool> MsmallMem({"-msmall-mem"},
                               "  -msmall-mem             \tBy default, Maple generates code assuming that addresses "
                               "are never larger than 18 bits.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MsmallModel({"-msmall-model"},
                               "  -msmall-model             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MsmallText({"-msmall-text"},
                               "  -msmall-text             \tWhen -msmall-text is used, the compiler assumes that "
                               "the code of the entire program (or shared library) fits in 4MB, and is thus reachable "
                               "with a branch instruction. \n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Msmall16({"-msmall16"},
                               "  -msmall16             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Msmallc({"-msmallc"},
                               "  -msmallc             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Msmartmips({"-msmartmips"},
                               "  -msmartmips             \tUse the MIPS SmartMIPS ASE.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-smartmips"));

maplecl::Option<bool> MsoftFloat({"-msoft-float"},
                               "  -msoft-float             \tThis option ignored; it is provided for compatibility "
                               "purposes only. \n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-soft-float"));

maplecl::Option<bool> MsoftQuadFloat({"-msoft-quad-float"},
                               "  -msoft-quad-float             \tGenerate output containing library calls for "
                               "quad-word (long double) floating-point instructions.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MsoftStack({"-msoft-stack"},
                               "  -msoft-stack             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Msp8({"-msp8"},
                               "  -msp8             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mspace({"-mspace"},
                               "  -mspace             \tTry to make the code as small as possible.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<std::string> Mspe({"-mspe="},
                               "  -mspe=             \tThis option has been deprecated. Use -mspe and -mno-spe "
                               "instead.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MspecldAnomaly({"-mspecld-anomaly"},
                               "  -mspecld-anomaly             \tWhen enabled, the compiler ensures that the "
                               "generated code does not contain speculative loads after jump instructions.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-specld-anomaly"));

maplecl::Option<bool> Mspfp({"-mspfp"},
                               "  -mspfp             \tGenerate single-precision FPX instructions, tuned "
                               "for the compact implementation.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MspfpCompact({"-mspfp-compact"},
                               "  -mspfp-compact             \tGenerate single-precision FPX instructions, "
                               "tuned for the compact implementation.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MspfpFast({"-mspfp-fast"},
                               "  -mspfp-fast             \tGenerate single-precision FPX instructions, "
                               "tuned for the fast implementation.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mspfp_compact({"-mspfp_compact"},
                               "  -mspfp_compact             \tReplaced by -mspfp-compact.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mspfp_fast({"-mspfp_fast"},
                               "  -mspfp_fast             \tReplaced by -mspfp-fast.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MsplitAddresses({"-msplit-addresses"},
                               "  -msplit-addresses             \tEnable (disable) use of the %hi() and %lo() "
                               "assembler relocation operators.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-split-addresses"));

maplecl::Option<bool> MsplitVecmoveEarly({"-msplit-vecmove-early"},
                               "  -msplit-vecmove-early             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Msse({"-msse"},
                               "  -msse             \tThese switches enable the use of instructions in the msse\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Msse2({"-msse2"},
                               "  -msse2             \tThese switches enable the use of instructions in the msse2\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Msse2avx({"-msse2avx"},
                               "  -msse2avx             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Msse3({"-msse3"},
                               "  -msse3             \tThese switches enable the use of instructions in the msse2avx\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Msse4({"-msse4"},
                               "  -msse4             \tThese switches enable the use of instructions in the msse4\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Msse41({"-msse4.1"},
                               "  -msse4.1             \tThese switches enable the use of instructions in the "
                               "msse4.1\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Msse42({"-msse4.2"},
                               "  -msse4.2             \tThese switches enable the use of instructions in the "
                               "msse4.2\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Msse4a({"-msse4a"},
                               "  -msse4a             \tThese switches enable the use of instructions in the msse4a\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Msseregparm({"-msseregparm"},
                               "  -msseregparm             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mssse3({"-mssse3"},
                               "  -mssse3             \tThese switches enable the use of instructions in the mssse3\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MstackAlign({"-mstack-align"},
                               "  -mstack-align             \t\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-stack-align"));

maplecl::Option<bool> MstackBias({"-mstack-bias"},
                               "  -mstack-bias             \tWith -mstack-bias, GCC assumes that the stack pointer, "
                               "and frame pointer if present, are offset by -2047 which must be added back when making"
                               " stack frame references.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-stack-bias"));

maplecl::Option<bool> MstackCheckL1({"-mstack-check-l1"},
                               "  -mstack-check-l1             \tDo stack checking using information placed into L1 "
                               "scratchpad memory by the uClinux kernel.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<std::string> MstackGuard({"-mstack-guard"},
                               "  -mstack-guard             \tThe S/390 back end emits additional instructions in the "
                               "function prologue that trigger a trap if the stack size is stack-guard bytes above "
                               "the stack-size (remember that the stack on S/390 grows downward). \n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MstackIncrement({"-mstack-increment"},
                               "  -mstack-increment             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MstackOffset({"-mstack-offset"},
                               "  -mstack-offset             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<std::string> MstackProtectorGuard({"-mstack-protector-guard"},
                               "  -mstack-protector-guard             \tGenerate stack protection code using canary at "
                               "guard.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<std::string> MstackProtectorGuardOffset({"-mstack-protector-guard-offset"},
                               "  -mstack-protector-guard-offset             \tWith the latter choice the options "
                               "-mstack-protector-guard-reg=reg and -mstack-protector-guard-offset=offset furthermore "
                               "specify which register to use as base register for reading the canary, and from what "
                               "offset from that base register. \n",
                               {driverCategory, unSupCategory});

maplecl::Option<std::string> MstackProtectorGuardReg({"-mstack-protector-guard-reg"},
                               "  -mstack-protector-guard-reg             \tWith the latter choice the options "
                               "-mstack-protector-guard-reg=reg and -mstack-protector-guard-offset=offset furthermore "
                               "specify which register to use as base register for reading the canary, and from what "
                               "offset from that base register. \n",
                               {driverCategory, unSupCategory});

maplecl::Option<std::string> MstackSize({"-mstack-size"},
                               "  -mstack-size             \tThe S/390 back end emits additional instructions in the "
                               "function prologue that trigger a trap if the stack size is stack-guard bytes above "
                               "the stack-size (remember that the stack on S/390 grows downward). \n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mstackrealign({"-mstackrealign"},
                               "  -mstackrealign             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MstdStructReturn({"-mstd-struct-return"},
                               "  -mstd-struct-return             \tWith -mstd-struct-return, the compiler generates "
                               "checking code in functions returning structures or unions to detect size mismatches "
                               "between the two sides of function calls, as per the 32-bit ABI.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-std-struct-return"));

maplecl::Option<bool> Mstdmain({"-mstdmain"},
                               "  -mstdmain             \tWith -mstdmain, Maple links your program against startup code"
                               " that assumes a C99-style interface to main, including a local copy of argv strings.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MstrictAlign({"-mstrict-align"},
                               "  -mstrict-align             \tDon't assume that unaligned accesses are handled "
                               "by the system.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-strict-align"));

maplecl::Option<bool> MstrictX({"-mstrict-X"},
                               "  -mstrict-X             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mstring({"-mstring"},
                               "  -mstring             \t\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-string"));

maplecl::Option<bool> MstringopStrategyAlg({"-mstringop-strategy=alg"},
                               "  -mstringop-strategy=alg             \tOverride the internal decision heuristic for "
                               "the particular algorithm to use for inlining string operations. \n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MstructureSizeBoundary({"-mstructure-size-boundary"},
                               "  -mstructure-size-boundary             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Msubxc({"-msubxc"},
                               "  -msubxc             \tWith -msubxc, Maple generates code that takes advantage of "
                               "the UltraSPARC Subtract-Extended-with-Carry instruction. \n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-subxc"));

maplecl::Option<bool> MsvMode({"-msv-mode"},
                               "  -msv-mode             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Msvr4StructReturn({"-msvr4-struct-return"},
                               "  -msvr4-struct-return             \tReturn structures smaller than 8 bytes in "
                               "registers (as specified by the SVR4 ABI).\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mswap({"-mswap"},
                               "  -mswap             \tGenerate swap instructions.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mswape({"-mswape"},
                               "  -mswape             \tPassed down to the assembler to enable the swap byte "
                               "ordering extension instruction. \n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Msym32({"-msym32"},
                               "  -msym32             \tAssume that all symbols have 32-bit values, "
                               "regardless of the selected ABI. \n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-sym32"));

maplecl::Option<bool> Msynci({"-msynci"},
                               "  -msynci             \tEnable (disable) generation of synci instructions on "
                               "architectures that support it. \n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-mno-synci"));

maplecl::Option<bool> MsysCrt0({"-msys-crt0"},
                               "  -msys-crt0             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MsysLib({"-msys-lib"},
                               "  -msys-lib             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MtargetAlign({"-mtarget-align"},
                               "  -mtarget-align             \t\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-target-align"));

maplecl::Option<bool> Mtas({"-mtas"},
                               "  -mtas             \tGenerate the tas.b opcode for __atomic_test_and_set. \n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mtbm({"-mtbm"},
                               "  -mtbm             \tThese switches enable the use of instructions in the mtbm.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<std::string> Mtda({"-mtda"},
                               "  -mtda             \tPut static or global variables whose size is n bytes or less "
                               "into the tiny data area that register ep points to.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mtelephony({"-mtelephony"},
                               "  -mtelephony             \tPassed down to the assembler to enable dual- and "
                               "single-operand instructions for telephony. \n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MtextSectionLiterals({"-mtext-section-literals"},
                               "  -mtext-section-literals             \t\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-text-section-literals"));

maplecl::Option<bool> Mtf({"-mtf"},
                               "  -mtf             \tCauses all functions to default to the .far section.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mthread({"-mthread"},
                               "  -mthread             \tThis option is available for MinGW targets. \n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mthreads({"-mthreads"},
                               "  -mthreads             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mthumb({"-mthumb"},
                               "  -mthumb             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MthumbInterwork({"-mthumb-interwork"},
                               "  -mthumb-interwork             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MtinyStack({"-mtiny-stack"},
                               "  -mtiny-stack             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<std::string> Mtiny({"-mtiny"},
                               "  -mtiny             \tVariables that are n bytes or smaller are allocated to "
                               "the .tiny section.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MTLS({"-mTLS"},
                               "  -mTLS             \tAssume a large TLS segment when generating thread-local code.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-mtls"));

maplecl::Option<std::string> MtlsDialect({"-mtls-dialect"},
                               "  -mtls-dialect             \tSpecify TLS dialect.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MtlsDirectSegRefs({"-mtls-direct-seg-refs"},
                               "  -mtls-direct-seg-refs             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MtlsMarkers({"-mtls-markers"},
                               "  -mtls-markers             \tMark calls to __tls_get_addr with a relocation "
                               "specifying the function argument. \n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-tls-markers"));

maplecl::Option<bool> Mtoc({"-mtoc"},
                               "  -mtoc             \tOn System V.4 and embedded PowerPC systems do not assume "
                               "that register 2 contains a pointer to a global area pointing to the addresses "
                               "used in the program.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-toc"));

maplecl::Option<bool> MtomcatStats({"-mtomcat-stats"},
                               "  -mtomcat-stats             \tCause gas to print out tomcat statistics.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MtoplevelSymbols({"-mtoplevel-symbols"},
                               "  -mtoplevel-symbols             \tPrepend (do not prepend) a ':' to all global "
                               "symbols, so the assembly code can be used with the PREFIX assembly directive.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-toplevel-symbols"));

maplecl::Option<std::string> Mtp({"-mtp"},
                               "  -mtp             \tSpecify the access model for the thread local storage pointer. \n",
                               {driverCategory, unSupCategory});

maplecl::Option<std::string> MtpRegno({"-mtp-regno"},
                               "  -mtp-regno             \tSpecify thread pointer register number.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MtpcsFrame({"-mtpcs-frame"},
                               "  -mtpcs-frame             \tGenerate a stack frame that is compliant with the Thumb "
                               "Procedure Call Standard for all non-leaf functions.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MtpcsLeafFrame({"-mtpcs-leaf-frame"},
                               "  -mtpcs-leaf-frame             \tGenerate a stack frame that is compliant with "
                               "the Thumb Procedure Call Standard for all leaf functions.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MtpfTrace({"-mtpf-trace"},
                               "  -mtpf-trace             \tGenerate code that adds in TPF OS specific branches to "
                               "trace routines in the operating system. \n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-tpf-trace"));

maplecl::Option<std::string> MtrapPrecision({"-mtrap-precision"},
                               "  -mtrap-precision             \tIn the Alpha architecture, floating-point traps "
                               "are imprecise.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<std::string> Mtune({"-mtune="},
                               "  -mtune=             \tOptimize for CPU. \n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MtuneCtrlFeatureList({"-mtune-ctrl=feature-list"},
                               "  -mtune-ctrl=feature-list             \tThis option is used to do fine grain "
                               "control of x86 code generation features. \n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Muclibc({"-muclibc"},
                               "  -muclibc             \tUse uClibc C library.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Muls({"-muls"},
                               "  -muls             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<std::string> Multcost({"-multcost"},
                               "  -multcost             \tReplaced by -mmultcost.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MultcostNumber({"-multcost=number"},
                               "  -multcost=number             \tSet the cost to assume for a multiply insn.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MultilibLibraryPic({"-multilib-library-pic"},
                               "  -multilib-library-pic             \tLink with the (library, not FD) pic libraries.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MmultiplyEnabled({"-mmultiply-enabled"},
                               "  -multiply-enabled             \tEnable multiply instructions.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Multiply_defined({"-multiply_defined"},
                               "  -multiply_defined             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Multiply_defined_unused({"-multiply_defined_unused"},
                               "  -multiply_defined_unused             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Multi_module({"-multi_module"},
                               "  -multi_module             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MunalignProbThreshold({"-munalign-prob-threshold"},
                               "  -munalign-prob-threshold             \tSet probability threshold for unaligning"
                               " branches. \n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MunalignedAccess({"-munaligned-access"},
                               "  -munaligned-access             \tEnables (or disables) reading and writing of 16- "
                               "and 32- bit values from addresses that are not 16- or 32- bit aligned.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-unaligned-access"));

maplecl::Option<bool> MunalignedDoubles({"-munaligned-doubles"},
                               "  -munaligned-doubles             \tAssume that doubles have 8-byte alignment. \n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-unaligned-doubles"));

maplecl::Option<bool> Municode({"-municode"},
                               "  -municode             \tThis option is available for MinGW-w64 targets.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MuniformSimt({"-muniform-simt"},
                               "  -muniform-simt             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MuninitConstInRodata({"-muninit-const-in-rodata"},
                               "  -muninit-const-in-rodata             \tPut uninitialized const variables in the "
                               "read-only data section. \n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-uninit-const-in-rodata"));

maplecl::Option<std::string> Munix({"-munix"},
                               "  -munix             \tGenerate compiler predefines and select a startfile for "
                               "the specified UNIX standard. \n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MunixAsm({"-munix-asm"},
                               "  -munix-asm             \tUse Unix assembler syntax. \n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mupdate({"-mupdate"},
                               "  -mupdate             \tGenerate code that uses the load string instructions and "
                               "the store string word instructions to save multiple registers and do small "
                               "block moves. \n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-update"));

maplecl::Option<bool> MupperRegs({"-mupper-regs"},
                               "  -mupper-regs             \tGenerate code that uses (does not use) the scalar "
                               "instructions that target all 64 registers in the vector/scalar floating point "
                               "register set, depending on the model of the machine.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-upper-regs"));

maplecl::Option<bool> MupperRegsDf({"-mupper-regs-df"},
                               "  -mupper-regs-df             \tGenerate code that uses (does not use) the scalar "
                               "double precision instructions that target all 64 registers in the vector/scalar "
                               "floating point register set that were added in version 2.06 of the PowerPC ISA.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-upper-regs-df"));

maplecl::Option<bool> MupperRegsDi({"-mupper-regs-di"},
                               "  -mupper-regs-di             \tGenerate code that uses (does not use) the scalar "
                               "instructions that target all 64 registers in the vector/scalar floating point "
                               "register set that were added in version 2.06 of the PowerPC ISA when processing "
                               "integers. \n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-upper-regs-di"));

maplecl::Option<bool> MupperRegsSf({"-mupper-regs-sf"},
                               "  -mupper-regs-sf             \tGenerate code that uses (does not use) the scalar "
                               "single precision instructions that target all 64 registers in the vector/scalar "
                               "floating point register set that were added in version 2.07 of the PowerPC ISA.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-upper-regs-sf"));

maplecl::Option<bool> MuserEnabled({"-muser-enabled"},
                               "  -muser-enabled             \tEnable user-defined instructions.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MuserMode({"-muser-mode"},
                               "  -muser-mode             \tDo not generate code that can only run in supervisor "
                               "mode.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-user-mode"));

maplecl::Option<bool> Musermode({"-musermode"},
                               "  -musermode             \tDon't allow (allow) the compiler generating "
                               "privileged mode code.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-usermode"));

maplecl::Option<bool> Mv3push({"-mv3push"},
                               "  -mv3push             \tGenerate v3 push25/pop25 instructions.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-v3push"));

maplecl::Option<bool> Mv850({"-mv850"},
                               "  -mv850             \tSpecify that the target processor is the V850.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mv850e({"-mv850e"},
                               "  -mv850e             \tSpecify that the target processor is the V850E.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mv850e1({"-mv850e1"},
                               "  -mv850e1             \tSpecify that the target processor is the V850E1. \n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mv850e2({"-mv850e2"},
                               "  -mv850e2             \tSpecify that the target processor is the V850E2.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mv850e2v3({"-mv850e2v3"},
                               "  -mv850e2v3             \tSpecify that the target processor is the V850E2V3.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mv850e2v4({"-mv850e2v4"},
                               "  -mv850e2v4             \tSpecify that the target processor is the V850E3V5.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mv850e3v5({"-mv850e3v5"},
                               "  -mv850e3v5             \tpecify that the target processor is the V850E3V5.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mv850es({"-mv850es"},
                               "  -mv850es             \tSpecify that the target processor is the V850ES.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mv8plus({"-mv8plus"},
                               "  -mv8plus             \tWith -mv8plus, Maple generates code for the SPARC-V8+ ABI.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-v8plus"));

maplecl::Option<std::string> Mveclibabi({"-mveclibabi"},
                               "  -mveclibabi             \tSpecifies the ABI type to use for vectorizing "
                               "intrinsics using an external library. \n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mvect8RetInMem({"-mvect8-ret-in-mem"},
                               "  -mvect8-ret-in-mem             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mvirt({"-mvirt"},
                               "  -mvirt             \tUse the MIPS Virtualization (VZ) instructions.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-virt"));

maplecl::Option<bool> Mvis({"-mvis"},
                               "  -mvis             \tWith -mvis, Maple generates code that takes advantage of the "
                               "UltraSPARC Visual Instruction Set extensions. \n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-vis"));

maplecl::Option<bool> Mvis2({"-mvis2"},
                               "  -mvis2             \tWith -mvis2, Maple generates code that takes advantage of "
                               "version 2.0 of the UltraSPARC Visual Instruction Set extensions. \n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-vis2"));

maplecl::Option<bool> Mvis3({"-mvis3"},
                               "  -mvis3             \tWith -mvis3, Maple generates code that takes advantage of "
                               "version 3.0 of the UltraSPARC Visual Instruction Set extensions.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-vis3"));

maplecl::Option<bool> Mvis4({"-mvis4"},
                               "  -mvis4             \tWith -mvis4, GCC generates code that takes advantage of "
                               "version 4.0 of the UltraSPARC Visual Instruction Set extensions.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-vis4"));

maplecl::Option<bool> Mvis4b({"-mvis4b"},
                               "  -mvis4b             \tWith -mvis4b, GCC generates code that takes advantage of "
                               "version 4.0 of the UltraSPARC Visual Instruction Set extensions, plus the additional "
                               "VIS instructions introduced in the Oracle SPARC Architecture 2017.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-vis4b"));

maplecl::Option<bool> MvliwBranch({"-mvliw-branch"},
                               "  -mvliw-branch             \tRun a pass to pack branches into VLIW instructions "
                               "(default).\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-vliw-branch"));

maplecl::Option<bool> MvmsReturnCodes({"-mvms-return-codes"},
                               "  -mvms-return-codes             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MvolatileAsmStop({"-mvolatile-asm-stop"},
                               "  -mvolatile-asm-stop             \t\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-volatile-asm-stop"));

maplecl::Option<bool> MvolatileCache({"-mvolatile-cache"},
                               "  -mvolatile-cache             \tUse ordinarily cached memory accesses for "
                               "volatile references. \n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-volatile-cache"));

maplecl::Option<bool> Mvr4130Align({"-mvr4130-align"},
                               "  -mvr4130-align             \tThe VR4130 pipeline is two-way superscalar, but "
                               "can only issue two instructions together if the first one is 8-byte aligned. \n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-mno-vr4130-align"));

maplecl::Option<bool> Mvrsave({"-mvrsave"},
                               "  -mvrsave             \tGenerate VRSAVE instructions when generating AltiVec code.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-vrsave"));

maplecl::Option<bool> Mvsx({"-mvsx"},
                               "  -mvsx             \tGenerate code that uses (does not use) vector/scalar (VSX)"
                               " instructions, and also enable the use of built-in functions that allow more direct "
                               "access to the VSX instruction set.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-vsx"));

maplecl::Option<bool> Mvx({"-mvx"},
                               "  -mvx             \tGenerate code using the instructions available with the vector "
                               "extension facility introduced with the IBM z13 machine generation. \n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-vx"));

maplecl::Option<bool> Mvxworks({"-mvxworks"},
                               "  -mvxworks             \tOn System V.4 and embedded PowerPC systems, specify that "
                               "you are compiling for a VxWorks system.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mvzeroupper({"-mvzeroupper"},
                               "  -mvzeroupper             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MwarnCellMicrocode({"-mwarn-cell-microcode"},
                               "  -mwarn-cell-microcode             \tWarn when a Cell microcode instruction is "
                               "emitted. \n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MwarnDynamicstack({"-mwarn-dynamicstack"},
                               "  -mwarn-dynamicstack             \tEmit a warning if the function calls alloca or "
                               "uses dynamically-sized arrays.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<std::string> MwarnFramesize({"-mwarn-framesize"},
                               "  -mwarn-framesize             \tEmit a warning if the current function exceeds "
                               "the given frame size.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MwarnMcu({"-mwarn-mcu"},
                               "  -mwarn-mcu             \tThis option enables or disables warnings about conflicts "
                               "between the MCU name specified by the -mmcu option and the ISA set by the -mcpu"
                               " option and/or the hardware multiply support set by the -mhwmult option.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-warn-mcu"));

maplecl::Option<bool> MwarnMultipleFastInterrupts({"-mwarn-multiple-fast-interrupts"},
                               "  -mwarn-multiple-fast-interrupts             \t\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-warn-multiple-fast-interrupts"));

maplecl::Option<bool> MwarnReloc({"-mwarn-reloc"},
                               "  -mwarn-reloc             \t-mwarn-reloc generates a warning instead.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-merror-reloc"));

maplecl::Option<bool> MwideBitfields({"-mwide-bitfields"},
                               "  -mwide-bitfields             \t\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-wide-bitfields"));

maplecl::Option<bool> Mwin32({"-mwin32"},
                               "  -mwin32             \tThis option is available for Cygwin and MinGW targets.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mwindows({"-mwindows"},
                               "  -mwindows             \tThis option is available for MinGW targets.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MwordRelocations({"-mword-relocations"},
                               "  -mword-relocations             \tOnly generate absolute relocations on "
                               "word-sized values (i.e. R_ARM_ABS32). \n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mx32({"-mx32"},
                               "  -mx32             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mxgot({"-mxgot"},
                               "  -mxgot             \tLift (do not lift) the usual restrictions on the "
                               "size of the global offset table.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-xgot"));

maplecl::Option<bool> MxilinxFpu({"-mxilinx-fpu"},
                               "  -mxilinx-fpu             \tPerform optimizations for the floating-point unit "
                               "on Xilinx PPC 405/440.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MxlBarrelShift({"-mxl-barrel-shift"},
                               "  -mxl-barrel-shift             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MxlCompat({"-mxl-compat"},
                               "  -mxl-compat             \tProduce code that conforms more closely to IBM XL "
                               "compiler semantics when using AIX-compatible ABI. \n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-xl-compat"));

maplecl::Option<bool> MxlFloatConvert({"-mxl-float-convert"},
                               "  -mxl-float-convert             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MxlFloatSqrt({"-mxl-float-sqrt"},
                               "  -mxl-float-sqrt             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MxlGpOpt({"-mxl-gp-opt"},
                               "  -mxl-gp-opt             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MxlMultiplyHigh({"-mxl-multiply-high"},
                               "  -mxl-multiply-high             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MxlPatternCompare({"-mxl-pattern-compare"},
                               "  -mxl-pattern-compare             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MxlReorder({"-mxl-reorder"},
                               "  -mxl-reorder             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MxlSoftDiv({"-mxl-soft-div"},
                               "  -mxl-soft-div             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MxlSoftMul({"-mxl-soft-mul"},
                               "  -mxl-soft-mul             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> MxlStackCheck({"-mxl-stack-check"},
                               "  -mxl-stack-check             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mxop({"-mxop"},
                               "  -mxop             \tThese switches enable the use of instructions in the mxop.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mxpa({"-mxpa"},
                               "  -mxpa             \tUse the MIPS eXtended Physical Address (XPA) instructions.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-xpa"));

maplecl::Option<bool> Mxsave({"-mxsave"},
                               "  -mxsave             \tThese switches enable the use of instructions in "
                               "the mxsave.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mxsavec({"-mxsavec"},
                               "  -mxsavec             \tThese switches enable the use of instructions in "
                               "the mxsavec.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mxsaveopt({"-mxsaveopt"},
                               "  -mxsaveopt             \tThese switches enable the use of instructions in "
                               "the mxsaveopt.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mxsaves({"-mxsaves"},
                               "  -mxsaves             \tThese switches enable the use of instructions in "
                               "the mxsaves.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mxy({"-mxy"},
                               "  -mxy             \tPassed down to the assembler to enable the XY memory "
                               "extension. \n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Myellowknife({"-myellowknife"},
                               "  -myellowknife             \tOn embedded PowerPC systems, assume that the startup "
                               "module is called crt0.o and the standard C libraries are libyk.a and libc.a.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mzarch({"-mzarch"},
                               "  -mzarch             \tWhen -mzarch is specified, generate code using the "
                               "instructions available on z/Architecture.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<std::string> Mzda({"-mzda"},
                               "  -mzda             \tPut static or global variables whose size is n bytes or less "
                               "into the first 32 kilobytes of memory.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Mzdcbranch({"-mzdcbranch"},
                               "  -mzdcbranch             \tAssume (do not assume) that zero displacement conditional"
                               " branch instructions bt and bf are fast. \n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-zdcbranch"));

maplecl::Option<bool> MzeroExtend({"-mzero-extend"},
                               "  -mzero-extend             \tWhen reading data from memory in sizes shorter than "
                               "64 bits, use zero-extending load instructions by default, rather than "
                               "sign-extending ones.\n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-zero-extend"));

maplecl::Option<bool> Mzvector({"-mzvector"},
                               "  -mzvector             \tThe -mzvector option enables vector language extensions and "
                               "builtins using instructions available with the vector extension facility introduced "
                               "with the IBM z13 machine generation. \n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("--mno-zvector"));

maplecl::Option<bool> No80387({"-no-80387"},
                               "  -no-80387             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> NoCanonicalPrefixes({"-no-canonical-prefixes"},
                               "  -no-canonical-prefixes             \t    0\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> NoIntegratedCpp({"-no-integrated-cpp"},
                               "  -no-integrated-cpp             \t    0\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> NoSysrootSuffix({"-no-sysroot-suffix"},
                               "  -no-sysroot-suffix             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Noall_load({"-noall_load"},
                               "  -noall_load             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Nocpp({"-nocpp"},
                               "  -nocpp             \tDisable preprocessing.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Nodefaultlibs({"-nodefaultlibs"},
                               "  -nodefaultlibs             \tDo not use the standard system libraries when "
                               "linking. \n",
                               {driverCategory, ldCategory});

maplecl::Option<bool> Nodevicelib({"-nodevicelib"},
                               "  -nodevicelib             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Nofixprebinding({"-nofixprebinding"},
                               "  -nofixprebinding             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Nolibdld({"-nolibdld"},
                               "  -nolibdld             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Nomultidefs({"-nomultidefs"},
                               "  -nomultidefs             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> NonStatic({"-non-static"},
                               "  -non-static             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Noprebind({"-noprebind"},
                               "  -noprebind             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Noseglinkedit({"-noseglinkedit"},
                               "  -noseglinkedit             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Nostartfiles({"-nostartfiles"},
                               "  -nostartfiles             \tDo not use the standard system startup files "
                               "when linking. \n",
                               {driverCategory, ldCategory});

maplecl::Option<bool> Nostdinc({"-nostdinc++"},
                               "  -nostdinc++             \tDo not search standard system include directories for"
                               " C++.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> No_dead_strip_inits_and_terms({"-no_dead_strip_inits_and_terms"},
                               "  -no_dead_strip_inits_and_terms             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Ofast({"-Ofast"},
                               "  -Ofast             \tOptimize for speed disregarding exact standards compliance.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Og({"-Og"},
                               "  -Og             \tOptimize for debugging experience rather than speed or size.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> P({"-p"},
                               "  -p             \tEnable function profiling.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> LargeP({"-P"},
                               "  -P             \tDo not generate #line directives.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Pagezero_size({"-pagezero_size"},
                               "  -pagezero_size             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Param({"-param"},
                               "  -param             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> PassExitCodes({"-pass-exit-codes"},
                               "  -pass-exit-codes             \t    0\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Pedantic({"-pedantic"},
                               "  -pedantic             \t    0\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> PedanticErrors({"-pedantic-errors"},
                               "  -pedantic-errors             \tLike -pedantic but issue them as errors.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Pg({"-pg"},
                               "  -pg             \t    0\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Plt({"-plt"},
                               "  -plt             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Prebind({"-prebind"},
                               "  -prebind             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Prebind_all_twolevel_modules({"-prebind_all_twolevel_modules"},
                               "  -prebind_all_twolevel_modules             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> PrintFileName({"-print-file-name"},
                               "  -print-file-name             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> PrintLibgccFileName({"-print-libgcc-file-name"},
                               "  -print-libgcc-file-name             \t    0\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> PrintMultiDirectory({"-print-multi-directory"},
                               "  -print-multi-directory             \t    0\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> PrintMultiLib({"-print-multi-lib"},
                               "  -print-multi-lib             \t    0\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> PrintMultiOsDirectory({"-print-multi-os-directory"},
                               "  -print-multi-os-directory             \t    0\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> PrintMultiarch({"-print-multiarch"},
                               "  -print-multiarch             \t    0\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> PrintObjcRuntimeInfo({"-print-objc-runtime-info"},
                               "  -print-objc-runtime-info             \tGenerate C header of platform-specific "
                               "features.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> PrintProgName({"-print-prog-name"},
                               "  -print-prog-name             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> PrintSearchDirs({"-print-search-dirs"},
                               "  -print-search-dirs             \t    0\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> PrintSysroot({"-print-sysroot"},
                               "  -print-sysroot             \t    0\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> PrintSysrootHeadersSuffix({"-print-sysroot-headers-suffix"},
                               "  -print-sysroot-headers-suffix             \t    0\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Private_bundle({"-private_bundle"},
                               "  -private_bundle             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Pthreads({"-pthreads"},
                               "  -pthreads             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Q({"-Q"},
                               "  -Q             \tMakes the compiler print out each function name as it is "
                               "compiled, and print some statistics about each pass when it finishes.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Qn({"-Qn"},
                               "  -Qn             \tRefrain from adding .ident directives to the output "
                               "file (this is the default).\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Qy({"-Qy"},
                               "  -Qy             \tIdentify the versions of each tool used by the compiler, "
                               "in a .ident assembler directive in the output.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Read_only_relocs({"-read_only_relocs"},
                               "  -read_only_relocs             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Remap({"-remap"},
                               "  -remap             \tRemap file names when including files.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Sectalign({"-sectalign"},
                               "  -sectalign             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Sectcreate({"-sectcreate"},
                               "  -sectcreate             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Sectobjectsymbols({"-sectobjectsymbols"},
                               "  -sectobjectsymbols             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Sectorder({"-sectorder"},
                               "  -sectorder             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Seg1addr({"-seg1addr"},
                               "  -seg1addr             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Segaddr({"-segaddr"},
                               "  -segaddr             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Seglinkedit({"-seglinkedit"},
                               "  -seglinkedit             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Segprot({"-segprot"},
                               "  -segprot             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Segs_read_only_addr({"-segs_read_only_addr"},
                               "  -segs_read_only_addr             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Segs_read_write_addr({"-segs_read_write_addr"},
                               "  -segs_read_write_addr             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Seg_addr_table({"-seg_addr_table"},
                               "  -seg_addr_table             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Seg_addr_table_filename({"-seg_addr_table_filename"},
                               "  -seg_addr_table_filename             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> SharedLibgcc({"-shared-libgcc"},
                               "  -shared-libgcc             \tOn systems that provide libgcc as a shared library, "
                               "these options force the use of either the shared or static version, respectively.\n",
                               {driverCategory, ldCategory});

maplecl::Option<bool> Sim({"-sim"},
                               "  -sim             \tThis option, recognized for the cris-axis-elf, arranges to "
                               "link with input-output functions from a simulator library\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Sim2({"-sim2"},
                               "  -sim2             \tLike -sim, but pass linker options to locate initialized "
                               "data at 0x40000000 and zero-initialized data at 0x80000000.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Single_module({"-single_module"},
                               "  -single_module             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> StaticLibgcc({"-static-libgcc"},
                               "  -static-libgcc             \tOn systems that provide libgcc as a shared library,"
                               " these options force the use of either the shared or static version, respectively.\n",
                               {driverCategory, ldCategory});

maplecl::Option<bool> StaticLibstdc({"-static-libstdc++"},
                               "  -static-libstdc++             \tWhen the g++ program is used to link a C++ program,"
                               " it normally automatically links against libstdc++. \n",
                               {driverCategory, ldCategory});

maplecl::Option<bool> Sub_library({"-sub_library"},
                               "  -sub_library             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Sub_umbrella({"-sub_umbrella"},
                               "  -sub_umbrella             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<std::string> T({"-T"},
                               "  -T             \tUse script as the linker script. This option is supported "
                               "by most systems using the GNU linker\n",
                               {driverCategory, ldCategory});

maplecl::Option<bool> TargetHelp({"-target-help"},
                               "  -target-help             \tPrint (on the standard output) a description of "
                               "target-specific command-line options for each tool.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Threads({"-threads"},
                               "  -threads             \tAdd support for multithreading with the dce thread "
                               "library under HP-UX. \n",
                               {driverCategory, unSupCategory});

maplecl::Option<std::string> Time({"-time"},
                               "  -time             \tReport the CPU time taken by each subprocess in the "
                               "compilation sequence. \n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> TnoAndroidCc({"-tno-android-cc"},
                               "  -tno-android-cc             \tDisable compilation effects of -mandroid, i.e., "
                               "do not enable -mbionic, -fPIC, -fno-exceptions and -fno-rtti by default.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> TnoAndroidLd({"-tno-android-ld"},
                               "  -tno-android-ld             \tDisable linking effects of -mandroid, i.e., pass "
                               "standard Linux linking options to the linker.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Traditional({"-traditional"},
                               "  -traditional             \tEnable traditional preprocessing. \n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> TraditionalCpp({"-traditional-cpp"},
                               "  -traditional-cpp             \tEnable traditional preprocessing.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Trigraphs({"-trigraphs"},
                               "  -trigraphs             \t-trigraphs   Support ISO C trigraphs.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Twolevel_namespace({"-twolevel_namespace"},
                               "  -twolevel_namespace             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Umbrella({"-umbrella"},
                               "  -umbrella             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Undef({"-undef"},
                               "  -undef             \tDo not predefine system-specific and GCC-specific macros.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Undefined({"-undefined"},
                               "  -undefined             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Unexported_symbols_list({"-unexported_symbols_list"},
                               "  -unexported_symbols_list             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<std::string> Wa({"-Wa"},
                               "  -Wa             \tPass option as an option to the assembler. If option contains"
                               " commas, it is split into multiple options at the commas.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Whatsloaded({"-whatsloaded"},
                               "  -whatsloaded             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Whyload({"-whyload"},
                               "  -whyload             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> WLtoTypeMismatch({"-Wlto-type-mismatch"},
                               "  -Wno-lto-type-mismatch             \tDuring the link-time optimization warn about"
                               " type mismatches in global declarations from different compilation units. \n",
                               {driverCategory, unSupCategory},
                               maplecl::DisableWith("-Wno-lto-type-mismatch"));

maplecl::Option<bool> WmisspelledIsr({"-Wmisspelled-isr"},
                               "  -Wmisspelled-isr             \t\n",
                               {driverCategory, unSupCategory});

maplecl::Option<std::string> Wp({"-Wp"},
                               "  -Wp             \tYou can use -Wp,option to bypass the compiler driver and pass "
                               "option directly through to the preprocessor. \n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> Wrapper({"-wrapper"},
                               "  -wrapper             \tInvoke all subcommands under a wrapper program. \n",
                               {driverCategory, unSupCategory});

maplecl::Option<std::string> Xassembler({"-Xassembler"},
                               "  -Xassembler             \tPass option as an option to the assembler. \n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> XbindLazy({"-Xbind-lazy"},
                               "  -Xbind-lazy             \tEnable lazy binding of function calls. \n",
                               {driverCategory, unSupCategory});

maplecl::Option<bool> XbindNow({"-Xbind-now"},
                               "  -Xbind-now             \tDisable lazy binding of function calls. \n",
                               {driverCategory, unSupCategory});

maplecl::Option<std::string> Xlinker({"-Xlinker"},
                               "  -Xlinker             \tPass option as an option to the linker. \n",
                               {driverCategory, ldCategory});

maplecl::Option<std::string> Xpreprocessor({"-Xpreprocessor"},
                               "  -Xpreprocessor             \tPass option as an option to the preprocessor. \n",
                               {driverCategory, unSupCategory});

maplecl::Option<std::string> Ym({"-Ym"},
                               "  -Ym             \tLook in the directory dir to find the M4 preprocessor. \n",
                               {driverCategory, unSupCategory});

maplecl::Option<std::string> YP({"-YP"},
                               "  -YP             \tSearch the directories dirs, and no others, for "
                               "libraries specified with -l.\n",
                               {driverCategory, unSupCategory});

maplecl::Option<std::string> Z({"-z"},
                               "  -z             \tPassed directly on to the linker along with the keyword keyword.\n",
                               {driverCategory, ldCategory});

maplecl::Option<std::string> U({"-u"},
                               "  -u             \tPretend the symbol symbol is undefined, to force linking of "
                               "library modules to define it. \n",
                               {driverCategory, ldCategory});

maplecl::Option<bool> std03({"-std=c++03"},
                            "  -std=c++03 \tConform to the ISO 1998 C++ standard revised by the 2003 technical "
                            "corrigendum.\n",
                            {driverCategory, clangCategory, unSupCategory});

maplecl::Option<bool> std0x({"-std=c++0x"},
                            "  -std=c++0x \tDeprecated in favor of -std=c++11.\n",
                            {driverCategory, clangCategory, unSupCategory});

maplecl::Option<bool> std11({"-std=c++11"},
                            "  -std=c++11 \tConform to the ISO 2011 C++ standard.\n",
                            {driverCategory, ldCategory, clangCategory});

maplecl::Option<bool> std14({"-std=c++14"},
                            "  -std=c++14 \tConform to the ISO 2014 C++ standard.\n",
                            {driverCategory, clangCategory, unSupCategory});

maplecl::Option<bool> std17({"-std=c++17"},
                            "  -std=c++17 \ttConform to the ISO 2017 C++ standard.\n",
                            {driverCategory, clangCategory, unSupCategory});

maplecl::Option<bool> std1y({"-std=c++1y"},
                            "  -std=c++1y \tDeprecated in favor of -std=c++14.\n",
                            {driverCategory, clangCategory, unSupCategory});

maplecl::Option<bool> std1z({"-std=c++1z"},
                            "  -std=c++1z \tConform to the ISO 2017(?) C++ draft standard (experimental and "
                            "incomplete support).\n",
                            {driverCategory, clangCategory, unSupCategory});

maplecl::Option<bool> std98({"-std=c++98"},
                            "  -std=c++98\tConform to the ISO 1998 C++ standard revised by the 2003 technical "
                            "corrigendum.\n",
                            {driverCategory, clangCategory, unSupCategory});

maplecl::Option<bool> std11p({"-std=c11"},
                            "  -std=c11 \tConform to the ISO 2011 C standard.\n",
                            {driverCategory, clangCategory, unSupCategory});

maplecl::Option<bool> stdc1x({"-std=c1x"},
                            "  -std=c1x \tDeprecated in favor of -std=c11.\n",
                            {driverCategory, clangCategory, unSupCategory});

maplecl::Option<bool> std89({"-std=c89"},
                            "  -std=c89\tConform to the ISO 1990 C standard.\n",
                            {driverCategory, clangCategory, unSupCategory});

maplecl::Option<bool> std90({"-std=c90"},
                            "  -std \tConform to the ISO 1998 C++ standard revised by the 2003 technical "
                            "corrigendum.\n",
                            {driverCategory, clangCategory, unSupCategory});

maplecl::Option<bool> std99({"-std=c99"},
                            "  -std=c99 \tConform to the ISO 1999 C standard.\n",
                            {driverCategory, clangCategory, unSupCategory});

maplecl::Option<bool> std9x({"-std=c9x"},
                            "  -std=c9x \tDeprecated in favor of -std=c99.\n",
                            {driverCategory, clangCategory, unSupCategory});

maplecl::Option<bool> std2003({"-std=f2003"},
                            "  -std=f2003 \tConform to the ISO Fortran 2003 standard.\n",
                            {driverCategory, clangCategory, unSupCategory});

maplecl::Option<bool> std2008({"-std=f2008"},
                            "  -std=f2008 \tConform to the ISO Fortran 2008 standard.\n",
                            {driverCategory, clangCategory, unSupCategory});

maplecl::Option<bool> std2008ts({"-std=f2008ts"},
                            "  -std=f2008ts \tConform to the ISO Fortran 2008 standard including TS 29113.\n",
                            {driverCategory, clangCategory, unSupCategory});

maplecl::Option<bool> stdf95({"-std=f95"},
                            "  -std=f95 \tConform to the ISO Fortran 95 standard.\n",
                            {driverCategory, clangCategory, unSupCategory});

maplecl::Option<bool> stdgnu({"-std=gnu"},
                            "  -std=gnu \tConform to nothing in particular.\n",
                            {driverCategory, clangCategory, unSupCategory});

maplecl::Option<bool> stdgnu03p({"-std=gnu++03"},
                            "  -std=gnu++03 \tConform to the ISO 1998 C++ standard revised by the 2003 technical "
                            "corrigendum with GNU extensions.\n",
                            {driverCategory, clangCategory, unSupCategory});

maplecl::Option<bool> stdgnuoxp({"-std=gnu++0x"},
                            "  -std=gnu++0x \tDeprecated in favor of -std=gnu++11.\n",
                            {driverCategory, clangCategory, unSupCategory});

maplecl::Option<bool> stdgnu11p({"-std=gnu++11"},
                            "  -std=gnu++11 \tConform to the ISO 2011 C++ standard with GNU extensions.\n",
                            {driverCategory, clangCategory, unSupCategory});

maplecl::Option<bool> stdgnu14p({"-std=gnu++14"},
                            "  -std=gnu++14 \tConform to the ISO 2014 C++ standard with GNU extensions.\n",
                            {driverCategory, clangCategory, unSupCategory});

maplecl::Option<bool> stdgnu17p({"-std=gnu++17"},
                            "  -std=gnu++17 \tConform to the ISO 2017 C++ standard with GNU extensions.\n",
                            {driverCategory, clangCategory, unSupCategory});

maplecl::Option<bool> stdgnu1yp({"-std=gnu++1y"},
                            "  -std=gnu++1y \tDeprecated in favor of -std=gnu++14.\n",
                            {driverCategory, clangCategory, unSupCategory});

maplecl::Option<bool> stdgnu1zp({"-std=gnu++1z"},
                            "  -std=gnu++1z \tConform to the ISO 201z(7?) C++ draft standard with GNU extensions "
                            "(experimental and incomplete support).\n",
                            {driverCategory, clangCategory, unSupCategory});

maplecl::Option<bool> stdgnu98p({"-std=gnu++98"},
                            "  -std=gnu++98 \tConform to the ISO 1998 C++ standard revised by the 2003 technical "
                            "corrigendum with GNU extensions.\n",
                            {driverCategory, clangCategory, unSupCategory});

maplecl::Option<bool> stdgnu11({"-std=gnu11"},
                            "  -std=gnu11 \tConform to the ISO 2011 C standard with GNU extensions.\n",
                            {driverCategory, clangCategory, unSupCategory});

maplecl::Option<bool> stdgnu1x({"-std=gnu1x"},
                            "  -std=gnu1x \tDeprecated in favor of -std=gnu11.\n",
                            {driverCategory, clangCategory, unSupCategory});

maplecl::Option<bool> stdgnu89({"-std=gnu89"},
                            "  -std=gnu89 \tConform to the ISO 1990 C standard with GNU extensions.\n",
                            {driverCategory, clangCategory, unSupCategory});

maplecl::Option<bool> stdgnu90({"-std=gnu90"},
                            "  -std=gnu90 \tConform to the ISO 1990 C standard with GNU extensions.\n",
                            {driverCategory, clangCategory, unSupCategory});

maplecl::Option<bool> stdgnu99({"-std=gnu99"},
                            "  -std=gnu99 \tConform to the ISO 1999 C standard with GNU extensions.\n",
                            {driverCategory, clangCategory, unSupCategory});

maplecl::Option<bool> stdgnu9x({"-std=gnu9x"},
                            "  -std=gnu9x \tDeprecated in favor of -std=gnu99.\n",
                            {driverCategory, clangCategory, unSupCategory});

maplecl::Option<bool> std1990({"-std=iso9899:1990"},
                            "  -std=iso9899:1990 \tConform to the ISO 1990 C standard.\n",
                            {driverCategory, clangCategory, unSupCategory});

maplecl::Option<bool> std1994({"-std=iso9899:199409"},
                            "  -std=iso9899:199409 \tConform to the ISO 1990 C standard as amended in 1994.\n",
                            {driverCategory, clangCategory, unSupCategory});

maplecl::Option<bool> std1999({"-std=iso9899:1999"},
                            "  -std=iso9899:1999 \tConform to the ISO 1999 C standard.\n",
                            {driverCategory, clangCategory, unSupCategory});

maplecl::Option<bool> std199x({"-std=iso9899:199x"},
                            "  -std=iso9899:199x \tDeprecated in favor of -std=iso9899:1999.\n",
                            {driverCategory, clangCategory, unSupCategory});

maplecl::Option<bool> std2011({"-std=iso9899:2011"},
                            "  -std=iso9899:2011\tConform to the ISO 2011 C standard.\n",
                            {driverCategory, clangCategory, unSupCategory});

maplecl::Option<bool> stdlegacy({"-std=legacy"},
                            "  -std=legacy\tAccept extensions to support legacy code.\n",
                            {driverCategory, clangCategory, unSupCategory});

} /* namespace opts */
