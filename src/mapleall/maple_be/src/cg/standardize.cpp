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

#include "isel.h"
#include "standardize.h"
namespace maplebe {
void Standardize::DoStandardize() {
  if (NeedTwoAddressMapping()) {
    TwoAddressMapping();
  }
  FOR_ALL_BB(bb, cgFunc) {
    FOR_BB_INSNS(insn, bb) {
      MOperator mOp = insn->GetMachineOpcode();
      switch (mOp) {
        case isel::kMOP_copyri:
          STDZcopyri(*insn);
          break;
        case isel::kMOP_str:
          STDZstr(*insn);
          break;
        default:
          break;
      }
    }
  }
}
}