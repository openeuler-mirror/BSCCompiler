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
public class StringBuilderSubstringAndToStringTest {
    private static int processResult = 99;
    public static void main(String[] argv) {
        System.out.println(run(argv, System.out));
    }
    public static int run(String[] argv, PrintStream out) {
        int result = 2; /* STATUS_Success*/

        try {
            StringBuilderSubstringAndToStringTest_1();
            StringBuilderSubstringAndToStringTest_2();
        } catch (Exception e) {
            processResult -= 10;
        }
        if (result == 2 && processResult == 99) {
            result = 0;
        }
        return result;
    }
    public static void StringBuilderSubstringAndToStringTest_1() {
        StringBuilder strBuilder1_1 = new StringBuilder("qwertyuiop{}[]\\|asdfghjkl;:'\"zxcvbnm,.<>/?~`1234567890" +
                "-=!@#$%^&*()_+ ASDFGHJKLQWERTYUIOPZXCVBNM0x96");
        StringBuilder strBuilder1_2 = new StringBuilder(" @!.&%()*");
        StringBuilder strBuilder1_3 = new StringBuilder("abc123abc");
        test1(strBuilder1_1);
        test1(strBuilder1_2);
        test1(strBuilder1_3);
        test2(strBuilder1_1);
        test2(strBuilder1_2);
        test2(strBuilder1_3);
    }
    public static void StringBuilderSubstringAndToStringTest_2() {
        StringBuilder strBuilder1_1 = new StringBuilder("qwertyuiop{}[]\\|asdfghjkl;:'\"zxcvbnm,.<>/?~`1234567890" +
                "-=!@#$%^&*()_+ ASDFGHJKLQWERTYUIOPZXCVBNM0x96");
        StringBuilder strBuilder1_2 = new StringBuilder(" @!.&%");
        StringBuilder strBuilder1_3 = new StringBuilder("abc123abc");
        StringBuilder strBuilder1_4 = new StringBuilder("");
        StringBuilder strBuilder1_5 = new StringBuilder();
        test3(strBuilder1_1);
        test3(strBuilder1_2);
        test3(strBuilder1_3);
        test3(strBuilder1_4);
        test3(strBuilder1_5);
    }
    // Test method substring(int start, int end).
    private static void test1(StringBuilder strBuilder) {
        System.out.println(strBuilder.substring(2, 7));
        // Test start = 0 & end = strBuffer.length().
        System.out.println(strBuilder.substring(0, strBuilder.length()));
        // Test start = strBuffer.length().
        System.out.println(strBuilder.substring(strBuilder.length(), strBuilder.length()));
        // Test end = 0.
        System.out.println(strBuilder.substring(0, 0));
    }
    // Test method substring(int start).
    private static void test2(StringBuilder strBuilder) {
        System.out.println(strBuilder.substring(3));
        // Test start = 0.
        System.out.println(strBuilder.substring(0));
        // Test start = strBuffer.length().
        System.out.println(strBuilder.substring(strBuilder.length()));
    }
    // Test method toString().
    private static void test3(StringBuilder strBuilder) {
        System.out.println(strBuilder.toString());
    }
}
