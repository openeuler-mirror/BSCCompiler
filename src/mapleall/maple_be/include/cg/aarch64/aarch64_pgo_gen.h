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
#ifndef MAPLEBE_CG_INCLUDE_AARCH64_PGO_GEN_H
#define MAPLEBE_CG_INCLUDE_AARCH64_PGO_GEN_H

#include "cg_pgo_gen.h"
namespace maplebe {
class AArch64ProfGen : public CGProfGen {
 public:
  AArch64ProfGen(CGFunc &curF, MemPool &mp) : CGProfGen(curF, mp) {}
  virtual ~AArch64ProfGen() = default;

  void InstrumentBB(BB &bb, MIRSymbol &countTab, uint32 offset) override;
  void CreateCallForDump(BB &bb, const MIRSymbol &dumpCall) override;
  void CreateCallForAbort(BB &bb) override;
};
}
#endif // MAPLEBE_CG_INCLUDE_AARCH64_PGO_GEN_H