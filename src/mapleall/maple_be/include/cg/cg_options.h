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

namespace opts::cg {

extern maplecl::Option<bool> fpie;
extern maplecl::Option<bool> fpic;
extern maplecl::Option<bool> fPIE;
extern maplecl::Option<bool> fPIC;
extern maplecl::Option<bool> fnoSemanticInterposition;
extern maplecl::Option<bool> verboseAsm;
extern maplecl::Option<bool> verboseCg;
extern maplecl::Option<bool> maplelinker;
extern maplecl::Option<bool> quiet;
extern maplecl::Option<bool> cg;
extern maplecl::Option<bool> replaceAsm;
extern maplecl::Option<bool> generalRegOnly;
extern maplecl::Option<bool> lazyBinding;
extern maplecl::Option<bool> hotFix;
extern maplecl::Option<bool> ebo;
extern maplecl::Option<bool> ipara;
extern maplecl::Option<bool> cfgo;
extern maplecl::Option<bool> ico;
extern maplecl::Option<bool> storeloadopt;
extern maplecl::Option<bool> globalopt;
extern maplecl::Option<bool> hotcoldsplit;
extern maplecl::Option<bool> prelsra;
extern maplecl::Option<bool> lsraLvarspill;
extern maplecl::Option<bool> lsraOptcallee;
extern maplecl::Option<bool> calleeregsPlacement;
extern maplecl::Option<bool> ssapreSave;
extern maplecl::Option<bool> ssupreRestore;
extern maplecl::Option<bool> newCg;
extern maplecl::Option<bool> prepeep;
extern maplecl::Option<bool> peep;
extern maplecl::Option<bool> preschedule;
extern maplecl::Option<bool> schedule;
extern maplecl::Option<bool> retMerge;
extern maplecl::Option<bool> vregRename;
extern maplecl::Option<bool> fullcolor;
extern maplecl::Option<bool> writefieldopt;
extern maplecl::Option<bool> dumpOlog;
extern maplecl::Option<bool> nativeopt;
extern maplecl::Option<bool> objmap;
extern maplecl::Option<bool> yieldpoint;
extern maplecl::Option<bool> proepilogue;
extern maplecl::Option<bool> localRc;
extern maplecl::Option<bool> addDebugTrace;
extern maplecl::Option<std::string> classListFile;
extern maplecl::Option<bool> genCMacroDef;
extern maplecl::Option<bool> genGctibFile;
extern maplecl::Option<bool> unwindTables;
extern maplecl::Option<bool> debug;
extern maplecl::Option<bool> gdwarf;
extern maplecl::Option<bool> gsrc;
extern maplecl::Option<bool> gmixedsrc;
extern maplecl::Option<bool> gmixedasm;
extern maplecl::Option<bool> profile;
extern maplecl::Option<bool> withRaLinearScan;
extern maplecl::Option<bool> withRaGraphColor;
extern maplecl::Option<bool> patchLongBranch;
extern maplecl::Option<bool> constFold;
extern maplecl::Option<std::string> ehExclusiveList;
extern maplecl::Option<bool> o0;
extern maplecl::Option<bool> o1;
extern maplecl::Option<bool> o2;
extern maplecl::Option<bool> os;
extern maplecl::Option<bool> oLitecg;
extern maplecl::Option<uint64_t> lsraBb;
extern maplecl::Option<uint64_t> lsraInsn;
extern maplecl::Option<uint64_t> lsraOverlap;
extern maplecl::Option<uint8_t> remat;
extern maplecl::Option<bool> suppressFileinfo;
extern maplecl::Option<bool> dumpCfg;
extern maplecl::Option<std::string> target;
extern maplecl::Option<std::string> dumpPhases;
extern maplecl::Option<std::string> skipPhases;
extern maplecl::Option<std::string> skipFrom;
extern maplecl::Option<std::string> skipAfter;
extern maplecl::Option<std::string> dumpFunc;
extern maplecl::Option<bool> timePhases;
extern maplecl::Option<bool> useBarriersForVolatile;
extern maplecl::Option<std::string> range;
extern maplecl::Option<uint8_t> fastAlloc;
extern maplecl::Option<std::string> spillRange;
extern maplecl::Option<bool> dupBb;
extern maplecl::Option<bool> calleeCfi;
extern maplecl::Option<bool> printFunc;
extern maplecl::Option<std::string> cyclePatternList;
extern maplecl::Option<std::string> duplicateAsmList;
extern maplecl::Option<std::string> duplicateAsmList2;
extern maplecl::Option<std::string> blockMarker;
extern maplecl::Option<bool> soeCheck;
extern maplecl::Option<bool> checkArraystore;
extern maplecl::Option<bool> debugSchedule;
extern maplecl::Option<bool> bruteforceSchedule;
extern maplecl::Option<bool> simulateSchedule;
extern maplecl::Option<bool> crossLoc;
extern maplecl::Option<std::string> floatAbi;
extern maplecl::Option<std::string> filetype;
extern maplecl::Option<bool> longCalls;
extern maplecl::Option<bool> functionSections;
extern maplecl::Option<bool> omitFramePointer;
extern maplecl::Option<bool> omitLeafFramePointer;
extern maplecl::Option<bool> fastMath;
extern maplecl::Option<bool> tailcall;
extern maplecl::Option<bool> alignAnalysis;
extern maplecl::Option<bool> cgSsa;
extern maplecl::Option<bool> layoutColdPath;
extern maplecl::Option<bool> globalSchedule;
extern maplecl::Option<bool> localSchedule;
extern maplecl::Option<bool> calleeEnsureParam;
extern maplecl::Option<bool> common;
extern maplecl::Option<bool> condbrAlign;
extern maplecl::Option<uint32_t> alignMinBbSize;
extern maplecl::Option<uint32_t> alignMaxBbSize;
extern maplecl::Option<uint32_t> loopAlignPow;
extern maplecl::Option<uint32_t> jumpAlignPow;
extern maplecl::Option<uint32_t> funcAlignPow;
extern maplecl::Option<uint32_t> coldPathThreshold;
extern maplecl::Option<bool> litePgoGen;
extern maplecl::Option<std::string> litePgoOutputFunc;
extern maplecl::Option<std::string> instrumentationDir;
extern maplecl::Option<std::string> litePgoWhiteList;
extern maplecl::Option<std::string> litePgoFile;
extern maplecl::Option<std::string> functionPriority;
extern maplecl::Option<bool> litePgoVerify;
extern maplecl::Option<bool> optimizedFrameLayout;
}

#endif /* MAPLE_BE_INCLUDE_CG_OPTIONS_H */
