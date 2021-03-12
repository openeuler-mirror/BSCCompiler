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
public class StringBuilderCapacityAndEnsureCapacityTest {
    private static int processResult = 99;
    public static void main(String[] argv) {
        System.out.println(run(argv, System.out));
    }
    public static int run(String[] argv, PrintStream out) {
        int result = 2; /* STATUS_Success*/

        try {
            StringBuilderCapacityAndEnsureCapacityTest_1();
        } catch (Exception e) {
            processResult -= 10;
        }
        if (result == 2 && processResult == 99) {
            result = 0;
        }
        return result;
    }
    public static void StringBuilderCapacityAndEnsureCapacityTest_1() {
        StringBuilder strBuilder1_1 = new StringBuilder("qwertyuiop{}[]\\|asdfghjkl;:'\"zxcvbnm,.<>/?~`1234567890" +
                "-=!@#$%^&*()_+ ASDFGHJKLQWERTYUIOPZXCVBNM0x96");
        StringBuilder strBuilder1_2 = new StringBuilder(" @!.&%()*");
        StringBuilder strBuilder1_3 = new StringBuilder("abc123");
        StringBuilder strBuilder1_4 = new StringBuilder("");
        StringBuilder strBuilder1_5 = new StringBuilder();
        test1(strBuilder1_1);
        test1(strBuilder1_2);
        test1(strBuilder1_3);
        test1(strBuilder1_4);
        test1(strBuilder1_5);
    }
    private static void test1(StringBuilder strBuilder) {
        System.out.println("1-capacity: " + strBuilder.capacity());
        System.out.println("1-lent: " + strBuilder.length());
        strBuilder = strBuilder.append("01234567890123456789");
        System.out.println("2-capacity: " + strBuilder.capacity());
        System.out.println("2-lent: " + strBuilder.length());
        strBuilder.ensureCapacity(80);
        System.out.println("3-capacity: " + strBuilder.capacity());
        System.out.println("3-lent: " + strBuilder.length());
        strBuilder.setLength(100);
        System.out.println("4-capacity: " + strBuilder.capacity());
        System.out.println("4-lent: " + strBuilder.length());
        strBuilder.trimToSize();
        System.out.println("5-capacity: " + strBuilder.capacity());
        System.out.println("5-lent: " + strBuilder.length());
        strBuilder.ensureCapacity(-2);
        System.out.println("6-capacity: " + strBuilder.capacity());
        System.out.println("6-lent: " + strBuilder.length());
        strBuilder.ensureCapacity(0);
        System.out.println("7-capacity: " + strBuilder.capacity());
        System.out.println("7-lent: " + strBuilder.length());
        strBuilder.ensureCapacity(2);
        System.out.println("8-capacity: " + strBuilder.capacity());
        System.out.println("8-lent: " + strBuilder.length());
    }
}
