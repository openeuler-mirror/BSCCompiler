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
public class StringNullPointExceptionTest {
    private static int processResult = 99;
    public static void main(String[] argv) {
        System.out.println(run(argv, System.out));
    }
    public static int run(String[] argv, PrintStream out) {
        int result = 2;  /* STATUS_Success*/

        try {
            StringNullPointExceptionTest_1();
        } catch (Exception e) {
            processResult -= 10;
        }
        if (result == 2 && processResult == 83) {
            result = 0;
        }
        return result;
    }
    public static void StringNullPointExceptionTest_1() {
        String str2_1 = "abc123";
        String str2_5 = null;
        test0(str2_1);
        test1(str2_5);
        test2(str2_5);
        test3(str2_5);
        test4(str2_5);
        test5(str2_5);
        test6(str2_5);
        test7(str2_5);
    }
    private static void test0(String str) {
        try {
            char t0 = str.charAt(-2);
        } catch (StringIndexOutOfBoundsException e) {
            processResult -= 2;
        }
    }
    private static void test1(String str) {
        try {
            char t1 = str.charAt(0);
        } catch (NullPointerException e) {
            processResult -= 2;
        }
    }
    private static void test2(String str) {
        char[] t2 = null;
        try {
            t2 = str.toCharArray();
        } catch (NullPointerException e) {
            processResult -= 2;
        }
    }
    private static void test3(String str) {
        String t3 = null;
        try {
            t3 = str.substring(1);
        } catch (NullPointerException e) {
            processResult -= 2;
        }
    }
    private static void test4(String str) {
        try {
            int t4 = str.compareTo("aaa");
        } catch (NullPointerException e) {
            processResult -= 2;
        }
    }
    private static void test5(String str) {
        try {
            String t5 = str.intern();
        } catch (NullPointerException e) {
            processResult -= 2;
        }
    }
    private static void test6(String str) {
        try {
            String t6 = str.replace("a", "b");
        } catch (NullPointerException e) {
            processResult -= 2;
        }
    }
    private static void test7(String str) {
        try {
            String t7 = str.concat("aaa");
        } catch (NullPointerException e) {
            processResult -= 2;
        }
    }
}