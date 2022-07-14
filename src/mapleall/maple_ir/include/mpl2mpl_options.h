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

#ifndef MAPLE_IR_INCLUDE_MPL2MPL_OPTION_H
#define MAPLE_IR_INCLUDE_MPL2MPL_OPTION_H

#include "cl_option.h"
#include "cl_parser.h"

#include <bits/stdint-uintn.h>
#include <stdint.h>
#include <string>

namespace opts::mpl2mpl {

extern maplecl::Option<std::string> dumpPhase;
extern maplecl::Option<std::string> skipPhase;
extern maplecl::Option<std::string> skipFrom;
extern maplecl::Option<std::string> skipAfter;
extern maplecl::Option<std::string> dumpFunc;
extern maplecl::Option<bool> quiet;
extern maplecl::Option<bool> mapleLinker;
extern maplecl::Option<bool> regNativeFunc;
extern maplecl::Option<bool> inlineWithProfile;
extern maplecl::Option<bool> inlineOpt;
extern maplecl::Option<bool> ipaClone;
extern maplecl::Option<std::string> noInlineFunc;
extern maplecl::Option<std::string> importFileList;
extern maplecl::Option<bool> crossModuleInline;
extern maplecl::Option<uint32_t> inlineSmallFunctionThreshold;
extern maplecl::Option<uint32_t> inlineHotFunctionThreshold;
extern maplecl::Option<uint32_t> inlineRecursiveFunctionThreshold;
extern maplecl::Option<uint32_t> inlineDepth;
extern maplecl::Option<uint32_t> inlineModuleGrow;
extern maplecl::Option<uint32_t> inlineColdFuncThresh;
extern maplecl::Option<uint32_t> profileHotCount;
extern maplecl::Option<uint32_t> profileColdCount;
extern maplecl::Option<uint32_t> profileHotRate;
extern maplecl::Option<uint32_t> profileColdRate;
extern maplecl::Option<bool> nativeWrapper;
extern maplecl::Option<bool> regNativeDynamicOnly;
extern maplecl::Option<std::string> staticBindingList;
extern maplecl::Option<bool> dumpBefore;
extern maplecl::Option<bool> dumpAfter;
extern maplecl::Option<bool> dumpMuid;
extern maplecl::Option<bool> emitVtableImpl;

#if MIR_JAVA
extern maplecl::Option<bool> skipVirtual;
#endif

extern maplecl::Option<bool> userc;
extern maplecl::Option<bool> strictNaiveRc;
extern maplecl::Option<bool> rcOpt1;
extern maplecl::Option<bool> nativeOpt;
extern maplecl::Option<bool> o0;
extern maplecl::Option<bool> o2;
extern maplecl::Option<bool> os;
extern maplecl::Option<std::string> criticalNative;
extern maplecl::Option<std::string> fastNative;
extern maplecl::Option<bool> noDot;
extern maplecl::Option<bool> genIrProfile;
extern maplecl::Option<bool> proFileTest;
extern maplecl::Option<bool> barrier;
extern maplecl::Option<std::string> nativeFuncPropertyFile;
extern maplecl::Option<bool> mapleLinkerNolocal;
extern maplecl::Option<uint32_t> buildApp;
extern maplecl::Option<bool> partialAot;
extern maplecl::Option<uint32_t> decoupleInit;
extern maplecl::Option<std::string> sourceMuid;
extern maplecl::Option<bool> deferredVisit;
extern maplecl::Option<bool> deferredVisit2;
extern maplecl::Option<bool> decoupleSuper;
extern maplecl::Option<bool> genDecoupleVtab;
extern maplecl::Option<bool> profileFunc;
extern maplecl::Option<std::string> dumpDevirtual;
extern maplecl::Option<std::string> readDevirtual;
extern maplecl::Option<bool> useWhiteClass;
extern maplecl::Option<std::string> appPackageName;
extern maplecl::Option<std::string> checkClInvocation;
extern maplecl::Option<bool> dumpClInvocation;
extern maplecl::Option<uint32_t> warning;
extern maplecl::Option<bool> lazyBinding;
extern maplecl::Option<bool> hotFix;
extern maplecl::Option<bool> compactMeta;
extern maplecl::Option<bool> genPGOReport;
extern maplecl::Option<uint32_t> inlineCache;
extern maplecl::Option<bool> noComment;
extern maplecl::Option<bool> rmNouseFunc;
extern maplecl::Option<bool> sideEffect;
extern maplecl::Option<bool> dumpIPA;
extern maplecl::Option<bool> wpaa;
extern maplecl::Option<uint32_t> numOfCloneVersions;
extern maplecl::Option<uint32_t> numOfImpExprLowBound;
extern maplecl::Option<uint32_t> numOfImpExprHighBound;
extern maplecl::Option<uint32_t> numOfCallSiteLowBound;
extern maplecl::Option<uint32_t> numOfCallSiteUpBound;
extern maplecl::Option<uint32_t> numOfConstpropValue;

}

#endif /* MAPLE_IR_INCLUDE_MPL2MPL_OPTION_H */
