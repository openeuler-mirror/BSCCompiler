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
public class BasicToStringTest {
    private static int processResult = 99;
    public static void main(String[] argv) {
        System.out.println(run(argv, System.out));
    }
    public static int run(String[] argv, PrintStream out) {
        int result = 2; /* STATUS_Success*/

        try {
            BasicToStringTest_1();
            BasicToStringTest_2();
            BasicToStringTest_3();
            BasicToStringTest_4();
        } catch (Exception e) {
            processResult -= 10;
        }
        if (result == 2 && processResult == 99) {
            result = 0;
        }
        return result;
    }
    // Long.toString(long l, int p)
    public static void BasicToStringTest_1() {
        long l_a = 0;
        long l_b = 111;
        long l_c = 2147483647;
        test1(l_a);
        test1(l_b);
        test1(l_c);
    }
    public static void BasicToStringTest_2() {
        Integer i_a = 0;
        Integer i_b = 111;
        Integer i_c = 2147483647;
        test2(i_a);
        test2(i_b);
        test2(i_c);
    }
    public static void BasicToStringTest_3() {
        float f_a = 0f;
        float f_b = 11.1f;
        float f_c = 2147483.647f;
        test3(f_a);
        test3(f_b);
        test3(f_c);
    }
    public static void BasicToStringTest_4() {
        double d_a = 0;
        double d_b = 1.11d;
        double d_c = 21474836.47;
        test4(d_a);
        test4(d_b);
        test4(d_c);
    }
    private static void test1(Long lo) {
        System.out.println(Long.toString(lo));
    }
    private static void test2(Integer i) {
        System.out.println(Integer.toString(i));
    }
    private static void test3(float f) {
        System.out.println(Float.toString(f));
    }
    private static void test4(double f) {
        System.out.println(Double.toString(f));
    }
}
