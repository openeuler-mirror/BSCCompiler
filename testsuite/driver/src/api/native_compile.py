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


class NativeCompile(ShellOperator):

    def __init__(self, mpldep, infile, model, return_value_list=None, redirection=None):
        super().__init__(return_value_list, redirection)
        self.mpldep = mpldep
        self.infile = infile
        self.model =model

    def get_command(self, variables):
        if self.model == "arm32_hard":
            self.command = "/usr/bin/clang++-9 -O2 -g3 -c -fPIC -march=armv7-a -mfpu=vfpv4 -mfloat-abi=hard -target armv7a-linux-gnueabihf -c "
            for file in self.mpldep:
                self.command += "-I" + file + " "
            self.command += " -isystem /usr/arm-linux-gnueabihf/include/c++/5 -isystem /usr/arm-linux-gnueabihf/include/c++/5/arm-linux-gnueabihf -isystem /usr/arm-linux-gnueabihf/include/c++/5/backward -isystem /usr/lib/gcc-cross/arm-linux-gnueabihf/5/include -isystem /usr/lib/gcc-cross/arm-linux-gnueabihf/5/include-fixed -isystem /usr/arm-linux-gnueabihf/include"
            self.command += " " + self.infile + ".cpp; "
            self.command += " /usr/bin/clang++-9 "
            self.command += " " + self.infile + ".o"
            self.command += " -O2 -Wall -Werror -Wno-unused-command-line-argument -fstack-protector-strong -std=c++14 -nostdlibinc -march=armv7-a -mfpu=vfpv4 -mfloat-abi=hard -target armv7a-linux-gnueabihf -fPIC -shared -fuse-ld=lld -rdynamic"
            self.command += " -L" + self.mpldep[0] + " -lcore-all -lcommon-bridge"
            self.command += " -Wl,-z,notext  -o"
            self.command += " lib" + self.infile + ".so;"
        if self.model == "arm32_softfp":
            self.command = "/usr/bin/clang++-9 -O2 -g3 -c -fPIC -march=armv7-a -mfpu=vfpv4 -mfloat-abi=softfp -target armv7a-linux-gnueabi -c "
            for file in self.mpldep:
                self.command += "-I" + file + " "
            self.command += " -isystem /usr/arm-linux-gnueabi/include/c++/5 -isystem /usr/arm-linux-gnueabi/include/c++/5/arm-linux-gnueabi -isystem /usr/arm-linux-gnueabi/include/c++/5/backward -isystem /usr/lib/gcc-cross/arm-linux-gnueabi/5/include -isystem /usr/lib/gcc-cross/arm-linux-gnueabi/5/include-fixed -isystem /usr/arm-linux-gnueabi/include"
            self.command += " " + self.infile + ".cpp; "
            self.command += " /usr/bin/clang++-9 "
            self.command += " " + self.infile + ".o"
            self.command += " -O2 -Wall -Werror -Wno-unused-command-line-argument -fstack-protector-strong -std=c++14 -nostdlibinc -march=armv7-a -mfpu=vfpv4 -mfloat-abi=softfp -target armv7a-linux-gnueabi -fPIC -shared -fuse-ld=lld -rdynamic"
            self.command += " -L" + self.mpldep[0] + " -lcore-all -lcommon-bridge"
            self.command += " -Wl,-z,notext  -o"
            self.command += " lib" + self.infile + ".so;"
        if self.model == "arm64":
            self.command = " /usr/bin/clang++-9 -O2 -g3 -c -fPIC -march=armv8-a -target aarch64-linux-gnu "
            for file in self.mpldep:
                self.command += "-I" + file + " "
            self.command += " -isystem /usr/aarch64-linux-gnu/include/c++/5 -isystem /usr/aarch64-linux-gnu/include/c++/5/aarch64-linux-gnu -isystem /usr/aarch64-linux-gnu/include/c++/5/backward -isystem /usr/lib/gcc-cross/aarch64-linux-gnu/5/include -isystem /usr/lib/gcc-cross/aarch64-linux-gnu/5/include-fixed -isystem /usr/aarch64-linux-gnu/include"
            self.command += " " + self.infile + ".cpp; "
            self.command += " /usr/bin/clang++-9 "
            self.command += " " + self.infile + ".o"
            self.command += " -O2 -Wall -Werror -Wno-unused-command-line-argument -fstack-protector-strong -std=c++14 -nostdlibinc -march=armv8-a -target aarch64-linux-gnu -fPIC -shared -fuse-ld=lld -rdynamic"
            self.command += " -L" + self.mpldep[0] + " -lcore-all -lcommon-bridge"
            self.command += " -Wl,-z,notext  -o"
            self.command += " lib" + self.infile + ".so;"
        if self.model == "arm64_ifile":
            self.command = " /usr/bin/clang++-9 -O2 -g3 -c -fPIC -march=armv8-a -target aarch64-linux-gnu "
            for file in self.mpldep:
                self.command += "-I" + file + " "
            self.command += " -isystem /usr/aarch64-linux-gnu/include/c++/5 -isystem /usr/aarch64-linux-gnu/include/c++/5/aarch64-linux-gnu -isystem /usr/aarch64-linux-gnu/include/c++/5/backward -isystem /usr/lib/gcc-cross/aarch64-linux-gnu/5/include -isystem /usr/lib/gcc-cross/aarch64-linux-gnu/5/include-fixed -isystem /usr/aarch64-linux-gnu/include"
            self.command += " " + self.infile + ".cpp; "
            self.command += " /usr/bin/clang++-9 "
            self.command += " " + self.infile + ".o"
            self.command += " -O2 -Wall -Werror -Wno-unused-command-line-argument -fstack-protector-strong -std=c++14 -nostdlibinc -march=armv8-a -target aarch64-linux-gnu -fPIC -shared -fuse-ld=lld -rdynamic"
            self.command += " -L" + self.mpldep[0] + " -lmrt -lcommon-bridge"
            self.command += " -Wl,-z,notext  -o"
            self.command += " lib" + self.infile + ".so;"
        return super().get_final_command(variables)
