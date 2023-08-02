/*
 * Copyright (c) [2021] Huawei Technologies Co.,Ltd.All rights reserved.
 *
 * OpenArkCompiler is licensed under the Mulan PSL v1.
 * You can use this software according to the terms and conditions of the Mulan PSL v1.
 * You may obtain a copy of Mulan PSL v1 at:
 *
 *     http://license.coscl.org.cn/MulanPSL
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR
 * FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v1 for more details.
 */
#ifndef MPL2MPL_INCLUDE_CLASS_HIERARCHY_PHASE_H
#define MPL2MPL_INCLUDE_CLASS_HIERARCHY_PHASE_H
#include "class_hierarchy.h"
#include "maple_phase.h"

namespace maple {
MAPLE_MODULE_PHASE_DECLARE_BEGIN(M2MKlassHierarchy)
  KlassHierarchy *GetResult() {
    return kh;
  }
  KlassHierarchy *kh = nullptr;
MAPLE_MODULE_PHASE_DECLARE_END
}  // namespace maple
#endif  // MPL2MPL_INCLUDE_CLASS_HIERARCHY_H

