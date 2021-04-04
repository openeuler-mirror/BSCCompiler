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

from basic_tools.file import read_file
from basic_tools.string_list import *
from basic_tools.string import split_string_by_comma_not_in_double_quotes, get_string_in_outermost_brackets

class TestCFG(object):

    mode_phase_pattern = re.compile('^[0-9a-zA-Z_]+$')

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
                        variables_list = split_string_by_comma_not_in_double_quotes(get_string_in_outermost_brackets(line))
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
                    elif "=" in line and " " not in line:
                        global_variable_name = line.split("=")[0]
                        global_variable_value = "=".join(line.split("=")[1:])
                        if global_variable_value.startswith('"') and global_variable_value.endswith('"'):
                            global_variable_value = global_variable_value[1:-1]
                        self.test_cfg[mode].append(("global", {global_variable_name: global_variable_value}))
                    else:
                        self.test_cfg[mode].append(("shell", line))