/*
 * Copyright (c) [2020-2021] Huawei Technologies Co.,Ltd.All rights reserved.
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
#include "sync_select.h"
#include "safe_cast.h"
#include "mpl_logging.h"
#include "me_cfg.h"

namespace {
using namespace maple;

enum class SyncKind : uint8 {
  kSpinLock = 0,
  kDefault = 2,
  kSysCall = 3
};

class SyncSelect {
 public:
  explicit SyncSelect(MeFunction &func) : func(func) {
    AddFunc(SyncKind::kDefault, "Landroid_2Fos_2FParcel_3B_7Crecycle_7C_28_29V");
  }
  ~SyncSelect() = default;

  void SyncOptimization() {
    // deal with func white list where all syncenter use same version
    auto it = syncMap.find(func.GetName());
    if (it != syncMap.cend()) {
      SetAllSyncKind(it->second);
    }
  }
 private:
  void AddFunc(SyncKind kind, const std::string &funcName) {
    syncMap[funcName] = kind;
  }

  void SetAllSyncKind(SyncKind kind) {
    auto cfg = func.GetCfg();
    for (auto bIt = cfg->valid_begin(), eIt = cfg->valid_end(); bIt != eIt; ++bIt) {
      for (auto &stmt : (*bIt)->GetStmtNodes()) {
        if (stmt.GetOpCode() == OP_syncenter) {
          SetSyncKind(stmt, kind);
        }
      }
    }
  }

  void SetSyncKind(StmtNode &stmt, SyncKind kind) const {
    CHECK_FATAL(stmt.GetOpCode() == OP_syncenter, "expect syncenter in SetSyncKind");
    auto &nd = static_cast<NaryStmtNode&>(stmt);
    CHECK_FATAL(nd.GetNopndSize() > 1, "access nd->nOpnd failed");
    auto *cst = static_cast<ConstvalNode*>(nd.GetNopndAt(1));
    auto *intConst = safe_cast<MIRIntConst>(cst->GetConstVal());
    utils::ToRef(intConst).SetValue(static_cast<int64>(kind));
  }

  MeFunction &func;
  std::map<const std::string, SyncKind> syncMap;
};
}

namespace maple {
bool MESyncSelect::PhaseRun(MeFunction &f) {
  SyncSelect(f).SyncOptimization();
  if (DEBUGFUNC_NEWPM(f)) {
    f.Dump(true);
  }
  return true;
}

void MESyncSelect::GetAnalysisDependence(maple::AnalysisDep &aDep) const {
  aDep.SetPreservedAll();
}
}  // namespace maple
