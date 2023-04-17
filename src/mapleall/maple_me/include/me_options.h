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

#ifndef MAPLE_ME_INCLUDE_ME_OPTIONS_H
#define MAPLE_ME_INCLUDE_ME_OPTIONS_H

#include "cl_option.h"
#include "cl_parser.h"
#include "types_def.h"

#include <cstdint>
#include <string>

namespace opts::me {

extern maplecl::Option<bool> help;
extern maplecl::Option<bool> o1;
extern maplecl::Option<bool> o2;
extern maplecl::Option<bool> os;
extern maplecl::Option<bool> o3;
extern maplecl::Option<std::string> refusedcheck;
extern maplecl::Option<std::string> range;
extern maplecl::Option<std::string> pgoRange;
extern maplecl::Option<std::string> dumpPhases;
extern maplecl::Option<std::string> skipPhases;
extern maplecl::Option<std::string> dumpFunc;
extern maplecl::Option<bool> quiet;
extern maplecl::Option<bool> nodot;
extern maplecl::Option<bool> userc;
extern maplecl::Option<bool> strictNaiverc;
extern maplecl::Option<std::string> skipFrom;
extern maplecl::Option<std::string> skipAfter;
extern maplecl::Option<bool> calleeHasSideEffect;
extern maplecl::Option<bool> ubaa;
extern maplecl::Option<bool> tbaa;
extern maplecl::Option<bool> ddaa;
extern maplecl::Option<uint8_t> aliasAnalysisLevel;
extern maplecl::Option<bool> stmtnum;
extern maplecl::Option<bool> rclower;
extern maplecl::Option<bool> gconlyopt;
extern maplecl::Option<bool> usegcbar;
extern maplecl::Option<bool> regnativefunc;
extern maplecl::Option<bool> warnemptynative;
extern maplecl::Option<bool> dumpBefore;
extern maplecl::Option<bool> dumpAfter;
extern maplecl::Option<bool> realcheckcast;
extern maplecl::Option<uint32_t> eprelimit;
extern maplecl::Option<uint32_t> eprepulimit;
extern maplecl::Option<uint32_t> epreuseprofilelimit;
extern maplecl::Option<uint32_t> stmtprepulimit;
extern maplecl::Option<uint32_t> lprelimit;
extern maplecl::Option<uint32_t> lprepulimit;
extern maplecl::Option<uint32_t> pregrenamelimit;
extern maplecl::Option<uint32_t> rename2preglimit;
extern maplecl::Option<uint32_t> proplimit;
extern maplecl::Option<uint32_t> copyproplimit;
extern maplecl::Option<uint32_t> delrcpulimit;
extern maplecl::Option<uint32_t> profileBbHotRate;
extern maplecl::Option<uint32_t> profileBbColdRate;
extern maplecl::Option<bool> ignoreipa;
extern maplecl::Option<bool> enableHotColdSplit;
extern maplecl::Option<bool> aggressiveABCO;
extern maplecl::Option<bool> commonABCO;
extern maplecl::Option<bool> conservativeABCO;
extern maplecl::Option<bool> epreincluderef;
extern maplecl::Option<bool> eprelocalrefvar;
extern maplecl::Option<bool> eprelhsivar;
extern maplecl::Option<bool> dsekeepref;
extern maplecl::Option<bool> propbase;
extern maplecl::Option<bool> propiloadref;
extern maplecl::Option<bool> propglobalref;
extern maplecl::Option<bool> propfinaliloadref;
extern maplecl::Option<bool> propiloadrefnonparm;
extern maplecl::Option<bool> lessthrowalias;
extern maplecl::Option<bool> nodelegaterc;
extern maplecl::Option<bool> nocondbasedrc;
extern maplecl::Option<bool> subsumrc;
extern maplecl::Option<bool> performFSAA;
extern maplecl::Option<bool> strengthreduction;
extern maplecl::Option<bool> sradd;
extern maplecl::Option<bool> lftr;
extern maplecl::Option<bool> ivopts;
extern maplecl::Option<bool> gvn;
extern maplecl::Option<bool> checkcastopt;
extern maplecl::Option<bool> parmtoptr;
extern maplecl::Option<bool> nullcheckpre;
extern maplecl::Option<bool> clinitpre;
extern maplecl::Option<bool> dassignpre;
extern maplecl::Option<bool> assign2finalpre;
extern maplecl::Option<bool> regreadatreturn;
extern maplecl::Option<bool> propatphi;
extern maplecl::Option<bool> propduringbuild;
extern maplecl::Option<bool> propwithinverse;
extern maplecl::Option<bool> nativeopt;
extern maplecl::Option<bool> optdirectcall;
extern maplecl::Option<bool> enableEa;
extern maplecl::Option<bool> lprespeculate;
extern maplecl::Option<bool> lpre4address;
extern maplecl::Option<bool> lpre4largeint;
extern maplecl::Option<bool> spillatcatch;
extern maplecl::Option<bool> placementrc;
extern maplecl::Option<bool> lazydecouple;
extern maplecl::Option<bool> mergestmts;
extern maplecl::Option<bool> generalRegOnly;
extern maplecl::Option<std::string> inlinefunclist;
extern maplecl::Option<uint32_t> threads;
extern maplecl::Option<bool> ignoreInferredRetType;
extern maplecl::Option<bool> meverify;
extern maplecl::Option<uint32_t> dserunslimit;
extern maplecl::Option<uint32_t> hdserunslimit;
extern maplecl::Option<uint32_t> hproprunslimit;
extern maplecl::Option<uint32_t> sinklimit;
extern maplecl::Option<uint32_t> sinkPUlimit;
extern maplecl::Option<bool> loopvec;
extern maplecl::Option<bool> seqvec;
extern maplecl::Option<bool> layoutwithpredict;
extern maplecl::Option<uint32_t> veclooplimit;
extern maplecl::Option<uint32_t> ivoptslimit;
extern maplecl::Option<std::string> acquireFunc;
extern maplecl::Option<std::string> releaseFunc;
extern maplecl::Option<bool> toolonly;
extern maplecl::Option<bool> toolstrict;
extern maplecl::Option<bool> skipvirtual;
extern maplecl::Option<uint32_t> warning;
extern maplecl::Option<uint8_t> remat;
extern maplecl::Option<bool> unifyrets;
extern maplecl::Option<bool> lfo;
extern maplecl::Option<bool> dumpCfgOfPhases;
extern maplecl::Option<bool> epreUseProfile;
#ifdef ENABLE_MAPLE_SAN
extern maplecl::Option<uint32_t> asanFlags;
#endif
}

#endif /* MAPLE_ME_INCLUDE_ME_OPTIONS_H */
