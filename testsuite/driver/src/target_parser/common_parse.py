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
import re
import sys

from env_var import EnvVar
from mode_table import ModeTable


class CommonParse(object):
    case_regx = re.compile("^[A-Z]{1,9}[0-9]{3,10}-[a-zA-Z0-9_.]")
    module_regx = re.compile("^[a-z0-9_]{1,20}_test$")

    def __init__(self, input: dict):
        self.target = input["target"]
        self.mode = input["mode"]
        self.default = os.environ.get("MAPLE_BUILD_TYPE")
        if os.path.exists(os.path.join(EnvVar.CONFIG_FILE_PATH, self.target + ".conf")):
            self.mode_table = ModeTable(os.path.join(EnvVar.CONFIG_FILE_PATH, self.target + ".conf"))
        elif self.default:
            self.mode_table = ModeTable(os.path.join(EnvVar.CONFIG_FILE_PATH, "%s.conf"%(self.default)))
        else:
            self.mode_table = ModeTable(os.path.join(EnvVar.CONFIG_FILE_PATH, "testall.conf"))
        self.cases = {}

    def parser_cases(self):
        targets = [self.target]
        if os.path.exists(os.path.join(EnvVar.CONFIG_FILE_PATH, self.target + ".conf")):
            targets = self.mode_table.get_targets()
        for single_target in targets:
            if not os.path.exists(os.path.join(EnvVar.TEST_SUITE_ROOT, single_target)):
                print("Target " + single_target + " doesn't exist !")
                sys.exit(1)
            if CommonParse.case_regx.match(single_target.split('/')[-1]):
                self.cases[single_target] = self.mode_table.get_case_mode_set(single_target)
            elif CommonParse.module_regx.match(single_target.split('/')[-1]):
                subtarget_list = [single_target]
                while subtarget_list:
                    subtarget = subtarget_list.pop(0)
                    for dir in os.listdir(os.path.join(EnvVar.TEST_SUITE_ROOT, subtarget)):
                        if CommonParse.case_regx.match(dir):
                            self.cases[os.path.join(subtarget, dir)] = self.mode_table.get_case_mode_set(os.path.join(subtarget, dir))
                        elif CommonParse.module_regx.match(dir):
                            subtarget_list.append(os.path.join(subtarget, dir))

    def execute(self):
        self.parser_cases()
        if self.mode != None:
            for case in list(self.cases.keys()):
                if self.mode in self.cases[case]:
                    self.cases[case] = {self.mode}
                elif len(self.cases) == 1:
                    print("target has no '%s' mode! alternative modes:{%s}"%(self.mode,",".join(self.cases[case])))
                    del self.cases[case]
                else:
                    del self.cases[case]

    def get_output(self):
        return self.cases
