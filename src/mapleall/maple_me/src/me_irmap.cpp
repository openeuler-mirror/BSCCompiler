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
#include "me_irmap.h"

namespace maple {
void MeIRMap::Dump() {
  // back up mempool and use a new mempool every time
  // we dump IRMap, restore the mempool afterwards
  MIRFunction *mirFunction = func.GetMirFunc();
  MemPool *backup = mirFunction->GetCodeMempool();
  mirFunction->SetMemPool(new ThreadLocalMemPool(memPoolCtrler, "IR Dump"));
  auto theCFG = func.GetCfg();
  LogInfo::MapleLogger() << "===================Me IR dump==================\n";
  auto eIt = theCFG->valid_end();
  for (auto bIt = theCFG->valid_begin(); bIt != eIt; ++bIt) {
    auto *bb = *bIt;
    bb->DumpHeader(&GetMIRModule());
    LogInfo::MapleLogger() << "frequency : " << bb->GetFrequency() << "\n";
    bb->DumpMeVarPiList(this);
    bb->DumpMePhiList(this);
    int i = 0;
    for (auto &meStmt : bb->GetMeStmts()) {
      if (GetDumpStmtNum()) {
        LogInfo::MapleLogger() << "(" << i++ << ") ";
      }
      if (meStmt.GetOp() != OP_piassign) {
        meStmt.EmitStmt(GetSSATab()).Dump(0);
      }
      meStmt.Dump(this);
    }
  }
  mirFunction->ReleaseCodeMemory();
  mirFunction->SetMemPool(backup);
}
}  // namespace maple
