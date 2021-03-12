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
public class StringBuilderIndexOfAndLastIndexOfTest {
    private static int processResult = 99;
    public static void main(String[] argv) {
        System.out.println(run(argv, System.out));
    }
    public static int run(String[] argv, PrintStream out) {
        int result = 2; /* STATUS_Success*/

        try {
            StringBuilderIndexOfAndLastIndexOfTest_1();
            StringBuilderIndexOfAndLastIndexOfTest_2();
            StringBuilderIndexOfAndLastIndexOfTest_3();
            StringBuilderIndexOfAndLastIndexOfTest_4();
        } catch (Exception e) {
            processResult -= 10;
        }
        if (result == 2 && processResult == 99) {
            result = 0;
        }
        return result;
    }
    public static void StringBuilderIndexOfAndLastIndexOfTest_1() {
        StringBuilder strBuilder1_1 = new StringBuilder("qwertyuiop{}[]\\|asdfghjkl;:'\"zxcvbnm,.<>/?~`1234567890" +
                "-=!@#$%^&*()_+ ASDFGHJKLQWERTYUIOPZXCVBNM0x96");
        StringBuilder strBuilder1_2 = new StringBuilder(" @!.&%()*");
        StringBuilder strBuilder1_3 = new StringBuilder("abc123abc");
        StringBuilder strBuilder1_4 = new StringBuilder("");
        StringBuilder strBuilder1_5 = new StringBuilder();
        test1(strBuilder1_1);
        test1(strBuilder1_2);
        test1(strBuilder1_3);
        test1(strBuilder1_4);
        test1(strBuilder1_5);
    }
    public static void StringBuilderIndexOfAndLastIndexOfTest_2() {
        StringBuilder strBuilder1_1 = new StringBuilder("qwertyuiop{}[]\\|asdfghjkl;:'\"zxcvbnm,.<>/?~`1234567890" +
                "-=!@#$%^&*()_+ ASDFGHJKLQWERTYUIOPZXCVBNM0x96");
        StringBuilder strBuilder1_2 = new StringBuilder(" @!.&%()*");
        StringBuilder strBuilder1_3 = new StringBuilder("abc123abc");
        StringBuilder strBuilder1_4 = new StringBuilder("");
        StringBuilder strBuilder1_5 = new StringBuilder();
        test2(strBuilder1_1);
        test2(strBuilder1_2);
        test2(strBuilder1_3);
        test2(strBuilder1_4);
        test2(strBuilder1_5);
    }
    public static void StringBuilderIndexOfAndLastIndexOfTest_3() {
        StringBuilder strBuilder1_1 = new StringBuilder("qwertyuiop{}[]\\|asdfghjkl;:'\"zxcvbnm,.<>/?~`1234567890" +
                "-=!@#$%^&*()_+ ASDFGHJKLQWERTYUIOPZXCVBNM0x96");
        StringBuilder strBuilder1_2 = new StringBuilder(" @!.&%()*");
        StringBuilder strBuilder1_3 = new StringBuilder("abc123abc");
        StringBuilder strBuilder1_4 = new StringBuilder("");
        StringBuilder strBuilder1_5 = new StringBuilder();
        test3(strBuilder1_1);
        test3(strBuilder1_2);
        test3(strBuilder1_3);
        test3(strBuilder1_4);
        test3(strBuilder1_5);
    }
    public static void StringBuilderIndexOfAndLastIndexOfTest_4() {
        StringBuilder strBuilder1_1 = new StringBuilder("qwertyuiop{}[]\\|asdfghjkl;:'\"zxcvbnm,.<>/?~`1234567890" +
                "-=!@#$%^&*()_+ ASDFGHJKLQWERTYUIOPZXCVBNM0x96");
        StringBuilder strBuilder1_2 = new StringBuilder(" @!.&%()*");
        StringBuilder strBuilder1_3 = new StringBuilder("abc123abc");
        StringBuilder strBuilder1_4 = new StringBuilder("");
        StringBuilder strBuilder1_5 = new StringBuilder();
        test4(strBuilder1_1);
        test4(strBuilder1_2);
        test4(strBuilder1_3);
        test4(strBuilder1_4);
        test4(strBuilder1_5);
    }
    // Test method indexOf(String str).
    private static void test1(StringBuilder strBuilder) {
        System.out.println(strBuilder.indexOf("b"));
        // Test empty String.
        System.out.println(strBuilder.indexOf(""));
    }
    // Test method indexOf(String str, int fromIndex).
    private static void test2(StringBuilder strBuilder) {
        System.out.println(strBuilder.indexOf("b", 2));
        // Test fromIndex = 0.
        System.out.println(strBuilder.indexOf("b", 0));
        // Test fromIndex = strBuilder.length() - 1.
        System.out.println(strBuilder.indexOf("6", strBuilder.length() - 1));
    }
    // Test method lastIndexOf(String str).
    private static void test3(StringBuilder strBuilder) {
        System.out.println(strBuilder.lastIndexOf("b"));
        // Test empty String.
        System.out.println(strBuilder.lastIndexOf(""));
    }
    // Test method lastIndexOf(String str, int fromIndex).
    private static void test4(StringBuilder strBuilder) {
        System.out.println(strBuilder.lastIndexOf("b", 1));
        // Test fromIndex = 0.
        System.out.println(strBuilder.lastIndexOf("b", 0));
        // Test fromIndex = strBuilder.length() - 1.
        System.out.println(strBuilder.lastIndexOf("6", strBuilder.length() - 1));
    }
}