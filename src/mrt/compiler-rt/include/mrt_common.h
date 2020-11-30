/*
 * Copyright (c) [2019-2020] Huawei Technologies Co.,Ltd.All rights reserved.
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
#ifndef MRT_COMMON_H
#define MRT_COMMON_H

// This file is used to declare some common use variables between mrt and mapleall
// For decoupling load
constexpr uint32_t kMplStaticDecoupleMagicNumber = 0x1a0;               // For non-lazy static decouple trigger SIGSEGV
constexpr uint32_t kMplLazyStaticDecoupleMagicNumber = 0x1a1;           // For lazy static decouple trigger SIGSEGV
constexpr uint32_t kMplLazyLoadMagicNumber = 0x1a2;                     // For lazy decouple trigger SIGSEGV
constexpr uint64_t kMplLazyLoadSentryNumber = 0x1a27b10d10810ade;       // Sentry for offset table
constexpr uint64_t kMplStaticLazyLoadSentryNumber = 0x1a27b10d10810ad1; // Sentry for static offset table
constexpr uint32_t kMplArrayClassCacheMagicNumber = 0x1a3;              // For init array class cache trigger SIGSEGV
#endif // MRT_COMMON_H
