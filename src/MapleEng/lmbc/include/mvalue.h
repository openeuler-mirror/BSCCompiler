/*
 * Copyright (c) [2022] Futurewei Technologies, Inc. All rights reserved.
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
#ifndef MPLENG_MVALUE_H_
#define MPLENG_MVALUE_H_

#include <cstdint>
#include "prim_types.h"

namespace maple {
  struct MValue {
    union {
      int8    i8;
      int16   i16;
      int32   i32;
      int64   i64;
      uint8   u8;
      uint16  u16;
      uint32  u32;
      uint64  u64;
      float   f32;
      double  f64;
      uint8   *a64;    // object ref (use uint8_t* instead of void* for reference)
      void    *ptr;
      void    *str;
    } x;
    PrimType ptyp:8;
    size_t   aggSize;  // for PTY_agg only
  };
}

#endif // MPLENG_MVALUE_H_

