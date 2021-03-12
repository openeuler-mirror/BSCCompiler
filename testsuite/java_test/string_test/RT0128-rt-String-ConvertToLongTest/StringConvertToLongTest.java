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
public class StringConvertToLongTest {
    private static int processResult = 99;
    public static void main(String[] argv) {
        System.out.println(run(argv, System.out));
    }
    public static int run(String[] argv, PrintStream out) {
        int result = 2; /* STATUS_Success*/

        try {
            StringConvertToLongTest_1();
        } catch (Exception e) {
            processResult -= 10;
        }
        if (result == 2 && processResult == 99) {
            result = 0;
        }
        return result;
    }
    // Test public static long parseLong(String s, int radix).
    public static void StringConvertToLongTest_1() {
        long l_1 = Long.parseLong("1234", 10);
        // Test parses the string with specified radix
        System.out.println(l_1);
        long l_a = Long.parseLong("0", 10);
        System.out.println(l_a);
        long l_b = Long.parseLong("111", 10);
        System.out.println(l_b);
        long l_c = Long.parseLong("-0", 10);
        System.out.println(l_c);
        long l_d = Long.parseLong("-BB", 16);
        System.out.println(l_d);
        long l_e = Long.parseLong("1010110", 2);
        System.out.println(l_e);
        long l_f = Long.parseLong("2147483647", 10);
        System.out.println(l_f);
        long l_g = Long.parseLong("-2147483648", 10);
        System.out.println(l_g);
        long l_h = Long.parseLong("ADMIN", 27);
        System.out.println(l_h);
    }
}
