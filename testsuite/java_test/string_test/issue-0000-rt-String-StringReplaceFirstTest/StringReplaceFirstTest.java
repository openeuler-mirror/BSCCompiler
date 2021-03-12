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
public class StringReplaceFirstTest {
    private static int processResult = 99;
    public static void main(String[] argv) {
        run(argv, System.out);
    }
    public static int run(String argv[],PrintStream out){
        int result = 2/*STATUS_FAILED*/;
        try {
            StringReplaceFirstTest_1();
        } catch(Exception e){
            System.out.println(e);
            processResult = processResult - 10;
        }
//        System.out.println("result: " + result);
//        System.out.println("processResult:" + processResult);
        if (result == 1 && processResult == 99){
            result =0;
        }
        return result;
    }
    public static void StringReplaceFirstTest_1() {
        String testCaseID = "StringReplaceFirstTest_1";
        System.out.println("========================" + testCaseID);
        String str1_1 = new String("abc123abc");
        String str1_2 = new String("      ");
        String str1_3 = new String("abc123abc");
        String str1_4 = new String("");
        String str1_5 = new String();
        String str2_1 = "abc123ABC";
        String str2_2 = "      ";
        String str2_3 = "abc123ABC";
        String str2_4 = "";
        String str2_5 = null;
        test(str1_1);
        test(str1_2);
        test(str1_3);
        test(str1_4);
        test(str1_5);
        test(str2_1);
        test(str2_2);
        test(str2_3);
        test(str2_4);
//        test(str2_5);
        System.out.println("test add: regular expression");
        String str3_1 = "abc123ABC-abc123ABC";
        System.out.println(str3_1.replaceFirst("[ab][bc][cd]", "@@@"));
        String str4_1 = "abc$123ABC-abc$123ABC";
        System.out.println(str4_1.replaceFirst("\\$", "@"));
        String str5_1 = "2018-01-02-2018-01-02";
        System.out.println(str5_1.replaceFirst("\\d{4}", "@@@@"));
        String str6_1 = "abc123\rABC-abc123\\rABC";
        System.out.println(str6_1.replaceFirst("\\\r", "@"));
        String str7_1 = "abc123ABC-abc123ABC";
        System.out.println(str7_1.replaceFirst("^abc", "@@@"));
    }
    private static void test(String str) {
        System.out.println(str.replaceFirst("a", "@"));
    }
}
