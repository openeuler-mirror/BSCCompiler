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


class C2ast(ShellOperator):

    def __init__(self, clang, infile, include_path, outfile, option="", return_value_list=None, redirection=None, extra_opt=""):
        super().__init__(return_value_list, redirection)
        self.clang = clang
        self.infile = infile
        self.option = option
        self.include_path = include_path
        self.outfile = outfile
        self.extra_opt = extra_opt

    def get_command(self, variables):
        include_path_str = " ".join(["-isystem " + path for path in self.include_path])
        self.command = self.clang + " -emit-ast " + self.option + " " + include_path_str + " -o " + self.outfile + " " + self.extra_opt + " " + self.infile
        return super().get_final_command(variables)
