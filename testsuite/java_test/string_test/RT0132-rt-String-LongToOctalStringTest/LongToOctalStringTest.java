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
public class LongToOctalStringTest {
    private static int processResult = 99;
    public static void main(String[] argv) {
        System.out.println(run(argv, System.out));
    }
    public static int run(String[] argv, PrintStream out) {
        int result = 2; /* STATUS_Success*/

        try {
            LongToOctalStringTest_1();
            LongToOctalStringTest_2();
        } catch (Exception e) {
            processResult -= 10;
        }
        if (result == 2 && processResult == 99) {
            result = 0;
        }
        return result;
    }
    // Long.toOctalString(long l)
    public static void LongToOctalStringTest_1() {
        long l_a = 0L;
        long l_b = 111L;
        long l_c = 2147483647L;
        test(l_a);
        test(l_b);
        test(l_c);
    }
    public static void LongToOctalStringTest_2() {
        Integer i_a = 0;
        Integer i_b = 111;
        Integer i_c = 2147483647;
        test2(i_a);
        test2(i_b);
        test2(i_c);
    }
    private static void test(Long lo) {
        System.out.println(Long.toOctalString(lo));
    }
    private static void test2(Integer i) {
        System.out.println(Integer.toOctalString(i));
    }
}
