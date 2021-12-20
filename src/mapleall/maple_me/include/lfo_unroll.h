/*
 * Copyright (c) [2021] Huawei Technologies Co., Ltd. All rights reserved.
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

#ifndef MAPLE_ME_INCLUDE_LFO_UNROLL_H
#define MAPLE_ME_INCLUDE_LFO_UNROLL_H

#include "pme_function.h"
#include "pme_emit.h"
#include "lfo_dep_test.h"
#include "orig_symbol.h"
#include "me_ir.h"

namespace maple {

class LfoUnrollOneLoop {
 public:
  LfoUnrollOneLoop(PreMeFunction *f, PreMeEmitter *preEm, DoloopInfo *doinfo) :
        preMeFunc(f), preEmit(preEm), doloopInfo(doinfo), doloop(doinfo->doloop),
        mirModule(&f->meFunc->GetMIRModule()), codeMP(preEm->GetCodeMP()),
        mirBuilder(mirModule->GetMIRBuilder()) {}
  ~LfoUnrollOneLoop() = default;
  BaseNode *CloneIVNode();
  bool IsIVNode(BaseNode *x);
  void ReplaceIV(BaseNode *x, BaseNode *repNode);
  BlockNode *DoFullUnroll(size_t tripCount);
  BlockNode *DoUnroll(size_t times, size_t tripCount);
  void Process();

  PreMeFunction *preMeFunc;
  PreMeEmitter *preEmit;
  DoloopInfo *doloopInfo;
  DoloopNode *doloop;
  MIRModule *mirModule;
  MemPool *codeMP;     // to generate new code
  MIRBuilder *mirBuilder;
  int64 stepAmount = 0;
  PrimType ivPrimType = PTY_unknown;
  static uint32 countOfLoopsUnrolled;
};

MAPLE_FUNC_PHASE_DECLARE(MELfoUnroll, MeFunction)

}  // namespace maple
#endif  // MAPLE_ME_INCLUDE_LFO_UNROLL_H
