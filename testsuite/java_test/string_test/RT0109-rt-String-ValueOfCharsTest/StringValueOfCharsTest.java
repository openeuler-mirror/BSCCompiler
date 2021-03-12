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
public class StringValueOfCharsTest {
    private static int processResult = 99;
    public static void main(String[] argv) {
        System.out.println(run(argv, System.out));
    }
    public static int run(String[] argv, PrintStream out) {
        int result = 2; /* STATUS_Success*/

        try {
            StringValueOfCharsTest_1();
        } catch (Exception e) {
            processResult -= 10;
        }
        if (result == 2 && processResult == 99) {
            result = 0;
        }
        return result;
    }
    public static void StringValueOfCharsTest_1() {
        char[] ch1_1 = {'a', 'b', 'c', '1', '2', '3'};
        char[] ch1_2 = {0x61, 0x62, 0x63, 0x31, 0x32, 0x33};
        char[] ch1_3 = {'\u0061', '\u0062', '\u0063', '\u0031', '\u0032', '\u0033'};
        char[] ch1_4 = {0x0061, 0x0062, 0x0063, 0x0031, 0x0032, 0x0033};
        char[] ch1_5 = {};
        test(ch1_1);
        test(ch1_2);
        test(ch1_3);
        test(ch1_4);
        test(ch1_5);
    }
    private static void test(char[] ch1_1) {
        System.out.println(String.valueOf(ch1_1));
    }
}
