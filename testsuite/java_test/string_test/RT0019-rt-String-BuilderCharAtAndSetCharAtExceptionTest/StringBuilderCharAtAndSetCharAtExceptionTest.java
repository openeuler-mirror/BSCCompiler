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
public class StringBuilderCharAtAndSetCharAtExceptionTest {
    private static int processResult = 99;
    public static void main(String[] argv) {
        System.out.println(run(argv, System.out));
    }
    public static int run(String[] argv, PrintStream out) {
        int result = 2; /* STATUS_Success*/

        try {
            StringBuilderCharAtAndSetCharAtExceptionTest_1();
            StringBuilderCharAtAndSetCharAtExceptionTest_2();
        } catch (Exception e) {
            processResult -= 10;
        }
        if (result == 2 && processResult == 99) {
            result = 0;
        }
        return result;
    }
    public static void StringBuilderCharAtAndSetCharAtExceptionTest_1() {
        StringBuilder strBuilder1_1 = new StringBuilder("qwertyuiop{}[]\\|asdfghjkl;:'\"zxcvbnm,.<>/?~`1234567890" +
                "-=!@#$%^&*()_+ ASDFGHJKLQWERTYUIOPZXCVBNM0x96");
        StringBuilder strBuilder1_2 = new StringBuilder(" @!.&%()*");
        StringBuilder strBuilder1_3 = new StringBuilder("abc123");
        StringBuilder strBuilder1_4 = new StringBuilder("");
        test1(strBuilder1_1);
        test1(strBuilder1_2);
        test1(strBuilder1_3);
        test1(strBuilder1_4);
    }
    public static void StringBuilderCharAtAndSetCharAtExceptionTest_2() {
        StringBuilder strBuilder1_1 = new StringBuilder("qwertyuiop{}[]\\|asdfghjkl;:'\"zxcvbnm,.<>/?~`1234567890" +
                "-=!@#$%^&*()_+ ASDFGHJKLQWERTYUIOPZXCVBNM0x96");
        StringBuilder strBuilder1_2 = new StringBuilder(" @!.&%()*");
        StringBuilder strBuilder1_3 = new StringBuilder("abc123");
        StringBuilder strBuilder1_4 = new StringBuilder("");
        test2(strBuilder1_1);
        test2(strBuilder1_2);
        test2(strBuilder1_3);
        test2(strBuilder1_4);
    }
    // Test method char charAt(int index).
    private static void test1(StringBuilder strBuilder) {
        int charAt = 0;
        for (int i = -1; i < 8; i++) {
            try {
                charAt = strBuilder.charAt(i);
                System.out.println("i=" + i + "  " + "charAt=" + charAt);
            } catch (StringIndexOutOfBoundsException e1) {
                System.out.println("index: " + i + "  String index out of range");
            }
        }
    }
    // Test Method void setCharAt(int index, char ch).
    private static void test2(StringBuilder strBuilder) {
        char ch = 'A';
        for (int i = -1; i < 8; i++) {
            try {
                strBuilder.setCharAt(i, ch);
                System.out.println("i=" + i + "  " + "setcharAt=" + strBuilder);
            } catch (StringIndexOutOfBoundsException e1) {
                System.out.println("index: " + i + "  String index out of range");
            }
        }
    }
}