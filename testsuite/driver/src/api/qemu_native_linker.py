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


class QemuNativeLinker(ShellOperator):
    def __init__(self, lib, return_value_list=None, redirection=None):
        super().__init__(return_value_list, redirection)
        self.lib = lib

    def get_command(self, variables):
        self.command = "/usr/bin/clang++-9 -march=armv8-a -g3 -O2 -x assembler-with-cpp -target aarch64-linux-gnu -c ${APP}.VtableImpl.s -o ${APP}.VtableImpl.qemu.o;/usr/bin/clang++-9 -g3 -O2 -Wall -Werror -Wno-unused-command-line-argument -fstack-protector-strong -fPIC -std=c++14 -nostdlibinc -march=armv8-a -isystem /usr/lib/gcc-cross/aarch64-linux-gnu/5/../../../../aarch64-linux-gnu/include/c++/5 -isystem /usr/lib/gcc-cross/aarch64-linux-gnu/5/../../../../aarch64-linux-gnu/include/c++/5/aarch64-linux-gnu -isystem /usr/lib/gcc-cross/aarch64-linux-gnu/5/../../../../aarch64-linux-gnu/include/c++/5/backward -isystem /usr/lib/gcc-cross/aarch64-linux-gnu/5/include -isystem /usr/lib/gcc-cross/aarch64-linux-gnu/5/include-fixed -isystem /usr/aarch64-linux-gnu/include -target aarch64-linux-gnu -fPIC -shared -o ${APP}.so ${OUT_ROOT}/target/product/maple_arm64/lib/mrt_module_init.o ${NATIVE_SRC} -I${OUT_ROOT}/target/product/public/lib ${APP}.VtableImpl.qemu.o -fuse-ld=lld -rdynamic -L${OUT_ROOT}/target/product/maple_arm64/lib/" + self.lib + "/ -lcore-all -lcommon-bridge -Wl\,-z\,notext -Wl\,-T ${OUT_ROOT}/target/product/public/lib/linker/maplelld.so.lds"
        return super().get_final_command(variables)
