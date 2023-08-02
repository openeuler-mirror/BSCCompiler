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

from api import *

CORT = {
    "compile": [
      Shell("/usr/bin/clang-9 -O2 -g3 -c -fPIC -march=armv8-a -target aarch64-linux-gnu -I${MAPLE_ROOT}/mrt/coroutine/api/ ${APP}.c;/usr/bin/clang++-9 -s -fuse-ld=lld -O2 -g -Wall -fstack-protector-strong -fPIC -Werror -fPIE -rdynamic -pie -W -Wno-macro-redefined -Wno-inconsistent-missing-override -Wno-deprecated -Wno-unused-command-line-argument -isystem /usr/lib/gcc-cross/aarch64-linux-gnu/5/../../../../aarch64-linux-gnu/include/c++/5 -isystem /usr/lib/gcc-cross/aarch64-linux-gnu/5/../../../../aarch64-linux-gnu/include/c++/5/aarch64-linux-gnu -isystem /usr/lib/gcc-cross/aarch64-linux-gnu/5/../../../../aarch64-linux-gnu/include/c++/5/backward -isystem /usr/lib/gcc-cross/aarch64-linux-gnu/5/include -isystem /usr/lib/gcc-cross/aarch64-linux-gnu/5/include-fixed -isystem /usr/aarch64-linux-gnu/include -target aarch64-linux-gnu -Wl,-z,relro -Wl,-z,now -Wl,-z,noexecstack -fPIE -o test.out -Wl,--start-group test.o  -L${OUT_ROOT}/target/product/maple_arm64-clang-release/lib/host-x86_64-O2/ -lcoroutine -ldl -lhuawei_secure_c -Wl,--end-group")
    ],
    "run": [
       Shell(
            "/usr/bin/qemu-aarch64 -L /usr/aarch64-linux-gnu -E LD_LIBRARY_PATH=${OUT_ROOT}/target/product/maple_arm64/lib/host-x86_64-O2:./ ./${APP}.out > output.log 2>&1"
        ),
        CheckFileEqual(
            file1="output.log",
            file2="expected.txt"
        )
    ]
}
