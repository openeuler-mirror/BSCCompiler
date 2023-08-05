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


class MapleCg(ShellOperator):

    def __init__(self, maple, run, option, global_option, infile, return_value_list=None, redirection=None, outfile=""):
        super().__init__(return_value_list, redirection)
        self.maple = maple
        self.run = run
        self.option_dict = option
        self.global_option = global_option
        self.infile = infile
        self.outfile = outfile

    def get_command(self, variables):
        self.command = self.maple + " --run=" + ":".join(self.run) + " "
        option = []
        for cmd in self.run:
            option.append(self.option_dict[cmd])
        self.command += "--option=\"" + ":".join(option) + "\" "
        self.command += self.global_option + " "
        self.command += "--infile " + self.infile
        if self.outfile:
            self.command += " -o " + self.outfile + " "
        return super().get_final_command(variables)
