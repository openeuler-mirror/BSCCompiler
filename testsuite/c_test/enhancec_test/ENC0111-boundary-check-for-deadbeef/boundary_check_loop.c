/*
 * Copyright (c) [2020-2022] Huawei Technologies Co.,Ltd.All rights reserved.
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
int isdigit(int a) {
  return a > 0;
}
char *fullname(char *gecos)
{
        static char fname[100];
        register char *cend;

        (void) strcpy(fname, gecos);
        if (cend = index(fname, ','))
                *cend = '\0';
        if (cend = index(fname, '('))
                *cend = '\0';
        /*
        ** Skip USG-style 0000-Name nonsense if necessary.
        */
        if (isdigit(*(cend = fname))) {
                if ((cend = index(fname, '-')) != NULL)
                        cend++;
                else
                        /*
                        ** There was no `-' following digits.
                        */
                        cend = fname;
        }
        return (cend);
}

int main()
{
  return 0;
}
