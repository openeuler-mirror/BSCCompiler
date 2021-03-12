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
public class StringBuilderCodePointExceptionTest {
    private static int processResult = 99;
    public static void main(String[] argv) {
        System.out.println(run(argv, System.out));
    }
    public static int run(String[] argv, PrintStream out) {
        int result = 2; /* STATUS_Success*/

        try {
            StringBuilderCodePointExceptionTest_1();
        } catch (Exception e) {
            processResult -= 10;
        }
        if (result == 2 && processResult == 99) {
            result = 0;
        }
        return result;
    }
    public static void StringBuilderCodePointExceptionTest_1() {
        StringBuilder strBuilder1_1 = new StringBuilder("qwertyuiop{}[]\\|asdfghjkl;:'\"zxcvbnm,.<>/?~`123456789" +
                "0-=!@#$%^&*()_+ ASDFGHJKLQWERTYUIOPZXCVBNM0x96");
        StringBuilder strBuilder1_2 = new StringBuilder(" @!.&%");
        StringBuilder strBuilder1_3 = new StringBuilder("abc123");
        test1(strBuilder1_1);
        test1(strBuilder1_2);
        test1(strBuilder1_3);
        test2(strBuilder1_1);
        test2(strBuilder1_2);
        test2(strBuilder1_3);
        test3(strBuilder1_1);
        test3(strBuilder1_2);
        test3(strBuilder1_3);
        test4(strBuilder1_1);
        test4(strBuilder1_2);
        test4(strBuilder1_3);
    }
    // Test method codePointAt(int index).
    private static void test1(StringBuilder strBuilder) {
        int codePoint = 0;
        for (int i = 0; i < 6; i++) {
            try {
                codePoint = strBuilder.codePointAt(i);
                System.out.println("i=" + i + " " + "codePointAt=" + codePoint);
            } catch (StringIndexOutOfBoundsException e1) {
                System.out.println("codePointAt(): " + i + "out of length");
            } finally {
                try {
                    System.out.println(strBuilder.charAt(i) + " Unicode is" + ":" + codePoint);
                } catch (StringIndexOutOfBoundsException e2) {
                    System.out.println(i + " out of length");
                }
            }
        }
    }
    // Test method codePointBefore(int index).
    private static void test2(StringBuilder strBuilder) {
        int codePoint = 0;
        for (int i = 0; i < 6; i++) {
            try {
                codePoint = strBuilder.codePointBefore(i);
                System.out.println("i=" + i + " " + "codePointBefore=" + codePoint);
            } catch (StringIndexOutOfBoundsException e1) {
                System.out.println("codePointBefore(): " + i + "out of length");
            } finally {
                try {
                    System.out.println(strBuilder.charAt(i - 1) + " Unicode is" + ":" + codePoint);
                } catch (StringIndexOutOfBoundsException e2) {
                    System.out.println(i + " out of length");
                }
            }
        }
    }
    // Test method codePointCount(int beginIndex, int endIndex).
    private static void test3(StringBuilder strBuilder) {
        int codePoint = 0;
        for (int i = 0; i < 4; i++) {
            try {
                codePoint = strBuilder.codePointCount(i, i + 3);
                System.out.println("i=" + i + " " + "codePointCount=" + codePoint);
            } catch (IndexOutOfBoundsException e1) {
                System.out.println("codePointCount(): " + i + " out of length");
            } finally {
                try {
                    System.out.println(strBuilder.charAt(i) + "~" + strBuilder.charAt(i + 3)
                            + " codePointCount is " + ":" + codePoint);
                } catch (StringIndexOutOfBoundsException e2) {
                    System.out.println(i + " out of length");
                }
            }
        }
    }
    // Test method int offsetByCodePoints(int index, int codePointOffset).
    private static void test4(StringBuilder strBuilder) {
        int codePoint = 0;
        for (int i = 0; i < 4; i++) {
            try {
                codePoint = strBuilder.offsetByCodePoints(i, i + 3);
                System.out.println("i=" + i + " " + "offsetByCodePoints=" + codePoint);
            } catch (IndexOutOfBoundsException e1) {
                System.out.println("offsetByCodePoints(): " + i + "+3  out of length");
            } finally {
                try {
                    System.out.println(strBuilder.charAt(i) + " offsetByCodePoints +3 is " + ":" + codePoint);
                } catch (StringIndexOutOfBoundsException e2) {
                    System.out.println("charAt(): " + i + " out of length");
                }
            }
        }
    }
}
