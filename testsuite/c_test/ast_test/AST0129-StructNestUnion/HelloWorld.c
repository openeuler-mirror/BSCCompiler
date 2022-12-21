/*
 * Copyright (c) [2022] Huawei Technologies Co.,Ltd.All rights reserved.
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
#include<stdint.h>

// CHECK: [[# FILENUM:]] "{{.*}}/HelloWorld.c"
typedef int32_t Status;
typedef struct {

    uint16_t a;
    union {
        struct {
            Status opStatus;
            uint32_t b;
        };
        struct {
            uint32_t c;
            uint32_t flags;
        };
    };
    union {
      uint64_t reqStart;
      uint64_t reqEnd;
    };
} MsgT;

int main() {
  static const struct {
    MsgT msg;
    // CHECK: LOC [[# FILENUM]] [[# @LINE + 1 ]]
  } init = {
    // CHECK : var %init_26_5 pstatic <$unnamed.1_24_16> const = [1= [1= 1, 2= [2= [1= 0, 2= 0]], 3= [1= 0]]]
      .msg.a = 1,
      .msg.flags = 0x00000000,
  };
  return 0;
}