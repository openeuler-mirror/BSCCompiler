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

#ifndef HIR2MPL_COMMON_INCLUDE_HIR2MPL_OPTION_H
#define HIR2MPL_COMMON_INCLUDE_HIR2MPL_OPTION_H

#include "driver_options.h"

#include <stdint.h>
#include <string>

namespace opts::hir2mpl {
extern cl::Option<bool> help;
extern cl::Option<bool> version;
extern cl::Option<std::string> mpltSys;
extern cl::Option<std::string> mpltApk;
extern cl::Option<std::string> mplt;
extern cl::Option<std::string> inClass;
extern cl::Option<std::string> inJar;
extern cl::Option<std::string> inDex;
extern cl::Option<std::string> inAst;
extern cl::Option<std::string> inMast;
extern cl::Option<std::string> output;
extern cl::Option<std::string> outputName;
extern cl::Option<bool> mpltOnly;
extern cl::Option<bool> asciimplt;
extern cl::Option<bool> dumpInstComment;
extern cl::Option<bool> noMplFile;
extern cl::Option<uint32_t> dumpLevel;
extern cl::Option<bool> dumpTime;
extern cl::Option<bool> dumpComment;
extern cl::Option<bool> dumpLOC;
extern cl::Option<bool> dbgFriendly;
extern cl::Option<bool> dumpPhaseTime;
extern cl::Option<bool> dumpPhaseTimeDetail;
extern cl::Option<bool> rc;
extern cl::Option<bool> nobarrier;
extern cl::Option<bool> usesignedchar;
extern cl::Option<bool> be;
extern cl::Option<bool> o2;
extern cl::Option<bool> simplifyShortCircuit;
extern cl::Option<bool> enableVariableArray;
extern cl::Option<uint32_t> funcInliceSize;
extern cl::Option<uint32_t> np;
extern cl::Option<bool> dumpThreadTime;
extern cl::Option<std::string> xbootclasspath;
extern cl::Option<std::string> classloadercontext;
extern cl::Option<std::string> dep;
extern cl::Option<std::string> depsamename;
extern cl::Option<bool> npeCheckDynamic;
extern cl::Option<bool> boundaryCheckDynamic;
extern cl::Option<bool> safeRegion;
extern cl::Option<bool> dumpFEIRBB;
extern cl::Option<std::string> dumpFEIRCFGGraph;
extern cl::Option<bool> wpaa;
extern cl::Option<bool> debug;

}

#endif /* HIR2MPL_COMMON_INCLUDE_HIR2MPL_OPTION_H */