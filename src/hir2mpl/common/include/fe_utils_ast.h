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
#ifndef HIR2MPL_INCLUDE_FE_UTILS_AST_H
#define HIR2MPL_INCLUDE_FE_UTILS_AST_H
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
std::function<T()> OpGenerator(Opcode op, T p0, T p1, bool isSigned) {
  switch (op) {
    case OP_add: {
      return [p0, p1]() { return p0 + p1; };
    }
    case OP_sub: {
      return [p0, p1]() { return p0 - p1; };
    }
    case OP_mul: {
      return [p0, p1]() { return p0 * p1; };
    }
    case OP_div: {
      if (isSigned) {
        return [p0, p1]() { return static_cast<int64>(p0) / static_cast<int64>(p1); };
      } else {
        return [p0, p1]() { return static_cast<uint64>(p0) / static_cast<uint64>(p1); };
      }
    }
    case OP_rem: {
      if (isSigned) {
        return [p0, p1]() { return static_cast<int64>(p0) % static_cast<int64>(p1); };
      } else {
        return [p0, p1]() { return static_cast<uint64>(p0) % static_cast<uint64>(p1); };
      }
    }
    case OP_shl: {
      if (isSigned) {
        return [p0, p1]() { return static_cast<int64>(p0) << static_cast<int64>(p1); };
      } else {
        return [p0, p1]() { return static_cast<uint64>(p0) << static_cast<uint64>(p1); };
      }
    }
    case OP_lshr:
    case OP_ashr: {
      if (isSigned) {
        return [p0, p1]() { return static_cast<int64>(p0) >> static_cast<int64>(p1); };
      } else {
        return [p0, p1]() { return static_cast<uint64>(p0) >> static_cast<uint64>(p1); };
      }
    }
    case OP_bior: {
      if (isSigned) {
        return [p0, p1]() { return static_cast<int64>(p0) | static_cast<int64>(p1); };
      } else {
        return [p0, p1]() { return static_cast<uint64>(p0) | static_cast<uint64>(p1); };
      }
    }
    case OP_band: {
      if (isSigned) {
        return [p0, p1]() { return static_cast<int64>(p0) & static_cast<int64>(p1); };
      } else {
        return [p0, p1]() { return static_cast<uint64>(p0) & static_cast<uint64>(p1); };
      }
    }
    case OP_bxor: {
      if (isSigned) {
        return [p0, p1]() { return static_cast<int64>(p0) ^ static_cast<int64>(p1); };
      } else {
        return [p0, p1]() { return static_cast<uint64>(p0) ^ static_cast<uint64>(p1); };
      }
    }
    case OP_land: {
      return [p0, p1]() {
        if (!p0) {
          return static_cast<int32>(0);
        } else if (!p1) {
          return static_cast<int32>(0);
        } else {
          return static_cast<int32>(1);
        }
      };
    }
    case OP_lior: {
      return [p0, p1]() {
        if (p0) {
          return static_cast<int32>(1);
        } else if (p1) {
          return static_cast<int32>(1);
        } else {
          return static_cast<int32>(0);
        }
      };
    }
    case OP_cand: {
      return [p0, p1]() {
        if (!p0) {
          return static_cast<int32>(0);
        } else if (!p1) {
          return static_cast<int32>(0);
        } else {
          return static_cast<int32>(1);
        }
      };
    }
    case OP_cior: {
      return [p0, p1]() {
        if (p0) {
          return static_cast<int32>(1);
        } else if (p1) {
          return static_cast<int32>(1);
        } else {
          return static_cast<int32>(0);
        }
      };
    }
    default: {
      return nullptr;
    }
  }
  return nullptr;
}

template <class T>
T *MIRConstGenerator(MemPool *mp, T *konst0, T *konst1, Opcode op) {
  bool isSigned = IsSignedInteger(konst0->GetType().GetPrimType()) && IsSignedInteger(konst1->GetType().GetPrimType());
  return mp->New<T>(OpGenerator(op, konst0->GetValue(), konst1->GetValue(), isSigned)(), konst0->GetType());
}
}  // namespace maple
#endif  // HIR2MPL_INCLUDE_FE_UTILS_AST_H
