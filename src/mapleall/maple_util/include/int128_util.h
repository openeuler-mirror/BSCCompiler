/*
 * Copyright (c) [2023] Huawei Technologies Co.,Ltd.All rights reserved.
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

#ifndef MAPLE_UTIL_INCLUDE_INT128
#define MAPLE_UTIL_INCLUDE_INT128

#include "mpl_logging.h"
#include "securec.h"
#include "types_def.h"

namespace maple {
using Int128ElemTy = uint64;
constexpr size_t kInt128ElemNum = 2;
constexpr size_t kInt128BitSize = 128;
using Int128Arr = Int128ElemTy[kInt128ElemNum];

class Int128Util {
 public:
  static inline void CopyInt128(Int128ElemTy *dst, const Int128ElemTy *src) {
    constexpr size_t copySize = kInt128ElemNum * sizeof(Int128ElemTy);
    errno_t err = memcpy_s(dst, copySize, src, copySize);
    CHECK_FATAL(err == EOK, "memcpy_s failed");
  }
};

}  // namespace maple

#endif  // MAPLE_UTIL_INCLUDE_INT128
