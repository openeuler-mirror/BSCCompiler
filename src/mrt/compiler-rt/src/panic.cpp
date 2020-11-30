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
#include "panic.h"

asm("    .text\n"
    "    .align 2\n"
    "    .globl abort_saferegister\n"
    "    .type  abort_saferegister, %function\n"
    "abort_saferegister:\n"
#if defined(__aarch64__)
    "    brk #1\n"
    "    ret\n"
#endif
    "    .size    abort_saferegister, .-abort_saferegister");

namespace maplert {
void MRT_Panic() {
  abort();
}

#if __MRT_DEBUG
void __MRT_AssertBreakPoint() {
  // hook for debug
}
#endif
}
