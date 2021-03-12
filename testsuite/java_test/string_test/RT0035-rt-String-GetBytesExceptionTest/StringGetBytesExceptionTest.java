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
public class StringGetBytesExceptionTest {
    private static int processResult = 99;
    public static void main(String[] argv) {
        System.out.println(run(argv, System.out));
    }
    public static int run(String[] argv, PrintStream out) {
        int result = 2; /* STATUS_Success*/

        try {
            StringGetBytesExceptionTest_1();
        } catch (Exception e) {
            processResult -= 10;
        }
        if (result == 2 && processResult == 99) {
            result = 0;
        }
        return result;
    }
    public static void StringGetBytesExceptionTest_1() {
        String str1_1 = new String("qwertyuiop{}[]\\|asdfghjkl;:'\"zxcvbnm,.<>/?~`1234567890-=!" +
                "@#$%^&*()_+ ASDFGHJKLQWERTYUIOPZXCVBNM0x96");
        String str1_2 = new String(" @!.&%");
        String str1_3 = new String("abc123");
        String str1_4 = new String("");
        String str1_5 = new String();
        String str2_1 = "qwertyuiop{}[]\\|asdfghjkl;:'\"zxcvbnm,.<>/?~`1234567890-=!" +
                "@#$%^&*()_+ ASDFGHJKLQWERTYUIOPZXCVBNM0x96";
        String str2_2 = " @!.&%";
        String str2_3 = "abc123";
        test(str1_1);
        test(str1_2);
        test(str1_3);
        test(str1_4);
        test(str1_5);
        test(str2_1);
        test(str2_2);
        test(str2_3);
    }
    private static void test(String str) {
        byte[] dst1_1 = null;
        try {
            str.getBytes(2, 5, dst1_1, 2);
            String str_new = new String(dst1_1);
        } catch (NullPointerException e1) {
            System.out.println("EXCEPTION 1_1");
        } catch (StringIndexOutOfBoundsException e1) {
            System.out.println("EXCEPTION 1_2");
        }
        byte[] dst1_2 = {0x41, 0x42, 0x43, 0x44, 0x45, 0x46};
        try {
            str.getBytes(-1, 5, dst1_2, 2);
            String str_new = new String(dst1_2);
        } catch (StringIndexOutOfBoundsException e1) {
            System.out.println("EXCEPTION 2_1");
        }
        // if (srcEnd > length()) ...throw new StringIndexOutOfBoundsException(this, srcEnd);
        byte[] dst1_3 = {0x41, 0x42, 0x43, 0x44, 0x45, 0x46};
        try {
            str.getBytes(2, 6, dst1_3, 2);
            String str_new = new String(dst1_3);
        } catch (ArrayIndexOutOfBoundsException e1) {
            System.out.println("EXCEPTION 3_1");
        } catch (StringIndexOutOfBoundsException e1) {
            System.out.println("EXCEPTION 3_2");
        }
        byte[] dst1_4 = {0x41, 0x42, 0x43, 0x44, 0x45, 0x46};
        try {
            str.getBytes(4, 3, dst1_4, 2);
            String str_new = new String(dst1_4);
        } catch (StringIndexOutOfBoundsException e1) {
            System.out.println("EXCEPTION 4_1");
        }
        byte[] dst1_5 = {0x41, 0x42, 0x43, 0x44, 0x45, 0x46};
        try {
            str.getBytes(4, 3, dst1_5, -1);
            String str_new = new String(dst1_5);
        } catch (StringIndexOutOfBoundsException e1) {
            System.out.println("EXCEPTION 5_1");
        }
    }
}