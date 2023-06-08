/*
 * Copyright (c) [2021] Huawei Technologies Co.,Ltd.All rights reserved.
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
#include "openProfile.h"
#include "bin_mpl_export.h"
#include "bin_mplt.h"
#include "update_mplt.h"


// namespace maple
namespace maple {
void M2MOpenprofile::GetAnalysisDependence(maple::AnalysisDep &aDep) const {
  aDep.SetPreservedAll();
}

bool M2MOpenprofile::PhaseRun(maple::MIRModule &m) {
  uint32 dexNameIdx = m.GetFileinfo(GlobalTables::GetStrTable().GetOrCreateStrIdxFromName("INFO_filename"));
  const std::string &dexName = GlobalTables::GetStrTable().GetStringFromStrIdx(GStrIdx(dexNameIdx));
  bool deCompressSucc = m.GetProfile().DeCompress(Options::profile, dexName);
  if (!deCompressSucc) {
    LogInfo::MapleLogger() << "WARN: DeCompress() failed in DoOpenProfile::Run()\n";
  }
  return false;
}

}