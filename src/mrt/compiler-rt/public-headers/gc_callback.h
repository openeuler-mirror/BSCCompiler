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
#ifndef MAPLE_RUNTIME_GCCALLBACK_H
#define MAPLE_RUNTIME_GCCALLBACK_H

#include <functional>

namespace maplert {
// This call back function type is called when GC finished.
using GCFinishCallbackFunc = std::function<void()>;

using address_t = uintptr_t;
using offset_t = intptr_t;

#ifdef USE_32BIT_REF
using reffield_t = uint32_t;
#else
using reffield_t = address_t ;
#endif // USE_32BIT_REF

#ifdef USE_32BIT_REF
#define DEADVALUE (0xdeaddeadul)  // only stores 32bit value
#else   // !USE_32BIT_REF
#define DEADVALUE (0xdeaddeaddeaddeadul)
#endif  // USE_32BIT_REF

using AddressVisitor = std::function<void(address_t)>;
using RefVisitor = std::function<void(address_t&)>;
using HeapRefVisitor = std::function<void(reffield_t&)>;
}

#endif // MAPLE_RUNTIME_GCCALLBACK_H
