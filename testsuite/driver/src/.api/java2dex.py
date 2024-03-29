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


class Java2dex(ShellOperator):

    def __init__(self, jar_file, outfile, infile, return_value_list=None, redirection=None):
        super().__init__(return_value_list, redirection)
        self.jar_file = jar_file
        self.outfile = outfile
        self.infile = infile

    def get_command(self, variables):
        self.command = "bash ${MAPLE_ROOT}/tools/bin/java2dex  -o " + self.outfile + " -p " + ":".join(self.jar_file) + " -i " + ":".join(self.infile)
        return super().get_final_command(variables)
