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
#include "global_tables.h"

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
  virtual ~GenericAttrs() = default;

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

  virtual void ContentResize(size_t newSize) = 0;

  virtual void ContentClear() = 0;

  virtual void ContentShrinkToFit() = 0;

  virtual void ContentInsert(GenericAttrKind key, GStrIdx nameIdx) = 0;

  virtual void ContentInsert(GenericAttrKind key, int val) = 0;

  virtual const std::string &GetAttrStrName(GenericAttrKind key) const = 0;

  virtual int GetPriority(GenericAttrKind attr) const = 0;

  virtual uint32 GetPack(GenericAttrKind attr) const = 0;

  void InitContentMap() {
    ContentResize(kMaxATTRNum);
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
      ContentInsert(key, val);
      contentFlag.set(key);
    }
  }

  void InsertStrIdxContentMap(GenericAttrKind key, const GStrIdx &nameIdx) {
    if (!isInit) {
      InitContentMap();
    }
    if (!contentFlag[key]) {
      ContentInsert(key, nameIdx);
      contentFlag.set(key);
    }
  }

  void ClearContentMap() {
    ContentClear();
    ContentShrinkToFit();
  }

  FieldAttrs ConvertToFieldAttrs();
  TypeAttrs ConvertToTypeAttrs() const;
  FuncAttrs ConvertToFuncAttrs();

  std::bitset<kMaxATTRNum> GetAttrs() const {
    return attrFlag;
  }

  std::bitset<kMaxATTRNum> GetContents() const {
    return contentFlag;
  }

  bool GetInit() const {
    return isInit;
  }

 protected:
  GenericAttrs(const GenericAttrs &ta) = default;
  GenericAttrs &operator=(const GenericAttrs &p) = default;
  std::bitset<kMaxATTRNum> attrFlag = 0;
  std::bitset<kMaxATTRNum> contentFlag = 0;
  bool isInit = false;
};

class MapleGenericAttrs : public GenericAttrs {
 public:
  explicit MapleGenericAttrs(MapleAllocator &allocatorIn) : contentMap(allocatorIn.Adapter()) {}
  MapleGenericAttrs(const MapleGenericAttrs &ta) = default;
  MapleGenericAttrs &operator=(const MapleGenericAttrs &p) = default;

  void ContentResize(size_t newSize) override {
    contentMap.resize(newSize);
  };

  void ContentClear() override {
    contentMap.clear();
  };

  void ContentShrinkToFit() override {
    contentMap.shrink_to_fit();
  };

  void ContentInsert(GenericAttrKind key, int val) override {
    contentMap[key] = val;
  };

  void ContentInsert(GenericAttrKind key, GStrIdx nameIdx) override {
    contentMap[key] = nameIdx;
  };

  const std::string &GetAttrStrName(GenericAttrKind key) const override {
    return GlobalTables::GetStrTable().GetStringFromStrIdx(std::get<GStrIdx>(contentMap[key]));
  }

  int GetPriority(GenericAttrKind attr) const override {
    return std::get<int>(contentMap[attr]);
  }

  uint32 GetPack(GenericAttrKind attr) const override {
    return static_cast<uint32>(std::get<int>(contentMap[attr]));
  }

  MapleVector<AttrContent> contentMap;
};

class StlGenericAttrs : public GenericAttrs {
 public:
  StlGenericAttrs() = default;

  StlGenericAttrs(const StlGenericAttrs &ta) = default;

  explicit StlGenericAttrs(const MapleGenericAttrs &ma) {
    contentMap.assign(ma.contentMap.begin(), ma.contentMap.end());
    attrFlag = ma.GetAttrs();
    contentFlag = ma.GetContents();
    isInit = ma.GetInit();
  }

  StlGenericAttrs &operator=(const StlGenericAttrs &p) = default;

  StlGenericAttrs &operator=(const MapleGenericAttrs &pa) {
    contentMap.assign(pa.contentMap.begin(), pa.contentMap.end());
    attrFlag = pa.GetAttrs();
    contentFlag = pa.GetContents();
    isInit = pa.GetInit();
    return *this;
  }

  void ContentResize(size_t newSize) override {
    contentMap.resize(newSize);
  };

  void ContentClear() override {
    contentMap.clear();
  };

  void ContentShrinkToFit() override {
    contentMap.shrink_to_fit();
  };

  void ContentInsert(GenericAttrKind key, int val) override {
    contentMap[key] = val;
  };

  void ContentInsert(GenericAttrKind key, GStrIdx nameIdx) override {
    contentMap[key] = nameIdx;
  };

  const std::string &GetAttrStrName(GenericAttrKind key) const override {
    return GlobalTables::GetStrTable().GetStringFromStrIdx(std::get<GStrIdx>(contentMap[key]));
  }

  int GetPriority(GenericAttrKind attr) const override {
    return std::get<int>(contentMap[attr]);
  }

  uint32 GetPack(GenericAttrKind attr) const override {
    return static_cast<uint32>(std::get<int>(contentMap[attr]));
  }

 private:
  std::vector<AttrContent> contentMap;
};
}
#endif // GENERIC_ATTRS_H