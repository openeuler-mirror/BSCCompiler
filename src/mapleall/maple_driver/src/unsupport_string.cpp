/*
 * Copyright (c) [2023] Huawei Technologies Co.,Ltd.All rights reserved.
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

namespace opts {

maplecl::Option<bool> fpic({"-fpic", "--fpic"},
    "  --fpic                      \tGenerate position-independent shared library in small mode.\n"
    "  --no-pic/-fno-pic           \n",
    {cgCategory, driverCategory, ldCategory}, kOptCommon | kOptLd | kOptNotFiltering,
    maplecl::DisableEvery({"-fno-pic", "--no-pic"}));

/* ##################### STRING Options ############################################################### */

maplecl::Option<std::string> foffloadOptions({"-foffload-options="},
    "  -foffload-options=          \t-foffload-options=<targets>=<options> Specify options for the offloading targets."
    " options or targets=options missing after %qs\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<std::string> linkerTimeOptE({"-flto="},
    "  -flto=<value>               \tSet LTO mode to either 'full' or 'thin'.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<std::string> fStrongEvalOrderE({"-fstrong-eval-order="},
    "  -fstrong-eval-order         \tFollow the C++17 evaluation order requirementsfor assignment expressions, "
    "shift, member function calls, etc.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<std::string> oFmaxErrors({"-fmax-errors"},
    "  -fmax-errors               \tLimits the maximum number of error messages to n, If n is 0 (the default), "
    "there is no limit on the number of error messages produced. If -Wfatal-errors is also specified, then "
    "-Wfatal-errors takes precedence over this option.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<std::string> oMalignData({"-malign-data"},
    "  -malign-data                \tControl how GCC aligns variables. \n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<std::string> oMatomicModel({"-matomic-model"},
    "  -matomic-model              \tSets the model of atomic operations and additional parameters as a comma "
    "separated list. \n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<std::string> oMaxVectAlign({"-max-vect-align"},
    "  -max-vect-align             \tThe maximum alignment for SIMD vector mode types.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<std::string> oMbased({"-mbased"},
    "  -mbased                     \tVariables of size n bytes or smaller are placed in the .based "
    "section by default\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<std::string> oMblockMoveInlineLimit({"-mblock-move-inline-limit"},
    "  -mblock-move-inline-limit   \tInline all block moves (such as calls to memcpy or structure copies) "
    "less than or equal to num bytes. \n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<std::string> oMbranchCost({"-mbranch-cost"},
    "  -mbranch-cost               \tSet the cost of branches to roughly num 'simple' instructions.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<std::string> oMc({"-mc"},
    "  -mc                         \tSelects which section constant data is placed in. name may be 'tiny', "
    "'near', or 'far'.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<std::string> oMcacheBlockSize({"-mcache-block-size"},
    "  -mcache-block-size          \tSpecify the size of each cache block, which must be a power of 2 "
    "between 4 and 512.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<std::string> oMcacheSize({"-mcache-size"},
    "  -mcache-size                \tThis option controls the version of libgcc that the compiler links to an "
    "executable and selects a software-managed cache for accessing variables in the __ea address space with a "
    "particular cache size.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<std::string> oMcmodel({"-mcmodel"},
    "  -mcmodel                    \tSpecify the code model.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<std::string> oMcodeReadable({"-mcode-readable"},
    "  -mcode-readable             \tSpecify whether Maple may generate code that reads from executable sections.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<std::string> oMconfig({"-mconfig"},
    "  -mconfig                    \tSelects one of the built-in core configurations. \n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<std::string> oMcpu({"-mcpu"},
    "  -mcpu                       \tSpecify the name of the target processor, optionally suffixed by "
    "one or more feature modifiers.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<std::string> oMdataRegion({"-mdata-region"},
    "  -mdata-region               \ttell the compiler where to place functions and data that do not have one "
    "of the lower, upper, either or section attributes.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<std::string> oMdivsi3_libfuncName({"-mdivsi3_libfunc"},
    "  -mdivsi3_libfunc            \tSet the name of the library function used for 32-bit signed division to name.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<std::string> oMdualNopsE({"-mdual-nops="},
    "  -mdual-nops=                \tBy default, GCC inserts NOPs to increase dual issue when it expects it "
    "to increase performance.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<std::string> oMemregsE({"-memregs="},
    "  -memregs=                   \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<std::string> oMfixedRange({"-mfixed-range"},
    "  -mfixed-range               \tGenerate code treating the given register range as fixed registers.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<std::string> oMfloatGprs({"-mfloat-gprs"},
    "  -mfloat-gprs                \tThis switch enables the generation of floating-point operations on the "
    "general-purpose registers for architectures that support it.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<std::string> oMflushFunc({"-mflush-func"},
    "  -mflush-func                \tSpecifies the function to call to flush the I and D caches, or to not "
    "call any such function. \n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-flush-func"), maplecl::kHide);

maplecl::Option<std::string> oMflushTrap({"-mflush-trap"},
    "  -mflush-trap                \t\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-flush-trap"), maplecl::kHide);

maplecl::Option<std::string> oMfpRoundingMode({"-mfp-rounding-mode"},
    "  -mfp-rounding-mode          \tSelects the IEEE rounding mode.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<std::string> oMfpTrapMode({"-mfp-trap-mode"},
    "  -mfp-trap-mode              \tThis option controls what floating-point related traps are enabled.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<std::string> oMfpu({"-mfpu"},
    "  -mfpu                       \tEnables support for specific floating-point hardware extensions for "
    "ARCv2 cores.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-fpu"), maplecl::kHide);

maplecl::Option<std::string> oMhintMaxDistance({"-mhint-max-distance"},
    "  -mhint-max-distance         \tThe encoding of the branch hint instruction limits the hint to be within 256 "
    "instructions of the branch it is affecting.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<std::string> oMhintMaxNops({"-mhint-max-nops"},
    "  -mhint-max-nops             \tMaximum number of NOPs to insert for a branch hint.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<std::string> oMhotpatch({"-mhotpatch"},
    "  -mhotpatch                  \tIf the hotpatch option is enabled, a “hot-patching” function prologue is "
    "generated for all functions in the compilation unit. \n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<std::string> oMhwmultE({"-mhwmult="},
    "  -mhwmult=                   \tDescribes the type of hardware multiply supported by the target.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<std::string> oMinsertSchedNops({"-minsert-sched-nops"},
    "  -minsert-sched-nops         \tThis option controls which NOP insertion scheme is used during the second "
    "scheduling pass.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<std::string> oMiselE({"-misel="},
    "  -misel=                     \tThis switch has been deprecated. Use -misel and -mno-isel instead.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<std::string> oMisel({"-misel"},
    "  -misel                      \tThis switch enables or disables the generation of ISEL instructions.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-isel"), maplecl::kHide);

maplecl::Option<std::string> oMisrVectorSize({"-misr-vector-size"},
    "  -misr-vector-size           \tSpecify the size of each interrupt vector, which must be 4 or 16.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<std::string> oMmcuE({"-mmcu="},
    "  -mmcu=                      \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<std::string> oMmemoryLatency({"-mmemory-latency"},
    "  -mmemory-latency            \tSets the latency the scheduler should assume for typical memory references "
    "as seen by the application.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<std::string> oMmemoryModel({"-mmemory-model"},
    "  -mmemory-model              \tSet the memory model in force on the processor to one of.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<std::string> oMoverrideE({"-moverride="},
    "  -moverride=                 \tPower users only! Override CPU optimization parameters.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<std::string> oMprioritizeRestrictedInsns({"-mprioritize-restricted-insns"},
    "  -mprioritize-restricted-insns  \tThis option controls the priority that is assigned to dispatch-slot "
    "restricted instructions during the second scheduling pass. \n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<std::string> oMrecip({"-mrecip"},
    "  -mrecip                     \tThis option enables use of the reciprocal estimate and reciprocal square "
    "root estimate instructions with additional Newton-Raphson steps to increase precision instead of doing a "
    "divide or square root and divide for floating-point arguments.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<std::string> oMrecipE({"-mrecip="},
    "  -mrecip=                    \tThis option controls which reciprocal estimate instructions may be used.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<std::string> oMschedCostlyDep({"-msched-costly-dep"},
    "  -msched-costly-dep          \tThis option controls which dependences are considered costly by the target "
    "during instruction scheduling.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<std::string> oMschedule({"-mschedule"},
    "  -mschedule                  \tSchedule code according to the constraints for the machine type cpu-type. \n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<std::string> oMsda({"-msda"},
    "  -msda                       \tPut static or global variables whose size is n bytes or less into the small "
    "data area that register gp points to.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<std::string> oMsharedLibraryId({"-mshared-library-id"},
    "  -mshared-library-id         \tSpecifies the identification number of the ID-based shared library being "
    "compiled.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<std::string> oMsignReturnAddress({"-msign-return-address"},
    "  -msign-return-address       \tSelect return address signing scope.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<std::string> oMsiliconErrata({"-msilicon-errata"},
    "  -msilicon-errata            \tThis option passes on a request to assembler to enable the fixes for the "
    "named silicon errata.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<std::string> oMsiliconErrataWarn({"-msilicon-errata-warn"},
    "  -msilicon-errata-warn       \tThis option passes on a request to the assembler to enable warning messages "
    "when a silicon errata might need to be applied.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<std::string> oMsizeLevel({"-msize-level"},
    "  -msize-level                \tFine-tune size optimization with regards to instruction lengths and alignment.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<std::string> oMspe({"-mspe="},
    "  -mspe=                      \tThis option has been deprecated. Use -mspe and -mno-spe instead.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<std::string> oMstackGuard({"-mstack-guard"},
    "  -mstack-guard               \tThe S/390 back end emits additional instructions in the function prologue that "
    "trigger a trap if the stack size is stack-guard bytes above the stack-size (remember that the stack on S/390 "
    "grows downward). \n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<std::string> oMstackProtectorGuard({"-mstack-protector-guard"},
    "  -mstack-protector-guard     \tGenerate stack protection code using canary at guard.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<std::string> oMstackProtectorGuardOffset({"-mstack-protector-guard-offset"},
    "  -mstack-protector-guard-offset  \tWith the latter choice the options -mstack-protector-guard-reg=reg and "
    "-mstack-protector-guard-offset=offset furthermore specify which register to use as base register for reading "
    "the canary, and from what offset from that base register. \n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<std::string> oMstackProtectorGuardReg({"-mstack-protector-guard-reg"},
    "  -mstack-protector-guard-reg \tWith the latter choice the options -mstack-protector-guard-reg=reg and "
    "-mstack-protector-guard-offset=offset furthermore specify which register to use as base register for reading "
    "the canary, and from what offset from that base register. \n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<std::string> oMstackSize({"-mstack-size"},
    "  -mstack-size                \tThe S/390 back end emits additional instructions in the function prologue that"
    " trigger a trap if the stack size is stack-guard bytes above the stack-size (remember that the stack on S/390 "
    "grows downward). \n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<std::string> oMtda({"-mtda"},
    "  -mtda                       \tPut static or global variables whose size is n bytes or less into the tiny "
    "data area that register ep points to.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<std::string> oMtiny({"-mtiny"},
    "  -mtiny                      \tVariables that are n bytes or smaller are allocated to the .tiny section.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<std::string> oMveclibabi({"-mveclibabi"},
    "  -mveclibabi                 \tSpecifies the ABI type to use for vectorizing intrinsics using an external "
    "library. \n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<std::string> oMunix({"-munix"},
    "  -munix                      \tGenerate compiler predefines and select a startfile for the specified "
    "UNIX standard. \n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<std::string> oMtrapPrecision({"-mtrap-precision"},
    "  -mtrap-precision            \tIn the Alpha architecture, floating-point traps are imprecise.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<std::string> oMtune({"-mtune="},
    "  -mtune=                     \tOptimize for CPU. \n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<std::string> oMultcost({"-multcost"},
    "  -multcost                   \tReplaced by -mmultcost.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<std::string> oMtp({"-mtp"},
    "  -mtp                        \tSpecify the access model for the thread local storage pointer. \n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<std::string> oMtpRegno({"-mtp-regno"},
    "  -mtp-regno                  \tSpecify thread pointer register number.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<std::string> oMtlsDialect({"-mtls-dialect"},
    "  -mtls-dialect               \tSpecify TLS dialect.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<std::string> oIplugindir({"-iplugindir="},
    "  -iplugindir=                \t-iplugindir=<dir> o Set <dir> oto be the default plugin directory.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<std::string> oIprefix({"-iprefix"},
    "  -iprefix                    \t-iprefix <path> o       Specify <path> oas a prefix for next two options.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<std::string> oIquote({"-iquote"},
    "  -iquote                     \t-iquote <dir> o  Add <dir> oto the end of the quote include path.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<std::string> oIsysroot({"-isysroot"},
    "  -isysroot                   \t-isysroot <dir> o      Set <dir> oto be the system root directory.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<std::string> oIwithprefix({"-iwithprefix"},
    "  -iwithprefix                \t-iwithprefix <dir> oAdd <dir> oto the end of the system include path.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<std::string> oIwithprefixbefore({"-iwithprefixbefore"},
    "  -iwithprefixbefore          \t-iwithprefixbefore <dir> o    Add <dir> oto the end ofthe main include path.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<std::string> oImultilib({"-imultilib"},
    "  -imultilib                  \t-imultilib <dir> o    Set <dir> oto be the multilib include subdirectory.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<std::string> oInclude({"-include"},
    "  -include                    \t-include <file> o       Include the contents of <file> obefore other files.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<std::string> oIframework({"-iframework"},
    "  -iframework                 \tLike -F except the directory is a treated as a system directory. \n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<std::string> oG({"-G"},
    "  -G                          \tOn embedded PowerPC systems, put global and static items less than or equal "
    "to num bytes into the small data or BSS sections instead of the normal data or BSS section. \n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<std::string> oYm({"-Ym"},
    "  -Ym                         \tLook in the directory dir to find the M4 preprocessor. \n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<std::string> oYP({"-YP"},
    "  -YP                         \tSearch the directories dirs, and no others, for libraries specified with -l.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<std::string> oXassembler({"-Xassembler"},
    "  -Xassembler                 \tPass option as an option to the assembler. \n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<std::string> oWa({"-Wa"},
    "  -Wa                         \tPass option as an option to the assembler. If option contains commas, it is "
    "split into multiple options at the commas.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<std::string> oTime({"-time"},
    "  -time                       \tReport the CPU time taken by each subprocess in the compilation sequence. \n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<std::string> oMzda({"-mzda"},
    "  -mzda                       \tPut static or global variables whose size is n bytes or less into the first 32"
    " kilobytes of memory.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<std::string> oFwideExecCharset({"-fwide-exec-charset="},
    "  -fwide-exec-charset=        \tConvert all wide strings and character constants to character set <cset>.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<std::string> oMwarnFramesize({"-mwarn-framesize"},
    "  -mwarn-framesize            \tEmit a warning if the current function exceeds the given frame size.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<std::string> oFtrackMacroExpansionE({"-ftrack-macro-expansion="},
    "  -ftrack-macro-expansion=    \tTrack locations of tokens across macro expansions.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<std::string> oFtemplateBacktraceLimit({"-ftemplate-backtrace-limit="},
    "  -ftemplate-backtrace-limit= \tSet the maximum number of template instantiation notes for a single warning "
    "or error to n.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<std::string> oFtemplateDepth({"-ftemplate-depth-"},
    "  -ftemplate-depth-           \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<std::string> oFtemplateDepthE({"-ftemplate-depth="},
    "  -ftemplate-depth=           \tSpecify maximum template instantiation depth.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<std::string> oFstack_reuse({"-fstack-reuse="},
    "  -fstack_reuse=              \tSet stack reuse level for local variables.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<std::string> oFstackCheckE({"-fstack-check="},
    "  -fstack-check=              \t-fstack-check=[no|generic|specific]	Insert stack checking code into "
    "the program.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<std::string> oFstackLimitRegister({"-fstack-limit-register="},
    "  -fstack-limit-register=     \tTrap if the stack goes past <register>\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<std::string> oFstackLimitSymbol({"-fstack-limit-symbol="},
    "  -fstack-limit-symbol=       \tTrap if the stack goes past symbol <name>.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<std::string> oFsanitize({"-fsanitize"},
    "  -fsanitize                  \tSelect what to sanitize.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-sanitize"), maplecl::kHide);

maplecl::Option<std::string> oFsanitizeRecoverE({"-fsanitize-recover="},
    "  -fsanitize-recover=         \tAfter diagnosing undefined behavior attempt to continue execution.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<std::string> oFsanitizeSections({"-fsanitize-sections="},
    "  -fsanitize-sections=        \tSanitize global variables in user-defined sections.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<std::string> oFrandomSeedE({"-frandom-seed="},
    "  -frandom-seed=              \tMake compile reproducible using <string>.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<std::string> oFreorderBlocksAlgorithm({"-freorder-blocks-algorithm="},
    "  -freorder-blocks-algorithm= \tSet the used basic block reordering algorithm.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<std::string> oFprofileUseE({"-fprofile-use="},
    "  -fprofile-use=              \tEnable common options for performing profile feedback directed optimizations, "
    "and set -fprofile-dir=.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<std::string> oFprofileDir({"-fprofile-dir="},
    "  -fprofile-dir=              \tSet the top-level directory for storing the profile data. The default is 'pwd'.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<std::string> oFprofileUpdate({"-fprofile-update="},
    "  -fprofile-update=           \tSet the profile update method.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<std::string> oFplugin({"-fplugin="},
    "  -fplugin=                   \tSpecify a plugin to load.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<std::string> oFpluginArg({"-fplugin-arg-"},
    "  -fplugin-arg-               \tSpecify argument <key>=<value> ofor plugin <name>.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<std::string> oFpermittedFltEvalMethods({"-fpermitted-flt-eval-methods="},
    "  -fpermitted-flt-eval-methods=  \tSpecify which values of FLT_EVAL_METHOD are permitted.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<std::string> oFobjcAbiVersion({"-fobjc-abi-version="},
    "  -fobjc-abi-version=         \tSpecify which ABI to use for Objective-C family code and meta-data generation.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<std::string> oFoffloadAbi({"-foffload-abi="},
    "  -foffload-abi=              \tSet the ABI to use in an offload compiler.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<std::string> oFoffload({"-foffload="},
    "  -foffload=                  \tSpecify offloading targets and options for them.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<std::string> oFopenaccDim({"-fopenacc-dim="},
    "  -fopenacc-dim=              \tSpecify default OpenACC compute dimensions.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<std::string> oFCheckingE({"-fchecking="},
    "  -fchecking=                 \tPerform internal consistency checkings.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<std::string> oFmessageLength({"-fmessage-length="},
    "  -fmessage-length=           \t-fmessage-length=<number> o    Limit diagnostics to <number> ocharacters per "
    "line.  0 suppresses line-wrapping.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<std::string> oFltoPartition({"-flto-partition="},
    "  -flto-partition=            \tSpecify the algorithm to partition symbols and vars at linktime.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<std::string> oFltoCompressionLevel({"-flto-compression-level="},
    "  -flto-compression-level=    \tUse zlib compression level <number> ofor IL.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<std::string> oFivarVisibility({"-fivar-visibility="},
    "  -fivar-visibility=          \tSet the default symbol visibility.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<std::string> oFliveRangeShrinkage({"-flive-range-shrinkage"},
    "  -flive-range-shrinkage      \tRelief of register pressure through live range shrinkage\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-live-range-shrinkage"), maplecl::kHide);

maplecl::Option<std::string> oFiraRegion({"-fira-region="},
    "  -fira-region=               \tSet regions for IRA.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<std::string> oFiraVerbose({"-fira-verbose="},
    "  -fira-verbose=              \tControl IRA's level of diagnostic messages.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<std::string> oFiraAlgorithmE({"-fira-algorithm="},
    "  -fira-algorithm=            \tSet the used IRA algorithm.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<std::string> oFinstrumentFunctionsExcludeFileList({"-finstrument-functions-exclude-file-list="},
    "  -finstrument-functions-exclude-file-list=  \tDo not instrument functions listed in files.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<std::string> oFinstrumentFunctionsExcludeFunctionList({"-finstrument-functions-exclude-function-list="},
    "  -finstrument-functions-exclude-function-list=  \tDo not instrument listed functions.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<std::string> oFinlineLimit({"-finline-limit-"},
    "  -finline-limit-             \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<std::string> oFinlineLimitE({"-finline-limit="},
    "  -finline-limit=             \tLimit the size of inlined functions to <number>.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<std::string> oFinlineMatmulLimitE({"-finline-matmul-limit="},
    "  -finline-matmul-limit=      \tecify the size of the largest matrix for which matmul will be inlined.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<std::string> oFfpContract ({"-ffp-contract="},
    "  -ffp-contract=              \tPerform floating-point expression contraction.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<std::string> oFfixed({"-ffixed-"},
    "  -ffixed-                    \t-ffixed-<register>	Mark <register> oas being unavailable to the compiler.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<std::string> oFexcessPrecision({"-fexcess-precision="},
    "  -fexcess-precision=         \tSpecify handling of excess floating-point precision.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<std::string> oFenable({"-fenable-"},
    "  -fenable-                   \t-fenable-[tree|rtl|ipa]-<pass>=range1+range2 enables an optimization pass.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<std::string> oFemitStructDebugDetailedE({"-femit-struct-debug-detailed="},
    "  -femit-struct-debug-detailed  \t-femit-struct-debug-detailed=<spec-list> oDetailed reduced debug info for "
    "structs.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<std::string> oFdumpRtlPass({"-fdump-rtl-pass"},
    "  -fdump-rtl-pass             \tSays to make debugging dumps during compilation at times specified by letters.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<std::string> oFdumpFinalInsns({"-fdump-final-insns"},
    "  -fdump-final-insns          \tDump the final internal representation (RTL) to file.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<std::string> oFdumpGoSpec({"-fdump-go-spec="},
    "  -fdump-go-spec              \tWrite all declarations to file as Go code.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<std::string> oFdisable({"-fdisable-"},
    "  -fdisable-                  \t-fdisable-[tree|rtl|ipa]-<pass>=range1+range2 disables an optimization pass.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<std::string> oFdiagnosticsShowLocation({"-fdiagnostics-show-location"},
    "  -fdiagnostics-show-location \tHow often to emit source location at the beginning of line-wrapped diagnostics.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<std::string> oFcompareDebugE({"-fcompare-debug="},
    "  -fcompare-debug=            \tCompile with and without e.g. -gtoggle, and compare the final-insns dump.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<std::string> oFconstantStringClass({"-fconstant-string-class="},
    "  -fconstant-string-class=    \tUse class <name> ofor constant strings. No class name specified with %qs\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<std::string> oFconstexprDepth({"-fconstexpr-depth="},
    "  -fconstexpr-depth=          \tpecify maximum constexpr recursion depth.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<std::string> oFconstexprLoopLimit({"-fconstexpr-loop-limit="},
    "  -fconstexpr-loop-limit=     \tSpecify maximum constexpr loop iteration count.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<std::string> oFabiCompatVersion({"-fabi-compat-version="},
    "  -fabi-compat-version=       \tThe version of the C++ ABI in use.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<std::string> oFabiVersion({"-fabi-version="},
    "  -fabi-version=              \tUse version n of the C++ ABI. The default is version 0.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<std::string> oFadaSpecParent({"-fada-spec-parent="},
    "  -fada-spec-parent=          \tIn conjunction with -fdump-ada-spec[-slim] above, generate Ada specs as "
    "child units of parent unit.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<std::string> oBundle_loader({"-bundle_loader"},
    "  -bundle_loader              \tThis option specifies the executable that will load the build output file "
    "being linked.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

/* ##################### BOOL Options ############################################################### */

maplecl::Option<bool> oFipaBitCp({"-fipa-bit-cp"},
    "  -fipa-bit-cp                \tWhen enabled, perform interprocedural bitwise constant propagation. This "
    "flag is enabled by default at -O2. It requires that -fipa-cp is enabled.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oFipaVrp({"-fipa-vrp"},
    "  -fipa-vrp                   \tWhen enabled, perform interprocedural propagation of value ranges. This "
    "flag is enabled by default at -O2. It requires that -fipa-cp is enabled.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMindirectBranchRegister({"-mindirect-branch-register"},
    "  -mindirect-branch-register  \tForce indirect call and jump via register.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMlowPrecisionDiv({"-mlow-precision-div"},
    "  -mlow-precision-div         \tEnable the division approximation. Enabling this reduces precision of "
    "division results to about 16 bits for single precision and to 32 bits for double precision.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-mno-low-precision-div"), maplecl::kHide);

maplecl::Option<bool> oMlowPrecisionSqrt({"-mlow-precision-sqrt"},
    "  -mlow-precision-sqrt        \tEnable the reciprocal square root approximation. Enabling this reduces precision"
    " of reciprocal square root results to about 16 bits for single precision and to 32 bits for double precision.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-mno-low-precision-sqrt"), maplecl::kHide);

maplecl::Option<bool> oM80387({"-m80387"},
    "  -m80387                     \tGenerate output containing 80387 instructions for floating point.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-mno-80387"), maplecl::kHide);

maplecl::Option<bool> oAllowable_client({"-allowable_client"},
    "  -allowable_client           \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oAll_load({"-all_load"},
    "  -all_load                   \tLoads all members of static archive libraries.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oArch_errors_fatal({"-arch_errors_fatal"},
    "  -arch_errors_fatal          \tCause the errors having to do with files that have the wrong architecture "
    "to be fatal.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oAuxInfo({"-aux-info"},
    "  -aux-info                   \tEmit declaration information into <file>.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oBdynamic({"-Bdynamic"},
    "  -Bdynamic                   \tDefined for compatibility with Diab.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oBind_at_load({"-bind_at_load"},
    "  -bind_at_load               \tCauses the output file to be marked such that the dynamic linker will bind "
    "all undefined references when the file is loaded or launched.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oBstatic({"-Bstatic"},
    "  -Bstatic                    \tdefined for compatibility with Diab.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oBundle({"-bundle"},
    "  -bundle                     \tProduce a Mach-o bundle format file. \n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oC({"-C"},
    "  -C                          \tDo not discard comments.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oCC({"-CC"},
    "  -CC                         \tDo not discard comments in macro expansions.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oClient_name({"-client_name"},
    "  -client_name                \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oCompatibility_version({"-compatibility_version"},
    "  -compatibility_version      \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oCoverage({"-coverage"},
    "  -coverage                   \tThe option is a synonym for -fprofile-arcs -ftest-coverage (when compiling) "
    "and -lgcov (when linking). \n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oCurrent_version({"-current_version"},
    "  -current_version            \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oDa({"-da"},
    "  -da                         \tProduce all the dumps listed above.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oDA({"-dA"},
    "  -dA                         \tAnnotate the assembler output with miscellaneous debugging information.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oDD({"-dD"},
    "  -dD                         \tDump all macro definitions, at the end of preprocessing, in addition to "
    "normal output.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oDead_strip({"-dead_strip"},
    "  -dead_strip                 \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oDependencyFile({"-dependency-file"},
    "  -dependency-file            \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oDH({"-dH"},
    "  -dH                         \tProduce a core dump whenever an error occurs.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oDp({"-dp"},
    "  -dp                         \tAnnotate the assembler output with a comment indicating which pattern and "
    "alternative is used. The length of each instruction is also printed.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oDP({"-dP"},
    "  -dP                         \tDump the RTL in the assembler output as a comment before each instruction. "
    "Also turns on -dp annotation.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oDumpfullversion({"-dumpfullversion"},
    "  -dumpfullversion            \tPrint the full compiler version, always 3 numbers separated by dots, major,"
    " minor and patchlevel version.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oDumpmachine({"-dumpmachine"},
    "  -dumpmachine                \tPrint the compiler's target machine (for example, 'i686-pc-linux-gnu') and "
    "don't do anything else.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oDumpspecs({"-dumpspecs"},
    "  -dumpspecs                  \tPrint the compiler's built-in specs—and don't do anything else. (This is "
    "used when MAPLE itself is being built.)\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oDx({"-dx"},
    "  -dx                         \tJust generate RTL for a function instead of compiling it. Usually used "
    "with -fdump-rtl-expand.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oDylib_file({"-dylib_file"},
    "  -dylib_file                 \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oDylinker_install_name({"-dylinker_install_name"},
    "  -dylinker_install_name      \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oDynamic({"-dynamic"},
    "  -dynamic                    \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oDynamiclib({"-dynamiclib"},
    "  -dynamiclib                 \tWhen passed this option, GCC produces a dynamic library instead of an "
    "executable when linking, using the Darwin libtool command.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oEB({"-EB"},
    "  -EB                         \tCompile code for big-endian targets.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oEL({"-EL"},
    "  -EL                         \tCompile code for little-endian targets. \n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oExported_symbols_list({"-exported_symbols_list"},
    "  -exported_symbols_list      \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oFaggressiveLoopOptimizations({"-faggressive-loop-optimizations"},
    "  -faggressive-loop-optimizations  \tThis option tells the loop optimizer to use language constraints to "
    "derive bounds for the number of iterations of a loop.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-aggressive-loop-optimizations"), maplecl::kHide);

maplecl::Option<bool> oFchkpFlexibleStructTrailingArrays({"-fchkp-flexible-struct-trailing-arrays"},
    "  -fchkp-flexible-struct-trailing-arrays  \tForces Pointer Bounds Checker to treat all trailing arrays in "
    "structures as possibly flexible.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-chkp-flexible-struct-trailing-arrays"), maplecl::kHide);

maplecl::Option<bool> oFchkpInstrumentCalls({"-fchkp-instrument-calls"},
    "  -fchkp-instrument-calls     \tInstructs Pointer Bounds Checker to pass pointer bounds to calls."
    " Enabled by default.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-chkp-instrument-calls"), maplecl::kHide);

maplecl::Option<bool> oFchkpInstrumentMarkedOnly({"-fchkp-instrument-marked-only"},
    "  -fchkp-instrument-marked-only  \tInstructs Pointer Bounds Checker to instrument only functions marked with "
    "the bnd_instrument attribute \n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-chkp-instrument-marked-only"), maplecl::kHide);

maplecl::Option<bool> oFchkpNarrowBounds({"-fchkp-narrow-bounds"},
    "  -fchkp-narrow-bounds        \tControls bounds used by Pointer Bounds Checker for pointers to object fields.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-chkp-narrow-bounds"), maplecl::kHide);

maplecl::Option<bool> oFchkpNarrowToInnermostArray({"-fchkp-narrow-to-innermost-array"},
    "  -fchkp-narrow-to-innermost-array  \tForces Pointer Bounds Checker to use bounds of the innermost arrays in "
    "case of nested static array access.\n",
    {driverCategory, unSupCategory},  maplecl::DisableWith("-fno-chkp-narrow-to-innermost-array"), maplecl::kHide);

maplecl::Option<bool> oFchkpOptimize({"-fchkp-optimize"},
    "  -fchkp-optimize             \tEnables Pointer Bounds Checker optimizations.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-chkp-optimize"), maplecl::kHide);

maplecl::Option<bool> oFchkpStoreBounds({"-fchkp-store-bounds"},
    "  -fchkp-store-bounds         tInstructs Pointer Bounds Checker to generate bounds tores for pointer writes.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-chkp-store-bounds"), maplecl::kHide);

maplecl::Option<bool> oFchkpTreatZeroDynamicSizeAsInfinite({"-fchkp-treat-zero-dynamic-size-as-infinite"},
    "  -fchkp-treat-zero-dynamic-size-as-infinite  \tWith this option, objects with incomplete type whose "
    "dynamically-obtained size is zero are treated as having infinite size instead by Pointer Bounds Checker. \n",
    {driverCategory, unSupCategory},
    maplecl::DisableWith("-fno-chkp-treat-zero-dynamic-size-as-infinite"), maplecl::kHide);

maplecl::Option<bool> oFchkpUseFastStringFunctions({"-fchkp-use-fast-string-functions"},
    "  -fchkp-use-fast-string-functions  \tEnables use of *_nobnd versions of string functions (not copying bounds) "
    "by Pointer Bounds Checker. \n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-chkp-use-fast-string-functions"), maplecl::kHide);

maplecl::Option<bool> oFchkpUseNochkStringFunctions({"-fchkp-use-nochk-string-functions"},
    "  -fchkp-use-nochk-string-functions  \tEnables use of *_nochk versions of string functions (not checking bounds) "
    "by Pointer Bounds Checker. \n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-chkp-use-nochk-string-functions"), maplecl::kHide);

maplecl::Option<bool> oFchkpUseStaticBounds({"-fchkp-use-static-bounds"},
    "  -fchkp-use-static-bounds    \tAllow Pointer Bounds Checker to generate static bounds holding bounds of "
    "static variables. \n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-chkp-use-static-bounds"), maplecl::kHide);

maplecl::Option<bool> oFchkpUseStaticConstBounds({"-fchkp-use-static-const-bounds"},
    "  -fchkp-use-static-const-bounds  \tUse statically-initialized bounds for constant bounds instead of generating"
    " them each time they are required.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-chkp-use-static-const-bounds"), maplecl::kHide);

maplecl::Option<bool> oFchkpUseWrappers({"-fchkp-use-wrappers"},
    "  -fchkp-use-wrappers         \tAllows Pointer Bounds Checker to replace calls to built-in functions with calls"
    " to wrapper functions.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-chkp-use-wrappers"), maplecl::kHide);

maplecl::Option<bool> oFcilkplus({"-fcilkplus"},
    "  -fcilkplus                  \tEnable Cilk Plus.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-cilkplus"), maplecl::kHide);

maplecl::Option<bool> oFcodeHoisting({"-fcode-hoisting"},
    "  -fcode-hoisting             \tPerform code hoisting. \n",
    {driverCategory, unSupCategory},  maplecl::DisableWith("-fno-code-hoisting"), maplecl::kHide);

maplecl::Option<bool> oFcombineStackAdjustments({"-fcombine-stack-adjustments"},
    "  -fcombine-stack-adjustments \tTracks stack adjustments (pushes and pops) and stack memory references and "
    "then tries to find ways to combine them.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-combine-stack-adjustments"), maplecl::kHide);

maplecl::Option<bool> oFcompareDebug({"-fcompare-debug"},
    "  -fcompare-debug             \tCompile with and without e.g. -gtoggle, and compare the final-insns dump.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-compare-debug"), maplecl::kHide);

maplecl::Option<bool> oFcompareDebugSecond({"-fcompare-debug-second"},
    "  -fcompare-debug-second      tRun only the second compilation of -fcompare-debug.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oFcompareElim({"-fcompare-elim"},
    "  -fcompare-elim              \tPerform comparison elimination after register allocation has finished.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-compare-elim"), maplecl::kHide);

maplecl::Option<bool> oFconcepts({"-fconcepts"},
    "  -fconcepts                  \tEnable support for C++ concepts.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-concepts"), maplecl::kHide);

maplecl::Option<bool> oFcondMismatch({"-fcond-mismatch"},
    "  -fcond-mismatch             \tAllow the arguments of the '?' operator to have different types.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-cond-mismatch"), maplecl::kHide);

maplecl::Option<bool> oFconserveStack({"-fconserve-stack"},
    "  -fconserve-stack            \tDo not perform optimizations increasing noticeably stack usage.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-conserve-stack"), maplecl::kHide);

maplecl::Option<bool> oFcpropRegisters({"-fcprop-registers"},
    "  -fcprop-registers           \tPerform a register copy-propagation optimization pass.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-cprop-registers"), maplecl::kHide);

maplecl::Option<bool> oFcrossjumping({"-fcrossjumping"},
    "  -fcrossjumping              \tPerform cross-jumping transformation. This transformation unifies equivalent "
    "code and saves code size.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-crossjumping"), maplecl::kHide);

maplecl::Option<bool> oFcseFollowJumps({"-fcse-follow-jumps"},
    "  -fcse-follow-jumps          \tWhen running CSE, follow jumps to their targets.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-cse-follow-jumps"), maplecl::kHide);

maplecl::Option<bool> oFcseSkipBlocks({"-fcse-skip-blocks"},
    "  -fcse-skip-blocks           \tDoes nothing.  Preserved for backward compatibility.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-cse-skip-blocks"), maplecl::kHide);

maplecl::Option<bool> oFcxFortranRules({"-fcx-fortran-rules"},
    "  -fcx-fortran-rules          \tComplex multiplication and division follow Fortran rules.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-cx-fortran-rules"), maplecl::kHide);

maplecl::Option<bool> oFcxLimitedRange({"-fcx-limited-range"},
    "  -fcx-limited-range          \tOmit range reduction step when performing complex division.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-cx-limited-range"), maplecl::kHide);

maplecl::Option<bool> oFdbgCnt({"-fdbg-cnt"},
    "  -fdbg-cnt                   \tPlace data items into their own section.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-dbg-cnt"), maplecl::kHide);

maplecl::Option<bool> oFdbgCntList({"-fdbg-cnt-list"},
    "  -fdbg-cnt-list              \tList all available debugging counters with their limits and counts.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-dbg-cnt-list"), maplecl::kHide);

maplecl::Option<bool> oFdce({"-fdce"},
    "  -fdce                       \tUse the RTL dead code elimination pass.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-dce"), maplecl::kHide);

maplecl::Option<bool> oFdebugCpp({"-fdebug-cpp"},
    "  -fdebug-cpp                 \tEmit debug annotations during preprocessing.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-debug-cpp"), maplecl::kHide);

maplecl::Option<bool> oFdebugPrefixMap({"-fdebug-prefix-map"},
    "  -fdebug-prefix-map          \tMap one directory name to another in debug information.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-debug-prefix-map"), maplecl::kHide);

maplecl::Option<bool> oFdebugTypesSection({"-fdebug-types-section"},
    "  -fdebug-types-section       \tOutput .debug_types section when using DWARF v4 debuginfo.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-debug-types-section"), maplecl::kHide);

maplecl::Option<bool> oFdecloneCtorDtor({"-fdeclone-ctor-dtor"},
    "  -fdeclone-ctor-dtor         \tFactor complex constructors and destructors to favor space over speed.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-declone-ctor-dtor"), maplecl::kHide);

maplecl::Option<bool> oFdeduceInitList({"-fdeduce-init-list"},
    "  -fdeduce-init-list          \tenable deduction of std::initializer_list for a template type parameter "
    "from a brace-enclosed initializer-list.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-deduce-init-list"), maplecl::kHide);

maplecl::Option<bool> oFdelayedBranch({"-fdelayed-branch"},
    "  -fdelayed-branch            \tAttempt to fill delay slots of branch instructions.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-delayed-branch"), maplecl::kHide);

maplecl::Option<bool> oFdeleteDeadExceptions({"-fdelete-dead-exceptions"},
    "  -fdelete-dead-exceptions    \tDelete dead instructions that may throw exceptions.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-delete-dead-exceptions"), maplecl::kHide);

maplecl::Option<bool> oFdeleteNullPointerChecks({"-fdelete-null-pointer-checks"},
    "  -fdelete-null-pointer-checks  \tDelete useless null pointer checks.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-delete-null-pointer-checks"), maplecl::kHide);

maplecl::Option<bool> oFdevirtualize({"-fdevirtualize"},
    "  -fdevirtualize              \tTry to convert virtual calls to direct ones.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-devirtualize"), maplecl::kHide);

maplecl::Option<bool> oFdevirtualizeAtLtrans({"-fdevirtualize-at-ltrans"},
    "  -fdevirtualize-at-ltrans    \tStream extra data to support more aggressive devirtualization in LTO local "
    "transformation mode.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-devirtualize-at-ltrans"), maplecl::kHide);

maplecl::Option<bool> oFdevirtualizeSpeculatively({"-fdevirtualize-speculatively"},
    "  -fdevirtualize-speculatively  \tPerform speculative devirtualization.\n",
    {driverCategory, unSupCategory},  maplecl::DisableWith("-fno-devirtualize-speculatively"), maplecl::kHide);

maplecl::Option<bool> oFdiagnosticsGeneratePatch({"-fdiagnostics-generate-patch"},
    "  -fdiagnostics-generate-patch  \tPrint fix-it hints to stderr in unified diff format.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-diagnostics-generate-patch"), maplecl::kHide);

maplecl::Option<bool> oFdiagnosticsParseableFixits({"-fdiagnostics-parseable-fixits"},
    "  -fdiagnostics-parseable-fixits  \tPrint fixit hints in machine-readable form.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-diagnostics-parseable-fixits"), maplecl::kHide);

maplecl::Option<bool> oFdiagnosticsShowCaret({"-fdiagnostics-show-caret"},
    "  -fdiagnostics-show-caret    \tShow the source line with a caret indicating the column.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-diagnostics-show-caret"), maplecl::kHide);

maplecl::Option<bool> oFdiagnosticsShowOption({"-fdiagnostics-show-option"},
    "  -fdiagnostics-show-option   \tAmend appropriate diagnostic messages with the command line option that "
    "controls them.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--fno-diagnostics-show-option"), maplecl::kHide);

maplecl::Option<bool> oFdirectivesOnly({"-fdirectives-only"},
    "  -fdirectives-only           \tPreprocess directives only.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-fdirectives-only"), maplecl::kHide);

maplecl::Option<bool> oFdse({"-fdse"},
    "  -fdse                       \tUse the RTL dead store elimination pass.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-dse"), maplecl::kHide);

maplecl::Option<bool> oFdumpAdaSpec({"-fdump-ada-spec"},
    "  -fdump-ada-spec             \tWrite all declarations as Ada code transitively.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oFdumpClassHierarchy({"-fdump-class-hierarchy"},
    "  -fdump-class-hierarchy      \tC++ only.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-dump-class-hierarchy"), maplecl::kHide);

maplecl::Option<bool> oFdumpIpa({"-fdump-ipa"},
    "  -fdump-ipa                  \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oFdumpNoaddr({"-fdump-noaddr"},
    "  -fdump-noaddr               \tSuppress output of addresses in debugging dumps.\n",
    {driverCategory, unSupCategory},  maplecl::DisableWith("-fno-dump-noaddr"), maplecl::kHide);

maplecl::Option<bool> oFdumpPasses({"-fdump-passes"},
    "  -fdump-passes               \tDump optimization passes.\n",
    {driverCategory, unSupCategory},  maplecl::DisableWith("-fno-dump-passes"), maplecl::kHide);

maplecl::Option<bool> oFdumpRtlAlignments({"-fdump-rtl-alignments"},
    "  -fdump-rtl-alignments       \tDump after branch alignments have been computed.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oFdumpRtlAll({"-fdump-rtl-all"},
    "  -fdump-rtl-all              \tProduce all the dumps listed above.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-dump-rtl-all"), maplecl::kHide);

maplecl::Option<bool> oFdumpRtlAsmcons({"-fdump-rtl-asmcons"},
    "  -fdump-rtl-asmcons          \tDump after fixing rtl statements that have unsatisfied in/out constraints.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-dump-rtl-asmcons"), maplecl::kHide);

maplecl::Option<bool> oFdumpRtlAuto_inc_dec({"-fdump-rtl-auto_inc_dec"},
    "  -fdump-rtl-auto_inc_dec     \tDump after auto-inc-dec discovery. \n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-dump-rtl-auto_inc_dec"), maplecl::kHide);

maplecl::Option<bool> oFdumpRtlBarriers({"-fdump-rtl-barriers"},
    "  -fdump-rtl-barriers         \tDump after cleaning up the barrier instructions.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-dump-rtl-barriers"), maplecl::kHide);

maplecl::Option<bool> oFdumpRtlBbpart({"-fdump-rtl-bbpart"},
    "  -fdump-rtl-bbpart           \tDump after partitioning hot and cold basic blocks.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-dump-rtl-bbpart"), maplecl::kHide);

maplecl::Option<bool> oFdumpRtlBbro({"-fdump-rtl-bbro"},
    "  -fdump-rtl-bbro             \tDump after block reordering.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-dump-rtl-bbro"), maplecl::kHide);

maplecl::Option<bool> oFdumpRtlBtl2({"-fdump-rtl-btl2"},
    "  -fdump-rtl-btl2             \t-fdump-rtl-btl1 and -fdump-rtl-btl2 enable dumping after the two branch target "
    "load optimization passes.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-dump-rtl-btl2"), maplecl::kHide);

maplecl::Option<bool> oFdumpRtlBypass({"-fdump-rtl-bypass"},
    "  -fdump-rtl-bypass           \tDump after jump bypassing and control flow optimizations.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-dump-rtl-bypass"), maplecl::kHide);

maplecl::Option<bool> oFdumpRtlCe1({"-fdump-rtl-ce1"},
    "  -fdump-rtl-ce1              \tEnable dumping after the three if conversion passes.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-dump-rtl-ce1"), maplecl::kHide);

maplecl::Option<bool> oFdumpRtlCe2({"-fdump-rtl-ce2"},
    "  -fdump-rtl-ce2              \tEnable dumping after the three if conversion passes.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-dump-rtl-ce2"), maplecl::kHide);

maplecl::Option<bool> oFdumpRtlCe3({"-fdump-rtl-ce3"},
    "  -fdump-rtl-ce3              \tEnable dumping after the three if conversion passes.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-dump-rtl-ce3"), maplecl::kHide);

maplecl::Option<bool> oFdumpRtlCombine({"-fdump-rtl-combine"},
    "  -fdump-rtl-combine          \tDump after the RTL instruction combination pass.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-dump-rtl-combine"), maplecl::kHide);

maplecl::Option<bool> oFdumpRtlCompgotos({"-fdump-rtl-compgotos"},
    "  -fdump-rtl-compgotos        \tDump after duplicating the computed gotos.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-dump-rtl-compgotos"), maplecl::kHide);

maplecl::Option<bool> oFdumpRtlCprop_hardreg({"-fdump-rtl-cprop_hardreg"},
    "  -fdump-rtl-cprop_hardreg    \tDump after hard register copy propagation.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-dump-rtl-cprop_hardreg"), maplecl::kHide);

maplecl::Option<bool> oFdumpRtlCsa({"-fdump-rtl-csa"},
    "  -fdump-rtl-csa              \tDump after combining stack adjustments.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-dump-rtl-csa"), maplecl::kHide);

maplecl::Option<bool> oFdumpRtlCse1({"-fdump-rtl-cse1"},
    "  -fdump-rtl-cse1             \tEnable dumping after the two common subexpression elimination passes.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-dump-rtl-cse1"), maplecl::kHide);

maplecl::Option<bool> oFdumpRtlCse2({"-fdump-rtl-cse2"},
    "  -fdump-rtl-cse2             \tEnable dumping after the two common subexpression elimination passes.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-dump-rtl-cse2"), maplecl::kHide);

maplecl::Option<bool> oFdumpRtlDbr({"-fdump-rtl-dbr"},
    "  -fdump-rtl-dbr              \tDump after delayed branch scheduling.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-dump-rtl-dbr"), maplecl::kHide);

maplecl::Option<bool> oFdumpRtlDce({"-fdump-rtl-dce"},
    "  -fdump-rtl-dce              \tDump after the standalone dead code elimination passes.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-fdump-rtl-dce"), maplecl::kHide);

maplecl::Option<bool> oFdumpRtlDce1({"-fdump-rtl-dce1"},
    "  -fdump-rtl-dce1             \tenable dumping after the two dead store elimination passes.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-dump-rtl-dce1"), maplecl::kHide);

maplecl::Option<bool> oFdumpRtlDce2({"-fdump-rtl-dce2"},
    "  -fdump-rtl-dce2             \tenable dumping after the two dead store elimination passes.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-dump-rtl-dce2"), maplecl::kHide);

maplecl::Option<bool> oFdumpRtlDfinish({"-fdump-rtl-dfinish"},
    "  -fdump-rtl-dfinish          \tThis dump is defined but always produce empty files.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-dump-rtl-dfinish"), maplecl::kHide);

maplecl::Option<bool> oFdumpRtlDfinit({"-fdump-rtl-dfinit"},
    "  -fdump-rtl-dfinit           \tThis dump is defined but always produce empty files.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-dump-rtl-dfinit"), maplecl::kHide);

maplecl::Option<bool> oFdumpRtlEh({"-fdump-rtl-eh"},
    "  -fdump-rtl-eh               \tDump after finalization of EH handling code.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-dump-rtl-eh"), maplecl::kHide);

maplecl::Option<bool> oFdumpRtlEh_ranges({"-fdump-rtl-eh_ranges"},
    "  -fdump-rtl-eh_ranges        \tDump after conversion of EH handling range regions.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-dump-rtl-eh_ranges"), maplecl::kHide);

maplecl::Option<bool> oFdumpRtlExpand({"-fdump-rtl-expand"},
    "  -fdump-rtl-expand           \tDump after RTL generation.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-dump-rtl-expand"), maplecl::kHide);

maplecl::Option<bool> oFdumpRtlFwprop1({"-fdump-rtl-fwprop1"},
    "  -fdump-rtl-fwprop1          \tenable dumping after the two forward propagation passes.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-dump-rtl-fwprop1"), maplecl::kHide);

maplecl::Option<bool> oFdumpRtlFwprop2({"-fdump-rtl-fwprop2"},
    "  -fdump-rtl-fwprop2          \tenable dumping after the two forward propagation passes.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-dump-rtl-fwprop2"), maplecl::kHide);

maplecl::Option<bool> oFdumpRtlGcse1({"-fdump-rtl-gcse1"},
    "  -fdump-rtl-gcse1            \tenable dumping after global common subexpression elimination.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-dump-rtl-gcse1"), maplecl::kHide);

maplecl::Option<bool> oFdumpRtlGcse2({"-fdump-rtl-gcse2"},
    "  -fdump-rtl-gcse2            \tenable dumping after global common subexpression elimination.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-dump-rtl-gcse2"), maplecl::kHide);

maplecl::Option<bool> oFdumpRtlInitRegs({"-fdump-rtl-init-regs"},
    "  -fdump-rtl-init-regs        \tDump after the initialization of the registers.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-tedump-rtl-init-regsst"), maplecl::kHide);

maplecl::Option<bool> oFdumpRtlInitvals({"-fdump-rtl-initvals"},
    "  -fdump-rtl-initvals         \tDump after the computation of the initial value sets.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-dump-rtl-initvals"), maplecl::kHide);

maplecl::Option<bool> oFdumpRtlInto_cfglayout({"-fdump-rtl-into_cfglayout"},
    "  -fdump-rtl-into_cfglayout   \tDump after converting to cfglayout mode.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-dump-rtl-into_cfglayout"), maplecl::kHide);

maplecl::Option<bool> oFdumpRtlIra({"-fdump-rtl-ira"},
    "  -fdump-rtl-ira              \tDump after iterated register allocation.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-dump-rtl-ira"), maplecl::kHide);

maplecl::Option<bool> oFdumpRtlJump({"-fdump-rtl-jump"},
    "  -fdump-rtl-jump             \tDump after the second jump optimization.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-dump-rtl-jump"), maplecl::kHide);

maplecl::Option<bool> oFdumpRtlLoop2({"-fdump-rtl-loop2"},
    "  -fdump-rtl-loop2            \tenables dumping after the rtl loop optimization passes.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-dump-rtl-loop2"), maplecl::kHide);

maplecl::Option<bool> oFdumpRtlMach({"-fdump-rtl-mach"},
    "  -fdump-rtl-mach             \tDump after performing the machine dependent reorganization pass, "
    "if that pass exists.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-dump-rtl-mach"), maplecl::kHide);

maplecl::Option<bool> oFdumpRtlMode_sw({"-fdump-rtl-mode_sw"},
    "  -fdump-rtl-mode_sw          \tDump after removing redundant mode switches.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-dump-rtl-mode_sw"), maplecl::kHide);

maplecl::Option<bool> oFdumpRtlOutof_cfglayout({"-fdump-rtl-outof_cfglayout"},
    "  -fdump-rtl-outof_cfglayout  \tDump after converting from cfglayout mode.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-dump-rtl-outof_cfglayout"), maplecl::kHide);

maplecl::Option<bool> oFdumpRtlPeephole2({"-fdump-rtl-peephole2"},
    "  -fdump-rtl-peephole2        \tDump after the peephole pass.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-dump-rtl-peephole2"), maplecl::kHide);

maplecl::Option<bool> oFdumpRtlPostreload({"-fdump-rtl-postreload"},
    "  -fdump-rtl-postreload       \tDump after post-reload optimizations.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-dump-rtl-postreload"), maplecl::kHide);

maplecl::Option<bool> oFdumpRtlPro_and_epilogue({"-fdump-rtl-pro_and_epilogue"},
    "  -fdump-rtl-pro_and_epilogue \tDump after generating the function prologues and epilogues.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-dump-rtl-pro_and_epilogue"), maplecl::kHide);

maplecl::Option<bool> oFdumpRtlRee({"-fdump-rtl-ree"},
    "  -fdump-rtl-ree              \tDump after sign/zero extension elimination.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-dump-rtl-ree"), maplecl::kHide);

maplecl::Option<bool> oFdumpRtlRegclass({"-fdump-rtl-regclass"},
    "  -fdump-rtl-regclass         \tThis dump is defined but always produce empty files.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-dump-rtl-regclass"), maplecl::kHide);

maplecl::Option<bool> oFdumpRtlRnreg({"-fdump-rtl-rnreg"},
    "  -fdump-rtl-rnreg            \tDump after register renumbering.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-dump-rtl-rnreg"), maplecl::kHide);

maplecl::Option<bool> oFdumpRtlSched1({"-fdump-rtl-sched1"},
    "  -fdump-rtl-sched1           \tnable dumping after the basic block scheduling passes.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-dump-rtl-sched1"), maplecl::kHide);

maplecl::Option<bool> oFdumpRtlSched2({"-fdump-rtl-sched2"},
    "  -fdump-rtl-sched2           \tnable dumping after the basic block scheduling passes.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-dump-rtl-sched2"), maplecl::kHide);

maplecl::Option<bool> oFdumpRtlSeqabstr({"-fdump-rtl-seqabstr"},
    "  -fdump-rtl-seqabstr         \tDump after common sequence discovery.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-dump-rtl-seqabstr"), maplecl::kHide);

maplecl::Option<bool> oFdumpRtlShorten({"-fdump-rtl-shorten"},
    "  -fdump-rtl-shorten          \tDump after shortening branches.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-dump-rtl-shorten"), maplecl::kHide);

maplecl::Option<bool> oFdumpRtlSibling({"-fdump-rtl-sibling"},
    "  -fdump-rtl-sibling          \tDump after sibling call optimizations.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-dump-rtl-sibling"), maplecl::kHide);

maplecl::Option<bool> oFdumpRtlSms({"-fdump-rtl-sms"},
    "  -fdump-rtl-sms              \tDump after modulo scheduling. This pass is only run on some architectures.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-dump-rtl-sms"), maplecl::kHide);

maplecl::Option<bool> oFdumpRtlSplit1({"-fdump-rtl-split1"},
    "  -fdump-rtl-split1           \tThis option enable dumping after five rounds of instruction splitting.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-dump-rtl-split1"), maplecl::kHide);

maplecl::Option<bool> oFdumpRtlSplit2({"-fdump-rtl-split2"},
    "  -fdump-rtl-split2           \tThis option enable dumping after five rounds of instruction splitting.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-dump-rtl-split2"), maplecl::kHide);

maplecl::Option<bool> oFdumpRtlSplit3({"-fdump-rtl-split3"},
    "  -fdump-rtl-split3           \tThis option enable dumping after five rounds of instruction splitting.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-dump-rtl-split3"), maplecl::kHide);

maplecl::Option<bool> oFdumpRtlSplit4({"-fdump-rtl-split4"},
    "  -fdump-rtl-split4           \tThis option enable dumping after five rounds of instruction splitting.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-dump-rtl-split4"), maplecl::kHide);

maplecl::Option<bool> oFdumpRtlSplit5({"-fdump-rtl-split5"},
    "  -fdump-rtl-split5           \tThis option enable dumping after five rounds of instruction splitting.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-dump-rtl-split5"), maplecl::kHide);

maplecl::Option<bool> oFdumpRtlStack({"-fdump-rtl-stack"},
    "  -fdump-rtl-stack            \tDump after conversion from GCC's 'flat register file' registers to the x87's "
    "stack-like registers. This pass is only run on x86 variants.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-dump-rtl-stack"), maplecl::kHide);

maplecl::Option<bool> oFdumpRtlSubreg1({"-fdump-rtl-subreg1"},
    "  -fdump-rtl-subreg1          \tenable dumping after the two subreg expansion passes.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-dump-rtl-subreg1"), maplecl::kHide);

maplecl::Option<bool> oFdumpRtlSubreg2({"-fdump-rtl-subreg2"},
    "  -fdump-rtl-subreg2          \tenable dumping after the two subreg expansion passes.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-dump-rtl-subreg2"), maplecl::kHide);

maplecl::Option<bool> oFdumpRtlSubregs_of_mode_finish({"-fdump-rtl-subregs_of_mode_finish"},
    "  -fdump-rtl-subregs_of_mode_finish  \tThis dump is defined but always produce empty files.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-dump-rtl-subregs_of_mode_finish"), maplecl::kHide);

maplecl::Option<bool> oFdumpRtlSubregs_of_mode_init({"-fdump-rtl-subregs_of_mode_init"},
    "  -fdump-rtl-subregs_of_mode_init  \tThis dump is defined but always produce empty files.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-dump-rtl-subregs_of_mode_init"), maplecl::kHide);

maplecl::Option<bool> oFdumpRtlUnshare({"-fdump-rtl-unshare"},
    "  -fdump-rtl-unshare          \tDump after all rtl has been unshared.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-dump-rtl-unshare"), maplecl::kHide);

maplecl::Option<bool> oFdumpRtlVartrack({"-fdump-rtl-vartrack"},
    "  -fdump-rtl-vartrack         \tDump after variable tracking.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-dump-rtl-vartrack"), maplecl::kHide);

maplecl::Option<bool> oFdumpRtlVregs({"-fdump-rtl-vregs"},
    "  -fdump-rtl-vregs            \tDump after converting virtual registers to hard registers.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-dump-rtl-vregs"), maplecl::kHide);

maplecl::Option<bool> oFdumpRtlWeb({"-fdump-rtl-web"},
    "  -fdump-rtl-web              \tDump after live range splitting.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-dump-rtl-web"), maplecl::kHide);

maplecl::Option<bool> oFdumpStatistics({"-fdump-statistics"},
    "  -fdump-statistics           \tEnable and control dumping of pass statistics in a separate file.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-dump-statistics"), maplecl::kHide);

maplecl::Option<bool> oFdumpTranslationUnit({"-fdump-translation-unit"},
    "  -fdump-translation-unit     \tC++ only\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-dump-translation-unit"), maplecl::kHide);

maplecl::Option<bool> oFdumpTree({"-fdump-tree"},
    "  -fdump-tree                 \tControl the dumping at various stages of processing the intermediate language "
    "tree to a file.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-dump-tree"), maplecl::kHide);

maplecl::Option<bool> oFdumpTreeAll({"-fdump-tree-all"},
    "  -fdump-tree-all             \tControl the dumping at various stages of processing the intermediate "
    "language tree to a file.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-dump-tree-all"), maplecl::kHide);

maplecl::Option<bool> oFdumpUnnumbered({"-fdump-unnumbered"},
    "  -fdump-unnumbered           \tWhen doing debugging dumps, suppress instruction numbers and address output. \n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-dump-unnumbered"), maplecl::kHide);

maplecl::Option<bool> oFdumpUnnumberedLinks({"-fdump-unnumbered-links"},
    "  -fdump-unnumbered-links     \tWhen doing debugging dumps (see -d option above), suppress instruction numbers "
    "for the links to the previous and next instructions in a sequence.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-dump-unnumbered-links"), maplecl::kHide);

maplecl::Option<bool> oFdwarf2CfiAsm({"-fdwarf2-cfi-asm"},
    "  -fdwarf2-cfi-asm            \tEnable CFI tables via GAS assembler directives.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-dwarf2-cfi-asm"), maplecl::kHide);

maplecl::Option<bool> oFearlyInlining({"-fearly-inlining"},
    "  -fearly-inlining            \tPerform early inlining.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-early-inlining"), maplecl::kHide);

maplecl::Option<bool> oFeliminateDwarf2Dups({"-feliminate-dwarf2-dups"},
    "  -feliminate-dwarf2-dups     \tPerform DWARF duplicate elimination.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-eliminate-dwarf2-dups"), maplecl::kHide);

maplecl::Option<bool> oFeliminateUnusedDebugSymbols({"-feliminate-unused-debug-symbols"},
    "  -feliminate-unused-debug-symbols \tPerform unused symbol elimination in debug info.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-feliminate-unused-debug-symbols"), maplecl::kHide);

maplecl::Option<bool> oFeliminateUnusedDebugTypes({"-feliminate-unused-debug-types"},
    "  -feliminate-unused-debug-types \tPerform unused type elimination in debug info.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-eliminate-unused-debug-types"), maplecl::kHide);

maplecl::Option<bool> oFemitClassDebugAlways({"-femit-class-debug-always"},
    "  -femit-class-debug-always   \tDo not suppress C++ class debug information.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-emit-class-debug-always"), maplecl::kHide);

maplecl::Option<bool> oFemitStructDebugBaseonly({"-femit-struct-debug-baseonly"},
    "  -femit-struct-debug-baseonly  \tAggressive reduced debug info for structs.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-emit-struct-debug-baseonly"), maplecl::kHide);

maplecl::Option<bool> oFemitStructDebugReduced({"-femit-struct-debug-reduced"},
    "  -femit-struct-debug-reduced \tConservative reduced debug info for structs.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-emit-struct-debug-reduced"), maplecl::kHide);

maplecl::Option<bool> oFexceptions({"-fexceptions"},
    "  -fexceptions                \tEnable exception handling.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-exceptions"), maplecl::kHide);

maplecl::Option<bool> oFexpensiveOptimizations({"-fexpensive-optimizations"},
    "  -fexpensive-optimizations   \tPerform a number of minor, expensive optimizations.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-expensive-optimizations"), maplecl::kHide);

maplecl::Option<bool> oFextNumericLiterals({"-fext-numeric-literals"},
    "  -fext-numeric-literals      \tInterpret imaginary, fixed-point, or other gnu number suffix as the "
    "corresponding number literal rather than a user-defined number literal.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-ext-numeric-literals"), maplecl::kHide);

maplecl::Option<bool> oFnoExtendedIdentifiers({"-fno-extended-identifiers"},
    "  -fno-extended-identifiers   \tDon't ermit universal character names (\\u and \\U) in identifiers.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oFexternTlsInit({"-fextern-tls-init"},
    "  -fextern-tls-init           \tSupport dynamic initialization of thread-local variables in a different "
    "translation unit.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-extern-tls-init"), maplecl::kHide);

maplecl::Option<bool> oFfastMath({"-ffast-math"},
    "  -ffast-math                 \tThis option causes the preprocessor macro __FAST_MATH__ to be defined.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-fast-math"), maplecl::kHide);

maplecl::Option<bool> oFfatLtoObjects({"-ffat-lto-objects"},
    "  -ffat-lto-objects           \tOutput lto objects containing both the intermediate language and binary output.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-fat-lto-objects"), maplecl::kHide);

maplecl::Option<bool> oFfiniteMathOnly({"-ffinite-math-only"},
    "  -ffinite-math-only          \tAssume no NaNs or infinities are generated.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-finite-math-only"), maplecl::kHide);

maplecl::Option<bool> oFfixAndContinue({"-ffix-and-continue"},
    "  -ffix-and-continue          \tGenerate code suitable for fast turnaround development, such as to allow GDB"
    " to dynamically load .o files into already-running programs.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-fix-and-continue"), maplecl::kHide);

maplecl::Option<bool> oFfloatStore({"-ffloat-store"},
    "  -ffloat-store               \ton't allocate floats and doubles in extended-precision registers.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-float-store"), maplecl::kHide);

maplecl::Option<bool> oFforScope({"-ffor-scope"},
    "  -ffor-scope                 \tScope of for-init-statement variables is local to the loop.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-for-scope"), maplecl::kHide);

maplecl::Option<bool> oFforwardPropagate({"-fforward-propagate"},
    "  -fforward-propagate         \tPerform a forward propagation pass on RTL.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oFfreestanding({"-ffreestanding"},
    "  -ffreestanding              \tDo not assume that standard C libraries and 'main' exist.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-freestanding"), maplecl::kHide);

maplecl::Option<bool> oFfriendInjection({"-ffriend-injection"},
    "  -ffriend-injection          \tInject friend functions into enclosing namespace.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-friend-injection"), maplecl::kHide);

maplecl::Option<bool> oFgcse({"-fgcse"},
    "  -fgcse                      \tPerform global common subexpression elimination.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-gcse"), maplecl::kHide);

maplecl::Option<bool> oFgcseAfterReload({"-fgcse-after-reload"},
    "  -fgcse-after-reload         \t-fgcse-after-reload\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-gcse-after-reload"), maplecl::kHide);

maplecl::Option<bool> oFgcseLas({"-fgcse-las"},
    "  -fgcse-las                  \tPerform redundant load after store elimination in global common subexpression "
    "elimination.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-gcse-las"), maplecl::kHide);

maplecl::Option<bool> oFgcseLm({"-fgcse-lm"},
    "  -fgcse-lm                   \tPerform enhanced load motion during global common subexpression elimination.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-gcse-lm"), maplecl::kHide);

maplecl::Option<bool> oFgcseSm({"-fgcse-sm"},
    "  -fgcse-sm                   \tPerform store motion after global common subexpression elimination.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-gcse-sm"), maplecl::kHide);

maplecl::Option<bool> oFgimple({"-fgimple"},
    "  -fgimple                    \tEnable parsing GIMPLE.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-gimple"), maplecl::kHide);

maplecl::Option<bool> oFgnuRuntime({"-fgnu-runtime"},
    "  -fgnu-runtime              \tGenerate code for GNU runtime environment.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-gnu-runtime"), maplecl::kHide);

maplecl::Option<bool> oFgnuTm({"-fgnu-tm"},
    "  -fgnu-tm                    \tEnable support for GNU transactional memory.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-gnu-tm"), maplecl::kHide);

maplecl::Option<bool> oFgraphiteIdentity({"-fgraphite-identity"},
    "  -fgraphite-identity         \tEnable Graphite Identity transformation.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-graphite-identity"), maplecl::kHide);

maplecl::Option<bool> oFhoistAdjacentLoads({"-fhoist-adjacent-loads"},
    "  -fhoist-adjacent-loads      \tEnable hoisting adjacent loads to encourage generating conditional move"
    " instructions.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-hoist-adjacent-loads"), maplecl::kHide);

maplecl::Option<bool> oFhosted({"-fhosted"},
    "  -fhosted                    \tAssume normal C execution environment.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-hosted"), maplecl::kHide);

maplecl::Option<bool> oFifConversion({"-fif-conversion"},
    "  -fif-conversion             \tPerform conversion of conditional jumps to branchless equivalents.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-if-conversion"), maplecl::kHide);

maplecl::Option<bool> oFifConversion2({"-fif-conversion2"},
    "  -fif-conversion2            \tPerform conversion of conditional jumps to conditional execution.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-if-conversion2"), maplecl::kHide);

maplecl::Option<bool> oFilelist({"-filelist"},
    "  -filelist                   \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oFindirectData({"-findirect-data"},
    "  -findirect-data             \tGenerate code suitable for fast turnaround development, such as to allow "
    "GDB to dynamically load .o files into already-running programs\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-indirect-data"), maplecl::kHide);

maplecl::Option<bool> oFindirectInlining({"-findirect-inlining"},
    "  -findirect-inlining         tPerform indirect inlining.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-indirect-inlining"), maplecl::kHide);

maplecl::Option<bool> oFinhibitSizeDirective({"-finhibit-size-directive"},
    "  -finhibit-size-directive    \tDo not generate .size directives.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-inhibit-size-directive"), maplecl::kHide);

maplecl::Option<bool> oFinlineFunctions({"-finline-functions"},
    "  -finline-functions          \tIntegrate functions not declared 'inline' into their callers when profitable.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-inline-functions"), maplecl::kHide);

maplecl::Option<bool> oFinlineFunctionsCalledOnce({"-finline-functions-called-once"},
    "  -finline-functions-called-once  \tIntegrate functions only required by their single caller.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-inline-functions-called-once"), maplecl::kHide);

maplecl::Option<bool> oFinlineSmallFunctions({"-finline-small-functions"},
    "  -finline-small-functions    \tIntegrate functions into their callers when code size is known not to grow.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-inline-small-functions"), maplecl::kHide);

maplecl::Option<bool> oFinstrumentFunctions({"-finstrument-functions"},
    "  -finstrument-functions      \tInstrument function entry and exit with profiling calls.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-instrument-functions"), maplecl::kHide);

maplecl::Option<bool> oFipaCp({"-fipa-cp"},
    "  -fipa-cp                   \tPerform interprocedural constant propagation.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-ipa-cp"), maplecl::kHide);

maplecl::Option<bool> oFipaCpClone({"-fipa-cp-clone"},
    "  -fipa-cp-clone              \tPerform cloning to make Interprocedural constant propagation stronger.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-ipa-cp-clone"), maplecl::kHide);

maplecl::Option<bool> oFipaIcf({"-fipa-icf"},
    "  -fipa-icf                   \tPerform Identical Code Folding for functions and read-only variables.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-ipa-icf"), maplecl::kHide);

maplecl::Option<bool> oFipaProfile({"-fipa-profile"},
    "  -fipa-profile               \tPerform interprocedural profile propagation.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-ipa-profile"), maplecl::kHide);

maplecl::Option<bool> oFipaPta({"-fipa-pta"},
    "  -fipa-pta                   \tPerform interprocedural points-to analysis.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-ipa-pta"), maplecl::kHide);

maplecl::Option<bool> oFipaPureConst({"-fipa-pure-const"},
    "  -fipa-pure-const            \tDiscover pure and const functions.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-ipa-pure-const"), maplecl::kHide);

maplecl::Option<bool> oFipaRa({"-fipa-ra"},
    "  -fipa-ra                    \tUse caller save register across calls if possible.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-ipa-ra"), maplecl::kHide);

maplecl::Option<bool> oFipaReference({"-fipa-reference"},
    "  -fipa-reference             \tDiscover readonly and non addressable static variables.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-fno-ipa-reference"), maplecl::kHide);

}