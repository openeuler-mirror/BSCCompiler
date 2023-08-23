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

maplecl::Option<bool> oDumpversion({"-dumpversion"},
    "  -dumpversion                \tPrint the compiler version (for example, 3.0, 6.3.0 or 7) and don't do"
    " anything else.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

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

maplecl::Option<bool> oMbmi({"-mbmi"},
    "  -mbmi                       \tThese switches enable the use of instructions in the mbmi.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMbranchCheap({"-mbranch-cheap"},
    "  -mbranch-cheap              \tDo not pretend that branches are expensive.\n",
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

maplecl::Option<bool> oMintRegister({"-mint-register"},
    "  -mint-register              \t\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<bool> oMint16({"-mint16"},
    "  -mint16                     \tUse 16-bit int.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-int16"), maplecl::kHide);

maplecl::Option<bool> oMint32({"-mint32"},
    "  -mint32                     \tUse 32-bit int.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("--mno-int32"), maplecl::kHide);

}
