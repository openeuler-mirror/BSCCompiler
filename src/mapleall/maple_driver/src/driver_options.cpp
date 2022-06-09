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

#include <stdint.h>
#include <string>

namespace opts {

/* ##################### BOOL Options ############################################################### */

maplecl::Option<bool> version({"--version", "-v"},
                         "  --version [command]         \tPrint version and exit.\n",
                         {driverCategory});

maplecl::Option<bool> o0({"--O0", "-O0"},
                    "  -O0                         \tNo optimization.\n",
                    {driverCategory});

maplecl::Option<bool> o1({"--O1", "-O1"},
                    "  -O1                         \tDo some optimization.\n",
                    {driverCategory});

maplecl::Option<bool> o2({"--O2", "-O2"},
                    "  -O2                         \tDo more optimization. (Default)\n",
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

maplecl::Option<bool> bigendian({"-Be", "--BigEndian"},
                           "  --BigEndian/-Be            \tUsing BigEndian\n"
                           "  --no-BigEndian             \tUsing LittleEndian\n",
                           {driverCategory, meCategory, mpl2mplCategory, cgCategory},
                           maplecl::DisableWith("--no-BigEndian"));

maplecl::Option<bool> gconly({"--gconly", "-gconly"},
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

maplecl::Option<bool> printDriverPhases({"--print-driver-phases"},
                                   "  --print-driver-phases       \tPrint Driver Phases\n",
                                   {driverCategory});

maplecl::Option<bool> ldStatic({"-static", "--static"},
                          "  -static                     \tForce the linker to link a program statically\n",
                          {driverCategory, ldCategory});

maplecl::Option<bool> maplePhase({"--maple-phase"},
                            "  --maple-phase               \tRun maple phase only\n  --no-maple-phase\n",
                            {driverCategory},
                            maplecl::DisableWith("--no-maple-phase"),
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

maplecl::List<std::string> includeSystem({"--isystem"},
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

/* ##################### DIGITAL Options ############################################################### */

maplecl::Option<uint32_t> helpLevel({"--level"},
                               "  --level=NUM                 \tPrint the help info of specified level.\n"
                               "                              \tNUM=0: All options (Default)\n"
                               "                              \tNUM=1: Product options\n"
                               "                              \tNUM=2: Experimental options\n"
                               "                              \tNUM=3: Debug options\n",
                               {driverCategory});

/* #################################################################################################### */

} /* namespace opts */