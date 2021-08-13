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


class CheckRegContain(ShellOperator):

    def __init__(self, reg, file, return_value_list=None):
        super().__init__(return_value_list)
        self.reg = reg
        self.file = file

    def get_command(self, variables):
        self.command = "python3 ${OUT_ROOT}/script/check.py --check=contain --str=\"" + self.reg + "\" --result=" + self.file
        return super().get_final_command(variables)
