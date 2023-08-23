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

#ifndef MAPLE_RUNTIME_THREAD_OFFSETS_H_
#define MAPLE_RUNTIME_THREAD_OFFSETS_H_

#if defined(__aarch64__)
constexpr size_t kThreadIdOffset = 8; // 8 bytes
#elif defined(__arm__)
constexpr size_t kThreadIdOffset = 4; // 4 bytes
#endif

// obj+ofst = lock_word address;
#ifdef USE_32BIT_REF
constexpr size_t kLockWordOffset = 4;  // 4 bytes
#else
constexpr size_t kLockWordOffset = 8;  // 8 bytes
#endif

#endif  // MAPLE_RUNTIME_THREAD_OFFSETS_H_
