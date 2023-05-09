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
    {driverCategory, dex2mplCategory, meCategory, mpl2mplCategory, cgCategory},
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
    {driverCategory, jbc2mplCategory, hir2mplCategory, meCategory, mpl2mplCategory, cgCategory});

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
    {driverCategory, clangCategory});

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
    {driverCategory, cgCategory});

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

maplecl::Option<bool> useSignedChar({"-fsigned-char", "-usesignedchar", "--usesignedchar"},
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

maplecl::Option<bool> fm({"-fm", "--fm"},
    "  static function merge will be enabled"               \
    "  only when wpaa is enabled at the same time",
    {driverCategory, hir2mplCategory});

maplecl::Option<bool> dumpTime({"--dump-time", "-dump-time"},
    "  -dump-time             : dump time",
    {driverCategory, hir2mplCategory});

maplecl::Option<bool> aggressiveTlsLocalDynamicOpt({"--tls-local-dynamic-opt"},
    "  --tls-local-dynamic-opt    \tdo aggressive tls local dynamic opt\n",
    {driverCategory});

/* ##################### STRING Options ############################################################### */

maplecl::Option<std::string> help({"--help", "-h"},
    "  --help                   \tPrint help\n",
    {driverCategory},
    maplecl::kOptionalValue);

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
    {driverCategory, clangCategory},
    maplecl::kOptionalValue);

maplecl::Option<std::string> target({"--target", "-target"},
    "  --target=<arch><abi>        \tDescribe target platform\n"
    "  \t\t\t\tExample: --target=aarch64-gnu or --target=aarch64_be-gnuilp32\n",
    {driverCategory, clangCategory, hir2mplCategory, dex2mplCategory, ipaCategory});

maplecl::Option<std::string> oMT({"-MT"},
    "  -MT<args>                   \tSpecify name of main file output in depfile\n",
    {driverCategory, clangCategory}, maplecl::joinedValue);

maplecl::Option<std::string> oMF({"-MF"},
    "  -MF<file>                   \tWrite depfile output from -MD, -M to <file>\n",
    {driverCategory, clangCategory}, maplecl::joinedValue);

maplecl::Option<std::string> oWl({"-Wl"},
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
    {driverCategory, asCategory, ldCategory});

maplecl::Option<std::string> folder({"-tmp-folder"},
    "  -tmp-folder    \tsave former folder when generating multiple output.\n",
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

maplecl::Option<std::string> ftlsModel({"-ftls-model"},
                            "  -ftls-model=[global-dynamic|local-dynamic|initial-exec|local-exec|warmup-dynamic] "
                            "   \tAlter the thread-local storage model to be used.\n",
                            {driverCategory});

maplecl::Option<std::string> functionReorderAlgorithm({"--function-reorder-algorithm"},
                                                      " --function-reorder-algorithm=[call-chain-clustering]"
                                                      "          \t choose function reorder algorithm\n",
                                                      {driverCategory, cgCategory});
maplecl::Option<std::string> functionReorderProfile({"--function-reorder-profile"},
                                                    " --function-reorder-profile=filepath"
                                                    "          \t profile for function reorder\n",
                                                    {driverCategory, cgCategory});

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

/* ##################### maple Options ############################################################### */

maplecl::Option<std::string> oFmaxErrors({"-fmax-errors"},
    "  -fmax-errors             \tLimits the maximum number of "
    "error messages to n, If n is 0 (the default), there is no limit on the number of "
    "error messages produced. If -Wfatal-errors is also specified, then -Wfatal-errors "
    "takes precedence over this option.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oStaticLibasan({"-static-libasan"},
    "  -static-libasan             \tThe -static-libasan option directs the MAPLE driver"
    " to link libasan statically, without necessarily linking other libraries "
    "statically.\n",
    {driverCategory, ldCategory});

maplecl::Option<bool> oStaticLiblsan({"-static-liblsan"},
    "  -static-liblsan             \tThe -static-liblsan option directs the MAPLE driver"
    " to link liblsan statically, without necessarily linking other libraries "
    "statically.\n",
    {driverCategory, ldCategory});

maplecl::Option<bool> oStaticLibtsan({"-static-libtsan"},
    "  -static-libtsan             \tThe -static-libtsan option directs the MAPLE driver"
    " to link libtsan statically, without necessarily linking other libraries "
    "statically.\n",
    {driverCategory, ldCategory});

maplecl::Option<bool> oStaticLibubsan({"-static-libubsan"},
    "  -static-libubsan             \tThe -static-libubsan option directs the MAPLE"
    " driver to link libubsan statically, without necessarily linking other libraries "
    "statically.\n",
    {driverCategory, ldCategory});

maplecl::Option<bool> oStaticLibmpx({"-static-libmpx"},
    "  -static-libmpx             \tThe -static-libmpx option directs the MAPLE driver to "
    "link libmpx statically, without necessarily linking other libraries statically.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oStaticLibmpxwrappers({"-static-libmpxwrappers"},
    "  -static-libmpxwrappers             \tThe -static-libmpxwrappers option directs the"
    " MAPLE driver to link libmpxwrappers statically, without necessarily linking other"
    " libraries statically.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oSymbolic({"-symbolic"},
    "  -symbolic             \tWarn about any unresolved references (unless overridden "
    "by the link editor option -Xlinker -z -Xlinker defs).\n",
    {driverCategory, ldCategory});

maplecl::Option<bool> oFipaBitCp({"-fipa-bit-cp"},
    "  -fipa-bit-cp             \tWhen enabled, perform interprocedural bitwise constant"
    " propagation. This flag is enabled by default at -O2. It requires that -fipa-cp "
    "is enabled.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oFipaVrp({"-fipa-vrp"},
    "  -fipa-vrp             \tWhen enabled, perform interprocedural propagation of value"
    " ranges. This flag is enabled by default at -O2. It requires that -fipa-cp is "
    "enabled.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oMindirectBranchRegister({"-mindirect-branch-register"},
    "  -mindirect-branch-register            \tForce indirect call and jump via register.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oMlowPrecisionDiv({"-mlow-precision-div"},
    "  -mlow-precision-div             \tEnable the division approximation. "
    "Enabling this reduces precision of division results to about 16 bits for "
    "single precision and to 32 bits for double precision.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-mno-low-precision-div"));

maplecl::Option<bool> oMlowPrecisionSqrt({"-mlow-precision-sqrt"},
    "  -mlow-precision-sqrt             \tEnable the reciprocal square root approximation."
    " Enabling this reduces precision of reciprocal square root results to about 16 bits "
    "for single precision and to 32 bits for double precision.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-mno-low-precision-sqrt"));

maplecl::Option<bool> oM80387({"-m80387"},
    "  -m80387             \tGenerate output containing 80387 instructions for "
    "floating point.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-mno-80387"));

maplecl::Option<bool> oAllowable_client({"-allowable_client"},
    "  -allowable_client             \t\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oAll_load({"-all_load"},
    "  -all_load             \tLoads all members of static archive libraries.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oAnsi({"-ansi", "--ansi"},
    "  -ansi             \tIn C mode, this is equivalent to -std=c90. "
    "In C++ mode, it is equivalent to -std=c++98.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oArch_errors_fatal({"-arch_errors_fatal"},
    "  -arch_errors_fatal             \tCause the errors having to do with files "
    "that have the wrong architecture to be fatal.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oAuxInfo({"-aux-info"},
    "  -aux-info             \tEmit declaration information into <file>.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oBdynamic({"-Bdynamic"},
    "  -Bdynamic             \tDefined for compatibility with Diab.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oBind_at_load({"-bind_at_load"},
    "  -bind_at_load             \tCauses the output file to be marked such that the dynamic"
    " linker will bind all undefined references when the file is loaded or launched.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oBstatic({"-Bstatic"},
    "  -Bstatic             \tdefined for compatibility with Diab.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oBundle({"-bundle"},
    "  -bundle             \tProduce a Mach-o bundle format file. \n",
    {driverCategory, unSupCategory});

maplecl::Option<std::string> oBundle_loader({"-bundle_loader"},
    "  -bundle_loader             \tThis option specifies the executable that will load "
    "the build output file being linked.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oC({"-C"},
    "  -C             \tDo not discard comments. \n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oCC({"-CC"},
    "  -CC             \tDo not discard comments in macro expansions.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oClient_name({"-client_name"},
    "  -client_name             \t\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oCompatibility_version({"-compatibility_version"},
    "  -compatibility_version             \t\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oCoverage({"-coverage"},
    "  -coverage             \tThe option is a synonym for -fprofile-arcs -ftest-coverage"
    " (when compiling) and -lgcov (when linking). \n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oCurrent_version({"-current_version"},
    "  -current_version             \t\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oDa({"-da"},
    "  -da             \tProduce all the dumps listed above.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oDA({"-dA"},
    "  -dA             \tAnnotate the assembler output with miscellaneous debugging "
    "information.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oDD({"-dD"},
    "  -dD             \tDump all macro definitions, at the end of preprocessing, "
    "in addition to normal output.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oDead_strip({"-dead_strip"},
    "  -dead_strip             \t\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oDependencyFile({"-dependency-file"},
    "  -dependency-file             \t\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oDH({"-dH"},
    "  -dH             \tProduce a core dump whenever an error occurs.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oDI({"-dI"},
    "  -dI             \tOutput '#include' directives in addition to the result of"
    " preprocessing.\n",
    {driverCategory, clangCategory});

maplecl::Option<bool> oDM({"-dM"},
    "  -dM             \tInstead of the normal output, generate a list of '#define' "
    "directives for all the macros defined during the execution of the preprocessor, "
    "including predefined macros. \n",
    {driverCategory, clangCategory});

maplecl::Option<bool> oDN({"-dN"},
    "  -dN             \t Like -dD, but emit only the macro names, not their expansions.\n",
    {driverCategory, clangCategory});

maplecl::Option<bool> oDp({"-dp"},
    "  -dp             \tAnnotate the assembler output with a comment indicating which "
    "pattern and alternative is used. The length of each instruction is also printed.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oDP({"-dP"},
    "  -dP             \tDump the RTL in the assembler output as a comment before each"
    " instruction. Also turns on -dp annotation.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oDU({"-dU"},
    "  -dU             \tLike -dD except that only macros that are expanded.\n",
    {driverCategory, clangCategory});

maplecl::Option<bool> oDumpfullversion({"-dumpfullversion"},
    "  -dumpfullversion             \tPrint the full compiler version, always 3 numbers "
    "separated by dots, major, minor and patchlevel version.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oDumpmachine({"-dumpmachine"},
    "  -dumpmachine             \tPrint the compiler's target machine (for example, "
    "'i686-pc-linux-gnu')—and don't do anything else.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oDumpspecs({"-dumpspecs"},
    "  -dumpspecs             \tPrint the compiler's built-in specs—and don't do anything "
    "else. (This is used when MAPLE itself is being built.)\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oDumpversion({"-dumpversion"},
    "  -dumpversion             \tPrint the compiler version "
    "(for example, 3.0, 6.3.0 or 7)—and don't do anything else.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oDx({"-dx"},
    "  -dx             \tJust generate RTL for a function instead of compiling it."
    " Usually used with -fdump-rtl-expand.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oDylib_file({"-dylib_file"},
    "  -dylib_file             \t\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oDylinker_install_name({"-dylinker_install_name"},
    "  -dylinker_install_name             \t\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oDynamic({"-dynamic"},
    "  -dynamic             \t\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oDynamiclib({"-dynamiclib"},
    "  -dynamiclib             \tWhen passed this option, GCC produces a dynamic library "
    "instead of an executable when linking, using the Darwin libtool command.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oEB({"-EB"},
    "  -EB             \tCompile code for big-endian targets.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oEL({"-EL"},
    "  -EL             \tCompile code for little-endian targets. \n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oExported_symbols_list({"-exported_symbols_list"},
    "  -exported_symbols_list             \t\n",
    {driverCategory, unSupCategory});

maplecl::Option<std::string> oFabiCompatVersion({"-fabi-compat-version="},
    "  -fabi-compat-version=             \tThe version of the C++ ABI in use.\n",
    {driverCategory, unSupCategory});

maplecl::Option<std::string> oFabiVersion({"-fabi-version="},
    "  -fabi-version=             \tUse version n of the C++ ABI. The default is "
    "version 0.\n",
    {driverCategory, unSupCategory});

maplecl::Option<std::string> oFadaSpecParent({"-fada-spec-parent="},
    "  -fada-spec-parent=             \tIn conjunction with -fdump-ada-spec[-slim] above, "
    "generate Ada specs as child units of parent unit.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oFaggressiveLoopOptimizations({"-faggressive-loop-optimizations"},
    "  -faggressive-loop-optimizations             \tThis option tells the loop optimizer "
    "to use language constraints to derive bounds for the number of iterations of a loop.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-aggressive-loop-optimizations"));

maplecl::Option<bool> oFchkpFlexibleStructTrailingArrays({"-fchkp-flexible-struct-trailing-arrays"},
    "  -fchkp-flexible-struct-trailing-arrays             \tForces Pointer Bounds Checker "
    "to treat all trailing arrays in structures as possibly flexible. \n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-chkp-flexible-struct-trailing-arrays"));

maplecl::Option<bool> oFchkpInstrumentCalls({"-fchkp-instrument-calls"},
    "  -fchkp-instrument-calls             \tInstructs Pointer Bounds Checker to pass "
    "pointer bounds to calls. Enabled by default.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-chkp-instrument-calls"));

maplecl::Option<bool> oFchkpInstrumentMarkedOnly({"-fchkp-instrument-marked-only"},
    "  -fchkp-instrument-marked-only             \tInstructs Pointer Bounds Checker to "
    "instrument only functions marked with the bnd_instrument attribute \n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-chkp-instrument-marked-only"));

maplecl::Option<bool> oFchkpNarrowBounds({"-fchkp-narrow-bounds"},
    "  -fchkp-narrow-bounds             \tControls bounds used by Pointer Bounds Checker"
    " for pointers to object fields. \n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-chkp-narrow-bounds"));

maplecl::Option<bool> oFchkpNarrowToInnermostArray({"-fchkp-narrow-to-innermost-array"},
    "  -fchkp-narrow-to-innermost-array             \tForces Pointer Bounds Checker to use "
    "bounds of the innermost arrays in case of nested static array access.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-chkp-narrow-to-innermost-array"));

maplecl::Option<bool> oFchkpOptimize({"-fchkp-optimize"},
    "  -fchkp-optimize             \tEnables Pointer Bounds Checker optimizations.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-chkp-optimize"));

maplecl::Option<bool> oFchkpStoreBounds({"-fchkp-store-bounds"},
    "  -fchkp-store-bounds             \tInstructs Pointer Bounds Checker to generate bounds"
    " stores for pointer writes.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-chkp-store-bounds"));

maplecl::Option<bool> oFchkpTreatZeroDynamicSizeAsInfinite({"-fchkp-treat-zero-dynamic-size-as-infinite"},
    "  -fchkp-treat-zero-dynamic-size-as-infinite             \tWith this option, objects "
    "with incomplete type whose dynamically-obtained size is zero are treated as having "
    "infinite size instead by Pointer Bounds Checker. \n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-chkp-treat-zero-dynamic-size-as-infinite"));

maplecl::Option<bool> oFchkpUseFastStringFunctions({"-fchkp-use-fast-string-functions"},
    "  -fchkp-use-fast-string-functions             \tEnables use of *_nobnd versions of "
    "string functions (not copying bounds) by Pointer Bounds Checker. \n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-chkp-use-fast-string-functions"));

maplecl::Option<bool> oFchkpUseNochkStringFunctions({"-fchkp-use-nochk-string-functions"},
    "  -fchkp-use-nochk-string-functions             \tEnables use of *_nochk versions of "
    "string functions (not checking bounds) by Pointer Bounds Checker. \n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-chkp-use-nochk-string-functions"));

maplecl::Option<bool> oFchkpUseStaticBounds({"-fchkp-use-static-bounds"},
    "  -fchkp-use-static-bounds             \tAllow Pointer Bounds Checker to generate "
    "static bounds holding bounds of static variables. \n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-chkp-use-static-bounds"));

maplecl::Option<bool> oFchkpUseStaticConstBounds({"-fchkp-use-static-const-bounds"},
    "  -fchkp-use-static-const-bounds             \tUse statically-initialized bounds for "
    "constant bounds instead of generating them each time they are required.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-chkp-use-static-const-bounds"));

maplecl::Option<bool> oFchkpUseWrappers({"-fchkp-use-wrappers"},
    "  -fchkp-use-wrappers             \tAllows Pointer Bounds Checker to replace calls to "
    "built-in functions with calls to wrapper functions. \n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-chkp-use-wrappers"));

maplecl::Option<bool> oFcilkplus({"-fcilkplus"},
    "  -fcilkplus             \tEnable Cilk Plus.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-cilkplus"));

maplecl::Option<bool> oFcodeHoisting({"-fcode-hoisting"},
    "  -fcode-hoisting             \tPerform code hoisting. \n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-code-hoisting"));

maplecl::Option<bool> oFcombineStackAdjustments({"-fcombine-stack-adjustments"},
    "  -fcombine-stack-adjustments             \tTracks stack adjustments (pushes and pops)"
    " and stack memory references and then tries to find ways to combine them.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-combine-stack-adjustments"));

maplecl::Option<bool> oFcompareDebug({"-fcompare-debug"},
    "  -fcompare-debug             \tCompile with and without e.g. -gtoggle, and compare "
    "the final-insns dump.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-compare-debug"));

maplecl::Option<std::string> oFcompareDebugE({"-fcompare-debug="},
    "  -fcompare-debug=             \tCompile with and without e.g. -gtoggle, and compare "
    "the final-insns dump.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oFcompareDebugSecond({"-fcompare-debug-second"},
    "  -fcompare-debug-second             \tRun only the second compilation of "
    "-fcompare-debug.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oFcompareElim({"-fcompare-elim"},
    "  -fcompare-elim             \tPerform comparison elimination after register "
    "allocation has finished.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-compare-elim"));

maplecl::Option<bool> oFconcepts({"-fconcepts"},
    "  -fconcepts             \tEnable support for C++ concepts.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-concepts"));

maplecl::Option<bool> oFcondMismatch({"-fcond-mismatch"},
    "  -fcond-mismatch             \tAllow the arguments of the '?' operator to have "
    "different types.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-cond-mismatch"));

maplecl::Option<bool> oFconserveStack({"-fconserve-stack"},
    "  -fconserve-stack             \tDo not perform optimizations increasing noticeably"
    " stack usage.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-conserve-stack"));

maplecl::Option<std::string> oFconstantStringClass({"-fconstant-string-class="},
    "  -fconstant-string-class=             \tUse class <name> ofor constant strings."
    "no class name specified with %qs\n",
    {driverCategory, unSupCategory});

maplecl::Option<std::string> oFconstexprDepth({"-fconstexpr-depth="},
    "  -fconstexpr-depth=             \tpecify maximum constexpr recursion depth.\n",
    {driverCategory, unSupCategory});

maplecl::Option<std::string> oFconstexprLoopLimit({"-fconstexpr-loop-limit="},
    "  -fconstexpr-loop-limit=             \tSpecify maximum constexpr loop iteration "
    "count.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oFcpropRegisters({"-fcprop-registers"},
    "  -fcprop-registers             \tPerform a register copy-propagation optimization"
    " pass.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-cprop-registers"));

maplecl::Option<bool> oFcrossjumping({"-fcrossjumping"},
    "  -fcrossjumping             \tPerform cross-jumping transformation. This "
    "transformation unifies equivalent code and saves code size. \n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-crossjumping"));

maplecl::Option<bool> oFcseFollowJumps({"-fcse-follow-jumps"},
    "  -fcse-follow-jumps             \tWhen running CSE, follow jumps to their targets.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-cse-follow-jumps"));

maplecl::Option<bool> oFcseSkipBlocks({"-fcse-skip-blocks"},
    "  -fcse-skip-blocks             \tDoes nothing.  Preserved for backward "
    "compatibility.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-cse-skip-blocks"));

maplecl::Option<bool> oFcxFortranRules({"-fcx-fortran-rules"},
    "  -fcx-fortran-rules             \tComplex multiplication and division follow "
    "Fortran rules.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-cx-fortran-rules"));

maplecl::Option<bool> oFcxLimitedRange({"-fcx-limited-range"},
    "  -fcx-limited-range             \tOmit range reduction step when performing complex "
    "division.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-cx-limited-range"));

maplecl::Option<bool> oFdbgCnt({"-fdbg-cnt"},
    "  -fdbg-cnt             \tPlace data items into their own section.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-dbg-cnt"));

maplecl::Option<bool> oFdbgCntList({"-fdbg-cnt-list"},
    "  -fdbg-cnt-list             \tList all available debugging counters with their "
    "limits and counts.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-dbg-cnt-list"));

maplecl::Option<bool> oFdce({"-fdce"},
    "  -fdce             \tUse the RTL dead code elimination pass.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-dce"));

maplecl::Option<bool> oFdebugCpp({"-fdebug-cpp"},
    "  -fdebug-cpp             \tEmit debug annotations during preprocessing.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-debug-cpp"));

maplecl::Option<bool> oFdebugPrefixMap({"-fdebug-prefix-map"},
    "  -fdebug-prefix-map             \tMap one directory name to another in debug "
    "information.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-debug-prefix-map"));

maplecl::Option<bool> oFdebugTypesSection({"-fdebug-types-section"},
    "  -fdebug-types-section             \tOutput .debug_types section when using DWARF "
    "v4 debuginfo.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-debug-types-section"));

maplecl::Option<bool> oFdecloneCtorDtor({"-fdeclone-ctor-dtor"},
    "  -fdeclone-ctor-dtor             \tFactor complex constructors and destructors to "
    "favor space over speed.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-declone-ctor-dtor"));

maplecl::Option<bool> oFdeduceInitList({"-fdeduce-init-list"},
    "  -fdeduce-init-list             \tenable deduction of std::initializer_list for a "
    "template type parameter from a brace-enclosed initializer-list.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-deduce-init-list"));

maplecl::Option<bool> oFdelayedBranch({"-fdelayed-branch"},
    "  -fdelayed-branch             \tAttempt to fill delay slots of branch instructions.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-delayed-branch"));

maplecl::Option<bool> oFdeleteDeadExceptions({"-fdelete-dead-exceptions"},
    "  -fdelete-dead-exceptions             \tDelete dead instructions that may throw "
    "exceptions.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-delete-dead-exceptions"));

maplecl::Option<bool> oFdeleteNullPointerChecks({"-fdelete-null-pointer-checks"},
    "  -fdelete-null-pointer-checks             \tDelete useless null pointer checks.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-delete-null-pointer-checks"));

maplecl::Option<bool> oFdevirtualize({"-fdevirtualize"},
    "  -fdevirtualize             \tTry to convert virtual calls to direct ones.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-devirtualize"));

maplecl::Option<bool> oFdevirtualizeAtLtrans({"-fdevirtualize-at-ltrans"},
    "  -fdevirtualize-at-ltrans             \tStream extra data to support more aggressive "
    "devirtualization in LTO local transformation mode.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-devirtualize-at-ltrans"));

maplecl::Option<bool> oFdevirtualizeSpeculatively({"-fdevirtualize-speculatively"},
    "  -fdevirtualize-speculatively             \tPerform speculative devirtualization.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-devirtualize-speculatively"));

maplecl::Option<bool> oFdiagnosticsGeneratePatch({"-fdiagnostics-generate-patch"},
    "  -fdiagnostics-generate-patch             \tPrint fix-it hints to stderr in unified "
    "diff format.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-diagnostics-generate-patch"));

maplecl::Option<bool> oFdiagnosticsParseableFixits({"-fdiagnostics-parseable-fixits"},
    "  -fdiagnostics-parseable-fixits             \tPrint fixit hints in machine-readable "
    "form.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-diagnostics-parseable-fixits"));

maplecl::Option<bool> oFdiagnosticsShowCaret({"-fdiagnostics-show-caret"},
    "  -fdiagnostics-show-caret             \tShow the source line with a caret indicating "
    "the column.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-diagnostics-show-caret"));

maplecl::Option<std::string> oFdiagnosticsShowLocation({"-fdiagnostics-show-location"},
    "  -fdiagnostics-show-location             \tHow often to emit source location at the "
    "beginning of line-wrapped diagnostics.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oFdiagnosticsShowOption({"-fdiagnostics-show-option"},
    "  -fdiagnostics-show-option             \tAmend appropriate diagnostic messages with "
    "the command line option that controls them.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("--fno-diagnostics-show-option"));

maplecl::Option<bool> oFdirectivesOnly({"-fdirectives-only"},
    "  -fdirectives-only             \tPreprocess directives only.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-fdirectives-only"));

maplecl::Option<std::string> oFdisable({"-fdisable-"},
    "  -fdisable-             \t-fdisable-[tree|rtl|ipa]-<pass>=range1+range2 "
    "disables an optimization pass.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oFdollarsInIdentifiers({"-fdollars-in-identifiers"},
    "  -fdollars-in-identifiers             \tPermit '$' as an identifier character.\n",
    {driverCategory, clangCategory},
    maplecl::DisableWith("-fno-dollars-in-identifiers"));

maplecl::Option<bool> oFdse({"-fdse"},
    "  -fdse             \tUse the RTL dead store elimination pass.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-dse"));

maplecl::Option<bool> oFdumpAdaSpec({"-fdump-ada-spec"},
    "  -fdump-ada-spec             \tWrite all declarations as Ada code transitively.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oFdumpClassHierarchy({"-fdump-class-hierarchy"},
    "  -fdump-class-hierarchy             \tC++ only)\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-dump-class-hierarchy"));

maplecl::Option<std::string> oFdumpFinalInsns({"-fdump-final-insns"},
    "  -fdump-final-insns             \tDump the final internal representation (RTL) to "
    "file.\n",
    {driverCategory, unSupCategory});

maplecl::Option<std::string> oFdumpGoSpec({"-fdump-go-spec="},
    "  -fdump-go-spec             \tWrite all declarations to file as Go code.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oFdumpIpa({"-fdump-ipa"},
    "  -fdump-ipa             \t\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oFdumpNoaddr({"-fdump-noaddr"},
    "  -fdump-noaddr             \tSuppress output of addresses in debugging dumps.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-dump-noaddr"));

maplecl::Option<bool> oFdumpPasses({"-fdump-passes"},
    "  -fdump-passes             \tDump optimization passes.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-dump-passes"));

maplecl::Option<bool> oFdumpRtlAlignments({"-fdump-rtl-alignments"},
    "  -fdump-rtl-alignments             \tDump after branch alignments have been "
    "computed.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oFdumpRtlAll({"-fdump-rtl-all"},
    "  -fdump-rtl-all             \tProduce all the dumps listed above.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-dump-rtl-all"));

maplecl::Option<bool> oFdumpRtlAsmcons({"-fdump-rtl-asmcons"},
    "  -fdump-rtl-asmcons             \tDump after fixing rtl statements that have "
    "unsatisfied in/out constraints.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-dump-rtl-asmcons"));

maplecl::Option<bool> oFdumpRtlAuto_inc_dec({"-fdump-rtl-auto_inc_dec"},
    "  -fdump-rtl-auto_inc_dec             \tDump after auto-inc-dec discovery. \n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-dump-rtl-auto_inc_dec"));

maplecl::Option<bool> oFdumpRtlBarriers({"-fdump-rtl-barriers"},
    "  -fdump-rtl-barriers             \tDump after cleaning up the barrier instructions.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-dump-rtl-barriers"));

maplecl::Option<bool> oFdumpRtlBbpart({"-fdump-rtl-bbpart"},
    "  -fdump-rtl-bbpart             \tDump after partitioning hot and cold basic blocks.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-dump-rtl-bbpart"));

maplecl::Option<bool> oFdumpRtlBbro({"-fdump-rtl-bbro"},
    "  -fdump-rtl-bbro             \tDump after block reordering.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-dump-rtl-bbro"));

maplecl::Option<bool> oFdumpRtlBtl2({"-fdump-rtl-btl2"},
    "  -fdump-rtl-btl2             \t-fdump-rtl-btl1 and -fdump-rtl-btl2 enable dumping "
    "after the two branch target load optimization passes.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-dump-rtl-btl2"));

maplecl::Option<bool> oFdumpRtlBypass({"-fdump-rtl-bypass"},
    "  -fdump-rtl-bypass             \tDump after jump bypassing and control flow "
    "optimizations.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-dump-rtl-bypass"));

maplecl::Option<bool> oFdumpRtlCe1({"-fdump-rtl-ce1"},
    "  -fdump-rtl-ce1             \tEnable dumping after the three if conversion passes.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-dump-rtl-ce1"));

maplecl::Option<bool> oFdumpRtlCe2({"-fdump-rtl-ce2"},
    "  -fdump-rtl-ce2             \tEnable dumping after the three if conversion passes.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-dump-rtl-ce2"));

maplecl::Option<bool> oFdumpRtlCe3({"-fdump-rtl-ce3"},
    "  -fdump-rtl-ce3             \tEnable dumping after the three if conversion passes.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-dump-rtl-ce3"));

maplecl::Option<bool> oFdumpRtlCombine({"-fdump-rtl-combine"},
    "  -fdump-rtl-combine             \tDump after the RTL instruction combination pass.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-dump-rtl-combine"));

maplecl::Option<bool> oFdumpRtlCompgotos({"-fdump-rtl-compgotos"},
    "  -fdump-rtl-compgotos             \tDump after duplicating the computed gotos.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-dump-rtl-compgotos"));

maplecl::Option<bool> oFdumpRtlCprop_hardreg({"-fdump-rtl-cprop_hardreg"},
    "  -fdump-rtl-cprop_hardreg             \tDump after hard register copy propagation.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-dump-rtl-cprop_hardreg"));

maplecl::Option<bool> oFdumpRtlCsa({"-fdump-rtl-csa"},
    "  -fdump-rtl-csa             \tDump after combining stack adjustments.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-dump-rtl-csa"));

maplecl::Option<bool> oFdumpRtlCse1({"-fdump-rtl-cse1"},
    "  -fdump-rtl-cse1             \tEnable dumping after the two common subexpression "
    "elimination passes.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-dump-rtl-cse1"));

maplecl::Option<bool> oFdumpRtlCse2({"-fdump-rtl-cse2"},
    "  -fdump-rtl-cse2             \tEnable dumping after the two common subexpression "
    "elimination passes.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-dump-rtl-cse2"));

maplecl::Option<bool> oFdumpRtlDbr({"-fdump-rtl-dbr"},
    "  -fdump-rtl-dbr             \tDump after delayed branch scheduling.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-dump-rtl-dbr"));

maplecl::Option<bool> oFdumpRtlDce({"-fdump-rtl-dce"},
    "  -fdump-rtl-dce             \tDump after the standalone dead code elimination "
    "passes.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-fdump-rtl-dce"));

maplecl::Option<bool> oFdumpRtlDce1({"-fdump-rtl-dce1"},
    "  -fdump-rtl-dce1             \tenable dumping after the two dead store elimination "
    "passes.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-dump-rtl-dce1"));

maplecl::Option<bool> oFdumpRtlDce2({"-fdump-rtl-dce2"},
    "  -fdump-rtl-dce2             \tenable dumping after the two dead store elimination "
    "passes.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-dump-rtl-dce2"));

maplecl::Option<bool> oFdumpRtlDfinish({"-fdump-rtl-dfinish"},
    "  -fdump-rtl-dfinish             \tThis dump is defined but always produce empty "
    "files.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-dump-rtl-dfinish"));

maplecl::Option<bool> oFdumpRtlDfinit({"-fdump-rtl-dfinit"},
    "  -fdump-rtl-dfinit             \tThis dump is defined but always produce empty "
    "files.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-dump-rtl-dfinit"));

maplecl::Option<bool> oFdumpRtlEh({"-fdump-rtl-eh"},
    "  -fdump-rtl-eh             \tDump after finalization of EH handling code.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-dump-rtl-eh"));

maplecl::Option<bool> oFdumpRtlEh_ranges({"-fdump-rtl-eh_ranges"},
    "  -fdump-rtl-eh_ranges             \tDump after conversion of EH handling range "
    "regions.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-dump-rtl-eh_ranges"));

maplecl::Option<bool> oFdumpRtlExpand({"-fdump-rtl-expand"},
    "  -fdump-rtl-expand             \tDump after RTL generation.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-dump-rtl-expand"));

maplecl::Option<bool> oFdumpRtlFwprop1({"-fdump-rtl-fwprop1"},
    "  -fdump-rtl-fwprop1             \tenable dumping after the two forward propagation "
    "passes.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-dump-rtl-fwprop1"));

maplecl::Option<bool> oFdumpRtlFwprop2({"-fdump-rtl-fwprop2"},
    "  -fdump-rtl-fwprop2             \tenable dumping after the two forward propagation "
    "passes.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-dump-rtl-fwprop2"));

maplecl::Option<bool> oFdumpRtlGcse1({"-fdump-rtl-gcse1"},
    "  -fdump-rtl-gcse1             \tenable dumping after global common subexpression "
    "elimination.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-dump-rtl-gcse1"));

maplecl::Option<bool> oFdumpRtlGcse2({"-fdump-rtl-gcse2"},
    "  -fdump-rtl-gcse2             \tenable dumping after global common subexpression "
    "elimination.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-dump-rtl-gcse2"));

maplecl::Option<bool> oFdumpRtlInitRegs({"-fdump-rtl-init-regs"},
    "  -fdump-rtl-init-regs             \tDump after the initialization of the registers.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-tedump-rtl-init-regsst"));

maplecl::Option<bool> oFdumpRtlInitvals({"-fdump-rtl-initvals"},
    "  -fdump-rtl-initvals             \tDump after the computation of the initial "
    "value sets.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-dump-rtl-initvals"));

maplecl::Option<bool> oFdumpRtlInto_cfglayout({"-fdump-rtl-into_cfglayout"},
    "  -fdump-rtl-into_cfglayout             \tDump after converting to cfglayout mode.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-dump-rtl-into_cfglayout"));

maplecl::Option<bool> oFdumpRtlIra({"-fdump-rtl-ira"},
    "  -fdump-rtl-ira             \tDump after iterated register allocation.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-dump-rtl-ira"));

maplecl::Option<bool> oFdumpRtlJump({"-fdump-rtl-jump"},
    "  -fdump-rtl-jump             \tDump after the second jump optimization.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-dump-rtl-jump"));

maplecl::Option<bool> oFdumpRtlLoop2({"-fdump-rtl-loop2"},
    "  -fdump-rtl-loop2             \tenables dumping after the rtl loop optimization "
    "passes.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-dump-rtl-loop2"));

maplecl::Option<bool> oFdumpRtlMach({"-fdump-rtl-mach"},
    "  -fdump-rtl-mach             \tDump after performing the machine dependent "
    "reorganization pass, if that pass exists.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-dump-rtl-mach"));

maplecl::Option<bool> oFdumpRtlMode_sw({"-fdump-rtl-mode_sw"},
    "  -fdump-rtl-mode_sw             \tDump after removing redundant mode switches.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-dump-rtl-mode_sw"));

maplecl::Option<bool> oFdumpRtlOutof_cfglayout({"-fdump-rtl-outof_cfglayout"},
    "  -fdump-rtl-outof_cfglayout             \tDump after converting from cfglayout "
    "mode.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-dump-rtl-outof_cfglayout"));

maplecl::Option<std::string> oFdumpRtlPass({"-fdump-rtl-pass"},
    "  -fdump-rtl-pass             \tSays to make debugging dumps during compilation at "
    "times specified by letters.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oFdumpRtlPeephole2({"-fdump-rtl-peephole2"},
    "  -fdump-rtl-peephole2             \tDump after the peephole pass.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-dump-rtl-peephole2"));

maplecl::Option<bool> oFdumpRtlPostreload({"-fdump-rtl-postreload"},
    "  -fdump-rtl-postreload             \tDump after post-reload optimizations.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-dump-rtl-postreload"));

maplecl::Option<bool> oFdumpRtlPro_and_epilogue({"-fdump-rtl-pro_and_epilogue"},
    "  -fdump-rtl-pro_and_epilogue             \tDump after generating the function "
    "prologues and epilogues.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-dump-rtl-pro_and_epilogue"));

maplecl::Option<bool> oFdumpRtlRee({"-fdump-rtl-ree"},
    "  -fdump-rtl-ree             \tDump after sign/zero extension elimination.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-dump-rtl-ree"));

maplecl::Option<bool> oFdumpRtlRegclass({"-fdump-rtl-regclass"},
    "  -fdump-rtl-regclass             \tThis dump is defined but always produce "
    "empty files.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-dump-rtl-regclass"));

maplecl::Option<bool> oFdumpRtlRnreg({"-fdump-rtl-rnreg"},
    "  -fdump-rtl-rnreg             \tDump after register renumbering.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-dump-rtl-rnreg"));

maplecl::Option<bool> oFdumpRtlSched1({"-fdump-rtl-sched1"},
    "  -fdump-rtl-sched1             \tnable dumping after the basic block scheduling "
    "passes.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-dump-rtl-sched1"));

maplecl::Option<bool> oFdumpRtlSched2({"-fdump-rtl-sched2"},
    "  -fdump-rtl-sched2             \tnable dumping after the basic block scheduling "
    "passes.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-dump-rtl-sched2"));

maplecl::Option<bool> oFdumpRtlSeqabstr({"-fdump-rtl-seqabstr"},
    "  -fdump-rtl-seqabstr             \tDump after common sequence discovery.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-dump-rtl-seqabstr"));

maplecl::Option<bool> oFdumpRtlShorten({"-fdump-rtl-shorten"},
    "  -fdump-rtl-shorten             \tDump after shortening branches.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-dump-rtl-shorten"));

maplecl::Option<bool> oFdumpRtlSibling({"-fdump-rtl-sibling"},
    "  -fdump-rtl-sibling             \tDump after sibling call optimizations.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-dump-rtl-sibling"));

maplecl::Option<bool> oFdumpRtlSms({"-fdump-rtl-sms"},
    "  -fdump-rtl-sms             \tDump after modulo scheduling. This pass is only "
    "run on some architectures.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-dump-rtl-sms"));

maplecl::Option<bool> oFdumpRtlSplit1({"-fdump-rtl-split1"},
    "  -fdump-rtl-split1             \tThis option enable dumping after five rounds of "
    "instruction splitting.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-dump-rtl-split1"));

maplecl::Option<bool> oFdumpRtlSplit2({"-fdump-rtl-split2"},
    "  -fdump-rtl-split2             \tThis option enable dumping after five rounds of "
    "instruction splitting.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-dump-rtl-split2"));

maplecl::Option<bool> oFdumpRtlSplit3({"-fdump-rtl-split3"},
    "  -fdump-rtl-split3             \tThis option enable dumping after five rounds of "
    "instruction splitting.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-dump-rtl-split3"));

maplecl::Option<bool> oFdumpRtlSplit4({"-fdump-rtl-split4"},
    "  -fdump-rtl-split4             \tThis option enable dumping after five rounds of "
    "instruction splitting.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-dump-rtl-split4"));

maplecl::Option<bool> oFdumpRtlSplit5({"-fdump-rtl-split5"},
    "  -fdump-rtl-split5             \tThis option enable dumping after five rounds of "
    "instruction splitting.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-dump-rtl-split5"));

maplecl::Option<bool> oFdumpRtlStack({"-fdump-rtl-stack"},
    "  -fdump-rtl-stack             \tDump after conversion from GCC's "
    "'flat register file' registers to the x87's stack-like registers. "
    "This pass is only run on x86 variants.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-dump-rtl-stack"));

maplecl::Option<bool> oFdumpRtlSubreg1({"-fdump-rtl-subreg1"},
    "  -fdump-rtl-subreg1             \tenable dumping after the two subreg expansion "
    "passes.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-dump-rtl-subreg1"));

maplecl::Option<bool> oFdumpRtlSubreg2({"-fdump-rtl-subreg2"},
    "  -fdump-rtl-subreg2             \tenable dumping after the two subreg expansion "
    "passes.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-dump-rtl-subreg2"));

maplecl::Option<bool> oFdumpRtlSubregs_of_mode_finish({"-fdump-rtl-subregs_of_mode_finish"},
    "  -fdump-rtl-subregs_of_mode_finish             \tThis dump is defined but always "
    "produce empty files.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-dump-rtl-subregs_of_mode_finish"));

maplecl::Option<bool> oFdumpRtlSubregs_of_mode_init({"-fdump-rtl-subregs_of_mode_init"},
    "  -fdump-rtl-subregs_of_mode_init             \tThis dump is defined but always "
    "produce empty files.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-dump-rtl-subregs_of_mode_init"));

maplecl::Option<bool> oFdumpRtlUnshare({"-fdump-rtl-unshare"},
    "  -fdump-rtl-unshare             \tDump after all rtl has been unshared.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-dump-rtl-unshare"));

maplecl::Option<bool> oFdumpRtlVartrack({"-fdump-rtl-vartrack"},
    "  -fdump-rtl-vartrack             \tDump after variable tracking.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-dump-rtl-vartrack"));

maplecl::Option<bool> oFdumpRtlVregs({"-fdump-rtl-vregs"},
    "  -fdump-rtl-vregs             \tDump after converting virtual registers to "
    "hard registers.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-dump-rtl-vregs"));

maplecl::Option<bool> oFdumpRtlWeb({"-fdump-rtl-web"},
    "  -fdump-rtl-web             \tDump after live range splitting.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-dump-rtl-web"));

maplecl::Option<bool> oFdumpStatistics({"-fdump-statistics"},
    "  -fdump-statistics             \tEnable and control dumping of pass statistics "
    "in a separate file.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-dump-statistics"));

maplecl::Option<bool> oFdumpTranslationUnit({"-fdump-translation-unit"},
    "  -fdump-translation-unit             \tC++ only\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-dump-translation-unit"));

maplecl::Option<bool> oFdumpTree({"-fdump-tree"},
    "  -fdump-tree             \tControl the dumping at various stages of processing the "
    "intermediate language tree to a file.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-dump-tree"));

maplecl::Option<bool> oFdumpTreeAll({"-fdump-tree-all"},
    "  -fdump-tree-all             \tControl the dumping at various stages of processing "
    "the intermediate language tree to a file.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-dump-tree-all"));

maplecl::Option<bool> oFdumpUnnumbered({"-fdump-unnumbered"},
    "  -fdump-unnumbered             \tWhen doing debugging dumps, suppress instruction "
    "numbers and address output. \n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-dump-unnumbered"));

maplecl::Option<bool> oFdumpUnnumberedLinks({"-fdump-unnumbered-links"},
    "  -fdump-unnumbered-links             \tWhen doing debugging dumps (see -d option "
    "above), suppress instruction numbers for the links to the previous and next "
    "instructions in a sequence.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-dump-unnumbered-links"));

maplecl::Option<bool> oFdwarf2CfiAsm({"-fdwarf2-cfi-asm"},
    "  -fdwarf2-cfi-asm             \tEnable CFI tables via GAS assembler directives.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-dwarf2-cfi-asm"));

maplecl::Option<bool> oFearlyInlining({"-fearly-inlining"},
    "  -fearly-inlining             \tPerform early inlining.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-early-inlining"));

maplecl::Option<bool> oFeliminateDwarf2Dups({"-feliminate-dwarf2-dups"},
    "  -feliminate-dwarf2-dups             \tPerform DWARF duplicate elimination.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-eliminate-dwarf2-dups"));

maplecl::Option<bool> oFeliminateUnusedDebugSymbols({"-feliminate-unused-debug-symbols"},
    "  -feliminate-unused-debug-symbols             \tPerform unused symbol elimination "
    "in debug info.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-feliminate-unused-debug-symbols"));

maplecl::Option<bool> oFeliminateUnusedDebugTypes({"-feliminate-unused-debug-types"},
    "  -feliminate-unused-debug-types             \tPerform unused type elimination in "
    "debug info.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-eliminate-unused-debug-types"));

maplecl::Option<bool> oFemitClassDebugAlways({"-femit-class-debug-always"},
    "  -femit-class-debug-always             \tDo not suppress C++ class debug "
    "information.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-emit-class-debug-always"));

maplecl::Option<bool> oFemitStructDebugBaseonly({"-femit-struct-debug-baseonly"},
    "  -femit-struct-debug-baseonly             \tAggressive reduced debug info for "
    "structs.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-emit-struct-debug-baseonly"));

maplecl::Option<std::string> oFemitStructDebugDetailedE({"-femit-struct-debug-detailed="},
    "  -femit-struct-debug-detailed             \t-femit-struct-debug-detailed=<spec-list> o"
    "Detailed reduced debug info for structs.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oFemitStructDebugReduced({"-femit-struct-debug-reduced"},
    "  -femit-struct-debug-reduced             \tConservative reduced debug info for "
    "structs.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-emit-struct-debug-reduced"));

maplecl::Option<std::string> oFenable({"-fenable-"},
    "  -fenable-             \t-fenable-[tree|rtl|ipa]-<pass>=range1+range2 enables an "
    "optimization pass.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oFexceptions({"-fexceptions"},
    "  -fexceptions             \tEnable exception handling.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-exceptions"));

maplecl::Option<std::string> oFexcessPrecision({"-fexcess-precision="},
    "  -fexcess-precision=             \tSpecify handling of excess floating-point "
    "precision.\n",
    {driverCategory, unSupCategory});

maplecl::Option<std::string> oFexecCharset({"-fexec-charset="},
    "  -fexec-charset=             \tConvert all strings and character constants to "
    "character set <cset>.\n",
    {driverCategory, clangCategory});

maplecl::Option<bool> oFexpensiveOptimizations({"-fexpensive-optimizations"},
    "  -fexpensive-optimizations             \tPerform a number of minor, expensive "
    "optimizations.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-expensive-optimizations"));

maplecl::Option<bool> oFextNumericLiterals({"-fext-numeric-literals"},
    "  -fext-numeric-literals             \tInterpret imaginary, fixed-point, or other "
    "gnu number suffix as the corresponding number literal rather than a user-defined "
    "number literal.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-ext-numeric-literals"));

maplecl::Option<bool> oFextendedIdentifiers({"-fextended-identifiers"},
    "  -fextended-identifiers             \tPermit universal character names (\\u and \\U) in identifiers.\n",
    {driverCategory, clangCategory});

maplecl::Option<bool> oFnoExtendedIdentifiers({"-fno-extended-identifiers"},
    "  -fno-extended-identifiers             \tDon't ermit universal character names (\\u and \\U) in identifiers.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oFexternTlsInit({"-fextern-tls-init"},
    "  -fextern-tls-init             \tSupport dynamic initialization of thread-local "
    "variables in a different translation unit.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-extern-tls-init"));

maplecl::Option<bool> oFfastMath({"-ffast-math"},
    "  -ffast-math             \tThis option causes the preprocessor macro __FAST_MATH__ "
    "to be defined.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-fast-math"));

maplecl::Option<bool> oFfatLtoObjects({"-ffat-lto-objects"},
    "  -ffat-lto-objects             \tOutput lto objects containing both the intermediate "
    "language and binary output.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-fat-lto-objects"));

maplecl::Option<bool> oFfiniteMathOnly({"-ffinite-math-only"},
    "  -ffinite-math-only             \tAssume no NaNs or infinities are generated.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-finite-math-only"));

maplecl::Option<bool> oFfixAndContinue({"-ffix-and-continue"},
    "  -ffix-and-continue             \tGenerate code suitable for fast turnaround "
    "development, such as to allow GDB to dynamically load .o files into already-running "
    "programs.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-fix-and-continue"));

maplecl::Option<std::string> oFfixed({"-ffixed-"},
    "  -ffixed-             \t-ffixed-<register>	Mark <register> oas being unavailable to "
    "the compiler.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oFfloatStore({"-ffloat-store"},
    "  -ffloat-store             \ton't allocate floats and doubles in extended-precision "
    "registers.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-float-store"));

maplecl::Option<bool> oFforScope({"-ffor-scope"},
    "  -ffor-scope             \tScope of for-init-statement variables is local to the "
    "loop.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-for-scope"));

maplecl::Option<bool> oFforwardPropagate({"-fforward-propagate"},
    "  -fforward-propagate             \tPerform a forward propagation pass on RTL.\n",
    {driverCategory, unSupCategory});

maplecl::Option<std::string> oFfpContract ({"-ffp-contract="},
    "  -ffp-contract=             \tPerform floating-point expression contraction.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oFfreestanding({"-ffreestanding"},
    "  -ffreestanding             \tDo not assume that standard C libraries and 'main' "
    "exist.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-freestanding"));

maplecl::Option<bool> oFfriendInjection({"-ffriend-injection"},
    "  -ffriend-injection             \tInject friend functions into enclosing namespace.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-friend-injection"));

maplecl::Option<bool> oFgcse({"-fgcse"},
    "  -fgcse             \tPerform global common subexpression elimination.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-gcse"));

maplecl::Option<bool> oFgcseAfterReload({"-fgcse-after-reload"},
    "  -fgcse-after-reload             \t-fgcse-after-reload\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-gcse-after-reload"));

maplecl::Option<bool> oFgcseLas({"-fgcse-las"},
    "  -fgcse-las             \tPerform redundant load after store elimination in global "
    "common subexpression elimination.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-gcse-las"));

maplecl::Option<bool> oFgcseLm({"-fgcse-lm"},
    "  -fgcse-lm             \tPerform enhanced load motion during global common "
    "subexpression elimination.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-gcse-lm"));

maplecl::Option<bool> oFgcseSm({"-fgcse-sm"},
    "  -fgcse-sm             \tPerform store motion after global common subexpression "
    "elimination.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-gcse-sm"));

maplecl::Option<bool> oFgimple({"-fgimple"},
    "  -fgimple             \tEnable parsing GIMPLE.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-gimple"));

maplecl::Option<bool> oFgnuRuntime({"-fgnu-runtime"},
    "  -fgnu-runtime             \tGenerate code for GNU runtime environment.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-gnu-runtime"));

maplecl::Option<bool> oFgnuTm({"-fgnu-tm"},
    "  -fgnu-tm             \tEnable support for GNU transactional memory.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-gnu-tm"));

maplecl::Option<bool> oFgnu89Inline({"-fgnu89-inline"},
    "  -fgnu89-inline             \tUse traditional GNU semantics for inline functions.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-gnu89-inline"));

maplecl::Option<bool> oFgraphiteIdentity({"-fgraphite-identity"},
    "  -fgraphite-identity             \tEnable Graphite Identity transformation.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-graphite-identity"));

maplecl::Option<bool> oFhoistAdjacentLoads({"-fhoist-adjacent-loads"},
    "  -fhoist-adjacent-loads             \tEnable hoisting adjacent loads to encourage "
    "generating conditional move instructions.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-hoist-adjacent-loads"));

maplecl::Option<bool> oFhosted({"-fhosted"},
    "  -fhosted             \tAssume normal C execution environment.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-hosted"));

maplecl::Option<bool> oFifConversion({"-fif-conversion"},
    "  -fif-conversion             \tPerform conversion of conditional jumps to "
    "branchless equivalents.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-if-conversion"));

maplecl::Option<bool> oFifConversion2({"-fif-conversion2"},
    "  -fif-conversion2             \tPerform conversion of conditional "
    "jumps to conditional execution.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-if-conversion2"));

maplecl::Option<bool> oFilelist({"-filelist"},
    "  -filelist             \t\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oFindirectData({"-findirect-data"},
    "  -findirect-data             \tGenerate code suitable for fast turnaround "
    "development, such as to allow GDB to dynamically load .o files into "
    "already-running programs\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-indirect-data"));

maplecl::Option<bool> oFindirectInlining({"-findirect-inlining"},
    "  -findirect-inlining             \tPerform indirect inlining.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-indirect-inlining"));

maplecl::Option<bool> oFinhibitSizeDirective({"-finhibit-size-directive"},
    "  -finhibit-size-directive             \tDo not generate .size directives.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-inhibit-size-directive"));

maplecl::Option<bool> oFinlineFunctions({"-finline-functions"},
    "  -finline-functions             \tIntegrate functions not declared 'inline' "
    "into their callers when profitable.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-inline-functions"));

maplecl::Option<bool> oFinlineFunctionsCalledOnce({"-finline-functions-called-once"},
    "  -finline-functions-called-once             \tIntegrate functions only required "
    "by their single caller.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-inline-functions-called-once"));

maplecl::Option<std::string> oFinlineLimit({"-finline-limit-"},
    "  -finline-limit-             \t\n",
    {driverCategory, unSupCategory});

maplecl::Option<std::string> oFinlineLimitE({"-finline-limit="},
    "  -finline-limit=             \tLimit the size of inlined functions to <number>.\n",
    {driverCategory, unSupCategory});

maplecl::Option<std::string> oFinlineMatmulLimitE({"-finline-matmul-limit="},
    "  -finline-matmul-limit=             \tecify the size of the largest matrix for "
    "which matmul will be inlined.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oFinlineSmallFunctions({"-finline-small-functions"},
    "  -finline-small-functions             \tIntegrate functions into their callers"
    " when code size is known not to grow.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-inline-small-functions"));

maplecl::Option<std::string> oFinputCharset({"-finput-charset="},
    "  -finput-charset=             \tSpecify the default character set for source files.\n",
    {driverCategory, clangCategory});

maplecl::Option<bool> oFinstrumentFunctions({"-finstrument-functions"},
    "  -finstrument-functions             \tInstrument function entry and exit with "
    "profiling calls.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-instrument-functions"));

maplecl::Option<std::string> oFinstrumentFunctionsExcludeFileList({"-finstrument-functions-exclude-file-list="},
    "  -finstrument-functions-exclude-file-list=             \t Do not instrument functions "
    "listed in files.\n",
    {driverCategory, unSupCategory});

maplecl::Option<std::string> oFinstrumentFunctionsExcludeFunctionList({"-finstrument-functions-exclude-function-list="},
    "  -finstrument-functions-exclude-function-list=             \tDo not instrument "
    "listed functions.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oFipaCp({"-fipa-cp"},
    "  -fipa-cp             \tPerform interprocedural constant propagation.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-ipa-cp"));

maplecl::Option<bool> oFipaCpClone({"-fipa-cp-clone"},
    "  -fipa-cp-clone             \tPerform cloning to make Interprocedural constant "
    "propagation stronger.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-ipa-cp-clone"));

maplecl::Option<bool> oFipaIcf({"-fipa-icf"},
    "  -fipa-icf             \tPerform Identical Code Folding for functions and "
    "read-only variables.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-ipa-icf"));

maplecl::Option<bool> oFipaProfile({"-fipa-profile"},
    "  -fipa-profile             \tPerform interprocedural profile propagation.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-ipa-profile"));

maplecl::Option<bool> oFipaPta({"-fipa-pta"},
    "  -fipa-pta             \tPerform interprocedural points-to analysis.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-ipa-pta"));

maplecl::Option<bool> oFipaPureConst({"-fipa-pure-const"},
    "  -fipa-pure-const             \tDiscover pure and const functions.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-ipa-pure-const"));

maplecl::Option<bool> oFipaRa({"-fipa-ra"},
    "  -fipa-ra             \tUse caller save register across calls if possible.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-ipa-ra"));

maplecl::Option<bool> oFipaReference({"-fipa-reference"},
    "  -fipa-reference             \tDiscover readonly and non addressable static "
    "variables.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-ipa-reference"));

maplecl::Option<bool> oFipaSra({"-fipa-sra"},
    "  -fipa-sra             \tPerform interprocedural reduction of aggregates.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-ipa-sra"));

maplecl::Option<std::string> oFiraAlgorithmE({"-fira-algorithm="},
    "  -fira-algorithm=             \tSet the used IRA algorithm.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oFiraHoistPressure({"-fira-hoist-pressure"},
    "  -fira-hoist-pressure             \tUse IRA based register pressure calculation in "
    "RTL hoist optimizations.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-ira-hoist-pressure"));

maplecl::Option<bool> oFiraLoopPressure({"-fira-loop-pressure"},
    "  -fira-loop-pressure             \tUse IRA based register pressure calculation in "
    "RTL loop optimizations.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-ira-loop-pressure"));

maplecl::Option<std::string> oFiraRegion({"-fira-region="},
    "  -fira-region=             \tSet regions for IRA.\n",
    {driverCategory, unSupCategory});

maplecl::Option<std::string> oFiraVerbose({"-fira-verbose="},
    "  -fira-verbose=             \t	Control IRA's level of diagnostic messages.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oFisolateErroneousPathsAttribute({"-fisolate-erroneous-paths-attribute"},
    "  -fisolate-erroneous-paths-attribute             \tDetect paths that trigger "
    "erroneous or undefined behavior due to a null value being used in a way forbidden "
    "by a returns_nonnull or nonnull attribute.  Isolate those paths from the main control "
    "flow and turn the statement with erroneous or undefined behavior into a trap.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-isolate-erroneous-paths-attribute"));

maplecl::Option<bool> oFisolateErroneousPathsDereference({"-fisolate-erroneous-paths-dereference"},
    "  -fisolate-erroneous-paths-dereference             \tDetect paths that trigger "
    "erroneous or undefined behavior due to dereferencing a null pointer.  Isolate those "
    "paths from the main control flow and turn the statement with erroneous or undefined "
    "behavior into a trap.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-isolate-erroneous-paths-dereference"));

maplecl::Option<std::string> oFivarVisibility({"-fivar-visibility="},
    "  -fivar-visibility=             \tSet the default symbol visibility.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oFivopts({"-fivopts"},
    "  -fivopts             \tOptimize induction variables on trees.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-ivopts"));

maplecl::Option<bool> oFkeepInlineFunctions({"-fkeep-inline-functions"},
    "  -fkeep-inline-functions             \tGenerate code for functions even if they "
    "are fully inlined.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-keep-inline-functions"));

maplecl::Option<bool> oFkeepStaticConsts({"-fkeep-static-consts"},
    "  -fkeep-static-consts             \tEmit static const variables even if they are"
    " not used.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-keep-static-consts"));

maplecl::Option<bool> oFkeepStaticFunctions({"-fkeep-static-functions"},
    "  -fkeep-static-functions             \tGenerate code for static functions even "
    "if they are never called.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-keep-static-functions"));

maplecl::Option<bool> oFlat_namespace({"-flat_namespace"},
    "  -flat_namespace             \t\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-lat_namespace"));

maplecl::Option<bool> oFlaxVectorConversions({"-flax-vector-conversions"},
    "  -flax-vector-conversions             \tAllow implicit conversions between vectors "
    "with differing numbers of subparts and/or differing element types.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-lax-vector-conversions"));

maplecl::Option<bool> oFleadingUnderscore({"-fleading-underscore"},
    "  -fleading-underscore             \tGive external symbols a leading underscore.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-leading-underscore"));

maplecl::Option<std::string> oFliveRangeShrinkage({"-flive-range-shrinkage"},
    "  -flive-range-shrinkage             \tRelief of register pressure through live range "
    "shrinkage\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-live-range-shrinkage"));

maplecl::Option<bool> oFlocalIvars({"-flocal-ivars"},
    "  -flocal-ivars             \tAllow access to instance variables as if they were local"
    " declarations within instance method implementations.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-local-ivars"));

maplecl::Option<bool> oFloopBlock({"-floop-block"},
    "  -floop-block             \tEnable loop nest transforms.  Same as "
    "-floop-nest-optimize.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-loop-block"));

maplecl::Option<bool> oFloopInterchange({"-floop-interchange"},
    "  -floop-interchange             \tEnable loop nest transforms.  Same as "
    "-floop-nest-optimize.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-loop-interchange"));

maplecl::Option<bool> oFloopNestOptimize({"-floop-nest-optimize"},
    "  -floop-nest-optimize             \tEnable the loop nest optimizer.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-loop-nest-optimize"));

maplecl::Option<bool> oFloopParallelizeAll({"-floop-parallelize-all"},
    "  -floop-parallelize-all             \tMark all loops as parallel.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-loop-parallelize-all"));

maplecl::Option<bool> oFloopStripMine({"-floop-strip-mine"},
    "  -floop-strip-mine             \tEnable loop nest transforms. "
    "Same as -floop-nest-optimize.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-loop-strip-mine"));

maplecl::Option<bool> oFloopUnrollAndJam({"-floop-unroll-and-jam"},
    "  -floop-unroll-and-jam             \tEnable loop nest transforms. "
    "Same as -floop-nest-optimize.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-loop-unroll-and-jam"));

maplecl::Option<bool> oFlraRemat({"-flra-remat"},
    "  -flra-remat             \tDo CFG-sensitive rematerialization in LRA.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-lra-remat"));

maplecl::Option<std::string> oFltoCompressionLevel({"-flto-compression-level="},
    "  -flto-compression-level=             \tUse zlib compression level <number> ofor IL.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oFltoOdrTypeMerging({"-flto-odr-type-merging"},
    "  -flto-odr-type-merging             \tMerge C++ types using One Definition Rule.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-lto-odr-type-merging"));

maplecl::Option<std::string> oFltoPartition({"-flto-partition="},
    "  -flto-partition=             \tSpecify the algorithm to partition symbols and "
    "vars at linktime.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oFltoReport({"-flto-report"},
    "  -flto-report             \tReport various link-time optimization statistics.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-lto-report"));

maplecl::Option<bool> oFltoReportWpa({"-flto-report-wpa"},
    "  -flto-report-wpa             \tReport various link-time optimization statistics "
    "for WPA only.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-lto-report-wpa"));

maplecl::Option<bool> oFmemReport({"-fmem-report"},
    "  -fmem-report             \tReport on permanent memory allocation.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-mem-report"));

maplecl::Option<bool> oFmemReportWpa({"-fmem-report-wpa"},
    "  -fmem-report-wpa             \tReport on permanent memory allocation in WPA only.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-mem-report-wpa"));

maplecl::Option<bool> oFmergeAllConstants({"-fmerge-all-constants"},
    "  -fmerge-all-constants             \tAttempt to merge identical constants and "
    "constantvariables.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-merge-all-constants"));

maplecl::Option<bool> oFmergeConstants({"-fmerge-constants"},
    "  -fmerge-constants             \tAttempt to merge identical constants across "
    "compilation units.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-merge-constants"));

maplecl::Option<bool> oFmergeDebugStrings({"-fmerge-debug-strings"},
    "  -fmerge-debug-strings             \tAttempt to merge identical debug strings "
    "across compilation units.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-merge-debug-strings"));

maplecl::Option<std::string> oFmessageLength({"-fmessage-length="},
    "  -fmessage-length=             \t-fmessage-length=<number> o    Limit diagnostics to "
    "<number> ocharacters per line.  0 suppresses line-wrapping.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oFmoduloSched({"-fmodulo-sched"},
    "  -fmodulo-sched             \tPerform SMS based modulo scheduling before the first "
    "scheduling pass.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-modulo-sched"));

maplecl::Option<bool> oFmoduloSchedAllowRegmoves({"-fmodulo-sched-allow-regmoves"},
    "  -fmodulo-sched-allow-regmoves             \tPerform SMS based modulo scheduling with "
    "register moves allowed.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-modulo-sched-allow-regmoves"));

maplecl::Option<bool> oFmoveLoopInvariants({"-fmove-loop-invariants"},
    "  -fmove-loop-invariants             \tMove loop invariant computations "
    "out of loops.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-move-loop-invariants"));

maplecl::Option<bool> oFmsExtensions({"-fms-extensions"},
    "  -fms-extensions             \tDon't warn about uses of Microsoft extensions.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-ms-extensions"));

maplecl::Option<bool> oFnewInheritingCtors({"-fnew-inheriting-ctors"},
    "  -fnew-inheriting-ctors             \tImplement C++17 inheriting constructor "
    "semantics.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-new-inheriting-ctors"));

maplecl::Option<bool> oFnewTtpMatching({"-fnew-ttp-matching"},
    "  -fnew-ttp-matching             \tImplement resolution of DR 150 for matching of "
    "template template arguments.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-new-ttp-matching"));

maplecl::Option<bool> oFnextRuntime({"-fnext-runtime"},
    "  -fnext-runtime             \tGenerate code for NeXT (Apple Mac OS X) runtime "
    "environment.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oFnoAccessControl({"-fno-access-control"},
    "  -fno-access-control             \tTurn off all access checking.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oFnoAsm({"-fno-asm"},
    "  -fno-asm             \tDo not recognize asm, inline or typeof as a keyword, "
    "so that code can use these words as identifiers. \n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oFnoBranchCountReg({"-fno-branch-count-reg"},
    "  -fno-branch-count-reg             \tAvoid running a pass scanning for opportunities"
    " to use “decrement and branch” instructions on a count register instead of generating "
    "sequences of instructions that decrement a register, compare it against zero, and then"
    " branch based upon the result.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oFnoBuiltin({"-fno-builtin", "-fno-builtin-function"},
    "  -fno-builtin             \tDon't recognize built-in functions that do not begin "
    "with '__builtin_' as prefix.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oFnoCanonicalSystemHeaders({"-fno-canonical-system-headers"},
    "  -fno-canonical-system-headers             \tWhen preprocessing, do not shorten "
    "system header paths with canonicalization.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oFCheckPointerBounds({"-fcheck-pointer-bounds"},
    "  -fcheck-pointer-bounds             \tEnable Pointer Bounds Checker "
    "instrumentation.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-check-pointer-bounds"));

maplecl::Option<bool> oFChecking({"-fchecking"},
    "  -fchecking             \tPerform internal consistency checkings.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-checking"));

maplecl::Option<std::string> oFCheckingE({"-fchecking="},
    "  -fchecking=             \tPerform internal consistency checkings.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oFChkpCheckIncompleteType({"-fchkp-check-incomplete-type"},
    "  -fchkp-check-incomplete-type             \tGenerate pointer bounds checks for "
    "variables with incomplete type.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-chkp-check-incomplete-type"));

maplecl::Option<bool> oFChkpCheckRead({"-fchkp-check-read"},
    "  -fchkp-check-read             \tGenerate checks for all read accesses to memory.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-chkp-check-read"));

maplecl::Option<bool> oFChkpCheckWrite({"-fchkp-check-write"},
    "  -fchkp-check-write             \tGenerate checks for all write accesses to memory.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-chkp-check-write"));

maplecl::Option<bool> oFChkpFirstFieldHasOwnBounds({"-fchkp-first-field-has-own-bounds"},
    "  -fchkp-first-field-has-own-bounds             \tForces Pointer Bounds Checker to "
    "use narrowed bounds for address of the first field in the structure.  By default "
    "pointer to the first field has the same bounds as pointer to the whole structure.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-chkp-first-field-has-own-bounds"));

maplecl::Option<bool> oFDefaultInline({"-fdefault-inline"},
    "  -fdefault-inline             \tDoes nothing.  Preserved for backward "
    "compatibility.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-default-inline"));

maplecl::Option<bool> oFdefaultInteger8({"-fdefault-integer-8"},
    "  -fdefault-integer-8             \tSet the default integer kind to an 8 byte "
    "wide type.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-default-integer-8"));

maplecl::Option<bool> oFdefaultReal8({"-fdefault-real-8"},
    "  -fdefault-real-8             \tSet the default real kind to an 8 byte wide type.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-default-real-8"));

maplecl::Option<bool> oFDeferPop({"-fdefer-pop"},
    "  -fdefer-pop             \tDefer popping functions args from stack until later.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-defer-pop"));

maplecl::Option<bool> oFElideConstructors({"-felide-constructors"},
    "  -felide-constructors             \t\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("fno-elide-constructors"));

maplecl::Option<bool> oFEnforceEhSpecs({"-fenforce-eh-specs"},
    "  -fenforce-eh-specs             \tGenerate code to check exception "
    "specifications.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-enforce-eh-specs"));

maplecl::Option<bool> oFFpIntBuiltinInexact({"-ffp-int-builtin-inexact"},
    "  -ffp-int-builtin-inexact             \tAllow built-in functions ceil, floor, "
    "round, trunc to raise 'inexact' exceptions.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-fp-int-builtin-inexact"));

maplecl::Option<bool> oFFunctionCse({"-ffunction-cse"},
    "  -ffunction-cse             \tAllow function addresses to be held in registers.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-function-cse"));

maplecl::Option<bool> oFGnuKeywords({"-fgnu-keywords"},
    "  -fgnu-keywords             \tRecognize GNU-defined keywords.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-gnu-keywords"));

maplecl::Option<bool> oFGnuUnique({"-fgnu-unique"},
    "  -fgnu-unique             \tUse STB_GNU_UNIQUE if supported by the assembler.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-gnu-unique"));

maplecl::Option<bool> oFGuessBranchProbability({"-fguess-branch-probability"},
    "  -fguess-branch-probability             \tEnable guessing of branch probabilities.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-guess-branch-probability"));

maplecl::Option<bool> oFIdent({"-fident"},
    "  -fident             \tProcess #ident directives.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-ident"));

maplecl::Option<bool> oFImplementInlines({"-fimplement-inlines"},
    "  -fimplement-inlines             \tExport functions even if they can be inlined.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-implement-inlines"));

maplecl::Option<bool> oFImplicitInlineTemplates({"-fimplicit-inline-templates"},
    "  -fimplicit-inline-templates             \tEmit implicit instantiations of inline "
    "templates.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-implicit-inline-templates"));

maplecl::Option<bool> oFImplicitTemplates({"-fimplicit-templates"},
    "  -fimplicit-templates             \tEmit implicit instantiations of templates.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("no-implicit-templates"));

maplecl::Option<bool> oFIraShareSaveSlots({"-fira-share-save-slots"},
    "  -fira-share-save-slots             \tShare slots for saving different hard "
    "registers.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-ira-share-save-slots"));

maplecl::Option<bool> oFIraShareSpillSlots({"-fira-share-spill-slots"},
    "  -fira-share-spill-slots             \tShare stack slots for spilled "
    "pseudo-registers.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-ira-share-spill-slots"));

maplecl::Option<bool> oFJumpTables({"-fjump-tables"},
    "  -fjump-tables             \tUse jump tables for sufficiently large "
    "switch statements.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-jump-tables"));

maplecl::Option<bool> oFKeepInlineDllexport({"-fkeep-inline-dllexport"},
    "  -fkeep-inline-dllexport             \tDon't emit dllexported inline functions "
    "unless needed.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-keep-inline-dllexport"));

maplecl::Option<bool> oFLifetimeDse({"-flifetime-dse"},
    "  -flifetime-dse             \tTell DSE that the storage for a C++ object is "
    "dead when the constructor starts and when the destructor finishes.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-lifetime-dse"));

maplecl::Option<bool> oFMathErrno({"-fmath-errno"},
    "  -fmath-errno             \tSet errno after built-in math functions.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-math-errno"));

maplecl::Option<bool> oFNilReceivers({"-fnil-receivers"},
    "  -fnil-receivers             \tAssume that receivers of Objective-C "
    "messages may be nil.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-nil-receivers"));

maplecl::Option<bool> oFNonansiBuiltins({"-fnonansi-builtins"},
    "  -fnonansi-builtins             \t\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-nonansi-builtins"));

maplecl::Option<bool> oFOperatorNames({"-foperator-names"},
    "  -foperator-names             \tRecognize C++ keywords like 'compl' and 'xor'.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-operator-names"));

maplecl::Option<bool> oFOptionalDiags({"-foptional-diags"},
    "  -foptional-diags             \t\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-optional-diags"));

maplecl::Option<bool> oFPeephole({"-fpeephole"},
    "  -fpeephole             \tEnable machine specific peephole optimizations.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-peephole"));

maplecl::Option<bool> oFPeephole2({"-fpeephole2"},
    "  -fpeephole2             \tEnable an RTL peephole pass before sched2.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-peephole2"));

maplecl::Option<bool> oFPrettyTemplates({"-fpretty-templates"},
    "  -fpretty-templates             \tpretty-print template specializations as "
    "the template signature followed by the arguments.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-pretty-templates"));

maplecl::Option<bool> oFPrintfReturnValue({"-fprintf-return-value"},
    "  -fprintf-return-value             \tTreat known sprintf return values as "
    "constants.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-printf-return-value"));

maplecl::Option<bool> oFRtti({"-frtti"},
    "  -frtti             \tGenerate run time type descriptor information.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-rtti"));

maplecl::Option<bool> oFnoSanitizeAll({"-fno-sanitize=all"},
    "  -fno-sanitize=all             \t\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oFSchedInterblock({"-fsched-interblock"},
    "  -fsched-interblock             \tEnable scheduling across basic blocks.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-sched-interblock"));

maplecl::Option<bool> oFSchedSpec({"-fsched-spec"},
    "  -fsched-spec             \tAllow speculative motion of non-loads.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-sched-spec"));

maplecl::Option<bool> oFnoSetStackExecutable({"-fno-set-stack-executable"},
    "  -fno-set-stack-executable             \t\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oFShowColumn({"-fshow-column"},
    "  -fshow-column             \tShow column numbers in diagnostics, "
    "when available.  Default on.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-show-column"));

maplecl::Option<bool> oFSignedZeros({"-fsigned-zeros"},
    "  -fsigned-zeros             \tDisable floating point optimizations that "
    "ignore the IEEE signedness of zero.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-signed-zeros"));

maplecl::Option<bool> oFStackLimit({"-fstack-limit"},
    "  -fstack-limit             \t\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-stack-limit"));

maplecl::Option<bool> oFThreadsafeStatics({"-fthreadsafe-statics"},
    "  -fthreadsafe-statics             \tDo not generate thread-safe code for "
    "initializing local statics.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-threadsafe-statics"));

maplecl::Option<bool> oFToplevelReorder({"-ftoplevel-reorder"},
    "  -ftoplevel-reorder             \tReorder top level functions, variables, and asms.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-toplevel-reorder"));

maplecl::Option<bool> oFTrappingMath({"-ftrapping-math"},
    "  -ftrapping-math             \tAssume floating-point operations can trap.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-trapping-math"));

maplecl::Option<bool> oFUseCxaGetExceptionPtr({"-fuse-cxa-get-exception-ptr"},
    "  -fuse-cxa-get-exception-ptr             \tUse __cxa_get_exception_ptr in "
    "exception handling.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-use-cxa-get-exception-ptr"));

maplecl::Option<bool> oFWeak({"-fweak"},
    "  -fweak             \tEmit common-like symbols as weak symbols.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-weak"));

maplecl::Option<bool> oFnoWritableRelocatedRdata({"-fno-writable-relocated-rdata"},
    "  -fno-writable-relocated-rdata             \t\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oFZeroInitializedInBss({"-fzero-initialized-in-bss"},
    "  -fzero-initialized-in-bss             \tPut zero initialized data in the"
    " bss section.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-zero-initialized-in-bss"));

maplecl::Option<bool> oFnonCallExceptions({"-fnon-call-exceptions"},
    "  -fnon-call-exceptions             \tSupport synchronous non-call exceptions.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-non-call-exceptions"));

maplecl::Option<bool> oFnothrowOpt({"-fnothrow-opt"},
    "  -fnothrow-opt             \tTreat a throw() exception specification as noexcept to "
    "improve code size.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-nothrow-opt"));

maplecl::Option<std::string> oFobjcAbiVersion({"-fobjc-abi-version="},
    "  -fobjc-abi-version=             \tSpecify which ABI to use for Objective-C "
    "family code and meta-data generation.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oFobjcCallCxxCdtors({"-fobjc-call-cxx-cdtors"},
    "  -fobjc-call-cxx-cdtors             \tGenerate special Objective-C methods to "
    "initialize/destroy non-POD C++ ivars\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-objc-call-cxx-cdtors"));

maplecl::Option<bool> oFobjcDirectDispatch({"-fobjc-direct-dispatch"},
    "  -fobjc-direct-dispatch             \tAllow fast jumps to the message dispatcher.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-objc-direct-dispatch"));

maplecl::Option<bool> oFobjcExceptions({"-fobjc-exceptions"},
    "  -fobjc-exceptions             \tEnable Objective-C exception and synchronization "
    "syntax.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-objc-exceptions"));

maplecl::Option<bool> oFobjcGc({"-fobjc-gc"},
    "  -fobjc-gc             \tEnable garbage collection (GC) in Objective-C/Objective-C++ "
    "programs.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-objc-gc"));

maplecl::Option<bool> oFobjcNilcheck({"-fobjc-nilcheck"},
    "  -fobjc-nilcheck             \tEnable inline checks for nil receivers with the NeXT "
    "runtime and ABI version 2.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-fobjc-nilcheck"));

maplecl::Option<bool> oFobjcSjljExceptions({"-fobjc-sjlj-exceptions"},
    "  -fobjc-sjlj-exceptions             \tEnable Objective-C setjmp exception "
    "handling runtime.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-objc-sjlj-exceptions"));

maplecl::Option<bool> oFobjcStd({"-fobjc-std=objc1"},
    "  -fobjc-std             \tConform to the Objective-C 1.0 language as "
    "implemented in GCC 4.0.\n",
    {driverCategory, unSupCategory});

maplecl::Option<std::string> oFoffloadAbi({"-foffload-abi="},
    "  -foffload-abi=             \tSet the ABI to use in an offload compiler.\n",
    {driverCategory, unSupCategory});

maplecl::Option<std::string> oFoffload({"-foffload="},
    "  -foffload=             \tSpecify offloading targets and options for them.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oFopenacc({"-fopenacc"},
    "  -fopenacc             \tEnable OpenACC.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-openacc"));

maplecl::Option<std::string> oFopenaccDim({"-fopenacc-dim="},
    "  -fopenacc-dim=             \tSpecify default OpenACC compute dimensions.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oFopenmp({"-fopenmp"},
    "  -fopenmp             \tEnable OpenMP (implies -frecursive in Fortran).\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-openmp"));

maplecl::Option<bool> oFopenmpSimd({"-fopenmp-simd"},
    "  -fopenmp-simd             \tEnable OpenMP's SIMD directives.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-openmp-simd"));

maplecl::Option<bool> oFoptInfo({"-fopt-info"},
    "  -fopt-info             \tEnable all optimization info dumps on stderr.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-opt-info"));

maplecl::Option<bool> oFoptimizeStrlen({"-foptimize-strlen"},
    "  -foptimize-strlen             \tEnable string length optimizations on trees.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-foptimize-strlen"));

maplecl::Option<bool> oForce_cpusubtype_ALL({"-force_cpusubtype_ALL"},
    "  -force_cpusubtype_ALL             \tThis causes GCC's output file to have the "
    "'ALL' subtype, instead of one controlled by the -mcpu or -march option.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oForce_flat_namespace({"-force_flat_namespace"},
    "  -force_flat_namespace             \t\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oFpackStruct({"-fpack-struct"},
    "  -fpack-struct             \tPack structure members together without holes.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-pack-struct"));

maplecl::Option<bool> oFpartialInlining({"-fpartial-inlining"},
    "  -fpartial-inlining             \tPerform partial inlining.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-partial-inlining"));

maplecl::Option<bool> oFpccStructReturn({"-fpcc-struct-return"},
    "  -fpcc-struct-return             \tReturn small aggregates in memory\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-pcc-struct-return"));

maplecl::Option<bool> oFpchDeps({"-fpch-deps"},
    "  -fpch-deps             \tWhen using precompiled headers (see Precompiled Headers), "
    "this flag causes the dependency-output flags to also list the files from the "
    "precompiled header's dependencies.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-pch-deps"));

maplecl::Option<bool> oFpchPreprocess({"-fpch-preprocess"},
    "  -fpch-preprocess             \tLook for and use PCH files even when preprocessing.\n",
    {driverCategory, clangCategory});

maplecl::Option<bool> oFnoPchPreprocess({"-fno-pch-preprocess"},
    "  -fno-pch-preprocess            \tLook for and use PCH files even when preprocessing.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oFpeelLoops({"-fpeel-loops"},
    "  -fpeel-loops             \tPerform loop peeling.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-peel-loops"));

maplecl::Option<bool> oFpermissive({"-fpermissive"},
    "  -fpermissive             \tDowngrade conformance errors to warnings.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-permissive"));

maplecl::Option<std::string> oFpermittedFltEvalMethods({"-fpermitted-flt-eval-methods="},
    "  -fpermitted-flt-eval-methods=             \tSpecify which values of FLT_EVAL_METHOD"
    " are permitted.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oFplan9Extensions({"-fplan9-extensions"},
    "  -fplan9-extensions             \tEnable Plan 9 language extensions.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-fplan9-extensions"));

maplecl::Option<std::string> oFplugin({"-fplugin="},
    "  -fplugin=             \tSpecify a plugin to load.\n",
    {driverCategory, unSupCategory});

maplecl::Option<std::string> oFpluginArg({"-fplugin-arg-"},
    "  -fplugin-arg-             \tSpecify argument <key>=<value> ofor plugin <name>.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oFpostIpaMemReport({"-fpost-ipa-mem-report"},
    "  -fpost-ipa-mem-report             \tReport on memory allocation before "
    "interprocedural optimization.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-post-ipa-mem-report"));

maplecl::Option<bool> oFpreIpaMemReport({"-fpre-ipa-mem-report"},
    "  -fpre-ipa-mem-report             \tReport on memory allocation before "
    "interprocedural optimization.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-pre-ipa-mem-report"));

maplecl::Option<bool> oFpredictiveCommoning({"-fpredictive-commoning"},
    "  -fpredictive-commoning             \tRun predictive commoning optimization.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-fpredictive-commoning"));

maplecl::Option<bool> oFprefetchLoopArrays({"-fprefetch-loop-arrays"},
    "  -fprefetch-loop-arrays             \tGenerate prefetch instructions\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-prefetch-loop-arrays"));

maplecl::Option<bool> oFpreprocessed({"-fpreprocessed"},
    "  -fpreprocessed             \tTreat the input file as already preprocessed.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-preprocessed"));

maplecl::Option<bool> oFprofileArcs({"-fprofile-arcs"},
    "  -fprofile-arcs             \tInsert arc-based program profiling code.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-profile-arcs"));

maplecl::Option<bool> oFprofileCorrection({"-fprofile-correction"},
    "  -fprofile-correction             \tEnable correction of flow inconsistent profile "
    "data input.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-profile-correction"));

maplecl::Option<std::string> oFprofileDir({"-fprofile-dir="},
    "  -fprofile-dir=             \tSet the top-level directory for storing the profile "
    "data. The default is 'pwd'.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oFprofileGenerate({"-fprofile-generate"},
    "  -fprofile-generate             \tEnable common options for generating profile "
    "info for profile feedback directed optimizations.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-profile-generate"));

maplecl::Option<bool> oFprofileReorderFunctions({"-fprofile-reorder-functions"},
    "  -fprofile-reorder-functions             \tEnable function reordering that "
    "improves code placement.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-profile-reorder-functions"));

maplecl::Option<bool> oFprofileReport({"-fprofile-report"},
    "  -fprofile-report             \tReport on consistency of profile.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-profile-report"));

maplecl::Option<std::string> oFprofileUpdate({"-fprofile-update="},
    "  -fprofile-update=             \tSet the profile update method.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oFprofileUse({"-fprofile-use"},
    "  -fprofile-use             \tEnable common options for performing profile feedback "
    "directed optimizations.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-profile-use"));

maplecl::Option<std::string> oFprofileUseE({"-fprofile-use="},
    "  -fprofile-use=             \tEnable common options for performing profile feedback "
    "directed optimizations, and set -fprofile-dir=.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oFprofileValues({"-fprofile-values"},
    "  -fprofile-values             \tInsert code to profile values of expressions.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-profile-values"));

maplecl::Option<bool> oFpu({"-fpu"},
    "  -fpu             \tEnables (-fpu) or disables (-nofpu) the use of RX "
    "floating-point hardware. \n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-nofpu"));

maplecl::Option<bool> oFrandomSeed({"-frandom-seed"},
    "  -frandom-seed             \tMake compile reproducible using <string>.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-random-seed"));

maplecl::Option<std::string> oFrandomSeedE({"-frandom-seed="},
    "  -frandom-seed=             \tMake compile reproducible using <string>.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oFreciprocalMath({"-freciprocal-math"},
    "  -freciprocal-math             \tSame as -fassociative-math for expressions which "
    "include division.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-reciprocal-math"));

maplecl::Option<bool> oFrecordGccSwitches({"-frecord-gcc-switches"},
    "  -frecord-gcc-switches             \tRecord gcc command line switches in the "
    "object file.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-record-gcc-switches"));

maplecl::Option<bool> oFree({"-free"},
    "  -free             \tTurn on Redundant Extensions Elimination pass.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-ree"));

maplecl::Option<bool> oFrenameRegisters({"-frename-registers"},
    "  -frename-registers             \tPerform a register renaming optimization pass.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-rename-registers"));

maplecl::Option<bool> oFreorderBlocks({"-freorder-blocks"},
    "  -freorder-blocks             \tReorder basic blocks to improve code placement.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-reorder-blocks"));

maplecl::Option<std::string> oFreorderBlocksAlgorithm({"-freorder-blocks-algorithm="},
    "  -freorder-blocks-algorithm=             \tSet the used basic block reordering "
    "algorithm.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oFreorderBlocksAndPartition({"-freorder-blocks-and-partition"},
    "  -freorder-blocks-and-partition             \tReorder basic blocks and partition into "
    "hot and cold sections.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-reorder-blocks-and-partition"));

maplecl::Option<bool> oFreorderFunctions({"-freorder-functions"},
    "  -freorder-functions             \tReorder functions to improve code placement.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-reorder-functions"));

maplecl::Option<bool> oFreplaceObjcClasses({"-freplace-objc-classes"},
    "  -freplace-objc-classes             \tUsed in Fix-and-Continue mode to indicate that"
    " object files may be swapped in at runtime.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-replace-objc-classes"));

maplecl::Option<bool> oFrepo({"-frepo"},
    "  -frepo             \tEnable automatic template instantiation.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-repo"));

maplecl::Option<bool> oFreportBug({"-freport-bug"},
    "  -freport-bug             \tCollect and dump debug information into temporary file "
    "if ICE in C/C++ compiler occurred.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-report-bug"));

maplecl::Option<bool> oFrerunCseAfterLoop({"-frerun-cse-after-loop"},
    "  -frerun-cse-after-loop             \tAdd a common subexpression elimination pass "
    "after loop optimizations.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-rerun-cse-after-loop"));

maplecl::Option<bool> oFrescheduleModuloScheduledLoops({"-freschedule-modulo-scheduled-loops"},
    "  -freschedule-modulo-scheduled-loops             \tEnable/Disable the traditional "
    "scheduling in loops that already passed modulo scheduling.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-reschedule-modulo-scheduled-loops"));

maplecl::Option<bool> oFroundingMath({"-frounding-math"},
    "  -frounding-math             \tDisable optimizations that assume default FP "
    "rounding behavior.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-rounding-math"));

maplecl::Option<bool> oFsanitizeAddressUseAfterScope({"-fsanitize-address-use-after-scope"},
    "  -fsanitize-address-use-after-scope             \tEnable sanitization of local "
    "variables to detect use-after-scope bugs. \n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-sanitize-address-use-after-scope"));

maplecl::Option<bool> oFsanitizeCoverageTracePc({"-fsanitize-coverage=trace-pc"},
    "  -fsanitize-coverage=trace-pc             \tEnable coverage-guided fuzzing code i"
    "nstrumentation. Inserts call to __sanitizer_cov_trace_pc into every basic block.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-sanitize-coverage=trace-pc"));

maplecl::Option<bool> oFsanitizeRecover({"-fsanitize-recover"},
    "  -fsanitize-recover             \tAfter diagnosing undefined behavior attempt to "
    "continue execution.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-sanitize-recover"));

maplecl::Option<std::string> oFsanitizeRecoverE({"-fsanitize-recover="},
    "  -fsanitize-recover=             \tAfter diagnosing undefined behavior attempt to "
    "continue execution.\n",
    {driverCategory, unSupCategory});

maplecl::Option<std::string> oFsanitizeSections({"-fsanitize-sections="},
    "  -fsanitize-sections=             \tSanitize global variables in user-defined "
    "sections.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oFsanitizeUndefinedTrapOnError({"-fsanitize-undefined-trap-on-error"},
    "  -fsanitize-undefined-trap-on-error             \tUse trap instead of a library "
    "function for undefined behavior sanitization.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-sanitize-undefined-trap-on-error"));

maplecl::Option<std::string> oFsanitize({"-fsanitize"},
    "  -fsanitize             \tSelect what to sanitize.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-sanitize"));

maplecl::Option<bool> oFschedCriticalPathHeuristic({"-fsched-critical-path-heuristic"},
    "  -fsched-critical-path-heuristic             \tEnable the critical path heuristic "
    "in the scheduler.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-sched-critical-path-heuristic"));

maplecl::Option<bool> oFschedDepCountHeuristic({"-fsched-dep-count-heuristic"},
    "  -fsched-dep-count-heuristic             \tEnable the dependent count heuristic in "
    "the scheduler.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-sched-dep-count-heuristic"));

maplecl::Option<bool> oFschedGroupHeuristic({"-fsched-group-heuristic"},
    "  -fsched-group-heuristic             \tEnable the group heuristic in the scheduler.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-sched-group-heuristic"));

maplecl::Option<bool> oFschedLastInsnHeuristic({"-fsched-last-insn-heuristic"},
    "  -fsched-last-insn-heuristic             \tEnable the last instruction heuristic "
    "in the scheduler.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-sched-last-insn-heuristic"));

maplecl::Option<bool> oFschedPressure({"-fsched-pressure"},
    "  -fsched-pressure             \tEnable register pressure sensitive insn scheduling.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-sched-pressure"));

maplecl::Option<bool> oFschedRankHeuristic({"-fsched-rank-heuristic"},
    "  -fsched-rank-heuristic             \tEnable the rank heuristic in the scheduler.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-sched-rank-heuristic"));

maplecl::Option<bool> oFschedSpecInsnHeuristic({"-fsched-spec-insn-heuristic"},
    "  -fsched-spec-insn-heuristic             \tEnable the speculative instruction "
    "heuristic in the scheduler.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-sched-spec-insn-heuristic"));

maplecl::Option<bool> oFschedSpecLoad({"-fsched-spec-load"},
    "  -fsched-spec-load             \tAllow speculative motion of some loads.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-sched-spec-load"));

maplecl::Option<bool> oFschedSpecLoadDangerous({"-fsched-spec-load-dangerous"},
    "  -fsched-spec-load-dangerous             \tAllow speculative motion of more loads.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-sched-spec-load-dangerous"));

maplecl::Option<bool> oFschedStalledInsns({"-fsched-stalled-insns"},
    "  -fsched-stalled-insns             \tAllow premature scheduling of queued insns.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-sched-stalled-insns"));

maplecl::Option<bool> oFschedStalledInsnsDep({"-fsched-stalled-insns-dep"},
    "  -fsched-stalled-insns-dep             \tSet dependence distance checking in "
    "premature scheduling of queued insns.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-sched-stalled-insns-dep"));

maplecl::Option<bool> oFschedVerbose({"-fsched-verbose"},
    "  -fsched-verbose             \tSet the verbosity level of the scheduler.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-sched-verbose"));

maplecl::Option<bool> oFsched2UseSuperblocks({"-fsched2-use-superblocks"},
    "  -fsched2-use-superblocks             \tIf scheduling post reload\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-sched2-use-superblocks"));

maplecl::Option<bool> oFscheduleFusion({"-fschedule-fusion"},
    "  -fschedule-fusion             \tPerform a target dependent instruction fusion"
    " optimization pass.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-schedule-fusion"));

maplecl::Option<bool> oFscheduleInsns({"-fschedule-insns"},
    "  -fschedule-insns             \tReschedule instructions before register allocation.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-schedule-insns"));

maplecl::Option<bool> oFscheduleInsns2({"-fschedule-insns2"},
    "  -fschedule-insns2             \tReschedule instructions after register allocation.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-schedule-insns2"));

maplecl::Option<bool> oFsectionAnchors({"-fsection-anchors"},
    "  -fsection-anchors             \tAccess data in the same section from shared "
    "anchor points.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-fsection-anchors"));

maplecl::Option<bool> oFselSchedPipelining({"-fsel-sched-pipelining"},
    "  -fsel-sched-pipelining             \tPerform software pipelining of inner "
    "loops during selective scheduling.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-sel-sched-pipelining"));

maplecl::Option<bool> oFselSchedPipeliningOuterLoops({"-fsel-sched-pipelining-outer-loops"},
    "  -fsel-sched-pipelining-outer-loops             \tPerform software pipelining of "
    "outer loops during selective scheduling.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-sel-sched-pipelining-outer-loops"));

maplecl::Option<bool> oFselectiveScheduling({"-fselective-scheduling"},
    "  -fselective-scheduling             \tSchedule instructions using selective "
    "scheduling algorithm.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-selective-scheduling"));

maplecl::Option<bool> oFselectiveScheduling2({"-fselective-scheduling2"},
    "  -fselective-scheduling2             \tRun selective scheduling after reload.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-selective-scheduling2"));

maplecl::Option<bool> oFshortEnums({"-fshort-enums"},
    "  -fshort-enums             \tUse the narrowest integer type possible for "
    "enumeration types.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-short-enums"));

maplecl::Option<bool> oFshortWchar({"-fshort-wchar"},
    "  -fshort-wchar             \tForce the underlying type for 'wchar_t' to "
    "be 'unsigned short'.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-short-wchar"));

maplecl::Option<bool> oFshrinkWrap({"-fshrink-wrap"},
    "  -fshrink-wrap             \tEmit function prologues only before parts of the "
    "function that need it\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-shrink-wrap"));

maplecl::Option<bool> oFshrinkWrapSeparate({"-fshrink-wrap-separate"},
    "  -fshrink-wrap-separate             \tShrink-wrap parts of the prologue and "
    "epilogue separately.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-shrink-wrap-separate"));

maplecl::Option<bool> oFsignalingNans({"-fsignaling-nans"},
    "  -fsignaling-nans             \tDisable optimizations observable by IEEE "
    "signaling NaNs.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-signaling-nans"));

maplecl::Option<bool> oFsignedBitfields({"-fsigned-bitfields"},
    "  -fsigned-bitfields             \tWhen 'signed' or 'unsigned' is not given "
    "make the bitfield signed.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-signed-bitfields"));

maplecl::Option<bool> oFsimdCostModel({"-fsimd-cost-model"},
    "  -fsimd-cost-model             \t\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-simd-cost-model"));

maplecl::Option<bool> oFsinglePrecisionConstant({"-fsingle-precision-constant"},
    "  -fsingle-precision-constant             \tConvert floating point constants to "
    "single precision constants.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-single-precision-constant"));

maplecl::Option<bool> oFsizedDeallocation({"-fsized-deallocation"},
    "  -fsized-deallocation             \tEnable C++14 sized deallocation support.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-sized-deallocation"));

maplecl::Option<bool> oFsplitIvsInUnroller({"-fsplit-ivs-in-unroller"},
    "  -fsplit-ivs-in-unroller             \tSplit lifetimes of induction variables "
    "when loops are unrolled.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-split-ivs-in-unroller"));

maplecl::Option<bool> oFsplitLoops({"-fsplit-loops"},
    "  -fsplit-loops             \tPerform loop splitting.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-split-loops"));

maplecl::Option<bool> oFsplitPaths({"-fsplit-paths"},
    "  -fsplit-paths             \tSplit paths leading to loop backedges.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-split-paths"));

maplecl::Option<bool> oFsplitStack({"-fsplit-stack"},
    "  -fsplit-stack             \tGenerate discontiguous stack frames.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-split-stack"));

maplecl::Option<bool> oFsplitWideTypes({"-fsplit-wide-types"},
    "  -fsplit-wide-types             \tSplit wide types into independent registers.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-split-wide-types"));

maplecl::Option<bool> oFssaBackprop({"-fssa-backprop"},
    "  -fssa-backprop             \tEnable backward propagation of use properties at "
    "the SSA level.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-ssa-backprop"));

maplecl::Option<bool> oFssaPhiopt({"-fssa-phiopt"},
    "  -fssa-phiopt             \tOptimize conditional patterns using SSA PHI nodes.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-ssa-phiopt"));

maplecl::Option<bool> oFssoStruct({"-fsso-struct"},
    "  -fsso-struct             \tSet the default scalar storage order.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-sso-struct"));

maplecl::Option<bool> oFstackCheck({"-fstack-check"},
    "  -fstack-check             \tInsert stack checking code into the program.  Same "
    "as -fstack-check=specific.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-stack-check"));

maplecl::Option<std::string> oFstackCheckE({"-fstack-check="},
    "  -fstack-check=             \t-fstack-check=[no|generic|specific]	Insert stack "
    "checking code into the program.\n",
    {driverCategory, unSupCategory});

maplecl::Option<std::string> oFstackLimitRegister({"-fstack-limit-register="},
    "  -fstack-limit-register=             \tTrap if the stack goes past <register>\n",
    {driverCategory, unSupCategory});

maplecl::Option<std::string> oFstackLimitSymbol({"-fstack-limit-symbol="},
    "  -fstack-limit-symbol=             \tTrap if the stack goes past symbol <name>.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oFstackProtector({"-fstack-protector"},
    "  -fstack-protector             \tUse propolice as a stack protection method.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-stack-protector"));

maplecl::Option<bool> oFstackProtectorAll({"-fstack-protector-all"},
    "  -fstack-protector-all             \tUse a stack protection method for "
    "every function.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-stack-protector-all"));

maplecl::Option<bool> oFstackProtectorExplicit({"-fstack-protector-explicit"},
    "  -fstack-protector-explicit             \tUse stack protection method only for"
    " functions with the stack_protect attribute.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-stack-protector-explicit"));

maplecl::Option<bool> oFstackUsage({"-fstack-usage"},
    "  -fstack-usage             \tOutput stack usage information on a per-function "
    "basis.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-stack-usage"));

maplecl::Option<std::string> oFstack_reuse({"-fstack-reuse="},
    "  -fstack_reuse=             \tSet stack reuse level for local variables.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oFstats({"-fstats"},
    "  -fstats             \tDisplay statistics accumulated during compilation.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-stats"));

maplecl::Option<bool> oFstdargOpt({"-fstdarg-opt"},
    "  -fstdarg-opt             \tOptimize amount of stdarg registers saved to stack at "
    "start of function.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-stdarg-opt"));

maplecl::Option<bool> oFstoreMerging({"-fstore-merging"},
    "  -fstore-merging             \tMerge adjacent stores.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-store-merging"));

maplecl::Option<bool> oFstrictAliasing({"-fstrict-aliasing"},
    "  -fstrict-aliasing             \tAssume strict aliasing rules apply.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oFstrictEnums({"-fstrict-enums"},
    "  -fstrict-enums             \tAssume that values of enumeration type are always "
    "within the minimum range of that type.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-strict-enums"));

maplecl::Option<bool> oFstrictOverflow({"-fstrict-overflow"},
    "  -fstrict-overflow             \tTreat signed overflow as undefined.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-strict-overflow"));

maplecl::Option<bool> oFstrictVolatileBitfields({"-fstrict-volatile-bitfields"},
    "  -fstrict-volatile-bitfields             \tForce bitfield accesses to match their "
    "type width.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-strict-volatile-bitfields"));

maplecl::Option<bool> oFsyncLibcalls({"-fsync-libcalls"},
    "  -fsync-libcalls             \tImplement __atomic operations via libcalls to "
    "legacy __sync functions.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-sync-libcalls"));

maplecl::Option<bool> oFsyntaxOnly({"-fsyntax-only"},
    "  -fsyntax-only             \tCheck for syntax errors\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-syntax-only"));

maplecl::Option<std::string> oFtabstop({"-ftabstop="},
    "  -ftabstop=             \tSet the distance between tab stops.\n",
    {driverCategory, clangCategory});

maplecl::Option<std::string> oFtemplateBacktraceLimit({"-ftemplate-backtrace-limit="},
    "  -ftemplate-backtrace-limit=             \tSet the maximum number of template "
    "instantiation notes for a single warning or error to n.\n",
    {driverCategory, unSupCategory});

maplecl::Option<std::string> oFtemplateDepth({"-ftemplate-depth-"},
    "  -ftemplate-depth-             \t\n",
    {driverCategory, unSupCategory});

maplecl::Option<std::string> oFtemplateDepthE({"-ftemplate-depth="},
    "  -ftemplate-depth=             \tSpecify maximum template instantiation depth.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oFtestCoverage({"-ftest-coverage"},
    "  -ftest-coverage             \tCreate data files needed by \"gcov\".\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-test-coverage"));

maplecl::Option<bool> oFthreadJumps({"-fthread-jumps"},
    "  -fthread-jumps             \tPerform jump threading optimizations.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-thread-jumps"));

maplecl::Option<bool> oFtimeReport({"-ftime-report"},
    "  -ftime-report             \tReport the time taken by each compiler pass.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-time-report"));

maplecl::Option<bool> oFtimeReportDetails({"-ftime-report-details"},
    "  -ftime-report-details             \tRecord times taken by sub-phases separately.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-time-report-details"));

maplecl::Option<bool> oFtracer({"-ftracer"},
    "  -ftracer             \tPerform superblock formation via tail duplication.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-tracer"));

maplecl::Option<bool> oFtrackMacroExpansion({"-ftrack-macro-expansion"},
    "  -ftrack-macro-expansion             \tTrack locations of tokens across "
    "macro expansions.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-track-macro-expansion"));

maplecl::Option<std::string> oFtrackMacroExpansionE({"-ftrack-macro-expansion="},
    "  -ftrack-macro-expansion=             \tTrack locations of tokens across "
    "macro expansions.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oFtrampolines({"-ftrampolines"},
    "  -ftrampolines             \tFor targets that normally need trampolines for "
    "nested functions\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-trampolines"));

maplecl::Option<bool> oFtrapv({"-ftrapv"},
    "  -ftrapv             \tTrap for signed overflow in addition\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-trapv"));

maplecl::Option<bool> oFtreeBitCcp({"-ftree-bit-ccp"},
    "  -ftree-bit-ccp             \tEnable SSA-BIT-CCP optimization on trees.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-tree-bit-ccp"));

maplecl::Option<bool> oFtreeBuiltinCallDce({"-ftree-builtin-call-dce"},
    "  -ftree-builtin-call-dce             \tEnable conditional dead code elimination for"
    " builtin calls.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-tree-builtin-call-dce"));

maplecl::Option<bool> oFtreeCcp({"-ftree-ccp"},
    "  -ftree-ccp             \tEnable SSA-CCP optimization on trees.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-tree-ccp"));

maplecl::Option<bool> oFtreeCh({"-ftree-ch"},
    "  -ftree-ch             \tEnable loop header copying on trees.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-tree-ch"));

maplecl::Option<bool> oFtreeCoalesceVars({"-ftree-coalesce-vars"},
    "  -ftree-coalesce-vars             \tEnable SSA coalescing of user variables.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-tree-coalesce-vars"));

maplecl::Option<bool> oFtreeCopyProp({"-ftree-copy-prop"},
    "  -ftree-copy-prop             \tEnable copy propagation on trees.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-tree-copy-prop"));

maplecl::Option<bool> oFtreeDce({"-ftree-dce"},
    "  -ftree-dce             \tEnable SSA dead code elimination optimization on trees.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-tree-dce"));

maplecl::Option<bool> oFtreeDominatorOpts({"-ftree-dominator-opts"},
    "  -ftree-dominator-opts             \tEnable dominator optimizations.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-tree-dominator-opts"));

maplecl::Option<bool> oFtreeDse({"-ftree-dse"},
    "  -ftree-dse             \tEnable dead store elimination.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-tree-dse"));

maplecl::Option<bool> oFtreeForwprop({"-ftree-forwprop"},
    "  -ftree-forwprop             \tEnable forward propagation on trees.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-tree-forwprop"));

maplecl::Option<bool> oFtreeFre({"-ftree-fre"},
    "  -ftree-fre             \tEnable Full Redundancy Elimination (FRE) on trees.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-tree-fre"));

maplecl::Option<bool> oFtreeLoopDistributePatterns({"-ftree-loop-distribute-patterns"},
    "  -ftree-loop-distribute-patterns             \tEnable loop distribution for "
    "patterns transformed into a library call.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-tree-loop-distribute-patterns"));

maplecl::Option<bool> oFtreeLoopDistribution({"-ftree-loop-distribution"},
    "  -ftree-loop-distribution             \tEnable loop distribution on trees.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-tree-loop-distribution"));

maplecl::Option<bool> oFtreeLoopIfConvert({"-ftree-loop-if-convert"},
    "  -ftree-loop-if-convert             \tConvert conditional jumps in innermost loops "
    "to branchless equivalents.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-tree-loop-if-convert"));

maplecl::Option<bool> oFtreeLoopIm({"-ftree-loop-im"},
    "  -ftree-loop-im             \tEnable loop invariant motion on trees.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-tree-loop-im"));

maplecl::Option<bool> oFtreeLoopIvcanon({"-ftree-loop-ivcanon"},
    "  -ftree-loop-ivcanon             \tCreate canonical induction variables in loops.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-tree-loop-ivcanon"));

maplecl::Option<bool> oFtreeLoopLinear({"-ftree-loop-linear"},
    "  -ftree-loop-linear             \tEnable loop nest transforms.  Same as "
    "-floop-nest-optimize.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-tree-loop-linear"));

maplecl::Option<bool> oFtreeLoopOptimize({"-ftree-loop-optimize"},
    "  -ftree-loop-optimize             \tEnable loop optimizations on tree level.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-tree-loop-optimize"));

maplecl::Option<bool> oFtreeLoopVectorize({"-ftree-loop-vectorize"},
    "  -ftree-loop-vectorize             \tEnable loop vectorization on trees.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-tree-loop-vectorize"));

maplecl::Option<bool> oFtreeParallelizeLoops({"-ftree-parallelize-loops"},
    "  -ftree-parallelize-loops             \t\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-tree-parallelize-loops"));

maplecl::Option<bool> oFtreePartialPre({"-ftree-partial-pre"},
    "  -ftree-partial-pre             \tIn SSA-PRE optimization on trees\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-tree-partial-pre"));

maplecl::Option<bool> oFtreePhiprop({"-ftree-phiprop"},
    "  -ftree-phiprop             \tEnable hoisting loads from conditional pointers.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-tree-phiprop"));

maplecl::Option<bool> oFtreePre({"-ftree-pre"},
    "  -ftree-pre             \tEnable SSA-PRE optimization on trees.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-tree-pre"));

maplecl::Option<bool> oFtreePta({"-ftree-pta"},
    "  -ftree-pta             \tPerform function-local points-to analysis on trees.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-tree-pta"));

maplecl::Option<bool> oFtreeReassoc({"-ftree-reassoc"},
    "  -ftree-reassoc             \tEnable reassociation on tree level.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-tree-reassoc"));

maplecl::Option<bool> oFtreeSink({"-ftree-sink"},
    "  -ftree-sink             \tEnable SSA code sinking on trees.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-tree-sink"));

maplecl::Option<bool> oFtreeSlpVectorize({"-ftree-slp-vectorize"},
    "  -ftree-slp-vectorize             \tEnable basic block vectorization (SLP) "
    "on trees.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-tree-slp-vectorize"));

maplecl::Option<bool> oFtreeSlsr({"-ftree-slsr"},
    "  -ftree-slsr             \tPerform straight-line strength reduction.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-tree-slsr"));

maplecl::Option<bool> oFtreeSra({"-ftree-sra"},
    "  -ftree-sra             \tPerform scalar replacement of aggregates.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-tree-sra"));

maplecl::Option<bool> oFtreeSwitchConversion({"-ftree-switch-conversion"},
    "  -ftree-switch-conversion             \tPerform conversions of switch "
    "initializations.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-tree-switch-conversion"));

maplecl::Option<bool> oFtreeTailMerge({"-ftree-tail-merge"},
    "  -ftree-tail-merge             \tEnable tail merging on trees.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-tree-tail-merge"));

maplecl::Option<bool> oFtreeTer({"-ftree-ter"},
    "  -ftree-ter             \tReplace temporary expressions in the SSA->normal pass.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-tree-ter"));

maplecl::Option<bool> oFtreeVrp({"-ftree-vrp"},
    "  -ftree-vrp             \tPerform Value Range Propagation on trees.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-tree-vrp"));

maplecl::Option<bool> oFunconstrainedCommons({"-funconstrained-commons"},
    "  -funconstrained-commons             \tAssume common declarations may be "
    "overridden with ones with a larger trailing array.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-unconstrained-commons"));

maplecl::Option<bool> oFunitAtATime({"-funit-at-a-time"},
    "  -funit-at-a-time             \tCompile whole compilation unit at a time.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-unit-at-a-time"));

maplecl::Option<bool> oFunrollAllLoops({"-funroll-all-loops"},
    "  -funroll-all-loops             \tPerform loop unrolling for all loops.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-unroll-all-loops"));

maplecl::Option<bool> oFunrollLoops({"-funroll-loops"},
    "  -funroll-loops             \tPerform loop unrolling when iteration count is known.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-unroll-loops"));

maplecl::Option<bool> oFunsafeMathOptimizations({"-funsafe-math-optimizations"},
    "  -funsafe-math-optimizations             \tAllow math optimizations that may "
    "violate IEEE or ISO standards.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-unsafe-math-optimizations"));

maplecl::Option<bool> oFunsignedBitfields({"-funsigned-bitfields"},
    "  -funsigned-bitfields             \tWhen \"signed\" or \"unsigned\" is not given "
    "make the bitfield unsigned.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-unsigned-bitfields"));

maplecl::Option<bool> oFunswitchLoops({"-funswitch-loops"},
    "  -funswitch-loops             \tPerform loop unswitching.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-unswitch-loops"));

maplecl::Option<bool> oFunwindTables({"-funwind-tables"},
    "  -funwind-tables             \tJust generate unwind tables for exception handling.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-unwind-tables"));

maplecl::Option<bool> oFuseCxaAtexit({"-fuse-cxa-atexit"},
    "  -fuse-cxa-atexit             \tUse __cxa_atexit to register destructors.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-use-cxa-atexit"));

maplecl::Option<bool> oFuseLdBfd({"-fuse-ld=bfd"},
    "  -fuse-ld=bfd             \tUse the bfd linker instead of the default linker.\n",
    {driverCategory, ldCategory});

maplecl::Option<bool> oFuseLdGold({"-fuse-ld=gold"},
    "  -fuse-ld=gold             \tUse the gold linker instead of the default linker.\n",
    {driverCategory, ldCategory});

maplecl::Option<bool> oFuseLinkerPlugin({"-fuse-linker-plugin"},
    "  -fuse-linker-plugin             \tEnables the use of a linker plugin during "
    "link-time optimization.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-use-linker-plugin"));

maplecl::Option<bool> oFvarTracking({"-fvar-tracking"},
    "  -fvar-tracking             \tPerform variable tracking.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-var-tracking"));

maplecl::Option<bool> oFvarTrackingAssignments({"-fvar-tracking-assignments"},
    "  -fvar-tracking-assignments             \tPerform variable tracking by "
    "annotating assignments.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-var-tracking-assignments"));

maplecl::Option<bool> oFvarTrackingAssignmentsToggle({"-fvar-tracking-assignments-toggle"},
    "  -fvar-tracking-assignments-toggle             \tToggle -fvar-tracking-assignments.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-var-tracking-assignments-toggle"));

maplecl::Option<bool> oFvariableExpansionInUnroller({"-fvariable-expansion-in-unroller"},
    "  -fvariable-expansion-in-unroller             \tApply variable expansion when "
    "loops are unrolled.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-variable-expansion-in-unroller"));

maplecl::Option<bool> oFvectCostModel({"-fvect-cost-model"},
    "  -fvect-cost-model             \tEnables the dynamic vectorizer cost model. "
    "Preserved for backward compatibility.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-vect-cost-model"));

maplecl::Option<bool> oFverboseAsm({"-fverbose-asm"},
    "  -fverbose-asm             \tAdd extra commentary to assembler output.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-verbose-asm"));

maplecl::Option<bool> oFvisibilityInlinesHidden({"-fvisibility-inlines-hidden"},
    "  -fvisibility-inlines-hidden             \tMarks all inlined functions and methods"
    " as having hidden visibility.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-visibility-inlines-hidden"));

maplecl::Option<bool> oFvisibilityMsCompat({"-fvisibility-ms-compat"},
    "  -fvisibility-ms-compat             \tChanges visibility to match Microsoft Visual"
    " Studio by default.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-visibility-ms-compat"));

maplecl::Option<bool> oFvpt({"-fvpt"},
    "  -fvpt             \tUse expression value profiles in optimizations.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-vpt"));

maplecl::Option<bool> oFvtableVerify({"-fvtable-verify"},
    "  -fvtable-verify             \t\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-vtable-verify"));

maplecl::Option<bool> oFvtvCounts({"-fvtv-counts"},
    "  -fvtv-counts             \tOutput vtable verification counters.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-vtv-counts"));

maplecl::Option<bool> oFvtvDebug({"-fvtv-debug"},
    "  -fvtv-debug             \tOutput vtable verification pointer sets information.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-vtv-debug"));

maplecl::Option<bool> oFweb({"-fweb"},
    "  -fweb             \tConstruct webs and split unrelated uses of single variable.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-web"));

maplecl::Option<bool> oFwholeProgram({"-fwhole-program"},
    "  -fwhole-program             \tPerform whole program optimizations.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-whole-program"));

maplecl::Option<std::string> oFwideExecCharset({"-fwide-exec-charset="},
    "  -fwide-exec-charset=             \tConvert all wide strings and character "
    "constants to character set <cset>.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oMwarnDynamicstack({"-mwarn-dynamicstack"},
    "  -mwarn-dynamicstack             \tEmit a warning if the function calls alloca or "
    "uses dynamically-sized arrays.\n",
    {driverCategory, unSupCategory});

maplecl::Option<std::string> oMwarnFramesize({"-mwarn-framesize"},
    "  -mwarn-framesize             \tEmit a warning if the current function exceeds "
    "the given frame size.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oMwarnMcu({"-mwarn-mcu"},
    "  -mwarn-mcu             \tThis option enables or disables warnings about conflicts "
    "between the MCU name specified by the -mmcu option and the ISA set by the -mcpu"
    " option and/or the hardware multiply support set by the -mhwmult option.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("--mno-warn-mcu"));

maplecl::Option<bool> oMwarnMultipleFastInterrupts({"-mwarn-multiple-fast-interrupts"},
    "  -mwarn-multiple-fast-interrupts             \t\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("--mno-warn-multiple-fast-interrupts"));

maplecl::Option<bool> oMwarnReloc({"-mwarn-reloc"},
    "  -mwarn-reloc             \t-mwarn-reloc generates a warning instead.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-merror-reloc"));

maplecl::Option<bool> oMwideBitfields({"-mwide-bitfields"},
    "  -mwide-bitfields             \t\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("--mno-wide-bitfields"));

maplecl::Option<bool> oMwin32({"-mwin32"},
    "  -mwin32             \tThis option is available for Cygwin and MinGW targets.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oMwindows({"-mwindows"},
    "  -mwindows             \tThis option is available for MinGW targets.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oMwordRelocations({"-mword-relocations"},
    "  -mword-relocations             \tOnly generate absolute relocations on "
    "word-sized values (i.e. R_ARM_ABS32). \n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oMx32({"-mx32"},
    "  -mx32             \t\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oMxgot({"-mxgot"},
    "  -mxgot             \tLift (do not lift) the usual restrictions on the "
    "size of the global offset table.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("--mno-xgot"));

maplecl::Option<bool> oMxilinxFpu({"-mxilinx-fpu"},
    "  -mxilinx-fpu             \tPerform optimizations for the floating-point unit "
    "on Xilinx PPC 405/440.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oMxlBarrelShift({"-mxl-barrel-shift"},
    "  -mxl-barrel-shift             \t\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oMxlCompat({"-mxl-compat"},
    "  -mxl-compat             \tProduce code that conforms more closely to IBM XL "
    "compiler semantics when using AIX-compatible ABI. \n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("--mno-xl-compat"));

maplecl::Option<bool> oMxlFloatConvert({"-mxl-float-convert"},
    "  -mxl-float-convert             \t\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oMxlFloatSqrt({"-mxl-float-sqrt"},
    "  -mxl-float-sqrt             \t\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oMxlGpOpt({"-mxl-gp-opt"},
    "  -mxl-gp-opt             \t\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oMxlMultiplyHigh({"-mxl-multiply-high"},
    "  -mxl-multiply-high             \t\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oMxlPatternCompare({"-mxl-pattern-compare"},
    "  -mxl-pattern-compare             \t\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oMxlReorder({"-mxl-reorder"},
    "  -mxl-reorder             \t\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oMxlSoftDiv({"-mxl-soft-div"},
    "  -mxl-soft-div             \t\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oMxlSoftMul({"-mxl-soft-mul"},
    "  -mxl-soft-mul             \t\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oMxlStackCheck({"-mxl-stack-check"},
    "  -mxl-stack-check             \t\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oMxop({"-mxop"},
    "  -mxop             \tThese switches enable the use of instructions in the mxop.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oMxpa({"-mxpa"},
    "  -mxpa             \tUse the MIPS eXtended Physical Address (XPA) instructions.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("--mno-xpa"));

maplecl::Option<bool> oMxsave({"-mxsave"},
    "  -mxsave             \tThese switches enable the use of instructions in "
    "the mxsave.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oMxsavec({"-mxsavec"},
    "  -mxsavec             \tThese switches enable the use of instructions in "
    "the mxsavec.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oMxsaveopt({"-mxsaveopt"},
    "  -mxsaveopt             \tThese switches enable the use of instructions in "
    "the mxsaveopt.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oMxsaves({"-mxsaves"},
    "  -mxsaves             \tThese switches enable the use of instructions in "
    "the mxsaves.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oMxy({"-mxy"},
    "  -mxy             \tPassed down to the assembler to enable the XY memory "
    "extension. \n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oMyellowknife({"-myellowknife"},
    "  -myellowknife             \tOn embedded PowerPC systems, assume that the startup "
    "module is called crt0.o and the standard C libraries are libyk.a and libc.a.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oMzarch({"-mzarch"},
    "  -mzarch             \tWhen -mzarch is specified, generate code using the "
    "instructions available on z/Architecture.\n",
    {driverCategory, unSupCategory});

maplecl::Option<std::string> oMzda({"-mzda"},
    "  -mzda             \tPut static or global variables whose size is n bytes or less "
    "into the first 32 kilobytes of memory.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oMzdcbranch({"-mzdcbranch"},
    "  -mzdcbranch             \tAssume (do not assume) that zero displacement conditional"
    " branch instructions bt and bf are fast. \n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("--mno-zdcbranch"));

maplecl::Option<bool> oMzeroExtend({"-mzero-extend"},
    "  -mzero-extend             \tWhen reading data from memory in sizes shorter than "
    "64 bits, use zero-extending load instructions by default, rather than "
    "sign-extending ones.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("--mno-zero-extend"));

maplecl::Option<bool> oMzvector({"-mzvector"},
    "  -mzvector             \tThe -mzvector option enables vector language extensions and "
    "builtins using instructions available with the vector extension facility introduced "
    "with the IBM z13 machine generation. \n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("--mno-zvector"));

maplecl::Option<bool> oNo80387({"-no-80387"},
    "  -no-80387             \t\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oNoCanonicalPrefixes({"-no-canonical-prefixes"},
    "  -no-canonical-prefixes             \t\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oNoIntegratedCpp({"-no-integrated-cpp"},
    "  -no-integrated-cpp             \tPerform preprocessing as a separate pass before compilation.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oNoSysrootSuffix({"-no-sysroot-suffix"},
    "  -no-sysroot-suffix             \t\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oNoall_load({"-noall_load"},
    "  -noall_load             \t\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oNocpp({"-nocpp"},
    "  -nocpp             \tDisable preprocessing.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oNodefaultlibs({"-nodefaultlibs"},
    "  -nodefaultlibs             \tDo not use the standard system libraries when "
    "linking. \n",
    {driverCategory, ldCategory});

maplecl::Option<bool> oNodevicelib({"-nodevicelib"},
    "  -nodevicelib             \t\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oNofixprebinding({"-nofixprebinding"},
    "  -nofixprebinding             \t\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oNolibdld({"-nolibdld"},
    "  -nolibdld             \t\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oNomultidefs({"-nomultidefs"},
    "  -nomultidefs             \t\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oNonStatic({"-non-static"},
    "  -non-static             \t\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oNoprebind({"-noprebind"},
    "  -noprebind             \t\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oNoseglinkedit({"-noseglinkedit"},
    "  -noseglinkedit             \t\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oNostartfiles({"-nostartfiles"},
    "  -nostartfiles             \tDo not use the standard system startup files "
    "when linking. \n",
    {driverCategory, ldCategory});

maplecl::Option<bool> oNostdinc({"-nostdinc++"},
    "  -nostdinc++             \tDo not search standard system include directories for"
    " C++.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oNo_dead_strip_inits_and_terms({"-no_dead_strip_inits_and_terms"},
    "  -no_dead_strip_inits_and_terms             \t\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oOfast({"-Ofast"},
    "  -Ofast             \tOptimize for speed disregarding exact standards compliance.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oOg({"-Og"},
    "  -Og             \tOptimize for debugging experience rather than speed or size.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oP({"-p"},
    "  -p             \tEnable function profiling.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oLargeP({"-P"},
    "  -P             \tDo not generate #line directives.\n",
    {driverCategory, clangCategory});

maplecl::Option<bool> oPagezero_size({"-pagezero_size"},
    "  -pagezero_size             \t\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oParam({"--param"},
    "  --param <param>=<value>             \tSet parameter <param> to value.  See below for a complete "
    "list of parameters.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oPassExitCodes({"-pass-exit-codes"},
    "  -pass-exit-codes             \t\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oPedantic({"-pedantic"},
    "  -pedantic             \t\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oPedanticErrors({"-pedantic-errors"},
    "  -pedantic-errors             \tLike -pedantic but issue them as errors.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oPg({"-pg"},
    "  -pg             \t\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oPlt({"-plt"},
    "  -plt             \t\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oPrebind({"-prebind"},
    "  -prebind             \t\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oPrebind_all_twolevel_modules({"-prebind_all_twolevel_modules"},
    "  -prebind_all_twolevel_modules             \t\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oPrintFileName({"-print-file-name"},
    "  -print-file-name             \t\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oPrintLibgccFileName({"-print-libgcc-file-name"},
    "  -print-libgcc-file-name             \t\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oPrintMultiDirectory({"-print-multi-directory"},
    "  -print-multi-directory             \t\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oPrintMultiLib({"-print-multi-lib"},
    "  -print-multi-lib             \t\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oPrintMultiOsDirectory({"-print-multi-os-directory"},
    "  -print-multi-os-directory             \t\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oPrintMultiarch({"-print-multiarch"},
    "  -print-multiarch             \t\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oPrintObjcRuntimeInfo({"-print-objc-runtime-info"},
    "  -print-objc-runtime-info             \tGenerate C header of platform-specific "
    "features.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oPrintProgName({"-print-prog-name"},
    "  -print-prog-name             \t\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oPrintSearchDirs({"-print-search-dirs"},
    "  -print-search-dirs             \t\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oPrintSysroot({"-print-sysroot"},
    "  -print-sysroot             \t\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oPrintSysrootHeadersSuffix({"-print-sysroot-headers-suffix"},
    "  -print-sysroot-headers-suffix             \t\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oPrivate_bundle({"-private_bundle"},
    "  -private_bundle             \t\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oPthreads({"-pthreads"},
    "  -pthreads             \t\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oQ({"-Q"},
    "  -Q             \tMakes the compiler print out each function name as it is "
    "compiled, and print some statistics about each pass when it finishes.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oQn({"-Qn"},
    "  -Qn             \tRefrain from adding .ident directives to the output "
    "file (this is the default).\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oQy({"-Qy"},
    "  -Qy             \tIdentify the versions of each tool used by the compiler, "
    "in a .ident assembler directive in the output.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oRead_only_relocs({"-read_only_relocs"},
    "  -read_only_relocs             \t\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oRemap({"-remap"},
    "  -remap             \tRemap file names when including files.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oSectalign({"-sectalign"},
    "  -sectalign             \t\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oSectcreate({"-sectcreate"},
    "  -sectcreate             \t\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oSectobjectsymbols({"-sectobjectsymbols"},
    "  -sectobjectsymbols             \t\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oSectorder({"-sectorder"},
    "  -sectorder             \t\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oSeg1addr({"-seg1addr"},
    "  -seg1addr             \t\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oSegaddr({"-segaddr"},
    "  -segaddr             \t\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oSeglinkedit({"-seglinkedit"},
    "  -seglinkedit             \t\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oSegprot({"-segprot"},
    "  -segprot             \t\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oSegs_read_only_addr({"-segs_read_only_addr"},
    "  -segs_read_only_addr             \t\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oSegs_read_write_addr({"-segs_read_write_addr"},
    "  -segs_read_write_addr             \t\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oSeg_addr_table({"-seg_addr_table"},
    "  -seg_addr_table             \t\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oSeg_addr_table_filename({"-seg_addr_table_filename"},
    "  -seg_addr_table_filename             \t\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oSharedLibgcc({"-shared-libgcc"},
    "  -shared-libgcc             \tOn systems that provide libgcc as a shared library, "
    "these options force the use of either the shared or static version, respectively.\n",
    {driverCategory, ldCategory});

maplecl::Option<bool> oSim({"-sim"},
    "  -sim             \tThis option, recognized for the cris-axis-elf, arranges to "
    "link with input-output functions from a simulator library\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oSim2({"-sim2"},
    "  -sim2             \tLike -sim, but pass linker options to locate initialized "
    "data at 0x40000000 and zero-initialized data at 0x80000000.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oSingle_module({"-single_module"},
    "  -single_module             \t\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oStaticLibgcc({"-static-libgcc"},
    "  -static-libgcc             \tOn systems that provide libgcc as a shared library,"
    " these options force the use of either the shared or static version, respectively.\n",
    {driverCategory, ldCategory});

maplecl::Option<bool> oStaticLibstdc({"-static-libstdc++"},
    "  -static-libstdc++             \tWhen the g++ program is used to link a C++ program,"
    " it normally automatically links against libstdc++. \n",
    {driverCategory, ldCategory});

maplecl::Option<bool> oSub_library({"-sub_library"},
    "  -sub_library             \t\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oSub_umbrella({"-sub_umbrella"},
    "  -sub_umbrella             \t\n",
    {driverCategory, unSupCategory});

maplecl::Option<std::string> oT({"-T"},
    "  -T             \tUse script as the linker script. This option is supported "
    "by most systems using the GNU linker\n",
    {driverCategory, ldCategory});

maplecl::Option<bool> oTargetHelp({"-target-help"},
    "  -target-help             \tPrint (on the standard output) a description of "
    "target-specific command-line options for each tool.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oThreads({"-threads"},
    "  -threads             \tAdd support for multithreading with the dce thread "
    "library under HP-UX. \n",
    {driverCategory, unSupCategory});

maplecl::Option<std::string> oTime({"-time"},
    "  -time             \tReport the CPU time taken by each subprocess in the "
    "compilation sequence. \n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oTnoAndroidCc({"-tno-android-cc"},
    "  -tno-android-cc             \tDisable compilation effects of -mandroid, i.e., "
    "do not enable -mbionic, -fPIC, -fno-exceptions and -fno-rtti by default.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oTnoAndroidLd({"-tno-android-ld"},
    "  -tno-android-ld             \tDisable linking effects of -mandroid, i.e., pass "
    "standard Linux linking options to the linker.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oTraditional({"-traditional"},
    "  -traditional             \tEnable traditional preprocessing. \n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oTraditionalCpp({"-traditional-cpp"},
    "  -traditional-cpp             \tEnable traditional preprocessing.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oTrigraphs({"-trigraphs"},
    "  -trigraphs             \t-trigraphs   Support ISO C trigraphs.\n",
    {driverCategory, clangCategory});

maplecl::Option<bool> oTwolevel_namespace({"-twolevel_namespace"},
    "  -twolevel_namespace             \t\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oUmbrella({"-umbrella"},
    "  -umbrella             \t\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oUndef({"-undef"},
    "  -undef             \tDo not predefine system-specific and GCC-specific macros.\n",
    {driverCategory, clangCategory});

maplecl::Option<bool> oUndefined({"-undefined"},
    "  -undefined             \t\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oUnexported_symbols_list({"-unexported_symbols_list"},
    "  -unexported_symbols_list             \t\n",
    {driverCategory, unSupCategory});

maplecl::Option<std::string> oWa({"-Wa"},
    "  -Wa             \tPass option as an option to the assembler. If option contains"
    " commas, it is split into multiple options at the commas.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oWhatsloaded({"-whatsloaded"},
    "  -whatsloaded             \t\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oWhyload({"-whyload"},
    "  -whyload             \t\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oWLtoTypeMismatch({"-Wlto-type-mismatch"},
    "  -Wno-lto-type-mismatch             \tDuring the link-time optimization warn about"
    " type mismatches in global declarations from different compilation units. \n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-Wno-lto-type-mismatch"));

maplecl::Option<bool> oWmisspelledIsr({"-Wmisspelled-isr"},
    "  -Wmisspelled-isr             \t\n",
    {driverCategory, unSupCategory});

maplecl::Option<std::string> oWp({"-Wp"},
    "  -Wp             \tYou can use -Wp,option to bypass the compiler driver and pass "
    "option directly through to the preprocessor. \n",
    {driverCategory, clangCategory}, maplecl::joinedValue);

maplecl::Option<bool> oWrapper({"-wrapper"},
    "  -wrapper             \tInvoke all subcommands under a wrapper program. \n",
    {driverCategory, unSupCategory});

maplecl::Option<std::string> oXassembler({"-Xassembler"},
    "  -Xassembler             \tPass option as an option to the assembler. \n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oXbindLazy({"-Xbind-lazy"},
    "  -Xbind-lazy             \tEnable lazy binding of function calls. \n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oXbindNow({"-Xbind-now"},
    "  -Xbind-now             \tDisable lazy binding of function calls. \n",
    {driverCategory, unSupCategory});

maplecl::Option<std::string> oXlinker({"-Xlinker"},
    "  -Xlinker             \tPass option as an option to the linker. \n",
    {driverCategory, ldCategory});

maplecl::Option<std::string> oXpreprocessor({"-Xpreprocessor"},
    "  -Xpreprocessor             \tPass option as an option to the preprocessor. \n",
    {driverCategory, clangCategory});

maplecl::Option<std::string> oYm({"-Ym"},
    "  -Ym             \tLook in the directory dir to find the M4 preprocessor. \n",
    {driverCategory, unSupCategory});

maplecl::Option<std::string> oYP({"-YP"},
    "  -YP             \tSearch the directories dirs, and no others, for "
    "libraries specified with -l.\n",
    {driverCategory, unSupCategory});

maplecl::Option<std::string> oZ({"-z"},
    "  -z             \tPassed directly on to the linker along with the keyword keyword.\n",
    {driverCategory, ldCategory});

maplecl::Option<std::string> oU({"-u"},
    "  -u             \tPretend the symbol symbol is undefined, to force linking of "
    "library modules to define it. \n",
    {driverCategory, ldCategory});

maplecl::Option<bool> oStd03({"-std=c++03"},
    "  -std=c++03             \ttConform to the ISO 1998 C++ standard revised by the 2003 technical "
    "corrigendum.\n",
    {driverCategory, clangCategory, unSupCategory});

maplecl::Option<bool> oStd0x({"-std=c++0x"},
    "  -std=c++0x             \tDeprecated in favor of -std=c++11.\n",
    {driverCategory, clangCategory, unSupCategory});

maplecl::Option<bool> oStd11({"-std=c++11"},
    "  -std=c++11             \tConform to the ISO 2011 C++ standard.\n",
    {driverCategory, ldCategory, clangCategory});

maplecl::Option<bool> oStd14({"-std=c++14"},
    "  -std=c++14             \tConform to the ISO 2014 C++ standard.\n",
    {driverCategory, clangCategory, unSupCategory});

maplecl::Option<bool> oStd17({"-std=c++17"},
    "  -std=c++17             \ttConform to the ISO 2017 C++ standard.\n",
    {driverCategory, clangCategory, unSupCategory});

maplecl::Option<bool> oStd1y({"-std=c++1y"},
    "  -std=c++1y             \tDeprecated in favor of -std=c++14.\n",
    {driverCategory, clangCategory, unSupCategory});

maplecl::Option<bool> oStd1z({"-std=c++1z"},
    "  -std=c++1z             \tConform to the ISO 2017(?) C++ draft standard (experimental and "
    "incomplete support).\n",
    {driverCategory, clangCategory, unSupCategory});

maplecl::Option<bool> oStd98({"-std=c++98"},
    "  -std=c++98             \tConform to the ISO 1998 C++ standard revised by the 2003 technical "
    "corrigendum.\n",
    {driverCategory, clangCategory, unSupCategory});

maplecl::Option<bool> oStd11p({"-std=c11"},
    "  -std=c11             \tConform to the ISO 2011 C standard.\n",
    {driverCategory, clangCategory, unSupCategory});

maplecl::Option<bool> oStdc1x({"-std=c1x"},
    "  -std=c1x             \tDeprecated in favor of -std=c11.\n",
    {driverCategory, clangCategory, unSupCategory});

maplecl::Option<bool> oStd89({"-std=c89"},
    "  -std=c89             \tConform to the ISO 1990 C standard.\n",
    {driverCategory, clangCategory, unSupCategory});

maplecl::Option<bool> oStd90({"-std=c90"},
    "  -std             \tConform to the ISO 1998 C++ standard revised by the 2003 technical "
    "corrigendum.\n",
    {driverCategory, clangCategory, unSupCategory});

maplecl::Option<bool> oStd99({"-std=c99"},
    "  -std=c99             \tConform to the ISO 1999 C standard.\n",
    {driverCategory, clangCategory, unSupCategory});

maplecl::Option<bool> oStd9x({"-std=c9x"},
    "  -std=c9x             \tDeprecated in favor of -std=c99.\n",
    {driverCategory, clangCategory, unSupCategory});

maplecl::Option<bool> oStd2003({"-std=f2003"},
    "  -std=f2003             \tConform to the ISO Fortran 2003 standard.\n",
    {driverCategory, clangCategory, unSupCategory});

maplecl::Option<bool> oStd2008({"-std=f2008"},
    "  -std=f2008             \tConform to the ISO Fortran 2008 standard.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oStd2008ts({"-std=f2008ts"},
    "  -std=f2008ts             \tConform to the ISO Fortran 2008 standard including TS 29113.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oStdf95({"-std=f95"},
    "  -std=f95             \tConform to the ISO Fortran 95 standard.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oStdgnu({"-std=gnu"},
    "  -std=gnu             \tConform to nothing in particular.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oStdgnu03p({"-std=gnu++03"},
    "  -std=gnu++03             \tConform to the ISO 1998 C++ standard revised by the 2003 technical "
    "corrigendum with GNU extensions.\n",
    {driverCategory, clangCategory, unSupCategory});

maplecl::Option<bool> oStdgnuoxp({"-std=gnu++0x"},
    "  -std=gnu++0x             \tDeprecated in favor of -std=gnu++11.\n",
    {driverCategory, clangCategory, unSupCategory});

maplecl::Option<bool> oStdgnu11p({"-std=gnu++11"},
    "  -std=gnu++11             \tConform to the ISO 2011 C++ standard with GNU extensions.\n",
    {driverCategory, clangCategory, unSupCategory});

maplecl::Option<bool> oStdgnu14p({"-std=gnu++14"},
    "  -std=gnu++14             \tConform to the ISO 2014 C++ standard with GNU extensions.\n",
    {driverCategory, clangCategory, unSupCategory});

maplecl::Option<bool> oStdgnu17p({"-std=gnu++17"},
    "  -std=gnu++17             \tConform to the ISO 2017 C++ standard with GNU extensions.\n",
    {driverCategory, clangCategory, unSupCategory});

maplecl::Option<bool> oStdgnu1yp({"-std=gnu++1y"},
    "  -std=gnu++1y             \tDeprecated in favor of -std=gnu++14.\n",
    {driverCategory, clangCategory, unSupCategory});

maplecl::Option<bool> oStdgnu1zp({"-std=gnu++1z"},
    "  -std=gnu++1z             \tConform to the ISO 201z(7?) C++ draft standard with GNU extensions "
    "(experimental and incomplete support).\n",
    {driverCategory, clangCategory, unSupCategory});

maplecl::Option<bool> oStdgnu98p({"-std=gnu++98"},
    "  -std=gnu++98             \tConform to the ISO 1998 C++ standard revised by the 2003 technical "
    "corrigendum with GNU extensions.\n",
    {driverCategory, clangCategory, unSupCategory});

maplecl::Option<bool> oStdgnu11({"-std=gnu11"},
    "  -std=gnu11             \tConform to the ISO 2011 C standard with GNU extensions.\n",
    {driverCategory, clangCategory, unSupCategory});

maplecl::Option<bool> oStdgnu1x({"-std=gnu1x"},
    "  -std=gnu1x             \tDeprecated in favor of -std=gnu11.\n",
    {driverCategory, clangCategory, unSupCategory});

maplecl::Option<bool> oStdgnu89({"-std=gnu89"},
    "  -std=gnu89             \tConform to the ISO 1990 C standard with GNU extensions.\n",
    {driverCategory, clangCategory, unSupCategory});

maplecl::Option<bool> oStdgnu90({"-std=gnu90"},
    "  -std=gnu90             \tConform to the ISO 1990 C standard with GNU extensions.\n",
    {driverCategory, clangCategory, unSupCategory});

maplecl::Option<bool> oStdgnu99({"-std=gnu99"},
    "  -std=gnu99             \tConform to the ISO 1999 C standard with GNU extensions.\n",
    {driverCategory, clangCategory, unSupCategory});

maplecl::Option<bool> oStdgnu9x({"-std=gnu9x"},
    "  -std=gnu9x             \tDeprecated in favor of -std=gnu99.\n",
    {driverCategory, clangCategory, unSupCategory});

maplecl::Option<bool> oStd1990({"-std=iso9899:1990"},
    "  -std=iso9899:1990             \tConform to the ISO 1990 C standard.\n",
    {driverCategory, clangCategory, unSupCategory});

maplecl::Option<bool> oStd1994({"-std=iso9899:199409"},
    "  -std=iso9899:199409             \tConform to the ISO 1990 C standard as amended in 1994.\n",
    {driverCategory, clangCategory, unSupCategory});

maplecl::Option<bool> oStd1999({"-std=iso9899:1999"},
    "  -std=iso9899:1999             \tConform to the ISO 1999 C standard.\n",
    {driverCategory, clangCategory, unSupCategory});

maplecl::Option<bool> oStd199x({"-std=iso9899:199x"},
    "  -std=iso9899:199x             \tDeprecated in favor of -std=iso9899:1999.\n",
    {driverCategory, clangCategory, unSupCategory});

maplecl::Option<bool> oStd2011({"-std=iso9899:2011"},
    "  -std=iso9899:2011\tConform to the ISO 2011 C standard.\n",
    {driverCategory, clangCategory, unSupCategory});

maplecl::Option<bool> oStdlegacy({"-std=legacy"},
    "  -std=legacy\tAccept extensions to support legacy code.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oFworkingDirectory({"-fworking-directory"},
    "  -fworking-directory             \tGenerate a #line directive pointing at the current working directory.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-working-directory"));

maplecl::Option<bool> oFwrapv({"-fwrapv"},
    "  -fwrapv             \tAssume signed arithmetic overflow wraps around.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-fwrapv"));

maplecl::Option<bool> oFzeroLink({"-fzero-link"},
    "  -fzero-link             \tGenerate lazy class lookup (via objc_getClass()) for use "
    "inZero-Link mode.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-zero-link"));

maplecl::Option<std::string> oG({"-G"},
    "  -G             \tOn embedded PowerPC systems, put global and static items less than"
    " or equal to num bytes into the small data or BSS sections instead of the normal data "
    "or BSS section. \n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oGcoff({"-gcoff"},
    "  -gcoff             \tGenerate debug information in COFF format.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oGcolumnInfo({"-gcolumn-info"},
    "  -gcolumn-info             \tRecord DW_AT_decl_column and DW_AT_call_column "
    "in DWARF.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oGdwarf({"-gdwarf"},
    "  -gdwarf             \tGenerate debug information in default version of DWARF "
    "format.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oGenDecls({"-gen-decls"},
    "  -gen-decls             \tDump declarations to a .decl file.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oGfull({"-gfull"},
    "  -gfull             \t\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oGgdb({"-ggdb"},
    "  -ggdb             \tGenerate debug information in default extended format.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oGgnuPubnames({"-ggnu-pubnames"},
    "  -ggnu-pubnames             \tGenerate DWARF pubnames and pubtypes sections with "
    "GNU extensions.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oGnoColumnInfo({"-gno-column-info"},
    "  -gno-column-info             \tDon't record DW_AT_decl_column and DW_AT_call_column"
    " in DWARF.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oGnoRecordGccSwitches({"-gno-record-gcc-switches"},
    "  -gno-record-gcc-switches             \tDon't record gcc command line switches "
    "in DWARF DW_AT_producer.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oGnoStrictDwarf({"-gno-strict-dwarf"},
    "  -gno-strict-dwarf             \tEmit DWARF additions beyond selected version.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oGpubnames({"-gpubnames"},
    "  -gpubnames             \tGenerate DWARF pubnames and pubtypes sections.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oGrecordGccSwitches({"-grecord-gcc-switches"},
    "  -grecord-gcc-switches             \tRecord gcc command line switches in "
    "DWARF DW_AT_producer.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oGsplitDwarf({"-gsplit-dwarf"},
    "  -gsplit-dwarf             \tGenerate debug information in separate .dwo files.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oGstabs({"-gstabs"},
    "  -gstabs             \tGenerate debug information in STABS format.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oGstabsA({"-gstabs+"},
    "  -gstabs+             \tGenerate debug information in extended STABS format.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oGstrictDwarf({"-gstrict-dwarf"},
    "  -gstrict-dwarf             \tDon't emit DWARF additions beyond selected version.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oGtoggle({"-gtoggle"},
    "  -gtoggle             \tToggle debug information generation.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oGused({"-gused"},
    "  -gused             \t\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oGvms({"-gvms"},
    "  -gvms             \tGenerate debug information in VMS format.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oGxcoff({"-gxcoff"},
    "  -gxcoff             \tGenerate debug information in XCOFF format.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oGxcoffA({"-gxcoff+"},
    "  -gxcoff+             \tGenerate debug information in extended XCOFF format.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oGz({"-gz"},
    "  -gz             \tGenerate compressed debug sections.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oH({"-H"},
    "  -H             \tPrint the name of header files as they are used.\n",
    {driverCategory, clangCategory});

maplecl::Option<bool> oHeaderpad_max_install_names({"-headerpad_max_install_names"},
    "  -headerpad_max_install_names             \t\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oI({"-I-"},
    "  -I-             \t\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oIdirafter({"-idirafter"},
    "  -idirafter             \t-idirafter <dir> o    Add <dir> oto the end of the system "
    "include path.\n",
    {driverCategory, unSupCategory});

maplecl::Option<std::string> oIframework({"-iframework"},
    "  -iframework             \tLike -F except the directory is a treated as a system "
    "directory. \n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oImage_base({"-image_base"},
    "  -image_base             \t\n",
    {driverCategory, unSupCategory});

maplecl::Option<std::string> oImultilib({"-imultilib"},
    "  -imultilib             \t-imultilib <dir> o    Set <dir> oto be the multilib include"
    " subdirectory.\n",
    {driverCategory, unSupCategory});

maplecl::Option<std::string> oInclude({"-include"},
    "  -include             \t-include <file> o       Include the contents of <file> o"
    "before other files.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oInit({"-init"},
    "  -init             \t\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oInstall_name({"-install_name"},
    "  -install_name             \t\n",
    {driverCategory, unSupCategory});

maplecl::Option<std::string> oIplugindir({"-iplugindir="},
    "  -iplugindir=             \t-iplugindir=<dir> o Set <dir> oto be the default plugin "
    "directory.\n",
    {driverCategory, unSupCategory});

maplecl::Option<std::string> oIprefix({"-iprefix"},
    "  -iprefix             \t-iprefix <path> o       Specify <path> oas a prefix for next "
    "two options.\n",
    {driverCategory, unSupCategory});

maplecl::Option<std::string> oIquote({"-iquote"},
    "  -iquote             \t-iquote <dir> o  Add <dir> oto the end of the quote include "
    "path.\n",
    {driverCategory, unSupCategory});

maplecl::Option<std::string> oIsysroot({"-isysroot"},
    "  -isysroot             \t-isysroot <dir> o      Set <dir> oto be the system root "
    "directory.\n",
    {driverCategory, unSupCategory});

maplecl::Option<std::string> oIwithprefix({"-iwithprefix"},
    "  -iwithprefix             \t-iwithprefix <dir> oAdd <dir> oto the end of the system "
    "include path.\n",
    {driverCategory, unSupCategory});

maplecl::Option<std::string> oIwithprefixbefore({"-iwithprefixbefore"},
    "  -iwithprefixbefore             \t-iwithprefixbefore <dir> o    Add <dir> oto the end "
    "ofthe main include path.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oKeep_private_externs({"-keep_private_externs"},
    "  -keep_private_externs             \t\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oM({"-M"},
    "  -M             \tGenerate make dependencies.\n",
    {driverCategory, clangCategory});

maplecl::Option<bool> oM1({"-m1"},
    "  -m1             \tGenerate code for the SH1.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oM10({"-m10"},
    "  -m10             \tGenerate code for a PDP-11/10.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oM128bitLongDouble({"-m128bit-long-double"},
    "  -m128bit-long-double             \tControl the size of long double type.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oM16({"-m16"},
    "  -m16             \tGenerate code for a 16-bit.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oM16Bit({"-m16-bit"},
    "  -m16-bit             \tArrange for stack frame, writable data and "
    "constants to all be 16-bit.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-mno-16-bit"));

maplecl::Option<bool> oM2({"-m2"},
    "  -m2             \tGenerate code for the SH2.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oM210({"-m210"},
    "  -m210             \tGenerate code for the 210 processor.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oM2a({"-m2a"},
    "  -m2a             \tGenerate code for the SH2a-FPU assuming the floating-point"
    " unit is in double-precision mode by default.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oM2e({"-m2e"},
    "  -m2e             \tGenerate code for the SH2e.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oM2aNofpu({"-m2a-nofpu"},
    "  -m2a-nofpu             \tGenerate code for the SH2a without FPU, or for a"
    " SH2a-FPU in such a way that the floating-point unit is not used.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oM2aSingle({"-m2a-single"},
    "  -m2a-single             \tGenerate code for the SH2a-FPU assuming the "
    "floating-point unit is in single-precision mode by default.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oM2aSingleOnly({"-m2a-single-only"},
    "  -m2a-single-only             \tGenerate code for the SH2a-FPU, in such a way"
    " that no double-precision floating-point operations are used.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oM3({"-m3"},
    "  -m3             \tGenerate code for the SH3.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oM31({"-m31"},
    "  -m31             \tWhen -m31 is specified, generate code compliant to"
    " the GNU/Linux for S/390 ABI. \n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oM32({"-m32"},
    "  -m32             \tGenerate code for 32-bit ABI.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oM32Bit({"-m32-bit"},
    "  -m32-bit             \tArrange for stack frame, writable data and "
    "constants to all be 32-bit.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oM32bitDoubles({"-m32bit-doubles"},
    "  -m32bit-doubles             \tMake the double data type  32 bits "
    "(-m32bit-doubles) in size.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oM32r({"-m32r"},
    "  -m32r             \tGenerate code for the M32R.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oM32r2({"-m32r2"},
    "  -m32r2             \tGenerate code for the M32R/2.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oM32rx({"-m32rx"},
    "  -m32rx             \tGenerate code for the M32R/X.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oM340({"-m340"},
    "  -m340             \tGenerate code for the 210 processor.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oM3dnow({"-m3dnow"},
    "  -m3dnow             \tThese switches enable the use of instructions "
    "in the m3dnow.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oM3dnowa({"-m3dnowa"},
    "  -m3dnowa             \tThese switches enable the use of instructions "
    "in the m3dnowa.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oM3e({"-m3e"},
    "  -m3e             \tGenerate code for the SH3e.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oM4({"-m4"},
    "  -m4             \tGenerate code for the SH4.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oM4100({"-m4-100"},
    "  -m4-100             \tGenerate code for SH4-100.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oM4100Nofpu({"-m4-100-nofpu"},
    "  -m4-100-nofpu             \tGenerate code for SH4-100 in such a way that the "
    "floating-point unit is not used.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oM4100Single({"-m4-100-single"},
    "  -m4-100-single             \tGenerate code for SH4-100 assuming the floating-point "
    "unit is in single-precision mode by default.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oM4100SingleOnly({"-m4-100-single-only"},
    "  -m4-100-single-only             \tGenerate code for SH4-100 in such a way that no "
    "double-precision floating-point operations are used.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oM4200({"-m4-200"},
    "  -m4-200             \tGenerate code for SH4-200.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oM4200Nofpu({"-m4-200-nofpu"},
    "  -m4-200-nofpu             \tGenerate code for SH4-200 without in such a way that"
    " the floating-point unit is not used.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oM4200Single({"-m4-200-single"},
    "  -m4-200-single             \tGenerate code for SH4-200 assuming the floating-point "
    "unit is in single-precision mode by default.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oM4200SingleOnly({"-m4-200-single-only"},
    "  -m4-200-single-only             \tGenerate code for SH4-200 in such a way that no "
    "double-precision floating-point operations are used.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oM4300({"-m4-300"},
    "  -m4-300             \tGenerate code for SH4-300.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oM4300Nofpu({"-m4-300-nofpu"},
    "  -m4-300-nofpu             \tGenerate code for SH4-300 without in such a way that"
    " the floating-point unit is not used.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oM4300Single({"-m4-300-single"},
    "  -m4-300-single             \tGenerate code for SH4-300 in such a way that no "
    "double-precision floating-point operations are used.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oM4300SingleOnly({"-m4-300-single-only"},
    "  -m4-300-single-only             \tGenerate code for SH4-300 in such a way that "
    "no double-precision floating-point operations are used.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oM4340({"-m4-340"},
    "  -m4-340             \tGenerate code for SH4-340 (no MMU, no FPU).\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oM4500({"-m4-500"},
    "  -m4-500             \tGenerate code for SH4-500 (no FPU). Passes "
    "-isa=sh4-nofpu to the assembler.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oM4Nofpu({"-m4-nofpu"},
    "  -m4-nofpu             \tGenerate code for the SH4al-dsp, or for a "
    "SH4a in such a way that the floating-point unit is not used.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oM4Single({"-m4-single"},
    "  -m4-single             \tGenerate code for the SH4a assuming the floating-point "
    "unit is in single-precision mode by default.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oM4SingleOnly({"-m4-single-only"},
    "  -m4-single-only             \tGenerate code for the SH4a, in such a way that "
    "no double-precision floating-point operations are used.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oM40({"-m40"},
    "  -m40             \tGenerate code for a PDP-11/40.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oM45({"-m45"},
    "  -m45             \tGenerate code for a PDP-11/45. This is the default.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oM4a({"-m4a"},
    "  -m4a             \tGenerate code for the SH4a.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oM4aNofpu({"-m4a-nofpu"},
    "  -m4a-nofpu             \tGenerate code for the SH4al-dsp, or for a SH4a in such "
    "a way that the floating-point unit is not used.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oM4aSingle({"-m4a-single"},
    "  -m4a-single             \tGenerate code for the SH4a assuming the floating-point "
    "unit is in single-precision mode by default.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oM4aSingleOnly({"-m4a-single-only"},
    "  -m4a-single-only             \tGenerate code for the SH4a, in such a way that no"
    " double-precision floating-point operations are used.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oM4al({"-m4al"},
    "  -m4al             \tSame as -m4a-nofpu, except that it implicitly passes -dsp"
    " to the assembler. GCC doesn't generate any DSP instructions at the moment.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oM4byteFunctions({"-m4byte-functions"},
    "  -m4byte-functions             \tForce all functions to be aligned to a 4-byte"
    " boundary.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-mno-4byte-functions"));

maplecl::Option<bool> oM5200({"-m5200"},
    "  -m5200             \tGenerate output for a 520X ColdFire CPU.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oM5206e({"-m5206e"},
    "  -m5206e             \tGenerate output for a 5206e ColdFire CPU. \n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oM528x({"-m528x"},
    "  -m528x             \tGenerate output for a member of the ColdFire 528X family. \n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oM5307({"-m5307"},
    "  -m5307             \tGenerate output for a ColdFire 5307 CPU.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oM5407({"-m5407"},
    "  -m5407             \tGenerate output for a ColdFire 5407 CPU.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oM64({"-m64"},
    "  -m64             \tGenerate code for 64-bit ABI.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oM64bitDoubles({"-m64bit-doubles"},
    "  -m64bit-doubles             \tMake the double data type be 64 bits (-m64bit-doubles)"
    " in size.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oM68000({"-m68000"},
    "  -m68000             \tGenerate output for a 68000.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oM68010({"-m68010"},
    "  -m68010             \tGenerate output for a 68010.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oM68020({"-m68020"},
    "  -m68020             \tGenerate output for a 68020. \n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oM6802040({"-m68020-40"},
    "  -m68020-40             \tGenerate output for a 68040, without using any of "
    "the new instructions. \n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oM6802060({"-m68020-60"},
    "  -m68020-60             \tGenerate output for a 68060, without using any of "
    "the new instructions. \n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oMTLS({"-mTLS"},
    "  -mTLS             \tAssume a large TLS segment when generating thread-local code.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-mtls"));

maplecl::Option<std::string> oMtlsDialect({"-mtls-dialect"},
    "  -mtls-dialect             \tSpecify TLS dialect.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oMtlsDirectSegRefs({"-mtls-direct-seg-refs"},
    "  -mtls-direct-seg-refs             \t\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oMtlsMarkers({"-mtls-markers"},
    "  -mtls-markers             \tMark calls to __tls_get_addr with a relocation "
    "specifying the function argument. \n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("--mno-tls-markers"));

maplecl::Option<bool> oMtoc({"-mtoc"},
    "  -mtoc             \tOn System V.4 and embedded PowerPC systems do not assume "
    "that register 2 contains a pointer to a global area pointing to the addresses "
    "used in the program.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("--mno-toc"));

maplecl::Option<bool> oMtomcatStats({"-mtomcat-stats"},
    "  -mtomcat-stats             \tCause gas to print out tomcat statistics.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oMtoplevelSymbols({"-mtoplevel-symbols"},
    "  -mtoplevel-symbols             \tPrepend (do not prepend) a ':' to all global "
    "symbols, so the assembly code can be used with the PREFIX assembly directive.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("--mno-toplevel-symbols"));

maplecl::Option<std::string> oMtp({"-mtp"},
    "  -mtp             \tSpecify the access model for the thread local storage pointer. \n",
    {driverCategory, unSupCategory});

maplecl::Option<std::string> oMtpRegno({"-mtp-regno"},
    "  -mtp-regno             \tSpecify thread pointer register number.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oMtpcsFrame({"-mtpcs-frame"},
    "  -mtpcs-frame             \tGenerate a stack frame that is compliant with the Thumb "
    "Procedure Call Standard for all non-leaf functions.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oMtpcsLeafFrame({"-mtpcs-leaf-frame"},
    "  -mtpcs-leaf-frame             \tGenerate a stack frame that is compliant with "
    "the Thumb Procedure Call Standard for all leaf functions.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oMtpfTrace({"-mtpf-trace"},
    "  -mtpf-trace             \tGenerate code that adds in TPF OS specific branches to "
    "trace routines in the operating system. \n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("--mno-tpf-trace"));

maplecl::Option<std::string> oMtrapPrecision({"-mtrap-precision"},
    "  -mtrap-precision             \tIn the Alpha architecture, floating-point traps "
    "are imprecise.\n",
    {driverCategory, unSupCategory});

maplecl::Option<std::string> oMtune({"-mtune="},
    "  -mtune=             \tOptimize for CPU. \n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oMtuneCtrlFeatureList({"-mtune-ctrl=feature-list"},
    "  -mtune-ctrl=feature-list             \tThis option is used to do fine grain "
    "control of x86 code generation features. \n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oMuclibc({"-muclibc"},
    "  -muclibc             \tUse uClibc C library.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oMuls({"-muls"},
    "  -muls             \t\n",
    {driverCategory, unSupCategory});

maplecl::Option<std::string> oMultcost({"-multcost"},
    "  -multcost             \tReplaced by -mmultcost.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oMultcostNumber({"-multcost=number"},
    "  -multcost=number             \tSet the cost to assume for a multiply insn.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oMultilibLibraryPic({"-multilib-library-pic"},
    "  -multilib-library-pic             \tLink with the (library, not FD) pic libraries.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oMmultiplyEnabled({"-mmultiply-enabled"},
    "  -multiply-enabled             \tEnable multiply instructions.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oMultiply_defined({"-multiply_defined"},
    "  -multiply_defined             \t\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oMultiply_defined_unused({"-multiply_defined_unused"},
    "  -multiply_defined_unused             \t\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oMulti_module({"-multi_module"},
    "  -multi_module             \t\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oMunalignProbThreshold({"-munalign-prob-threshold"},
    "  -munalign-prob-threshold             \tSet probability threshold for unaligning"
    " branches. \n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oMunalignedAccess({"-munaligned-access"},
    "  -munaligned-access             \tEnables (or disables) reading and writing of 16- "
    "and 32- bit values from addresses that are not 16- or 32- bit aligned.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("--mno-unaligned-access"));

maplecl::Option<bool> oMunalignedDoubles({"-munaligned-doubles"},
    "  -munaligned-doubles             \tAssume that doubles have 8-byte alignment. \n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("--mno-unaligned-doubles"));

maplecl::Option<bool> oMunicode({"-municode"},
    "  -municode             \tThis option is available for MinGW-w64 targets.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oMuniformSimt({"-muniform-simt"},
    "  -muniform-simt             \t\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oMuninitConstInRodata({"-muninit-const-in-rodata"},
    "  -muninit-const-in-rodata             \tPut uninitialized const variables in the "
    "read-only data section. \n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("--mno-uninit-const-in-rodata"));

maplecl::Option<std::string> oMunix({"-munix"},
    "  -munix             \tGenerate compiler predefines and select a startfile for "
    "the specified UNIX standard. \n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oMunixAsm({"-munix-asm"},
    "  -munix-asm             \tUse Unix assembler syntax. \n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oMupdate({"-mupdate"},
    "  -mupdate             \tGenerate code that uses the load string instructions and "
    "the store string word instructions to save multiple registers and do small "
    "block moves. \n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("--mno-update"));

maplecl::Option<bool> oMupperRegs({"-mupper-regs"},
    "  -mupper-regs             \tGenerate code that uses (does not use) the scalar "
    "instructions that target all 64 registers in the vector/scalar floating point "
    "register set, depending on the model of the machine.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("--mno-upper-regs"));

maplecl::Option<bool> oMupperRegsDf({"-mupper-regs-df"},
    "  -mupper-regs-df             \tGenerate code that uses (does not use) the scalar "
    "double precision instructions that target all 64 registers in the vector/scalar "
    "floating point register set that were added in version 2.06 of the PowerPC ISA.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("--mno-upper-regs-df"));

maplecl::Option<bool> oMupperRegsDi({"-mupper-regs-di"},
    "  -mupper-regs-di             \tGenerate code that uses (does not use) the scalar "
    "instructions that target all 64 registers in the vector/scalar floating point "
    "register set that were added in version 2.06 of the PowerPC ISA when processing "
    "integers. \n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("--mno-upper-regs-di"));

maplecl::Option<bool> oMupperRegsSf({"-mupper-regs-sf"},
    "  -mupper-regs-sf             \tGenerate code that uses (does not use) the scalar "
    "single precision instructions that target all 64 registers in the vector/scalar "
    "floating point register set that were added in version 2.07 of the PowerPC ISA.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("--mno-upper-regs-sf"));

maplecl::Option<bool> oMuserEnabled({"-muser-enabled"},
    "  -muser-enabled             \tEnable user-defined instructions.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oMuserMode({"-muser-mode"},
    "  -muser-mode             \tDo not generate code that can only run in supervisor "
    "mode.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("--mno-user-mode"));

maplecl::Option<bool> oMusermode({"-musermode"},
    "  -musermode             \tDon't allow (allow) the compiler generating "
    "privileged mode code.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("--mno-usermode"));

maplecl::Option<bool> oMv3push({"-mv3push"},
    "  -mv3push             \tGenerate v3 push25/pop25 instructions.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("--mno-v3push"));

maplecl::Option<bool> oMv850({"-mv850"},
    "  -mv850             \tSpecify that the target processor is the V850.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oMv850e({"-mv850e"},
    "  -mv850e             \tSpecify that the target processor is the V850E.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oMv850e1({"-mv850e1"},
    "  -mv850e1             \tSpecify that the target processor is the V850E1. \n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oMv850e2({"-mv850e2"},
    "  -mv850e2             \tSpecify that the target processor is the V850E2.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oMv850e2v3({"-mv850e2v3"},
    "  -mv850e2v3             \tSpecify that the target processor is the V850E2V3.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oMv850e2v4({"-mv850e2v4"},
    "  -mv850e2v4             \tSpecify that the target processor is the V850E3V5.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oMv850e3v5({"-mv850e3v5"},
    "  -mv850e3v5             \tpecify that the target processor is the V850E3V5.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oMv850es({"-mv850es"},
    "  -mv850es             \tSpecify that the target processor is the V850ES.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oMv8plus({"-mv8plus"},
    "  -mv8plus             \tWith -mv8plus, Maple generates code for the SPARC-V8+ ABI.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("--mno-v8plus"));

maplecl::Option<std::string> oMveclibabi({"-mveclibabi"},
    "  -mveclibabi             \tSpecifies the ABI type to use for vectorizing "
    "intrinsics using an external library. \n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oMvect8RetInMem({"-mvect8-ret-in-mem"},
    "  -mvect8-ret-in-mem             \t\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oMvirt({"-mvirt"},
    "  -mvirt             \tUse the MIPS Virtualization (VZ) instructions.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("--mno-virt"));

maplecl::Option<bool> oMvis({"-mvis"},
    "  -mvis             \tWith -mvis, Maple generates code that takes advantage of the "
    "UltraSPARC Visual Instruction Set extensions. \n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("--mno-vis"));

maplecl::Option<bool> oMvis2({"-mvis2"},
    "  -mvis2             \tWith -mvis2, Maple generates code that takes advantage of "
    "version 2.0 of the UltraSPARC Visual Instruction Set extensions. \n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("--mno-vis2"));

maplecl::Option<bool> oMvis3({"-mvis3"},
    "  -mvis3             \tWith -mvis3, Maple generates code that takes advantage of "
    "version 3.0 of the UltraSPARC Visual Instruction Set extensions.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("--mno-vis3"));

maplecl::Option<bool> oMvis4({"-mvis4"},
    "  -mvis4             \tWith -mvis4, GCC generates code that takes advantage of "
    "version 4.0 of the UltraSPARC Visual Instruction Set extensions.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("--mno-vis4"));

maplecl::Option<bool> oMvis4b({"-mvis4b"},
    "  -mvis4b             \tWith -mvis4b, GCC generates code that takes advantage of "
    "version 4.0 of the UltraSPARC Visual Instruction Set extensions, plus the additional "
    "VIS instructions introduced in the Oracle SPARC Architecture 2017.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("--mno-vis4b"));

maplecl::Option<bool> oMvliwBranch({"-mvliw-branch"},
    "  -mvliw-branch             \tRun a pass to pack branches into VLIW instructions "
    "(default).\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("--mno-vliw-branch"));

maplecl::Option<bool> oMvmsReturnCodes({"-mvms-return-codes"},
    "  -mvms-return-codes             \t\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oMvolatileAsmStop({"-mvolatile-asm-stop"},
    "  -mvolatile-asm-stop             \t\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("--mno-volatile-asm-stop"));

maplecl::Option<bool> oMvolatileCache({"-mvolatile-cache"},
    "  -mvolatile-cache             \tUse ordinarily cached memory accesses for "
    "volatile references. \n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("--mno-volatile-cache"));

maplecl::Option<bool> oMvr4130Align({"-mvr4130-align"},
    "  -mvr4130-align             \tThe VR4130 pipeline is two-way superscalar, but "
    "can only issue two instructions together if the first one is 8-byte aligned. \n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-mno-vr4130-align"));

maplecl::Option<bool> oMvrsave({"-mvrsave"},
    "  -mvrsave             \tGenerate VRSAVE instructions when generating AltiVec code.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("--mno-vrsave"));

maplecl::Option<bool> oMvsx({"-mvsx"},
    "  -mvsx             \tGenerate code that uses (does not use) vector/scalar (VSX)"
    " instructions, and also enable the use of built-in functions that allow more direct "
    "access to the VSX instruction set.\n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("--mno-vsx"));

maplecl::Option<bool> oMvx({"-mvx"},
    "  -mvx             \tGenerate code using the instructions available with the vector "
    "extension facility introduced with the IBM z13 machine generation. \n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("--mno-vx"));

maplecl::Option<bool> oMvxworks({"-mvxworks"},
    "  -mvxworks             \tOn System V.4 and embedded PowerPC systems, specify that "
    "you are compiling for a VxWorks system.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oMvzeroupper({"-mvzeroupper"},
    "  -mvzeroupper             \t\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oMwarnCellMicrocode({"-mwarn-cell-microcode"},
    "  -mwarn-cell-microcode             \tWarn when a Cell microcode instruction is emitted. \n",
    {driverCategory, unSupCategory});

maplecl::Option<std::string> oA({"-A"},
    "  -A<question>=<answer>             \tAssert the <answer> to <question>.  Putting '-' before <question> disables "
    "the <answer> to <question> assertion missing after %qs.\n",
    {driverCategory, clangCategory}, maplecl::joinedValue);

maplecl::Option<bool> oO({"-O"},
    "  -O             \tReduce code size and execution time, without performing any optimizations that "
    "take a great deal of compilation time.\n",
    {driverCategory, unSupCategory});

maplecl::Option<bool> oNoPie({"-no-pie"},
    "  -no-pie             \tDon't create a position independent executable.\n",
    {driverCategory, ldCategory});

maplecl::Option<bool> staticLibmplpgo({"--static-libmplpgo"},
    " --static-libmplpgo             \tStatic link using libmplpgo\n",
    {driverCategory, cgCategory});

    /* #################################################################################################### */

} /* namespace opts */
