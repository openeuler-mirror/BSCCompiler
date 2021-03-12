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
public class StringBufferIndexOfAndLastIndexOfTest {
    private static int processResult = 99;
    public static void main(String[] argv) {
        System.out.println(run(argv, System.out));
    }
    public static int run(String[] argv, PrintStream out) {
        int result = 2; /* STATUS_Success*/

        try {
            StringBufferIndexOfAndLastIndexOfTest_1();
            StringBufferIndexOfAndLastIndexOfTest_2();
            StringBufferIndexOfAndLastIndexOfTest_3();
            StringBufferIndexOfAndLastIndexOfTest_4();
        } catch (Exception e) {
            processResult -= 10;
        }
        if (result == 2 && processResult == 99) {
            result = 0;
        }
        return result;
    }
    public static void StringBufferIndexOfAndLastIndexOfTest_1() {
        StringBuffer strBuffer1_1 = new StringBuffer("qwertyuiop{}[]\\|asdfghjkl;:'\"zxcvbnm,.<>/?~`1234567890-=!" +
                "@#$%^&*()_+ ASDFGHJKLQWERTYUIOPZXCVBNM0x96");
        StringBuffer strBuffer1_2 = new StringBuffer(" @!.&%()*");
        StringBuffer strBuffer1_3 = new StringBuffer("abc123abc");
        StringBuffer strBuffer1_4 = new StringBuffer("");
        StringBuffer strBuffer1_5 = new StringBuffer();
        test1(strBuffer1_1);
        test1(strBuffer1_2);
        test1(strBuffer1_3);
        test1(strBuffer1_4);
        test1(strBuffer1_5);
    }
    public static void StringBufferIndexOfAndLastIndexOfTest_2() {
        StringBuffer strBuffer1_1 = new StringBuffer("qwertyuiop{}[]\\|asdfghjkl;:'\"zxcvbnm,.<>/?~`1234567890-=!" +
                "@#$%^&*()_+ ASDFGHJKLQWERTYUIOPZXCVBNM0x96");
        StringBuffer strBuffer1_2 = new StringBuffer(" @!.&%()*");
        StringBuffer strBuffer1_3 = new StringBuffer("abc123abc");
        StringBuffer strBuffer1_4 = new StringBuffer("");
        StringBuffer strBuffer1_5 = new StringBuffer();
        test2(strBuffer1_1);
        test2(strBuffer1_2);
        test2(strBuffer1_3);
        test2(strBuffer1_4);
        test2(strBuffer1_5);
    }
    public static void StringBufferIndexOfAndLastIndexOfTest_3() {
        StringBuffer strBuffer1_1 = new StringBuffer("qwertyuiop{}[]\\|asdfghjkl;:'\"zxcvbnm,.<>/?~`1234567890-=!" +
                "@#$%^&*()_+ ASDFGHJKLQWERTYUIOPZXCVBNM0x96");
        StringBuffer strBuffer1_2 = new StringBuffer(" @!.&%()*");
        StringBuffer strBuffer1_3 = new StringBuffer("abc123abc");
        StringBuffer strBuffer1_4 = new StringBuffer("");
        StringBuffer strBuffer1_5 = new StringBuffer();
        test3(strBuffer1_1);
        test3(strBuffer1_2);
        test3(strBuffer1_3);
        test3(strBuffer1_4);
        test3(strBuffer1_5);
    }
    public static void StringBufferIndexOfAndLastIndexOfTest_4() {
        StringBuffer strBuffer1_1 = new StringBuffer("qwertyuiop{}[]\\|asdfghjkl;:'\"zxcvbnm,.<>/?~`1234567890-=!" +
                "@#$%^&*()_+ ASDFGHJKLQWERTYUIOPZXCVBNM0x96");
        StringBuffer strBuffer1_2 = new StringBuffer(" @!.&%()*");
        StringBuffer strBuffer1_3 = new StringBuffer("abc123abc");
        StringBuffer strBuffer1_4 = new StringBuffer("");
        StringBuffer strBuffer1_5 = new StringBuffer();
        test4(strBuffer1_1);
        test4(strBuffer1_2);
        test4(strBuffer1_3);
        test4(strBuffer1_4);
        test4(strBuffer1_5);
    }
    // Test method indexOf(String str).
    private static void test1(StringBuffer strBuffer) {
        System.out.println(strBuffer.indexOf("b"));
        // Test empty String.
        System.out.println(strBuffer.indexOf(""));
    }
    // Test method indexOf(String str, int fromIndex).
    private static void test2(StringBuffer strBuffer) {
        System.out.println(strBuffer.indexOf("b", 2));
        // Test fromIndex = 0.
        System.out.println(strBuffer.indexOf("b", 0));
        // Test fromIndex = strBuffer.length() - 1.
        System.out.println(strBuffer.indexOf("6", strBuffer.length() - 1));
    }
    // Test method lastIndexOf(String str).
    private static void test3(StringBuffer strBuffer) {
        System.out.println(strBuffer.lastIndexOf("b"));
        // Test empty String.
        System.out.println(strBuffer.lastIndexOf(""));
    }
    // Test method lastIndexOf(String str, int fromIndex).
    private static void test4(StringBuffer strBuffer) {
        System.out.println(strBuffer.lastIndexOf("b", 1));
        // Test fromIndex = 0.
        System.out.println(strBuffer.lastIndexOf("b", 0));
        // Test fromIndex = strBuffer.length() - 1.
        System.out.println(strBuffer.lastIndexOf("6", strBuffer.length() - 1));
    }
}
