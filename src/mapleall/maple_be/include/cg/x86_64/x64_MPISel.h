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

#ifndef MAPLEBE_INCLUDE_X64_MPISEL_H
#define MAPLEBE_INCLUDE_X64_MPISEL_H

#include "isel.h"
namespace maplebe {
class X64MPIsel : public MPISel {
 public:
  X64MPIsel(MemPool &mp, CGFunc &f) : MPISel(mp, f) {}
  ~X64MPIsel() override = default;

 private:
  CGMemOperand &GetSymbolFromMemory(const MIRSymbol &symbol) override;
};
}

#endif  /* MAPLEBE_INCLUDE_X64_MPISEL_H */