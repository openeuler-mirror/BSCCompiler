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

from basic_tools.file import get_sub_files
from env_var import EnvVar
from case.component import Component


class NoteSrc(Component):
    def __init__(self, input: dict):
        self.case_name = input["case_name"]
        self.case_path = os.path.join(EnvVar.TEST_SUITE_ROOT, self.case_name)

    def execute(self):
        all_cur_files = os.listdir(self.case_path)
        all_sub_files = [file for file in get_sub_files(self.case_path) if '_tmp@' not in file]
        if '.raw_file_list.txt' in all_cur_files:
            return
        with open(os.path.join(self.case_path, '.raw_file_list.txt'), 'w') as f:
            f.writelines('\n'.join(all_cur_files))
            f.writelines('\n-----\n')
            f.writelines('\n'.join(all_sub_files))

    def get_output(self):
        pass
