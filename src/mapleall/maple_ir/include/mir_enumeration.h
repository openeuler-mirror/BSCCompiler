/*
 * Copyright (c) [2022] Futurewei Technologies Co., Ltd. All rights reserved.
 *
 * OpenArkCompiler is licensed under the Mulan Permissive Software License v2.
 * You can use this software according to the terms and conditions of the MulanPSL - 2.0.
 * You may obtain a copy of MulanPSL - 2.0 at:
 *
 *   https://opensource.org/licenses/MulanPSL-2.0
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR
 * FIT FOR A PARTICULAR PURPOSE.
 * See the MulanPSL - 2.0 for more details.
 */
#ifndef MAPLEIR_INCLUDE_MIR_ENUMERATION_H
#define MAPLEIR_INCLUDE_MIR_ENUMERATION_H

namespace maple {

using EnumElem = std::pair<GStrIdx, IntVal>;
class MIREnum {
 public:
  explicit MIREnum(PrimType ptyp, GStrIdx stridx): primType(ptyp), nameStrIdx(stridx) {}
  ~MIREnum() = default;

  void NewElement(GStrIdx sidx, IntVal value) {
    elements.push_back(EnumElem(sidx, value));
  }
  void AddNextElement(GStrIdx sidx) {
    if (elements.empty()) {
      elements.push_back(EnumElem(sidx, IntVal(0, primType)));
      return;
    }
    IntVal newValue = elements.back().second + 1;
    elements.push_back(EnumElem(sidx, newValue));
  }
  const std::string &GetName() const;
  void Dump() const;

 public:
  PrimType primType = PTY_i32;  // must be integer primtype
  GStrIdx nameStrIdx{ 0 };      // name of this enum in global string table
  std::vector<EnumElem> elements{};
};

struct EnumTable {
  std::vector<MIREnum*> enumTable;
  void Dump() {
    for (MIREnum *mirEnum : enumTable) {
      mirEnum->Dump();
    }
  }
};

}  /* namespace maple */

#endif  /* MAPLEIR_INCLUDE_MIR_ENUMERATION_H */
