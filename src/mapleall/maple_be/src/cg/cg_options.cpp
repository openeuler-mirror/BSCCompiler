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

#include <string>
#include "driver_options.h"

namespace opts::cg {

maplecl::Option<bool> fnoSemanticInterposition({"-fno-semantic-interposition"},
    "  --fno-semantic-interposition\tIf interposition happens for functions, the overwriting function "
    "will have precisely the same semantics (and side effects).\n"
    "  -fsemantic-interposition\n",
    {cgCategory, driverCategory}, kOptCommon | kOptOptimization, maplecl::DisableWith("-fsemantic-interposition"));

maplecl::Option<bool> verboseAsm({"--verbose-asm"},
    "  --verbose-asm               \tAdd comments to asm output\n"
    "  --no-verbose-asm            \n",
    {driverCategory, cgCategory}, kOptMaple, maplecl::DisableWith("--no-verbose-asm"));

maplecl::Option<bool> verboseCg({"--verbose-cg"},
    "  --verbose-cg                \tAdd comments to cg output\n"
    "  --no-verbose-cg             \n",
    {driverCategory, cgCategory}, kOptMaple, maplecl::DisableWith("--no-verbose-cg"));

maplecl::Option<bool> maplelinker({"--maplelinker"},
    "  --maplelinker               \tGenerate the MapleLinker .s format\n"
    "  --no-maplelinker            \n",
    {cgCategory}, maplecl::DisableWith("--no-maplelinker"));

maplecl::Option<bool> quiet({"--quiet"},
    "  --quiet                     \tBe quiet (don't output debug messages)\n"
    "  --no-quiet                  \n",
    {driverCategory, cgCategory}, kOptMaple, maplecl::DisableWith("--no-quiet"));

maplecl::Option<bool> cg({"--cg"},
    "  --cg                        \tGenerate the output .s file\n"
    "  --no-cg                     \n",
    {cgCategory}, maplecl::DisableWith("--no-cg"));

maplecl::Option<bool> replaceAsm({"--replaceasm"},
    "  --replaceasm                \tReplace the the assembly code\n"
    "  --no-replaceasm             \n",
    {cgCategory}, maplecl::DisableWith("--no-replaceasm"));

maplecl::Option<bool> generalRegOnly({"--general-reg-only"},
    " --general-reg-only           \tdisable floating-point or Advanced SIMD registers\n"
    " --no-general-reg-only         \n",
    {cgCategory}, maplecl::DisableWith("--no-general-reg-only"));

maplecl::Option<bool> lazyBinding({"--lazy-binding"},
    "  --lazy-binding              \tBind class symbols lazily[default off]\n",
    {cgCategory}, maplecl::DisableWith("--no-lazy-binding"));

maplecl::Option<bool> hotFix({"--hot-fix"},
    "  --hot-fix                   \tOpen for App hot fix[default off]\n"
    "  --no-hot-fix                \n",
    {cgCategory}, maplecl::DisableWith("--no-hot-fix"));

maplecl::Option<bool> ebo({"--ebo"},
    "  --ebo                       \tPerform Extend block optimization\n"
    "  --no-ebo                    \n",
    {cgCategory}, maplecl::DisableWith("--no-ebo"));

maplecl::Option<bool> ipara({"--ipa-ra"},
    "  --ipa-ra                    \tPerform IPA RA optimization\n"
    "  --no-ipa-ra                 \n",
    {cgCategory}, maplecl::DisableWith("--no-ipa-ra"));

maplecl::Option<bool> cfgo({"--cfgo"},
    "  --cfgo                      \tPerform control flow optimization\n"
    "  --no-cfgo                   \n",
    {cgCategory}, maplecl::DisableWith("--no-cfgo"));

maplecl::Option<bool> ico({"--ico"},
    "  --ico                       \tPerform if-conversion optimization\n"
    "  --no-ico                    \n",
    {cgCategory}, maplecl::DisableWith("--no-ico"));

maplecl::Option<bool> storeloadopt({"--storeloadopt"},
    "  --storeloadopt              \tPerform global store-load optimization\n"
    "  --no-storeloadopt           \n",
    {cgCategory}, maplecl::DisableWith("--no-storeloadopt"));

maplecl::Option<bool> globalopt({"--globalopt"},
    "  --globalopt                 \tPerform global optimization\n"
    "  --no-globalopt              \n",
    {cgCategory}, maplecl::DisableWith("--no-globalopt"));

maplecl::Option<bool> hotcoldsplit({"--hotcoldsplit"},
    "  --hotcoldsplit              \tPerform HotColdSplit optimization\n"
    "  --no-hotcoldsplit           \n",
    {cgCategory}, maplecl::DisableWith("--no-hotcoldsplit"));

maplecl::Option<bool> preraopt({"--preraopt"},
    "  --preraopt                  \tPerform live interval simplification in RA\n"
    "  --no-preraopt               \n",
    {cgCategory}, maplecl::DisableWith("--no-preraopt"));

maplecl::Option<bool> lsraLvarspill({"--lsra-lvarspill"},
    "  --lsra-lvarspill            \tPerform LSRA spill using local ref var stack locations\n"
    "  --no-lsra-lvarspill         \n",
    {cgCategory}, maplecl::DisableWith("--no-lsra-lvarspill"));

maplecl::Option<bool> lsraOptcallee({"--lsra-optcallee"},
    "  --lsra-optcallee            \tSpill callee if only one def to use\n"
    "  --no-lsra-optcallee         \n",
    {cgCategory}, maplecl::DisableWith("--no-lsra-optcallee"));

maplecl::Option<bool> calleeregsPlacement({"--calleeregs-placement"},
    "  --calleeregs-placement      \tOptimize placement of callee-save registers\n"
    "  --no-calleeregs-placement   \n",
    {cgCategory}, maplecl::DisableWith("--no-calleeregs-placement"));

maplecl::Option<bool> ssapreSave({"--ssapre-save"},
    "  --ssapre-save               \tUse ssapre algorithm to save callee-save registers\n"
    "  --no-ssapre-save             \n",
    {cgCategory}, maplecl::DisableWith("--no-ssapre-save"));

maplecl::Option<bool> ssupreRestore({"--ssupre-restore"},
    "  --ssupre-restore            \tUse ssupre algorithm to restore callee-save registers\n"
    "  --no-ssupre-restore         \n",
    {cgCategory}, maplecl::DisableWith("--no-ssupre-restore"));

maplecl::Option<bool> newCg({"--newcg"},
    "  --newcg                     \tUse new CG infrastructure\n"
    "  --no-newcg                  \n",
    {cgCategory}, maplecl::DisableWith("--no-newcg"));

maplecl::Option<bool> prepeep({"--prepeep"},
    "  --prepeep                   \tPerform peephole optimization before RA\n"
    "  --no-prepeep                \n",
    {cgCategory}, maplecl::DisableWith("--no-prepeep"));

maplecl::Option<bool> peep({"--peep"},
    "  --peep                      \tPerform peephole optimization after RA\n"
    "  --no-peep                   \n",
    {cgCategory}, maplecl::DisableWith("--no-peep"));

maplecl::Option<bool> preschedule({"--preschedule"},
    "  --preschedule               \tPerform prescheduling\n"
    "  --no-preschedule            \n",
    {cgCategory}, maplecl::DisableWith("--no-preschedule"));

maplecl::Option<bool> schedule({"--schedule"},
    "  --schedule                  \tPerform scheduling\n"
    "  --no-schedule               \n",
    {cgCategory}, maplecl::DisableWith("--no-schedule"));

maplecl::Option<bool> retMerge({"--ret-merge"},
    "  --ret-merge                 \tMerge return bb into a single destination\n"
    "  --no-ret-merge              \tallows for multiple return bb\n",
    {cgCategory}, maplecl::DisableWith("--no-ret-merge"));

maplecl::Option<bool> vregRename({"--vreg-rename"},
    "  --vreg-rename               \tPerform rename of long live range around loops in coloring RA.\n"
    "  --no-vreg-rename            \n",
    {cgCategory}, maplecl::DisableWith("--no-vreg-rename"));

maplecl::Option<bool> fullcolor({"--fullcolor"},
    "  --fullcolor                 \tPerform multi-pass coloring RA\n"
    "  --no-fullcolor              \n",
    {cgCategory}, maplecl::DisableWith("--no-fullcolor"));

maplecl::Option<bool> writefieldopt({"--writefieldopt"},
    "  --writefieldopt             \tPerform WriteRefFieldOpt\n"
    "  --no-writefieldopt          \n",
    {cgCategory}, maplecl::DisableWith("--no-writefieldopt"));

maplecl::Option<bool> dumpOlog({"--dump-olog"},
    "  --dump-olog                 \tDump CFGO and ICO debug information\n"
    "  --no-dump-olog              \n",
    {cgCategory}, maplecl::DisableWith("--no-dump-olog"));

maplecl::Option<bool> nativeopt({"--nativeopt"},
    "  --nativeopt                 \tEnable native opt\n"
    "  --no-nativeopt              \n",
    {cgCategory}, maplecl::DisableWith("--no-nativeopt"));

maplecl::Option<bool> objmap({"--objmap"},
    "  --objmap                    \tCreate object maps (GCTIBs) inside the main output (.s) file\n"
    "  --no-objmap                 \n",
    {cgCategory}, maplecl::DisableWith("--no-objmap"));

maplecl::Option<bool> yieldpoint({"--yieldpoint"},
    "  --yieldpoint                \tGenerate yieldpoints [default]\n"
    "  --no-yieldpoint             \n",
    {cgCategory}, maplecl::DisableWith("--no-yieldpoint"));

maplecl::Option<bool> proepilogue({"--proepilogue"},
    "  --proepilogue               \tDo tail call optimization and eliminate unnecessary prologue and epilogue.\n"
    "  --no-proepilogue            \n",
    {cgCategory}, maplecl::DisableWith("--no-proepilogue"));

maplecl::Option<bool> localRc({"--local-rc"},
    "  --local-rc                  \tHandle Local Stack RC [default]\n"
    "  --no-local-rc               \n",
    {cgCategory}, maplecl::DisableWith("--no-local-rc"));

maplecl::Option<bool> addDebugTrace({"--add-debug-trace"},
    "  --add-debug-trace           \tInstrumentation the output .s file to print call traces at runtime\n",
    {cgCategory});


maplecl::Option<std::string> classListFile({"--class-list-file"},
    "  --class-list-file           \tSet the class list file for the following generation options, if not given, "
    "generate for all visible classes  \t--class-list-file=class_list_file\n",
    {cgCategory});

maplecl::Option<bool> genCMacroDef({"--gen-c-macro-def"},
    "  --gen-c-macro-def           \tGenerate a .def file that contains extra type metadata, including the class "
    "instance sizes and field offsets (default)\n"
    "  --no-gen-c-macro-def        \n",
    {cgCategory}, maplecl::DisableWith("--no-gen-c-macro-def"));

maplecl::Option<bool> genGctibFile({"--gen-gctib-file"},
    "  --gen-gctib-file            \tGenerate a separate .s file for GCTIBs. Usually used together with "
    "--no-objmap (not implemented yet)\n"
    "  --no-gen-gctib-file         \n",
    {cgCategory}, maplecl::DisableWith("--no-gen-gctib-file"));

maplecl::Option<bool> unwindTables({"--unwind-tables"},
    "  --unwind-tables             \tgenerate unwind tables for function \n"
    "  --no-unwind-tables          \n",
    {cgCategory, driverCategory}, kOptMaple, maplecl::DisableWith("--no-unwind-tables"));

maplecl::Option<bool> debug({"-g", "--g"},
    "  -g                          \tGenerate debug information\n",
    {cgCategory});

maplecl::Option<bool> gdwarf({"--gdwarf"},
    "  --gdwarf                    \tGenerate dwarf infomation\n",
    {cgCategory});

maplecl::Option<bool> gsrc({"--gsrc"},
    "  --gsrc                       \tUse original source file instead of mpl file for debugging\n",
    {cgCategory});

maplecl::Option<bool> gmixedsrc({"--gmixedsrc"},
    "  --gmixedsrc                 \tUse both original source file and mpl file for debugging\n",
    {cgCategory});

maplecl::Option<bool> gmixedasm({"--gmixedasm"},
    "  --gmixedasm                 \tComment out both original source file and mpl file for debugging\n",
    {cgCategory});

maplecl::Option<bool> profile({"--p", "-p"},
    "  -p                          \tGenerate profiling infomation\n",
    {cgCategory});

maplecl::Option<bool> withRaLinearScan({"--with-ra-linear-scan"},
    "  --with-ra-linear-scan       \tDo linear-scan register allocation\n",
    {cgCategory});

maplecl::Option<bool> withRaGraphColor({"--with-ra-graph-color"},
    "  --with-ra-graph-color       \tDo coloring-based register allocation\n",
    {cgCategory});

maplecl::Option<bool> patchLongBranch({"--patch-long-branch"},
    "  --patch-long-branch         \tEnable patching long distance branch with jumping pad\n",
    {driverCategory, cgCategory}, kOptMaple);

maplecl::Option<bool> constFold({"--const-fold"},
    "  --const-fold                \tEnable constant folding\n"
    "  --no-const-fold             \n",
    {cgCategory}, maplecl::DisableWith("--no-const-fold"));

maplecl::Option<std::string> ehExclusiveList({"--eh-exclusive-list"},
    "  --eh-exclusive-list         \tFor generating gold files in unit testing --eh-exclusive-list=list_file\n",
    {cgCategory});

maplecl::Option<bool> o0({"-O0", "--O0"},
    "  -O0                         \tNo optimization.\n",
    {cgCategory});

maplecl::Option<bool> o1({"-O1", "--O1"},
    "  -O1                         \tDo some optimization.\n",
    {cgCategory});

maplecl::Option<bool> o2({"-O2", "--O2"},
    "  -O2                         \tDo some optimization.\n",
    {cgCategory});

maplecl::Option<bool> os({"-Os", "--Os"},
    "  -Os                         \tOptimize for size, based on O2.\n",
    {cgCategory});

maplecl::Option<bool> oLitecg({"-Olitecg", "--Olitecg"},
    " -Olitecg                     \tOptimize for litecg.\n",
    {cgCategory});

maplecl::Option<uint64_t> lsraBb({"--lsra-bb"},
    "  --lsra-bb=NUM               \tSwitch to spill mode if number of bb in function exceeds NUM\n",
    {cgCategory});

maplecl::Option<uint64_t> lsraInsn({"--lsra-insn"},
    "  --lsra-insn=NUM             \tSwitch to spill mode if number of instructons in function exceeds NUM.\n",
    {cgCategory});

maplecl::Option<uint64_t> lsraOverlap({"--lsra-overlap"},
    "  --lsra-overlap=NUM          \toverlap NUM to decide pre spill in lsra\n",
    {cgCategory});

maplecl::Option<uint8_t> remat({"--remat"},
    "  --remat                     \tEnable rematerialization during register allocation\n"
    "                              \t    0: no rematerialization (default)\n"
    "                              \t  >= 1: rematerialize constants\n"
    "                              \t  >= 2: rematerialize addresses\n"
    "                              \t  >= 3: rematerialize local dreads\n"
    "                              \t  >= 4: rematerialize global dreads\n",
    {cgCategory});

maplecl::Option<bool> suppressFileinfo({"--suppress-fileinfo"},
    "  --suppress-fileinfo         \tFor generating gold files in unit testing\n",
    {cgCategory});

maplecl::Option<bool> dumpCfg({"--dump-cfg"},
    "  --dump-cfg                  \n",
    {cgCategory});

maplecl::Option<std::string> target({"--target"},
    "  --target=TARGETMACHINE      \tgenerate code for TARGETMACHINE\n",
    {cgCategory}, maplecl::kOptionalValue);

maplecl::Option<std::string> dumpPhases({"--dump-phases"},
    "  --dump-phases=PHASENAME,... \tEnable debug trace for specified phases in the comma separated list\n",
    {cgCategory});

maplecl::Option<std::string> skipPhases({"--skip-phases"},
    "  --skip-phases=PHASENAME,... \tSkip the phases specified in the comma separated list\n",
    {cgCategory, driverCategory}, kOptMaple);

maplecl::Option<std::string> skipFrom({"--skip-from"},
    "  --skip-from=PHASENAME       \tSkip the rest phases from PHASENAME(included)\n",
    {cgCategory});

maplecl::Option<std::string> skipAfter({"--skip-after"},
    "  --skip-after=PHASENAME      \tSkip the rest phases after PHASENAME(excluded)\n",
    {cgCategory});

maplecl::Option<std::string> dumpFunc({"--dump-func"},
    "  --dump-func=FUNCNAME        \tDump/trace only for functions whose names contain FUNCNAME as substring "
    "(can only specify once)\n",
    {cgCategory});

maplecl::Option<bool> useBarriersForVolatile({"--use-barriers-for-volatile"},
    "  --use-barriers-for-volatile \tOptimize volatile load/str\n"
    "  --no-use-barriers-for-volatile\n",
    {cgCategory}, maplecl::DisableWith("--no-use-barriers-for-volatile"));

maplecl::Option<std::string> range({"--range"},
    "  --range=NUM0,NUM1           \tOptimize only functions in the range [NUM0, NUM1]\n",
    {cgCategory});

maplecl::Option<uint8_t> fastAlloc({"--fast-alloc"},
    "  --fast-alloc=[0/1]          \tO2 RA fast mode, set to 1 to spill all registers\n",
    {cgCategory});

maplecl::Option<std::string> spillRange({"--spill_range"},
    "  --spill_range=NUM0,NUM1     \tO2 RA spill registers in the range [NUM0, NUM1]\n",
    {cgCategory});

maplecl::Option<bool> dupBb({"--dup-bb"},
    "  --dup-bb                    \tAllow cfg optimizer to duplicate bb\n"
    "  --no-dup-bb                 \tDon't allow cfg optimizer to duplicate bb\n",
    {cgCategory}, maplecl::DisableWith("--no-dup-bb"));

maplecl::Option<bool> calleeCfi({"--callee-cfi"},
    "  --callee-cfi                \tcallee cfi message will be generated\n"
    "  --no-callee-cfi             \tcallee cfi message will not be generated\n",
    {cgCategory}, maplecl::DisableWith("--no-callee-cfi"));

maplecl::Option<bool> printFunc({"--print-func"},
    "  --print-func                \n"
    "  --no-print-func             \n",
    {cgCategory}, maplecl::DisableWith("--no-print-func"));

maplecl::Option<std::string> cyclePatternList({"--cycle-pattern-list"},
    "  --cycle-pattern-list        \tFor generating cycle pattern meta\n"
    "                              \t--cycle-pattern-list=list_file\n",
    {cgCategory});

maplecl::Option<std::string> duplicateAsmList({"--duplicate_asm_list"},
    "  --duplicate_asm_list        \tDuplicate asm functions to delete plt call\n"
    "                              \t--duplicate_asm_list=list_file\n",
    {driverCategory, cgCategory}, kOptMaple);

maplecl::Option<std::string> duplicateAsmList2({"--duplicate_asm_list2"},
    "  --duplicate_asm_list2       \tDuplicate more asm functions to delete plt call\n"
    "                              \t--duplicate_asm_list2=list_file\n",
    {cgCategory});

maplecl::Option<std::string> blockMarker({"--block-marker"},
    "  --block-marker              \tEmit block marker symbols in emitted assembly files\n",
    {cgCategory});

maplecl::Option<bool> soeCheck({"--soe-check"},
    "  --soe-check                 \tInsert a soe check instruction[default off]\n",
    {cgCategory});

maplecl::Option<bool> checkArraystore({"--check-arraystore"},
    "  --check-arraystore          \tcheck arraystore exception[default off]\n"
    "  --no-check-arraystore       \n",

    {cgCategory}, maplecl::DisableWith("--no-check-arraystore"));

maplecl::Option<bool> debugSchedule({"--debug-schedule"},
    "  --debug-schedule            \tdump scheduling information\n"
    "  --no-debug-schedule         \n",
    {cgCategory}, maplecl::DisableWith("--no-debug-schedule"));

maplecl::Option<bool> bruteforceSchedule({"--bruteforce-schedule"},
    "  --bruteforce-schedule       \tdo brute force schedule\n"
    "  --no-bruteforce-schedule    \n",
    {cgCategory}, maplecl::DisableWith("--no-bruteforce-schedule"));

maplecl::Option<bool> simulateSchedule({"--simulate-schedule"},
    "  --simulate-schedule         \tdo simulate schedule\n"
    "  --no-simulate-schedule      \n",
    {cgCategory}, maplecl::DisableWith("--no-simulate-schedule"));

maplecl::Option<bool> crossLoc({"--cross-loc"},
    "  --cross-loc                 \tcross loc insn schedule\n"
    "  --no-cross-loc              \n",
    {cgCategory}, maplecl::DisableWith("--no-cross-loc"));

maplecl::Option<std::string> floatAbi({"--float-abi"},
    "  --float-abi=name            \tPrint the abi type.\n"
    "                              \tname=hard: abi-hard (Default)\n"
    "                              \tname=soft: abi-soft\n"
    "                              \tname=softfp: abi-softfp\n",
    {cgCategory});

maplecl::Option<std::string> filetype({"--filetype"},
    "  --filetype=name             \tChoose a file type.\n"
    "                              \tname=asm: Emit an assembly file (Default)\n"
    "                              \tname=obj: Emit an object file\n"
    "                              \tname=null: not support yet\n",
    {cgCategory});

maplecl::Option<bool> longCalls({"--long-calls"},
    "  --long-calls                \tgenerate long call\n"
    "  --no-long-calls             \n",
    {cgCategory}, maplecl::DisableWith("--no-long-calls"));

maplecl::Option<bool> functionSections({"-ffunction-sections", "--function-sections"},
    "  --function-sections         \tPlace each function item into its own section in the output file "
    "if the target supports arbitrary sections.\n"
    "  --no-function-sections      \n",
    {cgCategory, driverCategory}, kOptCommon, maplecl::DisableWith("--no-function-sections"));

maplecl::Option<bool> dataSections({"-fdata-sections", "--data-sections"},
    "  --data-sections             \tPlace each data item into its own section in the output file "
    "if the target supports arbitrary sections.\n"
    "  --no-data-sections          \n",
    {cgCategory, driverCategory}, kOptCommon, maplecl::DisableWith("--no-data-sections"));

maplecl::Option<bool> omitFramePointer({"-fomit-frame-pointer", "--omit-frame-pointer"},
    "  --omit-frame-pointer        \tDo not use frame pointer for non-leaf func\n"
    "  --no-omit-frame-pointer     \n",
    {cgCategory, driverCategory}, kOptCommon | kOptOptimization,
    maplecl::DisableEvery({"--no-omit-frame-pointer", "-fno-omit-frame-pointer"}));

maplecl::Option<bool> omitLeafFramePointer({"-momit-leaf-frame-pointer", "--omit-leaf-frame-pointer"},
    "  --omit-leaf-frame-pointer   \tDo not use frame pointer for leaf func\n"
    "  --no-omit-leaf-frame-pointer\n",
    {cgCategory, driverCategory}, kOptCommon,
    maplecl::DisableEvery({"--no-omit-leaf-frame-pointer", "-mno-omit-leaf-frame-pointer"}));

maplecl::Option<bool> fastMath({"--fast-math"},
    "  --fast-math                 \tPerform fast math\n"
    "  --no-fast-math              \n",
    {cgCategory}, maplecl::DisableWith("--no-fast-math"));

maplecl::Option<bool> alignAnalysis({"--align-analysis"},
    "  --align-analysis            \tPerform alignanalysis\n"
    "  --no-align-analysis         \n",
    {driverCategory, cgCategory}, kOptMaple, maplecl::DisableWith("--no-align-analysis"));

maplecl::Option<bool> cgSsa({"--cg-ssa"},
    "  --cg-ssa                    \tPerform cg ssa\n"
    "  --no-cg-ssa                 \n",
    {cgCategory}, maplecl::DisableWith("--no-cg-ssa"));

maplecl::Option<bool> layoutColdPath({"--layout-cold-path"},
    "  --layout-cold-path          \tLayout cold path out of hot path\n"
    "  --no-layout-cold-path       \n",
    {cgCategory}, maplecl::DisableWith("--no-layout-cold-path"));

maplecl::Option<bool> globalSchedule({"--global-schedule"},
    "  --global-schedule           \tPerform global schedule\n"
    "  --no-global-schedule        \n",
    {cgCategory}, maplecl::DisableWith("--no-global-schedule"));

maplecl::Option<bool> localSchedule({"--local-schedule"},
    "  --local-schedule            \tPerform local schedule\n"
    "  --no-local-schedule         \n",
    {cgCategory}, maplecl::DisableWith("--no-local-schedule"));

maplecl::Option<bool> cgMemAlias({"--cg-mem-alias"},
    "  --cg-mem-alias              \tEnable cg mem alias\n"
    "  --no-cg-mem-alias           \n",
    {cgCategory}, maplecl::DisableWith("--no-cg-mem-alias"));

maplecl::Option<bool> calleeEnsureParam({"--callee-ensure-param"},
    "  --callee-ensure-param       \tCallee ensure valid vb of params\n"
    "  --caller-ensure-param       \n",
    {cgCategory}, maplecl::DisableWith("--caller-ensure-param"));

maplecl::Option<bool> common({"-fcommon", "--common"},
    "  --common                    \t\n"
    "  --no-common                 \n",
    {cgCategory, driverCategory}, kOptCommon, maplecl::DisableEvery({"--no-common", "-fno-common"}));

maplecl::Option<bool> condbrAlign({"--condbr-align"},
    "  --condbr-align              \tPerform condbr align\n"
    "  --no-condbr-align           \n",
    {cgCategory}, maplecl::DisableWith("--no-condbr-align"));

maplecl::Option<uint32_t> alignMinBbSize({"--align-min-bb-size"},
    "  --align-min-bb-size=NUM     \tO2 Minimum bb size for alignment unit:byte\n",
    {cgCategory});

maplecl::Option<uint32_t> alignMaxBbSize({"--align-max-bb-size"},
    "  --align-max-bb-size=NUM     \tO2 Maximum bb size for alignment unit:byte\n",
    {cgCategory});

maplecl::Option<uint32_t> loopAlignPow({"--loop-align-pow"},
    "  --loop-align-pow=NUM        \tO2 loop bb align pow (NUM == 0, no loop-align)\n",
    {cgCategory});

maplecl::Option<uint32_t> jumpAlignPow({"--jump-align-pow"},
    "  --jump-align-pow=NUM        \tO2 jump bb align pow (NUM == 0, no jump-align)\n",
    {cgCategory});

maplecl::Option<uint32_t> funcAlignPow({"--func-align-pow"},
    "  --func-align-pow=NUM        \tO2 func bb align pow (NUM == 0, no func-align)\n",
    {driverCategory, cgCategory}, kOptMaple);

maplecl::Option<uint32_t> coldPathThreshold({"--cold-path-threshold"},
    "  --cold-path-threshold=NUM   \tLayout cold path out of hot path\n",
    {cgCategory});

maplecl::Option<bool> litePgoGen({"--lite-pgo-gen"},
    "  --lite-pgo-gen              \tInstrumentation CG bb and generate bb-cnt info\n"
    "  --no-lite-pgo-gen           \n",
    {driverCategory, cgCategory}, kOptMaple, maplecl::DisableWith("--no-lite-pgo-gen"));

maplecl::Option<std::string> instrumentationDir ({"--instrumentation-dir"},
    "  --instrumentation-dir=directory\n"
    "                              \tInstrumentation file output directory\n",
    {driverCategory, cgCategory}, kOptMaple);

maplecl::Option<std::string> litePgoWhiteList ({"--lite-pgo-white-list"},
    "  --lite-pgo-white-list=filepath\n"
    "                              \tInstrumentation function white list\n",
    {driverCategory, cgCategory}, kOptMaple);

maplecl::Option<std::string> litePgoOutputFunc ({"--lite-pgo-output-func"},
    "  --lite-pgo-output-func=function name\n"
    "                              \tGenerate lite profile at the exit of the output "
    "function[default none]\n",
    {driverCategory, cgCategory}, kOptMaple);

maplecl::Option<std::string> litePgoFile({"--lite-pgo-file"},
    "  --lite-pgo-file=filepath    \tLite pgo guide file\n",
    {driverCategory, cgCategory}, kOptMaple);

maplecl::Option<std::string> functionPriority({"--function-priority"},
    "  --function-priority=filepath \tWhen profile data is given, priority suffix is added to section "
    "name in order to improve code locality\n",
    {cgCategory});

maplecl::Option<bool> litePgoVerify({"--lite-pgo-verify"},
    "  --lite-pgo-verify           \tverify lite-pgo data strictly, abort when encountering mismatch "
    "data(default:skip).\n"
    "  --no-lite-pgo-verify\n",
    {driverCategory, cgCategory}, kOptMaple,  maplecl::DisableWith("--no-lite-pgo-verify"));

maplecl::Option<bool> optimizedFrameLayout({"--optimized-frame-layout"},
    " --optimized-frame-layout     \tEnable optimized framelayout, put small local variables near sp, put "
    "callee save region near sp\n",
    {cgCategory}, maplecl::DisableWith("--no-optimized-frame-layout"));

maplecl::Option<bool> loopAlign({"--loop-align"},
    "  --loop-align                \tPerform add nop for loop instead of insert .p2align\n"
    "  --no-loop-align        \n",
    {cgCategory}, maplecl::DisableWith("--no-loop-align"));

maplecl::Option<bool> pgoCodeAlign({"--pgo-code-align"},
    " --pgo-code-align             \tuse the bb's frequency generated by pgo"
    " to do the alignment analysis\n",
    {cgCategory});

maplecl::Option<uint32_t> alignThreshold({"--align-threshold"},
    " --align-threshold=NUM(1, 100)\talign thresold, default 100",
    {cgCategory});

maplecl::Option<uint32_t> alignLoopIterations({"--align-loop-Iterations"},
    " --align-loop-Iterations=NUM(1, 4) \tdefault 4",
    {cgCategory});

maplecl::Option<uint32_t> dupFreqThreshold({"--dup-threshold"},
    " --dup-threshold=NUM(1, 100)  \tdup thresold, default 100",
    {cgCategory});
} // namespace opts::cg
