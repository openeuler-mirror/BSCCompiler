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
import sys

from basic_tools.file import read_file

class ModeTable(object):

    def __init__(self, table_file):
        global key
        self.table_file = table_file
        mode_table_content_tmp = read_file(table_file)
        self.mode_table_content = {}
        for line in mode_table_content_tmp:
            line = line.replace(" ", "")
            if line == "" or line.startswith("#"):
                continue
            elif line.startswith("["):
                key = line.split("[")[1].split("]")[0]
                self.mode_table_content[key] = {}
            elif line.endswith(":all"):
                target = line.split(":")[0]
                self.mode_table_content[key][target] = {"all"}
            else:
                target = line.split(":")[0]
                mode_set = set(line.split(":")[1].split(","))
                if target in self.mode_table_content[key].keys():
                    self.mode_table_content[key][target] = self.mode_table_content[key][target].union(mode_set)
                else:
                    self.mode_table_content[key][target] = mode_set
        self.mode_table = {}
        self._parse()

    def _parse(self):
        self.mode_table = self.mode_table_content["DEFAULT_TEST_SUITE"]
        if "MODE_SET" in self.mode_table_content.keys():
            for options in self.mode_table_content["MODE_SET"].keys():
                for target in self.mode_table.keys():
                    if options in self.mode_table[target]:
                        self.mode_table[target].remove(options)
                        self.mode_table[target] = self.mode_table[target].union(self.mode_table_content["MODE_SET"][options])
        if "BAN_TEST_SUITE" in self.mode_table_content.keys():
            for target in self.mode_table_content["BAN_TEST_SUITE"].keys():
                if "all" in self.mode_table_content["BAN_TEST_SUITE"][target]:
                    self.mode_table[target] = set()
                    continue
                father_target = target
                while father_target not in self.mode_table.keys():
                    father_target = os.path.dirname(father_target)
                    if father_target == "":
                        print("\033[31m%s have no DEFAULT_TEST_SUITE settings. pls delete this in %s\033[0m"%(target, self.table_file))
                        sys.exit(1)
                self.mode_table[target] = self.mode_table[father_target] - self.mode_table_content["BAN_TEST_SUITE"][target]

    def get_case_mode_set(self, case):
        target = case
        while target not in self.mode_table.keys() and len(target) != 0:
            target = os.path.dirname(target)
            if len(target) == 0:
                return set()
        return self.mode_table[target]

    def get_targets(self):
        return self.mode_table.keys()
