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

#include <bits/stdint-uintn.h>
#include <stdint.h>
#include <string>

namespace opts::me {

maplecl::Option<bool> help({"--help", "-h"},
                      "  -h --help                   \tPrint usage and exit.Available command names:\n"
                      "                              \tme\n", {meCategory});

maplecl::Option<bool> o1({"--O1", "-O1"},
                    "  -O1                         \tDo some optimization.\n",
                    {meCategory});

maplecl::Option<bool> o2({"--O2", "-O2"},
                    "  -O2                         \tDo some optimization.\n",
                    {meCategory});

maplecl::Option<bool> os({"--Os", "-Os"},
                    "  -Os                         \tOptimize for size, based on O2.\n",
                    {meCategory});

maplecl::Option<bool> o3({"--O3", "-O3"},
                    "  -O3                         \tDo aggressive optimizations.\n",
                    {meCategory});

maplecl::Option<std::string> refusedcheck({"--refusedcheck"},
                                     "  --refusedcheck=FUNCNAME,...    \tEnable ref check used in func in the comma"
                                     " separated list, * means all func.\n",
                                     {meCategory});

maplecl::Option<std::string> range({"--range"},
                              "  --range                     \tOptimize only functions in the range [NUM0, NUM1]\n"
                              "                              \t--range=NUM0,NUM1\n",
                              {meCategory});

maplecl::Option<std::string> pgoRange({"--pgorange"},
                              "  --pglrange                  \tUse profile-guided optimizations only for funcid in the range [NUM0, NUM1]\n"
                              "                              \t--pgorange=NUM0,NUM1\n",
                              {meCategory});

maplecl::Option<std::string> dumpPhases({"--dump-phases"},
                                   "  --dump-phases               \tEnable debug trace for specified phases"
                                   " in the comma separated list\n"
                                   "                              \t--dump-phases=PHASENAME,...\n",
                                   {meCategory});

maplecl::Option<std::string> skipPhases({"--skip-phases"},
                                   "  --skip-phases               \tSkip the phases specified"
                                   " in the comma separated list\n"
                                   "                              \t--skip-phases=PHASENAME,...\n",
                                   {meCategory});

maplecl::Option<std::string> dumpFunc({"--dump-func"},
                                 "  --dump-func                 \tDump/trace only for functions whose names"
                                 " contain FUNCNAME as substring\n"
                                 "                              \t(can only specify once)\n"
                                 "                              \t--dump-func=FUNCNAME\n",
                                 {meCategory});

maplecl::Option<bool> quiet({"--quiet"},
                       "  --quiet                     \tDisable brief trace messages with phase/function names\n"
                       "  --no-quiet                  \tEnable brief trace messages with phase/function names\n",
                       {meCategory},
                       maplecl::DisableWith("--no-quiet"));

maplecl::Option<bool> nodot({"--nodot"},
                       "  --nodot                     \tDisable dot file generation from cfg\n"
                       "  --no-nodot                  \tEnable dot file generation from cfg\n",
                       {meCategory},
                       maplecl::DisableWith("--no-nodot"));

maplecl::Option<bool> userc({"--userc"},
                       "  --userc                     \tEnable reference counting [default]\n"
                       "  --no-userc                  \tDisable reference counting [default]\n",
                       {meCategory},
                       maplecl::DisableWith("--no-userc"));

maplecl::Option<bool> strictNaiverc({"--strict-naiverc"},
                               "  --strict-naiverc            \tStrict Naive RC mode, assume no unsafe multi-thread"
                               " read/write racing\n"
                               "  --no-strict-naiverc         \tDisable Strict Naive RC mode, assume no unsafe"
                               " multi-thread read/write racing\n",
                               {meCategory},
                               maplecl::DisableWith("--no-strict-naiverc"));

maplecl::Option<std::string> skipFrom({"--skip-from"},
                                 "  --skip-from                 \tSkip the rest phases from PHASENAME(included)\n"
                                 "                              \t--skip-from=PHASENAME\n",
                                 {meCategory});

maplecl::Option<std::string> skipAfter({"--skip-after"},
                                  "  --skip-after                \tSkip the rest phases after PHASENAME(excluded)\n"
                                  "                              \t--skip-after=PHASENAME\n",
                                  {meCategory});

maplecl::Option<bool> calleeHasSideEffect({"--setCalleeHasSideEffect"},
                                     "  --setCalleeHasSideEffect    \tSet all the callees have side effect\n"
                                     "  --no-setCalleeHasSideEffect \tNot set all the callees have side effect\n",
                                     {meCategory},
                                     maplecl::DisableWith("--no-setCalleeHasSideEffect"));

maplecl::Option<bool> ubaa({"--ubaa"},
                      "  --ubaa                      \tEnable UnionBased alias analysis\n"
                      "  --no-ubaa                   \tDisable UnionBased alias analysis\n",
                      {meCategory},
                      maplecl::DisableWith("--no-ubaa"));

maplecl::Option<bool> tbaa({"--tbaa"},
                      "  --tbaa                      \tEnable type-based alias analysis\n"
                      "  --no-tbaa                   \tDisable type-based alias analysis\n",
                      {meCategory},
                      maplecl::DisableWith("--no-tbaa"));

maplecl::Option<bool> ddaa({"--ddaa"},
                      "  --ddaa                      \tEnable demand driven alias analysis\n"
                      "  --no-ddaa                   \tDisable demand driven alias analysis\n",
                      {meCategory},
                      maplecl::DisableWith("--no-ddaa"));

maplecl::Option<uint8_t> aliasAnalysisLevel({"--aliasAnalysisLevel"},
                                       "  --aliasAnalysisLevel        \tSet level of alias analysis. \n"
                                       "                              \t0: most conservative;\n"
                                       "                              \t1: Union-based alias analysis;"
                                       " 2: type-based alias analysis;\n"
                                       "                              \t3: Union-based and type-based alias analysis\n"
                                       "                              \t--aliasAnalysisLevel=NUM\n",
                                       {meCategory});

maplecl::Option<bool> stmtnum({"--stmtnum"},
                         "  --stmtnum                   \tPrint MeStmt index number in IR dump\n"
                         "  --no-stmtnum                \tDon't print MeStmt index number in IR dump\n",
                         {meCategory},
                         maplecl::DisableWith("--no-stmtnum"));

maplecl::Option<bool> rclower({"--rclower"},
                         "  --rclower                   \tEnable rc lowering\n"
                         "  --no-rclower                \tDisable rc lowering\n",
                         {meCategory},
                         maplecl::DisableWith("--no-rclower"));

maplecl::Option<bool> gconlyopt({"--gconlyopt"},
                           "  --gconlyopt                     \tEnable write barrier optimization in gconly\n"
                           "  --no-gconlyopt                  \tDisable write barrier optimization in gconly\n",
                           {meCategory},
                           maplecl::DisableWith("--no-gconlyopt"));

maplecl::Option<bool> usegcbar({"--usegcbar"},
                          "  --usegcbar                  \tEnable GC barriers\n"
                          "  --no-usegcbar               \tDisable GC barriers\n",
                          {meCategory},
                          maplecl::DisableWith("--no-usegcbar"));

maplecl::Option<bool> regnativefunc({"--regnativefunc"},
                               "  --regnativefunc             \tGenerate native stub function to support JNI"
                               " registration and calling\n"
                               "  --no-regnativefunc          \tDon't generate native stub function to support JNI"
                               " registration and calling\n",
                               {meCategory},
                               maplecl::DisableWith("--no-regnativefunc"));

maplecl::Option<bool> warnemptynative({"--warnemptynative"},
                                 "  --warnemptynative           \tGenerate warning and abort unimplemented native"
                                 " function\n"
                                 "  --no-warnemptynative        \tDon't generate warning and abort unimplemented"
                                 " native function\n",
                                 {meCategory},
                                 maplecl::DisableWith("--no-warnemptynative"));

maplecl::Option<bool> dumpBefore({"--dump-before"},
                            "  --dump-before               \tDo extra IR dump before the specified phase in me\n"
                            "  --no-dump-before            \tDon't extra IR dump before the specified phase in me\n",
                            {meCategory},
                            maplecl::DisableWith("--no-dump-before"));

maplecl::Option<bool> dumpAfter({"--dump-after"},
                           "  --dump-after                \tDo extra IR dump after the specified phase in me\n"
                           "  --no-dump-after             \tDo not extra IR dump after the specified phase in me\n",
                           {meCategory},
                           maplecl::DisableWith("--no-dump-after"));

maplecl::Option<bool> realcheckcast({"--realcheckcast"},
                               "  --realcheckcast\n"
                               "  --no-realcheckcast\n",
                               {meCategory},
                               maplecl::DisableWith("--no-realcheckcast"));

maplecl::Option<uint32_t> eprelimit({"--eprelimit"},
                               "  --eprelimit                 \tApply EPRE optimization only for"
                               " the first NUM expressions\n"
                               "                              \t--eprelimit=NUM\n",
                               {meCategory});

maplecl::Option<uint32_t> eprepulimit({"--eprepulimit"},
                                 "  --eprepulimit               \tApply EPRE optimization only for the first NUM PUs\n"
                                 "                              \t--eprepulimit=NUM\n",
                                 {meCategory});

maplecl::Option<uint32_t> epreuseprofilelimit({"--epreuseprofilelimit"},
                                 "  --epreuseprofilelimit       \tMake EPRE take advantage of profile data only for the first NUM expressions\n"
                                 "                              \t--epreuseprofilelimit=NUM\n",
                                 {meCategory});

maplecl::Option<uint32_t> stmtprepulimit({"--stmtprepulimit"},
                                    "  --stmtprepulimit            \tApply STMTPRE optimization only for"
                                    " the first NUM PUs\n"
                                    "                              \t--stmtprepulimit=NUM\n",
                                    {meCategory});

maplecl::Option<uint32_t> lprelimit({"--lprelimit"},
                               "  --lprelimit                 \tApply LPRE optimization only for"
                               " the first NUM variables\n"
                               "                              \t--lprelimit=NUM\n",
                               {meCategory});

maplecl::Option<uint32_t> lprepulimit({"--lprepulimit"},
                                 "  --lprepulimit               \tApply LPRE optimization only for the first NUM PUs\n"
                                 "                              \t--lprepulimit=NUM\n",
                                 {meCategory});

maplecl::Option<uint32_t> pregrenamelimit({"--pregrenamelimit"},
                                     "  --pregrenamelimit"
                                     "           \tApply Preg Renaming optimization only up to NUM times\n"
                                     "                              \t--pregrenamelimit=NUM\n",
                                     {meCategory});

maplecl::Option<uint32_t> rename2preglimit({"--rename2preglimit"},
                                      "  --rename2preglimit"
                                      "          \tApply Rename-to-Preg optimization only up to NUM times\n"
                                      "                              \t--rename2preglimit=NUM\n",
                                      {meCategory});

maplecl::Option<uint32_t> proplimit({"--proplimit"},
                               "  --proplimit"
                               "             \tApply propagation only up to NUM times in each hprop invocation\n"
                               "                              \t--proplimit=NUM\n",
                               {meCategory});

maplecl::Option<uint32_t> copyproplimit({"--copyproplimit"},
                                   "  --copyproplimit             \tApply copy propagation only up to NUM times\n"
                                   "                              \t--copyproplimit=NUM\n",
                                   {meCategory});

maplecl::Option<uint32_t> delrcpulimit({"--delrcpulimit"},
                                  "  --delrcpulimit"
                                  "              \tApply DELEGATERC optimization only for the first NUM PUs\n"
                                  "                              \t--delrcpulimit=NUM\n",
                                  {meCategory});

maplecl::Option<uint32_t> profileBbHotRate({"--profile-bb-hot-rate"},
                                      "  --profile-bb-hot-rate=10"
                                      "   \tA count is regarded as hot if it is in the largest 10%\n",
                                      {meCategory});

maplecl::Option<uint32_t> profileBbColdRate({"--profile-bb-cold-rate"},
                                       "  --profile-bb-cold-rate=99"
                                       "  \tA count is regarded as cold if it is in the smallest 1%\n",
                                       {meCategory});

maplecl::Option<bool> ignoreipa({"--ignoreipa"},
                           "  --ignoreipa                 \tIgnore information provided by interprocedural analysis\n"
                           "  --no-ignoreipa"
                           "              \tDon't ignore information provided by interprocedural analysis\n",
                           {meCategory},
                           maplecl::DisableWith("--no-ignoreipa"));

maplecl::Option<bool> enableHotColdSplit({"--enableHotColdSplit"},
                                    "  --enableHotColdSplit        \tEnable the HotCold function split\n"
                                    "  --no-enableHotColdSplit   \tDisable the HotCold function split\n",
                                    {meCategory},
                                    maplecl::DisableWith("--no-enableHotColdSplit"));

maplecl::Option<bool> aggressiveABCO({"--aggressiveABCO"},
                                "  --aggressiveABCO"
                                "                 \tEnable aggressive array boundary check optimization\n"
                                "  --no-aggressiveABCO"
                                "              \tDon't enable aggressive array boundary check optimization\n",
                                {meCategory},
                                maplecl::DisableWith("--no-aggressiveABCO"));

maplecl::Option<bool> commonABCO({"--commonABCO"},
                            "  --commonABCO                 \tEnable aggressive array boundary check optimization\n"
                            "  --no-commonABCO"
                            "              \tDon't enable aggressive array boundary check optimization\n",
                            {meCategory},
                            maplecl::DisableWith("--no-commonABCO"));

maplecl::Option<bool> conservativeABCO({"--conservativeABCO"},
                                  "  --conservativeABCO"
                                  "                 \tEnable aggressive array boundary check optimization\n"
                                  "  --no-conservativeABCO"
                                  "              \tDon't enable aggressive array boundary check optimization\n",
                                  {meCategory},
                                  maplecl::DisableWith("--no-conservativeABCO"));

maplecl::Option<bool> epreincluderef({"--epreincluderef"},
                                "  --epreincluderef"
                                "            \tInclude ref-type expressions when performing epre optimization\n"
                                "  --no-epreincluderef"
                                "         \tDon't include ref-type expressions when performing epre optimization\n",
                                {meCategory},
                                maplecl::DisableWith("--no-epreincluderef"));

maplecl::Option<bool> eprelocalrefvar({"--eprelocalrefvar"},
                                 "  --eprelocalrefvar"
                                 "           \tThe EPRE phase will create new localrefvars when appropriate\n"
                                 "  --no-eprelocalrefvar"
                                 "        \tDisable the EPRE phase create new localrefvars when appropriate\n",
                                 {meCategory},
                                 maplecl::DisableWith("--no-eprelocalrefvar"));

maplecl::Option<bool> eprelhsivar({"--eprelhsivar"},
                             "  --eprelhsivar"
                             "               \tThe EPRE phase will consider iassigns when optimizing ireads\n"
                             "  --no-eprelhsivar"
                             "            \tDisable the EPRE phase consider iassigns when optimizing ireads\n",
                             {meCategory},
                             maplecl::DisableWith("--no-eprelhsivar"));

maplecl::Option<bool> dsekeepref({"--dsekeepref"},
                            "  --dsekeepref"
                            "                \tPreverse dassign of local var that are of ref type to anywhere\n"
                            "  --no-dsekeepref"
                            "             \tDon't preverse dassign of local var that are of ref type to anywhere\n",
                            {meCategory},
                            maplecl::DisableWith("--no-dsekeepref"));

maplecl::Option<bool> propbase({"--propbase"},
                          "  --propbase                  \tApply copy propagation"
                          " that can change the base of indirect memory accesses\n"
                          "  --no-propbase               \tDon't apply copy propagation\n",
                          {meCategory},
                          maplecl::DisableWith("--no-propbase"));

maplecl::Option<bool> propiloadref({"--propiloadref"},
                              "  --propiloadref"
                              "              \tAllow copy propagating iloads that are of ref type to anywhere\n"
                              "  --no-propiloadref"
                              "           \tDon't aAllow copy propagating iloads that are of ref type to anywhere\n",
                              {meCategory},
                              maplecl::DisableWith("--no-propiloadref"));

maplecl::Option<bool> propglobalref({"--propglobalref"},
                               "  --propglobalref"
                               "             \tAllow copy propagating global that are of ref type to anywhere\n"
                               "  --no-propglobalref"
                               "          \tDon't allow copy propagating global that are of ref type to anywhere\n",
                               {meCategory},
                               maplecl::DisableWith("--no-propglobalref"));

maplecl::Option<bool> propfinaliloadref({"--propfinaliloadref"},
                                   "  --propfinaliloadref         \tAllow copy propagating iloads of\n"
                                   "                              \tfinal fields that are of ref type to anywhere\n"
                                   "  --no-propfinaliloadref      \tDisable propfinaliloadref\n",
                                   {meCategory},
                                   maplecl::DisableWith("--no-propfinaliloadref"));

maplecl::Option<bool> propiloadrefnonparm({"--propiloadrefnonparm"},
                                     "  --propiloadrefnonparm"
                                     "       \tAllow copy propagating iloads that are of ref type to\n"
                                     "                              \tanywhere except actual parameters\n"
                                     "  --no-propiloadrefnonparm    \tDisbale propiloadref\n",
                                     {meCategory},
                                     maplecl::DisableWith("--no-propiloadrefnonparm"));

maplecl::Option<bool> lessthrowalias({"--lessthrowalias"},
                                "  --lessthrowalias"
                                "            \tHandle aliases at java throw statements more accurately\n"
                                "  --no-lessthrowalias         \tDisable lessthrowalias\n",
                                {meCategory},
                                maplecl::DisableWith("--no-lessthrowalias"));

maplecl::Option<bool> nodelegaterc({"--nodelegaterc"},
                              "  --nodelegateerc"
                              "             \tDo not apply RC delegation to local object reference pointers\n"
                              "  --no-nodelegateerc          \tDisable nodelegateerc\n",
                              {meCategory},
                              maplecl::DisableWith("--no-nodelegateerc"));

maplecl::Option<bool> nocondbasedrc({"--nocondbasedrc"},
                               "  --nocondbasedrc             \tDo not apply condition-based RC optimization to\n"
                               "                              \tlocal object reference pointers\n"
                               "  --no-nocondbasedrc          \tDisable nocondbasedrc\n",
                               {meCategory},
                               maplecl::DisableWith("--no-nocondbasedrc"));

maplecl::Option<bool> subsumrc({"--subsumrc"},
                          "  --subsumrc"
                          "               \tDelete decrements for localrefvars whose live range is just in\n"
                          "                           \tanother which point to the same obj\n"
                          "  --no-subsumrc            \tDisable subsumrc\n",
                          {meCategory},
                          maplecl::DisableWith("--no-subsumrc"));

maplecl::Option<bool> performFSAA({"--performFSAA"},
                             "  --performFSAA            \tPerform flow sensitive alias analysis\n"
                             "  --no-performFSAA         \tDisable flow sensitive alias analysis\n",
                             {meCategory},
                             maplecl::DisableWith("--no-performFSAA"));

maplecl::Option<bool> strengthreduction({"--strengthreduction"},
                                   "  --strengthreduction      \tPerform strength reduction\n"
                                   "  --no-strengthreduction   \tDisable strength reduction\n",
                                   {meCategory},
                                   maplecl::DisableWith("--no-strengthreduction"));

maplecl::Option<bool> sradd({"--sradd"},
                       "  --sradd                   \tPerform strength reduction for OP_add/sub\n"
                       "  --no-sradd                \tDisable strength reduction for OP_add/sub\n",
                       {meCategory},
                       maplecl::DisableWith("--no-sradd"));

maplecl::Option<bool> lftr({"--lftr"},
                      "  --lftr                   \tPerform linear function test replacement\n"
                      "  --no-lftr                \tDisable linear function test replacement\n",
                      {meCategory},
                      maplecl::DisableWith("--no-lftr"));

maplecl::Option<bool> ivopts({"--ivopts"},
                        "  --ivopts                   \tPerform induction variables optimization\n"
                        "  --no-ivopts                \tDisable induction variables optimization\n",
                        {meCategory},
                        maplecl::DisableWith("--no-ivopts"));

maplecl::Option<bool> checkcastopt({"--checkcastopt"},
                              "  --checkcastopt             \tApply template--checkcast optimization \n"
                              "  --no-checkcastopt          \tDisable checkcastopt \n",
                              {meCategory},
                              maplecl::DisableWith("--no-checkcastopt"));

maplecl::Option<bool> parmtoptr({"--parmtoptr"},
                           "  --parmtoptr"
                           "                 \tAllow rcoptlocals to change actual parameters from ref to ptr type\n"
                           "  --no-parmtoptr              \tDisable parmtoptr\n",
                           {meCategory},
                           maplecl::DisableWith("--no-parmtoptr"));

maplecl::Option<bool> nullcheckpre({"--nullcheckpre"},
                              "  --nullcheckpre"
                              "              \tTurn on partial redundancy elimination of null pointer checks\n"
                              "  --no-nullcheckpre           \tDisable nullcheckpre\n",
                              {meCategory},
                              maplecl::DisableWith("--no-nullcheckpre"));

maplecl::Option<bool> clinitpre({"--clinitpre"},
                           "  --clinitpre"
                           "                 \tTurn on partial redundancy elimination of class initialization checks\n"
                           "  --no-clinitpre              \tDisable clinitpre\n",
                           {meCategory},
                           maplecl::DisableWith("--no-clinitpre"));

maplecl::Option<bool> dassignpre({"--dassignpre"},
                            "  --dassignpre""                \tTurn on partial redundancy"
                            " elimination of assignments to scalar variables\n"
                            "  --no-dassignpre             \tDisable dassignpre\n",
                            {meCategory},
                            maplecl::DisableWith("--no-dassignpre"));

maplecl::Option<bool> assign2finalpre({"--assign2finalpre"},
                                 "  --assign2finalpre           \tTurn on partial redundancy"
                                 " elimination of assignments to final variables\n"
                                 "  --no-assign2finalpre        \tDisable assign2finalpre\n",
                                 {meCategory},
                                 maplecl::DisableWith("--no-assign2finalpre"));

maplecl::Option<bool> regreadatreturn({"--regreadatreturn"},
                                 "  --regreadatreturn           \tAllow register promotion"
                                 " to promote the operand of return statements\n"
                                 "  --no-regreadatreturn        \tDisable regreadatreturn\n",
                                 {meCategory},
                                 maplecl::DisableWith("--no-regreadatreturn"));

maplecl::Option<bool> propatphi({"--propatphi"},
                           "  --propatphi                 \tEnable copy propagation across phi nodes\n"
                           "  --no-propatphi              \tDisable propatphi\n",
                           {meCategory},
                           maplecl::DisableWith("--no-propatphi"));

maplecl::Option<bool> propduringbuild({"--propduringbuild"},
                                 "  --propduringbuild           \tEnable copy propagation when building HSSA\n"
                                 "  --no-propduringbuild        \tDisable propduringbuild\n",
                                 {meCategory},
                                 maplecl::DisableWith("--no-propduringbuild"));

maplecl::Option<bool> propwithinverse({"--propwithinverse"},
                                 "  --propwithinverse"
                                 "           \tEnable copy propagation across statements with inverse functions"
                                 "  --no-propwithinverse"
                                 "        \tDisable copy propagation across statements with inverse functions\n",
                                 {meCategory},
                                 maplecl::DisableWith(""));

maplecl::Option<bool> nativeopt({"--nativeopt"},
                           "  --nativeopt              \tEnable native opt\n"
                           "  --no-nativeopt           \tDisable native opt\n",
                           {meCategory},
                           maplecl::DisableWith("--no-nativeopt"));

maplecl::Option<bool> optdirectcall({"--optdirectcall"},
                               "  --optdirectcall             \tEnable redundancy elimination of directcalls\n"
                               "  --no-optdirectcall          \tDisable optdirectcall\n",
                               {meCategory},
                               maplecl::DisableWith("--no-optdirectcall"));

maplecl::Option<bool> enableEa({"--enable-ea"},
                           "  --enable-ea                 \tEnable escape analysis\n"
                           "  --no-enable-ea              \tDisable escape analysis\n",
                          {meCategory},
                           maplecl::DisableWith("--no-enable-ea"));

maplecl::Option<bool> lprespeculate({"--lprespeculate"},
                               "  --lprespeculate             \tEnable speculative code motion in LPRE phase\n"
                               "  --no-lprespeculate          \tDisable speculative code motion in LPRE phase\n",
                               {meCategory},
                               maplecl::DisableWith("--no-lprespeculate"));

maplecl::Option<bool> lpre4address({"--lpre4address"},
                              "  --lpre4address              \tEnable register promotion of addresses in LPRE phase\n"
                              "  --no-lpre4address"
                              "           \tDisable register promotion of addresses in LPRE phase\n",
                              maplecl::DisableWith("--no-lpre4address"));

maplecl::Option<bool> lpre4largeint({"--lpre4largeint"},
                               "  --lpre4largeint"
                               "             \tEnable register promotion of large integers in LPRE phase\n"
                               "  --no-lpre4largeint"
                               "          \tDisable register promotion of large integers in LPRE phase\n",
                               {meCategory},
                               maplecl::DisableWith("--no-lpre4largeint"));

maplecl::Option<bool> spillatcatch({"--spillatcatch"},
                              "  --spillatcatch              \tMinimize upward exposed preg usage in catch blocks\n"
                              "  --no-spillatcatch           \tDisable spillatcatch\n",
                              {meCategory},
                              maplecl::DisableWith("--no-spillatcatch"));

maplecl::Option<bool> placementrc({"--placementrc"},
                             "  --placementrc               \tInsert RC decrements for localrefvars using\n"
                             "                              \tthe placement optimization approach\n"
                             "  --no-placementrc            \tInsert RC decrements for localrefvars using\n",
                             {meCategory},
                             maplecl::DisableWith("--no-placementrc"));

maplecl::Option<bool> lazydecouple({"--lazydecouple"},
                              "  --lazydecouple              \tDo optimized for lazy Decouple\n"
                              "  --no-lazydecouple           \tDo not optimized for lazy Decouple\n",
                              {meCategory},
                              maplecl::DisableWith("--no-lazydecouple"));

maplecl::Option<bool> mergestmts({"--mergestmts"},
                            "  --mergestmts                \tTurn on statment merging optimization\n"
                            "  --no-mergestmts             \tDisable mergestmts\n",
                            {meCategory},
                            maplecl::DisableWith("--no-mergestmts"));

maplecl::Option<bool> generalRegOnly({"--general-reg-only"},
                                "  --general-reg-only"
                                "        \tME will avoid generate fp type when enable general-reg-only\n"
                                "  --no-general-reg-only     \tDisable general-reg-only\n",
                                {meCategory});

maplecl::Option<std::string> inlinefunclist({"--inlinefunclist"},
                                       "  --inlinefunclist            \tInlining related configuration\n"
                                       "                              \t--inlinefunclist=\n",
                                       {meCategory});

maplecl::Option<uint32_t> threads({"--threads"},
                             "  --threads=n                 \tOptimizing me functions using n threads\n"
                             "                              \tIf n >= 2,"
                             " ignore-inferred-return-type will be enabled default\n",
                             {meCategory});

maplecl::Option<bool> ignoreInferredRetType({"--ignore-inferred-ret-type"},
                                       "  --ignore-inferred-ret-type  \tIgnore func return type inferred by ssadevrt\n"
                                       "  --no-ignore-inferred-ret-type"
                                       "\tDo not ignore func return type inferred by ssadevirt\n",
                                       {meCategory},
                                       maplecl::DisableWith("--no-ignore-inferred-ret-type"));

maplecl::Option<bool> meverify({"--meverify"},
                          "  --meverify                       \tenable meverify features\n",
                          {meCategory},
                          maplecl::DisableWith("--no-meverify"));

maplecl::Option<uint32_t> dserunslimit({"--dserunslimit"},
                                  "  --dserunslimit=n            \tControl number of times dse phase can be run\n"
                                  "                              \t--dserunslimit=NUM\n",
                                  {meCategory});

maplecl::Option<uint32_t> hdserunslimit({"--hdserunslimit"},
                                   "  --hdserunslimit=n           \tControl number of times hdse phase can be run\n"
                                   "                              \t--hdserunslimit=NUM\n",
                                   {meCategory});

maplecl::Option<uint32_t> hproprunslimit({"--hproprunslimit"},
                                    "  --hproprunslimit=n          \tControl number of times hprop phase can be run\n"
                                    "                              \t--hproprunslimit=NUM\n",
                                    {meCategory});

maplecl::Option<uint32_t> sinklimit({"--sinklimit"},
                               "  --sinklimit=n          \tControl number of stmts sink-phase can sink\n"
                               "                              \t--sinklimit=NUM\n",
                               {meCategory});

maplecl::Option<uint32_t> sinkPUlimit({"--sinkPUlimit"},
                                 "  --sinkPUlimit=n          \tControl number of function sink-phase can be run\n"
                                 "                              \t--sinkPUlimit=NUM\n",
                                 {meCategory});

maplecl::Option<bool> loopvec({"--loopvec"},
                         "  --loopvec                   \tEnable auto loop vectorization\n"
                         "  --no-loopvec                \tDisable auto loop vectorization\n",
                         {meCategory},
                         maplecl::DisableWith("--no-loopvec"));

maplecl::Option<bool> seqvec({"--seqvec"},
                        "  --seqvec                   \tEnable auto sequencial vectorization\n"
                        "  --no-seqvec                \tDisable auto sequencial vectorization\n",
                        {meCategory},
                        maplecl::DisableWith("--no-seqvec"));

maplecl::Option<bool> layoutwithpredict({"--layoutwithpredict"},
                                   "  --layoutwithpredict"
                                   "        \tEnable optimizing output layout using branch prediction\n"
                                   "  --no-layoutwithpredict"
                                   "     \tDisable optimizing output layout using branch prediction\n",
                                   {meCategory},
                                   maplecl::DisableWith("--no-layoutwithpredict"));

maplecl::Option<uint32_t> veclooplimit({"--veclooplimit"},
                                  "  --veclooplimit             \tApply vectorize loops only up to NUM \n"
                                  "                              \t--veclooplimit=NUM\n",
                                  {meCategory});

maplecl::Option<uint32_t> ivoptslimit({"--ivoptslimit"},
                                 "  --ivoptslimit              \tApply ivopts only up to NUM loops \n"
                                 "                              \t--ivoptslimit=NUM\n",
                                 {meCategory});

maplecl::Option<std::string> acquireFunc({"--acquire-func"},
                                    "  --acquire-func              \t--acquire-func=FUNCNAME\n",
                                    {meCategory});

maplecl::Option<std::string> releaseFunc({"--release-func"},
                                     "  --release-func              \t--release-func=FUNCNAME\n",
                                    {meCategory});

maplecl::Option<bool> toolonly({"--toolonly"},
                          "  --toolonly\n"
                          "  --no-toolonly\n",
                          {meCategory},
                          maplecl::DisableWith("--no-toolonly"));

maplecl::Option<bool> toolstrict({"--toolstrict"},
                            "  --toolstrict\n"
                            "  --no-toolstrict\n",
                            {meCategory},
                            maplecl::DisableWith("--no-toolstrict"));

maplecl::Option<bool> skipvirtual({"--skipvirtual"},
                             "  --skipvirtual\n"
                             "  --no-skipvirtual\n",
                             {meCategory},
                             maplecl::DisableWith("--no-skipvirtual"));

maplecl::Option<uint32_t> warning({"--warning"},
                             "  --warning=level             \t--warning=level\n",
                             {meCategory});

maplecl::Option<uint8_t> remat({"--remat"},
                          "  --remat                     \tEnable rematerialization during register allocation\n"
                          "                              \t     0: no rematerialization (default)\n"
                          "                              \t  >= 1: rematerialize constants\n"
                          "                              \t  >= 2: rematerialize addresses\n"
                          "                              \t  >= 3: rematerialize local dreads\n"
                          "                              \t  >= 4: rematerialize global dreads\n",
                          {meCategory});

maplecl::Option<bool> unifyrets({"--unifyrets"},
                           "  --unifyrets                   \tEnable return blocks unification\n"
                           "  --no-unifyrets                \tDisable return blocks unification\n",
                           {meCategory},
                           maplecl::DisableWith("--no-unifyrets"));

maplecl::Option<bool> lfo({"--lfo"},
                     "  --lfo                   \tEnable LFO framework\n"
                     "  --no-lfo                \tDisable LFO framework\n",
                     {meCategory},
                     maplecl::DisableWith("--no-lfo"));

maplecl::Option<bool> dumpCfgOfPhases({"--dumpcfgofphases"},
                     "  --dumpcfgofphases       \tDump CFG from various phases to .dot files\n",
                     {meCategory});
maplecl::Option<bool> epreUseProfile({"--epreuseprofile"},
                     "  --epreuseprofile        \tEnable profile-guided epre phase\n"
                     "  --no-epreuseprofile     \tDisable profile-guided epre phase\n",
                     {meCategory},
                     maplecl::DisableWith("--no-epreuseprofile"));

}
