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

from case.case_executor.clean import Clean
from case.case_executor.generate_shell_script import GenerateShellScript
from case.case_parser.origin_parse import OriginParse


class CICDRun():

    def __init__(self, input: dict):
        self.input = input

    def execute(self):
        clean = Clean(self.input)
        clean.execute()
        origin_parse = OriginParse(self.input)
        origin_parse.execute()
        self.input["config"], self.input["command_suite"] = origin_parse.get_output()
        generate_shell_script = GenerateShellScript(self.input)
        generate_shell_script.execute()
        return generate_shell_script.get_output()
