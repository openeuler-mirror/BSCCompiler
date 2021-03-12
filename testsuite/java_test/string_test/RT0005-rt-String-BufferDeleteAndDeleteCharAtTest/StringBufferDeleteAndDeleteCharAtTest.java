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
public class StringBufferDeleteAndDeleteCharAtTest {
    private static int processResult = 99;
    public static void main(String[] argv) {
        System.out.println(run(argv, System.out));
    }
    public static int run(String[] argv, PrintStream out) {
        int result = 2; /* STATUS_Success*/

        try {
            StringBufferDeleteAndDeleteCharAtTest_1();
        } catch (Exception e) {
            processResult -= 10;
        }
        if (result == 2 && processResult == 99) {
            result = 0;
        }
        return result;
    }
    public static void StringBufferDeleteAndDeleteCharAtTest_1() {
        StringBuffer strBuffer1_1 = new StringBuffer("qwertyuiop{}[]\\|asdfghjkl;:'\"zxcvbnm,.<>/?~`1234567890-=!" +
                "@#$%^&*()_+ ASDFGHJKLQWERTYUIOPZXCVBNM0x96");
        StringBuffer strBuffer1_2 = new StringBuffer(" @!.&%()*");
        StringBuffer strBuffer1_3 = new StringBuffer("abc123abc");
        StringBuffer strBuffer2_1 = new StringBuffer("qwertyuiop{}[]\\|asdfghjkl;:'\"zxcvbnm,.<>/?~`1234567890-=!" +
                "@#$%^&*()_+ ASDFGHJKLQWERTYUIOPZXCVBNM0x96");
        StringBuffer strBuffer2_2 = new StringBuffer(" @!.&%()*");
        StringBuffer strBuffer2_3 = new StringBuffer("abc123abc");
        test1(strBuffer1_1);
        test1(strBuffer1_2);
        test1(strBuffer1_3);
        test2(strBuffer2_1);
        test2(strBuffer2_2);
        test2(strBuffer2_3);
    }
    // Test method delete(int start, int end).
    private static void test1(StringBuffer strBuffer) {
        // Test 0 < start < end, end < strBuffer.length().
        System.out.println(strBuffer.delete(3, 5));
        // Test start = 0.
        System.out.println(strBuffer.delete(0, 1));
        // Test end = 0.
        System.out.println(strBuffer.delete(0, 0));
        // Test start=strBuffer.length().
        System.out.println(strBuffer.delete(strBuffer.length(), strBuffer.length()));
        // Test end=strBuffer.length().
        System.out.println(strBuffer.delete(2, strBuffer.length()));
    }
    // Test method deleteCharAt(int index).
    private static void test2(StringBuffer strBuffer) {
        // Test 0 < index < strBuffer.length().
        System.out.println(strBuffer.deleteCharAt(1));
        // Test index = 0.
        System.out.println(strBuffer.deleteCharAt(0));
        // Test index = strBuffer.length() - 1.
        System.out.println(strBuffer.deleteCharAt(strBuffer.length() - 1));
    }
}
