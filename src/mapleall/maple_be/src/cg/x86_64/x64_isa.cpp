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

#include "x64_isa.h"
#include "insn.h"

namespace maplebe {
namespace x64 {
MOperator FlipConditionOp(MOperator flippedOp) {
  switch (flippedOp) {
    case X64MOP_t::MOP_je_l:
      return X64MOP_t::MOP_jne_l;
    case X64MOP_t::MOP_jne_l:
      return X64MOP_t::MOP_je_l;
    case X64MOP_t::MOP_ja_l:
      return X64MOP_t::MOP_jbe_l;
    case X64MOP_t::MOP_jbe_l:
      return X64MOP_t::MOP_ja_l;
    case X64MOP_t::MOP_jae_l:
      return X64MOP_t::MOP_jb_l;
    case X64MOP_t::MOP_jb_l:
      return X64MOP_t::MOP_jae_l;
    case X64MOP_t::MOP_jg_l:
      return X64MOP_t::MOP_jle_l;
    case X64MOP_t::MOP_jle_l:
      return X64MOP_t::MOP_jg_l;
    case X64MOP_t::MOP_jge_l:
      return X64MOP_t::MOP_jl_l;
    case X64MOP_t::MOP_jl_l:
      return X64MOP_t::MOP_jge_l;
     default:
      break;
  }
  return X64MOP_t::MOP_begin;
}

uint32 GetJumpTargetIdx(const Insn &insn) {
  CHECK_FATAL(insn.IsCondBranch() || insn.IsUnCondBranch(), "Not a jump insn");
  return kInsnFirstOpnd;
}
} /* namespace x64 */
}  /* namespace maplebe */
