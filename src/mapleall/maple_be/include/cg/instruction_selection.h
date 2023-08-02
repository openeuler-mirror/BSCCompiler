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

#ifndef MAPLEBE_INCLUDE_INSTRUCTION_SELECTION_H
#define MAPLEBE_INCLUDE_INSTRUCTION_SELECTION_H

#include "maple_phase_manager.h"

namespace maplebe {

class InsnSel {
 public:
  explicit InsnSel(CGFunc &tempCGFunc) : cgFunc(&tempCGFunc) {}

  virtual ~InsnSel() = default;

  virtual bool InsnSel() = 0;

 protected:
  CGFunc *cgFunc;
};

MAPLE_FUNC_PHASE_DECLARE_BEGIN(CgIsel, maplebe::CGFunc)
MAPLE_FUNC_PHASE_DECLARE_END

} /* namespace maplebe */


#endif /* MAPLEBE_INCLUDE_INSTRUCTION_SELECTION_H */
