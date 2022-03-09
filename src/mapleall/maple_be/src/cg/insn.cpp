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
#include "isa.h"
namespace maplebe {
bool Insn::IsMachineInstruction() const {
  return md ? md->IsPhysicalInsn() : false;
}
bool Insn::IsLoad() const {
  return md ? md->IsLoad() : false;
}
bool Insn::IsStore() const {
  return md ? md->IsStore() : false;
}
bool Insn::IsMove() const {
  return md ? md->IsMove() : false;
}
bool Insn::IsBasicOp() const {
  return md ? md->IsBasicOp() : false;
}

void Insn::CommuteOperands(uint32 dIndex, uint32 sIndex) {
  Operand *tempCopy = opnds[sIndex];
  opnds[sIndex] = opnds[dIndex];
  opnds[dIndex] = tempCopy;
}

#ifdef TARGX86_64
void Insn::SetMOP(const InsnDescription &idesc) {
  mOp = idesc.GetOpc();
  md = &idesc;
}
#endif

void Insn::Dump() const {

  LogInfo::MapleLogger() << "MOP (";
  if (md) {
    LogInfo::MapleLogger() << md->GetName();
  }
  LogInfo::MapleLogger() << ")";
  for (auto opnd : opnds) {
    LogInfo::MapleLogger() << " (opnd:";
    opnd->Dump();
    LogInfo::MapleLogger() << ")";
  }
  LogInfo::MapleLogger() << "\n";
}
}
