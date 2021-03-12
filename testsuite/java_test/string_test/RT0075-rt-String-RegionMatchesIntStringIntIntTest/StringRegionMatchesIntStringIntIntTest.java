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
public class StringRegionMatchesIntStringIntIntTest {
    private static int processResult = 99;
    public static void main(String[] argv) {
        System.out.println(run(argv, System.out));
    }
    public static int run(String[] argv, PrintStream out) {
        int result = 2; /* STATUS_Success*/

        try {
            StringRegionMatchesIntStringIntIntTest_1();
        } catch (Exception e) {
            processResult -= 10;
        }
        if (result == 2 && processResult == 99) {
            result = 0;
        }
        return result;
    }
    public static void StringRegionMatchesIntStringIntIntTest_1() {
        String str1_1 = new String("abc123_SAME");
        String str1_2 = new String("      _same");
        String str1_3 = new String("abc123_same");
        String str1_4 = new String("");
        String str1_5 = new String();
        String str2_1 = "abc123_SAME";
        String str2_2 = "      _same";
        String str2_3 = "abc123_same";
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
        String test = "same";
        // Test 0 =< toffset < instance.length, 0 =< ooffset < other.length,
        // 0 =< len < 0 =< len < Min value of other.length - ooffset -1 and instance.length - toffset - 1
        System.out.println(str.regionMatches(7, test, 0, 4));
        System.out.println(str.regionMatches(7, test, 0, 5));
        System.out.println(str.regionMatches(8, test, 1, 3));
        // Test toffset < 0.
        System.out.println(str.regionMatches(-2, test, 0, 2));
        // Test toffset = 0.
        System.out.println(str.regionMatches(0, test, 1, 1));
        // Test toffset < instance.length - 1.
        System.out.println(str.regionMatches(10, test, 1, 1));
        // Test ooffset = other.length - 1.
        System.out.println(str.regionMatches(10, test, 3, 1));
        // Test len = 0.
        System.out.println(str.regionMatches(7, test, 0, 0));
        // Test len = str.length().
        System.out.println(str.regionMatches(7, test, 0, str.length()));
    }
}
