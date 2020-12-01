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
#ifndef MAPLE_RUNTIME_MM_CONFIG_COMMON_H
#define MAPLE_RUNTIME_MM_CONFIG_COMMON_H

#ifndef CONFIG_JSAN
#define CONFIG_JSAN 0
#endif

// By default, enable unit test code on non-android
// platform (for example: qemu on host linux).
#ifndef MRT_UNIT_TEST
#ifndef __ANDROID__
#define MRT_UNIT_TEST 1
#endif
#endif

namespace maplert {
// Use this inline function to "use" a variable so that the compiler will not
// complain about unused variables.  This is especially useful when implementing
// gc-only where functions like RC_LOCAL_INC_REF and RC_LOCAL_DEC_REF do not
// do anything.
template<class ... Types>
__attribute__((always_inline)) inline void MRT_DummyUse(Types&& ... args __attribute__((unused))) {}
} // namespace maplert


#endif //MAPLE_RUNTIME_MMCONFIG_COMMON_H
