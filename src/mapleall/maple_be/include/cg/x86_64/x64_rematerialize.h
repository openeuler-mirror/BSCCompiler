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
#ifndef MAPLEBE_INCLUDE_CG_X64_X64_REMATERIALIZE_H
#define MAPLEBE_INCLUDE_CG_X64_X64_REMATERIALIZE_H

#include "rematerialize.h"

namespace maplebe {
class X64Rematerializer : public Rematerializer {
 public:
  X64Rematerializer() = default;
  virtual ~X64Rematerializer() = default;
 private:
  bool IsRematerializableForConstval(int64 val, uint32 bitLen) const override {
    return false;
  }
  bool IsRematerializableForDread(int32 offset) const override {
    return false;
  }

  std::vector<Insn*> RematerializeForConstval(CGFunc &cgFunc, RegOperand &regOp,
      const LiveRange &lr) override;

  std::vector<Insn*> RematerializeForAddrof(CGFunc &cgFunc, RegOperand &regOp,
      int32 offset) override;

  std::vector<Insn*> RematerializeForDread(CGFunc &cgFunc, RegOperand &regOp,
      int32 offset, PrimType type) override;
};
}  /* namespace maplebe */

#endif  /* MAPLEBE_INCLUDE_CG_X64_X64_REMATERIALIZE_H */
