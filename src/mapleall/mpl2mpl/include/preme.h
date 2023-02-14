/*
 * Copyright (c) [2021] Huawei Technologies Co.,Ltd.All rights reserved.
 *
 * OpenArkCompiler is licensed under the Mulan PSL v1.
 * You can use this software according to the terms and conditions of the Mulan PSL v1.
 * You may obtain a copy of Mulan PSL v1 at:
 *
 *     http://license.coscl.org.cn/MulanPSL
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR
 * FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v1 for more details.
 */
#ifndef MPL2MPL_INCLUDE_PREME_H
#define MPL2MPL_INCLUDE_PREME_H
#include "phase_impl.h"
#include "me_option.h"

namespace maple {
class Preme : public FuncOptimizeImpl {
 public:
  Preme(MIRModule &mod, KlassHierarchy *kh, bool dump) : FuncOptimizeImpl(mod, kh, dump) {}
  ~Preme() override = default;
  // Create global symbols and functions here when iterating mirFunc is needed
  void ProcessFunc(MIRFunction *func) override;
  FuncOptimizeImpl *Clone() override {
    return new Preme(*this);
  }
  static void CreateMIRTypeForLowerGlobalDreads();

 private:
  void CreateMIRTypeForAddrof(const MIRFunction &func, const BaseNode *baseNode) const;
};

MAPLE_MODULE_PHASE_DECLARE(M2MPreme)
}  // namespace maple
#endif  // MPL2MPL_INCLUDE_PREME_H
