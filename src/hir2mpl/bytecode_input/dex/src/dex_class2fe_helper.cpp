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
#include "dex_class2fe_helper.h"
#include "dex_file_util.h"
#include "fe_options.h"
#include "mpl_logging.h"
#include "fe_manager.h"
namespace maple {
namespace bc {
// ========== DexClass2FEHelper ==========
DexClass2FEHelper::DexClass2FEHelper(MapleAllocator &allocator, bc::BCClass &klassIn)
    : BCClass2FEHelper(allocator, klassIn) {}

TypeAttrs DexClass2FEHelper::GetStructAttributeFromInputImpl() const {
  TypeAttrs attrs;
  uint32 klassAccessFlag = klass.GetAccessFlag();
  if (klassAccessFlag & DexFileUtil::kDexAccPublic) {
    attrs.SetAttr(ATTR_public);
  }
  if (klassAccessFlag & DexFileUtil::kDexAccFinal) {
    attrs.SetAttr(ATTR_final);
  }
  if (klassAccessFlag & DexFileUtil::kDexAccAbstract) {
    attrs.SetAttr(ATTR_abstract);
  }
  if (klassAccessFlag & DexFileUtil::kDexAccSynthetic) {
    attrs.SetAttr(ATTR_synthetic);
  }
  if (klassAccessFlag & DexFileUtil::kDexAccAnnotation) {
    attrs.SetAttr(ATTR_annotation);
  }
  if (klassAccessFlag & DexFileUtil::kDexAccEnum) {
    attrs.SetAttr(ATTR_enum);
  }
  return attrs;
}

void DexClass2FEHelper::InitFieldHelpersImpl() {
  MemPool *mp = allocator.GetMemPool();
  ASSERT(mp != nullptr, "mem pool is nullptr");
  for (const std::unique_ptr<BCClassField> &field : klass.GetFields()) {
    ASSERT(field != nullptr, "field is nullptr");
    BCClassField2FEHelper *fieldHelper = mp->New<DexClassField2FEHelper>(allocator, *field);
    fieldHelpers.push_back(fieldHelper);
  }
}

void DexClass2FEHelper::InitMethodHelpersImpl() {
  MemPool *mp = allocator.GetMemPool();
  ASSERT(mp != nullptr, "mem pool is nullptr");
  for (std::unique_ptr<BCClassMethod> &method : klass.GetMethods()) {
    ASSERT(method != nullptr, "method is nullptr");
    BCClassMethod2FEHelper *methodHelper = mp->New<DexClassMethod2FEHelper>(allocator, method);
    methodHelpers.push_back(methodHelper);
  }
}

// ========== DexClassField2FEHelper ==========
FieldAttrs DexClassField2FEHelper::AccessFlag2AttributeImpl(uint32 accessFlag) const {
  FieldAttrs attrs;
  if (accessFlag & DexFileUtil::kDexAccPublic) {
    attrs.SetAttr(FLDATTR_public);;
  }
  if (accessFlag & DexFileUtil::kDexAccPrivate) {
    attrs.SetAttr(FLDATTR_private);
  }
  if (accessFlag & DexFileUtil::kDexAccProtected) {
    attrs.SetAttr(FLDATTR_protected);
  }
  if (accessFlag & DexFileUtil::kDexAccStatic) {
    attrs.SetAttr(FLDATTR_static);
  }
  if (accessFlag & DexFileUtil::kDexAccFinal) {
    attrs.SetAttr(FLDATTR_final);
  }
  if (accessFlag & DexFileUtil::kDexAccVolatile) {
    attrs.SetAttr(FLDATTR_volatile);
  }
  if (accessFlag & DexFileUtil::kDexAccTransient) {
    attrs.SetAttr(FLDATTR_transient);
  }
  if (accessFlag & DexFileUtil::kDexAccSynthetic) {
    attrs.SetAttr(FLDATTR_synthetic);
  }
  if (accessFlag & DexFileUtil::kDexAccEnum) {
    attrs.SetAttr(FLDATTR_enum);
  }
  return attrs;
}

// ========== DexClassMethod2FEHelper ==========
FuncAttrs DexClassMethod2FEHelper::GetAttrsImpl() const {
  FuncAttrs attrs;
  uint32 accessFlag = method->GetAccessFlag();
  if (accessFlag & DexFileUtil::kDexAccPublic) {
    attrs.SetAttr(FUNCATTR_public);
  }
  if (accessFlag & DexFileUtil::kDexAccPrivate) {
    attrs.SetAttr(FUNCATTR_private);
  }
  if (accessFlag & DexFileUtil::kDexAccProtected) {
    attrs.SetAttr(FUNCATTR_protected);
  }
  if (accessFlag & DexFileUtil::kDexAccStatic) {
    attrs.SetAttr(FUNCATTR_static);
  }
  if (accessFlag & DexFileUtil::kDexAccFinal) {
    attrs.SetAttr(FUNCATTR_final);
  }
  if (accessFlag & DexFileUtil::kAccDexSynchronized) {
    attrs.SetAttr(FUNCATTR_synchronized);
  }
  if (accessFlag & DexFileUtil::kDexAccBridge) {
    attrs.SetAttr(FUNCATTR_bridge);
  }
  if (accessFlag & DexFileUtil::kDexAccVarargs) {
    attrs.SetAttr(FUNCATTR_varargs);
  }
  if (accessFlag & DexFileUtil::kDexAccNative) {
    attrs.SetAttr(FUNCATTR_native);
  }
  if (accessFlag & DexFileUtil::kDexAccAbstract) {
    attrs.SetAttr(FUNCATTR_abstract);
  }
  if (accessFlag & DexFileUtil::kDexAccStrict) {
    attrs.SetAttr(FUNCATTR_strict);
  }
  if (accessFlag & DexFileUtil::kDexAccSynthetic) {
    attrs.SetAttr(FUNCATTR_synthetic);
  }
  if (accessFlag & DexFileUtil::kDexAccConstructor) {
    attrs.SetAttr(FUNCATTR_constructor);
  }
  if (accessFlag & DexFileUtil::kDexDeclaredSynchronized) {
    attrs.SetAttr(FUNCATTR_declared_synchronized);
  }
  if (IsVirtual()) {
    attrs.SetAttr(FUNCATTR_virtual);
  }
  return attrs;
}

bool DexClassMethod2FEHelper::IsStaticImpl() const {
  if (IsClinit()) {
    return true;
  }
  uint32 accessFlag = method->GetAccessFlag();
  if ((accessFlag & DexFileUtil::kDexAccStatic) != 0) {
    return true;
  }
  return false;
}

bool DexClassMethod2FEHelper::IsClinit() const {
  return method->IsClinit();
}

bool DexClassMethod2FEHelper::IsInit() const {
  return method->IsInit();
}
}  // namespace bc
}  // namespace maple