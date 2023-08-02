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
from basic_tools.file import get_sub_files


class Clean(Component):

    def __init__(self, input: dict):
        self.case_name = input["case_name"]
        self.case_path = os.path.join(EnvVar.TEST_SUITE_ROOT, self.case_name)
        self.detail = input["detail"]

    def rm_tmp_files(self):
        all_cur_files = [file for file in os.listdir(self.case_path) if not file.endswith('_tmp@')]
        if '.raw_file_list.txt' not in all_cur_files:
            return
        with open(os.path.join(self.case_path,'.raw_file_list.txt'), 'r') as f:
            raw_cur_files, raw_sub_files = f.read().split('\n-----\n')
        tmp_cur_files = list(set(all_cur_files) - set(raw_cur_files.split('\n')))
        if tmp_cur_files:
            subprocess.run('rm -rf %s'%(' '.join([os.path.join(self.case_path,f) for f in tmp_cur_files])), shell=True, check=True)
        all_sub_files = [file for file in get_sub_files(self.case_path) if '_tmp@' not in file]
        tmp_sub_files = list(set(all_sub_files) - set(raw_sub_files.split('\n')))
        if tmp_sub_files:
            subprocess.run('rm -rf %s'%(' '.join([os.path.join(self.case_path,f) for f in tmp_sub_files])), shell=True, check=True)
        if self.detail and (tmp_cur_files or tmp_sub_files):
            print("\033[1;32m [[ CMD : rm -rf %s ]]\033[0m"%('  '.join(tmp_cur_files + tmp_sub_files)))

    def rm_tmp_folders(self):
        del_file_list = [os.path.join(self.case_path,f) for f in os.listdir(self.case_path) if f.endswith("_tmp@")]
        subprocess.run('rm -rf ' + " ".join(del_file_list), shell=True, check=True)
        if self.detail and del_file_list != []:
            print("\033[1;32m [[ CMD : rm -rf %s ]]\033[0m"%('  '.join(del_file_list)))

    def execute(self):
        self.rm_tmp_files()
        self.rm_tmp_folders()

    def get_output(self):
        pass
