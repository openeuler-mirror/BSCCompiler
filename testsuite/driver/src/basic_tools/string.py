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

def pre_deal(str):
    str = rm_outermost_quotes(str)
    str = rm_outermost_double_quotes(str)
    str = rm_wrap(str)
    str = rm_outermost_blank(str)
    return str


def rm_outermost_blank(str):
    return str.strip()


def rm_outermost_double_quotes(str):
    if str.startswith('"') and str.endswith('"'):
        return str[1:-1]
    else:
        return str


def rm_outermost_quotes(str):
    if str.startswith("'") and str.endswith("'"):
        return str[1:-1]
    else:
        return str


def rm_wrap(str):
    if str.endswith('\n'):
        return str.strip('\n')
    else:
        return str


def get_char_count(char, str):
    count = 0
    for i in str:
        if i == char:
            count += 1
    return count


def get_outermost_layer_with_two_symbols(split_symbol1, split_symbol2, str):
    if split_symbol1 not in str or split_symbol2 not in str:
        return ""
    result_str = ""
    start = 0
    num = get_char_count(split_symbol2, str)
    for char in str:
        if char == split_symbol1:
            if start == 0:
                start = 1
            else:
                result_str += char
        elif char == split_symbol2:
            num -= 1
            if num > 0 and start == 1:
                result_str += char
        else:
            if num >= 1 and start == 1:
                result_str += char
    return result_str


def get_outermost_layer_with_one_symbol(split_symbol, str):
    if split_symbol not in str:
        return str
    result_str = ""
    num = get_char_count(split_symbol, str)
    start = 0
    for char in str:
        if char == split_symbol:
            num -= 1
            if start == 0:
                start = 1
            else:
                if num > 0:
                    result_str += char
            num -= 1
        else:
            if start == 1:
                result_str += char
    return result_str


def get_string_in_outermost_brackets(str):
    return get_outermost_layer_with_two_symbols('(', ')', str)


def get_string_in_outermost_double_quotes(str):
    return get_outermost_layer_with_one_symbol('"', str)


def split_string_not_in_two_symbols(symbol1, symbol2, split_symbol, str):
    in_or_not = 0
    split_list = []
    tmp = ""
    for char in str:
        if char == symbol1:
            in_or_not += 1
            tmp += char
        elif char == symbol2:
            in_or_not -= 1
            tmp += char
        elif char == split_symbol and in_or_not == 0:
            split_list.append(tmp)
            tmp = ""
        else:
            tmp += char
    if tmp != "":
        split_list.append(tmp)
    return split_list


def split_string_not_in_the_same_symbols(symbol, split_symbol, str):
    in_or_not = 0
    split_list = []
    tmp = ""
    old_char = ""
    for char in str:
        if char == symbol and in_or_not == 0:
            in_or_not = 1
            tmp += char
        elif char == symbol and in_or_not == 1:
            in_or_not = 0
            tmp += char
        elif char == split_symbol and old_char != "\\" and in_or_not == 0:
            split_list.append(tmp)
            tmp = ""
        else:
            tmp += char
        old_char = char
    if tmp != "":
        split_list.append(tmp)
    return split_list

def split_string_by_comma_not_in_double_quotes(str):
    return split_string_not_in_the_same_symbols('"', ',', str)


def contain_symbol_outside_outermost_two_symbols(symbol, symbol1, symbol2, str):
    in_or_not = 0
    for char in str:
        if char == symbol1:
            in_or_not += 1
        elif char == symbol2:
            in_or_not -= 1
        elif char == symbol and in_or_not == 0:
            return True
        else:
            continue
    return False


def contain_symbol_outside_outermost_the_same_symbols(symbol, symbol_in, str):
    in_or_not = 0
    for char in str:
        if char == symbol_in and in_or_not == 0:
            in_or_not = 1
        elif char == symbol_in and in_or_not == 1:
            in_or_not = 0
        elif char == symbol and in_or_not == 0:
            return True
        else:
            continue
    return False


def contain_symbol_outside_outermost_common_symbols(symbol, str):
    pre_deal(str)
    result1 = contain_symbol_outside_outermost_the_same_symbols(symbol, '"', str)
    result2 = contain_symbol_outside_outermost_the_same_symbols(symbol, "'", str)
    result3 = contain_symbol_outside_outermost_two_symbols(symbol, '(', ')', str)
    result4 = contain_symbol_outside_outermost_two_symbols(symbol, '[', ']', str)
    result5 = contain_symbol_outside_outermost_two_symbols(symbol, '{', '}', str)
    return result1 & result2 & result3 & result4


def split_string_by_comma_not_in_bracket_and_quote(String):
    in_bracket = 0
    in_double_quote = 0
    in_single_quote = 0
    split_list = []
    tmp = ""
    old_char = ""
    for ch in String:
        if ch == '[' and in_bracket == 0:
            in_bracket = 1
        elif ch == ']' and in_bracket == 1:
            in_bracket = 0
        elif ch == '"' and in_double_quote == 0:
            in_double_quote = 1
        elif ch == '"' and in_double_quote == 1:
            in_double_quote = 0
        elif ch == "'" and in_single_quote == 0:
            in_single_quote = 1
        elif ch == "'" and in_single_quote == 1:
            in_single_quote = 0
        elif ch == ',' and old_char != '\\' and not (in_bracket or in_double_quote or in_single_quote):
            split_list.append(tmp)
            old_char = ch
            tmp = ''
            continue
        tmp += ch
        old_char = ch
    if tmp != "":
        split_list.append(tmp)
    return split_list
