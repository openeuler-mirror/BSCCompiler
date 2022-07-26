/*
 * Copyright (c) [2019-2021] Huawei Technologies Co.,Ltd.All rights reserved.
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
#include "ver_symbol.h"
#include "bb.h"
#include "me_ssa.h"
#include "ssa_mir_nodes.h"

namespace maple {
void VersionSt::DumpDefStmt() const {
  if (version <= 0) {
    return;
  }
  switch (defType) {
    case kAssign:
      defStmt.assign->Dump(0);
      return;
    case kPhi:
      defStmt.phi->Dump();
      return;
    case kMayDef:
      defStmt.mayDef->Dump();
      return;
    case kMustDef:
      defStmt.mustDef->Dump();
      return;
    default:
      ASSERT(false, "not yet implement");
  }
}

VersionSt *VersionStTable::CreateNextVersionSt(OriginalSt *ost) {
  ASSERT(ost->GetVersionsIndices().size() != 0, "CreateNextVersionSt: zero version must have been created first");
  VersionSt *vst = vstAlloc.GetMemPool()->New<VersionSt>(versionStVector.size(), ost->GetVersionsIndices().size(), ost);
  versionStVector.push_back(vst);
  ost->PushbackVersionsIndices(vst->GetIndex());
  vst->SetOst(ost);
  return vst;
}

void VersionStTable::CreateZeroVersionSt(OriginalSt *ost) {
  ASSERT_NOT_NULL(ost);
  if (ost->GetZeroVersionIndex() != 0) {
    return;  // already created
  }
  ASSERT(ost->GetVersionsIndices().size() == 0, "ssa version need to be created incrementally!");
  VersionSt *vst = vstAlloc.GetMemPool()->New<VersionSt>(versionStVector.size(), kInitVersion, ost);
  versionStVector.push_back(vst);
  ost->PushbackVersionsIndices(vst->GetIndex());
  ost->SetZeroVersionIndex(vst->GetIndex());
  vst->SetOst(ost);
  return;
}

VersionSt *VersionStTable::GetOrCreateZeroVersionSt(OriginalSt &ost) {
  auto zeroVersionIndex = ost.GetZeroVersionIndex();
  if (zeroVersionIndex == kInvalidVstIdx) {
    CreateZeroVersionSt(&ost);
  }
  auto *zeroVersionVst = GetVersionStVectorItem(ost.GetZeroVersionIndex());
  ASSERT(zeroVersionVst != nullptr, "failed in creating zero-version-vst of ost");
  return zeroVersionVst;
}

void VersionStTable::Dump(const MIRModule *mod) const {
  ASSERT(mod != nullptr, "nullptr check");
  LogInfo::MapleLogger() << "=======version st table entries=======\n";
  for (size_t i = 1; i < versionStVector.size(); ++i) {
    const VersionSt *vst = versionStVector[i];
    if (vst == nullptr) {
      continue;
    }
    vst->Dump();
    if (vst->GetVersion() > 0) {
      LogInfo::MapleLogger() << " defined BB" << vst->GetDefBB()->GetBBId() << ": ";
      vst->DumpDefStmt();
    } else {
      LogInfo::MapleLogger() << '\n';
    }
  }
  mod->GetOut() << "=======end version st table===========\n";
}
}  // namespace maple
