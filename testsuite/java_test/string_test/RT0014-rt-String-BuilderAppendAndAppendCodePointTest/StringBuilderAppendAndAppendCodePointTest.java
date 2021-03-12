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
public class StringBuilderAppendAndAppendCodePointTest {
    private static int processResult = 99;
    public static void main(String[] argv) {
        System.out.println(run(argv, System.out));
    }
    public static int run(String[] argv, PrintStream out) {
        int result = 2; /* STATUS_Success*/

        try {
            StringBuilderAppendAndAppendCodePointTest_1();
        } catch (Exception e) {
            processResult -= 10;
        }
        if (result == 2 && processResult == 99) {
            result = 0;
        }
        return result;
    }
    public static void StringBuilderAppendAndAppendCodePointTest_1() {
        StringBuilder strBuilder1_1 = new StringBuilder("abc123abc");
        StringBuilder strBuilder1_2 = new StringBuilder("      ");
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
        Object obj = new String("object");
        String str = "string";
        double d = 8888.8888;
        double dMin = 0;
        float f = 99999999;
        float fMin = 0;
        int i = 77777777;
        int iMin = 0;
        long lng = 66666666;
        long lngMin = 0;
        StringBuilder sb = new StringBuilder("StringBuilder");
        int codePoint = 74; // unicode 74 is J
        System.out.println(strBuilder.append("-").append(true).append("-").append(false).append("-").append(c)
                .append("-").append(data).append("-").append(chs1_1).append("-").append(chs1_1, 1, 2).append("-")
                .append(chs1_1, 0, 3).append("-").append(chs1_1, 3, 3).append("-").append(chs1_1, 0, 0).append("-")
                .append(d).append("-").append(dMin).append("-").append(f).append("-").append(fMin).append("-").append(i)
                .append("-").append(iMin).append("-").append(lng).append("-").append(lngMin).append("-").append(obj)
                .append("-").append(str).append("-").append(sb).append("-").appendCodePoint(codePoint));
        System.out.println(strBuilder.subSequence(2, 8));
    }
}
