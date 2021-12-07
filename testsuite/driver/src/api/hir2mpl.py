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


class Hir2mpl(ShellOperator):

    def __init__(self, hir2mpl, infile, outfile, option="-enable-variable-array", return_value_list=None, redirection=None):
        super().__init__(return_value_list, redirection)
        self.hir2mpl = hir2mpl
        self.infile = infile
        self.outfile = outfile
        self.option = option

    def get_command(self, variables):
        self.command = self.hir2mpl + " " + self.option + " " + self.infile + " -o " + self.outfile
        return super().get_final_command(variables)
