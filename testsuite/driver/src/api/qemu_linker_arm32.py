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


class QemuLinkerArm32(ShellOperator):

    def __init__(self, lib, model, parse=None, return_value_list=None, redirection=None):
        super().__init__(return_value_list, redirection)
        self.lib = lib
        self.model = model
        if parse is None:
            self.parse = '${APP}'
        else:
            self.parse = parse

    def get_command(self, variables):
        if self.model == "hard":
            self.command = "/usr/bin/clang++-9 -march=armv7-a -mfpu=vfpv4 -mfloat-abi=hard -g3 -O2 -x assembler-with-cpp -target armv7a-linux-gnueabihf -c " + self.parse + ".VtableImpl.s -o " + self.parse + ".VtableImpl.qemu.o;/usr/bin/clang++-9 -g3 -O2 -march=armv7-a -mfpu=vfpv4 -mfloat-abi=hard -target armv7a-linux-gnueabihf -fPIC -shared -o " + self.parse + ".so ${OUT_ROOT}/target/product/maple_arm32/lib/hard/mrt_module_init.o -I${OUT_ROOT}/target/product/maple_arm32/lib/nativehelper " + self.parse + ".VtableImpl.qemu.o -fuse-ld=lld -rdynamic -L${OUT_ROOT}/target/product/maple_arm32/lib/" + self.lib + "/ -lcore-all -lcommon-bridge -Wl\,-z\,notext -Wl\,-T ${OUT_ROOT}/target/product/public/lib/linker/mapleArm32lld.so.lds"
        if self.model == "native_hard":
            self.command = "/usr/bin/clang++-9 -march=armv7-a -mfpu=vfpv4 -mfloat-abi=hard -g3 -O2 -x assembler-with-cpp -target armv7a-linux-gnueabihf -c " + self.parse + ".VtableImpl.s -o " + self.parse + ".VtableImpl.qemu.o;/usr/bin/clang++-9 -march=armv7-a -mfpu=vfpv4 -mfloat-abi=hard -g3 -O2 -Wall -Werror -Wno-unused-command-line-argument -fstack-protector-strong -fPIC -std=c++14 -nostdlibinc -isystem /usr/arm-linux-gnueabihf/include/c++/5 -isystem /usr/arm-linux-gnueabihf/include/c++/5/arm-linux-gnueabihf -isystem /usr/arm-linux-gnueabihf/include/c++/5/backward -isystem /usr/lib/gcc-cross/arm-linux-gnueabihf/5/include -isystem /usr/lib/gcc-cross/arm-linux-gnueabihf/5/include-fixed -isystem /usr/arm-linux-gnueabihf/include -target armv7a-linux-gnueabihf -fPIC -shared -o " + self.parse + ".so ${OUT_ROOT}/target/product/maple_arm32/lib/hard/mrt_module_init.o ${NATIVE_SRC} -I${OUT_ROOT}/target/product/public/lib " + self.parse + ".VtableImpl.qemu.o -fuse-ld=lld -rdynamic -L${OUT_ROOT}/target/product/maple_arm32/lib/" + self.lib + "/ -lcore-all -lcommon-bridge -Wl\,-z\,notext -Wl\,-T ${OUT_ROOT}/target/product/public/lib/linker/mapleArm32lld.so.lds"
        if self.model == "native_softfp":
            self.command = "/usr/bin/clang++-9 -march=armv7-a -mfpu=vfpv4 -mfloat-abi=softfp -g3 -O2 -x assembler-with-cpp -target armv7a-linux-gnueabi -c " + self.parse + ".VtableImpl.s -o " + self.parse + ".VtableImpl.qemu.o;/usr/bin/clang++-9 -march=armv7-a -mfpu=vfpv4 -mfloat-abi=softfp -g3 -O2 -Wall -Werror -Wno-unused-command-line-argument -fstack-protector-strong -fPIC -std=c++14 -nostdlibinc -isystem /usr/arm-linux-gnueabi/include/c++/5 -isystem /usr/arm-linux-gnueabi/include/c++/5/arm-linux-gnueabi -isystem /usr/arm-linux-gnueabi/include/c++/5/backward -isystem /usr/lib/gcc-cross/arm-linux-gnueabi/5/include -isystem /usr/lib/gcc-cross/arm-linux-gnueabi/5/include-fixed -isystem /usr/arm-linux-gnueabi/include -target armv7a-linux-gnueabi -fPIC -shared -o " + self.parse + ".so ${OUT_ROOT}/target/product/maple_arm32/lib/softfp/mrt_module_init.o ${NATIVE_SRC} -I${OUT_ROOT}/target/product/public/lib " + self.parse + ".VtableImpl.qemu.o -fuse-ld=lld -rdynamic -L${OUT_ROOT}/target/product/maple_arm32/lib/" + self.lib + "/ -lcore-all -lcommon-bridge -Wl\,-z\,notext -Wl\,-T ${OUT_ROOT}/target/product/public/lib/linker/mapleArm32lld.so.lds"
        if self.model == "softfp":
            self.command = "/usr/bin/clang++-9 -march=armv7-a -mfpu=vfpv4 -mfloat-abi=softfp -g3 -O2 -x assembler-with-cpp -target armv7a-linux-gnueabi -c " + self.parse + ".VtableImpl.s -o " + self.parse + ".VtableImpl.qemu.o;/usr/bin/clang++-9 -g3 -O2 -march=armv7-a -mfpu=vfpv4 -mfloat-abi=softfp -target armv7a-linux-gnueabi -fPIC -shared -o " + self.parse + ".so ${OUT_ROOT}/target/product/maple_arm32/lib/softfp/mrt_module_init.o -I${OUT_ROOT}/target/product/maple_arm32/lib/nativehelper " + self.parse + ".VtableImpl.qemu.o -fuse-ld=lld -rdynamic -L${OUT_ROOT}/target/product/maple_arm32/lib/" + self.lib + "/ -lcore-all -lcommon-bridge -Wl\,-z\,notext -Wl\,-T ${OUT_ROOT}/target/product/public/lib/linker/mapleArm32lld.so.lds"
        return super().get_final_command(variables)
