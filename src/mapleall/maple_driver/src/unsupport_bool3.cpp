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

maplecl::Option<bool> fPIC({"-fPIC", "--fPIC"},
    "  --fPIC                      \tGenerate position-independent shared library in large mode.\n"
    "  --no-pic/-fno-pic           \n",
    {cgCategory, driverCategory, ldCategory}, kOptCommon | kOptLd | kOptNotFiltering);

maplecl::Option<bool> oMint8({"-mint8"},
    "  -mint8                      \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMinterlinkCompressed({"-minterlink-compressed"},
    "  -minterlink-compressed      \tRequire that code using the standard (uncompressed) MIPS ISA be "
    "link-compatible with MIPS16 and microMIPS code, and vice versa.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-interlink-compressed"), maplecl::kHide);

maplecl::Option<bool> oMinterlinkMips16({"-minterlink-mips16"},
    "  -minterlink-mips16          \tPredate the microMIPS ASE and are retained for backwards compatibility.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-interlink-mips16"), maplecl::kHide);

maplecl::Option<bool> oMioVolatile({"-mio-volatile"},
    "  -mio-volatile               \tTells the compiler that any variable marked with the io attribute is to "
    "be considered volatile.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMips1({"-mips1"},
    "  -mips1                      \tEquivalent to -march=mips1.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMips16({"-mips16"},
    "  -mips16                     \tGenerate MIPS16 code.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-mips16"), maplecl::kHide);

maplecl::Option<bool> oMips2({"-mips2"},
    "  -mips2                      \tEquivalent to -march=mips2.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMips3({"-mips3"},
    "  -mips3                      \tEquivalent to -march=mips3.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMips32({"-mips32"},
    "  -mips32                     \tEquivalent to -march=mips32.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMips32r3({"-mips32r3"},
    "  -mips32r3                   \tEquivalent to -march=mips32r3.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMips32r5({"-mips32r5"},
    "  -mips32r5                   \tEquivalent to -march=mips32r5.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMips32r6({"-mips32r6"},
    "  -mips32r6                   \tEquivalent to -march=mips32r6.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMips3d({"-mips3d"},
    "  -mips3d                     \tUse (do not use) the MIPS-3D ASE.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-mips3d"), maplecl::kHide);

maplecl::Option<bool> oMips4({"-mips4"},
    "  -mips4                      \tEquivalent to -march=mips4.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMips64({"-mips64"},
    "  -mips64                     \tEquivalent to -march=mips64.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMips64r2({"-mips64r2"},
    "  -mips64r2                   \tEquivalent to -march=mips64r2.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMips64r3({"-mips64r3"},
    "  -mips64r3                   \tEquivalent to -march=mips64r3.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMips64r5({"-mips64r5"},
    "  -mips64r5                   \tEquivalent to -march=mips64r5.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMips64r6({"-mips64r6"},
    "  -mips64r6                   \tEquivalent to -march=mips64r6.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMisize({"-misize"},
    "  -misize                     \tAnnotate assembler instructions with estimated addresses.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMissueRateNumber({"-missue-rate=number"},
    "  -missue-rate=number         \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMivc2({"-mivc2"},
    "  -mivc2                      \tEnables IVC2 scheduling. IVC2 is a 64-bit VLIW coprocessor.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMjsr({"-mjsr"},
    "  -mjsr                       \t\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-jsr"), maplecl::kHide);

maplecl::Option<bool> oMjumpInDelay({"-mjump-in-delay"},
    "  -mjump-in-delay             \tThis option is ignored and provided for compatibility purposes only.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMkernel({"-mkernel"},
    "  -mkernel                    \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMknuthdiv({"-mknuthdiv"},
    "  -mknuthdiv                  \tMake the result of a division yielding a remainder have the same sign as "
    "the divisor. \n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-knuthdiv"), maplecl::kHide);

maplecl::Option<bool> oMl({"-ml"},
    "  -ml                         \tCauses variables to be assigned to the .far section by default.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMlarge({"-mlarge"},
    "  -mlarge                     \tUse large-model addressing (20-bit pointers, 32-bit size_t).\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMlargeData({"-mlarge-data"},
    "  -mlarge-data                \tWith this option the data area is limited to just below 2GB.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMlargeDataThreshold({"-mlarge-data-threshold"},
    "  -mlarge-data-threshold      \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMlargeMem({"-mlarge-mem"},
    "  -mlarge-mem                 \tWith -mlarge-mem code is generated that assumes a full 32-bit address.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMlargeText({"-mlarge-text"},
    "  -mlarge-text                \tWhen -msmall-data is used, the compiler can assume that all local symbols "
    "share the same $gp value, and thus reduce the number of instructions required for a function call from 4 to 1.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMleadz({"-mleadz"},
    "  -mleadz                     \tnables the leadz (leading zero) instruction.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMleafIdSharedLibrary({"-mleaf-id-shared-library"},
    "  -mleaf-id-shared-library    \tenerate code that supports shared libraries via the library ID method, but "
    "assumes that this library or executable won't link against any other ID shared libraries.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-leaf-id-shared-library"), maplecl::kHide);

maplecl::Option<bool> oMlibfuncs({"-mlibfuncs"},
    "  -mlibfuncs                  \tSpecify that intrinsic library functions are being compiled, passing all "
    "values in registers, no matter the size.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-libfuncs"), maplecl::kHide);

maplecl::Option<bool> oMlibraryPic({"-mlibrary-pic"},
    "  -mlibrary-pic               \tGenerate position-independent EABI code.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMlinkedFp({"-mlinked-fp"},
    "  -mlinked-fp                 \tFollow the EABI requirement of always creating a frame pointer whenever a "
    "stack frame is allocated.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMlinkerOpt({"-mlinker-opt"},
    "  -mlinker-opt                \tEnable the optimization pass in the HP-UX linker.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMlinux({"-mlinux"},
    "  -mlinux                     \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMlittle({"-mlittle"},
    "  -mlittle                    \tAssume target CPU is configured as little endian.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMlittleEndian({"-mlittle-endian"},
    "  -mlittle-endian             \tAssume target CPU is configured as little endian.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMlittleEndianData({"-mlittle-endian-data"},
    "  -mlittle-endian-data        \tStore data (but not code) in the big-endian format.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMliw({"-mliw"},
    "  -mliw                       \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMll64({"-mll64"},
    "  -mll64                      \tEnable double load/store operations for ARC HS cores.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMllsc({"-mllsc"},
    "  -mllsc                      \tUse (do not use) 'll', 'sc', and 'sync' instructions to implement atomic "
    "memory built-in functions.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-llsc"), maplecl::kHide);

maplecl::Option<bool> oMloadStorePairs({"-mload-store-pairs"},
    "  -mload-store-pairs          \tEnable (disable) an optimization that pairs consecutive load or store "
    "instructions to enable load/store bonding. \n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-load-store-pairs"), maplecl::kHide);

maplecl::Option<bool> oMlocalSdata({"-mlocal-sdata"},
    "  -mlocal-sdata               \tExtend (do not extend) the -G behavior to local data too, such as to "
    "static variables in C. \n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-local-sdata"), maplecl::kHide);

maplecl::Option<bool> oMlock({"-mlock"},
    "  -mlock                      \tPassed down to the assembler to enable the locked load/store conditional "
    "extension. \n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMlongCalls({"-mlong-calls"},
    "  -mlong-calls                \tGenerate calls as register indirect calls, thus providing access to the "
    "full 32-bit address range.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-long-calls"), maplecl::kHide);

maplecl::Option<bool> oMlongDouble128({"-mlong-double-128"},
    "  -mlong-double-128           \tControl the size of long double type. \n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMlongDouble64({"-mlong-double-64"},
    "  -mlong-double-64            \tControl the size of long double type.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMlongDouble80({"-mlong-double-80"},
    "  -mlong-double-80            \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMlongJumpTableOffsets({"-mlong-jump-table-offsets"},
    "  -mlong-jump-table-offsets   \tUse 32-bit offsets in switch tables. \n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMlongJumps({"-mlong-jumps"},
    "  -mlong-jumps                \tDisable (or re-enable) the generation of PC-relative jump instructions.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-long-jumps"), maplecl::kHide);

maplecl::Option<bool> oMlongLoadStore({"-mlong-load-store"},
    "  -mlong-load-store           \tGenerate 3-instruction load and store sequences as sometimes required by "
    "the HP-UX 10 linker.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMlong32({"-mlong32"},
    "  -mlong32                    \tForce long, int, and pointer types to be 32 bits wide.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMlong64({"-mlong64"},
    "  -mlong64                    \tForce long types to be 64 bits wide. \n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMlongcall({"-mlongcall"},
    "  -mlongcall                  \tBy default assume that all calls are far away so that a longer and more "
    "expensive calling sequence is required. \n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-longcall"), maplecl::kHide);

maplecl::Option<bool> oMlongcalls({"-mlongcalls"},
    "  -mlongcalls                 \t\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-longcalls"), maplecl::kHide);

maplecl::Option<bool> oMloop({"-mloop"},
    "  -mloop                      \tEnables the use of the e3v5 LOOP instruction. \n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMlow64k({"-mlow-64k"},
    "  -mlow-64k                   \tWhen enabled, the compiler is free to take advantage of the knowledge that "
    "the entire program fits into the low 64k of memory.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-low-64k"), maplecl::kHide);

maplecl::Option<bool> oMlowPrecisionRecipSqrt({"-mlow-precision-recip-sqrt"},
    "  -mlow-precision-recip-sqrt  \tEnable the reciprocal square root approximation.  Enabling this reduces precision"
    " of reciprocal square root results to about 16 bits for single precision and to 32 bits for double precision.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-mno-low-precision-recip-sqrt"), maplecl::kHide);

maplecl::Option<bool> oMlp64({"-mlp64"},
    "  -mlp64                      \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMlra({"-mlra"},
    "  -mlra                       \tEnable Local Register Allocation. By default the port uses LRA.(i.e. -mno-lra).\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-lra"), maplecl::kHide);

maplecl::Option<bool> oMlraPriorityCompact({"-mlra-priority-compact"},
    "  -mlra-priority-compact      \tIndicate target register priority for r0..r3 / r12..r15.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMlraPriorityNoncompact({"-mlra-priority-noncompact"},
    "  -mlra-priority-noncompact   \tReduce target register priority for r0..r3 / r12..r15.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMlraPriorityNone({"-mlra-priority-none"},
    "  -mlra-priority-none         \tDon't indicate any priority for target registers.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMlwp({"-mlwp"},
    "  -mlwp                       \tThese switches enable the use of instructions in the mlwp.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMlxc1Sxc1({"-mlxc1-sxc1"},
    "  -mlxc1-sxc1                 \tWhen applicable, enable (disable) the generation of lwxc1, swxc1, ldxc1, "
    "sdxc1 instructions.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMlzcnt({"-mlzcnt"},
    "  -mlzcnt                     \these switches enable the use of instructions in the mlzcnt\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMm({"-Mm"},
    "  -Mm                         \tCauses variables to be assigned to the .near section by default.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMmac({"-mmac"},
    "  -mmac                       \tEnable the use of multiply-accumulate instructions.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMmac24({"-mmac-24"},
    "  -mmac-24                    \tPassed down to the assembler. \n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMmacD16({"-mmac-d16"},
    "  -mmac-d16                   \tPassed down to the assembler.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMmac_24({"-mmac_24"},
    "  -mmac_24                    \tReplaced by -mmac-24.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMmac_d16({"-mmac_d16"},
    "  -mmac_d16                   \tReplaced by -mmac-d16.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMmad({"-mmad"},
    "  -mmad                       \tEnable (disable) use of the mad, madu and mul instructions, as provided by "
    "the R4650 ISA.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-mad"), maplecl::kHide);

maplecl::Option<bool> oMmadd4({"-mmadd4"},
    "  -mmadd4                     \tWhen applicable, enable (disable) the generation of 4-operand madd.s, madd.d "
    "and related instructions.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMmainkernel({"-mmainkernel"},
    "  -mmainkernel                \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMmalloc64({"-mmalloc64"},
    "  -mmalloc64                  \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMmax({"-mmax"},
    "  -mmax                       \tGenerate code to use MAX instruction sets.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-max"), maplecl::kHide);

maplecl::Option<bool> oMmaxConstantSize({"-mmax-constant-size"},
    "  -mmax-constant-size         \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMmaxStackFrame({"-mmax-stack-frame"},
    "  -mmax-stack-frame           \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMmcountRaAddress({"-mmcount-ra-address"},
    "  -mmcount-ra-address         \tEmit (do not emit) code that allows _mcount to modify the calling function's "
    "return address.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-mcount-ra-address"), maplecl::kHide);

maplecl::Option<bool> oMmcu({"-mmcu"},
    "  -mmcu                       \tUse the MIPS MCU ASE instructions.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-mcu"), maplecl::kHide);

maplecl::Option<bool> oMmedia({"-mmedia"},
    "  -mmedia                     \tUse media instructions.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-media"), maplecl::kHide);

maplecl::Option<bool> oMmediumCalls({"-mmedium-calls"},
    "  -mmedium-calls              \tDon't use less than 25-bit addressing range for calls, which is the offset "
    "available for an unconditional branch-and-link instruction. \n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMmemcpy({"-mmemcpy"},
    "  -mmemcpy                    \tForce (do not force) the use of memcpy for non-trivial block moves.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-memcpy"), maplecl::kHide);

maplecl::Option<bool> oMmemcpyStrategyStrategy({"-mmemcpy-strategy=strategy"},
    "  -mmemcpy-strategy=strategy  \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMmemsetStrategyStrategy({"-mmemset-strategy=strategy"},
    "  -mmemset-strategy=strategy  \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMmfcrf({"-mmfcrf"},
    "  -mmfcrf                     \tSpecify which instructions are available on the processor you are using. \n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-mfcrf"), maplecl::kHide);

maplecl::Option<bool> oMmfpgpr({"-mmfpgpr"},
    "  -mmfpgpr                    \tSpecify which instructions are available on the processor you are using.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-mfpgpr"), maplecl::kHide);

maplecl::Option<bool> oMmicromips({"-mmicromips"},
    "  -mmicromips                 \tGenerate microMIPS code.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-mmicromips"), maplecl::kHide);

maplecl::Option<bool> oMminimalToc({"-mminimal-toc"},
    "  -mminimal-toc               \tModify generation of the TOC (Table Of Contents), which is created for "
    "every executable file. \n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMminmax({"-mminmax"},
    "  -mminmax                    \tEnables the min and max instructions.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMmitigateRop({"-mmitigate-rop"},
    "  -mmitigate-rop              \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMmixedCode({"-mmixed-code"},
    "  -mmixed-code                \tTweak register allocation to help 16-bit instruction generation.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMmmx({"-mmmx"},
    "  -mmmx                       \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMmodelLarge({"-mmodel=large"},
    "  -mmodel=large               \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMmodelMedium({"-mmodel=medium"},
    "  -mmodel=medium              \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMmodelSmall({"-mmodel=small"},
    "  -mmodel=small               \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMmovbe({"-mmovbe"},
    "  -mmovbe                     \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMmpx({"-mmpx"},
    "  -mmpx                       \tThese switches enable the use of instructions in the mmpx.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMmpyOption({"-mmpy-option"},
    "  -mmpy-option                \tCompile ARCv2 code with a multiplier design option. \n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMmsBitfields({"-mms-bitfields"},
    "  -mms-bitfields              \t\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-ms-bitfields"), maplecl::kHide);

maplecl::Option<bool> oMmt({"-mmt"},
    "  -mmt                        \tUse MT Multithreading instructions.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-mt"), maplecl::kHide);

maplecl::Option<bool> oMmul({"-mmul"},
    "  -mmul                       \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMmulBugWorkaround({"-mmul-bug-workaround"},
    "  -mmul-bug-workaround        \t\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-mul-bug-workaround"), maplecl::kHide);

maplecl::Option<bool> oMmulx({"-mmul.x"},
    "  -mmul.x                     \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMmul32x16({"-mmul32x16"},
    "  -mmul32x16                  \tGenerate 32x16-bit multiply and multiply-accumulate instructions.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMmul64({"-mmul64"},
    "  -mmul64                     \tGenerate mul64 and mulu64 instructions. Only valid for -mcpu=ARC600.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMmuladd({"-mmuladd"},
    "  -mmuladd                    \tUse multiply and add/subtract instructions.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-muladd"), maplecl::kHide);

maplecl::Option<bool> oMmulhw({"-mmulhw"},
    "  -mmulhw                     \tGenerate code that uses the half-word multiply and multiply-accumulate "
    "instructions on the IBM 405, 440, 464 and 476 processors.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-mulhw"), maplecl::kHide);

maplecl::Option<bool> oMmult({"-mmult"},
    "  -mmult                      \tEnables the multiplication and multiply-accumulate instructions.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMmultBug({"-mmult-bug"},
    "  -mmult-bug                  \t\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-mult-bug"), maplecl::kHide);

maplecl::Option<bool> oMmultcost({"-mmultcost"},
    "  -mmultcost             \tCost to assume for a multiply instruction, with '4' being equal to a normal "
    "instruction.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMmultiCondExec({"-mmulti-cond-exec"},
    "  -mmulti-cond-exec           \tEnable optimization of && and || in conditional execution (default).\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-multi-cond-exec"), maplecl::kHide);

maplecl::Option<bool> oMmulticore({"-mmulticore"},
    "  -mmulticore                 \tBuild a standalone application for multicore Blackfin processors. \n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMmultiple({"-mmultiple"},
    "  -mmultiple                  \tGenerate code that uses (does not use) the load multiple word instructions "
    "and the store multiple word instructions. \n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-multiple"), maplecl::kHide);

maplecl::Option<bool> oMmusl({"-mmusl"},
    "  -mmusl                      \tUse musl C library.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMmvcle({"-mmvcle"},
    "  -mmvcle                     \tGenerate code using the mvcle instruction to perform block moves.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-mvcle"), maplecl::kHide);

maplecl::Option<bool> oMmvme({"-mmvme"},
    "  -mmvme                      \tOn embedded PowerPC systems, assume that the startup module is called crt0.o "
    "and the standard C libraries are libmvme.a and libc.a.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMmwaitx({"-mmwaitx"},
    "  -mmwaitx                    \tThese switches enable the use of instructions in the mmwaitx.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMn({"-mn"},
    "  -mn                         \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMnFlash({"-mn-flash"},
    "  -mn-flash                   \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMnan2008({"-mnan=2008"},
    "  -mnan=2008                  \tControl the encoding of the special not-a-number (NaN) IEEE 754 "
    "floating-point data.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMnanLegacy({"-mnan=legacy"},
    "  -mnan=legacy                \tControl the encoding of the special not-a-number (NaN) IEEE 754 "
    "floating-point data.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMneonFor64bits({"-mneon-for-64bits"},
    "  -mneon-for-64bits           \tEnables using Neon to handle scalar 64-bits operations.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMnestedCondExec({"-mnested-cond-exec"},
    "  -mnested-cond-exec          \tEnable nested conditional execution optimizations (default).\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-nested-cond-exec"), maplecl::kHide);

maplecl::Option<bool> oMnhwloop({"-mnhwloop"},
    "  -mnhwloop                   \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMnoAlignStringops({"-mno-align-stringops"},
    "  -mno-align-stringops        \tDo not align the destination of inlined string operations.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMnoBrcc({"-mno-brcc"},
    "  -mno-brcc                   \tThis option disables a target-specific pass in arc_reorg to generate "
    "compare-and-branch (brcc) instructions. \n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMnoClearbss({"-mno-clearbss"},
    "  -mno-clearbss               \tThis option is deprecated. \n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMnoCrt0({"-mno-crt0"},
    "  -mno-crt0                   \tDo not link in the C run-time initialization object file.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMnoDefault({"-mno-default"},
    "  -mno-default                \tThis option instructs Maple to turn off all tunable features.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMnoDpfpLrsr({"-mno-dpfp-lrsr"},
    "  -mno-dpfp-lrsr              \tDisable lr and sr instructions from using FPX extension aux registers.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMnoEflags({"-mno-eflags"},
    "  -mno-eflags                 \tDo not mark ABI switches in e_flags.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMnoFancyMath387({"-mno-fancy-math-387"},
    "  -mno-fancy-math-387         \tSome 387 emulators do not support the sin, cos and sqrt instructions for the "
    "387. Specify this option to avoid generating those instructions. \n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMnoFloat({"-mno-float"},
    "  -mno-float                  \tEquivalent to -msoft-float, but additionally asserts that the program being "
    "compiled does not perform any floating-point operations. \n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMnoFpInToc({"-mno-fp-in-toc"},
    "  -mno-fp-in-toc              \tModify generation of the TOC (Table Of Contents), which is created for every "
    "executable file.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMFpReg({"-mfp-reg"},
    "  -mfp-reg                    \tGenerate code that uses (does not use) the floating-point register set. \n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-fp-regs"), maplecl::kHide);

maplecl::Option<bool> oMnoFpRetIn387({"-mno-fp-ret-in-387"},
    "  -mno-fp-ret-in-387          \tDo not use the FPU registers for return values of functions.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMnoInlineFloatDivide({"-mno-inline-float-divide"},
    "  -mno-inline-float-divide    \tDo not generate inline code for divides of floating-point values.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMnoInlineIntDivide({"-mno-inline-int-divide"},
    "  -mno-inline-int-divide      \tDo not generate inline code for divides of integer values.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMnoInlineSqrt({"-mno-inline-sqrt"},
    "  -mno-inline-sqrt            \tDo not generate inline code for sqrt.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMnoInterrupts({"-mno-interrupts"},
    "  -mno-interrupts             \tGenerated code is not compatible with hardware interrupts.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMnoLsim({"-mno-lsim"},
    "  -mno-lsim                   \tAssume that runtime support has been provided and so there is no need to "
    "include the simulator library (libsim.a) on the linker command line.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMnoMillicode({"-mno-millicode"},
    "  -mno-millicode              \tWhen optimizing for size (using -Os), prologues and epilogues that have to "
    "save or restore a large number of registers are often shortened by using call to a special function in libgcc\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMnoMpy({"-mno-mpy"},
    "  -mno-mpy                    \tDo not generate mpy-family instructions for ARC700. \n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMnoOpts({"-mno-opts"},
    "  -mno-opts                   \tDisables all the optional instructions enabled by -mall-opts.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMnoPic({"-mno-pic"},
    "  -mno-pic                    \tGenerate code that does not use a global pointer register. \n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMnoPostinc({"-mno-postinc"},
    "  -mno-postinc                \tCode generation tweaks that disable, respectively, splitting of 32-bit loads, "
    "generation of post-increment addresses, and generation of post-modify addresses.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMnoPostmodify({"-mno-postmodify"},
    "  -mno-postmodify             \tCode generation tweaks that disable, respectively, splitting of 32-bit loads, "
    "generation of post-increment addresses, and generation of post-modify addresses. \n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMnoRedZone({"-mno-red-zone"},
    "  -mno-red-zone               \tDo not use a so-called “red zone” for x86-64 code.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMnoRoundNearest({"-mno-round-nearest"},
    "  -mno-round-nearest          \tMake the scheduler assume that the rounding mode has been set to truncating.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMnoSchedProlog({"-mno-sched-prolog"},
    "  -mno-sched-prolog           \tPrevent the reordering of instructions in the function prologue, or the "
    "merging of those instruction with the instructions in the function's body. \n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMnoSideEffects({"-mno-side-effects"},
    "  -mno-side-effects           \tDo not emit instructions with side effects in addressing modes other than "
    "post-increment.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMnoSoftCmpsf({"-mno-soft-cmpsf"},
    "  -mno-soft-cmpsf             \tFor single-precision floating-point comparisons, emit an fsub instruction "
    "and test the flags. \n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMnoSpaceRegs({"-mno-space-regs"},
    "  -mno-space-regs             \tGenerate code that assumes the target has no space registers.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMSpe({"-mspe"},
    "  -mspe                       \tThis switch enables the generation of SPE simd instructions.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-mno-spe"), maplecl::kHide);

maplecl::Option<bool> oMnoSumInToc({"-mno-sum-in-toc"},
    "  -mno-sum-in-toc             \tModify generation of the TOC (Table Of Contents), which is created for "
    "every executable file. \n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMnoVectDouble({"-mnovect-double"},
    "  -mno-vect-double            \tChange the preferred SIMD mode to SImode. \n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMnobitfield({"-mnobitfield"},
    "  -mnobitfield                \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMnodiv({"-mnodiv"},
    "  -mnodiv                     \tDo not use div and mod instructions.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMnoliw({"-mnoliw"},
    "  -mnoliw                     \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMnomacsave({"-mnomacsave"},
    "  -mnomacsave                 \tMark the MAC register as call-clobbered, even if -mrenesas is given.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMnopFunDllimport({"-mnop-fun-dllimport"},
    "  -mnop-fun-dllimport         \tThis option is available for Cygwin and MinGW targets.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMnopMcount({"-mnop-mcount"},
    "  -mnop-mcount                \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMnops({"-mnops"},
    "  -mnops                      \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMnorm({"-mnorm"},
    "  -mnorm                      \tGenerate norm instructions. \n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMnosetlb({"-mnosetlb"},
    "  -mnosetlb                   \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMnosplitLohi({"-mnosplit-lohi"},
    "  -mnosplit-lohi              \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oModdSpreg({"-modd-spreg"},
    "  -modd-spreg                 \tEnable the use of odd-numbered single-precision floating-point registers "
    "for the o32 ABI.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-odd-spreg"), maplecl::kHide);

maplecl::Option<bool> oMoneByteBool({"-mone-byte-bool"},
    "  -mone-byte-bool             \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMoptimize({"-moptimize"},
    "  -moptimize                  \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMoptimizeMembar({"-moptimize-membar"},
    "  -moptimize-membar           \tThis switch removes redundant membar instructions from the compiler-generated "
    "code. \n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-optimize-membar"), maplecl::kHide);

maplecl::Option<bool> oMpaRisc10({"-mpa-risc-1-0"},
    "  -mpa-risc-1-0               \tSynonyms for -march=1.0 respectively.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMpaRisc11({"-mpa-risc-1-1"},
    "  -mpa-risc-1-1               \tSynonyms for -march=1.1 respectively.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMpaRisc20({"-mpa-risc-2-0"},
    "  -mpa-risc-2-0               \tSynonyms for -march=2.0 respectively.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMpack({"-mpack"},
    "  -mpack                      \tPack VLIW instructions.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-pack"), maplecl::kHide);

maplecl::Option<bool> oMpackedStack({"-mpacked-stack"},
    "  -mpacked-stack              \tUse the packed stack layout.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-packed-stack"), maplecl::kHide);

maplecl::Option<bool> oMpadstruct({"-mpadstruct"},
    "  -mpadstruct                 \tThis option is deprecated.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMpaired({"-mpaired"},
    "  -mpaired                    \tThis switch enables or disables the generation of PAIRED simd instructions.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-paired"), maplecl::kHide);

maplecl::Option<bool> oMpairedSingle({"-mpaired-single"},
    "  -mpaired-single             \tUse paired-single floating-point instructions.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-paired-single"), maplecl::kHide);

maplecl::Option<bool> oMpcRelativeLiteralLoads({"-mpc-relative-literal-loads"},
    "  -mpc-relative-literal-loads \tPC relative literal loads.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-mno-pc-relative-literal-loads"), maplecl::kHide);

maplecl::Option<bool> oMpc32({"-mpc32"},
    "  -mpc32                      \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMpc64({"-mpc64"},
    "  -mpc64                      \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMpc80({"-mpc80"},
    "  -mpc80                      \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMpclmul({"-mpclmul"},
    "  -mpclmul                    \tThese switches enable the use of instructions in the mpclmul.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMpcrel({"-mpcrel"},
    "  -mpcrel                     \tUse the pc-relative addressing mode of the 68000 directly, instead of using "
    "a global offset table.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMpdebug({"-mpdebug"},
    "  -mpdebug                    \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMpe({"-mpe"},
    "  -mpe                        \tSupport IBM RS/6000 SP Parallel Environment (PE). \n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMpeAlignedCommons({"-mpe-aligned-commons"},
    "  -mpe-aligned-commons        \tThis option is available for Cygwin and MinGW targets.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMperfExt({"-mperf-ext"},
    "  -mperf-ext                  \tGenerate performance extension instructions.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-perf-ext"), maplecl::kHide);

maplecl::Option<bool> oMpicDataIsTextRelative({"-mpic-data-is-text-relative"},
    "  -mpic-data-is-text-relative \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMpicRegister({"-mpic-register"},
    "  -mpic-register              \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMpid({"-mpid"},
    "  -mpid                       \t\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-pid"), maplecl::kHide);

maplecl::Option<bool> oMpku({"-mpku"},
    "  -mpku                       \tThese switches enable the use of instructions in the mpku.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMpointerSizeSize({"-mpointer-size=size"},
    "  -mpointer-size=size         \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMpointersToNestedFunctions({"-mpointers-to-nested-functions"},
    "  -mpointers-to-nested-functions  \tGenerate (do not generate) code to load up the static chain register (r11) "
    "when calling through a pointer on AIX and 64-bit Linux systems where a function pointer points to a 3-word "
    "descriptor giving the function address, TOC value to be loaded in register r2, and static chain value to be "
    "loaded in register r11. \n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMpokeFunctionName({"-mpoke-function-name"},
    "  -mpoke-function-name        \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMpopc({"-mpopc"},
    "  -mpopc                      \tWith -mpopc, Maple generates code that takes advantage of the UltraSPARC "
    "Population Count instruction.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-popc"), maplecl::kHide);

maplecl::Option<bool> oMpopcnt({"-mpopcnt"},
    "  -mpopcnt                    \tThese switches enable the use of instructions in the mpopcnt.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMpopcntb({"-mpopcntb"},
    "  -mpopcntb                   \tSpecify which instructions are available on the processor you are using. \n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-popcntb"), maplecl::kHide);

maplecl::Option<bool> oMpopcntd({"-mpopcntd"},
    "  -mpopcntd                   \tSpecify which instructions are available on the processor you are using. \n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-popcntd"), maplecl::kHide);

maplecl::Option<bool> oMportableRuntime({"-mportable-runtime"},
    "  -mportable-runtime          \tUse the portable calling conventions proposed by HP for ELF systems.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMpower8Fusion({"-mpower8-fusion"},
    "  -mpower8-fusion             \tGenerate code that keeps some integer operations adjacent so that the "
    "instructions can be fused together on power8 and later processors.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-power8-fusion"), maplecl::kHide);

maplecl::Option<bool> oMpower8Vector({"-mpower8-vector"},
    "  -mpower8-vector             \tGenerate code that uses the vector and scalar instructions that were added "
    "in version 2.07 of the PowerPC ISA.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-power8-vector"), maplecl::kHide);

maplecl::Option<bool> oMpowerpcGfxopt({"-mpowerpc-gfxopt"},
    "  -mpowerpc-gfxopt            \tSpecify which instructions are available on the processor you are using\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-powerpc-gfxopt"), maplecl::kHide);

maplecl::Option<bool> oMpowerpcGpopt({"-mpowerpc-gpopt"},
    "  -mpowerpc-gpopt             \tSpecify which instructions are available on the processor you are using\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-powerpc-gpopt"), maplecl::kHide);

maplecl::Option<bool> oMpowerpc64({"-mpowerpc64"},
    "  -mpowerpc64                 \tSpecify which instructions are available on the processor you are using\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-powerpc64"), maplecl::kHide);

maplecl::Option<bool> oMpreferAvx128({"-mprefer-avx128"},
    "  -mprefer-avx128             \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMpreferShortInsnRegs({"-mprefer-short-insn-regs"},
    "  -mprefer-short-insn-regs    \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMprefergot({"-mprefergot"},
    "  -mprefergot                 \tWhen generating position-independent code, emit function calls using the "
    "Global Offset Table instead of the Procedure Linkage Table.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMpreferredStackBoundary({"-mpreferred-stack-boundary"},
    "  -mpreferred-stack-boundary  \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMprefetchwt1({"-mprefetchwt1"},
    "  -mprefetchwt1               \tThese switches enable the use of instructions in the mprefetchwt1.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMpretendCmove({"-mpretend-cmove"},
    "  -mpretend-cmove             \tPrefer zero-displacement conditional branches for conditional move "
    "instruction patterns.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMprintTuneInfo({"-mprint-tune-info"},
    "  -mprint-tune-info           \tPrint CPU tuning information as comment in assembler file.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMprologFunction({"-mprolog-function"},
    "  -mprolog-function           \tDo use external functions to save and restore registers at the prologue "
    "and epilogue of a function.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-prolog-function"), maplecl::kHide);

maplecl::Option<bool> oMprologueEpilogue({"-mprologue-epilogue"},
    "  -mprologue-epilogue         \t\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-prologue-epilogue"), maplecl::kHide);

maplecl::Option<bool> oMprototype({"-mprototype"},
    "  -mprototype                 \tOn System V.4 and embedded PowerPC systems assume that all calls to variable "
    "argument functions are properly prototyped. \n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-prototype"), maplecl::kHide);

maplecl::Option<bool> oMpureCode({"-mpure-code"},
    "  -mpure-code                 \tDo not allow constant data to be placed in code sections.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMpushArgs({"-mpush-args"},
    "  -mpush-args                 \t\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-push-args"), maplecl::kHide);

maplecl::Option<bool> oMqClass({"-mq-class"},
    "  -mq-class                   \tEnable 'q' instruction alternatives.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMquadMemory({"-mquad-memory"},
    "  -mquad-memory               \tGenerate code that uses the non-atomic quad word memory instructions. \n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-quad-memory"), maplecl::kHide);

maplecl::Option<bool> oMquadMemoryAtomic({"-mquad-memory-atomic"},
    "  -mquad-memory-atomic        \tGenerate code that uses the atomic quad word memory instructions.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-quad-memory-atomic"), maplecl::kHide);

maplecl::Option<bool> oMr10kCacheBarrier({"-mr10k-cache-barrier"},
    "  -mr10k-cache-barrier        \tSpecify whether Maple should insert cache barriers to avoid the side-effects "
    "of speculation on R10K processors.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMRcq({"-mRcq"},
    "  -mRcq                       \tEnable 'Rcq' constraint handling. \n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMRcw({"-mRcw"},
    "  -mRcw                       \tEnable 'Rcw' constraint handling. \n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMrdrnd({"-mrdrnd"},
    "  -mrdrnd                     \tThese switches enable the use of instructions in the mrdrnd.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMreadonlyInSdata({"-mreadonly-in-sdata"},
    "  -mreadonly-in-sdata         \t\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-readonly-in-sdata"), maplecl::kHide);

maplecl::Option<bool> oMrecipPrecision({"-mrecip-precision"},
    "  -mrecip-precision           \tAssume (do not assume) that the reciprocal estimate instructions provide "
    "higher-precision estimates than is mandated by the PowerPC ABI.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMrecordMcount({"-mrecord-mcount"},
    "  -mrecord-mcount             \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMreducedRegs({"-mreduced-regs"},
    "  -mreduced-regs              \tUse reduced-set registers for register allocation.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMregisterNames({"-mregister-names"},
    "  -mregister-names            \t\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-register-names"), maplecl::kHide);

maplecl::Option<bool> oMregnames({"-mregnames"},
    "  -mregnames                  \tOn System V.4 and embedded PowerPC systems do emit register names in the "
    "assembly language output using symbolic forms.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-regnames"), maplecl::kHide);

maplecl::Option<bool> oMregparm({"-mregparm"},
    "  -mregparm                   \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMrelax({"-mrelax"},
    "  -mrelax                     \tGuide linker to relax instructions.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-relax"), maplecl::kHide);

maplecl::Option<bool> oMrelaxImmediate({"-mrelax-immediate"},
    "  -mrelax-immediate           \t\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-relax-immediate"), maplecl::kHide);

maplecl::Option<bool> oMrelaxPicCalls({"-mrelax-pic-calls"},
    "  -mrelax-pic-calls           \tTry to turn PIC calls that are normally dispatched via register $25 "
    "into direct calls.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMrelocatable({"-mrelocatable"},
    "  -mrelocatable               \tGenerate code that allows a static executable to be relocated to a different "
    "address at run time. \n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-relocatable"), maplecl::kHide);

maplecl::Option<bool> oMrelocatableLib({"-mrelocatable-lib"},
    "  -mrelocatable-lib           \tGenerates a .fixup section to allow static executables to be relocated at "
    "run time, but -mrelocatable-lib does not use the smaller stack alignment of -mrelocatable.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-relocatable-lib"), maplecl::kHide);

maplecl::Option<bool> oMrenesas({"-mrenesas"},
    "  -mrenesas                   \tComply with the calling conventions defined by Renesas.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-renesas"), maplecl::kHide);

maplecl::Option<bool> oMrepeat({"-mrepeat"},
    "  -mrepeat                    \tEnables the repeat and erepeat instructions, used for low-overhead looping.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMrestrictIt({"-mrestrict-it"},
    "  -mrestrict-it               \tRestricts generation of IT blocks to conform to the rules of ARMv8.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMreturnPointerOnD0({"-mreturn-pointer-on-d0"},
    "  -mreturn-pointer-on-d0      \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMrh850Abi({"-mrh850-abi"},
    "  -mrh850-abi                 \tEnables support for the RH850 version of the V850 ABI.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMrl78({"-mrl78"},
    "  -mrl78                      \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMrmw({"-mrmw"},
    "  -mrmw                       \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMrtd({"-mrtd"},
    "  -mrtd                       \tUse a different function-calling convention, in which functions that take a "
    "fixed number of arguments return with the rtd instruction, which pops their arguments while returning.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-rtd"), maplecl::kHide);

maplecl::Option<bool> oMrtm({"-mrtm"},
    "  -mrtm                       \tThese switches enable the use of instructions in the mrtm.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMrtp({"-mrtp"},
    "  -mrtp                       \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMrtsc({"-mrtsc"},
    "  -mrtsc                      \tPassed down to the assembler to enable the 64-bit time-stamp counter "
    "extension instruction.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMs({"-ms"},
    "  -ms                         \tCauses all variables to default to the .tiny section.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMs2600({"-ms2600"},
    "  -ms2600                     \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMsafeDma({"-msafe-dma"},
    "  -msafe-dma                  \ttell the compiler to treat the DMA instructions as potentially affecting "
    "all memory.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-munsafe-dma"), maplecl::kHide);

maplecl::Option<bool> oMsafeHints({"-msafe-hints"},
    "  -msafe-hints                \tWork around a hardware bug that causes the SPU to stall indefinitely. \n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMsahf({"-msahf"},
    "  -msahf                      \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMsatur({"-msatur"},
    "  -msatur                     \tEnables the saturation instructions. \n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMsaveAccInInterrupts({"-msave-acc-in-interrupts"},
    "  -msave-acc-in-interrupts    \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMsaveMducInInterrupts({"-msave-mduc-in-interrupts"},
    "  -msave-mduc-in-interrupts   \t\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-save-mduc-in-interrupts"), maplecl::kHide);

maplecl::Option<bool> oMsaveRestore({"-msave-restore"},
    "  -msave-restore              \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMsaveTocIndirect({"-msave-toc-indirect"},
    "  -msave-toc-indirect         \tGenerate code to save the TOC value in the reserved stack location in the "
    "function prologue if the function calls through a pointer on AIX and 64-bit Linux systems.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMscc({"-mscc"},
    "  -mscc                       \tEnable the use of conditional set instructions (default).\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-scc"), maplecl::kHide);

maplecl::Option<bool> oMschedArDataSpec({"-msched-ar-data-spec"},
    "  -msched-ar-data-spec        \t\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-sched-ar-data-spec"), maplecl::kHide);

maplecl::Option<bool> oMschedArInDataSpec({"-msched-ar-in-data-spec"},
    "  -msched-ar-in-data-spec     \t\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-sched-ar-in-data-spec"), maplecl::kHide);

maplecl::Option<bool> oMschedBrDataSpec({"-msched-br-data-spec"},
    "  -msched-br-data-spec        \t\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-sched-br-data-spec"), maplecl::kHide);

maplecl::Option<bool> oMschedBrInDataSpec({"-msched-br-in-data-spec"},
    "  -msched-br-in-data-spec     \t\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-sched-br-in-data-spec"), maplecl::kHide);

maplecl::Option<bool> oMschedControlSpec({"-msched-control-spec"},
    "  -msched-control-spec        \t\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-sched-control-spec"), maplecl::kHide);

maplecl::Option<bool> oMschedCountSpecInCriticalPath({"-msched-count-spec-in-critical-path"},
    "  -msched-count-spec-in-critical-path  \t\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-sched-count-spec-in-critical-path"), maplecl::kHide);

maplecl::Option<bool> oMschedFpMemDepsZeroCost({"-msched-fp-mem-deps-zero-cost"},
    "  -msched-fp-mem-deps-zero-cost  \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMschedInControlSpec({"-msched-in-control-spec"},
    "  -msched-in-control-spec     \t\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-sched-in-control-spec"), maplecl::kHide);

maplecl::Option<bool> oMschedMaxMemoryInsns({"-msched-max-memory-insns"},
    "  -msched-max-memory-insns    \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMschedMaxMemoryInsnsHardLimit({"-msched-max-memory-insns-hard-limit"},
    "  -msched-max-memory-insns-hard-limit  \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMschedPreferNonControlSpecInsns({"-msched-prefer-non-control-spec-insns"},
    "  -msched-prefer-non-control-spec-insns  \t\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-sched-prefer-non-control-spec-insns"), maplecl::kHide);

maplecl::Option<bool> oMschedPreferNonDataSpecInsns({"-msched-prefer-non-data-spec-insns"},
    "  -msched-prefer-non-data-spec-insns  \t\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-sched-prefer-non-data-spec-insns"), maplecl::kHide);

maplecl::Option<bool> oMschedSpecLdc({"-msched-spec-ldc"},
    "  -msched-spec-ldc            \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMschedStopBitsAfterEveryCycle({"-msched-stop-bits-after-every-cycle"},
    "  -msched-stop-bits-after-every-cycle  \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMscore5({"-mscore5"},
    "  -mscore5                    \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMscore5u({"-mscore5u"},
    "  -mscore5u                   \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMscore7({"-mscore7"},
    "  -mscore7                    \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMscore7d({"-mscore7d"},
    "  -mscore7d                   \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMsdata({"-msdata"},
    "  -msdata                     \tOn System V.4 and embedded PowerPC systems, if -meabi is used, compile code "
    "the same as -msdata=eabi, otherwise compile code the same as -msdata=sysv.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-sdata"), maplecl::kHide);

maplecl::Option<bool> oMsdataAll({"-msdata=all"},
    "  -msdata=all                 \tPut all data, not just small objects, into the sections reserved for small "
    "data, and use addressing relative to the B14 register to access them.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMsdataData({"-msdata=data"},
    "  -msdata=data                \tOn System V.4 and embedded PowerPC systems, put small global data in the "
    ".sdata section.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMsdataDefault({"-msdata=default"},
    "  -msdata=default             \tOn System V.4 and embedded PowerPC systems, if -meabi is used, compile code "
    "the same as -msdata=eabi, otherwise compile code the same as -msdata=sysv.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMsdataEabi({"-msdata=eabi"},
    "  -msdata=eabi                \tOn System V.4 and embedded PowerPC systems, put small initialized const "
    "global and static data in the .sdata2 section, which is pointed to by register r2. \n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMsdataNone({"-msdata=none"},
    "  -msdata=none                \tOn embedded PowerPC systems, put all initialized global and static data "
    "in the .data section, and all uninitialized data in the .bss section.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMsdataSdata({"-msdata=sdata"},
    "  -msdata=sdata               \tPut small global and static data in the small data area, but do not generate "
    "special code to reference them.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMsdataSysv({"-msdata=sysv"},
    "  -msdata=sysv                \tOn System V.4 and embedded PowerPC systems, put small global and static data "
    "in the .sdata section, which is pointed to by register r13.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMsdataUse({"-msdata=use"},
    "  -msdata=use                 \tPut small global and static data in the small data area, and generate special "
    "instructions to reference them.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMsdram({"-msdram"},
    "  -msdram                     \tLink the SDRAM-based runtime instead of the default ROM-based runtime.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMsecurePlt({"-msecure-plt"},
    "  -msecure-plt                \tenerate code that allows ld and ld.so to build executables and shared libraries "
    "with non-executable .plt and .got sections. \n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMselSchedDontCheckControlSpec({"-msel-sched-dont-check-control-spec"},
    "  -msel-sched-dont-check-control-spec  \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMsepData({"-msep-data"},
    "  -msep-data                  \tGenerate code that allows the data segment to be located in a different area "
    "of memory from the text segment.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-sep-data"), maplecl::kHide);

maplecl::Option<bool> oMserializeVolatile({"-mserialize-volatile"},
    "  -mserialize-volatile        \t\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-serialize-volatile"), maplecl::kHide);

maplecl::Option<bool> oMsetlb({"-msetlb"},
    "  -msetlb                     \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMsha({"-msha"},
    "  -msha                       \tThese switches enable the use of instructions in the msha.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMshort({"-mshort"},
    "  -mshort                     \tConsider type int to be 16 bits wide, like short int.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-short"), maplecl::kHide);

maplecl::Option<bool> oMsignExtendEnabled({"-msign-extend-enabled"},
    "  -msign-extend-enabled       \tEnable sign extend instructions.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMsim({"-msim"},
    "  -msim                       \tLink the simulator run-time libraries.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-sim"), maplecl::kHide);

maplecl::Option<bool> oMsimd({"-msimd"},
    "  -msimd                      \tEnable generation of ARC SIMD instructions via target-specific builtins. "
    "Only valid for -mcpu=ARC700.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMsimnovec({"-msimnovec"},
    "  -msimnovec                  \tLink the simulator runtime libraries, excluding built-in support for reset "
    "and exception vectors and tables.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMsimpleFpu({"-msimple-fpu"},
    "  -msimple-fpu                \tDo not generate sqrt and div instructions for hardware floating-point unit.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMsingleExit({"-msingle-exit"},
    "  -msingle-exit               \tForce generated code to have a single exit point in each function.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-single-exit"), maplecl::kHide);

maplecl::Option<bool> oMsingleFloat({"-msingle-float"},
    "  -msingle-float              \tGenerate code for single-precision floating-point operations. \n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMsinglePicBase({"-msingle-pic-base"},
    "  -msingle-pic-base           \tTreat the register used for PIC addressing as read-only, rather than loading "
    "it in the prologue for each function.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMsio({"-msio"},
    "  -msio                       \tGenerate the predefine, _SIO, for server IO. \n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMskipRaxSetup({"-mskip-rax-setup"},
    "  -mskip-rax-setup            \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMslowBytes({"-mslow-bytes"},
    "  -mslow-bytes                \t\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-slow-bytes"), maplecl::kHide);

maplecl::Option<bool> oMslowFlashData({"-mslow-flash-data"},
    "  -mslow-flash-data           \tAssume loading data from flash is slower than fetching instruction.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMsmall({"-msmall"},
    "  -msmall                     \tUse small-model addressing (16-bit pointers, 16-bit size_t).\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMsmallData({"-msmall-data"},
    "  -msmall-data                \tWhen -msmall-data is used, objects 8 bytes long or smaller are placed in a small"
    " data area (the .sdata and .sbss sections) and are accessed via 16-bit relocations off of the $gp register. \n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMsmallDataLimit({"-msmall-data-limit"},
    "  -msmall-data-limit          \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMsmallDivides({"-msmall-divides"},
    "  -msmall-divides             \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMsmallExec({"-msmall-exec"},
    "  -msmall-exec                \tGenerate code using the bras instruction to do subroutine calls. \n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-small-exec"), maplecl::kHide);

maplecl::Option<bool> oMsmallMem({"-msmall-mem"},
    "  -msmall-mem                 \tBy default, Maple generates code assuming that addresses are never larger "
    "than 18 bits.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMsmallModel({"-msmall-model"},
    "  -msmall-model               \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMsmallText({"-msmall-text"},
    "  -msmall-text                \tWhen -msmall-text is used, the compiler assumes that the code of the entire"
    " program (or shared library) fits in 4MB, and is thus reachable with a branch instruction. \n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMsmall16({"-msmall16"},
    "  -msmall16                   \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMsmallc({"-msmallc"},
    "  -msmallc                    \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMsmartmips({"-msmartmips"},
    "  -msmartmips                 \tUse the MIPS SmartMIPS ASE.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-smartmips"), maplecl::kHide);

maplecl::Option<bool> oMsoftFloat({"-msoft-float"},
    "  -msoft-float                \tThis option ignored; it is provided for compatibility purposes only. \n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-soft-float"), maplecl::kHide);

maplecl::Option<bool> oMsoftQuadFloat({"-msoft-quad-float"},
    "  -msoft-quad-float           \tGenerate output containing library calls for quad-word (long double) "
    "floating-point instructions.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMsoftStack({"-msoft-stack"},
    "  -msoft-stack                \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMsp8({"-msp8"},
    "  -msp8                       \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMspace({"-mspace"},
    "  -mspace                     \tTry to make the code as small as possible.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMspecldAnomaly({"-mspecld-anomaly"},
    "  -mspecld-anomaly            \tWhen enabled, the compiler ensures that the generated code does not contain "
    "speculative loads after jump instructions.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-specld-anomaly"), maplecl::kHide);

maplecl::Option<bool> oMspfp({"-mspfp"},
    "  -mspfp                      \tGenerate single-precision FPX instructions, tuned for the compact "
    "implementation.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMspfpCompact({"-mspfp-compact"},
    "  -mspfp-compact              \tGenerate single-precision FPX instructions, tuned for the compact "
    "implementation.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMspfpFast({"-mspfp-fast"},
    "  -mspfp-fast                 \tGenerate single-precision FPX instructions, tuned for the fast implementation.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMspfp_compact({"-mspfp_compact"},
    "  -mspfp_compact              \tReplaced by -mspfp-compact.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMspfp_fast({"-mspfp_fast"},
    "  -mspfp_fast                 \tReplaced by -mspfp-fast.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMsplitAddresses({"-msplit-addresses"},
    "  -msplit-addresses           \tEnable (disable) use of the %hi() and %lo() assembler relocation operators.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-split-addresses"), maplecl::kHide);

maplecl::Option<bool> oMsplitVecmoveEarly({"-msplit-vecmove-early"},
    "  -msplit-vecmove-early       \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMsse({"-msse"},
    "  -msse                       \tThese switches enable the use of instructions in the msse.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMsse2({"-msse2"},
    "  -msse2                      \tThese switches enable the use of instructions in the msse2.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMsse2avx({"-msse2avx"},
    "  -msse2avx                   \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMsse3({"-msse3"},
    "  -msse3                      \tThese switches enable the use of instructions in the msse2avx.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMsse4({"-msse4"},
    "  -msse4                      \tThese switches enable the use of instructions in the msse4.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMsse41({"-msse4.1"},
    "  -msse4.1                    \tThese switches enable the use of instructions in the msse4.1.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMsse42({"-msse4.2"},
    "  -msse4.2                    \tThese switches enable the use of instructions in the msse4.2.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMsse4a({"-msse4a"},
    "  -msse4a                     \tThese switches enable the use of instructions in the msse4a.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMsseregparm({"-msseregparm"},
    "  -msseregparm                \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMssse3({"-mssse3"},
    "  -mssse3                     \tThese switches enable the use of instructions in the mssse3.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMstackAlign({"-mstack-align"},
    "  -mstack-align               \t\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-stack-align"), maplecl::kHide);

maplecl::Option<bool> oMstackBias({"-mstack-bias"},
    "  -mstack-bias                \tWith -mstack-bias, GCC assumes that the stack pointer, and frame pointer if "
    "present, are offset by -2047 which must be added back when making stack frame references.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-stack-bias"), maplecl::kHide);

maplecl::Option<bool> oMstackCheckL1({"-mstack-check-l1"},
    "  -mstack-check-l1            \tDo stack checking using information placed into L1 scratchpad memory by the "
    "uClinux kernel.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMstackIncrement({"-mstack-increment"},
    "  -mstack-increment           \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMstackOffset({"-mstack-offset"},
    "  -mstack-offset              \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMstackrealign({"-mstackrealign"},
    "  -mstackrealign              \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMstdStructReturn({"-mstd-struct-return"},
    "  -mstd-struct-return         \tWith -mstd-struct-return, the compiler generates checking code in functions "
    "returning structures or unions to detect size mismatches between the two sides of function calls, as per the "
    "32-bit ABI.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-std-struct-return"), maplecl::kHide);

maplecl::Option<bool> oMstdmain({"-mstdmain"},
    "  -mstdmain                   \tWith -mstdmain, Maple links your program against startup code that assumes a "
    "C99-style interface to main, including a local copy of argv strings.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMstrictAlign({"-mstrict-align"},
    "  -mstrict-align              \tDon't assume that unaligned accesses are handled by the system.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-mno-strict-align"), maplecl::kHide);

maplecl::Option<bool> oMstrictX({"-mstrict-X"},
    "  -mstrict-X                  \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMstring({"-mstring"},
    "  -mstring                    \t\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-string"), maplecl::kHide);

maplecl::Option<bool> oMstringopStrategyAlg({"-mstringop-strategy=alg"},
    "  -mstringop-strategy=alg     \tOverride the internal decision heuristic for the particular algorithm to use "
    "for inlining string operations. \n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMstructureSizeBoundary({"-mstructure-size-boundary"},
    "  -mstructure-size-boundary   \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMsubxc({"-msubxc"},
    "  -msubxc                     \tWith -msubxc, Maple generates code that takes advantage of the UltraSPARC "
    "Subtract-Extended-with-Carry instruction. \n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-subxc"), maplecl::kHide);

maplecl::Option<bool> oMsvMode({"-msv-mode"},
    "  -msv-mode                   \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMsvr4StructReturn({"-msvr4-struct-return"},
    "  -msvr4-struct-return        \tReturn structures smaller than 8 bytes in registers (as specified by the "
    "SVR4 ABI).\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMswap({"-mswap"},
    "  -mswap                      \tGenerate swap instructions.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMswape({"-mswape"},
    "  -mswape                     \tPassed down to the assembler to enable the swap byte ordering extension "
    "instruction.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMsym32({"-msym32"},
    "  -msym32                     \tAssume that all symbols have 32-bit values, regardless of the selected ABI.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-sym32"), maplecl::kHide);

maplecl::Option<bool> oMsynci({"-msynci"},
    "  -msynci                     \tEnable (disable) generation of synci instructions on architectures that "
    "support it.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-mno-synci"), maplecl::kHide);

maplecl::Option<bool> oMsysCrt0({"-msys-crt0"},
    "  -msys-crt0                  \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMsysLib({"-msys-lib"},
    "  -msys-lib                   \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMtargetAlign({"-mtarget-align"},
    "  -mtarget-align              \t\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-target-align"), maplecl::kHide);

maplecl::Option<bool> oMtas({"-mtas"},
    "  -mtas                       \tGenerate the tas.b opcode for __atomic_test_and_set.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMtbm({"-mtbm"},
    "  -mtbm                       \tThese switches enable the use of instructions in the mtbm.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMtelephony({"-mtelephony"},
    "  -mtelephony                 \tPassed down to the assembler to enable dual- and single-operand instructions "
    "for telephony. \n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMtextSectionLiterals({"-mtext-section-literals"},
    "  -mtext-section-literals     \t\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-text-section-literals"), maplecl::kHide);

maplecl::Option<bool> oMtf({"-mtf"},
    "  -mtf                        \tCauses all functions to default to the .far section.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMthread({"-mthread"},
    "  -mthread                    \tThis option is available for MinGW targets.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMthreads({"-mthreads"},
    "  -mthreads                   \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMthumb({"-mthumb"},
    "  -mthumb                     \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMthumbInterwork({"-mthumb-interwork"},
    "  -mthumb-interwork           \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMtinyStack({"-mtiny-stack"},
    "  -mtiny-stack                \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> usePipe({"-pipe"},
    "  -pipe                       \tUse pipes between commands, when possible.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> fDataSections({"-fdata-sections"},
    "  -fdata-sections             \tPlace each data in its own section (ELF Only).\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> fStrongEvalOrder({"-fstrong-eval-order"},
    "  -fstrong-eval-order         \tFollow the C++17 evaluation order requirements for assignment expressions,"
    " shift, member function calls, etc.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

}