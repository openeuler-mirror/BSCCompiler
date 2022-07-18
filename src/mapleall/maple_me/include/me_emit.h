/*
 * Copyright (c) [2019-2021] Huawei Technologies Co.,Ltd.All rights reserved.
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
#ifndef MAPLE_ME_INCLUDE_ME_EMIT_H
#define MAPLE_ME_INCLUDE_ME_EMIT_H
#include "me_function.h"
#include "maple_phase.h"

// emit ir to specified file
namespace maple {
MAPLE_FUNC_PHASE_DECLARE(MEEmit, MeFunction)
MAPLE_FUNC_PHASE_DECLARE(ProfileGenEmit, MeFunction)
}  // namespace maple
#endif  // MAPLE_ME_INCLUDE_ME_EMIT_H
