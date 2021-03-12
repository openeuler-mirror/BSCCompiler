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
public class StringVariableOfInterfaceTest implements StringVariableOfInterfaceTest_1 {
    private static int processResult = 99;
    public static void main(String[] argv) {
        System.out.println(run(argv, System.out));
    }
    public static int run(String[] argv, PrintStream out) {
        int result = 2; /* STATUS_Success*/

        try {
            StringVariableOfInterfaceTest_1();
        } catch (Exception e) {
            processResult -= 10;
        }
        if (result == 2 && processResult == 99) {
            result = 0;
        }
        return result;
    }
    static String test(String str) {
        String result = str + str;
        System.out.println(result);
        return result;
    }
    public static void StringVariableOfInterfaceTest_1() {
        String str = null;
        System.out.println(str);
        System.out.println(StringVariableOfInterfaceTest_1.str1_1);
        str = "abc";
        System.out.println(StringVariableOfInterfaceTest.test(str));
    }
}
interface StringVariableOfInterfaceTest_1 {
    static String str1_1 = "abc1_1";
    static void test(String str) {
    }
}
