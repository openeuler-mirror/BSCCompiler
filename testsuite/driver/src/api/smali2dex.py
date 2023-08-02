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


class Smali2dex(ShellOperator):

    def __init__(self, file, return_value_list=None, redirection=None):
        super().__init__(return_value_list, redirection)
        self.file = file

    def get_command(self, variables):
        self.command = "java -jar ${OUT_ROOT}/target/product/public/bin/smali-2.2.4.jar ass  " + " ".join(self.file) + "; mv out.dex ${APP}.dex"
        self.command += super().get_redirection()
        return super().get_final_command(variables)
