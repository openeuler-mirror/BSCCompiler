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
#ifndef MPLFE_INCLUDE_FE_UTILS_AST_H
#define MPLFE_INCLUDE_FE_UTILS_AST_H
#include <string>
#include <vector>
#include "types_def.h"
#include "cfg_primitive_types.h"
#include "mempool.h"
#include "opcodes.h"
#include "mir_const.h"

namespace maple {
class FEUtilAST {
 public:
  static PrimType GetTypeFromASTTypeName(const std::string &typeName);
  static const std::string Type2Label(PrimType primType);

 private:
  FEUtilAST() = default;
  ~FEUtilAST() = default;
};

template <class T>
std::function<T()> OpGenerator(Opcode op, T p0, T p1) {
  switch (op) {
    case OP_add: {
       return [&]() { return p0 + p1; };
    }
    case OP_sub: {
      return [&]() { return p0 - p1; };
    }
    case OP_mul: {
      return [&]() { return p0 * p1; };
    }
    case OP_div: {
      return [&]() { return p0 / p1; };
    }
    case OP_shl: {
      return [&]() { return static_cast<int64>(p0) << static_cast<int64>(p1); };
    }
    case OP_lshr:
    case OP_ashr: {
      return [&]() { return static_cast<int64>(p0) >> static_cast<int64>(p1); };
    }
    case OP_bior: {
      return [&]() { return static_cast<int64>(p0) | static_cast<int64>(p1); };
    }
    case OP_bxor: {
      return [&]() { return static_cast<int64>(p0) ^ static_cast<int64>(p1); };
    }
    default: {
      return nullptr;
    }
  }
  return nullptr;
}

template <class T>
T *MIRConstGenerator(MemPool *mp, T *konst0, T *konst1, Opcode op) {
  return mp->New<T>(OpGenerator(op, konst0->GetValue(), konst1->GetValue())(), konst0->GetType());
}
}  // namespace maple
#endif  // MPLFE_INCLUDE_FE_UTILS_AST_H
