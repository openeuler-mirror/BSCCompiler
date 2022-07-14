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
#ifndef MAPLEBE_INCLUDE_CG_X64_X64_ISA_TBL_H
#define MAPLEBE_INCLUDE_CG_X64_X64_ISA_TBL_H

#include "x64_isa.h"
#include "operand.h"

namespace maplebe {

namespace x64 {
 /* register,                  imm ,                    memory,                   cond */
#define DEF_X64_MOV_MAPPING_INT(SIZE)                                                                  \
X64MOP_t movIselMap##SIZE[Operand::OperandType::kOpdPhi][Operand::OperandType::kOpdPhi] = {            \
    {MOP_mov##SIZE##_r_r,      MOP_begin,              MOP_mov##SIZE##_r_m,      MOP_begin},           \
    {MOP_mov##SIZE##_i_r,      MOP_begin,              MOP_mov##SIZE##_i_m,      MOP_begin},           \
    {MOP_mov##SIZE##_m_r,      MOP_begin,              MOP_begin,                MOP_begin},           \
    {MOP_begin,                MOP_begin,              MOP_begin,                MOP_begin},           \
};

 /* register,                  imm ,              memory,                   cond */
#define DEF_X64_CMP_MAPPING_INT(SIZE)                                                                  \
X64MOP_t cmpIselMap##SIZE[Operand::OperandType::kOpdPhi][Operand::OperandType::kOpdPhi] = {            \
    {MOP_cmp##SIZE##_r_r,      MOP_cmp##SIZE##_r_i,         MOP_cmp##SIZE##_r_m,      MOP_begin},      \
    {MOP_begin,                MOP_begin,                   MOP_begin,                MOP_begin},      \
    {MOP_cmp##SIZE##_m_r,      MOP_cmp##SIZE##_m_i,         MOP_begin,                MOP_begin},      \
    {MOP_begin,                MOP_begin,                   MOP_begin,                MOP_begin},      \
};

DEF_X64_MOV_MAPPING_INT(b)
DEF_X64_MOV_MAPPING_INT(l)
DEF_X64_MOV_MAPPING_INT(q)
DEF_X64_CMP_MAPPING_INT(b)
DEF_X64_CMP_MAPPING_INT(l)
DEF_X64_CMP_MAPPING_INT(q)

static inline X64MOP_t GetMovMop(Operand::OperandType dTy, PrimType dType, Operand::OperandType sTy, PrimType sType) {
  X64MOP_t movOp = MOP_begin;
  switch (GetPrimTypeSize(dType)) {
    case k1ByteSize:
      movOp = movIselMapb[sTy][dTy];
      break;
    case k4ByteSize:
      movOp = movIselMapl[sTy][dTy];
      break;
    case k8ByteSize:
      movOp = movIselMapq[sTy][dTy];
      break;
    default:
      movOp= MOP_begin;
  }
  return movOp;
}

static inline X64MOP_t GetCmpMop(Operand::OperandType dTy, PrimType dType,
    Operand::OperandType sTy, PrimType sType) {
  X64MOP_t cmpOp = MOP_begin;
  switch (GetPrimTypeSize(dType)) {
    case k1ByteSize:
      cmpOp = cmpIselMapb[sTy][dTy];
      break;
    case k4ByteSize:
      cmpOp = cmpIselMapl[sTy][dTy];
      break;
    case k8ByteSize:
      cmpOp = cmpIselMapq[sTy][dTy];
      break;
    default:
      cmpOp= MOP_begin;
      break;
  }
  return cmpOp;
}

 /* {OPCODE, {register,             imm ,              memory,                   cond}} */
#define DEF_X64_SET_MAPPING_INT(OPCODE, TYPE)                                                   \
{OPCODE, {x64::MOP_##TYPE##_r,      x64::MOP_begin,    x64::MOP_##TYPE##_m,      x64::MOP_begin}}

using SetIselMappingType = std::unordered_map<maple::Opcode, std::array<X64MOP_t, Operand::OperandType::kOpdPhi>>;
SetIselMappingType setUnsignedIselMapping = {
    DEF_X64_SET_MAPPING_INT(OP_le, setbe),
    DEF_X64_SET_MAPPING_INT(OP_ge, setae),
    DEF_X64_SET_MAPPING_INT(OP_gt, seta),
    DEF_X64_SET_MAPPING_INT(OP_lt, setb),
    DEF_X64_SET_MAPPING_INT(OP_ne, setne),
    DEF_X64_SET_MAPPING_INT(OP_eq, sete),
};
SetIselMappingType setSignedIselMapping = {
    DEF_X64_SET_MAPPING_INT(OP_le, setle),
    DEF_X64_SET_MAPPING_INT(OP_ge, setge),
    DEF_X64_SET_MAPPING_INT(OP_gt, setg),
    DEF_X64_SET_MAPPING_INT(OP_lt, setl),
    DEF_X64_SET_MAPPING_INT(OP_ne, setne),
    DEF_X64_SET_MAPPING_INT(OP_eq, sete),
};
#undef DEF_X64_SET_MAPPING_INT

static inline X64MOP_t GetSetCCMop(maple::Opcode opcode, Operand::OperandType dTy, bool isSigned) {
  ASSERT(dTy < Operand::OperandType::kOpdPhi, "illegal operand type");
  SetIselMappingType& setIselMapping = isSigned ? setSignedIselMapping :
                                                  setUnsignedIselMapping;
  auto iter = setIselMapping.find(opcode);
  if (iter == setIselMapping.end()) {
    return x64::MOP_begin;
  }
  return iter->second[dTy];
}

}

}  /* namespace maplebe */

#endif  /* MAPLEBE_INCLUDE_CG_X64_X64_ISA_TBL_H */
