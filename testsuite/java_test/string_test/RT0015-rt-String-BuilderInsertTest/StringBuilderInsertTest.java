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
public class StringBuilderInsertTest {
    private static int processResult = 99;
    public static void main(String[] argv) {
        System.out.println(run(argv, System.out));
    }
    public static int run(String[] argv, PrintStream out) {
        int result = 2; /* STATUS_Success*/

        try {
            StringBuilderInsertTest_1();
        } catch (Exception e) {
            processResult -= 10;
        }
        if (result == 2 && processResult == 99) {
            result = 0;
        }
        return result;
    }
    public static void StringBuilderInsertTest_1() {
        StringBuilder strBuilder1_1 = new StringBuilder("qwertyuiop{}[]\\|asdfghjkl;:'\"zxcvbnm,.<>/?~`1234567890" +
                "-=!@#$%^&*()_+ ASDFGHJKLQWERTYUIOPZXCVBNM0x96");
        StringBuilder strBuilder1_2 = new StringBuilder(" @!.&%()*");
        StringBuilder strBuilder1_3 = new StringBuilder("abc123abc");
        StringBuilder strBuilder1_4 = new StringBuilder("");
        StringBuilder strBuilder1_5 = new StringBuilder();
        test1(strBuilder1_1);
        test1(strBuilder1_2);
        test1(strBuilder1_3);
        test1(strBuilder1_4);
        test1(strBuilder1_5);
    }
    private static void test1(StringBuilder strBuilder) {
        char c = 'C';
        char[] data = {'A', 'B', 'C'};
        CharSequence chs1_1 = "xyz";
        double d = 8888.8888;
        double dMin = 0;
        float f = 99999999;
        float fMin = 0;
        int i = 77777777;
        int iMin = 0;
        long lng = 66666666;
        long lngMin = 0;
        Object obj = new String("object");
        String str = "-";
        System.out.println(strBuilder.insert(0, str).insert(1, true).insert(strBuilder.length(), str)
                .insert(strBuilder.length(), true).insert(0, str).insert(0, false).insert(strBuilder.length(), str)
                .insert(strBuilder.length(), false).insert(0, str).insert(0, c).insert(strBuilder.length(), str)
                .insert(strBuilder.length(), c).insert(0, str).insert(0, data).insert(strBuilder.length(), str)
                .insert(strBuilder.length(), data).insert(0, str).insert(0, chs1_1).insert(strBuilder.length(), str)
                .insert(strBuilder.length(), chs1_1).insert(0, str).insert(0, chs1_1, 1, 2)
                .insert(strBuilder.length(), str).insert(strBuilder.length(), chs1_1, 1, 2).insert(0, str)
                .insert(0, chs1_1, 0, 3).insert(strBuilder.length(), str).insert(strBuilder.length(), chs1_1, 0, 3)
                .insert(0, str).insert(0, chs1_1, 3, 3).insert(strBuilder.length(), str)
                .insert(strBuilder.length(), chs1_1, 3, 3).insert(0, str).insert(0, chs1_1, 0, 0)
                .insert(strBuilder.length(), str).insert(strBuilder.length(), chs1_1, 0, 0).insert(0, str).insert(0, d)
                .insert(strBuilder.length(), str).insert(strBuilder.length(), d).insert(0, str).insert(0, dMin)
                .insert(0, str).insert(0, f).insert(strBuilder.length(), str).insert(strBuilder.length(), f)
                .insert(0, str).insert(0, fMin).insert(0, str).insert(0, i).insert(strBuilder.length(), str)
                .insert(strBuilder.length(), i).insert(0, str).insert(0, iMin).insert(0, str).insert(0, lng)
                .insert(strBuilder.length(), str).insert(strBuilder.length(), lng).insert(0, str).insert(0, lngMin)
                .insert(0, str).insert(0, obj).insert(strBuilder.length(), str).insert(strBuilder.length(), obj));
        System.out.println(strBuilder.subSequence(2, 8));
    }
}
