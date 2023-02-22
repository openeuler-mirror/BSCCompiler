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

import re
import os

from basic_tools.file import read_file
from basic_tools.string_list import *
from basic_tools.string import split_string_by_comma_not_in_bracket_and_quote, get_string_in_outermost_brackets
from case.component import Component
from env_var import EnvVar
from api.shell import Shell
from mode import mode_dict


class OriginParse(Component):

    def __init__(self, input: dict):
        self.case_name = input["case_name"]
        self.mode_set = input["mode_set"]
        self.case_command_suite = {}
        self.case_config = {}

    def execute(self):
        test_cfg = TestCFG(os.path.join(EnvVar.TEST_SUITE_ROOT, self.case_name, "test.cfg"))
        test_cfg_content = test_cfg.test_cfg
        # print(self.case_name)
        # print(self.mode_set)
        for mode in self.mode_set:
            if mode == '':
                continue
            case = SingleCaseParser(self.case_name, mode, test_cfg_content)
            case.parse_case_config()
            self.case_config[mode], self.case_command_suite[mode] = case.get_command_suite()

    def get_output(self):
        return self.case_config, self.case_command_suite


class SingleCaseParser():

    def __init__(self, case_name: str, mode: str, test_cfg_content: dict):
        self.case_name = case_name
        self.mode = mode
        self.test_cfg_content = test_cfg_content
        self.command_suite = []
        self.global_variables = {"CASE": self.case_name, "MODE": self.mode}
        if EnvVar.MAPLE_BUILD_TYPE is not None:
            self.global_variables["MAPLE_BUILD_TYPE"] = EnvVar.MAPLE_BUILD_TYPE

    def parse_case_config(self):
        if self.mode in self.test_cfg_content.keys():
            mode_cfg_content = self.test_cfg_content[self.mode]
        else:
            mode_cfg_content = self.test_cfg_content["default"]
        for line in mode_cfg_content:
            if line[0] == "global":
                self.global_variables.update(line[1])
            elif line[0] == "shell":
                self.command_suite.append(Shell(line[1]).get_command(self.global_variables))
            else:
                local_variables = line[1]
                variables = self.global_variables.copy()
                variables.update(local_variables)
                phase = line[0]
                if phase in mode_dict[self.mode].keys():
                    for command_object in mode_dict[self.mode][phase]:
                        self.command_suite.append(command_object.get_command(variables))
                else:
                    print(line[0] + " not found !")
                    os._exit(1)

    def get_command_suite(self):
        return self.global_variables, self.command_suite


class TestCFG(object):

    mode_phase_pattern = re.compile('^[0-9a-zA-Z_]+$')
    global_var_pattern = re.compile(r"^[\w]+=[\"\'][-\w/\\=. ]+[\"\']$|^[\w]+=[-\w/\\.]+$")

    def __init__(self, test_cfg_path):
        self.test_cfg_content = read_file(test_cfg_path)
        rm_note(self.test_cfg_content)
        rm_wrap(self.test_cfg_content)
        self.test_cfg = {"default":[]}
        self.parse_test_cfg_contest()

    def parse_test_cfg_contest(self):
        mode_list = ["default"]
        for line in self.test_cfg_content:
            if line.endswith(":"):
                mode_list = line[:-1].split(",")
                for mode in mode_list:
                    self.test_cfg[mode] = []
            else:
                for mode in mode_list:
                    if "(" in line and line[-1] == ")" and TestCFG.mode_phase_pattern.match(line.split("(")[0]):
                        phase = line.split("(")[0]
                        variable_table = {}
                        variables_list = split_string_by_comma_not_in_bracket_and_quote(get_string_in_outermost_brackets(line))
                        for variable in variables_list:
                            if "=" in variable:
                                variable_name = variable.split("=")[0]
                                variable_value = "=".join(variable.split("=")[1:])
                                if variable_value.startswith('"') and variable_value.endswith('"'):
                                    variable_value = variable_value[1:-1]
                                variable_table[variable_name] = variable_value
                            else:
                                variable_table["APP"] = variable
                        self.test_cfg[mode].append((phase, variable_table))
                    elif TestCFG.global_var_pattern.match(line):
                        global_variable_name = line.split("=")[0]
                        global_variable_value = "=".join(line.split("=")[1:])
                        if global_variable_value.startswith('"') and global_variable_value.endswith('"'):
                            global_variable_value = global_variable_value[1:-1]
                        self.test_cfg[mode].append(("global", {global_variable_name: global_variable_value}))
                    else:
                        self.test_cfg[mode].append(("shell", line))
