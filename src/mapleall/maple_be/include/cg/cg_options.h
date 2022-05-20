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

#ifndef MAPLE_BE_INCLUDE_CG_OPTIONS_H
#define MAPLE_BE_INCLUDE_CG_OPTIONS_H

#include "cl_option.h"
#include "cl_parser.h"

#include <bits/stdint-uintn.h>
#include <stdint.h>
#include <string>

namespace opts::cg {

extern cl::Option<bool> pie;
extern cl::Option<bool> fpic;
extern cl::Option<bool> verboseAsm;
extern cl::Option<bool> verboseCg;
extern cl::Option<bool> maplelinker;
extern cl::Option<bool> quiet;
extern cl::Option<bool> cg;
extern cl::Option<bool> replaceAsm;
extern cl::Option<bool> generalRegOnly;
extern cl::Option<bool> lazyBinding;
extern cl::Option<bool> hotFix;
extern cl::Option<bool> ebo;
extern cl::Option<bool> cfgo;
extern cl::Option<bool> ico;
extern cl::Option<bool> storeloadopt;
extern cl::Option<bool> globalopt;
extern cl::Option<bool> hotcoldsplit;
extern cl::Option<bool> prelsra;
extern cl::Option<bool> lsraLvarspill;
extern cl::Option<bool> lsraOptcallee;
extern cl::Option<bool> calleeregsPlacement;
extern cl::Option<bool> ssapreSave;
extern cl::Option<bool> ssupreRestore;
extern cl::Option<bool> prepeep;
extern cl::Option<bool> peep;
extern cl::Option<bool> preschedule;
extern cl::Option<bool> schedule;
extern cl::Option<bool> retMerge;
extern cl::Option<bool> vregRename;
extern cl::Option<bool> fullcolor;
extern cl::Option<bool> writefieldopt;
extern cl::Option<bool> dumpOlog;
extern cl::Option<bool> nativeopt;
extern cl::Option<bool> objmap;
extern cl::Option<bool> yieldpoint;
extern cl::Option<bool> proepilogue;
extern cl::Option<bool> localRc;
extern cl::Option<std::string> insertCall;
extern cl::Option<bool> addDebugTrace;
extern cl::Option<bool> addFuncProfile;
extern cl::Option<std::string> classListFile;
extern cl::Option<bool> genCMacroDef;
extern cl::Option<bool> genGctibFile;
extern cl::Option<bool> stackProtectorStrong;
extern cl::Option<bool> stackProtectorAll;
extern cl::Option<bool> debug;
extern cl::Option<bool> gdwarf;
extern cl::Option<bool> gsrc;
extern cl::Option<bool> gmixedsrc;
extern cl::Option<bool> gmixedasm;
extern cl::Option<bool> profile;
extern cl::Option<bool> withRaLinearScan;
extern cl::Option<bool> withRaGraphColor;
extern cl::Option<bool> patchLongBranch;
extern cl::Option<bool> constFold;
extern cl::Option<std::string> ehExclusiveList;
extern cl::Option<bool> o0;
extern cl::Option<bool> o1;
extern cl::Option<bool> o2;
extern cl::Option<bool> os;
extern cl::Option<uint64_t> lsraBb;
extern cl::Option<uint64_t> lsraInsn;
extern cl::Option<uint64_t> lsraOverlap;
extern cl::Option<uint8_t> remat;
extern cl::Option<bool> suppressFileinfo;
extern cl::Option<bool> dumpCfg;
extern cl::Option<std::string> target;
extern cl::Option<std::string> dumpPhases;
extern cl::Option<std::string> skipPhases;
extern cl::Option<std::string> skipFrom;
extern cl::Option<std::string> skipAfter;
extern cl::Option<std::string> dumpFunc;
extern cl::Option<bool> timePhases;
extern cl::Option<bool> useBarriersForVolatile;
extern cl::Option<std::string> range;
extern cl::Option<uint8_t> fastAlloc;
extern cl::Option<std::string> spillRange;
extern cl::Option<bool> dupBb;
extern cl::Option<bool> calleeCfi;
extern cl::Option<bool> printFunc;
extern cl::Option<std::string> cyclePatternList;
extern cl::Option<std::string> duplicateAsmList;
extern cl::Option<std::string> duplicateAsmList2;
extern cl::Option<std::string> blockMarker;
extern cl::Option<bool> soeCheck;
extern cl::Option<bool> checkArraystore;
extern cl::Option<bool> debugSchedule;
extern cl::Option<bool> bruteforceSchedule;
extern cl::Option<bool> simulateSchedule;
extern cl::Option<bool> crossLoc;
extern cl::Option<std::string> floatAbi;
extern cl::Option<std::string> filetype;
extern cl::Option<bool> longCalls;
extern cl::Option<bool> functionSections;
extern cl::Option<bool> omitFramePointer;
extern cl::Option<bool> fastMath;
extern cl::Option<bool> tailcall;
extern cl::Option<bool> alignAnalysis;
extern cl::Option<bool> cgSsa;
extern cl::Option<bool> common;
extern cl::Option<bool> arm64Ilp32;
extern cl::Option<bool> condbrAlign;
extern cl::Option<uint32_t> alignMinBbSize;
extern cl::Option<uint32_t> alignMaxBbSize;
extern cl::Option<uint32_t> loopAlignPow;
extern cl::Option<uint32_t> jumpAlignPow;
extern cl::Option<uint32_t> funcAlignPow;

}

#endif /* MAPLE_BE_INCLUDE_CG_OPTIONS_H */
