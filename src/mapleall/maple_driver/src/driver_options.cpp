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

maplecl::Option<bool> ignoreUnkOpt({"--ignore-unknown-options"},
    "  --ignore-unknown-options    \tIgnore unknown compilation options\n",
    {driverCategory}, kOptDriver | kOptMaple);

maplecl::Option<bool> ignoreUnsupOpt({"--ignore-unsupport-option"},
    "  --ignore-unsupport-option    \tIgnore unsupport compilation options\n",
    {driverCategory, meCategory, mpl2mplCategory, cgCategory}, kOptDriver | kOptMaple);

maplecl::Option<bool> o0({"-O0", "--O0"},
    "  -O0                         \tNo optimization. (Default)\n",
    {driverCategory}, kOptCommon | kOptOptimization);

maplecl::Option<bool> o1({"-O1", "--O1"},
    "  -O1                         \tDo some optimization.\n",
    {driverCategory}, kOptCommon | kOptOptimization, maplecl::kHide);

maplecl::Option<bool> o2({"-O2", "--O2"},
    "  -O2                         \tDo more optimization.\n",
    {driverCategory, hir2mplCategory}, kOptCommon | kOptOptimization);

maplecl::Option<bool> o3({"-O3", "--O3"},
    "  -O3                         \tDo more optimization.\n",
    {driverCategory}, kOptCommon | kOptOptimization, maplecl::kHide);

maplecl::Option<bool> os({"-Os", "--Os"},
    "  -Os                         \tOptimize for size, based on O2.\n",
    {driverCategory}, kOptCommon | kOptOptimization);

maplecl::Option<bool> verify({"--verify"},
    "  --verify                    \tVerify mpl file\n",
    {driverCategory, dex2mplCategory, mpl2mplCategory}, kOptMaple);

maplecl::Option<bool> decoupleStatic({"--decouple-static", "-decouple-static"},
    "  --decouple-static           \tDecouple the static method and field.\n"
    "  --no-decouple-static        \tDon't decouple the static method and field.\n",
    {driverCategory, dex2mplCategory, meCategory, mpl2mplCategory},
    maplecl::DisableWith("--no-decouple-static"), kOptMaple, maplecl::kHide);

maplecl::Option<bool> gcOnly({"--gconly", "-gconly"},
    "  --gconly                    \tMake gconly is enable.\n"
    "  --no-gconly                 \tDon't make gconly is enable.\n",
    {driverCategory, dex2mplCategory, meCategory, mpl2mplCategory, cgCategory},
	maplecl::DisableWith("--no-gconly"), kOptMaple, maplecl::kHide);

maplecl::Option<bool> timePhase({"-time-phases"},
    "  -time-phases                \tTiming phases and print percentages.\n",
    {driverCategory}, kOptMaple);

maplecl::Option<bool> genMeMpl({"--genmempl"},
    "  --genmempl                  \tGenerate me.mpl file.\n",
    {driverCategory}, kOptMaple, maplecl::kHide);

maplecl::Option<bool> compileWOLink({"-c"},
    "  -c                          \tCompile the source files without linking.\n",
    {driverCategory}, kOptDriver);

maplecl::Option<bool> genVtable({"--genVtableImpl"},
    "  --genVtableImpl             \tGenerate VtableImpl.mpl file.\n",
    {driverCategory}, kOptMaple, maplecl::kHide);

maplecl::Option<bool> verbose({"--verbose", "-verbose"},
    "  -verbose                    \tPrint informations.\n",
    {driverCategory, jbc2mplCategory, hir2mplCategory, meCategory, mpl2mplCategory, cgCategory},
    kOptDriver | kOptCommon);

maplecl::Option<bool> debug({"--debug"},
    "  --debug                     \tPrint debug info.\n",
    {driverCategory}, kOptCommon);

maplecl::Option<bool> withDwarf({"-g", "-g1", "-g2", "-g3"},
    "  -g                          \tPrint debug info.\n",
    {driverCategory, hir2mplCategory}, kOptCommon);

maplecl::Option<bool> noOptO0({"-no-opt-O0"},
    "  -no-opt-O0                     \tDonot do O0 opt which will interference debug.\n",
    {driverCategory}, kOptMaple);

maplecl::Option<bool> withIpa({"--with-ipa"},
    "  --with-ipa                  \tRun IPA when building.\n"
    "  --no-with-ipa               \n",
    {driverCategory}, kOptMaple, maplecl::DisableWith("--no-with-ipa"));

maplecl::Option<bool> npeNoCheck({"--no-npe-check"},
    "  --no-npe-check              \tDisable null pointer check (Default).\n",
    {driverCategory}, kOptMaple);

maplecl::Option<bool> npeStaticCheck({"--npe-check-static"},
    "  --npe-check-static          \tEnable null pointer static check only.\n",
    {driverCategory}, kOptMaple);

maplecl::Option<bool> npeDynamicCheck({"--npe-check-dynamic", "-npe-check-dynamic"},
    "  --npe-check-dynamic         \tEnable null pointer dynamic check with static warning.\n",
    {driverCategory, hir2mplCategory}, kOptMaple);

maplecl::Option<bool> npeDynamicCheckSilent({"--npe-check-dynamic-silent"},
    "  --npe-check-dynamic-silent  \tEnable null pointer dynamic without static warning.\n",
    {driverCategory}, kOptMaple);

maplecl::Option<bool> npeDynamicCheckAll({"--npe-check-dynamic-all"},
    "  --npe-check-dynamic-all     \tKeep dynamic check before dereference, used with --npe-check-dynamic* options\n",
    {driverCategory}, kOptMaple);

maplecl::Option<bool> boundaryNoCheck({"--no-boundary-check"},
    "  --no-boundary-check         \tDisable boundary check (Default).\n",
    {driverCategory}, kOptMaple);

maplecl::Option<bool> boundaryStaticCheck({"--boundary-check-static"},
    "  --boundary-check-static     \tEnable boundary static check.\n",
    {driverCategory}, kOptMaple);

maplecl::Option<bool> boundaryDynamicCheck({"--boundary-check-dynamic", "-boundary-check-dynamic"},
    "  --boundary-check-dynamic    \tEnable boundary dynamic check with static warning.\n",
    {driverCategory, hir2mplCategory}, kOptMaple);

maplecl::Option<bool> boundaryDynamicCheckSilent({"--boundary-check-dynamic-silent"},
    "  --boundary-check-dynamic-silent\n"
    "                              \tEnable boundary dynamic check without static warning\n",
    {driverCategory}, kOptMaple);

maplecl::Option<bool> safeRegionOption({"--safe-region", "-safe-region"},
    "  --safe-region               \tEnable safe region.\n",
    {driverCategory, hir2mplCategory}, kOptMaple);

maplecl::Option<bool> enableArithCheck({"--boundary-arith-check"},
    "  --boundary-arith-check      \tEnable pointer arithmetic check.\n",
    {driverCategory}, kOptMaple);

maplecl::Option<bool> enableCallFflush({"--boudary-dynamic-call-fflush"},
    "  --boudary-dynamic-call-fflush\tEnable call fflush function to flush boundary-dynamic-check error "
    "message to the STDOUT\n",
    {driverCategory}, kOptMaple);

maplecl::Option<bool> onlyCompile({"-S"},
    "  -S                          \tOnly run preprocess and compilation steps.\n",
    {driverCategory}, kOptDriver);

maplecl::Option<bool> printDriverPhases({"--print-driver-phases"},
    "  --print-driver-phases       \tPrint Driver Phases.\n",
    {driverCategory}, kOptMaple, maplecl::kHide);

maplecl::Option<bool> ldStatic({"-static", "--static"},
    "  -static                     \tForce the linker to link a program statically.\n",
    {driverCategory, ldCategory}, kOptDriver | kOptLd);

maplecl::Option<bool> maplePhase({"--maple-phase"},
    "  --maple-phase               \tRun maple phase only\n  --no-maple-phase\n",
    {driverCategory}, kOptMaple, maplecl::DisableWith("--maple-toolchain"), maplecl::Init(true));

maplecl::Option<bool> genMapleBC({"--genmaplebc"},
    "  --genmaplebc                \tGenerate .mbc file.\n",
    {driverCategory}, kOptMaple, maplecl::kHide);

maplecl::Option<bool> genLMBC({"--genlmbc"},
    "  --genlmbc                   \tGenerate .lmbc file.\n",
    {driverCategory, mpl2mplCategory}, kOptMaple, maplecl::kHide);

maplecl::Option<bool> profileGen({"--profileGen"},
    "  --profileGen                \tGenerate profile data for static languages.\n",
    {driverCategory, meCategory, mpl2mplCategory, cgCategory}, kOptMaple, maplecl::kHide);

maplecl::Option<bool> profileUse({"--profileUse"},
    "  --profileUse                \tOptimize static languages with profile data.\n",
    {driverCategory, mpl2mplCategory}, kOptMaple, maplecl::kHide);

maplecl::Option<bool> missingProfDataIsError({"--missing-profdata-is-error"},
    "  --missing-profdata-is-error \tTreat missing profile data file as error.\n"
    "  --no-missing-profdata-is-error\n"
    "                              \tOnly warn on missing profile data file.\n",
    {driverCategory}, kOptMaple, maplecl::DisableWith("--no-missing-profdata-is-error"), maplecl::Init(true),
    maplecl::kHide);

maplecl::Option<bool> stackProtectorStrong({"-fstack-protector-strong", "--stack-protector-strong"},
    "  -fstack-protector-strong    \tAdd stack guard for some function.\n",
    {driverCategory, meCategory, cgCategory}, kOptCommon | kOptOptimization);

maplecl::Option<bool> stackProtectorAll({"-fstack-protector-all", "--stack-protector-all"},
    "  -fstack-protector-all       \tAdd stack guard for all functions.\n",
    {driverCategory, meCategory, cgCategory}, kOptCommon | kOptOptimization,
    maplecl::DisableWith("-fno-stack-protector-all"));

maplecl::Option<bool> inlineAsWeak({"-inline-as-weak", "--inline-as-weak"},
    "  --inline-as-weak            \tSet inline functions as weak symbols as it's in C++\n",
    {driverCategory, hir2mplCategory}, kOptMaple);

maplecl::Option<bool> legalizeNumericTypes({"--legalize-numeric-types"},
    "  --legalize-numeric-types    \tEnable legalize-numeric-types pass\n",
    {driverCategory}, kOptMaple, maplecl::DisableWith("--no-exp nd128floats"), maplecl::kHide, maplecl::Init(true));

maplecl::Option<bool> oMD({"-MD"},
    "  -MD                         \tWrite a depfile containing user and system headers.\n",
    {driverCategory, clangCategory}, kOptFront);

maplecl::Option<bool> fNoPlt({"-fno-plt"},
    "  -fno-plt                    \tDo not use the PLT to make function calls.\n",
    {driverCategory});

maplecl::Option<bool> fRegStructReturn({"-freg-struct-return"},
    "  -freg-struct-return         \tOverride the default ABI to return small structs in registers.\n",
    {driverCategory}, kOptCommon | kOptOptimization);

maplecl::Option<bool> fTreeVectorize({"-ftree-vectorize"},
    "  -ftree-vectorize            \tEnable vectorization on trees.\n",
    {driverCategory}, kOptCommon | kOptOptimization);

maplecl::Option<bool> gcSections({"--gc-sections"},
    "  -gc-sections                \tDiscard all sections that are not accessed in the final elf.\n",
    {driverCategory, ldCategory}, kOptMaple | kOptLd, maplecl::kHide);

maplecl::Option<bool> copyDtNeededEntries({"--copy-dt-needed-entries"},
    "  --copy-dt-needed-entries    \tGenerate a DT_NEEDED entry for each lib that is present in the link command.\n",
    {driverCategory, ldCategory}, kOptMaple | kOptLd, maplecl::kHide);

maplecl::Option<bool> sOpt({"-s"},
    "  -s                          \tRemove all symbol table and relocation information from the executable\n",
    {driverCategory, ldCategory}, kOptDriver | kOptLd);

maplecl::Option<bool> noStdinc({"-nostdinc"},
    "  -nostdinc                   \tDo not search standard system include directories(those specified with "
    "-isystem will still be used).\n",
    {driverCategory, clangCategory}, kOptFront);

maplecl::Option<bool> pie({"-pie"},
    "  -pie                        \tCreate a position independent executable.\n",
    {driverCategory, ldCategory}, kOptDriver | kOptLd);

maplecl::Option<bool> linkerTimeOpt({"-flto"},
    "  -flto                       \tEnable LTO in 'full' mode.\n",
    {driverCategory, cgCategory}, kOptCommon);

maplecl::Option<bool> shared({"-shared"},
    "  -shared                     \tCreate a shared library.\n",
    {driverCategory, ldCategory}, kOptDriver | kOptLd);

maplecl::Option<bool> rdynamic({"-rdynamic"},
    "  -rdynamic                   \tPass the flag '-export-dynamic' to the ELF linker, on targets that support it. "
    "This instructs the linker to add all symbols, not only used ones, to the dynamic symbol table.\n",
    {driverCategory, ldCategory}, kOptDriver | kOptLd);

maplecl::Option<bool> dndebug({"-DNDEBUG"},
    "  -DNDEBUG                    \t\n",
    {driverCategory, ldCategory, clangCategory}, kOptFront | kOptLd);

maplecl::Option<bool> useSignedChar({"-fsigned-char", "-usesignedchar", "--usesignedchar"},
    "  -fsigned-char               \tuse signed char\n",
    {driverCategory, clangCategory, hir2mplCategory}, kOptFront | kOptNotFiltering,
    maplecl::DisableWith("-funsigned-char"));

maplecl::Option<bool> pthread({"-pthread"},
    "  -pthread                    \tDefine additional macros required for using the POSIX threads library.\n",
    {driverCategory, clangCategory, asCategory, ldCategory}, kOptFront | kOptDriver | kOptLd);

maplecl::Option<bool> passO2ToClang({"-pO2ToCl"},
    "  -pthread                    \tTmp for option -D_FORTIFY_SOURCE=2.\n",
    {clangCategory}, kOptMaple, maplecl::kHide);

maplecl::Option<bool> defaultSafe({"-defaultSafe", "--defaultSafe"},
    "  --defaultSafe               \tTreat unmarked function or blocks as safe region by default.\n",
    {driverCategory, hir2mplCategory}, kOptMaple);

maplecl::Option<bool> onlyPreprocess({"-E"},
    "  -E                          \tPreprocess only; do not compile, assemble or link.\n",
    {driverCategory, clangCategory}, kOptDriver | kOptFront);

maplecl::Option<bool> tailcall({"-foptimize-sibling-calls", "--tailcall"},
    "  --tailcall/-foptimize-sibling-calls\n"
    "                              \tDo tail call optimization.\n"
    "  --no-tailcall/-fno-optimize-sibling-calls\n",
    {cgCategory, driverCategory}, kOptCommon | kOptOptimization,
    maplecl::DisableEvery({"-fno-optimize-sibling-calls", "--no-tailcall"}));

maplecl::Option<bool> noStdLib({"-nostdlib"},
    "  -nostdlib                   \tDo not look for object files in standard path.\n",
    {driverCategory, ldCategory}, kOptDriver | kOptLd);

maplecl::Option<bool> r({"-r"},
    "  -r                          \tProduce a relocatable object as output. This is also known as partial linking.\n",
    {driverCategory, ldCategory}, kOptDriver | kOptLd);

maplecl::Option<bool> wCCompat({"-Wc++-compat"},
    "  -Wc++-compat                \tWarn about C constructs that are not in the common subset of C and C++.\n",
    {driverCategory, asCategory, ldCategory}, kOptLd | kOptAs);

maplecl::Option<bool> wpaa({"-wpaa", "--wpaa"},
    "  --wpaa                       \tEnable whole program ailas analysis.\n",
    {driverCategory, hir2mplCategory}, kOptMaple);

maplecl::Option<bool> fm({"-fm", "--fm"},
    "  -fm                         \tStatic function merge will be enabled only when wpaa is enabled "
    "at the same time.\n",
    {driverCategory, hir2mplCategory}, kOptMaple, maplecl::DisableEvery({"-no-fm", "--no-fm"}));

maplecl::Option<bool> dumpTime({"--dump-time", "-dump-time"},
    "  --dump-time                  \tDump time.\n",
    {driverCategory, hir2mplCategory}, kOptMaple);

maplecl::Option<bool> aggressiveTlsLocalDynamicOpt({"--tls-local-dynamic-opt"},
    "  --tls-local-dynamic-opt     \tDo aggressive tls local dynamic opt.\n",
    {driverCategory}, kOptMaple);

maplecl::Option<bool> aggressiveTlsLocalDynamicOptMultiThread({"--tls-local-dynamic-opt-multi-thread"},
    "  --tls-local-dynamic-opt-multi-thread     \tDo aggressive tls local dynamic opt for multi thread.\n",
    {driverCategory});

maplecl::Option<bool> aggressiveTlsSafeAnchor({"--tls-safe-anchor"},
    "  --tls-safe-anchor    \taggressive tls local dynamic opt use safe site for saving anchor\n",
    {driverCategory});

maplecl::Option<bool> marchArmV8({"-march=armv8-a"},
    "  -march=armv8-a              \tGenerate code for armv8-a.\n",
    {driverCategory, clangCategory, asCategory, ldCategory}, kOptFront | kOptLd | kOptNotFiltering);

maplecl::Option<bool> marchArmV8Crc({"-march=armv8-a+crc"},
    "  -march=armv8-a+crc          \tGenerate code for armv8-a+crc.\n",
    {driverCategory, clangCategory, asCategory, ldCategory}, kOptFront | kOptLd | kOptNotFiltering);

maplecl::Option<bool> oStaticLibasan({"-static-libasan"},
    "  -static-libasan             \tThe -static-libasan option directs the MAPLE driver to link libasan statically,"
    " without necessarily linking other libraries statically.\n",
    {driverCategory, ldCategory}, kOptDriver | kOptLd);

maplecl::Option<bool> oStaticLiblsan({"-static-liblsan"},
    "  -static-liblsan             \tThe -static-liblsan option directs the MAPLE driver to link liblsan statically,"
    " without necessarily linking other libraries statically.\n",
    {driverCategory, ldCategory}, kOptDriver | kOptLd);

maplecl::Option<bool> oStaticLibtsan({"-static-libtsan"},
    "  -static-libtsan             \tThe -static-libtsan option directs the MAPLE driver to link libtsan statically, "
    "without necessarily linking other libraries statically.\n",
    {driverCategory, ldCategory}, kOptDriver | kOptLd);

maplecl::Option<bool> oStaticLibubsan({"-static-libubsan"},
    "  -static-libubsan            \tThe -static-libubsan option directs the MAPLE driver to link libubsan statically,"
    " without necessarily linking other libraries statically.\n",
    {driverCategory, ldCategory}, kOptDriver | kOptLd);

maplecl::Option<bool> oStaticLibmpx({"-static-libmpx"},
    "  -static-libmpx              \tThe -static-libmpx option directs the MAPLE driver to link libmpx statically,"
    " without necessarily linking other libraries statically.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oStaticLibmpxwrappers({"-static-libmpxwrappers"},
    "  -static-libmpxwrappers      \tThe -static-libmpxwrappers option directs the MAPLE driver to link "
    "libmpxwrappers statically, without necessarily linking other libraries statically.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oSymbolic({"-symbolic"},
    "  -symbolic                   \tWarn about any unresolved references (unless overridden by the link editor "
    "option -Xlinker -z -Xlinker defs).\n",
    {driverCategory, ldCategory}, kOptDriver | kOptLd);

maplecl::Option<bool> oNoPie({"-no-pie"},
    "  -no-pie                     \tDon't create a position independent executable.\n",
    {driverCategory, ldCategory}, kOptDriver | kOptLd);

maplecl::Option<bool> staticLibmplpgo({"--static-libmplpgo"},
    "  --static-libmplpgo          \tStatic link using libmplpgo\n",
    {driverCategory, cgCategory}, kOptMaple);

maplecl::Option<bool> fPIE({"-fPIE", "--fPIE"},
    "  --fPIE                      \tGenerate position-independent executable in large mode.\n"
    "  --no-pie/-fno-pie           \n",
    {cgCategory, driverCategory, ldCategory}, kOptCommon | kOptLd | kOptNotFiltering);

maplecl::Option<bool> oUndef({"-undef"},
    "  -undef                      \tDo not predefine system-specific and GCC-specific macros.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::kHide);

maplecl::Option<bool> oTrigraphs({"-trigraphs"},
    "  -trigraphs                  \t-trigraphs   Support ISO C trigraphs.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::kHide);

maplecl::Option<bool> oStd03({"-std=c++03"},
    "  -std=c++03                  \tConform to the ISO 1998 C++ standard revised by the 2003 technical corrigendum.\n",
    {driverCategory, clangCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oStd0x({"-std=c++0x"},
    "  -std=c++0x                 \tDeprecated in favor of -std=c++11.\n",
    {driverCategory, clangCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oStd11({"-std=c++11"},
    "  -std=c++11                  \tConform to the ISO 2011 C++ standard.\n",
    {driverCategory, ldCategory, clangCategory, unSupCategory}, kOptLd, maplecl::kHide);

maplecl::Option<bool> oStd14({"-std=c++14"},
    "  -std=c++14                  \tConform to the ISO 2014 C++ standard.\n",
    {driverCategory, clangCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oStd17({"-std=c++17"},
    "  -std=c++17                  \tConform to the ISO 2017 C++ standard.\n",
    {driverCategory, clangCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oStd1y({"-std=c++1y"},
    "  -std=c++1y                  \tDeprecated in favor of -std=c++14.\n",
    {driverCategory, clangCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oStd1z({"-std=c++1z"},
    "  -std=c++1z                  \tConform to the ISO 2017(?) C++ draft standard (experimental and incomplete "
    "support).\n",
    {driverCategory, clangCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oStd98({"-std=c++98"},
    "  -std=c++98                  \tConform to the ISO 1998 C++ standard revised by the 2003 technical corrigendum.\n",
    {driverCategory, clangCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oStd11p({"-std=c11"},
    "  -std=c11                    \tConform to the ISO 2011 C standard.\n",
    {driverCategory, clangCategory}, kOptFront);

maplecl::Option<bool> oStdc1x({"-std=c1x"},
    "  -std=c1x                    \tDeprecated in favor of -std=c11.\n",
    {driverCategory, clangCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oStd89({"-std=c89"},
    "  -std=c89                    \tConform to the ISO 1990 C standard.\n",
    {driverCategory, clangCategory, unSupCategory, hir2mplCategory}, maplecl::kHide);

maplecl::Option<bool> oStd90({"-std=c90"},
    "  -std                        \tConform to the ISO 1998 C++ standard revised by the 2003 technical corrigendum.\n",
    {driverCategory, clangCategory, unSupCategory, hir2mplCategory}, maplecl::kHide);

maplecl::Option<bool> oStd99({"-std=c99"},
    "  -std=c99                    \tConform to the ISO 1999 C standard.\n",
    {driverCategory, clangCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oStd9x({"-std=c9x"},
    "  -std=c9x                    \tDeprecated in favor of -std=c99.\n",
    {driverCategory, clangCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oStd2003({"-std=f2003"},
    "  -std=f2003                  \tConform to the ISO Fortran 2003 standard.\n",
    {driverCategory, clangCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oStd2008({"-std=f2008"},
    "  -std=f2008                  \tConform to the ISO Fortran 2008 standard.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oStd2008ts({"-std=f2008ts"},
    "  -std=f2008ts                \tConform to the ISO Fortran 2008 standard including TS 29113.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oStdf95({"-std=f95"},
    "  -std=f95                    \tConform to the ISO Fortran 95 standard.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oStdgnu({"-std=gnu"},
    "  -std=gnu                    \tConform to nothing in particular.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oStdgnu03p({"-std=gnu++03"},
    "  -std=gnu++03                \tConform to the ISO 1998 C++ standard revised by the 2003 technical corrigendum "
    "with GNU extensions.\n",
    {driverCategory, clangCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oStdgnuoxp({"-std=gnu++0x"},
    "  -std=gnu++0x                \tDeprecated in favor of -std=gnu++11.\n",
    {driverCategory, clangCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oStdgnu11p({"-std=gnu++11"},
    "  -std=gnu++11                \tConform to the ISO 2011 C++ standard with GNU extensions.\n",
    {driverCategory, clangCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oStdgnu14p({"-std=gnu++14"},
    "  -std=gnu++14                \tConform to the ISO 2014 C++ standard with GNU extensions.\n",
    {driverCategory, clangCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oStdgnu17p({"-std=gnu++17"},
    "  -std=gnu++17                \tConform to the ISO 2017 C++ standard with GNU extensions.\n",
    {driverCategory, clangCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oStdgnu1yp({"-std=gnu++1y"},
    "  -std=gnu++1y                \tDeprecated in favor of -std=gnu++14.\n",
    {driverCategory, clangCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oStdgnu1zp({"-std=gnu++1z"},
    "  -std=gnu++1z                \tConform to the ISO 201z(7?) C++ draft standard with GNU extensions "
    "(experimental and incomplete support).\n",
    {driverCategory, clangCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oStdgnu98p({"-std=gnu++98"},
    "  -std=gnu++98                \tConform to the ISO 1998 C++ standard revised by the 2003 technical corrigendum "
    "with GNU extensions.\n",
    {driverCategory, clangCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oStdgnu11({"-std=gnu11"},
    "  -std=gnu11                  \tConform to the ISO 2011 C standard with GNU extensions.\n",
    {driverCategory, clangCategory}, kOptFront);

maplecl::Option<bool> oStdgnu1x({"-std=gnu1x"},
    "  -std=gnu1x                  \tDeprecated in favor of -std=gnu11.\n",
    {driverCategory, clangCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oStdgnu89({"-std=gnu89"},
    "  -std=gnu89                  \tConform to the ISO 1990 C standard with GNU extensions.\n",
    {driverCategory, clangCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oStdgnu90({"-std=gnu90"},
    "  -std=gnu90                  \tConform to the ISO 1990 C standard with GNU extensions.\n",
    {driverCategory, clangCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oStdgnu99({"-std=gnu99"},
    "  -std=gnu99                  \tConform to the ISO 1999 C standard with GNU extensions.\n",
    {driverCategory, clangCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oStdgnu9x({"-std=gnu9x"},
    "  -std=gnu9x                  \tDeprecated in favor of -std=gnu99.\n",
    {driverCategory, clangCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oStd1990({"-std=iso9899:1990"},
    "  -std=iso9899:1990           \tConform to the ISO 1990 C standard.\n",
    {driverCategory, clangCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oStd1994({"-std=iso9899:199409"},
    "  -std=iso9899:199409         \tConform to the ISO 1990 C standard as amended in 1994.\n",
    {driverCategory, clangCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oStd1999({"-std=iso9899:1999"},
    "  -std=iso9899:1999           \tConform to the ISO 1999 C standard.\n",
    {driverCategory, clangCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oStd199x({"-std=iso9899:199x"},
    "  -std=iso9899:199x           \tDeprecated in favor of -std=iso9899:1999.\n",
    {driverCategory, clangCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oStd2011({"-std=iso9899:2011"},
    "  -std=iso9899:2011           \tConform to the ISO 2011 C standard.\n",
    {driverCategory, clangCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oStdlegacy({"-std=legacy"},
    "  -std=legacy                 \tAccept extensions to support legacy code.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oH({"-H"},
    "  -H                          \tPrint the name of header files as they are used.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::kHide);

maplecl::Option<bool> oM({"-M"},
    "  -M                          \tGenerate make dependencies.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::kHide);

maplecl::Option<bool> oDI({"-dI"},
    "  -dI                         \tOutput '#include' directives in addition to the result of preprocessing.\n",
    {driverCategory, clangCategory}, kOptFront);

maplecl::Option<bool> oDM({"-dM"},
    "  -dM                         \tInstead of the normal output, generate a list of '#define' directives for all "
    "the macros defined during the execution of the preprocessor, including predefined macros. \n",
    {driverCategory, clangCategory}, kOptFront);

maplecl::Option<bool> oDN({"-dN"},
    "  -dN                         \tLike -dD, but emit only the macro names, not their expansions.\n",
    {driverCategory, clangCategory}, kOptFront);

maplecl::Option<bool> oDU({"-dU"},
    "  -dU                         \tLike -dD except that only macros that are expanded.\n",
    {driverCategory, clangCategory}, kOptFront);

maplecl::Option<bool> oFdollarsInIdentifiers({"-fdollars-in-identifiers"},
    "  -fdollars-in-identifiers    \tPermit '$' as an identifier character.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-fno-dollars-in-identifiers"));

maplecl::Option<bool> oFextendedIdentifiers({"-fextended-identifiers"},
    "  -fextended-identifiers      \tPermit universal character names (\\u and \\U) in identifiers.\n",
    {driverCategory, clangCategory}, kOptFront);

maplecl::Option<bool> oFpchPreprocess({"-fpch-preprocess"},
    "  -fpch-preprocess            \tLook for and use PCH files even when preprocessing.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::kHide);

maplecl::Option<bool> oLargeP({"-P"},
    "  -P                          \tDo not generate #line directives.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::kHide);

maplecl::Option<bool> oMfixCortexA53835769({"-mfix-cortex-a53-835769"},
    "  -mfix-cortex-a53-835769     \tWorkaround for ARM Cortex-A53 Erratum number 835769.\n",
    {driverCategory, ldCategory}, kOptLd, maplecl::DisableWith("-mno-fix-cortex-a53-835769"));

maplecl::Option<bool> oMfixCortexA53843419({"-mfix-cortex-a53-843419"},
    "  -mfix-cortex-a53-843419     \tWorkaround for ARM Cortex-A53 Erratum number 843419.\n",
    {driverCategory, ldCategory}, kOptLd, maplecl::DisableWith("-mno-fix-cortex-a53-843419"));

maplecl::Option<bool> oFuseLdBfd({"-fuse-ld=bfd"},
    "  -fuse-ld=bfd                \tUse the bfd linker instead of the default linker.\n",
    {driverCategory, ldCategory}, kOptCommon | kOptDriver | kOptLd, maplecl::kHide);

maplecl::Option<bool> oFuseLdGold({"-fuse-ld=gold"},
    "  -fuse-ld=gold               \tUse the gold linker instead of the default linker.\n",
    {driverCategory, ldCategory}, kOptCommon | kOptDriver | kOptLd, maplecl::kHide);

maplecl::Option<bool> oNodefaultlibs({"-nodefaultlibs"},
    "  -nodefaultlibs              \tDo not use the standard system libraries when linking. \n",
    {driverCategory, ldCategory}, kOptDriver | kOptLd, maplecl::kHide);

maplecl::Option<bool> oNostartfiles({"-nostartfiles"},
    "  -nostartfiles               \tDo not use the standard system startup files when linking. \n",
    {driverCategory, ldCategory}, kOptDriver | kOptLd, maplecl::kHide);

maplecl::Option<bool> oSharedLibgcc({"-shared-libgcc"},
    "  -shared-libgcc              \tOn systems that provide libgcc as a shared library, these options force the use"
    " of either the shared or static version, respectively.\n",
    {driverCategory, ldCategory}, kOptDriver | kOptLd, maplecl::kHide);

maplecl::Option<bool> oStaticLibgcc({"-static-libgcc"},
    "  -static-libgcc              \tOn systems that provide libgcc as a shared library, these options "
    "force the use of either the shared or static version, respectively.\n",
    {driverCategory, ldCategory}, kOptDriver | kOptLd, maplecl::kHide);

maplecl::Option<bool> oStaticLibstdc({"-static-libstdc++"},
    "  -static-libstdc++           \tWhen the g++ program is used to link a C++ program, it normally automatically "
    "links against libstdc++. \n",
    {driverCategory, ldCategory}, kOptDriver | kOptLd, maplecl::kHide);

maplecl::Option<bool> oMG({"-MG"},
    "  -MG                         \tTreat missing header files as generated files.\n",
    {driverCategory, clangCategory}, kOptFront);

maplecl::Option<bool> oMM({"-MM"},
    "  -MM                         \tLike -M but ignore system header files.\n",
    {driverCategory, clangCategory}, kOptFront);

maplecl::Option<bool> oMMD({"-MMD"},
    "  -MMD                        \tLike -MD but ignore system header files.\n",
    {driverCategory, clangCategory}, kOptFront);

maplecl::Option<bool> oMP({"-MP"},
    "  -MP                         \tGenerate phony targets for all headers.\n",
    {driverCategory, clangCategory}, kOptFront);

maplecl::Option<bool> oFgnu89Inline({"-fgnu89-inline"},
    "  -fgnu89-inline              \tUse traditional GNU semantics for inline functions.\n",
    {driverCategory, unSupCategory, hir2mplCategory}, maplecl::DisableWith("-fno-gnu89-inline"), maplecl::kHide);

maplecl::Option<bool> oAnsi({"-ansi", "--ansi"},
    "  -ansi                       \tIn C mode, this is equivalent to -std=c90. In C++ mode, it is equivalent "
    "to -std=c++98.\n",
    {driverCategory, unSupCategory, hir2mplCategory}, maplecl::kHide);

maplecl::Option<bool> oFnoBuiltin({"-fno-builtin", "-fno-builtin-function"},
    "  -fno-builtin                \tDon't recognize built-in functions that do not begin with '__builtin_' as"
    " prefix.\n",
    {driverCategory, hir2mplCategory}, kOptCommon | kOptOptimization | kOptMaple);

maplecl::Option<bool> exportMpltInline({"-fexport-inline-mplt"},
    "  -fexport-inline-mplt        \tExport mplt for cross module inline.\n",
    {driverCategory, hir2mplCategory}, kOptCommon | kOptOptimization | kOptMaple,
     maplecl::DisableWith("-fno-export-inline-mplt"), maplecl::Init(false));

maplecl::Option<bool> importMpltInline({"-fimport-inline-mplt"},
    "  -fimport-inline-mplt        \tImport mplt for cross module inline.\n",
    {driverCategory, mpl2mplCategory}, kOptCommon | kOptOptimization | kOptMaple,
     maplecl::DisableWith("-fno-import-inline-mplt"), maplecl::Init(false));

/* ##################### STRING Options ############################################################### */

maplecl::Option<std::string> help({"--help", "-h"},
    "  --help                      \tPrint help\n",
    {driverCategory}, kOptCommon | kOptDriver, maplecl::kOptionalValue);

maplecl::Option<std::string> infile({"--infile"},
    "  --infile file1,file2,file3  \tInput files.\n",
    {driverCategory}, kOptMaple);

maplecl::Option<std::string> mplt({"--mplt", "-mplt"},
    "  --mplt=file1,file2,file3    \tImport mplt files.\n",
    {driverCategory, dex2mplCategory, jbc2mplCategory}, kOptMaple, maplecl::kHide);

maplecl::Option<std::string> partO2({"--partO2"},
    "  --partO2                    \tSet func list for O2.\n",
    {driverCategory}, kOptMaple);

maplecl::List<std::string> jbc2mplOpt({"--jbc2mpl-opt"},
    "  --jbc2mpl-opt               \tSet options for jbc2mpl.\n",
    {driverCategory}, kOptMaple, maplecl::kHide);

maplecl::List<std::string> hir2mplOpt({"--hir2mpl-opt"},
    "  --hir2mpl-opt               \tSet options for hir2mpl.\n",
    {driverCategory}, kOptMaple, maplecl::kHide);

maplecl::List<std::string> clangOpt({"--clang-opt"},
    "  --clang-opt                 \tSet options for clang as AST generator.\n",
    {driverCategory}, kOptMaple, maplecl::kHide);

maplecl::List<std::string> asOpt({"--as-opt"},
    "  --as-opt                    \tSet options for as.\n",
    {driverCategory}, kOptMaple, maplecl::kHide);

maplecl::List<std::string> ldOpt({"--ld-opt"},
    "  --ld-opt                    \tSet options for ld.\n",
    {driverCategory}, kOptMaple, maplecl::kHide);

maplecl::List<std::string> dex2mplOpt({"--dex2mpl-opt"},
    "  --dex2mpl-opt               \tSet options for dex2mpl.\n",
    {driverCategory}, kOptMaple, maplecl::kHide);

maplecl::List<std::string> mplipaOpt({"--mplipa-opt"},
    "  --mplipa-opt                \tSet options for mplipa.\n",
    {driverCategory}, kOptMaple, maplecl::kHide);

maplecl::List<std::string> mplcgOpt({"--mplcg-opt"},
    "  --mplcg-opt                 \tSet options for mplcg.\n",
    {driverCategory}, kOptMaple, maplecl::kHide);

maplecl::List<std::string> meOpt({"--me-opt"},
    "  --me-opt                    \tSet options for me.\n",
    {driverCategory}, kOptMaple, maplecl::kHide);

maplecl::List<std::string> mpl2mplOpt({"--mpl2mpl-opt"},
    "  --mpl2mpl-opt               \tSet options for mpl2mpl.\n",
    {driverCategory}, kOptMaple, maplecl::kHide);

maplecl::Option<std::string> profile({"--profile"},
    "  --profile                   \tFor PGO optimization. eg: --profile=list_file.\n",
    {driverCategory, dex2mplCategory, mpl2mplCategory, cgCategory}, kOptCommon);

maplecl::Option<std::string> run({"--run"},
    "  --run=cmd1:cmd2             \tThe name of executables that are going to execute. IN SEQUENCE.\nSeparated "
    "by \":\".Available exe names: jbc2mpl, me, mpl2mpl, mplcg. Input file must match the tool can handle.\n",
    {driverCategory}, kOptMaple, maplecl::kHide);

maplecl::Option<std::string> optionOpt({"--option"},
    "  --option=\"opt1:opt2\"      \tOptions for each executable, separated by \":\". The sequence must match the "
    "sequence in --run.\n",
    {driverCategory}, kOptMaple, maplecl::kHide);

maplecl::List<std::string> ldLib({"-l"},
    "  -l <lib>                    \tLinks with a library file.\n",
    {driverCategory, ldCategory}, kOptDriver | kOptLd, maplecl::kJoinedValue);

maplecl::List<std::string> ldLibPath({"-L"},
    "  -L <libpath>                \tAdd directory to library search path.\n",
    {driverCategory, ldCategory}, kOptDriver | kOptLd, maplecl::kJoinedValue);

maplecl::List<std::string> enableMacro({"-D"},
    "  -D <macro>=<value>          \tDefine <macro> to <value> (or 1 if <value> omitted).\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::kJoinedValue);

maplecl::List<std::string> disableMacro({"-U"},
    "  -U <macro>                  \tUndefine macro <macro>.\n",
    {driverCategory, clangCategory}, kOptFront,  maplecl::kJoinedValue);

maplecl::List<std::string> includeDir({"-I"},
    "  -I <dir>                    \tAdd directory to include search path.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::kJoinedValue);

maplecl::List<std::string> includeSystem({"-isystem"},
    "  -isystem <dir>              \tAdd directory to SYSTEM include search path.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::kJoinedValue);

maplecl::Option<std::string> output({"-o"},
    "  -o <outfile>                \tPlace the output into <file>.\n",
    {driverCategory}, kOptMaple | kOptDriver | kOptCommon, maplecl::kJoinedValue, maplecl::Init("a.out"));

maplecl::Option<std::string> saveTempOpt({"--save-temps"},
    "  --save-temps                \tDo not delete intermediate files. --save-temps Save all intermediate files.\n"
    "                              \t--save-temps=file1,file2,file3 Save the target files.\n",
    {driverCategory}, kOptDriver, maplecl::kOptionalValue);

maplecl::Option<std::string> target({"--target", "-target"},
    "  --target=<arch><abi>        \tDescribe target platform. Example: --target=aarch64-gnu or "
    "--target=aarch64_be-gnuilp32\n",
    {driverCategory, clangCategory, hir2mplCategory, dex2mplCategory, ipaCategory}, kOptFront | kOptNotFiltering);

maplecl::Option<std::string> oMT({"-MT"},
    "  -MT<args>                   \tSpecify name of main file output in depfile.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::kJoinedValue);

maplecl::Option<std::string> oMF({"-MF"},
    "  -MF<file>                   \tWrite depfile output from -MD, -M to <file>.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::kJoinedValue);

maplecl::Option<std::string> oWl({"-Wl"},
    "  -Wl,<arg>                   \tPass the comma separated arguments in <arg> to the linker.\n",
    {driverCategory, ldCategory}, kOptDriver | kOptLd, maplecl::kJoinedValue);

maplecl::Option<std::string> fVisibility({"-fvisibility"},
    "  -fvisibility=[default|hidden|protected|internal]\n"
    "                              \tSet the default symbol visibility for every global declaration unless "
    "overridden within the code.\n", {driverCategory}, kOptCommon);

maplecl::Option<std::string> aggressiveTlsWarmupFunction({"--tls-warmup-function"},
    "  --tls-warmup-function    \taggressive tls warmup function\n",
    {driverCategory});

maplecl::Option<std::string> sysRoot({"--sysroot"},
    "  --sysroot <value>           \tSet the root directory of the target platform.\n"
    "  --sysroot=<value>           \tSet the root directory of the target platform.\n",
    {driverCategory, clangCategory, ldCategory}, kOptFront | kOptDriver | kOptLd | kOptNotFiltering);

maplecl::Option<std::string> specs({"-specs"},
    "  -specs <value>              \tOverride built-in specs with the contents of <file>.\n",
    {driverCategory, asCategory, ldCategory}, kOptDriver | kOptLd);

maplecl::Option<std::string> folder({"-tmp-folder"},
    "  -tmp-folder                 \tsave former folder when generating multiple output.\n",
    {driverCategory}, kOptMaple, maplecl::kHide);

maplecl::Option<std::string> imacros({"-imacros", "--imacros"},
    "  -imacros                    \tExactly like '-include', except that any output produced by scanning FILE is "
    "thrown away.\n",
    {driverCategory, clangCategory}, kOptFront);

maplecl::Option<std::string> fdiagnosticsColor({"-fdiagnostics-color"},
    "  -fdiagnostics-color         \tUse color in diagnostics. WHEN is 'never', 'always', or 'auto'.\n",
    {driverCategory, clangCategory, asCategory, ldCategory}, kOptFront | kOptCommon | kOptLd | kOptNotFiltering);

maplecl::Option<std::string> mtlsSize({"-mtls-size"},
    "  -mtls-size                  \tSpecify bit size of immediate TLS offsets. Valid values are 12, 24, 32, 48. "
    "This option requires binutils 2.26 or newer.\n",
    {driverCategory, asCategory, ldCategory});

maplecl::Option<std::string> ftlsModel({"-ftls-model"},
    "  -ftls-model=[global-dynamic|local-dynamic|initial-exec|local-exec|warmup-dynamic]\n"
    "                               \tAlter the thread-local storage model to be used.\n",
    {driverCategory}, kOptCommon);

maplecl::Option<std::string> functionReorderAlgorithm({"--function-reorder-algorithm"},
    "  --function-reorder-algorithm=[call-chain-clustering]\n"
    "                               \tChoose function reorder algorithm\n",
    {driverCategory, cgCategory}, kOptMaple);

maplecl::Option<std::string> functionReorderProfile({"--function-reorder-profile"},
    "  --function-reorder-profile=filepath\n"
    "                               \tProfile for function reorder\n",
    {driverCategory, cgCategory}, kOptMaple);

#ifdef ENABLE_MAPLE_SAN
maplecl::Option<std::string> sanitizer({"-fsanitizer"},
    "  -fsanitizer=address          \tEnable AddressSanitizer.\n",
    {driverCategory, meCategory}, kOptMaple);
#endif

maplecl::Option<std::string> rootPath({"-rootPath"},
    "  -rootPath          \tPass tmp root path to hir2mpl\n",
    {driverCategory, hir2mplCategory}, kOptMaple, maplecl::kHide);

maplecl::Option<std::string> oMQ({"-MQ"},
    "  -MQ                         \t-MQ <target> o       Add a MAKE-quoted target.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::kJoinedValue);

maplecl::Option<std::string> oXlinker({"-Xlinker"},
    "  -Xlinker                    \tPass option as an option to the linker. \n",
    {driverCategory, ldCategory}, kOptDriver | kOptLd, maplecl::kHide);

maplecl::Option<std::string> oXpreprocessor({"-Xpreprocessor"},
    "  -Xpreprocessor              \tPass option as an option to the preprocessor. \n",
    {driverCategory, clangCategory}, kOptFront, maplecl::kHide);

maplecl::Option<std::string> oZ({"-z"},
    "  -z                          \tPassed directly on to the linker along with the keyword keyword.\n",
    {driverCategory, ldCategory}, kOptDriver | kOptLd, maplecl::kHide);

maplecl::Option<std::string> oU({"-u"},
    "  -u                          \tPretend the symbol symbol is undefined, to force linking of library modules "
    "to define it. \n",
    {driverCategory, ldCategory}, kOptDriver | kOptLd, maplecl::kHide);

maplecl::Option<std::string> oWp({"-Wp"},
    "  -Wp                         \tYou can use -Wp,option to bypass the compiler driver and pass "
    "option directly through to the preprocessor.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::kJoinedValue);

maplecl::Option<std::string> oT({"-T"},
    "  -T                          \tUse script as the linker script. This option is supported by most systems "
    "using the GNU linker\n",
    {driverCategory, ldCategory}, kOptDriver | kOptLd, maplecl::kHide);

maplecl::Option<std::string> oFtabstop({"-ftabstop="},
    "  -ftabstop=                  \tSet the distance between tab stops.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::kHide);

maplecl::Option<std::string> oFinputCharset({"-finput-charset="},
    "  -finput-charset=            \tSpecify the default character set for source files.\n",
    {driverCategory, clangCategory}, kOptFront);

maplecl::Option<std::string> oFexecCharset({"-fexec-charset="},
    "  -fexec-charset=             \tConvert all strings and character constants to character set <cset>.\n",
    {driverCategory, clangCategory}, kOptFront);

maplecl::Option<std::string> marchE({"-march="},
    "  -march=                     \tGenerate code for given CPU.\n",
    {driverCategory, clangCategory, asCategory, ldCategory, unSupCategory},
    kOptFront | kOptLd | kOptNotFiltering, maplecl::kHide);

maplecl::Option<std::string> oA({"-A"},
    "  -A<question>=<answer>       \tAssert the <answer> to <question>.  Putting '-' before <question> disables "
    "the <answer> to <question> assertion missing after %qs.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::kJoinedValue);

maplecl::Option<std::string> inlineMpltDir({"-finline-mplt-dir="},
    "  -finline-mplt-dir=       \tSpecify the inline mplt directory for exporting and importing.\n",
    {driverCategory, hir2mplCategory, mpl2mplCategory}, kOptCommon | kOptOptimization | kOptMaple, maplecl::Init(""));

/* ##################### DIGITAL Options ############################################################### */

maplecl::Option<uint32_t> helpLevel({"--level"},
    "  --level=NUM                 \tPrint the help info of specified level.\n"
    "                              \tNUM=0: All options (Default)\n"
    "                              \tNUM=1: Product options\n"
    "                              \tNUM=2: Experimental options\n"
    "                              \tNUM=3: Debug options\n",
    {driverCategory}, kOptMaple, maplecl::kHide);

maplecl::Option<uint32_t> funcInliceSize({"-func-inline-size", "--func-inline-size"},
    "  --func-inline-size           \tSet func inline size.\n",
    {driverCategory, hir2mplCategory}, kOptMaple);

maplecl::Option<uint32_t> initOptNum({"-initOptNum", "--initOptNum"},
    "  -initOptNum            \tif initNum > 10000, hir2mpl will Make optimization for initListElements, "
    "default value is 10000.\n",
    {driverCategory, hir2mplCategory}, kOptMaple, maplecl::Init(10000));

/* ##################### unsupport Options ############################################################### */

maplecl::Option<bool> oWhatsloaded({"-whatsloaded"},
    "  -whatsloaded                \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oWhyload({"-whyload"},
    "  -whyload                    \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oWLtoTypeMismatch({"-Wlto-type-mismatch"},
    "  -Wno-lto-type-mismatch      \tDuring the link-time optimization warn about type mismatches in global "
    "declarations from different compilation units. \n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-Wno-lto-type-mismatch"), maplecl::kHide);

maplecl::Option<bool> oWmisspelledIsr({"-Wmisspelled-isr"},
    "  -Wmisspelled-isr            \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oWrapper({"-wrapper"},
    "  -wrapper                    \tInvoke all subcommands under a wrapper program. \n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oXbindLazy({"-Xbind-lazy"},
    "  -Xbind-lazy                 \tEnable lazy binding of function calls. \n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oXbindNow({"-Xbind-now"},
    "  -Xbind-now                  \tDisable lazy binding of function calls. \n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oFworkingDirectory({"-fworking-directory"},
    "  -fworking-directory         \tGenerate a #line directive pointing at the current working directory.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-working-directory"), maplecl::kHide);

maplecl::Option<bool> oFwrapv({"-fwrapv"},
    "  -fwrapv                     \tAssume signed arithmetic overflow wraps around.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-fwrapv"), maplecl::kHide);

maplecl::Option<bool> oFzeroLink({"-fzero-link"},
    "  -fzero-link                 \tGenerate lazy class lookup (via objc_getClass()) for use inZero-Link mode.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-zero-link"), maplecl::kHide);

maplecl::Option<bool> oGcoff({"-gcoff"},
    "  -gcoff                      \tGenerate debug information in COFF format.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oGcolumnInfo({"-gcolumn-info"},
    "  -gcolumn-info               \tRecord DW_AT_decl_column and DW_AT_call_column in DWARF.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oGdwarf({"-gdwarf"},
    "  -gdwarf                     \tGenerate debug information in default version of DWARF format.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oGenDecls({"-gen-decls"},
    "  -gen-decls                  \tDump declarations to a .decl file.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oGfull({"-gfull"},
    "  -gfull                      \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oGgdb({"-ggdb"},
    "  -ggdb                       \tGenerate debug information in default extended format.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oGgnuPubnames({"-ggnu-pubnames"},
    "  -ggnu-pubnames              \tGenerate DWARF pubnames and pubtypes sections with GNU extensions.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oGnoColumnInfo({"-gno-column-info"},
    "  -gno-column-info            \tDon't record DW_AT_decl_column and DW_AT_call_column in DWARF.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oGnoRecordGccSwitches({"-gno-record-gcc-switches"},
    "  -gno-record-gcc-switches    \tDon't record gcc command line switches in DWARF DW_AT_producer.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oGnoStrictDwarf({"-gno-strict-dwarf"},
    "  -gno-strict-dwarf           \tEmit DWARF additions beyond selected version.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oGpubnames({"-gpubnames"},
    "  -gpubnames                  \tGenerate DWARF pubnames and pubtypes sections.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oGrecordGccSwitches({"-grecord-gcc-switches"},
    "  -grecord-gcc-switches       \tRecord gcc command line switches in DWARF DW_AT_producer.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oGsplitDwarf({"-gsplit-dwarf"},
    "  -gsplit-dwarf               \tGenerate debug information in separate .dwo files.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oGstabs({"-gstabs"},
    "  -gstabs                     \tGenerate debug information in STABS format.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oGstabsA({"-gstabs+"},
    "  -gstabs+                    \tGenerate debug information in extended STABS format.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oGstrictDwarf({"-gstrict-dwarf"},
    "  -gstrict-dwarf              \tDon't emit DWARF additions beyond selected version.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oGtoggle({"-gtoggle"},
    "  -gtoggle                    \tToggle debug information generation.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oGused({"-gused"},
    "  -gused                      \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oGvms({"-gvms"},
    "  -gvms                       \tGenerate debug information in VMS format.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oGxcoff({"-gxcoff"},
    "  -gxcoff                     \tGenerate debug information in XCOFF format.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oGxcoffA({"-gxcoff+"},
    "  -gxcoff+                    \tGenerate debug information in extended XCOFF format.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oGz({"-gz"},
    "  -gz                         \tGenerate compressed debug sections.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oHeaderpad_max_install_names({"-headerpad_max_install_names"},
    "  -headerpad_max_install_names  \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oI({"-I-"},
    "  -I-                         \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oIdirafter({"-idirafter"},
    "  -idirafter                  \t-idirafter <dir> o    Add <dir> oto the end of the system include path.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oImage_base({"-image_base"},
    "  -image_base                 \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oInit({"-init"},
    "  -init                       \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oInstall_name({"-install_name"},
    "  -install_name               \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oKeep_private_externs({"-keep_private_externs"},
    "  -keep_private_externs       \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oM1({"-m1"},
    "  -m1                         \tGenerate code for the SH1.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oM10({"-m10"},
    "  -m10                        \tGenerate code for a PDP-11/10.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oM128bitLongDouble({"-m128bit-long-double"},
    "  -m128bit-long-double        \tControl the size of long double type.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oM16({"-m16"},
    "  -m16                        \tGenerate code for a 16-bit.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oM16Bit({"-m16-bit"},
    "  -m16-bit                    \tArrange for stack frame, writable data and constants to all be 16-bit.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-mno-16-bit"), maplecl::kHide);

maplecl::Option<bool> oM2({"-m2"},
    "  -m2                         \tGenerate code for the SH2.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oM210({"-m210"},
    "  -m210                       \tGenerate code for the 210 processor.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oM2a({"-m2a"},
    "  -m2a                        \tGenerate code for the SH2a-FPU assuming the floating-point unit is in "
    "double-precision mode by default.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oM2e({"-m2e"},
    "  -m2e                        \tGenerate code for the SH2e.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oM2aNofpu({"-m2a-nofpu"},
    "  -m2a-nofpu                  \tGenerate code for the SH2a without FPU, or for a SH2a-FPU in such a way that "
    "the floating-point unit is not used.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oM2aSingle({"-m2a-single"},
    "  -m2a-single                 \tGenerate code for the SH2a-FPU assuming the floating-point unit is in "
    "single-precision mode by default.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oM2aSingleOnly({"-m2a-single-only"},
    "  -m2a-single-only            \tGenerate code for the SH2a-FPU, in such a way that no double-precision "
    "floating-point operations are used.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oM3({"-m3"},
    "  -m3                         \tGenerate code for the SH3.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oM31({"-m31"},
    "  -m31                        \tWhen -m31 is specified, generate code compliant to the GNU/Linux for "
    "S/390 ABI. \n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oM32({"-m32"},
    "  -m32                        \tGenerate code for 32-bit ABI.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oM32Bit({"-m32-bit"},
    "  -m32-bit                    \tArrange for stack frame, writable data and constants to all be 32-bit.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oM32bitDoubles({"-m32bit-doubles"},
    "  -m32bit-doubles             \tMake the double data type  32 bits (-m32bit-doubles) in size.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oM32r({"-m32r"},
    "  -m32r                       \tGenerate code for the M32R.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oM32r2({"-m32r2"},
    "  -m32r2                      \tGenerate code for the M32R/2.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oM32rx({"-m32rx"},
    "  -m32rx                      \tGenerate code for the M32R/X.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oM340({"-m340"},
    "  -m340                       \tGenerate code for the 210 processor.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oM3dnow({"-m3dnow"},
    "  -m3dnow                     \tThese switches enable the use of instructions in the m3dnow.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oM3dnowa({"-m3dnowa"},
    "  -m3dnowa                    \tThese switches enable the use of instructions in the m3dnowa.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oM3e({"-m3e"},
    "  -m3e                        \tGenerate code for the SH3e.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oM4({"-m4"},
    "  -m4                         \tGenerate code for the SH4.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oM4100({"-m4-100"},
    "  -m4-100                     \tGenerate code for SH4-100.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oM4100Nofpu({"-m4-100-nofpu"},
    "  -m4-100-nofpu               \tGenerate code for SH4-100 in such a way that the floating-point unit is "
    "not used.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oM4100Single({"-m4-100-single"},
    "  -m4-100-single              \tGenerate code for SH4-100 assuming the floating-point unit is in "
    "single-precision mode by default.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oM4100SingleOnly({"-m4-100-single-only"},
    "  -m4-100-single-only         \tGenerate code for SH4-100 in such a way that no double-precision "
    "floating-point operations are used.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oM4200({"-m4-200"},
    "  -m4-200                     \tGenerate code for SH4-200.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oM4200Nofpu({"-m4-200-nofpu"},
    "  -m4-200-nofpu               \tGenerate code for SH4-200 without in such a way that the floating-point "
    "unit is not used.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oM4200Single({"-m4-200-single"},
    "  -m4-200-single              \tGenerate code for SH4-200 assuming the floating-point unit is in "
    "single-precision mode by default.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oM4200SingleOnly({"-m4-200-single-only"},
    "  -m4-200-single-only         \tGenerate code for SH4-200 in such a way that no double-precision "
    "floating-point operations are used.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oM4300({"-m4-300"},
    "  -m4-300                     \tGenerate code for SH4-300.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oM4300Nofpu({"-m4-300-nofpu"},
    "  -m4-300-nofpu               \tGenerate code for SH4-300 without in such a way that the floating-point "
    "unit is not used.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oM4300Single({"-m4-300-single"},
    "  -m4-300-single              \tGenerate code for SH4-300 in such a way that no double-precision "
    "floating-point operations are used.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oM4300SingleOnly({"-m4-300-single-only"},
    "  -m4-300-single-only         \tGenerate code for SH4-300 in such a way that no double-precision "
    "floating-point operations are used.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oM4340({"-m4-340"},
    "  -m4-340                     \tGenerate code for SH4-340 (no MMU, no FPU).\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oM4500({"-m4-500"},
    "  -m4-500                     \tGenerate code for SH4-500 (no FPU). Passes -isa=sh4-nofpu to the assembler.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oM4Nofpu({"-m4-nofpu"},
    "  -m4-nofpu                   \tGenerate code for the SH4al-dsp, or for a SH4a in such a way that the "
    "floating-point unit is not used.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oM4Single({"-m4-single"},
    "  -m4-single                  \tGenerate code for the SH4a assuming the floating-point unit is in "
    "single-precision mode by default.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oM4SingleOnly({"-m4-single-only"},
    "  -m4-single-only             \tGenerate code for the SH4a, in such a way that no double-precision "
    "floating-point operations are used.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oM40({"-m40"},
    "  -m40                        \tGenerate code for a PDP-11/40.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oM45({"-m45"},
    "  -m45                        \tGenerate code for a PDP-11/45. This is the default.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oM4a({"-m4a"},
    "  -m4a                        \tGenerate code for the SH4a.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oM4aNofpu({"-m4a-nofpu"},
    "  -m4a-nofpu                  \tGenerate code for the SH4al-dsp, or for a SH4a in such a way that the "
    "floating-point unit is not used.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oM4aSingle({"-m4a-single"},
    "  -m4a-single                 \tGenerate code for the SH4a assuming the floating-point unit is in "
    "single-precision mode by default.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oM4aSingleOnly({"-m4a-single-only"},
    "  -m4a-single-only            \tGenerate code for the SH4a, in such a way that no double-precision "
    "floating-point operations are used.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oM4al({"-m4al"},
    "  -m4al                       \tSame as -m4a-nofpu, except that it implicitly passes -dsp to the assembler. "
    "GCC doesn't generate any DSP instructions at the moment.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oM4byteFunctions({"-m4byte-functions"},
    "  -m4byte-functions           \tForce all functions to be aligned to a 4-byte boundary.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-mno-4byte-functions"), maplecl::kHide);

maplecl::Option<bool> oM5200({"-m5200"},
    "  -m5200                      \tGenerate output for a 520X ColdFire CPU.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oM5206e({"-m5206e"},
    "  -m5206e                     \tGenerate output for a 5206e ColdFire CPU. \n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oM528x({"-m528x"},
    "  -m528x                      \tGenerate output for a member of the ColdFire 528X family. \n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oM5307({"-m5307"},
    "  -m5307                      \tGenerate output for a ColdFire 5307 CPU.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oM5407({"-m5407"},
    "  -m5407                      \tGenerate output for a ColdFire 5407 CPU.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oM64({"-m64"},
    "  -m64                        \tGenerate code for 64-bit ABI.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oM64bitDoubles({"-m64bit-doubles"},
    "  -m64bit-doubles             \tMake the double data type be 64 bits (-m64bit-doubles) in size.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oM68000({"-m68000"},
    "  -m68000                     \tGenerate output for a 68000.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oM68010({"-m68010"},
    "  -m68010                     \tGenerate output for a 68010.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oM68020({"-m68020"},
    "  -m68020                     \tGenerate output for a 68020. \n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oM6802040({"-m68020-40"},
    "  -m68020-40                  \tGenerate output for a 68040, without using any of the new instructions. \n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oM6802060({"-m68020-60"},
    "  -m68020-60                  \tGenerate output for a 68060, without using any of the new instructions. \n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMTLS({"-mTLS"},
    "  -mTLS                       \tAssume a large TLS segment when generating thread-local code.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-mtls"), maplecl::kHide);

maplecl::Option<bool> oMtlsDirectSegRefs({"-mtls-direct-seg-refs"},
    "  -mtls-direct-seg-refs       \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMtlsMarkers({"-mtls-markers"},
    "  -mtls-markers               \tMark calls to __tls_get_addr with a relocation specifying the function "
    "argument. \n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-tls-markers"), maplecl::kHide);

maplecl::Option<bool> oMtoc({"-mtoc"},
    "  -mtoc                       \tOn System V.4 and embedded PowerPC systems do not assume that register 2 "
    "contains a pointer to a global area pointing to the addresses used in the program.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-toc"), maplecl::kHide);

maplecl::Option<bool> oMtomcatStats({"-mtomcat-stats"},
    "  -mtomcat-stats              \tCause gas to print out tomcat statistics.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMtoplevelSymbols({"-mtoplevel-symbols"},
    "  -mtoplevel-symbols          \tPrepend (do not prepend) a ':' to all global symbols, so the assembly code "
    "can be used with the PREFIX assembly directive.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-toplevel-symbols"), maplecl::kHide);

maplecl::Option<bool> oMtpcsFrame({"-mtpcs-frame"},
    "  -mtpcs-frame                \tGenerate a stack frame that is compliant with the Thumb Procedure Call "
    "Standard for all non-leaf functions.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMtpcsLeafFrame({"-mtpcs-leaf-frame"},
    "  -mtpcs-leaf-frame           \tGenerate a stack frame that is compliant with the Thumb Procedure Call "
    "Standard for all leaf functions.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMtpfTrace({"-mtpf-trace"},
    "  -mtpf-trace                 \tGenerate code that adds in TPF OS specific branches to trace routines "
    "in the operating system. \n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-tpf-trace"), maplecl::kHide);

maplecl::Option<bool> oMtuneCtrlFeatureList({"-mtune-ctrl=feature-list"},
    "  -mtune-ctrl=feature-list    \tThis option is used to do fine grain control of x86 code generation features.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMuclibc({"-muclibc"},
    "  -muclibc                    \tUse uClibc C library.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMuls({"-muls"},
    "  -muls                       \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMultcostNumber({"-multcost=number"},
    "  -multcost=number            \tSet the cost to assume for a multiply insn.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMultilibLibraryPic({"-multilib-library-pic"},
    "  -multilib-library-pic       \tLink with the (library, not FD) pic libraries.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMmultiplyEnabled({"-mmultiply-enabled"},
    "  -multiply-enabled           \tEnable multiply instructions.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMultiply_defined({"-multiply_defined"},
    "  -multiply_defined           \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMultiply_defined_unused({"-multiply_defined_unused"},
    "  -multiply_defined_unused    \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMulti_module({"-multi_module"},
    "  -multi_module               \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMunalignProbThreshold({"-munalign-prob-threshold"},
    "  -munalign-prob-threshold    \tSet probability threshold for unaligning branches. \n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMunalignedAccess({"-munaligned-access"},
    "  -munaligned-access          \tEnables (or disables) reading and writing of 16- and 32- bit values from "
    "addresses that are not 16- or 32- bit aligned.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-unaligned-access"), maplecl::kHide);

maplecl::Option<bool> oMunalignedDoubles({"-munaligned-doubles"},
    "  -munaligned-doubles         \tAssume that doubles have 8-byte alignment. \n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-unaligned-doubles"), maplecl::kHide);

maplecl::Option<bool> oMunicode({"-municode"},
    "  -municode                   \tThis option is available for MinGW-w64 targets.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMuniformSimt({"-muniform-simt"},
    "  -muniform-simt              \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMuninitConstInRodata({"-muninit-const-in-rodata"},
    "  -muninit-const-in-rodata    \tPut uninitialized const variables in the read-only data section. \n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-uninit-const-in-rodata"), maplecl::kHide);

maplecl::Option<bool> oMunixAsm({"-munix-asm"},
    "  -munix-asm                  \tUse Unix assembler syntax. \n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMupdate({"-mupdate"},
    "  -mupdate                  \tGenerate code that uses the load string instructions and the store string word "
    "instructions to save multiple registers and do small block moves. \n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-update"), maplecl::kHide);

maplecl::Option<bool> oMupperRegs({"-mupper-regs"},
    "  -mupper-regs                \tGenerate code that uses (does not use) the scalar instructions that target all "
    "64 registers in the vector/scalar floating point register set, depending on the model of the machine.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-upper-regs"), maplecl::kHide);

maplecl::Option<bool> oMupperRegsDf({"-mupper-regs-df"},
    "  -mupper-regs-df             \tGenerate code that uses (does not use) the scalar double precision instructions "
    "that target all 64 registers in the vector/scalar floating point register set that were added in version 2.06 of"
    " the PowerPC ISA.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-upper-regs-df"), maplecl::kHide);

maplecl::Option<bool> oMupperRegsDi({"-mupper-regs-di"},
    "  -mupper-regs-di             \tGenerate code that uses (does not use) the scalar instructions that target all "
    "64 registers in the vector/scalar floating point register set that were added in version 2.06 of the PowerPC ISA"
    " when processing integers. \n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-upper-regs-di"), maplecl::kHide);

maplecl::Option<bool> oMupperRegsSf({"-mupper-regs-sf"},
    "  -mupper-regs-sf             \tGenerate code that uses (does not use) the scalar single precision instructions "
    "that target all 64 registers in the vector/scalar floating point register set that were added in version 2.07 of "
    "the PowerPC ISA.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-upper-regs-sf"), maplecl::kHide);

maplecl::Option<bool> oMuserEnabled({"-muser-enabled"},
    "  -muser-enabled              \tEnable user-defined instructions.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMuserMode({"-muser-mode"},
    "  -muser-mode                 \tDo not generate code that can only run in supervisor mode.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-user-mode"), maplecl::kHide);

maplecl::Option<bool> oMusermode({"-musermode"},
    "  -musermode                  \tDon't allow (allow) the compiler generating privileged mode code.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-usermode"), maplecl::kHide);

maplecl::Option<bool> oMv3push({"-mv3push"},
    "  -mv3push                    \tGenerate v3 push25/pop25 instructions.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-v3push"), maplecl::kHide);

maplecl::Option<bool> oMv850({"-mv850"},
    "  -mv850                      \tSpecify that the target processor is the V850.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMv850e({"-mv850e"},
    "  -mv850e                     \tSpecify that the target processor is the V850E.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMv850e1({"-mv850e1"},
    "  -mv850e1                    \tSpecify that the target processor is the V850E1. \n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMv850e2({"-mv850e2"},
    "  -mv850e2                    \tSpecify that the target processor is the V850E2.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMv850e2v3({"-mv850e2v3"},
    "  -mv850e2v3                  \tSpecify that the target processor is the V850E2V3.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMv850e2v4({"-mv850e2v4"},
    "  -mv850e2v4                  \tSpecify that the target processor is the V850E3V5.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMv850e3v5({"-mv850e3v5"},
    "  -mv850e3v5                  \tpecify that the target processor is the V850E3V5.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMv850es({"-mv850es"},
    "  -mv850es                    \tSpecify that the target processor is the V850ES.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMv8plus({"-mv8plus"},
    "  -mv8plus                    \tWith -mv8plus, Maple generates code for the SPARC-V8+ ABI.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-v8plus"), maplecl::kHide);

maplecl::Option<bool> oMvect8RetInMem({"-mvect8-ret-in-mem"},
    "  -mvect8-ret-in-mem          \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMvirt({"-mvirt"},
    "  -mvirt                      \tUse the MIPS Virtualization (VZ) instructions.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-virt"), maplecl::kHide);

maplecl::Option<bool> oMvis({"-mvis"},
    "  -mvis                       \tWith -mvis, Maple generates code that takes advantage of the UltraSPARC Visual "
    "Instruction Set extensions. \n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-vis"), maplecl::kHide);

maplecl::Option<bool> oMvis2({"-mvis2"},
    "  -mvis2                      \tWith -mvis2, Maple generates code that takes advantage of version 2.0 of the "
    "UltraSPARC Visual Instruction Set extensions. \n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-vis2"), maplecl::kHide);

maplecl::Option<bool> oMvis3({"-mvis3"},
    "  -mvis3                      \tWith -mvis3, Maple generates code that takes advantage of version 3.0 of the "
    "UltraSPARC Visual Instruction Set extensions.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-vis3"), maplecl::kHide);

maplecl::Option<bool> oMvis4({"-mvis4"},
    "  -mvis4                      \tWith -mvis4, Maple generates code that takes advantage of version 4.0 of the "
    "UltraSPARC Visual Instruction Set extensions.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-vis4"), maplecl::kHide);

maplecl::Option<bool> oMvis4b({"-mvis4b"},
    "  -mvis4b                     \tWith -mvis4b, Maple generates code that takes advantage of version 4.0 of the "
    "UltraSPARC Visual Instruction Set extensions, plus the additional VIS instructions introduced in the Oracle "
    "SPARC Architecture 2017.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-vis4b"), maplecl::kHide);

maplecl::Option<bool> oMvliwBranch({"-mvliw-branch"},
    "  -mvliw-branch               \tRun a pass to pack branches into VLIW instructions (default).\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-vliw-branch"), maplecl::kHide);

maplecl::Option<bool> oMvmsReturnCodes({"-mvms-return-codes"},
    "  -mvms-return-codes          \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMvolatileAsmStop({"-mvolatile-asm-stop"},
    "  -mvolatile-asm-stop         \t\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-volatile-asm-stop"), maplecl::kHide);

maplecl::Option<bool> oMvolatileCache({"-mvolatile-cache"},
    "  -mvolatile-cache            \tUse ordinarily cached memory accesses for volatile references.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-volatile-cache"), maplecl::kHide);

maplecl::Option<bool> oMvr4130Align({"-mvr4130-align"},
    "  -mvr4130-align              \tThe VR4130 pipeline is two-way superscalar, but can only issue two instructions "
    "together if the first one is 8-byte aligned. \n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-mno-vr4130-align"), maplecl::kHide);

maplecl::Option<bool> oMvrsave({"-mvrsave"},
    "  -mvrsave                    \tGenerate VRSAVE instructions when generating AltiVec code.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-vrsave"), maplecl::kHide);

maplecl::Option<bool> oMvsx({"-mvsx"},
    "  -mvsx                       \tGenerate code that uses (does not use) vector/scalar (VSX) instructions, and "
    "also enable the use of built-in functions that allow more direct access to the VSX instruction set.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-vsx"), maplecl::kHide);

maplecl::Option<bool> oMvx({"-mvx"},
    "  -mvx                        \tGenerate code using the instructions available with the vector extension facility"
    " introduced with the IBM z13 machine generation. \n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-vx"), maplecl::kHide);

maplecl::Option<bool> oMvxworks({"-mvxworks"},
    "  -mvxworks                   \tOn System V.4 and embedded PowerPC systems, specify that you are compiling "
    "for a VxWorks system.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMvzeroupper({"-mvzeroupper"},
    "  -mvzeroupper                \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMwarnCellMicrocode({"-mwarn-cell-microcode"},
    "  -mwarn-cell-microcode       \tWarn when a Cell microcode instruction is emitted. \n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oO({"-O"},
    "  -O                          \tReduce code size and execution time, without performing any optimizations "
    "that take a great deal of compilation time.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

    /* #################################################################################################### */

} /* namespace opts */
