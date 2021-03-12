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
public class StringBufferExceptionsTest {
    private static int processResult = 99;
    public static void main(String[] argv) {
        System.out.println(run(argv, System.out));
    }
    public static int run(String[] argv, PrintStream out) {
        int result = 2;  /* STATUS_Success*/

        try {
            result = StringBufferGetCharsIndexOutOfBoundsException_1();
        } catch (Exception e) {
            processResult -= 10;
        }
        try {
            result = StringBufferReplaceStringIndexOutOfBoundsException_2();
        } catch (Exception e) {
            processResult -= 10;
        }
        try {
            result = StringBufferSubstringStringIndexOutOfBoundsException_1();
        } catch (Exception e) {
            processResult -= 10;
        }
        try {
            result = StringBufferSubstringStringIndexOutOfBoundsException_2();
        } catch (Exception e) {
            processResult -= 10;
        }
        if (result == 2 && processResult == 80) {
            result = 0;
        }
        return result;
    }
    public static int StringBufferGetCharsIndexOutOfBoundsException_1() {
        int result1 = 3; /*STATUS_FAILED*/
        // IndexOutOfBoundsException- If either off or len is negative, or if off + len is greater than b.length.
        // Test public void getChars(int srcBegin, int srcEnd, char[] dst, int dstBegin).
        StringBuffer buff = new StringBuffer("java programming");
        char[] chArr = new char[]{'t', 'u', 't', 'o', 'r', 'i', 'a', 'l', 's'};
        // Test dstBegin > dst.length.
        try {
            buff.getChars(5, 10, chArr, 30);
            processResult -= 10;
        } catch (IndexOutOfBoundsException e1) {
            processResult--;
        }
        // Test dstBegin < 0.
        try {
            buff.getChars(5, 10, chArr, -2);
            processResult -= 10;
        } catch (IndexOutOfBoundsException e1) {
            processResult--;
        }
        // Test dst.length - dstBegin < srcEnd - srcBegin.
        try {
            buff.getChars(5, 10, chArr, 7);
            processResult -= 10;
        } catch (IndexOutOfBoundsException e1) {
            processResult--;
        }
        // Test srcBegin < 0.
        try {
            buff.getChars(-2, 10, chArr, 5);
            processResult -= 10;
        } catch (IndexOutOfBoundsException e1) {
            processResult--;
        }
        // Test srcBegin > buff.length.
        try {
            buff.getChars(buff.length(), 10, chArr, 5);
            processResult -= 10;
        } catch (IndexOutOfBoundsException e1) {
            processResult--;
        }
        // Test srcEnd < 0.
        try {
            buff.getChars(5, -2, chArr, 5);
            processResult -= 10;
        } catch (IndexOutOfBoundsException e1) {
            processResult--;
        }
        // Test srcEnd < srcBegin.
        try {
            buff.getChars(5, 2, chArr, 5);
            processResult -= 10;
        } catch (IndexOutOfBoundsException e1) {
            processResult--;
        }
        // Test srcEnd > buff.length.
        try {
            buff.getChars(5, buff.length(), chArr, 5);
            processResult -= 10;
        } catch (IndexOutOfBoundsException e1) {
            processResult--;
        }
        return result1;
    }
    public static int StringBufferReplaceStringIndexOutOfBoundsException_2() {
        int result1 = 3; /*STATUS_FAILED*/
        // StringIndexOutOfBoundsException- Access parameter range of index.
        // Test public StringBuffer replace(int start, int end, String str).
        StringBuffer buff1 = new StringBuffer("HelloWorld");
        // Test start > buff1.length.
        try {
            buff1.replace(20, 6, "QQQ");
            processResult -= 10;
        } catch (StringIndexOutOfBoundsException e1) {
            processResult--;
        }
        // Test start < 0.
        try {
            buff1.replace(-2, 6, "QQQ");
            processResult -= 10;
        } catch (StringIndexOutOfBoundsException e1) {
            processResult--;
        }
        // Test start > end.
        try {
            buff1.replace(7, 6, "QQQ");
            processResult -= 10;
        } catch (StringIndexOutOfBoundsException e1) {
            processResult--;
        }
        // Test end < 0.
        try {
            buff1.replace(7, -2, "QQQ");
            processResult -= 10;
        } catch (StringIndexOutOfBoundsException e1) {
            processResult--;
        }
        return result1;
    }
    public static int StringBufferSubstringStringIndexOutOfBoundsException_1() {
        int result1 = 3; /*STATUS_FAILED*/
        // StringIndexOutOfBoundsException- Intercept character string specifying scope, exceed the range error.
        // Test public String substring(int start, int end).
        StringBuffer buff2 = new StringBuffer("HelloWorld");
        // Test end > buff2.length.
        try {
            String str = buff2.substring(9, 11);
            System.out.println(str);
            processResult -= 10;
        } catch (StringIndexOutOfBoundsException e1) {
            processResult--;
        }
        // Test end < 0.
        try {
            String str = buff2.substring(9, 11);
            System.out.println(str);
            processResult -= 10;
        } catch (StringIndexOutOfBoundsException e1) {
            processResult--;
        }
        // Test start > buff2.length.
        try {
            String str = buff2.substring(11, 5);
            System.out.println(str);
            processResult -= 10;
        } catch (StringIndexOutOfBoundsException e1) {
            processResult--;
        }
        // Test start > end.
        try {
            String str = buff2.substring(8, 5);
            System.out.println(str);
            processResult -= 10;
        } catch (StringIndexOutOfBoundsException e1) {
            processResult--;
        }
        // Test start < 0.
        try {
            String str = buff2.substring(-2, 5);
            System.out.println(str);
            processResult -= 10;
        } catch (StringIndexOutOfBoundsException e1) {
            processResult--;
        }
        return result1;
    }
    public static int StringBufferSubstringStringIndexOutOfBoundsException_2() {
        int result1 = 3; /*STATUS_FAILED*/
        // StringIndexOutOfBoundsException- Intercept character string specifying scope, exceed the range error.
        // Test public String substring(int start).
        StringBuffer buff3 = new StringBuffer("HelloWorld");
        // Test start > buff3.length.
        try {
            String str = buff3.substring(15);
            System.out.println(str);
            processResult -= 10;
        } catch (StringIndexOutOfBoundsException e1) {
            processResult--;
        }
        // Test start < 0.
        try {
            String str = buff3.substring(-2);
            System.out.println(str);
            processResult -= 10;
        } catch (StringIndexOutOfBoundsException e1) {
            result1 = 2;
            processResult--;
        }
        return result1;
    }
}