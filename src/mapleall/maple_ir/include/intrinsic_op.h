/*
 * Copyright (c) [2019] Huawei Technologies Co.,Ltd.All rights reserved.
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
#ifndef MAPLE_IR_INCLUDE_INTRINSIC_OP_H
#define MAPLE_IR_INCLUDE_INTRINSIC_OP_H

namespace maple {
#define CASE_INTRN_C_SYNC \
  case INTRN_C___sync_add_and_fetch_1: \
  case INTRN_C___sync_add_and_fetch_2: \
  case INTRN_C___sync_add_and_fetch_4: \
  case INTRN_C___sync_add_and_fetch_8: \
  case INTRN_C___sync_sub_and_fetch_1: \
  case INTRN_C___sync_sub_and_fetch_2: \
  case INTRN_C___sync_sub_and_fetch_4: \
  case INTRN_C___sync_sub_and_fetch_8: \
  case INTRN_C___sync_fetch_and_add_1: \
  case INTRN_C___sync_fetch_and_add_2: \
  case INTRN_C___sync_fetch_and_add_4: \
  case INTRN_C___sync_fetch_and_add_8: \
  case INTRN_C___sync_fetch_and_sub_1: \
  case INTRN_C___sync_fetch_and_sub_2: \
  case INTRN_C___sync_fetch_and_sub_4: \
  case INTRN_C___sync_fetch_and_sub_8: \
  case INTRN_C___sync_bool_compare_and_swap_1: \
  case INTRN_C___sync_bool_compare_and_swap_2: \
  case INTRN_C___sync_bool_compare_and_swap_4: \
  case INTRN_C___sync_bool_compare_and_swap_8: \
  case INTRN_C___sync_val_compare_and_swap_1: \
  case INTRN_C___sync_val_compare_and_swap_2: \
  case INTRN_C___sync_val_compare_and_swap_4: \
  case INTRN_C___sync_val_compare_and_swap_8: \
  case INTRN_C___sync_lock_test_and_set_1: \
  case INTRN_C___sync_lock_test_and_set_2: \
  case INTRN_C___sync_lock_test_and_set_4: \
  case INTRN_C___sync_lock_test_and_set_8: \
  case INTRN_C___sync_lock_release_8: \
  case INTRN_C___sync_lock_release_4: \
  case INTRN_C___sync_lock_release_2: \
  case INTRN_C___sync_lock_release_1: \
  case INTRN_C___sync_fetch_and_and_1: \
  case INTRN_C___sync_fetch_and_and_2: \
  case INTRN_C___sync_fetch_and_and_4: \
  case INTRN_C___sync_fetch_and_and_8: \
  case INTRN_C___sync_fetch_and_or_1: \
  case INTRN_C___sync_fetch_and_or_2: \
  case INTRN_C___sync_fetch_and_or_4: \
  case INTRN_C___sync_fetch_and_or_8: \
  case INTRN_C___sync_fetch_and_xor_1: \
  case INTRN_C___sync_fetch_and_xor_2: \
  case INTRN_C___sync_fetch_and_xor_4: \
  case INTRN_C___sync_fetch_and_xor_8: \
  case INTRN_C___sync_fetch_and_nand_1: \
  case INTRN_C___sync_fetch_and_nand_2: \
  case INTRN_C___sync_fetch_and_nand_4: \
  case INTRN_C___sync_fetch_and_nand_8: \
  case INTRN_C___sync_and_and_fetch_1: \
  case INTRN_C___sync_and_and_fetch_2: \
  case INTRN_C___sync_and_and_fetch_4: \
  case INTRN_C___sync_and_and_fetch_8: \
  case INTRN_C___sync_or_and_fetch_1: \
  case INTRN_C___sync_or_and_fetch_2: \
  case INTRN_C___sync_or_and_fetch_4: \
  case INTRN_C___sync_or_and_fetch_8: \
  case INTRN_C___sync_xor_and_fetch_1: \
  case INTRN_C___sync_xor_and_fetch_2: \
  case INTRN_C___sync_xor_and_fetch_4: \
  case INTRN_C___sync_xor_and_fetch_8: \
  case INTRN_C___sync_nand_and_fetch_1: \
  case INTRN_C___sync_nand_and_fetch_2: \
  case INTRN_C___sync_nand_and_fetch_4: \
  case INTRN_C___sync_nand_and_fetch_8

enum MIRIntrinsicID : uint32 {
#define DEF_MIR_INTRINSIC(STR, NAME, NUM_INSN, INTRN_CLASS, RETURN_TYPE, ...) INTRN_##STR,
#include "intrinsics.def"
#undef DEF_MIR_INTRINSIC
};
}  // namespace maple
#endif  // MAPLE_IR_INCLUDE_INTRINSIC_OP_H
