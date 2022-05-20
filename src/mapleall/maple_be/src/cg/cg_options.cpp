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

namespace opts::cg {

cl::Option<bool> pie({"--pie"},
                     "  --pie                       \tGenerate position-independent executable\n"
                     "  --no-pie\n",
                     {cgCategory},
                     cl::DisableWith("--no-pie"));

cl::Option<bool> fpic({"--fpic"},
                      "  --fpic                      \tGenerate position-independent shared library\n"
                      "  --no-fpic\n",
                      {cgCategory},
                      cl::DisableWith("--no-fpic"));

cl::Option<bool> verboseAsm({"--verbose-asm"},
                            "  --verbose-asm               \tAdd comments to asm output\n"
                            "  --no-verbose-asm\n",
                            {cgCategory},
                            cl::DisableWith("--no-verbose-asm"));

cl::Option<bool> verboseCg({"--verbose-cg"},
                           "  --verbose-cg               \tAdd comments to cg output\n"
                           "  --no-verbose-cg\n",
                           {cgCategory},
                           cl::DisableWith("--no-verbose-cg"));

cl::Option<bool> maplelinker({"--maplelinker"},
                             "  --maplelinker               \tGenerate the MapleLinker .s format\n"
                             "  --no-maplelinker\n",
                             {cgCategory},
                             cl::DisableWith("--no-maplelinker"));

cl::Option<bool> quiet({"--quiet"},
                       "  --quiet                     \tBe quiet (don't output debug messages)\n"
                       "  --no-quiet\n",
                       {cgCategory},
                       cl::DisableWith("--no-quiet"));

cl::Option<bool> cg({"--cg"},
                    "  --cg                        \tGenerate the output .s file\n"
                    "  --no-cg\n",
                    {cgCategory},
                    cl::DisableWith("--no-cg"));

cl::Option<bool> replaceAsm({"--replaceasm"},
                            "  --replaceasm                \tReplace the the assembly code\n"
                            "  --no-replaceasm\n",
                            {cgCategory},
                            cl::DisableWith("--no-replaceasm"));

cl::Option<bool> generalRegOnly({"--general-reg-only"},
                                " --general-reg-only           \tdisable floating-point or Advanced SIMD registers\n"
                                " --no-general-reg-only\n",
                                {cgCategory},
                                cl::DisableWith("--no-general-reg-only"));

cl::Option<bool> lazyBinding({"--lazy-binding"},
                              "  --lazy-binding              \tBind class symbols lazily[default off]\n",
                             {cgCategory},
                              cl::DisableWith("--no-lazy-binding"));

cl::Option<bool> hotFix({"--hot-fix"},
                        "  --hot-fix                   \tOpen for App hot fix[default off]\n"
                        "  --no-hot-fix\n",
                        {cgCategory},
                        cl::DisableWith("--no-hot-fix"));

cl::Option<bool> ebo({"--ebo"},
                     "  --ebo                       \tPerform Extend block optimization\n"
                     "  --no-ebo\n",
                     {cgCategory},
                     cl::DisableWith("--no-ebo"));

cl::Option<bool> cfgo({"--cfgo"},
                      "  --cfgo                      \tPerform control flow optimization\n"
                      "  --no-cfgo\n",
                      {cgCategory},
                      cl::DisableWith("--no-cfgo"));

cl::Option<bool> ico({"--ico"},
                     "  --ico                       \tPerform if-conversion optimization\n"
                     "  --no-ico\n",
                     {cgCategory},
                     cl::DisableWith("--no-ico"));

cl::Option<bool> storeloadopt({"--storeloadopt"},
                              "  --storeloadopt              \tPerform global store-load optimization\n"
                              "  --no-storeloadopt\n",
                              {cgCategory},
                              cl::DisableWith("--no-storeloadopt"));

cl::Option<bool> globalopt({"--globalopt"},
                           "  --globalopt                 \tPerform global optimization\n"
                           "  --no-globalopt\n",
                           {cgCategory},
                           cl::DisableWith("--no-globalopt"));

cl::Option<bool> hotcoldsplit({"--hotcoldsplit"},
                              "  --hotcoldsplit        \tPerform HotColdSplit optimization\n"
                              "  --no-hotcoldsplit\n",
                              {cgCategory},
                              cl::DisableWith("--no-hotcoldsplit"));

cl::Option<bool> prelsra({"--prelsra"},
                         "  --prelsra                   \tPerform live interval simplification in LSRA\n"
                         "  --no-prelsra\n",
                         {cgCategory},
                         cl::DisableWith("--no-prelsra"));

cl::Option<bool> lsraLvarspill({"--lsra-lvarspill"},
                                "  --lsra-lvarspill"
                                "            \tPerform LSRA spill using local ref var stack locations\n"
                                "  --no-lsra-lvarspill\n",
                               {cgCategory},
                                cl::DisableWith("--no-lsra-lvarspill"));

cl::Option<bool> lsraOptcallee({"--lsra-optcallee"},
                               "  --lsra-optcallee            \tSpill callee if only one def to use\n"
                               "  --no-lsra-optcallee\n",
                               {cgCategory},
                               cl::DisableWith("--no-lsra-optcallee"));

cl::Option<bool> calleeregsPlacement({"--calleeregs-placement"},
                                     "  --calleeregs-placement      \tOptimize placement of callee-save registers\n"
                                     "  --no-calleeregs-placement\n",
                                     {cgCategory},
                                     cl::DisableWith("--no-calleeregs-placement"));

cl::Option<bool> ssapreSave({"--ssapre-save"},
                            "  --ssapre-save                \tUse ssapre algorithm to save callee-save registers\n"
                            "  --no-ssapre-save\n",
                            {cgCategory},
                            cl::DisableWith("--no-ssapre-save"));

cl::Option<bool> ssupreRestore({"--ssupre-restore"},
                                "  --ssupre-restore"
                                "             \tUse ssupre algorithm to restore callee-save registers\n"
                                "  --no-ssupre-restore\n",
                               {cgCategory},
                                cl::DisableWith("--no-ssupre-restore"));

cl::Option<bool> prepeep({"--prepeep"},
                         "  --prepeep                   \tPerform peephole optimization before RA\n"
                         "  --no-prepeep\n",
                         {cgCategory},
                         cl::DisableWith("--no-prepeep"));

cl::Option<bool> peep({"--peep"},
                      "  --peep                      \tPerform peephole optimization after RA\n"
                      "  --no-peep\n",
                      {cgCategory},
                      cl::DisableWith("--no-peep"));

cl::Option<bool> preschedule({"--preschedule"},
                             "  --preschedule               \tPerform prescheduling\n"
                             "  --no-preschedule\n",
                             {cgCategory},
                             cl::DisableWith("--no-preschedule"));

cl::Option<bool> schedule({"--schedule"},
                          "  --schedule                  \tPerform scheduling\n"
                          "  --no-schedule\n",
                          {cgCategory},
                          cl::DisableWith("--no-schedule"));

cl::Option<bool> retMerge({"--ret-merge"},
                           "  --ret-merge                 \tMerge return bb into a single destination\n"
                           "  --no-ret-merge              \tallows for multiple return bb\n",
                          {cgCategory},
                           cl::DisableWith("--no-ret-merge"));

cl::Option<bool> vregRename({"--vreg-rename"},
                             "  --vreg-rename"
                             "                  \tPerform rename of long live range around loops in coloring RA\n"
                             "  --no-vreg-rename\n",
                            {cgCategory},
                             cl::DisableWith("--no-vreg-rename"));

cl::Option<bool> fullcolor({"--fullcolor"},
                           "  --fullcolor                  \tPerform multi-pass coloring RA\n"
                           "  --no-fullcolor\n",
                           {cgCategory},
                           cl::DisableWith("--no-fullcolor"));

cl::Option<bool> writefieldopt({"--writefieldopt"},
                               "  --writefieldopt                  \tPerform WriteRefFieldOpt\n"
                               "  --no-writefieldopt\n",
                               {cgCategory},
                               cl::DisableWith("--no-writefieldopt"));

cl::Option<bool> dumpOlog({"--dump-olog"},
                          "  --dump-olog                 \tDump CFGO and ICO debug information\n"
                          "  --no-dump-olog\n",
                          {cgCategory},
                          cl::DisableWith("--no-dump-olog"));

cl::Option<bool> nativeopt({"--nativeopt"},
                           "  --nativeopt                 \tEnable native opt\n"
                           "  --no-nativeopt\n",
                           {cgCategory},
                           cl::DisableWith("--no-nativeopt"));

cl::Option<bool> objmap({"--objmap"},
                        "  --objmap"
                        "                    \tCreate object maps (GCTIBs) inside the main output (.s) file\n"
                        "  --no-objmap\n",
                        {cgCategory},
                        cl::DisableWith("--no-objmap"));

cl::Option<bool> yieldpoint({"--yieldpoint"},
                            "  --yieldpoint                \tGenerate yieldpoints [default]\n"
                            "  --no-yieldpoint\n",
                            {cgCategory},
                            cl::DisableWith("--no-yieldpoint"));

cl::Option<bool> proepilogue({"--proepilogue"},
                             "  --proepilogue               \tDo tail call optimization and"
                             " eliminate unnecessary prologue and epilogue.\n"
                             "  --no-proepilogue\n",
                             {cgCategory},
                             cl::DisableWith("--no-proepilogue"));

cl::Option<bool> localRc({"--local-rc"},
                         "  --local-rc                  \tHandle Local Stack RC [default]\n"
                         "  --no-local-rc\n",
                         {cgCategory},
                         cl::DisableWith("--no-local-rc"));

cl::Option<std::string> insertCall({"--insert-call"},
                                   "  --insert-call=name          \tInsert a call to the named function\n",
                                   {cgCategory});

cl::Option<bool> addDebugTrace({"--add-debug-trace"},
                               "  --add-debug-trace"
                               "           \tInstrument the output .s file to print call traces at runtime\n",
                               {cgCategory});

cl::Option<bool> addFuncProfile({"--add-func-profile"},
                                "  --add-func-profile"
                                "          \tInstrument the output .s file to record func at runtime\n",
                                {cgCategory});

cl::Option<std::string> classListFile({"--class-list-file"},
                                      "  --class-list-file"
                                      "           \tSet the class list file for the following generation options,\n"
                                      "                              \tif not given, "
                                      "generate for all visible classes\n"
                                      "                              \t--class-list-file=class_list_file\n",
                                      {cgCategory});

cl::Option<bool> genCMacroDef({"--gen-c-macro-def"},
                              "  --gen-c-macro-def"
                              "           \tGenerate a .def file that contains extra type metadata, including the\n"
                              "                              \tclass instance sizes and field offsets (default)\n"
                              "  --no-gen-c-macro-def\n",
                              {cgCategory},
                              cl::DisableWith("--no-gen-c-macro-def"));

cl::Option<bool> genGctibFile({"--gen-gctib-file"},
                              "  --gen-gctib-file"
                              "            \tGenerate a separate .s file for GCTIBs. Usually used together with\n"
                              "                              \t--no-objmap (not implemented yet)\n"
                              "  --no-gen-gctib-file\n",
                              {cgCategory},
                              cl::DisableWith("--no-gen-gctib-file"));

cl::Option<bool> stackProtectorStrong({"--stack-protector-strong"},
                            "  --stack-protector-strong                \tadd stack guard for some function \n"
                            "  --no-stack-protector-strong \n",
                            {cgCategory},
                            cl::DisableWith("--no-stack-protector-strong"));

cl::Option<bool> stackProtectorAll({"--stack-protector-all"},
                            "  --stack-protector-all                 \tadd stack guard for all functions \n"
                            "  --no-stack-protector-all\n",
                            {cgCategory},
                            cl::DisableWith("--no-stack-protector-all"));

cl::Option<bool> debug({"-g", "--g"},
                       "  -g                          \tGenerate debug information\n",
                       {cgCategory});

cl::Option<bool> gdwarf({"--gdwarf"},
                        "  --gdwarf                    \tGenerate dwarf infomation\n",
                        {cgCategory});

cl::Option<bool> gsrc({"--gsrc"},
                      "  --gsrc                      \tUse original source file instead of mpl file for debugging\n",
                      {cgCategory});

cl::Option<bool> gmixedsrc({"--gmixedsrc"},
                           "  --gmixedsrc"
                           "                 \tUse both original source file and mpl file for debugging\n",
                           {cgCategory});

cl::Option<bool> gmixedasm({"--gmixedasm"},
                           "  --gmixedasm"
                           "                 \tComment out both original source file and mpl file for debugging\n",
                           {cgCategory});

cl::Option<bool> profile({"--p", "-p"},
                         "  -p                          \tGenerate profiling infomation\n",
                         {cgCategory});

cl::Option<bool> withRaLinearScan({"--with-ra-linear-scan"},
                                  "  --with-ra-linear-scan       \tDo linear-scan register allocation\n",
                                  {cgCategory});

cl::Option<bool> withRaGraphColor({"--with-ra-graph-color"},
                                  "  --with-ra-graph-color       \tDo coloring-based register allocation\n",
                                  {cgCategory});

cl::Option<bool> patchLongBranch({"--patch-long-branch"},
                                 "  --patch-long-branch"
                                 "         \tEnable patching long distance branch with jumping pad\n",
                                 {cgCategory});

cl::Option<bool> constFold({"--const-fold"},
                           "  --const-fold                \tEnable constant folding\n"
                           "  --no-const-fold\n",
                           {cgCategory},
                           cl::DisableWith("--no-const-fold"));

cl::Option<std::string> ehExclusiveList({"--eh-exclusive-list"},
                                          "  --eh-exclusive-list         \tFor generating gold files in unit testing\n"
                                          "                              \t--eh-exclusive-list=list_file\n",
                                        {cgCategory});

cl::Option<bool> o0({"-O0", "--O0"},
                    "  -O0                         \tNo optimization.\n",
                    {cgCategory});

cl::Option<bool> o1({"-O1", "--O1"},
                    "  -O1                         \tDo some optimization.\n",
                    {cgCategory});

cl::Option<bool> o2({"-O2", "--O2"},
                    "  -O2                          \tDo some optimization.\n",
                    {cgCategory});

cl::Option<bool> os({"-Os", "--Os"},
                    "  -Os                          \tOptimize for size, based on O2.\n",
                    {cgCategory});

cl::Option<uint64_t> lsraBb({"--lsra-bb"},
                            "  --lsra-bb=NUM"
                            "               \tSwitch to spill mode if number of bb in function exceeds NUM\n",
                            {cgCategory});

cl::Option<uint64_t> lsraInsn({"--lsra-insn"},
                              "  --lsra-insn=NUM"
                              "             \tSwitch to spill mode if number of instructons in function exceeds NUM\n",
                              {cgCategory});

cl::Option<uint64_t> lsraOverlap({"--lsra-overlap"},
                                 "  --lsra-overlap=NUM          \toverlap NUM to decide pre spill in lsra\n",
                                 {cgCategory});

cl::Option<uint8_t> remat({"--remat"},
                          "  --remat                     \tEnable rematerialization during register allocation\n"
                          "                              \t     0: no rematerialization (default)\n"
                          "                              \t  >= 1: rematerialize constants\n"
                          "                              \t  >= 2: rematerialize addresses\n"
                          "                              \t  >= 3: rematerialize local dreads\n"
                          "                              \t  >= 4: rematerialize global dreads\n",
                          {cgCategory});

cl::Option<bool> suppressFileinfo({"--suppress-fileinfo"},
                                  "  --suppress-fileinfo         \tFor generating gold files in unit testing\n",
                                  {cgCategory});

cl::Option<bool> dumpCfg({"--dump-cfg"},
                         "  --dump-cfg\n",
                         {cgCategory});

cl::Option<std::string> target({"--target"},
                               "  --target=TARGETMACHINE \t generate code for TARGETMACHINE\n",
                               {cgCategory},
                               cl::optionalValue);

cl::Option<std::string> dumpPhases({"--dump-phases"},
                                   "  --dump-phases=PHASENAME,..."
                                   " \tEnable debug trace for specified phases in the comma separated list\n",
                                   {cgCategory});

cl::Option<std::string> skipPhases({"--skip-phases"},
                                   "  --skip-phases=PHASENAME,..."
                                   " \tSkip the phases specified in the comma separated list\n",
                                   {cgCategory});

cl::Option<std::string> skipFrom({"--skip-from"},
                                 "  --skip-from=PHASENAME       \tSkip the rest phases from PHASENAME(included)\n",
                                 {cgCategory});

cl::Option<std::string> skipAfter({"--skip-after"},
                                  "  --skip-after=PHASENAME      \tSkip the rest phases after PHASENAME(excluded)\n",
                                  {cgCategory});

cl::Option<std::string> dumpFunc({"--dump-func"},
                                 "  --dump-func=FUNCNAME"
                                 "        \tDump/trace only for functions whose names contain FUNCNAME as substring\n"
                                 "                              \t(can only specify once)\n",
                                 {cgCategory});

cl::Option<bool> timePhases({"--time-phases"},
                            "  --time-phases               \tCollect compilation time stats for each phase\n"
                            "  --no-time-phases            \tDon't Collect compilation time stats for each phase\n",
                            {cgCategory},
                            cl::DisableWith("--no-time-phases"));

cl::Option<bool> useBarriersForVolatile({"--use-barriers-for-volatile"},
                                        "  --use-barriers-for-volatile \tOptimize volatile load/str\n"
                                        "  --no-use-barriers-for-volatile\n",
                                        {cgCategory},
                                        cl::DisableWith("--no-use-barriers-for-volatile"));

cl::Option<std::string> range({"--range"},
                              "  --range=NUM0,NUM1           \tOptimize only functions in the range [NUM0, NUM1]\n",
                              {cgCategory});

cl::Option<uint8_t> fastAlloc({"--fast-alloc"},
                              "  --fast-alloc=[0/1]          \tO2 RA fast mode, set to 1 to spill all registers\n",
                              {cgCategory});

cl::Option<std::string> spillRange({"--spill_range"},
                                   "  --spill_range=NUM0,NUM1     \tO2 RA spill registers in the range [NUM0, NUM1]\n",
                                   {cgCategory});

cl::Option<bool> dupBb({"--dup-bb"},
                       "  --dup-bb                 \tAllow cfg optimizer to duplicate bb\n"
                       "  --no-dup-bb              \tDon't allow cfg optimizer to duplicate bb\n",
                       {cgCategory},
                       cl::DisableWith("--no-dup-bb"));

cl::Option<bool> calleeCfi({"--callee-cfi"},
                           "  --callee-cfi                \tcallee cfi message will be generated\n"
                           "  --no-callee-cfi             \tcallee cfi message will not be generated\n",
                           {cgCategory},
                           cl::DisableWith("--no-callee-cfi"));

cl::Option<bool> printFunc({"--print-func"},
                            "  --print-func\n"
                            "  --no-print-func\n",
                           {cgCategory},
                            cl::DisableWith("--no-print-func"));

cl::Option<std::string> cyclePatternList({"--cycle-pattern-list"},
                                         "  --cycle-pattern-list        \tFor generating cycle pattern meta\n"
                                         "                              \t--cycle-pattern-list=list_file\n",
                                         {cgCategory});

cl::Option<std::string> duplicateAsmList({"--duplicate_asm_list"},
                                         "  --duplicate_asm_list        \tDuplicate asm functions to delete plt call\n"
                                         "                              \t--duplicate_asm_list=list_file\n",
                                         {cgCategory});

cl::Option<std::string> duplicateAsmList2({"--duplicate_asm_list2"},
                                          "  --duplicate_asm_list2"
                                          "       \tDuplicate more asm functions to delete plt call\n"
                                          "                              \t--duplicate_asm_list2=list_file\n",
                                          {cgCategory});

cl::Option<std::string> blockMarker({"--block-marker"},
                                    "  --block-marker"
                                    "              \tEmit block marker symbols in emitted assembly files\n",
                                    {cgCategory});

cl::Option<bool> soeCheck({"--soe-check"},
                          "  --soe-check                 \tInsert a soe check instruction[default off]\n",
                          {cgCategory});

cl::Option<bool> checkArraystore({"--check-arraystore"},
                                 "  --check-arraystore          \tcheck arraystore exception[default off]\n"
                                 "  --no-check-arraystore\n",
                                 {cgCategory},
                                 cl::DisableWith("--no-check-arraystore"));

cl::Option<bool> debugSchedule({"--debug-schedule"},
                               "  --debug-schedule            \tdump scheduling information\n"
                               "  --no-debug-schedule\n",
                               {cgCategory},
                               cl::DisableWith("--no-debug-schedule"));

cl::Option<bool> bruteforceSchedule({"--bruteforce-schedule"},
                                    "  --bruteforce-schedule       \tdo brute force schedule\n"
                                    "  --no-bruteforce-schedule\n",
                                    {cgCategory},
                                    cl::DisableWith("--no-bruteforce-schedule"));

cl::Option<bool> simulateSchedule({"--simulate-schedule"},
                                  "  --simulate-schedule         \tdo simulate schedule\n"
                                  "  --no-simulate-schedule\n",
                                  {cgCategory},
                                  cl::DisableWith("--no-simulate-schedule"));

cl::Option<bool> crossLoc({"--cross-loc"},
                          "  --cross-loc                 \tcross loc insn schedule\n"
                          "  --no-cross-loc\n",
                          {cgCategory},
                          cl::DisableWith("--no-cross-loc"));

cl::Option<std::string> floatAbi({"--float-abi"},
                                 "  --float-abi=name            \tPrint the abi type.\n"
                                 "                              \tname=hard: abi-hard (Default)\n"
                                 "                              \tname=soft: abi-soft\n"
                                 "                              \tname=softfp: abi-softfp\n",
                                 {cgCategory});

cl::Option<std::string> filetype({"--filetype"},
                                 "  --filetype=name             \tChoose a file type.\n"
                                 "                              \tname=asm: Emit an assembly file (Default)\n"
                                 "                              \tname=obj: Emit an object file\n"
                                 "                              \tname=null: not support yet\n",
                                 {cgCategory});

cl::Option<bool> longCalls({"--long-calls"},
                           "  --long-calls                \tgenerate long call\n"
                           "  --no-long-calls\n",
                           {cgCategory},
                           cl::DisableWith("--no-long-calls"));

cl::Option<bool> functionSections({"--function-sections"},
                                  " --function-sections           \t \n"
                                  "  --no-function-sections\n",
                                  {cgCategory},
                                  cl::DisableWith("--no-function-sections"));

cl::Option<bool> omitFramePointer({"--omit-frame-pointer"},
                                  " --omit-frame-pointer          \t do not use frame pointer \n"
                                  " --no-omit-frame-pointer\n",
                                  {cgCategory},
                                  cl::DisableWith("--no-omit-frame-pointer"));

cl::Option<bool> fastMath({"--fast-math"},
                          "  --fast-math                  \tPerform fast math\n"
                          "  --no-fast-math\n",
                          {cgCategory},
                          cl::DisableWith("--no-fast-math"));

cl::Option<bool> tailcall({"--tailcall"},
                          "  --tailcall                   \tDo tail call optimization\n"
                          "  --no-tailcall\n",
                          {cgCategory},
                          cl::DisableWith("--no-tailcall"));

cl::Option<bool> alignAnalysis({"--align-analysis"},
                               "  --align-analysis                 \tPerform alignanalysis\n"
                               "  --no-align-analysis\n",
                               {cgCategory},
                               cl::DisableWith("--no-align-analysis"));

cl::Option<bool> cgSsa({"--cg-ssa"},
                       "  --cg-ssa                     \tPerform cg ssa\n"
                       "  --no-cg-ssa\n",
                       {cgCategory},
                       cl::DisableWith("--no-cg-ssa"));

cl::Option<bool> common({"--common"},
                        " --common           \t \n"
                        " --no-common\n",
                       {cgCategory},
                       cl::DisableWith("--no-common"));

cl::Option<bool> arm64Ilp32({"--arm64-ilp32"},
                            " --arm64-ilp32                 \tarm64 with a 32-bit ABI instead of a 64bit ABI\n"
                            " --no-arm64-ilp32\n",
                            {cgCategory},
                            cl::DisableWith("--no-arm64-ilp32"));

cl::Option<bool> condbrAlign({"--condbr-align"},
                             "  --condbr-align                   \tPerform condbr align\n"
                             "  --no-condbr-align\n",
                             {cgCategory},
                             cl::DisableWith("--no-condbr-align"));

cl::Option<uint32_t> alignMinBbSize({"--align-min-bb-size"},
                                    " --align-min-bb-size=NUM"
                                    "           \tO2 Minimum bb size for alignment   unit:byte\n",
                                    {cgCategory});

cl::Option<uint32_t> alignMaxBbSize({"--align-max-bb-size"},
                                    " --align-max-bb-size=NUM"
                                    "           \tO2 Maximum bb size for alignment   unit:byte\n",
                                    {cgCategory});

cl::Option<uint32_t> loopAlignPow({"--loop-align-pow"},
                                  " --loop-align-pow=NUM           \tO2 loop bb align pow (NUM == 0, no loop-align)\n",
                                  {cgCategory});

cl::Option<uint32_t> jumpAlignPow({"--jump-align-pow"},
                                  " --jump-align-pow=NUM           \tO2 jump bb align pow (NUM == 0, no jump-align)\n",
                                  {cgCategory});

cl::Option<uint32_t> funcAlignPow({"--func-align-pow"},
                                  " --func-align-pow=NUM           \tO2 func bb align pow (NUM == 0, no func-align)\n",
                                  {cgCategory});

}
