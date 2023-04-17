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

#ifndef MAPLE_DRIVER_INCLUDE_DRIVER_OPTIONS_H
#define MAPLE_DRIVER_INCLUDE_DRIVER_OPTIONS_H

#include "cl_option.h"
#include "cl_parser.h"

static maplecl::OptionCategory &driverCategory = maplecl::CommandLine::GetCommandLine().defaultCategory;

static maplecl::OptionCategory &clangCategory = maplecl::CommandLine::GetCommandLine().clangCategory;
static maplecl::OptionCategory &hir2mplCategory = maplecl::CommandLine::GetCommandLine().hir2mplCategory;
static maplecl::OptionCategory &mpl2mplCategory = maplecl::CommandLine::GetCommandLine().mpl2mplCategory;
static maplecl::OptionCategory &meCategory = maplecl::CommandLine::GetCommandLine().meCategory;
static maplecl::OptionCategory &cgCategory = maplecl::CommandLine::GetCommandLine().cgCategory;
static maplecl::OptionCategory &asCategory = maplecl::CommandLine::GetCommandLine().asCategory;
static maplecl::OptionCategory &ldCategory = maplecl::CommandLine::GetCommandLine().ldCategory;

static maplecl::OptionCategory &dex2mplCategory = maplecl::CommandLine::GetCommandLine().dex2mplCategory;
static maplecl::OptionCategory &jbc2mplCategory = maplecl::CommandLine::GetCommandLine().jbc2mplCategory;
static maplecl::OptionCategory &ipaCategory = maplecl::CommandLine::GetCommandLine().ipaCategory;

static maplecl::OptionCategory &unSupCategory = maplecl::CommandLine::GetCommandLine().unSupCategory;

namespace opts {

/* ##################### BOOL Options ############################################################### */

extern maplecl::Option<bool> version;
extern maplecl::Option<bool> ignoreUnkOpt;
extern maplecl::Option<bool> o0;
extern maplecl::Option<bool> o1;
extern maplecl::Option<bool> o2;
extern maplecl::Option<bool> o3;
extern maplecl::Option<bool> os;
extern maplecl::Option<bool> verify;
extern maplecl::Option<bool> decoupleStatic;
extern maplecl::Option<bool> bigEndian;
extern maplecl::Option<bool> gcOnly;
extern maplecl::Option<bool> timePhase;
extern maplecl::Option<bool> genMeMpl;
extern maplecl::Option<bool> compileWOLink;
extern maplecl::Option<bool> genVtable;
extern maplecl::Option<bool> verbose;
extern maplecl::Option<bool> debug;
extern maplecl::Option<bool> withDwarf;
extern maplecl::Option<bool> withIpa;
extern maplecl::Option<bool> npeNoCheck;
extern maplecl::Option<bool> npeStaticCheck;
extern maplecl::Option<bool> npeDynamicCheck;
extern maplecl::Option<bool> npeDynamicCheckSilent;
extern maplecl::Option<bool> npeDynamicCheckAll;
extern maplecl::Option<bool> boundaryNoCheck;
extern maplecl::Option<bool> boundaryStaticCheck;
extern maplecl::Option<bool> boundaryDynamicCheck;
extern maplecl::Option<bool> boundaryDynamicCheckSilent;
extern maplecl::Option<bool> safeRegionOption;
extern maplecl::Option<bool> printDriverPhases;
extern maplecl::Option<bool> ldStatic;
extern maplecl::Option<bool> maplePhase;
extern maplecl::Option<bool> genMapleBC;
extern maplecl::Option<bool> genLMBC;
extern maplecl::Option<bool> profileGen;
extern maplecl::Option<bool> profileUse;
extern maplecl::Option<bool> missingProfDataIsError;
extern maplecl::Option<bool> stackProtectorStrong;
extern maplecl::Option<bool> stackProtectorAll;
extern maplecl::Option<bool> inlineAsWeak;
extern maplecl::Option<bool> enableArithCheck;
extern maplecl::Option<bool> enableCallFflush;
extern maplecl::Option<bool> onlyCompile;
extern maplecl::Option<bool> fNoPlt;
extern maplecl::Option<bool> usePipe;
extern maplecl::Option<bool> fDataSections;
extern maplecl::Option<bool> fRegStructReturn;
extern maplecl::Option<bool> fTreeVectorize;
extern maplecl::Option<bool> fNoStrictAliasing;
extern maplecl::Option<bool> fNoFatLtoObjects;
extern maplecl::Option<bool> gcSections;
extern maplecl::Option<bool> copyDtNeededEntries;
extern maplecl::Option<bool> sOpt;
extern maplecl::Option<bool> noStdinc;
extern maplecl::Option<bool> pie;
extern maplecl::Option<bool> fStrongEvalOrder;
extern maplecl::Option<bool> linkerTimeOpt;
extern maplecl::Option<bool> expand128Floats;
extern maplecl::Option<bool> shared;
extern maplecl::Option<bool> rdynamic;
extern maplecl::Option<bool> dndebug;
extern maplecl::Option<bool> usesignedchar;
extern maplecl::Option<bool> suppressWarnings;
extern maplecl::Option<bool> pthread;
extern maplecl::Option<bool> passO2ToClang;
extern maplecl::Option<bool> defaultSafe;
extern maplecl::Option<bool> onlyPreprocess;
extern maplecl::Option<bool> noStdLib;
extern maplecl::Option<bool> r;

/* ##################### STRING Options ############################################################### */

extern maplecl::Option<std::string> help;
extern maplecl::Option<std::string> infile;
extern maplecl::Option<std::string> mplt;
extern maplecl::Option<std::string> partO2;
extern maplecl::List<std::string> jbc2mplOpt;
extern maplecl::List<std::string> hir2mplOpt;
extern maplecl::List<std::string> clangOpt;
extern maplecl::List<std::string> asOpt;
extern maplecl::List<std::string> ldOpt;
extern maplecl::List<std::string> dex2mplOpt;
extern maplecl::List<std::string> mplipaOpt;
extern maplecl::List<std::string> mplcgOpt;
extern maplecl::List<std::string> meOpt;
extern maplecl::List<std::string> mpl2mplOpt;
extern maplecl::Option<std::string> profile;
extern maplecl::Option<std::string> run;
extern maplecl::Option<std::string> optionOpt;
extern maplecl::List<std::string> ldLib;
extern maplecl::List<std::string> ldLibPath;
extern maplecl::List<std::string> enableMacro;
extern maplecl::List<std::string> disableMacro;
extern maplecl::List<std::string> includeDir;
extern maplecl::List<std::string> includeSystem;
extern maplecl::Option<std::string> output;
extern maplecl::Option<std::string> saveTempOpt;
extern maplecl::Option<std::string> target;
extern maplecl::Option<std::string> linkerTimeOptE;
extern maplecl::Option<std::string> MT;
extern maplecl::Option<std::string> MF;
extern maplecl::Option<std::string> std;
extern maplecl::Option<std::string> Wl;
extern maplecl::Option<std::string> fVisibility;
extern maplecl::Option<std::string> fStrongEvalOrderE;
extern maplecl::Option<std::string> march;
extern maplecl::Option<std::string> sysRoot;
extern maplecl::Option<std::string> specs;
extern maplecl::Option<std::string> folder;
#ifdef ENABLE_MAPLE_SAN
extern maplecl::Option<std::string> sanitizer;
#endif

/* ##################### DIGITAL Options ############################################################### */

extern maplecl::Option<uint32_t> helpLevel;
extern maplecl::Option<uint32_t> funcInliceSize;

/* ##################### Warnings Options ############################################################### */

extern maplecl::Option<bool> wUnusedMacro;
extern maplecl::Option<bool> wBadFunctionCast;
extern maplecl::Option<bool> wStrictPrototypes;
extern maplecl::Option<bool> wUndef;
extern maplecl::Option<bool> wCastQual;
extern maplecl::Option<bool> wMissingFieldInitializers;
extern maplecl::Option<bool> wUnusedParameter;
extern maplecl::Option<bool> wAll;
extern maplecl::Option<bool> wExtra;
extern maplecl::Option<bool> wWriteStrings;
extern maplecl::Option<bool> wVla;
extern maplecl::Option<bool> wFormatSecurity;
extern maplecl::Option<bool> wShadow;
extern maplecl::Option<bool> wTypeLimits;
extern maplecl::Option<bool> wSignCompare;
extern maplecl::Option<bool> wShiftNegativeValue;
extern maplecl::Option<bool> wPointerArith;
extern maplecl::Option<bool> wIgnoredQualifiers;
extern maplecl::Option<bool> wFormat;
extern maplecl::Option<bool> wFloatEqual;
extern maplecl::Option<bool> wDateTime;
extern maplecl::Option<bool> wImplicitFallthrough;
extern maplecl::Option<bool> wShiftOverflow;

/* #################################################################################################### */

} /* opts */

#endif /* MAPLE_DRIVER_INCLUDE_DRIVER_OPTIONS_H */
