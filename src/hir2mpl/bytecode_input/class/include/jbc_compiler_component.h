/*
 * Copyright (c) [2020-2021] Huawei Technologies Co.,Ltd.All rights reserved.
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
#ifndef HIR2MPL_INCLUDE_JBC_COMPILER_COMPONENT_H
#define HIR2MPL_INCLUDE_JBC_COMPILER_COMPONENT_H
#include "fe_macros.h"
#include "hir2mpl_compiler_component.h"
#include "jbc_input.h"
#include "fe_function_phase_result.h"

namespace maple {
class JBCCompilerComponent : public HIR2MPLCompilerComponent {
 public:
  explicit JBCCompilerComponent(MIRModule &module);
  ~JBCCompilerComponent() override;

 protected:
  bool ParseInputImpl() override;
  bool LoadOnDemandTypeImpl() override;
  void ProcessPragmaImpl() override;
  std::unique_ptr<FEFunction> CreatFEFunctionImpl(FEInputMethodHelper *methodHelper) override;
  std::string GetComponentNameImpl() const override;
  bool ParallelableImpl() const override;
  void DumpPhaseTimeTotalImpl() const override;
  void ReleaseMemPoolImpl() override;

 private:
  MemPool *mp;
  MapleAllocator allocator;
  jbc::JBCInput jbcInput;
};  // class JBCCompilerComponent
}  // namespace maple
#endif  // HIR2MPL_INCLUDE_JBC_COMPILER_COMPONENT_H
