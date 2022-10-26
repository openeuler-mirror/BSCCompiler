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

  virtual ~Standardize() {
    cgFunc = nullptr;
  }

  /*
   * for cpu instruction contains different operands
   * maple provide a default implement from three address to two address
   * convertion rule is:
   * mop(dest, src1, src2) -> mov(dest, src1)
   *                          mop(dest, src2)
   * maple provide a default implement from two address to one address for unary op
   * convertion rule is:
   * mop(dest, src) -> mov(dest, src1)
   *                   mop(dest)
   */
  void AddressMapping(Insn &insn) const;

  void DoStandardize();

 protected:
  void SetAddressMapping(bool needMapping) {
    needAddrMapping = needMapping;
  }
  bool NeedAddressMapping(const Insn &insn) {
    /* Operand number for two addressing mode is 2 */
    /* and 3 for three addressing mode */
    needAddrMapping = (insn.GetOperandSize() > 2) || (insn.IsUnaryOp());
    return needAddrMapping;
  }
 private:
  virtual void StdzMov(Insn &insn) = 0;
  virtual void StdzStrLdr(Insn &insn) = 0;
  virtual void StdzBasicOp(Insn &insn) = 0;
  virtual void StdzUnaryOp(Insn &insn, CGFunc &cgFunc) = 0;
  virtual void StdzCvtOp(Insn &insn, CGFunc &cgFunc) = 0;
  virtual void StdzShiftOp(Insn &insn, CGFunc &cgFunc) = 0;
  CGFunc *cgFunc;
  bool needAddrMapping = false;
};
MAPLE_FUNC_PHASE_DECLARE_BEGIN(InstructionStandardize, maplebe::CGFunc)
MAPLE_FUNC_PHASE_DECLARE_END
}
#endif  /* MAPLEBE_INCLUDE_STANDARDIZE_H */
