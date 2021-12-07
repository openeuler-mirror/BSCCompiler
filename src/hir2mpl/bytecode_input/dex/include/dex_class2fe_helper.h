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

#ifndef HIR2MPL_BC_INPUT_INCLUDE_DEX_CLASS2FE_HELPER_H
#define HIR2MPL_BC_INPUT_INCLUDE_DEX_CLASS2FE_HELPER_H
#include "bc_class2fe_helper.h"
namespace maple {
namespace bc {
class DexClass2FEHelper : public BCClass2FEHelper {
 public:
  DexClass2FEHelper(MapleAllocator &allocator, bc::BCClass &klassIn);
  ~DexClass2FEHelper() = default;

 protected:
  void InitFieldHelpersImpl() override;
  void InitMethodHelpersImpl() override;
  TypeAttrs GetStructAttributeFromInputImpl() const override;
};

class DexClassField2FEHelper : public BCClassField2FEHelper {
 public:
  DexClassField2FEHelper(MapleAllocator &allocator, const BCClassField &fieldIn)
      : BCClassField2FEHelper(allocator, fieldIn) {}
  ~DexClassField2FEHelper() = default;

 protected:
  FieldAttrs AccessFlag2AttributeImpl(uint32 accessFlag) const override;
};

class DexClassMethod2FEHelper : public BCClassMethod2FEHelper {
 public:
  DexClassMethod2FEHelper(MapleAllocator &allocator, std::unique_ptr<BCClassMethod> &methodIn)
      : BCClassMethod2FEHelper(allocator, methodIn) {}
  ~DexClassMethod2FEHelper() = default;

 protected:
  FuncAttrs GetAttrsImpl() const override;
  bool IsStaticImpl() const override;

  bool IsClinit() const override;
  bool IsInit() const override;
};
}
}
#endif  // HIR2MPL_BC_INPUT_INCLUDE_DEX_CLASS2FE_HELPER_H