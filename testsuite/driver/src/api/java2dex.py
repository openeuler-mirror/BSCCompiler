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

    def __init__(self, jar_file, outfile, infile, usesimplejava=False, return_value_list=None, redirection=None):
        super().__init__(return_value_list, redirection)
        self.jar_file = jar_file
        self.outfile = outfile
        self.infile = infile
        self.usesimplejava = usesimplejava

    def java2dex_i_output(self, variables):
        if 'EXTRA_JAVA_FILE' in variables.keys():
            variables['EXTRA_JAVA_FILE'] = variables['EXTRA_JAVA_FILE'].replace('[','').replace(']','').replace(',',':')
            return ':'.join(self.infile)
        return self.infile[0]

    def get_command(self, variables):
        if not self.usesimplejava:
            self.command = "${OUT_ROOT}/target/product/public/bin/java2dex  -o " + self.outfile + " -p " + ":".join(self.jar_file) + " -i " + self.java2dex_i_output(variables)
        else:
            self.command = "${OUT_ROOT}/target/product/public/bin/java2dex  -o " + self.outfile + " -p " + ":".join(self.jar_file) + " -i " + self.java2dex_i_output(variables) + " -s useSimpleJava"
        # print(super().get_final_command(variables))
        return super().get_final_command(variables)
