/*
 * Copyright (c) [2020-2022] Huawei Technologies Co.,Ltd.All rights reserved.
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
#include "generic_attrs.h"
#include "global_tables.h"

namespace maple {
TypeAttrs GenericAttrs::ConvertToTypeAttrs() const {
  TypeAttrs attr;
  for (uint32 i = 0; i < kMaxATTRNum; ++i) {
    if (attrFlag[i] == 0) {
      continue;
    }
    auto tA = static_cast<GenericAttrKind>(i);
    switch (tA) {
#define TYPE_ATTR
#define ATTR(STR)               \
    case GENATTR_##STR:         \
      attr.SetAttr(ATTR_##STR); \
      break;
#include "all_attributes.def"
#undef ATTR
#undef TYPE_ATTR
      default:
        ASSERT(false, "unknown TypeAttrs");
        break;
    }
  }
  if (GetContentFlag(GENATTR_pack)) {
    attr.SetPack(static_cast<uint32>(std::get<int>(contentMap[GENATTR_pack])));
  }
  return attr;
}

FuncAttrs GenericAttrs::ConvertToFuncAttrs() {
  FuncAttrs attr;
  for (uint32 i = 0; i < kMaxATTRNum; ++i) {
    if (attrFlag[i] == 0) {
      continue;
    }
    auto tA = static_cast<GenericAttrKind>(i);
    switch (tA) {
#define FUNC_ATTR
#define ATTR(STR)                   \
    case GENATTR_##STR:             \
      attr.SetAttr(FUNCATTR_##STR); \
      break;
#include "all_attributes.def"
#undef ATTR
#undef FUNC_ATTR
      default:
        ASSERT(false, "unknown FuncAttrs");
        break;
    }
  }
  if (GetContentFlag(GENATTR_alias)) {
      std::string name = GlobalTables::GetStrTable().GetStringFromStrIdx(std::get<GStrIdx>(contentMap[GENATTR_alias]));
      attr.SetAliasFuncName(name);
    }
    if (GetContentFlag(GENATTR_constructor_priority)) {
      attr.SetConstructorPriority(std::get<int>(contentMap[GENATTR_constructor_priority]));
    }
    if (GetContentFlag(GENATTR_destructor_priority)) {
      attr.SetDestructorPriority(std::get<int>(contentMap[GENATTR_destructor_priority]));
    }
  return attr;
}

FieldAttrs GenericAttrs::ConvertToFieldAttrs() {
  FieldAttrs attr;
  constexpr uint32 maxAttrNum = 128;
  for (uint32 i = 0; i < maxAttrNum; ++i) {
    if (attrFlag[i] == 0) {
      continue;
    }
    auto tA = static_cast<GenericAttrKind>(i);
    switch (tA) {
#define FIELD_ATTR
#define ATTR(STR)                  \
    case GENATTR_##STR:            \
      attr.SetAttr(FLDATTR_##STR); \
      break;
#include "all_attributes.def"
#undef ATTR
#undef FIELD_ATTR
      default:
        ASSERT(false, "unknown FieldAttrs");
        break;
    }
  }
  return attr;
}
}
