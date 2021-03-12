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
public class StringBufferReplaceAndReverseTest {
    private static int processResult = 99;
    public static void main(String[] argv) {
        System.out.println(run(argv, System.out));
    }
    public static int run(String[] argv, PrintStream out) {
        int result = 2; /* STATUS_Success*/

        try {
            StringBufferReplaceAndReverseTest_1();
            StringBufferReplaceAndReverseTest_2();
        } catch (Exception e) {
            processResult -= 10;
        }
        if (result == 2 && processResult == 99) {
            result = 0;
        }
        return result;
    }
    public static void StringBufferReplaceAndReverseTest_1() {
        StringBuffer strBuffer1_1 = new StringBuffer("qwertyuiop{}[]\\|asdfghjkl;:'\"zxcvbnm,.<>/?~`1234567890-=!" +
                "@#$%^&*()_+ ASDFGHJKLQWERTYUIOPZXCVBNM0x96");
        StringBuffer strBuffer1_2 = new StringBuffer(" @!.&%");
        StringBuffer strBuffer1_3 = new StringBuffer("abc123abc");
        test1(strBuffer1_1);
        test1(strBuffer1_2);
        test1(strBuffer1_3);
    }
    public static void StringBufferReplaceAndReverseTest_2() {
        StringBuffer strBuffer1_1 = new StringBuffer("qwertyuiop{}[]\\|asdfghjkl;:'\"zxcvbnm,.<>/?~`1234567890-=!" +
                "@#$%^&*()_+ ASDFGHJKLQWERTYUIOPZXCVBNM0x96");
        StringBuffer strBuffer1_2 = new StringBuffer(" @!.&%");
        StringBuffer strBuffer1_3 = new StringBuffer("abc123abc");
        StringBuffer strBuffer1_4 = new StringBuffer("");
        StringBuffer strBuffer1_5 = new StringBuffer();
        test2(strBuffer1_1);
        test2(strBuffer1_2);
        test2(strBuffer1_3);
        test2(strBuffer1_4);
        test2(strBuffer1_5);
    }
    // Test method replace(int start, int end, String str).
    private static void test1(StringBuffer strBuffer) {
        System.out.println(strBuffer.replace(2, 6, "xyz"));
        // end = 0
        System.out.println(strBuffer.replace(0, 0, "xyz"));
        // start = 0 & end = strBuffer.length()
        System.out.println(strBuffer.replace(0, strBuffer.length(), "xyz"));
        // start = strBuffer.length() & end = strBuffer.length()
        System.out.println(strBuffer.replace(strBuffer.length(), strBuffer.length(), "xyz"));
    }
    // Test method reverse().
    private static void test2(StringBuffer strBuffer) {
        strBuffer.reverse();
        System.out.println(strBuffer);
    }
}
