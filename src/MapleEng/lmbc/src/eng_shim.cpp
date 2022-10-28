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
#include "mfunction.h"
#include "massert.h"
#include "lmbc_eng.h"

namespace maple {

extern "C" int64
__engineShim(LmbcFunc* fn, ...) {
  uint8   frame[fn->frameSize];
  MValue  pregs[fn->numPregs];
  MValue  formalVars[fn->formalsNumVars+1];
  MFunction shim_caller(fn, nullptr, frame, pregs, formalVars); // create local Mfunction obj for shim

  MValue val;
  if (fn->formalsNum > 0) {
    MValue callArgs[fn->formalsNum];
    va_list args;
    va_start (args, fn);

    int argIdx = 0;
    while (argIdx < fn->formalsNum) {
      // convert argv args to interpreter types and push on operand stack
      val.ptyp = fn->pos2Parm[argIdx]->ptyp;
      switch(val.ptyp) {
        case PTY_i8:
            val.x.i8 = va_arg(args, int);
            break;
        case PTY_i16:
            val.x.i16 = va_arg(args, int);
            break;
        case PTY_i32:
            val.x.i32 = va_arg(args, int);
            break;
        case PTY_i64:
            val.x.i64 = va_arg(args, long long);
            break;
        case PTY_u16:
            val.x.u16 = va_arg(args, int);
            break;
        case PTY_a64:
            val.x.a64 = va_arg(args, uint8*);
            break;
        case PTY_f32:
            // Variadic function expects that float arg is promoted to double
        case PTY_f64:
            val.x.f64 = va_arg(args, double);
            break;
        default:
            MIR_FATAL("Unsupported PrimType %d", val.ptyp);
      }
      callArgs[argIdx] = val;
      ++argIdx;
    }
    shim_caller.numCallArgs = fn->formalsNum;
    shim_caller.callArgs = callArgs;
  }
  val = InvokeFunc(fn, &shim_caller);
  return 0;
}

}
