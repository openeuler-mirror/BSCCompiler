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
public class StringToHexStringCharTest {
    private static int processResult = 99;
    public static void main(String[] argv) {
        System.out.println(run(argv, System.out));
    }
    public static int run(String[] argv, PrintStream out) {
        int result = 2; /* STATUS_Success*/

        try {
            StringToHexStringCharTest_1();
        } catch (Exception e) {
            processResult -= 10;
        }
        if (result == 2 && processResult == 99) {
            result = 0;
        }
        return result;
    }
    public static void StringToHexStringCharTest_1() {
        // 0x00~0x7F.
        String str1_1 = "a b^c~AB";
        //10
        char data1_2[] = {97, 32, 98, 94, 99, 126, 65, 66};
        String str1_2 = new String(data1_2);
        //16
        char data1_3[] = {0x61, 0x20, 0x62, 0x5E, 0x63, 0x7E, 0x41, 0x42};
        String str1_3 = new        String(data1_3);
        String str2_1 = "中国";
        // Test unicode
        char data2_2[] = {'\u4E2D', '\u56FD'};
        String str2_2 = new String(data2_2);
        String str3_1 = "中国abc";
        //unicode
        char data3_2[] = {'\u4E2D', '\u56FD', 97, 98, 99};
        String str3_2 = new String(data3_2);
        test1(str1_1);
        test1(str1_2);
        test1(str1_3);
        test1(str2_1);
        test1(str2_2);
        test1(str3_1);
        test1(str3_2);
    }
    private static void test1(String str) {
        for (int i = 0; i < str.length(); i++) {
            char ch = str.charAt(i);
            System.out.println(Integer.toHexString(ch));
        }
    }
}
