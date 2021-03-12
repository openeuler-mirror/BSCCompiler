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
public class StringUnCompressTest {
    private static int processResult = 99;
    public static void main(String[] argv) {
        System.out.println(run(argv, System.out));
    }
    public static int run(String[] argv, PrintStream out) {
        int result = 2; /* STATUS_Success*/

        try {
            StringUnCompressTest_1();
        } catch (Exception e) {
            processResult -= 10;
        }
        if (result == 2 && processResult == 99) {
            result = 0;
        }
        return result;
    }
    public static void StringUnCompressTest_1() {
        String str = "哈";
        byte[] strbyte = str.getBytes();
        String res1_1 = new String(strbyte);
        System.out.println(res1_1.equals(str)); // true
        String str1 = "abc";
        String str2 = "abc哈";
        String res2_1 = str1.replace("a", "哈"); // 哈bc
        String res2_2 = str2.replace("a", "哈"); // 哈bc哈
        String res2_3 = str2.replace("哈", "a"); // abca
        String res2_4 = str2.replace("a", "d"); // dbc哈
        String res2_5 = str2.replace("哈", "中"); // abc中
        System.out.println(res2_1.equals("哈bc"));
        System.out.println(res2_2.equals("哈bc哈"));
        System.out.println(res2_3.equals("abca"));
        System.out.println(res2_4.equals("dbc哈"));
        System.out.println(res2_5.equals("abc中"));
        String str3 = "abc中"; // not compress
        String res3_1 = str1.concat(str2);
        String res3_2 = str2.concat(str1);
        String res3_3 = str2.concat(str3);
        System.out.println(res3_1.equals("abcabc哈"));
        System.out.println(res3_2.equals("abc哈abc"));
        System.out.println(res3_3.equals("abc哈abc中"));
        int res4_1 = str3.indexOf("中");
        int res4_2 = str3.indexOf("a");
        System.out.println(res4_1);
        System.out.println(res4_2);
    }
}
