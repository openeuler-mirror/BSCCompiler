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


class Irbuild(ShellOperator):

    def __init__(self, irbuild, infile, return_value_list=[0], redirection=None):
        super().__init__(return_value_list, redirection)
        self.irbuild = irbuild
        self.infile = infile

    def get_command(self, variables):
        self.command = self.irbuild + " " + self.infile
        return super().get_final_command(variables)