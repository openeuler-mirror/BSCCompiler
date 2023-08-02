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

#include "triple.h"
#include "driver_options.h"

namespace opts {

const maplecl::Option<bool> bigendian({"-Be", "--Be", "--BigEndian", "-be", "--be", "-mbig-endian"},
    "  --BigEndian/-Be             \tUsing BigEndian\n"
    "  --no-BigEndian              \tUsing LittleEndian\n",
    {driverCategory, hir2mplCategory, dex2mplCategory, ipaCategory}, kOptMaple, maplecl::DisableWith("--no-BigEndian"));

const maplecl::Option<bool> ilp32({"--ilp32", "-ilp32", "--arm64-ilp32"},
    "  --ilp32                     \tArm64 with a 32-bit ABI instead of a 64bit ABI\n",
    {driverCategory, hir2mplCategory, dex2mplCategory, ipaCategory}, kOptMaple);

const maplecl::Option<std::string> mabi({"-mabi"},
    "  -mabi=<abi>                 \tSpecify integer and floating-point calling convention\n",
    {driverCategory, hir2mplCategory, dex2mplCategory, ipaCategory}, kOptMaple, maplecl::kHide);

maplecl::Option<bool> oM68030({"-m68030"},
    "  -m68030                     \tGenerate output for a 68030. \n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oM68040({"-m68040"},
    "  -m68040                     \tGenerate output for a 68040.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oM68060({"-m68060"},
    "  -m68060                     \tGenerate output for a 68060.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oM68881({"-m68881"},
    "  -m68881                     \tGenerate floating-point instructions. \n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oM8Bit({"-m8-bit"},
    "  -m8-bit                     \tArrange for stack frame, writable data and constants to all 8-bit aligned\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oM8bitIdiv({"-m8bit-idiv"},
    "  -m8bit-idiv                 \tThis option generates a run-time check\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oM8byteAlign({"-m8byte-align"},
    "  -m8byte-align               \tEnables support for double and long long types to be aligned on "
    "8-byte boundaries.\n",
    {driverCategory, unSupCategory}, maplecl::kHide, maplecl::DisableWith("-mno-8byte-align"));

maplecl::Option<bool> oM96bitLongDouble({"-m96bit-long-double"},
    "  -m96bit-long-double         \tControl the size of long double type\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMA6({"-mA6"},
    "  -mA6                        \tCompile for ARC600.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMA7({"-mA7"},
    "  -mA7                        \tCompile for ARC700.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMabicalls({"-mabicalls"},
    "  -mabicalls                  \tGenerate (do not generate) code that is suitable for SVR4-style dynamic "
    "objects. -mabicalls is the default for SVR4-based systems.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-mno-abicalls"), maplecl::kHide);

maplecl::Option<bool> oMabm({"-mabm"},
    "  -mabm                       \tThese switches enable the use of instructions in the mabm.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMabortOnNoreturn({"-mabort-on-noreturn"},
    "  -mabort-on-noreturn         \tGenerate a call to the function abort at the end of a noreturn function.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMabs2008({"-mabs=2008"},
    "  -mabs=2008                  \tThe option selects the IEEE 754-2008 treatment\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMabsLegacy({"-mabs=legacy"},
    "  -mabs=legacy                \tThe legacy treatment is selected\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMabsdata({"-mabsdata"},
    "  -mabsdata                   \tAssume that all data in static storage can be accessed by LDS/STS instructions.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMabsdiff({"-mabsdiff"},
    "  -mabsdiff                   \tEnables the abs instruction, which is the absolute difference between "
    "two registers.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMabshi({"-mabshi"},
    "  -mabshi                     \tUse abshi2 pattern. This is the default.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-mno-abshi"), maplecl::kHide);

maplecl::Option<bool> oMac0({"-mac0"},
    "  -mac0                       \tReturn floating-point results in ac0 (fr0 in Unix assembler syntax).\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-mno-ac0"), maplecl::kHide);

maplecl::Option<bool> oMacc4({"-macc-4"},
    "  -macc-4                     \tUse only the first four media accumulator registers.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMacc8({"-macc-8"},
    "  -macc-8                     \tUse all eight media accumulator registers.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMaccumulateArgs({"-maccumulate-args"},
    "  -maccumulate-args           \tAccumulate outgoing function arguments and acquire/release the needed stack "
    "space for outgoing function arguments once in function prologue/epilogue.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMaccumulateOutgoingArgs({"-maccumulate-outgoing-args"},
    "  -maccumulate-outgoing-args  \tReserve space once for outgoing arguments in the function prologue rather "
    "than around each call. Generally beneficial for performance and size\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMaddressModeLong({"-maddress-mode=long"},
    "  -maddress-mode=long         \tGenerate code for long address mode.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMaddressModeShort({"-maddress-mode=short"},
    "  -maddress-mode=short        \tGenerate code for short address mode.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMaddressSpaceConversion({"-maddress-space-conversion"},
    "  -maddress-space-conversion  \tAllow/disallow treating the __ea address space as superset of the generic "
    "address space. \n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-mno-address-space-conversion"), maplecl::kHide);

maplecl::Option<bool> oMads({"-mads"},
    "  -mads                       \tOn embedded PowerPC systems, assume that the startup module is called "
    "crt0.o and the standard C libraries are libads.a and libc.a.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMaes({"-maes"},
    "  -maes                       \tThese switches enable the use of instructions in the maes.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMaixStructReturn({"-maix-struct-return"},
    "  -maix-struct-return         \tReturn all structures in memory (as specified by the AIX ABI).\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMaix32({"-maix32"},
    "  -maix32                     \tEnable 64-bit AIX ABI and calling convention: 32-bit pointers, 32-bit long "
    "type, and the infrastructure needed to support them.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMaix64({"-maix64"},
    "  -maix64                     \tEnable 64-bit AIX ABI and calling convention: 64-bit pointers, 64-bit "
    "long type, and the infrastructure needed to support them.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMalign300({"-malign-300"},
    "  -malign-300                 \tOn the H8/300H and H8S, use the same alignment rules as for the H8/300\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMalignCall({"-malign-call"},
    "  -malign-call                \tDo alignment optimizations for call instructions.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<std::string> oMalignData({"-malign-data"},
    "  -malign-data                \tControl how GCC aligns variables. \n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMalignDouble({"-malign-double"},
    "  -malign-double              \tControl whether GCC aligns double, long double, and long long variables "
    "on a two-word boundary or a one-word boundary.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-mno-align-double"), maplecl::kHide);

maplecl::Option<bool> oMalignInt({"-malign-int"},
    "  -malign-int                 \tAligning variables on 32-bit boundaries produces code that runs somewhat "
    "faster on processors with 32-bit busses at the expense of more memory.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-mno-align-int"), maplecl::kHide);

maplecl::Option<bool> oMalignLabels({"-malign-labels"},
    "  -malign-labels              \tTry to align labels to an 8-byte boundary by inserting NOPs into the "
    "previous packet.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMalignLoops({"-malign-loops"},
    "  -malign-loops               \tAlign all loops to a 32-byte boundary.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-mno-align-loops"), maplecl::kHide);

maplecl::Option<bool> oMalignNatural({"-malign-natural"},
    "  -malign-natural             \tThe option -malign-natural overrides the ABI-defined alignment of larger types\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMalignPower({"-malign-power"},
    "  -malign-power             \tThe option -malign-power instructs Maple to follow the ABI-specified alignment "
    "rules.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMallOpts({"-mall-opts"},
    "  -mall-opts                  \tEnables all the optional instructions—average, multiply, divide, bit "
    "operations, leading zero, absolute difference, min/max, clip, and saturation.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-opts"), maplecl::kHide);

maplecl::Option<bool> oMallocCc({"-malloc-cc"},
    "  -malloc-cc                  \tDynamically allocate condition code registers.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMallowStringInsns({"-mallow-string-insns"},
    "  -mallow-string-insns        \tEnables or disables the use of the string manipulation instructions SMOVF, "
    "SCMPU, SMOVB, SMOVU, SUNTIL SWHILE and also the RMPA instruction.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-mno-allow-string-insns"), maplecl::kHide);

maplecl::Option<bool> oMallregs({"-mallregs"},
    "  -mallregs                   \tAllow the compiler to use all of the available registers.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMaltivec({"-maltivec"},
    "  -maltivec                   \tGenerate code that uses (does not use) AltiVec instructions, and also "
    "enable the use of built-in functions that allow more direct access to the AltiVec instruction set. \n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-mno-altivec"), maplecl::kHide);

maplecl::Option<bool> oMaltivecBe({"-maltivec=be"},
    "  -maltivec=be                \tGenerate AltiVec instructions using big-endian element order, regardless "
    "of whether the target is big- or little-endian. \n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMaltivecLe({"-maltivec=le"},
    "  -maltivec=le                \tGenerate AltiVec instructions using little-endian element order,"
    " regardless of whether the target is big- or little-endian.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMam33({"-mam33"},
    "  -mam33                      \tGenerate code using features specific to the AM33 processor.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-mno-am33"), maplecl::kHide);

maplecl::Option<bool> oMam332({"-mam33-2"},
    "  -mam33-2                    \tGenerate code using features specific to the AM33/2.0 processor.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMam34({"-mam34"},
    "  -mam34                      \tGenerate code using features specific to the AM34 processor.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMandroid({"-mandroid"},
    "  -mandroid                   \tCompile code compatible with Android platform.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMannotateAlign({"-mannotate-align"},
    "  -mannotate-align            \tExplain what alignment considerations lead to the decision to make an "
    "instruction short or long.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMapcs({"-mapcs"},
    "  -mapcs                      \tThis is a synonym for -mapcs-frame and is deprecated.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMapcsFrame({"-mapcs-frame"},
    "  -mapcs-frame                \tGenerate a stack frame that is compliant with the ARM Procedure Call Standard "
    "for all functions, even if this is not strictly necessary for correct execution of the code. \n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMappRegs({"-mapp-regs"},
    "  -mapp-regs                  \tSpecify -mapp-regs to generate output using the global registers 2 through 4, "
    "which the SPARC SVR4 ABI reserves for applications. \n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-mno-app-regs"), maplecl::kHide);

maplecl::Option<bool> oMARC600({"-mARC600"},
    "  -mARC600                    \tCompile for ARC600.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMARC601({"-mARC601"},
    "  -mARC601                    \tCompile for ARC601.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMARC700({"-mARC700"},
    "  -mARC700                    \tCompile for ARC700.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMarclinux({"-marclinux"},
    "  -marclinux                  \tPassed through to the linker, to specify use of the arclinux emulation.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMarclinux_prof({"-marclinux_prof"},
    "  -marclinux_prof             \tPassed through to the linker, to specify use of the arclinux_prof emulation. \n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMargonaut({"-margonaut"},
    "  -margonaut                  \tObsolete FPX.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMarm({"-marm"},
    "  -marm                       \tSelect between generating code that executes in ARM and Thumb states. \n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMas100Syntax({"-mas100-syntax"},
    "  -mas100-syntax              \tWhen generating assembler output use a syntax that is compatible with "
    "Renesas's AS100 assembler. \n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-mno-as100-syntax"), maplecl::kHide);

maplecl::Option<bool> oMasmHex({"-masm-hex"},
    "  -masm-hex                   \tForce assembly output to always use hex constants.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMasmSyntaxUnified({"-masm-syntax-unified"},
    "  -masm-syntax-unified        \tAssume inline assembler is using unified asm syntax. \n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMasmDialect({"-masm=dialect"},
    "  -masm=dialect               \tOutput assembly instructions using selected dialect. \n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMatomic({"-matomic"},
    "  -matomic                    \tThis enables use of the locked load/store conditional extension to implement "
    "atomic memory built-in functions.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<std::string> oMatomicModel({"-matomic-model"},
    "  -matomic-model              \tSets the model of atomic operations and additional parameters as a comma "
    "separated list. \n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMatomicUpdates({"-matomic-updates"},
    "  -matomic-updates            \tThis option controls the version of libgcc that the compiler links to "
    "an executable and selects whether atomic updates to the software-managed cache of PPU-side variables are used.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-mno-atomic-updates"), maplecl::kHide);

maplecl::Option<bool> oMautoLitpools({"-mauto-litpools"},
    "  -mauto-litpools             \tControl the treatment of literal pools.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-mno-auto-litpools"), maplecl::kHide);

maplecl::Option<bool> oMautoModifyReg({"-mauto-modify-reg"},
    "  -mauto-modify-reg           \tEnable the use of pre/post modify with register displacement.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMautoPic({"-mauto-pic"},
    "  -mauto-pic                  \tGenerate code that is self-relocatable. \n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMaverage({"-maverage"},
    "  -maverage                   \tEnables the ave instruction, which computes the average of two registers.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMavoidIndexedAddresses({"-mavoid-indexed-addresses"},
    "  -mavoid-indexed-addresses   \tGenerate code that tries to avoid (not avoid) the use of indexed load or "
    "store instructions.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-mno-avoid-indexed-addresses"), maplecl::kHide);

maplecl::Option<bool> oMavx({"-mavx"},
    "  -mavx                       \tMaple depresses SSEx instructions when -mavx is used. \n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMavx2({"-mavx2"},
    "  -mavx2                      \tEnable the use of instructions in the mavx2.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMavx256SplitUnalignedLoad({"-mavx256-split-unaligned-load"},
    "  -mavx256-split-unaligned-load  \tSplit 32-byte AVX unaligned load .\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMavx256SplitUnalignedStore({"-mavx256-split-unaligned-store"},
    "  -mavx256-split-unaligned-store  \tSplit 32-byte AVX unaligned store.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMavx512bw({"-mavx512bw"},
    "  -mavx512bw                  \tEnable the use of instructions in the mavx512bw.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMavx512cd({"-mavx512cd"},
    "  -mavx512cd                  \tEnable the use of instructions in the mavx512cd.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMavx512dq({"-mavx512dq"},
    "  -mavx512dq                  \tEnable the use of instructions in the mavx512dq.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMavx512er({"-mavx512er"},
    "  -mavx512er                  \tEnable the use of instructions in the mavx512er.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMavx512f({"-mavx512f"},
    "  -mavx512f                   \tEnable the use of instructions in the mavx512f.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMavx512ifma({"-mavx512ifma"},
    "  -mavx512ifma                \tEnable the use of instructions in the mavx512ifma.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMavx512pf({"-mavx512pf"},
    "  -mavx512pf                  \tEnable the use of instructions in the mavx512pf.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMavx512vbmi({"-mavx512vbmi"},
    "  -mavx512vbmi                \tEnable the use of instructions in the mavx512vbmi.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMavx512vl({"-mavx512vl"},
    "  -mavx512vl                  \tEnable the use of instructions in the mavx512vl.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<std::string> oMaxVectAlign({"-max-vect-align"},
    "  -max-vect-align             \tThe maximum alignment for SIMD vector mode types.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMb({"-mb"},
    "  -mb                         \tCompile code for the processor in big-endian mode.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMbackchain({"-mbackchain"},
    "  -mbackchain                 \tStore (do not store) the address of the caller's frame as backchain "
    "pointer into the callee's stack frame.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-mno-backchain"), maplecl::kHide);

maplecl::Option<bool> oMbarrelShiftEnabled({"-mbarrel-shift-enabled"},
    "  -mbarrel-shift-enabled      \tEnable barrel-shift instructions.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMbarrelShifter({"-mbarrel-shifter"},
    "  -mbarrel-shifter            \tGenerate instructions supported by barrel shifter.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMbarrel_shifter({"-mbarrel_shifter"},
    "  -mbarrel_shifter            \tReplaced by -mbarrel-shifter.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMbaseAddresses({"-mbase-addresses"},
    "  -mbase-addresses            \tGenerate code that uses base addresses.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-base-addresses"), maplecl::kHide);

maplecl::Option<std::string> oMbased({"-mbased"},
    "  -mbased                     \tVariables of size n bytes or smaller are placed in the .based "
    "section by default\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMbbitPeephole({"-mbbit-peephole"},
    "  -mbbit-peephole             \tEnable bbit peephole2.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMbcopy({"-mbcopy"},
    "  -mbcopy                     \tDo not use inline movmemhi patterns for copying memory.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMbcopyBuiltin({"-mbcopy-builtin"},
    "  -mbcopy-builtin             \tUse inline movmemhi patterns for copying memory.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMbig({"-mbig"},
    "  -mbig                       \tCompile code for big-endian targets.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMbigEndianData({"-mbig-endian-data"},
    "  -mbig-endian-data           \tStore data (but not code) in the big-endian format.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMbigSwitch({"-mbig-switch"},
    "  -mbig-switch                \tGenerate code suitable for big switch tables.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMbigtable({"-mbigtable"},
    "  -mbigtable                  \tUse 32-bit offsets in switch tables.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMbionic({"-mbionic"},
    "  -mbionic                    \tUse Bionic C library.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMbitAlign({"-mbit-align"},
    "  -mbit-align                 \tOn System V.4 and embedded PowerPC systems do not force structures and "
    "unions that contain bit-fields to be aligned to the base type of the bit-field.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-bit-align"), maplecl::kHide);

maplecl::Option<bool> oMbitOps({"-mbit-ops"},
    "  -mbit-ops                   \tGenerates sbit/cbit instructions for bit manipulations.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMbitfield({"-mbitfield"},
    "  -mbitfield                  \tDo use the bit-field instructions.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-bitfield"), maplecl::kHide);

maplecl::Option<bool> oMbitops({"-mbitops"},
    "  -mbitops                    \tEnables the bit operation instructions—bit test (btstm), set (bsetm), "
    "clear (bclrm), invert (bnotm), and test-and-set (tas).\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<std::string> oMblockMoveInlineLimit({"-mblock-move-inline-limit"},
    "  -mblock-move-inline-limit   \tInline all block moves (such as calls to memcpy or structure copies) "
    "less than or equal to num bytes. \n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMbmi({"-mbmi"},
    "  -mbmi                       \tThese switches enable the use of instructions in the mbmi.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMbranchCheap({"-mbranch-cheap"},
    "  -mbranch-cheap              \tDo not pretend that branches are expensive.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<std::string> oMbranchCost({"-mbranch-cost"},
    "  -mbranch-cost               \tSet the cost of branches to roughly num 'simple' instructions.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMbranchExpensive({"-mbranch-expensive"},
    "  -mbranch-expensive          \tPretend that branches are expensive.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMbranchHints({"-mbranch-hints"},
    "  -mbranch-hints              \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMbranchLikely({"-mbranch-likely"},
    "  -mbranch-likely             \tEnable or disable use of Branch Likely instructions, regardless of "
    "the default for the selected architecture.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-branch-likely"), maplecl::kHide);

maplecl::Option<bool> oMbranchPredict({"-mbranch-predict"},
    "  -mbranch-predict            \tUse the probable-branch instructions, when static branch prediction indicates "
    "a probable branch.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-branch-predict"), maplecl::kHide);

maplecl::Option<bool> oMbssPlt({"-mbss-plt"},
    "  -mbss-plt                   \tGenerate code that uses a BSS .plt section that ld.so fills in, and requires "
    ".plt and .got sections that are both writable and executable.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMbuildConstants({"-mbuild-constants"},
    "  -mbuild-constants           \tConstruct all integer constants using code, even if it takes more "
    "instructions (the maximum is six).\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMbwx({"-mbwx"},
    "  -mbwx                       \tGenerate code to use the optional BWX instruction sets.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-bwx"), maplecl::kHide);

maplecl::Option<bool> oMbypassCache({"-mbypass-cache"},
    "  -mbypass-cache              \t\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-bypass-cache"), maplecl::kHide);

maplecl::Option<bool> oMc68000({"-mc68000"},
    "  -mc68000                    \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMc68020({"-mc68020"},
    "  -mc68020                    \t\n",
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

maplecl::Option<bool> oMcacheVolatile({"-mcache-volatile"},
    "  -mcache-volatile            \t\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-cache-volatile"), maplecl::kHide);

maplecl::Option<bool> oMcallEabi({"-mcall-eabi"},
    "  -mcall-eabi                 \tSpecify both -mcall-sysv and -meabi options.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMcallAixdesc({"-mcall-aixdesc"},
    "  -mcall-aixdesc              \tOn System V.4 and embedded PowerPC systems compile code for the AIX "
    "operating system.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMcallFreebsd({"-mcall-freebsd"},
    "  -mcall-freebsd              \tOn System V.4 and embedded PowerPC systems compile code for the FreeBSD "
    "operating system.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMcallLinux({"-mcall-linux"},
    "  -mcall-linux                \tOn System V.4 and embedded PowerPC systems compile code for the "
    "Linux-based GNU system.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMcallOpenbsd({"-mcall-openbsd"},
    "  -mcall-openbsd              \tOn System V.4 and embedded PowerPC systems compile code for the "
    "OpenBSD operating system.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMcallNetbsd({"-mcall-netbsd"},
    "  -mcall-netbsd               \tOn System V.4 and embedded PowerPC systems compile code for the "
    "NetBSD operating system.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMcallPrologues({"-mcall-prologues"},
    "  -mcall-prologues            \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMcallSysv({"-mcall-sysv"},
    "  -mcall-sysv                 \tOn System V.4 and embedded PowerPC systems compile code using calling conventions"
    " that adhere to the March 1995 draft of the System V Application Binary Interface, PowerPC processor "
    "supplement.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMcallSysvEabi({"-mcall-sysv-eabi"},
    "  -mcall-sysv-eabi            \tSpecify both -mcall-sysv and -meabi options.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMcallSysvNoeabi({"-mcall-sysv-noeabi"},
    "  -mcall-sysv-noeabi          \tSpecify both -mcall-sysv and -mno-eabi options.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMcalleeSuperInterworking({"-mcallee-super-interworking"},
    "  -mcallee-super-interworking \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMcallerCopies({"-mcaller-copies"},
    "  -mcaller-copies             \tThe caller copies function arguments passed by hidden reference.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMcallerSuperInterworking({"-mcaller-super-interworking"},
    "  -mcaller-super-interworking \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMcallgraphData({"-mcallgraph-data"},
    "  -mcallgraph-data            \t\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-callgraph-data"), maplecl::kHide);

maplecl::Option<bool> oMcaseVectorPcrel({"-mcase-vector-pcrel"},
    "  -mcase-vector-pcrel         \tUse PC-relative switch case tables to enable case table shortening. \n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMcbcond({"-mcbcond"},
    "  -mcbcond                    \tWith -mcbcond, Maple generates code that takes advantage of the UltraSPARC "
    "Compare-and-Branch-on-Condition instructions. \n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-cbcond"), maplecl::kHide);

maplecl::Option<bool> oMcbranchForceDelaySlot({"-mcbranch-force-delay-slot"},
    "  -mcbranch-force-delay-slot  \tForce the usage of delay slots for conditional branches, which stuffs the "
    "delay slot with a nop if a suitable instruction cannot be found. \n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMccInit({"-mcc-init"},
    "  -mcc-init                   \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMcfv4e({"-mcfv4e"},
    "  -mcfv4e                     \tGenerate output for a ColdFire V4e family CPU (e.g. 547x/548x).\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMcheckZeroDivision({"-mcheck-zero-division"},
    "  -mcheck-zero-division       \tTrap (do not trap) on integer division by zero.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-check-zero-division"), maplecl::kHide);

maplecl::Option<bool> oMcix({"-mcix"},
    "  -mcix                       \tGenerate code to use the optional CIX instruction sets.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-cix"), maplecl::kHide);

maplecl::Option<bool> oMcld({"-mcld"},
    "  -mcld                       \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMclearHwcap({"-mclear-hwcap"},
    "  -mclear-hwcap               \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMclflushopt({"-mclflushopt"},
    "  -mclflushopt                \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMclip({"-mclip"},
    "  -mclip                      \tEnables the clip instruction.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMclzero({"-mclzero"},
    "  -mclzero                    \tThese switches enable the use of instructions in the mclzero.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<std::string> oMcmodel({"-mcmodel"},
    "  -mcmodel                    \tSpecify the code model.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMcmov({"-mcmov"},
    "  -mcmov                      \tGenerate conditional move instructions.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-cmov"), maplecl::kHide);

maplecl::Option<bool> oMcmove({"-mcmove"},
    "  -mcmove                     \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMcmpb({"-mcmpb"},
    "  -mcmpb                      \tSpecify which instructions are available on the processor you are using. \n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-cmpb"), maplecl::kHide);

maplecl::Option<bool> oMcmse({"-mcmse"},
    "  -mcmse                      \tGenerate secure code as per the ARMv8-M Security Extensions.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMcodeDensity({"-mcode-density"},
    "  -mcode-density              \tEnable code density instructions for ARC EM. \n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<std::string> oMcodeReadable({"-mcode-readable"},
    "  -mcode-readable             \tSpecify whether Maple may generate code that reads from executable sections.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMcodeRegion({"-mcode-region"},
    "  -mcode-region               \tThe compiler where to place functions and data that do not have one of the "
    "lower, upper, either or section attributes.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMcompactBranchesAlways({"-mcompact-branches=always"},
    "  -mcompact-branches=always   \tThe -mcompact-branches=always option ensures that a compact branch instruction "
    "will be generated if available. \n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMcompactBranchesNever({"-mcompact-branches=never"},
    "  -mcompact-branches=never    \tThe -mcompact-branches=never option ensures that compact branch instructions "
    "will never be generated.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMcompactBranchesOptimal({"-mcompact-branches=optimal"},
    "  -mcompact-branches=optimal  \tThe -mcompact-branches=optimal option will cause a delay slot branch to be "
    "used if one is available in the current ISA and the delay slot is successfully filled.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMcompactCasesi({"-mcompact-casesi"},
    "  -mcompact-casesi            \tEnable compact casesi pattern.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMcompatAlignParm({"-mcompat-align-parm"},
    "  -mcompat-align-parm         \tGenerate code to pass structure parameters with a maximum alignment of 64 bits, "
    "for compatibility with older versions of maple.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMcondExec({"-mcond-exec"},
    "  -mcond-exec                 \tEnable the use of conditional execution (default).\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-mno-cond-exec"), maplecl::kHide);

maplecl::Option<bool> oMcondMove({"-mcond-move"},
    "  -mcond-move                 \tEnable the use of conditional-move instructions (default).\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-cond-move"), maplecl::kHide);

maplecl::Option<std::string> oMconfig({"-mconfig"},
    "  -mconfig                    \tSelects one of the built-in core configurations. \n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMconsole({"-mconsole"},
    "  -mconsole                   \tThis option specifies that a console application is to be generated, by "
    "instructing the linker to set the PE header subsystem type required for console applications.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMconstAlign({"-mconst-align"},
    "  -mconst-align               \t\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-const-align"), maplecl::kHide);

maplecl::Option<bool> oMconst16({"-mconst16"},
    "  -mconst16                   \t\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-const16"), maplecl::kHide);

maplecl::Option<bool> oMconstantGp({"-mconstant-gp"},
    "  -mconstant-gp               \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMcop({"-mcop"},
    "  -mcop                       \tEnables the coprocessor instructions. \n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMcop32({"-mcop32"},
    "  -mcop32                     \tEnables the 32-bit coprocessor's instructions.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMcop64({"-mcop64"},
    "  -mcop64                     \tEnables the 64-bit coprocessor's instructions.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMcorea({"-mcorea"},
    "  -mcorea                     \tBuild a standalone application for Core A of BF561 when using the "
    "one-application-per-core programming model.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMcoreb({"-mcoreb"},
    "  -mcoreb                     \tBuild a standalone application for Core B of BF561 when using the "
    "one-application-per-core programming model.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<std::string> oMcpu({"-mcpu"},
    "  -mcpu                       \tSpecify the name of the target processor, optionally suffixed by "
    "one or more feature modifiers.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMcpu32({"-mcpu32"},
    "  -mcpu32                     \tGenerate output for a CPU32.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMcr16c({"-mcr16c"},
    "  -mcr16c                     \tGenerate code for CR16C architecture.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMcr16cplus({"-mcr16cplus"},
    "  -mcr16cplus                 \tGenerate code for CR16C+ architecture.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMcrc32({"-mcrc32"},
    "  -mcrc32                     \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMcrypto({"-mcrypto"},
    "  -mcrypto                    \tEnable the use of the built-in functions that allow direct access to the "
    "cryptographic instructions that were added in version 2.07 of the PowerPC ISA.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-crypto"), maplecl::kHide);

maplecl::Option<bool> oMcsyncAnomaly({"-mcsync-anomaly"},
    "  -mcsync-anomaly             \tWhen enabled, the compiler ensures that the generated code does not contain "
    "CSYNC or SSYNC instructions too soon after conditional branches.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-csync-anomaly"), maplecl::kHide);

maplecl::Option<bool> oMctorDtor({"-mctor-dtor"},
    "  -mctor-dtor                 \tEnable constructor/destructor feature.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMcustomFpuCfg({"-mcustom-fpu-cfg"},
    "  -mcustom-fpu-cfg            \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMcustomInsn({"-mcustom-insn"},
    "  -mcustom-insn               \t\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-custom-insn"), maplecl::kHide);

maplecl::Option<bool> oMcx16({"-mcx16"},
    "  -mcx16                      \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMdalign({"-mdalign"},
    "  -mdalign                    \tAlign doubles at 64-bit boundaries.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMdataAlign({"-mdata-align"},
    "  -mdata-align                \t\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-data-align"), maplecl::kHide);

maplecl::Option<bool> oMdataModel({"-mdata-model"},
    "  -mdata-model                \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<std::string> oMdataRegion({"-mdata-region"},
    "  -mdata-region               \ttell the compiler where to place functions and data that do not have one "
    "of the lower, upper, either or section attributes.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMdc({"-mdc"},
    "  -mdc                        \tCauses constant variables to be placed in the .near section.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMdebug({"-mdebug"},
    "  -mdebug                     \tPrint additional debug information when compiling. \n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-debug"), maplecl::kHide);

maplecl::Option<bool> oMdebugMainPrefix({"-mdebug-main=prefix"},
    "  -mdebug-main=prefix         \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMdecAsm({"-mdec-asm"},
    "  -mdec-asm                   \tUse DEC assembler syntax. \n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMdirectMove({"-mdirect-move"},
    "  -mdirect-move               \tGenerate code that uses the instructions to move data between the general "
    "purpose registers and the vector/scalar (VSX) registers that were added in version 2.07 of the PowerPC ISA.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-direct-move"), maplecl::kHide);

maplecl::Option<bool> oMdisableCallt({"-mdisable-callt"},
    "  -mdisable-callt             \tThis option suppresses generation of the  CALLT instruction for the v850e, "
    "v850e1, v850e2, v850e2v3 and v850e3v5 flavors of the v850 architecture.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-disable-callt"), maplecl::kHide);

maplecl::Option<bool> oMdisableFpregs({"-mdisable-fpregs"},
    "  -mdisable-fpregs            \tPrevent floating-point registers from being used in any manner.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMdisableIndexing({"-mdisable-indexing"},
    "  -mdisable-indexing          \tPrevent the compiler from using indexing address modes.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMdiv({"-mdiv"},
    "  -mdiv                       \tEnables the div and divu instructions.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-div"), maplecl::kHide);

maplecl::Option<bool> oMdivRem({"-mdiv-rem"},
    "  -mdiv-rem                   \tEnable div and rem instructions for ARCv2 cores.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMdivStrategy({"-mdiv=strategy"},
    "  -mdiv=strategy              \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMdivideBreaks({"-mdivide-breaks"},
    "  -mdivide-breaks             \tMIPS systems check for division by zero by generating either a conditional "
    "trap or a break instruction.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMdivideEnabled({"-mdivide-enabled"},
    "  -mdivide-enabled            \tEnable divide and modulus instructions.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMdivideTraps({"-mdivide-traps"},
    "  -mdivide-traps              \tMIPS systems check for division by zero by generating either a conditional "
    "trap or a break instruction.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<std::string> oMdivsi3_libfuncName({"-mdivsi3_libfunc"},
    "  -mdivsi3_libfunc            \tSet the name of the library function used for 32-bit signed division to name.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMdll({"-mdll"},
    "  -mdll                       \tThis option is available for Cygwin and MinGW targets.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMdlmzb({"-mdlmzb"},
    "  -mdlmzb                     \tGenerate code that uses the string-search 'dlmzb' instruction on the IBM "
    "405, 440, 464 and 476 processors. \n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-dlmzb"), maplecl::kHide);

maplecl::Option<bool> oMdmx({"-mdmx"},
    "  -mdmx                       \tUse MIPS Digital Media Extension instructions.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-mdmx"), maplecl::kHide);

maplecl::Option<bool> oMdouble({"-mdouble"},
    "  -mdouble                    \tUse floating-point double instructions.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-double"), maplecl::kHide);

maplecl::Option<bool> oMdoubleFloat({"-mdouble-float"},
    "  -mdouble-float              \tGenerate code for double-precision floating-point operations.",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMdpfp({"-mdpfp"},
    "  -mdpfp                      \tGenerate double-precision FPX instructions, tuned for the compact "
    "implementation.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMdpfpCompact({"-mdpfp-compact"},
    "  -mdpfp-compact              \tGenerate double-precision FPX instructions, tuned for the compact "
    "implementation.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMdpfpFast({"-mdpfp-fast"},
    "  -mdpfp-fast                 \tGenerate double-precision FPX instructions, tuned for the fast implementation.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMdpfp_compact({"-mdpfp_compact"},
    "  -mdpfp_compact              \tReplaced by -mdpfp-compact.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMdpfp_fast({"-mdpfp_fast"},
    "  -mdpfp_fast                 \tReplaced by -mdpfp-fast.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMdsp({"-mdsp"},
    "  -mdsp                       \t\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-dsp"), maplecl::kHide);

maplecl::Option<bool> oMdspPacka({"-mdsp-packa"},
    "  -mdsp-packa                 \tPassed down to the assembler to enable the DSP Pack A extensions. \n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMdspr2({"-mdspr2"},
    "  -mdspr2                     \tUse revision 2 of the MIPS DSP ASE.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-dspr2"), maplecl::kHide);

maplecl::Option<bool> oMdsp_packa({"-mdsp_packa"},
    "  -mdsp_packa                 \tReplaced by -mdsp-packa.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMdualNops({"-mdual-nops"},
    "  -mdual-nops                 \tBy default, GCC inserts NOPs to increase dual issue when it expects it "
    "to increase performance.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<std::string> oMdualNopsE({"-mdual-nops="},
    "  -mdual-nops=                \tBy default, GCC inserts NOPs to increase dual issue when it expects it "
    "to increase performance.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMdumpTuneFeatures({"-mdump-tune-features"},
    "  -mdump-tune-features        \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMdvbf({"-mdvbf"},
    "  -mdvbf                      \tPassed down to the assembler to enable the dual Viterbi butterfly extension.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMdwarf2Asm({"-mdwarf2-asm"},
    "  -mdwarf2-asm                \t\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-dwarf2-asm"), maplecl::kHide);

maplecl::Option<bool> oMdword({"-mdword"},
    "  -mdword                     \tChange ABI to use double word insns.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-dword"), maplecl::kHide);

maplecl::Option<bool> oMdynamicNoPic({"-mdynamic-no-pic"},
    "  -mdynamic-no-pic            \tOn Darwin and Mac OS X systems, compile code so that it is not relocatable, "
    "but that its external references are relocatable. \n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMea({"-mea"},
    "  -mea                        \tGenerate extended arithmetic instructions.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMEa({"-mEa"},
    "  -mEa                        \tReplaced by -mea.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMea32({"-mea32"},
    "  -mea32                      \tCompile code assuming that pointers to the PPU address space accessed via "
    "the __ea named address space qualifier are either 32 bits wide. \n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMea64({"-mea64"},
    "  -mea64                      \tCompile code assuming that pointers to the PPU address space accessed "
    "via the __ea named address space qualifier are either 64 bits wide. \n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMeabi({"-meabi"},
    "  -meabi                      \tOn System V.4 and embedded PowerPC systems adhere to the Embedded "
    "Applications Binary Interface (EABI), which is a set of modifications to the System V.4 specifications. \n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-eabi"), maplecl::kHide);

maplecl::Option<bool> oMearlyCbranchsi({"-mearly-cbranchsi"},
    "  -mearly-cbranchsi           \tEnable pre-reload use of the cbranchsi pattern.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMearlyStopBits({"-mearly-stop-bits"},
    "  -mearly-stop-bits           \t\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-early-stop-bits"), maplecl::kHide);

maplecl::Option<bool> oMeb({"-meb"},
    "  -meb                        \tGenerate big-endian code.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMel({"-mel"},
    "  -mel                        \tGenerate little-endian code.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMelf({"-melf"},
    "  -melf                       \tGenerate an executable in the ELF format, rather than the default 'mmo' "
    "format used by the mmix simulator.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMemb({"-memb"},
    "  -memb                       \tOn embedded PowerPC systems, set the PPC_EMB bit in the ELF flags header to "
    "indicate that 'eabi' extended relocations are used.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMembeddedData({"-membedded-data"},
    "  -membedded-data             \tAllocate variables to the read-only data section first if possible, then "
    "next in the small data section if possible, otherwise in data.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-embedded-data"), maplecl::kHide);

maplecl::Option<std::string> oMemregsE({"-memregs="},
    "  -memregs=                   \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMep({"-mep"},
    "  -mep                        \tDo not optimize basic blocks that use the same index pointer 4 or more times "
    "to copy pointer into the ep register, and use the shorter sld and sst instructions. \n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-ep"), maplecl::kHide);

maplecl::Option<bool> oMepsilon({"-mepsilon"},
    "  -mepsilon                   \tGenerate floating-point comparison instructions that compare with respect "
    "to the rE epsilon register.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-epsilon"), maplecl::kHide);

maplecl::Option<bool> oMesa({"-mesa"},
    "  -mesa                       \tWhen -mesa is specified, generate code using the instructions available on "
    "ESA/390.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMetrax100({"-metrax100"},
    "  -metrax100                  \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMetrax4({"-metrax4"},
    "  -metrax4                    \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMeva({"-meva"},
    "  -meva                       \tUse the MIPS Enhanced Virtual Addressing instructions.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-eva"), maplecl::kHide);

maplecl::Option<bool> oMexpandAdddi({"-mexpand-adddi"},
    "  -mexpand-adddi              \tExpand adddi3 and subdi3 at RTL generation time into add.f, adc etc.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMexplicitRelocs({"-mexplicit-relocs"},
    "  -mexplicit-relocs           \tUse assembler relocation operators when dealing with symbolic addresses.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-explicit-relocs"), maplecl::kHide);

maplecl::Option<bool> oMexr({"-mexr"},
    "  -mexr                       \t\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-exr"), maplecl::kHide);

maplecl::Option<bool> oMexternSdata({"-mextern-sdata"},
    "  -mextern-sdata              \tAssume (do not assume) that externally-defined data is in a small data section "
    "if the size of that data is within the -G limit. -mextern-sdata is the default for all configurations.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-extern-sdata"), maplecl::kHide);

maplecl::Option<bool> oMf16c({"-mf16c"},
    "  -mf16c                      \tThese switches enable the use of instructions in the mf16c.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMfastFp({"-mfast-fp"},
    "  -mfast-fp                   \tLink with the fast floating-point library. \n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMfastIndirectCalls({"-mfast-indirect-calls"},
    "  -mfast-indirect-calls       \tGenerate code that assumes calls never cross space boundaries.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMfastSwDiv({"-mfast-sw-div"},
    "  -mfast-sw-div               \t\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-fast-sw-div"), maplecl::kHide);

maplecl::Option<bool> oMfasterStructs({"-mfaster-structs"},
    "  -mfaster-structs            \tWith -mfaster-structs, the compiler assumes that structures should have "
    "8-byte alignment.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-faster-structs"), maplecl::kHide);

maplecl::Option<bool> oMfdiv({"-mfdiv"},
    "  -mfdiv                      \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMfdpic({"-mfdpic"},
    "  -mfdpic                     \tSelect the FDPIC ABI, which uses function descriptors to represent pointers "
    "to functions.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMfentry({"-mfentry"},
    "  -mfentry                    \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMfix({"-mfix"},
    "  -mfix                       \tGenerate code to use the optional FIX instruction sets.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-fix"), maplecl::kHide);

maplecl::Option<bool> oMfix24k({"-mfix-24k"},
    "  -mfix-24k                   \tWork around the 24K E48 (lost data on stores during refill) errata. \n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-fix-24k"), maplecl::kHide);

maplecl::Option<bool> oMfixAndContinue({"-mfix-and-continue"},
    "  -mfix-and-continue          \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMfixAt697f({"-mfix-at697f"},
    "  -mfix-at697f                \tEnable the documented workaround for the single erratum of the Atmel AT697F "
    "processor (which corresponds to erratum #13 of the AT697E processor).\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMfixCortexA53835769({"-mfix-cortex-a53-835769"},
    "  -mfix-cortex-a53-835769     \tWorkaround for ARM Cortex-A53 Erratum number 835769.\n",
    {driverCategory, ldCategory}, kOptLd, maplecl::DisableWith("-mno-fix-cortex-a53-835769"));

maplecl::Option<bool> oMfixCortexA53843419({"-mfix-cortex-a53-843419"},
    "  -mfix-cortex-a53-843419     \tWorkaround for ARM Cortex-A53 Erratum number 843419.\n",
    {driverCategory, ldCategory}, kOptLd, maplecl::DisableWith("-mno-fix-cortex-a53-843419"));

maplecl::Option<bool> oMfixCortexM3Ldrd({"-mfix-cortex-m3-ldrd"},
    "  -mfix-cortex-m3-ldrd        \tSome Cortex-M3 cores can cause data corruption when ldrd instructions with "
    "overlapping destination and base registers are used. \n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMfixGr712rc({"-mfix-gr712rc"},
    "  -mfix-gr712rc               \tEnable the documented workaround for the back-to-back store errata of "
    "the GR712RC processor.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMfixR10000({"-mfix-r10000"},
    "  -mfix-r10000                \tWork around certain R10000 errata\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-fix-r10000"), maplecl::kHide);

maplecl::Option<bool> oMfixR4000({"-mfix-r4000"},
    "  -mfix-r4000                 \tWork around certain R4000 CPU errata\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-fix-r4000"), maplecl::kHide);

maplecl::Option<bool> oMfixR4400({"-mfix-r4400"},
    "  -mfix-r4400                 \tWork around certain R4400 CPU errata\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-fix-r4400"), maplecl::kHide);

maplecl::Option<bool> oMfixRm7000({"-mfix-rm7000"},
    "  -mfix-rm7000                \tWork around the RM7000 dmult/dmultu errata.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-mno-fix-rm7000"), maplecl::kHide);

maplecl::Option<bool> oMfixSb1({"-mfix-sb1"},
    "  -mfix-sb1                   \tWork around certain SB-1 CPU core errata.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-mno-fix-sb1"), maplecl::kHide);

maplecl::Option<bool> oMfixUt699({"-mfix-ut699"},
    "  -mfix-ut699                 \tEnable the documented workarounds for the floating-point errata and the data "
    "cache nullify errata of the UT699 processor.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMfixUt700({"-mfix-ut700"},
    "  -mfix-ut700                 \tEnable the documented workaround for the back-to-back store errata of the "
    "UT699E/UT700 processor.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMfixVr4120({"-mfix-vr4120"},
    "  -mfix-vr4120                \tWork around certain VR4120 errata\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-mno-fix-vr4120"), maplecl::kHide);

maplecl::Option<bool> oMfixVr4130({"-mfix-vr4130"},
    "  -mfix-vr4130                \tWork around the VR4130 mflo/mfhi errata.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMfixedCc({"-mfixed-cc"},
    "  -mfixed-cc                  \tDo not try to dynamically allocate condition code registers, only use "
    "icc0 and fcc0.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<std::string> oMfixedRange({"-mfixed-range"},
    "  -mfixed-range               \tGenerate code treating the given register range as fixed registers.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMflat({"-mflat"},
    "  -mflat                      \tWith -mflat, the compiler does not generate save/restore instructions and "
    "uses a 'flat' or single register window model.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-flat"), maplecl::kHide);

maplecl::Option<bool> oMflipMips16({"-mflip-mips16"},
    "  -mflip-mips16               \tGenerate MIPS16 code on alternating functions.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMfloatAbi({"-mfloat-abi"},
    "  -mfloat-abi                 \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<std::string> oMfloatGprs({"-mfloat-gprs"},
    "  -mfloat-gprs                \tThis switch enables the generation of floating-point operations on the "
    "general-purpose registers for architectures that support it.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMfloatIeee({"-mfloat-ieee"},
    "  -mfloat-ieee                \tGenerate code that does not use VAX F and G floating-point arithmetic "
    "instead of IEEE single and double precision.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMfloatVax({"-mfloat-vax"},
    "  -mfloat-vax                 \tGenerate code that uses  VAX F and G floating-point arithmetic instead "
    "of IEEE single and double precision.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMfloat128({"-mfloat128"},
    "  -mfloat128                  \tEisable the __float128 keyword for IEEE 128-bit floating point and use "
    "either software emulation for IEEE 128-bit floating point or hardware instructions.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-float128"), maplecl::kHide);

maplecl::Option<bool> oMfloat128Hardware({"-mfloat128-hardware"},
    "  -mfloat128-hardware         \tEisable using ISA 3.0 hardware instructions to support the __float128 "
    "data type.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-float128-hardware"), maplecl::kHide);

maplecl::Option<bool> oMfloat32({"-mfloat32"},
    "  -mfloat32                   \tUse 32-bit float.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-float32"), maplecl::kHide);

maplecl::Option<bool> oMfloat64({"-mfloat64"},
    "  -mfloat64                   \tUse 64-bit float.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-float64"), maplecl::kHide);

maplecl::Option<std::string> oMflushFunc({"-mflush-func"},
    "  -mflush-func                \tSpecifies the function to call to flush the I and D caches, or to not "
    "call any such function. \n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-flush-func"), maplecl::kHide);

maplecl::Option<std::string> oMflushTrap({"-mflush-trap"},
    "  -mflush-trap                \t\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-flush-trap"), maplecl::kHide);

maplecl::Option<bool> oMfma({"-mfma"},
    "  -mfma                       \tThese switches enable the use of instructions in the mfma.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-fmaf"), maplecl::kHide);

maplecl::Option<bool> oMfma4({"-mfma4"},
    "  -mfma4                      \tThese switches enable the use of instructions in the mfma4.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMfmaf({"-mfmaf"},
    "  -mfmaf                      \tWith -mfmaf, Maple generates code that takes advantage of the UltraSPARC "
    "Fused Multiply-Add Floating-point instructions.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMfmovd({"-mfmovd"},
    "  -mfmovd                     \tEnable the use of the instruction fmovd.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMforceNoPic({"-mforce-no-pic"},
    "  -mforce-no-pic              \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMfpExceptions({"-mfp-exceptions"},
    "  -mfp-exceptions             \tSpecifies whether FP exceptions are enabled. \n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-mno-fp-exceptions"), maplecl::kHide);

maplecl::Option<bool> oMfpMode({"-mfp-mode"},
    "  -mfp-mode                   \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<std::string> oMfpRoundingMode({"-mfp-rounding-mode"},
    "  -mfp-rounding-mode          \tSelects the IEEE rounding mode.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<std::string> oMfpTrapMode({"-mfp-trap-mode"},
    "  -mfp-trap-mode              \tThis option controls what floating-point related traps are enabled.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMfp16Format({"-mfp16-format"},
    "  -mfp16-format               \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMfp32({"-mfp32"},
    "  -mfp32                      \tAssume that floating-point registers are 32 bits wide.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMfp64({"-mfp64"},
    "  -mfp64                      \tAssume that floating-point registers are 64 bits wide.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMfpmath({"-mfpmath"},
    "  -mfpmath                    \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMfpr32({"-mfpr-32"},
    "  -mfpr-32                    \tUse only the first 32 floating-point registers.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMfpr64({"-mfpr-64"},
    "  -mfpr-64                    \tUse all 64 floating-point registers.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMfprnd({"-mfprnd"},
    "  -mfprnd                     \tSpecify which instructions are available on the processor you are using. \n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-fprnd"), maplecl::kHide);

maplecl::Option<std::string> oMfpu({"-mfpu"},
    "  -mfpu                       \tEnables support for specific floating-point hardware extensions for "
    "ARCv2 cores.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-fpu"), maplecl::kHide);

maplecl::Option<bool> oMfpxx({"-mfpxx"},
    "  -mfpxx                      \tDo not assume the width of floating-point registers.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMfractConvertTruncate({"-mfract-convert-truncate"},
    "  -mfract-convert-truncate    \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMframeHeaderOpt({"-mframe-header-opt"},
    "  -mframe-header-opt          \tEnable (disable) frame header optimization in the o32 ABI.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-mno-frame-header-opt"), maplecl::kHide);

maplecl::Option<bool> oMfriz({"-mfriz"},
    "  -mfriz                      \tGenerate the friz instruction when the -funsafe-math-optimizations option "
    "is used to optimize rounding of floating-point values to 64-bit integer and back to floating point. \n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMfsca({"-mfsca"},
    "  -mfsca                      \tAllow or disallow the compiler to emit the fsca instruction for sine and "
    "cosine approximations.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-fsca"), maplecl::kHide);

maplecl::Option<bool> oMfsgsbase({"-mfsgsbase"},
    "  -mfsgsbase                  \tThese switches enable the use of instructions in the mfsgsbase.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMfsmuld({"-mfsmuld"},
    "  -mfsmuld                    \tWith -mfsmuld, Maple generates code that takes advantage of the Floating-point"
    " Multiply Single to Double (FsMULd) instruction. \n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-fsmuld"), maplecl::kHide);

maplecl::Option<bool> oMfsrra({"-mfsrra"},
    "  -mfsrra                     \tAllow or disallow the compiler to emit the fsrra instruction for reciprocal "
    "square root approximations.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-fsrra"), maplecl::kHide);

maplecl::Option<bool> oMfullRegs({"-mfull-regs"},
    "  -mfull-regs                 \tUse full-set registers for register allocation.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMfullToc({"-mfull-toc"},
    "  -mfull-toc                  \tModify generation of the TOC (Table Of Contents), which is created for every "
    "executable file. \n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMfusedMadd({"-mfused-madd"},
    "  -mfused-madd                \tGenerate code that uses the floating-point multiply and accumulate "
    "instructions. \n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-fused-madd"), maplecl::kHide);

maplecl::Option<bool> oMfxsr({"-mfxsr"},
    "  -mfxsr                      \tThese switches enable the use of instructions in the mfxsr.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMG({"-MG"},
    "  -MG                         \tTreat missing header files as generated files.\n",
    {driverCategory, clangCategory}, kOptFront);

maplecl::Option<bool> oMg10({"-mg10"},
    "  -mg10                       \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMg13({"-mg13"},
    "  -mg13                       \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMg14({"-mg14"},
    "  -mg14                       \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMgas({"-mgas"},
    "  -mgas                       \tEnable the use of assembler directives only GAS understands.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMgccAbi({"-mgcc-abi"},
    "  -mgcc-abi                   \tEnables support for the old GCC version of the V850 ABI.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMgenCellMicrocode({"-mgen-cell-microcode"},
    "  -mgen-cell-microcode        \tGenerate Cell microcode instructions.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMgeneralRegsOnly({"-mgeneral-regs-only"},
    "  -mgeneral-regs-only         \tGenerate code which uses only the general registers.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMghs({"-mghs"},
    "  -mghs                       \tEnables support for the RH850 version of the V850 ABI.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMglibc({"-mglibc"},
    "  -mglibc                     \tUse GNU C library.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMgnu({"-mgnu"},
    "  -mgnu                       \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMgnuAs({"-mgnu-as"},
    "  -mgnu-as                    \t\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-gnu-as"), maplecl::kHide);

maplecl::Option<bool> oMgnuAttribute({"-mgnu-attribute"},
    "  -mgnu-attribute             \tEmit .gnu_attribute assembly directives to set tag/value pairs in a "
    ".gnu.attributes section that specify ABI variations in function parameters or return values.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-gnu-attribute"), maplecl::kHide);

maplecl::Option<bool> oMgnuLd({"-mgnu-ld"},
    "  -mgnu-ld                    \tUse options specific to GNU ld.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-gnu-ld"), maplecl::kHide);

maplecl::Option<bool> oMgomp({"-mgomp"},
    "  -mgomp                      \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMgotplt({"-mgotplt"},
    "  -mgotplt                    \t\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-gotplt"), maplecl::kHide);

maplecl::Option<bool> oMgp32({"-mgp32"},
    "  -mgp32                      \tAssume that general-purpose registers are 32 bits wide.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMgp64({"-mgp64"},
    "  -mgp64                      \tAssume that general-purpose registers are 64 bits wide.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMgpopt({"-mgpopt"},
    "  -mgpopt                     \tUse GP-relative accesses for symbols that are known to be in a small "
    "data section.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-gpopt"), maplecl::kHide);

maplecl::Option<bool> oMgpr32({"-mgpr-32"},
    "  -mgpr-32                    \tOnly use the first 32 general-purpose registers.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMgpr64({"-mgpr-64"},
    "  -mgpr-64                    \tUse all 64 general-purpose registers.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMgprelRo({"-mgprel-ro"},
    "  -mgprel-ro                  \tEnable the use of GPREL relocations in the FDPIC ABI for data that is "
    "known to be in read-only sections.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMh({"-mh"},
    "  -mh                         \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMhal({"-mhal"},
    "  -mhal                       \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMhalfRegFile({"-mhalf-reg-file"},
    "  -mhalf-reg-file             \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMhardDfp({"-mhard-dfp"},
    "  -mhard-dfp                  \tUse the hardware decimal-floating-point instructions for decimal-floating-point "
    "operations. \n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-hard-dfp"), maplecl::kHide);

maplecl::Option<bool> oMhardFloat({"-mhard-float"},
    "  -mhard-float                \tUse the hardware floating-point instructions and registers for floating-point"
    " operations.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMhardQuadFloat({"-mhard-quad-float"},
    "  -mhard-quad-float           \tGenerate output containing quad-word (long double) floating-point instructions.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMhardlit({"-mhardlit"},
    "  -mhardlit                   \t\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-hardlit"), maplecl::kHide);

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

maplecl::Option<bool> oMhpLd({"-mhp-ld"},
    "  -mhp-ld                     \tUse options specific to HP ld.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMhtm({"-mhtm"},
    "  -mhtm                       \tThe -mhtm option enables a set of builtins making use of instructions "
    "available with the transactional execution facility introduced with the IBM zEnterprise EC12 machine generation "
    "S/390 System z Built-in Functions.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-htm"), maplecl::kHide);

maplecl::Option<bool> oMhwDiv({"-mhw-div"},
    "  -mhw-div                    \t\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-hw-div"), maplecl::kHide);

maplecl::Option<bool> oMhwMul({"-mhw-mul"},
    "  -mhw-mul                    \t\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-hw-mul"), maplecl::kHide);

maplecl::Option<bool> oMhwMulx({"-mhw-mulx"},
    "  -mhw-mulx                   \t\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-hw-mulx"), maplecl::kHide);

maplecl::Option<std::string> oMhwmultE({"-mhwmult="},
    "  -mhwmult=                   \tDescribes the type of hardware multiply supported by the target.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMiamcu({"-miamcu"},
    "  -miamcu                     \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMicplb({"-micplb"},
    "  -micplb                     \tAssume that ICPLBs are enabled at run time.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMidSharedLibrary({"-mid-shared-library"},
    "  -mid-shared-library         \tGenerate code that supports shared libraries via the library ID method.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-id-shared-library"), maplecl::kHide);

maplecl::Option<bool> oMieee({"-mieee"},
    "  -mieee                      \tThis option generates code fully IEEE-compliant code except that the "
    "inexact-flag is not maintained (see below).\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-ieee"), maplecl::kHide);

maplecl::Option<bool> oMieeeConformant({"-mieee-conformant"},
    "  -mieee-conformant           \tThis option marks the generated code as IEEE conformant.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMieeeFp({"-mieee-fp"},
    "  -mieee-fp                   \t\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-ieee-fp"), maplecl::kHide);

maplecl::Option<bool> oMieeeWithInexact({"-mieee-with-inexact"},
    "  -mieee-with-inexact         \tTurning on this option causes the generated code to implement fully-compliant "
    "IEEE math. \n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMilp32({"-milp32"},
    "  -milp32                     \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMimadd({"-mimadd"},
    "  -mimadd                     \tEnable (disable) use of the madd and msub integer instructions.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-imadd"), maplecl::kHide);

maplecl::Option<bool> oMimpureText({"-mimpure-text"},
    "  -mimpure-text               \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMincomingStackBoundary({"-mincoming-stack-boundary"},
    "  -mincoming-stack-boundary   \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMindexedLoads({"-mindexed-loads"},
    "  -mindexed-loads             \tEnable the use of indexed loads. \n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMinlineAllStringops({"-minline-all-stringops"},
    "  -minline-all-stringops      \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMinlineFloatDivideMaxThroughput({"-minline-float-divide-max-throughput"},
    "  -minline-float-divide-max-throughput  \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMinlineFloatDivideMinLatency({"-minline-float-divide-min-latency"},
    "  -minline-float-divide-min-latency  \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMinlineIc_invalidate({"-minline-ic_invalidate"},
    "  -minline-ic_invalidate      \tInline code to invalidate instruction cache entries after setting up nested "
    "function trampolines.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMinlineIntDivideMaxThroughput({"-minline-int-divide-max-throughput"},
    "  -minline-int-divide-max-throughput  \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMinlineIntDivideMinLatency({"-minline-int-divide-min-latency"},
    "  -minline-int-divide-min-latency  \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMinlinePlt({"-minline-plt"},
    "  -minline-plt                \tEnable inlining of PLT entries in function calls to functions that are not "
    "known to bind locally.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMinlineSqrtMaxThroughput({"-minline-sqrt-max-throughput"},
    "  -minline-sqrt-max-throughput  \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMinlineSqrtMinLatency({"-minline-sqrt-min-latency"},
    "  -minline-sqrt-min-latency   \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMinlineStringopsDynamically({"-minline-stringops-dynamically"},
    "  -minline-stringops-dynamically  \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMinrt({"-minrt"},
    "  -minrt                      \tEnable the use of a minimum runtime environment - no static initializers or "
    "constructors.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<std::string> oMinsertSchedNops({"-minsert-sched-nops"},
    "  -minsert-sched-nops         \tThis option controls which NOP insertion scheme is used during the second "
    "scheduling pass.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMintRegister({"-mint-register"},
    "  -mint-register              \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMint16({"-mint16"},
    "  -mint16                     \tUse 16-bit int.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-int16"), maplecl::kHide);

maplecl::Option<bool> oMint32({"-mint32"},
    "  -mint32                     \tUse 32-bit int.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-int32"), maplecl::kHide);

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

maplecl::Option<std::string> oMiselE({"-misel="},
    "  -misel=                     \tThis switch has been deprecated. Use -misel and -mno-isel instead.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<std::string> oMisel({"-misel"},
    "  -misel                      \tThis switch enables or disables the generation of ISEL instructions.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-isel"), maplecl::kHide);

maplecl::Option<bool> oMisize({"-misize"},
    "  -misize                     \tAnnotate assembler instructions with estimated addresses.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<std::string> oMisrVectorSize({"-misr-vector-size"},
    "  -misr-vector-size           \tSpecify the size of each interrupt vector, which must be 4 or 16.\n",
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

maplecl::Option<bool> oMM({"-MM"},
    "  -MM                         \tLike -M but ignore system header files.\n",
    {driverCategory, clangCategory}, kOptFront);

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

maplecl::Option<std::string> oMmcuE({"-mmcu="},
    "  -mmcu=                      \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMMD({"-MMD"},
    "  -MMD                        \tLike -MD but ignore system header files.\n",
    {driverCategory, clangCategory}, kOptFront);

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

maplecl::Option<std::string> oMmemoryLatency({"-mmemory-latency"},
    "  -mmemory-latency            \tSets the latency the scheduler should assume for typical memory references "
    "as seen by the application.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<std::string> oMmemoryModel({"-mmemory-model"},
    "  -mmemory-model              \tSet the memory model in force on the processor to one of.\n",
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

maplecl::Option<std::string> oMoverrideE({"-moverride="},
    "  -moverride=                 \tPower users only! Override CPU optimization parameters.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMP({"-MP"},
    "  -MP                         \tGenerate phony targets for all headers.\n",
    {driverCategory, clangCategory}, kOptFront);

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

maplecl::Option<std::string> oMprioritizeRestrictedInsns({"-mprioritize-restricted-insns"},
    "  -mprioritize-restricted-insns  \tThis option controls the priority that is assigned to dispatch-slot "
    "restricted instructions during the second scheduling pass. \n",
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

maplecl::Option<std::string> oMQ({"-MQ"},
    "  -MQ                         \t-MQ <target> o       Add a MAKE-quoted target.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::kJoinedValue);

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

maplecl::Option<std::string> oMrecip({"-mrecip"},
    "  -mrecip                     \tThis option enables use of the reciprocal estimate and reciprocal square "
    "root estimate instructions with additional Newton-Raphson steps to increase precision instead of doing a "
    "divide or square root and divide for floating-point arguments.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMrecipPrecision({"-mrecip-precision"},
    "  -mrecip-precision           \tAssume (do not assume) that the reciprocal estimate instructions provide "
    "higher-precision estimates than is mandated by the PowerPC ABI.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<std::string> oMrecipE({"-mrecip="},
    "  -mrecip=                    \tThis option controls which reciprocal estimate instructions may be used.\n",
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

maplecl::Option<std::string> oMschedCostlyDep({"-msched-costly-dep"},
    "  -msched-costly-dep          \tThis option controls which dependences are considered costly by the target "
    "during instruction scheduling.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

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

maplecl::Option<std::string> oMschedule({"-mschedule"},
    "  -mschedule                  \tSchedule code according to the constraints for the machine type cpu-type. \n",
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

maplecl::Option<std::string> oMsda({"-msda"},
    "  -msda                       \tPut static or global variables whose size is n bytes or less into the small "
    "data area that register gp points to.\n",
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

maplecl::Option<std::string> oMsharedLibraryId({"-mshared-library-id"},
    "  -mshared-library-id         \tSpecifies the identification number of the ID-based shared library being "
    "compiled.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMshort({"-mshort"},
    "  -mshort                     \tConsider type int to be 16 bits wide, like short int.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-short"), maplecl::kHide);

maplecl::Option<bool> oMsignExtendEnabled({"-msign-extend-enabled"},
    "  -msign-extend-enabled       \tEnable sign extend instructions.\n",
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

maplecl::Option<std::string> oMsizeLevel({"-msize-level"},
    "  -msize-level                \tFine-tune size optimization with regards to instruction lengths and alignment.\n",
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

maplecl::Option<std::string> oMspe({"-mspe="},
    "  -mspe=                      \tThis option has been deprecated. Use -mspe and -mno-spe instead.\n",
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

maplecl::Option<std::string> oMstackGuard({"-mstack-guard"},
    "  -mstack-guard               \tThe S/390 back end emits additional instructions in the function prologue that "
    "trigger a trap if the stack size is stack-guard bytes above the stack-size (remember that the stack on S/390 "
    "grows downward). \n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMstackIncrement({"-mstack-increment"},
    "  -mstack-increment           \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMstackOffset({"-mstack-offset"},
    "  -mstack-offset              \t\n",
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

maplecl::Option<std::string> oMtda({"-mtda"},
    "  -mtda                       \tPut static or global variables whose size is n bytes or less into the tiny "
    "data area that register ep points to.\n",
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

maplecl::Option<std::string> oMtiny({"-mtiny"},
    "  -mtiny                      \tVariables that are n bytes or smaller are allocated to the .tiny section.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

/* ##################### BOOL Options ############################################################### */

maplecl::Option<bool> wUnusedMacro({"-Wunused-macros"},
    "  -Wunused-macros             \twarning: macro is not used\n",
    {driverCategory, clangCategory}, kOptFront);

maplecl::Option<bool> wBadFunctionCast({"-Wbad-function-cast"},
    "  -Wbad-function-cast         \twarning: cast from function call of type A to non-matching type B\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-bad-function-cast"));

maplecl::Option<bool> wStrictPrototypes({"-Wstrict-prototypes"},
    "  -Wstrict-prototypes         \twarning: Warn if a function is declared or defined without specifying the "
    "argument types\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-strict-prototypes"));

maplecl::Option<bool> wUndef({"-Wundef"},
    "  -Wundef                     \twarning: Warn if an undefined identifier is evaluated in an #if directive. "
    "Such identifiers are replaced with zero\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-undef"));

maplecl::Option<bool> wCastQual({"-Wcast-qual"},
    "  -Wcast-qual                 \twarning: Warn whenever a pointer is cast so as to remove a type qualifier "
    "from the target type. For example, warn if a const char * is cast to an ordinary char *\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-cast-qual"));

maplecl::Option<bool> wMissingFieldInitializers({"-Wmissing-field-initializers"},
    "  -Wmissing-field-initializers\twarning: Warn if a structure's initializer has some fields missing\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-missing-field-initializers"));

maplecl::Option<bool> wUnusedParameter({"-Wunused-parameter"},
    "  -Wunused-parameter          \twarning: Warn whenever a function parameter is unused aside from its "
    "declaration\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-unused-parameter"));

maplecl::Option<bool> wAll({"-Wall"},
    "  -Wall                       \tThis enables all the warnings about constructions that some users consider "
    "questionable\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-all"));

maplecl::Option<bool> wExtra({"-Wextra"},
    "  -Wextra                     \tEnable some extra warning flags that are not enabled by -Wall\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-extra"));

maplecl::Option<bool> wWriteStrings({"-Wwrite-strings"},
    "  -Wwrite-strings             \tWhen compiling C, give string constants the type const char[length] so that "
    "copying the address of one into a non-const char * pointer produces a warning\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-write-strings"));

maplecl::Option<bool> wVla({"-Wvla"},
    "  -Wvla                       \tWarn if a variable-length array is used in the code\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-vla"));

maplecl::Option<bool> wFormatSecurity({"-Wformat-security"},
    "  -Wformat-security           \tWwarn about uses of format functions that represent possible security problems.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-format-security"));

maplecl::Option<bool> wShadow({"-Wshadow"},
    "  -Wshadow                    \tWarn whenever a local variable or type declaration shadows another variable.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-shadow"));

maplecl::Option<bool> wTypeLimits({"-Wtype-limits"},
    "  -Wtype-limits               \tWarn if a comparison is always true or always false due to the limited range "
    "of the data type.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-type-limits"));

maplecl::Option<bool> wSignCompare({"-Wsign-compare"},
    "  -Wsign-compare              \tWarn when a comparison between signed and  unsigned values could produce an "
    "incorrect result when the signed value is converted to unsigned.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-sign-compare"));

maplecl::Option<bool> wShiftNegativeValue({"-Wshift-negative-value"},
    "  -Wshift-negative-value      \tWarn if left shifting a negative value.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-shift-negative-value"));

maplecl::Option<bool> wPointerArith({"-Wpointer-arith"},
    "  -Wpointer-arith             \tWarn about anything that depends on the “size of” a function type or of void.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-pointer-arith"));

maplecl::Option<bool> wIgnoredQualifiers({"-Wignored-qualifiers"},
    "  -Wignored-qualifiers        \tWarn if the return type of a function has a type qualifier such as const.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-ignored-qualifiers"));

maplecl::Option<bool> wFormat({"-Wformat"},
    "  -Wformat                    \tCheck calls to printf and scanf, etc., to make sure that the arguments "
    "supplied have types appropriate to the format string specified.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-format"));

maplecl::Option<bool> wFloatEqual({"-Wfloat-equal"},
    "  -Wfloat-equal               \tWarn if floating-point values are used in equality comparisons.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-float-equal"));

maplecl::Option<bool> wDateTime({"-Wdate-time"},
    "  -Wdate-time                 \tWarn when macros __TIME__, __DATE__ or __TIMESTAMP__ are encountered as "
    "they might prevent bit-wise-identical reproducible compilations\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-date-time"));

maplecl::Option<bool> wImplicitFallthrough({"-Wimplicit-fallthrough"},
    "  -Wimplicit-fallthrough      \tWarn when a switch case falls through\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-implicit-fallthrough"));

maplecl::Option<bool> wShiftOverflow({"-Wshift-overflow"},
    "  -Wshift-overflow            \tWarn about left shift overflows\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-shift-overflow"));

maplecl::Option<bool> oWnounusedcommandlineargument({"-Wno-unused-command-line-argument"},
    "  -Wno-unused-command-line-argument\n"
    "                              \tno unused command line argument\n",
    {driverCategory, clangCategory}, kOptFront);

maplecl::Option<bool> oWnoconstantconversion({"-Wno-constant-conversion"},
    "  -Wno-constant-conversion    \tno constant conversion\n",
    {driverCategory, clangCategory}, kOptFront);

maplecl::Option<bool> oWnounknownwarningoption({"-Wno-unknown-warning-option"},
    "  -Wno-unknown-warning-option \tno unknown warning option\n",
    {driverCategory, clangCategory}, kOptFront);

maplecl::Option<bool> oW({"-W"},
    "  -W                          \tThis switch is deprecated; use -Wextra instead.\n",
    {driverCategory, clangCategory}, kOptFront);

maplecl::Option<bool> oWabi({"-Wabi"},
    "  -Wabi                       \tWarn about things that will change when compiling with an ABI-compliant "
    "compiler.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-abi"));

maplecl::Option<bool> oWabiTag({"-Wabi-tag"},
    "  -Wabi-tag                   \tWarn if a subobject has an abi_tag attribute that the complete object type "
    "does not have.\n",
    {driverCategory, clangCategory}, kOptFront);

maplecl::Option<bool> oWaddrSpaceConvert({"-Waddr-space-convert"},
    "  -Waddr-space-convert        \tWarn about conversions between address spaces in the case where the resulting "
    "address space is not contained in the incoming address space.\n",
    {driverCategory, clangCategory}, kOptFront);

maplecl::Option<bool> oWaddress({"-Waddress"},
    "  -Waddress                   \tWarn about suspicious uses of memory addresses.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-address"));

maplecl::Option<bool> oWaggregateReturn({"-Waggregate-return"},
    "  -Waggregate-return          \tWarn about returning structures\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-aggregate-return"));

maplecl::Option<bool> oWaggressiveLoopOptimizations({"-Waggressive-loop-optimizations"},
    "  -Waggressive-loop-optimizations\n"
    "                              \tWarn if a loop with constant number of iterations triggers undefined "
    "behavior.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-aggressive-loop-optimizations"));

maplecl::Option<bool> oWalignedNew({"-Waligned-new"},
    "  -Waligned-new               \tWarn about 'new' of type with extended alignment without -faligned-new.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-aligned-new"));

maplecl::Option<bool> oWallocZero({"-Walloc-zero"},
    "  -Walloc-zero                \t-Walloc-zero Warn for calls to allocation functions that specify zero bytes.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-alloc-zero"));

maplecl::Option<bool> oWalloca({"-Walloca"},
    "  -Walloca                    \tWarn on any use of alloca.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-alloca"));

maplecl::Option<bool> oWarrayBounds({"-Warray-bounds"},
    "  -Warray-bounds              \tWarn if an array is accessed out of bounds.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-array-bounds"));

maplecl::Option<bool> oWassignIntercept({"-Wassign-intercept"},
    "  -Wassign-intercept          \tWarn whenever an Objective-C assignment is being intercepted by the garbage "
    "collector.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-assign-intercept"));

maplecl::Option<bool> oWattributes({"-Wattributes"},
    "  -Wattributes                \tWarn about inappropriate attribute usage.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-attributes"));

maplecl::Option<bool> oWboolCompare({"-Wbool-compare"},
    "  -Wbool-compare              \tWarn about boolean expression compared with an integer value different from "
    "true/false.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-bool-compare"));

maplecl::Option<bool> oWboolOperation({"-Wbool-operation"},
    "  -Wbool-operation            \tWarn about certain operations on boolean expressions.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-bool-operation"));

maplecl::Option<bool> oWbuiltinDeclarationMismatch({"-Wbuiltin-declaration-mismatch"},
    "  -Wbuiltin-declaration-mismatch\n"
    "                              \tWarn when a built-in function is declared with the wrong signature.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-builtin-declaration-mismatch"));

maplecl::Option<bool> oWbuiltinMacroRedefined({"-Wbuiltin-macro-redefined"},
    "  -Wbuiltin-macro-redefined   \tWarn when a built-in preprocessor macro is undefined or redefined.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-builtin-macro-redefined"));

maplecl::Option<bool> oW11Compat({"-Wc++11-compat"},
    "  -Wc++11-compat              \tWarn about C++ constructs whose meaning differs between ISO C++ 1998 and "
    "ISO C++ 2011.\n",
    {driverCategory, clangCategory}, kOptFront);

maplecl::Option<bool> oW14Compat({"-Wc++14-compat"},
    "  -Wc++14-compat              \tWarn about C++ constructs whose meaning differs between ISO C++ 2011 and ISO "
    "C++ 2014.\n",
    {driverCategory, clangCategory}, kOptFront);

maplecl::Option<bool> oW1zCompat({"-Wc++1z-compat"},
    "  -Wc++1z-compat              \tWarn about C++ constructs whose meaning differs between ISO C++ 2014 and "
    "(forthcoming) ISO C++ 201z(7?).\n",
    {driverCategory, clangCategory}, kOptFront);

maplecl::Option<bool> oWc90C99Compat({"-Wc90-c99-compat"},
    "  -Wc90-c99-compat            \tWarn about features not present in ISO C90\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-c90-c99-compat"));

maplecl::Option<bool> oWc99C11Compat({"-Wc99-c11-compat"},
    "  -Wc99-c11-compat            \tWarn about features not present in ISO C99\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-c99-c11-compat"));

maplecl::Option<bool> oWcastAlign({"-Wcast-align"},
    "  -Wcast-align                \tWarn about pointer casts which increase alignment.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-cast-align"));

maplecl::Option<bool> oWcharSubscripts({"-Wchar-subscripts"},
    "  -Wchar-subscripts           \tWarn about subscripts whose type is \"char\".\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-char-subscripts"));

maplecl::Option<bool> oWchkp({"-Wchkp"},
    "  -Wchkp                      \tWarn about memory access errors found by Pointer Bounds Checker.\n",
    {driverCategory, clangCategory}, kOptFront);

maplecl::Option<bool> oWclobbered({"-Wclobbered"},
    "  -Wclobbered                 \tWarn about variables that might be changed by \"longjmp\" or \"vfork\".\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-clobbered"));

maplecl::Option<bool> oWcomment({"-Wcomment"},
    "  -Wcomment                   \tWarn about possibly nested block comments\n",
    {driverCategory, clangCategory}, kOptFront);

maplecl::Option<bool> oWcomments({"-Wcomments"},
    "  -Wcomments                  \tSynonym for -Wcomment.\n",
    {driverCategory, clangCategory}, kOptFront);

maplecl::Option<bool> oWconditionallySupported({"-Wconditionally-supported"},
    "  -Wconditionally-supported   \tWarn for conditionally-supported constructs.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-conditionally-supported"));

maplecl::Option<bool> oWconversion({"-Wconversion"},
    "  -Wconversion                \tWarn for implicit type conversions that may change a value.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-conversion"));

maplecl::Option<bool> oWconversionNull({"-Wconversion-null"},
    "  -Wconversion-null           \tWarn for converting NULL from/to a non-pointer type.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-conversion-null"));

maplecl::Option<bool> oWctorDtorPrivacy({"-Wctor-dtor-privacy"},
    "  -Wctor-dtor-privacy         \tWarn when all constructors and destructors are private.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-ctor-dtor-privacy"));

maplecl::Option<bool> oWdanglingElse({"-Wdangling-else"},
    "  -Wdangling-else             \tWarn about dangling else.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-dangling-else"));

maplecl::Option<bool> oWdeclarationAfterStatement({"-Wdeclaration-after-statement"},
    "  -Wdeclaration-after-statement\n"
    "                              \tWarn when a declaration is found after a statement.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-declaration-after-statement"));

maplecl::Option<bool> oWdeleteIncomplete({"-Wdelete-incomplete"},
    "  -Wdelete-incomplete         \tWarn when deleting a pointer to incomplete type.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-delete-incomplete"));

maplecl::Option<bool> oWdeleteNonVirtualDtor({"-Wdelete-non-virtual-dtor"},
    "  -Wdelete-non-virtual-dtor   \tWarn about deleting polymorphic objects with non-virtual destructors.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-delete-non-virtual-dtor"));

maplecl::Option<bool> oWdeprecated({"-Wdeprecated"},
    "  -Wdeprecated                \tWarn if a deprecated compiler feature\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-deprecated"));

maplecl::Option<bool> oWdeprecatedDeclarations({"-Wdeprecated-declarations"},
    "  -Wdeprecated-declarations   \tWarn about uses of __attribute__((deprecated)) declarations.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-deprecated-declarations"));

maplecl::Option<bool> oWdisabledOptimization({"-Wdisabled-optimization"},
    "  -Wdisabled-optimization     \tWarn when an optimization pass is disabled.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-disabled-optimization"));

maplecl::Option<bool> oWdiscardedArrayQualifiers({"-Wdiscarded-array-qualifiers"},
    "  -Wdiscarded-array-qualifiers\tWarn if qualifiers on arrays which are pointer targets are discarded.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-discarded-array-qualifiers"));

maplecl::Option<bool> oWdiscardedQualifiers({"-Wdiscarded-qualifiers"},
    "  -Wdiscarded-qualifiers      \tWarn if type qualifiers on pointers are discarded.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-discarded-qualifiers"));

maplecl::Option<bool> oWdivByZero({"-Wdiv-by-zero"},
    "  -Wdiv-by-zero               \tWarn about compile-time integer division by zero.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-div-by-zero"));

maplecl::Option<bool> oWdoublePromotion({"-Wdouble-promotion"},
    "  -Wdouble-promotion          \tWarn about implicit conversions from \"float\" to \"double\".\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-double-promotion"));

maplecl::Option<bool> oWduplicateDeclSpecifier({"-Wduplicate-decl-specifier"},
    "  -Wduplicate-decl-specifier  \tWarn when a declaration has duplicate const\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-duplicate-decl-specifier"));

maplecl::Option<bool> oWduplicatedBranches({"-Wduplicated-branches"},
    "  -Wduplicated-branches       \tWarn about duplicated branches in if-else statements.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-duplicated-branches"));

maplecl::Option<bool> oWduplicatedCond({"-Wduplicated-cond"},
    "  -Wduplicated-cond           \tWarn about duplicated conditions in an if-else-if chain.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-duplicated-cond"));

maplecl::Option<bool> oWeffc({"-Weffc++"},
    "  -Weffc++                    \tWarn about violations of Effective C++ style rules.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-effc++"));

maplecl::Option<bool> oWemptyBody({"-Wempty-body"},
    "  -Wempty-body                \tWarn about an empty body in an if or else statement.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-empty-body"));

maplecl::Option<bool> oWendifLabels({"-Wendif-labels"},
    "  -Wendif-labels              \tWarn about stray tokens after #else and #endif.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-endif-labels"));

maplecl::Option<bool> oWenumCompare({"-Wenum-compare"},
    "  -Wenum-compare              \tWarn about comparison of different enum types.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-enum-compare"));

maplecl::Option<bool> oWerror({"-Werror"},
    "  -Werror                     \tTreat all warnings as errors.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-error"));

maplecl::Option<bool> oWexpansionToDefined({"-Wexpansion-to-defined"},
    "  -Wexpansion-to-defined      \tWarn if 'defined' is used outside #if.\n",
    {driverCategory, clangCategory}, kOptFront);

maplecl::Option<bool> oWfatalErrors({"-Wfatal-errors"},
    "  -Wfatal-errors              \tExit on the first error occurred.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-fatal-errors"));

maplecl::Option<bool> oWfloatConversion({"-Wfloat-conversion"},
    "  -Wfloat-conversion          \tWarn for implicit type conversions that cause loss of floating point precision.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-float-conversion"));

maplecl::Option<bool> oWformatContainsNul({"-Wformat-contains-nul"},
    "  -Wformat-contains-nul       \tWarn about format strings that contain NUL bytes.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-format-contains-nul"));

maplecl::Option<bool> oWformatExtraArgs({"-Wformat-extra-args"},
    "  -Wformat-extra-args         \tWarn if passing too many arguments to a function for its format string.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-format-extra-args"));

maplecl::Option<bool> oWformatNonliteral({"-Wformat-nonliteral"},
    "  -Wformat-nonliteral         \tWarn about format strings that are not literals.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-format-nonliteral"));

maplecl::Option<bool> oWformatOverflow({"-Wformat-overflow"},
    "  -Wformat-overflow           \tWarn about function calls with format strings that write past the end of the "
    "destination region.  Same as -Wformat-overflow=1.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-format-overflow"));

maplecl::Option<bool> oWformatSignedness({"-Wformat-signedness"},
    "  -Wformat-signedness         \tWarn about sign differences with format functions.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-format-signedness"));

maplecl::Option<bool> oWformatTruncation({"-Wformat-truncation"},
    "  -Wformat-truncation         \tWarn about calls to snprintf and similar functions that truncate output. "
    "Same as -Wformat-truncation=1.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-format-truncation"));

maplecl::Option<bool> oWformatY2k({"-Wformat-y2k"},
    "  -Wformat-y2k                \tWarn about strftime formats yielding 2-digit years.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-format-y2k"));

maplecl::Option<bool> oWformatZeroLength({"-Wformat-zero-length"},
    "  -Wformat-zero-length        \tWarn about zero-length formats.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-format-zero-length"));

maplecl::Option<bool> oWframeAddress({"-Wframe-address"},
    "  -Wframe-address             \tWarn when __builtin_frame_address or __builtin_return_address is used unsafely.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-frame-address"));

maplecl::Option<uint32_t> oWframeLargerThan({"-Wframe-larger-than="},
    "  -Wframe-larger-than=        \tWarn if a function's stack frame requires in excess of <byte-size>.\n",
    {driverCategory, clangCategory}, kOptFront);

maplecl::Option<bool> oWfreeNonheapObject({"-Wfree-nonheap-object"},
    "  -Wfree-nonheap-object       \tWarn when attempting to free a non-heap object.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-free-nonheap-object"));

maplecl::Option<bool> oWignoredAttributes({"-Wignored-attributes"},
    "  -Wignored-attributes        \tWarn whenever attributes are ignored.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-ignored-attributes"));

maplecl::Option<bool> oWimplicit({"-Wimplicit"},
    "  -Wimplicit                  \tWarn about implicit declarations.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-implicit"));

maplecl::Option<bool> oWimplicitFunctionDeclaration({"-Wimplicit-function-declaration"},
    "  -Wimplicit-function-declaration\n"
    "                              \tWarn about implicit function declarations.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-implicit-function-declaration"));

maplecl::Option<bool> oWimplicitInt({"-Wimplicit-int"},
    "  -Wimplicit-int              \tWarn when a declaration does not specify a type.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-implicit-int"));

maplecl::Option<bool> oWincompatiblePointerTypes({"-Wincompatible-pointer-types"},
    "  -Wincompatible-pointer-types\tWarn when there is a conversion between pointers that have incompatible types.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-incompatible-pointer-types"));

maplecl::Option<bool> oWinheritedVariadicCtor({"-Winherited-variadic-ctor"},
    "  -Winherited-variadic-ctor   \tWarn about C++11 inheriting constructors when the base has a variadic "
    "constructor.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-inherited-variadic-ctor"));

maplecl::Option<bool> oWinitSelf({"-Winit-self"},
    "  -Winit-self                 \tWarn about variables which are initialized to themselves.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-init-self"));

maplecl::Option<bool> oWinline({"-Winline"},
    "  -Winline                    \tWarn when an inlined function cannot be inlined.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-inline"));

maplecl::Option<bool> oWintConversion({"-Wint-conversion"},
    "  -Wint-conversion            \tWarn about incompatible integer to pointer and pointer to integer conversions.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-int-conversion"));

maplecl::Option<bool> oWintInBoolContext({"-Wint-in-bool-context"},
    "  -Wint-in-bool-context       \tWarn for suspicious integer expressions in boolean context.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-int-in-bool-context"));

maplecl::Option<bool> oWintToPointerCast({"-Wint-to-pointer-cast"},
    "  -Wint-to-pointer-cast       \tWarn when there is a cast to a pointer from an integer of a different size.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-int-to-pointer-cast"));

maplecl::Option<bool> oWinvalidMemoryModel({"-Winvalid-memory-model"},
    "  -Winvalid-memory-model      \tWarn when an atomic memory model parameter is known to be outside the valid "
    "range.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-invalid-memory-model"));

maplecl::Option<bool> oWinvalidOffsetof({"-Winvalid-offsetof"},
    "  -Winvalid-offsetof          \tWarn about invalid uses of the \"offsetof\" macro.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-invalid-offsetof"));

maplecl::Option<bool> oWLiteralSuffix({"-Wliteral-suffix"},
    "  -Wliteral-suffix            \tWarn when a string or character literal is followed by a ud-suffix which does "
    "not begin with an underscore.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-literal-suffix"));

maplecl::Option<bool> oWLogicalNotParentheses({"-Wlogical-not-parentheses"},
    "  -Wlogical-not-parentheses   \tWarn about logical not used on the left hand side operand of a comparison.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-logical-not-parentheses"));

maplecl::Option<bool> oWinvalidPch({"-Winvalid-pch"},
    "  -Winvalid-pch               \tWarn about PCH files that are found but not used.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-invalid-pch"));

maplecl::Option<bool> oWjumpMissesInit({"-Wjump-misses-init"},
    "  -Wjump-misses-init          \tWarn when a jump misses a variable initialization.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-jump-misses-init"));

maplecl::Option<bool> oWLogicalOp({"-Wlogical-op"},
    "  -Wlogical-op                \tWarn about suspicious uses of logical operators in expressions. \n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-logical-op"));

maplecl::Option<bool> oWLongLong({"-Wlong-long"},
    "  -Wlong-long                 \tWarn if long long type is used.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-long-long"));

maplecl::Option<bool> oWmain({"-Wmain"},
    "  -Wmain                      \tWarn about suspicious declarations of \"main\".\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-main"));

maplecl::Option<bool> oWmaybeUninitialized({"-Wmaybe-uninitialized"},
    "  -Wmaybe-uninitialized       \tWarn about maybe uninitialized automatic variables.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-maybe-uninitialized"));

maplecl::Option<bool> oWmemsetEltSize({"-Wmemset-elt-size"},
    "  -Wmemset-elt-size           \tWarn about suspicious calls to memset where the third argument contains the "
    "number of elements not multiplied by the element size.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-memset-elt-size"));

maplecl::Option<bool> oWmemsetTransposedArgs({"-Wmemset-transposed-args"},
    "  -Wmemset-transposed-args    \tWarn about suspicious calls to memset where the third argument is constant "
    "literal zero and the second is not.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-memset-transposed-args"));

maplecl::Option<bool> oWmisleadingIndentation({"-Wmisleading-indentation"},
    "  -Wmisleading-indentation    \tWarn when the indentation of the code does not reflect the block structure.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-misleading-indentatio"));

maplecl::Option<bool> oWmissingBraces({"-Wmissing-braces"},
    "  -Wmissing-braces            \tWarn about possibly missing braces around initializers.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-missing-bracesh"));

maplecl::Option<bool> oWmissingDeclarations({"-Wmissing-declarations"},
    "  -Wmissing-declarations      \tWarn about global functions without previous declarations.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-missing-declarations"));

maplecl::Option<bool> oWmissingFormatAttribute({"-Wmissing-format-attribute"},
    "  -Wmissing-format-attribute  \t\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-missing-format-attribute"));

maplecl::Option<bool> oWmissingIncludeDirs({"-Wmissing-include-dirs"},
    "  -Wmissing-include-dirs      \tWarn about user-specified include directories that do not exist.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-missing-include-dirs"));

maplecl::Option<bool> oWmissingParameterType({"-Wmissing-parameter-type"},
    "  -Wmissing-parameter-type    \tWarn about function parameters declared without a type specifier in K&R-style "
    "functions.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-missing-parameter-type"));

maplecl::Option<bool> oWmissingPrototypes({"-Wmissing-prototypes"},
    "  -Wmissing-prototypes        \tWarn about global functions without prototypes.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-missing-prototypes"));

maplecl::Option<bool> oWmultichar({"-Wmultichar"},
    "  -Wmultichar                 \tWarn about use of multi-character character constants.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-multichar"));

maplecl::Option<bool> oWmultipleInheritance({"-Wmultiple-inheritance"},
    "  -Wmultiple-inheritance      \tWarn on direct multiple inheritance.\n",
    {driverCategory, clangCategory}, kOptFront);

maplecl::Option<bool> oWnamespaces({"-Wnamespaces"},
    "  -Wnamespaces                \tWarn on namespace definition.\n",
    {driverCategory, clangCategory}, kOptFront);

maplecl::Option<bool> oWnarrowing({"-Wnarrowing"},
    "  -Wnarrowing                 \tWarn about narrowing conversions within { } that are ill-formed in C++11.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-narrowing"));

maplecl::Option<bool> oWnestedExterns({"-Wnested-externs"},
    "  -Wnested-externs            \tWarn about \"extern\" declarations not at file scope.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-nested-externs"));

maplecl::Option<bool> oWnoexcept({"-Wnoexcept"},
    "  -Wnoexcept                  \tWarn when a noexcept expression evaluates to false even though the expression "
    "can't actually throw.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-noexcept"));

maplecl::Option<bool> oWnoexceptType({"-Wnoexcept-type"},
    "  -Wnoexcept-type             \tWarn if C++1z noexcept function type will change the mangled name of a symbol.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-noexcept-type"));

maplecl::Option<bool> oWnonTemplateFriend({"-Wnon-template-friend"},
    "  -Wnon-template-friend       \tWarn when non-templatized friend functions are declared within a template.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-non-template-friend"));

maplecl::Option<bool> oWnonVirtualDtor({"-Wnon-virtual-dtor"},
    "  -Wnon-virtual-dtor          \tWarn about non-virtual destructors.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-non-virtual-dtor"));

maplecl::Option<bool> oWnonnull({"-Wnonnull"},
    "  -Wnonnull                   \tWarn about NULL being passed to argument slots marked as requiring non-NULL.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-nonnull"));

maplecl::Option<bool> oWnonnullCompare({"-Wnonnull-compare"},
    "  -Wnonnull-compare           \tWarn if comparing pointer parameter with nonnull attribute with NULL.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-nonnull-compare"));

maplecl::Option<bool> oWnormalized({"-Wnormalized"},
    "  -Wnormalized                \tWarn about non-normalized Unicode strings.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-normalized"));

maplecl::Option<bool> oWnullDereference({"-Wnull-dereference"},
    "  -Wnull-dereference          \tWarn if dereferencing a NULL pointer may lead to erroneous or undefined "
    "behavior.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-null-dereference"));

maplecl::Option<bool> oWodr({"-Wodr"},
    "  -Wodr                       \tWarn about some C++ One Definition Rule violations during link time "
    "optimization.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-odr"));

maplecl::Option<bool> oWoldStyleCast({"-Wold-style-cast"},
    "  -Wold-style-cast            \tWarn if a C-style cast is used in a program.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-old-style-cast"));

maplecl::Option<bool> oWoldStyleDeclaration({"-Wold-style-declaration"},
    "  -Wold-style-declaration     \tWarn for obsolescent usage in a declaration.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-old-style-declaration"));

maplecl::Option<bool> oWoldStyleDefinition({"-Wold-style-definition"},
    "  -Wold-style-definition      \tWarn if an old-style parameter definition is used.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-old-style-definition"));

maplecl::Option<bool> oWopenmSimd({"-Wopenm-simd"},
    "  -Wopenm-simd                \tWarn if the vectorizer cost model overrides the OpenMP or the Cilk Plus "
    "simd directive set by user. \n",
    {driverCategory, clangCategory}, kOptFront);

maplecl::Option<bool> oWoverflow({"-Woverflow"},
    "  -Woverflow                  \tWarn about overflow in arithmetic expressions.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-overflow"));

maplecl::Option<bool> oWoverlengthStrings({"-Woverlength-strings"},
    "  -Woverlength-strings        \tWarn if a string is longer than the maximum portable length specified by "
    "the standard.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-overlength-strings"));

maplecl::Option<bool> oWoverloadedVirtual({"-Woverloaded-virtual"},
    "  -Woverloaded-virtual        \tWarn about overloaded virtual function names.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-overloaded-virtual"));

maplecl::Option<bool> oWoverrideInit({"-Woverride-init"},
    "  -Woverride-init             \tWarn about overriding initializers without side effects.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-override-init"));

maplecl::Option<bool> oWoverrideInitSideEffects({"-Woverride-init-side-effects"},
    "  -Woverride-init-side-effects\tWarn about overriding initializers with side effects.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-override-init-side-effects"));

maplecl::Option<bool> oWpacked({"-Wpacked"},
    "  -Wpacked                    \tWarn when the packed attribute has no effect on struct layout.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-packed"));

maplecl::Option<bool> oWpackedBitfieldCompat({"-Wpacked-bitfield-compat"},
    "  -Wpacked-bitfield-compat    \tWarn about packed bit-fields whose offset changed in GCC 4.4.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-packed-bitfield-compat"));

maplecl::Option<bool> oWpadded({"-Wpadded"},
    "  -Wpadded                    \tWarn when padding is required to align structure members.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-padded"));

maplecl::Option<bool> oWparentheses({"-Wparentheses"},
    "  -Wparentheses               \tWarn about possibly missing parentheses.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-parentheses"));

maplecl::Option<bool> oWpedantic({"-Wpedantic"},
    "  -Wpedantic                  \tIssue warnings needed for strict compliance to the standard.\n",
    {driverCategory, clangCategory}, kOptFront);

maplecl::Option<bool> oWpedanticMsFormat({"-Wpedantic-ms-format"},
    "  -Wpedantic-ms-format        \t\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-pedantic-ms-format"));

maplecl::Option<bool> oWplacementNew({"-Wplacement-new"},
    "  -Wplacement-new             \tWarn for placement new expressions with undefined behavior.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-placement-new"));

maplecl::Option<bool> oWpmfConversions({"-Wpmf-conversions"},
    "  -Wpmf-conversions           \tWarn when converting the type of pointers to member functions.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-pmf-conversions"));

maplecl::Option<bool> oWpointerCompare({"-Wpointer-compare"},
    "  -Wpointer-compare           \tWarn when a pointer is compared with a zero character constant.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-pointer-compare"));

maplecl::Option<bool> oWpointerSign({"-Wpointer-sign"},
    "  -Wpointer-sign              \tWarn when a pointer differs in signedness in an assignment.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-pointer-sign"));

maplecl::Option<bool> oWpointerToIntCast({"-Wpointer-to-int-cast"},
    "  -Wpointer-to-int-cast       \tWarn when a pointer is cast to an integer of a different size.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-pointer-to-int-cast"));

maplecl::Option<bool> oWpragmas({"-Wpragmas"},
    "  -Wpragmas                   \tWarn about misuses of pragmas.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-pragmas"));

maplecl::Option<bool> oWprotocol({"-Wprotocol"},
    "  -Wprotocol                  \tWarn if inherited methods are unimplemented.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-protocol"));

maplecl::Option<bool> oWredundantDecls({"-Wredundant-decls"},
    "  -Wredundant-decls           \tWarn about multiple declarations of the same object.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-redundant-decls"));

maplecl::Option<bool> oWregister({"-Wregister"},
    "  -Wregister                  \tWarn about uses of register storage specifier.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-register"));

maplecl::Option<bool> oWreorder({"-Wreorder"},
    "  -Wreorder                   \tWarn when the compiler reorders code.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-reorder"));

maplecl::Option<bool> oWrestrict({"-Wrestrict"},
    "  -Wrestrict                  \tWarn when an argument passed to a restrict-qualified parameter aliases with "
    "another argument.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-restrict"));

maplecl::Option<bool> oWreturnLocalAddr({"-Wreturn-local-addr"},
    "  -Wreturn-local-addr         \tWarn about returning a pointer/reference to a local or temporary variable.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-return-local-addr"));

maplecl::Option<bool> oWreturnType({"-Wreturn-type"},
    "  -Wreturn-type               \tWarn whenever a function's return type defaults to \"int\" (C)\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-return-type"));

maplecl::Option<bool> oWselector({"-Wselector"},
    "  -Wselector                  \tWarn if a selector has multiple methods.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-selector"));

maplecl::Option<bool> oWsequencePoint({"-Wsequence-point"},
    "  -Wsequence-point            \tWarn about possible violations of sequence point rules.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-sequence-point"));

maplecl::Option<bool> oWshadowIvar({"-Wshadow-ivar"},
    "  -Wshadow-ivar               \tWarn if a local declaration hides an instance variable.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-shadow-ivar"));

maplecl::Option<bool> oWshiftCountNegative({"-Wshift-count-negative"},
    "  -Wshift-count-negative      \tWarn if shift count is negative.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-shift-count-negative"));

maplecl::Option<bool> oWshiftCountOverflow({"-Wshift-count-overflow"},
    "  -Wshift-count-overflow      \tWarn if shift count >= width of type.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-shift-count-overflow"));

maplecl::Option<bool> oWsignConversion({"-Wsign-conversion"},
    "  -Wsign-conversion           \tWarn for implicit type conversions between signed and unsigned integers.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-sign-conversion"));

maplecl::Option<bool> oWsignPromo({"-Wsign-promo"},
    "  -Wsign-promo                \tWarn when overload promotes from unsigned to signed.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-sign-promo"));

maplecl::Option<bool> oWsizedDeallocation({"-Wsized-deallocation"},
    "  -Wsized-deallocation        \tWarn about missing sized deallocation functions.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-sized-deallocation"));

maplecl::Option<bool> oWsizeofArrayArgument({"-Wsizeof-array-argument"},
    "  -Wsizeof-array-argument     \tWarn when sizeof is applied on a parameter declared as an array.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-sizeof-array-argument"));

maplecl::Option<bool> oWsizeofPointerMemaccess({"-Wsizeof-pointer-memaccess"},
    "  -Wsizeof-pointer-memaccess  \tWarn about suspicious length parameters to certain string functions if the "
    "argument uses sizeof.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-sizeof-pointer-memaccess"));

maplecl::Option<bool> oWstackProtector({"-Wstack-protector"},
    "  -Wstack-protector           \tWarn when not issuing stack smashing protection for some reason.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-stack-protector"));

maplecl::Option<bool> oWstrictAliasing({"-Wstrict-aliasing"},
    "  -Wstrict-aliasing           \tWarn about code which might break strict aliasing rules.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-strict-aliasing"));

maplecl::Option<bool> oWstrictNullSentinel({"-Wstrict-null-sentinel"},
    "  -Wstrict-null-sentinel      \tWarn about uncasted NULL used as sentinel.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-strict-null-sentinel"));

maplecl::Option<bool> oWstrictOverflow({"-Wstrict-overflow"},
    "  -Wstrict-overflow           \tWarn about optimizations that assume that signed overflow is undefined.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-strict-overflow"));

maplecl::Option<bool> oWstrictSelectorMatch({"-Wstrict-selector-match"},
    "  -Wstrict-selector-match     \tWarn if type signatures of candidate methods do not match exactly.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-strict-selector-match"));

maplecl::Option<bool> oWstringopOverflow({"-Wstringop-overflow"},
    "  -Wstringop-overflow         \tWarn about buffer overflow in string manipulation functions like memcpy "
    "and strcpy.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-stringop-overflow"));

maplecl::Option<bool> oWsubobjectLinkage({"-Wsubobject-linkage"},
    "  -Wsubobject-linkage         \tWarn if a class type has a base or a field whose type uses the anonymous "
    "namespace or depends on a type with no linkage.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-subobject-linkage"));

maplecl::Option<bool> oWsuggestAttributeConst({"-Wsuggest-attribute=const"},
    "  -Wsuggest-attribute=const   \tWarn about functions which might be candidates for __attribute__((const)).\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-suggest-attribute=const"));

maplecl::Option<bool> oWsuggestAttributeFormat({"-Wsuggest-attribute=format"},
    "  -Wsuggest-attribute=format  \tWarn about functions which might be candidates for format attributes.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-suggest-attribute=format"));

maplecl::Option<bool> oWsuggestAttributeNoreturn({"-Wsuggest-attribute=noreturn"},
    "  -Wsuggest-attribute=noreturn\tWarn about functions which might be candidates for __attribute__((noreturn)).\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-suggest-attribute=noreturn"));

maplecl::Option<bool> oWsuggestAttributePure({"-Wsuggest-attribute=pure"},
    "  -Wsuggest-attribute=pure    \tWarn about functions which might be candidates for __attribute__((pure)).\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-suggest-attribute=pure"));

maplecl::Option<bool> oWsuggestFinalMethods({"-Wsuggest-final-methods"},
    "  -Wsuggest-final-methods     \tWarn about C++ virtual methods where adding final keyword would improve "
    "code quality.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-suggest-final-methods"));

maplecl::Option<bool> oWsuggestFinalTypes({"-Wsuggest-final-types"},
    "  -Wsuggest-final-types       \tWarn about C++ polymorphic types where adding final keyword would improve "
    "code quality.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-suggest-final-types"));

maplecl::Option<bool> oWswitch({"-Wswitch"},
    "  -Wswitch                    \tWarn about enumerated switches.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-switch"));

maplecl::Option<bool> oWswitchBool({"-Wswitch-bool"},
    "  -Wswitch-bool               \tWarn about switches with boolean controlling expression.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-switch-bool"));

maplecl::Option<bool> oWswitchDefault({"-Wswitch-default"},
    "  -Wswitch-default            \tWarn about enumerated switches missing a \"default:\" statement.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-switch-default"));

maplecl::Option<bool> oWswitchEnum({"-Wswitch-enum"},
    "  -Wswitch-enum               \tWarn about all enumerated switches missing a specific case.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-switch-enum"));

maplecl::Option<bool> oWswitchUnreachable({"-Wswitch-unreachable"},
    "  -Wswitch-unreachable        \tWarn about statements between switch's controlling expression and the "
    "first case.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-switch-unreachable"));

maplecl::Option<bool> oWsyncNand({"-Wsync-nand"},
    "  -Wsync-nand                 \tWarn when __sync_fetch_and_nand and __sync_nand_and_fetch built-in "
    "functions are used.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-sync-nand"));

maplecl::Option<bool> oWsystemHeaders({"-Wsystem-headers"},
    "  -Wsystem-headers            \tDo not suppress warnings from system headers.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-system-headers"));

maplecl::Option<bool> oWtautologicalCompare({"-Wtautological-compare"},
    "  -Wtautological-compare      \tWarn if a comparison always evaluates to true or false.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-tautological-compare"));

maplecl::Option<bool> oWtemplates({"-Wtemplates"},
    "  -Wtemplates                 \tWarn on primary template declaration.\n",
    {driverCategory, clangCategory}, kOptFront);

maplecl::Option<bool> oWterminate({"-Wterminate"},
    "  -Wterminate                 \tWarn if a throw expression will always result in a call to terminate().\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-terminate"));

maplecl::Option<bool> oWtraditional({"-Wtraditional"},
    "  -Wtraditional               \tWarn about features not present in traditional C. \n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-traditional"));

maplecl::Option<bool> oWtraditionalConversion({"-Wtraditional-conversion"},
    "  -Wtraditional-conversion    \tWarn of prototypes causing type conversions different from what would "
    "happen in the absence of prototype. \n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-traditional-conversion"));

maplecl::Option<bool> oWtrampolines({"-Wtrampolines"},
    "  -Wtrampolines               \tWarn whenever a trampoline is generated. \n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-trampolines"));

maplecl::Option<bool> oWtrigraphs({"-Wtrigraphs"},
    "  -Wtrigraphs                 \tWarn if trigraphs are encountered that might affect the meaning of the program.\n",
    {driverCategory, clangCategory}, kOptFront);

maplecl::Option<bool> oWundeclaredSelector({"-Wundeclared-selector"},
    "  -Wundeclared-selector       \tWarn about @selector()s without previously declared methods. \n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-undeclared-selector"));

maplecl::Option<bool> oWuninitialized({"-Wuninitialized"},
    "  -Wuninitialized             \tWarn about uninitialized automatic variables. \n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-uninitialized"));

maplecl::Option<bool> oWunknownPragmas({"-Wunknown-pragmas"},
    "  -Wunknown-pragmas           \tWarn about unrecognized pragmas. \n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-unknown-pragmas"));

maplecl::Option<bool> oWunsafeLoopOptimizations({"-Wunsafe-loop-optimizations"},
    "  -Wunsafe-loop-optimizations \tWarn if the loop cannot be optimized due to nontrivial assumptions. \n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-unsafe-loop-optimizations"));

maplecl::Option<bool> oWunsuffixedFloatConstants({"-Wunsuffixed-float-constants"},
    "  -Wunsuffixed-float-constants\tWarn about unsuffixed float constants. \n",
    {driverCategory, clangCategory}, kOptFront);

maplecl::Option<bool> oWunused({"-Wunused"},
    "  -Wunused                    \tEnable all -Wunused- warnings. \n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-unused"));

maplecl::Option<bool> oWunusedButSetParameter({"-Wunused-but-set-parameter"},
    "  -Wunused-but-set-parameter  \tWarn when a function parameter is only set, otherwise unused. \n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-unused-but-set-parameter"));

maplecl::Option<bool> oWunusedButSetVariable({"-Wunused-but-set-variable"},
    "  -Wunused-but-set-variable   \tWarn when a variable is only set, otherwise unused. \n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-unused-but-set-variable"));

maplecl::Option<bool> oWunusedConstVariable({"-Wunused-const-variable"},
    "  -Wunused-const-variable     \tWarn when a const variable is unused. \n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-unused-const-variable"));

maplecl::Option<bool> oWunusedFunction({"-Wunused-function"},
    "  -Wunused-function           \tWarn when a function is unused. \n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-unused-function"));

maplecl::Option<bool> oWunusedLabel({"-Wunused-label"},
    "  -Wunused-label              \tWarn when a label is unused. \n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-unused-label"));

maplecl::Option<bool> oWunusedLocalTypedefs({"-Wunused-local-typedefs"},
    "  -Wunused-local-typedefs     \tWarn when typedefs locally defined in a function are not used. \n",
    {driverCategory, clangCategory}, kOptFront);

maplecl::Option<bool> oWunusedResult({"-Wunused-result"},
    "  -Wunused-result             \tWarn if a caller of a function, marked with attribute warn_unused_result,"
    " does not use its return value. \n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-unused-result"));

maplecl::Option<bool> oWunusedValue({"-Wunused-value"},
    "  -Wunused-value              \tWarn when an expression value is unused. \n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-unused-value"));

maplecl::Option<bool> oWunusedVariable({"-Wunused-variable"},
    "  -Wunused-variable           \tWarn when a variable is unused. \n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-unused-variable"));

maplecl::Option<bool> oWuselessCast({"-Wuseless-cast"},
    "  -Wuseless-cast              \tWarn about useless casts. \n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-useless-cast"));

maplecl::Option<bool> oWvarargs({"-Wvarargs"},
    "  -Wvarargs                   \tWarn about questionable usage of the macros used to retrieve variable "
    "arguments.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-varargs"));

maplecl::Option<bool> oWvariadicMacros({"-Wvariadic-macros"},
    "  -Wvariadic-macros           \tWarn about using variadic macros. \n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-variadic-macros"));

maplecl::Option<bool> oWvectorOperationPerformance({"-Wvector-operation-performance"},
    "  -Wvector-operation-performance\n"
    "                              \tWarn when a vector operation is compiled outside the SIMD. \n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-vector-operation-performance"));

maplecl::Option<bool> oWvirtualInheritance({"-Wvirtual-inheritance"},
    "  -Wvirtual-inheritance       \tWarn on direct virtual inheritance.\n",
    {driverCategory, clangCategory}, kOptFront);

maplecl::Option<bool> oWvirtualMoveAssign({"-Wvirtual-move-assign"},
    "  -Wvirtual-move-assign       \tWarn if a virtual base has a non-trivial move assignment operator. \n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-virtual-move-assign"));

maplecl::Option<bool> oWvolatileRegisterVar({"-Wvolatile-register-var"},
    "  -Wvolatile-register-var     \tWarn when a register variable is declared volatile. \n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-volatile-register-var"));

maplecl::Option<bool> oWzeroAsNullPointerConstant({"-Wzero-as-null-pointer-constant"},
    "  -Wzero-as-null-pointer-constant\n"
    "                              \tWarn when a literal '0' is used as null pointer. \n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-zero-as-null-pointer-constant"));

maplecl::Option<bool> oWnoScalarStorageOrder({"-Wno-scalar-storage-order"},
    "  -Wno-scalar-storage-order   \tDo not warn on suspicious constructs involving reverse scalar storage order.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-Wscalar-storage-order"), maplecl::kHide);

maplecl::Option<bool> oWnoReservedIdMacro({"-Wno-reserved-id-macro"},
    "  -Wno-reserved-id-macro      \tDo not warn when macro name is a reserved identifier.\n",
    {driverCategory, clangCategory}, kOptFront);

maplecl::Option<bool> oWnoGnuZeroVariadicMacroArguments({"-Wno-gnu-zero-variadic-macro-arguments"},
    "  -Wno-gnu-zero-variadic-macro-arguments\n"
    "                              \tDo not warn when token pasting of ',' and __VA_ARGS__ is a "
    "GNU extension.\n",
    {driverCategory, clangCategory}, kOptFront);

maplecl::Option<bool> oWnoGnuStatementExpression({"-Wno-gnu-statement-expression"},
    "  -Wno-gnu-statement-expression\n"
    "                              \tDo not warn when use of GNU statement expression extension.\n",
    {driverCategory, clangCategory}, kOptFront);

maplecl::Option<bool> suppressWarnings({"-w"},
    "  -w                          \tSuppress all warnings.\n",
    {driverCategory, clangCategory, asCategory, ldCategory}, kOptFront);

/* ##################### String Options ############################################################### */

maplecl::Option<std::string> oWeakReferenceMismatches({"-weak_reference_mismatches"},
    "  -weak_reference_mismatches  \tSpecifies what to do if a symbol import conflicts between file "
    "(weak in one and not in another) the default is to treat the symbol as non-weak.\n",
    {driverCategory, clangCategory}, kOptFront);

maplecl::Option<std::string> oWerrorE({"-Werror="},
    "  -Werror=                    \tTreat specified warning as error.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-error="));

maplecl::Option<std::string> oWnormalizedE({"-Wnormalized="},
    "  -Wnormalized=               \t-Wnormalized=[none|id|nfc|nfkc]   Warn about non-normalized Unicode strings.\n",
    {driverCategory, clangCategory}, kOptFront);

maplecl::Option<std::string> oWplacementNewE({"-Wplacement-new="},
    "  -Wplacement-new=             \tWarn for placement new expressions with undefined behavior.\n",
    {driverCategory, clangCategory}, kOptFront);

maplecl::Option<std::string> oWstackUsage({"-Wstack-usage="},
    "  -Wstack-usage=              \t-Wstack-usage=<byte-size> Warn if stack usage might exceed <byte-size>.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<std::string> oWstrictAliasingE({"-Wstrict-aliasing="},
    "  -Wstrict-aliasing=          \tWarn about code which might break strict aliasing rules.\n",
    {driverCategory, clangCategory}, kOptFront);

/* ###################################################################################################### */

}

namespace maple {

Triple::ArchType Triple::ParseArch(const std::string_view archStr) const {
  if (maple::utils::Contains({"aarch64", "aarch64_le"}, archStr)) {
    return Triple::ArchType::kAarch64;
  } else if (maple::utils::Contains({"aarch64_be"}, archStr)) {
    return Triple::ArchType::kAarch64Be;
  }

  // Currently Triple support only aarch64
  return Triple::kUnknownArch;
}

Triple::EnvironmentType Triple::ParseEnvironment(const std::string_view archStr) const {
  if (maple::utils::Contains({"ilp32", "gnu_ilp32", "gnuilp32"}, archStr)) {
    return Triple::EnvironmentType::kGnuIlp32;
  } else if (maple::utils::Contains({"gnu"}, archStr)) {
    return Triple::EnvironmentType::kGnu;
  }

  // Currently Triple support only ilp32 and default gnu/LP64 ABI
  return Triple::kUnknownEnvironment;
}

void Triple::Init() {
  /* Currently Triple is used only to configure aarch64: be/le, ILP32/LP64
   * Other architectures (TARGX86_64, TARGX86, TARGARM32, TARGVM) are configured with compiler build config */
#if TARGAARCH64
  arch = (opts::bigendian) ? Triple::ArchType::kAarch64Be : Triple::ArchType::kAarch64;
  environment = (opts::ilp32) ? Triple::EnvironmentType::kGnuIlp32 : Triple::EnvironmentType::kGnu;

  if (opts::mabi.IsEnabledByUser()) {
    auto tmpEnvironment = ParseEnvironment(opts::mabi.GetValue());
    if (tmpEnvironment != Triple::kUnknownEnvironment) {
      environment = tmpEnvironment;
    }
  }
#endif
}

void Triple::Init(const std::string &target) {
  data = target;

  /* Currently Triple is used only to configure aarch64: be/le, ILP32/LP64.
   * Other architectures (TARGX86_64, TARGX86, TARGARM32, TARGVM) are configured with compiler build config */
#if TARGAARCH64
  Init();

  std::vector<std::string_view> components;
  maple::StringUtils::SplitSV(data, components, '-');
  if (components.size() == 0) { // as minimum 1 component must be
    return;
  }

  auto tmpArch = ParseArch(components[0]); // to not overwrite arch seting by opts::bigendian
  if (tmpArch == Triple::kUnknownArch) {
    return;
  }
  arch = tmpArch;

  /* Try to check environment in option.
   * As example, it can be: aarch64-none-linux-gnu or aarch64-linux-gnu or aarch64-gnu, where gnu is environment */
  for (uint i = 1; i < components.size(); ++i) {
    auto tmpEnvironment = ParseEnvironment(components[i]);
    if (tmpEnvironment != Triple::kUnknownEnvironment) {
      environment = tmpEnvironment;
      break;
    }
  }
#endif
}

std::string Triple::GetArchName() const {
  switch (arch) {
    case ArchType::kAarch64Be: return "aarch64_be";
    case ArchType::kAarch64: return "aarch64";
    default: ASSERT(false, "Unknown Architecture Type\n");
  }
  return "";
}

std::string Triple::GetEnvironmentName() const {
  switch (environment) {
    case EnvironmentType::kGnuIlp32: return "gnu_ilp32";
    case EnvironmentType::kGnu: return "gnu";
    default: ASSERT(false, "Unknown Environment Type\n");
  }
  return "";
}

std::string Triple::Str() const {
  if (!data.empty()) {
    return data;
  }

  if (GetArch() != ArchType::kUnknownArch &&
      GetEnvironment() != Triple::EnvironmentType::kUnknownEnvironment) {
    /* only linux platform is supported, so "-linux-" is hardcoded */
    return GetArchName() + "-linux-" + GetEnvironmentName();
  }

  CHECK_FATAL(false, "Only aarch64/aarch64_be GNU/GNUILP32 targets are supported\n");
  return data;
}

} // namespace maple
