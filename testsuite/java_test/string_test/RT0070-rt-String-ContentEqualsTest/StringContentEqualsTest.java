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
public class StringContentEqualsTest {
    private static int processResult = 99;
    public static void main(String[] argv) {
        System.out.println(run(argv, System.out));
    }
    public static int run(String[] argv, PrintStream out) {
        int result = 2; /* STATUS_Success*/

        try {
            StringContentEqualsTest_1();
        } catch (Exception e) {
            processResult -= 10;
        }
        if (result == 2 && processResult == 99) {
            result = 0;
        }
        return result;
    }
    public static void StringContentEqualsTest_1() {
        String str1_1 = new String("abc123");
        String str1_2 = new String("******");
        String str1_3 = new String("ABc123");
        String str1_4 = new String("");
        String str1_5 = new String();
        String str2_1 = "abc123";
        String str2_2 = "******";
        String str2_3 = "ABc123";
        String str2_4 = "";
        test(str1_1);
        test(str1_2);
        test(str1_3);
        test(str1_4);
        test(str1_5);
        test(str2_1);
        test(str2_2);
        test(str2_3);
        test(str2_4);
    }
    private static void test(String str) {
        String test = "abc123";
        StringBuffer test1_1 = new StringBuffer(str);
        boolean result = test.contentEquals(test1_1);
        System.out.println(result);
    }
}
