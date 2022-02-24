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

#include "x64_cg.h"
#include "x64_cgfunc.h"

namespace maplebe {

void X64CG::EnrollTargetPhases(maple::MaplePhaseManager *pm) const {
#include "x64_phases.def"
}

CGFunc *X64CG::CreateCGFunc(MIRModule &mod, MIRFunction &mirFunc, BECommon &bec, MemPool &memPool,
                            StackMemPool &stackMp, MapleAllocator &mallocator, uint32 funcId) {
  return memPool.New<X64CGFunc>(mod, *this, mirFunc, bec, memPool, stackMp, mallocator, funcId);
}

Insn &X64CG::BuildPhiInsn(RegOperand &defOpnd, Operand &listParam) {
  CHECK_FATAL(false, "NIY");
  Insn *a = nullptr;
  return *a;
}

PhiOperand &X64CG::CreatePhiOperand(MemPool &mp, MapleAllocator &mAllocator) {
  CHECK_FATAL(false, "NIY");
  PhiOperand *a = nullptr;
  return *a;
}

bool X64CG::IsExclusiveFunc(MIRFunction &mirFunc) {
  return false;
}

/* NOTE: Consider making be_common a field of CG. */
void X64CG::GenerateObjectMaps(BECommon &beCommon) {

}

/* Used for GCTIB pattern merging */
std::string X64CG::FindGCTIBPatternName(const std::string &name) const {
  return "";
}
}