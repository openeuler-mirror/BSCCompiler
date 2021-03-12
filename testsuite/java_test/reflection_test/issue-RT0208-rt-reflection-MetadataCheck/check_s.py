/*
 * Copyright (c) [2021] Huawei Technologies Co.,Ltd.All rights reserved.
 *
 * OpenArkCompiler is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 *
 *     http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR
 * FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 */

import sys

def check_vtableimpl_s(filename):
    f = open(filename, 'r')
    result = list()
    check_cnt = 0
    pass_cnt = 0
    fail_cnt = 0
    for line in f.readlines():
        line = line.strip()
        if line.startswith(".type"):
            result = list()
        result.append(line)
        if line.startswith(".size"):
            if result[0].find("methods_info") != -1:
                check_cnt += 1
                for i in result:
                    if i.find(".section") != -1:
                        if i == '.section .metadata.method,"aw",%progbits':
                            pass_cnt += 1
                        else:
                            fail_cnt += 1
                    elif i.find(".data") != -1:
                        fail_cnt += 1
            elif result[0].find("fields_info") != -1:
                check_cnt += 1
                for i in result:
                    if i.find(".section") != -1:
                        if i == '.section .rometadata.field,"a",%progbits':
                            pass_cnt += 1
                        else:
                            fail_cnt += 1
                    elif i.find(".data") != -1:
                        fail_cnt += 1
            elif result[0].find("fieldOffsetData") != -1:
                check_cnt += 1
                for i in result:
                    if i.find(".section") != -1:
                        fail_cnt += 1
                    elif i.find(".data") != -1:
                        pass_cnt += 1
            else:
                continue
    if check_cnt == pass_cnt and fail_cnt == 0:
        print(0)
    else:
        print(2)


if __name__ == '__main__':
    check_vtableimpl_s(sys.argv[1])

