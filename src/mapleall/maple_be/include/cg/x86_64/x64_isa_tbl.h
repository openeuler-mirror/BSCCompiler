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
#include "Operand.h"

namespace maplebe {

namespace x64 {

X64MOP_t byteMovIselMap[Operand::OperandType::kOpdPhi][Operand::OperandType::kOpdPhi] = {
    /* register,         imm ,              memory,           cond */
    {MOP_movb_r_r,      MOP_undef,         MOP_movb_r_m,      MOP_undef},     /* reg    */
    {MOP_movb_i_r,      MOP_undef,         MOP_movb_i_m,      MOP_undef},     /* imm    */
    {MOP_movb_m_r,      MOP_undef,         MOP_undef,         MOP_undef},     /* memory */
    {MOP_undef,         MOP_undef,         MOP_undef,         MOP_undef},     /* cond   */
};

X64MOP_t longMovIselMap[Operand::OperandType::kOpdPhi][Operand::OperandType::kOpdPhi] = {
    /* register,         imm ,              memory,           cond */
    {MOP_movl_r_r,      MOP_undef,         MOP_movl_r_m,      MOP_undef},     /* reg    */
    {MOP_movl_i_r,      MOP_undef,         MOP_movl_i_m,      MOP_undef},     /* imm    */
    {MOP_movl_m_r,      MOP_undef,         MOP_undef,         MOP_undef},     /* memory */
    {MOP_undef,         MOP_undef,         MOP_undef,         MOP_undef},     /* cond   */
};

X64MOP_t quadMovIselMap[Operand::OperandType::kOpdPhi][Operand::OperandType::kOpdPhi] = {
    /* register,         imm ,              memory,           cond */
    {MOP_movq_r_r,      MOP_undef,         MOP_movq_r_m,      MOP_undef},     /* reg    */
    {MOP_movq_i_r,      MOP_undef,         MOP_undef,         MOP_undef},     /* imm    */
    {MOP_movq_m_r,      MOP_undef,         MOP_undef,         MOP_undef},     /* memory */
    {MOP_undef,         MOP_undef,         MOP_undef,         MOP_undef},     /* cond   */
};


static inline X64MOP_t GetMovMop(Operand::OperandType dTy, PrimType dType,
    Operand::OperandType sTy, PrimType sType) {

  X64MOP_t movOp= MOP_undef;
  switch(GetPrimTypeSize(dType)){
    case k1ByteSize:
      movOp = byteMovIselMap[sTy][dTy];
    case k4ByteSize:
      movOp = longMovIselMap[sTy][dTy];
    case k8ByteSize:
      movOp = quadMovIselMap[sTy][dTy];
    default:
      movOp= MOP_undef;
  }
  return movOp;
}

}

}  /* namespace maplebe */

#endif  /* MAPLEBE_INCLUDE_CG_X64_X64_ISA_TBL_H */
