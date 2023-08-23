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

#ifndef MAPLE_SYSCALL_H_
#define MAPLE_SYSCALL_H_
#include "sys/syscall.h"
#include <sys/prctl.h>
#include "linux/futex.h"
#include  <ctime>

namespace maple {

int futex(volatile int *uaddr, int op, int val, const struct timespec *timeout,
          volatile int *uaddr2, int val3);

pid_t GetTid();

pid_t GetPid();

int TgKill(int tgid, int tid, int sig);

#ifndef PR_SET_VMA
#define PR_SET_VMA           0x53564d41
#define PR_SET_VMA_ANON_NAME 0
#endif

#define MRT_PRCTL(base_address, allocated_size, mmtag) \
    (void)prctl(PR_SET_VMA, PR_SET_VMA_ANON_NAME, base_address, allocated_size, mmtag)

} // namespace maple
#endif // MAPLE_SYSCALL_H_
