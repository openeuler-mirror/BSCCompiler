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


class GenBin(ShellOperator):

    def __init__(self, infile, outfile, return_value_list=None, redirection=None):
        super().__init__(return_value_list, redirection)
        self.infile = infile
        self.outfile = outfile

    def get_command(self, variables):
        self.command = "${OUT_ROOT}/tools/bin/aarch64-linux-gnu-gcc -O2 -static -L../../lib/c -std=c89 -o " + self.outfile + " " + self.infile + " -lst -lm"
        return super().get_final_command(variables)
