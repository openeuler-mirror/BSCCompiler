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
import subprocess

from case.component import Component
from env_var import EnvVar


class SaveTmp(Component):

    def __init__(self, input: dict):
        self.case_name = input["case_name"]
        self.case_path = os.path.join(EnvVar.TEST_SUITE_ROOT, self.case_name)

    def execute(self):
        all_cur_files = [file for file in os.listdir(self.case_path) if not file.endswith('_tmp@')]
        if '.raw_file_list.txt' not in all_cur_files:
            return
        else:
            with open(os.path.join(self.case_path, ".raw_file_list.txt")) as f:
                raw_files = f.read().split('-----')[0].split('\n')
        all_tmp_files = set(all_cur_files) - set(raw_files)
        tmp_folder = [int(file.split('_tmp@')[0]) for file in os.listdir(self.case_path) if file.endswith('_tmp@')]
        cur_max_folder = 0
        if tmp_folder != []:
            cur_max_folder = max(tmp_folder)
        os.mkdir(str(cur_max_folder + 1) + '_tmp@')
        for file in all_tmp_files:
            subprocess.run('mv %s %d_tmp@' % (file, cur_max_folder + 1), shell=True, check=True)

    def get_output(self):
        pass
