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
#ifndef MPLFE_BC_INPUT_INCLUDE_BC_COMPILER_COMPONENT_H
#define MPLFE_BC_INPUT_INCLUDE_BC_COMPILER_COMPONENT_H
#include "fe_macros.h"
#include "mplfe_compiler_component.h"
#include "bc_input.h"

namespace maple {
namespace bc {
template <class T>
class BCCompilerComponent : public MPLFECompilerComponent {
 public:
  explicit BCCompilerComponent(MIRModule &module);
  ~BCCompilerComponent();

 protected:
  bool ParseInputImpl() override;
  bool LoadOnDemandTypeImpl() override;
  bool PreProcessDeclImpl() override;
  bool ProcessDeclImpl() override;
  void ProcessPragmaImpl() override;
  bool PreProcessWithoutFunctionImpl() override;
  bool PreProcessWithFunctionImpl() override;
  std::string GetComponentNameImpl() const override;
  bool ParallelableImpl() const override;
  void DumpPhaseTimeTotalImpl() const override;
  void ReleaseMemPoolImpl() override;

 private:
  bool LoadOnDemandType2BCClass(const std::unordered_set<std::string> &allDepsSet,
                                const std::unordered_set<std::string> &allDefsSet,
                                std::list<std::unique_ptr<bc::BCClass>> &klassList);
  bool LoadOnDemandBCClass2FEClass(const std::list<std::unique_ptr<bc::BCClass>> &klassList,
                                   std::list<std::unique_ptr<FEInputStructHelper>> &structHelpers,
                                   bool isEmitDepsMplt);

  MemPool *mp;
  MapleAllocator allocator;
  std::unique_ptr<bc::BCInput<T>> bcInput;
};  // class BCCompilerComponent
}  // namespace bc
}  // namespace maple
#include "bc_compiler_component-inl.h"
#endif  // MPLFE_BC_INPUT_INCLUDE_BC_COMPILER_COMPONENT_H