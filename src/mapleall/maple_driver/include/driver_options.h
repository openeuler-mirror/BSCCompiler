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

#include <string>

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

namespace opts {

/* ##################### BOOL Options ############################################################### */

extern maplecl::Option<bool> version;
extern maplecl::Option<bool> o0;
extern maplecl::Option<bool> o1;
extern maplecl::Option<bool> o2;
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

/* ##################### DIGITAL Options ############################################################### */

extern maplecl::Option<uint32_t> helpLevel;

/* #################################################################################################### */

} /* opts */

#endif /* MAPLE_DRIVER_INCLUDE_DRIVER_OPTIONS_H */
