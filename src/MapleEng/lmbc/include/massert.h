/*
 * Copyright (c) [2022] Futurewei Technologies, Inc. All rights reserved.
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
#ifndef MPLENG_MASSERT_H_
#define MPLENG_MASSERT_H_

#include <cstdio>
#include <cstdlib>

#define MASSERT(cond, fmt, ...) \
    do { \
      if (!(cond)) { \
        fprintf(stderr, __FILE__ ":%d: Assert failed: " fmt "\n", __LINE__, ##__VA_ARGS__); \
        abort(); \
      } \
    } while (0)

#endif // MPLENG_MASSERT_H_

