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


class QemuRun(ShellOperator):

    def __init__(self, qemu_libc, infile, return_value_list=None, redirection=None):
        super().__init__(return_value_list, redirection)
        self.qemu_libc = qemu_libc
        self.infile = infile

    def get_command(self, variables):
        self.command = "${TOOL_BIN_PATH}/qemu-aarch64 " + " ".join(["-L " + lib_path for lib_path in self.qemu_libc]) + " " + self.infile
        return super().get_final_command(variables)
