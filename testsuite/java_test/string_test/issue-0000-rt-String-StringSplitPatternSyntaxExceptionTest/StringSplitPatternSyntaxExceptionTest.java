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
import java.nio.charset.Charset;
import java.util.regex.PatternSyntaxException;
public class StringSplitPatternSyntaxExceptionTest {
    private static int processResult = 99;
    public static void main(String argv[]) {
        System.out.println(run(argv, System.out));
    }
    public static int run(String argv[], PrintStream out) {
        int result = 2; /*STATUS_FAILED*/
        try {
            result = StringSplitPatternSyntaxExceptionTest_1();
        } catch (Exception e) {
            processResult -= 10;
        }
        if (result == 4 && processResult == 97) {
            result = 0;
        }
//        System.out.println("result: " + result);
//        System.out.println("processResult:" + processResult);
        return result;
    }
    public static int StringSplitPatternSyntaxExceptionTest_1() {
        int result1 = 4; /*STATUS_FAILED*/
        String str1_1 = new String("abc123ABC");
//        String str1_2 = new String("      ");
//        String str1_3 = new String("abc123ABC");
        String str1_4 = new String("");
        String str1_5 = new String();
        String str2_1 = "abc123ABC";
//        String str2_2 = "      ";
//        String str2_3 = "abc123ABC";
        String str2_4 = "";
//		String str2_5 = null;
        //PatternSyntaxException - if the regular expression's syntax is invalid
        try {
            String[] str = str1_1.split("\\", 3);
            processResult -= 10;
        } catch (PatternSyntaxException e1) {
            processResult--;
        }
        try {
            String[] str = str1_1.split("\\");
            processResult -= 10;
        } catch (PatternSyntaxException e1) {
            processResult--;
        }
        return result1;
    }
}
