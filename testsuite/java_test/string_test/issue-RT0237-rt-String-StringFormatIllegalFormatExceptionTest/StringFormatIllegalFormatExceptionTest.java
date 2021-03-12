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
import java.util.IllegalFormatException;
import java.util.Locale;
public class StringFormatIllegalFormatExceptionTest {
    private static int processResult = 99;
    public static void main(String[] argv) {
        System.out.println(run(argv, System.out));
    }
    public static int run(String argv[], PrintStream out) {
        int result = 2; /*STATUS_FAILED*/
        try {
            result = StringFormatIllegalFormatExceptionTest_1();
        } catch (Exception e) {
            processResult -= 10;
        }
        if (result == 4 && processResult == 96) {
            result = 0;
        }
//        System.out.println("result: " + result);
//        System.out.println("processResult:" + processResult);
        return result;
    }
    public static int StringFormatIllegalFormatExceptionTest_1() {
        int result1 = 4; /*STATUS_FAILED*/
        ;
//
        String str1_1 = new String("abc123ABC");
//        String str1_2 = new String("      ");
//        String str1_3 = new String("abc123ABC");
//        String str1_4 = new String("");
//        String str1_5 = new String();
//
//        String str2_1 = "abc123ABC";
//        String str2_2 = "      ";
//        String str2_3 = "abc123ABC";
//        String str2_4 = "";
//        String str2_5 = null;
        //IllegalFormatException - If a format string contains an illegal syntax,
        // a format specifier that is incompatible with the given arguments, insufficient arguments given the format string,
        // or other illegal conditions.
        // For specification of all possible formatting errors, see the Details section of the formatter class specification.
        try {
//            String str1 =String.format("%c %n", 'A');
            String str1 = String.format("%p %n", 'A');
            processResult -= 10;
        } catch (IllegalFormatException e1) {
            processResult--;
//            System.out.println("1-1");
        }
        try {
//            String str1 = String.format("%b %n", 3>7);
            String str1 = String.format("%p %n", 3 > 7);
            processResult -= 10;
        } catch (IllegalFormatException e1) {
            processResult--;
//            System.out.println("1-2");
        }
        try {
//            String str1=String.format(Locale.ENGLISH,"%c %n", 'A');
            String str1 = String.format(Locale.ENGLISH, "%p %n", 'A');
            processResult -= 10;
        } catch (IllegalFormatException e1) {
            processResult--;
//            System.out.println("1-3");
        }
        return result1;
    }
}
