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
#include "mvalue.h"
#include "mfunction.h"
#include "massert.h"

namespace maple {

void mload(uint8* addr, PrimType ptyp, MValue& res, size_t aggSize) {
  res.ptyp = ptyp;
  switch(ptyp) {
    case PTY_i8:
      res.x.i64 = *(int8 *)addr;
      return;
    case PTY_i16:
      res.x.i64 = *(int16 *)addr;
      return;
    case PTY_i32:
      res.x.i64 = *(int32 *)addr;
      return;
    case PTY_i64:
      res.x.i64 = *(int64 *)addr;
      return;
    case PTY_u8:
      res.x.u64 = *(uint8 *)addr;
      return;
    case PTY_u16:
      res.x.u64 = *(uint16 *)addr;
      return;
    case PTY_u32:
      res.x.u64 = *(uint32 *)addr;
      return;
    case PTY_u64:
      res.x.u64 = *(uint64 *)addr;
      return;
    case PTY_f32:
      res.x.f32 = *(float *)addr;
      return;
    case PTY_f64:
      res.x.f64 = *(double *)addr;
      return;
    case PTY_a64:
      res.x.a64 = *(uint8 **)addr;
      return;
    case PTY_agg:
      res.x.a64   = addr;
      res.aggSize = aggSize; // agg size
      return;
    default:
      MASSERT(false, "mload ptyp %d NYI", ptyp);
      break;
  }
}

void mstore(uint8* addr, PrimType ptyp, MValue& val, bool toVarArgStack) {
  if (!IsPrimitiveInteger(ptyp) || !IsPrimitiveInteger(val.ptyp)) {
    MASSERT(ptyp == val.ptyp ||
            ptyp == PTY_a64 && val.ptyp == PTY_u64 ||
            ptyp == PTY_u64 && val.ptyp == PTY_a64, 
      "mstore type mismatch: %d and %d", ptyp, val.ptyp);
  }
  switch(ptyp) {
    case PTY_i8:
      *(int8 *)addr  = val.x.i8;
      return;
    case PTY_i16:
      *(int16 *)addr = val.x.i16;
      return;
    case PTY_i32:
      *(int32 *)addr = val.x.i32;
      return;
    case PTY_i64:
      *(int64 *)addr = val.x.i64;
      return;
    case PTY_u8:
      *(uint8 *)addr = val.x.u8;
      return;
    case PTY_u16:
      *(uint16 *)addr = val.x.u16;
      return;
    case PTY_u32:
      *(uint32 *)addr = val.x.u32;
      return;
    case PTY_u64:
      *(uint64 *)addr = val.x.u64;
      return;
    case PTY_f32:
      *(float *)addr  = val.x.f32;
      return;
    case PTY_f64:
      *(double *)addr = val.x.f64;
      return;
    case PTY_a64:
      *(uint8 **)addr = val.x.a64;
      return;
    case PTY_agg:
      if (toVarArgStack) {
        if (val.aggSize > 16) {
          *(uint8 **)addr = val.x.a64;
        } else {
          memcpy(addr, val.x.a64, val.aggSize);
        }
      } else {
        // val holds aggr data (regassign agg of <= 16 bytes to %%retval0) instead of ptr to aggr data
        MASSERT(val.aggSize <= 16, "mstore agg > 16");
        memcpy(addr, &(val.x.u64), val.aggSize);
      }
      return;
    default:
      MASSERT(false, "mstore ptyp %d NYI", ptyp);
      break;
  }
}

} // namespace maple
