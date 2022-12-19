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

#ifndef MAPLEBE_INCLUDE_X64_STANDARDIZE_H
#define MAPLEBE_INCLUDE_X64_STANDARDIZE_H

#include "standardize.h"

namespace maplebe {
class X64Standardize : public Standardize {
 public:
  explicit X64Standardize(CGFunc &f) : Standardize(f) {
    SetAddressMapping(true);
  }

  ~X64Standardize() override = default;

 private:
  void StdzMov(Insn &insn) override;
  void StdzStrLdr(Insn &insn) override;
  void StdzBasicOp(Insn &insn) override;
  void StdzUnaryOp(Insn &insn) override;
  void StdzCvtOp(Insn &insn) override;
  void StdzShiftOp(Insn &insn) override;
  void StdzFloatingNeg(Insn &insn);
  void StdzCommentOp(Insn &insn) override;
};
}
#endif  /* MAPLEBE_INCLUDEX_64_STANDARDIZE_H */
