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
public class StringSubSequenceIntIntTest {
    private static int processResult = 99;
    public static void main(String[] argv) {
        System.out.println(run(argv, System.out));
    }
    public static int run(String[] argv, PrintStream out) {
        int result = 2; /* STATUS_Success*/

        try {
            StringSubSequenceIntIntTest_1();
        } catch (Exception e) {
            processResult -= 10;
        }
        if (result == 2 && processResult == 99) {
            result = 0;
        }
        return result;
    }
    public static void StringSubSequenceIntIntTest_1() {
        String str1_1 = new String("qwertyuiop{}[]\\|asdfghjkl;:'\"zxcvbnm,.<>/?~`1234567890-=!@#$%^&*()_+ A" +
                "SDFGHJKLQWERTYUIOPZXCVBNM0x96");
        String str1_2 = new String(" @!.&%");
        String str1_3 = new String("abc123abc");
        String str2_1 = "qwertyuiop{}[]\\|asdfghjkl;:'\"zxcvbnm,.<>/?~`1234567890-=!@#$%^&*()_+ ASDFGHJKLQWERTYUIOPZ" +
                "XCVBNM0x96";
        String str2_2 = " @!.&%";
        String str2_3 = "abc123ABC";
        test(str1_1);
        test(str1_2);
        test(str1_3);
        test(str2_1);
        test(str2_2);
        test(str2_3);
    }
    private static void test(String str) {
        // Test 0 < beginIndex < endIndex < str.length().
        System.out.println(str.subSequence(2, 6));
        // Test beginIndex = 0.
        System.out.println(str.subSequence(0, str.length()));
        // Test enfIndex = 0.
        System.out.println(str.subSequence(0, 0));
        // Test beginIndex = str.length().
        System.out.println(str.subSequence(str.length(), str.length()));
        // Test endIndex = str.length().
        System.out.println(str.subSequence(2, str.length()));
    }
}
