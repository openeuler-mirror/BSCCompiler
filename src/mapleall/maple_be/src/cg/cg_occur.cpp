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
#include "cg_occur.h"
#include "cg_pre.h"

/* The methods associated with the data structures that represent occurrences and work candidates for PRE */
namespace maplebe {
/* return if this occur dominate occ */
bool CgOccur::IsDominate(DomAnalysis &dom, CgOccur &occ) {
  return dom.Dominate(*GetBB(), *occ.GetBB());
}

/* compute bucket index for the work candidate in workCandHashTable */
uint32 PreWorkCandHashTable::ComputeWorkCandHashIndex(const Operand &opnd) {
  auto hashIdx = std::hash<std::string>{}(opnd.GetHashContent());
  return hashIdx % workCandHashLength;
}

uint32 PreWorkCandHashTable::ComputeStmtWorkCandHashIndex(const Insn &insn) {
  uint32 hIdx = (static_cast<uint32>(insn.GetMachineOpcode())) << k3ByteSize;
  return hIdx % workCandHashLength;
}
}  // namespace maple
