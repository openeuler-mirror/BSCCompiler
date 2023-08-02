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


class Gendeps(ShellOperator):

    def __init__(self, gendeps, apk, emui, infile, return_value_list=None, redirection=None, extra_option=""):
        super().__init__(return_value_list, redirection)
        self.gendeps = gendeps
        self.apk = apk
        self.emui = emui
        self.infile = infile
        self.extra_option = extra_option

    def get_command(self, variables):
        self.command = self.gendeps + " -emui-map ${OUT_ROOT}/target/product/public/lib/dex_module_map_sdk.list -classpath libmaplecore-all -verbose -out-module-name "
        self.command += "-apk " + self.apk + " "
        self.command += "-emui " + self.emui + " "
        self.command += self.extra_option + " "
        self.command +="-in-dex " + self.infile
        return super().get_final_command(variables)