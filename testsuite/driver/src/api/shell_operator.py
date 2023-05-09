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

class ShellOperator(object):

    def __init__(self, return_value_list, redirection=None):
        self.command = ""
        if return_value_list is None:
            self.return_value_list = [0]
        else:
            self.return_value_list = return_value_list
        self.redirection = redirection

    def get_redirection(self):
        if self.redirection is not None:
            return " > " + self.redirection + " 2>&1"
        else:
            return ""

    def get_check_command(self):
        if len(self.return_value_list) == 1:
            if 0 in self.return_value_list:
                return ""
            else:
                return " || [ $? -eq " + str(self.return_value_list[0]) + " ]"
        elif len(self.return_value_list) == 0:
            return " || true"
        else:
            return_value_check_str_list = []
            for return_value in self.return_value_list:
                return_value_check_str_list.append("[ ${return_value} -eq " + str(return_value) + " ]")
            return " || (return_value=$? && (" + " || ".join(return_value_check_str_list) + "))"

    def get_final_command(self, variables):
        final_command = self.command
        if variables is not None:
            for variable in variables.keys():
                if "${" + variable + "}" in final_command:
                    final_command = final_command.replace("${" + variable + "}", variables[variable])
        final_command += self.get_redirection()
        final_command += self.get_check_command()
        return final_command