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


class Clang2mpl(ShellOperator):

    def __init__(self, infile, return_value_list=None, redirection=None):
        super().__init__(return_value_list, redirection)
        self.infile = infile

    def get_command(self, variables):
        linaroDir = "${MAPLE_ROOT}/tools/gcc-linaro-7.5.0"
        isystemFlags = "-isystem " + linaroDir + "/aarch64-linux-gnu/libc/usr/include -isystem " + linaroDir + "/lib/gcc/aarch64-linux-gnu/7.5.0/include"
        clangFlags = "--target=aarch64-linux-elf -Wno-return-type -U__SIZEOF_INT128__ " + isystemFlags
        self.command = "${MAPLE_EXECUTE_BIN}/clang2mpl --ascii " + self.infile + " -- " + clangFlags
        return super().get_final_command(variables)
