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
#include "x64_isa.h"
namespace maplebe {
using namespace x64;
#define DEFINE_MOP(...) {__VA_ARGS__},
const InsnDesc X64CG::kMd[kMopLast] = {
#include "abstract_mmir.def"
#include "x64_md.def"
};
#undef DEFINE_MOP

std::array<std::array<const std::string, kAllRegNum>, kIntRegTypeNum> X64CG::intRegNames = {
    std::array<const std::string, kAllRegNum> {
        "err", "al", "bl", "cl", "dl", "spl", "bpl", "sil", "dil", "r8b", "r9b", "r10b", "r11b", "r12b", "r13b",
        "r14b", "r15b", "err1", "errMaxRegNum"
    }, std::array<const std::string, kAllRegNum> {
        "err", "ah", "bh", "ch", "dh", "err0", "err1", "err2", "err3", "err4", "err5", "err6", "err7", "err8", "err9",
        "err10", "err11", "err12", "errMaxRegNum"
    }, std::array<const std::string, kAllRegNum> {
        "err", "ax", "bx", "cx", "dx", "sp", "bp", "si", "di", "r8w", "r9w", "r10w", "r11w", "r12w", "r13w",
        "r14w", "r15w", "err1", "errMaxRegNum"
    }, std::array<const std::string, kAllRegNum> {
        "err", "eax", "ebx", "ecx", "edx", "esp", "ebp", "esi", "edi", "r8d", "r9d", "r10d", "r11d", "r12d", "r13d",
        "r14d", "r15d", "err1", "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5", "xmm6", "xmm7", "xmm8", "xmm9", "xmm10",
        "xmm11", "xmm12", "xmm13", "xmm14", "xmm15", "errMaxRegNum"
    }, std::array<const std::string, kAllRegNum> {
        "err", "rax", "rbx", "rcx", "rdx", "rsp", "rbp", "rsi", "rdi", "r8", "r9", "r10", "r11", "r12", "r13",
        "r14", "r15", "rip", "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5", "xmm6", "xmm7", "xmm8", "xmm9", "xmm10",
    "xmm11", "xmm12", "xmm13", "xmm14", "xmm15", "errMaxRegNum"
    },
};

void X64CG::EnrollTargetPhases(maple::MaplePhaseManager *pm) const {
#include "x64_phases.def"
}

CGFunc *X64CG::CreateCGFunc(MIRModule &mod, MIRFunction &mirFunc, BECommon &bec, MemPool &memPool,
                            StackMemPool &stackMp, MapleAllocator &mallocator, uint32 funcId) {
  return memPool.New<X64CGFunc>(mod, *this, mirFunc, bec, memPool, stackMp, mallocator, funcId);
}

bool X64CG::IsEffectiveCopy(Insn &insn) const {
  return false;
}
bool X64CG::IsTargetInsn(MOperator mOp) const {
  return (mOp >= MOP_movb_r_r && mOp <= MOP_pseudo_ret_int);
}
bool X64CG::IsClinitInsn(MOperator mOp) const {
  return false;
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

void X64CG::DumpTargetOperand(Operand &opnd, const OpndDesc &opndDesc) const {
  X64OpndDumpVisitor visitor(opndDesc);
  opnd.Accept(visitor);
}

bool X64CG::IsExclusiveFunc(MIRFunction &mirFunc) {
  return false;
}

/* NOTE: Consider making be_common a field of CG. */
void X64CG::GenerateObjectMaps(BECommon &beCommon) {}

/* Used for GCTIB pattern merging */
std::string X64CG::FindGCTIBPatternName(const std::string &name) const {
  return "";
}
}
