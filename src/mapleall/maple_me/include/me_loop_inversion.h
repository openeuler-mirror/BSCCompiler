/*
 * Copyright (c) [2022] Huawei Technologies Co.,Ltd.All rights reserved.
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
#ifndef MAPLE_ME_INCLUDE_MELOOPINVERSION_H
#define MAPLE_ME_INCLUDE_MELOOPINVERSION_H
#include "me_function.h"
#include "me_loop_analysis.h"

namespace maple {
// convert loop to do-while format
class MeLoopInversion {
 public:
  MeLoopInversion(bool enableDebugFunc, MemPool &givenMp) : isDebugFunc(enableDebugFunc), innerMp(&givenMp) {}
  ~MeLoopInversion() = default;
  void ExecuteLoopInversion(MeFunction &func, const Dominance &dom);

  bool IsCFGChange() const {
    return isCFGChange;
  }

 private:
  using Key = std::pair<BB*, BB*>;
  void Convert(MeFunction &func, BB &bb, BB &pred, MapleMap<Key, bool> &swapSuccs) const;
  bool NeedConvert(MeFunction *func, BB &bb, BB &pred,
                   MapleAllocator &localAlloc, MapleMap<Key, bool> &swapSuccs) const;

  bool isDebugFunc;
  MemPool *innerMp;
  bool isCFGChange = false;
};

MAPLE_FUNC_PHASE_DECLARE(MELoopInversion, MeFunction)
}  // namespace maple
#endif  // MAPLE_ME_INCLUDE_MELOOPINVERSION_H
