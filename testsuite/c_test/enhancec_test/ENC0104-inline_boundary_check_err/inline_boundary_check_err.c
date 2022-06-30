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
static char * parson_strndup(const char *string, size_t n) {
    char *output_string = (char*)malloc(n);
    if (!output_string) {
        return NULL;
    }
    output_string[n] = '\0'; // CHECK: [[# @LINE ]] error: the pointer >= the upper bounds when accessing the memory and inlined to main
    return output_string;
}

int main() {
    const char *string = "test";
    char *s = parson_strndup(string, strlen(string) + 1);
    return 0;
}
