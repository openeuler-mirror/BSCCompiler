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


class Mplsh(ShellOperator):

    def __init__(self, mplsh, xbootclasspath, infile, garbage_collection_kind, return_value_list=None, env=None, qemu=None, qemu_libc=None, qemu_ld_lib=None, main="${APP}", args=None, redirection=None):
        super().__init__(return_value_list, redirection)
        self.env = env
        self.qemu = qemu
        self.qemu_libc = qemu_libc
        self.qemu_ld_lib = qemu_ld_lib
        self.mplsh = mplsh
        self.garbage_collection_kind = garbage_collection_kind
        self.xbootclasspath = xbootclasspath
        self.infile = infile
        self.main = main
        self.args = args

    def get_command(self, variables):
        self.command = ""
        if self.env is not None:
            for env_var in self.env.keys():
                self.command += env_var + "=" + self.env[env_var] + " "
        if self.qemu is not None:
            self.command += self.qemu + " -L " + self.qemu_libc + " -E LD_LIBRARY_PATH=" + ":".join(self.qemu_ld_lib) + " "
        self.command += self.mplsh + " "
        if self.garbage_collection_kind == "GC":
            self.command += "-Xgconly "
        if self.args is not None:
            self.command +="-Xbootclasspath:" + self.xbootclasspath + " -cp " + self.infile + " " + self.main + " " + self.args
        else:
            self.command += "-Xbootclasspath:" + self.xbootclasspath + " -cp " + self.infile + " " + self.main
        return super().get_final_command(variables)
