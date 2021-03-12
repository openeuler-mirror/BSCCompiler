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

from basic_tools.file import read_file

class ModTable(object):

    def __init__(self, table_file):
        global key
        mod_table_content_tmp = read_file(table_file)
        self.mod_table_content = {}
        for line in mod_table_content_tmp:
            line = line.replace(" ", "")
            if line == "" or line.startswith("#"):
                continue
            elif line.startswith("["):
                key = line.split("[")[1].split("]")[0]
                self.mod_table_content[key] = {}
            elif line.endswith(":all"):
                target = line.split(":")[0]
                self.mod_table_content[key][target] = {"all"}
            else:
                target = line.split(":")[0]
                mod_set = set(line.split(":")[1].split(","))
                if target in self.mod_table_content[key].keys():
                    self.mod_table_content[key][target] = self.mod_table_content[key][target].union(mod_set)
                else:
                    self.mod_table_content[key][target] = mod_set
        self.mod_table = {}
        self._parse()

    def _parse(self):
        self.mod_table = self.mod_table_content["DEFAULT_SUITE"]
        if "OPTION_SUITE" in self.mod_table_content.keys():
            for options in self.mod_table_content["OPTION_SUITE"].keys():
                for target in self.mod_table.keys():
                    if options in self.mod_table[target]:
                        self.mod_table[target].remove(options)
                        self.mod_table[target] = self.mod_table[target].union(self.mod_table_content["OPTION_SUITE"][options])
        if "BAN_SUITE" in self.mod_table_content.keys():
            for target in self.mod_table_content["BAN_SUITE"].keys():
                if "all" in self.mod_table_content["BAN_SUITE"][target]:
                    self.mod_table[target] = set()
                    continue
                father_target = target
                while father_target not in self.mod_table.keys():
                    father_target = os.path.dirname(father_target)
                self.mod_table[target] = self.mod_table[father_target] - self.mod_table_content["BAN_SUITE"][target]

    def get_target_mod_set(self, target):
        while target not in self.mod_table.keys() and len(target) != 0:
            target = os.path.dirname(target)
            if len(target) == 0:
                return set()
        return self.mod_table[target]

    def get_targets(self):
        return self.mod_table.keys()
