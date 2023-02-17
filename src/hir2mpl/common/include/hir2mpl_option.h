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

namespace opts::hir2mpl {
extern maplecl::Option<bool> help;
extern maplecl::Option<bool> version;
extern maplecl::Option<std::string> mpltSys;
extern maplecl::Option<std::string> mpltApk;
extern maplecl::Option<std::string> mplt;
extern maplecl::Option<std::string> inClass;
extern maplecl::Option<std::string> inJar;
extern maplecl::Option<std::string> inDex;
extern maplecl::Option<std::string> inAst;
extern maplecl::Option<std::string> inMast;
extern maplecl::Option<std::string> output;
extern maplecl::Option<std::string> outputName;
extern maplecl::Option<bool> mpltOnly;
extern maplecl::Option<bool> asciimplt;
extern maplecl::Option<bool> dumpInstComment;
extern maplecl::Option<bool> noMplFile;
extern maplecl::Option<uint32_t> dumpLevel;
extern maplecl::Option<bool> dumpTime;
extern maplecl::Option<bool> dumpComment;
extern maplecl::Option<bool> dumpLOC;
extern maplecl::Option<bool> dbgFriendly;
extern maplecl::Option<bool> dumpPhaseTime;
extern maplecl::Option<bool> dumpPhaseTimeDetail;
extern maplecl::Option<bool> rc;
extern maplecl::Option<bool> nobarrier;
extern maplecl::Option<bool> o2;
extern maplecl::Option<bool> simplifyShortCircuit;
extern maplecl::Option<bool> enableVariableArray;
extern maplecl::Option<uint32_t> funcInliceSize;
extern maplecl::Option<uint32_t> np;
extern maplecl::Option<bool> dumpThreadTime;
extern maplecl::Option<std::string> xbootclasspath;
extern maplecl::Option<std::string> classloadercontext;
extern maplecl::Option<std::string> dep;
extern maplecl::Option<std::string> depsamename;
extern maplecl::Option<bool> dumpFEIRBB;
extern maplecl::Option<std::string> dumpFEIRCFGGraph;
extern maplecl::Option<bool> wpaa;
extern maplecl::Option<bool> debug;

}

#endif /* HIR2MPL_COMMON_INCLUDE_HIR2MPL_OPTION_H */
