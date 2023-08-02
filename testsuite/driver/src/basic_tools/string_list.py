#!/usr/bin/env python
# coding=utf-8
#
# Copyright (c) [2021] Huawei Technologies Co.,Ltd.All rights reserved.
#
# OpenArkCompiler is licensed under Mulan PSL v2.
# You can use this software according to the terms and conditions of the Mulan PSL v2.
#
#     http://license.coscl.org.cn/MulanPSL2
#
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR
# FIT FOR A PARTICULAR PURPOSE.
# See the Mulan PSL v2 for more details.
#

def rm_wrap(string_list):
    for index in range(len(string_list)):
        string_list[index] = string_list[index].strip('\n')


def rm_note(string_list):
    string_list_tmp = string_list
    for line in string_list_tmp:
        if line.startswith("#"):
            string_list.remove(line)


def rm_outermost_double_quotes(string_list):
    for item in range(len(string_list)):
        if string_list[item].startswith('"') and string_list[item].endswith('"'):
            string_list[item] = string_list[item][1:-1]
