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

from case.case_executor.clean import Clean
from case.case_executor.note_src import NoteSrc
from case.case_executor.origin_execute import OriginExecute
from case.case_parser.origin_parse import OriginParse


class OriginRun():

    def __init__(self, input: dict):
        self.input = input
        self.result_suite = {}

    def execute(self):
        sys.stdout.flush()
        clean = Clean(self.input)
        clean.execute()
        note_src = NoteSrc(self.input)
        note_src.execute()
        origin_parse = OriginParse(self.input)
        origin_parse.execute()
        self.input["command_suite"] = origin_parse.get_output()
        origin_execute = OriginExecute(self.input)
        origin_execute.execute()
        return origin_execute.get_output()
