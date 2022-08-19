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
#ifndef MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_CFI_GENERATOR_H
#define MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_CFI_GENERATOR_H

#include "cfi_generator.h"

namespace maplebe {
class AArch64GenCfi : public GenCfi  {
 public:
  explicit AArch64GenCfi(CGFunc &func) : GenCfi(func) {
    useFP = func.UseFP();
    if (func.GetMirModule().GetFlavor() == MIRFlavor::kFlavorLmbc) {
      stackBaseReg = RFP;
    } else {
      stackBaseReg = useFP ? R29 : RSP;
    }
  }
  ~AArch64GenCfi() = default;

 private:
  void GenerateRegisterSaveDirective(BB &bb) override;
  void GenerateRegisterRestoreDirective(BB &bb) override;

  /* frame pointer(x29) is available as a general-purpose register if useFP is set as false */
  AArch64reg stackBaseReg = RFP;
};
} /* namespace maplebe */
#endif /* MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_CFI_GENERATOR_H */
