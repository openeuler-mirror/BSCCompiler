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
public class StringConsIntArrayExceptionsTest {
    private static int processResult = 99;
    public static void main(String[] argv) {
        System.out.println(run(argv, System.out));
    }
    public static int run(String[] argv,  PrintStream out) {
        int result = 2; /* STATUS_Success*/

        try {
            result = StringConsIntArrayExceptionsTest_1();
        } catch (Exception e) {
            processResult -= 10;
        }
        if (result == 2 && processResult == 96) {
            result = 0;
        }
        return result;
    }
    public static int StringConsIntArrayExceptionsTest_1() {
        int result1 = 2; /* STATUS_Success*/

        // IndexOutOfBoundsException - If the offset and count arguments index characters outside the bounds of the
        // codePoints array
        int[] str1_1 = new int[]{97, 98, 99, 49, 50, 51};
        try {
            String str1 = new String(str1_1, -1, str1_1.length);
            processResult -= 10;
        } catch (IndexOutOfBoundsException e1) {
            processResult--;
        }
        try {
            String str1 = new String(str1_1, 0, 10);
            processResult -= 10;
        } catch (IndexOutOfBoundsException e1) {
            processResult--;
        }
        //IllegalArgumentException - If any invalid Unicode code point is found in codePoints
        int[] str1_2 = new int[]{0xFFFFFFF, 0x62, 0x63, 0x31, 0x32, 0x33};
        try {
            String str2 = new String(str1_2, 0, str1_2.length);
            processResult -= 10;
        } catch (IllegalArgumentException e1) {
            processResult--;
        }
        return result1;
    }
}
