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
public class StringBufferInsertTest {
    private static int processResult = 99;
    public static void main(String[] argv) {
        System.out.println(run(argv, System.out));
    }
    public static int run(String[] argv, PrintStream out) {
        int result = 2; /* STATUS_Success*/

        try {
            StringBufferInsertTest_1();
        } catch (Exception e) {
            processResult -= 10;
        }
        if (result == 2 && processResult == 99) {
            result = 0;
        }
        return result;
    }
    public static void StringBufferInsertTest_1() {
        StringBuffer strBuffer1_1 = new StringBuffer("qwertyuiop{}[]\\|asdfghjkl;:'\"zxcvbnm,.<>/?~`1234567890-=!" +
                "@#$%^&*()_+ ASDFGHJKLQWERTYUIOPZXCVBNM0x96");
        StringBuffer strBuffer1_2 = new StringBuffer(" @!.&%()*");
        StringBuffer strBuffer1_3 = new StringBuffer("abc123abc");
        StringBuffer strBuffer1_4 = new StringBuffer("");
        StringBuffer strBuffer1_5 = new StringBuffer();
        test1(strBuffer1_1);
        test1(strBuffer1_2);
        test1(strBuffer1_3);
        test1(strBuffer1_4);
        test1(strBuffer1_5);
    }
    private static void test1(StringBuffer strBuffer) {
        boolean b = true;
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
        System.out.println(strBuffer.insert(0, str).insert(0, b).insert(strBuffer.length(), str)
                .insert(strBuffer.length(), b).insert(0, str).insert(0, false).insert(strBuffer.length(), str)
                .insert(strBuffer.length(), false).insert(0, str).insert(0, c).insert(strBuffer.length(), str)
                .insert(strBuffer.length(), c).insert(0, str).insert(0, data).insert(strBuffer.length(), str)
                .insert(strBuffer.length(), data).insert(0, str).insert(0, chs1_1).insert(strBuffer.length(), str)
                .insert(strBuffer.length(), chs1_1).insert(0, str).insert(0, chs1_1, 1, 2)
                .insert(strBuffer.length(), str).insert(strBuffer.length(), chs1_1, 1, 2).insert(0, str)
                .insert(0, chs1_1, 0, 3).insert(strBuffer.length(), str).insert(strBuffer.length(), chs1_1, 0, 3)
                .insert(0, str).insert(0, chs1_1, 3, 3).insert(strBuffer.length(), str)
                .insert(strBuffer.length(), chs1_1, 3, 3).insert(0, str).insert(0, chs1_1, 0, 0)
                .insert(strBuffer.length(), str).insert(strBuffer.length(), chs1_1, 0, 0).insert(0, str).insert(0, d)
                .insert(strBuffer.length(), str).insert(strBuffer.length(), d).insert(0, str).insert(0, dMin)
                .insert(0, str).insert(0, f).insert(strBuffer.length(), str).insert(strBuffer.length(), f)
                .insert(0, str).insert(0, fMin).insert(0, str).insert(0, i).insert(strBuffer.length(), str)
                .insert(strBuffer.length(), i).insert(0, str).insert(0, iMin).insert(0, str).insert(0, lng)
                .insert(strBuffer.length(), str).insert(strBuffer.length(), lng).insert(0, str)
                .insert(0, lngMin).insert(0, str).insert(0, obj).insert(strBuffer.length(), str)
                .insert(strBuffer.length(), obj));
        System.out.println(strBuffer.subSequence(2, 8));
    }
}
