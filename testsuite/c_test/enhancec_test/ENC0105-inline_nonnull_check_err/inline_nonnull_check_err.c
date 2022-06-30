/*
 * Copyright (c) [2020-2021] Huawei Technologies Co.,Ltd.All rights reserved.
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static int parse_utf16_hex(unsigned int *result) {
    int x1, x2, x3, x4;
    x1 = 1;
    x2 = 2;
    x3 = 3;
    x4 = 4;
    *result = (unsigned int)((x1 << 12) | (x2 << 8) | (x3 << 4) | x4); // CHECK: [[# @LINE ]] error: Dereference of null pointer when inlined to main
    return 1;
}

int main() {
    int hex = parse_utf16_hex(NULL);
    return 0;
}
