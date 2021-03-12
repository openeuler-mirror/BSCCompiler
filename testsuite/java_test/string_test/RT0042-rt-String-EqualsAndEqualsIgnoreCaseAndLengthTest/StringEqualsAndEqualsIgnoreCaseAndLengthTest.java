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
public class StringEqualsAndEqualsIgnoreCaseAndLengthTest {
    private static int processResult = 99;
    public static void main(String[] argv) {
        System.out.println(run(argv, System.out));
    }
    public static int run(String[] argv,  PrintStream out) {
        int result = 3; /* STATUS_Failed*/

        result = StringEqualsAndEqualsIgnoreCaseAndLengthTest_1();
        if (result == 2 && processResult == 87) {
            result = 0;
        }
        return result;
    }
    public static int StringEqualsAndEqualsIgnoreCaseAndLengthTest_1() {
        test1();
        test2();
        test3();
        test4();
        test5();
        test6();
        test7();
        test8();
        test9();
        test10();
        test11();
        test12();
        return 2;
    }
    public static int test_equals(String str1, String str2) {
        if (str1.equals(str2)) {
            return 1; // right result
        }
        return 30; // wrong result
    }
    public static int test_not_equals(String str1, String str2) {
        if (!str1.equals(str2)) {
            return 1; // right result
        }
        return 30; // wrong result
    }
    public static int test_equalsIgnoreCase(String str1, String str2) {
        if (str1.equalsIgnoreCase(str2)) {
            return 1; // right result
        }
        return 30; // wrong result
    }
    public static int test_length(String str1, int a) {
        if (str1.length() == a) {
            return 1; // right result
        }
        return 30; // wrong result
    }
    private static void test1() {
        String str1_1 = "{";
        String str1_2 = "{";
        if (test_equals(str1_1, str1_2) == 1 && test_length(str1_1, 1) == 1) {
            processResult--;
        } else {
            processResult -= 10;
        }
    }
    private static void test2() {
        String str2_1 = "{12ab";
        String str2_2 = "{12ab";
        if (test_equals(str2_1, str2_2) == 1 && test_length(str2_1, 5) == 1) {
            processResult--;
        } else {
            processResult -= 10;
        }
    }
    private static void test3() {
        String str2_1 = "{12ab";
        String str2_3 = "{12AB";
        if (test_equalsIgnoreCase(str2_1, str2_3) == 1 && test_not_equals(str2_1, str2_3) == 1) {
            processResult--;
        } else {
            processResult -= 10;
        }
    }
    private static void test4() {
        String str3_1 = " abc ";
        String str3_3 = " ";
        if (test_not_equals(str3_1, str3_3) == 1 && test_length(str3_1, 5) == 1) {
            processResult--;
        } else {
            processResult -= 10;
        }
    }
    private static void test5() {
        String str3_2 = "null";
        String str3_3 = " ";
        if (test_not_equals(str3_2, str3_3) == 1) {
            processResult--;
        } else {
            processResult -= 10;
        }
    }
    private static void test6() {
        String str3_2 = "null";
        String str3_4 = "NULL";
        if (test_not_equals(str3_2, str3_4) == 1 && test_equalsIgnoreCase(str3_2, str3_4) == 1) {
            processResult--;
        } else {
            processResult -= 10;
        }
    }
    private static void test7() {
        String str3_1 = " abc ";
        String str3_5 = " abc";
        if (test_not_equals(str3_1, str3_5) == 1) {
            processResult--;
        } else {
            processResult -= 10;
        }
    }
    private static void test8() {
        String str4_1 = "$";
        String str4_2 = "￥";
        if (test_not_equals(str4_1, str4_2) == 1 && test_length(str4_1, 1) == 1 && test_length(str4_2, 1) == 1)
        {
            processResult--;
        } else {
            processResult -= 10;
        }
    }
    private static void test9() {
        String str5_1 = "abc\n";
        String str5_2 = "abc\t";
        if (test_not_equals(str5_1, str5_2) == 1 && test_length(str5_1, 4) == 1) {
            processResult--;
        } else {
            processResult -= 10;
        }
    }
    private static void test10() {
        String str6_1 = "‘OR‘1’=’1";
        String str6_2 = "<‘script’>alter(“Test,Bom~~~”)<‘/script’>";
        if (test_equals(str6_1, "‘OR‘1’=’1") == 1 && test_equals(str6_2, "<‘script’>alter(“Test,Bom~~~”)" +
                "<‘/script’>") == 1) {
            processResult--;
        } else {
            processResult -= 10;
        }
    }
    private static void test11() {
        String str7_1 = "``_+-=asdfadfqwerqwz!@##$%&#*^%^&(&*^()*&^*)^&%^~@#%@#$%^sdfgrw5646843131324fshst{}|[]]:,\\," +
                "dkd d/.';,;'mmm:\"<>?;',./";
        String str7_2 = "``_+-=asdfadfqwerqwz!@##$%&#*^%^&(&*^()*&^*)^&%^~@#%@#$%^sdfgrw5646843131324fshst{}|[]]:,\\," +
                "dkd d/.';,;'mmm:\"<>?;',./";
        if (test_equals(str7_1, str7_2) == 1 && test_length(str7_1, 116) == 1) {
            processResult--;
        } else {
            processResult -= 10;
        }
    }
    private static void test12() {
        String str7_1 = "``_+-=asdfadfqwerqwz!@##$%&#*^%^&(&*^()*&^*)^&%^~@#%@#$%^sdfgrw5646843131324fshst{}|[]]:,\\," +
                "dkd d/.';,;'mmm:\"<>?;',./";
        String str7_3 = "``_+-=asdfadfqwerqwz!@##$%&#*^%^&(&*^()*&^*)^&%^~@#%@#$%^sdfgrw5646843131324fshst{}|[]]:,\\," +
                "dkd d/.';,;'mmm:\"<>?;',./123";
        if (test_not_equals(str7_1, str7_3) == 1) {
            processResult--;
        } else {
            processResult -= 10;
        }
    }
}
