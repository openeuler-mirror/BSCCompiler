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


class QemuLinkerArm64(ShellOperator):

    def __init__(self, lib, parse=None, return_value_list=None, redirection=None):
        super().__init__(return_value_list, redirection)
        self.lib = lib
        if parse is None:
            self.parse = '${APP}'
        else:
            self.parse = parse

    def get_command(self, variables):
        self.command = "/usr/bin/clang++-9 -march=armv8-a -g3 -O2 -x assembler-with-cpp -target aarch64-linux-gnu -c " + self.parse + ".VtableImpl.s -o " + self.parse + ".VtableImpl.qemu.o;/usr/bin/clang++-9 -g3 -O2 -march=armv8-a -target aarch64-linux-gnu -fPIC -shared -o " + self.parse + ".so ${OUT_ROOT}/target/product/maple_arm64/lib/mrt_module_init.o -I${OUT_ROOT}/target/product/maple_arm64/lib/nativehelper " + self.parse + ".VtableImpl.qemu.o -fuse-ld=lld -rdynamic -L${OUT_ROOT}/target/product/maple_arm64/lib/" + self.lib + "/ -lcore-all -lcommon-bridge -Wl,-z,notext -Wl,-T ${OUT_ROOT}/target/product/public/lib/linker/maplelld.so.lds"
        return super().get_final_command(variables)
