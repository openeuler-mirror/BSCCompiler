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

#ifndef MAPLEBE_INCLUDE_STANDARDIZE_H
#define MAPLEBE_INCLUDE_STANDARDIZE_H

#include "cgfunc.h"
namespace maplebe {
class Standardize {
 public:
  explicit Standardize(CGFunc &f) : cgFunc(&f) {}

  virtual ~Standardize() = default;

  /*
   * for cpu instruction contains different operands
   * maple provide a default implement from three address to two address
   * convertion rule is:
   * mop(dest, src1, src2) -> mov(dest, src1)
   *                          mop(dest, src2)
   */
  void TwoAddressMapping(Insn &insn);

  void DoStandardize();

 protected:
  void SetTwoAddressMapping(bool needMapping) {
    needTwoAddrMapping = needMapping;
  }
  bool NeedTwoAddressMapping(Insn &insn) {
    /* Operand number for two addressing mode is 2 */
    /* and 3 for three addressing mode */
    needTwoAddrMapping = insn.GetOperandSize() > 2;
    return needTwoAddrMapping;
  }
 private:
  /*
   * For cpu's isa which approximates to maple backend machine IR.
   * For example in ARM.
   * Two instruction which have same operands and properties are considered to be same.
   * It is able to do one instruction mapping only. [SISO]
   * Usage guide:
   * 1. The order of target_md.def and abstarct_mmir.def need to be same
   * 2. write target Instruction Description comparsion rule For InsnDescription.IsSame().
   * Otherwise it will return false by default.
   */
  virtual bool TryFastTargetIRMapping(Insn &insn) = 0;

  virtual void StdzMov(Insn &insn) = 0;
  virtual void StdzStrLdr(Insn &insn) = 0;
  virtual void StdzBasicOp(Insn &insn) = 0;
  CGFunc *cgFunc;
  bool needTwoAddrMapping = false;
};
}
#endif  /* MAPLEBE_INCLUDE_STANDARDIZE_H */