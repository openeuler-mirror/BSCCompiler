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

from api.shell_operator import ShellOperator
import os

class CLinkerAll(ShellOperator):

    def __init__(self, front_option, outfile, back_option, mid_opt="",return_value_list=None, redirection=None):
        super().__init__(return_value_list, redirection)
        self.infile = " ".join([f.replace(".c", ".s") for f in os.listdir() if f.endswith(".c")])
        print(self.infile)
        self.front_option = front_option
        self.outfile = outfile
        self.back_option = back_option
        self.mid_opt = mid_opt

    def get_command(self, variables):
        self.command = "${MAPLE_ROOT}/tools/bin/aarch64-linux-gnu-gcc " + self.front_option + " -o " + self.outfile + " " + self.mid_opt + " " +  self.infile + " " + self.back_option
        return super().get_final_command(variables)
