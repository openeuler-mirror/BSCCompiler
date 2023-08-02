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
import stat

from case.component import Component
from env_var import EnvVar


class GenerateShellScript(Component):

    def __init__(self, input: dict):
        self.case_name = input["case_name"]
        self.case_path = os.path.join(EnvVar.TEST_SUITE_ROOT, self.case_name)
        self.command_suite = input["command_suite"]

    def execute(self):
        for mode in self.command_suite.keys():
            mode_file_path = os.path.join(self.case_path, mode + "_test.sh")
            with open(mode_file_path, 'w') as f:
                f.write("#!/bin/bash\nset -e\nset -x\n")
                f.write("\n".join(self.command_suite[mode]) + "\n")
            os.chmod(mode_file_path, stat.S_IRWXU)

    def get_output(self):
        return [{
            "name": self.case_name,
            "memory": 0,
            "cpu": 0.7,
            "option": mode,
            "cmd": "cd ${WORKSPACE}/out/host/test/" + self.case_name + ";export OUT_ROOT=${WORKSPACE}/out;bash " + mode + "_test.sh",
            "timeout": 900
        } for mode in self.command_suite.keys()]
