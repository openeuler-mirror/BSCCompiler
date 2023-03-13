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
#ifndef GENERIC_ATTRS_H
#define GENERIC_ATTRS_H
#include <bitset>
#include <variant>
#include "mir_type.h"

namespace maple {
using AttrContent = std::variant<int, GStrIdx>;
// only for internal use, not emitted
enum GenericAttrKind {
#define FUNC_ATTR
#define TYPE_ATTR
#define FIELD_ATTR
#define ATTR(STR) GENATTR_##STR,
#include "all_attributes.def"
#undef ATTR
#undef FUNC_ATTR
#undef TYPE_ATTR
#undef FIELD_ATTR
};
constexpr uint32 kMaxATTRNum = 128;

class GenericAttrs {
 public:
  GenericAttrs() = default;
  GenericAttrs(const GenericAttrs &ta) = default;
  GenericAttrs &operator=(const GenericAttrs &p) = default;
  ~GenericAttrs() = default;

  void SetAttr(GenericAttrKind x) {
    attrFlag.set(x);
  }

  void ResetAttr(GenericAttrKind x) {
    (void)attrFlag.reset(x);
  }

  bool GetAttr(GenericAttrKind x) const {
    return attrFlag[x];
  }

  bool operator==(const GenericAttrs &tA) const {
    return attrFlag == tA.attrFlag;
  }

  bool operator!=(const GenericAttrs &tA) const {
    return !(*this == tA);
  }

  void InitContentMap() {
    contentMap.resize(kMaxATTRNum);
    isInit = true;
  }

  bool GetContentFlag(GenericAttrKind key) const {
    return contentFlag[key];
  }

  void InsertIntContentMap(GenericAttrKind key, int val) {
    if (!isInit) {
      InitContentMap();
    }
    if (!contentFlag[key]) {
      contentMap[key] = val;
      contentFlag.set(key);
    }
  }

  void InsertStrIdxContentMap(GenericAttrKind key, GStrIdx nameIdx) {
    if (!isInit) {
      InitContentMap();
    }
    if (!contentFlag[key]) {
      contentMap[key] = nameIdx;
      contentFlag.set(key);
    }
  }

  void ClearContentMap() {
    contentMap.clear();
    contentMap.shrink_to_fit();
  }

  FieldAttrs ConvertToFieldAttrs();
  TypeAttrs ConvertToTypeAttrs() const;
  FuncAttrs ConvertToFuncAttrs();

 private:
  std::bitset<kMaxATTRNum> attrFlag = 0;
  std::bitset<kMaxATTRNum> contentFlag = 0;
  std::vector<AttrContent> contentMap;
  bool isInit = false;
};
}
#endif // GENERIC_ATTRS_H