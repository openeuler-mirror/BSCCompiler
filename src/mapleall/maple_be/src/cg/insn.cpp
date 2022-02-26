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

#include "insn.h"
#include "isel.h"
namespace maplebe {

std::string TempTransForm(MOperator mOp) {
  std::string s = "";
  switch (mOp) {
    case isel::kMOP_undef:
      s = "undef";
      break;
    case isel::kMOP_copyrr:
      s = "copyrr";
      break;
    case isel::kMOP_copyri:
      s = "copyri";
      break;
    case isel::kMOP_str:
      s = "str";
      break;
    case isel::kMOP_load:
      s = "load";
      break;
    case isel::kMOP_addrrr:
      s = "addrrr";
      break;
    default:
      break;
  }
  return s;
}
void CGInsn::Dump() const {
  LogInfo::MapleLogger() << "MOP (" << TempTransForm(mOp) << ")";
  for (auto opnd : opnds) {
    LogInfo::MapleLogger() << " (opnd:";
    opnd->Dump();
    LogInfo::MapleLogger() << ")";
  }
  LogInfo::MapleLogger() << "\n";
}
}
