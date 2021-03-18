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

import os

from api.shell_operator import ShellOperator


class Clangfe(ShellOperator):

    aarch64_linux_gnu_version = os.listdir("/usr/lib/gcc-cross/aarch64-linux-gnu")[0]

    def __init__(self, infile, return_value_list=[0], redirection=None):
        super().__init__(return_value_list, redirection)
        self.infile = infile

    def get_command(self, variables):
        self.command = "${TOOL_BIN_PATH}/clangfe -cc1 -emit-llvm -triple aarch64-linux-gnu -D__clang__ -D__BLOCKS__ -isystem /usr/aarch64-linux-gnu/include -isystem /usr/lib/gcc-cross/aarch64-linux-gnu/" + str(Clangfe.aarch64_linux_gnu_version) + "/include " + self.infile
        return super().get_final_command(variables)
