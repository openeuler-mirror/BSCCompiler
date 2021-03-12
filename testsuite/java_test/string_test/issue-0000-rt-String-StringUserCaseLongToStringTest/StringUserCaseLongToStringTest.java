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
public class StringUserCaseLongToStringTest {
    private static int processResult = 99;
    public static void main(String[] argv) {
        run(argv, System.out);
    }
    public static int run(String argv[],PrintStream out){
        int result = 2/*STATUS_FAILED*/;
        try {
            StringUserCaseLongToStringTest_1();
            StringUserCaseLongToStringTest_2();
            StringUserCaseLongToStringTest_3();
            StringUserCaseLongToStringTest_4();
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
    //		1.Long.toHexString(long l)
    public static void StringUserCaseLongToStringTest_1() {
        String testCaseID = "StringUserCaseLongToStringTest_1";
        System.out.println("========================" + testCaseID);
        long l_a = 0;
        long l_b = 111;
        long l_c = 2147483647;
        test1(l_a);
        test1(l_b);
        test1(l_c);
    }
    public static void StringUserCaseLongToStringTest_2() {
        String testCaseID = "StringUserCaseLongToStringTest_2";
        System.out.println("========================" + testCaseID);
        Integer i_a = 0;
        Integer i_b = 111;
        Integer i_c = 2147483647;
        test2(i_a);
        test2(i_b);
        test2(i_c);
    }
    public static void StringUserCaseLongToStringTest_3() {
        String testCaseID = "StringUserCaseLongToStringTest_3";
        System.out.println("========================" + testCaseID);
        float f_a = 0f;
        float f_b = 11.1f;
        float f_c = 2147483.647f;
        test3(f_a);
        test3(f_b);
        test3(f_c);
    }
    public static void StringUserCaseLongToStringTest_4() {
        String testCaseID = "StringUserCaseLongToStringTest_4";
        System.out.println("========================" + testCaseID);
        double d_a = 0;
        double d_b = 1.11d;
        double d_c = 21474836.47;
        test4(d_a);
        test4(d_b);
        test4(d_c);
    }
    private static void test1(Long lo) {
        System.out.println(Long.toHexString(lo));
    }
    private static void test2(Integer i) {
        System.out.println(Integer.toHexString(i));
    }
    private static void test3(float f) {
        System.out.println(Float.toHexString(f));
    }
    private static void test4(double f) {
        System.out.println(Double.toHexString(f));
    }
}
