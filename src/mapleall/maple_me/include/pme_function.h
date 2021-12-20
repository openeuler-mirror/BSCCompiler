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

#ifndef MAPLE_ME_INCLUDE_PME_FUNCTION_H
#define MAPLE_ME_INCLUDE_PME_FUNCTION_H
#include "pme_mir_extension.h"
#include "me_ir.h"

namespace maple {
class MeFunction;

class PreMeWhileInfo {
 public:
  MIRSymbol *injectedIVSym = nullptr;   // the injected IV
  OriginalSt *ivOst = nullptr;          // the primary IV
  MeExpr *initExpr = nullptr;
  int32 stepValue = 0;
  MeExpr *tripCount = nullptr;
  bool canConvertDoloop = false;
};

class PreMeIfInfo {
 public:
  LabelIdx endLabel = 0;   // the label that is out of the if statement
  LabelIdx elseLabel = 0;  // the label that is the begin of else branch
};

class PreMeFunction {
 public:
  MemPool *pmemp;
  MapleAllocator pmeAlloc;
  MeFunction *meFunc;
  // key is label at beginning of lowered while code sequence
  MapleMap<LabelIdx, PreMeWhileInfo*> label2WhileInfo;
  // key is target label of first conditional branch of lowered if code sequence
  MapleMap<LabelIdx, PreMeIfInfo*> label2IfInfo;
  // for the labels that were created by PreMe, we won't emit it
  MapleSet<LabelIdx> pmeCreatedIfLabelSet;
  MapleSet<LabelIdx> pmeCreatedWhileLabelSet;

 public:
  PreMeFunction(MemPool *mp, MeFunction *func)
      : pmemp(mp),
        pmeAlloc(mp),
        meFunc(func),
        label2WhileInfo(pmeAlloc.Adapter()),
        label2IfInfo(pmeAlloc.Adapter()),
        pmeCreatedIfLabelSet(pmeAlloc.Adapter()),
        pmeCreatedWhileLabelSet(pmeAlloc.Adapter()) {}
  virtual ~PreMeFunction() = default;

  void SetIfLabelCreatedByPreMe(LabelIdx lbidx) {
    pmeCreatedIfLabelSet.insert(lbidx);
  }

  bool IfLabelCreatedByPreMe(LabelIdx lbidx) {
    return pmeCreatedIfLabelSet.count(lbidx) != 0;
  }

  void SetWhileLabelCreatedByPreMe(LabelIdx lbidx) {
    pmeCreatedWhileLabelSet.insert(lbidx);
  }

  bool WhileLabelCreatedByPreMe(LabelIdx lbidx) {
    return pmeCreatedWhileLabelSet.count(lbidx) != 0;
  }
};
}  // namespace maple
#endif  // MAPLE_ME_INCLUDE_PME_FUNCTION_H
