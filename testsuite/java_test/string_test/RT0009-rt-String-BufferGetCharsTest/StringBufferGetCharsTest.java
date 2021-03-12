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
public class StringBufferGetCharsTest {
    private static int processResult = 99;
    public static void main(String[] argv) {
        System.out.println(run(argv, System.out));
    }
    public static int run(String[] argv, PrintStream out) {
        int result = 2; /* STATUS_Success*/

        try {
            StringBufferGetCharsTest_1();
        } catch (Exception e) {
            processResult -= 10;
        }
        if (result == 2 && processResult == 99) {
            result = 0;
        }
        return result;
    }
    public static void StringBufferGetCharsTest_1() {
        StringBuffer strBuffer1_1 = new StringBuffer("qwertyuiop{}[]\\|asdfghjkl;:'\"zxcvbnm,.<>/?~`1234567890-=!" +
                "@#$%^&*()_+ ASDFGHJKLQWERTYUIOPZXCVBNM0x96");
        StringBuffer strBuffer1_2 = new StringBuffer(" @!.&%()*");
        StringBuffer strBuffer1_3 = new StringBuffer("abc123");
        test1(strBuffer1_1);
        test1(strBuffer1_2);
        test1(strBuffer1_3);
    }
    private static void test1(StringBuffer strBuffer) {
        // Test srcBegin  < srcEnd < instance.length, dstBegin < dst(char[]).length.
        char[] dst = {'A', 'B', 'C', 'D', 'E', 'F'};
        strBuffer.getChars(2, 5, dst, 2);
        System.out.println(dst);
        // Test srcBegin = 0.
        dst = new char[]{'A', 'B', 'C', 'D', 'E', 'F'};
        strBuffer.getChars(0, 3, dst, 2);
        System.out.println(dst);
        // Test srcEnd = 0.
        dst = new char[]{'A', 'B', 'C', 'D', 'E', 'F'};
        strBuffer.getChars(0, 0, dst, 2);
        System.out.println(dst);
        // Test dstBegin = 0.
        dst = new char[]{'A', 'B', 'C', 'D', 'E', 'F'};
        strBuffer.getChars(2, 5, dst, 0);
        System.out.println(dst);
        // Test srcBegin = srcEnd.
        dst = new char[]{'A', 'B', 'C', 'D', 'E', 'F'};
        strBuffer.getChars(2, 2, dst, 0);
        System.out.println(dst);
        // Test srcBegin = strBuffer.length() - 1.
        dst = new char[]{'A', 'B', 'C', 'D', 'E', 'F'};
        int index = strBuffer.length() - 1;
        strBuffer.getChars(index, index, dst, 0);
        System.out.println(dst);
        // Test srcEnd = strBuffer.length() - 1.
        dst = new char[]{'A', 'B', 'C', 'D', 'E', 'F'};
        strBuffer.getChars(index - 2, index, dst, 0);
        System.out.println(dst);
        // Test dstBegin = dst.length - 1.
        dst = new char[]{'A', 'B', 'C', 'D', 'E', 'F'};
        strBuffer.getChars(0, 1, dst, dst.length - 1);
        System.out.println(dst);
    }
}
