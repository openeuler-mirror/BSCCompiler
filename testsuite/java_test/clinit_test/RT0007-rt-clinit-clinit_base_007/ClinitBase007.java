/*
 * Copyright (c) [2021] Huawei Technologies Co.,Ltd.All rights reserved.
 *
 * OpenArkCompiler is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 *
 *     http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR
 * FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
*/


import java.io.PrintStream;
public class ClinitBase007 {
    static int res = 2;
    public static void main(String[] argv) {
        System.out.println(run(argv, System.out));
    }
    public static int run(String[] argv, PrintStream out) {
        ClinitBaseB.testAsserts();
        return res;
    }
}
class ClinitBaseA {
    static {
        ClinitBaseB.testAsserts();
    }
}
class ClinitBaseB extends ClinitBaseA {
    static void testAsserts() {
        boolean enable = false;
        assert enable = true;
        if (!enable) {
            ClinitBase007.res -= 1;
        } else {
            System.out.println("Asserts enable");
        }
    }
}
