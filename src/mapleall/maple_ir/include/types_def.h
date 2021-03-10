/*
 * Copyright (c) [2019-2020] Huawei Technologies Co.,Ltd.All rights reserved.
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
#ifndef MAPLE_IR_INCLUDE_TYPES_DEF_H
#define MAPLE_IR_INCLUDE_TYPES_DEF_H

// NOTE: Since we already committed to -std=c++0x, we should eventually use the
// standard definitions in the <cstdint> and <limits> headers rather than
// reinventing our own primitive types.
#include <cstdint>
#include <cstddef>
#include "mpl_number.h"

namespace maple {
// Let's keep the following definitions so that existing code will continue to work.
using int8 = std::int8_t;
using int16 = std::int16_t;
using int32 = std::int32_t;
using int64 = std::int64_t;
using uint8 = std::uint8_t;
using uint16 = std::uint16_t;
using uint32 = std::uint32_t;
using uint64 = std::uint64_t;
class StIdx {  // scope nesting level + symbol table index
 public:
  union un {
    struct {
      uint32 idx : 24;
      uint8 scope;  // scope level, with the global scope is at level 1
    } scopeIdx;

    uint32 fullIdx;
  };

  StIdx() {
    u.fullIdx = 0;
  }

  StIdx(uint32 level, uint32 i) {
    u.scopeIdx.scope = level;
    u.scopeIdx.idx = i;
  }

  StIdx(uint32 fidx) {
    u.fullIdx = fidx;
  }

  ~StIdx() = default;

  uint32 Idx() const {
    return u.scopeIdx.idx;
  }

  void SetIdx(uint32 idx) {
    u.scopeIdx.idx = idx;
  }

  uint32 Scope() const {
    return u.scopeIdx.scope;
  }

  void SetScope(uint32 scpe) {
    u.scopeIdx.scope = scpe;
  }

  uint32 FullIdx() const {
    return u.fullIdx;
  }

  void SetFullIdx(uint32 idx) {
    u.fullIdx = idx;
  }

  bool Islocal() const {
    return u.scopeIdx.scope > 1;
  }

  bool IsGlobal() const {
    return u.scopeIdx.scope == 1;
  }

  bool operator==(const StIdx &x) const {
    return u.fullIdx == x.u.fullIdx;
  }

  bool operator!=(const StIdx &x) const {
    return !(*this == x);
  }

  bool operator<(const StIdx &x) const {
    return u.fullIdx < x.u.fullIdx;
  }

 private:
  un u;
};

using LabelIdx = uint32;
using LabelIDOrder = uint32;
using PUIdx = uint32;
using PregIdx = int32;
using PregIdx16 = int16;
using ExprIdx = int32;
using FieldID = int32;

class TypeTag;
using TyIdx = utils::Index<TypeTag, uint32>;        // global type table index

class GStrTag;
using GStrIdx = utils::Index<GStrTag, uint32>;      // global string table index

class UStrTag;
using UStrIdx = utils::Index<UStrTag, uint32>;      // user string table index (from the conststr opcode)

class U16StrTag;
using U16StrIdx = utils::Index<U16StrTag, uint32>;  // user string table index (from the conststr opcode)

const TyIdx kInitTyIdx = TyIdx(0);
const TyIdx kNoneTyIdx = TyIdx(UINT32_MAX);

constexpr uint8 kOperandNumUnary = 1;
constexpr uint8 kOperandNumBinary = 2;
constexpr uint8 kOperandNumTernary = 3;
}  // namespace maple
#endif  // MAPLE_IR_INCLUDE_TYPES_DEF_H
