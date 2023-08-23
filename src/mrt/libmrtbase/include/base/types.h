/*
 * Copyright (c) [2020] Huawei Technologies Co.,Ltd.All rights reserved.
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

#ifndef MAPLE_RUNTIME_BASE_TYPES_H_
#define MAPLE_RUNTIME_BASE_TYPES_H_

#include <atomic>

namespace maple {

using address_t = uintptr_t;

using MonitorId = size_t;

using AtomicUInteger = std::atomic<uint32_t>;

} // namespace maple

#endif  // MAPLE_RUNTIME_BASE_TYPES_H_
