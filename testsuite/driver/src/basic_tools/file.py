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

import os

from .string import pre_deal


def read_file(file):
    with open(file, 'r') as f:
        source_lines = f.readlines()
    result_lines = []
    for line in source_lines:
        result_lines.append(pre_deal(line))
    return result_lines


def read_file_in_string(file):
    with open(file, 'r') as f:
        str = f.read()
    return str


def file_exist(path, file):
    target_file = search_file(path, file, equal)
    if target_file == "":
        return False
    else:
        return True


def directory_exist(path, dir):
    target_file = search_dir(path, dir, equal)
    if target_file == "":
        return False
    else:
        return True


def lambda_fun(a, b, fun):
    return fun(a, b)


def contain(a, b):
    return True if a in b else False


def equal(a, b):
    return True if a == b else False


def regular(a, b):
    return True if a.match(b) else False


def search_dir(path, dir_name, type):
    """Search directory in certain path"""
    target_path = ""
    for item in os.listdir(path):
        item_path = os.path.join(path, item)
        if os.path.isdir(item_path):
            if lambda_fun(dir_name, item, type):
                target_path = item_path
                break
            else:
                target_path = search_dir(item_path, dir_name, type)
                if target_path != "":
                    break
    return target_path


def search_file(path, file_name, type):
    """Search file in certain path"""
    target_path = ""
    for item in os.listdir(path):
        item_path = os.path.join(path, item)
        if os.path.isdir(item_path):
            target_path = search_file(item_path, file_name, type)
            if target_path != "":
                break
        elif os.path.isfile(item_path):
            if lambda_fun(file_name, item, type):
                target_path = item_path
                break
    return target_path


def search_all_file(path, file_name, type):
    """Search all file in certain path"""
    all_target_path = []
    target_path = []
    for item in os.listdir(path):
        item_path = os.path.join(path, item)
        if os.path.isdir(item_path):
            target_path = search_all_file(item_path, file_name, type)
            if target_path != []:
                all_target_path += target_path
        elif os.path.isfile(item_path):
            if lambda_fun(file_name, item, type):
                all_target_path.append(item_path)
                break
    return all_target_path

def walk_all_files(path):
    all_file_list = []
    for fpathe, dirs, fs in os.walk(path):
        all_file_list += [os.path.join(fpathe,f) for f in fs]
    return all_file_list

def rm_empty_folder(path):
    if not os.path.isdir(path):
        return
    files = os.listdir(path)
    for fullpath in [os.path.join(path, f) for f in files]:
        rm_empty_folder(fullpath)
    files = os.listdir(path)
    if not len(files):
        os.rmdir(path)

def get_sub_files(path):
    all_sub_files = []
    for folder in os.listdir(path):
        if not os.path.isdir(folder):
            continue
        for fpathe, dirs, fs in os.walk(folder):
            all_sub_files += [os.path.join(fpathe,file) for file in fs]
    return all_sub_files

def get_test_and_nontest_curdir(curdir:str, exclude_subdir=[]):
    test_suites = []
    nontest_suites = []
    for subdir in os.listdir(curdir):
        if subdir in exclude_subdir:
            continue
        elif subdir.endswith("_test"):
            test_suites.append(subdir)
        else:
            nontest_suites.append(subdir)
    return test_suites, nontest_suites
