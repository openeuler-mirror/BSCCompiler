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

import sys

from case.case_executor.origin_execute import OriginExecute
from case.case_parser.origin_parse import OriginParse


class OriginRun():

    def __init__(self, input: dict):
        self.input = input
        self.result_suite = {}

    def execute(self):
        sys.stdout.flush()
        origin_parse = OriginParse(self.input)
        origin_parse.execute()
        self.input["config"], self.input["command_suite"] = origin_parse.get_output()
        origin_execute = OriginExecute(self.input)
        origin_execute.execute()
        return origin_execute.get_output()
