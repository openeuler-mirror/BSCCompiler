#
# Copyright (c) [2023] Huawei Technologies Co.,Ltd.All rights reserved.
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
 
 
class MapleDriver(ShellOperator):
 
    def __init__(self, maple, infiles, outfile, option, return_value_list=None, redirection=None, include_path="", extra_opt=""):
        super().__init__(return_value_list, redirection)
        self.maple = maple
        self.include_path = include_path
        self.infiles = infiles
        self.outfile = outfile
        self.option = option
        self.extra_opt = extra_opt
 
    def get_command(self, variables):
        include_path_str = " ".join(["-isystem " + path for path in self.include_path])
        self.command = self.maple + " "
        self.command += " ".join(self.infiles)
        self.command += " -o " + self.outfile
        self.command += " " + include_path_str
        self.command += " " + self.option + " " + self.extra_opt
        return super().get_final_command(variables)